#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <set>

#include <stdint.h>

#ifndef _WIN32
#include <strings.h>
#endif

#include <sys/stat.h>
#include <assert.h>

#ifdef _WIN32
#define	snprintf	sprintf_s
#else
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#endif

using namespace std;

#ifdef __APPLE__
pthread_t gettid();
#else
#ifndef _WIN32
//获取线程ID的接口
pid_t gettid();
#else
int gettid();
#endif
#endif

void writePid(const std::string& pid_file_path, const std::string& server_name);

uint64_t get_local_time();

string int2string(uint32_t user_id);
string long2string(uint64_t value);
uint32_t string2int(const string& value);
uint64_t string2long(const string& value);

string UrlEncode(const std::string& str);
string URLDecode(const string &sIn);


#ifndef UINT32_MAX
#define UINT32_MAX        4294967295U
#endif

#ifndef INT32_MAX
#define INT32_MAX        2147483647
#endif

#endif /* __UTIL_H__ */
