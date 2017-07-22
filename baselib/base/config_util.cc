#include "base/config_util.h"

#include <iostream>
#include <folly/Format.h>
#include <folly/Singleton.h>
#include <folly/json.h>

#include "base/configuration.h"

bool ServiceConfig::SetConf(const std::string& conf_name, const Configuration& conf) {
    folly::dynamic v = nullptr;
    
    v = conf.GetValue("service_name");
    if (v.isString()) service_name = v.asString();
    v = conf.GetValue("service_type");
    if (v.isString()) service_type = v.asString();
    v = conf.GetValue("hosts");
    if (v.isString()) hosts = v.asString();
    v = conf.GetValue("port");
    if (v.isInt()) port = static_cast<uint32_t>(v.asInt());
    v = conf.GetValue("max_conn_cnt");
    if (v.isInt()) max_conn_cnt = static_cast<uint32_t>(v.asInt());
    
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
std::vector<std::shared_ptr<ServiceConfig>> ToServiceConfigs(const Configuration& conf) {
    std::vector<std::shared_ptr<ServiceConfig>> v;

    for (auto& v2 : conf.GetDynamicConf()) {
            auto config_info = std::make_shared<ServiceConfig>();
            Configuration config(v2);
            config_info->SetConf("", config);
            v.push_back(config_info);
    }
    
    return v;
}


bool SystemConfig::SetConf(const std::string& conf_name, const Configuration& conf) {
    folly::dynamic v = nullptr;
    
    v = conf.GetValue("io_thread_pool_size");
    if (v.isInt()) io_thread_pool_size = static_cast<uint32_t>(v.asInt());

    return true;
}
