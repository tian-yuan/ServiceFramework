//
// Created by tianyuan on 2018/8/25.
//

#ifndef SERVICEFRAMEWORK_IOTHUBSERVER_H
#define SERVICEFRAMEWORK_IOTHUBSERVER_H

#include "net/root.h"

class IotHubServer : public Root {
public:
    IotHubServer();

    virtual ~IotHubServer() = default;

    bool Initialize() override;

    void OnTimer(int timer_id) override;
};


#endif //SERVICEFRAMEWORK_IOTHUBSERVER_H
