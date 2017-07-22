#ifndef NET_BASE_SERVICE_BASE_H_
#define NET_BASE_SERVICE_BASE_H_

#include "base/config_util.h"
#include "base/factory_object.h"

namespace folly {
class IOBuf;
}

// 通过服务类型和服务名创建ServiceBase
// 服务类型为:
//   tcp_server
//   tcp_client
//   udp_server
//   udp_client
//   http_server
// 服务名为每个应用自定义:
//   比如access_server的服务名access_server
//   通过配置创建ServiceBase
//   如果未设置服务名，则使用服务类型值为默认服务名为：
//    tcp_server <-> tcp_server
//    tcp_client <-> tcp_client
//    udp_server <-> udp_server
//    udp_client <-> udp_client
//    http_server <-> http_server
//    http_client <-> http_client

// 默认提供的服务配置类型
//enum class ServiceType {
//    kServiceTypeInvalid = 0,        // invalid
//    kServiceTypeTcpServer = 1,      // tcp_server
//    kServiceTypeTcpClient = 2,      // tcp_client
//    kServiceTypeUdpServer = 3,      // udp_server
//    kServiceTypeUdpClient = 4,      // udp_client
//    kServiceTypeHttpServer = 5,     // http_server
//    kServiceTypeHttpClient = 6,     // http_client
//    kServiceTypeRedis = 7,          // redis
//    kServiceTypeMysql = 8,          // mysql
//    kServiceTypeMaxSize = 9,        // 最大值
//};

// 数据转发策略
enum class DispatchStrategy : int {
    kDefault = 0,           // 默认，使用kRandom
    kRandom = 1,            // 随机
    kRoundRobin = 2,        // 轮询
    kConsistencyHash = 3,   // 一致性hash
    kBroadCast = 4,         // 广播
};
    
enum class ServiceModuleType : int {
    INVALID = 0,
    TCP_SERVER = 1,
    TCP_CLIENT_GROUP = 2,
    TCP_CLIENT_POOL = 3,
    TCP_CLIENT = 4,
    UDP_SERVER = 5,
    UDP_CLIENT = 6,
    HTTP_SERVER = 7,
    MAX_SIZE = 8
};
    
class ServiceRouterManager;
    
// 服务基础类
// 注意：
//   一个ServiceBase并不和一个ServiceType对应
//   和ServiceModuleType对应
class ServiceBase {
public:
    ServiceBase(const ServiceConfig& config) :
        config_(config) {}

    virtual ~ServiceBase() = default;
    
    // 模块名
    // ServiceBase有一些中间类型，特别是client
    // 有n个连接到同一服务不同主机上的tcp_client，我们会将这些连接分组到tcp_client_group
    // 同一主机上的同一服务我们也可能发起多个连接，我们会将这些连接分组到tcp_client_pool
    virtual ServiceModuleType GetModuleType() const = 0;
    
    // 服务配置信息
    inline const ServiceConfig& GetServiceConfig() const {
        return config_;
    }
    inline const std::string& GetServiceName() const {
        return config_.service_name;
    }
    inline const std::string& GetServiceType() const {
        return config_.service_type;
    }
    
    virtual bool Start() { return false; }
    virtual bool Pause() { return false; }
    virtual bool Stop() { return false; }
    
protected:
    ServiceConfig config_;
};

//////////////////////////////////////////////////////////////////////////////
class ServiceBaseFactory : public FactoryObject<ServiceBase> {
public:
    explicit ServiceBaseFactory(const std::string& name)
        : name_(name) {}
    
    virtual ~ServiceBaseFactory() = default;
    
    // service_name: 配置文件制定，比如gateway_server
    virtual const std::string& GetName() const {
        return name_;
    }

    virtual std::shared_ptr<ServiceBase> CreateInstance(const ServiceConfig& config) const = 0;
    
protected:
    std::shared_ptr<ServiceBase> CreateInstance() const override {
        return nullptr;
    }

    std::string name_;
};

#endif
