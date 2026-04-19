// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef CONFIG_FILE_PARSER_HPP
#define CONFIG_FILE_PARSER_HPP

#include "asterisk/RegisterEventHandler.hpp"
#include <c++ami/Connection.hpp>
#include <httplib.h>
#include <string_view>
#include <yaml-cpp/yaml.h>

void configureSyslog(YAML::Node const &config, std::string_view app_name, bool const is_daemon);

std::shared_ptr<cpp_ami::Connection> createAMIConnection(YAML::Node const &config, std::string_view filter = "on");

std::tuple<std::unique_ptr<httplib::Server>, std::string, uint16_t> createHTTPServer(YAML::Node const &config);

void createPhonebooks(YAML::Node const &config, httplib::Server &http_server,
                      std::shared_ptr<cpp_ami::Connection> const &conn);

std::shared_ptr<asterisk::RegisterEventHandler>
    createRegisterEventHandler(YAML::Node const &config, httplib::Server &http_server,
                               std::shared_ptr<cpp_ami::Connection> const &conn);

#endif
