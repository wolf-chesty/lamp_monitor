// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ApplicationParameters.hpp"
#include "LampFieldMonitor.hpp"
#include "NightLampMonitor.hpp"
#include "ParkedCallMonitor.hpp"
#include "phonebook/PjsipWizardAdapter.hpp"
#include "xml/yealink/CallParkMenu.hpp"
#include "xml/yealink/Phonebook.hpp"
#include <argparse/argparse.hpp>
#include <atomic>
#include <c++ami/action/Login.hpp>
#include <c++ami/action/Logoff.hpp>
#include <c++ami/action/Ping.hpp>
#include <c++ami/Connection.hpp>
#include <filesystem>
#include <httplib.h>
#include <inicpp.h>
#include <memory>
#include <string_view>
#include <unistd.h>

// Function declarations ===============================================================================================

void signalHandler(int signum);

ApplicationParameters getApplicationParameters(int argc, char *argv[]);

void login(std::shared_ptr<cpp_ami::Connection> const &ami_conn, ini::IniFile &cfg_ini, bool is_daemon);

void serviceThread(ini::IniFile &cfg_ini, std::shared_ptr<cpp_ami::Connection> const &io_conn);

// Globals =============================================================================================================

std::unique_ptr<httplib::Server> g_server;

int main(int argc, char *argv[])
{
    auto const args = getApplicationParameters(argc, argv);

    ini::IniFile ami_ini;
    ami_ini.load(args.config_file);

    if (args.is_daemon) {
        if (daemon(0, 0) == -1) {
            exit(EXIT_FAILURE);
        }
    }

    // Create AMI connection
    auto const ami_hostname = ami_ini["ami"]["host"].as<std::string>();
    auto const ami_port = ami_ini["ami"]["port"].as<uint16_t>();
    auto io_conn = std::make_shared<cpp_ami::Connection>(ami_hostname, ami_port);

    login(io_conn, ami_ini, args.is_daemon);

    serviceThread(ami_ini, io_conn);

    // Logout of AMI; wait for the response before terminating
    cpp_ami::action::Logoff const logoff;
    auto const reaction = io_conn->invoke(logoff);

    return EXIT_SUCCESS;
}

/// @brief Handles termination events sent to the application.
///
/// @param signum Signal sent to the application.
///
/// The application is waiting for the HTTP server to spin down before continuing on. When the application encounters a
/// termination signal it will tell the HTTP server to stop, allowing the application to continue on and stop.
void signalHandler(int signum)
{
    if (g_server && signum == SIGINT) {
        g_server->stop();
    }
}

/// @brief Gets application parameters.
///
/// @param argc Number of application parameters.
/// @param argv Application parameters
///
/// @return Application parameters structure.
ApplicationParameters getApplicationParameters(int argc, char *argv[])
{
    argparse::ArgumentParser parser("lamp_monitor");
    parser.add_argument("-d", "--daemon").default_value(false).implicit_value(true).nargs(0);
    parser.add_argument("config_ini").help("Application config file").required();

    try {
        parser.parse_args(argc, argv);
    }
    catch (std::runtime_error const &err) {
        std::cerr << err.what() << '\r' << parser;
        exit(EXIT_FAILURE);
    }

    return ApplicationParameters{parser.get<std::string>("config_ini"), parser.get<bool>("--daemon")};
}

/// @brief Logs the application into the Asterisk server.
///
/// @param conn Pointer to Asterisk AMI connection.
/// @param cfg_ini INI configuration object.
/// @param is_daemon Boolean flag indicating application is a running in daemon mode.
void login(std::shared_ptr<cpp_ami::Connection> const &conn, ini::IniFile &cfg_ini, bool is_daemon)
{
    // Create AMI Login action
    auto const ami_username = cfg_ini["ami"]["username"].as<std::string>();
    auto const ami_password = cfg_ini["ami"]["secret"].as<std::string>();
    cpp_ami::action::Login login(ami_username, ami_password);
    login["AuthType"] = "plain";
    login["Events"] = "on";
    // Send login to AMI
    auto const reaction = conn->invoke(login);
    // Confirm login
    if (!reaction->isSuccess()) {
        if (!is_daemon) {
            std::cerr << "Unable to login to AMI";
        }
        exit(EXIT_FAILURE);
    }
}

/// @brief Create a thread that will ping the AMI server.
///
/// @param ami_conn AMI connection.
/// @param cv Condition variable used to prematurely awaken the thread.
/// @param run_thread_flag Flag used to stop the thread execution.
/// @param period Period to sleep between pings.
///
/// @return \c thread object that holds a handle to the newly created thread.
///
/// The Asterisk Management Interface (AMI) server will periodically cull connections that are believed to be dead. In
/// order to avoid having the connection to the AMI server closed it is recommended for an application to start a thread
/// that will periodically send a Ping action to the AMI server. This function creates that thread.
std::thread createAmiPingThread(std::shared_ptr<cpp_ami::Connection> const &ami_conn, std::condition_variable &cv,
                                std::atomic<bool> &run_thread_flag, std::chrono::milliseconds period)
{
    std::thread work_thread([&cv, &run_thread_flag, ami_conn, period, ping = cpp_ami::action::Ping()]() -> void {
        do {
            // Send ping to the AMI server, waiting for the result
            auto const r = ami_conn->invoke(ping);

            // Wait for the application to terminate the ping thread
            std::mutex mut;
            std::unique_lock lock(mut);
            cv.wait_for(lock, period, [&run_thread_flag]() -> bool { return !run_thread_flag; });
        } while (run_thread_flag);
    });

    pthread_setname_np(work_thread.native_handle(), "ping_ami");

    return work_thread;
}

/// @brief Craetes lamp field monitor object.
///
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param io_conn Pointer to Asterisk AMI connection.
/// @param cfg_ini INI configuration object.
/// @param lamp_monitors Collection of lamp monitors that make up the lamp field.
void createLampFieldMonitor(std::unique_ptr<httplib::Server> const &http_server,
                            std::shared_ptr<cpp_ami::Connection> const &io_conn, ini::IniFile &cfg_ini,
                            std::list<std::shared_ptr<LampMonitor>> const &lamp_monitors)
{
    // Create deskphone cache
    auto const db_filename = cfg_ini["handset_cache"]["filename"].as<std::string>();
    auto const db_expiry = cfg_ini["handset_cache"]["expiry"].as<uint32_t>();
    auto deskphone_cache = std::make_unique<DeskphoneCache>(db_filename, std::chrono::seconds(db_expiry));
    // Create lamp field monitor
    auto lamp_field_monitor = std::make_shared<LampFieldMonitor>(std::move(deskphone_cache), io_conn);
    lamp_field_monitor->addLamps(lamp_monitors);

    // Bind URI to get current lamp settings
    auto const blf_state_uri = cfg_ini["http_server"]["blf_state_uri"].as<std::string>();
    http_server->Get(blf_state_uri, [lamp_field_monitor](httplib::Request const &req, httplib::Response &res) -> void {
        // Remove requesting deskphone from cache
        if (req.has_param("aor") && req.has_param("ip")) {
            lamp_field_monitor->invalidateAOR(req.get_param_value("aor"), req.get_param_value("ip"));
        }

        // Publish current lamp field button state
        auto const state = lamp_field_monitor->getCachedButtonState();
        res.set_content(state->getXMLString(), "text/xml");
    });
}

/// @brief Creates night lamp monitor object.
///
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param io_conn Pointer to Asterisk AMI connection.
/// @param cfg_ini INI configuration object.
///
/// @return Pointer to night lamp monitor object.
std::shared_ptr<LampMonitor> createNightLampMonitor(std::unique_ptr<httplib::Server> const &http_server,
                                                    std::shared_ptr<cpp_ami::Connection> const &io_conn,
                                                    ini::IniFile &cfg_ini)
{
    auto const night_button_id = cfg_ini["night_button"]["button_id"].as<unsigned int>();
    auto const night_exten = cfg_ini["night_button"]["exten"].as<std::string>();
    auto const night_context = cfg_ini["night_button"]["context"].as<std::string>();
    auto const night_device = cfg_ini["night_button"]["device"].as<std::string>();
    auto night_lamp_monitor =
        std::make_shared<NightLampMonitor>(io_conn, night_button_id, night_exten, night_context, night_device);

    // Bind URI to change night mode
    auto const night_uri = cfg_ini["http_server"]["night_uri"].as<std::string>();
    http_server->Get(
        night_uri, [night_lamp_monitor]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
            res.set_content(night_lamp_monitor->resetNightState(), "text/xml");
        });

    return night_lamp_monitor;
}

/// @brief Creates park lamp monitor object.
///
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param io_conn Pointer to Asterisk AMI connection.
/// @param cfg_ini INI configuration object.
///
/// @return Pointer to call park lamp monitor object.
std::shared_ptr<LampMonitor> createParkLampMonitor(std::unique_ptr<httplib::Server> const &http_server,
                                                   std::shared_ptr<cpp_ami::Connection> const &io_conn,
                                                   ini::IniFile &cfg_ini)
{
    auto const park_button_id = cfg_ini["park_button"]["button_id"].as<unsigned int>();
    auto const http_url = cfg_ini["http_server"]["url"].as<std::string>();
    auto const park_info_uri = cfg_ini["http_server"]["park_info_uri"].as<std::string>();
    auto park_lamp_monitor = std::make_shared<ParkedCallMonitor>(io_conn, park_button_id);
    auto parked_call_menu = std::make_shared<xml::yealink::CallParkMenu>(io_conn, http_url + park_info_uri);

    // Bind URI to get list of parked calls
    auto const park_list_uri = cfg_ini["http_server"]["park_list_uri"].as<std::string>();
    http_server->Get(park_list_uri,
                     [parked_call_menu]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
                         res.set_content(parked_call_menu->getParkedCallMenu(), "text/xml");
                     });

    // Bind URI to get parked call info
    http_server->Get(park_info_uri, [parked_call_menu](httplib::Request const &req, httplib::Response &res) -> void {
        if (req.has_param("selection")) {
            res.set_content(parked_call_menu->getParkedCallDetails(req.get_param_value("selection")), "text/xml");
            return;
        }

        static auto const error_xml = parked_call_menu->createMessageXML(true, 5, "Missing Extension Parameter",
                                                                         "Missing URL parameter '&selection=xxx'.");
        res.set_content(error_xml, "text/xml");
    });

    return park_lamp_monitor;
}

/// @brief Creates and configures the phonebook service for the application.
///
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param io_conn Asterisk AMI connection.
/// @param cfg_ini INI configuration object.
void configurePhonebookService(std::unique_ptr<httplib::Server> const &http_server,
                               std::shared_ptr<cpp_ami::Connection> const &io_conn, ini::IniFile &cfg_ini)
{
    auto phonebook_provider = std::make_shared<phonebook::PJSIPWizardAdapter>(io_conn, "local-phone");

    // Setup Yealink phonebook URI
    auto const yealink_phonebook_uri = cfg_ini["http_server"]["phonebook_uri"].as<std::string>();
    auto yealink_phonebook = std::make_shared<xml::yealink::Phonebook>(phonebook_provider, std::chrono::minutes(60));
    http_server->Get(yealink_phonebook_uri,
                     [yealink_phonebook](httplib::Request const &req, httplib::Response &res) -> void {
                         res.set_content(yealink_phonebook->getPhonebookXML(), "text/xml");
                     });
}

/// @brief Main service thread.
///
/// @param cfg_ini Configuration file.
/// @param io_conn Asterisk AMI connection.
void serviceThread(ini::IniFile &cfg_ini, std::shared_ptr<cpp_ami::Connection> const &io_conn)
{
    // Asterisk will drop AMI connections it hasn't heard from in a while; start an AMI ping thread
    std::atomic<bool> thread_run_flag{true};
    std::condition_variable cv;
    std::thread ami_ping_thread = createAmiPingThread(io_conn, cv, thread_run_flag, std::chrono::milliseconds(2500));

    g_server = std::make_unique<httplib::Server>();
    signal(SIGINT, signalHandler);

    // Create phonebook service
    configurePhonebookService(g_server, io_conn, cfg_ini);
    // Create monitor for the night button
    auto night_lamp_monitor = createNightLampMonitor(g_server, io_conn, cfg_ini);
    // Create monitor for the parking button
    auto park_lamp_monitor = createParkLampMonitor(g_server, io_conn, cfg_ini);
    // Create lamp field monitor
    createLampFieldMonitor(g_server, io_conn, cfg_ini, {night_lamp_monitor, park_lamp_monitor});

    // httplib::Server::listen is a blocking call
    auto const http_addr = cfg_ini["http_server"]["addr"].as<std::string>();
    auto const http_port = cfg_ini["http_server"]["port"].as<uint16_t>();
    g_server->listen(http_addr, http_port);

    // Stop ping thread
    thread_run_flag = false;
    cv.notify_all();
    ami_ping_thread.join();
}
