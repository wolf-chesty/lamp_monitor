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
        std::cerr << "Unable to find config file " << args.config_file << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        // Load config file
        auto const config_yaml = YAML::LoadFile(args.config_file);

        // Fork to background if requested
        if (args.is_daemon) {
            if (daemon(0, 0) == -1) {
                std::cerr << "Unable to daemonize" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        setUserGroup(args.user, args.group);

        configureSyslog(config_yaml["syslog"], "lamp_monitor", args.is_daemon);

        syslog(LOG_DEBUG, "Starting application");

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

        // Create HTTP phonebooks
        createPhonebooks(config_yaml["phonebooks"], *g_server, io_conn);
        // Create Asterisk registration event handler; this object maintains the lifetimes of all button plans in use by
        // the application.
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

    syslog(LOG_DEBUG, "Stopping application");
    closelog();

    return EXIT_SUCCESS;
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
    parser.add_argument("-u", "--user").default_value("");
    parser.add_argument("-g", "--group").default_value("");
    parser.add_argument("config_ini").help("Application config file").required();

    try {
        parser.parse_args(argc, argv);
    }
    catch (std::runtime_error const &err) {
        std::cerr << err.what() << '\r' << parser;
        exit(EXIT_FAILURE);
    }

    return ApplicationParameters{parser.get<std::string>("config_ini"), parser.get<std::string>("--user"),
                                 parser.get<std::string>("--group"), parser.get<bool>("--daemon")};
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
        syslog(LOG_DEBUG, "Starting AMI ping thread");
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
