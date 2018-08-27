//
// Created by tianyuan on 2018/8/27.
//

#include <iostream>
#include "thirdparty/nlohmann/json.hpp"

int main() {
    std::string configString = "{\"service_name\":\"tcp_server\", \"service_type\":\"tcp_server\", \"hosts\":\"0.0.0.0\", \"port\":18889, \"max_conn_cnt\":2}";
    nlohmann::json configJson = nlohmann::json::parse(configString);
    if (configJson["service_name1"] != nullptr) {
        std::cout << "service_name : " << configJson["service_name1"].get<std::string>().c_str() << std::endl;
    } else {
        std::cout << "service_name1 is null." << std::endl;
    }
    if (configJson.find("service_name") != configJson.end()) {
        std::cout << "service_name : " <<  configJson["service_name"].get<std::string>().c_str() << std::endl;
    } else {
        std::cout << "service_name is null." << std::endl;
    }
    std::cout << "nlohmann json test." << std::endl;

    configJson["test"]["name"] = "bobo";
    for (nlohmann::json::iterator it = configJson.begin(); it != configJson.end(); ++it) {
        std::cout << it.key() << " : " << it.value() << "\n";
    }
    return 0;
}