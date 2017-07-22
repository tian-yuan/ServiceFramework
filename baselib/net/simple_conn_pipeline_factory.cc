#include "net/simple_conn_pipeline_factory.h"

#include <wangle/channel/EventBaseHandler.h>

///////////////////////////////////////////////////////////////////////////////////////////
SimpleConnPipeline::Ptr SimpleConnPipelineFactory::newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) {
    auto pipeline = SimpleConnPipeline::create();
    
    // Initialize TransportInfo and set it on the pipeline
    auto transportInfo = std::make_shared<TransportInfo>();
    folly::SocketAddress localAddr, peerAddr;
    sock->getLocalAddress(&localAddr);
    sock->getPeerAddress(&peerAddr);
    transportInfo->localAddr = std::make_shared<folly::SocketAddress>(localAddr);
    transportInfo->remoteAddr = std::make_shared<folly::SocketAddress>(peerAddr);
    pipeline->setTransportInfo(transportInfo);
    
    pipeline->addBack(AsyncSocketHandler(sock));
    pipeline->addBack(EventBaseHandler()); // ensure we can write from any thread
    //pipeline->addBack(CImPduRawDataDecoder());
    pipeline->addBack(SimpleConnHandler(callback_, name_));
    pipeline->finalize();
    
    return pipeline;
}

///////////////////////////////////////////////////////////////////////////////////////////
SimpleConnPipeline::Ptr SimpleServerConnPipelineFactory::newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) {
    auto pipeline = SimpleConnPipeline::create();

    pipeline->addBack(AsyncSocketHandler(sock));
    //pipeline->addBack(EventBaseHandler()); // ensure we can write from any thread
    //pipeline->addBack(CImPduRawDataDecoder());
    pipeline->addBack(SimpleConnHandler(callback_, name_));
    pipeline->finalize();

    return pipeline;
}


