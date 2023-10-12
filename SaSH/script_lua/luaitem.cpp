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
import Global;
import String;
#include "stdafx.h"
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"

__int64 CLuaItem::use(__int64 itemIndex, __int64 target, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->useItem(--itemIndex, target);

	return TRUE;
}

__int64 CLuaItem::drop(__int64 itemIndex, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->dropItem(--itemIndex);

	return TRUE;
}

__int64 CLuaItem::pick(__int64 dir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->pickItem(--dir);

	return TRUE;
}

__int64 CLuaItem::swap(__int64 from, __int64 to, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->swapItem(--from, --to);

	return TRUE;
}

__int64 CLuaItem::craft(__int64 type, sol::table ingres, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	if (type < 1 || type > 2)
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QStringList ingreList;
	for (auto& ingre : ingres)
	{
		//not string continue
		if (!ingre.second.is<std::string>())
			continue;

		QString str = toQString(ingre.second);
		ingreList.append(str);
	}

	injector.server->craft(static_cast<CraftType>(--type), ingreList);

	return TRUE;
}

__int64 CLuaItem::buy(__int64 productIndex, __int64 amount, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->buy(--productIndex, amount, dialogid, unitid);

	return TRUE;
}

__int64 CLuaItem::sell(__int64 itemIndex, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	if (itemIndex > 0 && itemIndex <= static_cast<__int64>(MAX_ITEM - CHAR_EQUIPPLACENUM))
	{
		itemIndex = itemIndex - 1 + CHAR_EQUIPPLACENUM;
		injector.server->sell(itemIndex, dialogid, unitid);

		return TRUE;
	}

	return FALSE;
}

__int64 CLuaItem::deposit(__int64 itemIndex, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->depositItem(--itemIndex, dialogid, unitid);

	return TRUE;
}

__int64 CLuaItem::withdraw(__int64 itemIndex, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->withdrawItem(--itemIndex, dialogid, unitid);

	return TRUE;
}