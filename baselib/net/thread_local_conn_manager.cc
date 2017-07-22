#include "net/thread_local_conn_manager.h"

#include <folly/io/async/EventBaseManager.h>
#include <folly/SingletonThreadLocal.h>

#include "net/base/service_base.h"

///////////////////////////////////////////////////////////////////////////////////////
// EventBase线程里执行
uint64_t ThreadLocalConnManager::OnNewConnection(IMConnPipeline* pipeline) {
    uint32_t conn_id = 0;
    bool sucess = false;
    do {
        conn_id = ++current_conn_id_;
        // 保证conn_id不为0
        if (conn_id == 0) ++current_conn_id_;
        sucess = pipelines_.insert(std::make_pair(conn_id, pipeline)).second;
    } while(!sucess);

    return static_cast<uint64_t>(thread_id_) << 32 | conn_id;
}

// EventBase线程里执行
bool ThreadLocalConnManager::OnConnectionClosed(uint64_t conn_id, IMConnPipeline* pipeline) {
    // auto it = pipelines_.find(conn_id & 0xffffffff);
    bool rv = true;
    auto it = pipelines_.find(conn_id & 0xffffffff);
    if (it!=pipelines_.end()) {
        pipelines_.erase(it);
    } else {
        rv = false;
        LOG(ERROR) << "OnConnectionClosed - not find conn_id: " << conn_id;
    }
    return rv;
}

bool ThreadLocalConnManager::SendIOBufByConnID(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data) {
    LOG(INFO) << "DispatchIOBufByConnID - Ready find pipeline: by conn_id: " << conn_id
                << ", thread_id: " << thread_id_;
    
    auto it = pipelines_.find(conn_id & 0xffffffff);
    if (it!=pipelines_.end()) {
        it->second->write(std::move(data));
    } else {
        LOG(ERROR) << "DispatchIOBufByConnID - Not find conn_id: " << conn_id
                    << ", thread_id: " << thread_id_;
    }
    return true;
}

ThreadLocalConnManager& GetConnManagerByThreadLocal() {
    static folly::SingletonThreadLocal<ThreadLocalConnManager> g_cache([]() {
        return new ThreadLocalConnManager();
    });
    return g_cache.get();
}
