// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "match/SymbolMap.hpp"

#include <syslog.h>

using namespace match;

SymbolMap::SymbolMap(symbol_map_t symbol_map)
    : symbol_map_(std::move(symbol_map))
{
}

SymbolMap SymbolMap::create(YAML::Node const &config)
{
    std::unordered_map<std::string, std::string> symbol_map;
    for (auto const &itr : config) {
        auto const symbol = itr["symbol"].as<std::string>();
        auto const key = itr["key"].as<std::string>();
        auto const [_, success] = symbol_map.emplace(symbol, key);
        if (!success) {
            syslog(LOG_WARNING, "Duplicate symbol %s found", symbol.c_str());
        }
    }
    return SymbolMap(symbol_map);
}

std::string const &SymbolMap::getSymbol(std::string const &symbol) const
{
    auto const itr = symbol_map_.find(symbol);
    if (itr == symbol_map_.end()) {
        return symbol;
    }
    return itr->second;
}
