// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_PHONE_ADAPTER_HPP
#define XML_YEALINK_PHONE_ADAPTER_HPP

#include "monitor/PhoneUIAdapter.hpp"

namespace xml::yealink {

class PhoneAdapter : public monitor::PhoneUIAdapter {
public:
    PhoneAdapter() = default;
    ~PhoneAdapter() override = default;
};

}

#endif
