#include "net/base/service_factory_manager.h"

#include <folly/Singleton.h>

folly::Singleton<ServiceFactoryManager> g_service_factory_manager;

std::shared_ptr<ServiceFactoryManager> ServiceFactoryManager::GetInstance() {
    return g_service_factory_manager.try_get();
}

void ServiceFactoryManager::RegisterServiceFactory(std::shared_ptr<ServiceBaseFactory> factory) {
    factories_.push_back(factory);
}

std::shared_ptr<ServiceBase> ServiceFactoryManager::CreateService(const ServiceConfig& config) {
    std::shared_ptr<ServiceBase> service;
    
    for (auto& factory : factories_) {
        if (factory->GetType() == config.service_type) {
            if (factory->GetName() == config.service_name) {
                service = factory->CreateInstance(config);
                break;
            }
        }
    }
    
    return service;
}

