#include "net/rabbit_http_request_handler.h"
#include "net/fiber_data_manager.h"

#include <proxygen/httpserver/ResponseBuilder.h>

#define USE_FIBER

#ifdef USE_FIBER

void RabbitHttpRequestHandler::onEOM() noexcept {
	VLOG(4)<< "MoguHttpRequestHandler::onEOM!";
    FiberDataManager& fiber_data_manager = GetFiberDataManagerByThreadLocal();
    if (fiber_data_manager.IsInited()) {
        FiberDataManager::IncreaseStats(FiberDataManager::f_add_task);
        std::shared_ptr<RabbitHttpRequestHandler> capture_ptr(this);
        self = shared_from_this();
        std::weak_ptr<RabbitHttpRequestHandler> weak_capture_ptr = capture_ptr;
        fiber_data_manager.GetFiberManager()->addTask([this, weak_capture_ptr]() {
        	VLOG(4)<< "MoguHttpRequestHandler::onEOM, do task!!";

            try {
                proxygen::ResponseBuilder r(downstream_);
                r.status(200, "OK");
                OnHttpHandler(*request_, requestBody_.move(), r);
                ///< 由于 OnHttpHandler 里面有可能间接调用 FiberDataManager::Wait() 函数，当前函数可能被切换出去
                ///< 可能切换到 MoguHttpRequestHandler::onError 函数，所以需要在 OnHttpHandler 后面判断
                ///< weak_capture_ptr 是否为空
                auto spt = weak_capture_ptr.lock();
				if (!spt) {
					LOG(ERROR) << "MoguHttpRequestHandler::onEOM, addTask, handler destructed!";
					return;
				}
                r.sendWithEOM();
            } catch (const std::exception& ex) {
				auto spt = weak_capture_ptr.lock();
				if (!spt) {
					LOG(ERROR) << "MoguHttpRequestHandler::onEOM, addTask, handler destructed!";
					return;
				}
                proxygen::ResponseBuilder(downstream_)
                    .status(500, "Internal Server Error")
                    .body(ex.what())
                    .sendWithEOM();
            } catch (...) {
            	auto spt = weak_capture_ptr.lock();
				if (!spt) {
					LOG(ERROR) << "MoguHttpRequestHandler::onEOM, addTask, handler destructed!";
					return;
				}
                proxygen::ResponseBuilder(downstream_)
                .status(500, "Internal Server Error")
                .body("Unknown exception thrown")
                .sendWithEOM();
            }
        });
    }
}

#else

void RabbitHttpRequestHandler::onEOM() noexcept {
    try {
        proxygen::ResponseBuilder r(downstream_);
        r.status(200, "OK");
        OnHttpHandler(*request_, requestBody_.move(), r);
        r.sendWithEOM();
    } catch (const std::exception& ex) {
        proxygen::ResponseBuilder(downstream_)
        .status(500, "Internal Server Error")
        .body(ex.what())
        .sendWithEOM();
    } catch (...) {
        proxygen::ResponseBuilder(downstream_)
        .status(500, "Internal Server Error")
        .body("Unknown exception thrown")
        .sendWithEOM();
    }
}

#endif

void RabbitHttpRequestHandler::OnHttpHandler(const proxygen::HTTPMessage& headers,
                                           std::unique_ptr<folly::IOBuf> body,
                                           proxygen::ResponseBuilder& r) {
//    RabbitHttpRequestHandler::DispatchMoguHttpHandler(headers, std::move(body), r);

}
