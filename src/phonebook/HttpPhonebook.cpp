// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/HttpPhonebook.hpp"

#include "bridge/snom/HttpPhonebook.hpp"
#include "bridge/yealink/HttpPhonebook.hpp"
#include <cassert>
#include <syslog.h>

using namespace phonebook;

HTTPPhonebook::HTTPPhonebook(std::shared_ptr<Adapter> phonebook_adapter, std::chrono::minutes expiry)
    : phonebook_adapter_(std::move(phonebook_adapter))
    , expiry_(std::move(expiry))
{
}

std::shared_ptr<HTTPPhonebook> HTTPPhonebook::create(YAML::Node const &config, std::shared_ptr<Adapter> const &adapter)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "snom") {
        return bridge::snom::HTTPPhonebook::create(config, adapter);
    }
    else if (type == "yealink") {
        return bridge::yealink::HTTPPhonebook::create(config, adapter);
    }

    assert(false);
    return nullptr;
}

std::shared_ptr<Adapter> HTTPPhonebook::getPhonebookAdapter()
{
    assert(phonebook_adapter_);
    return phonebook_adapter_;
}

std::string HTTPPhonebook::getPhonebook()
{
    syslog(LOG_DEBUG, "HTTPPhonebook::getPhonebook() : Creating HTTP phonebook");

    std::lock_guard const lock(cached_phonebook_mut_);

    // Phonebook string still valid?
    if (timestamp_ < clock_t::now()) {
        cached_phonebook_ = getPhonebookImpl();
        timestamp_ = clock_t::now() + expiry_;
    }
    return cached_phonebook_;
}
