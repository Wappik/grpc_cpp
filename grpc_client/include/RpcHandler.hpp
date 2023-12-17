//
// Created by asus_tuf on 09.10.2023.
//

#ifndef CLIENT_RPCHANDLER_HPP
#define CLIENT_RPCHANDLER_HPP

#include <mutex>
#include <condition_variable>

#include "copy_service.grpc.pb.h"
#include "ClientInterface.hpp"

class RpcHandler {
public:
    RpcHandler(grpc::CompletionQueue *cq_);

    void AsyncCompleteRpc();

    static void handling_thread_main(grpc::CompletionQueue *cq,
                                     std::mutex& m_cq);


private:
    grpc::CompletionQueue *cq_;
    std::mutex m_cq;
};


#endif //CLIENT_RPCHANDLER_HPP
