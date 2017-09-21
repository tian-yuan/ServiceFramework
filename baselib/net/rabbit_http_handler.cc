#include "net/rabbit_http_handler.h"

void RabbitHttpHandlerFactory::DispatchRabbitHttpHandler(const proxygen::HTTPMessage& headers,
                                                     std::unique_ptr<folly::IOBuf> body,
                                                     proxygen::ResponseBuilder& r) {
    
    std::string path = headers.getPath();
    
    auto& a = RabbitHttpHandlerFactory::GetInstance().factories_;
    auto it = a.find(path);
    if (it == a.end()) {
        LOG(ERROR) << "DispatchMessageHandler - not find path: " << path;
        return;
    }
    
    if (it->second == nullptr) {
        LOG(ERROR) << "DispatchMessageHandler - not register path: " << path;
        return;
    }
    
    // 执行
    it->second(headers, std::move(body), r);
}
