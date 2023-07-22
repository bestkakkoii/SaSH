#ifndef PCH_H
#define PCH_H

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

#ifndef UTF8_EXECUTION
#define UTF8_EXECUTION
#pragma execution_character_set("utf-8")
#endif

#endif

#endif //PCH_H
