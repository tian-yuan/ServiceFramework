#include "net/service_manager.h"

#include <glog/logging.h>
#include <folly/MoveWrapper.h>

#include "net/base/service_factory_manager.h"
#include "net/thread_local_conn_manager.h"

#include "net/server/tcp_client_group.h"
#include "net/server/tcp_server.h"
#include "net/server/http_server.h"

folly::Singleton<ServiceManager> g_service_manager;

std::shared_ptr<ServiceManager> ServiceManager::GetInstance() {
    return g_service_manager.try_get();
}

void ServiceManager::set_thread_groups(const std::shared_ptr<ThreadGroupListManager>& thread_groups) {
    thread_groups_ = thread_groups;
    
    // 注册默认服务名"tcp_server"
    auto acc = thread_groups_->GetIOThreadPoolExecutor(ThreadType::CONN_ACCEPT);
    if (acc) {
        ServiceFactoryManager::GetInstance()->RegisterServiceFactory(TcpServerFactory::GetDefaultFactory(acc));
    }

    // 注册默认服务名"tcp_client"
    auto conn = thread_groups_->GetIOThreadPoolExecutor(ThreadType::CONN_CLIENT);
    if (conn) {
        ServiceFactoryManager::GetInstance()->RegisterServiceFactory(TcpClientFactory::GetDefaultFactory(conn));
    }
}


void ServiceManager::RegisterServiceFactory(const std::string& name,
                                                            const std::string& type) {
    std::shared_ptr<ServiceBaseFactory> factory;
    std::shared_ptr<wangle::IOThreadPoolExecutor> io_group;
    
    if (type == "tcp_server") {
        io_group = thread_groups_->GetIOThreadPoolExecutor(ThreadType::CONN_ACCEPT);
        if (io_group) {
            factory = TcpServerFactory::CreateFactory(name, io_group);
        } else {
            LOG(ERROR) << "RegisterServiceFactory - Unknow set_up conn_accept: name = " << name << ", type = " << type;
        }
    } else if (type == "tcp_client") {
        io_group = thread_groups_->GetIOThreadPoolExecutor(ThreadType::CONN_CLIENT);
        if (io_group) {
            factory = TcpClientFactory::CreateFactory(name, io_group);
        } else {
            LOG(ERROR) << "RegisterServiceFactory - Unknow set_up conn_client: name = " << name << ", type = " << type;
        }
    } else if (type == "http_server") {
        io_group = thread_groups_->GetIOThreadPoolExecutor(ThreadType::CONN_ACCEPT);
        if (io_group) {
            factory = HttpServerFactory::CreateFactory(name, io_group);
        } else {
            LOG(ERROR) << "RegisterServiceFactory - Unknow set_up conn_accept: name = " << name << ", type = " << type;
        }
    } else {
        LOG(ERROR) << "RegisterServiceFactory - UNKNOWN service_type: name = " << name << ", type = " << type;
    }

    if (factory) {
        ServiceFactoryManager::GetInstance()->RegisterServiceFactory(factory);
    }
}


void ServiceManager::SetupService(const ServicesConfig& config) {
    LOG(INFO) << "service manager setup service : ";
    config.PrintDebug();
    service_configs_ = config.service_configs;
}

bool ServiceManager::Start() {
    // 从配置文件里读取
    for (auto& service_config : service_configs_) {
        // 创建出一个service
        auto service = ServiceFactoryManager::GetInstance()->CreateService(*service_config);
        if (service) {
            // tcp_client需要特殊处理，在RegisterServiceBase
            if (RegisterServiceBase(service)) {
                // service->Initialize(*service_config);
                LOG(INFO) << "start service : ";
                service_config->PrintDebug();
                service->Start();
            } else {
                LOG(ERROR) << "Start - RegisterServiceBase error by " << service->GetServiceName();
            }
        } else {
            LOG(ERROR) << "Start - CreateService error, type = " << service_config->ToString();
        }
    }
    return true;
}

bool ServiceManager::Pause() {
    return true;
}

bool ServiceManager::Stop() {
    for (auto it = services_.begin(); it!=services_.end(); ++it) {
        it->second->Stop();
    }
    // services_.clear();
    return true;
}

inline std::string ToString(uint64_t conn_id) {
    return folly::sformat("[conn_id={}, thread_idx={}, conn_idx={}]", conn_id, conn_id>>32, conn_id & 0xffffffff);
}

bool ServiceManager::SendIOBuf(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data) {
    auto evb = thread_groups_->GetEventBaseByThreadID(conn_id >> 32);
    if (!evb) {
        LOG(ERROR) << "SendIOBufByConnID - thread_id not find, invalid conn_id: " << ToString(conn_id);
        return false;
    }
    
    // std::shared_ptr<folly::IOBuf> d = std::move(data);
    auto d = folly::makeMoveWrapper(std::move(data));

    // TODO(@benqi):
    //   C++14支持lambada捕获
    //   线上gcc版本是否支持c++11
    evb->runInEventBaseThread([conn_id, d]() mutable {
        GetConnManagerByThreadLocal().SendIOBufByConnID(conn_id, std::move((*d)->clone()));
    });
    return true;
}

bool ServiceManager::SendIOBuf(const std::string& service_name,
                               std::unique_ptr<folly::IOBuf> data,
                               DispatchStrategy dispatch_strategy) {
    bool rv = false;
    // 首先找到tcp_client_group
    auto service = Lookup(service_name);
    if (service) {
        if (service->GetModuleType() == ServiceModuleType::TCP_CLIENT_GROUP) {
            rv = std::static_pointer_cast<TcpClientGroup>(service)->SendIOBuf(std::move(data), dispatch_strategy);
        } else {
            LOG(ERROR) << "SendIOBuf - service_name's module_type not TCP_CLIENT_GROUP, "
                        << service_name << " type is " << static_cast<int>(service->GetModuleType());
        }
    } else {
        LOG(ERROR) << "SendIOBuf - not lookup: " << service_name;
    }
    return rv;
}

bool ServiceManager::SendIOBuf(const std::string& service_name,
               std::unique_ptr<folly::IOBuf> data,
               const std::function<void()>& c) {
    
    bool rv = false;
    // 首先找到tcp_client_group
    auto service = Lookup(service_name);
    if (service) {
        if (service->GetModuleType() == ServiceModuleType::TCP_CLIENT_GROUP) {
            rv = std::static_pointer_cast<TcpClientGroup>(service)->SendIOBuf(std::move(data), c);
        } else {
            LOG(ERROR) << "SendIOBuf - service_name's module_type not TCP_CLIENT_GROUP, "
            << service_name << " type is " << static_cast<int>(service->GetModuleType());
        }
    } else {
        LOG(ERROR) << "SendIOBuf - not lookup: " << service_name;
    }
    return rv;
}

// 查找分组
std::shared_ptr<ServiceBase> ServiceManager::Lookup(const std::string& service_name) {
    std::shared_ptr<ServiceBase> service;
    
    auto it = services_.find(service_name);
    if (it!=services_.end()) {
        service = it->second;
    }
    
    return service;
}

bool ServiceManager::RegisterServiceBase(std::shared_ptr<ServiceBase> service) {
    bool rv = false;
    
    LOG(INFO) << "RegisterServiceBase - " << service->GetServiceName();
    
    auto it = services_.find(service->GetServiceName());
    
    // bool is_group = service->GetServiceType() == "tcp_client";
    if (service->GetServiceType() == "tcp_client") {
        // tcp_client
        if (it != services_.end()) {
            // it找到，必须检查是否是组，而且类型相同
            if (it->second->GetModuleType() == ServiceModuleType::TCP_CLIENT_GROUP &&
                service->GetServiceType() == it->second->GetServiceType()) {
                
                LOG(INFO) << "RegisterServiceBase - tcp_client: register group's service " << service->GetServiceName();
                
                auto tcp_client_group = std::static_pointer_cast<TcpClientGroup>(it->second);
                // TODO(@benqi)
                //  修改成通过TcpClientGroup来创建和加入
                auto tcp_client = std::static_pointer_cast<TcpClient>(service);
                tcp_client->set_tcp_client_group(tcp_client_group.get());
                // tcp_client->SetThreadConnManager(std::static_pointer_cast<TcpClientGroup>(it->second)->GetThreadConnManager());
                tcp_client_group->RegisterTcpClient(tcp_client);
                rv = true;
            } else {
                // 非组或非相同服务类型，则配置有问题
                LOG(FATAL) << "RegisterServiceBase - tcp_client: not group, duplicated service name: " << service->GetServiceName();
            }
        } else {
            // 未找到，则加进
            LOG(INFO) << "RegisterServiceBase - tcp_client: register group's service " << service->GetServiceName();
            auto tcp_client_group = std::make_shared<TcpClientGroup>(service->GetServiceConfig(),
                                                          thread_groups_->GetIOThreadPoolExecutor(ThreadType::CONN_CLIENT));
            auto tcp_client = std::static_pointer_cast<TcpClient>(service);
            tcp_client->set_tcp_client_group(tcp_client_group.get());
            tcp_client_group->RegisterTcpClient(tcp_client);
            services_.insert(std::make_pair(service->GetServiceName(), tcp_client_group));
            rv = true;
        }
    } else if (service->GetServiceType() == "tcp_server") {
        // tcp_server
        if (it == services_.end()) {
            LOG(INFO) << "RegisterServiceBase - tcp_server: register tcp_server service " << service->GetServiceName();
            services_.insert(std::make_pair(service->GetServiceName(), service));
            rv = true;
        } else {
            LOG(FATAL) << "RegisterServiceBase - tcp_server: not group, duplicated service name: " << service->GetServiceName();
        }
    } else if (service->GetServiceType() == "secure_tcp_server") {
        // secure_tcp_server
        if (it == services_.end()) {
            LOG(INFO) << "RegisterServiceBase - secure_tcp_server: register secure_tcp_server service " << service->GetServiceName();
            services_.insert(std::make_pair(service->GetServiceName(), service));
            rv = true;
        } else {
            LOG(FATAL) << "RegisterServiceBase - tcp_server: not group, duplicated service name: " << service->GetServiceName();
        }
    } else if (service->GetServiceType() == "http_server") {
        // http_server
        if (it == services_.end()) {
            LOG(INFO) << "RegisterServiceBase - http_server: register http_server service " << service->GetServiceName();
            services_.insert(std::make_pair(service->GetServiceName(), service));
            rv = true;
        } else {
            LOG(FATAL) << "RegisterServiceBase - http_server: not group, duplicated service name: " << service->GetServiceName();
        }
    } else {
        // invalid service_type
        LOG(FATAL) << "RegisterServiceBase - invalid service type: " << service->GetServiceType();
    }
    
    return rv;
}

