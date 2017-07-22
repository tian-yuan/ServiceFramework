//
// Created by tianyuan on 2017/7/21.
//

#include "base/config/rendezvous_hash.h"

#include <stdio.h>


int main() {
    int dispatch_id = -1;
    ///< 获取mwp的地址信息，从连接成功的客户端列表中获取
    std::vector<std::pair<std::string, double>> mwp_service_ip;

    mwp_service_ip.push_back(std::make_pair("192.168.1.100", 1.0));
    mwp_service_ip.push_back(std::make_pair("192.168.1.101", 1.0));

    dispatch_id = RendezvousHash(mwp_service_ip.begin(), mwp_service_ip.end()).get(1);

    printf("dispatch id : %d\n", dispatch_id);

    dispatch_id = RendezvousHash(mwp_service_ip.begin(), mwp_service_ip.end()).get(2);

    printf("dispatch id : %d\n", dispatch_id);
    return 0;
}