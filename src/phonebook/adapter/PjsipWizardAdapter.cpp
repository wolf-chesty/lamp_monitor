// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "phonebook/adapter/PjsipWizardAdapter.hpp"

#include <c++ami/action/GetConfigJson.hpp>
#include <cassert>
#include <exprtk.hpp>
#include <regex>
#include <syslog.h>
#include <yaml-cpp/yaml.h>

using namespace phonebook::adapter;

PJSIPWizardAdapter::PJSIPWizardAdapter(std::shared_ptr<cpp_ami::Connection> io_conn, std::string context)
    : io_conn_(std::move(io_conn))
    , context_(std::move(context))
{
}

std::shared_ptr<PJSIPWizardAdapter> PJSIPWizardAdapter::create(YAML::Node const &config,
                                                               std::shared_ptr<cpp_ami::Connection> const &conn)
{
    return std::make_shared<PJSIPWizardAdapter>(conn, config["context"].as<std::string>());
}

std::vector<phonebook::CallerIDInfo> processWizardFile(cpp_ami::EventDispatcher::reaction_t const &reaction,
                                                       std::string const &context)
{
    std::vector<phonebook::CallerIDInfo> phonebook_details;

    reaction.forEach([context, &phonebook_details](cpp_ami::event::Event const &event) mutable -> bool {
        try {
            auto const cfg_yaml = YAML::Load(event["JSON"]);
            for (auto const endpoint : cfg_yaml) {
                auto const &aor_cfg_yaml = endpoint.second;
                auto const type_node = aor_cfg_yaml["endpoint/context"];
                if (!type_node) {
                    continue;
                }
                if (type_node.as<std::string>() != context) {
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
        }
        catch (std::exception const &e) {
            syslog(LOG_ERR, "Unable to parse pjsip_wizard.conf");
        }
        return true;
    });

    return phonebook_details;
}

/// Parses the Asterisk pjsip_wizard.conf file, returning the caller ID information for each AOR endpoint found.
/// Endpoints are filtered by the template setting.
std::vector<phonebook::CallerIDInfo> PJSIPWizardAdapter::getPhonebookDetails() const
{
    cpp_ami::action::GetConfigJSON action;
    action["Filename"] = "pjsip_wizard.conf";
    if (auto const reaction = io_conn_->invoke(action); reaction->isSuccess()) {
        return processWizardFile(*reaction, context_);
    }
    return std::vector<phonebook::CallerIDInfo>{};
}
