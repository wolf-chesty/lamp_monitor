// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/provider/PjsipWizardProvider.hpp"

#include <regex>
#include <syslog.h>
#include <yaml-cpp/yaml.h>

using namespace phonebook::provider;

PJSIPWizardProvider::PJSIPWizardProvider(std::shared_ptr<cpp_ami::Connection> io_conn, std::string match,
                                         match::SymbolMap symbol_map)
    : config_(std::move(io_conn))
    , matcher_(std::move(match), std::move(symbol_map))
{
}

std::shared_ptr<PJSIPWizardProvider> PJSIPWizardProvider::create(YAML::Node const &config,
                                                                 std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const match = config["match"].as<std::string>();
    auto const symbol_map = match::SymbolMap::create(config["symbol_maps"]);
    return std::make_shared<PJSIPWizardProvider>(conn, match, symbol_map);
}

/// Parses the Asterisk pjsip_wizard.conf file, returning the caller ID information for each AOR endpoint found.
std::vector<phonebook::PhonebookProvider::CallerIDDetails> PJSIPWizardProvider::getCallerDetails()
{
    std::vector<CallerIDDetails> phonebook_data;
    config_.process([this, &phonebook_data](YAML::Node const &aor_cfg_json) mutable -> void {
        if (!matcher_.isMatch(aor_cfg_json)) {
            return;
        }

        auto const &caller_id_node = aor_cfg_json["endpoint/callerid"];
        // Missing caller ID node; move onto next AoR
        if (!caller_id_node) {
            return;
        }
        // Parse out caller ID details
        if (auto const caller_id = caller_id_node.as<std::string>(); !caller_id.empty()) {
            std::regex re_pattern("\"([^\"]*)\" <([0-9]*)>");
            std::smatch matches;
            std::regex_search(caller_id, matches, re_pattern);
            phonebook_data.emplace_back(matches[1].str(), matches[2].str());
        }
    });
    return phonebook_data;
}
