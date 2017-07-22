#ifndef NET_SERVER_HTTP_SERVER_H_
#define NET_SERVER_HTTP_SERVER_H_

#include "net/base/service_base.h"
#include "net/server/http_server_lib.h"

// ClusterManager
class HttpServer : public ServiceBase {
public:
    HttpServer(const ServiceConfig& config, const std::shared_ptr<wangle::IOThreadPoolExecutor>& io_group)
        : ServiceBase(config),
          io_group_(io_group) {}
    
    virtual ~HttpServer() = default;
    
    // Impl from TcpServiceBase
    ServiceModuleType GetModuleType() const override {
        return ServiceModuleType::HTTP_SERVER;
    }

    bool Start() override;
    bool Pause() override;
    bool Stop() override;
    
private:
    std::shared_ptr<wangle::IOThreadPoolExecutor> io_group_;
    std::unique_ptr<proxygen::HTTPServerLib> server_;
};

//////////////////////////////////////////////////////////////////////////////
class HttpServerFactory : public ServiceBaseFactory {
public:
    HttpServerFactory(const std::string& name,
                      const std::shared_ptr<wangle::IOThreadPoolExecutor>& io_group)
        : ServiceBaseFactory(name),
          io_group_(io_group) {}
    
    virtual ~HttpServerFactory() = default;
    
    static std::shared_ptr<HttpServerFactory> GetDefaultFactory(const std::shared_ptr<wangle::IOThreadPoolExecutor>& io_group);
    static std::shared_ptr<HttpServerFactory> CreateFactory(const std::string& name,
                                                            const std::shared_ptr<wangle::IOThreadPoolExecutor>& io_group);
    
    
    const std::string& GetType() const override;
    std::shared_ptr<ServiceBase> CreateInstance(const ServiceConfig& config) const override;
    
protected:
    std::shared_ptr<wangle::IOThreadPoolExecutor> io_group_;
};


#endif
