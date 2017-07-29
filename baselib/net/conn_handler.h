#ifndef BASE_NET_CONN_HANDLER_H_
#define BASE_NET_CONN_HANDLER_H_

#include <string>
#include <wangle/channel/AsyncSocketHandler.h>

#include "net/server/tcp_service_base.h"

class TcpServiceBase;
using ConnPipeline = wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;

// 使用AttachData存储连接附加数据
// 比如用户名等
struct ConnAttachData {
    virtual ~ConnAttachData() = default;
};

typedef std::shared_ptr<ConnAttachData> ConnAttachDataPtr;

class ConnHandler : public wangle::HandlerAdapter<std::unique_ptr<folly::IOBuf>> {
public:
    enum ConnState {
        kNone = 0,
        kConnected,
        kConnectedButDisable,
        kClosed,
        kError
    };

    explicit ConnHandler(ServiceBase* service)
            : service_(dynamic_cast<TcpServiceBase*>(service)),
              conn_id_(0),
              conn_state_(kNone) {
    }

    //////////////////////////////////////////////////////////////////////////
    inline TcpServiceBase* GetServiceBase() {
        return service_;
    }

    inline ConnAttachData* GetAttachData() {
        return attach_data_.get();
    }

    //////////////////////////////////////////////////////////////////////////
    template <class T>
    inline T* CastByGetAttachData() {
        return nullptr;
    }

    template <class T>
    inline typename std::enable_if<std::is_base_of<ConnAttachData, T>::value>::type*
    CastByGetAttachData() {
        return dynamic_cast<T*>(attach_data_.get());
    }

    inline void SetAttachData(const std::shared_ptr<ConnAttachData>& v) {
        attach_data_ = v;
    }

    inline uint64_t GetConnID() const {
        return conn_id_;
    }

    //////////////////////////////////////////////////////////////////////////
    // 重载 HandlerAdapter<std::unique_ptr<IOBuf>>
    void read(Context* ctx, std::unique_ptr<folly::IOBuf> msg) override;
    folly::Future<folly::Unit> write(Context* ctx, std::unique_ptr<folly::IOBuf> out) override;

    void readEOF(Context* ctx) override;
    void readException(Context* ctx, folly::exception_wrapper e) override;

    void transportActive(Context* ctx) override;
    folly::Future<folly::Unit> close(Context* ctx) override;

    inline const std::string& GetRemoteAddress() const {
        return remote_address_;
    }

protected:
    // 全局的
    TcpServiceBase* service_{nullptr};
    uint64_t conn_id_;
    std::shared_ptr<ConnAttachData> attach_data_;
    int conn_state_;

    std::string remote_address_;
};

#endif
