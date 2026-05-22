// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef CONFIG_FILE_PARSER_HPP
#define CONFIG_FILE_PARSER_HPP

#include "asterisk/RegisterEventHandler.hpp"
#include <c++ami/Connection.hpp>
#include <httplib.h>
#include <string_view>
#include <yaml-cpp/yaml.h>

YAML::Node loadConfigFile(std::string const &config_file);

/// @brief Sets the current user/group that the application is running as.
///
/// @param user Username to run under.
/// @param group Group to run under.
void setUserGroup(std::string const &user, std::string const &group);

/// @brief Configures syslog.
///
/// @param config Configuration parameters for syslog.
/// @param app_name Application name.
/// @param is_daemon \c true if application is running as daemon.
void configureSyslog(YAML::Node const &config, std::string_view app_name, bool const is_daemon);

/// @brief Creates Asterisk AMI configuration connection.
///
/// @param config Configration parameters for Asterisk AMI connection.
/// @param filter Event filter.
///
/// @return Pointer to Asterisk AMI connection.
std::shared_ptr<cpp_ami::Connection> createAMIConnection(YAML::Node const &config, std::string_view filter = "on");

/// @brief Configures HTTP server.
///
/// @param config Configuration parameters for HTTP server.
///
/// @return Pointer to HTTP server, HTTP URL, and port.
std::tuple<std::unique_ptr<httplib::Server>, std::string, uint16_t> createHTTPServer(YAML::Node const &config);

/// @brief Creates phonebook endpoints.
///
/// @param config Configuration parameters for phonebooks.
/// @param http_server HTTP server to configuration.
/// @param conn Pointer to Asterisk AMI connection.
void createPhonebooks(YAML::Node const &config, httplib::Server &http_server,
                      std::shared_ptr<cpp_ami::Connection> const &conn);

/// @brief Creates Asterisk registration event handler.
///
/// @param config Configuration for registration event handler.
/// @param http_server HTTP server to configuration.
/// @param conn Pointer to Asterisk AMI connection.
///
/// @return Pointer to Asterisk registration event handler.
std::shared_ptr<asterisk::RegisterEventHandler>
    createRegisterEventHandler(YAML::Node const &config, httplib::Server &http_server,
                               std::shared_ptr<cpp_ami::Connection> const &conn);

#endif
