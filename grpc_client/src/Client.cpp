//
// Created by asus_tuf on 10.10.2023.
//

#include <sstream>
#include <fstream>
#include <thread>
#include <utility>

#include "Poco/StringTokenizer.h"

#include "Client.hpp"
#include "RpcHandler.hpp"
#include "Models.hpp"
#include "ConfigReader.hpp"
#include "Logger.hpp"


Client::Client(grpc::CompletionQueue *cq_, const std::string&  address_,
               std::string file_address, ClientImpl* client) :
               file_address(std::move(file_address)), address(address_), start_bytes(0), end_bytes(0),
               correct_call(true) {
    client_ = client;
    this->cq_ = cq_;
}

Client::Client(const Client& obj) {
    file_address = obj.file_address;
    data_ = obj.data_;
    client_ = obj.client_;
    start_bytes = obj.start_bytes;
    end_bytes = obj.end_bytes;
    cq_ = obj.cq_;
    address = obj.address;
    correct_call = obj.correct_call;
}

Client::~Client() {
}

std::vector<std::string> Client::dataGeneration() {
    std::ifstream file(file_address, std::ios::binary);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string file_chunks = ss.str();
//    std::cout << "file read" << std::endl;
    start_bytes = file_chunks.length();
    Poco::StringTokenizer st_file_address(file_address, "/");
    std::vector<std::string> split_address(st_file_address.begin(), st_file_address.end());

    std::vector<std::string> data;
//    data.push_back(split_address[split_address.size() - 1]);
    unsigned count = 0;

    unsigned batch_size = ConfigReader::getConfig()->config().batch_size;

    while(file_chunks.size() > batch_size) {
//        std::cout << "[data]: " << count++ << std::endl;
        std::string token = file_chunks.substr(0, batch_size);
        data.push_back(token);
        file_chunks.erase(0, batch_size);
    } data.push_back(file_chunks);


    return data;
}

void Client::ClientStartCall(std::vector<std::string>& white_list, std::vector<std::string>& black_list,
                             std::condition_variable& cv_client_address, bool& is_client_address, std::string& data_address) {
    std::unique_lock lock(mutex_);

    std::cout << "YES" << std::endl;

    unsigned size = ConfigReader::getConfig()->config().batch_size;
    std::ifstream file(file_address, std::ios::binary);
    std::string buffer(size, '\0');


    std::string error_msg_read;
    std::string error_msg_write;


    std::string data;
    CallType type = CallType::START_CALL;
    bool edit = false;

    std::thread write_thread(&ClientImpl::StartWrite, client_, std::ref(edit), std::ref(error_msg_write), [&]() {
        copy_service::CopyRequest request;

        switch (type) {
            case CallType::START_CALL: {
                copy_service::StartCall* pStartCall = new copy_service::StartCall;
                pStartCall->set_title(data);
                request.set_allocated_start_call(pStartCall);
                client_->Write(request);
            } break;
            case CallType::BODY: {
                request.set_body(data);
//                std::cout << "[BODY]\t" << data << std::endl;
                client_->Write(request);
            } break;
            case CallType::END: {
                copy_service::EndFile* pEndFile = new copy_service::EndFile;
                request.set_allocated_end(pEndFile);
                client_->WriteLast(request);
            } break;
            case CallType::READ: {
                copy_service::ReadRequest *pReadRequest = new copy_service::ReadRequest;
                request.set_allocated_read(pReadRequest);
                client_->Write(request);
            } break;
        }
    });

    size_t count_read = 0;

    std::thread read_thread(&ClientImpl::StartRead, client_, std::ref(error_msg_read), [&]() {
        client_->Read(msg);
        if (!msg.message().empty()) {
            std::cout << "[Message #" << count_read++ << "]:\t" << msg.message() << "\n";
            end_bytes = atoi(msg.message().c_str());
        }
        msg.Clear();
    });

    while(!file.eof()) {
        client_->cv_write.wait(lock, [&] { return client_->is_write; });
        client_->is_write = false;

        file.read(&buffer[0], size);
        std::streamsize s = file.gcount();
        data = buffer;


        if (size != s) data.erase(s);

        type = BODY;
        edit = true;
        client_->cv.notify_one();
    }

    client_->cv_write.wait(lock, [&] { return client_->is_write; });
    client_->is_write = false;
    type = END;
    edit = true;
    client_->cv.notify_one();

    write_thread.join();
    read_thread.join();

    std::cout << "???" << std::endl;

    if (!error_msg_write.empty()) {
        std::cout << error_msg_write;
        poco_error(Logger::app(), error_msg_write);
    }
    if (!error_msg_read.empty()) {
        std::cout << error_msg_read;
        poco_error(Logger::app(), error_msg_read);
    }

    std::stringstream ss;
    ss << "[PACKAGE INFORMATION]: Title = " << ""
       << "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\tThe initial number of bytes in the file = " << start_bytes << "."
       << "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\tThe number of bytes received by the server in the file = " << end_bytes
       << "."
       << "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
       << ((start_bytes == end_bytes) ? "A file was received without data loss." : "Data loss has occurred.");
    poco_information(Logger::app(), ss.str());

    std::cout << "[CLIENT END]: edit = " << ((edit) ? "true" : "false") << " | type = ";
    switch (type) {
        case CallType::TITLE:
            std::cout << "TITLE";
            break;
        case CallType::READ:
            std::cout << "READ";
            break;
        case CallType::BODY:
            std::cout << "BODY";
            break;
        case CallType::END:
            std::cout << "END";
            break;
        case CallType::START_CALL:
            std::cout << "START_CALL";
            break;
    }
    std::cout << std::endl;

    lock.unlock();
}




























