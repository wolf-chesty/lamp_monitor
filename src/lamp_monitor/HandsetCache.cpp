// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "lamp_monitor/HandsetCache.hpp"

bool HandsetCache::aorNeedsUpdate(std::string const &aor)
{
    std::lock_guard const lock(valid_aors_mut_);
    auto inserted_aor = valid_aors_.insert(aor);
    return inserted_aor.second;
}

void HandsetCache::clearValidAORs()
{
    std::lock_guard const lock(valid_aors_mut_);
    valid_aors_.clear();
}

void HandsetCache::invalidateAOR(std::string const &aor)
{
    std::lock_guard const lock(valid_aors_mut_);
    valid_aors_.erase(aor);
}
