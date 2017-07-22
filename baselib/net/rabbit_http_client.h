/*
 *  Copyright (c) 2016, mogujie.com
 *  All rights reserved.
 *
 *  Created by benqi@mogujie.com on 2016-02-05.
 *
 *  基于：https://github.com/facebook/proxygen/blob/master/proxygen/httpclient/samples/curl/CurlClient.h(cpp)
 *  将CurlClient集成至我们的网络库里
 */

#ifndef NET_SERVER_MOGU_HTTP_CLIENT_H_
#define NET_SERVER_MOGU_HTTP_CLIENT_H_

#include <folly/io/async/EventBase.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/utils/URL.h>


class RabbitHttpClientCallback {
public:
    virtual ~RabbitHttpClientCallback() {}
    
    virtual void OnHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) = 0;
    virtual void OnBodyComplete(std::unique_ptr<folly::IOBuf> body) = 0;
    virtual void OnConnectError(const folly::AsyncSocketException& ex) = 0;
    virtual void OnError(const proxygen::HTTPException& error) = 0;
};

class RabbitHttpClient : public proxygen::HTTPConnector::Callback,
                       public proxygen::HTTPTransactionHandler {
 public:

    RabbitHttpClient(folly::EventBase* evb, proxygen::HTTPMethod httpMethod,
             const proxygen::URL& url, const proxygen::HTTPHeaders& headers,
             std::unique_ptr<folly::IOBuf> postData, RabbitHttpClientCallback* cb);
  ~RabbitHttpClient() override;

  // initial SSL related structures
  // void initializeSsl(const std::string& certPath,
  //                   const std::string& nextProtos);
  // void sslHandshakeFollowup(proxygen::HTTPUpstreamSession* session) noexcept;

  // HTTPConnector methods
  void connectSuccess(proxygen::HTTPUpstreamSession* session) override;
  void connectError(const folly::AsyncSocketException& ex) override;

  // HTTPTransactionHandler methods
  void setTransaction(proxygen::HTTPTransaction* txn) noexcept override;
  void detachTransaction() noexcept override;
  void onHeadersComplete(
      std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override;
  void onTrailers(
      std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
  void onError(const proxygen::HTTPException& error) noexcept override;
  void onEgressPaused() noexcept override;
  void onEgressResumed() noexcept override;

  // Getters
  // folly::SSLContextPtr getSSLContext() { return sslContext_; }

  const std::string& getServerName() const;

protected:
  proxygen::HTTPTransaction* txn_{nullptr};
  folly::EventBase* evb_{nullptr};
  proxygen::HTTPMethod httpMethod_;
  proxygen::URL url_;
  proxygen::HTTPMessage request_;
  std::unique_ptr<folly::IOBuf> postData_;

  RabbitHttpClientCallback* cb_;
  // folly::SSLContextPtr sslContext_;
                           
  std::unique_ptr<folly::IOBuf> body_;
};

#endif

