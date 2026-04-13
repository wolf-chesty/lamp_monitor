// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef APPLICATION_PARAMETERS_HPP
#define APPLICATION_PARAMETERS_HPP

#include <string>

/// @struct ApplicationParameters
///
/// @brief Object contains the parameters for the application.
struct ApplicationParameters {
    std::string config_file; ///< Name of the configuration file.
    bool is_daemon{};        ///< Flag indicating if the application should run as a daemon.
};

#endif
