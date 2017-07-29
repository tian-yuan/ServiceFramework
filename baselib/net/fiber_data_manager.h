#ifndef NET_FIBER_DATA_MANAGER_H_
#define NET_FIBER_DATA_MANAGER_H_

#include <boost/any.hpp>
#include <folly/io/async/EventBase.h>
#include <folly/fibers/Baton.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/EventBaseLoopController.h>

#define FIBER_HTTP_TIMEOUT 2
#define FIBER_SUSPEND_TIMEOUT 3
#define FIBER_RPC_TIMEOUT 8
#define FIBER_DB_TIMEOUT 4
#define FIBER_REDIS_TIMEOUT 4
#define FIBER_MAX_QUEUE_SIZE  4096


// folly::fibers::Baton::TimeoutHandler batonTimeoutHandler_;
// std::chrono::milliseconds batonWaitTimeout_{0};

// #define FIBER_OBJ_POOL
// #define USE_TIMEOUT_HANDLER

// TODO(@benqi), 设计有点问题：FiberDataInfo可以使用一个全局管理器
// 设计有点问题，

namespace base {
    
struct Stat {
    Stat() = default;
    ~Stat() = default;
    
    void Increase() {
        ++stat;
    }
    
    void Increase(uint64_t diff) {
    	stat += diff;
    }

    void Decrease() {
    	--stat;
    }

    void Decrease(uint64_t diff) {
    	stat -= diff;
    }

    ///< 获取当前统计数据与上一次统计数据的差值
    uint64_t CalcAndRestoreLastStat() {
        auto this_stat = stat.load();
        uint64_t calc = 0;
        if (this_stat < last_stat) {
            calc = 0xffffffff-last_stat+this_stat;
        } else {
            calc = this_stat - last_stat;
        }
        last_stat = this_stat;
        return calc;
    }
    
    ///< 获取最新的统计数量
    uint64_t GetLatestStat() {
    	auto this_stat = stat.load();
    	return this_stat;
    }

    std::string stat_name;          // 类型
    uint64_t last_stat{0};          // 上一次统计点
    std::atomic<uint64_t> stat;     // 总计
};

class Stats {
public:
    explicit Stats(const std::string& v, size_t size) :
        stats_name_(v),
        stats_(size) {}
    
    void InitStat(size_t idx, const std::string& name) {
        if (idx<stats_.size()) {
            stats_[idx].stat_name = name;
        }
    }

    void Increase(size_t idx) {
        if (LIKELY(idx<stats_.size())) {
            stats_[idx].Increase();
        }
    }

    void Increase(size_t idx, uint64_t diff) {
		if (LIKELY(idx<stats_.size())) {
			stats_[idx].Increase(diff);
		}
	}

    void Decrease(size_t idx) {
		if (LIKELY(idx<stats_.size())) {
			stats_[idx].Decrease();
		}
	}

	void Decrease(size_t idx, uint64_t diff) {
		if (LIKELY(idx<stats_.size())) {
			stats_[idx].Decrease(diff);
		}
	}

    std::string CalcAndToString() {
        std::ostringstream  o;
        o << "stats: {" << stats_name_ << ":[";
        if (!stats_.empty()) {
            o << "{" << stats_[0].stat_name << ":" << stats_[0].CalcAndRestoreLastStat() << "}";
        }
        
        for (size_t i=1; i<stats_.size(); ++i) {
            o << ", {" << stats_[i].stat_name << ":" << stats_[i].CalcAndRestoreLastStat() << "}";
        }
        
        o << "]}";
        return o.str();
    }

    uint64_t CalcAndRestoreLastStat(size_t idx) {
    	uint64_t last_stat = 0;
    	if (LIKELY(idx<stats_.size())) {
    		last_stat = stats_[idx].CalcAndRestoreLastStat();
		}
    	return last_stat;
    }

    uint64_t GetLatestStat(size_t idx) {
    	uint64_t last_stat = 0;
		if (LIKELY(idx<stats_.size())) {
			last_stat = stats_[idx].GetLatestStat();
		}
		return last_stat;
    }

    std::string GetStatsName(size_t idx) {
    	std::string stats_name;
    	if (LIKELY(idx<stats_.size())) {
    		stats_name = stats_[idx].stat_name;
		}
		return stats_name;
    }

private:
    std::string stats_name_;    // 打印
    std::vector<Stat> stats_;   // 组
};
    
}

// 协程统计:
// 1. addTask
// 2. rpc
// 3. db(tps/qps)
// 4. redis
// 5. http

// 协程fiber帮助类
class FiberDataManager;

struct FiberDataInfo {
    enum State {
        kIdle = 0,          // 空闲
        kNotWaiting,        // 没有进入协程切换就返回，例如：调用SendFiberData时无连接／切换协程时输入参数有问题／Http连接不成功
        kWaiting,           // 切换到协程等待状态
        kTimeout,           // 协程超时退出
        kPosted,            // 正常唤醒
        kPostedHasError,    // 错误数据包等原因被唤醒
    };
    
    std::string ToString() const;
    
    void Clear() {
        state = kIdle;
        baton.reset();
    }
    
    void Wait(int time_out);
    void Post(bool rc);
    
    // |池id|thread_id|使用次数||
    uint64_t fiber_id{0};               //
    // uint32_t lower_fiber_id{0};      // 低32位fiber_id
    
    folly::fibers::Baton baton;
    
    int state{kIdle};
    
#ifdef USE_TIMEOUT_HANDLER
    folly::fibers::Baton::TimeoutHandler baton_timeout_handler;
#endif

    std::shared_ptr<folly::fibers::FiberManager> fm;
    FiberDataManager* fiber_data_manager{nullptr};
};

std::string ToStringByFiberID(uint64_t fiber_id);
size_t GetThreadIDByFiberID(uint64_t fiber_id);

typedef std::shared_ptr<FiberDataInfo> FiberDataInfoPtr;

// typedef std::list<FiberDataInfo> FiberDataPool;

class FiberDataManager {
public:
    // fiber统计项
    enum FiberStat {
        f_add_task = 0,
        f_rpc,
        f_db_tps,
        f_db_qps,
        f_reids,
        f_http,
        f_max_number,
    };

    FiberDataManager(size_t max_queue_size = FIBER_MAX_QUEUE_SIZE) :
        max_queue_size_(max_queue_size) {}
    FiberDataManager(folly::EventBase* main_evb,
                     uint32_t thread_id,
                     size_t max_queue_size = FIBER_MAX_QUEUE_SIZE);
    ~FiberDataManager() = default;

    bool Initialize(folly::EventBase* main_evb, uint32_t thread_id);
    
    // 从池里分配
    FiberDataInfoPtr AllocFiberData();
    FiberDataInfoPtr FindFiberData(uint64_t fiber_id) const;
    bool DeleteFiberData(const FiberDataInfoPtr& data);
    bool DeleteFiberData(uint64_t fiber_id);
    
    folly::EventBase* GetEventBase() const {
        return main_evb_;
    }
    
    std::shared_ptr<folly::fibers::FiberManager> GetFiberManager() const {
        return fm_;
    }
    
    size_t GetFiberQueueSize() const {
        return datas_.size();
    }
    
    bool IsInited() const {
        return inited_;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    // fiber
    static void InitFiberStats() {
        g_fiber_stats.InitStat(f_add_task, "f_add_task");
        g_fiber_stats.InitStat(f_rpc, "f_rpc");
        g_fiber_stats.InitStat(f_db_tps, "f_db_tps");
        g_fiber_stats.InitStat(f_db_qps, "f_db_qps");
        g_fiber_stats.InitStat(f_reids, "f_reids");
        g_fiber_stats.InitStat(f_http, "f_http");
    }
    
    static void IncreaseStats(size_t idx) {
        g_fiber_stats.Increase(idx);
    }
    
    static std::string FiberCalcAndToString() {
        return g_fiber_stats.CalcAndToString();
    }
    
    // static void InitFiberStats();
    
    /////////////////////////////////////////////////////////////////////////////////
//    static void StatsIncrTask() {
//        ++g_fiber_task_stat;
//
//    }
//    static void StatsResetTask() {
//        g_fiber_task_stat.store(0, std::memory_order_relaxed);
//    }
//
//    static uint32_t GetTaskSize() {
//        return g_fiber_task_stat.load();
//    }
    
private:
    folly::EventBase* main_evb_{nullptr};
    
    // 协程ID和当前线程内已经分配的ID组合成协程ID
    uint32_t thread_id_{0};
    // uint32_t current_fiber_id_{0};
    
#ifdef FIBER_OBJ_POOL
    std::list<FiberDataInfoPtr> idles_;
#else
    uint32_t current_fiber_id_{0};
#endif
    std::unordered_map<uint64_t, std::shared_ptr<FiberDataInfo>> datas_;

    // 对象池
//    std::unordered_map<uint64_t, FiberDataInfoPtr> datas_;
    
    std::shared_ptr<folly::fibers::FiberManager> fm_;
    bool inited_{false};

    size_t max_queue_size_;
    
    static base::Stats g_fiber_stats;
    // std::atomic<uint32_t> g_fiber_task_stat;
};

FiberDataManager& GetFiberDataManagerByThreadLocal();

#endif
