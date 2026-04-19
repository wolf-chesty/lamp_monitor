// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ApplicationParameters.hpp"
#include "asterisk/RegisterEventHandler.hpp"
#include "button_state/ButtonPlan.hpp"
#include "ConfigFileParser.hpp"
#include <argparse/argparse.hpp>
#include <atomic>
#include <c++ami/action/Login.hpp>
#include <c++ami/action/Logoff.hpp>
#include <c++ami/action/Ping.hpp>
#include <c++ami/Connection.hpp>
#include <filesystem>
#include <httplib.h>
#include <memory>
#include <string_view>
#include <syslog.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

// Globals =============================================================================================================

std::unique_ptr<httplib::Server> g_server;

/// @brief Handles termination events sent to the application.
///
/// @param signum Signal sent to the application.
///
/// The application is waiting for the HTTP server to spin down before continuing on. When the application encounters a
/// termination signal it will tell the HTTP server to stop, allowing the application to continue on and stop.
void signalHandler(int signum, [[maybe_unused]] siginfo_t *info, [[maybe_unused]] void *ptr)
{
    if (g_server && signum == SIGTERM) {
        syslog(LOG_DEBUG, "Stopping server");
        g_server->stop();
    }
}

// Function declarations ===============================================================================================

ApplicationParameters getApplicationParameters(int argc, char *argv[]);

std::thread createAmiPingThread(std::shared_ptr<cpp_ami::Connection> const &ami_conn, std::condition_variable &cv,
                                std::atomic<bool> &run_thread_flag, std::chrono::milliseconds period,
                                std::string_view thread_name);

int main(int argc, char *argv[])
{
    auto const args = getApplicationParameters(argc, argv);

    // Make sure that the config file exists
    if (!std::filesystem::exists(args.config_file)) {
        syslog(LOG_ERR, "Unable to find config file %s", args.config_file.c_str());
        exit(EXIT_FAILURE);
    }

    try {
        // Load config file
        auto const config_yaml = YAML::LoadFile(args.config_file);

        // Fork to background if requested
        if (args.is_daemon) {
            if (daemon(0, 0) == -1) {
                syslog(LOG_CRIT, "Unable to daemonize");
                exit(EXIT_FAILURE);
            }
        }

        configureSyslog(config_yaml["syslog"], "lamp_monitor", args.is_daemon);

        // Create HTTP server
        std::string http_addr;
        uint16_t http_port{};
        std::tie(g_server, http_addr, http_port) = createHTTPServer(config_yaml["http"]);

        // Register SIGTERM handler
        struct sigaction sa{};
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_sigaction = signalHandler;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGTERM, &sa, nullptr) < 0) {
            syslog(LOG_ERR, "Unable to register SIGTERM handler");
            exit(EXIT_FAILURE);
        }

        // Create AMI connection
        auto const io_conn = createAMIConnection(config_yaml["ami"]);
        // Ping AMI server so application doesn't get kicked
        std::atomic<bool> thread_run_flag{true};
        std::condition_variable cv;
        std::chrono::seconds const period{120};
        auto ping_thread = createAmiPingThread(io_conn, cv, thread_run_flag, period, "ping_thread");

        createPhonebooks(config_yaml["phonebooks"], *g_server, io_conn);

        auto const phone_register_handler = createRegisterEventHandler(config_yaml, *g_server, io_conn);

        // httplib::Server::listen is a blocking call
        g_server->listen(http_addr, http_port);

        // Stop and join on ping thread
        thread_run_flag = false;
        cv.notify_all();
        ping_thread.join();

        // Logout of AMI; wait for the response before terminating
        cpp_ami::action::Logoff const logoff;
        auto const reaction = io_conn->invoke(logoff);
    }
    catch (std::exception const &e) {
        syslog(LOG_CRIT, "Exception: %s", e.what());
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "Halting application");
    closelog();

    return EXIT_SUCCESS;

    /*

    int const options = args.is_daemon ? LOG_NDELAY : (LOG_CONS | LOG_NDELAY);
    openlog("lamp_monitor", options, LOG_USER);
    syslog(LOG_DEBUG, "Starting lamp_monitor");


    // Set log level
    auto const log_level = ami_ini["settings"]["log_level"].as<int>();
    setlogmask(LOG_UPTO(log_level));
    cpp_ami::util::ScopeGuard log_guard([]() -> void {
        syslog(LOG_DEBUG, "Stopping lamp_monitor");
        closelog();
    });



    // Create AMI connection
    auto const ami_hostname = ami_ini["ami"]["host"].as<std::string>();
    auto const ami_port = ami_ini["ami"]["port"].as<uint16_t>();
    auto io_conn = std::make_shared<cpp_ami::Connection>(ami_hostname, ami_port);

    login(io_conn, ami_ini, args.is_daemon);

    // Register SIGTERM handler
    struct sigaction sa{};
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, nullptr) < 0) {
        syslog(LOG_ERR, "Unable to registery SIGTERM handler");
        exit(EXIT_FAILURE);
    }

    serviceThread(ami_ini, io_conn);

    // Logout of AMI; wait for the response before terminating
    cpp_ami::action::Logoff const logoff;
    auto const reaction = io_conn->invoke(logoff);

    return EXIT_SUCCESS;
    */
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

/// @brief Create a thread that will ping the AMI server.
///
/// @param ami_conn AMI connection.
/// @param cv Condition variable used to prematurely awaken the thread.
/// @param run_thread_flag Flag used to stop the thread execution.
/// @param period Period to sleep between pings.
/// @param thread_name Thread name that should show up in top.
///
/// @return \c thread object that holds a handle to the newly created thread.
///
/// The Asterisk Management Interface (AMI) server will periodically cull connections that are believed to be dead. In
/// order to avoid having the connection to the AMI server closed it is recommended for an application to start a thread
/// that will periodically send a Ping action to the AMI server. This function creates that thread.
std::thread createAmiPingThread(std::shared_ptr<cpp_ami::Connection> const &ami_conn, std::condition_variable &cv,
                                std::atomic<bool> &run_thread_flag, std::chrono::milliseconds period,
                                std::string_view thread_name)
{
    std::thread work_thread([&cv, &run_thread_flag, ami_conn, period, ping = cpp_ami::action::Ping()]() -> void {
        syslog(LOG_DEBUG, "Starting ping thread");
        do {
            // Send ping to the AMI server, waiting for the result
            auto const r = ami_conn->invoke(ping);

            // Wait for the application to terminate the ping thread
            std::mutex mut;
            std::unique_lock lock(mut);
            cv.wait_for(lock, period, [&run_thread_flag]() -> bool { return !run_thread_flag; });
        } while (run_thread_flag);
    });

    pthread_setname_np(work_thread.native_handle(), thread_name.data());

    return work_thread;
}

/*
/// @brief Creates deskphone cache object.
///
/// @param cfg_ini Object containing configuration information.
///
/// @return Pointer to deskphone cache object.
std::shared_ptr<DeskphoneCache> createDeskphoneCache(ini::IniFile &cfg_ini)
{
    auto const db_filename = cfg_ini["handset_cache"]["filename"].as<std::string>();
    auto const db_expiry = cfg_ini["handset_cache"]["expiry"].as<uint32_t>();
    return std::make_shared<DeskphoneCache>(db_filename, std::chrono::seconds(db_expiry));
}

/// @brief Creates a phone UI and attaches it to the deskphone.
///
/// @param http_server HTTP server that will interact with the deskphone UI.
/// @param deskphone_cache Deskphone cache.
/// @param cfg_ini Object containing configuration parameters.
///
/// This function will create a phone UI and attach a configuration specified HTTP URI endpoint to it. This HTTP URI
/// will return the current state of the deskphone so that deskphones that have recently rebooted can query the current
/// deskphone state on boot.
///
/// This HTTP endpoint will also expire the deskphone from the deskphone cache in the event tha deskphone can't render
/// the returned deskphone state (in case the phone gets the URI before the phone screen has become ready). This will
/// cause the application to publish the current deskphone state the next time the deskphone registers with the Asterisk
/// server.
std::shared_ptr<ui::PhoneUI> createPhoneUI(std::unique_ptr<httplib::Server> const &http_server,
                                           std::shared_ptr<DeskphoneCache> const &deskphone_cache,
                                           ini::IniFile &cfg_ini)
{
    // Bind URI to get current lamp settings
    auto const phone_ui = std::make_shared<xml::yealink::PhoneUI>();
    auto const blf_state_uri = cfg_ini["http_server"]["blf_state_uri"].as<std::string>();
    http_server->Get(blf_state_uri,
                     [deskphone_cache, phone_ui](httplib::Request const &req, httplib::Response &res) -> void {
                         // Remove requesting deskphone from cache
                         if (req.has_param("aor") && req.has_param("ip")) {
                             deskphone_cache->deleteEndpoint(req.get_param_value("aor"), req.get_param_value("ip"));
                         }
                         // Return current deskphone state
                         res.set_content(phone_ui->toString(), "text/xml");
                     });
    return phone_ui;
}

/// @brief Creates night button object.
///
/// @param lamp_field Button factory.
/// @param io_conn Pointer to Asterisk AMI connection.
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param cfg_ini INI configuration object.
std::unique_ptr<ast_bridge::NightButton> createNightButton(std::shared_ptr<button_state::LampField> const &lamp_field,
                                                           std::shared_ptr<cpp_ami::Connection> const &io_conn,
                                                           std::unique_ptr<httplib::Server> const &http_server,
                                                           ini::IniFile &cfg_ini)
{
    // Setup XML browser
    auto const button_id = cfg_ini["night_button"]["button_id"].as<unsigned int>();
    auto const device = cfg_ini["night_button"]["device"].as<std::string>();
    auto const button_state = lamp_field->getButton(button_id, button_state::PhoneButton::Color::Red, false);
    auto const http_button = std::make_shared<xml::yealink::HTTPNightButton>(button_state, io_conn, device);
    // Bind URI to HTTP night button interface
    auto const night_uri = cfg_ini["http_server"]["night_uri"].as<std::string>();
    http_server->Get(night_uri,
                     [http_button]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
                         res.set_content(http_button->pushButton(), "text/xml");
                     });

    // Hook night button to Asterisk
    auto const exten = cfg_ini["night_button"]["exten"].as<std::string>();
    auto const context = cfg_ini["night_button"]["context"].as<std::string>();
    return std::make_unique<ast_bridge::NightButton>(button_state, io_conn, exten, context, device);
}

/// @brief Creates park button.
///
/// @param lamp_field Button factory.
/// @param io_conn Pointer to Asterisk AMI connection.
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param cfg_ini INI configuration object.
std::unique_ptr<ast_bridge::ParkButton> createParkButton(std::shared_ptr<button_state::LampField> const &lamp_field,
                                                         std::shared_ptr<cpp_ami::Connection> const &io_conn,
                                                         std::unique_ptr<httplib::Server> const &http_server,
                                                         ini::IniFile &cfg_ini)
{
    // Setup XML browser interface
    auto const http_url = cfg_ini["http_server"]["url"].as<std::string>();
    auto const park_info_uri = cfg_ini["http_server"]["park_info_uri"].as<std::string>();
    auto const parked_call_menu = std::make_shared<xml::yealink::CallParkMenu>(io_conn, http_url + park_info_uri);
    // Parked call list handler
    auto const park_list_uri = cfg_ini["http_server"]["park_list_uri"].as<std::string>();
    http_server->Get(park_list_uri,
                     [parked_call_menu]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
                         res.set_content(parked_call_menu->getParkedCallMenu(), "text/xml");
                     });
    // Parked call info handler
    http_server->Get(park_info_uri, [parked_call_menu](httplib::Request const &req, httplib::Response &res) -> void {
        // Serve parked call details
        if (req.has_param("selection")) {
            res.set_content(parked_call_menu->getParkedCallDetails(req.get_param_value("selection")), "text/xml");
            return;
        }
        // Warn of missing parameters
        static auto const error_xml = parked_call_menu->createMessageXML(true, 5, "Missing Extension Parameter",
                                                                         "Missing URL parameter '&selection=xxx'.");
        res.set_content(error_xml, "text/xml");
    });

    // Hook night button to Asterisk
    auto const button_id = cfg_ini["park_button"]["button_id"].as<unsigned int>();
    auto const button_state = lamp_field->getButton(button_id, button_state::PhoneButton::Color::Red, true);
    return std::make_unique<ast_bridge::ParkButton>(button_state, io_conn);
}

/// @brief Creates and configures the phonebook service for the application.
///
/// @param http_server Pointer to HTTP server to bind URI's to.
/// @param io_conn Asterisk AMI connection.
/// @param cfg_ini INI configuration object.
void configurePhonebookService(std::unique_ptr<httplib::Server> const &http_server,
                               std::shared_ptr<cpp_ami::Connection> const &io_conn, ini::IniFile &cfg_ini)
{
    auto phonebook_adapter = std::make_shared<phonebook::adapter::PJSIPWizardAdapter>(io_conn, "local-phone");

    // Setup Yealink phonebook URI
    auto const phonebook_uri = cfg_ini["http_server"]["phonebook_uri"].as<std::string>();
    auto const phonebook = std::make_shared<xml::yealink::XMLPhonebook>(phonebook_adapter, std::chrono::minutes(60));
    http_server->Get(
        phonebook_uri,
        [phonebook, phonebook_uri]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
            syslog(LOG_DEBUG, "Retrieving phonebook at %s", phonebook_uri.c_str());
            res.set_content(phonebook->getPhonebookString(), "text/xml");
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

    // Create phonebook service
    configurePhonebookService(g_server, io_conn, cfg_ini);

    // Setup application to deskphone bridge; all application lamp fields can share these objects
    auto const deskphone_cache = createDeskphoneCache(cfg_ini);
    auto const phone_bridge = std::make_shared<ui::PhonePJSIPNotifyBridge>(io_conn, deskphone_cache);

    // Setup up application buttons (lamp fields are specific to hardware desk phones). The following is the
    // configuration for our sites deskphones.
    auto const lamp_field = std::make_shared<button_state::LampField>(phone_bridge);
    auto const night_button = createNightButton(lamp_field, io_conn, g_server, cfg_ini);
    auto const park_button = createParkButton(lamp_field, io_conn, g_server, cfg_ini);
    // Register a deskphone UI with the lamp field
    auto const phone_ui = createPhoneUI(g_server, deskphone_cache, cfg_ini);
    lamp_field->registerUI("t88w", phone_ui);
    // Setup Asterisk to field lamp state bridge
    auto const deskphone = std::make_shared<ast_bridge::Deskphone>(deskphone_cache, lamp_field, io_conn);

    // httplib::Server::listen is a blocking call
    auto const http_addr = cfg_ini["http_server"]["addr"].as<std::string>();
    auto const http_port = cfg_ini["http_server"]["port"].as<uint16_t>();
    g_server->listen(http_addr, http_port);

    // Stop ping thread
    thread_run_flag = false;
    cv.notify_all();
    ami_ping_thread.join();
}

void configSyslog(YAML::Node const &cfg)
{
}
*/