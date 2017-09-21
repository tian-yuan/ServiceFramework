//
// Created by tianyuan on 2017/9/21.
//

#include "net/rabbit_http_handler.h"
#include "net/server/http_server.h"
#include "net/io_thread_pool_manager.h"
#include "base/configuration.h"

#include <folly/json.h>

void LoginHttpHandler(const proxygen::HTTPMessage& headers,
                      std::unique_ptr<folly::IOBuf> body,
                      proxygen::ResponseBuilder& r);
REGISTER_HTTP_HANDLER(LoginHttpHandler, "/login", LoginHttpHandler);

using namespace proxygen;

void LoginHttpHandler(const proxygen::HTTPMessage& headers,
                      std::unique_ptr<folly::IOBuf> body,
                      proxygen::ResponseBuilder& r) {

    std::string url = headers.getURL();

    std::string userId = headers.getDecodedQueryParam("userId");
    std::string appId = headers.getDecodedQueryParam("appId");
    folly::dynamic rv = folly::dynamic::object();

    if(userId.empty() || appId.empty()) {
        rv["code"] = 4004;
        rv["msg"] = "PARAM ERROR";
    } else {
        rv["code"] = 1001;
        rv["msg"] = "OK";
    }

    auto json = toJson(rv);
    json.append("\n");

    std::unique_ptr<folly::IOBuf> json_string = folly::IOBuf::copyBuffer(json.c_str(), json.length());

    // 返回给客户端
    r.header("Content-Type", "application/json;charset=utf-8").body(std::move(json_string));

    return;
}

folly::dynamic parseJsonString(folly::StringPiece s,
                               bool allow_trailing_comma = true) {
    folly::json::serialization_opts opts;
    opts.allow_trailing_comma = allow_trailing_comma;
    return folly::parseJson(s, opts);
}

int main() {
    size_t thread_size = GetIOThreadSize();
    LOG(INFO) << "thread size is : " << thread_size;
    std::shared_ptr<IOThreadPoolConnManager> conns_;
    conns_ = std::make_shared<IOThreadPoolConnManager>(GetIOThreadSize());

    std::shared_ptr<HttpServerFactory> serverFactory = HttpServerFactory::CreateFactory(
            "http_server", conns_->GetIOThreadPoolExecutor());

    folly::StringPiece configString = "{\"service_name\":\"http_server\", \"service_type\":\"http_server\", "
            "\"hosts\":\"0.0.0.0\", \"port\":18889, \"max_conn_cnt\":2}";
    folly::dynamic dynamicConfig = parseJsonString(configString);
    Configuration configuration(dynamicConfig);
    ServiceConfig config;
    config.SetConf("http_server", configuration);

    std::shared_ptr<ServiceBase> httpServer = serverFactory->CreateInstance(config);
    httpServer->Start();

    auto main_evb = folly::EventBaseManager::get()->getEventBase();
    main_evb->loopForever();
    return 0;
}
