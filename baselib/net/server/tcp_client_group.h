#ifndef NET_SERVER_TCP_CLIENT_GROUP_H_
#define NET_SERVER_TCP_CLIENT_GROUP_H_

#include "net/server/tcp_client.h"

//////////////////////////////////////////////////////////////////////////////
// 线程安全的保证
// 问题的提出:
//  某一类的服务器属于一个服务器组，比如线上IM的一组biz_server_buddy
//  共部署了8个实例
//  在logic_server_buddy会从8个实例里找出一个连接收发数据
//  这8个连接可能会断线或成功重连
// 引出下面的问题:
//  假如所属同一组的8个连接都落在不同的线程，则网络事件(连接建立或断开)的触发分属不同线程
//  从这个分组里随机找出可用连接的话，需要保护clients_
//  随之而来的是需要考虑每一步骤是否会需要同步，一旦考虑不好会引发各种各样问题。
// 采用一种比较简单的方式解决线程安全问题
//  在创建group时就给这个分组分配一个固定的线程，以后属于这个分组的所有网络事件都在这个线程里触发
//  其他线程给这个分组发送数据包时，通过runInEventBaseThread将数据包投递到该分组所属的线程
//  然后在这个分组线程里进行处理策略
//  如果所属分组线程已经没可用连接了，那也没办法，只好丢掉
//  即使是当前线上的单线程实现，分组里没可用连接，也只好丢掉。

// 功能：
//  1. 消息分发均匀分布，每个连接可分布在不同的线程
//  2. 可运行时新增和减少连接
// 实现思路
//  1. 每连接创建时通过IOThreadPoolExecutor分发到不同的线程里
//  2. tcp_client_group维护一个连接列表，此列表只新增不删除，一旦删除只是做个删除标记
//     因为从列表里移除，则除了要同步连接列表以外，还要同步tcp_client内容，锁粒度太大
//  tcp_client添加状态
class TcpClientGroup : public TcpServiceBase {
public:
    TcpClientGroup(const ServiceConfig& config, const IOThreadPoolExecutorPtr& io_group)
        : TcpServiceBase(config, io_group) {}
    
    virtual ~TcpClientGroup() = default;
    
    // Impl from TcpServiceBase
    ServiceModuleType GetModuleType() const override {
        return ServiceModuleType::TCP_CLIENT_GROUP;
    }

    void RegisterTcpClient(std::shared_ptr<TcpServiceBase> client);
    
    bool SendIOBuf(std::unique_ptr<folly::IOBuf> data, DispatchStrategy dispatch_strategy);
    bool SendIOBuf(std::unique_ptr<folly::IOBuf> data, const std::function<void()>& c);

    //////////////////////////////////////////////////////////////////////////
    bool Start() override {
        std::lock_guard<std::mutex> g(mutex_);
        // TODO(@benqi):
        // 检查状态，未删除的才能Start
        for (auto& c : clients_) {
            c->Start();
        }
        return true;
    }
    
    bool Pause() override {
        return true;
    }
    
    bool Stop() override;

protected:
    typedef std::vector<std::shared_ptr<TcpClient>> TcpClientList;
    
    TcpClientList GetOnlineClients() {
        TcpClientList clients;
        
        std::lock_guard<std::mutex> g(mutex_);
        for (auto& c : clients_) {
            clients.push_back(c);
        }
        
        return clients;
    }
    
    // 线程同步
    std::mutex mutex_;
    TcpClientList clients_;
};


//////////////////////////////////////////////////////////////////////////////
// TcpClient由TcpClientGroup创建
// 需要TcpClientGroupFactory
class TcpClientGroupFactory : public ServiceBaseFactory {
public:
    TcpClientGroupFactory(const std::string& name, const IOThreadPoolExecutorPtr& io_group)
        : ServiceBaseFactory(name),
          io_group_(io_group) {}
    
    
    virtual ~TcpClientGroupFactory() = default;
    
    static std::shared_ptr<TcpClientGroupFactory> GetDefaultFactory(const IOThreadPoolExecutorPtr& io_group);
    static std::shared_ptr<TcpClientGroupFactory> CreateFactory(const std::string& name,
                                                                const IOThreadPoolExecutorPtr& io_group);
    
    const std::string& GetType() const override;
    std::shared_ptr<ServiceBase> CreateInstance(const ServiceConfig& config) const override;
    
protected:
    IOThreadPoolExecutorPtr io_group_;
};

#endif
