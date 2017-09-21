/*
 *  Copyright (c) 2016, mogujie.com
 *  All rights reserved.
 *
 *  Created by benqi@mogujie.com on 2016-02-05.
 *
 *  基于：https://github.com/facebook/proxygen/blob/master/proxygen/httpclient/samples/curl/CurlClient.h(cpp)
 *  将CurlClient集成至我们的网络库里
 */

#include "net/rabbit_http_client.h"

#include <fstream>

#include <folly/FileUtil.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>

using namespace folly;
using namespace proxygen;
using namespace std;

RabbitHttpClient::RabbitHttpClient(EventBase* evb,
                               HTTPMethod httpMethod,
                               const URL& url,
                               const HTTPHeaders& headers,
                               std::unique_ptr<folly::IOBuf> postData,
                               RabbitHttpClientCallback* cb) :
    evb_(evb), httpMethod_(httpMethod), url_(url),
    postData_(std::move(postData)),
    cb_(cb) {
        
  headers.forEach([this] (const string& header, const string& val) {
      request_.getHeaders().add(header, val);
    });
}

RabbitHttpClient::~RabbitHttpClient() {
}

void RabbitHttpClient::connectSuccess(HTTPUpstreamSession* session) {
  txn_ = session->newTransaction(this);
  request_.setMethod(httpMethod_);
  request_.setHTTPVersion(1, 1);
  request_.setURL(url_.makeRelativeURL());
  request_.setSecure(url_.isSecure());

  if (!request_.getHeaders().getNumberOfValues(HTTP_HEADER_USER_AGENT)) {
    request_.getHeaders().add(HTTP_HEADER_USER_AGENT, "proxygen_curl");
  }
  if (!request_.getHeaders().getNumberOfValues(HTTP_HEADER_HOST)) {
    request_.getHeaders().add(HTTP_HEADER_HOST, url_.getHostAndPort());
  }
  if (!request_.getHeaders().getNumberOfValues(HTTP_HEADER_ACCEPT)) {
    request_.getHeaders().add("Accept", "*/*");
  }
  request_.dumpMessage(4);

  txn_->sendHeaders(request_);

  unique_ptr<IOBuf> buf;
  if (httpMethod_ == HTTPMethod::POST) {
    // TODO, 是否需要流控
    txn_->sendBody(move(postData_));
  }

  txn_->sendEOM();
  session->closeWhenIdle();
}

void RabbitHttpClient::connectError(const folly::AsyncSocketException& ex) {
  LOG(ERROR) << "Coudln't connect to " << url_.getHostAndPort() << ":" <<
    ex.what();
    
    if (cb_) cb_->OnConnectError(ex);
}

void RabbitHttpClient::setTransaction(HTTPTransaction*) noexcept {
    // LOG(INFO) << "setTransaction()";
}

void RabbitHttpClient::detachTransaction() noexcept {
    // LOG(INFO) << "detachTransaction()";
    
    // TODO(@benqi): detachTransaction还是onEOM里回调？？？
    if (cb_) cb_->OnBodyComplete(std::move(body_));
}

void RabbitHttpClient::onHeadersComplete(unique_ptr<HTTPMessage> msg) noexcept {
//    msg->getHeaders().forEach([&] (const string& header, const string& val) {
//        cout << header << ": " << val << endl;
//    });
    if (cb_) cb_->OnHeadersComplete(std::move(msg));
}

void RabbitHttpClient::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
    
    if (!body_) {
        // LOG(INFO) << "onBody1";
        body_ = std::move(chain);
    } else {
        // LOG(INFO) << "onBody2";
        body_->prependChain(std::move(chain));
    }

}

void RabbitHttpClient::onTrailers(std::unique_ptr<HTTPHeaders>) noexcept {
    // LOG(INFO) << "Discarding trailers";
}

void RabbitHttpClient::onEOM() noexcept {
    // LOG(INFO) << "Got EOM";
}

void RabbitHttpClient::onUpgrade(UpgradeProtocol) noexcept {
    // LOG(INFO) << "Discarding upgrade protocol";
}

void RabbitHttpClient::onError(const HTTPException& error) noexcept {
    LOG(ERROR) << "An error occurred: " << error.what();
    if (cb_) cb_->OnError(error);
}

void RabbitHttpClient::onEgressPaused() noexcept {
    // LOG(INFO) << "Egress paused";
}

void RabbitHttpClient::onEgressResumed() noexcept {
    // LOG(INFO) << "Egress resumed";
}

const string& RabbitHttpClient::getServerName() const {
    const string& res = request_.getHeaders().getSingleOrEmpty(HTTP_HEADER_HOST);
    if (res.empty()) {
        return url_.getHost();
    }
    return res;
}

