#include "net/io_thread_pool_manager.h"
#include "net/server/http_client.h"
#include <iostream>

void http_callback(HttpClientReplyData& replyData) {
    std::cout << "receive result !" << std::endl;
    //std::cout << replyData.ToString() << std::endl;
}

int main() {
    std::shared_ptr<DefaultIOThreadPoolExecutor> threadPool = std::make_shared<DefaultIOThreadPoolExecutor>();
    folly::EventBase* evb = threadPool->GetEventBase(1);
    auto main_evb = folly::EventBaseManager::get()->getEventBase();
    main_evb->runAfterDelay([&]{
        HttpClientGet(main_evb, "http://www.baidu.com:90", http_callback);
    }, 1000);
    main_evb->loopForever();
    return 0;
}
