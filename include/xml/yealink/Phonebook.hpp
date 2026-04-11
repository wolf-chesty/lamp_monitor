// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_PHONEBOOK_HPP
#define XML_YEALINK_PHONEBOOK_HPP

#include "PhonebookXMLCreator.hpp"

#include "PhonebookProvider.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

namespace xml::yealink {

class Phonebook : public PhonebookXMLCreator {
public:
    using clock_t = std::chrono::steady_clock;

public:
    explicit Phonebook(std::shared_ptr<PhonebookProvider> phonebook_provider, std::chrono::minutes expiry);
    ~Phonebook() override = default;

    std::string getPhonebookXML() override;

private:
    std::shared_ptr<PhonebookProvider> phonebook_provider_;
    clock_t::time_point timestamp_{};
    std::chrono::minutes expiry_;
    std::string phonebook_xml_;
    std::mutex phonebook_xml_mut_;
};

}

#endif
