#ifndef BASE_CONFIG_UTIL_H_
#define BASE_CONFIG_UTIL_H_

#include "base/configurable.h"

#include <iostream>
#include <vector>
#include <mutex>
#include <folly/FBString.h>

// 服务配置信息
struct ServiceConfig : public Configurable {
    virtual ~ServiceConfig() = default;

    // Override from Configurable
    bool SetConf(const std::string& conf_name, const Configuration& conf) override;

    std::string ToString() const;
    void PrintDebug() const;
    
    std::string service_name;   // 服务名
    std::string service_type;   // 服务类型：server/client/redis/db...
    std::string hosts;          // 主机地址（单台机器使用的多个IP采用‘,’分割）
    uint32_t port;                  // 端口号
    uint32_t max_conn_cnt{0};       // 1. 对于Listen为最大连接数
                                    // 2. 对于Connect为连接池大小
                                    // 3. 数据库或cache为池大小
};

std::vector<std::shared_ptr<ServiceConfig>> ToServiceConfigs(const Configuration& conf);

struct ServicesConfig : public Configurable {
    virtual ~ServicesConfig() = default;
    
    bool SetConf(const std::string& conf_name, const Configuration& conf) override {
        service_configs = ToServiceConfigs(conf);
        return true;
    }

    void PrintDebug() const {
        for(auto& v : service_configs) {
            v->PrintDebug();
        }
    }

    std::vector<std::shared_ptr<ServiceConfig>> service_configs;
};

struct SystemConfig : public Configurable {
    virtual ~SystemConfig() = default;

    bool SetConf(const std::string& conf_name, const Configuration& conf) override;

    void PrintDebug() const {
        std::cout << "io_thread_pool_size: " << io_thread_pool_size << std::endl;
    }

    // AccessServerConfig access_config_;
    uint32_t io_thread_pool_size = {0};     // 1：为单线程，
                                            // 未设置：为一个核一个线程
                                            // n：为n个线程
    
    // uint32_t srv_number;                    // 服务器编号
    // 机房.集群.组.服务名.编号

    // LOG
};

struct SecurityKeyConfig : public Configurable {
    virtual ~SecurityKeyConfig() = default;

    bool SetConf(const std::string& conf_name, const Configuration& conf) override;

    void PrintDebug() const;

    std::string get_server_public_key() {
    	std::lock_guard<std::mutex> guard(mutex_);
    	return server_public_key_;
    }
    std::string get_server_private_key() {
    	return server_private_key_;
    }
    std::string get_server_public_sign_key() {
    	std::lock_guard<std::mutex> guard(mutex_);
    	return server_public_sign_key_;
    }
    std::string get_server_private_sign_key() {
    	std::lock_guard<std::mutex> guard(mutex_);
    	return server_private_sign_key_;
    }
    std::string get_ticket_key() {
    	std::lock_guard<std::mutex> guard(mutex_);
    	return ticket_key_;
    }
    uint32_t get_aes_timeout() {
    	std::lock_guard<std::mutex> guard(mutex_);
    	return aes_timeout_;
    }
    uint8_t get_server_public_key_version() {
    	std::lock_guard<std::mutex> guard(mutex_);
    	return server_public_key_version_;
    }

private:
    std::mutex mutex_;							///< 配置更新锁
    std::string server_public_key_;				///< 服务端公钥
    std::string server_private_key_;			///< 服务端私钥
    std::string server_public_sign_key_;		///< 服务端sign公钥
	std::string server_private_sign_key_;		///< 服务端sign私钥
    std::string ticket_key_;					///< 用于加密 ticket
    uint32_t aes_timeout_;					///< aes_key 过期时间
    uint8_t server_public_key_version_;		///< 服务端公钥的版本
};

typedef std::shared_ptr<SecurityKeyConfig> SecurityKeyConfigPtr;
SecurityKeyConfigPtr GetSecurityKeyConfigInstance();

#endif
