#include "net/service_router_manager.h"

#include "net/server/tcp_client_group.h"
#include "net/server/tcp_server.h"


ServiceRouterManager::ServiceRouterManager(std::shared_ptr<IOThreadPoolConnManager> conn_pool)
    : conn_pool_(conn_pool) {
    
    // 注册默认服务名"tcp_server"
    service_factory_manager_.RegisterServiceFactory(TcpServerFactory::GetDefaultFactory(conn_pool_->GetIOThreadPoolExecutor()));
    
    // 注册默认服务名"tcp_client"
    service_factory_manager_.RegisterServiceFactory(TcpClientFactory::GetDefaultFactory(conn_pool_->GetIOThreadPoolExecutor()));
}

bool ServiceRouterManager::Initialize(const ServicesConfig& conf) {
    service_configs_ = conf.service_configs;// ToServiceConfigs(conf);
    return true;
}

uint64_t ServiceRouterManager::OnNewConnection(ConnPipeline* pipeline) {
    return conn_pool_->OnNewConnection(pipeline);
}

bool ServiceRouterManager::OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) {
    return conn_pool_->OnConnectionClosed(conn_id, pipeline);
}

void ServiceRouterManager::OnServerRegister(ConnPipeline* pipeline, const std::string& server_name, uint32_t server_number) {
    conn_pool_->OnServerRegister(pipeline, server_name, server_number);
}

bool ServiceRouterManager::Start() {
    // 从配置文件里读取
    for (auto& service_config : service_configs_) {
        // 创建出一个service
        auto service = service_factory_manager_.CreateService(*service_config);
        if (service) {
            // tcp_client需要特殊处理，在RegisterServiceBase
            if (RegisterServiceBase(service)) {
                service->Start();
            } else {
                LOG(ERROR) << "Start - RegisterServiceBase error by " << service->GetServiceName();
            }
        } else {
            LOG(ERROR) << "Start - CreateService error, type = " << service_config->service_type
                            << ", name = " << service_config->service_name;
        }
    }
    return true;
}

bool ServiceRouterManager::Pause() {
    return true;
}

bool ServiceRouterManager::Stop() {
    for (auto it = services_.begin(); it!=services_.end(); ++it) {
        it->second->Stop();
    }
    return true;
}

bool ServiceRouterManager::DispatchIOBuf(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data) {
    return conn_pool_->DispatchIOBufByConnID(conn_id, std::move(data));
}


bool ServiceRouterManager::DispatchIOBuf(const std::string& server_name, uint32_t server_number, std::unique_ptr<folly::IOBuf> data) {
    return conn_pool_->DispatchIOBufByServer(server_name, server_number, std::move(data));
}


bool ServiceRouterManager::RegisterServiceBase(std::shared_ptr<ServiceBase> service) {
    bool rv = false;
    
    LOG(INFO) << "RegisterServiceBase - " << service->GetServiceName();
    
    auto it = services_.find(service->GetServiceName());
    
    if (service->GetServiceType() == "tcp_client") {
        LOG(INFO) << "RegisterServiceBase - tcp_client : register tcp_client service " << service->GetServiceName();
    } else if (service->GetServiceType() == "tcp_server") {
        // tcp_server
        if (it == services_.end()) {
            LOG(INFO) << "RegisterServiceBase - tcp_server: register tcp_server service " << service->GetServiceName();
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

