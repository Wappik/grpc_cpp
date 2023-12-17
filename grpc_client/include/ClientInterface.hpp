//
// Created by asus_tuf on 09.10.2023.
//

#ifndef CLIENT_CLIENTINTERFACE_HPP
#define CLIENT_CLIENTINTERFACE_HPP

#include <mutex>
#include <condition_variable>
#include <functional>

#include "copy_service.grpc.pb.h"

class ClientInterface {
public:
    using Request = copy_service::CopyRequest;
    using Response = copy_service::CopyResponse;

    struct TagData {
        enum class Type : int {
            start_done = 1,
            read_done = 2,
            write_done = 3,
            finish_done = 4,
        };
        ClientInterface* handler;
        Type evt;
    };

    struct TagSet {
        TagSet(ClientInterface* self): start_done{self, TagData::Type::start_done},
                                       read_done{self, TagData::Type::read_done},
                                       write_done{self, TagData::Type::write_done},
                                       finish_done{self, TagData::Type::finish_done} {}

        TagData start_done;
        TagData read_done;
        TagData write_done;
        TagData finish_done;
    };

    explicit ClientInterface(const std::string& address, grpc::CompletionQueue *cq_);

    virtual ~ClientInterface();

    void StartCall();

    void StartCallCheckConnection();

    void Write(Request req);

    void WriteLast(Request req);

    virtual void StartWrite(bool &edit, std::string &error_msg_write, const std::function<void()>& writer_function) {};

    void Read(Response &msg);

    virtual void StartRead(std::string& error_msg_read, const std::function<void()>& reader_function) {};

    void Finish();

    void FinishDone();

    TagSet tags;
    grpc::Status status;

    std::condition_variable cv_read;
    std::condition_variable cv_write;
    std::condition_variable cv_finish;
    std::condition_variable cv;

    bool is_read;
    bool is_write;
    bool is_finish;

    bool is_finished;

    TagData::Type read_type;
    TagData::Type write_type;

protected:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<::copy_service::CopyService::Stub> stub_;
    std::unique_ptr<grpc::ClientAsyncReaderWriter<copy_service::CopyRequest, copy_service::CopyResponse>> stream_;


    grpc::ClientContext context;
    grpc::CompletionQueue *cq_;

    std::mutex m_read;
    std::mutex m_write;
};

#endif //CLIENT_CLIENTINTERFACE_HPP
