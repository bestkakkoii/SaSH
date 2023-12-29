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
#include <gamedevice.h>
#include "map/mapdevice.h"

//action
long long Interpreter::useitem(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

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
		min += sa::CHAR_EQUIPSLOT_COUNT;
		max += sa::CHAR_EQUIPSLOT_COUNT;

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


		return Parser::kNoChange;
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

	waitfor(500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return Parser::kNoChange;
}

long long Interpreter::dropitem(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString tempName;
	QString memo;
	long long itemIndex = -1;
	checkString(TK, 1, &tempName);
	checkInteger(TK, 1, &itemIndex);

	checkString(TK, 2, &memo);
	if (tempName.isEmpty() && memo.isEmpty() && itemIndex == -1)
	{
		gamedevice.worker->dropItem(-1);
		return Parser::kNoChange;
	}

	if (tempName == kFuzzyPrefix)
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

		gamedevice.worker->dropItem(itemIndex);
		return Parser::kNoChange;
	}

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	//指定丟棄白名單，位於白名單的物品不丟棄
	if (tempName == QString("非"))
	{
		long long min = 0, max = static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT - 1);
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError + 2ll;

		min += sa::CHAR_EQUIPSLOT_COUNT;
		max += sa::CHAR_EQUIPSLOT_COUNT;

		QString itemNames;
		checkString(TK, 3, &itemNames);
		if (itemNames.isEmpty())
			return Parser::kArgError + 3ll;

		QVector<long long> indexs;
		if (gamedevice.worker->getItemIndexsByName(itemNames, "", &indexs, min, max))
		{
			//排除掉所有包含在indexs的
			for (long long i = min; i <= max; ++i)
			{
				if (!indexs.contains(i))
				{
					gamedevice.worker->dropItem(i);
				}
			}
		}
	}
	else
	{
		long long onlyOne = 0;
		bool bOnlyOne = false;
		checkInteger(TK, 1, &onlyOne);
		if (onlyOne == 1)
			bOnlyOne = true;

		QString itemNames = tempName;

		QStringList itemNameList = itemNames.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNameList.isEmpty())
			return Parser::kArgError + 1ll;

		//long long size = itemNameList.size();
		for (const QString& name : itemNameList)
		{
			QVector<long long> indexs;
			bool ok = false;
			if (gamedevice.worker->getItemIndexsByName(name, memo, &indexs))
			{
				for (const long long it : indexs)
				{
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

	waitfor(500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return Parser::kNoChange;
}

long long Interpreter::trade(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
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

	long long gold = 0;
	if (!checkInteger(TK, 4, &gold))
	{
		QString tmp;
		if (checkString(TK, 4, &tmp) && tmp == "all")
			gold = gamedevice.worker->getCharacter().gold;
	}


	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 5, &timeout);

	sa::map_unit_t unit;
	if (!gamedevice.worker->findUnit(name, sa::kObjectHuman, &unit))
		return Parser::kNoChange;

	QPoint dst;
	AStarDevice astar;
	long long dir = gamedevice.worker->mapDevice.calcBestFollowPointByDstPoint(
		currentIndex, &astar, gamedevice.worker->getFloor(), gamedevice.worker->getPoint(), unit.p, &dst, true, unit.dir);
	if (dir == -1)
		return Parser::kNoChange;

	gamedevice.worker->setCharFaceDirection(dir);

	if (!gamedevice.worker->tradeStart(name, timeout))
		return Parser::kNoChange;

	//bool ok = false;
	if (!itemListStr.isEmpty())
	{
		QStringList itemIndexList = itemListStr.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (long long i = 1; i <= 15; ++i)
				itemIndexList.append(util::toQString(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			long long min = 1;
			long long max = static_cast<long long>(sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT);
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError + 2ll;

			for (long long i = min; i <= max; ++i)
				itemIndexList.append(util::toQString(i));
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
			gamedevice.worker->tradeAppendItems(name, itemIndexVec);
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
			for (long long i = 1; i <= sa::MAX_PET; ++i)
			{
				if (gamedevice.worker->getPet(i - 1).valid)
					petIndexList.append(util::toQString(i));
			}
		}
		else if (itemListStr.count("-") == 1)
		{
			long long min = 1;
			long long max = sa::MAX_PET;
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError + 2ll;

			for (long long i = min; i <= max; ++i)
				petIndexList.append(util::toQString(i));
		}

		QVector<long long> petIndexVec;
		for (const QString& petIndex : petIndexList)
		{
			bool bret = false;
			long long index = petIndex.toLongLong(&bret);
			--index;

			if (bret && index >= 0 && index < sa::MAX_PET)
			{
				sa::pet_t pet = gamedevice.worker->getPet(index);
				if (pet.valid)
					petIndexVec.append(index);
			}
		}

		std::sort(petIndexVec.begin(), petIndexVec.end());
		auto it = std::unique(petIndexVec.begin(), petIndexVec.end());
		petIndexVec.erase(it, petIndexVec.end());

		if (!petIndexVec.isEmpty())
		{
			gamedevice.worker->tradeAppendPets(name, petIndexVec);
			//ok = true;
		}
		else
			return Parser::kArgError + 3ll;
	}

	if (gold > 0 && gold <= gamedevice.worker->getCharacter().gold)
	{
		gamedevice.worker->tradeAppendGold(name, gold);
		//ok = true;
	}

	gamedevice.worker->tradeComfirm(name);

	waitfor(timeout, [&gamedevice]()
		{
			return gamedevice.worker->getCharacter().trade_confirm >= 3;
		});

	gamedevice.worker->tradeComplete(name);

	waitfor(timeout, [&gamedevice]()
		{
			return !gamedevice.worker->IS_TRADING.get();
		});

	return Parser::kNoChange;
}

long long Interpreter::usemagic(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString magicName;
	checkString(TK, 1, &magicName);
	if (magicName.isEmpty())
		return Parser::kArgError + 1ll;

	long long target = -1;
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
				return Parser::kArgError + 2ll;

			target = hash.value(targetTypeName, -1);
			if (target < 0)
				return Parser::kArgError + 2ll;
		}
	}

	long long magicIndex = gamedevice.worker->getMagicIndexByName(magicName);
	if (magicIndex < 0)
		return Parser::kArgError + 1ll;

	gamedevice.worker->useMagic(magicIndex, target);

	return Parser::kNoChange;
}

long long Interpreter::mail(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QVariant card;
	QString name;
	long long addrIndex = -1;
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
		if (addrIndex <= 0 || addrIndex >= sa::MAX_ADDRESS_BOOK)
			return Parser::kArgError + 1ll;
		--addrIndex;

		card = addrIndex;
	}

	QString text;
	checkString(TK, 2, &text);

	long long petIndex = -1;
	checkInteger(TK, 3, &petIndex);
	if (petIndex != -1)
	{
		--petIndex;
		if (petIndex < 0 || petIndex >= sa::MAX_PET)
			return Parser::kArgError + 3ll;
	}

	QString itemName = "";
	checkString(TK, 4, &itemName);

	QString itemMemo = "";
	checkString(TK, 5, &itemMemo);

	if (petIndex != -1 && itemMemo.isEmpty() && !itemName.isEmpty())
		return Parser::kArgError + 4ll;

	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();

	gamedevice.worker->mail(card, text, petIndex, itemName, itemMemo);

	waitfor(500, [&gamedevice]()->bool { return gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.get() <= 0; });
	gamedevice.worker->IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	return Parser::kNoChange;
}