#include "net/conn_handler.h"

#include "net/thread_local_conn_manager.h"

///////////////////////////////////////////////////////////////////////////////////////////
void ConnHandler::read(Context* ctx, std::unique_ptr<folly::IOBuf> msg) {
    if (nullptr == msg.get() || msg->length() <= 0) {
        LOG(INFO) << "IMConnHandler::read, get one empty iobuf!";
        close(ctx);
        return;
    }
    // decode message
}

folly::Future<folly::Unit> ConnHandler::write(Context* ctx, std::unique_ptr<folly::IOBuf> out) {
    return ctx->fireWrite(std::forward<std::unique_ptr<folly::IOBuf>>(out));
}

void ConnHandler::readEOF(Context* ctx) {
    LOG(INFO) << "readEOF - conn_id = " << conn_id_ << ", ConnHandler - Connection closed by "
              << remote_address_
              << ", conn_info: " << service_->GetServiceConfig().ToString();
    close(ctx);
}

void ConnHandler::readException(Context* ctx, folly::exception_wrapper e) {
    LOG(ERROR) << "readException - conn_id = " << conn_id_
               << ", IMConnHandler - Local error: " << exceptionStr(e)
               << ", by "  << remote_address_
               << ", conn_info: " << service_->GetServiceConfig().ToString();
    close(ctx);
}

void ConnHandler::transportActive(Context* ctx) {
    conn_state_ = kConnected;

    // 缓存连接地址
    remote_address_ = ctx->getPipeline()->getTransportInfo()->remoteAddr->getAddressStr();
    if (service_) {
        conn_id_ = service_->OnNewConnection(reinterpret_cast<ConnPipeline*>(ctx->getPipeline()));
    }

    LOG(INFO) << "transportActive - conn_id = " << conn_id_
              << ", IMConnHandler - Connection connected by "
              << remote_address_
              << ", conn_info: " << service_->GetServiceConfig().ToString();

//    impdu::MessageHandlerFactory::DispatchConnEventHandler(service_,
//                                                           reinterpret_cast<IMConnPipeline*>(ctx->getPipeline()),
//                                                           conn_id_, true);
}

folly::Future<folly::Unit> ConnHandler::close(Context* ctx) {
    if (conn_state_ == kConnected) {
        // 有可能close了多次，但是只能回调一次
//        impdu::MessageHandlerFactory::DispatchConnEventHandler(service_,
//                                                               reinterpret_cast<IMConnPipeline*>(ctx->getPipeline()),
//                                                               conn_id_,
//                                                               false);
        if (service_) {
            service_->OnConnectionClosed(conn_id_, reinterpret_cast<ConnPipeline*>(ctx->getPipeline()));
        }

        conn_state_ = kClosed;
        remote_address_.clear();
    }

    return ctx->fireClose();
}

