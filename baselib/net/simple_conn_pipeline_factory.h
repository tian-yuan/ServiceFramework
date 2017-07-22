#ifndef BASE_NET_SIMPLE_CONN_PIPELINE_FACTORY_H_
#define BASE_NET_SIMPLE_CONN_PIPELINE_FACTORY_H_

#include "net/simple_conn_handler.h"

// 新客户端
class SimpleConnPipelineFactory : public PipelineFactory<SimpleConnPipeline> {
public:
    explicit SimpleConnPipelineFactory(SocketEventCallback* callback, const std::string& name = "") :
        callback_(callback),
        name_(name) {}
    
    SimpleConnPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock);
    
private:
    SocketEventCallback* callback_{nullptr};
    std::string name_;
};

// 服务器用
class SimpleServerConnPipelineFactory : public PipelineFactory<SimpleConnPipeline> {
public:
    explicit SimpleServerConnPipelineFactory(SocketEventCallback* callback, const std::string& name = "") :
        callback_(callback),
        name_(name) {}
    
    SimpleConnPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock);
    
private:
    SocketEventCallback* callback_{nullptr};
    std::string name_;
};

#endif
