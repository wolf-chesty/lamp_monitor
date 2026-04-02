// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef HANDSET_CACHE_HPP
#define HANDSET_CACHE_HPP

#include <unordered_set>
#include <mutex>
#include <string>

class HandsetCache {
public:
    HandsetCache() = default;
    HandsetCache(HandsetCache const &) = default;
    HandsetCache(HandsetCache &&) noexcept = default;
~HandsetCache() = default;

    bool aorNeedsUpdate(std::string const &aor);
    void clearValidAORs();
    void invalidateAOR(std::string const &aor);

private:
    std::unordered_set<std::string> valid_aors_;
    std::mutex valid_aors_mut_;
};

#endif
