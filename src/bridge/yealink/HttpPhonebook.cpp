// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "bridge/yealink/HttpPhonebook.hpp"

#include <cassert>
#include <pugixml.hpp>
#include <sstream>

using namespace bridge::yealink;

HTTPPhonebook::HTTPPhonebook(std::shared_ptr<phonebook::Adapter> phonebook_adapter, std::chrono::minutes expiry)
    : phonebook::HTTPPhonebook(std::move(phonebook_adapter), std::move(expiry))
{
}

std::shared_ptr<phonebook::HTTPPhonebook> HTTPPhonebook::create(YAML::Node const &config,
                                                               std::shared_ptr<phonebook::Adapter> const &adapter)
{
    assert(config["type"].as<std::string>() == "yealink");
    std::chrono::minutes const expiry{std::max(config["ttl"].as<uint32_t>(), uint32_t{120})};
    return std::make_shared<HTTPPhonebook>(adapter, expiry);
}

std::string HTTPPhonebook::getContentType()
{
    return "text/xml";
}

std::string HTTPPhonebook::getPhonebookImpl()
{
    // Create XML document
    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto phonebook_xml = xml_doc.append_child("YealinkIPPhoneDirectory");

    auto const phonebook_adapter = getPhonebookAdapter();
    for (auto const &phonebook_detail : phonebook_adapter->getPhonebookDetails()) {
        auto dir_entry_xml = phonebook_xml.append_child("DirectoryEntry");
        // Add name
        auto name_xml = dir_entry_xml.append_child("Name");
        name_xml.append_child(pugi::node_pcdata).set_value(phonebook_detail.name);
        // Add extension
        auto num_xml = dir_entry_xml.append_child("Telephone");
        num_xml.append_child(pugi::node_pcdata).set_value(phonebook_detail.number);
    }

    // Create XML string
    std::ostringstream xml_string;
    xml_doc.save(xml_string, "", pugi::format_raw);
    return xml_string.str();
}
