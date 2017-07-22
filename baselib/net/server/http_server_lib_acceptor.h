#ifndef NET_SERVER_HTTP_SERVER_LIB_ACCEPTOR_H_
#define NET_SERVER_HTTP_SERVER_LIB_ACCEPTOR_H_


// #include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/lib/http/session/HTTPSessionAcceptor.h>

#include "net/server/http_server_lib.h"

namespace proxygen {

class HTTPServerLibAcceptor final : public HTTPSessionAcceptor {
 public:
  static AcceptorConfiguration makeConfig(
    const HTTPServerLib::IPConfig& ipConfig,
    const HTTPServerOptions& opts);

  static std::unique_ptr<HTTPServerLibAcceptor> make(
    const AcceptorConfiguration& conf,
    const HTTPServerOptions& opts,
    const std::shared_ptr<HTTPCodecFactory>& codecFactory = nullptr);

  /**
   * Invokes the given method when all the connections are drained
   */
  void setCompletionCallback(std::function<void()> f);

  ~HTTPServerLibAcceptor() override;

 private:
  HTTPServerLibAcceptor(const AcceptorConfiguration& conf,
                     const std::shared_ptr<HTTPCodecFactory>& codecFactory,
                     std::vector<RequestHandlerFactory*> handlerFactories);

  // HTTPSessionAcceptor
  HTTPTransaction::Handler* newHandler(HTTPTransaction& txn,
                                       HTTPMessage* msg) noexcept override;
  void onConnectionsDrained() override;

  std::function<void()> completionCallback_;
  const std::vector<RequestHandlerFactory*> handlerFactories_{nullptr};
};

}

#endif


