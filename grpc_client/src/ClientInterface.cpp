//
// Created by asus_tuf on 12.10.2023.
//

#include <thread>
#include <sstream>
#include <utility>

#include "grpcpp/grpcpp.h"

#include "ClientInterface.hpp"

ClientInterface::ClientInterface(const std::string& address, grpc::CompletionQueue *cq_) :
        tags(this), is_read(false), is_write(false), is_finished(false), is_finish(true) {
    grpc::ChannelArguments channel_args;
    channel_args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, 10 * 1024 * 1024);
    channel_ = grpc::CreateCustomChannel(address, grpc::InsecureChannelCredentials(), channel_args);
    stub_ = ::copy_service::CopyService::NewStub(channel_);
    std::cout << "Connecting to gRPC service at: " << address << std::endl;
    assert(stub_);
    this->cq_ = cq_;
    read_type = TagData::Type::start_done;
    write_type = TagData::Type::start_done;
}

ClientInterface::~ClientInterface() {}

void ClientInterface::StartCall() {
    stream_ = stub_->AsyncCopy(&context, cq_, &tags.start_done);
}

void ClientInterface::StartCallCheckConnection() {
    stream_ = stub_->AsyncCopy(&context, cq_, &tags.start_done);
}

void ClientInterface::Write(Request req) {
    stream_->Write(req, &tags.write_done);
}

void ClientInterface::WriteLast(Request req) {
    grpc::WriteOptions options;
    stream_->Write(req, options.set_last_message(), &tags.write_done);
}

void ClientInterface::Read(Response &msg) {
    stream_->Read(&msg, &tags.read_done);
}

void ClientInterface::Finish() {
    stream_->Finish(&status, &tags.finish_done);
}

void ClientInterface::FinishDone() {
    if (status.ok()) {
        std::cout << "[FINISH HANDLER]" << std::endl;
    }
}