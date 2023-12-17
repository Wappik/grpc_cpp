//
// Created by asus_tuf on 11.10.2023.
//

#ifndef CLIENT_CLIENTSERVICE_HPP
#define CLIENT_CLIENTSERVICE_HPP

#include <condition_variable>
#include "Poco/Util/Application.h"
#include "Poco/AsyncChannel.h"

class ClientService : public Poco::Util::Application {
public:
    ClientService();
    ~ClientService();

    int main (const std::vector<std::string> &args) override;

    void completion_blacklist(std::vector<std::string>& white_list, std::vector<std::string>& black_list,
                              std::condition_variable& cv_client_address, bool& is_client_address, std::string& data);

    void initialize(Application& self);

    void uninitialize();

    void _initLoggingSystem(const std::string& logDir, int logLevel);

    void createLogger(const std::string& logDir, const std::string& logName, int logLevel);

private:
    std::mutex mutex_;

    Poco::AsyncChannel *asyncChannel;
};


#endif //CLIENT_CLIENTSERVICE_HPP
