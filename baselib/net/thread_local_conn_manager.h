#ifndef NET_THREAD_LOCAL_CONN_MANAGER_H_
#define NET_THREAD_LOCAL_CONN_MANAGER_H_

#include <folly/FBVector.h>

#include <wangle/concurrent/IOThreadPoolExecutor.h>
#include <wangle/channel/Pipeline.h>

#include "net/server/tcp_service_base.h"

// class IMConnPipeline;
using IMConnPipeline = wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;


folly::EventBase* GetEventBaseByThreadID(size_t idx);
folly::EventBase* GetEventBaseByRand();

///////////////////////////////////////////////////////////////////////////////////////
// 基于ThreadLocal的连接管理器（从IOThreadConnManager改进）
// @benqi, 改进思路
// 以前的实现里有两个地方不是最优解：
//   1. service_pipelines_, 仅仅用于在gateway里，一旦status_server和gateway建立连接后,
//      gateway上报所有客户端的在线状态
//   2. server_conns_, 仅仅用于msg_server指定server_number发送
// 这两个功能是和具体的业务耦合在一起的:
//   对biz_server，1不使用
//   对gateway_server，2不使用
// 从ConnManager类的职责来说，不应该在此实现这两个功能
// 即使要实现1的功能，也不需要专门维护一个service_pipelines_来实现
class ThreadLocalConnManager : public TcpConnEventCallback {
public:
    ThreadLocalConnManager() {}

    // 设置线程ID
    // 注意线程一启动时要调用set_thread_id
    inline void set_thread_id(size_t thread_id) { thread_id_ = thread_id; }
    inline size_t thread_id() const { return thread_id_; }
    
    // EventBase线程里执行
    uint64_t OnNewConnection(ConnPipeline* pipeline) override;
    // EventBase线程里执行
    bool OnConnectionClosed(uint64_t conn_id, IMConnPipeline* pipeline) override;
    
    ConnPipeline* FindPipeline(uint64_t conn_id);

    // 发送给指定的连接ID
    // 低32位（本线程内的连接ID有效）
    bool SendIOBufByConnID(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data);
    
protected:
    inline uint32_t GetNextID() {
        if (current_conn_id_ == 0) ++current_conn_id_;
        return current_conn_id_++;
    }
    
    // 用于计数（可以使用IDMap）
    uint32_t current_conn_id_ {0};      // 0为无效值

    // TODO(@benqi): 使用boost::multi_index_container进行管理
    //   需要考虑性能，操作比较频繁
    size_t thread_id_ {0};  // 线程ID
    std::unordered_map<uint32_t, ConnPipeline*> pipelines_;
    // std::unordered_map<std::string, std::unordered_set<uint32_t>> service_pipelines_;
};


ThreadLocalConnManager& GetConnManagerByThreadLocal();

#endif
