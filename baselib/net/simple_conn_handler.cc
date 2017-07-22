#include "net/simple_conn_handler.h"

#include <wangle/channel/EventBaseHandler.h>

///////////////////////////////////////////////////////////////////////////////////////////
SimpleConnHandler::SimpleConnHandler(SocketEventCallback* callback, const std::string& name) :
    callback_(callback),
    name_(name) {
//    LOG(INFO) << "SimpleConnHandler::SimpleConnHandler(), Allocate: " << name;
}

SimpleConnHandler::~SimpleConnHandler() {
//    LOG(INFO) << "SimpleConnHandler::SimpleConnHandler(), Destroy: " << name_;
}

void SimpleConnHandler::read(Context* ctx, std::unique_ptr<folly::IOBuf> msg) {
    //LOG(INFO) << "SimpleConnHandler - read data: len = " << msg->length() << ", name = " << name_;

    int rv = 0;
    if (callback_) {
        rv = callback_->OnDataArrived(reinterpret_cast<SimpleConnPipeline*>(ctx->getPipeline()), std::move(msg));
    }
    if (rv == -1) {
        LOG(ERROR) << "SimpleConnHandler - read: callback_->OnDataArrived error, recv len " << msg->length()
            << ", by peer " << ctx->getPipeline()->getTransportInfo()->remoteAddr->getAddressStr();
        // 直接关闭
        close(ctx);
    }
    
    //LOG(INFO) << "SimpleConnHandler - read data end!";
}

folly::Future<folly::Unit> SimpleConnHandler::write(Context* ctx, std::unique_ptr<IOBuf> out) {
    // LOG(INFO) << "conn_id = " << conn_id_ << ", IMConnHandler - write length: " << out->length();
    return ctx->fireWrite(std::forward<std::unique_ptr<IOBuf>>(out));
}

void SimpleConnHandler::readEOF(Context* ctx) {
    LOG(INFO) << "SimpleConnHandler - Connection closed by "
                << ctx->getPipeline()->getTransportInfo()->remoteAddr->getAddressStr()
                << ", name = " << name_;
    
    close(ctx);
}

void SimpleConnHandler::readException(Context* ctx, exception_wrapper e) {
    LOG(ERROR) << "SimpleConnHandler - Local error: " << exceptionStr(e)
                << ", by "  << ctx->getPipeline()->getTransportInfo()->remoteAddr->getAddressStr();
    
    close(ctx);
}

void SimpleConnHandler::transportActive(Context* ctx) {
//    LOG(INFO) << "SimpleConnHandler - Connection connected by " << ctx->getPipeline()->getTransportInfo()->remoteAddr->getAddressStr()
//                << ", name = " << name_;

    if (callback_) {
        callback_->OnNewConnection(reinterpret_cast<SimpleConnPipeline*>(ctx->getPipeline()));
    }
}

folly::Future<folly::Unit> SimpleConnHandler::close(Context* ctx) {
    if (callback_) {
        callback_->OnConnectionClosed(reinterpret_cast<SimpleConnPipeline*>(ctx->getPipeline()));
    }

    return ctx->fireClose();
}

