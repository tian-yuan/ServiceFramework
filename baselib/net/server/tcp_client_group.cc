/*
 *  Copyright (c) 2015, mogujie.com
 *  All rights reserved.
 *
 *  Created by benqi@mogujie.com on 2016-01-04.
 *
 */

#include "net/server/tcp_client_group.h"

#include <glog/logging.h>

#include <folly/Random.h>
#include <folly/SocketAddress.h>
// #inclide <folly/io/async/EventBase.h>
// #include "net/io_thread_pool_manager.h"


//////////////////////////////////////////////////////////////////////////////

void TcpClientGroup::RegisterTcpClient(std::shared_ptr<TcpServiceBase> client) {
    // TODO(@benqi)
    //   检查是否是TcpClient
    //   检查类型名和服务名是否一样
    std::lock_guard<std::mutex> g(mutex_);
    clients_.push_back(std::static_pointer_cast<TcpClient>(client));
}

// TODO(@benqi)
//  存在线程安全的问题
//  如何保证？？？
// 考虑如下解决方案：
//  当创建分组时，保证这个分组里的所有连接都落在一个线程里
//  投递数据给分组时，先将网络包发送到分组所属的线程
//  然后在这个线程里按策略分发数据
// 简单实现
//  为一组主动发起的连接创建全分配到一个线程里
//  所有主动发起的连接共用这个线程
bool TcpClientGroup::SendIOBuf(std::unique_ptr<folly::IOBuf> data, DispatchStrategy dispatch_strategy) {
    auto onlines = GetOnlineClients();
    if (onlines.empty()) {
        LOG(ERROR) << "SendIOBuf - Not active tcp_client: ";
        return false;
    }
    switch (dispatch_strategy) {
        case DispatchStrategy::kBroadCast:
        {
            // 广播: 每个都发，保险起见，直接使用
            for (auto & c : onlines) {
                if (c->connected()) {
                    c->SendIOBufThreadSafe(std::move(data->clone()));
                }
            }
        }
            break;
        case DispatchStrategy::kDefault:
        case DispatchStrategy::kRandom:
        case DispatchStrategy::kRoundRobin:
        case DispatchStrategy::kConsistencyHash:
        default:
        {
            // TODO(@benqi)
            //  后续补上其他的分发策略
            //  当前只支持随机策略
            folly::ThreadLocalPRNG rng;
            uint32_t idx = folly::Random::rand32(static_cast<uint32_t>(onlines.size()));
            onlines[idx]->SendIOBufThreadSafe(std::move(data));
            
            LOG(INFO) << "SendIOBuf - dispatch_strategy, dispatch idx: " << idx;
        }
            break;
    }
    
    return true;
}

bool TcpClientGroup::SendIOBuf(std::unique_ptr<folly::IOBuf> data,
               const std::function<void()>& c) {
    auto onlines = GetOnlineClients();
    if (onlines.empty()) {
        LOG(ERROR) << "SendIOBuf - Not active tcp_client: ";
        return false;
    }

    folly::ThreadLocalPRNG rng;
    uint32_t idx = folly::Random::rand32(static_cast<uint32_t>(onlines.size()));
    onlines[idx]->SendIOBufThreadSafe(std::move(data), c);
    // LOG(INFO) << "SendIOBuf - dispatch_strategy, dispatch idx: " << idx;
    return true;

}

bool TcpClientGroup::Stop() {
    LOG(INFO) << "TcpClientGroup - Stop service: " << GetServiceConfig().ToString();
    
    std::lock_guard<std::mutex> g(mutex_);
    for (auto& c : clients_) {
        c->Stop();
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////
std::shared_ptr<TcpClientGroupFactory> TcpClientGroupFactory::GetDefaultFactory(const IOThreadPoolExecutorPtr& io_group) {
    static auto g_default_factory = std::make_shared<TcpClientGroupFactory>("tcp_client", io_group);
    
    return g_default_factory;
}

std::shared_ptr<TcpClientGroupFactory> TcpClientGroupFactory::CreateFactory(const std::string& name,
                                                                            const IOThreadPoolExecutorPtr& io_group) {
    return std::make_shared<TcpClientGroupFactory>(name, io_group);
}

const std::string& TcpClientGroupFactory::GetType() const {
    static std::string g_tcp_client_service_name("tcp_client");
    return g_tcp_client_service_name;
}

std::shared_ptr<ServiceBase> TcpClientGroupFactory::CreateInstance(const ServiceConfig& config) const {
    return std::make_shared<TcpClientGroup>(config, io_group_);
}



