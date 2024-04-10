/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

long long CLuaChar::kick(sol::object o, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QStringList kickWhiteList;
	long long teammateIndex = -1;
	if (o.is<long long>())
	{
		teammateIndex = o.as<long long>();
		return gamedevice.worker->kickteam(--teammateIndex);
	}
	else if (o.is<std::string>())
	{
		QString teammateNames = util::toQString(o.as<std::string>());
		if (teammateNames.isEmpty())
			return FALSE;

		static const QRegularExpression rex(util::rexOR);
		kickWhiteList = teammateNames.split(rex, Qt::SkipEmptyParts);

		if (kickWhiteList.isEmpty())
			return FALSE;

		//獲取名單
		GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
		if (gamedevice.worker.isNull())
			return FALSE;

		QHash<long long, sa::team_t> teammates = gamedevice.worker->getTeams();
		if (teammates.isEmpty())
			return FALSE;

		bool ok = false;
		// 匹配到的隊員不踢
		for (auto it = teammates.begin(); it != teammates.end(); ++it)
		{
			long long index = it.key();
			if (index < 0 || index >= sa::MAX_TEAM)
				continue;

			sa::team_t team = it.value();

			if (team.name.isEmpty())
				continue;

			if (kickWhiteList.contains(team.name))
				continue;

			if (gamedevice.worker->kickteam(index) && !ok)
				ok = true;
		}
	}

	return FALSE;
}

long long CLuaChar::mail(sol::object oaddrIndex, sol::object omessage, sol::object opetindex, sol::object sitemname, sol::object sitemmemo, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QVariant card;

	long long addrIndex = -1;
	if (oaddrIndex.is<long long>())
	{
		if (addrIndex <= 0 || addrIndex >= sa::MAX_ADDRESS_BOOK)
			return FALSE;
		--addrIndex;

		card = addrIndex;
	}
	else if (oaddrIndex.is<std::string>())
	{
		card = util::toQString(oaddrIndex.as<std::string>());
	}
	else
		return FALSE;

	QString text = util::toQString(omessage);

	long long petIndex = -1;
	if (opetindex.is<long long>())
	{
		petIndex = opetindex.as<long long>();
		--petIndex;
		if (petIndex < 0 || petIndex >= sa::MAX_PET)
			return FALSE;
	}

	if (petIndex != -1)
	{
		--petIndex;
		if (petIndex < 0 || petIndex >= sa::MAX_PET)
			return FALSE;
	}

	QString itemName;
	if (sitemname.is<std::string>())
		itemName = util::toQString(sitemname);

	QString itemMemo;
	if (sitemmemo.is<std::string>())
		itemMemo = util::toQString(sitemmemo);

	if (petIndex != -1 && itemMemo.isEmpty() && !itemName.isEmpty())
		return FALSE;

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	gamedevice.worker->mail(card, text, petIndex, itemName, itemMemo);

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaChar::usemagic(sol::object omagic, sol::object otarget, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long magicIndex = -1;
	QString magicName = "";
	if (omagic.is<long long>())
		magicIndex = omagic.as<long long>();
	else if (omagic.is<std::string>())
		magicName = util::toQString(omagic.as<std::string>());

	if (-1 == magicIndex && magicName.isEmpty())
		return FALSE;

	long long target = -1;
	if (otarget.is <long long>())
		target = otarget.as<long long>();

	if (target < 0)
	{
		QString targetTypeName;
		if (otarget.is<std::string>())
			util::toQString(otarget.as<std::string>());

		if (targetTypeName.isEmpty())
		{
			target = 0;

		}
		else
		{
			QHash<QString, long long> hash = {
				{ "自己", 0},
				{ "戰寵", gamedevice.worker->getCharacter().battlePetNo},
				{ "騎寵", gamedevice.worker->getCharacter().ridePetNo},
				{ "隊長", 6},

				{ "自己", 0},
				{ "战宠", gamedevice.worker->getCharacter().battlePetNo},
				{ "骑宠", gamedevice.worker->getCharacter().ridePetNo},
				{ "队长", 6},

				{ "self", 0},
				{ "battlepet", gamedevice.worker->getCharacter().battlePetNo},
				{ "ride", gamedevice.worker->getCharacter().ridePetNo},
				{ "leader", 6},
			};

			for (long long i = 0; i < sa::MAX_PET; ++i)
			{
				hash.insert("寵物" + util::toQString(i + 1), i + 1);
				hash.insert("宠物" + util::toQString(i + 1), i + 1);
				hash.insert("pet" + util::toQString(i + 1), i + 1);
			}

			for (long long i = 1; i < sa::MAX_TEAM; ++i)
			{
				hash.insert("隊員" + util::toQString(i), i + 1 + sa::MAX_PET);
				hash.insert("队员" + util::toQString(i), i + 1 + sa::MAX_PET);
				hash.insert("teammate" + util::toQString(i), i + 1 + sa::MAX_PET);
			}

			if (!hash.contains(targetTypeName))
				return FALSE;

			target = hash.value(targetTypeName, -1);
			if (target < 0)
				return FALSE;
		}
	}

	magicIndex = gamedevice.worker->getMagicIndexByName(magicName);
	if (magicIndex < 0)
		return FALSE;

	return gamedevice.worker->useMagic(magicIndex, target);
}