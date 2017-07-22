#ifndef NET_RABBIT_HTTP_REQUEST_HANDLER_H_
#define NET_RABBIT_HTTP_REQUEST_HANDLER_H_

#include <folly/Memory.h>

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/ResponseBuilder.h>

class RabbitHttpRequestHandler : public proxygen::RequestHandler,
	public std::enable_shared_from_this<RabbitHttpRequestHandler> {
public:
    RabbitHttpRequestHandler() = default;
    virtual ~RabbitHttpRequestHandler() {
    	VLOG(4) << "~MoguHttpRequestHandler";
    }
    
    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override {
        request_ = std::move(headers);
    }
    
    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
        requestBody_.append(std::move(body));
    }
    
    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override {}
    
    void onEOM() noexcept override;
    
    virtual void OnHttpHandler(const proxygen::HTTPMessage& headers,
                               std::unique_ptr<folly::IOBuf> body,
                               proxygen::ResponseBuilder& r);
    
    void requestComplete() noexcept override {
    	VLOG(4) << "MoguHttpRequestHandler, requestComplete!";
    	self.reset();
    }
    
    void onError(proxygen::ProxygenError err) noexcept override {
    	LOG(ERROR) << "MoguHttpRequestHandler, onError : " << err;
    	self.reset();
    }
    
protected:
    std::unique_ptr<proxygen::HTTPMessage> request_;
    folly::IOBufQueue requestBody_;
    std::shared_ptr<RabbitHttpRequestHandler> self;
};

class RabbitHttpRequestHandlerFactory : public proxygen::RequestHandlerFactory {
public:
    RabbitHttpRequestHandlerFactory() {}
    
    void onServerStart(folly::EventBase* evb) noexcept override {
    }
    
    void onServerStop() noexcept override {
    }
    
    proxygen::RequestHandler* onRequest(proxygen::RequestHandler*, proxygen::HTTPMessage*) noexcept override {
        return new RabbitHttpRequestHandler();
    }
};


#endif

