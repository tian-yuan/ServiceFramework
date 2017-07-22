#ifndef BASE_CONFIGURATION_H_
#define BASE_CONFIGURATION_H_

#include <string>
#include <vector>

#include <folly/dynamic.h>

// 动态配置框架
class Configuration {
public:
    Configuration() = default;
    explicit Configuration(folly::dynamic dynamic_conf) :
        dynamic_conf_(dynamic_conf) {}
    
    virtual ~Configuration() = default;

    inline folly::dynamic GetDynamicConf() const {
        return dynamic_conf_;
    }
    inline void SetDynamicConf(folly::dynamic dynamic_conf) {
        dynamic_conf_ = dynamic_conf;
    }
    
    folly::dynamic GetValue(const std::string& k) const;

    // TODO(@bneqi):
    //   提供方便使用的接口

protected:
    folly::dynamic dynamic_conf_ = nullptr;
};

// services

#endif
