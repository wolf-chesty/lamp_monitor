// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "monitor/yealink/LampField.hpp"

#include <c++ami/action/PjsipNotify.hpp>
#include <cassert>
#include <fmt/core.h>

using namespace monitor::yealink;

LampField::LampField(std::shared_ptr<lamp_state::LampField> lamp_field, std::shared_ptr<DeskphoneCache> deskphone_cache,
                     std::shared_ptr<cpp_ami::Connection> io_conn)
    : LampFieldObserver(std::move(lamp_field))
    , deskphone_cache_(std::move(deskphone_cache))
    , io_conn_(std::move(io_conn))
{
    assert(deskphone_cache_);

    assert(io_conn_);
    ami_callback_id_ =
        io_conn_->addCallback([this](cpp_ami::util::KeyValDict const &event) -> void { amiEventHandler(event); });

    startWorkThread();
}

LampField::~LampField()
{
    assert(io_conn_);
    io_conn_->removeCallback(ami_callback_id_);

    stopWorkThread();
}

/// This callback is invoked for every AMI event that is published by the Asterisk server. This callback will process
/// SuccessfulAuth events from the PJSIP service. This event type is published every time a phone successfully registers
/// with the Asterisk server.
///
/// Upon successful registration of a deskphone this callback will publish the current lamp states to the phone.
void LampField::amiEventHandler(cpp_ami::util::KeyValDict const &event)
{
    // Filter for monitored events
    auto const event_type = event.getValue("Event");
    if (!event_type) {
        return;
    }
    static std::unordered_set<std::string> const valid_events{"SuccessfulAuth"};
    if (!valid_events.contains(event_type.value())) {
        return;
    }

    // Android based Yealink deskphones will wake the screen whenever they receive a PJSIP notify message. This
    // can cause wear on the backlight mechanism of the deskphone. Make sure to only publish the phone state if
    // the phone wasn't present for the lamp state change or the state requires the screen to be on (i.e., in
    // order to catch the users attention).
    if (event["Service"] == "PJSIP") {
        auto const &aor = event["AccountID"];
        auto const state = getCachedPhoneState();
        if (deskphone_cache_->addEndpoint(aor, event["RemoteAddress"]) || state->isCritical()) {
            publishButtonState(aor, state->toString());
        }
    }
}

std::string LampField::getPhoneStateXMLString()
{
    auto const state = getCachedPhoneState();
    return state->toString();
}

void LampField::invalidate([[maybe_unused]] std::vector<std::shared_ptr<lamp_state::PhoneButton>> const &buttons)
{
    button_state_valid_ = false;
    work_thread_cv_.notify_one();
}

char const *LampField::toColorString(lamp_state::PhoneButton::Color const color)
{
    static constexpr char const *const BUTTON_COLOR_RED = "RED";
    static constexpr char const *const BUTTON_COLOR_GREEN = "GREEN";
    static constexpr char const *const BUTTON_COLOR_BLUE = "BLUE";

    char const *button_color;
    switch (color) {
    case lamp_state::PhoneButton::Color::Red:
        button_color = BUTTON_COLOR_RED;
        break;
    case lamp_state::PhoneButton::Color::Green:
        button_color = BUTTON_COLOR_GREEN;
        break;
    case lamp_state::PhoneButton::Color::Blue:
        button_color = BUTTON_COLOR_BLUE;
        break;
    default:
        assert(false);
    }
    return button_color;
}

char const *LampField::toButtonStateString(lamp_state::PhoneButton const &button)
{
    static constexpr char const *const BUTTON_STATE_OFF = "off";
    static constexpr char const *const BUTTON_STATE_ON = "on";
    static constexpr char const *const BUTTON_STATE_FLASH = "slowflash";

    char const *button_state = BUTTON_STATE_OFF;
    if (button.isOn()) {
        button_state = button.flash() ? BUTTON_STATE_FLASH : BUTTON_STATE_ON;
    }
    return button_state;
}

pugi::xml_document LampField::createPhoneStateXML(std::vector<std::shared_ptr<lamp_state::PhoneButton>> const &buttons)
{
    bool critical = false;
    return createPhoneStateXML(buttons, critical);
}

pugi::xml_document LampField::createPhoneStateXML(std::vector<std::shared_ptr<lamp_state::PhoneButton>> const &buttons,
                                                  bool &critical)
{
    assert(!buttons.empty());

    pugi::xml_document xml_doc;
    auto decl = xml_doc.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "ISO-8859-1";

    auto execute_xml = xml_doc.append_child("YealinkIPPhoneExecute");
    execute_xml.append_attribute("refresh") = "1";

    for (auto const &button : buttons) {
        critical |= button->isCritical();

        auto button_xml = execute_xml.append_child("ExecuteItem");
        button_xml.append_attribute("URI") = fmt::format("Led:LINE{}_{}={}", button->buttonID(),
                                                         toColorString(button->color()), toButtonStateString(*button));
    }
    execute_xml.append_attribute("Beep") = critical ? "yes" : "no";

    return xml_doc;
}

void LampField::startWorkThread()
{
    work_thread_run_ = true;
    work_thread_ = std::thread(&LampField::workThread, this);

    pthread_setname_np(work_thread_.native_handle(), "lamp_field");
}

void LampField::stopWorkThread()
{
    work_thread_run_ = false;
    work_thread_cv_.notify_one();

    assert(work_thread_.joinable());
    work_thread_.join();
}

void LampField::workThread()
{
    std::mutex button_state_mut;
    while (work_thread_run_) {
        std::unique_lock lock(button_state_mut);
        work_thread_cv_.wait(lock, [this]() -> bool { return !work_thread_run_ || button_state_valid_; });
        lock.unlock();

        if (!button_state_valid_.exchange(true) || work_thread_run_) {
            // Create current phone button state XML
            bool critical = false;
            auto xml_doc = createPhoneStateXML(getButtons(), critical);
            // Create new phone state and cache it
            auto state = std::make_shared<LampFieldState>(std::move(xml_doc), critical);
            setCachedPhoneState(state);
            // Publish button state to deskphones
            publishButtonState(state->toString());
        }
    }
}

std::shared_ptr<LampFieldState> LampField::getCachedPhoneState()
{
    std::lock_guard const lock(cached_button_state_mut_);
    return cached_button_state_;
}

void LampField::setCachedPhoneState(std::shared_ptr<LampFieldState> const &lamp_field_state)
{
    std::lock_guard const lock(cached_button_state_mut_);
    cached_button_state_ = lamp_field_state;
}

void LampField::publishButtonState(std::string const &aor, std::string const &button_state_xml)
{
    cpp_ami::action::PJSIPNotify action;
    action["Endpoint"] = aor;
    action.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});
    io_conn_->asyncInvoke(action);
}

void LampField::publishButtonState(std::string const &button_state_xml)
{
    cpp_ami::action::PJSIPNotify action;
    action.setValues("Variable", {"Event=Yealink-xml", "Content-Type=application/xml",
                                  fmt::format("Content={}", cpp_ami::util::KeyValDict::escape(button_state_xml))});

    deskphone_cache_->forEachAOR([this, action](std::string_view aor) mutable -> void {
        action["Endpoint"] = aor;
        io_conn_->asyncInvoke(action);
    });
}
