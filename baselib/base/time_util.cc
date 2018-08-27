#include "base/time_util.h"

#include <sys/time.h>


int64_t NowInMsecTime() {
    timeval tv;
    gettimeofday(&tv, 0);
    return int64_t(tv.tv_sec) * 1000 + tv.tv_usec/1000;
}

int64_t NowInUsecTime() {
    timeval tv;
    gettimeofday(&tv, 0);
    return int64_t(tv.tv_sec) * 1000000 + tv.tv_usec;
}
