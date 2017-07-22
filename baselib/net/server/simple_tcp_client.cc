#include "net/server/simple_tcp_client.h"

#include <glog/logging.h>

#include <folly/Random.h>
#include <folly/SocketAddress.h>

#include "net/simple_conn_pipeline_factory.h"

// #include "net/io_thread_pool_manager.h"

//////////////////////////////////////////////////////////////////////////////
void SimpleTcpClient::Connect(const std::string& ip, uint16_t port, bool isold) {
    client_.pipelineFactory(std::make_shared<SimpleConnPipelineFactory>(callback_));

    folly::SocketAddress address(ip.c_str(), port);
	client_.connect(address).then([this, ip, port](SimpleConnPipeline* pipeline) {
		LOG(INFO) << "SimpleTcpClient - Connect sucess:" << ip << ":" << port;
		pipeline->setPipelineManager(this);
		this->connected_ = true;
	}).onError([this, ip, port](const std::exception& ex) {
        // TODO(@benqi), 是否要通知
        if (connect_error_callback_) {
            connect_error_callback_->OnClientConnectionError(this);
        }
        
        if(callback_) {
        	callback_->OnConnectionError();
        }

        LOG(ERROR) << "SimpleTcpClient - Error connecting to :" << ip << ":" << port << ", exception: " << ex.what();
    });
}

void SimpleTcpClient::ConnectByFactory(const std::string& ip,
                                       uint16_t port,
                                       std::shared_ptr<PipelineFactory<SimpleConnPipeline>> factory) {
    if (!factory) {
        LOG(ERROR) << "SimpleTcpClient - ConnectByFactory, factory is null, ip = " << ip
                    << ", port = " << port;
        return;
    }
    
    client_.pipelineFactory(factory);
    folly::SocketAddress address(ip.c_str(), port);
    client_.connect(address).then([this, ip, port](SimpleConnPipeline* pipeline) {
//        LOG(INFO) << "SimpleTcpClient - Connect sucess: ip = " << ip << ", port = " << port;
        pipeline->setPipelineManager(this);
        this->connected_ = true;
    }).onError([this, ip, port](const std::exception& ex) {
        // TODO(@benqi), 是否要通知
        
        if (callback_) {
            callback_->OnConnectionError();
        }
        
        if (connect_error_callback_) {
            connect_error_callback_->OnClientConnectionError(this);
        }

        LOG(ERROR) << "SimpleTcpClient - Error connecting to :" << ip << ":" << port << ", exception: " << ex.what();
    });
    
}

void SimpleTcpClient::deletePipeline(wangle::PipelineBase* pipeline) {
//    LOG(INFO) << "deletePipeline ";

    CHECK(client_.getPipeline() == pipeline);
    connected_ = false;
}

void SimpleTcpClient::refreshTimeout() {
    // LOG(INFO) << "refreshTimeout ";
}

