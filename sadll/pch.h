#ifndef PCH_H
#define PCH_H


#define WIN32_LEAN_AND_MEAN             // 從 Windows 標頭排除不常使用的項目
// Windows 標頭檔
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
#ifndef MINT_USE_SEPARATE_NAMESPACE
#define MINT_USE_SEPARATE_NAMESPACE
#include <MINT/MINT.h>
#endif

#include <detours.h>

#ifndef UTF8_EXECUTION
#define UTF8_EXECUTION
#pragma execution_character_set("utf-8")
#endif

#endif //PCH_H
