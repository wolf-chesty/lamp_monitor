// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/adapter/PjsipWizardAdapter.hpp"

#include <c++ami/action/GetConfigJson.hpp>
#include <regex>
#include <syslog.h>
#include <yaml-cpp/yaml.h>

using namespace phonebook::adapter;

PJSIPWizardAdapter::PJSIPWizardAdapter(std::shared_ptr<cpp_ami::Connection> io_conn, std::string match,
                                       std::unordered_map<std::string, std::string> symbol_map)
    : config_(std::move(io_conn))
    , matcher_(std::move(match), std::move(symbol_map))
{
}

std::shared_ptr<PJSIPWizardAdapter> PJSIPWizardAdapter::create(YAML::Node const &config,
                                                               std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const match = config["match"].as<std::string>();

    std::unordered_map<std::string, std::string> symbol_map;
    for (auto const &itr : config["symbol_maps"]) {
        auto const symbol = itr["symbol"].as<std::string>();
        auto const key = itr["key"].as<std::string>();
        auto const [_, success] = symbol_map.emplace(symbol, key);
        if (!success) {
            syslog(LOG_WARNING, "Duplicate symbol %s found", symbol.c_str());
        }
    }

    return std::make_shared<PJSIPWizardAdapter>(conn, match, symbol_map);
}

/// Parses the Asterisk pjsip_wizard.conf file, returning the caller ID information for each AOR endpoint found.
/// Endpoints are filtered by the template setting.
std::vector<phonebook::CallerIDInfo> PJSIPWizardAdapter::getPhonebookDetails()
{
    std::vector<phonebook::CallerIDInfo> phonebook_details;
    config_.process([this, &phonebook_details](YAML::Node const &aor_cfg_json) mutable -> void {
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
            phonebook_details.emplace_back(matches[1].str(), matches[2].str());
        }
    });
    return phonebook_details;
}
