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

qint64 CLuaItem::use(qint64 itemIndex, qint64 target, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->useItem(--itemIndex, target);

	return TRUE;
}

qint64 CLuaItem::drop(qint64 itemIndex, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->dropItem(--itemIndex);

	return TRUE;
}

qint64 CLuaItem::pick(qint64 dir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->pickItem(--dir);

	return TRUE;
}

qint64 CLuaItem::swap(qint64 from, qint64 to, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->swapItem(--from, --to);

	return TRUE;
}

qint64 CLuaItem::craft(qint64 type, sol::table ingres, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
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

		QString str = util::toQString(ingre.second);
		ingreList.append(str);
	}

	injector.server->craft(static_cast<util::CraftType>(--type), ingreList);

	return TRUE;
}

qint64 CLuaItem::buy(qint64 productIndex, qint64 amount, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->buy(--productIndex, amount, dialogid, unitid);

	return TRUE;
}

qint64 CLuaItem::sell(qint64 itemIndex, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	if (itemIndex > 0 && itemIndex <= static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM))
	{
		itemIndex = itemIndex - 1 + CHAR_EQUIPPLACENUM;
		injector.server->sell(itemIndex, dialogid, unitid);

		return TRUE;
	}

	return FALSE;
}

qint64 CLuaItem::deposit(qint64 itemIndex, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->depositItem(--itemIndex, dialogid, unitid);

	return TRUE;
}

qint64 CLuaItem::withdraw(qint64 itemIndex, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->withdrawItem(--itemIndex, dialogid, unitid);

	return TRUE;
}