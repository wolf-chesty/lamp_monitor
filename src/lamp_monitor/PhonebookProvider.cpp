// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/PhonebookProvider.hpp"

#include <c++ami/action/GetConfigJson.hpp>
#include <cassert>
#include <iostream>
#include <regex>
#include <yaml-cpp/yaml.h>
#include <pugixml.hpp>

PhonebookProvider::PhonebookProvider(std::shared_ptr<cpp_ami::Connection> const &io_conn, std::chrono::minutes expiry)
    : io_conn_(std::move(io_conn))
    , expiry_(expiry)
{
    startThread();
}

PhonebookProvider::~PhonebookProvider()
{
    stopThread();
}

std::string PhonebookProvider::getPhonebookXML()
{
    std::lock_guard const lock(phonebook_xml_mut_);
    return phonebook_xml_;
}

void PhonebookProvider::setPhonebookXML(std::string phonebook_xml)
{
    std::lock_guard const lock(phonebook_xml_mut_);
    phonebook_xml_ = std::move(phonebook_xml);
}

void PhonebookProvider::startThread()
{
    work_thread_run_ = true;
    work_thread_ = std::thread(&PhonebookProvider::workThread, this);

    pthread_setname_np(work_thread_.native_handle(), "phonebook");
}

void PhonebookProvider::stopThread()
{
    work_thread_run_ = false;
    work_thread_cv_.notify_one();

    assert(work_thread_.joinable());
    work_thread_.join();
}

std::string createPhonebookXML(std::vector<std::string> callerid_list)
{
    std::ranges::sort(callerid_list.begin(), callerid_list.end());

    // Populate XML phonebook from
    pugi::xml_document doc;
    auto decl = doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto phonebook_xml = doc.append_child("YealinkIPPhoneDirectory>");

    for (auto const &callerid : callerid_list) {
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

void PhonebookProvider::workThread()
{
    auto generate_phonebook = [this](cpp_ami::event::Event const &event) -> bool {
        // Grab list of caller IDs
        std::vector<std::string> callerid_list;
        auto const cfg_yaml = YAML::Load(event["JSON"]);
        for (auto const endpoint : cfg_yaml) {
            auto const &aor_cfg_yaml = endpoint.second;
            if (!aor_cfg_yaml["templates"]) {
                continue;
            }
            auto const type = aor_cfg_yaml["templates"].as<std::string>();
            if (type != "local-phone") {
                continue;
            }

            callerid_list.emplace_back(aor_cfg_yaml["endpoint/callerid"].as<std::string>());
        }

        // Convert list to Yealink XML Phonebook
        setPhonebookXML(createPhonebookXML(std::move(callerid_list)));

        return true;
    };

    std::mutex empty_mut;
    do {
        cpp_ami::action::GetConfigJSON action;
        action["Filename"] = "pjsip_wizard.conf";
        if (auto const reaction = io_conn_->invoke(action); reaction->isSuccess()) {
            reaction->forEach(generate_phonebook);
        }

        std::unique_lock lock(empty_mut);
        work_thread_cv_.wait_for(lock, std::chrono::seconds(expiry_), [this]() -> bool { return !work_thread_run_; });
    } while (work_thread_run_);
}
