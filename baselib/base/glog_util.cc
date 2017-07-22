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
    v = config_data.get_ptr(#trace_flag); \
    if (v && v->isBool()) { \
        glog_util::GLOG_##trace_flag = v->asBool(); \
    }

bool LogInitializer::SetConf(const std::string& conf_name, const Configuration& conf) {
    folly::fbstring program_name = google::ProgramInvocationShortName() ? google::ProgramInvocationShortName() : "";
    folly::fbstring logger_name;
    
    folly::dynamic config_data = conf.GetDynamicConf();
    if (!config_data.isObject()) {
        return false;
    }
    // 设置 google::FATAL 级别的日志存储路径和文件名前缀
    auto fatal_dest = config_data.get_ptr("fatal_dest");
    if (fatal_dest && fatal_dest->isString()) {
        logger_name = fatal_dest->asString() + program_name.toStdString() + "_fatal_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::INFO, logger_name.c_str());
    }
    
    // 设置 google::ERROR 级别的日志存储路径和文件名前缀
    auto error_dest = config_data.get_ptr("error_dest");
    if (error_dest && error_dest->isString()) {
        logger_name = error_dest->asString() + program_name.toStdString() + "_error_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::ERROR, logger_name.c_str());
    }
    
    // 设置 google::WARNING 级别的日志存储路径和文件名前缀
    auto warning_dest = config_data.get_ptr("warning_dest");
    if (warning_dest && warning_dest->isString()) {
        logger_name = warning_dest->asString() + program_name.toStdString() + "_warning_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::WARNING, logger_name.c_str());
    }
    
    // 设置 google::INFO 级别的日志存储路径和文件名前缀
    auto info_dest = config_data.get_ptr("info_dest");
    if (info_dest && info_dest->isString()) {
        logger_name = info_dest->asString() + program_name.toStdString() + "_info_";
        // std::cout << logger_name << std::endl;
        google::SetLogDestination(google::INFO, logger_name.c_str());
    }
    
    auto logbufsecs = config_data.get_ptr("logbufsecs");
    if (logbufsecs && logbufsecs->isInt()) {
        FLAGS_logbufsecs = static_cast<int32_t>(logbufsecs->asInt());
    } else {
        FLAGS_logbufsecs = 10;
    }
    
    auto logbuflevel = config_data.get_ptr("logbuflevel");
    if (logbuflevel && logbuflevel->isInt()) {
        FLAGS_logbuflevel = static_cast<int32_t>(logbuflevel->asInt());
    } else {
        FLAGS_logbuflevel = 4;
    }
    
    auto max_log_size = config_data.get_ptr("max_log_size");
    if (max_log_size && max_log_size->isInt()) {
        FLAGS_max_log_size = static_cast<int32_t>(max_log_size->asInt());
    } else {
        FLAGS_max_log_size = 1000;
    }
    
    ///打印级别小于FLAGS_minloglevel的都不会输出到日志文件
    auto minloglevel = config_data.get_ptr("minloglevel");
    if (minloglevel && minloglevel->isInt()) {
        FLAGS_minloglevel = static_cast<int32_t>(minloglevel->asInt());
    } else {
        FLAGS_minloglevel = 0;
    }
    
    auto stop_logging_if_full_disk = config_data.get_ptr("stop_logging_if_full_disk");
    if (stop_logging_if_full_disk && stop_logging_if_full_disk->isBool()) {
        FLAGS_stop_logging_if_full_disk = stop_logging_if_full_disk->asBool();
    } else {
        FLAGS_stop_logging_if_full_disk = true;
    }
    
    auto v = config_data.get_ptr("enable_folly_debug");
    if (v && v->isBool() && v->asBool()) {
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
