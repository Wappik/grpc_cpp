//
// Created by asus_tuf on 09.10.2023.
//

#include "RpcHandler.hpp"

RpcHandler::RpcHandler(grpc::CompletionQueue *cq_): cq_(cq_) {
}

void RpcHandler::AsyncCompleteRpc() {
    handling_thread_main(cq_, m_cq);
}

void RpcHandler::handling_thread_main(grpc::CompletionQueue *cq, std::mutex& m_cq) {
    std::unique_lock lock(m_cq);
    void *raw_tag = nullptr;
    bool ok = false;

    while (cq->Next(&raw_tag, &ok)) {
        ClientInterface::TagData *tag = reinterpret_cast<ClientInterface::TagData *>(raw_tag);

        if (!ok) {
            if (!tag->handler->is_finished) {
                tag->handler->Finish();
                tag->handler->is_finished = true;
            }
        }
        else {
            switch (tag->evt) {
                case ClientInterface::TagData::Type::start_done:
                    tag->handler->is_read = true;
                    tag->handler->is_write = true;
                    tag->handler->cv_read.notify_one();
                    tag->handler->cv_write.notify_one();
                    break;
                case ClientInterface::TagData::Type::read_done:
                    tag->handler->is_read = true;
                    tag->handler->cv_read.notify_all();
                    break;
                case ClientInterface::TagData::Type::write_done:
                    tag->handler->is_write = true;
                    tag->handler->cv_write.notify_one();
                    break;
                case ClientInterface::TagData::Type::finish_done:
                    tag->handler->FinishDone();
                    tag->handler->write_type = ClientInterface::TagData::Type::finish_done;
                    tag->handler->read_type = ClientInterface::TagData::Type::finish_done;
                    tag->handler->is_read = true;
                    tag->handler->is_write = true;
                    tag->handler->is_finish = true;
                    tag->handler->cv_finish.notify_one();
                    tag->handler->cv_read.notify_one();
                    tag->handler->cv_write.notify_one();
                    break;
            }

        }

    }
    lock.unlock();
}