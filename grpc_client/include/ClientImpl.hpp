//
// Created by asus_tuf on 09.10.2023.
//

#ifndef CLIENT_CLIENTIMPL_HPP
#define CLIENT_CLIENTIMPL_HPP

#include <mutex>
#include <condition_variable>
#include <functional>

#include "ClientInterface.hpp"

class ClientImpl : public ClientInterface {
public:

    ClientImpl(grpc::CompletionQueue *cq_,
               const std::string& address);

    ClientImpl(const ClientImpl& obj);

    void operator =(const ClientImpl& obj);

    ~ClientImpl();

    void StartRead(std::string& error_msg_read, const std::function<void()>& reader_function) override;

    void StartWrite(bool &edit, std::string &error_msg_write, const std::function<void()>& writer_function) override;

    bool CheckingConnection(const std::string& title);

    copy_service::CopyResponse copyResponse;

private:
    std::mutex m_read;
    std::mutex m_write;

};


#endif //CLIENT_CLIENTIMPL_HPP
