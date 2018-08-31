#ifndef NET_SERVICE_ROUTER_MANAGER_H_
#define NET_SERVICE_ROUTER_MANAGER_H_

#include <unordered_map>

#include "base/configuration.h"

#include "net/io_thread_pool_manager.h"
#include "net/base/service_factory_manager.h"

//////////////////////////////////////////////////////////////////////////////
//class TcpConnEventCallback {
//public:
//    virtual ~TcpConnEventCallback() = default;
//    
//    // EventBase线程里执行
//    virtual uint64_t OnNewConnection(ConnPipeline* pipeline) = 0;
//    
//    // EventBase线程里执行
//    virtual bool OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline) = 0;
//};

//////////////////////////////////////////////////////////////////////////////
// 集群路由管理器，管理网络分发等
// 整个基础库里最重要的类
/**
 <?xml version="1.0" encoding="UTF-8"?>
 <configure>
  <services>
   <tcp_server>
    <service_name>access_server</service_name>
    <service_type>tcp_server</service_type>
    <hosts>0.0.0.0</hosts>
    <port>12345</port>
    <max_conn_cnt>200000</max_conn_cnt>
   </tcp_server>
   <tcp_client>
    <service_name>status_client</service_name>
    <service_type>tcp_client</service_type>
    <hosts>127.0.0.1</hosts>
    <port>22345</port>
    <max_conn_cnt>200000</max_conn_cnt>
   </tcp_client>
  </services>
 </configure>
 */
class ServiceRouterManager {
public :
    explicit ServiceRouterManager(std::shared_ptr<IOThreadPoolConnManager> conn_pool);
    ~ServiceRouterManager() = default;

    std::shared_ptr<IOThreadPoolConnManager> GetConnPool() {
        return conn_pool_;
    }
    
    void RegisterServiceFactory(std::shared_ptr<ServiceBaseFactory> factory) {
        service_factory_manager_.RegisterServiceFactory(factory);
    }

    bool Initialize(const ServicesConfig& conf);

    // TcpConnEventCallback implementation
    uint64_t OnNewConnection(ConnPipeline* pipeline);
    bool OnConnectionClosed(uint64_t conn_id, ConnPipeline* pipeline);

    void OnServerRegister(ConnPipeline* pipeline, const std::string& server_name, uint32_t server_number);
    
    bool Start();
    bool Pause();
    bool Stop();

    // 指定连接ID
    bool DispatchIOBuf(uint64_t conn_id, std::unique_ptr<folly::IOBuf> data);
    
    // 通过TcpClientGroup分发
    bool DispatchIOBuf(const std::string& service_name,
                       std::unique_ptr<folly::IOBuf> data,
                       DispatchStrategy dispatch_strategy = DispatchStrategy::kDefault);
    
    bool DispatchIOBuf(const std::string& server_name, uint32_t server_number, std::unique_ptr<folly::IOBuf> data);
    
protected:
    bool RegisterServiceBase(std::shared_ptr<ServiceBase> service);
    std::shared_ptr<ServiceBase> Lookup(const std::string& service_name);
    
private:
    // 如果是client，则ServiceBase为分组ClientGroup
    // 添加的时候要处理分组
    typedef std::unordered_map<std::string, std::shared_ptr<ServiceBase>> ServiceInstanceMap;
    
    ServiceInstanceMap services_;
    std::shared_ptr<IOThreadPoolConnManager> conn_pool_;
    
    ServiceFactoryManager service_factory_manager_;
    
    std::vector<std::shared_ptr<ServiceConfig>> service_configs_;
};

#endif

