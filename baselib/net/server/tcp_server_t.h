#ifndef NET_SERVER_TCP_SERVER_T_H_
#define NET_SERVER_TCP_SERVER_T_H_

#include <wangle/bootstrap/ServerBootstrap.h>

#include "net/server/tcp_service_base.h"
#include "net/conn_handler.h"
#include "net/conn_pipeline_factory.h"
#include "net/conn_pipeline_factory_t.h"
#include "net/io_thread_pool_manager.h"
#include "net/thread_local_conn_manager.h"

//////////////////////////////////////////////////////////////////////////////
class TcpServerT : public TcpServiceBase {
public:
    TcpServerT(const ServiceConfig& config, const IOThreadPoolExecutorPtr& io_group);
    virtual ~TcpServerT() = default;

    template <typename D, typename H>
    void SetChildPipeline(std::shared_ptr<ServerConnPipelineFactoryT<D, H>> factory) {
        server_.childPipeline(factory);
    }

    bool Start() override;
    bool Pause() override;
    bool Stop() override;


    // Impl from TcpServiceBase
    ServiceModuleType GetModuleType() const override {
        return ServiceModuleType::TCP_SERVER;
    }

    // EventBase线程里执行
    uint64_t OnNewConnection(ConnPipeline* pipeline) override;
    // EventBase线程里执行
    bool OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) override;

private:
    // IOThreadPoolExecutorPtr io_group_;
    wangle::ServerBootstrap<ConnPipeline> server_;
};

/////////////////////////////////////////////////
template <typename D, typename H>
class TcpServerFactoryT : public ServiceBaseFactoryT {
public:
    TcpServerFactoryT(const std::string& name, const IOThreadPoolExecutorPtr& io_group)
            : ServiceBaseFactoryT(name),
              io_group_(io_group) {}

    virtual ~TcpServerFactoryT() = default;

    std::string GetType() const;
    std::shared_ptr<ServiceBase> CreateInstance(const ServiceConfig& config) const;

protected:
    IOThreadPoolExecutorPtr io_group_;
};


// 4分钟未收到数据后主动关闭
#define DEFAULT_CONN_IDLE_TIMEOUT 240000
//////////////////////////////////////////////////////////////////////////////
TcpServerT::TcpServerT(const ServiceConfig& config,
                       const IOThreadPoolExecutorPtr& io_group)
        : TcpServiceBase(config, io_group) {

    wangle::ServerSocketConfig acc_config;
    acc_config.connectionIdleTimeout = std::chrono::milliseconds(DEFAULT_CONN_IDLE_TIMEOUT);
    server_.acceptorConfig(acc_config);
}

bool TcpServerT::Start() {
    server_.group(io_group_);
    server_.bind(config_.port);

    return true;
}

bool TcpServerT::Pause() {
    return true;
}

bool TcpServerT::Stop() {
    LOG(INFO) << "TcpServer - Stop service: " << config_.ToString();

    server_.stop();
    return true;
}

// EventBase线程里执行
uint64_t TcpServerT::OnNewConnection(ConnPipeline* pipeline) {
    return GetConnManagerByThreadLocal().OnNewConnection(pipeline);
}

// EventBase线程里执行
bool TcpServerT::OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) {
    return GetConnManagerByThreadLocal().OnConnectionClosed(conn_id, pipeline);
}

///////////////////////////////////
template <typename D, typename H>
std::string TcpServerFactoryT<D, H>::GetType() const {
    return "tcp_server_t";
}

template <typename D, typename H>
std::shared_ptr<ServiceBase> TcpServerFactoryT<D, H>::CreateInstance(const ServiceConfig& config) const {
    LOG(INFO) << "CreateInstance - " << config.ToString();

    auto tcp_server = std::make_shared<TcpServerT>(config, io_group_);
    auto factory = std::make_shared<ServerConnPipelineFactoryT<D, H>>(tcp_server.get());
    tcp_server->SetChildPipeline(factory);
    return tcp_server;
}

#endif
