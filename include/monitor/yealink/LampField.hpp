// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef MONITOR_YEALINK_LAMP_FIELD_HPP
#define MONITOR_YEALINK_LAMP_FIELD_HPP

#include "lamp_state/LampFieldObserver.hpp"

#include "DeskphoneCache.hpp"
#include "LampFieldState.hpp"
#include <atomic>
#include <c++ami/Connection.hpp>
#include <c++ami/util/KeyValDict.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace monitor::yealink {

class LampField : public lamp_state::LampFieldObserver {
public:
    explicit LampField(std::shared_ptr<lamp_state::LampField> lamp_field,
                       std::shared_ptr<DeskphoneCache> deskphone_cache, std::shared_ptr<cpp_ami::Connection> io_conn);
    ~LampField() override;

    static pugi::xml_document createPhoneStateXML(std::vector<std::shared_ptr<lamp_state::PhoneButton>> const &buttons);

    std::string getPhoneStateXMLString();

    void invalidate(std::vector<std::shared_ptr<lamp_state::PhoneButton>> const &buttons) override;

private:
    void amiEventHandler(cpp_ami::util::KeyValDict const &event);

    static char const *toColorString(lamp_state::PhoneButton::Color const color);
    static char const *toButtonStateString(lamp_state::PhoneButton const &button);

    static pugi::xml_document createPhoneStateXML(std::vector<std::shared_ptr<lamp_state::PhoneButton>> const &buttons,
                                                  bool &critical);

    void startWorkThread();
    void stopWorkThread();
    void workThread();

    std::shared_ptr<LampFieldState> getCachedPhoneState();
    void setCachedPhoneState(std::shared_ptr<LampFieldState> const &button_state);

    void publishButtonState(std::string const &aor, std::string const &button_state_xml);
    void publishButtonState(std::string const &button_state_xml);

    std::shared_ptr<DeskphoneCache> deskphone_cache_;
    std::shared_ptr<cpp_ami::Connection> io_conn_;
    cpp_ami::Connection::event_callback_key_t ami_callback_id_; ///< AMI callback handler ID.
    std::shared_ptr<LampFieldState> cached_button_state_;
    std::mutex cached_button_state_mut_;
    std::atomic<bool> button_state_valid_{true};
    std::thread work_thread_;
    std::atomic<bool> work_thread_run_{};
    std::condition_variable work_thread_cv_{};
};

} // namespace monitor::yealink

#endif
