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


// 原始算法
/*************************************************************************************
// ** Copyright 2010-2012 Twitter, Inc. * /
package com.twitter.service.snowflake

import com.twitter.ostrich.stats.Stats
import com.twitter.service.snowflake.gen._
import java.util.Random
import com.twitter.logging.Logger

/ **
* An object that generates IDs.
* This is broken into a separate class in case
* we ever want to support multiple worker threads
* per process
* /
class IdWorker(val workerId: Long, val datacenterId: Long, private val reporter: Reporter, var sequence: Long = 0L)
extends Snowflake.Iface {
    private[this] def genCounter(agent: String) = {
        Stats.incr("ids_generated")
        Stats.incr("ids_generated_%s".format(agent))
    }
    private[this] val exceptionCounter = Stats.getCounter("exceptions")
    private[this] val log = Logger.get
    private[this] val rand = new Random
    
    val twepoch = 1288834974657L
    
    private[this] val workerIdBits = 5L
    private[this] val datacenterIdBits = 5L
    private[this] val maxWorkerId = -1L ^ (-1L << workerIdBits)
    private[this] val maxDatacenterId = -1L ^ (-1L << datacenterIdBits)
    private[this] val sequenceBits = 12L
    
    private[this] val workerIdShift = sequenceBits
    private[this] val datacenterIdShift = sequenceBits + workerIdBits
    private[this] val timestampLeftShift = sequenceBits + workerIdBits + datacenterIdBits
    private[this] val sequenceMask = -1L ^ (-1L << sequenceBits)
    
    private[this] var lastTimestamp = -1L
    
    // sanity check for workerId
    if (workerId > maxWorkerId || workerId < 0) {
        exceptionCounter.incr(1)
        throw new IllegalArgumentException("worker Id can't be greater than %d or less than 0".format(maxWorkerId))
    }
    
    if (datacenterId > maxDatacenterId || datacenterId < 0) {
        exceptionCounter.incr(1)
        throw new IllegalArgumentException("datacenter Id can't be greater than %d or less than 0".format(maxDatacenterId))
    }
    
    log.info("worker starting. timestamp left shift %d, datacenter id bits %d, worker id bits %d, sequence bits %d, workerid %d",
             timestampLeftShift, datacenterIdBits, workerIdBits, sequenceBits, workerId)
    
    def get_id(useragent: String): Long = {
        if (!validUseragent(useragent)) {
            exceptionCounter.incr(1)
            throw new InvalidUserAgentError
        }
        
        val id = nextId()
        genCounter(useragent)
        
        reporter.report(new AuditLogEntry(id, useragent, rand.nextLong))
        id
    }
    
    def get_worker_id(): Long = workerId
    def get_datacenter_id(): Long = datacenterId
    def get_timestamp() = System.currentTimeMillis
    
    protected[snowflake] def nextId(): Long = synchronized {
        var timestamp = timeGen()
        
        if (timestamp < lastTimestamp) {
            exceptionCounter.incr(1)
            log.error("clock is moving backwards.  Rejecting requests until %d.", lastTimestamp);
            throw new InvalidSystemClock("Clock moved backwards.  Refusing to generate id for %d milliseconds".format(
                                                                                                                      lastTimestamp - timestamp))
        }
        
        if (lastTimestamp == timestamp) {
            sequence = (sequence + 1) & sequenceMask
            if (sequence == 0) {
                timestamp = tilNextMillis(lastTimestamp)
            }
        } else {
            sequence = 0
        }
        
        lastTimestamp = timestamp
        ((timestamp - twepoch) << timestampLeftShift) |
        (datacenterId << datacenterIdShift) |
        (workerId << workerIdShift) |
        sequence
    }
    
    protected def tilNextMillis(lastTimestamp: Long): Long = {
        var timestamp = timeGen()
        while (timestamp <= lastTimestamp) {
            timestamp = timeGen()
        }
        timestamp
    }
    
    protected def timeGen(): Long = System.currentTimeMillis()
    
    val AgentParser = """([a-zA-Z][a-zA-Z\-0-9]*)""".r
    
    def validUseragent(useragent: String): Boolean = useragent match {
    case AgentParser(_) => true
    case _ => false
    }
}
 
 */

#include "base/snowflake_id_util.h"

#include <sys/time.h>
#include <folly/SingletonThreadLocal.h>

namespace {
    
uint16_t g_worker_id = 1;
uint16_t g_data_center_id = 1;
// uint32_t g_current_ms_id = 0;
    
}

/* 时间很紧，先能用
 val twepoch = 1288834974657L
 
 private[this] val workerIdBits = 5L
 private[this] val datacenterIdBits = 5L
 private[this] val maxWorkerId = -1L ^ (-1L << workerIdBits)
 private[this] val maxDatacenterId = -1L ^ (-1L << datacenterIdBits)
 private[this] val sequenceBits = 12L
 
 private[this] val workerIdShift = sequenceBits
 private[this] val datacenterIdShift = sequenceBits + workerIdBits
 private[this] val timestampLeftShift = sequenceBits + workerIdBits + datacenterIdBits
 private[this] val sequenceMask = -1L ^ (-1L << sequenceBits)
 
 private[this] var lastTimestamp = -1L
 */

// 非线程安全
// TODO(@benqi):
// 1. java因为无uint64_t，故最高位未使用
//    C++的实现可以使用最高位
// 2. gettimeofday可以通过其它更高效的方式实现
// 3. 每ms最大生成2^12个, 如果对更高要求的ID生成器，可以由具体业务适当调整各指标所占位数
class IdWorker {
public:
    IdWorker(uint16_t worker_id, uint16_t data_center_id)
        : worker_id_(worker_id),
          data_center_id_(data_center_id) {}
    
    uint64_t GetNextID() {
        uint64_t timestamp = nowInMsec();
        
        // 在当前秒内内
        if (last_timestamp_ == timestamp) {
            sequence_number_ = (sequence_number_ + 1) & 0xFFF;
            if (sequence_number_ == 0) {
                timestamp = tilNextMillis(sequence_number_);
            }
        } else {
            sequence_number_ = 0;
        }
        
        
        last_timestamp_ = timestamp;
        return ((timestamp & 0x1FFFFFF) << 22 |
                (data_center_id_ & 0x1F) << 17 |
                (worker_id_& 0x1F) << 12 |
                (sequence_number_ & 0xFFF));
        
    }
    
private:
    inline uint64_t nowInMsec() {
        timeval tv;
        gettimeofday(&tv, 0);
        return uint64_t(tv.tv_sec) * 1000 + tv.tv_usec/1000;
    }
    
    uint64_t tilNextMillis(uint64_t last_timestamp) {
        uint64_t timestamp = nowInMsec();
        while (timestamp <= last_timestamp) {
            timestamp = nowInMsec();
        }
        return timestamp;
    }
    
    uint16_t worker_id_{0};
    uint16_t data_center_id_{0};
    uint64_t last_timestamp_;
    uint32_t sequence_number_{0};
};

// TODO(@benqi, 使用run_once)
void    InitSnowflakeWorkerID(uint16_t worker_id, uint16_t data_center_id) {
    g_worker_id = worker_id;
    g_data_center_id = data_center_id;
}

IdWorker& GetIdWorkerByThreadLocal() {
    static folly::SingletonThreadLocal<IdWorker> g_cache([]() {
        static std::atomic<uint16_t> g_thread_id;
        // 注意：如果按照单线程用法，会有问题
        // g_thread_id++;
        // g_thread_id.load()可能不是期望的值，如果其它线程也g_thread_id++
        // return new IdWorker(g_worker_id, g_thread_id.load());
        
        uint16_t next_id = g_thread_id++;
        return new IdWorker(g_worker_id, next_id);
    });
    return g_cache.get();
}

uint64_t GetNextIDBySnowflake() {
    return GetIdWorkerByThreadLocal().GetNextID();
}

uint64_t GetNextIDBySnowflakeUnSafe() {
    static IdWorker g_id_worker(g_worker_id, g_data_center_id);
    return g_id_worker.GetNextID();
}

