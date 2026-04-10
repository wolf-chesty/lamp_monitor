// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef YEALINK_PHONEBOOK_PROVIDER_HPP
#define YEALINK_PHONEBOOK_PROVIDER_HPP

#include "lamp_monitor/PhonebookXMLCreator.hpp"

#include "lamp_monitor/PhonebookProvider.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

class YealinkPhonebookXML : public PhonebookXMLCreator {
public:
    using clock_t = std::chrono::steady_clock;

public:
    explicit YealinkPhonebookXML(std::shared_ptr<PhonebookProvider> phonebook_provider, std::chrono::minutes expiry);
    ~YealinkPhonebookXML() override = default;

    std::string getPhonebookXML() override;

private:
    std::shared_ptr<PhonebookProvider> phonebook_provider_;
    clock_t::time_point timestamp_{};
    std::chrono::minutes expiry_;
    std::string phonebook_xml_;
    std::mutex phonebook_xml_mut_;
};

#endif
