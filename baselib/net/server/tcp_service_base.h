#ifndef NET_SERVER_TCP_SERVICE_BASE_H_
#define NET_SERVER_TCP_SERVICE_BASE_H_

#include <wangle/channel/Pipeline.h>
#include <wangle/concurrent/IOThreadPoolExecutor.h>

#include "net/base/service_base.h"

using ConnPipeline = wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;
using IOThreadPoolExecutorPtr = std::shared_ptr<wangle::IOThreadPoolExecutor>;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class TcpConnEventCallback {
public:
    virtual ~TcpConnEventCallback() = default;
    
    // EventBase线程里执行
    virtual uint64_t OnNewConnection(ConnPipeline* pipeline) { return 0; }
    
    // EventBase线程里执行
    virtual bool OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) { return false; }
};

class TcpServiceBase : public ServiceBase, public TcpConnEventCallback {
public:
    TcpServiceBase(const ServiceConfig& config, const IOThreadPoolExecutorPtr& io_group)
        : ServiceBase(config),
          io_group_(io_group) {}
    
    virtual ~TcpServiceBase() = default;

    inline IOThreadPoolExecutorPtr GetIOThreadPoolExecutor() const {
        return io_group_;
    }
    
    // EventBase线程里执行
    // virtual uint32_t OnNewConnection(ConnPipeline* pipeline) { return 0; }
    // EventBase线程里执行
    // virtual bool OnConnectionClosed(uint32_t conn_id, ConnPipeline* pipeline) { return false; }

protected:
    IOThreadPoolExecutorPtr io_group_;
};


std::shared_ptr<ServiceBaseFactory> CreateTcpServiceFactory(const std::string& name,
                                                            const std::string& type,
                                                            const IOThreadPoolExecutorPtr& io_group);

#endif
