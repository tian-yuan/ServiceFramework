#ifndef NET_SERVER_SERVICE_PLUGIN_H_
#define NET_SERVER_SERVICE_PLUGIN_H_

#include "base/plugin.h"
#include "base/config_info.h"

#include "net/io_thread_pool_manager.h"

// namespace base {

class ServicePlugin : public Plugin {
public:
    explicit ServicePlugin(std::shared_ptr<IOThreadPoolConnManager> conns) :
        conns_(conns) {
    }

    virtual ~ServicePlugin() = default;
    
    void Install(const Configuration& conf) override {
        service_info_.SetConf("", conf);
    }
    
protected:
    ServiceConfig service_info_;
    std::shared_ptr<IOThreadPoolConnManager> conns_;
};

// }

#endif
