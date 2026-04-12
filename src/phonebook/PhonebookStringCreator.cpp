// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/PhonebookStringCreator.hpp"

#include <cassert>

using namespace phonebook;

PhonebookStringCreator::PhonebookStringCreator(std::shared_ptr<Adapter> phonebook_adapter)
    : phonebook_adapter_(std::move(phonebook_adapter))
{
}

std::shared_ptr<Adapter> PhonebookStringCreator::getPhonebookAdapter()
{
    assert(phonebook_adapter_);
    return phonebook_adapter_;
}
