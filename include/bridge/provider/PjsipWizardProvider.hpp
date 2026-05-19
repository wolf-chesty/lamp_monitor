// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef BRIDGE_PROVIDER_PJSIP_WIZARD_PROVIDER_HPP
#define BRIDGE_PROVIDER_PJSIP_WIZARD_PROVIDER_HPP

#include "bridge/AorProvider.hpp"

#include "asterisk/config/PjsipWizardConfig.hpp"
#include "match/JsonExpressionMatcher.hpp"
#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace bridge::provider {

class PJSIPWizardProvider : public bridge::AoRProvider {
public:
    explicit PJSIPWizardProvider(std::shared_ptr<cpp_ami::Connection> io_conn, std::string match,
                                match::SymbolMap symbol_map);
    ~PJSIPWizardProvider() override = default;

    static std::shared_ptr<PJSIPWizardProvider> create(YAML::Node const &config,
                                                      std::shared_ptr<cpp_ami::Connection> const &conn);

    std::unordered_set<std::string> getCompatibleAoRs() override;

private:
    asterisk::config::PJSIPWizardConfig config_;   ///< Asterisk pjsip_wizard.conf file reader.
    match::JSONExpressionMatcher<double> matcher_; ///< JSON matcher.
};

} // namespace bridge::adapter

#endif
