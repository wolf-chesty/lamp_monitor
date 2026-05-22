// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_PROVIDER_PJSIP_WIZARD_PROVIDER_HPP
#define PHONEBOOK_PROVIDER_PJSIP_WIZARD_PROVIDER_HPP

#include "phonebook/PhonebookProvider.hpp"

#include "asterisk/config/PjsipWizardConfig.hpp"
#include "match/JsonExpressionMatcher.hpp"
#include <c++ami/Connection.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace phonebook::provider {

/// @class PJSIPWizardProvider
/// @namespace phonebook::provider
///
/// @brief Retrieves caller ID details from the Asterisk pjsip_wizard.conf configuration file.
///
/// This class parses the Asterisk pjsip_wizard.conf configuration file for a list of all deskphones registered to the
/// system. This class will generate a list of caller ID records to be used with online phonebooks.
class PJSIPWizardProvider : public phonebook::PhonebookProvider {
public:
    explicit PJSIPWizardProvider(std::shared_ptr<cpp_ami::Connection> io_conn, std::string match,
                                 match::SymbolMap symbol_map);
    ~PJSIPWizardProvider() override = default;

    /// @brief Creates a new object using configuration parameters from \c config.
    ///
    /// @param config Configuration parameters for object.
    /// @param conn Pointer Asterisk AMI connection.
    ///
    /// @return Pointer to new adapter object.
    static std::shared_ptr<PJSIPWizardProvider> create(YAML::Node const &config,
                                                       std::shared_ptr<cpp_ami::Connection> const &conn);

    /// @brief Returns a collection of caller ID details.
    ///
    /// @return Collection of caller ID details.
    std::vector<CallerIDDetails> getCallerDetails() override;

private:
    asterisk::config::PJSIPWizardConfig config_;   ///< Asterisk pjsip_wizard.conf file reader.
    match::JSONExpressionMatcher<double> matcher_; ///< JSON matcher.
};

} // namespace phonebook::provider

#endif
