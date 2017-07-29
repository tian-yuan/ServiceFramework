#ifndef NET_CONN_PIPELINE_FACTORY_H_
#define NET_CONN_PIPELINE_FACTORY_H_

#include <string>
#include <wangle/channel/AsyncSocketHandler.h>
#include "net/server/tcp_service_base.h"

using ConnPipeline = wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;

class ConnPipelineFactory : public wangle::PipelineFactory<ConnPipeline> {
public:
    ConnPipelineFactory(ServiceBase* service)
            : service_(service) {}

    ConnPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock);

private:
    ServiceBase* service_{nullptr};
};

class ClientConnPipelineFactory : public wangle::PipelineFactory<ConnPipeline> {
public:
    ClientConnPipelineFactory(ServiceBase* service)
            : service_(service) {}

    ConnPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock);

private:
    ServiceBase* service_{nullptr};
};

class ServerConnPipelineFactory : public wangle::PipelineFactory<ConnPipeline> {
public:
    ServerConnPipelineFactory(ServiceBase* service)
            : service_(service) {}

    ConnPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock);

private:
    ServiceBase* service_{nullptr};
};

#endif

