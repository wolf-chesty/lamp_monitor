// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/yealink/Phonebook.hpp"

#include <cassert>
#include <pugixml.hpp>
#include <sstream>

using namespace phonebook::yealink;

Phonebook::Phonebook(std::shared_ptr<phonebook::PhonebookProvider> phonebook_adapter, std::chrono::minutes expiry)
    : phonebook::Phonebook(std::move(phonebook_adapter), std::move(expiry))
{
}

std::shared_ptr<phonebook::Phonebook> Phonebook::create(YAML::Node const &config,
                                                        std::shared_ptr<phonebook::PhonebookProvider> const &adapter)
{
    assert(config["type"].as<std::string>() == "yealink");
    std::chrono::minutes const expiry{std::max(config["ttl"].as<uint32_t>(), uint32_t{120})};
    return std::make_shared<Phonebook>(adapter, expiry);
}

std::string Phonebook::getContentType()
{
    return "text/xml";
}

std::string Phonebook::getPhonebook(std::shared_ptr<PhonebookProvider> const &phonebook_source)
{
    // Create XML document
    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto phonebook_xml = xml_doc.append_child("YealinkIPPhoneDirectory");

    for (auto const &phonebook_detail : phonebook_source->getCallerDetails()) {
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
