// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "xml/yealink/CallParkMenu.hpp"

#include <c++ami/action/ParkedCalls.hpp>
#include <c++ami/reaction/EventList.hpp>
#include <cassert>
#include <fmt/core.h>
#include <pugixml.hpp>
#include <sstream>
#include <syslog.h>

using namespace xml::yealink;

CallParkMenu::CallParkMenu(std::shared_ptr<cpp_ami::Connection> io_conn, std::string parked_call_info_uri)
    : io_conn_(std::move(io_conn))
    , parked_call_info_uri_(std::move(parked_call_info_uri))
{
    assert(io_conn_);
}

std::string CallParkMenu::createMessageXML(bool const beep, uint8_t const timeout, std::string const &title,
                                           std::string const &text)
{
    pugi::xml_document doc;
    auto decl = doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    // Setup phone message
    auto xml_screen = doc.append_child("YealinkIPPhoneTextScreen");
    xml_screen.append_attribute("Beep") = beep ? "yes" : "no";
    xml_screen.append_attribute("Timeout") = timeout;
    // Set message title.
    auto xml_title = xml_screen.append_child("Title");
    xml_title.append_attribute("wrap") = "no";
    xml_title.append_child(pugi::node_pcdata).set_value(title);
    // Format message.
    auto xml_text = xml_screen.append_child("Text");
    xml_text.append_child(pugi::node_pcdata).set_value(text);

    // Convert to string and return it.
    std::ostringstream xml_string;
    doc.save(xml_string, "", pugi::format_raw);
    return xml_string.str();
}

std::string CallParkMenu::getParkedCallMenu() const
{
    assert(io_conn_);

    cpp_ami::action::ParkedCalls parked_calls;
    auto const ami_response = io_conn_->invoke(parked_calls);
    auto const ami_response_list = dynamic_cast<cpp_ami::reaction::EventList const *>(ami_response.get());
    assert(ami_response_list);
    return ami_response_list->eventCount() == 0 ? createNoParkedCallMessage()
                                                : createParkedCallMenu(*ami_response_list);
}

void createParkedCallNode(pugi::xml_node parked_call, std::string_view parked_call_info_uri,
                          cpp_ami::event::Event const &event)
{
    auto prompt = parked_call.append_child("Prompt");
    prompt.append_child(pugi::node_pcdata)
        .set_value(fmt::format("{}\n{}", event["ParkeeCallerIDName"], event["ParkeeCallerIDNum"]));

    auto uri = parked_call.append_child("URI");
    uri.append_child(pugi::node_pcdata).set_value(parked_call_info_uri);

    auto dial = parked_call.append_child("Dial");
    dial.append_child(pugi::node_pcdata).set_value(event["ParkingSpace"]);

    auto selection = parked_call.append_child("Selection");
    selection.append_child(pugi::node_pcdata).set_value(event["ParkingSpace"]);
}

std::string CallParkMenu::createParkedCallMenu(cpp_ami::reaction::EventList const &parked_call_list) const
{
    syslog(LOG_DEBUG, "CallParkMenu::createParkedCallMenu() : Creating parked call menu");

    pugi::xml_document doc;
    auto decl = doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    // Setup phone menu
    auto xml_menu = doc.append_child("YealinkIPPhoneTextMenu");
    xml_menu.append_attribute("style") = "none";
    xml_menu.append_attribute("Beep") = "no";
    xml_menu.append_attribute("Timeout") = "10";

    auto menu_title = xml_menu.append_child("Title");
    menu_title.append_attribute("wrap") = "no";
    menu_title.append_child(pugi::node_pcdata)
        .set_value(fmt::format("Parked Calls: {}", parked_call_list.eventCount()));

    // Add parked calls to menu
    parked_call_list.forEach([this, &xml_menu](cpp_ami::event::Event const &event) mutable -> bool {
        createParkedCallNode(xml_menu.append_child("MenuItem"), parked_call_info_uri_, event);
        return false;
    });

    // Convert menu XML to string and return it
    std::ostringstream parked_call_menu_xml;
    doc.save(parked_call_menu_xml, "", pugi::format_raw);
    return parked_call_menu_xml.str();
}

std::string CallParkMenu::createNoParkedCallMessage()
{
    syslog(LOG_DEBUG, "CallParkMenu::createNoParkedCallMessages() : Creating phone screen");

    return createMessageXML(false, 5, "Parked Calls: 0", "No parked calls.");
}

std::string CallParkMenu::getParkedCallDetails(std::string const &park_exten) const
{
    syslog(LOG_DEBUG, "CallParkMenu::getParkedCallDetails(\"%s\")", park_exten.c_str());

    assert(io_conn_);
    cpp_ami::action::ParkedCalls parked_calls;
    auto const ami_response = io_conn_->invoke(parked_calls);
    auto const ami_response_list = dynamic_cast<cpp_ami::reaction::EventList const *>(ami_response.get());
    assert(ami_response_list);

    std::string message{"No parked calls."};
    ami_response_list->forEach([&message, &park_exten](cpp_ami::event::Event const &event) mutable -> bool {
        if (event["ParkingSpace"] != park_exten) {
            return false;
        }
        message = fmt::format("Caller: {}\n", event["ParkeeCallerIDName"]) +
                  fmt::format("Number: {}\n", event["ParkeeCallerIDNum"]) +
                  fmt::format("Parking Space: {}\n", park_exten) +
                  fmt::format("Duration: {}s", event["ParkingDuration"]);
        return true;
    });

    return createMessageXML(false, 10, "Parked Call Detail", message);
}
