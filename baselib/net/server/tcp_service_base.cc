#include "net/server/tcp_service_base.h"

#include <glog/logging.h>

#include "net/server/tcp_server.h"
#include "net/server/tcp_client.h"
#include "net/server/http_server.h"

std::shared_ptr<ServiceBaseFactory> CreateTcpServiceFactory(const std::string& name,
                                                            const std::string& type,
                                                            const IOThreadPoolExecutorPtr& io_group) {
    std::shared_ptr<ServiceBaseFactory> factory;
    
    if (type == "tcp_server") {
        factory = TcpServerFactory::CreateFactory(name, io_group);
    } else if (type == "tcp_client") {
        factory = TcpClientFactory::CreateFactory(name, io_group);
    } else if (type == "http_server") {
        factory = HttpServerFactory::CreateFactory(name, io_group);
    } else {
        LOG(ERROR) << "CreateTcpServiceFactory - UNKNOWN service_type: name = " << name << ", type = " << type;
    }

    return factory;
}
