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
import String;
#include "stdafx.h"
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"

__int64 CLuaChar::rename(std::string sfname, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString name = toQString(sfname);

	injector.server->setCharFreeName(name);

	return TRUE;
}

__int64 CLuaChar::useMagic(__int64 magicIndex, __int64 target, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->useMagic(--magicIndex, target);

	return TRUE;
}

__int64 CLuaChar::depositGold(__int64 gold, sol::object oispublic, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->depositGold(gold, oispublic.is<bool>() ? oispublic.as<bool>() : false);

	return TRUE;
}

__int64 CLuaChar::withdrawGold(__int64 gold, sol::object oispublic, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->withdrawGold(gold, oispublic.is<bool>() ? oispublic.as<bool>() : false);

	return TRUE;
}

__int64 CLuaChar::dropGold(__int64 gold, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->dropGold(gold);

	return TRUE;
}

__int64 CLuaChar::mail(__int64 cardIndex, std::string stext, __int64 petIndex, std::string sitemName, std::string sitemMemo, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = toQString(stext);

	QString itemName = toQString(sitemName);

	QString itemMemo = toQString(sitemMemo);

	injector.server->mail(--cardIndex, text, petIndex, itemName, itemMemo);

	return TRUE;
}

__int64 CLuaChar::mail(__int64 cardIndex, std::string stext, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = toQString(stext);

	injector.server->mail(--cardIndex, text, -1, "", "");

	return TRUE;
}

__int64 CLuaChar::skillUp(__int64 abilityIndex, __int64 amount, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->addPoint(--abilityIndex, amount);

	return TRUE;
}

//action-group
__int64 CLuaChar::setTeamState(bool join, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->setTeamState(join);

	return TRUE;
}

__int64 CLuaChar::kick(__int64 teammateIndex, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->kickteam(--teammateIndex);

	return TRUE;
}