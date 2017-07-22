#ifndef NET_SERVER_TCP_CLIENT_H_
#define NET_SERVER_TCP_CLIENT_H_

#include "net/server/tcp_service_base.h"
#include "net/base/client_bootstrap2.h"

//  如果连接断开以后，如何保证数据可靠
//  先不管
// 一旦分配到一个线程，即使断开重连，也落在这个线程里
class TcpClientGroup;
class TcpClient : public TcpServiceBase, public wangle::PipelineManager{
public:
    TcpClient(const ServiceConfig& config, const IOThreadPoolExecutorPtr& io_group)
        : TcpServiceBase(config, io_group),
          client_(std::make_shared<wangle::ClientBootstrap2<ConnPipeline>>(io_group ? io_group->getEventBase() : nullptr)),
          conn_address_(config.hosts.c_str(), static_cast<uint16_t>(config.port)) {
    	VLOG(4) << "TcpClient::TcpClient, numThreads : " << io_group->numThreads()
    			<< ", event base : " << client_->getEventBase()
    			<< ", host : " << config.hosts << ", port : " << config.port;
    }

    virtual ~TcpClient();

    // 设置所属分组
    void set_tcp_client_group(TcpClientGroup* tcp_client_group) {
        tcp_client_group_ = tcp_client_group;
    }
    
    ConnPipeline* GetPipeline() {
        return client_ ? client_->getPipeline() : nullptr;
    }

    folly::EventBase* GetEventBase() {
        return client_ ? client_->getEventBase() : nullptr;
    }
    
    // Impl from TcpServiceBase
    ServiceModuleType GetModuleType() const override {
        return ServiceModuleType::TCP_CLIENT;
    }

    void SetChildPipeline(std::shared_ptr<ClientConnPipelineFactory> factory) {
        factory_ = factory;
    }
                  
    bool Start() override;
    bool Pause() override;
    bool Stop() override;

    // PipelineManager implementation
    void deletePipeline(wangle::PipelineBase* pipeline) override;
    void refreshTimeout() override;

    bool SendIOBufThreadSafe(std::unique_ptr<folly::IOBuf> data);
    bool SendIOBufThreadSafe(std::unique_ptr<folly::IOBuf> data, const std::function<void()>& c);

    // EventBase线程里执行
    uint64_t OnNewConnection(IMConnPipeline* pipeline) override;
    // EventBase线程里执行
    bool OnConnectionClosed(uint64_t conn_id, IMConnPipeline* pipeline) override;

    bool connected() {
        return connected_.load();
    }

private:
    void DoHeartBeat(bool is_send);
    void DoConnect(int timeout = 0);
    
    // std::shared_ptr<IOThreadPoolConnManager> conns_;
    // 指定该连接所属EventBase线程
    // folly::EventBase* base_ = nullptr;
    // IOThreadConnManager* conn_{nullptr};
    std::shared_ptr<ClientConnPipelineFactory> factory_;
    std::shared_ptr<wangle::ClientBootstrap2<IMConnPipeline>> client_;
    std::atomic<bool> connected_ {false};
    bool stoped_ {false};
    
    // 所属分组
    TcpClientGroup* tcp_client_group_ {nullptr};
    folly::SocketAddress conn_address_;
};

//////////////////////////////////////////////////////////////////////////////
class TcpClientFactory : public ServiceBaseFactory {
public:
    using TcpClientFactoryPtr = std::shared_ptr<TcpClientFactory>;

    TcpClientFactory(const std::string& name, const IOThreadPoolExecutorPtr& io_group)
        : ServiceBaseFactory(name),
          io_group_(io_group) {}
    virtual ~TcpClientFactory() = default;

    static TcpClientFactoryPtr GetDefaultFactory(const IOThreadPoolExecutorPtr& io_group);
    static TcpClientFactoryPtr CreateFactory(const std::string& name,
                                             const IOThreadPoolExecutorPtr& io_group);

    const std::string& GetType() const override;
    std::shared_ptr<ServiceBase> CreateInstance(const ServiceConfig& config) const override;

protected:
    IOThreadPoolExecutorPtr io_group_;
};


#endif
