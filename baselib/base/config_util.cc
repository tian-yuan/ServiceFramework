#include "base/config_util.h"

#include <iostream>
#include <folly/Format.h>
#include <folly/Singleton.h>
#include <folly/json.h>

#include "base/configuration.h"

bool ServiceConfig::SetConf(const std::string& conf_name, const nlohmann::json& conf) {
    if (conf.find("service_name") != conf.end()) {
        service_name = conf["service_name"].get<std::string>();
    }
    if (conf.find("service_type") != conf.end()) {
        service_type = conf["service_type"].get<std::string>();
    }
    if (conf.find("hosts") != conf.end()) {
        hosts = conf["hosts"].get<std::string>();
    }
    if (conf.find("port") != conf.end()) {
        port = conf["port"].get<int>();
    }
    if (conf.find("max_conn_cnt") != conf.end()) {
        max_conn_cnt = conf["max_conn_cnt"].get<int>();
    }
    return true;
}

void ServiceConfig::PrintDebug() const {
    std::cout << "service_name: " << service_name
                << ", service_type: " << service_type
                << ", hosts: " << hosts
                << ", port: " << port
                << ", max_conn_cnt: " << max_conn_cnt
                << std::endl;
}

std::string ServiceConfig::ToString() const {
    return folly::sformat("{{service_name:{}, service_type:{}, hosts:{}, port:{}, max_conn_cnt:{}}}",
                          service_name,
                          service_type,
                          hosts,
                          port,
                          max_conn_cnt);
    
}

// services
std::vector<std::shared_ptr<ServiceConfig>> ToServiceConfigs(const nlohmann::json& conf) {
    std::vector<std::shared_ptr<ServiceConfig>> v;
    for (auto item : conf) {
        auto configInfo = std::make_shared<ServiceConfig>();
        configInfo->SetConf("", item);
        v.push_back(configInfo);
    }
    return v;
}


bool SystemConfig::SetConf(const std::string& conf_name, const nlohmann::json& conf) {
    if (conf.find("io_thread_pool_size") != conf.end()) {
        io_thread_pool_size = conf["io_thread_pool_size"].get<int>();
    }
    return true;
}

