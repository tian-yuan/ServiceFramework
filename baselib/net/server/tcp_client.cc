#include "net/server/tcp_client.h"

#include <glog/logging.h>

#include <folly/Random.h>
#include <folly/SocketAddress.h>
#include <folly/MoveWrapper.h>

#include "net/thread_local_conn_manager.h"
#include "net/server/tcp_client_group.h"

#define RECONNECT_TIMEOUT 10000 // 重连间隔时间：10s
#define HEARTBEAT_TIMEOUT 10000 // 心跳间隔时间：10s

//////////////////////////////////////////////////////////////////////////////
TcpClient::~TcpClient() {
    if (client_ && client_->getPipeline()) {
        client_->getPipeline()->setPipelineManager(nullptr);
    }
}

bool TcpClient::Start() {
    client_->pipelineFactory(factory_);
    auto main_eb = client_->getEventBase();
    main_eb->runImmediatelyOrRunInEventBaseThreadAndWait([this]() {
        DoConnect();
    });
    return true;
}

bool TcpClient::Pause() {
    return true;
}

bool TcpClient::Stop() {
    LOG(INFO) << "TcpClient - Stop service: " << GetServiceConfig().ToString();

    auto main_eb = client_->getEventBase();
    main_eb->runImmediatelyOrRunInEventBaseThreadAndWait([&]() {
        stoped_ = true;
        if (connected_.load()) {
            client_->getPipeline()->close();
        }
        client_.reset();
    });
    
    return true;
}

void TcpClient::DoConnect(int timeout) {
    client_->connect(conn_address_, std::chrono::milliseconds(timeout))
    .then([this](ConnPipeline* pipeline) {
        LOG(INFO) << "TcpClient - Connect sucess: " << config_.ToString();
        pipeline->setPipelineManager(this);
        this->connected_.store(true);
    })
    .onError([this, timeout](const std::exception& ex) {
        LOG(ERROR) << "TcpClient - Error connecting to : "
            << config_.ToString()
            << ", exception: " << folly::exceptionStr(ex);
        
        // folly::EventBaseManager::get()->getEventBase();
        auto main_eb = client_->getEventBase();
        main_eb->runAfterDelay([&] {
            DoConnect(timeout);
        }, RECONNECT_TIMEOUT);
    });
}

void TcpClient::deletePipeline(wangle::PipelineBase* pipeline) {
    connected_.store(false);

    CHECK(client_->getPipeline() == pipeline);

    if (!stoped_) {
        auto main_eb = client_->getEventBase();
        // folly::EventBase* main_eb = folly::EventBaseManager::get()->getEventBase();
        main_eb->runAfterDelay([&] {
            this->DoConnect();
            // this->Start();
        }, RECONNECT_TIMEOUT);
    }
}

void TcpClient::refreshTimeout() {
//    LOG(INFO) << "refreshTimeout ";
}

uint64_t TcpClient::OnNewConnection(ConnPipeline* pipeline) {
    return GetConnManagerByThreadLocal().OnNewConnection(pipeline);
}

bool TcpClient::OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) {
    return GetConnManagerByThreadLocal().OnConnectionClosed(conn_id, pipeline);
}

bool TcpClient::SendIOBufThreadSafe(std::unique_ptr<folly::IOBuf> data) {
    auto main_eb = client_->getEventBase();
    auto o = folly::makeMoveWrapper(std::move(data));
    //std::shared_ptr<folly::IOBuf> o(std::move(data));
    main_eb->runInEventBaseThread([this, o]() {
        if (connected()) {
            client_->getPipeline()->write(std::move((*o)->clone()));
        }
    });
    return true;
}

bool TcpClient::SendIOBufThreadSafe(std::unique_ptr<folly::IOBuf> data, const std::function<void()>& c) {
    auto main_eb = client_->getEventBase();
    
    std::function<void()> c2 = c;
    // std::shared_ptr<folly::IOBuf> o(std::move(data));
    auto o = folly::makeMoveWrapper(std::move(data));

    main_eb->runInEventBaseThread([this, o, c2]() {
        if (connected()) {
            client_->getPipeline()->write(std::move((*o)->clone()));
            c2();
        }
    });
    return true;
}

//////////////////////////////////////////////////////////////////////////////
std::shared_ptr<TcpClientFactory> TcpClientFactory::GetDefaultFactory(const IOThreadPoolExecutorPtr& io_group) {
    static auto g_default_factory = std::make_shared<TcpClientFactory>("tcp_client", io_group);

    return g_default_factory;
}

std::shared_ptr<TcpClientFactory> TcpClientFactory::CreateFactory(const std::string& name,
                                                                  const IOThreadPoolExecutorPtr& io_group) {
    return std::make_shared<TcpClientFactory>(name, io_group);
}

const std::string& TcpClientFactory::GetType() const {
    static std::string g_tcp_client_service_name("tcp_client");
    return g_tcp_client_service_name;
}

std::shared_ptr<ServiceBase> TcpClientFactory::CreateInstance(const ServiceConfig& config) const {
    std::shared_ptr<TcpClient> tcp_client = std::make_shared<TcpClient>(config, io_group_);
    auto factory = std::make_shared<ClientConnPipelineFactory>(tcp_client.get());
    // factory->SetServiceBase(tcp_client.get());
    tcp_client->SetChildPipeline(factory);
    return tcp_client;
}
