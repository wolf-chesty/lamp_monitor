// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/LampFieldMonitor.hpp"
#include "lamp_monitor/NightLampMonitor.hpp"
#include "lamp_monitor/ParkedCallMonitor.hpp"
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
#include <pugixml.hpp>
#include <unistd.h>

// Function declarations ===============================================================================================

void signalHandler(int signum);

std::unique_ptr<httplib::Server> createHttpServer(std::shared_ptr<ParkedCallMonitor> const &parked_call_monitor,
                                                  std::string const &park_call_list_uri,
                                                  std::string const &park_call_info_uri,
                                                  std::shared_ptr<NightLampMonitor> const &night_lamp_monitor,
                                                  std::string const &night_uri);

std::thread createAmiPingThread(std::shared_ptr<cpp_ami::Connection> ami_conn, std::condition_variable &cv,
                                std::atomic<bool> &run_thread_flag, std::chrono::milliseconds period);

// Globals =============================================================================================================

std::unique_ptr<httplib::Server> g_server;

int main(int argc, char *argv[])
{
    argparse::ArgumentParser parser("lamp_monitor");
    parser.add_argument("-d", "--daemon").default_value(false).implicit_value(true).nargs(0);
    parser.add_argument("config_ini").help("Application config file").required();

    try {
        parser.parse_args(argc, argv);
    }
    catch (std::runtime_error const &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        exit(1);
    }

    ini::IniFile ami_ini;
    ami_ini.load(parser.get<std::string>("config_ini"));

    auto const is_daemon = parser.is_used("--daemon");
    if (is_daemon) {
        if (daemon(0, 0) == -1) {
            exit(1);
        }
    }

    auto const ami_hostname = ami_ini["ami"]["host"].as<std::string>();
    auto const ami_port = ami_ini["ami"]["port"].as<uint16_t>();
    auto const ami_username = ami_ini["ami"]["username"].as<std::string>();
    auto const ami_password = ami_ini["ami"]["secret"].as<std::string>();

    auto const http_addr = ami_ini["http_server"]["addr"].as<std::string>();
    auto const http_port = ami_ini["http_server"]["port"].as<uint16_t>();
    auto const http_url = ami_ini["http_server"]["url"].as<std::string>();

    // Create AMI connection
    auto ami_conn = std::make_shared<cpp_ami::Connection>(ami_hostname, ami_port);

    // Create AMI Login action
    cpp_ami::action::Login login(ami_username, ami_password);
    login["AuthType"] = "plain";
    login["Events"] = "on";
    // Send login to AMI
    auto reaction = ami_conn->invoke(login);
    // Confirm login
    if (!reaction->isSuccess()) {
        if (is_daemon) {
            std::cerr << "Unable to login to AMI";
        }
        return -1;
    }

    // Asterisk will drop AMI connections it hasn't heard from in a while; start an AMI ping thread
    std::atomic<bool> thread_run_flag{true};
    std::condition_variable cv;
    std::thread ami_ping_thread = createAmiPingThread(ami_conn, cv, thread_run_flag, std::chrono::milliseconds(2500));

    // Create AMI monitors
    auto const night_button_id = ami_ini["night_button"]["button_id"].as<unsigned int>();
    auto const night_exten = ami_ini["night_button"]["exten"].as<std::string>();
    auto const night_context = ami_ini["night_button"]["context"].as<std::string>();
    auto const night_device = ami_ini["night_button"]["device"].as<std::string>();
    auto night_lamp_monitor =
        std::make_shared<NightLampMonitor>(ami_conn, night_button_id, night_exten, night_context, night_device);

    auto const park_button_id = ami_ini["park_button"]["button_id"].as<unsigned int>();
    auto const park_info_uri = ami_ini["park_button"]["info_uri"].as<std::string>();
    auto park_lamp_monitor = std::make_shared<ParkedCallMonitor>(ami_conn, park_button_id, http_url + park_info_uri);

    // Create lamp field monitor
    LampFieldMonitor lamp_field_handler(ami_conn);
    lamp_field_handler.addLamp(night_lamp_monitor);
    lamp_field_handler.addLamp(park_lamp_monitor);

    // Start HTTP server
    auto const night_uri = ami_ini["night_button"]["night_uri"].as<std::string>();
    auto const park_list_uri = ami_ini["park_button"]["list_uri"].as<std::string>();
    g_server = createHttpServer(park_lamp_monitor, park_list_uri, park_info_uri, night_lamp_monitor, night_uri);
    signal(SIGINT, signalHandler);
    g_server->listen(http_addr, http_port);

    // Stop ping thread
    thread_run_flag = false;
    cv.notify_one();
    ami_ping_thread.join();

    // Logout of AMI; wait for the response before terminating
    cpp_ami::action::Logoff const logoff;
    reaction = ami_conn->invoke(logoff);

    return 0;
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

/// @brief Creates a new HTTP server with URI endpoints attached to lamp monitor objects.
///
/// @param parked_call_monitor Object that monitors the parked calls on the system.
/// @param park_call_list_uri HTTP URI to query list of parked calls.
/// @param park_call_info_uri HTTP URI to query parked call details.
/// @param night_lamp_monitor Object that monitors the night mode of the system.
/// @param night_uri HTTP URI to disable/enable night mode.
///
/// @return Pointer to an HTTP server.
std::unique_ptr<httplib::Server> createHttpServer(std::shared_ptr<ParkedCallMonitor> const &parked_call_monitor,
                                                  std::string const &park_call_list_uri,
                                                  std::string const &park_call_info_uri,
                                                  std::shared_ptr<NightLampMonitor> const &night_lamp_monitor,
                                                  std::string const &night_uri)
{
    assert(parked_call_monitor);
    assert(night_lamp_monitor);

    auto http_server = std::make_unique<httplib::Server>();

    // Lambda to build and display a list of parked calls on the Asterisk system. Handsets can execute this URI to get a
    // list of parked calls.
    http_server->Get(
        park_call_list_uri,
        [parked_call_monitor]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
            res.set_content(parked_call_monitor->getParkedCallMenu(), "text/xml");
        });

    http_server->Get(
        park_call_info_uri, [parked_call_monitor](httplib::Request const &req, httplib::Response &res) -> void {
            if (req.has_param("selection")) {
                res.set_content(parked_call_monitor->getParkedCallDetails(req.get_param_value("selection")),
                                "text/xml");
            }
            else {
                static auto const error_xml = ParkedCallMonitor::createMessageXML(
                    true, 5, "Missing Extension Parameter", "Missing URL parameter '&selection=7xxx'.");
                res.set_content(error_xml, "text/xml");
            }
        });

    // Lambda to change the night state. Handsets can execute this URI to place/remove the call from night mode.
    http_server->Get(
        night_uri, [night_lamp_monitor]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
            res.set_content(night_lamp_monitor->resetNightState(), "text/xml");
        });

    return http_server;
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
std::thread createAmiPingThread(std::shared_ptr<cpp_ami::Connection> ami_conn, std::condition_variable &cv,
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

    // Name the thread so it shows up in top
    std::string_view thread_name("ping_ami");
    assert(thread_name.length() <= 16);
    pthread_setname_np(work_thread.native_handle(), thread_name.data());

    return work_thread;
}
