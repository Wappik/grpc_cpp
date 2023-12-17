//
// Created by asus_tuf on 18.09.2023.
//

#ifndef UNTITLED1_CONFIGREADER_HPP
#define UNTITLED1_CONFIGREADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"

class ConfigReader {
public:
    struct Config {
        std::vector<std::string> hosts;
        std::string path_to_directory;
        std::string file_name;
        std::string log_level;
        int batch_size;
    };

    ConfigReader(const ConfigReader&) = delete;
    void operator =(const ConfigReader&) = delete;

    static ConfigReader* getConfig() {
        if(configReader == nullptr)
            configReader = new ConfigReader();
        return configReader;
    }

    Config config() {
        return _data;
    }

private:
    ConfigReader() {
        std::cout << "ConfigReader" << std::endl;
        std::ifstream file("../../config/config.json");
        std::stringstream ss;
        nlohmann::json data;

        if(file.is_open()) {
            ss << file.rdbuf();
            data = nlohmann::json::parse(ss.str());
        }
        file.close();

        if (!data.empty()) {
            _data.hosts = data["server_settings"]["hosts"].get<std::vector<std::string>>();

            std::string path_to_directory = data["path_to_directory"];
            std::string path_to_directory_tag = "{workspaceFolder}";
            path_to_directory.replace(path_to_directory.find(path_to_directory_tag), path_to_directory_tag.size(), "../..");

            _data.path_to_directory = path_to_directory;
            _data.file_name = data["file_name"];
            _data.log_level = data["log"]["level"];
            _data.batch_size = data["batch_size"];
        }
    }

    static ConfigReader* configReader;
    Config _data;

};


#endif //UNTITLED1_CONFIGREADER_HPP
