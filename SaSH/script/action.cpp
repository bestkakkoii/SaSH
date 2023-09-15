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
#include "interpreter.h"
#include "util.h"
#include "injector.h"
#include "map/mapanalyzer.h"

//action
qint64 Interpreter::useitem(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QHash<QString, qint64> hash = {
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

	for (qint64 i = 0; i < MAX_PET; ++i)
	{
		hash.insert("寵物" + QString::number(i + 1), i + 1);
		hash.insert("宠物" + QString::number(i + 1), i + 1);
		hash.insert("pet" + QString::number(i + 1), i + 1);
	}

	for (qint64 i = 1; i < MAX_PARTY; ++i)
	{
		hash.insert("隊員" + QString::number(i), i + 1 + MAX_PET);
		hash.insert("队员" + QString::number(i), i + 1 + MAX_PET);
		hash.insert("teammate" + QString::number(i), i + 1 + MAX_PET);
	}

	QString itemName;
	QString itemMemo;
	QStringList itemNames;
	QStringList itemMemos;

	qint64 target = 0;
	qint64 totalUse = 1;

	qint64 min = 0, max = static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	if (!checkRange(TK, 1, &min, &max))
	{
		checkString(TK, 1, &itemName);
		checkString(TK, 2, &itemMemo);
		if (itemName.isEmpty() && itemMemo.isEmpty())
			return Parser::kArgError + 1ll;

		itemNames = itemName.split(util::rexOR);
		itemMemos = itemMemo.split(util::rexOR);

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

				QVector<int> v;
				if (!injector.server->getItemIndexsByName(name, memo, &v, CHAR_EQUIPPLACENUM))
					continue;

				totalUse = target - 100;

				bool ok = false;
				PC pc = injector.server->getPC();
				for (const qint64& it : v)
				{
					ITEM item = pc.item[it];
					qint64 size = pc.item[it].stack;
					for (qint64 i = 0; i < size; ++i)
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

				QVector<int> v;
				if (!injector.server->getItemIndexsByName(name, memo, &v, CHAR_EQUIPPLACENUM))
					continue;

				qint64 totalUse = target - 100;

				bool ok = false;
				PC pc = injector.server->getPC();
				for (const qint64& it : v)
				{
					ITEM item = pc.item[it];
					qint64 size = pc.item[it].stack;
					for (qint64 i = 0; i < size; ++i)
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

			QVector<int> v;
			if (!injector.server->getItemIndexsByName(itemName, memo, &v, CHAR_EQUIPPLACENUM))
				continue;

			if (totalUse == 1)
			{
				qint64 itemIndex = v.front();
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

				qint64 itemIndex = v.takeFirst();
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

			QVector<int> v;
			if (!injector.server->getItemIndexsByName(itemName, memo, &v, CHAR_EQUIPPLACENUM))
				continue;

			if (totalUse == 1)
			{
				qint64 itemIndex = v.front();
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

				qint64 itemIndex = v.takeFirst();
				injector.server->useItem(itemIndex, target);
				--n;
			}
		}
	}
	return Parser::kNoChange;
}

qint64 Interpreter::dropitem(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString tempName;
	QString memo;
	qint64 itemIndex = -1;
	checkString(TK, 1, &tempName);
	checkInteger(TK, 1, &itemIndex);

	checkString(TK, 2, &memo);
	if (tempName.isEmpty() && memo.isEmpty() && itemIndex == -1)
		return Parser::kArgError + 1ll;
	else if (tempName.isEmpty() && memo.isEmpty()
		&& ((itemIndex >= 1 && itemIndex <= (MAX_ITEM - CHAR_EQUIPPLACENUM)) || (itemIndex >= 101 && itemIndex <= (CHAR_EQUIPPLACENUM + 100))))
	{
		if (itemIndex < 100)
			--itemIndex;
		else
			itemIndex -= 100;

		injector.server->dropItem(itemIndex);
		return Parser::kNoChange;
	}

	//指定丟棄白名單，位於白名單的物品不丟棄
	if (tempName == QString("非"))
	{
		qint64 min = 0, max = static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError + 2ll;

		min += CHAR_EQUIPPLACENUM;
		max += CHAR_EQUIPPLACENUM;

		QString itemNames;
		checkString(TK, 3, &itemNames);
		if (itemNames.isEmpty())
			return Parser::kArgError + 3ll;

		QVector<int> indexs;
		injector.server->getItemIndexsByName(itemNames, "", &indexs, min, max);

		//排除掉所有包含在indexs的
		for (qint64 i = min; i <= max; ++i)
		{
			if (!indexs.contains(i))
				injector.server->dropItem(i);
		}
	}
	else
	{
		QString itemNames = tempName;

		QStringList itemNameList = itemNames.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNameList.isEmpty())
			return Parser::kArgError + 1ll;

		//qint64 size = itemNameList.size();
		for (const QString& name : itemNameList)
		{
			QVector<int> indexs;
			if (injector.server->getItemIndexsByName(name, memo, &indexs))
			{
				for (const qint64 it : indexs)
					injector.server->dropItem(it);
			}
		}

	}
	return Parser::kNoChange;
}

qint64 Interpreter::swapitem(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 a = 0, b = 0;
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

	injector.server->swapItem(a, b);

	return Parser::kNoChange;
}

qint64 Interpreter::playerrename(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	QString newName;
	checkString(TK, 1, &newName);

	injector.server->setPlayerFreeName(newName);

	return Parser::kNoChange;
}

qint64 Interpreter::petrename(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	qint64 petIndex = -1;
	checkInteger(TK, 1, &petIndex);
	petIndex -= 1;

	QString newName;
	checkString(TK, 2, &newName);

	injector.server->setPetFreeName(petIndex, newName);

	return Parser::kNoChange;
}

qint64 Interpreter::setpetstate(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 petIndex = -1;
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

qint64 Interpreter::droppet(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 petIndex = -1;
	checkInteger(TK, 1, &petIndex);
	petIndex -= 1;

	QString petName;
	checkString(TK, 1, &petName);

	if (petIndex >= 0)
		injector.server->dropPet(petIndex);
	else if (!petName.isEmpty())
	{
		QVector<int> v;
		if (injector.server->getPetIndexsByName(petName, &v))
		{
			for (const qint64 it : v)
				injector.server->dropPet(it);
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::buy(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 itemIndex = -1;
	checkInteger(TK, 1, &itemIndex);
	itemIndex -= 1;

	if (itemIndex < 0)
		return Parser::kArgError + 1ll;

	qint64 count = 0;
	checkInteger(TK, 2, &count);

	if (count <= 0)
		return Parser::kArgError + 2ll;

	QString npcName;
	checkString(TK, 3, &npcName);

	qint64 dlgid = -1;
	checkInteger(TK, 4, &dlgid);

	if (npcName.isEmpty())
		injector.server->buy(itemIndex, count, dlgid);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
		{
			injector.server->buy(itemIndex, count, dlgid, unit.id);
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::sell(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString name;
	checkString(TK, 1, &name);
	QStringList nameList = name.split(util::rexOR, Qt::SkipEmptyParts);
	if (nameList.isEmpty())
		return Parser::kArgError + 1ll;

	QString npcName;
	checkString(TK, 2, &npcName);

	qint64 dlgid = -1;
	checkInteger(TK, 3, &dlgid);

	QVector<int> itemIndexs;
	for (const QString& it : nameList)
	{
		QVector<int> indexs;
		if (!injector.server->getItemIndexsByName(it, "", &indexs, CHAR_EQUIPPLACENUM))
			continue;
		itemIndexs.append(indexs);
	}

	std::sort(itemIndexs.begin(), itemIndexs.end());
	auto it = std::unique(itemIndexs.begin(), itemIndexs.end());
	itemIndexs.erase(it, itemIndexs.end());

	if (npcName.isEmpty())
		injector.server->sell(itemIndexs, dlgid);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
		{
			injector.server->sell(itemIndexs, dlgid, unit.id);
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::sellpet(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	mapunit_t unit;
	if (!injector.server->findUnit("宠物店", util::OBJ_NPC, &unit))
	{
		if (!injector.server->findUnit("寵物店", util::OBJ_NPC, &unit))
			return Parser::kNoChange;
	}

	//qint64 petIndex = -1;
	qint64 min = 1, max = MAX_PET;
	if (!checkRange(TK, 1, &min, &max))
	{
		min = 0;
		checkInteger(TK, 1, &min);
		if (min <= 0 || min > MAX_PET)
			return Parser::kArgError + 1ll;
		max = min;
	}

	for (qint64 petIndex = min; petIndex <= max; ++petIndex)
	{
		if (isInterruptionRequested())
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
				//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
				injector.server->press(BUTTON_YES, 263, unit.id);
				//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
				bret = true;
				break;
			}
			case 262:
			{
				//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
				injector.server->press(petIndex, 262, unit.id);
				injector.server->press(BUTTON_YES, 263, unit.id);
				bret = true;
				//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
				break;
			}
			default:
			{
				//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
				injector.server->press(3, 261, unit.id);
				injector.server->press(petIndex, 262, unit.id);
				injector.server->press(BUTTON_YES, 263, unit.id);
				bret = true;
				//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
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

qint64 Interpreter::make(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError + 1ll;

	injector.server->craft(util::kCraftItem, ingreNameList);

	return Parser::kNoChange;
}

qint64 Interpreter::cook(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError + 1ll;

	injector.server->craft(util::kCraftFood, ingreNameList);

	return Parser::kNoChange;
}

qint64 Interpreter::learn(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 petIndex = 0;
	checkInteger(TK, 1, &petIndex);
	if (petIndex <= 0 || petIndex >= 6)
		return Parser::kArgError + 1ll;
	--petIndex;

	qint64 skillIndex = 0;
	checkInteger(TK, 2, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError + 2ll;
	--skillIndex;


	qint64 spot = 0;
	checkInteger(TK, 3, &spot);
	if (spot <= 0 || spot > static_cast<qint64>(MAX_SKILL + 1))
		return Parser::kArgError + 3ll;
	--spot;

	QString npcName;
	checkString(TK, 4, &npcName);


	qint64 dialogid = -1;
	checkInteger(TK, 5, &dialogid);

	if (npcName.isEmpty())
		injector.server->learn(skillIndex, petIndex, spot);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
		{
			injector.server->learn(skillIndex, petIndex, spot, dialogid, unit.id);
		}
	}

	return Parser::kNoChange;
}

//group
qint64 Interpreter::join(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->setTeamState(true);

	return Parser::kNoChange;
}

qint64 Interpreter::leave(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->setTeamState(false);

	return Parser::kNoChange;
}

qint64 Interpreter::kick(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	//0解隊 1-4隊員
	qint64 partyIndex = -1;
	checkInteger(TK, 1, &partyIndex);
	if (partyIndex < 0 || partyIndex >= MAX_PARTY)
	{
		QString exceptName;
		checkString(TK, 1, &exceptName);
		if (!exceptName.isEmpty())
		{
			QStringList list = exceptName.split(util::rexOR, Qt::SkipEmptyParts);
			if (list.isEmpty() || list.size() > 4)
				return Parser::kArgError + 1ll;

			QVector<qint64> wrongIndex;

			bool bret = false;
			for (qint64 i = 1; i < MAX_PARTY; ++i)
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
				for (qint64 i : wrongIndex)
				{
					injector.server->kickteam(i);
				}
			}
		}
	}

	injector.server->kickteam(partyIndex);

	return Parser::kNoChange;
}

qint64 Interpreter::usemagic(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString magicName;
	checkString(TK, 1, &magicName);
	if (magicName.isEmpty())
		return Parser::kArgError + 1ll;

	qint64 target = -1;
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
			QHash<QString, qint64> hash = {
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

			for (qint64 i = 0; i < MAX_PET; ++i)
			{
				hash.insert("寵物" + QString::number(i + 1), i + 1);
				hash.insert("宠物" + QString::number(i + 1), i + 1);
				hash.insert("pet" + QString::number(i + 1), i + 1);
			}

			for (qint64 i = 1; i < MAX_PARTY; ++i)
			{
				hash.insert("隊員" + QString::number(i), i + 1 + MAX_PET);
				hash.insert("队员" + QString::number(i), i + 1 + MAX_PET);
				hash.insert("teammate" + QString::number(i), i + 1 + MAX_PET);
			}

			if (!hash.contains(targetTypeName))
				return Parser::kArgError + 2ll;

			target = hash.value(targetTypeName, -1);
			if (target < 0)
				return Parser::kArgError + 2ll;
		}
	}

	qint64 magicIndex = injector.server->getMagicIndexByName(magicName);
	if (magicIndex < 0)
		return Parser::kArgError + 1ll;

	injector.server->useMagic(magicIndex, target);

	return Parser::kNoChange;
}

qint64 Interpreter::pickitem(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

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
		for (qint64 i = 0; i < 7; ++i)
		{
			injector.server->setPlayerFaceDirection(i);
			injector.server->pickItem(i);
		}
	}
	else
	{
		DirType type = dirMap.value(dirStr, kDirNone);
		if (type == kDirNone)
			return Parser::kArgError + 1ll;
		injector.server->setPlayerFaceDirection(type);
		injector.server->pickItem(type);
	}

	return Parser::kNoChange;
}

qint64 Interpreter::depositgold(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	if (injector.server->getPC().gold <= 0)
		return Parser::kNoChange;

	qint64 gold;
	checkInteger(TK, 1, &gold);

	if (gold <= 0)
		return Parser::kArgError + 1ll;

	qint64 isPublic = 0;
	checkInteger(TK, 2, &isPublic);

	if (gold > injector.server->getPC().gold)
		gold = injector.server->getPC().gold;

	injector.server->depositGold(gold, isPublic > 0);

	return Parser::kNoChange;
}

qint64 Interpreter::withdrawgold(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 gold;
	checkInteger(TK, 1, &gold);

	qint64 isPublic = 0;
	checkInteger(TK, 2, &isPublic);

	injector.server->withdrawGold(gold, isPublic > 0);

	return Parser::kNoChange;
}

util::SafeHash<qint64, ITEM> recordedEquip_;
qint64 Interpreter::recordequip(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	recordedEquip_.clear();
	for (qint64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
	{
		injector.server->announce(QObject::tr("record equip:[%1]%2").arg(i + 1).arg(injector.server->getPC().item[i].name));
		recordedEquip_.insert(i, injector.server->getPC().item[i]);
	}

	return Parser::kNoChange;
}

qint64 Interpreter::wearequip(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	if (!injector.server->getOnlineFlag())
		return Parser::kNoChange;

	for (qint64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
	{
		ITEM item = injector.server->getPC().item[i];
		ITEM recordedItem = recordedEquip_.value(i);
		if (!recordedItem.valid || recordedItem.name.isEmpty())
			continue;

		if (item.name == recordedItem.name && item.memo == recordedItem.memo)
			continue;

		qint64 itemIndex = injector.server->getItemIndexByName(recordedItem.name, true, "", CHAR_EQUIPPLACENUM);
		if (itemIndex == -1)
			continue;

		injector.server->useItem(itemIndex, 0);
	}

	return Parser::kNoChange;
}

qint64 Interpreter::unwearequip(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 part = CHAR_EQUIPNONE;

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
	if (part < 100)
	{
		qint64 spotIndex = injector.server->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return Parser::kNoChange;

		injector.server->swapItem(part, spotIndex);
	}
	else
	{
		QVector<int> v;
		if (!injector.server->getItemEmptySpotIndexs(&v))
			return Parser::kNoChange;

		for (qint64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
		{
			if (v.isEmpty())
				break;

			qint64 itemIndex = v.takeFirst();

			injector.server->swapItem(i, itemIndex);
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::petequip(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 petIndex = -1;
	if (!checkInteger(TK, 1, &petIndex))
		return Parser::kArgError + 1ll;

	--petIndex;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return Parser::kArgError + 1ll;

	QString itemName;
	checkString(TK, 2, &itemName);
	if (itemName.isEmpty())
		return Parser::kArgError;

	qint64 itemIndex = injector.server->getItemIndexByName(itemName, true, "", CHAR_EQUIPPLACENUM);
	if (itemIndex != -1)
		injector.server->petitemswap(petIndex, itemIndex, -1);

	return Parser::kNoChange;
}

qint64 Interpreter::petunequip(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 part = CHAR_EQUIPNONE;

	qint64 petIndex = -1;
	if (!checkInteger(TK, 1, &petIndex))
		return Parser::kArgError + 1l;

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
	if (part < 100)
	{
		qint64 spotIndex = injector.server->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return Parser::kNoChange;

		injector.server->petitemswap(petIndex, part, spotIndex);
	}
	else
	{
		QVector<int> v;
		if (!injector.server->getItemEmptySpotIndexs(&v))
			return Parser::kNoChange;

		for (qint64 i = 0; i < CHAR_EQUIPPLACENUM; ++i)
		{
			if (v.isEmpty())
				break;

			qint64 itemIndex = v.takeFirst();

			injector.server->petitemswap(petIndex, i, itemIndex);
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::depositpet(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 petIndex = 0;
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
		QVector<int> v;
		if (!injector.server->getPetIndexsByName(petName, &v))
			return Parser::kArgError + 1ll;
		petIndex = v.first();
	}

	if (petIndex == -1)
		return Parser::kArgError + 1ll;

	injector.server->IS_WAITFOR_DIALOG_FLAG = true;
	injector.server->depositPet(petIndex);
	waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	injector.server->IS_WAITFOR_DIALOG_FLAG = true;
	injector.server->press(BUTTON_YES);
	waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	injector.server->IS_WAITFOR_DIALOG_FLAG = true;
	injector.server->press(BUTTON_OK);
	waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	return Parser::kNoChange;
}

qint64 Interpreter::deposititem(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 min = 0, max = static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError + 1ll;

	min += CHAR_EQUIPPLACENUM;
	max += CHAR_EQUIPPLACENUM;

	QString itemName;
	checkString(TK, 2, &itemName);

	if (!itemName.isEmpty() && TK.value(2).type != TK_FUZZY)
	{
		QStringList itemNames = itemName.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNames.isEmpty())
			return Parser::kArgError + 2ll;

		QVector<int> allv;
		for (const QString& name : itemNames)
		{
			QVector<int> v;
			if (!injector.server->getItemIndexsByName(name, "", &v, CHAR_EQUIPPLACENUM))
				return Parser::kArgError;
			else
				allv.append(v);
		}

		std::sort(allv.begin(), allv.end());
		auto iter = std::unique(allv.begin(), allv.end());
		allv.erase(iter, allv.end());

		QVector<qint64> v;
		for (const qint64 it : allv)
		{
			if (it < min || it > max)
				continue;

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->depositItem(it);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			//injector.server->press(1);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
		}
	}
	else
	{
		for (qint64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
		{
			ITEM item = injector.server->getPC().item[i];
			if (item.name.isEmpty() || !item.valid)
				continue;

			if (i < min || i > max)
				continue;

			injector.server->depositItem(i);
			injector.server->press(1);
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::withdrawpet(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError + 1ll;

	qint64 level = 0;
	checkInteger(TK, 2, &level);

	qint64 maxHp = 0;
	checkInteger(TK, 3, &maxHp);

	for (;;)
	{
		QPair<qint64, QVector<bankpet_t>> bankPetList = injector.server->currentBankPetList;
		qint64 button = bankPetList.first;
		if (button == 0)
			break;

		QVector<bankpet_t> petList = bankPetList.second;
		qint64 petIndex = 0;
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
			injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->withdrawPet(petIndex);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->press(BUTTON_YES);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->press(BUTTON_OK);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
			break;
		}

		if ((button & BUTTON_NEXT) == BUTTON_NEXT)
		{
			injector.server->IS_WAITFOR_BANK_FLAG = true;
			injector.server->press(BUTTON_NEXT);
			if (!waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_BANK_FLAG; }))
				break;
		}
		else
			break;
	}

	return Parser::kNoChange;
}

qint64 Interpreter::withdrawitem(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString itemNames;
	checkString(TK, 1, &itemNames);
	QStringList itemList = itemNames.split(util::rexOR, Qt::SkipEmptyParts);
	qint64 itemListSize = itemList.size();

	QString memos;
	checkString(TK, 2, &memos);
	QStringList memoList = memos.split(util::rexOR, Qt::SkipEmptyParts);
	qint64 memoListSize = memoList.size();

	bool isAll = false;
	qint64 nisall = 0;
	checkInteger(TK, 3, &nisall);
	if (nisall > 1)
		isAll = true;

	if (itemListSize == 0 && memoListSize == 0)
		return Parser::kArgError + 1ll;

	qint64 max = 0;
	// max 以大於0且最小的為主 否則以不為0的為主
	if (itemListSize > 0)
		max = itemListSize;
	if (memoListSize > 0 && (max == 0 || memoListSize < max))
		max = memoListSize;

	QVector<ITEM> bankItemList = injector.server->currentBankItemList.toVector();

	for (int i = 0; i < max; ++i)
	{
		QString name = "";
		QString memo = "";
		if (!itemList.isEmpty())
			name = itemList.at(i);
		if (!memoList.isEmpty())
			memo = memoList.at(i);

		qint64 itemIndex = 0;
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
			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->withdrawItem(itemIndex);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			//injector.server->press(1);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

		}
	}
	return Parser::kNoChange;
}

qint64 Interpreter::addpoint(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString pointName;
	checkString(TK, 1, &pointName);
	if (pointName.isEmpty())
		return Parser::kArgError + 1ll;

	static const QHash<QString, qint64> hash = {
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

	qint64 point = hash.value(pointName.toLower(), -1);
	if (point == -1)
		return Parser::kArgError + 1ll;

	qint64 max = 0;
	checkInteger(TK, 2, &max);
	if (max <= 0)
		return Parser::kArgError + 2ll;

	injector.server->addPoint(point, max);

	return Parser::kNoChange;
}

qint64 Interpreter::leftclick(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 1, &x);
	checkInteger(TK, 2, &y);
	QPoint p(x, y);

	injector.leftClick(p.x(), p.y());

	return Parser::kNoChange;
}

qint64 Interpreter::rightclick(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 1, &x);
	checkInteger(TK, 2, &y);
	QPoint p(x, y);

	injector.rightClick(p.x(), p.y());

	return Parser::kNoChange;
}

qint64 Interpreter::leftdoubleclick(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 1, &x);
	checkInteger(TK, 2, &y);
	QPoint p(x, y);

	injector.leftDoubleClick(p.x(), p.y());

	return Parser::kNoChange;
}

qint64 Interpreter::mousedragto(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkBattleThenWait();

	qint64 xfrom = 0;
	qint64 yfrom = 0;
	checkInteger(TK, 1, &xfrom);
	checkInteger(TK, 2, &yfrom);
	QPoint pfrom(xfrom, yfrom);

	qint64 xto = 0;
	qint64 yto = 0;
	checkInteger(TK, 1, &xto);
	checkInteger(TK, 2, &yto);
	QPoint pto(xto, yto);

	injector.dragto(pfrom.x(), pfrom.y(), pto.x(), pto.y());

	return Parser::kNoChange;
}

qint64 Interpreter::trade(qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

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

	qint64 gold = 0;
	if (!checkInteger(TK, 4, &gold))
	{
		QString tmp;
		if (checkString(TK, 4, &tmp) && tmp == "all")
			gold = injector.server->getPC().gold;
	}


	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 5, &timeout);

	mapunit_s unit;
	if (!injector.server->findUnit(name, util::OBJ_HUMAN, &unit))
		return Parser::kNoChange;

	QPoint dst;
	qint64 dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(injector.server->nowFloor, injector.server->getPoint(), unit.p, &dst, true, unit.dir);
	if (dir == -1 || !findPath(currentLine, dst, 1, 0, timeout))
		return Parser::kNoChange;

	injector.server->setPlayerFaceDirection(dir);

	if (!injector.server->tradeStart(name, timeout))
		return Parser::kNoChange;

	//bool ok = false;
	if (!itemListStr.isEmpty())
	{
		QStringList itemIndexList = itemListStr.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (qint64 i = 1; i <= 15; ++i)
				itemIndexList.append(QString::number(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			qint64 min = 1;
			qint64 max = static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM);
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError + 2ll;

			for (qint64 i = min; i <= max; ++i)
				itemIndexList.append(QString::number(i));
		}

		QVector<int> itemIndexVec;
		for (const QString& itemIndex : itemIndexList)
		{
			bool bret = false;
			qint64 index = itemIndex.toLongLong(&bret);
			--index;
			index += CHAR_EQUIPPLACENUM;
			if (bret && index >= CHAR_EQUIPPLACENUM && index < MAX_ITEM)
			{
				if (injector.server->getPC().item[index].valid)
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
		QStringList petIndexList = petListStr.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (qint64 i = 1; i <= MAX_PET; ++i)
				petIndexList.append(QString::number(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			qint64 min = 1;
			qint64 max = MAX_PET;
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError + 2ll;

			for (qint64 i = min; i <= max; ++i)
				petIndexList.append(QString::number(i));
		}

		QVector<int> petIndexVec;
		for (const QString& petIndex : petIndexList)
		{
			bool bret = false;
			qint64 index = petIndex.toLongLong(&bret);
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
			return !injector.server->IS_TRADING;
		});

	return Parser::kNoChange;
}

qint64 Interpreter::mail(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QVariant card;
	QString name;
	qint64 addrIndex = -1;
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

	qint64 petIndex = -1;
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

	injector.server->mail(card, text, petIndex, itemName, itemMemo);

	return Parser::kNoChange;
}

qint64 Interpreter::doffstone(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 gold = 0;
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
qint64 Interpreter::bh(qint64, const TokenMap& TK)//atk
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;

	injector.server->sendBattlePlayerAttackAct(index);

	return Parser::kNoChange;
}
qint64 Interpreter::bj(qint64, const TokenMap& TK)//magic
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 magicIndex = 0;
	checkInteger(TK, 1, &magicIndex);
	if (magicIndex <= 0)
		return Parser::kArgError + 1ll;
	--magicIndex;

	qint64 target = 0;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 2ll;
	--target;

	injector.server->sendBattlePlayerMagicAct(magicIndex, target);

	return Parser::kNoChange;
}
qint64 Interpreter::bp(qint64, const TokenMap& TK)//skill
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 skillIndex = 0;
	checkInteger(TK, 1, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError + 1ll;
	--skillIndex;

	qint64 target = 0;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 2ll;
	--target;

	injector.server->sendBattlePlayerJobSkillAct(skillIndex, target);

	return Parser::kNoChange;
}
qint64 Interpreter::bs(qint64, const TokenMap& TK)//switch
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;
	injector.server->sendBattlePlayerSwitchPetAct(index);

	return Parser::kNoChange;
}
qint64 Interpreter::be(qint64, const TokenMap& TK)//escape
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	injector.server->sendBattlePlayerEscapeAct();

	return Parser::kNoChange;
}
qint64 Interpreter::bd(qint64, const TokenMap& TK)//defense
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	injector.server->sendBattlePlayerDefenseAct();

	return Parser::kNoChange;
}
qint64 Interpreter::bi(qint64, const TokenMap& TK)//item
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;
	index += CHAR_EQUIPPLACENUM;

	qint64 target = 1;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 2ll;
	--target;

	injector.server->sendBattlePlayerItemAct(index, target);

	return Parser::kNoChange;
}
qint64 Interpreter::bt(qint64, const TokenMap& TK)//catch
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 index = 0;
	checkInteger(TK, 1, &index);
	if (index <= 0)
		return Parser::kArgError + 1ll;
	--index;

	injector.server->sendBattlePlayerCatchPetAct(index);

	return Parser::kNoChange;
}
qint64 Interpreter::bn(qint64, const TokenMap& TK)//nothing
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	injector.server->sendBattlePlayerDoNothing();

	return Parser::kNoChange;
}
qint64 Interpreter::bw(qint64, const TokenMap& TK)//petskill
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 skillIndex = 0;
	checkInteger(TK, 1, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError + 1ll;
	--skillIndex;

	qint64 target = 0;
	checkInteger(TK, 2, &target);
	if (target <= 0)
		return Parser::kArgError + 1ll;
	--target;

	injector.server->sendBattlePetSkillAct(skillIndex, target);

	return Parser::kNoChange;
}
qint64 Interpreter::bwf(qint64, const TokenMap& TK)//pet nothing
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;
	injector.server->sendBattlePetDoNothing();
	return Parser::kNoChange;
}

qint64 Interpreter::bwait(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 1, &timeout);
	injector.sendMessage(Injector::kEnableBattleDialog, false, NULL);
	bool bret = waitfor(timeout, [&injector]()
		{
			if (!injector.server->getBattleFlag())
				return true;
			qint64 G = injector.server->getGameStatus();
			qint64 W = injector.server->getWorldStatus();

			return W == 10 && G == 4;
		});
	if (injector.server->getBattleFlag())
		injector.sendMessage(Injector::kEnableBattleDialog, true, NULL);
	else
		bret = false;

	return checkJump(TK, 2, bret, FailedJump);
}

qint64 Interpreter::bend(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	if (!injector.server->getBattleFlag())
		return Parser::kNoChange;


	qint64 G = injector.server->getGameStatus();
	if (G == 4)
	{
		mem::write<short>(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1);
		injector.server->setGameStatus(5);
		injector.server->isBattleDialogReady.store(false, std::memory_order_release);
	}
	return Parser::kNoChange;
}