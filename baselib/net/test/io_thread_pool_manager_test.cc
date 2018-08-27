#include "net/io_thread_pool_manager.h"
#include "net/server/tcp_server_t.h"
#include "base/configuration.h"

#include <folly/json.h>
#include <wangle/codec/ByteToMessageDecoder.h>
#include <wangle/channel/Handler.h>

folly::dynamic parseJsonString(folly::StringPiece s,
                               bool allow_trailing_comma = true) {
    folly::json::serialization_opts opts;
    opts.allow_trailing_comma = allow_trailing_comma;
    return folly::parseJson(s, opts);
}

class SimpleDecode : public wangle::ByteToByteDecoder {
public:
    bool decode(Context*,
                folly::IOBufQueue& buf,
                std::unique_ptr<folly::IOBuf>& result,
                size_t&) override {
        result = buf.move();
        return result != nullptr;
    }
};

int main() {
//    size_t thread_size = GetIOThreadSize();
//    LOG(INFO) << "thread size is : " << thread_size;
//    std::shared_ptr<IOThreadPoolConnManager> conns_;
//    conns_ = std::make_shared<IOThreadPoolConnManager>(GetIOThreadSize());
//
//    folly::StringPiece configString = "{\"service_name\":\"tcp_server\", \"service_type\":\"tcp_server\", \"hosts\":\"0.0.0.0\", \"port\":18888, \"max_conn_cnt\":2}";
//    folly::dynamic dynamicConfig = parseJsonString(configString);
//    Configuration configuration(dynamicConfig);
//    ServiceConfig config;
//    config.SetConf("test_tcp_server", configuration);
//
//    std::shared_ptr<DefaultIOThreadPoolExecutor> threadPool = std::make_shared<DefaultIOThreadPoolExecutor>();
//    static TcpServerFactory::TcpServerFactoryPtr factory = TcpServerFactory::CreateFactory("tcp_server", threadPool);
//    std::shared_ptr<ServiceBase> server_ = factory->CreateInstance(config);
//    server_->Start();
//    auto main_eb = folly::EventBaseManager::get()->getEventBase();
//    main_eb->loopForever();

    size_t thread_size = GetIOThreadSize();
    LOG(INFO) << "thread size is : " << thread_size;
    std::shared_ptr<IOThreadPoolConnManager> conns_;
    conns_ = std::make_shared<IOThreadPoolConnManager>(GetIOThreadSize());

    std::string configString = "{\"service_name\":\"tcp_server\", \"service_type\":\"tcp_server\", \"hosts\":\"0.0.0.0\", \"port\":18889, \"max_conn_cnt\":2}";
    nlohmann::json configJson = nlohmann::json::parse(configString);
    ServiceConfig config;
    config.SetConf("test_tcp_server", configJson);

    std::shared_ptr<DefaultIOThreadPoolExecutor> threadPool = std::make_shared<DefaultIOThreadPoolExecutor>();

    std::shared_ptr<TcpServerFactoryT<SimpleDecode, ConnHandler>> factory =
            std::make_shared<TcpServerFactoryT<SimpleDecode, ConnHandler>>("tcp_server", threadPool);

    std::string name = factory->GetType();
    std::shared_ptr<ServiceBase> server_ = factory->CreateInstance(config);
    server_->Start();
    auto main_eb = folly::EventBaseManager::get()->getEventBase();
    main_eb->loopForever();
    return 0;
}
