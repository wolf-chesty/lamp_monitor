// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_STATE_PROVIDER_HPP
#define BRIDGE_STATE_PROVIDER_HPP

#include <string>

namespace bridge {

/// @class HTTPStateProvider
/// @namespace bridge
///
/// @brief Provides an interface that can be implemented by objects that wish to provide their object state via HTTP.
class HTTPStateProvider {
public:
    virtual ~HTTPStateProvider() = default;

    virtual std::string getHTTPState() = 0;
    virtual std::string getContentType() = 0;
};

}

#endif
