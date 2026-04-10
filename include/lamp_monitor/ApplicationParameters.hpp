// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef APPLICATION_PARAMETERS_HPP
#define APPLICATION_PARAMETERS_HPP

#include <string>

struct ApplicationParameters {
    std::string config_file;
    bool is_daemon{};
};

#endif
