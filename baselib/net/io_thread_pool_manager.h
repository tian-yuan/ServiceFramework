#ifndef NET_IO_THREAD_POOL_MANAGER_H_
#define NET_IO_THREAD_POOL_MANAGER_H_

#include <folly/FBVector.h>

#include <wangle/concurrent/IOThreadPoolExecutor.h>
#include <wangle/channel/Pipeline.h>

#include "net/fiber_data_manager.h"

using ConnPipeline = wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;

inline size_t GetIOThreadSize() {
    auto threads = std::thread::hardware_concurrency();
    if (threads <= 0) {
        // Reasonable mid-point for concurrency when actual value unknown
        threads = 8;
    }
    return threads;
}

///////////////////////////////////////////////////////////////////////////////////////
// 默认线程池
class DefaultIOThreadPoolExecutor : public wangle::IOThreadPoolExecutor {
public:
    explicit DefaultIOThreadPoolExecutor(size_t num_threads = GetIOThreadSize());
    virtual ~DefaultIOThreadPoolExecutor();
    
    size_t GetEventBaseSize() const {
        return num_threads_;
    }
    
    folly::EventBase* GetEventBase(size_t idx);
    
private:
    size_t num_threads_;
};

folly::EventBase* GetEventBaseByThreadID(size_t idx);
folly::EventBase* GetEventBaseByRand();


///////////////////////////////////////////////////////////////////////////////////////
// 单个线程的连接管理器
class IOThreadConnManager {
public:
    IOThreadConnManager(size_t thread_id, folly::EventBase* event_base) :
        thread_id_(thread_id),
        event_base_(event_base) {
    }
    
    // EventBase线程里执行
    uint32_t OnNewConnection(ConnPipeline* pipeline);
    
    // EventBase线程里执行
    bool OnConnectionClosed(uint32_t conn_id, ConnPipeline* pipeline);
    
    void OnServerRegister(ConnPipeline* pipeline, const std::string& server_name, uint32_t server_number);

    // 需要找到网络线程
    bool DispatchIOBufByConnID(uint32_t conn_id, std::unique_ptr<folly::IOBuf> data);

    bool DispatchIOBufByServer(const std::string& server_name, uint32_t server_number, std::shared_ptr<folly::IOBuf> data);

    bool BroadCast(std::shared_ptr<folly::IOBuf> data);

    folly::EventBase* GetEventBase() {
        return event_base_;
    }

    ConnPipeline* FindPipeline(uint32_t conn_id);
    const std::set<uint32_t>& FindPipelines(const std::string& service) const;
    
    
protected:
    size_t thread_id_;  // 线程ID
    folly::EventBase* event_base_ = nullptr;
    
    // 用于计数（可以使用IDMap）
    uint32_t current_conn_id_ = 0;

    // 需要考虑性能，操作比较频繁
    std::map<uint32_t, ConnPipeline*> pipelines_;
    std::map<std::string, std::set<uint32_t>> service_pipelines_;
    
    struct ServerConnInfo {
        bool Equals(const std::string& _server_name, uint32_t _server_number) const {
            if (_server_name == server_name && server_number == _server_number) {
                return true;
            }
            return false;
        }
        
        bool Equals(ConnPipeline* _pipeline) {
            return _pipeline == pipeline;
        }
        
        std::string server_name;
        uint32_t server_number{0};
        ConnPipeline* pipeline{nullptr};
    };
    
    std::list<ServerConnInfo> server_conns_;
};

///////////////////////////////////////////////////////////////////////////////////////
// 所有的连接管理器
class IOThreadPoolConnManager {
public:
    IOThreadPoolConnManager(size_t num_threads);
    
    inline std::shared_ptr<DefaultIOThreadPoolExecutor> GetIOThreadPoolExecutor() const {
        return thread_pool_;
    }
    
    // EventBase线程里执行
    uint64_t OnNewConnection(ConnPipeline* pipeline);
    // EventBase线程里执行
    bool OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline);

    void OnServerRegister(ConnPipeline* pipeline, const std::string& server_name, uint32_t server_number);

    // 需要找到网络线程
    bool DispatchIOBufByConnID(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data);

    bool BroadCast(std::unique_ptr<folly::IOBuf> data);
    
    bool DispatchIOBufByServer(const std::string& server_name, uint32_t server_number, std::unique_ptr<folly::IOBuf> data);

    // 这个接口主要给group用
    // 因为所属group全部都要在一个EventBase里
    std::shared_ptr<IOThreadConnManager> GetNextThreadConnManager() const;
    
    // 这个接口主要给group用
    // 因为所属group全部都要在一个EventBase里
    std::shared_ptr<IOThreadConnManager> GetThreadConnManager(uint64_t conn_id) const;
    
    std::shared_ptr<IOThreadConnManager> GetThreadConnManager() const;

    size_t GetThreadConnManagersSize() const {
        return thread_conn_managers_.size();
    }
    
    std::shared_ptr<IOThreadConnManager> GetThreadConnManagerByIdx(size_t idx) {
        return thread_conn_managers_[idx];
    }
    
private:
    bool DispatchIOBufByIdx(size_t idx,
                            const std::string& server_name,
                            uint32_t server_number,
                            std::shared_ptr<folly::IOBuf> data);

    // 总共连接数
    std::shared_ptr<DefaultIOThreadPoolExecutor> thread_pool_;
    folly::fbvector<std::shared_ptr<IOThreadConnManager>> thread_conn_managers_;
    
    mutable size_t current_thread_id_{0};
};

#endif
