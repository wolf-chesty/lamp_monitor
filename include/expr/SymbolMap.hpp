// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_ADAPTER_SYMBOL_MAP_HPP
#define PHONEBOOK_ADAPTER_SYMBOL_MAP_HPP

#include <string>
#include <unordered_map>

namespace phonebook::adapter {

///
/// @class SymbolMap
/// @namespace phonebook::adapter
///
/// @brief Container of mapped symbols.
///
/// \c exprtk doesn't allow certain characters in variable names. This class maps \c exprtk compatible symbol names to
/// their actual symbol names in structures like YAML, which can contain characters like forward-slash (/) which is
/// incompatible with \c exprtk.
///
class SymbolMap {
public:
    using symbol_map_t = std::unordered_map<std::string, std::string>;

public:
    explicit SymbolMap(std::unordered_map<std::string, std::string> symbol_map);

    /// @brief This function will return a YAML key that is mapped to \c symbol.
    ///
    /// @param symbol Symbol name used in the exprtk expression.
    ///
    /// @return YAML value key.
    ///
    /// exprtk doesn't support forward-slashes (/) in variable names but some of the keys in the Asterisk configuration
    /// files does contain them. The user has some configuration options that will allow them to substitute symbols that
    /// are compatible with exprtk and the associated YAML key value.
    std::string const &getSymbol(std::string const &symbol) const;

private:
    symbol_map_t symbol_map_; ///< Collection of mapped symbol names.
};

} // namespace phonebook::adapter

#endif
