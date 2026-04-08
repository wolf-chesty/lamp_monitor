// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_HANDLER_HPP
#define PHONEBOOK_HANDLER_HPP

#include <c++ami/Connection.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <string>

class PhonebookProvider {
public:
    using clock_t = std::chrono::steady_clock;

public:
    explicit PhonebookProvider(std::shared_ptr<cpp_ami::Connection> const &io_conn, std::chrono::minutes expiry);
    ~PhonebookProvider() = default;

    std::string getPhonebookXML();

private:
    static std::string createPhonebookXML(std::set<std::string> const &phonebook);

    std::shared_ptr<cpp_ami::Connection> io_conn_;
    clock_t::time_point timestamp_;
    std::chrono::minutes expiry_;
    std::string phonebook_xml_;
    std::mutex phonebook_xml_mut_;
};

#endif
