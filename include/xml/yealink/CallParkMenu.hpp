// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_CALLPARKMENU_HPP
#define XML_YEALINK_CALLPARKMENU_HPP

#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace xml::yealink {

class CallParkMenu {
public:
    explicit CallParkMenu(std::shared_ptr<cpp_ami::Connection> io_conn, std::string parked_call_info_uri);
    ~CallParkMenu() = default;

    static std::string createMessageXML(bool beep, uint8_t timeout, std::string const &title, std::string const &text);

    std::string getParkedCallMenu() const;
    std::string getParkedCallDetails(std::string const &park_exten) const;

private:
    std::string createParkedCallMenu(cpp_ami::reaction::EventList const &parked_call_list) const;

    static std::string createNoParkedCallMessage();

    std::shared_ptr<cpp_ami::Connection> io_conn_; ///< Pointer to AMI Asterisk connection.
    std::string parked_call_info_uri_;             ///< URI for parked call info.
};

} // namespace xml::yealink

#endif
