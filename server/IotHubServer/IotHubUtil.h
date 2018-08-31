#ifndef IOTHUB_SERVER_IOTHUB_SERVER_UTIL_H_
#define IOTHUB_SERVER_IOTHUB_SERVER_UTIL_H_

#include <string>

#include "base/config_util.h"

struct IotHubConfig: public Configurable
{
	virtual ~IotHubConfig() = default;

	bool SetConf(const std::string& conf_name, const nlohmann::json& conf) override;

	void PrintDebug() const
	{
		std::cout << "server_number: " << server_number << ", server_ips: " << server_ips << std::endl;
	}

	uint16_t server_number;
	std::string server_ips;
	int server_port { 443 };
};

typedef std::shared_ptr<IotHubConfig> IotHubConfigPtr;
IotHubConfigPtr GetIotHubConfigInstance();

uint16_t    GetServerNumber();

#endif

