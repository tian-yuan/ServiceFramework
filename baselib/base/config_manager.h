#ifndef BASE_CONFIG_MANAGER_H_
#define BASE_CONFIG_MANAGER_H_

#include <string>
#include <map>

#include <folly/FBString.h>
#include <folly/Singleton.h>

#include "base/configurable.h"
#include "base/configuration.h"

namespace folly {
class EventBase;
} // folly

class ConfigManager {
public:
    ~ConfigManager() = default;

public:
    static enum ConfigType : int {
        FILE = 0,
        APOLLO = 1
    };

public:
    static ConfigManager* GetInstance();

    void Register(const folly::fbstring item_name, Configurable* item, bool recv_updated = false);
    void UnRegister(const folly::fbstring& item_name);

    bool Initialize(ConfigType configType, const std::string param);
    
    void StartObservingConfig(folly::EventBase* evb);
    
private:
    friend class folly::Singleton<ConfigManager>;

    ConfigManager() = default;

    bool OnConfigUpdated();
    bool OnConfigDataUpdated(const folly::fbstring& config_data, bool is_first);

    // if ConfigType is file, the param is file path
    // if ConfigType is apollo, the param is meta server address url
    std::string config_param;
    bool is_watched_ = false;

    typedef std::map<folly::fbstring, Configurable *> ConfigItemMap;
    ConfigItemMap config_items_;
};

#endif
