// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "match/SymbolMap.hpp"

using namespace match;

SymbolMap::SymbolMap(symbol_map_t symbol_map)
    : symbol_map_(std::move(symbol_map))
{
}

std::string const &SymbolMap::getSymbol(std::string const &symbol) const
{
    auto const itr = symbol_map_.find(symbol);
    if (itr == symbol_map_.end()) {
        return symbol;
    }
    return itr->second;
}
