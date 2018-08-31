#ifndef NET_SERVICE_MANAGER_H_
#define NET_SERVICE_MANAGER_H_

#include <unordered_map>
#include <folly/Singleton.h>

#include "base/configuration.h"
#include "base/config_util.h"

#include "net/thread_group_list_manager.h"
#include "net/base/service_base.h"

//////////////////////////////////////////////////////////////////////////////
// 集群路由管理器，管理网络分发等
// 整个基础库里最重要的类
// tcp_server为单个
// tcp_client属于一个组
// TODO(@benqi)
//  配置文件还是不太合理
//  对所有的tcp_client，应该配置在一个分组里
// 单件

class ServiceManager {
public :
    ~ServiceManager() = default;

    static std::shared_ptr<ServiceManager> GetInstance();
    
    void set_thread_groups(const std::shared_ptr<ThreadGroupListManager>& thread_groups);
    
    void SetupService(const ServicesConfig& config);

    void RegisterServiceFactory(const std::string& name, const std::string& type);

    bool Start();
    bool Pause();
    bool Stop();
    
    // 指定连接ID
    bool SendIOBuf(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data);
    
    // 通过TcpClientGroup分发
    bool SendIOBuf(const std::string& service_name,
                       std::unique_ptr<folly::IOBuf> data,
                       DispatchStrategy dispatch_strategy = DispatchStrategy::kDefault);
    
    bool SendIOBuf(const std::string& service_name,
                   std::unique_ptr<folly::IOBuf> data,
                   const std::function<void()>& c);

    folly::EventBase* GetEventBaseByThreadType(ThreadType thread_type) const {
        return thread_groups_->GetEventBaseByThreadType(thread_type);
    }
    
    folly::EventBase* GetEventBaseByThreadID(size_t idx) const {
        return thread_groups_->GetEventBaseByThreadID(idx);
    }

    std::shared_ptr<wangle::IOThreadPoolExecutor> GetIOThreadPoolExecutor(ThreadType thread_type) const {
        return thread_groups_->GetIOThreadPoolExecutor(thread_type);
    }
    
    ThreadGroupPtr GetThreadGroupByThreadType(ThreadType thread_type) const {
        return thread_groups_->GetThreadGroupByThreadType(thread_type);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    size_t thread_datas_size() const {
        return thread_groups_->thread_datas().size();
    }
    
    const ThreadData& thread_datas(size_t i) const {
        return thread_groups_->thread_datas()[i];
    }
    
protected:
    friend class folly::Singleton<ServiceManager>;
    ServiceManager() = default;

    bool RegisterServiceBase(std::shared_ptr<ServiceBase> service);
    std::shared_ptr<ServiceBase> Lookup(const std::string& service_name);

    // 如果是client，则ServiceBase为分组ClientGroup
    // 添加的时候要处理分组
    typedef std::unordered_map<std::string, std::shared_ptr<ServiceBase>> ServiceInstanceMap;
    
    ServiceInstanceMap services_;
    std::shared_ptr<ThreadGroupListManager> thread_groups_;

    std::vector<std::shared_ptr<ServiceConfig>> service_configs_;
};

#endif

