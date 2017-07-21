/*
 *  Copyright (c) 2016, mogujie.com
 *  All rights reserved.
 *
 *  Created by benqi@mogujie.com on 2016-03-17.
 *
 *  Twitter的分布式自增ID算法Snowflake的C++移植
 *  参考:
 *    https://github.com/twitter/snowflake
 *    http://itindex.net/detail/53406-twitter-id-%E7%AE%97%E6%B3%95
 *
 */

#ifndef BASE_SNOWFLAKE_ID_UTIL_H_
#define BASE_SNOWFLAKE_ID_UTIL_H_

#include <stdint.h>

/*
 结构为：
 
   0---0000000000 0000000000 0000000000 0000000000 0 --- 00000 ---00000 ---0000000000 00
   在上面的字符串中，第一位为未使用（实际上也可作为long的符号位），接下来的41位为毫秒级时间，然后5位datacenter标识位，
   5位机器ID（并不算标识符，实际是为线程标识），然后12位该毫秒内的当前毫秒内的计数，加起来刚好64位，为一个Long型。
 
   这样的好处是，整体上按照时间自增排序，并且整个分布式系统内不会产生ID碰撞（由datacenter和机器ID作区分），
   并且效率较高，经测试，snowflake每秒能够产生26万ID左右，完全满足需要。
 */

// 初始化WorkerID
void    InitSnowflakeWorkerID(uint16_t worker_id, uint16_t data_center_id=1);

// 线程安全，内部通过ThreadLocal实现
uint64_t GetNextIDBySnowflake();

// 非线程安全
uint64_t GetNextIDBySnowflakeUnSafe();

#endif

