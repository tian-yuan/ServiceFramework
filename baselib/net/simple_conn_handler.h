#ifndef BASE_NET_SIMPLE_CONN_HANDLER_H_
#define BASE_NET_SIMPLE_CONN_HANDLER_H_

#include <string>

#include <wangle/channel/AsyncSocketHandler.h>


using namespace folly;
using namespace wangle;

using SimpleConnPipeline = wangle::Pipeline<IOBufQueue&, std::unique_ptr<IOBuf>>;

class SocketEventCallback {
public:
    virtual ~SocketEventCallback() = default;
    
    virtual void OnNewConnection(SimpleConnPipeline* pipeline) = 0;
    virtual void OnConnectionClosed(SimpleConnPipeline* pipeline) = 0;
    virtual void OnConnectionError() {}
    virtual int  OnDataArrived(SimpleConnPipeline* pipeline, std::unique_ptr<IOBuf> buf) = 0;
};

class SimpleConnHandler : public HandlerAdapter<std::unique_ptr<IOBuf>> {
public:
    SimpleConnHandler(SocketEventCallback* callback, const std::string& name = "");
    ~SimpleConnHandler() override;
    
    // Impl from SimpleConnHandler
    void read(Context* ctx, std::unique_ptr<folly::IOBuf> msg) override;
    folly::Future<folly::Unit> write(Context* ctx, std::unique_ptr<IOBuf> out) override;
    void readEOF(Context* ctx) override;
    void readException(Context* ctx, exception_wrapper e) override;
    void transportActive(Context* ctx) override;
    folly::Future<folly::Unit> close(Context* ctx) override;

    const std::string& GetName() const {
        return name_;
    }
    
    void ClearCalllback() {
        callback_ = nullptr;
    }
private:
    SocketEventCallback* callback_{nullptr};
    std::string name_;
};

#endif
