// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_PHONEBOOK_HPP
#define XML_PHONEBOOK_HPP

#include "phonebook/Adapter.hpp"
#include <memory>
#include <string>

namespace phonebook {

/// @class PhonebookStringCreator
/// @namespace phonebook
///
/// @brief Provides an interface for classes that create phonebook text that is compatible with deskphones.
class PhonebookStringCreator {
public:
    PhonebookStringCreator(std::shared_ptr<Adapter> phonebook_adapter);
    virtual ~PhonebookStringCreator() = default;

    /// @brief Returns a phonebook string compatible with a deskphone.
    ///
    /// @return String containing phonebook string compatible with a deskphone.
    virtual std::string getPhonebookString() = 0;

protected:
    /// @brief Returns the phonebook adapter for this object.
    ///
    /// @return Pointer to the phonebook adapter for this object.
    std::shared_ptr<Adapter> getPhonebookAdapter();

private:
    std::shared_ptr<Adapter> phonebook_adapter_; ///< Pointer to source phonebook adapter.
};

} // namespace phonebook

#endif
