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
import astar;
import Utility;
import Mem;
import Safe;
import String;
#include "stdafx.h"
#include "interpreter.h"

#include "injector.h"
#include "map/mapanalyzer.h"

//action
__int64 Interpreter::useitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QHash<QString, __int64> hash = {
		{ "自己", 0},
		{ "戰寵", injector.server->getPC().battlePetNo},
		{ "騎寵", injector.server->getPC().ridePetNo},
		{ "隊長", 6},

		{ "自己", 0},
		{ "战宠", injector.server->getPC().battlePetNo},
		{ "骑宠", injector.server->getPC().ridePetNo},
		{ "队长", 6},

		{ "self", 0},
		{ "battlepet", injector.server->getPC().battlePetNo},
		{ "ride", injector.server->getPC().ridePetNo},
		{ "leader", 6},
	};

	for (__int64 i = 0; i < MAX_PET; ++i)
	{
		hash.insert("寵物" + toQString(i + 1), i + 1);
		hash.insert("宠物" + toQString(i + 1), i + 1);
		hash.insert("pet" + toQString(i + 1), i + 1);
	}

	for (__int64 i = 1; i < MAX_PARTY; ++i)
	{
		hash.insert("隊員" + toQString(i), i + 1 + MAX_PET);
		hash.insert("队员" + toQString(i), i + 1 + MAX_PET);
		hash.insert("teammate" + toQString(i), i + 1 + MAX_PET);
	}

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	QString itemName;
	QString itemMemo;
	QStringList itemNames;
	QStringList itemMemos;

	__int64 target = 0;
	__int64 totalUse = 1;

	__int64 min = 0, max = static_cast<__int64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	if (!checkRange(TK, 1, &min, &max))
	{
		checkString(TK, 1, &itemName);
		checkString(TK, 2, &itemMemo);
		if (itemName.isEmpty() && itemMemo.isEmpty())
			return Parser::kArgError + 1ll;

		itemNames = itemName.split(rexOR);
		itemMemos = itemMemo.split(rexOR);

		target = -2;
		checkInteger(TK, 3, &target);
		if (target == -2)
		{
			QString targetTypeName;
			checkString(TK, 3, &targetTypeName);
			if (targetTypeName.isEmpty())
				target = 0;
			else
			{


				if (!hash.contains(targetTypeName))
					return Parser::kArgError + 3ll;

				target = hash.value(targetTypeName, -1);
				if (target < 0)
					return Parser::kArgError + 3ll;
			}
		}

		checkInteger(TK, 4, &totalUse);
		if (totalUse <= 0)
			return Parser::kArgError + 4ll;
	}
	else
	{
		min += CHAR_EQUIPPLACENUM;
		max += CHAR_EQUIPPLACENUM;

		target = -2;
		checkInteger(TK, 2, &target);
		if (target == -2)
		{
			QString targetTypeName;
			checkString(TK, 2, &targetTypeName);
			if (targetTypeName.isEmpty())
				target = 0;
			else
			{
				if (!hash.contains(targetTypeName))
					return Parser::kArgError + 2ll;

				target = hash.value(targetTypeName, -1);
				if (target < 0)
					return Parser::kArgError + 2ll;
			}
		}

		checkString(TK, 3, &itemName);
		checkString(TK, 4, &itemMemo);
		if (itemName.isEmpty() && itemMemo.isEmpty())
			return Parser::kArgError + 3ll;

		itemNames = itemName.split(rexOR);
		itemMemos = itemMemo.split(rexOR);

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

				QVector<__int64> v;
				if (!injector.server->getItemIndexsByName(name, memo, &v, CHAR_EQUIPPLACENUM))
					continue;

				totalUse = target - 100;

				bool ok = false;
				QHash<__int64, ITEM> items = injector.server->getItems();
				for (const __int64& it : v)
				{
					ITEM item = items.value(it);
					__int64 size = item.stack;
					for (__int64 i = 0; i < size; ++i)
					{
						injector.server->useItem(it, 0);
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

				QVector<__int64> v;
				if (!injector.server->getItemIndexsByName(name, memo, &v, CHAR_EQUIPPLACENUM))
					continue;

				totalUse = target - 100;

				bool ok = false;
				QHash<__int64, ITEM> items = injector.server->getItems();
				for (const __int64& it : v)
				{
					ITEM item = items.value(it);
					__int64 size = item.stack;
					for (__int64 i = 0; i < size; ++i)
					{
						injector.server->useItem(it, 0);
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


		return Parser::kNoChange;
	}

	if (itemNames.size() >= itemMemos.size())
	{
		for (const QString& name : itemNames)
		{
			QString memo;
			if (itemMemos.size() > 0)
				memo = itemMemos.takeFirst();

			QVector<__int64> v;
			if (!injector.server->getItemIndexsByName(name, memo, &v, CHAR_EQUIPPLACENUM))
				continue;

			if (totalUse == 1)
			{
				__int64 itemIndex = v.front();
				injector.server->useItem(itemIndex, target);
				break;
			}

			int n = totalUse;
			for (;;)
			{
				if (totalUse != -1 && n <= 0)
					break;

				if (v.size() == 0)
					break;

				__int64 itemIndex = v.takeFirst();
				injector.server->useItem(itemIndex, target);
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

			QVector<__int64> v;
			if (!injector.server->getItemIndexsByName(itemName, memo, &v, CHAR_EQUIPPLACENUM))
				continue;

			if (totalUse == 1)
			{
				__int64 itemIndex = v.front();
				injector.server->useItem(itemIndex, target);
				injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
				break;
			}

			int n = totalUse;
			for (;;)
			{
				if (totalUse != -1 && n <= 0)
					break;

				if (v.size() == 0)
					break;

				__int64 itemIndex = v.takeFirst();
				injector.server->useItem(itemIndex, target);
				injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
				--n;
			}
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::dropitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString tempName;
	QString memo;
	__int64 itemIndex = -1;
	checkString(TK, 1, &tempName);
	checkInteger(TK, 1, &itemIndex);

	checkString(TK, 2, &memo);
	if (tempName.isEmpty() && memo.isEmpty() && itemIndex == -1)
	{
		injector.server->dropItem(-1);
		return Parser::kNoChange;
	}

	if (tempName == kFuzzyPrefix)
	{
		for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
			injector.server->dropItem(i);
	}

	if (tempName.isEmpty() && memo.isEmpty()
		&& ((itemIndex >= 1 && itemIndex <= (MAX_ITEM - CHAR_EQUIPPLACENUM)) || (itemIndex >= 101 && itemIndex <= static_cast<__int64>(CHAR_EQUIPPLACENUM + 100))))
	{
		if (itemIndex < 100)
		{
			--itemIndex;
			itemIndex += CHAR_EQUIPPLACENUM;
		}
		else
			itemIndex -= 100;

		injector.server->dropItem(itemIndex);
		return Parser::kNoChange;
	}

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	//指定丟棄白名單，位於白名單的物品不丟棄
	if (tempName == QString("非"))
	{
		__int64 min = 0, max = static_cast<__int64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError + 2ll;

		min += CHAR_EQUIPPLACENUM;
		max += CHAR_EQUIPPLACENUM;

		QString itemNames;
		checkString(TK, 3, &itemNames);
		if (itemNames.isEmpty())
			return Parser::kArgError + 3ll;

		QVector<__int64> indexs;
		if (injector.server->getItemIndexsByName(itemNames, "", &indexs, min, max))
		{
			//排除掉所有包含在indexs的
			for (__int64 i = min; i <= max; ++i)
			{
				if (!indexs.contains(i))
				{
					injector.server->dropItem(i);
				}
			}
		}
	}
	else
	{
		__int64 onlyOne = 0;
		bool bOnlyOne = false;
		checkInteger(TK, 1, &onlyOne);
		if (onlyOne == 1)
			bOnlyOne = true;

		QString itemNames = tempName;

		QStringList itemNameList = itemNames.split(rexOR, Qt::SkipEmptyParts);
		if (itemNameList.isEmpty())
			return Parser::kArgError + 1ll;

		//__int64 size = itemNameList.size();
		for (const QString& name : itemNameList)
		{
			QVector<__int64> indexs;
			bool ok = false;
			if (injector.server->getItemIndexsByName(name, memo, &indexs))
			{
				for (const __int64 it : indexs)
				{
					injector.server->dropItem(it);
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
	qDebug() << injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET;
	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::swapitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 a = 0, b = 0;
	if (!checkInteger(TK, 1, &a))
		return Parser::kArgError + 1ll;
	if (!checkInteger(TK, 2, &b))
		return Parser::kArgError + 2ll;

	if (a > 100)
		a -= 100;
	else
		a += CHAR_EQUIPPLACENUM;
	if (b > 100)
		b -= 100;
	else
		b += CHAR_EQUIPPLACENUM;

	--a;
	--b;

	if (a < 0 || a >= MAX_ITEM)
		return Parser::kArgError + 1ll;

	if (b < 0 || b >= MAX_ITEM)
		return Parser::kArgError + 2ll;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	injector.server->swapItem(a, b);

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::playerrename(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	QString newName;
	checkString(TK, 1, &newName);

	injector.server->setCharFreeName(newName);

	return Parser::kNoChange;
}

__int64 Interpreter::petrename(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	__int64 petIndex = -1;
	if (!checkInteger(TK, 1, &petIndex))
		return Parser::kArgError + 1ll;
	petIndex -= 1;

	QString newName;
	checkString(TK, 2, &newName);

	injector.server->setPetFreeName(petIndex, newName);

	return Parser::kNoChange;
}

__int64 Interpreter::setpetstate(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 petIndex = -1;
	checkInteger(TK, 1, &petIndex);
	petIndex -= 1;
	if (petIndex < 0 || petIndex >= MAX_PET)
		return Parser::kArgError + 1ll;

	QString stateStr;
	checkString(TK, 2, &stateStr);
	if (stateStr.isEmpty())
		stateStr = QString("rest");

	PetState state = petStateMap.value(stateStr.toLower(), PetState::kRest);


	injector.server->setPetState(petIndex, state);

	return Parser::kNoChange;
}

__int64 Interpreter::droppet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 petIndex = -1;
	checkInteger(TK, 1, &petIndex);
	petIndex -= 1;

	QString petName;
	checkString(TK, 1, &petName);

	if (petIndex >= 0)
		injector.server->dropPet(petIndex);
	else if (!petName.isEmpty())
	{
		QVector<__int64> v;
		if (injector.server->getPetIndexsByName(petName, &v))
		{
			for (const __int64 it : v)
				injector.server->dropPet(it);
		}
	}

	return Parser::kNoChange;
}

__int64 Interpreter::buy(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 itemIndex = -1;
	checkInteger(TK, 1, &itemIndex);
	itemIndex -= 1;

	if (itemIndex < 0)
		return Parser::kArgError + 1ll;

	__int64 count = 0;
	checkInteger(TK, 2, &count);

	if (count <= 0)
		return Parser::kArgError + 2ll;

	QString npcName;
	checkString(TK, 3, &npcName);

	__int64 dlgid = -1;
	checkInteger(TK, 4, &dlgid);

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	if (npcName.isEmpty())
		injector.server->buy(itemIndex, count, dlgid);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, OBJ_NPC, &unit))
		{
			injector.server->buy(itemIndex, count, dlgid, unit.id);
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::sell(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString name;
	checkString(TK, 1, &name);
	QStringList nameList = name.split(rexOR, Qt::SkipEmptyParts);
	if (nameList.isEmpty())
		return Parser::kArgError + 1ll;

	QString npcName;
	checkString(TK, 2, &npcName);

	__int64 dlgid = -1;
	checkInteger(TK, 3, &dlgid);

	QVector<__int64> itemIndexs;
	for (const QString& it : nameList)
	{
		QVector<__int64> indexs;
		if (!injector.server->getItemIndexsByName(it, "", &indexs, CHAR_EQUIPPLACENUM))
			continue;
		itemIndexs.append(indexs);
	}

	std::sort(itemIndexs.begin(), itemIndexs.end());
	auto it = std::unique(itemIndexs.begin(), itemIndexs.end());
	itemIndexs.erase(it, itemIndexs.end());

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	if (npcName.isEmpty())
		injector.server->sell(itemIndexs, dlgid);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, OBJ_NPC, &unit))
		{
			injector.server->sell(itemIndexs, dlgid, unit.id);
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::sellpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	mapunit_t unit;
	if (!injector.server->findUnit("宠物店", OBJ_NPC, &unit))
	{
		if (!injector.server->findUnit("寵物店", OBJ_NPC, &unit))
			return Parser::kNoChange;
	}

	//__int64 petIndex = -1;
	__int64 min = 1, max = MAX_PET;
	if (!checkRange(TK, 1, &min, &max))
	{
		min = 0;
		checkInteger(TK, 1, &min);
		if (min <= 0 || min > MAX_PET)
			return Parser::kArgError + 1ll;
		max = min;
	}

	for (__int64 petIndex = min; petIndex <= max; ++petIndex)
	{
		if (isInterruptionRequested())
			return Parser::kNoChange;

		if (injector.IS_SCRIPT_INTERRUPT.load(std::memory_order_acquire))
			return Parser::kNoChange;

		if (injector.server.isNull())
			return Parser::kServerNotReady;

		if (petIndex - 1 < 0 || petIndex - 1 >= MAX_PET)
			return Parser::kArgError + 1ll;

		PET pet = injector.server->getPet(petIndex - 1);

		if (!pet.valid)
			continue;

		bool bret = false;
		for (;;)
		{
			if (isInterruptionRequested())
				return Parser::kNoChange;

			if (injector.server.isNull())
				return Parser::kServerNotReady;

			dialog_t dialog = injector.server->currentDialog;
			switch (dialog.dialogid)
			{
			case 263:
			{
				//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
				injector.server->press(BUTTON_YES, 263, unit.id);
				//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
				bret = true;
				break;
			}
			case 262:
			{
				//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
				injector.server->press(petIndex, 262, unit.id);
				injector.server->press(BUTTON_YES, 263, unit.id);
				bret = true;
				//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
				break;
			}
			default:
			{
				//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
				injector.server->press(3, 261, unit.id);
				injector.server->press(petIndex, 262, unit.id);
				injector.server->press(BUTTON_YES, 263, unit.id);
				bret = true;
				//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
				break;
			}
			}

			if (bret)
				break;

			//QThread::msleep(300);
		}
	}


	return Parser::kNoChange;
}

__int64 Interpreter::make(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError + 1ll;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	injector.server->craft(kCraftItem, ingreNameList);

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::cook(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError + 1ll;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	injector.server->craft(kCraftFood, ingreNameList);

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::learn(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 petIndex = 0;
	checkInteger(TK, 1, &petIndex);
	if (petIndex <= 0 || petIndex >= 6)
		return Parser::kArgError + 1ll;
	--petIndex;

	__int64 skillIndex = 0;
	checkInteger(TK, 2, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError + 2ll;
	--skillIndex;


	__int64 spot = 0;
	checkInteger(TK, 3, &spot);
	if (spot <= 0 || spot > static_cast<__int64>(MAX_SKILL + 1))
		return Parser::kArgError + 3ll;
	--spot;

	QString npcName;
	checkString(TK, 4, &npcName);


	__int64 dialogid = -1;
	checkInteger(TK, 5, &dialogid);

	if (npcName.isEmpty())
		injector.server->learn(skillIndex, petIndex, spot);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, OBJ_NPC, &unit))
		{
			injector.server->learn(skillIndex, petIndex, spot, dialogid, unit.id);
		}
	}

	return Parser::kNoChange;
}

//group
__int64 Interpreter::join(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->setTeamState(true);

	return Parser::kNoChange;
}

__int64 Interpreter::leave(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->setTeamState(false);

	return Parser::kNoChange;
}

__int64 Interpreter::kick(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	//0解隊 1-4隊員
	__int64 partyIndex = -1;
	checkInteger(TK, 1, &partyIndex);
	if (partyIndex < 0 || partyIndex >= MAX_PARTY)
	{
		QString exceptName;
		checkString(TK, 1, &exceptName);
		if (!exceptName.isEmpty())
		{
			QStringList list = exceptName.split(rexOR, Qt::SkipEmptyParts);
			if (list.isEmpty() || list.size() > 4)
				return Parser::kArgError + 1ll;

			QVector<__int64> wrongIndex;

			bool bret = false;
			for (__int64 i = 1; i < MAX_PARTY; ++i)
			{
				PARTY party = injector.server->getParty(i);
				if (!party.valid)
					continue;

				for (QString it : list)
				{
					bool isExact = true;
					QString newName = it.trimmed();
					if (newName.startsWith(kFuzzyPrefix))
					{
						newName = newName.mid(1);
						isExact = false;
					}

					if (isExact && party.name == newName)
					{
						bret = true;
					}
					else if (!isExact && party.name.contains(newName))
					{
						bret = true;
					}

				}

				if (!bret)
				{
					wrongIndex.push_back(i);
				}
			}

			if (!wrongIndex.isEmpty())
			{
				for (__int64 i : wrongIndex)
				{
					injector.server->kickteam(i);
				}
			}
		}
	}

	injector.server->kickteam(partyIndex);

	return Parser::kNoChange;
}

__int64 Interpreter::usemagic(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString magicName;
	checkString(TK, 1, &magicName);
	if (magicName.isEmpty())
		return Parser::kArgError + 1ll;

	__int64 target = -1;
	checkInteger(TK, 2, &target);
	if (target < 0)
	{
		QString targetTypeName;
		checkString(TK, 2, &targetTypeName);
		if (targetTypeName.isEmpty())
		{
			target = 0;

		}
		else
		{
			QHash<QString, __int64> hash = {
				{ "自己", 0},
				{ "戰寵", injector.server->getPC().battlePetNo},
				{ "騎寵", injector.server->getPC().ridePetNo},
				{ "隊長", 6},

				{ "自己", 0},
				{ "战宠", injector.server->getPC().battlePetNo},
				{ "骑宠", injector.server->getPC().ridePetNo},
				{ "队长", 6},

				{ "self", 0},
				{ "battlepet", injector.server->getPC().battlePetNo},
				{ "ride", injector.server->getPC().ridePetNo},
				{ "leader", 6},
			};

			for (__int64 i = 0; i < MAX_PET; ++i)
			{
				hash.insert("寵物" + toQString(i + 1), i + 1);
				hash.insert("宠物" + toQString(i + 1), i + 1);
				hash.insert("pet" + toQString(i + 1), i + 1);
			}

			for (__int64 i = 1; i < MAX_PARTY; ++i)
			{
				hash.insert("隊員" + toQString(i), i + 1 + MAX_PET);
				hash.insert("队员" + toQString(i), i + 1 + MAX_PET);
				hash.insert("teammate" + toQString(i), i + 1 + MAX_PET);
			}

			if (!hash.contains(targetTypeName))
				return Parser::kArgError + 2ll;

			target = hash.value(targetTypeName, -1);
			if (target < 0)
				return Parser::kArgError + 2ll;
		}
	}

	__int64 magicIndex = injector.server->getMagicIndexByName(magicName);
	if (magicIndex < 0)
		return Parser::kArgError + 1ll;

	injector.server->useMagic(magicIndex, target);

	return Parser::kNoChange;
}

__int64 Interpreter::pickitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString dirStr;
	checkString(TK, 1, &dirStr);
	if (dirStr.isEmpty())
		return Parser::kArgError + 1ll;

	if (dirStr.startsWith("全"))
	{
		for (__int64 i = 0; i < 7; ++i)
		{
			injector.server->setCharFaceDirection(i);
			injector.server->pickItem(i);
		}
	}
	else
	{
		DirType type = dirMap.value(dirStr, kDirNone);
		if (type == kDirNone)
			return Parser::kArgError + 1ll;
		injector.server->setCharFaceDirection(type);
		injector.server->pickItem(type);
	}

	return Parser::kNoChange;
}

__int64 Interpreter::depositgold(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	if (injector.server->getPC().gold <= 0)
		return Parser::kNoChange;

	__int64 gold;
	checkInteger(TK, 1, &gold);

	if (gold <= 0)
		return Parser::kArgError + 1ll;

	__int64 isPublic = 0;
	checkInteger(TK, 2, &isPublic);

	if (gold > injector.server->getPC().gold)
		gold = injector.server->getPC().gold;

	injector.server->depositGold(gold, isPublic > 0);

	return Parser::kNoChange;
}

__int64 Interpreter::withdrawgold(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 gold;
	checkInteger(TK, 1, &gold);

	__int64 isPublic = 0;
	checkInteger(TK, 2, &isPublic);

	injector.server->withdrawGold(gold, isPublic > 0);

	return Parser::kNoChange;
}

SafeHash<__int64, QHash<__int64, ITEM>> recordedEquip_;
__int64 Interpreter::recordequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();


	QHash<__int64, ITEM> items = injector.server->getItems();
	QHash<__int64, ITEM> recordedItems = recordedEquip_.value(currentIndex);
	for (__int64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
	{
		injector.server->announce(QObject::tr("record equip:[%1]%2").arg(i + 1).arg(items.value(i).name));
		recordedItems.insert(i, items.value(i));
	}
	recordedEquip_.insert(currentIndex, recordedItems);
	return Parser::kNoChange;
}

__int64 Interpreter::wearequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	if (!injector.server->getOnlineFlag())
		return Parser::kNoChange;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	QHash<__int64, ITEM> items = injector.server->getItems();
	for (__int64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
	{
		ITEM item = items.value(i);
		ITEM recordedItem = recordedEquip_.value(currentIndex).value(i);
		if (!recordedItem.valid || recordedItem.name.isEmpty())
			continue;

		if (item.name == recordedItem.name && item.memo == recordedItem.memo)
			continue;

		__int64 itemIndex = injector.server->getItemIndexByName(recordedItem.name, true, "", CHAR_EQUIPPLACENUM);
		if (itemIndex == -1)
			continue;

		injector.server->useItem(itemIndex, 0);
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::unwearequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 part = CHAR_EQUIPNONE;

	if (!checkInteger(TK, 1, &part) || part < 1)
	{
		QString partStr;
		checkString(TK, 1, &partStr);
		if (partStr.isEmpty())
			return Parser::kArgError + 1ll;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			part = 100;
		}
		else
		{
			part = equipMap.value(partStr.toLower(), CHAR_EQUIPNONE);
			if (part == CHAR_EQUIPNONE)
				return Parser::kArgError + 1ll;
		}
	}
	else
		--part;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	if (part < 100)
	{
		__int64 spotIndex = injector.server->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return Parser::kNoChange;

		injector.server->swapItem(part, spotIndex);
	}
	else
	{
		QVector<__int64> v;
		if (!injector.server->getItemEmptySpotIndexs(&v))
			return Parser::kNoChange;

		for (__int64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
		{
			if (v.isEmpty())
				break;

			__int64 itemIndex = v.takeFirst();

			injector.server->swapItem(i, itemIndex);
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::petequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 petIndex = -1;
	if (!checkInteger(TK, 1, &petIndex))
		return Parser::kArgError + 1ll;

	--petIndex;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return Parser::kArgError + 1ll;

	QString itemName;
	checkString(TK, 2, &itemName);
	if (itemName.isEmpty())
		return Parser::kArgError;

	__int64 itemIndex = injector.server->getItemIndexByName(itemName, true, "", CHAR_EQUIPPLACENUM);
	if (itemIndex != -1)
		injector.server->petitemswap(petIndex, itemIndex, -1);

	return Parser::kNoChange;
}

__int64 Interpreter::petunequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 part = CHAR_EQUIPNONE;

	__int64 petIndex = -1;
	if (!checkInteger(TK, 1, &petIndex))
		return Parser::kArgError + 1ll;

	--petIndex;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return Parser::kArgError + 1ll;


	if (!checkInteger(TK, 2, &part) || part < 1)
	{
		QString partStr;
		checkString(TK, 2, &partStr);
		if (partStr.isEmpty())
			return Parser::kArgError + 2ll;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			part = 100;
		}
		else
		{
			part = petEquipMap.value(partStr.toLower(), PET_EQUIPNONE);
			if (part == PET_EQUIPNONE)
				return Parser::kArgError + 2ll;
		}
	}
	else
		--part;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	if (part < 100)
	{
		__int64 spotIndex = injector.server->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return Parser::kNoChange;

		injector.server->petitemswap(petIndex, part, spotIndex);
	}
	else
	{
		QVector<__int64> v;
		if (!injector.server->getItemEmptySpotIndexs(&v))
			return Parser::kNoChange;

		for (__int64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
		{
			if (v.isEmpty())
				break;

			__int64 itemIndex = v.takeFirst();

			injector.server->petitemswap(petIndex, i, itemIndex);
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::depositpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 petIndex = 0;
	QString petName;
	if (!checkInteger(TK, 1, &petIndex))
	{
		checkString(TK, 1, &petName);
		if (petName.isEmpty())
			return Parser::kArgError + 1ll;
	}

	if (petIndex == 0 && petName.isEmpty())
		return Parser::kArgError + 1ll;

	if (petIndex > 0)
		--petIndex;
	else
	{
		QVector<__int64> v;
		if (!injector.server->getPetIndexsByName(petName, &v))
			return Parser::kArgError + 1ll;
		petIndex = v.first();
	}

	if (petIndex == -1)
		return Parser::kArgError + 1ll;

	injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
	injector.server->depositPet(petIndex);
	waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire); });

	injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
	injector.server->press(BUTTON_YES);
	waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire); });

	injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
	injector.server->press(BUTTON_OK);
	waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire); });

	injector.server->IS_WAITFOR_DIALOG_FLAG.store(false, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::deposititem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 min = 0, max = static_cast<__int64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError + 1ll;

	min += CHAR_EQUIPPLACENUM;
	max += CHAR_EQUIPPLACENUM;

	QString itemName;
	checkString(TK, 2, &itemName);

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	if (!itemName.isEmpty() && TK.value(2).type != TK_FUZZY)
	{
		QStringList itemNames = itemName.split(rexOR, Qt::SkipEmptyParts);
		if (itemNames.isEmpty())
			return Parser::kArgError + 2ll;

		QVector<__int64> allv;
		for (const QString& name : itemNames)
		{
			QVector<__int64> v;
			if (!injector.server->getItemIndexsByName(name, "", &v, CHAR_EQUIPPLACENUM))
				return Parser::kArgError;
			else
				allv.append(v);
		}

		std::sort(allv.begin(), allv.end());
		auto iter = std::unique(allv.begin(), allv.end());
		allv.erase(iter, allv.end());

		QVector<__int64> v;
		for (const __int64 it : allv)
		{
			if (it < min || it > max)
				continue;

			//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			injector.server->depositItem(it);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
			//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			//injector.server->press(1);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
		}
	}
	else
	{
		QHash<__int64, ITEM> items = injector.server->getItems();
		for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
		{
			ITEM item = items.value(i);
			if (item.name.isEmpty() || !item.valid)
				continue;

			if (i < min || i > max)
				continue;

			injector.server->depositItem(i);
			injector.server->press(1);
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::withdrawpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError + 1ll;

	__int64 level = 0;
	checkInteger(TK, 2, &level);

	__int64 maxHp = 0;
	checkInteger(TK, 3, &maxHp);

	for (;;)
	{
		QPair<__int64, QVector<bankpet_t>> bankPetList = injector.server->currentBankPetList;
		__int64 button = bankPetList.first;
		if (button == 0)
			break;

		QVector<bankpet_t> petList = bankPetList.second;
		__int64 petIndex = 0;
		bool bret = false;
		for (const bankpet_t& it : petList)
		{
			if (!petName.startsWith(kFuzzyPrefix))
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
			injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			injector.server->withdrawPet(petIndex);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire); });

			injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			injector.server->press(BUTTON_YES);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire); });

			injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			injector.server->press(BUTTON_OK);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire); });

			injector.server->IS_WAITFOR_DIALOG_FLAG.store(false, std::memory_order_release);
			break;
		}

		if ((button & BUTTON_NEXT) == BUTTON_NEXT)
		{
			injector.server->IS_WAITFOR_BANK_FLAG.store(true, std::memory_order_release);
			injector.server->press(BUTTON_NEXT);
			if (!waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_BANK_FLAG.load(std::memory_order_acquire); }))
			{
				injector.server->IS_WAITFOR_BANK_FLAG.store(false, std::memory_order_release);
				break;
			}
			injector.server->IS_WAITFOR_BANK_FLAG.store(false, std::memory_order_release);
		}
		else
			break;
	}


	return Parser::kNoChange;
}

__int64 Interpreter::withdrawitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString itemNames;
	checkString(TK, 1, &itemNames);
	QStringList itemList = itemNames.split(rexOR, Qt::SkipEmptyParts);
	__int64 itemListSize = itemList.size();

	QString memos;
	checkString(TK, 2, &memos);
	QStringList memoList = memos.split(rexOR, Qt::SkipEmptyParts);
	__int64 memoListSize = memoList.size();

	bool isAll = false;
	__int64 nisall = 0;
	checkInteger(TK, 3, &nisall);
	if (nisall > 1)
		isAll = true;

	if (itemListSize == 0 && memoListSize == 0)
		return Parser::kArgError + 1ll;

	__int64 max = 0;
	// max 以大於0且最小的為主 否則以不為0的為主
	if (itemListSize > 0)
		max = itemListSize;
	if (memoListSize > 0 && (max == 0 || memoListSize < max))
		max = memoListSize;

	QVector<ITEM> bankItemList = injector.server->currentBankItemList.toVector();

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	for (int i = 0; i < max; ++i)
	{
		QString name = "";
		QString memo = "";
		if (!itemList.isEmpty())
			name = itemList.value(i);
		if (!memoList.isEmpty())
			memo = memoList.value(i);

		__int64 itemIndex = 0;
		bool bret = false;
		for (const ITEM& it : bankItemList)
		{
			if (!name.isEmpty())
			{
				if (name.startsWith(kFuzzyPrefix))
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

		if (bret)
		{
			//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			injector.server->withdrawItem(itemIndex);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
			//injector.server->IS_WAITFOR_DIALOG_FLAG.store(true, std::memory_order_release);
			//injector.server->press(1);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG.load(std::memory_order_acquire) });
		}
	}

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::addpoint(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString pointName;
	checkString(TK, 1, &pointName);
	if (pointName.isEmpty())
		return Parser::kArgError + 1ll;

	static const QHash<QString, __int64> hash = {
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

	__int64 point = hash.value(pointName.toLower(), -1);
	if (point == -1)
		return Parser::kArgError + 1ll;

	__int64 max = 0;
	checkInteger(TK, 2, &max);
	if (max <= 0)
		return Parser::kArgError + 2ll;

	injector.server->addPoint(point, max);

	return Parser::kNoChange;
}

__int64 Interpreter::trade(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString name;
	checkString(TK, 1, &name);
	if (name.isEmpty())
		return Parser::kArgError + 1ll;

	QString itemListStr;
	checkString(TK, 2, &itemListStr);

	QString petListStr;
	checkString(TK, 3, &petListStr);

	__int64 gold = 0;
	if (!checkInteger(TK, 4, &gold))
	{
		QString tmp;
		if (checkString(TK, 4, &tmp) && tmp == "all")
			gold = injector.server->getPC().gold;
	}


	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 5, &timeout);

	mapunit_s unit;
	if (!injector.server->findUnit(name, OBJ_HUMAN, &unit))
		return Parser::kNoChange;

	QPoint dst;
	AStar::Device astar;
	__int64 dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(astar, injector.server->getFloor(), injector.server->getPoint(), unit.p, &dst, true, unit.dir);
	if (dir == -1)
		return Parser::kNoChange;

	injector.server->setCharFaceDirection(dir);

	if (!injector.server->tradeStart(name, timeout))
		return Parser::kNoChange;

	//bool ok = false;
	if (!itemListStr.isEmpty())
	{
		QStringList itemIndexList = itemListStr.split(rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (__int64 i = 1; i <= 15; ++i)
				itemIndexList.append(toQString(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			__int64 min = 1;
			__int64 max = static_cast<__int64>(MAX_ITEM - CHAR_EQUIPPLACENUM);
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError + 2ll;

			for (__int64 i = min; i <= max; ++i)
				itemIndexList.append(toQString(i));
		}

		QVector<__int64> itemIndexVec;
		for (const QString& itemIndex : itemIndexList)
		{
			bool bret = false;
			__int64 index = itemIndex.toLongLong(&bret);
			--index;
			index += CHAR_EQUIPPLACENUM;
			QHash<__int64, ITEM> items = injector.server->getItems();
			if (bret && index >= CHAR_EQUIPPLACENUM && index < MAX_ITEM)
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
			injector.server->tradeAppendItems(name, itemIndexVec);
			//ok = true;
		}
		else
			return Parser::kArgError + 2ll;
	}

	if (!petListStr.isEmpty())
	{
		QStringList petIndexList = petListStr.split(rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (__int64 i = 1; i <= MAX_PET; ++i)
				petIndexList.append(toQString(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			__int64 min = 1;
			__int64 max = MAX_PET;
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError + 2ll;

			for (__int64 i = min; i <= max; ++i)
				petIndexList.append(toQString(i));
		}

		QVector<__int64> petIndexVec;
		for (const QString& petIndex : petIndexList)
		{
			bool bret = false;
			__int64 index = petIndex.toLongLong(&bret);
			--index;

			if (bret && index >= 0 && index < MAX_PET)
			{
				PET pet = injector.server->getPet(index);
				if (pet.valid)
					petIndexVec.append(index);
			}
		}

		std::sort(petIndexVec.begin(), petIndexVec.end());
		auto it = std::unique(petIndexVec.begin(), petIndexVec.end());
		petIndexVec.erase(it, petIndexVec.end());

		if (!petIndexVec.isEmpty())
		{
			injector.server->tradeAppendPets(name, petIndexVec);
			//ok = true;
		}
		else
			return Parser::kArgError + 3ll;
	}

	if (gold > 0 && gold <= injector.server->getPC().gold)
	{
		injector.server->tradeAppendGold(name, gold);
		//ok = true;
	}

	injector.server->tradeComfirm(name);

	waitfor(timeout, [&injector]()
		{
			return injector.server->getPC().trade_confirm >= 3;
		});

	injector.server->tradeComplete(name);

	waitfor(timeout, [&injector]()
		{
			return !injector.server->IS_TRADING.load(std::memory_order_acquire);
		});

	return Parser::kNoChange;
}

__int64 Interpreter::mail(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QVariant card;
	QString name;
	__int64 addrIndex = -1;
	if (!checkInteger(TK, 1, &addrIndex))
	{
		if (!checkString(TK, 1, &name))
			return Parser::kArgError + 1ll;
		if (name.isEmpty())
			return Parser::kArgError + 1ll;

		card = name;
	}
	else
	{
		if (addrIndex <= 0 || addrIndex >= MAX_ADDRESS_BOOK)
			return Parser::kArgError + 1ll;
		--addrIndex;

		card = addrIndex;
	}

	QString text;
	checkString(TK, 2, &text);

	__int64 petIndex = -1;
	checkInteger(TK, 3, &petIndex);
	if (petIndex != -1)
	{
		--petIndex;
		if (petIndex < 0 || petIndex >= MAX_PET)
			return Parser::kArgError + 3ll;
	}

	QString itemName = "";
	checkString(TK, 4, &itemName);

	QString itemMemo = "";
	checkString(TK, 5, &itemMemo);

	if (petIndex != -1 && itemMemo.isEmpty() && !itemName.isEmpty())
		return Parser::kArgError + 4ll;

	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);

	injector.server->mail(card, text, petIndex, itemName, itemMemo);

	waitfor(500, [&injector]()->bool { return injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) <= 0; });
	injector.server->IS_WAITOFR_ITEM_CHANGE_PACKET.store(0, std::memory_order_release);
	return Parser::kNoChange;
}

__int64 Interpreter::doffstone(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 gold = 0;
	if (!checkInteger(TK, 1, &gold))
		return Parser::kArgError + 1ll;

	checkOnlineThenWait();
	checkBattleThenWait();

	if (gold == -1)
	{
		PC pc = injector.server->getPC();
		gold = pc.gold;
	}

	if (gold <= 0)
		return Parser::kArgError + 1ll;

	checkBattleThenWait();

	injector.server->dropGold(gold);

	return Parser::kNoChange;
}

//battle
__int64 Interpreter::bh(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//atk
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;

	injector.server->sendBattleCharAttackAct(index);

	return Parser::kNoChange;
}
__int64 Interpreter::bj(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//magic
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 magicIndex = 0;
	checkInteger(TK, 1, &magicIndex);
	if (magicIndex <= 0)
		return Parser::kArgError + 1ll;
	--magicIndex;

	__int64 target = 0;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 2ll;
	--target;

	injector.server->sendBattleCharMagicAct(magicIndex, target);

	return Parser::kNoChange;
}
__int64 Interpreter::bp(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//skill
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 skillIndex = 0;
	checkInteger(TK, 1, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError + 1ll;
	--skillIndex;

	__int64 target = 0;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 2ll;
	--target;

	injector.server->sendBattleCharJobSkillAct(skillIndex, target);

	return Parser::kNoChange;
}
__int64 Interpreter::bs(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//switch
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;
	injector.server->sendBattleCharSwitchPetAct(index);

	return Parser::kNoChange;
}
__int64 Interpreter::be(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//escape
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	injector.server->sendBattleCharEscapeAct();

	return Parser::kNoChange;
}
__int64 Interpreter::bd(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//defense
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	injector.server->sendBattleCharDefenseAct();

	return Parser::kNoChange;
}
__int64 Interpreter::bi(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//item
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;
	index += CHAR_EQUIPPLACENUM;

	__int64 target = 1;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 2ll;
	--target;

	injector.server->sendBattleCharItemAct(index, target);

	return Parser::kNoChange;
}
__int64 Interpreter::bt(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//catch
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;

	injector.server->sendBattleCharCatchPetAct(index);

	return Parser::kNoChange;
}
__int64 Interpreter::bn(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//nothing
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	injector.server->sendBattleCharDoNothing();

	return Parser::kNoChange;
}
__int64 Interpreter::bw(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//petskill
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 skillIndex = 0;
	checkInteger(TK, 1, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError + 1ll;
	--skillIndex;

	__int64 target = 0;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 1ll;
	--target;

	injector.server->sendBattlePetSkillAct(skillIndex, target);

	return Parser::kNoChange;
}
__int64 Interpreter::bwf(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)//pet nothing
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	injector.server->sendBattlePetDoNothing();
	return Parser::kNoChange;
}

__int64 Interpreter::bwait(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;


	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 1, &timeout);
	injector.sendMessage(kEnableBattleDialog, false, NULL);
	bool bret = waitfor(timeout, [&injector]()
		{
			if (!injector.server->getBattleFlag())
				return true;
			__int64 G = injector.server->getGameStatus();
			__int64 W = injector.server->getWorldStatus();

			return W == 10 && G == 4;
		});
	if (injector.server->getBattleFlag())
		injector.sendMessage(kEnableBattleDialog, true, NULL);
	else
		bret = false;

	return checkJump(TK, 2, bret, FailedJump);
}

__int64 Interpreter::bend(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 G = injector.server->getGameStatus();
	if (G == 4)
	{
		mem::write<short>(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1);
		injector.server->setGameStatus(5);
		injector.server->isBattleDialogReady.store(false, std::memory_order_release);
	}
	return Parser::kNoChange;
}