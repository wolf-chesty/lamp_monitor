// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/PhonebookProvider.hpp"

#include <c++ami/action/GetConfigJson.hpp>
#include <iostream>
#include <pugixml.hpp>
#include <regex>
#include <yaml-cpp/yaml.h>

PhonebookProvider::PhonebookProvider(std::shared_ptr<cpp_ami::Connection> const &io_conn, std::chrono::minutes expiry)
    : io_conn_(std::move(io_conn))
    , timestamp_(clock_t::now() - std::chrono::minutes(1))
    , expiry_(expiry)
{
}

std::string PhonebookProvider::getPhonebookXML()
{
    std::lock_guard const lock(phonebook_xml_mut_);

    auto const now = clock_t::now();
    if (timestamp_ > now) {
        return phonebook_xml_;
    }

    std::set<std::string> caller_ids;
    auto generate_phonebook = [&caller_ids](cpp_ami::event::Event const &event) mutable -> bool {
        // Grab list of caller IDs
        auto const cfg_yaml = YAML::Load(event["JSON"]);
        for (auto const endpoint : cfg_yaml) {
            auto const &aor_cfg_yaml = endpoint.second;
            if (!aor_cfg_yaml["templates"]) {
                continue;
            }
            if (auto const type = aor_cfg_yaml["templates"].as<std::string>(); type != "local-phone") {
                continue;
            }

            if (auto const caller_id = aor_cfg_yaml["endpoint/callerid"].as<std::string>(); !caller_id.empty()) {
                caller_ids.insert(caller_id);
            }
        }
        return true;
    };

    cpp_ami::action::GetConfigJSON action;
    action["Filename"] = "pjsip_wizard.conf";
    if (auto const reaction = io_conn_->invoke(action); reaction->isSuccess()) {
        reaction->forEach(generate_phonebook);
    }

    phonebook_xml_ = createPhonebookXML(caller_ids);

    timestamp_ = clock_t::now() + expiry_;

    return phonebook_xml_;
}

std::string PhonebookProvider::createPhonebookXML(std::set<std::string> const &caller_ids)
{
    // Populate XML phonebook from
    pugi::xml_document doc;
    auto decl = doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto phonebook_xml = doc.append_child("YealinkIPPhoneDirectory>");

    for (auto const &callerid : caller_ids) {
        std::regex re_pattern("\"([^\"]*)\" <([0-9]*)>");
        std::smatch matches;
        std::regex_search(callerid, matches, re_pattern);

        auto dir_entry_xml = phonebook_xml.append_child("DirectoryEntry");
        // Add name
        auto name_xml = dir_entry_xml.append_child("Name");
        name_xml.append_child(pugi::node_pcdata).set_value(matches[1].str());
        // Add extension
        auto num_xml = dir_entry_xml.append_child("Telephone");
        num_xml.append_child(pugi::node_pcdata).set_value(matches[2].str());
    }

    // Convert to string and return it.
    std::ostringstream xml_string;
    doc.save(xml_string, "", pugi::format_raw);
    return xml_string.str();
}
