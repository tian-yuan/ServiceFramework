#include "net/root.h"

#include "base/util.h"

#include "net/server/tcp_client.h"
#include "net/server/tcp_server.h"

#include "net/service_manager.h"
#include "net/service_router_manager.h"

#define SIGNAL_AND_QUIT

folly::EventBase* g_main_evb = nullptr;

void OnShutdownDaemon(folly::EventBase* main_evb) {
    if (main_evb) {
        main_evb->terminateLoopSoon();
    }
}

void SignalHandler(int signum) {
    switch(signum) {
        case SIGTERM:
        case SIGINT:
        case SIGHUP:
            OnShutdownDaemon(g_main_evb);
            break;
    }
}


Root::Root() {
    auto config_manager = ConfigManager::GetInstance();
    
    config_manager->Register("services", &services_config_);
    config_manager->Register("system", &system_config_);
    config_manager->Register("thread_group", &thread_group_options_);
}

size_t Root::GetIOThreadPoolSize() {
    if (io_thread_pool_size_ == 0) {
        io_thread_pool_size_ = ::GetIOThreadSize();
    }
    return io_thread_pool_size_;
}

bool Root::LoadConfig(const std::string& config_path) {
    bool rv = BaseMain::LoadConfig(config_path);
    if (rv) {
        io_thread_pool_size_ = system_config_.io_thread_pool_size;
    }
    return rv;
}

bool Root::Initialize() {
    BaseMain::Initialize();

    // 初始化程序
    auto thread_groups = std::make_shared<ThreadGroupListManager>(thread_group_options_);
    auto service_manager = ServiceManager::GetInstance();
    service_manager->set_thread_groups(thread_groups);
    service_manager->SetupService(services_config_);

    for (auto it=services_.begin(); it!=services_.end(); ++it) {
        service_manager->RegisterServiceFactory(it->first, it->second);
    }
    
    FiberDataManager::InitFiberStats();

    return true;
}

void Root::DoTimer(folly::EventBase* evb, Root* root, uint32_t intervals) {
    evb->runAfterDelay([evb, root] {
        root->OnTimer(0);
        Root::DoTimer(evb, root);
    }, intervals);
}

bool Root::Run() {
    BaseMain::Run();
    
    auto main_eb = folly::EventBaseManager::get()->getEventBase();
    g_main_evb = main_eb;

    auto service_manager = ServiceManager::GetInstance();

    // 启动成功
    try {
        service_manager->Start();
    } catch (std::exception& e) {
        LOG(ERROR) << "Root::Run - catch exception: " << e.what();
        return -1;
    } catch (...) {
        LOG(ERROR) << "Root::Run - catch  a invalid exception";
        return -1;
    }

    writePid(pid_file_path_, google::ProgramInvocationShortName());

#ifdef SIGNAL_AND_QUIT
    signal(SIGTERM, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGHUP, SignalHandler);
#endif

    main_eb->runAfterDelay([main_eb, this] {
        Root::DoTimer(main_eb, this);
    }, 1000);
    
    main_eb->loopForever();
    service_manager->Stop();
    return true;
}


bool Root::Destroy() {
    return BaseMain::Destroy();
}
