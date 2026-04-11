// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "xml/yealink/Phonebook.hpp"

#include <cassert>
#include <pugixml.hpp>
#include <sstream>

using namespace xml::yealink;

Phonebook::Phonebook(std::shared_ptr<phonebook::Adapter> phonebook_provider, std::chrono::minutes expiry)
    : phonebook_adapter_(std::move(phonebook_provider))
    , expiry_(expiry)
{
}

std::string Phonebook::getPhonebookXML()
{
    std::lock_guard const lock(phonebook_xml_mut_);

    // Phonebook XML is still valid
    auto const now = clock_t::now();
    if (timestamp_ > now) {
        return phonebook_xml_;
    }

    // Create XML document
    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto phonebook_xml = xml_doc.append_child("YealinkIPPhoneDirectory");

    assert(phonebook_adapter_);
    auto const phonebook_details = phonebook_adapter_->getPhonebookDetails();
    for (auto const &phonebook_detail : phonebook_details) {
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
    phonebook_xml_ = xml_string.str();

    return phonebook_xml_;
}