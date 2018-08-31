#include <sstream>
#include <unistd.h>

#include "util.h"

#include <folly/Conv.h>

#ifdef __APPLE__
pthread_t gettid()
{
    return static_cast<pthread_t>(pthread_self());
}
#else
#ifndef _WIN32
pid_t gettid()
{
#ifdef __NR_gettid
    return static_cast<pid_t>(syscall(__NR_gettid));
#else
    return static_cast<pid_t>(pthread_self());
#endif
}
#else
int getpid()
{return 0;}
#endif
#endif

void writePid(const std::string& pid_file_path, const std::string& server_name)
{
    uint32_t curPid;
#ifdef _WIN32
    curPid = (uint32_t) GetCurrentProcess();
#else
    curPid = (uint32_t) getpid();
#endif
    string path = pid_file_path;
    if (!path.empty()) {
    	std::size_t found = pid_file_path.find_last_of("/");
		if (std::string::npos == found) {
			path.append("/");					// 没有路径，后面增加"/"
		}
    }

    std::string pid_file_name = path + server_name + ".pid";
    FILE* f = fopen(pid_file_name.c_str(), "w");
    assert(f);
    char szPid[32];
    snprintf(szPid, sizeof(szPid), "%d", curPid);
    fwrite(szPid, strlen(szPid), 1, f);
    fclose(f);
}

uint64_t get_local_time()
{
    struct timeval tm;
    gettimeofday(&tm, NULL);
    uint64_t ret = (tm.tv_sec * 1000) + (tm.tv_usec /1000);
    return ret;
}

string int2string(uint32_t user_id)
{
    return folly::to<std::string>(user_id);
}

string long2string(uint64_t value)
{
    return folly::to<std::string>(value);
}

uint32_t string2int(const string& value)
{
    try {
        return folly::to<uint32_t>(value);
    } catch (std::exception& e) {
        return 0;
    } catch (...) {
        return 0;
    }
}

uint64_t string2long(const string& value)
{
    try {
        return folly::to<uint64_t>(value);
    } catch (std::exception& e) {
        return 0;
    } catch (...) {
        return 0;
    }
}

unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

inline char fromHex(const char &x)
{
    return isdigit(x) ? x-'0' : x-'A'+10;
}

string URLDecode(const string &sIn)
{
    string sOut;
    for( size_t ix = 0; ix < sIn.size(); ix++ )
    {
        char ch = 0;
        if(sIn[ix]=='%')
        {
            ch = (fromHex(sIn[ix+1])<<4);
            ch |= fromHex(sIn[ix+2]);
            ix += 2;
        }
        else if(sIn[ix] == '+')
        {
            ch = ' ';
        }
        else
        {
            ch = sIn[ix];
        }
        sOut += (char)ch;
    }
    return sOut;
}
