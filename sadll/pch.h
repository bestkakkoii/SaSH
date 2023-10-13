#ifndef PCH_H
#define PCH_H

#ifndef UTF8_EXECUTION
#define UTF8_EXECUTION
#if _MSC_VER >= 1600 
#pragma execution_character_set("utf-8") 
#endif
#endif

#ifdef __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <iostream>
#include <random>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <timeapi.h>
#ifndef MINT_USE_SEPARATE_NAMESPACE
#define MINT_USE_SEPARATE_NAMESPACE
#include <MINT/MINT.h>
#endif

#include <detours.h>
#include <usermessage.h>
#include "util.h"

#define USE_ASYNC_TCP
#ifdef USE_ASYNC_TCP
#include "asyncclient.h"
#else
#include "syncclient.h"
#endif

#endif

#endif //PCH_H
