#ifndef BASE_CONFIGURABLE_H_
#define BASE_CONFIGURABLE_H_

#include <folly/dynamic.h>

class Configuration;

// 动态配置框架
struct Configurable {
public:
    virtual ~Configurable() = default;
    
    // 由Conf生成
    virtual bool SetConf(const Configuration& conf) {
        return true;
    }

    // 由Conf生成
    virtual bool SetConf(const std::string& conf_name, const Configuration& conf) {
        return true;
    }
};

#endif
