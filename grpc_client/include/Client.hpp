//
// Created by asus_tuf on 10.10.2023.
//

#ifndef CLIENT_CLIENT_HPP
#define CLIENT_CLIENT_HPP

#include <utility>

#include "ClientImpl.hpp"


class Client {
public:

    Client(grpc::CompletionQueue *cq_,
           const std::string&  address,
           std::string file_address,
           ClientImpl* client_);

    Client(const Client& obj);

    ~Client();

    std::vector<std::string> dataGeneration();

    void ClientStartCall(std::vector<std::string>& white_list, std::vector<std::string>& black_list,
                         std::condition_variable& cv_client_address, bool& is_client_address, std::string& data_address);

private:
    std::mutex mutex_;

    std::vector<std::string> data_;

    std::string file_address;

    copy_service::CopyResponse msg;

    grpc::CompletionQueue *cq_;
    std::string address;

    unsigned start_bytes;
    unsigned end_bytes;

    bool correct_call;

    ClientImpl *client_;
};


#endif //CLIENT;_CLIENT_HPP
