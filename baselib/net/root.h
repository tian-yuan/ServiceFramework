#ifndef NET_ROOT_H_
#define NET_ROOT_H_

#include <list>

#include "base/base_main.h"
#include "base/config_util.h"
#include "base/configuration.h"

#include "net/base/service_base.h"
#include "net/thread_group_list_manager.h"
#include "io_thread_pool_manager.h"

// 两种情况
// 刚启动时加载配置文件
class Root : public base::BaseMain {
public:
    Root();
    
    virtual ~Root() = default;

    static void DoTimer(folly::EventBase* evb, Root* root, uint32_t intervals = 1000);

    size_t GetIOThreadPoolSize();
    
    virtual void OnTimer(int timer_id) {}
    
protected:
    void set_io_thread_pool_size(size_t v) {
        io_thread_pool_size_ = v;
    }
    
    void RegisterService(const std::string& name, const std::string& type) {
        services_.push_back(std::make_pair(name, type));
    }
    
    bool LoadConfig(const std::string& config_path) override;
    virtual bool Initialize();
    bool Run() override;
    bool Destroy() override;

    // 为0默认多线程模式
    size_t io_thread_pool_size_{0};
    
    ServicesConfig services_config_;
    SystemConfig system_config_;
    
    ThreadGroupListOption thread_group_options_;
    
    typedef std::list<std::pair<std::string, std::string>> ServiceInstanceList;
    ServiceInstanceList services_;
};

#endif


