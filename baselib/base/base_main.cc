#include "base/base_main.h"

#include "base/glog_util.h"

#include "util.h"

DEFINE_string(json, "", "json config file");
DEFINE_string(pid_path, "", "pid_file create path");

#ifndef SERVER_VERSION
#define SERVER_VERSION "4.0.1"
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
    LOG(INFO) << "base main construct.";
    config_manager_ = ConfigManager::GetInstance();
    
    // 注册logger配置
    config_manager_->Register("logger", GetLogInitializerSingleton().get());
}

bool BaseMain::DoMain(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);

    LOG(INFO) << "base main .";
    google::ParseCommandLineFlags(&argc, &argv, true);
    LOG(INFO) << "parse command line flags.";
    google::SetVersionString(SERVER_VERSION);

    g_app_instance_name = google::ProgramInvocationShortName() ? google::ProgramInvocationShortName() : "";
    LOG(INFO) << "app instance name : " << g_app_instance_name;

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

        Run();
        Destroy();
        ret = true;
    } while (0);
    
    exit(0);
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
//    conn_pool_ = std::make_shared<IOThreadPoolConnManager>(GetIOThreadSize());

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


