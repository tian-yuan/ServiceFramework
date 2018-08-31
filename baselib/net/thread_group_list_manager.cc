#include "net/thread_group_list_manager.h"

#include <wangle/concurrent/CPUThreadPoolExecutor.h>

#include "net/thread_local_conn_manager.h"
#include "net/io_thread_pool_manager.h"
#include "base/configuration.h"

ThreadGroup::ThreadGroup(std::shared_ptr<wangle::ThreadPoolExecutor> pool,
                         ThreadType thread_type,
                         const NewThreadDataCallback& cb)
    : thread_pool(pool),
      thread_type(thread_type),
      thread_observer_(std::make_shared<ThreadGroupObserver>(this, cb)) {
    
    thread_pool->addObserver(thread_observer_);
}


void ThreadGroup::ThreadGroupObserver::threadStarted(wangle::ThreadPoolExecutor::ThreadHandle* handle) {
    // LOG(INFO) << "threadStarted - ";
    if (callback_) {
        // LOG(INFO) << "threadStarted - ";
        size_t idx = callback_(thread_group_, handle);
        thread_group_->thread_idxs.push_back(idx);
    }
}

void ThreadGroup::ThreadGroupObserver::threadStopped(wangle::ThreadPoolExecutor::ThreadHandle* handle) {
    if (callback_) {
        // callback_(thread_group_, handle);
    }
}

bool ThreadGroupListOption::SetConf(const std::string &conf_name, const nlohmann::json& conf) {
    int thread_size = 0;
    if (conf.find("conn") != conf.end()) {
        int conn = conf["conn"].get<int>();
    	thread_size = conn > 0 ? conn : (::GetIOThreadSize());
        options.emplace_back(ThreadType::CONN, thread_size);
    } else {
        if (conf.find("accept") != conf.end()) {
            int accept = conf["accept"].get<int>();
        	thread_size = accept > 0 ? accept : (::GetIOThreadSize());
            options.emplace_back(ThreadType::CONN_ACCEPT, thread_size);
        }
        
        if (conf.find("client") != conf.end()) {
            int client = conf["client"].get<int>();
        	thread_size = client > 0 ? client : (::GetIOThreadSize());
            options.emplace_back(ThreadType::CONN_CLIENT, thread_size);
        }
    }
    
    if (conf.find("fiber") != conf.end()) {
        int fiber = conf["fiber"].get<int>();
    	thread_size = fiber > 0 ? fiber : (::GetIOThreadSize());
        options.emplace_back(ThreadType::FIBER, thread_size);
    }
    
    if (conf.find("db") != conf.end()) {
        int db = conf["db"].get<int>();
    	thread_size = db > 0 ? db : (::GetIOThreadSize());
        options.emplace_back(ThreadType::DB, thread_size);
    }
    
    if (conf.find("redis") != conf.end()) {
        int redis = conf["redis"].get<int>();
    	thread_size = redis > 0 ? redis : (::GetIOThreadSize());
        options.emplace_back(ThreadType::REDIS, thread_size);
    }
    
    return true;
}

ThreadGroupListManager::ThreadGroupListManager(const ThreadGroupListOption& options)
    : thread_groups_(static_cast<int>(ThreadType::MAX)) {
    Initialize(options);
}

bool ThreadGroupListManager::Initialize(const ThreadGroupListOption& options) {
    if (options.options.empty()) {
        LOG(ERROR) << "Initialize - options is empty";
        return false;
    }
    
    for (auto& option : options.options) {
        auto thread_group = MakeThreadGroup(option.thread_type, option.thread_size);
        int idx = static_cast<int>(option.thread_type);
        thread_groups_[idx] = thread_group;
    }
    
    return true;
}

ThreadGroupPtr ThreadGroupListManager::MakeThreadGroup(ThreadType thread_type, int thread_size) {
    ThreadGroupPtr thread_group;
    std::shared_ptr<wangle::ThreadPoolExecutor> pool;
    switch (thread_type) {
        case ThreadType::NORMAL:
            break;
        case ThreadType::CONN_ACCEPT:
        case ThreadType::CONN_CLIENT:
        // case ThreadType::CONN:
        case ThreadType::FIBER:
            pool = std::make_shared<wangle::IOThreadPoolExecutor>(thread_size);
            break;
        case ThreadType::DB:
        case ThreadType::REDIS:
            pool = std::make_shared<wangle::CPUThreadPoolExecutor>(thread_size);
            break;
        default:
            LOG(ERROR) << "MakeThreadGroup - thread_type: " << ToString(thread_type) << " not support!!!";
            break;
    }
    
    if (pool) {
        auto cb = [&](ThreadGroup* group,
                      wangle::ThreadPoolExecutor::ThreadHandle* handle) -> size_t {
            size_t idx = thread_datas_.size();
            
            folly::EventBase* evb = nullptr;
            auto t = group->GetThreadType();
            switch (t) {
                case ThreadType::CONN_ACCEPT:
                case ThreadType::CONN_CLIENT:
                // case ThreadType::CONN:
                case ThreadType::FIBER:
                    evb = wangle::IOThreadPoolExecutor::getEventBase(handle);
                    evb->runImmediatelyOrRunInEventBaseThreadAndWait([idx](){
                        LOG(INFO) << "MakeThreadGroup - thread_idx: " << idx;
                        GetConnManagerByThreadLocal().set_thread_id(idx);
                    });
                    break;
                default:
                    break;
            }
            
            LOG(INFO) << "MakeThreadGroup - thread_type: " << ToString(thread_type) << ", thread_idx: " << idx;
            thread_datas_.emplace_back(group, idx, evb);
            return idx;
        };
        
        thread_group = std::make_shared<ThreadGroup>(pool, thread_type, cb);
    } else {
        LOG(ERROR) << "MakeThreadGroup - pool is nil";
    }
    
    return thread_group;
}


// 通过ThreadType获取IOThreadPoolExecutor
std::shared_ptr<wangle::IOThreadPoolExecutor> ThreadGroupListManager::GetIOThreadPoolExecutor(ThreadType thread_type) const {
    std::shared_ptr<wangle::IOThreadPoolExecutor> pool;
    if (thread_type == ThreadType::CONN_ACCEPT ||
        thread_type == ThreadType::CONN_CLIENT ||
        // thread_type == ThreadType::CONN ||
        thread_type == ThreadType::FIBER) {
        auto idx = static_cast<size_t>(thread_type);
        auto thread_group = thread_groups_[idx];
        if (thread_group) {
            auto p = thread_group->GetThreadPool();
            pool = std::static_pointer_cast<wangle::IOThreadPoolExecutor>(p);
            if (!pool) {
                LOG(ERROR) << "GetIOThreadPoolExecutor - Not find thread_type: " << ToString(thread_type)
                            << ", idx:" << idx << " in thread_groups!!!!";
            }
        }
        // auto idx = thread_groups_[static_cast<int>(thread_type);
        // pool = std::static_pointer_cast<wangle::IOThreadPoolExecutor>(]->GetThreadPool());
    }
    return pool;
}

folly::EventBase* ThreadGroupListManager::GetEventBaseByThreadType(ThreadType thread_type) const {
    auto group = GetThreadGroupByThreadType(thread_type);
    if (group) {
        folly::ThreadLocalPRNG rng;
        uint32_t idx = folly::Random::rand32(static_cast<uint32_t>(group->thread_idxs.size()));
        return thread_datas_[idx].evb;
    } else {
        return nullptr;
    }
}


