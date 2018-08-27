/*
 *  Copyright (c) 2015, mogujie.com
 *  All rights reserved.
 *
 *  Created by benqi@mogujie.com on 2015-12-20.
 *
 */

#include "base/base_main.h"

#include <signal.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <folly/Singleton.h>

#include "base/glog_util.h"
#include "base/config_manager.h"
// #include "base/util.h"

#include "util.h"
// #include <folly/experimental/symbolizer/SignalHandler.h>

// DEFINE_string(xml, "", "im_test_server port");
DEFINE_string(json, "", "json config file");
DEFINE_string(pid_path, "", "pid_file create path");

#ifndef IM_SERVER_VERSION
#define IM_SERVER_VERSION "4.0.1"
#endif

void FollyInitializer(int* argc, char*** argv, bool removeFlags = true) {
    // Install the handler now, to trap errors received during startup.
    // The callbacks, if any, can be installed later
    // folly::symbolizer::installFatalSignalHandler();
    
    auto programName = argc && argv && *argc > 0 ? (*argv)[0] : "unknown";
    google::InitGoogleLogging(programName);

    // google::ParseCommandLineFlags(argc, argv, removeFlags);

    // Don't use glog's DumpStackTraceAndExit; rely on our signal handler.
    google::InstallFailureFunction(abort);
    // google::InstallFailureSignalHandler();

    // Move from the registration phase to the "you can actually instantiate
    // things now" phase.
    folly::SingletonVault::singleton()->registrationComplete();
    
    // Actually install the callbacks into the handler.
    // folly::symbolizer::installFatalSignalCallbacks();
}

static std::string g_app_instance_name;

namespace base {

BaseMain::BaseMain() {
    config_manager_ = ConfigManager::GetInstance();
    
    // 注册logger配置
    config_manager_->Register("logger", GetLogInitializerSingleton().get());
}

bool BaseMain::DoMain(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::SetVersionString(IM_SERVER_VERSION);

    g_app_instance_name = google::ProgramInvocationShortName() ? google::ProgramInvocationShortName() : "";

    // folly
    FollyInitializer(&argc, &argv);
    
    // glog
    LogInitializer::Initialize(argv[0]);
    
    if (FLAGS_json.empty()) {
        config_file_ = google::ProgramInvocationShortName();
        config_file_ += ".json";
    } else {
        config_file_ = FLAGS_json;
    }
    
    pid_file_path_ = FLAGS_pid_path;

    //
    // google::VersionString();
    
    
//    google::ParseCommandLineFlags(&argc, &argv, true);
//    
//    // google::InitGoogleLogging(argv[0]);
//    google::InstallFailureSignalHandler();
    
//    google::SetLogDestination(google::INFO, "./log/");
//    google::InitGoogleLogging (argv[0]);
//    google::SetLogDestination(google::INFO, "./log/");
//    google::SetStderrLogging(google::FATAL);
//    FLAGS_stderrthreshold = google::INFO;
//    FLAGS_colorlogtostderr = true;
//    FLAGS_v = 20;
    
    // LOG(INFO) << "EventBase(): Starting loop.";
    
    LOG(INFO) << "DoMain - Ready DoMain()...";
    
    bool ret = false;
    do {
        if (!LoadConfig(config_file_)) {
            LOG(ERROR) << "DoMain - LoadConfig error: " << config_file_;
            break;
        }
        
        if (!Initialize()) {
            LOG(ERROR) << "DoMain - Initialize error!!!";
            break;
        }

        // writePid(google::ProgramInvocationShortName());
        
        Run();
        Destroy();
        ret = true;
    } while (0);
    
    exit(0);
    
    return ret;
}

bool BaseMain::LoadConfig(const std::string& config_path) {
    LOG(INFO) << "LoadConfig - " << config_path;
    
    bool rv = ConfigManager::GetInstance()->Initialize(ConfigManager::ConfigType::FILE, config_path);
    if (rv == false) {
        LOG(ERROR) << "LoadConfig - ConfigManager::Initialize() error!!!!";
    }
    return rv;
}

bool BaseMain::Initialize() {
    LOG(INFO) << "Initialize - Initialize...";
    // conn_pool_ = std::make_shared<IOThreadPoolConnManager>(GetIOThreadSize());

    return true;
}

bool BaseMain::Run() {
    LOG(INFO) << "Run - Run...";
  return true;
}

bool BaseMain::Destroy() {
    LOG(INFO) << "Destroy - Destroy...";
  return true;
}

}

const std::string& GetAppInstanceName() {
    return g_app_instance_name;
}


