#include "net/fiber_data_manager.h"

#include <folly/SingletonThreadLocal.h>
#include <folly/Format.h>

#include "message_util/call_chain_util.h"

//   使用ThreadLocal支持协程
//   但主框架没有这么用，后续需要将主框架也统一起来
//static folly::SingletonThreadLocal<FiberDataManager> g_fiber_data_manager;
// cache([id]() {

//FiberDataInfo::~FiberDataInfo() {
//    LOG(INFO) << "FiberDataInfo - Destroy fiber_id: " << fiber_id
//                << ", low_fiber_id: " << (fiber_id & 0xffffffff);;
//}
//
/**
 mcrouter选项:
 
 mcrouter_option_integer(
 size_t, fibers_max_pool_size, 1000,
 "fibers-max-pool-size", no_short,
 "Maximum number of preallocated free fibers to keep around")
 
 mcrouter_option_integer(
 size_t, fibers_stack_size, 24 * 1024,
 "fibers-stack-size", no_short,
 "Size of stack in bytes to allocate per fiber."
 " 0 means use fibers library default.")
 
 mcrouter_option_integer(
 size_t, fibers_record_stack_size_every, 100000,
 "fibers-record-stack-size-every", no_short,
 "Record exact amount of fibers stacks used for every N fiber. "
 "0 disables stack recording.")
 
 mcrouter_option_toggle(
 fibers_use_guard_pages, true,
 "disable-fibers-use-guard-pages", no_short,
 "If enabled, protect limited amount of fiber stacks with guard pages")
 
 mcrouter_option_integer(
 uint32_t, fibers_pool_resize_period_ms, 60000,
 "fibers-pool-resize-period-ms", no_short,
 "Free unnecessary fibers in the fibers pool every"
 " fibers-pool-resize-period-ms milliseconds.  If value is 0, periodic"
 " resizing of the free pool is disabled.")
 */

#define FIBER_STACK_SIZE    96*1024     // 默认设置为64K

static std::shared_ptr<FiberDataInfo> kEmptyFiberDataInfoPtr;

// std::atomic<uint32_t> FiberDataManager::g_fiber_task_stat;
base::Stats FiberDataManager::g_fiber_stats("fiber", FiberDataManager::f_max_number);


std::string ToStringByFiberID(uint64_t fiber_id) {
#ifdef FIBER_OBJ_POOL
    return folly::sformat("[fiber_id: {{pool_id:{}, thread_id:{}, use_count:{}}}]",
                          fiber_id >> 32,
                          ((fiber_id >> 16) & 0xffff),
                          fiber_id & 0xffff);
#else
    return folly::sformat("[fiber_data: {{thread_id:{}, pool_id:{}}}]",
                          fiber_id >> 32,
                          (fiber_id & 0xffffffff));
#endif
}

std::string FiberDataInfo::ToString() const {
#ifdef FIBER_OBJ_POOL
    return folly::sformat("[fiber_data: {{pool_id:{}, thread_id:{}, use_count:{}, queue_size:{}}}]",
                          fiber_id >> 32,
                          ((fiber_id >> 16) & 0xffff),
                          (fiber_id & 0xffff),
                          fiber_data_manager->GetFiberQueueSize());
#else
    return folly::sformat("[fiber_data: {{thread_id:{}, pool_id:{}, queue_size:{}}}]",
                          fiber_id >> 32,
                          (fiber_id & 0xffffffff),
                          fiber_data_manager->GetFiberQueueSize());
#endif
}

void FiberDataInfo::Wait(int time_out) {
#ifdef USE_TIMEOUT_HANDLER
    folly::fibers::addTask([this, time_out] {
        baton_timeout_handler.scheduleTimeout(std::chrono::seconds(time_out));
    });
    
    // bool r = false;
    state = kWaiting;
    baton.wait(baton_timeout_handler);
    if (state == kWaiting) {
        // 超时唤醒
        state = kTimeout;
    }
    
    // 正常唤醒
    // 未知错误唤醒
    // 由state判断
#else
    bool rv = baton.timed_wait(std::chrono::seconds(time_out));
    if (!rv) {
        state = kTimeout;
    }
#endif
}

void FiberDataInfo::Post(bool rc) {
    if (rc) {
        state = kPosted;
    } else {
        state = kPostedHasError;
    }
    baton.post();
}

size_t GetThreadIDByFiberID(uint64_t fiber_id) {
#ifdef FIBER_OBJ_POOL
    return fiber_id >> 16 & 0xffff;
#else
    return fiber_id >> 32;
#endif
}


folly::fibers::FiberManager::Options getFiberManagerOptions() {
    folly::fibers::FiberManager::Options fmOpts;
    fmOpts.stackSize = FIBER_STACK_SIZE;
    fmOpts.recordStackEvery = 100000;
    fmOpts.maxFibersPoolSize = 10240;
    fmOpts.useGuardPages = true;
    fmOpts.fibersPoolResizePeriodMs = 60000;
    return fmOpts;
}

FiberDataManager::FiberDataManager(folly::EventBase* main_evb,
                                   uint32_t thread_id,
                                   size_t max_queue_size)
    : main_evb_(main_evb),
      thread_id_(thread_id),
      max_queue_size_(max_queue_size) {
    
    Initialize(main_evb, thread_id);
}

bool FiberDataManager::Initialize(folly::EventBase* main_evb, uint32_t thread_id) {
    if (!inited_) {
        
        main_evb_ = main_evb;
        thread_id_ = thread_id;

        // GetFiberManager();
        fm_ = std::make_shared<folly::fibers::FiberManager>(folly::make_unique<folly::fibers::EventBaseLoopController>(),
                                                            getFiberManagerOptions());
        dynamic_cast<folly::fibers::EventBaseLoopController&>(fm_->loopController()).attachEventBase(*main_evb_);
        
#ifdef FIBER_OBJ_POOL
        // Init pool
        for (size_t i=0; i<max_queue_qysize_; ++i) {
            auto data = std::make_shared<FiberDataInfo>();
            data->fiber_id = static_cast<uint64_t>(static_cast<uint64_t>(i+1)) << 32 |
                                                   thread_id_<< 16 |
                                                   0;
            data->fm = fm_;
            data->fiber_data_manager = this;
            // fiber_datas_.insert(std::make_pair(current_fiber_id_, data));
            idles_.push_back(data);
        }
#endif
        
        inited_ = true;
    }
    return true;
}

// TODO(@benqi)
std::shared_ptr<FiberDataInfo> FiberDataManager::AllocFiberData() {
    CHECK(inited_) << "AllocFiberData - Not inited";
    
#ifdef FIBER_OBJ_POOL
//    if (!fm_) {
//        fm_ = std::make_shared<folly::fibers::FiberManager>(folly::make_unique<folly::fibers::EventBaseLoopController>(),
//                                                            getFiberManagerOptions());
//        // fm_ = std::make_shared<folly::fibers::FiberManager>(folly::make_unique<folly::fibers::EventBaseLoopController>());
//        dynamic_cast<folly::fibers::EventBaseLoopController&>(
//                                               fm_->loopController()).attachEventBase(*main_evb_);
//
//        // fm_->loopController().attachEventBase(main_evb_);
//    }
    
    if (!idles_.empty()) {
        auto data = idles_.front();
        
        uint16_t fiber_ues_count = data->fiber_id & 0xffff;
        ++fiber_ues_count;
        data->fiber_id = (data->fiber_id & 0xffffffffffff0000) | fiber_ues_count;
        
        idles_.pop_front();
        datas_.insert(std::make_pair(data->fiber_id, data));
        
        // 队列超过32则输出警告日志
        if (datas_.size() > 32) {
            LOG(WARNING) << "AllocFiberData - AllocaFiberData sucess, but queue_size > 32: " << data->ToString();
        }
        return data;
    } else {
        LOG(ERROR) << "AllocFiberData - AllocaFiberData error, not idle fiber_data max_queu_size: " << FIBER_MAX_QUEUE_SIZE;
        return kEmptyFiberDataInfoPtr;
    }
#else
    if (datas_.size() < FIBER_MAX_QUEUE_SIZE) {
        ++current_fiber_id_;
        if (current_fiber_id_ == 0) ++current_fiber_id_;
        
        auto data = std::make_shared<FiberDataInfo>();
        data->fiber_id = static_cast<uint64_t>(static_cast<uint64_t>(thread_id_) << 32) | current_fiber_id_;
        data->fm = fm_;
        data->fiber_data_manager = this;
        
        datas_.insert(std::make_pair(current_fiber_id_, data));
        // 队列超过32则
        if (datas_.size() > 32) {
            LOG(WARNING) << "AllocFiberData - AllocaFiberData sucess, but queue_size > 32: " << data->ToString();
        }

        // LOG(INFO) << "Allocate - fiber_id: " << data->fiber_id
        //            << ", low_fiber_id: " << current_fiber_id_
        //            << ", queue_size: " << fiber_datas_.size();
        
        return data;
    } else {
        LOG(ERROR) << "AllocFiberData - AllocaFiberData error > max_queu_size: " << FIBER_MAX_QUEUE_SIZE;
        return kEmptyFiberDataInfoPtr;
    }
#endif
}

std::shared_ptr<FiberDataInfo> FiberDataManager::FindFiberData(uint64_t fiber_id) const {
    std::shared_ptr<FiberDataInfo> data;
//    LOG(INFO) << "FindFiberData - fiber_id: " << fiber_id << ", low_fiber_id: " << (fiber_id & 0xffffffff);
#ifdef FIBER_OBJ_POOL
    auto it = datas_.find(fiber_id);
#else
    auto it = datas_.find(fiber_id & 0xffffffff);
#endif
    if (it!=datas_.end())  {
        return it->second;
    } else {
#ifdef FIBER_OBJ_POOL
        LOG(ERROR) << "FindFiberData - not find fiber_id,  " << ToStringByFiberID(fiber_id)
                    << ", queue_size: " << datas_.size();
#else
        LOG(ERROR) << "FindFiberData - not find fiber_id,  " << ToStringByFiberID(fiber_id)
                    << ", queue_size: " << datas_.size();
#endif
        return kEmptyFiberDataInfoPtr;
    }
}

bool FiberDataManager::DeleteFiberData(const FiberDataInfoPtr& data) {
    return DeleteFiberData(data->fiber_id);
}

bool FiberDataManager::DeleteFiberData(uint64_t fiber_id) {
    //     LOG(INFO) << "DeleteFiberData - fiber_id: " << fiber_id << ", low_fiber_id: " << (fiber_id & 0xffffffff);
#ifdef FIBER_OBJ_POOL
    auto it = datas_.find(fiber_id);
#else
    auto it = datas_.find(fiber_id & 0xffffffff);
#endif
    
    if (it!=datas_.end())  {
        //        LOG(INFO) << "DeleteFiberData - finded, ready delete, fiber_id: " << fiber_id
        //                        << ", low_fiber_id: " << (fiber_id & 0xffffffff)
        //                        << ", queue_size: " << fiber_datas_.size();
#ifdef FIBER_OBJ_POOL
        it->second->Clear();
        idles_.push_back(it->second);
#endif
        datas_.erase(it);
        return true;
    } else {
        LOG(ERROR) << "DeleteFiberData - not find, fiber_data: " << ToStringByFiberID(fiber_id);
        return false;
    }
}

FiberDataManager& GetFiberDataManagerByThreadLocal() {
    static folly::SingletonThreadLocal<FiberDataManager> g_cache([]() {
        return new FiberDataManager();
    });
    return g_cache.get();
}

