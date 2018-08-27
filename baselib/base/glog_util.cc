#include "base/glog_util.h"

#include <iostream>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include <folly/Singleton.h>
#include <folly/FBString.h>

#include "base/configuration.h"

namespace {
folly::Singleton<LogInitializer> g_log_initializer;
}

namespace glog_util {
    
bool GLOG_trace = true;
bool GLOG_trace_call_chain = true;
bool GLOG_trace_fiber = true;
bool GLOG_trace_http = true;
bool GLOG_trace_message_handler = true;
bool GLOG_trace_cost = true;

}

std::shared_ptr<LogInitializer> GetLogInitializerSingleton() {
    return g_log_initializer.try_get();
}

void LogInitializer::Initialize(const char* argv0) {
    // google::InitGoogleLogging(argv0);
    // google::InstallFailureSignalHandler();
    google::SetStderrLogging(google::FATAL);
    FLAGS_stderrthreshold = google::INFO;
    // FLAGS_colorlogtostderr = true;
}

/*
 "log" =
 {
 "fatal_dest" = "./log/log_fatal_",
 "error_dest" = "./log/log_error_",
 "warning_dest" = "./log/log_warning_",
 "info_dest" = "./log/log_info_",
 "logbufsecs" = 0,
 "max_log_size" = 1,
 "stop_logging_if_full_disk" = "true",
 "enable_folly_debug" = "true",
 }
 */

#define SET_TRACE_FLAG(trace_flag) \
    if (config_data.find(#trace_flag) != config_data.end()) { \
        glog_util::GLOG_##trace_flag = config_data[#trace_flag].get<bool>(); \
    }

bool LogInitializer::SetConf(const std::string& conf_name, const nlohmann::json& config_data) {
    folly::fbstring program_name = google::ProgramInvocationShortName() ? google::ProgramInvocationShortName() : "";
    folly::fbstring logger_name;

    // 设置 google::FATAL 级别的日志存储路径和文件名前缀
    if (config_data.find("fatal_dest") != config_data.end()) {
        logger_name = config_data["fatal_dest"].get<std::string>() + program_name.toStdString() + "_fatal_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::INFO, logger_name.c_str());
    }
    
    // 设置 google::ERROR 级别的日志存储路径和文件名前缀
    if (config_data.find("error_dest") != config_data.end()) {
        logger_name = config_data["error_dest"].get<std::string>() + program_name.toStdString() + "_error_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::ERROR, logger_name.c_str());
    }
    
    // 设置 google::WARNING 级别的日志存储路径和文件名前缀
    if (config_data.find("warning_dest") != config_data.end()) {
        logger_name = config_data["warning_dest"].get<std::string>() + program_name.toStdString() + "_warning_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::WARNING, logger_name.c_str());
    }
    
    // 设置 google::INFO 级别的日志存储路径和文件名前缀
    if (config_data.find("info_dest") != config_data.end()) {
        logger_name = config_data["info_dest"].get<std::string>() + program_name.toStdString() + "_info_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::INFO, logger_name.c_str());
    }
    
    if (config_data.find("logbufsecs") != config_data.end()) {
        FLAGS_logbufsecs = static_cast<int32_t>(config_data["logbufsecs"].get<int>());
    } else {
        FLAGS_logbufsecs = 10;
    }
    
    if (config_data.find("logbuflevel") != config_data.end()) {
        FLAGS_logbuflevel = static_cast<int32_t>(config_data["logbuflevel"].get<int>());
    } else {
        FLAGS_logbuflevel = 4;
    }
    
    if (config_data.find("max_log_size") != config_data.end()) {
        FLAGS_max_log_size = static_cast<int32_t>(config_data["max_log_size"].get<int>());
    } else {
        FLAGS_max_log_size = 1000;
    }
    
    ///打印级别小于FLAGS_minloglevel的都不会输出到日志文件
    if (config_data.find("minloglevel") != config_data.end()) {
        FLAGS_minloglevel = static_cast<int32_t>(config_data["minloglevel"].get<int>());
    } else {
        FLAGS_minloglevel = 0;
    }
    
    if (config_data.find("stop_logging_if_full_disk") != config_data.end()) {
        FLAGS_stop_logging_if_full_disk = config_data["stop_logging_if_full_disk"].get<bool>();
    } else {
        FLAGS_stop_logging_if_full_disk = true;
    }
    
    if (config_data.find("enable_folly_debug") != config_data.end() && config_data["enable_folly_debug"].get<bool>()) {
        FLAGS_v = 20;
    }

    SET_TRACE_FLAG(trace);
    SET_TRACE_FLAG(trace_call_chain);
    SET_TRACE_FLAG(trace_fiber);
    SET_TRACE_FLAG(trace_http);
    SET_TRACE_FLAG(trace_message_handler);
    SET_TRACE_FLAG(trace_cost);

    return true;
}
