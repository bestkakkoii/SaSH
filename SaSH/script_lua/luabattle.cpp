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

qint64 CLuaBattle::charUseAttack(qint64 objIndex, sol::this_state s)//atk
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharAttackAct(--objIndex);

	return TRUE;
}

qint64 CLuaBattle::charUseMagic(qint64 magicIndex, qint64 objIndex, sol::this_state s)//magic
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharMagicAct(--magicIndex, --objIndex);

	return TRUE;
}

qint64 CLuaBattle::charUseSkill(qint64 skillIndex, qint64 objIndex, sol::this_state s)//skill
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharJobSkillAct(--skillIndex, --objIndex);

	return TRUE;
}

qint64 CLuaBattle::switchPet(qint64 petIndex, sol::this_state s)//switch
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharSwitchPetAct(--petIndex);

	return TRUE;
}

qint64 CLuaBattle::escape(sol::this_state s)//escape
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharEscapeAct();

	return TRUE;
}

qint64 CLuaBattle::defense(sol::this_state s)//defense
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharDefenseAct();

	return TRUE;
}

qint64 CLuaBattle::useItem(qint64 itemIndex, qint64 objIndex, sol::this_state s)//item
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharItemAct(--itemIndex, --objIndex);

	return TRUE;
}

qint64 CLuaBattle::catchPet(qint64 objIndex, sol::this_state s)//catch
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharCatchPetAct(--objIndex);

	return TRUE;
}

qint64 CLuaBattle::nothing(sol::this_state s)//nothing
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattleCharDoNothing();

	return TRUE;
}

qint64 CLuaBattle::petUseSkill(qint64 petSkillIndex, qint64 objIndex, sol::this_state s)//petskill
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattlePetSkillAct(--petSkillIndex, --objIndex);

	return TRUE;
}

qint64 CLuaBattle::petNothing(sol::this_state s)//pet nothing
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	injector.server->sendBattlePetDoNothing();

	return TRUE;
}