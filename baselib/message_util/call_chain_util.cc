#include "call_chain_util.h"

#include <folly/SingletonThreadLocal.h>
#include <folly/FormatArg.h>

#include "base/time_util.h"

std::string CallChainData::ToString() const {
    uint64_t now = NowInUsecTime();
    uint64_t chain_timetick_diff = (now>chain_timetick) ? now-chain_timetick : 0;
    uint64_t timetick_diff = (now>timetick) ? now-timetick : 0;
    
//    CallChainTimeUtil diff(chain_timetick, timetick);
//    if (chain_timetick != 0) chain_timetick_diff = diff.GetDiffChainTime();
//    if (timetick != 0) timetick_diff = diff.GetDiffTime();
    return folly::to<std::string>("{{call_chain_data - call_chain_id:{}, chain_timetick:{}, timetick:{}, now:{}, chain_cost:{}, cost:{}}}",
                          call_chain_id,
                          chain_timetick,
                          timetick,
                          now,
                          chain_timetick_diff/1000,
                          timetick_diff/1000);
}

CallChainData& GetCallChainDataByThreadLocal() {
    static folly::SingletonThreadLocal<CallChainData> g_cache([]() {
        return new CallChainData();
    });
    
    return g_cache.get();
}

