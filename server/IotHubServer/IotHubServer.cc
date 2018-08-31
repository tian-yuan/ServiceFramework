//
// Created by tianyuan on 2018/8/25.
//

#include <baselib/net/base/service_factory_manager.h>
#include "IotHubServer/IotHubServer.h"
#include "IotHubServer/IotHubUtil.h"

#include "base/snowflake_id_util.h"


IotHubServer::IotHubServer()
{
    ConfigManager::GetInstance()->Register("iot_hub_server", GetIotHubConfigInstance().get());
    RegisterService("iot_hub_server", "tcp_server");
}

bool IotHubServer::Initialize()
{
    Root::Initialize();

    InitSnowflakeWorkerID(static_cast<uint16_t>(GetServerNumber()));

    LOG(INFO)<< "IotHubServer - Initialize";

    return true;
}

void IotHubServer::OnTimer(int timer_id)
{
    LOG(INFO) << "IotHubServer::OnTimer: " << 0;
}

///////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    LOG(INFO) << "Iot hub server start.";
    IotHubServer do_main;
    do_main.DoMain(argc, argv);

    return 0;
}
