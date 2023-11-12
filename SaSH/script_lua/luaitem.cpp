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

sa::item_t& CLuaItem::operator[](long long index)
{
	--index;
	if (index >= 100)
		index -= 100;
	else if (index < 100)
	{
		index += sa::CHAR_EQUIPSLOT_COUNT;
	}

	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (!gamedevice.worker.isNull())
	{
		gamedevice.worker->updateItemByMemory();
		QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
		if (items.contains(index))
			items_.insert(index, items.value(index));
	}

	if (!items_.contains(index))
		items_.insert(index, sa::item_t());

	return items_[index];
}

long long CLuaItem::swapitem(long long fromIndex, long long toIndex, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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
			bret = bret && gamedevice.worker->pickItem(i);
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

			//QThread::msleep(300);
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
		petName = util::toQString(oname.as<std::string>());

	if (petIndex < 0 && petName.isEmpty())
		return FALSE;

	if (petIndex >= 0)
		return gamedevice.worker->dropPet(petIndex);
	else if (!petName.isEmpty())
	{
		QVector<long long> v;
		if (gamedevice.worker->getPetIndexsByName(petName, &v))
		{
			bool bret = false;
			for (const long long it : v)
				bret = bret && gamedevice.worker->dropPet(it);

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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return bret;
}

long long CLuaItem::withdrawitem(std::string sname, sol::object omemo, sol::object oisall, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

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
		for (const sa::item_t& it : bankItemList)
		{
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
				bankItemList.remove(itemIndex);
				if (!isAll)
					break;
			}

			++itemIndex;
		}

		if (!bret)
			continue;

		if (!gamedevice.worker->withdrawItem(itemIndex))
			continue;

		++nret;
	}

	if (0 == nret)
		return FALSE;

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bool bret = luadebug::waitfor(s, 500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
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

	bret = bret && luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

	gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.on();
	if (!gamedevice.worker->press(sa::kButtonOk))
	{
		gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.off();
		return FALSE;
	}

	bret = bret && luadebug::waitfor(s, 1000, [&gamedevice]()->bool { return !gamedevice.worker->IS_WAITFOR_DIALOG_FLAG.get(); });

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

long long CLuaItem::getSpace()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	QVector<long long> itemIndexs;
	gamedevice.worker->getItemEmptySpotIndexs(&itemIndexs);

	return itemIndexs.size();
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

QVector<long long> itemGetIndexs(long long currentIndex, sol::object oitemnames, sol::object oitemmemos, bool includeEequip, sol::object ostartFrom)
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
	if (startFrom != -1)
		min += startFrom;

	long long max = sa::MAX_ITEM;
	if (includeEequip)
		min = 0;

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
	if (oitemmemos.is<bool>())
		includeEequip = oincludeEequip.as<bool>();

	long long c = 0;
	QVector<long long> itemIndexs = itemGetIndexs(index_, oitemnames, oitemmemos, includeEequip, ostartFrom);
	if (itemIndexs.isEmpty())
		return c;

	QHash<long long, sa::item_t> items = gamedevice.worker->getItems();
	for (const long long itemIndex : itemIndexs)
	{
		sa::item_t item = items.value(itemIndex);
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

	sa::item_t item = gamedevice.worker->getItem(index);

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