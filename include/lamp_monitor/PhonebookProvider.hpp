// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_HANDLER_HPP
#define PHONEBOOK_HANDLER_HPP

#include "lamp_monitor/PhonebookDetail.hpp"
#include <c++ami/Connection.hpp>
#include <memory>
#include <string>
#include <vector>

class PhonebookProvider {
public:
    explicit PhonebookProvider(std::shared_ptr<cpp_ami::Connection> io_conn, std::string filter);
    ~PhonebookProvider() = default;

    std::vector<PhonebookDetail> getPhonebookDetails() const;

private:
    std::shared_ptr<cpp_ami::Connection> io_conn_;
    std::string filter_;
};

#endif
