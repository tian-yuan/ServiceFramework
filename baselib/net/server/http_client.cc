#include "net/server/http_client.h"

#include <folly/io/async/EventBase.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/EventBaseManager.h>

#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPConnector.h>

#include "base/glog_util.h"

#include "message_util/call_chain_util.h"
#include "net/rabbit_http_client.h"

#include "net/fiber_data_manager.h"


// 默认为10s
#define HTTP_FETCH_TIMEOUT 2

using namespace folly;
using namespace proxygen;

#define kDefaultConnectTimeout 1000

namespace {

static __thread char g_resolve_buffer[64 * 1024];

static std::string S_GetHostByName(const std::string& hostname) {
    std::string ip;
    struct in_addr sin_addr;

    struct hostent hent;
    struct hostent* he = NULL;
    bzero(&hent, sizeof(hent));
    
#ifndef __MACH__
    int herrno = 0;
    int ret = gethostbyname_r(hostname.c_str(), &hent, g_resolve_buffer, sizeof(g_resolve_buffer), &he, &herrno);
#else
    he = gethostbyname(hostname.c_str());
    int ret = 0;
#endif
    if (ret == 0 && he != NULL && he->h_addr) {
        char host_ip[32] = {0};
        sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        inet_ntop(AF_INET, &sin_addr, host_ip, sizeof(host_ip));
        ip = host_ip;
    } else {
        LOG(ERROR) << "S_GetHostByName - gethostbyname error: " << hostname;
    }
    
    return ip;
}

}

class HttpClient : public RabbitHttpClientCallback {
public:
    static HttpClient* CreateHttpClient(folly::EventBase* evb, const HttpClientReplyCallback& cb) {
        return new HttpClient(evb, cb, 0);
    }
    
    static HttpClient* CreateHttpClient(folly::EventBase* evb,
                                        const HttpClientReplyCallback& cb,
                                        int64_t fiber_id) {
        return new HttpClient(evb, cb, fiber_id);
    }
    
    virtual ~HttpClient();
    
    void Fetch(proxygen::HTTPMethod method,
               const std::string& url,
               const std::string& post_data) {
        Fetch(method, url, "", post_data);
    }

    void Fetch(proxygen::HTTPMethod method,
               const std::string& url,
               const std::string& headers,
               const std::string& post_data);

    // 引用计数
    void AddRef() {
        ++ref_count_;
    }
    
    void ReleaseRef() {
        if (ref_count_ <= 0) {
            LOG(ERROR) << "HttpClient - ReleaseRef error, ref_count_: " << ref_count_;
        } else {
            --ref_count_;
            if (ref_count_ == 0) {
                delete this;
            }
        }
    }

protected:
    HttpClient(folly::EventBase* evb, const HttpClientReplyCallback& cb, int64_t fiber_id);

    // Impl from MoguHttpClientCallback
    void OnHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) override;
    void OnBodyComplete(std::unique_ptr<folly::IOBuf> body) override;
    void OnConnectError(const folly::AsyncSocketException& ex) override;
    void OnError(const proxygen::HTTPException& error) override;

    folly::EventBase* evb_{nullptr};

    HttpClientReplyCallback cb_;
    HttpClientReplyData reply_;

    std::unique_ptr<proxygen::HTTPConnector> connector_;
    std::unique_ptr<RabbitHttpClient> http_client_;
    folly::HHWheelTimer::UniquePtr timer_;

    int ref_count_{0};
    
    int64_t fiber_id_{0};
};


HttpClient::HttpClient(folly::EventBase* evb, const HttpClientReplyCallback& cb, int64_t fiber_id) :
    evb_(evb),
    cb_(cb),
    fiber_id_(fiber_id) {
        
    if (evb_ == nullptr) {
        evb_ = folly::EventBaseManager::get()->getEventBase();
    }
}

HttpClient::~HttpClient() {
    // LOG(INFO) << "HttpClient::~HttpClient()";
}

void HttpClient::Fetch(proxygen::HTTPMethod method,
                       const std::string& url,
                       const std::string& headers,
                       const std::string& post_data) {
    URL url2(url);
    if (!url2.isValid()) {
        reply_.result = HttpReplyCode::INVALID;
        if (cb_) {
            cb_(reply_);
        }
        return;
    }

    std::string str = S_GetHostByName(url2.getHost());
    if (str.empty()) {
        reply_.result = HttpReplyCode::INVALID;
        if (cb_) {
            cb_(reply_);
        }
        return;
    }

    AddRef();
    
    SocketAddress addr(str, url2.getPort(), false);
    
    proxygen::HTTPHeaders headers2;
    if (!headers.empty()) {
        std::vector<folly::StringPiece> header_list;
        folly::split(";", headers, header_list);
        for (const auto& header_pair: header_list) {
            std::vector<folly::StringPiece> nv;
            folly::split(':', header_pair, nv);
            if (nv.size() > 0) {
                if (nv[0].empty()) {
                    continue;
                }
                StringPiece value("");
                if (nv.size() > 1) {
                    value = nv[1];
                } // trim anything else
                headers2.add(nv[0], value);
            }
        }

    }

    std::unique_ptr<folly::IOBuf> post_data2;
    if (method == proxygen::HTTPMethod::POST && !post_data.empty()) {
        post_data2 = std::move(folly::IOBuf::copyBuffer(post_data));
    }

    http_client_ = folly::make_unique<RabbitHttpClient>(evb_,
                                                      method,
                                                      url2,
                                                      headers2,
                                                      std::move(post_data2),
                                                      this);
    
    // Note: HHWheelTimer is a large object and should be created at most
    // once per thread
    timer_.reset(new HHWheelTimer(
                                  evb_,
                                  std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
                                  AsyncTimeout::InternalEnum::NORMAL,
                                  std::chrono::milliseconds(5000)));
    
    connector_ = folly::make_unique<HTTPConnector>(http_client_.get(), timer_.get());
    
    static const AsyncSocket::OptionMap opts{{{SOL_SOCKET, SO_REUSEADDR}, 1}};
    
    connector_->connect(evb_, addr,
                        std::chrono::milliseconds(kDefaultConnectTimeout), opts);
}

void HttpClient::OnHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) {
    reply_.headers = std::move(msg);
}

void HttpClient::OnBodyComplete(std::unique_ptr<folly::IOBuf> body) {
    if (reply_.result == HttpReplyCode::NONE) {
        reply_.result = HttpReplyCode::OK;
        reply_.body = std::move(body);
    }

    if (cb_) {
        cb_(reply_);
    }
    
    ReleaseRef();
}

void HttpClient::OnConnectError(const folly::AsyncSocketException& ex) {
    reply_.result = HttpReplyCode::CONNECT_ERROR;
    
    if (cb_) {
        cb_(reply_);
    }
    
    ReleaseRef();
}

void HttpClient::OnError(const proxygen::HTTPException& error) {
    LOG(ERROR) << "http client on error, " << error.what();
    reply_.result = HttpReplyCode::HTTP_ERROR;

//    if (cb_) {
//        cb_(reply_);
//    }
//
//    ReleaseRef();
}

void HttpClientGet(folly::EventBase* evb,
                   const std::string& url,
                   HttpClientReplyCallback cb) {
    auto http_client = HttpClient::CreateHttpClient(evb, cb);
    http_client->AddRef();
    http_client->Fetch(proxygen::HTTPMethod::GET, url, "");
    http_client->ReleaseRef();
}

void HttpClientGet(folly::EventBase* evb,
                   const std::string& url,
                   const std::string& headers,
                   HttpClientReplyCallback cb) {
    auto http_client = HttpClient::CreateHttpClient(evb, cb);
    http_client->AddRef();
    http_client->Fetch(proxygen::HTTPMethod::GET, url, headers);
    http_client->ReleaseRef();
}

void HttpClientPost(folly::EventBase* evb,
                    const std::string& url,
                    const std::string& post_data,
                    HttpClientReplyCallback cb) {
    auto http_client = HttpClient::CreateHttpClient(evb, cb);
    http_client->AddRef();
    http_client->Fetch(proxygen::HTTPMethod::POST, url, "", post_data);
    http_client->ReleaseRef();
}

void HttpClientPost(folly::EventBase* evb,
                    const std::string& url,
                    const std::string& headers,
                    const std::string& post_data,
                    HttpClientReplyCallback cb) {
    auto http_client = HttpClient::CreateHttpClient(evb, cb);
    http_client->AddRef();
    http_client->Fetch(proxygen::HTTPMethod::POST, url, headers, post_data);
    http_client->ReleaseRef();
}

std::shared_ptr<HttpClientReplyData> HttpClientSyncFetch(folly::EventBase* evb,
                                                         FiberDataManager* fiber_data_manger,
                                                         proxygen::HTTPMethod method,
                                                         const std::string& url,
                                                         const std::string& headers,
                                                         const std::string& post_data) {
    auto reply_data = std::make_shared<HttpClientReplyData>();
    if (!fiber_data_manger) {
        LOG(ERROR) << "HttpClientSyncFetch - fiber_data_manger is null, by fetch: " << url;
        return reply_data;
    }
    
    auto fiber_data = fiber_data_manger->AllocFiberData();
    if (fiber_data) {
        // 有可能已经超时，需要使用weak_ptr来检查是否已经被释放
        std::weak_ptr<HttpClientReplyData> r1(reply_data);
        std::weak_ptr<FiberDataInfo> f1(fiber_data);
        auto http_client = HttpClient::CreateHttpClient(evb, [r1, f1, url] (HttpClientReplyData& reply) {
            auto r2 = r1.lock();
            auto f2 = f1.lock();
            if (r2 && f2) {
                reply.Move(*r2);
                if (reply.result != HttpReplyCode::INVALID) {
                    f2->Post(true);
                }
            }
        });
        
        CallChainData chain_back = GetCallChainDataByThreadLocal();
        TRACE_FIBER() << "HttpClientSyncFetch - Ready fetch url: " << url
                    << ", call_chain:" << chain_back.ToString()
                    <<  ", " << fiber_data->ToString();

        http_client->AddRef();
        FiberDataManager::IncreaseStats(FiberDataManager::f_http);
        http_client->Fetch(method, url, headers, post_data);
        if (reply_data->result != HttpReplyCode::INVALID) {
            
            fiber_data->Wait(FIBER_HTTP_TIMEOUT);
            if (fiber_data->state == FiberDataInfo::kTimeout) {
                // 添加调用链追踪
                LOG(ERROR) << "HttpClientSyncFetch - fiber wait timeout:  "
                            <<  fiber_data->ToString() << ", url: " << url;
            }
        }
        fiber_data_manger->DeleteFiberData(fiber_data);
        http_client->ReleaseRef();
        
        TRACE_FIBER() << "HttpClientSyncFetch - end fetch url: " << url
                            << ", call_chain:" << chain_back.ToString()
                            <<  "," << fiber_data->ToString();

        GetCallChainDataByThreadLocal() = chain_back;

    } else {
        LOG(ERROR) << "HttpClientSyncFetch - fiber_data is null by fetch: " << url;
    }
    
    return reply_data;
}

std::shared_ptr<HttpClientReplyData> HttpClientSyncFetch(folly::EventBase* evb,
                                                         proxygen::HTTPMethod method,
                                                         const std::string& url,
                                                         const std::string& headers,
                                                         const std::string& post_data) {
    auto reply_data = std::make_shared<HttpClientReplyData>();
    // reply_data->result = HttpReplyCode::INVALID;
    
    // FiberDataManager* fiber_data_manger = Root::GetServiceRouterManager()->FindFiberData();
    FiberDataManager& fiber_data_manger = GetFiberDataManagerByThreadLocal();
    if (!fiber_data_manger.IsInited()) {
        LOG(ERROR) << "HttpClientSyncFetch - fiber_data_manger is null by fetch: " << url;
        return reply_data;
    }
    
    return HttpClientSyncFetch(evb, &fiber_data_manger, method, url, headers, post_data);
}

std::shared_ptr<HttpClientReplyData> HttpClientSyncGet(folly::EventBase* evb,
                                                       const std::string& url) {
    return HttpClientSyncFetch(evb, proxygen::HTTPMethod::GET, url, "", "");
}

std::shared_ptr<HttpClientReplyData> HttpClientSyncGet(folly::EventBase* evb,
                                                       const std::string& url,
                                                       const std::string& headers) {
    return HttpClientSyncFetch(evb, proxygen::HTTPMethod::GET, url, headers, "");
}

std::shared_ptr<HttpClientReplyData> HttpClientSyncPost(folly::EventBase* evb,
                                                        const std::string& url,
                                                        const std::string& post_data) {
    return HttpClientSyncFetch(evb, proxygen::HTTPMethod::POST, url, "", post_data);
}

std::shared_ptr<HttpClientReplyData> HttpClientSyncPost(folly::EventBase* evb,
                                                        const std::string& url,
                                                        const std::string& headers,
                                                        const std::string& post_data) {
    return HttpClientSyncFetch(evb, proxygen::HTTPMethod::POST, url, headers, post_data);
}


