#include "net/server/tcp_server.h"

#include "net/thread_local_conn_manager.h"

// 4分钟未收到数据后主动关闭
#define DEFAULT_CONN_IDLE_TIMEOUT 240000

TcpServer::TcpServer(const ServiceConfig& config,
                     const IOThreadPoolExecutorPtr& io_group)
    : TcpServiceBase(config, io_group) {
        
    wangle::ServerSocketConfig acc_config;
    acc_config.connectionIdleTimeout = std::chrono::milliseconds(DEFAULT_CONN_IDLE_TIMEOUT);
    server_.acceptorConfig(acc_config);
}

bool TcpServer::Start() {
    server_.childPipeline(factory_);
    server_.group(io_group_);
    server_.bind(config_.port);

    return true;
}

bool TcpServer::Pause() {
    return true;
}

bool TcpServer::Stop() {
    LOG(INFO) << "TcpServer - Stop service: " << config_.ToString();

    server_.stop();
    return true;
}

// EventBase线程里执行
uint64_t TcpServer::OnNewConnection(IMConnPipeline* pipeline) {
    return GetConnManagerByThreadLocal().OnNewConnection(pipeline);
}

// EventBase线程里执行
bool TcpServer::OnConnectionClosed(uint64_t conn_id, IMConnPipeline* pipeline) {
    return GetConnManagerByThreadLocal().OnConnectionClosed(conn_id, pipeline);
}

//////////////////////////////////////////////////////////////////////////////
TcpServerFactory::TcpServerFactoryPtr TcpServerFactory::GetDefaultFactory(const IOThreadPoolExecutorPtr& io_group) {
    static auto g_default_factory =
        std::make_shared<TcpServerFactory>("tcp_server", io_group);
    
    return g_default_factory;
}

TcpServerFactory::TcpServerFactoryPtr TcpServerFactory::CreateFactory(const std::string& name,
                                                                      const IOThreadPoolExecutorPtr& io_group) {
    return std::make_shared<TcpServerFactory>(name, io_group);
}

const std::string& TcpServerFactory::GetType() const {
    static std::string g_tcp_server_service_name("tcp_server");
    return g_tcp_server_service_name;
}

std::shared_ptr<ServiceBase> TcpServerFactory::CreateInstance(const ServiceConfig& config) const {
    LOG(INFO) << "CreateInstance - " << config.ToString();

    auto tcp_server = std::make_shared<TcpServer>(config, io_group_);
    auto factory = std::make_shared<ServerConnPipelineFactory>(tcp_server.get());
    tcp_server->SetChildPipeline(factory);
    return tcp_server;
}
