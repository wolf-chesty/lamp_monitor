// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/adapter/PjsipWizardAdapter.hpp"

#include <c++ami/action/GetConfigJson.hpp>
#include <cassert>
#include <regex>
#include <yaml-cpp/yaml.h>

using namespace phonebook::adapter;

PJSIPWizardAdapter::PJSIPWizardAdapter(std::shared_ptr<cpp_ami::Connection> io_conn, std::string filter)
    : io_conn_(std::move(io_conn))
    , filter_(std::move(filter))
{
}

std::shared_ptr<PJSIPWizardAdapter> PJSIPWizardAdapter::create(std::shared_ptr<cpp_ami::Connection> const &conn,
                                                               YAML::Node const &config)
{
    assert(config["type"].as<std::string>() == "wizard");
    return std::make_shared<PJSIPWizardAdapter>(conn, config["filter"].as<std::string>());
}

/// Parses the Asterisk pjsip_wizard.conf file, returning the caller ID information for each AOR endpoint found.
/// Endpoints are filtered by the template setting.
std::vector<phonebook::CallerIDInfo> PJSIPWizardAdapter::getPhonebookDetails() const
{
    std::vector<CallerIDInfo> phonebook_details;

    cpp_ami::action::GetConfigJSON action;
    action["Filename"] = "pjsip_wizard.conf";
    if (auto const reaction = io_conn_->invoke(action); reaction->isSuccess()) {
        reaction->forEach([this, &phonebook_details](cpp_ami::event::Event const &event) mutable -> bool {
            auto const cfg_yaml = YAML::Load(event["JSON"]);
            for (auto const endpoint : cfg_yaml) {
                auto const &aor_cfg_yaml = endpoint.second;
                auto const type_node = aor_cfg_yaml["templates"];
                if (!type_node) {
                    continue;
                }
                if (type_node.as<std::string>() != filter_) {
                    continue;
                }

                auto const caller_id_node = aor_cfg_yaml["endpoint/callerid"];
                if (!caller_id_node) {
                    continue;
                }
                if (auto const caller_id = caller_id_node.as<std::string>(); !caller_id.empty()) {
                    std::regex re_pattern("\"([^\"]*)\" <([0-9]*)>");
                    std::smatch matches;
                    std::regex_search(caller_id, matches, re_pattern);
                    phonebook_details.emplace_back(matches[1].str(), matches[2].str());
                }
            }
            return true;
        });
    }

    return phonebook_details;
}
