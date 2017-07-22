#ifndef NET_SERVER_SIMPLE_TCP_CLIENT_H_
#define NET_SERVER_SIMPLE_TCP_CLIENT_H_

// #include "net/base/service_base.h"
#include "net/simple_conn_handler.h"
// #include "net/io_thread_pool_manager.h"
#include "net/base/client_bootstrap2.h"


class SimpleTcpClient;
class ConnectErrorCallback {
public:
    virtual void OnClientConnectionError(SimpleTcpClient* tcp_client) {}
};


// TODO(@benqi)
//  如果连接断开以后，如何保证数据可靠
//  先不管
class SimpleTcpClient : public wangle::PipelineManager {
public:
    explicit SimpleTcpClient(EventBase* base,
                             SocketEventCallback* callback = nullptr,
                             const std::string& name = "", ConnectErrorCallback* connect_error_callback = nullptr) :
        callback_(callback),
        client_(base),
        name_(name),
		connect_error_callback_(connect_error_callback){

    }
    
    virtual ~SimpleTcpClient() {
        if (client_.getPipeline()) {
            client_.getPipeline()->setPipelineManager(nullptr);
        }
    }
    
    SimpleConnPipeline* GetPipeline() {
        return client_.getPipeline();
    }

    folly::EventBase* GetEventBase() {
        return client_.getEventBase();
    }

    void Connect(const std::string& ip, uint16_t port, bool isold = false);
    void ConnectByFactory(const std::string& ip,
                          uint16_t port,
                          std::shared_ptr<PipelineFactory<SimpleConnPipeline>> factory);
    
    // PipelineManager implementation
    void deletePipeline(wangle::PipelineBase* pipeline) override;
    void refreshTimeout() override;

    bool connected() {
        return connected_;
    }
    
    void SetSocketEventCallback(SocketEventCallback* cb) {
        callback_ = cb;
    }

    void SetConnectErrorCallback(ConnectErrorCallback* cb) {
        connect_error_callback_ = cb;
    }

    void clearCallBack()  {
    	callback_ = nullptr;
    	connect_error_callback_ = nullptr;
    }

    const std::string& GetName() const {
        return name_;
    }

private:
    // void DoHeartBeat(bool is_send);
    // 指定该连接所属EventBase线程
    // folly::EventBase* base_ = nullptr;
    bool connected_ {false};
    SocketEventCallback* callback_{nullptr};
    ConnectErrorCallback* connect_error_callback_{nullptr};
    
    wangle::ClientBootstrap2<SimpleConnPipeline> client_;
    std::string name_;
};


#endif
