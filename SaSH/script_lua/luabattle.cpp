/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "stdafx.h"
#include "clua.h"
#include <gamedevice.h>
#include "signaldispatcher.h"

long long CLuaBattle::charUseAttack(long long objIndex, sol::this_state s)//atk
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharAttackAct(--objIndex);
}

long long CLuaBattle::charUseMagic(long long magicIndex, long long objIndex, sol::this_state s)//magic
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharMagicAct(--magicIndex, --objIndex);
}

long long CLuaBattle::charUseSkill(long long skillIndex, long long objIndex, sol::this_state s)//skill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharJobSkillAct(--skillIndex, --objIndex);
}

long long CLuaBattle::switchPet(long long petIndex, sol::this_state s)//switch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharSwitchPetAct(--petIndex);
}

long long CLuaBattle::escape(sol::this_state s)//escape
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharEscapeAct();
}

long long CLuaBattle::defense(sol::this_state s)//defense
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharDefenseAct();
}

long long CLuaBattle::useItem(long long itemIndex, long long objIndex, sol::this_state s)//item
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharItemAct(--itemIndex, --objIndex);
}

long long CLuaBattle::catchPet(long long objIndex, sol::this_state s)//catch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharCatchPetAct(--objIndex);
}

long long CLuaBattle::nothing(sol::this_state s)//nothing
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharDoNothing();
}

long long CLuaBattle::petUseSkill(long long petSkillIndex, long long objIndex, sol::this_state s)//petskill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattlePetSkillAct(--petSkillIndex, --objIndex);
}

long long CLuaBattle::petNothing(sol::this_state s)//pet nothing
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattlePetDoNothing();
}

long long CLuaBattle::bwait(sol::object otimeout, sol::object jump, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	if (gamedevice.sendMessage(kEnableBattleDialog, false, NULL) <= 0)
		return FALSE;

	bool bret = luadebug::waitfor(s, timeout, [&gamedevice]()
		{
			if (!gamedevice.worker->getBattleFlag())
				return true;
			long long G = gamedevice.worker->getGameStatus();
			long long W = gamedevice.worker->getWorldStatus();

			return W == 10 && G == 4;
		});

	if (gamedevice.worker->getBattleFlag())
	{
		if (gamedevice.sendMessage(kEnableBattleDialog, true, NULL) <= 0)
			return FALSE;
	}
	else
		bret = false;

	return bret;
}

long long CLuaBattle::bend(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	long long G = gamedevice.worker->getGameStatus();
	if (G == 4)
	{
		if (!mem::write<short>(gamedevice.getProcess(), gamedevice.getProcessModule() + 0xE21E8, 1))
			return FALSE;

		gamedevice.worker->setGameStatus(5);
		gamedevice.worker->isBattleDialogReady.off();
		return TRUE;
	}
	return FALSE;
}