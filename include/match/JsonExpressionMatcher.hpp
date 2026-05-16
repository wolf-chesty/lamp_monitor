// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MATCH_JSON_EXPRESSION_MATCHER_HPP
#define MATCH_JSON_EXPRESSION_MATCHER_HPP

#include "match/JsonUnknownSymbolResolver.hpp"
#include "match/SymbolMap.hpp"
#include <exprtk.hpp>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace match {

///
/// @tparam T Numeric type.
/// @class JSONExpressionMatcher
/// @namespace match
///
/// @brief Class that evaluates a JSON document against a matching expression.
///
template<Numeric T> class JSONExpressionMatcher {
public:
    using symbol_table_t = exprtk::symbol_table<T>;
    using expression_t = exprtk::expression<T>;
    using parser_t = exprtk::parser<T>;

public:
    JSONExpressionMatcher(std::string match, SymbolMap::symbol_map_t symbol_map)
        : match_(std::move(match))
        , symbol_map_(std::move(symbol_map))
    {
        // Initialize symbol table
        symbol_table_.add_constants();
        // Initialize expression
        expression_.register_symbol_table(symbol_table_);
    }

    ~JSONExpressionMatcher() = default;

    /// @brief Returns \c true if this YAML node matches the match expression.
    ///
    /// @param node Node to check.
    bool isMatch(YAML::Node const &node)
    {
        return is_valid_ast_ ? evaluate(node) : compile(node);
    }

private:
    /// @brief Compiles the match statement returning \c true if \c node matches the match statement.
    ///
    /// @param doc Node to check for match.
    ///
    /// @return \c true if \c node matches the match expression.
    ///
    /// This function will compile the statement populating the symbol table with symbols that are found in the
    /// expression. If the expression successfully compiles and all the symbols found this function will cache the AST
    /// so that future YAML nodes can be evaluated without having to recompile the match expression.
    bool compile(YAML::Node const &doc)
    {
        // Setup compiler
        exprtk::parser<T> parser;
        JSONUnknownSymbolResolver<T> usr(doc, symbol_map_, string_table_);
        parser.enable_unknown_symbol_resolver(&usr);
        // Compile statement
        if (is_valid_ast_ = parser.compile(match_, expression_); !is_valid_ast_) {
            return false;
        }
        // Expression may contain values from previous YAML doc; evaluate current YAML doc
        return evaluate(doc);
    }

    /// @brief Evaluates \c node against the cached abstract syntax tree returning \c true if \c node matches the match
    ///        statement.
    ///
    /// @param doc Node to check for match.
    ///
    /// @return \c true if \c node matches the match expression.
    bool evaluate(YAML::Node const &doc)
    {
        for (auto &[symbol, value] : string_table_) {
            auto const mapped_symbol = symbol_map_.getSymbol(symbol);
            auto const &node = doc[mapped_symbol];
            if (!node) {
                return false;
            }
            value = node.template as<std::string>();
        }
        auto const eval = expression_.value();
        return eval != 0.0;
    }

    std::string match_;           ///< Expression used to match YAML nodes.
    symbol_table_t symbol_table_; ///< Cached symbol table.
    expression_t expression_;     ///< Cached AST used to evaluate expression values.
    bool is_valid_ast_{false};    ///< Flag indicating if the expression was successfully parsed.
    SymbolMap symbol_map_;        ///< Symbol map.
    std::unordered_map<std::string, std::string> string_table_; ///< Table of string values.
};

} // namespace phonebook::adapter

#endif
