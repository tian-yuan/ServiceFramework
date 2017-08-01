#ifndef BASE_GLOG_UTIL_H_
#define BASE_GLOG_UTIL_H_

#include <glog/logging.h>

#include "base/configurable.h"

// 配置
struct LogInitializer : public Configurable {
    virtual ~LogInitializer() = default;
    
    static void Initialize(const char* argv0);
    
    // 配置改变
    bool SetConf(const std::string& conf_name, const Configuration& conf) override;
    
private:
};

std::shared_ptr<LogInitializer> GetLogInitializerSingleton();

namespace glog_util {
    
extern bool GLOG_trace;
extern bool GLOG_trace_call_chain;
extern bool GLOG_trace_fiber;
extern bool GLOG_trace_http;
extern bool GLOG_trace_message_handler;
extern bool GLOG_trace_cost;
    
}

#define TRACE()                 LOG_IF(INFO, glog_util::GLOG_trace)
#define TRACE_CALL_CHAIN()      LOG_IF(INFO, glog_util::GLOG_trace_call_chain)
#define TRACE_FIBER()           LOG_IF(INFO, glog_util::GLOG_trace_fiber)
#define TRACE_HTTP()            LOG_IF(INFO, glog_util::GLOG_trace_http)
#define TRACE_MESSAGE_HANDLER() LOG_IF(INFO, glog_util::GLOG_trace_message_handler)
#define TRACE_COST()            LOG_IF(INFO, glog_util::GLOG_trace_cost)

#endif
