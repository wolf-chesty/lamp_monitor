// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef LAMP_STATE_LAMP_FIELD_HPP
#define LAMP_STATE_LAMP_FIELD_HPP

#include "lamp_state/PhoneButton.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace lamp_state {

class LampField : public std::enable_shared_from_this<LampField> {
public:
    LampField() = default;
    virtual ~LampField() = default;

    std::shared_ptr<PhoneButton> getButton(uint16_t button_id, PhoneButton::Color color, bool flash);

    std::vector<std::shared_ptr<PhoneButton>> getButtons();

    void invalidate(uint16_t const button_id);

private:
    std::unordered_map<uint16_t, std::shared_ptr<PhoneButton>> buttons_;
    std::mutex buttons_mut_;
};

} // namespace lamp_state

#endif
