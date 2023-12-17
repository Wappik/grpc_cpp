//
// Created by asus_tuf on 09.10.2023.
//

#include <thread>
#include <sstream>
#include <utility>

#include "grpcpp/grpcpp.h"

#include "ClientImpl.hpp"
#include "RpcHandler.hpp"
#include "Logger.hpp"

ClientImpl::ClientImpl(grpc::CompletionQueue *cq_, const std::string& address) :
                       ClientInterface(address, cq_){
}

ClientImpl::~ClientImpl() {
}

void ClientImpl::StartRead(std::string& error_msg_read, const std::function<void()>& reader_function) {
    std::unique_lock lock(m_read);

    while (cv_read.wait_for(lock, std::chrono::milliseconds(2000), [&] {
        return  is_read;
    })) {

        if (status.error_code() == grpc::StatusCode::OK) {
            if (read_type == TagData::Type::finish_done) {
//                std::cout << "read_type == TagData::Type::finish_done" << std::endl;
                return;
            }

            is_read = false;

            reader_function();
        }
        else {
            std::stringstream error_msg;
            error_msg << "[Read Error] Message: Error code #" << status.error_code() << " - " << status.error_message() << "\n";
            error_msg_read = error_msg.str();
            break;
        }
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    m_read.unlock();
}

void ClientImpl::StartWrite(bool &edit, std::string &error_msg_write, const std::function<void()>& writer_function) {
    std::unique_lock lock(m_write);

    while(cv.wait_for(lock, std::chrono::milliseconds(2000), [&] {
        return edit;
    })) {
        if (status.error_code() == grpc::StatusCode::OK) {
            edit = false;

            writer_function();
        }
        else {
            std::stringstream error_msg;
            error_msg << "[Write Error] Message: Error code #" << status.error_code() << " - " << status.error_message() << "\n";
            error_msg_write = error_msg.str();
            break;
        }
//            std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    m_write.unlock();
}

bool ClientImpl::CheckingConnection(const std::string& title) {
    std::unique_lock lock(m_write);

    StartCall();

    cv_write.wait_for(lock, std::chrono::milliseconds(2000), [&] {
        return is_write && is_read;
    });

    if (status.ok()) {

        is_write = false;
        copy_service::CopyRequest copyRequest;
        copy_service::StartCall* startCall = new copy_service::StartCall;
        startCall->set_title(title);
        copyRequest.set_allocated_start_call(startCall);

        Write(copyRequest);
    }
    else {
        std::stringstream error_msg;
        error_msg << "[Write Error] Message: Error code #" << status.error_code() << " - " << status.error_message() << "\n";
        return false;
    }

    std::string error_msg_read;

    while (cv_read.wait_for(lock, std::chrono::milliseconds(2000), [&] {
        return  is_read;
    })) {

        if (status.ok()) {
            is_read = false;

            if (copyResponse.has_error_type()) {
//                std::cout << copyResponse.error_type().message();
                return false;
            }
            else if(copyResponse.has_successfully_request()) {
                return true;
            }

            copyResponse.Clear();
            Read(copyResponse);
        } else {
            std::stringstream error_msg;
            error_msg << "[Read Error] Message: Error code #" << status.error_code() << " - " << status.error_message()
                      << "\n";
            return false;
        }
    }

//    Finish();
//    cv_finish.wait(lock, [&] { return is_finish; });
//    is_finish = false;

    lock.unlock();
    return false;
}

























