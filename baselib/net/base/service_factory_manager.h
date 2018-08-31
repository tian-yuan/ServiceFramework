#ifndef NET_BASE_SERVICE_FACTORY_MANAGER_H_
#define NET_BASE_SERVICE_FACTORY_MANAGER_H_

#include <folly/Singleton.h>

#include "base/config_util.h"
#include "net/base/service_base.h"

//服务工厂管理器，通过配置文件创建服务
class ServiceFactoryManager {
public :
    ~ServiceFactoryManager() = default;
    
    static std::shared_ptr<ServiceFactoryManager> GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////
    void RegisterServiceFactory(std::shared_ptr<ServiceBaseFactory> factory);
    
    std::shared_ptr<ServiceBase> CreateService(const ServiceConfig& config);
    
private:
    friend class folly::Singleton<ServiceFactoryManager>;

    std::vector<std::shared_ptr<ServiceBaseFactory>> factories_;
};


#define REGISTER_SERVICE_FACTORY2(FACTORY, service_name, pool) \
    ServiceFactoryManager::GetInstance()->RegisterServiceFactory(FACTORY::CreateFactory(std::string(service_name), pool));
#endif
