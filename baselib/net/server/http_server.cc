#include "net/server/http_server.h"

#include "net/io_thread_pool_manager.h"
#include "net/rabbit_http_request_handler.h"

#define HTTP_IDLE_TIMEOUT 60000

//////////////////////////////////////////////////////////////////////////////
bool HttpServer::Start() {
    std::vector<proxygen::HTTPServerLib::IPConfig> IPs = {
        {folly::SocketAddress(config_.hosts.c_str(), static_cast<uint16_t>(config_.port), true), proxygen::HTTPServerLib::Protocol::HTTP},
    };
    
    proxygen::HTTPServerOptions options;
    // options.threads = static_cast<size_t>(FLAGS_threads);
    options.idleTimeout = std::chrono::milliseconds(HTTP_IDLE_TIMEOUT);
    // options.shutdownOn = {SIGINT, SIGTERM};
    // options.enableContentCompression = true;
    
    options.handlerFactories = proxygen::RequestHandlerChain()
        .addThen<RabbitHttpRequestHandlerFactory>()
        .build();
    
    if (!server_) {
        server_ = folly::make_unique<proxygen::HTTPServerLib>(std::move(options));
    }
    
    server_->bind(IPs);
    server_->start(io_group_);
    
    return true;
}

bool HttpServer::Pause() {
    return true;
}

bool HttpServer::Stop() {
    LOG(INFO) << "HttpServer - Stop service: " << config_.ToString();

    server_->stop();
    return true;
}

//////////////////////////////////////////////////////////////////////////////
std::shared_ptr<HttpServerFactory> HttpServerFactory::GetDefaultFactory(const std::shared_ptr<wangle::IOThreadPoolExecutor>& io_group) {
    static auto g_default_factory = std::make_shared<HttpServerFactory>("http_server", io_group);
    return g_default_factory;
}

std::shared_ptr<HttpServerFactory> HttpServerFactory::CreateFactory(const std::string& name,
                                                                    const std::shared_ptr<wangle::IOThreadPoolExecutor>& io_group) {
    return std::make_shared<HttpServerFactory>(name, io_group);
}

const std::string& HttpServerFactory::GetType() const {
    static std::string g_http_server_service_name("http_server");
    return g_http_server_service_name;
}

std::shared_ptr<ServiceBase> HttpServerFactory::CreateInstance(const ServiceConfig& config) const {
    LOG(INFO) << "CreateInstance - " << config.ToString();
    std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>(config, io_group_);
    return server;
}
