// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#include "ConfigFileParser.hpp"

#include "asterisk/NightEventHandler.hpp"
#include "asterisk/ParkEventHandler.hpp"
#include "button_state/ButtonPlan.hpp"
#include "cache/DeskphoneCache.hpp"
#include "phonebook/HTTPPhonebook.hpp"
#include "ui/HttpNightButton.hpp"
#include "ui/HttpParkButton.hpp"
#include "ui/HttpStateButton.hpp"
#include "ui/PhoneEventDispatcher.hpp"
#include "ui/PhoneUi.hpp"
#include <c++ami/action/Login.hpp>
#include <syslog.h>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <yaml-cpp/yaml.h>

void configureSyslog(YAML::Node const &config, std::string_view app_name, bool const is_daemon)
{
    int options = 0;
    options |= (!is_daemon && config["console"].as<bool>() ? LOG_CONS : 0);
    openlog(app_name.data(), options, LOG_USER);

    auto const log_level = config["level"].as<int>();
    setlogmask(LOG_UPTO(log_level));
}

std::shared_ptr<cpp_ami::Connection> createAMIConnection(YAML::Node const &config, std::string_view filter)
{
    // Create Asterisk AMI connection
    auto const &hostname{config["host"].as<std::string>()};
    auto const &port{config["port"].as<uint16_t>()};
    auto const conn = std::make_shared<cpp_ami::Connection>(hostname, port);

    // Login to Asterisk AMI server
    auto const &username = config["username"].as<std::string>();
    auto const secret{config["secret"].as<std::string>()};
    cpp_ami::action::Login login(username, secret);
    login["AuthType"] = "plain";
    login["Events"] = filter;
    // Send login to AMI server
    auto const reaction{conn->invoke(login)};
    // Confirm login
    if (!reaction->isSuccess()) {
        syslog(LOG_ERR, "Unable to login to AMI");
        exit(EXIT_FAILURE);
    }

    return conn;
}

std::tuple<std::unique_ptr<httplib::Server>, std::string, uint16_t> createHTTPServer(YAML::Node const &config)
{
    auto http_server = std::make_unique<httplib::Server>();
    auto const &host = config["addr"].as<std::string>();
    auto const &port = config["port"].as<uint16_t>();

    return std::make_tuple(std::move(http_server), host, port);
}

void createPhonebooks(YAML::Node const &config, httplib::Server &http_server,
                      std::shared_ptr<cpp_ami::Connection> const &conn)
{
    std::unordered_map<std::string, std::shared_ptr<phonebook::Adapter>> adapters;
    auto create_adapter = [&conn, &config,
                           &adapters](std::string const &name) mutable -> std::shared_ptr<phonebook::Adapter> {
        if (auto const itr = adapters.find(name); itr != adapters.end()) {
            return itr->second;
        }

        for (auto const adapter_yaml : config["adapters"]) {
            if (adapter_yaml["name"].as<std::string>() == name) {
                auto adapter = phonebook::Adapter::create(adapter_yaml, conn);
                adapters.emplace(name, adapter);
                return adapter;
            }
        }

        return nullptr;
    };

    for (auto const &phonebook_config : config["paths"]) {
        // Create phonebook source adapter
        auto const &adapter_id = phonebook_config["adapter"].as<std::string>();
        auto const adapter = create_adapter(adapter_id);
        if (!adapter) {
            syslog(LOG_ERR, "Missing phonebook source adapter: %s", adapter_id.c_str());
            exit(EXIT_FAILURE);
        }

        // Create phonebook HTTP adapter
        auto const &phonebook_uri = phonebook_config["path"].as<std::string>();
        auto const phonebook = phonebook::HTTPPhonebook::create(phonebook_config, adapter);
        if (!phonebook) {
            syslog(LOG_ERR, "Failed creating HTTP phonebook for: %s", phonebook_uri.c_str());
            exit(EXIT_FAILURE);
        }

        // Attach HTTP server to phonebook source
        http_server.Get(
            phonebook_uri,
            [phonebook, phonebook_uri]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
                syslog(LOG_DEBUG, "Retrieving phonebook: %s", phonebook_uri.c_str());
                res.set_content(phonebook->getPhonebook(), phonebook->getContentType());
            });
    }
}

std::shared_ptr<DeskphoneCache> createDeskphoneCache(YAML::Node const &config, httplib::Server &http_server)
{
    // Create cache of deskphones
    auto const phone_cache = DeskphoneCache::create(config);

    // Attach manual expiry HTTP URI to deskphone cache
    auto const &expire_uri = config["expire_uri"].as<std::string>();
    http_server.Get(expire_uri, [phone_cache](httplib::Request const &req, httplib::Response &res) -> void {
        if (!req.has_param("aor") || !req.has_param("ip")) {
            res.status = 400;
            return;
        }
        phone_cache->deleteEndpoint(req.get_param_value("aor"), req.get_param_value("ip"));
        res.status = 204;
    });

    return phone_cache;
}

std::vector<std::pair<uint16_t, std::shared_ptr<asterisk::EventHandler>>>
    createEventHandlers(std::shared_ptr<button_state::ButtonPlan> const &button_plan, YAML::Node const &plan_cfg,
                        std::shared_ptr<cpp_ami::Connection> const &conn)
{
    std::vector<std::pair<uint16_t, std::shared_ptr<asterisk::EventHandler>>> ast_buttons;
    for (auto const &button_cfg : plan_cfg["buttons"]) {
        auto const button_id = button_cfg["id"].as<uint16_t>();
        auto const &type = button_cfg["type"].as<std::string>();
        if (type == "night") {
            auto const button = button_plan->getButton(button_id);
            ast_buttons.emplace_back(button_id, asterisk::NightEventHandler::create(button_cfg, button, conn));
        }
        else if (type == "park") {
            auto const button = button_plan->getButton(button_id);
            ast_buttons.emplace_back(button_id, asterisk::ParkEventHandler::create(button_cfg, button, conn));
        }
        else {
            assert(false);
            syslog(LOG_WARNING, "Unknown button type %s in button plan %s", type.c_str(),
                   button_plan->getName().c_str());
        }
    }
    return ast_buttons;
}

void configureHTTPButton(std::string const &phone_type, YAML::Node const &config, httplib::Server &http_server,
                         std::string const &http_url, std::shared_ptr<cpp_ami::Connection> const &conn,
                         std::shared_ptr<button_state::ButtonPlan> const &button_plan,
                         std::shared_ptr<ui::PhoneUI> const &ui)
{
    auto const type = config["type"].as<std::string>();

    if (type == "state") {
        auto const uri = config["uri"].as<std::string>();
        auto const http_button = std::dynamic_pointer_cast<ui::HTTPStateButton>(ui);
        if (!http_button) {
            syslog(LOG_WARNING, "Unable to configure phone state URI %s", uri.c_str());
            return;
        }
        http_server.Get(uri, [http_button](httplib::Request const &req, httplib::Response &res) -> void {
            res.set_content(http_button->httpPushButton(), http_button->getContentType());
        });
    }
    else if (type == "night") {
        auto const button_id = config["button_id"].as<uint16_t>();
        auto const uri = config["uri"].as<std::string>();
        auto const ast_button = button_plan->getEventHandler(button_id);
        if (ast_button->getType() != asterisk::EventHandler::EventType::Night) {
            syslog(LOG_WARNING, "Unable to configure night button URI %s", uri.c_str());
            return;
        }

        auto const ast_night_button = std::dynamic_pointer_cast<asterisk::NightEventHandler>(ast_button);
        auto const http_button = ui::HTTPNightButton::create(phone_type, button_plan->getButton(button_id), conn,
                                                             ast_night_button->getDevice());
        assert(http_button);
        http_server.Get(uri, [http_button](httplib::Request const &req, httplib::Response &res) -> void {
            res.set_content(http_button->httpPushButton(), http_button->getContentType());
        });
    }
    else if (type == "park") {
        auto const button_id = config["button_id"].as<uint16_t>();
        auto const park_list_uri = config["park_list_uri"].as<std::string>();
        auto const park_info_uri = config["park_info_uri"].as<std::string>();
        auto const ast_button = button_plan->getEventHandler(button_id);
        if (ast_button->getType() != asterisk::EventHandler::EventType::Park) {
            syslog(LOG_WARNING, "Unable to configure park button URI %s", park_list_uri.c_str());
            return;
        }

        auto const ast_park_button = std::dynamic_pointer_cast<asterisk::ParkEventHandler>(ast_button);
        auto const http_button =
            ui::HTTPParkButton::create(phone_type, conn, ast_park_button->getParkingLot(), http_url + park_info_uri);
        assert(http_button);
        http_server.Get(park_list_uri,
                        [http_button]([[maybe_unused]] httplib::Request const &req, httplib::Response &res) -> void {
                            res.set_content(http_button->httpPushButton(), http_button->getContentType());
                        });
        http_server.Get(
            park_info_uri,
            [http_button](httplib::Request const &req, httplib::Response &res) -> void { // Serve parked call details
                if (req.has_param("selection")) {
                    auto const exten = req.get_param_value("selection");
                    res.set_content(http_button->httpPushButton(exten), http_button->getContentType());
                    return;
                }
                res.set_content(http_button->displayErrorMessage("Missing Extension Parameter",
                                                                 "Missing URL parameter '&selection=xxx'."),
                                http_button->getContentType());
            });
    }
}

std::unordered_map<std::string, std::shared_ptr<button_state::ButtonPlan>>
    createPhonePlans(YAML::Node const &config, httplib::Server &http_server,
                     std::shared_ptr<DeskphoneCache> const &phone_cache,
                     std::shared_ptr<cpp_ami::Connection> const &conn)
{
    // Create software to deskphone bridge
    auto const ami_bridge = std::make_shared<ui::PhoneEventDispatcher>(conn, phone_cache);

    // Lambda to create button plan
    auto create_button_plan = [&ami_bridge, &config,
                               &conn](std::string const &name) mutable -> std::shared_ptr<button_state::ButtonPlan> {
        std::shared_ptr<button_state::ButtonPlan> button_plan;
        for (auto const &plan_cfg : config["button_plans"]) {
            // Filter on plan name
            if (plan_cfg["name"].as<std::string>() != name) {
                continue;
            }
            // Create new button plan
            button_plan = button_state::ButtonPlan::create(plan_cfg, ami_bridge);
            // Add Asterisk event handlers to button plan
            for (auto const &[id, handler] : createEventHandlers(button_plan, plan_cfg, conn)) {
                button_plan->addEventHandler(id, handler);
            }
            break;
        }
        return button_plan;
    };

    auto const http_url = config["http"]["url"].as<std::string>();

    // Create button plans for existing phones
    std::unordered_map<std::string, std::shared_ptr<button_state::ButtonPlan>> button_plans;
    for (auto const &phone_cfg : config["phones"]) {
        auto const &plan_name = phone_cfg["button_plan"].as<std::string>();
        if (auto const itr = button_plans.find(plan_name); itr != button_plans.end()) {
            syslog(LOG_ERR, "Button plan %s already exists", plan_name.c_str());
            exit(EXIT_FAILURE);
        }

        // Create new button plan only for the phones that use them
        auto const button_plan = create_button_plan(plan_name);
        if (!button_plan) {
            syslog(LOG_ERR, "Unable to find configuration for button plan %s", plan_name.c_str());
            exit(EXIT_FAILURE);
        }
        auto [_, success] = button_plans.emplace(plan_name, button_plan);
        assert(success);

        // Create phone UI and add it as a renderer for the button plan
        auto const [ui_name, ui] = ui::PhoneUI::create(phone_cfg);
        success = button_plan->registerUI(ui_name, ui);
        assert(success);

        // Configure HTTP buttons for the phone
        auto const phone_type = phone_cfg["type"].as<std::string>();
        for (auto const &uri_cfg : phone_cfg["uris"]) {
            configureHTTPButton(phone_type, uri_cfg, http_server, http_url, conn, button_plan, ui);
        }
    }
    return button_plans;
}

std::shared_ptr<asterisk::RegisterEventHandler>
    createRegisterEventHandler(YAML::Node const &config, httplib::Server &http_server,
                               std::shared_ptr<cpp_ami::Connection> const &conn)
{
    auto const phone_cache = createDeskphoneCache(config["database"], http_server);
    auto const phone_plans = createPhonePlans(config, http_server, phone_cache, conn);

    auto event_handler = std::make_shared<asterisk::RegisterEventHandler>(phone_cache, conn);
    for (auto const &[name, plan] : phone_plans) {
        event_handler->addButtonPlan(name, plan);
    }
    return event_handler;
}
