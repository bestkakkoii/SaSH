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

	gamedevice.worker->sendBattleCharAttackAct(--objIndex);

	return TRUE;
}

long long CLuaBattle::charUseMagic(long long magicIndex, long long objIndex, sol::this_state s)//magic
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharMagicAct(--magicIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::charUseSkill(long long skillIndex, long long objIndex, sol::this_state s)//skill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharJobSkillAct(--skillIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::switchPet(long long petIndex, sol::this_state s)//switch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharSwitchPetAct(--petIndex);

	return TRUE;
}

long long CLuaBattle::escape(sol::this_state s)//escape
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharEscapeAct();

	return TRUE;
}

long long CLuaBattle::defense(sol::this_state s)//defense
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharDefenseAct();

	return TRUE;
}

long long CLuaBattle::useItem(long long itemIndex, long long objIndex, sol::this_state s)//item
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharItemAct(--itemIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::catchPet(long long objIndex, sol::this_state s)//catch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharCatchPetAct(--objIndex);

	return TRUE;
}

long long CLuaBattle::nothing(sol::this_state s)//nothing
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattleCharDoNothing();

	return TRUE;
}

long long CLuaBattle::petUseSkill(long long petSkillIndex, long long objIndex, sol::this_state s)//petskill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattlePetSkillAct(--petSkillIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::petNothing(sol::this_state s)//pet nothing
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	gamedevice.worker->sendBattlePetDoNothing();

	return TRUE;
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

	gamedevice.sendMessage(kEnableBattleDialog, false, NULL);
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
		gamedevice.sendMessage(kEnableBattleDialog, true, NULL);
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
		mem::write<short>(gamedevice.getProcess(), gamedevice.getProcessModule() + 0xE21E8, 1);
		gamedevice.worker->setGameStatus(5);
		gamedevice.worker->isBattleDialogReady.off();
		return TRUE;
	}
	return FALSE;
}