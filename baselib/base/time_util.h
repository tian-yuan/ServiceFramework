#ifndef BASE_TIME_UTIL_H_
#define BASE_TIME_UTIL_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

// 获取毫秒时间
int64_t NowInMsecTime();

// 获取微秒时间
int64_t NowInUsecTime();

///////////////////////////////////////////////////////////////////////////////
class DiffTimeUtil {
public:
    DiffTimeUtil() {
        gettimeofday(&now_, nullptr);
    }
    
    void Reset() {
        gettimeofday(&now_, nullptr);
    }
    
    uint64_t GetDiffTime() const {
        struct timeval t;
        gettimeofday(&t, nullptr);
        return static_cast<uint64_t>((t.tv_sec - now_.tv_sec)*1000) + (t.tv_usec - now_.tv_usec)/1000;
    }
    
    struct timeval now_;
};

#endif
