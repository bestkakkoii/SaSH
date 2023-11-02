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

long long CLuaChar::rename(std::string sfname, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString name = util::toQString(sfname);

	injector.worker->setCharFreeName(name);

	return TRUE;
}

long long CLuaChar::useMagic(long long magicIndex, long long target, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->useMagic(--magicIndex, target);

	return TRUE;
}

long long CLuaChar::depositGold(long long gold, sol::object oispublic, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->depositGold(gold, oispublic.is<bool>() ? oispublic.as<bool>() : false);

	return TRUE;
}

long long CLuaChar::withdrawGold(long long gold, sol::object oispublic, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->withdrawGold(gold, oispublic.is<bool>() ? oispublic.as<bool>() : false);

	return TRUE;
}

long long CLuaChar::dropGold(long long gold, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->dropGold(gold);

	return TRUE;
}

long long CLuaChar::mail(long long cardIndex, std::string stext, long long petIndex, std::string sitemName, std::string sitemMemo, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString text = util::toQString(stext);

	QString itemName = util::toQString(sitemName);

	QString itemMemo = util::toQString(sitemMemo);

	injector.worker->mail(--cardIndex, text, petIndex, itemName, itemMemo);

	return TRUE;
}

long long CLuaChar::mail(long long cardIndex, std::string stext, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString text = util::toQString(stext);

	injector.worker->mail(--cardIndex, text, -1, "", "");

	return TRUE;
}

long long CLuaChar::skillUp(long long abilityIndex, long long amount, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->addPoint(--abilityIndex, amount);

	return TRUE;
}

//action-group
long long CLuaChar::join(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->setTeamState(true);

	return TRUE;
}

long long CLuaChar::leave(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->setTeamState(false);

	return TRUE;
}

long long CLuaChar::kick(long long teammateIndex, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	injector.worker->kickteam(--teammateIndex);

	return TRUE;
}

long long CLuaChar::doffstone(long long goldamount, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	if (goldamount == -1)
	{
		sa::PC pc = injector.worker->getPC();
		goldamount = pc.gold;
	}

	if (goldamount <= 0)
		return FALSE;

	injector.worker->dropGold(goldamount);

	return TRUE;
}