// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MATCH_JSON_UNKNOWN_SYMBOL_RESOLVER_HPP
#define MATCH_JSON_UNKNOWN_SYMBOL_RESOLVER_HPP

#include "match/SymbolMap.hpp"
#include <exprtk.hpp>
#include <fmt/core.h>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace match {

template<typename T>
concept Numeric = std::same_as<T, float> || std::same_as<T, double>;

/// @tparam T
/// @class JSONUnknownSymbolResolver
/// @namespace match
///
/// @brief Class used to load a symbol table from an equation containing unknown symbols.
///
/// The match equations used to identify items in the software are specified by the end user. Since the user creates
/// these matching statements the symbols aren't known to the application. This class allows for the application to
/// determine the symbols in the equation at runtime.
template<Numeric T> class JSONUnknownSymbolResolver : public exprtk::parser<T>::unknown_symbol_resolver {
public:
    using symbol_table_t = exprtk::symbol_table<T>;
    using usr_t = exprtk::parser<T>::unknown_symbol_resolver;

public:
    explicit JSONUnknownSymbolResolver(YAML::Node const &node, SymbolMap const &symbol_map,
                                       std::unordered_map<std::string, std::string> &string_table)
        : usr_t(usr_t::e_usrmode_extended)
        , node_(node)
        , symbol_map_(symbol_map)
        , string_table_(string_table)
    {
    }

    ~JSONUnknownSymbolResolver() override = default;

    /// @brief This function is invoked whenever the parser encounters a symbol in the equation that is not defined in
    ///        the symbol table.
    ///
    /// @param symbol Symbol name.
    /// @param symbol_table Symbol table to store the symbol value.
    /// @param error_message Error message to populate when the symbol cannot be resolved.
    ///
    /// @return \c true if the symbol was added to the symbol table.
    bool process(std::string const &symbol, symbol_table_t &symbol_table, std::string &error_message) override
    {
        // Check JSON for the key/value pair
        auto const &mapped_symbol = symbol_map_.getSymbol(symbol);
        auto const &node = node_[mapped_symbol];
        if (!node) {
            error_message = fmt::format("JSON doesn't have key {}", symbol);
            return false;
        }

        // Symbol table is a collection of references; we need to keep track of the actual variables that were added to
        // the symbol table.
        string_table_[symbol] = node.template as<std::string>();
        if (!symbol_table.get_stringvar(symbol)) {
            symbol_table.add_stringvar(symbol, string_table_[symbol]);
        }
        return true;
    }

private:
    YAML::Node const &node_;      ///< Reference to YAML node containing symbol table values.
    SymbolMap const &symbol_map_; ///< Map of symbols to JSON keys.
    std::unordered_map<std::string, std::string> &string_table_; ///< Table of string vals.
};

} // namespace match

#endif
