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

long long CLuaItem::use(long long itemIndex, long long target, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->useItem(--itemIndex, target);

	return TRUE;
}

long long CLuaItem::drop(long long itemIndex, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->dropItem(--itemIndex);

	return TRUE;
}

long long CLuaItem::pick(long long dir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->pickItem(--dir);

	return TRUE;
}

long long CLuaItem::swap(long long from, long long to, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->swapItem(--from, --to);

	return TRUE;
}

long long CLuaItem::craft(long long type, sol::table ingres, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
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

	injector.worker->craft(static_cast<sa::CraftType>(--type), ingreList);

	return TRUE;
}

long long CLuaItem::buy(long long productIndex, long long amount, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->buy(--productIndex, amount, dialogid, unitid);

	return TRUE;
}

long long CLuaItem::sell(long long itemIndex, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	if (itemIndex > 0 && itemIndex <= static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPPLACENUM))
	{
		itemIndex = itemIndex - 1 + sa::CHAR_EQUIPPLACENUM;
		injector.worker->sell(itemIndex, dialogid, unitid);

		return TRUE;
	}

	return FALSE;
}

long long CLuaItem::deposit(long long itemIndex, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->depositItem(--itemIndex, dialogid, unitid);

	return TRUE;
}

long long CLuaItem::withdraw(long long itemIndex, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<long long>());
	if (injector.worker.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.worker->withdrawItem(--itemIndex, dialogid, unitid);

	return TRUE;
}