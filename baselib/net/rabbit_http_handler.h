#ifndef NET_RABBIT_HTTP_HANDLER_H_
#define NET_RABBIT_HTTP_HANDLER_H_

#include <folly/experimental/StringKeyedUnorderedMap.h>

#include "net/rabbit_http_request_handler.h"

/////////////////////////////////////////////////////////////////////////////////////////
typedef void (*RabbitHttpExecuteFunc)(const proxygen::HTTPMessage& headers,
                                    std::unique_ptr<folly::IOBuf> body,
                                    proxygen::ResponseBuilder& r);

class RabbitHttpHandlerFactory {
public:
    // 辅助自注册帮助类，只有一个构造函数
    struct RegisterTemplate {
        RegisterTemplate(folly::StringPiece path, RabbitHttpExecuteFunc func) {
            auto& a = RabbitHttpHandlerFactory::GetInstance().factories_;
            auto it = a.find(path);
            if (it == a.end()) {
                a[path] = func;
            } else {
                LOG(ERROR) << "RabbitHttpHandlerFactory::RegisterTemplate - is registered path: " << path;
            }
        }
    };
    
    // 通过headers.getPath()分发
    static void DispatchRabbitHttpHandler(const proxygen::HTTPMessage& headers,
                                       std::unique_ptr<folly::IOBuf> body,
                                       proxygen::ResponseBuilder& r);
    
protected:
    RabbitHttpHandlerFactory() {};
    
    static RabbitHttpHandlerFactory& GetInstance() {
        static RabbitHttpHandlerFactory g_rabbit_http_handler_factory;
        return g_rabbit_http_handler_factory;
    }
    
    folly::StringKeyedUnorderedMap<RabbitHttpExecuteFunc> factories_;
};

#define REGISTER_HTTP_HANDLER(K, PATH, HANDLER) \
    static RabbitHttpHandlerFactory::RegisterTemplate g_htt_reg_handler_##K(PATH, &HANDLER)


#endif /* NET_RABBIT_HTTP_HANDLER_H_ */
