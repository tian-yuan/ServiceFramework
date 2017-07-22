#ifndef NET_SERVER_HTTP_CLIENT_H_
#define NET_SERVER_HTTP_CLIENT_H_

#include <folly/io/async/EventBase.h>
#include <folly/io/IOBuf.h>
#include <proxygen/lib/http/HTTPMessage.h>

enum HttpReplyCode : int {
    OK = 0,             // 返回成功
    INVALID = -1,       // 输入参数错误（url解析出错，url主机名解析出错）
    CONNECT_ERROR = -2, // 网络连接错误
    HTTP_ERROR = -3,    // Http内部错误
    NONE = -4,       // 等待
};

// 异步http_client
// 注意：
//   如果body非常大，比如大文件下载，建议不要用此接口
struct HttpClientReplyData {
    HttpReplyCode result{HttpReplyCode::NONE}; // 返回值：为0成功，注意：为0时headers和body才会设置值
    std::unique_ptr<proxygen::HTTPMessage> headers;
    // 注意：IOBuf可能有多个组成
    std::unique_ptr<folly::IOBuf> body;
    
    void Move(HttpClientReplyData& other) {
        other.result = result;
        other.headers = std::move(headers);
        other.body = std::move(body);
    }
    
    std::string ToString() const;
};

typedef std::function<void(HttpClientReplyData&)> HttpClientReplyCallback;


void HttpClientGet(folly::EventBase* evb,
                   const std::string& url,
                   HttpClientReplyCallback cb);

void HttpClientGet(folly::EventBase* evb,
                   const std::string& url,
                   const std::string& headers,
                   HttpClientReplyCallback cb);

void HttpClientPost(folly::EventBase* evb,
                    const std::string& url,
                    const std::string& post_data,
                    HttpClientReplyCallback cb);

void HttpClientPost(folly::EventBase* evb,
                    const std::string& url,
                    const std::string& headers,
                    const std::string& post_data,
                    HttpClientReplyCallback cb);


// 使用协程，同步调用
std::shared_ptr<HttpClientReplyData> HttpClientSyncGet(folly::EventBase* evb,
                                                       const std::string& url);

std::shared_ptr<HttpClientReplyData> HttpClientSyncGet(folly::EventBase* evb,
                                                       const std::string& url,
                                                       const std::string& headers);

std::shared_ptr<HttpClientReplyData> HttpClientSyncPost(folly::EventBase* evb,
                                                        const std::string& url,
                                                        const std::string& post_data);

std::shared_ptr<HttpClientReplyData> HttpClientSyncPost(folly::EventBase* evb,
                                                        const std::string& url,
                                                        const std::string& headers,
                                                        const std::string& post_data);


#endif
