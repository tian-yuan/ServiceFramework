#ifndef BASE_CONFIGURABLE_H_
#define BASE_CONFIGURABLE_H_

#include "thirdparty/nlohmann/json.hpp"

// 动态配置框架
struct Configurable {
public:
    virtual ~Configurable() = default;
    
    // 由Conf生成
    virtual bool SetConf(const nlohmann::json& conf) {
        return true;
    }

    // 由Conf生成
    virtual bool SetConf(const std::string& conf_name, const nlohmann::json& conf) {
        return true;
    }
};

#endif
