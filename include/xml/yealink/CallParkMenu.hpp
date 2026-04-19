// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef XML_YEALINK_CALL_PARK_MENU_HPP
#define XML_YEALINK_CALL_PARK_MENU_HPP

#include "ui/HttpParkButton.hpp"

#include <c++ami/Connection.hpp>
#include <memory>
#include <string>

namespace xml::yealink {

/// @class CallParkMenu
/// @namespace xml::yealink
///
/// @brief This class will create a call park menu for Yealink IP deskphones.
///
/// This object can interact with the Asterisk server and return a list of parked phone calls that can be retrieved from
/// parking using XML compatible with Yealink Android based deskphones. Objects can also create screens to view the
/// details of parked calls.
class CallParkMenu : public ui::HTTPParkButton {
public:
    explicit CallParkMenu(std::shared_ptr<cpp_ami::Connection> io_conn, std::string parking_lot,
                          std::string parked_call_info_uri);
    ~CallParkMenu() override = default;

    /// @brief Creates an XML browser string containing a YealinkIPPhoneTextScreen body to display on a deskphone.
    ///
    /// @param beep Flag indicating if the IP deskphone should beep when displaying the text screen window.
    /// @param timeout Duration of the text screen window.
    /// @param title Title of the text screen window.
    /// @param text Text to display in the text screen window.
    ///
    /// @return XML browser string to be displayed by a Yealink IP deskphone.
    static std::string createMessageXML(bool const beep, uint8_t const timeout, std::string const &title,
                                        std::string const &text);

    /// @brief Creates an XML string containing Yealink XML browser text to display on a deskphone.
    ///
    /// @return XML browser string to be displayed by a Yealink IP deskphone.
    std::string httpPushButton() const override;

    /// @brief Creates an XML string containing Yealink XML browser text to display on a deskphone.
    ///
    /// @param park_exten Parking extension to display details for.
    ///
    /// @return XML browser string to be displayed on a Yealink IP deskphone.
    ///
    /// This function will create an XML browser string containing details of the call parked at \c park_exten.
    std::string httpPushButton(std::string const &park_exten) const override;

    /// @brief Displays an error message on the desktop phone.
    ///
    /// @param title Title for error message.
    /// @param text Text to display in error message.
    ///
    /// @return Error message formatted for display on the desktop phone.
    std::string displayErrorMessage(std::string const &title, std::string const &text) const override;

    /// @brief HTTP content type for HTTP messages produced by this object.
    ///
    /// @return HTTP content type for text produced by this object.
    std::string getContentType() const override;

private:
    /// @brief Creates an XML string containing Yealink XML browser text to display on a deskphone.
    ///
    /// @param parked_call_list AMI event containing a list of the parked calls on the Asterisk system.
    ///
    /// @return XML browser string to be displayed on a Yealink IP deskphone.
    ///
    /// This function will process the list of parked calls found in the \c parked_call_list object, creating an XML
    /// browser body representing all the parked calls.
    std::string createParkedCallMenu(cpp_ami::reaction::EventList const &parked_call_list) const;

    /// @brief Creates an XML string containing Yealink XML browser text to display on a deskphone.
    ///
    /// @return XML browser string to be displayed on a Yealink IP deskphone.
    ///
    /// In the event that there are no parked calls present, this call can be called to return an XML browser string
    /// indicating that there are no parked calls present.
    static std::string createNoParkedCallMessage();

    std::shared_ptr<cpp_ami::Connection> io_conn_; ///< Pointer to AMI Asterisk connection.
    std::string parking_lot_;                      ///< Parking lot to get calls for.
    std::string parked_call_info_uri_;             ///< URI for parked call info.
};

} // namespace xml::yealink

#endif
