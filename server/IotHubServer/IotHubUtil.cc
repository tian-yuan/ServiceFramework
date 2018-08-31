#include "IotHubServer/IotHubUtil.h"

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <folly/Singleton.h>

#include "net/root.h"

folly::Singleton<IotHubConfig> g_iothub_config;

IotHubConfigPtr GetIotHubConfigInstance()
{
	return g_iothub_config.try_get();
}

uint64_t nowMicro()
{
	timeval tv;
	gettimeofday(&tv, 0);
	return static_cast<uint64_t>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

bool IotHubConfig::SetConf(const std::string& conf_name, const nlohmann::json& conf)
{
	folly::dynamic v = nullptr;

	if (conf.find("server_number") != conf.end()) {
		server_number = conf["server_number"].get<int>();
	}

	if (conf.find("server_ips") != conf.end()) {
        server_ips = conf["server_ips"].get<std::string>();
	}

	if (conf.find("server_port") != conf.end()) {
        server_port = conf["server_port"].get<int>();
	}

	LOG(INFO) << "server ips : " << server_ips << ", server_port : " << server_port << ", server_number : " << server_number;
	return true;
}

uint16_t GetServerNumber()
{
	return GetIotHubConfigInstance()->server_number;
}

