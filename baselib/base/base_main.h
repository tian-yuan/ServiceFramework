#ifndef BASE_BASE_MAIN_H_
#define BASE_BASE_MAIN_H_

#include <string>
#include <memory>

#include "base/config_manager.h"

namespace base {

class BaseMain {
public:
    BaseMain();
    
    virtual ~BaseMain() = default;
    
    virtual bool DoMain(int argc, char** argv);
    
protected:
    virtual bool LoadConfig(const std::string& config_path);
    virtual bool Initialize();
    virtual bool Run();
    virtual bool Destroy();
    
    
    std::string config_file_;
    ConfigManager* config_manager_{nullptr};

    std::string pid_file_path_;
};

}

const std::string& GetInstanceName();

#endif
