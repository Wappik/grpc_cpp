#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <vector>

#include "Poco/Logger.h"


class Logger {
    public:
        static const std::vector<std::string>& getLoggersNames()
        {
            static std::vector<std::string> loggers  = {
                    "app"
            };

            return loggers;
        }

        static Poco::Logger& app()
        {
            static Poco::Logger& logger = Poco::Logger::get("app");
            return logger;
        }
};


#endif