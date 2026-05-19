// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/Phonebook.hpp"

#include "phonebook/snom/Phonebook.hpp"
#include "phonebook/yealink/Phonebook.hpp"
#include <cassert>
#include <syslog.h>

using namespace phonebook;

Phonebook::Phonebook(std::shared_ptr<PhonebookProvider> phonebook_adapter, std::chrono::minutes expiry)
    : phonebook_source_(std::move(phonebook_adapter))
    , expiry_(std::move(expiry))
{
}

std::shared_ptr<Phonebook> Phonebook::create(YAML::Node const &config, std::shared_ptr<PhonebookProvider> const &source)
{
    auto const &type = config["type"].as<std::string>();
    if (type == "snom") {
        return phonebook::snom::Phonebook::create(config, source);
    }
    else if (type == "yealink") {
        return phonebook::yealink::Phonebook::create(config, source);
    }

    assert(false);
    return nullptr;
}

std::string Phonebook::getPhonebook()
{
    syslog(LOG_DEBUG, "HTTPPhonebook::getPhonebook() : Creating HTTP phonebook");

    std::lock_guard const lock(cached_phonebook_mut_);

    // Phonebook string still valid?
    if (timestamp_ < clock_t::now()) {
        cached_phonebook_ = getPhonebook(phonebook_source_);
        timestamp_ = clock_t::now() + expiry_;
    }
    return cached_phonebook_;
}
