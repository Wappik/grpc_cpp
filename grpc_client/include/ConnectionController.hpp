//
// Created by asus_tuf on 17.10.2023.
//

#ifndef CLIENT_CONNECTIONCONTROLLER_HPP
#define CLIENT_CONNECTIONCONTROLLER_HPP

#include "Client.hpp"

class ConnectionController {
public:
    enum CheckConnectionType {
        NEXT = 1,
        CONNECTED = 2,
        QUEUE_IS_FULL = 3,
        INVALID_REQUEST_FORM = 4,
        NO_CONNECTION = 5,
        TIMEOUT = 6,
        ANY = 7
    };

    explicit ConnectionController(grpc::CompletionQueue *cq_);

    Client* get_client(const std::string& file_address);

    int check(size_t& i, ClientImpl* client_, bool &checking);

    void print_lists();

    void reset_lists();

private:
    std::vector<std::string> white_list;
    std::vector<std::string> black_list;

    std::string title;

    grpc::CompletionQueue *cq_;

    std::mutex mutex_;
    std::mutex mutex_lists;
};


#endif //CLIENT_CONNECTIONCONTROLLER_HPP
