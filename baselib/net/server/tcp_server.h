#ifndef NET_SERVER_TCP_SERVER_H_
#define NET_SERVER_TCP_SERVER_H_

#include <wangle/bootstrap/ServerBootstrap.h>

#include "net/server/tcp_service_base.h"
#include "net/conn_handler.h"
#include "net/conn_pipeline_factory.h"
#include "net/io_thread_pool_manager.h"

enum NetModuleState {
    kNetModuleState_None = 0,
};

class TcpServer : public TcpServiceBase {
public:
    TcpServer(const ServiceConfig& config, const IOThreadPoolExecutorPtr& io_group);
    virtual ~TcpServer() = default;
    
    void SetChildPipeline(std::shared_ptr<ServerConnPipelineFactory> factory) {
        factory_ = factory;
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
    std::shared_ptr<ServerConnPipelineFactory> factory_;
    wangle::ServerBootstrap<ConnPipeline> server_;
};

//////////////////////////////////////////////////////////////////////////////
class TcpServerFactory : public ServiceBaseFactory {
public:
    using TcpServerFactoryPtr = std::shared_ptr<TcpServerFactory>;
    
    TcpServerFactory(const std::string& name, const IOThreadPoolExecutorPtr& io_group)
        : ServiceBaseFactory(name),
          io_group_(io_group) {}

    virtual ~TcpServerFactory() = default;
    
    static TcpServerFactoryPtr GetDefaultFactory(const IOThreadPoolExecutorPtr& io_group);
    static TcpServerFactoryPtr CreateFactory(const std::string& name,
                                             const IOThreadPoolExecutorPtr& io_group);
    
    const std::string& GetType() const override;
    std::shared_ptr<ServiceBase> CreateInstance(const ServiceConfig& config) const override;
    
protected:
    IOThreadPoolExecutorPtr io_group_;
};

#endif
