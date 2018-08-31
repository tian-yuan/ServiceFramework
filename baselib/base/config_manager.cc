#include "base/config_manager.h"


#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/experimental/StringKeyedUnorderedMap.h>

ConfigManager* ConfigManager::GetInstance() {
    static ConfigManager g_config_manager;
    return &g_config_manager;
}

void ConfigManager::Register(const folly::fbstring item_name, Configurable* item, bool recv_updated) {
    LOG(INFO) << "ConfigManager::RegisterConfig - register config_item: " << item_name << std::endl;
    
    if (config_items_.find(item_name) != config_items_.end()) {
        LOG(INFO) << "ConfigManager::RegisterConfig - config_item is registed: " << item_name << std::endl;
    } else {
        config_items_.insert(std::make_pair(item_name, item));
    }
}

void ConfigManager::UnRegister(const folly::fbstring& item_name) {
    LOG(INFO) << "ConfigManager::UnRegisterConfig - register config_item: " << item_name;
    
    ConfigItemMap::iterator it = config_items_.find(item_name);
    if (it != config_items_.end()) {
        config_items_.erase(it);
    }
}

bool ConfigManager::Initialize(ConfigType configType, const std::string param) {
    LOG(INFO) << "Initialize - initialize param: " << param;
    
    if (param.empty()) {
        LOG(ERROR) << "Initialize - param is empty.";
        return false;
    }

    config_param = param;
    return OnConfigUpdated();
}

bool ConfigManager::OnConfigUpdated() {
    bool rv = false;
    
    // 1. 打开文件
    folly::fbstring config_data;
    if(!folly::readFile(config_param.c_str(), config_data)) {
        LOG(ERROR) << "OnConfigUpdated - Unable to open config_file << " << config_param;
        return rv;
    } else {
        rv = OnConfigDataUpdated(config_data, true);
    }

    return rv;
}

bool ConfigManager::OnConfigDataUpdated(const folly::fbstring& config_data, bool is_first) {
    nlohmann::json config_json = nlohmann::json::parse(config_data.toStdString());
    ConfigItemMap::iterator it = config_items_.begin();
    while (it != config_items_.end()) {
        folly::fbstring item_key = it->first;
        LOG(INFO) << "config item key : " << item_key;
        if (config_json.find(item_key) != config_json.end()) {
            Configurable* configurable = it->second;
            nlohmann::json::iterator iter = config_json.find(item_key);
            nlohmann::json item_value = iter.value();
            LOG(INFO) << "config : " << item_value;
            configurable->SetConf(item_key.toStdString(), item_value);
        }
        it++;
    }
    return true;
}


void ConfigManager::StartObservingConfig(folly::EventBase* evb) {
    if(is_watched_)
        return;

    is_watched_ = true;
}

