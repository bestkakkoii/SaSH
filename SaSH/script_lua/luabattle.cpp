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
#include "injector.h"
#include "signaldispatcher.h"

long long CLuaBattle::charUseAttack(long long objIndex, sol::this_state s)//atk
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharAttackAct(--objIndex);

	return TRUE;
}

long long CLuaBattle::charUseMagic(long long magicIndex, long long objIndex, sol::this_state s)//magic
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharMagicAct(--magicIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::charUseSkill(long long skillIndex, long long objIndex, sol::this_state s)//skill
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharJobSkillAct(--skillIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::switchPet(long long petIndex, sol::this_state s)//switch
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharSwitchPetAct(--petIndex);

	return TRUE;
}

long long CLuaBattle::escape(sol::this_state s)//escape
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharEscapeAct();

	return TRUE;
}

long long CLuaBattle::defense(sol::this_state s)//defense
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharDefenseAct();

	return TRUE;
}

long long CLuaBattle::useItem(long long itemIndex, long long objIndex, sol::this_state s)//item
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharItemAct(--itemIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::catchPet(long long objIndex, sol::this_state s)//catch
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharCatchPetAct(--objIndex);

	return TRUE;
}

long long CLuaBattle::nothing(sol::this_state s)//nothing
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattleCharDoNothing();

	return TRUE;
}

long long CLuaBattle::petUseSkill(long long petSkillIndex, long long objIndex, sol::this_state s)//petskill
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattlePetSkillAct(--petSkillIndex, --objIndex);

	return TRUE;
}

long long CLuaBattle::petNothing(sol::this_state s)//pet nothing
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	injector.worker->sendBattlePetDoNothing();

	return TRUE;
}