#include "net/io_thread_pool_manager.h"

#include <folly/io/async/EventBaseManager.h>
#include <folly/Random.h>

#include "net/im_conn_handler.h"
#include "net/base/service_base.h"

///////////////////////////////////////////////////////////////////////////////////////
// 使用单件
static DefaultIOThreadPoolExecutor* g_io_thread_pool = nullptr;

DefaultIOThreadPoolExecutor::DefaultIOThreadPoolExecutor(size_t num_threads) :
    wangle::IOThreadPoolExecutor(num_threads),
    num_threads_(num_threads) {
        
    g_io_thread_pool = this;
}

folly::EventBase* DefaultIOThreadPoolExecutor::GetEventBase(size_t idx) {
    auto thread = threadList_.get()[idx];
    return wangle::IOThreadPoolExecutor::getEventBase(thread.get());
}

DefaultIOThreadPoolExecutor::~DefaultIOThreadPoolExecutor() {
    g_io_thread_pool = nullptr;
}

folly::EventBase* GetEventBaseByThreadID(size_t idx) {
    folly::EventBase* rv = nullptr;
    
    if (g_io_thread_pool) {
        rv = g_io_thread_pool->GetEventBase(idx);
    }
    
    return rv;
}

folly::EventBase* GetEventBaseByRand() {
    size_t thread_size = g_io_thread_pool->GetEventBaseSize();
//    return GetEventBaseByThreadID(folly::Random::rand32(static_cast<uint32_t>(thread_size)));
    
    if (thread_size == 1) {
        return g_io_thread_pool->GetEventBase(0);
    } else {
        folly::EventBase* me = folly::EventBaseManager::get()->getEventBase();
        do {
            folly::EventBase* evb =  GetEventBaseByThreadID(folly::Random::rand32(static_cast<uint32_t>(thread_size)));
            if (me != evb) {
                me = evb;
                break;
            }
        } while(1);
        
        return me;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// EventBase线程里执行
uint32_t IOThreadConnManager::OnNewConnection(ConnPipeline* pipeline) {
    uint32_t conn_id = 0;
    bool sucess = false;
    do {
        conn_id = ++current_conn_id_;
        // 保证conn_id不为0
        if (conn_id == 0) ++current_conn_id_;
        sucess = pipelines_.insert(std::make_pair(conn_id, pipeline)).second;
    } while(!sucess);
    
    // pipelines_[conn_id] = pipeline;
    
    const std::string& service_name = pipeline->getHandler<IMConnHandler>()->GetServiceBase()->GetServiceName();
    auto it = service_pipelines_.find(service_name);
    if (it == service_pipelines_.end()) {
        std::set<uint32_t> tmp;
        tmp.insert(conn_id);
        service_pipelines_.insert(std::make_pair(service_name, tmp));
    } else {
        it->second.insert(conn_id);
    }
    return conn_id;
}

void IOThreadConnManager::OnServerRegister(ConnPipeline* pipeline, const std::string& server_name, uint32_t server_number) {
    
    LOG(INFO) << "OnServerRegister - access_server register, server_name: " << server_name
                << ", server_number: " << server_number;
    for (auto& v : server_conns_) {
        if (v.Equals(server_name, server_number)) {
            // v.pipeline = pipeline;
            
            LOG(ERROR) << "OnServerRegister - server_name is register, erver_name: " << server_name
                        << ", server_number: " << server_number;
            // return;
        }
    }
    
    ServerConnInfo conn_info;
    conn_info.server_name = server_name;
    conn_info.server_number = server_number;
    conn_info.pipeline = pipeline;
    
    server_conns_.push_back(conn_info);
}

// EventBase线程里执行
bool IOThreadConnManager::OnConnectionClosed(uint32_t conn_id, ConnPipeline* pipeline) {
    // auto it = pipelines_.find(conn_id & 0xffffffff);
    auto it = pipelines_.find(conn_id);
    if (it!=pipelines_.end()) {
        pipelines_.erase(it);
    }
    
    const std::string& service_name = pipeline->getHandler<IMConnHandler>()->GetServiceBase()->GetServiceName();
    auto it2 = service_pipelines_.find(service_name);
    if (it2 != service_pipelines_.end()) {
        it2->second.erase(conn_id);
    }

    // access_server
    // std::list<ServerConnInfo>::iterator it3;
    //for () {
    if (server_conns_.empty())
        return true;
    
    bool find = false;
    do {
        find = false;
        for (auto it3=server_conns_.begin(); it3!=server_conns_.end(); ++it3) {
            if (it3->Equals(pipeline)) {
                LOG(INFO) << "OnConnectionClosed - remove register, server_name: " << it3->server_name
                                << ", server_number: " << it3->server_number;
                server_conns_.erase(it3);
                find = true;
                
                break;
            }
        }
    } while(find);
    
    return true;
}

// 需要找到网络线程
bool IOThreadConnManager::DispatchIOBufByConnID(uint32_t conn_id, std::unique_ptr<folly::IOBuf> data) {
    // uint32_t low_conn_id = conn_id & 0xffffffff;
    LOG(INFO) << "DispatchIOBufByConnID - Ready find pipeline: by conn_id: " << conn_id
                << ", thread_id: " << thread_id_;

    auto it = pipelines_.find(conn_id);
    if (it!=pipelines_.end()) {
        it->second->write(std::move(data));
    } else {
        LOG(ERROR) << "DispatchIOBufByConnID - Not find conn_id: " << conn_id
                << ", thread_id: " << thread_id_;
    }
    return true;
}

bool IOThreadConnManager::DispatchIOBufByServer(const std::string& server_name, uint32_t server_number, std::shared_ptr<folly::IOBuf>data) {
    for (auto& v : server_conns_) {
        if (v.Equals(server_name, server_number)) {
            if (v.pipeline) {
                v.pipeline->write(data->clone());
                return true;
            }
        }
    }
    
    return false;
}

bool IOThreadConnManager::BroadCast(std::shared_ptr<folly::IOBuf> data) {
    for (auto it : pipelines_) {
        LOG(INFO) << "IOThreadConnManager - BroadCast()";
        it.second->write(data->clone());
    }
    return true;
}

ConnPipeline* IOThreadConnManager::FindPipeline(uint32_t conn_id) {
    ConnPipeline* pipeline = nullptr;

    LOG(INFO) << "FindPipeline - Ready find pipeline: by conn_id: " << conn_id
                    << ", thread_id: " << thread_id_;

    auto it = pipelines_.find(conn_id);
    if (it!=pipelines_.end()) {
        pipeline = it->second;
    } else {
        LOG(ERROR) << "FindPipeline - Not find by conn_id: " << conn_id
                    << ", thread_id: " << thread_id_;
    }
    
    return pipeline;
}

static std::set<uint32_t> kUint32SetEmpty;

const std::set<uint32_t>& IOThreadConnManager::FindPipelines(const std::string& service) const {
    auto it = service_pipelines_.find(service);
    if (it == service_pipelines_.end()) {
        return kUint32SetEmpty;
    } else {
        return it->second;
    }
}

///////////////////////////////////////////////////////////////////////////////////////
IOThreadPoolConnManager::IOThreadPoolConnManager(size_t num_threads) {
    
    thread_pool_ = std::make_shared<DefaultIOThreadPoolExecutor>(num_threads);
    for (size_t i=0; i<num_threads; ++i) {
        auto evb = thread_pool_->GetEventBase(i);
        thread_conn_managers_.push_back(std::make_shared<IOThreadConnManager>(i, evb));
        // 初始化
        evb->runImmediatelyOrRunInEventBaseThreadAndWait([i, evb] {
            // 初始化fibbers
            FiberDataManager& fibers = GetFiberDataManagerByThreadLocal();
            fibers.Initialize(evb, static_cast<uint32_t>(i));
            // fibers.GetFiberManager();
            LOG(INFO) << "IOThreadPoolConnManager - Initialize fiber_data_manager by id: " << i;
        });
    }
}

// EventBase线程里执行
uint64_t IOThreadPoolConnManager::OnNewConnection(ConnPipeline* pipeline) {
    folly::EventBase* event_base = pipeline->getTransport()->getEventBase();
    for (size_t i=0; i<thread_conn_managers_.size(); ++i) {
        if (thread_conn_managers_[i]->GetEventBase() == event_base) {
            uint32_t low = thread_conn_managers_[i]->OnNewConnection(pipeline);
            return static_cast<uint64_t>(i << 32) | low;
        }
    }

    return std::numeric_limits<uint64_t>::max();
}

void IOThreadPoolConnManager::OnServerRegister(ConnPipeline* pipeline, const std::string& server_name, uint32_t server_number) {
    folly::EventBase* event_base = pipeline->getTransport()->getEventBase();
    for (size_t i=0; i<thread_conn_managers_.size(); ++i) {
        if (thread_conn_managers_[i]->GetEventBase() == event_base) {
            thread_conn_managers_[i]->OnServerRegister(pipeline, server_name, server_number);
            break;
        }
    }
}

// EventBase线程里执行
bool IOThreadPoolConnManager::OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) {
    uint32_t high = conn_id >> 32;
    uint32_t low = conn_id & 0xffffffff;
    return thread_conn_managers_[high]->OnConnectionClosed(low, pipeline);
}

// 需要找到网络线程
bool IOThreadPoolConnManager::DispatchIOBufByConnID(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data) {
    uint32_t high = conn_id >> 32;
    uint32_t low = conn_id & 0xffffffff;

    std::shared_ptr<folly::IOBuf> o = std::shared_ptr<folly::IOBuf>(std::move(data));

    auto conn = thread_conn_managers_[high];
    conn->GetEventBase()->runInEventBaseThread([conn, low, o] {
        // std::shared_ptr<folly::IOBuf> o2 = std::shared_ptr<folly::IOBuf>(std::move(data));
        conn->DispatchIOBufByConnID(low, std::move(o->clone()));
    });

    return true;
}


bool IOThreadPoolConnManager::DispatchIOBufByIdx(size_t idx,
                                                 const std::string& server_name,
                                                 uint32_t server_number,
                                                 std::shared_ptr<folly::IOBuf> data) {
    size_t index = idx;
    if (index < thread_conn_managers_.size()) {
        
        if (thread_conn_managers_[idx]->DispatchIOBufByServer(server_name, server_number, data)) {
            LOG(INFO) << "DispatchIOBufByIdx - DispatchIOBufByServer sucess: "
                << "idx:" << idx
                << "server_name: " << server_name
                << "server_number: " << server_number;
        } else {
            // 继续往下找
            ++index;
            if (index < thread_conn_managers_.size()) {
                folly::EventBase* evb = thread_conn_managers_[index]->GetEventBase();
                if (evb) {
                    evb->runInEventBaseThread([this, index, server_name, server_number, data] {
                        this->DispatchIOBufByIdx(index, server_name, server_number, data);
                    });
                }
            } else {
                LOG(INFO) << "DispatchIOBufByIdx - Not thread_conn, maybe conn is closed: "
                    << "idx:" << idx
                    << "server_name: " << server_name
                    << "server_number: " << server_number;
            }
        }
    } else {
        LOG(ERROR) << "DispatchIOBufByIdx - array index range, "
            << "idx:" << idx
            << "server_name: " << server_name
            << "server_number: " << server_number;
    }
    
    return true;
}

bool IOThreadPoolConnManager::DispatchIOBufByServer(const std::string& server_name, uint32_t server_number, std::unique_ptr<folly::IOBuf> data) {
    std::shared_ptr<folly::IOBuf> o = std::shared_ptr<folly::IOBuf>(std::move(data));
    
    size_t idx = 0;
    folly::EventBase* evb = thread_conn_managers_[idx]->GetEventBase();
    if (evb) {
        evb->runInEventBaseThread([this, idx, server_name, server_number, o] {
            this->DispatchIOBufByIdx(idx, server_name, server_number, o);
        });
    } else {
        LOG(ERROR) << "DispatchIOBufByServer - evb is null";
    }
    
    return true;
}

bool IOThreadPoolConnManager::BroadCast(std::unique_ptr<folly::IOBuf> data) {
    std::shared_ptr<folly::IOBuf> o = std::shared_ptr<folly::IOBuf>(std::move(data));

    for (auto v : thread_conn_managers_) {
        v->GetEventBase()->runInEventBaseThread([v, o] {
            v->BroadCast(o);
        });
    }
    
    return true;
}

std::shared_ptr<IOThreadConnManager> IOThreadPoolConnManager::GetNextThreadConnManager() const {
    // 有一些应用会配置成单线程
    // 先检查一下是否是单线程
    if (thread_conn_managers_.size() == 1) {
        return thread_conn_managers_[0];
    } else {
        // 配置成了多线程
        if (current_thread_id_ == thread_conn_managers_.size()) {
            current_thread_id_ = 0;
        }
        
        return thread_conn_managers_[current_thread_id_++];
    }
}

std::shared_ptr<IOThreadConnManager> IOThreadPoolConnManager::GetThreadConnManager(uint64_t conn_id) const {
    std::shared_ptr<IOThreadConnManager> rv;
    
    uint32_t high = conn_id >> 32;
    if (high<thread_conn_managers_.size()) {
        rv = thread_conn_managers_[high];
    }
    
    return rv;
}

std::shared_ptr<IOThreadConnManager> IOThreadPoolConnManager::GetThreadConnManager() const {
    std::shared_ptr<IOThreadConnManager> conn;
    folly::EventBase* this_evb = folly::EventBaseManager::get()->getEventBase();
    for (auto& evb : thread_conn_managers_) {
        if (this_evb == evb->GetEventBase()) {
            conn = evb;
            break;
        }
    }
    
    return conn;
}

