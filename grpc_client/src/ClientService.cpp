//
// Created by asus_tuf on 11.10.2023.
//

#include "ClientService.hpp"

#include <thread>
#include <condition_variable>
#include <filesystem>

#include "Poco/StringTokenizer.h"
#include "Poco/Util/Application.h"
#include "Poco/Logger.h"
#include "Poco/File.h"
#include "Poco/FileChannel.h"
#include "Poco/PatternFormatter.h"
#include "Poco/FormattingChannel.h"
#include "Poco/SplitterChannel.h"

#include "Logger.hpp"
#include "ConfigReader.hpp"
#include "Exceptions.hpp"

#include "ConnectionController.hpp"
#include "ClientInterface.hpp"
#include "RpcHandler.hpp"
#include "Client.hpp"
#define TEST

ConfigReader* ConfigReader::configReader = nullptr;

ClientService::ClientService() {}

ClientService::~ClientService() {}

int ClientService::main (const std::vector<std::string> &args) {
    try{
        std::unique_ptr<grpc::CompletionQueue> cq_ = std::make_unique<grpc::CompletionQueue>();

//        std::stringstream address;
//        address << ConfigReader::getConfig()->config().hosts[0];
//        std::cout << address.str() << std::endl;

        std::vector<std::string> white_list = ConfigReader::getConfig()->config().hosts;
        std::vector<std::string> black_list;

        std::condition_variable cv_client_address;
        bool is_client_address = false;
        std::string data;

        ConnectionController connectionController(cq_.get());
        std::thread thread_list(&ConnectionController::reset_lists, &connectionController);

        std::vector<std::string> directory_files;
        for (const auto &file: std::filesystem::directory_iterator(
                ConfigReader::getConfig()->config().path_to_directory))
            directory_files.push_back(file.path());

        std::thread white_and_black(&ClientService::completion_blacklist, this,
                                    std::ref(white_list),
                                    std::ref(black_list),
                                    std::ref(cv_client_address),
                                    std::ref(is_client_address),
                                    std::ref(data));

#ifdef TEST
        RpcHandler handler(cq_.get());
        std::thread thread_cq(&RpcHandler::AsyncCompleteRpc, &handler);

        size_t n = directory_files.size();

        std::vector<Client*> clients;
        std::vector<std::unique_ptr<std::thread>> t_vec;

        for (size_t i = 0; i < n; i++) {
            Client* client = connectionController.get_client(directory_files[i]);
            if (client != nullptr) {
                clients.push_back(client);
                std::unique_ptr<std::thread> t = std::make_unique<std::thread>(&Client::ClientStartCall,
                                                                               clients[i],
                                                                               std::ref(white_list),
                                                                               std::ref(black_list),
                                                                               std::ref(cv_client_address),
                                                                               std::ref(is_client_address),
                                                                               std::ref(data));
                t_vec.emplace_back(std::move(t));
            }
        }

        for(size_t i = 0; i < t_vec.size(); i++) {
            t_vec[i].get()->join();
        }

        connectionController.print_lists();


        thread_cq.join();
        thread_list.join();
#else
        RpcHandler handler(cq_.get());
        std::thread thread_cq(&RpcHandler::AsyncCompleteRpc, &handler);

        std::vector<std::unique_ptr<std::thread>> t_vec;

        std::vector<Client*> clients;

        for (size_t i = 0; i < directory_files.size(); i++) {
            clients.push_back(connectionController.get_client(directory_files[i]));
            std::unique_ptr<std::thread> t = std::make_unique<std::thread>(&Client::ClientStartCall,
                                                                           clients[i],
                                                                           std::ref(white_list),
                                                                           std::ref(black_list),
                                                                           std::ref(cv_client_address),
                                                                           std::ref(is_client_address),
                                                                           std::ref(data));
            t_vec.emplace_back(std::move(t));
        }

        for(size_t i = 0; i < t_vec.size(); i++) {
            t_vec[i].get()->join();
        }
        std::cout << clients.size() << std::endl;

        connectionController.print_lists();

        thread_cq.join();
        thread_list.join();
#endif
        white_and_black.join();
    }
    catch (const Poco::Exception &ex) {
        poco_error_f1(logger(), "[Main Exception]: %s", exceptions::displayText(ex));
    }
    catch (const std::exception &ex) {
        poco_error_f1(logger(), "[Main Exception]: %s", std::string(ex.what()));
    }
    uninitialize();
    asyncChannel->close();
    return 0;
}

void ClientService::completion_blacklist(std::vector<std::string>& white_list, std::vector<std::string>& black_list,
                                         std::condition_variable& cv_client_address, bool& is_client_address, std::string& data) {
    std::unique_lock lock(mutex_);

    while(cv_client_address.wait_for(lock, std::chrono::minutes(5), [&]{
        return is_client_address;
    })) {
        if(!data.empty()) {
            auto white_point = std::find(white_list.begin(), white_list.end(), data);
            if (white_point != white_list.end()) {
                auto black_point = std::find(black_list.begin(), black_list.end(), data);
                if (black_point == black_list.end())
                    black_list.push_back(data);
                white_list.erase(white_point);
            }
        }
        is_client_address = false;
        data = "";
        cv_client_address.notify_one();
    }

    lock.unlock();
}

void ClientService::initialize(Application& self) {

    Poco::Util::Application::initialize(self);

    _initLoggingSystem("log", Poco::Message::PRIO_TRACE);

    try {
        Poco::Logger::setLevel(std::string(), Poco::Message::PRIO_TRACE);
        poco_information(logger(), "Application started!");
    }
    catch (const Poco::Exception &ex) {
        poco_error_f1(logger(), "Load configuration exception: %s", exceptions::displayText(ex));
        uninitialize();
    }
    catch (const std::exception &ex) {
        poco_error_f1(logger(), "Load configuration exception: %s", std::string(ex.what()));
        uninitialize();
    }
}

void ClientService::uninitialize() {
    Application::uninitialize();
}

void ClientService::_initLoggingSystem(const std::string& logDir, int logLevel) {
    Poco::File logDirFile(logDir);

    if (!logDirFile.exists()) {
        logDirFile.createDirectories();
    }

    for (const auto& log_name : Logger::getLoggersNames()) {
        createLogger(logDir, log_name, logLevel);
    };

    setLogger(Logger::app());
}

void ClientService::createLogger(const std::string& logDir, const std::string& logName, int logLevel){
    std::string pfDetailedStr = "<%U:%u thread:%I-%T>\n%q %Y-%m-%d %H:%M:%S:%i%z: %t\n";
    std::string pfStr = "%q %Y-%m-%d %H:%M:%S:%i%z [thr-%I]: %t[1000]";

    int archivedLogFileNumber = 5;
    size_t maxMessageLen = 1000;
    auto purgeAge = "5 days";

    // Poco::FileChannel *fileChannel = new Poco::FileChannel(logDir + "/" + logName + ".log");
    Poco::FileChannel *fileDetailedChannel = new Poco::FileChannel(logDir + "/" + logName + "-detailed.log");

//    fileChannel->setProperty("rotation", "50M");
//    fileChannel->setProperty("archive", "timestamp");
//    fileChannel->setProperty("compress", "true");
//    fileChannel->setProperty("purgeCount", std::to_string(archivedLogFileNumber));

    fileDetailedChannel->setProperty("rotation", "300M");
    fileDetailedChannel->setProperty("archive", "timestamp");
    fileDetailedChannel->setProperty("compress", "true");
    //fileDetailedChannel->setProperty("purgeCount", std::to_string(archivedLogFileNumber));
    fileDetailedChannel->setProperty ("purgeAge", purgeAge);

    //Formatting
    Poco::PatternFormatter *patternDetailedFormatter = new Poco::PatternFormatter(pfDetailedStr);
    patternDetailedFormatter->setProperty("times", "local");

    Poco::FormattingChannel *formattingDetailedChannel =
            new Poco::FormattingChannel(patternDetailedFormatter, fileDetailedChannel);

//    Poco::PatternFormatter *patternFormatter = new Poco::PatternFormatter(pfStr);
//    patternFormatter->setProperty("times", "local");

#ifdef LOG_TO_CONSOLE
    Poco::ConsoleChannel* consoleChannel = new Poco::ConsoleChannel();
    Poco::SplitterChannel* splitterChannel = new Poco::SplitterChannel();
    splitterChannel->addChannel(consoleChannel);
    splitterChannel->addChannel(fileChannel);

    Poco::FormattingChannel* formattingChannel = new Poco::FormattingChannel(patternFormatter, splitterChannel);
#else
    //Poco::FormattingChannel *formattingChannel = new Poco::FormattingChannel(patternFormatter, fileChannel);
#endif
    //Cutting
    //  utils::CuttingChannel *cuttingChannel = new utils::CuttingChannel(maxMessageLen, formattingChannel);

    //Splitter
    Poco::SplitterChannel *splitterDetNotDetChannel = new Poco::SplitterChannel();
    splitterDetNotDetChannel->addChannel(formattingDetailedChannel);
    //splitterDetNotDetChannel->addChannel(cuttingChannel);

    //Async
    asyncChannel = new Poco::AsyncChannel(splitterDetNotDetChannel);
    asyncChannel->open();


    //Logger
    Poco::Logger::get(logName).setLevel(logLevel);
    Poco::Logger::get(logName).setChannel(asyncChannel);
}

POCO_APP_MAIN(ClientService)