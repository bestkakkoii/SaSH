#include "stdafx.h"
#include "sahook/game/include/gameImplement.h"

#pragma comment(lib, "ws2_32.lib")

game::Implement::Implement(HMODULE processBase)
	: processBase_(reinterpret_cast<DWORD>(processBase))
{
}