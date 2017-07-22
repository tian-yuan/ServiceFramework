#include "net/server/http_server_lib.h"

#include <folly/ThreadName.h>
#include <folly/io/async/EventBaseManager.h>

#include "net/server/http_server_lib_acceptor.h"

// #include <proxygen/httpserver/SignalHandler.h>
#include <proxygen/httpserver/filters/RejectConnectFilter.h>
#include <proxygen/httpserver/filters/ZlibServerFilter.h>

using folly::AsyncServerSocket;
using folly::EventBase;
using folly::EventBaseManager;
using folly::SocketAddress;
using wangle::IOThreadPoolExecutor;
using wangle::ThreadPoolExecutor;

namespace proxygen {

class LibAcceptorFactory : public wangle::AcceptorFactory {
 public:
  LibAcceptorFactory(std::shared_ptr<HTTPServerOptions> options,
                  std::shared_ptr<HTTPCodecFactory> codecFactory,
                  AcceptorConfiguration config,
                  HTTPSession::InfoCallback* sessionInfoCb) :
      options_(options),
      codecFactory_(codecFactory),
      config_(config),
      sessionInfoCb_(sessionInfoCb) {}
  std::shared_ptr<wangle::Acceptor> newAcceptor(
      folly::EventBase* eventBase) override {
    auto acc = std::shared_ptr<HTTPServerLibAcceptor>(
      HTTPServerLibAcceptor::make(config_, *options_, codecFactory_).release());
    if (sessionInfoCb_) {
      acc->setSessionInfoCallback(sessionInfoCb_);
    }
    acc->init(nullptr, eventBase);
    return acc;
  }

 private:
  std::shared_ptr<HTTPServerOptions> options_;
  std::shared_ptr<HTTPCodecFactory> codecFactory_;
  AcceptorConfiguration config_;
  HTTPSession::InfoCallback* sessionInfoCb_;
};

HTTPServerLib::HTTPServerLib(HTTPServerOptions options):
    options_(std::make_shared<HTTPServerOptions>(std::move(options))) {

  // Insert a filter to fail all the CONNECT request, if required
  if (!options_->supportsConnect) {
    options_->handlerFactories.insert(
        options_->handlerFactories.begin(),
        folly::make_unique<RejectConnectFilterFactory>());
  }

  // Add Content Compression filter (gzip), if needed. Should be
  // final filter
  if (options_->enableContentCompression) {
    options_->handlerFactories.insert(
        options_->handlerFactories.begin(),
        folly::make_unique<ZlibServerFilterFactory>(
          options_->contentCompressionLevel,
          options_->contentCompressionMinimumSize,
          options_->contentCompressionTypes));
  }
}

HTTPServerLib::~HTTPServerLib() {
  // CHECK(!mainEventBase_) << "Forgot to stop() server?";
}

void HTTPServerLib::bind(std::vector<IPConfig>& addrs) {
  addresses_ = addrs;
}

class LibHandlerCallbacks : public ThreadPoolExecutor::Observer {
 public:
  explicit LibHandlerCallbacks(std::shared_ptr<HTTPServerOptions> options) : options_(options) {}

  void threadStarted(ThreadPoolExecutor::ThreadHandle* h) override {
    auto evb = IOThreadPoolExecutor::getEventBase(h);
    evb->runInEventBaseThread([=](){
      for (auto& factory: options_->handlerFactories) {
        factory->onServerStart(evb);
      }
    });
  }
  void threadStopped(ThreadPoolExecutor::ThreadHandle* h) override {
    IOThreadPoolExecutor::getEventBase(h)->runInEventBaseThread([&](){
      for (auto& factory: options_->handlerFactories) {
        factory->onServerStop();
      }
    });
  }

 private:
  std::shared_ptr<HTTPServerOptions> options_;
};


void HTTPServerLib::start(std::shared_ptr<IOThreadPoolExecutor> io_group, std::function<void()> onSuccess,
                       std::function<void(std::exception_ptr)> onError) {
  // mainEventBase_ = EventBaseManager::get()->getEventBase();

  auto accExe = std::make_shared<IOThreadPoolExecutor>(1);
  // auto exe = std::make_shared<IOThreadPoolExecutor>(options_->threads);
  auto exeObserver = std::make_shared<LibHandlerCallbacks>(options_);
  // Observer has to be set before bind(), so onServerStart() callbacks run
  io_group->addObserver(exeObserver);

  try {
    FOR_EACH_RANGE (i, 0, addresses_.size()) {
      auto codecFactory = addresses_[i].codecFactory;
      auto factory = std::make_shared<LibAcceptorFactory>(
        options_,
        codecFactory,
        HTTPServerLibAcceptor::makeConfig(addresses_[i], *options_),
        sessionInfoCb_);
      bootstrap_.push_back(
          wangle::ServerBootstrap<wangle::DefaultPipeline>());
      bootstrap_[i].childHandler(factory);
      bootstrap_[i].group(accExe, io_group);
      bootstrap_[i].bind(addresses_[i].address);
    }
  } catch (const std::exception& ex) {
    stop();

    if (onError) {
      onError(std::current_exception());
      return;
    }

    throw;
  }

  // Install signal handler if required
  // if (!options_->shutdownOn.empty()) {
  //   signalHandler_ = folly::make_unique<SignalHandler>(this);
  //   signalHandler_->install(options_->shutdownOn);
  // }

  // Start the main event loop
  if (onSuccess) {
    onSuccess();
  }
  // mainEventBase_->loopForever();
}

void HTTPServerLib::stop() {
  // CHECK(mainEventBase_);

  for (auto& bootstrap : bootstrap_) {
    bootstrap.stop();
  }

  for (auto& bootstrap : bootstrap_) {
    bootstrap.join();
  }

  // signalHandler_.reset();
  // mainEventBase_->terminateLoopSoon();
  // mainEventBase_ = nullptr;
}

const std::vector<const folly::AsyncSocketBase*>
  HTTPServerLib::getSockets() const {

  std::vector<const folly::AsyncSocketBase*> sockets;
  FOR_EACH_RANGE(i, 0, bootstrap_.size()) {
    auto& bootstrapSockets = bootstrap_[i].getSockets();
    FOR_EACH_RANGE(j, 0, bootstrapSockets.size()) {
      sockets.push_back(bootstrapSockets[j].get());
    }
  }

  return sockets;
}

}
