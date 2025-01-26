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

sa::item_t CLuaItem::operator[](long long index)
{
	--index;
	if (index >= 100)
		index -= 100;
	else if (index < 100)
	{
		index += sa::CHAR_EQUIPSLOT_COUNT;
	}

	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return sa::item_t();

	gamedevice.worker->updateItemByMemory();

	return gamedevice.worker->getItem(index);
}

long long CLuaItem::swapitem(sol::object ofromIndex, long long toIndex, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	long long fromIndex = -1;
	QString itemName;

	if (ofromIndex.is<long long>())
		fromIndex = ofromIndex.as<long long>();
	else if (ofromIndex.is<std::string>())
	{
		itemName = util::toQString(ofromIndex.as<std::string>());
	}

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	if (itemName.isEmpty())
	{
		if (fromIndex > 100)
			fromIndex -= 100;
		else
			fromIndex += sa::CHAR_EQUIPSLOT_COUNT;
		if (toIndex > 100)
			toIndex -= 100;
		else
			toIndex += sa::CHAR_EQUIPSLOT_COUNT;

		--fromIndex;
		--toIndex;

		if (fromIndex < 0 || fromIndex >= sa::MAX_ITEM)
			return FALSE;

		if (toIndex < 0 || toIndex >= sa::MAX_ITEM)
			return FALSE;

		gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
		if (!gamedevice.worker->swapItem(fromIndex, toIndex))
			return FALSE;
	}
	else
	{
		if (toIndex > sa::MAX_ITEM)
			return FALSE;

		// search all matching items
		QVector<long long> fromIndexs;
		if (!gamedevice.worker->getItemIndexsByName(itemName, "", &fromIndexs, sa::CHAR_EQUIPSLOT_COUNT))
			return FALSE;

		if (fromIndexs.isEmpty())
			return FALSE;

		for (long long fromIndex : fromIndexs)
		{
			if (fromIndex < sa::CHAR_EQUIPSLOT_COUNT)
				continue;

			if (!gamedevice.worker->swapItem(fromIndex, toIndex))
				continue;
		}

		return TRUE;
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::make(std::string singre, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString ingreName = util::toQString(singre);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return FALSE;

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (!gamedevice.worker->craft(sa::kCraftItem, ingreNameList))
		return FALSE;

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::cook(std::string singre, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString ingreName = util::toQString(singre);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return FALSE;

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (!gamedevice.worker->craft(sa::kCraftFood, ingreNameList))
		return FALSE;

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::depositgold(long long gold, sol::object oispublic, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long currentGold = gamedevice.worker->getCharacter().gold;
	if (currentGold <= 0)
		return FALSE;

	if (gold != -1 && gold <= 0)
		return false;

	if (-1 == gold)
		gold = currentGold;
	else if (gold > currentGold)
		gold = currentGold;

	bool isPublic = false;
	if (oispublic.is<bool>())
		isPublic = oispublic.as<bool>();

	return gamedevice.worker->depositGold(gold, isPublic);
}

long long CLuaItem::withdrawgold(long long gold, sol::object oispublic, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	bool isPublic = false;
	if (oispublic.is<bool>())
		isPublic = oispublic.as<bool>();

	return gamedevice.worker->withdrawGold(gold, isPublic);
}

long long CLuaItem::dropgold(long long goldamount, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	if (goldamount == -1)
	{
		sa::character_t pc = gamedevice.worker->getCharacter();
		goldamount = pc.gold;
	}

	if (goldamount <= 0)
		return FALSE;

	return gamedevice.worker->dropGold(goldamount);
}

long long CLuaItem::buy(long long productindex, long long count, sol::object ounit, sol::object odialogid, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	--productindex;

	if (productindex < 0)
		return FALSE;

	if (count <= 0)
		return FALSE;

	QString npcName;
	long long modelid = -1;
	if (ounit.is<std::string>())
		npcName = util::toQString(ounit.as<std::string>());
	else if (ounit.is<long long>())
		modelid = ounit.as<long long>();

	long long dialogid = -1;
	if (odialogid.is<long long>())
		dialogid = odialogid.as<long long>();

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (npcName.isEmpty() && -1 == modelid)
	{
		if (!gamedevice.worker->buy(productindex, count, dialogid))
			return false;
	}
	else
	{
		sa::map_unit_t unit;
		if (!gamedevice.worker->findUnit(npcName, sa::kObjectNPC, &unit, "", modelid))
			return false;

		if (!gamedevice.worker->buy(productindex, count, dialogid, unit.id))
			return false;
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::sell(std::string sname, sol::object ounit, sol::object odialogid, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString name = util::toQString(sname);
	QStringList nameList = name.split(util::rexOR, Qt::SkipEmptyParts);
	if (nameList.isEmpty())
		return FALSE;

	QString npcName;
	long long modelid = -1;
	if (ounit.is<std::string>())
		npcName = util::toQString(ounit.as<std::string>());
	else if (ounit.is<long long>())
		modelid = ounit.as<long long>();

	long long dialogid = -1;
	if (odialogid.is<long long>())
		dialogid = odialogid.as<long long>();

	QVector<long long> itemIndexs;
	for (const QString& it : nameList)
	{
		QVector<long long> indexs;
		if (!gamedevice.worker->getItemIndexsByName(it, "", &indexs, sa::CHAR_EQUIPSLOT_COUNT))
			continue;

		itemIndexs.append(indexs);
	}

	std::sort(itemIndexs.begin(), itemIndexs.end());
	auto it = std::unique(itemIndexs.begin(), itemIndexs.end());
	itemIndexs.erase(it, itemIndexs.end());

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (npcName.isEmpty() && -1 == modelid)
	{
		if (!gamedevice.worker->sell(itemIndexs, dialogid))
			return false;
	}
	else
	{
		sa::map_unit_t unit;
		if (!gamedevice.worker->findUnit(npcName, sa::kObjectNPC, &unit, "", modelid))
			return false;

		if (!gamedevice.worker->sell(itemIndexs, dialogid, unit.id))
			return false;
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::useitem(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

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

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	QString itemName;
	QString itemMemo;
	QStringList itemNames;
	QStringList itemMemos;

	long long target = 0;
	long long totalUse = 1;

	long long min = 0, max = static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT - 1);
	QVector<long long> indexs;
	if (!luatool::checkRange(p1, min, max, &indexs) || p1 == sol::lua_nil || p1.is<std::string>() && p1.as<std::string>().empty())
	{
		if (p1.is<std::string>())
			itemName = util::toQString(p1.as<std::string>());

		if (p2.is<std::string>())
			itemMemo = util::toQString(p2.as<std::string>());

		if (itemName.isEmpty() && itemMemo.isEmpty())
			return FALSE;

		itemNames = itemName.split(util::rexOR);
		itemMemos = itemMemo.split(util::rexOR);

		target = -2;
		if (p3.is<long long>())
			target = p3.as<long long>();

		if (target == -2)
		{
			QString targetTypeName;
			if (p3.is<std::string>())
				targetTypeName = util::toQString(p3.as<std::string>());

			if (targetTypeName.isEmpty())
				target = 0;
			else
			{
				if (!hash.contains(targetTypeName))
					return FALSE;

				target = hash.value(targetTypeName, -1);
				if (target < 0)
					return FALSE;
			}
		}

		if (p4.is<long long>())
			totalUse = p4.as<long long>();

		if (totalUse <= 0)
			return FALSE;
	}
	else
	{
		min += sa::CHAR_EQUIPSLOT_COUNT;
		max += sa::CHAR_EQUIPSLOT_COUNT;

		target = -2;
		if (p2.is<long long>())
			target = p2.as<long long>();

		if (target == -2)
		{
			QString targetTypeName;
			if (p2.is<std::string>())
				targetTypeName = util::toQString(p2.as<std::string>());

			if (targetTypeName.isEmpty())
				target = 0;
			else
			{
				if (!hash.contains(targetTypeName))
					return FALSE;

				target = hash.value(targetTypeName, -1);
				if (target < 0)
					return FALSE;
			}
		}

		if (p3.is<std::string>())
			itemName = util::toQString(p3.as<std::string>());

		if (p4.is<std::string>())
			itemMemo = util::toQString(p4.as<std::string>());

		if (itemName.isEmpty() && itemMemo.isEmpty())
			return FALSE;

		itemNames = itemName.split(util::rexOR);
		itemMemos = itemMemo.split(util::rexOR);
	}

	if (target > 100 || target == -1)
	{
		if (itemNames.size() >= itemMemos.size())
		{
			for (const QString& name : itemNames)
			{
				QString memo;
				if (itemMemos.size() > 0)
					memo = itemMemos.takeFirst();

				QVector<long long> v;
				if (!gamedevice.worker->getItemIndexsByName(name, memo, &v, sa::CHAR_EQUIPSLOT_COUNT))
					continue;

				totalUse = target - 100;

				bool ok = false;
				QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
				for (const long long& it : v)
				{
					sa::item_t item = items.value(it);
					long long size = item.stack;
					for (long long i = 0; i < size; ++i)
					{
						gamedevice.worker->useItem(it, 0);
						if (target != -1)
						{
							--totalUse;
							if (totalUse <= 0)
							{
								ok = true;
								break;
							}
						}
					}

					if (ok)
						break;
				}
			}
		}
		else
		{
			for (const QString& memo : itemMemos)
			{
				QString name;
				if (itemNames.size() > 0)
					name = itemNames.takeFirst();

				QVector<long long> v;
				if (!gamedevice.worker->getItemIndexsByName(name, memo, &v, sa::CHAR_EQUIPSLOT_COUNT))
					continue;

				totalUse = target - 100;

				bool ok = false;
				QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
				for (const long long& it : v)
				{
					sa::item_t item = items.value(it);
					long long size = item.stack;
					for (long long i = 0; i < size; ++i)
					{
						gamedevice.worker->useItem(it, 0);
						if (target != -1)
						{
							--totalUse;
							if (totalUse <= 0)
							{
								ok = true;
								break;
							}
						}
					}

					if (ok)
						break;
				}
			}
		}

		return FALSE;
	}

	if (itemNames.size() >= itemMemos.size())
	{
		for (const QString& name : itemNames)
		{
			QString memo;
			if (itemMemos.size() > 0)
				memo = itemMemos.takeFirst();

			QVector<long long> v;
			if (!gamedevice.worker->getItemIndexsByName(name, memo, &v, sa::CHAR_EQUIPSLOT_COUNT))
				continue;

			if (totalUse == 1)
			{
				long long itemIndex = v.front();
				gamedevice.worker->useItem(itemIndex, target);
				break;
			}

			long long n = totalUse;
			for (;;)
			{
				if (totalUse != -1 && n <= 0)
					break;

				if (v.size() == 0)
					break;

				long long itemIndex = v.takeFirst();
				gamedevice.worker->useItem(itemIndex, target);
				--n;
			}
		}
	}
	else
	{
		for (const QString& memo : itemMemos)
		{
			QString name;
			if (itemNames.size() > 0)
				name = itemNames.takeFirst();

			QVector<long long> v;
			if (!gamedevice.worker->getItemIndexsByName(itemName, memo, &v, sa::CHAR_EQUIPSLOT_COUNT))
				continue;

			if (totalUse == 1)
			{
				long long itemIndex = v.front();
				gamedevice.worker->useItem(itemIndex, target);
				gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.inc();
				break;
			}

			long long n = totalUse;
			for (;;)
			{
				if (totalUse != -1 && n <= 0)
					break;

				if (v.size() == 0)
					break;

				long long itemIndex = v.takeFirst();
				gamedevice.worker->useItem(itemIndex, target);
				gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.inc();
				--n;
			}
		}
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::pickitem(sol::object odir, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString dirStr;
	long long dir = -2;
	if (odir.is<std::string>())
		dirStr = util::toQString(odir.as<std::string>());
	else if (odir.is<long long>())
		dir = odir.as<long long>();

	if (dirStr.isEmpty() && dir == -2)
		return FALSE;

	if (dirStr.startsWith("全") || dir == -1)
	{
		bool bret = false;
		for (long long i = 0; i < 7; ++i)
		{
			bret = gamedevice.worker->pickItem(i);
		}

		return bret;
	}
	else if (dir >= 0)
	{
		return gamedevice.worker->pickItem(dir);
	}

	sa::DirType type = sa::dirMap.value(dirStr, sa::kDirNone);
	if (type == sa::kDirNone)
		return FALSE;

	return gamedevice.worker->pickItem(type);
}

long long CLuaItem::doffitem(sol::object oitem, sol::object p1, sol::object p2, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString tempName;
	QString memo;
	long long itemIndex = -1;
	if (oitem.is<std::string>())
		tempName = util::toQString(oitem.as<std::string>());
	else if (oitem.is<long long>())
		itemIndex = oitem.as<long long>();

	if (p1.is<std::string>())
		memo = util::toQString(p1.as<std::string>());
	if (tempName.isEmpty() && memo.isEmpty() && itemIndex == -1)
	{
		gamedevice.worker->dropItem(-1);
		return FALSE;
	}

	if (tempName == "?")
	{
		for (long long i = sa::CHAR_EQUIPSLOT_COUNT; i < sa::MAX_ITEM; ++i)
			gamedevice.worker->dropItem(i);
	}

	if (tempName.isEmpty() && memo.isEmpty()
		&& ((itemIndex >= 1 && itemIndex <= (sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT))
			|| (itemIndex >= 101 && itemIndex <= static_cast<long long>(sa::CHAR_EQUIPSLOT_COUNT + 100))))
	{
		if (itemIndex < 100)
		{
			--itemIndex;
			itemIndex += sa::CHAR_EQUIPSLOT_COUNT;
		}
		else
			itemIndex -= 100;

		if (itemIndex < sa::CHAR_EQUIPSLOT_COUNT)
			return FALSE;

		gamedevice.worker->dropItem(itemIndex);
		return TRUE;
	}

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	//指定丟棄白名單，位於白名單的物品不丟棄
	if (tempName == QString("非"))
	{
		long long min = 0, max = static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT - 1);
		QVector<long long> indexs;
		if (!luatool::checkRange(p1, min, max, &indexs))
			return FALSE;

		min += sa::CHAR_EQUIPSLOT_COUNT;
		max += sa::CHAR_EQUIPSLOT_COUNT;

		QString itemNames = p2.is<std::string>() ? util::toQString(p2.as<std::string>()) : "";
		if (itemNames.isEmpty())
			return FALSE;

		indexs.clear();
		if (gamedevice.worker->getItemIndexsByName(itemNames, "", &indexs, min, max))
		{
			//排除掉所有包含在indexs的
			for (long long i = min; i <= max; ++i)
			{
				if (i < sa::CHAR_EQUIPSLOT_COUNT)
					continue;

				if (!indexs.contains(i))
				{
					gamedevice.worker->dropItem(i);
				}
			}
		}
	}
	else
	{
		bool bOnlyOne = false;
		if (p2.is<long long>())
			bOnlyOne = p2.as<long long>() > 0;
		else if (p2.is<bool>())
			bOnlyOne = p2.as<bool>();

		QString itemNames = tempName;

		QStringList itemNameList = itemNames.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNameList.isEmpty())
			return FALSE;

		//long long size = itemNameList.size();
		for (const QString& name : itemNameList)
		{
			QVector<long long> indexs;
			bool ok = false;
			if (gamedevice.worker->getItemIndexsByName(name, memo, &indexs))
			{
				for (const long long it : indexs)
				{
					if (it < sa::CHAR_EQUIPSLOT_COUNT)
						continue;

					gamedevice.worker->dropItem(it);
					if (bOnlyOne)
					{
						ok = true;
						break;
					}
				}
			}

			if (ok)
				break;
		}

	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::sellpet(sol::object range, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	sa::map_unit_t unit;
	if (!gamedevice.worker->findUnit("宠物店", sa::kObjectNPC, &unit))
	{
		if (!gamedevice.worker->findUnit("寵物店", sa::kObjectNPC, &unit))
			return FALSE;
	}

	//long long petIndex = -1;
	long long min = 0, max = sa::MAX_PET;
	if (!luatool::checkRange(range, min, max, nullptr))
	{
		return FALSE;
	}

	for (long long petIndex = min; petIndex <= max; ++petIndex)
	{
		if (gamedevice.IS_SCRIPT_INTERRUPT.get())
			return FALSE;

		if (gamedevice.worker.isNull())
			return FALSE;

		if (petIndex < 0 || petIndex >= sa::MAX_PET)
			return FALSE;

		sa::pet_t pet = gamedevice.worker->getPet(petIndex);

		if (!pet.valid)
			continue;

		bool bret = false;
		for (;;)
		{
			if (gamedevice.worker.isNull())
				return FALSE;

			sa::dialog_t dialog = gamedevice.worker->currentDialog.get();
			switch (dialog.dialogid)
			{
			case 263:
			{
				gamedevice.worker->press(sa::kButtonYes, 263, unit.id);
				bret = true;
				break;
			}
			case 262:
			{
				gamedevice.worker->press(petIndex + 1, 262, unit.id);
				gamedevice.worker->press(sa::kButtonYes, 263, unit.id);
				bret = true;
				break;
			}
			default:
			{
				gamedevice.worker->press(3, 261, unit.id);
				gamedevice.worker->press(petIndex + 1, 262, unit.id);
				gamedevice.worker->press(sa::kButtonYes, 263, unit.id);
				bret = true;
				break;
			}
			}

			if (bret)
				break;
		}
	}

	return TRUE;
}

long long CLuaItem::droppet(sol::object oname, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long petIndex = -1;
	QString petName;
	if (oname.is <long long>())
	{
		petIndex = oname.as<long long>();
		--petIndex;
	}
	else if (oname.is<std::string>())
		petName = util::toQString(oname.as<std::string>()).simplified();

	if (petIndex < 0 && petName.isEmpty())
		return FALSE;

	if (petIndex >= 0)
		return gamedevice.worker->dropPet(petIndex);
	else if (!petName.isEmpty())
	{
		if (petName.count("-") == 1 || petName == "?")
		{
			long long min = 0, max = sa::MAX_PET - 1;
			if (!luatool::checkRange(oname, min, max, nullptr))
				return FALSE;

			QVector<long long> v;
			for (long long i = min; i <= max; ++i)
				v.append(i);

			bool bret = false;
			for (const long long it : v)
				bret = gamedevice.worker->dropPet(it);

			return bret;
		}

		QVector<long long> v;

		if (gamedevice.worker->getPetIndexsByName(petName, &v))
		{
			bool bret = false;
			for (const long long it : v)
				bret = gamedevice.worker->dropPet(it) && bret;

			return bret;
		}
	}


	return FALSE;
}

long long CLuaItem::deposititem(sol::object orange, std::string sname, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long min = 0, max = static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT - 1);
	luatool::checkRange(orange, min, max, nullptr);

	min += sa::CHAR_EQUIPSLOT_COUNT;
	max += sa::CHAR_EQUIPSLOT_COUNT;

	QString itemName = util::toQString(sname);

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (!itemName.isEmpty())
	{
		QStringList itemNames = itemName.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNames.isEmpty())
			return FALSE;

		QVector<long long> allv;
		for (const QString& name : itemNames)
		{
			QVector<long long> v;
			if (gamedevice.worker->getItemIndexsByName(name, "", &v, sa::CHAR_EQUIPSLOT_COUNT))
				allv.append(v);
			else if (gamedevice.worker->getItemIndexsByName("", name, &v, sa::CHAR_EQUIPSLOT_COUNT))
				allv.append(v);
		}

		std::sort(allv.begin(), allv.end());
		auto iter = std::unique(allv.begin(), allv.end());
		allv.erase(iter, allv.end());

		QVector<long long> v;
		long long nret = 0;
		for (const long long i : allv)
		{
			if (i < min || i > max)
				continue;

			if (!gamedevice.worker->depositItem(i))
				continue;

			++nret;
		}

		if (0 == nret)
			return FALSE;
	}
	else
	{
		gamedevice.worker->updateItemByMemory();
		QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
		long long nret = 0;
		for (long long i = sa::CHAR_EQUIPSLOT_COUNT; i < sa::MAX_ITEM; ++i)
		{
			sa::item_t item = items.value(i);
			if (item.name.isEmpty() || !item.valid)
				continue;

			if (i < min || i > max)
				continue;

			if (!gamedevice.worker->depositItem(i))
				continue;

			if (!gamedevice.worker->press(1))
				continue;

			++nret;
		}

		if (0 == nret)
			return FALSE;
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::withdrawitem(std::string sname, sol::object omemo, sol::object oisall, sol::object onpc, sol::object odialogid, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString npcName;
	if (onpc.is<std::string>())
		npcName = util::toQString(onpc.as<std::string>());

	long long unitid = -1;
	if (!npcName.isEmpty())
	{
		sa::map_unit_t unit;
		if (!gamedevice.worker->findUnit(npcName, sa::kObjectNPC, &unit))
			return FALSE;

		unitid = unit.id;
	}

	long long dialogid = -1;
	if (odialogid.is<long long>())
		dialogid = odialogid.as<long long>();


	QString itemNames = util::toQString(sname);

	QStringList itemList = itemNames.split(util::rexOR, Qt::SkipEmptyParts);
	long long itemListSize = itemList.size();

	QString memos;
	if (omemo.is<std::string>())
		memos = util::toQString(omemo);

	QStringList memoList = memos.split(util::rexOR, Qt::SkipEmptyParts);
	long long memoListSize = memoList.size();

	bool isAll = false;
	if (oisall.is<bool>())
		isAll = oisall.as<bool>();

	if (itemListSize == 0 && memoListSize == 0)
		return FALSE;

	long long max = 0;
	// max 以大於0且最小的為主 否則以不為0的為主
	if (itemListSize > 0)
		max = itemListSize;
	if (memoListSize > 0 && (max == 0 || memoListSize < max))
		max = memoListSize;

	QVector<sa::item_t> bankItemList = gamedevice.worker->currentBankItemList.toVector();

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	long long nret = 0;

	for (long long i = 0; i < max; ++i)
	{
		QString name = "";
		QString memo = "";
		if (!itemList.isEmpty())
			name = itemList.value(i);
		if (!memoList.isEmpty())
			memo = memoList.value(i);

		long long itemIndex = 0;
		bool bret = false;
		for (long long i = 0; i < bankItemList.size(); ++i)
		{
			const sa::item_t it = bankItemList.value(i);
			qDebug() << it.name << it.memo;
			if (!name.isEmpty())
			{
				if (name.startsWith("?"))
				{
					QString newName = name.mid(1);
					if (it.name.contains(newName) && memo.isEmpty())
						bret = true;
					else if (it.name.contains(newName) && it.memo.contains(memo))
						bret = true;
				}
				else
				{
					if (it.name == name && memo.isEmpty())
						bret = true;
					if (it.name == name && !memo.isEmpty() && it.memo.contains(memo))
						bret = true;

				}
			}
			else if (!memo.isEmpty() && it.memo.contains(memo))
			{
				bret = true;
			}

			if (bret)
			{
				if (!gamedevice.worker->withdrawItem(i, dialogid, unitid))
					continue;

				bankItemList.removeAt(i);
				i = -1;

				if (!isAll)
					break;
				bret = false;
			}

			++itemIndex;
		}

		if (!bret)
			continue;

		++nret;
	}

	if (0 == nret)
		return FALSE;

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

safe::hash<long long, QHash<long long, sa::item_t>> g_recordedEquip;//為了保證即使腳本停止後記錄的裝備也不會丟失，所以使用全局變量
long long CLuaItem::recordequip(sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	gamedevice.worker->updateItemByMemory();
	QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
	QHash<long long, sa::item_t> recordedItems = g_recordedEquip.value(currentIndex);
	for (long long i = 0; i < sa::CHAR_EQUIPSLOT_COUNT; ++i)
	{
		gamedevice.worker->announce(QObject::tr("record equip:[%1]%2").arg(i + 1).arg(items.value(i).name));
		recordedItems.insert(i, items.value(i));
	}

	g_recordedEquip.insert(currentIndex, recordedItems);
	return TRUE;
}

long long CLuaItem::wearrecordedequip(sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
	long long nret = 0;
	for (long long i = 0; i < sa::CHAR_EQUIPSLOT_COUNT; ++i)
	{
		sa::item_t item = items.value(i);
		sa::item_t recordedItem = g_recordedEquip.value(currentIndex).value(i);
		if (!recordedItem.valid || recordedItem.name.isEmpty())
			continue;

		if (item.name == recordedItem.name && item.memo == recordedItem.memo)
			continue;

		long long itemIndex = gamedevice.worker->getItemIndexByName(recordedItem.name, true, "", sa::CHAR_EQUIPSLOT_COUNT);
		if (itemIndex == -1)
			continue;

		if (!gamedevice.worker->useItem(itemIndex, 0))
			continue;

		++nret;
	}

	if (0 == nret)
		return FALSE;

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::unwearequip(sol::object opart, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long part = sa::CHAR_EQUIPNONE;

	if (!opart.is<long long>() || opart.is<long long>() && opart.as<long long>() < 1)
	{
		if (!opart.is<std::string>())
			return FALSE;

		QString partStr = util::toQString(opart);

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			part = 100;
		}
		else
		{
			part = sa::equipMap.value(partStr.toLower(), sa::CHAR_EQUIPNONE);
			if (part == sa::CHAR_EQUIPNONE)
				return FALSE;
		}
	}
	else
	{
		part = opart.as<long long>();
		--part;
	}

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (part < 100)
	{
		long long spotIndex = gamedevice.worker->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return FALSE;

		if (!gamedevice.worker->swapItem(part, spotIndex))
			return FALSE;
	}
	else
	{
		QVector<long long> v;
		if (!gamedevice.worker->getItemEmptySpotIndexs(&v))
			return FALSE;

		long long nret = 0;
		for (long long i = 0; i < sa::CHAR_EQUIPSLOT_COUNT; ++i)
		{
			if (v.isEmpty())
				break;

			long long itemIndex = v.takeFirst();

			if (!gamedevice.worker->swapItem(i, itemIndex))
				continue;

			++nret;
		}

		if (0 == nret)
			return FALSE;
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::petequip(long long petIndex, sol::object oname, sol::object omemo, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	--petIndex;

	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return FALSE;

	QString itemName;
	if (oname.is<std::string>())
		itemName = util::toQString(oname.as<std::string>());

	QString itemMemo;
	if (omemo.is<std::string>())
		itemMemo = util::toQString(omemo.as<std::string>());


	if (itemName.isEmpty() && itemMemo.isEmpty())
		return FALSE;

	QStringList itemNames = itemName.split(util::rexOR);

	QStringList itemMemos = itemMemo.split(util::rexOR);

	QStringList itemMemos2 = itemMemos;

	long long nret = 0;
	if (itemNames.size() >= itemMemos.size())
	{
		for (const QString& name : itemNames)
		{
			QString memo;
			if (!itemMemos2.isEmpty())
				memo = itemMemos2.takeFirst();

			long long itemIndex = gamedevice.worker->getItemIndexByName(name, true, memo, sa::CHAR_EQUIPSLOT_COUNT);
			if (itemIndex == -1)
				continue;

			if (!gamedevice.worker->petitemswap(petIndex, itemIndex, -1))
				continue;

			++nret;
		}

		return nret;
	}

	QStringList itemNames2 = itemNames;
	for (const QString& memo : itemMemos)
	{
		QString name;
		if (!itemNames2.isEmpty())
			name = itemNames2.takeFirst();

		long long itemIndex = gamedevice.worker->getItemIndexByName(name, true, memo, sa::CHAR_EQUIPSLOT_COUNT);
		if (itemIndex == -1)
			continue;

		if (!gamedevice.worker->petitemswap(petIndex, itemIndex, -1))
			continue;

		++nret;
	}

	return nret;
}

long long CLuaItem::petunequip(long long petIndex, sol::object opart, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long part = sa::CHAR_EQUIPNONE;

	--petIndex;

	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return FALSE;

	QVector<long long> partIndexs;
	if (!opart.is<long long>() || opart.is<long long>() && opart.as<long long>() < 1)
	{
		if (opart.is<std::string>())
			return FALSE;

		QString prePartStr = util::toQString(opart);

		QStringList partStrs = prePartStr.split(util::rexOR, Qt::SkipEmptyParts);

		if (prePartStr.toLower() == "all" || prePartStr.toLower() == QString("全部"))
		{
			part = 100;
		}
		else
		{
			for (const QString& partStr : partStrs)
			{
				part = sa::petEquipMap.value(partStr.toLower(), sa::PET_EQUIPNONE);
				if (part != sa::PET_EQUIPNONE)
					partIndexs.append(part);
			}
		}
	}
	else
	{
		part = opart.as<long long>();
		--part;
		partIndexs.append(part);
	}

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	if (!partIndexs.isEmpty())
	{
		QVector<long long> spots;
		if (!gamedevice.worker->getItemEmptySpotIndexs(&spots))
			return FALSE;

		long long nret = 0;
		for (const long long& index : partIndexs)
		{
			if (spots.isEmpty())
				break;

			if (!gamedevice.worker->petitemswap(petIndex, index, spots.takeFirst()))
				continue;

			++nret;
		}

		if (0 == nret)
			return FALSE;
	}
	else if (part == 100)
	{
		QVector<long long> spots;
		if (!gamedevice.worker->getItemEmptySpotIndexs(&spots))
			return FALSE;

		long long nret = 0;
		for (long long index = 0; index < sa::MAX_PET_ITEM; ++index)
		{
			if (spots.isEmpty())
				break;

			if (!gamedevice.worker->petitemswap(petIndex, index, spots.takeFirst()))
				continue;

			++nret;
		}

		if (0 == nret)
			return FALSE;
	}

	bool bret = luadebug::waitfor(s, 200, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::depositpet(sol::object oslots, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long petIndex = 0;
	QString petName;
	if (!oslots.is<long long>())
	{
		if (!oslots.is<std::string>())
			return FALSE;

		petName = util::toQString(oslots);
	}

	if (petIndex == 0 && petName.isEmpty())
		return FALSE;

	if (petIndex > 0)
		--petIndex;
	else
	{
		QVector<long long> v;
		if (!gamedevice.worker->getPetIndexsByName(petName, &v))
			return FALSE;
		petIndex = v.first();
	}

	if (petIndex == -1)
		return FALSE;

	gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();

	if (!gamedevice.worker->depositPet(petIndex))
	{
		gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.off();
		return FALSE;
	}

	bool bret = luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

	gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();
	if (!gamedevice.worker->press(sa::kButtonYes))
	{
		gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.off();
		return FALSE;
	}

	bret = luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

	gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();
	if (!gamedevice.worker->press(sa::kButtonOk))
	{
		gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.off();
		return FALSE;
	}

	bret = luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

	gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.off();
	return bret;
}

long long CLuaItem::withdrawpet(std::string sname, sol::object olevel, sol::object omaxhp, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString petName = util::toQString(sname);
	if (petName.isEmpty())
		return FALSE;

	long long level = 0;
	if (olevel.is<long long>())
		level = olevel.as<long long>();

	long long maxHp = 0;
	if (omaxhp.is<long long>())
		omaxhp.as<long long>();

	for (;;)
	{
		QPair<long long, QVector<sa::bank_pet_t>> bankPetList = gamedevice.worker->currentBankPetList;
		long long button = bankPetList.first;
		if (button == 0)
			break;

		QVector<sa::bank_pet_t> petList = bankPetList.second;
		long long petIndex = 0;
		bool bret = false;
		for (const sa::bank_pet_t& it : petList)
		{
			if (!petName.startsWith("?"))
			{
				if (it.name == petName && it.maxHp == maxHp && it.level == level)
					bret = true;
				else if (it.name == petName && 0 == maxHp && it.level == level)
					bret = true;
				else if (it.name == petName && it.maxHp == maxHp && 0 == level)
					bret = true;
				else if (it.name == petName && 0 == maxHp && 0 == level)
					bret = true;
			}
			else
			{
				QString newName = petName.mid(1);
				if (it.name.contains(newName) && it.maxHp == maxHp && it.level == level)
					bret = true;
				else if (it.name.contains(newName) && 0 == maxHp && it.level == level)
					bret = true;
				else if (it.name.contains(newName) && it.maxHp == maxHp && 0 == level)
					bret = true;
				else if (it.name.contains(newName) && 0 == maxHp && 0 == level)
					bret = true;
			}

			if (bret)
				break;

			++petIndex;
		}

		if (bret)
		{
			gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();
			gamedevice.worker->withdrawPet(petIndex);
			luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

			gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();
			gamedevice.worker->press(sa::kButtonYes);
			luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

			gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();
			gamedevice.worker->press(sa::kButtonOk);
			luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

			gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.off();
			break;
		}

		if (util::checkAND(button, sa::kButtonNext))
		{
			gamedevice.worker->IS_WAITFOR_BANK_FLAG.on();
			gamedevice.worker->press(sa::kButtonNext);
			if (!luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_BANK_FLAG.get(); }))
			{
				gamedevice.worker->IS_WAITFOR_BANK_FLAG.off();
				break;
			}
			gamedevice.worker->IS_WAITFOR_BANK_FLAG.off();
		}
		else
			break;
	}

	return TRUE;
}

long long CLuaItem::trade(std::string sname, sol::object oitem, sol::object opet, sol::object ogold, sol::object oitemout, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString name = util::toQString(sname);

	if (name.isEmpty())
	{
		qDebug() << "trade name is empty";
		return FALSE;
	}

	QString itemListStr = "";
	if (oitem.is<std::string>())
		itemListStr = util::toQString(oitem.as<std::string>());
	long long itemPos = -1;
	if (oitem.is<long long>() && oitem.as<long long>() > 0)
		itemPos = oitem.as<long long>() - 1 + sa::CHAR_EQUIPSLOT_COUNT;

	QString petListStr = "";
	if (opet.is<std::string>())
		petListStr = util::toQString(opet.as<std::string>());
	long long petPos = -1;
	if (opet.is<long long>() && opet.as<long long>() > 0)
		petPos = opet.as<long long>() - 1;

	long long gold = 0;
	if (ogold.is<long long>())
		gold = ogold.as<long long>();
	else if (ogold.is<std::string>())
	{
		QString qgold = util::toQString(ogold.as<std::string>());
		if (qgold == "all")
			gold = gamedevice.worker->getCharacter().gold;
		else
		{
			qDebug() << "you input a string type of value for gold but it only allowed 'all'";
			return FALSE;
		}
	}

	long long timeout = 5000;
	if (oitemout.is<long long>())
		timeout = oitemout.as<long long>();

	sa::map_unit_t unit;
	if (!gamedevice.worker->findUnit(name, sa::kObjectHuman, &unit))
	{
		qDebug() << "can not find unit:" << name;
		return FALSE;
	}

	QPoint dst;
	AStarDevice astar;
	long long dir = gamedevice.worker->mapDevice.calcBestFollowPointByDstPoint(
		currentIndex, &astar, gamedevice.worker->getFloor(), gamedevice.worker->getPoint(), unit.p, &dst, true, unit.dir);
	if (dir == -1)
	{
		qDebug() << "can not find path to unit:" << name;
		return FALSE;
	}

	gamedevice.worker->setCharFaceDirection(dir);

	if (!gamedevice.worker->tradeStart(name, timeout))
		return FALSE;

	//bool ok = false;
	if (!itemListStr.isEmpty())
	{
		QStringList itemIndexList;
		if (itemListStr.toLower() == "all")
		{
			for (long long i = 1; i <= (sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT); ++i)
			{
				itemIndexList.append(util::toQString(i));
			}
		}
		else if (itemListStr.count("-") == 1 || itemListStr.count("-") == 0)
		{
			long long min = 1;
			long long max = static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT);

			if (!luatool::checkRange(oitem, min, max, nullptr))
			{
				qDebug() << "not a valid range string for itemList";
				return FALSE;
			}

			for (long long i = min; i <= max + 1; ++i)
				itemIndexList.append(util::toQString(i));
		}
		else if (itemListStr.count("|") > 0)
		{
			QStringList itemIndexList2 = itemListStr.split("|", Qt::SkipEmptyParts);
			for (const QString& itemIndex : itemIndexList2)
			{
				bool ok = false;
				long long index = itemIndex.toLongLong(&ok);
				if (ok && index >= 1 && index <= (sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT))
					itemIndexList.append(util::toQString(index));
			}
		}

		QVector<long long> itemIndexVec;
		for (const QString& itemIndex : itemIndexList)
		{
			bool bret = false;
			long long index = itemIndex.toLongLong(&bret);
			--index;
			index += sa::CHAR_EQUIPSLOT_COUNT;
			QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
			if (bret && index >= sa::CHAR_EQUIPSLOT_COUNT && index < sa::MAX_ITEM)
			{
				if (items.value(index).valid)
					itemIndexVec.append(index);
			}
		}

		if (!itemIndexVec.isEmpty())
		{
			std::sort(itemIndexVec.begin(), itemIndexVec.end());
			auto it = std::unique(itemIndexVec.begin(), itemIndexVec.end());
			itemIndexVec.erase(it, itemIndexVec.end());
		}

		if (!itemIndexVec.isEmpty())
		{
			if (gamedevice.worker->tradeAppendItems(name, itemIndexVec) == -1)
			{
				gamedevice.worker->tradeCancel();
				qDebug() << "tradeAppendItems failed";
				return FALSE;
			}
		}
		else
		{
			gamedevice.worker->tradeCancel();
			qDebug() << "itemIndexVec is empty";
			return FALSE;
		}
	}
	else if (itemPos >= 0)
	{
		QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
		if (items.value(itemPos).valid)
		{
			if (gamedevice.worker->tradeAppendItems(name, QVector<long long>() << itemPos) == -1)
			{
				gamedevice.worker->tradeCancel();
				qDebug() << "tradeAppendItems failed";
				return FALSE;
			}
		}
		else
		{
			gamedevice.worker->tradeCancel();
			qDebug() << "itemPos is invalid";
			return FALSE;
		}
	}

	if (!petListStr.isEmpty())
	{
		QVector<long long> petIndexVec;
		if (petListStr.toLower() == "all")
		{
			for (long long i = 0; i < sa::MAX_PET; ++i)
			{
				sa::pet_t pet = gamedevice.worker->getPet(i);
				if (pet.valid)
					petIndexVec.append(i);
			}
		}
		else if (petListStr.count("-") == 1 || petListStr.count("-") == 0 && petListStr.count("|") == 0)
		{
			long long min = 1;
			long long max = sa::MAX_PET;
			if (!luatool::checkRange(opet, min, max, nullptr))
			{
				gamedevice.worker->tradeCancel();
				qDebug() << "not a valid range string for petList";
				return FALSE;
			}

			for (long long i = min; i <= max + 1; ++i)
			{
				sa::pet_t pet = gamedevice.worker->getPet(i - 1);
				if (pet.valid)
					petIndexVec.append(i - 1);
			}
		}
		else if (petListStr.count("|") > 0)
		{
			QStringList petIndexList = petListStr.split(util::rexOR, Qt::SkipEmptyParts);
			for (const QString& petIndex : petIndexList)
			{
				bool ok = false;
				long long index = petIndex.toLongLong(&ok);
				--index;
				if (ok && index >= 0 && index < sa::MAX_PET)
				{
					sa::pet_t pet = gamedevice.worker->getPet(index);
					if (pet.valid)
						petIndexVec.append(index);
				}
			}
		}

		std::sort(petIndexVec.begin(), petIndexVec.end());
		auto it = std::unique(petIndexVec.begin(), petIndexVec.end());
		petIndexVec.erase(it, petIndexVec.end());

		if (!petIndexVec.isEmpty())
		{
			if (gamedevice.worker->tradeAppendPets(name, petIndexVec) == -1)
			{
				gamedevice.worker->tradeCancel();
				qDebug() << "tradeAppendPets failed";
				return FALSE;
			}
		}
		else
		{
			gamedevice.worker->tradeCancel();
			qDebug() << "petIndexVec is empty";
			return FALSE;
		}
	}
	else if (petPos >= 0)
	{
		sa::pet_t pet = gamedevice.worker->getPet(petPos);
		if (!pet.valid)
		{
			gamedevice.worker->tradeCancel();
			qDebug() << "petPos is invalid";
			return FALSE;
		}

		if (gamedevice.worker->tradeAppendPets(name, QVector<long long>() << petPos) == -1)
		{
			gamedevice.worker->tradeCancel();
			qDebug() << "tradeAppendPets failed";
			return FALSE;
		}
	}

	if (gold > 0 && gold <= gamedevice.worker->getCharacter().gold)
	{
		if (!gamedevice.worker->tradeAppendGold(name, gold))
		{
			gamedevice.worker->tradeCancel();
			qDebug() << "tradeAppendGold failed";
			return FALSE;
		}
	}

	if (!gamedevice.worker->tradeComfirm(name))
	{
		gamedevice.worker->tradeCancel();
		qDebug() << "tradeComfirm failed";
		return FALSE;
	}

	bool ret = luadebug::waitfor(s, timeout, [&gamedevice]()
		{
			return gamedevice.worker->getCharacter().trade_confirm >= 3;
		});

	if (!ret)
	{
		gamedevice.worker->tradeCancel();
		qDebug() << "tradeComfirm failed2";
		return FALSE;
	}

	if (!gamedevice.worker->tradeComplete(name))
	{
		gamedevice.worker->tradeCancel();
		qDebug() << "tradeComplete failed";
		return FALSE;
	}

	ret = luadebug::waitfor(s, timeout, [&gamedevice]()
		{
			return !gamedevice.worker->IS_TRADING.get();
		});

	if (!ret)
	{
		gamedevice.worker->tradeCancel();
		qDebug() << "tradeComplete failed2";
		return FALSE;
	}

	return ret ? TRUE : FALSE;
}

long long CLuaItem::getSpace()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	QVector<long long> itemIndexs;
	gamedevice.worker->getItemEmptySpotIndexs(&itemIndexs);

	return itemIndexs.size();
}

long long CLuaItem::getSize()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
	long long size = 0;
	sa::item_t item;
	for (auto it = items.begin(); it != items.end(); ++it)
	{
		item = it.value();
		if (it.key() < sa::CHAR_EQUIPSLOT_COUNT || it.key() >= sa::MAX_ITEM)
			continue;

		if (it->valid)
			++size;
	}

	return size;
}

long long CLuaItem::getSpaceIndex()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	QVector<long long> itemIndexs;
	gamedevice.worker->getItemEmptySpotIndexs(&itemIndexs);

	if (itemIndexs.isEmpty())
		return -1;

	return itemIndexs.front();
}

bool CLuaItem::getIsFull()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return false;

	QVector<long long> itemIndexs;
	gamedevice.worker->getItemEmptySpotIndexs(&itemIndexs);

	return itemIndexs.isEmpty();
}

static QVector<long long> itemGetIndexs(long long currentIndex, sol::object oitemnames, sol::object oitemmemos, bool includeEequip, sol::object ostartFrom)
{
	QVector<long long> itemIndexs;
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return itemIndexs;

	QString itemnames;
	if (oitemnames.is<std::string>())
		itemnames = util::toQString(oitemnames);
	QString itemmemos;
	if (oitemmemos.is<std::string>())
		itemmemos = util::toQString(oitemmemos);

	if (itemnames.isEmpty() && itemmemos.isEmpty())
	{
		return itemIndexs;
	}

	long long startFrom = -1;
	if (ostartFrom.is<long long>())
		startFrom = ostartFrom.as<long long>();
	if (startFrom < 0 || startFrom >= (sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT))
		startFrom = -1;

	long long min = sa::CHAR_EQUIPSLOT_COUNT;

	if (includeEequip)
	{
		min = 0;
	}
	else
	{
		if (startFrom > 0)
			min += startFrom;
		else
			min = sa::CHAR_EQUIPSLOT_COUNT;
	}

	long long max = sa::MAX_ITEM;


	if (!gamedevice.worker->getItemIndexsByName(itemnames, itemmemos, &itemIndexs, min, max))
	{
		return itemIndexs;
	}
	return itemIndexs;
}

long long CLuaItem::count(sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::object ostartFrom, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return FALSE;

	bool includeEequip = true;
	if (oincludeEequip.is<bool>())
		includeEequip = oincludeEequip.as<bool>();

	long long c = 0;
	QVector<long long> itemIndexs = itemGetIndexs(index_, oitemnames, oitemmemos, includeEequip, ostartFrom);
	if (itemIndexs.isEmpty())
		return c;

	QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
	for (const long long itemIndex : itemIndexs)
	{
		sa::item_t item = items.value(includeEequip ? itemIndex : itemIndex + sa::CHAR_EQUIPSLOT_COUNT - 1);
		if (item.valid)
			c += item.stack;
	}

	return c;
}

long long CLuaItem::indexof(sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::object ostartFrom, sol::this_state s)
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	bool includeEequip = true;
	if (oincludeEequip.is<bool>())
		includeEequip = oincludeEequip.as<bool>();

	QVector<long long> itemIndexs = itemGetIndexs(index_, oitemnames, oitemmemos, includeEequip, ostartFrom);
	if (itemIndexs.isEmpty())
		return -1;

	long long index = itemIndexs.front();
	if (index < sa::CHAR_EQUIPSLOT_COUNT)
		index += 100LL;
	else
		index -= static_cast<long long>(sa::CHAR_EQUIPSLOT_COUNT);

	++index;

	return index;
}

sol::object CLuaItem::find(sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::object ostartFrom, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return sol::lua_nil;

	bool includeEequip = true;
	if (oincludeEequip.is<bool>())
		includeEequip = oincludeEequip.as<bool>();

	QVector<long long> itemIndexs = itemGetIndexs(index_, oitemnames, oitemmemos, includeEequip, ostartFrom);
	if (itemIndexs.isEmpty())
		return sol::lua_nil;

	long long index = itemIndexs.front();
	if (index < sa::CHAR_EQUIPSLOT_COUNT)
		index += 100LL;
	else
		index -= static_cast<long long>(sa::CHAR_EQUIPSLOT_COUNT);

	++index;

	if (index < 0 || index >= sa::MAX_ITEM)
		return sol::lua_nil;

	sa::item_t item = gamedevice.worker->getItem(includeEequip ? index : index + sa::CHAR_EQUIPSLOT_COUNT - 1);

	sol::table t = lua.create_table();
	t["valid"] = item.valid;
	t["index"] = index;
	t["name"] = util::toConstData(item.name);
	t["memo"] = util::toConstData(item.memo);
	t["name2"] = util::toConstData(item.name2);
	t["dura"] = item.damage;
	t["lv"] = item.level;
	t["stack"] = item.stack;
	t["field"] = item.field;
	t["target"] = item.target;
	t["type"] = item.type;
	t["modelid"] = item.modelid;

	return t;
}