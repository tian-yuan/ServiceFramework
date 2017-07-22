#ifndef MESSAGE_UTIL_CALL_CHAIN_UTIL_H_
#define MESSAGE_UTIL_CALL_CHAIN_UTIL_H_

#include <stdint.h>
#include <sys/time.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////
class CallChainTimeUtil {
public:
    CallChainTimeUtil() {
        struct timeval t;
        gettimeofday(&t, nullptr);
        now_ = CalcTimeVal(t);
        chain_t_ = now_;
    }
    
    explicit CallChainTimeUtil(uint64_t b)
        : chain_t_(b) {
        struct timeval t;
        gettimeofday(&t, nullptr);
        now_ = CalcTimeVal(t);
    }
    
    explicit CallChainTimeUtil(uint64_t b, uint64_t now)
        : chain_t_(b), now_(now) {
        
    }

    void SetChainTime(uint64_t t) {
        chain_t_ = t;
    }
    
    void Reset() {
        struct timeval t;
        gettimeofday(&t, nullptr);
        now_ = CalcTimeVal(t);
    }
    
    uint64_t GetDiffTime() const {
        struct timeval t;
        gettimeofday(&t, nullptr);
        uint64_t n = CalcTimeVal(t);
        if (now_ > n) {
            return 0;
        } else {
            return n-now_;
        }
    }
    
    uint64_t GetDiffChainTime() const {
        struct timeval t;
        gettimeofday(&t, nullptr);
        uint64_t n = CalcTimeVal(t);
        if (chain_t_ > n) {
            return 0;
        } else {
            return n-chain_t_;
        }
    }
    
    void GetChainDiffTimes(uint64_t& t, uint64_t& chain_t) const {
        t = GetDiffTime();
        chain_t = GetDiffChainTime();
    }
    
private:
    inline uint64_t CalcTimeVal(const timeval& t) const {
        return static_cast<uint64_t>(t.tv_sec)*1000000 + t.tv_usec;
    }
    
    uint64_t chain_t_;            // 全局调用链开始时间
    uint64_t now_;    // 对象创建时间
};

// 调用链信息
struct CallChainData {
    std::string ToString() const;
    // void CopyFrom(const impdu::CImPduHeader& header);
    
    uint64_t chain_timetick{0};         // 全局链路调用时间
    uint64_t timetick{0};               // 当前创建时间

//#ifdef NEW_EXTEND_HEADER
    uint64_t call_chain_id;          // 全局调用链ID
//#else
//    std::string call_chain_id;          // 全局调用链ID
//#endif
    // uint16_t mid{0};
    // uint16_t cid{0};
};

///////////////////////////////////////////////////////////////////////////////////////
CallChainData& GetCallChainDataByThreadLocal();

#endif
