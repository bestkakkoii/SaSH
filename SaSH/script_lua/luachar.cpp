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

sa::character_t CLuaChar::getCharacter() const
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return sa::character_t();

	gamedevice.worker->updateDatasFromMemory();

	return gamedevice.worker->getCharacter();
}

long long CLuaChar::rename(std::string sfname, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString name = util::toQString(sfname);
	return gamedevice.worker->setCharFreeName(name);
}

long long CLuaChar::skup(sol::object otype, long long count, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long point = -1;
	QString pointName;
	if (otype.is<std::string>())
		pointName = util::toQString(otype);
	else if (otype.is<long long>())
	{
		point = otype.as<long long>();
		--point;
	}
	else
		return FALSE;

	if (pointName.isEmpty())
		return FALSE;

	if (!pointName.isEmpty())
	{
		static const QHash<QString, long long> hash = {
		{ "體力", 0},
		{ "腕力", 1},
		{ "耐力", 2},
		{ "速度", 3},

		{ "体力", 0},
		{ "腕力", 1},
		{ "耐力", 2},
		{ "速度", 3},

		{ "vit", 0},
		{ "str", 1},
		{ "tgh", 2},
		{ "dex", 3},
		};

		point = hash.value(pointName.toLower(), -1);
	}

	if (point < 0)
		return FALSE;

	if (count <= 0)
		return FALSE;

	return gamedevice.worker->addPoint(point, count);
}

//action-group
long long CLuaChar::join(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->setTeamState(true);
}

long long CLuaChar::leave(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->setTeamState(false);
}

long long CLuaChar::kick(long long teammateIndex, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->kickteam(--teammateIndex);
}