// Copyright (c) 2026 Christopher L Walker
// SPDX-License-Identifier: MIT

#ifndef PHONEBOOK_HANDLER_HPP
#define PHONEBOOK_HANDLER_HPP

#include <c++ami/Connection.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>

class PhonebookProvider {
public:
    explicit PhonebookProvider(std::shared_ptr<cpp_ami::Connection> const &io_conn, std::chrono::minutes expiry);
    ~PhonebookProvider();

    std::string getPhonebookXML();

private:
    void setPhonebookXML(std::string phonebook_xml);

    void startThread();
    void stopThread();
    void workThread();

    std::shared_ptr<cpp_ami::Connection> io_conn_;

    std::thread work_thread_;
    std::atomic<bool> work_thread_run_;
    std::condition_variable work_thread_cv_;
    std::chrono::minutes expiry_;
    std::string phonebook_xml_;
    std::mutex phonebook_xml_mut_;
};

#endif
