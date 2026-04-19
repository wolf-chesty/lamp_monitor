// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/HTTPPhonebook.hpp"

#include "xml/yealink/XmlPhonebook.hpp"
#include <cassert>

using namespace phonebook;

HTTPPhonebook::HTTPPhonebook(std::shared_ptr<Adapter> phonebook_adapter)
    : phonebook_adapter_(std::move(phonebook_adapter))
{
}

std::shared_ptr<HTTPPhonebook> HTTPPhonebook::create(std::shared_ptr<Adapter> const &adapter, YAML::Node const &config)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "yealink") {
        return xml::yealink::XMLPhonebook::create(adapter, config);
    }
    assert(false);
    return nullptr;
}

std::shared_ptr<Adapter> HTTPPhonebook::getPhonebookAdapter()
{
    assert(phonebook_adapter_);
    return phonebook_adapter_;
}
