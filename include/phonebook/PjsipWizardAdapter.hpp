// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_PJSIP_WIZARD_ADAPTER_HPP
#define PHONEBOOK_PJSIP_WIZARD_ADAPTER_HPP

#include "phonebook/Adapter.hpp"

#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace phonebook {

/// @class PJSIPWizardAdapter
/// @namespace phonebook
///
/// @brief Retrieves caller ID details from the Asterisk pjsip_wizard.conf configuration file.
///
/// This class parses the Asterisk pjsip_wizard.conf configuration file for a list of all deskphones registered to the
/// system. This class will generate a list of caller ID records to be used with online phonebooks.
class PJSIPWizardAdapter : public Adapter {
public:
    explicit PJSIPWizardAdapter(std::shared_ptr<cpp_ami::Connection> io_conn, std::string filter);
    ~PJSIPWizardAdapter() override = default;

    /// @brief Returns a collection of caller ID details.
    std::vector<CallerIDInfo> getPhonebookDetails() const override;

private:
    std::shared_ptr<cpp_ami::Connection> io_conn_; ///< Pointer to Asterisk AMI server.
    std::string filter_;                           ///< Filter for pjsip_wizard clients to grab caller ID details for.
};

} // namespace phonebook

#endif
