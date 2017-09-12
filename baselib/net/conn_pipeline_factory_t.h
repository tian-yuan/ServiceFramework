#ifndef NET_CONN_PIPELINE_FACTORY_T_H_
#define NET_CONN_PIPELINE_FACTORY_T_H_

#include <string>
#include <wangle/channel/AsyncSocketHandler.h>
#include "net/server/tcp_service_base.h"

using ConnPipeline = wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;

template <typename D, typename H>
class ServerConnPipelineFactoryT : public wangle::PipelineFactory<ConnPipeline> {
public:
    ServerConnPipelineFactoryT(ServiceBase* service)
            : service_(service) {}

    ConnPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock);

private:
    ServiceBase* service_{nullptr};
};

template <typename D, typename H>
ConnPipeline::Ptr ServerConnPipelineFactoryT<D, H>::newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) {
    VLOG(4)<< "ServerConnPipelineFactory::newPipeline!";
    auto pipeline = ConnPipeline::create();
    pipeline->setReadBufferSettings(1024*32, 1024*64);
    pipeline->addBack(wangle::AsyncSocketHandler(sock));
    pipeline->addBack(D());

    pipeline->addBack(H(service_));
    pipeline->finalize();

    return pipeline;
}

#endif

