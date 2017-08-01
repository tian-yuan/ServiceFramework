#include "net/io_thread_pool_manager.h"
#include "net/server/tcp_server.h"
#include "base/configuration.h"

#include <folly/json.h>

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

    folly::StringPiece configString = "{\"service_name\":\"tcp_server\", \"service_type\":\"tcp_server\", \"hosts\":\"0.0.0.0\", \"port\":18888, \"max_conn_cnt\":2}";
    folly::dynamic dynamicConfig = parseJsonString(configString);
    Configuration configuration(dynamicConfig);
    ServiceConfig config;
    config.SetConf("test_tcp_server", configuration);

    std::shared_ptr<DefaultIOThreadPoolExecutor> threadPool = std::make_shared<DefaultIOThreadPoolExecutor>();
    static TcpServerFactory::TcpServerFactoryPtr factory = TcpServerFactory::CreateFactory("tcp_server", threadPool);
    std::shared_ptr<ServiceBase> server_ = factory->CreateInstance(config);
    server_->Start();
    return 0;
}
