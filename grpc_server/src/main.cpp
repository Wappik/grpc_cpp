//
// Created by asus_tuf on 26.09.2023.
//

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#include <grpc++/grpc++.h>

#include "copy_service.grpc.pb.h"
#include "ConfigReader.hpp"

ConfigReader* ConfigReader::configReader = nullptr;

class CopyServiceServerImpl : public copy_service::CopyService::Service {
public:
    CopyServiceServerImpl() : copy_service::CopyService::Service(), count(0) {}
    ~CopyServiceServerImpl() = default;

    ::grpc::Status Copy(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::copy_service::CopyResponse, ::copy_service::CopyRequest>* stream) {

        copy_service::CopyRequest copyRequest;

        std::stringstream ss;
        std::string title_;

        unsigned sum_bytes = 0;
        std::cout << "[Start Client Count] \t" << count << std::endl;

        stream->Read(&copyRequest);
        if (copyRequest.has_start_call()) {
            count++;
            if (count > ConfigReader::getConfig()->config().max_client) {
                std::stringstream ss;
                ss << "[Server message]: The maximum number of clients is currently connected to the server <"
                    << ConfigReader::getConfig()->config().grpc_host << ":"
                    << ConfigReader::getConfig()->config().grpc_port << ">.";
                copy_service::ErrorMessage* error = new copy_service::ErrorMessage;
                error->set_message(ss.str());
                copy_service::CopyResponse copyResponse;
                copyResponse.set_allocated_error_message(error);
                stream->Write(copyResponse);
                count--;
                return grpc::Status::OK;
            }
        }
        else {
            std::stringstream ss;
            ss << "[Server message]: The request flow should start with a StartCall request.";
            copy_service::ErrorMessage* error = new copy_service::ErrorMessage;
            error->set_message(ss.str());
            copy_service::CopyResponse copyResponse;
            copyResponse.set_allocated_error_message(error);
            stream->Write(copyResponse);
            return grpc::Status::OK;
        }

        while(stream->Read(&copyRequest)) {
            if (copyRequest.has_title()) {
                std::cout << "TITLE:\t"<< copyRequest.title() << std::endl;
                title_ = copyRequest.title();
                copy_service::CopyResponse copyResponse;
                copyResponse.set_message(title_);
                stream->Write(copyResponse);
            }

            if (copyRequest.has_body()) {
                ss << copyRequest.body();
                sum_bytes += copyRequest.body().length();
//                std::cout << sum_bytes << std::endl;
//                std::string body = std::to_string(copyRequest.body().length());
//                std::cout << "BODY :\t"<< copyRequest.body() << std::endl;
            }

            if (copyRequest.has_read()) {
//                std::cout << sum_bytes << std::endl;
                copy_service::CopyResponse copyResponse;
                copyResponse.set_message(std::to_string(sum_bytes));
                stream->Write(copyResponse);
            }

            if (copyRequest.has_end()) {
                
                struct stat st = {0};

                if (stat(ConfigReader::getConfig()->config().path_to_directory.c_str(), &st) == -1)
                    mkdir(ConfigReader::getConfig()->config().path_to_directory.c_str(), 0700);


                std::ofstream file(ConfigReader::getConfig()->config().path_to_directory + title_, std::ios::binary);
                if (file.is_open()) {
                    file << ss.str();
                }
                file.close();
                std::cout << "End" << std::endl;
                copy_service::CopyResponse copyResponse;
                copyResponse.set_message(std::to_string(sum_bytes));
                stream->Write(copyResponse);
                ss.str(std::string());
            }
        }
        count--;
        std::cout << "[End Client Count] \t" << count << std::endl;
        return grpc::Status::OK;
    }
private:
    unsigned count;
};

void RunServer() {
    std::stringstream ss;
    ss << ConfigReader::getConfig()->config().grpc_host << ":" << ConfigReader::getConfig()->config().grpc_port;
    std::string server_address(ss.str());

    CopyServiceServerImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}