//
// Created by asus_tuf on 17.10.2023.
//

#include <thread>

#include <Poco/StringTokenizer.h>

#include "ConnectionController.hpp"
#include "ConfigReader.hpp"
#include "Logger.hpp"


ConnectionController::ConnectionController(grpc::CompletionQueue *cq_) {
    this->cq_ = cq_;
    white_list = ConfigReader::getConfig()->config().hosts;
}

Client* ConnectionController::get_client(const std::string& file_address) {

    ClientImpl* client_;

    Poco::StringTokenizer st_file_address(file_address, "/");
    std::vector<std::string> split_address(st_file_address.begin(), st_file_address.end());
    title = split_address[split_address.size() - 1];

    bool is_connection = false;

    bool checking = false;

    size_t i = 0;
    do {
        checking = false;
        if (i < white_list.size()) {
            client_ = new ClientImpl(cq_, white_list[i]);
        } else {
            i = 0;
            continue;
        }

        switch (check(i, client_, checking)) {
            case CheckConnectionType::CONNECTED:
                is_connection = true;
                break;
            case CheckConnectionType::QUEUE_IS_FULL: {
                i++;
            } break;
            case CheckConnectionType::INVALID_REQUEST_FORM:
                return nullptr;
            case CheckConnectionType::NO_CONNECTION: {
                auto white_point = std::find(white_list.begin(), white_list.end(), white_list[i]);
                if (white_point != white_list.end()) {
                    auto black_point = std::find(black_list.begin(), black_list.end(), white_list[i]);
                    black_list.push_back(white_list[i]);
                    white_list.erase(white_point);
                }
                i++;
            } break;
            default:
                return nullptr;
        }
    } while(!is_connection);

    Client* client_obj = new Client(cq_, white_list[i], file_address, client_);

    return client_obj;
}

int ConnectionController::check(size_t& i, ClientImpl* client_, bool &checking) {
    std::unique_lock lock(mutex_);
    if (!white_list.empty()) {

        if (client_->CheckingConnection(title)) {
            client_->Finish();
            if (client_->cv_finish.wait_for(lock, std::chrono::milliseconds(1000), [&] {
                return client_->is_finish;
            })) {
                client_->is_finish = false;
                return CheckConnectionType::CONNECTED;
            }
            else {
                return CheckConnectionType::TIMEOUT;
            }
        } else {
            if (client_->copyResponse.error_type().has_queue_is_full()) {
                return CheckConnectionType::QUEUE_IS_FULL;
            }
            else if (client_->copyResponse.error_type().has_invalid_request_form()) {
                return CheckConnectionType::INVALID_REQUEST_FORM;
            } else {
                return CheckConnectionType::NO_CONNECTION;
            }
        }
    } else {
        if(checking) {
            poco_error(Logger::app(), "All servers are not available.");
            return CheckConnectionType::ANY;
        }
        else {
            white_list = black_list;
            black_list.clear();
            checking = true;
        }
    }
    lock.unlock();
    return CheckConnectionType::ANY;
}

void ConnectionController::print_lists() {
    std::cout << "[White list]: ";
    for(auto white: white_list) {
        std::cout << white << "\t";
    } std::cout << std::endl;


    std::cout << "[Black list]: ";
    for(auto black: black_list) {
        std::cout << black << "\t";
    } std::cout << std::endl;
}

void ConnectionController::reset_lists() {
    std::unique_lock lock(mutex_lists);

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30000));
        white_list = ConfigReader::getConfig()->config().hosts;
        black_list.clear();
    }

    lock.unlock();
}


















