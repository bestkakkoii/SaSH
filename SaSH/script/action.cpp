#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"
#include "map/mapanalyzer.h"

//action
int Interpreter::useitem(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString itemName;
	int target = 0;
	int min = 0, max = 14;
	if (!checkRange(TK, 1, &min, &max))
	{
		checkString(TK, 1, &itemName);

		if (itemName.isEmpty())
			return Parser::kArgError;
		target = -1;
		checkInt(TK, 2, &target);
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
				QHash<QString, int> hash = {
					{ u8"自己", 0},
					{ u8"戰寵", injector.server->pc.battlePetNo},
					{ u8"騎寵", injector.server->pc.ridePetNo},
					{ u8"隊長", 6},

					{ u8"自己", 0},
					{ u8"战宠", injector.server->pc.battlePetNo},
					{ u8"骑宠", injector.server->pc.ridePetNo},
					{ u8"队长", 6},

					{ u8"self", 0},
					{ u8"battlepet", injector.server->pc.battlePetNo},
					{ u8"ride", injector.server->pc.ridePetNo},
					{ u8"leader", 6},
				};

				for (int i = 0; i < MAX_PET; ++i)
				{
					hash.insert(u8"寵物" + QString::number(i + 1), i + 1);
					hash.insert(u8"宠物" + QString::number(i + 1), i + 1);
					hash.insert(u8"pet" + QString::number(i + 1), i + 1);
				}

				for (int i = 1; i < MAX_PARTY; ++i)
				{
					hash.insert(u8"隊員" + QString::number(i), i + 1 + MAX_PET);
					hash.insert(u8"队员" + QString::number(i), i + 1 + MAX_PET);
					hash.insert(u8"teammate" + QString::number(i), i + 1 + MAX_PET);
				}

				if (!hash.contains(targetTypeName))
					return Parser::kArgError;

				target = hash.value(targetTypeName, -1);
				if (target < 0)
					return Parser::kArgError;
			}
		}
	}
	else
	{
		min += CHAR_EQUIPPLACENUM;
		max += CHAR_EQUIPPLACENUM;

		checkInt(TK, 2, &target);

		checkString(TK, 3, &itemName);

		if (itemName.isEmpty())
			return Parser::kArgError;

	}

	int itemIndex = injector.server->getItemIndexByName(itemName);
	if (itemIndex == -1)
		return Parser::kNoChange;

	injector.server->useItem(itemIndex, target);
	return Parser::kNoChange;
}

int Interpreter::dropitem(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString tempName;
	checkString(TK, 1, &tempName);
	if (tempName.isEmpty())
		return Parser::kArgError;

	//指定丟棄白名單，位於白名單的物品不丟棄
	if (tempName == QString(u8"非"))
	{
		int min = 0, max = 14;
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError;

		min += CHAR_EQUIPPLACENUM;
		max += CHAR_EQUIPPLACENUM;

		QString itemNames;
		checkString(TK, 3, &itemNames);
		if (itemNames.isEmpty())
			return Parser::kArgError;

		QStringList itemNameList = itemNames.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNameList.isEmpty())
			return Parser::kArgError;

		QVector<int> indexs;
		for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
		{
			ITEM item = injector.server->pc.item[i];
			for (const QString name : itemNameList)
			{
				if (item.name == name)
				{
					if (!indexs.contains(i))
						indexs.append(i);
				}
				else if (name.startsWith(kFuzzyPrefix))
				{
					QString newName = name;
					newName.remove(0, 1);
					if (item.name.contains(newName))
					{
						if (!indexs.contains(i))
							indexs.append(i);
					}
				}
			}
		}

		//排除掉所有包含再indexs的
		for (int i = min; i <= max; ++i)
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
			return Parser::kArgError;

		int size = itemNameList.size();
		for (const QString name : itemNameList)
		{
			QVector<int> indexs;
			if (injector.server->getItemIndexsByName(name, "", &indexs))
			{
				for (const int it : indexs)
					injector.server->dropItem(it);
			}
		}

	}
	return Parser::kNoChange;
}

int Interpreter::playerrename(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString newName;
	checkString(TK, 1, &newName);

	injector.server->setPlayerFreeName(newName);

	return Parser::kNoChange;
}

int Interpreter::petrename(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int petIndex = -1;
	checkInt(TK, 1, &petIndex);
	petIndex -= 1;

	QString newName;
	checkString(TK, 2, &newName);

	injector.server->setPetFreeName(petIndex, newName);

	return Parser::kNoChange;
}

int Interpreter::setpetstate(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int petIndex = -1;
	checkInt(TK, 1, &petIndex);
	petIndex -= 1;

	QString stateStr;
	checkString(TK, 2, &stateStr);
	if (stateStr.isEmpty())
		stateStr = QString(u8"休息");

	PetState state = petStateMap.value(stateStr, PetState::kRest);

	injector.server->setPetState(petIndex, state);

	return Parser::kNoChange;
}

int Interpreter::droppet(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int petIndex = -1;
	checkInt(TK, 1, &petIndex);
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
			for (const int it : v)
				injector.server->dropPet(it);
		}
	}

	return Parser::kNoChange;
}

int Interpreter::buy(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int itemIndex = -1;
	checkInt(TK, 1, &itemIndex);
	itemIndex -= 1;

	int count = 0;
	checkInt(TK, 2, &count);

	if (itemIndex < 0 || count <= 0)
		return Parser::kArgError;

	QString npcName;
	checkString(TK, 3, &npcName);

	if (npcName.isEmpty())
		injector.server->buy(itemIndex, count);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
		{
			injector.server->buy(itemIndex, count, kDialogBuy, unit.id);
		}
	}

	return Parser::kNoChange;
}

int Interpreter::sell(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString name;
	checkString(TK, 1, &name);
	QStringList nameList = name.split(util::rexOR, Qt::SkipEmptyParts);
	if (nameList.isEmpty())
		return Parser::kArgError;

	QString npcName;
	checkString(TK, 2, &npcName);


	QVector<int> itemIndexs;
	for (const QString& it : nameList)
	{
		QVector<int> indexs;
		if (!injector.server->getItemIndexsByName(it, "", &indexs))
			continue;
		itemIndexs.append(indexs);
	}

	std::sort(itemIndexs.begin(), itemIndexs.end());
	auto it = std::unique(itemIndexs.begin(), itemIndexs.end());
	itemIndexs.erase(it, itemIndexs.end());

	if (npcName.isEmpty())
		injector.server->sell(itemIndexs);
	else
	{
		mapunit_t unit;
		if (injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
		{
			injector.server->sell(itemIndexs, kDialogSell, unit.id);

		}
	}

	return Parser::kNoChange;
}

int Interpreter::make(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError;

	injector.server->craft(util::kCraftItem, ingreNameList);

	return Parser::kNoChange;
}

int Interpreter::cook(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError;

	injector.server->craft(util::kCraftFood, ingreNameList);

	return Parser::kNoChange;
}

int Interpreter::learn(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int petIndex = 0;
	checkInt(TK, 1, &petIndex);
	if (petIndex <= 0 || petIndex >= 6)
		return Parser::kArgError;
	--petIndex;

	int skillIndex = 0;
	checkInt(TK, 2, &skillIndex);
	if (skillIndex <= 0)
		return Parser::kArgError;
	--skillIndex;


	int spot = 0;
	checkInt(TK, 3, &spot);
	if (spot <= 0 || spot > MAX_SKILL + 1)
		return Parser::kArgError;
	--spot;

	QString npcName;
	checkString(TK, 4, &npcName);


	int dialogid = -1;
	checkInt(TK, 5, &dialogid);

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
int Interpreter::join(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->setTeamState(true);

	return Parser::kNoChange;
}

int Interpreter::leave(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->setTeamState(false);

	return Parser::kNoChange;
}

int Interpreter::usemagic(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString magicName;
	checkString(TK, 1, &magicName);
	if (magicName.isEmpty())
		return Parser::kArgError;

	int target = -1;
	checkInt(TK, 2, &target);
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


			QHash<QString, int> hash = {
				{ u8"自己", 0},
				{ u8"戰寵", injector.server->pc.battlePetNo},
				{ u8"騎寵", injector.server->pc.ridePetNo},
				{ u8"隊長", 6},

				{ u8"自己", 0},
				{ u8"战宠", injector.server->pc.battlePetNo},
				{ u8"骑宠", injector.server->pc.ridePetNo},
				{ u8"队长", 6},

				{ u8"self", 0},
				{ u8"battlepet", injector.server->pc.battlePetNo},
				{ u8"ride", injector.server->pc.ridePetNo},
				{ u8"leader", 6},
			};

			for (int i = 0; i < MAX_PET; ++i)
			{
				hash.insert(u8"寵物" + QString::number(i + 1), i + 1);
				hash.insert(u8"宠物" + QString::number(i + 1), i + 1);
				hash.insert(u8"pet" + QString::number(i + 1), i + 1);
			}

			for (int i = 1; i < MAX_PARTY; ++i)
			{
				hash.insert(u8"隊員" + QString::number(i), i + 1 + MAX_PET);
				hash.insert(u8"队员" + QString::number(i), i + 1 + MAX_PET);
				hash.insert(u8"teammate" + QString::number(i), i + 1 + MAX_PET);
			}

			if (!hash.contains(targetTypeName))
				return Parser::kArgError;

			target = hash.value(targetTypeName, -1);
			if (target < 0)
				return Parser::kArgError;
		}
	}

	int magicIndex = injector.server->getMagicIndexByName(magicName);
	if (magicIndex < 0)
		return Parser::kArgError;

	injector.server->useMagic(magicIndex, target);

	return Parser::kNoChange;
}

int Interpreter::pickitem(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString dirStr;
	checkString(TK, 1, &dirStr);
	if (dirStr.isEmpty())
		return Parser::kArgError;

	if (dirStr.startsWith(u8"全"))
	{
		for (int i = 0; i < 7; ++i)
		{
			injector.server->setPlayerFaceDirection(i);
			injector.server->pickItem(i);
		}
	}
	else
	{
		DirType type = dirMap.value(dirStr, kDirNone);
		if (type == kDirNone)
			return Parser::kArgError;
		injector.server->setPlayerFaceDirection(type);
		injector.server->pickItem(type);
	}

	return Parser::kNoChange;
}

int Interpreter::depositgold(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	if (injector.server->pc.gold <= 0)
		return Parser::kNoChange;

	int gold;
	checkInt(TK, 1, &gold);

	int isPublic = 0;
	checkInt(TK, 2, &isPublic);

	if (gold <= 0)
		return Parser::kArgError;

	if (gold > injector.server->pc.gold)
		gold = injector.server->pc.gold;

	injector.server->depositGold(gold, isPublic > 0);

	return Parser::kNoChange;
}

int Interpreter::withdrawgold(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int gold;
	checkInt(TK, 1, &gold);

	int isPublic = 0;
	checkInt(TK, 2, &isPublic);

	injector.server->withdrawGold(gold, isPublic > 0);

	return Parser::kNoChange;
}


util::SafeHash<int, ITEM> recordedEquip_;
int Interpreter::recordequip(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	if (!injector.server->IS_ONLINE_FLAG)
		return Parser::kNoChange;

	recordedEquip_.clear();
	for (int i = 0; i < CHAR_EQUIPPLACENUM; ++i)
	{
		injector.server->announce(QObject::tr("record equip:[%1]%2").arg(i + 1).arg(injector.server->pc.item[i].name));
		recordedEquip_.insert(i, injector.server->pc.item[i]);
	}

	return Parser::kNoChange;
}

int Interpreter::wearequip(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	if (!injector.server->IS_ONLINE_FLAG)
		return Parser::kNoChange;

	for (int i = 0; i < CHAR_EQUIPPLACENUM; ++i)
	{
		ITEM item = injector.server->pc.item[i];
		ITEM recordedItem = recordedEquip_.value(i);
		if (recordedItem.useFlag == 0 || recordedItem.name.isEmpty())
			continue;

		if (item.name == recordedItem.name && item.memo == recordedItem.memo)
			continue;

		int itemIndex = injector.server->getItemIndexByName(recordedItem.name);
		if (itemIndex == -1)
			continue;

		injector.server->useItem(itemIndex, 0);
	}

	return Parser::kNoChange;
}

int Interpreter::unwearequip(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int part = CHAR_EQUIPNONE;

	if (!checkInt(TK, 1, &part) || part < 1)
	{
		QString partStr;
		checkString(TK, 1, &partStr);
		if (partStr.isEmpty())
			return Parser::kArgError;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			part = 100;
		}
		else
		{
			part = equipMap.value(partStr.toLower(), CHAR_EQUIPNONE);
			if (part == CHAR_EQUIPNONE)
				return Parser::kArgError;
		}
	}
	else
		--part;
	if (part < 100)
	{
		int spotIndex = injector.server->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return Parser::kNoChange;

		injector.server->swapItem(part, spotIndex);
	}
	else
	{
		QVector<int> v;
		if (!injector.server->getItemEmptySpotIndexs(&v))
			return Parser::kNoChange;

		for (int i = 0; i < CHAR_EQUIPPLACENUM; ++i)
		{
			if (v.isEmpty())
				break;

			int itemIndex = v.takeFirst();

			injector.server->swapItem(i, itemIndex);
		}
	}

	return Parser::kNoChange;
}

int Interpreter::petequip(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int petIndex = -1;
	if (!checkInt(TK, 1, &petIndex))
		return Parser::kArgError;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return Parser::kArgError;

	QString itemName;
	checkString(TK, 2, &itemName);
	if (itemName.isEmpty())
		return Parser::kArgError;

	int itemIndex = injector.server->getItemIndexByName(itemName);
	if (itemIndex != -1)
		injector.server->petitemswap(petIndex, itemIndex, -1);

	return Parser::kNoChange;
}

int Interpreter::petunequip(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int part = CHAR_EQUIPNONE;

	int petIndex = -1;
	if (!checkInt(TK, 1, &petIndex))
		return Parser::kArgError;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return Parser::kArgError;


	if (!checkInt(TK, 2, &part) || part < 1)
	{
		QString partStr;
		checkString(TK, 2, &partStr);
		if (partStr.isEmpty())
			return Parser::kArgError;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			part = 100;
		}
		else
		{
			part = petEquipMap.value(partStr.toLower(), PET_EQUIPNONE);
			if (part == PET_EQUIPNONE)
				return Parser::kArgError;
		}
	}
	else
		--part;
	if (part < 100)
	{
		int spotIndex = injector.server->getItemEmptySpotIndex();
		if (spotIndex == -1)
			return Parser::kNoChange;

		injector.server->petitemswap(petIndex, part, spotIndex);
	}
	else
	{
		QVector<int> v;
		if (!injector.server->getItemEmptySpotIndexs(&v))
			return Parser::kNoChange;

		for (int i = 0; i < CHAR_EQUIPPLACENUM; ++i)
		{
			if (v.isEmpty())
				break;

			int itemIndex = v.takeFirst();

			injector.server->petitemswap(petIndex, i, itemIndex);
		}
	}

	return Parser::kNoChange;
}

int Interpreter::depositpet(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int petIndex = 0;
	QString petName;
	if (!checkInt(TK, 1, &petIndex))
	{
		checkString(TK, 1, &petName);
		if (petName.isEmpty())
			return Parser::kArgError;
	}

	if (petIndex == 0 && petName.isEmpty())
		return Parser::kArgError;

	if (petIndex > 0)
		--petIndex;
	else
	{
		QVector<int> v;
		if (!injector.server->getPetIndexsByName(petName, &v))
			return Parser::kArgError;
		petIndex = v.first();
	}

	if (petIndex == -1)
		return Parser::kArgError;

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

int Interpreter::deposititem(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	int min = 0, max = 14;
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError;

	min += CHAR_EQUIPPLACENUM;
	max += CHAR_EQUIPPLACENUM;


	QString itemName;
	checkString(TK, 2, &itemName);

	if (!itemName.isEmpty() && TK.value(2).type != TK_FUZZY)
	{
		QStringList itemNames = itemName.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemNames.isEmpty())
			return Parser::kArgError;

		QVector<int> allv;
		for (const QString& name : itemNames)
		{
			QVector<int> v;
			if (!injector.server->getItemIndexsByName(name, "", &v))
				return Parser::kArgError;
			else
				allv.append(v);
		}

		std::sort(allv.begin(), allv.end());
		auto iter = std::unique(allv.begin(), allv.end());
		allv.erase(iter, allv.end());

		QVector<int> v;
		for (const int it : allv)
		{
			if (it < min || it > max)
				continue;

			injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->depositItem(it);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->press(1);
			waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
		}

	}
	else
	{
		for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
		{
			ITEM item = injector.server->pc.item[i];
			if (item.name.isEmpty() || item.useFlag == 0)
				continue;

			if (i < min || i > max)
				continue;

			injector.server->depositItem(i);
			injector.server->press(1);

		}
	}

	return Parser::kNoChange;
}

int Interpreter::withdrawpet(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError;

	int level = 0;
	checkInt(TK, 2, &level);

	int maxHp = 0;
	checkInt(TK, 3, &maxHp);

	for (;;)
	{
		QPair<int, QVector<bankpet_t>> bankPetList = injector.server->currentBankPetList;
		int button = bankPetList.first;
		if (button == 0)
			break;

		QVector<bankpet_t> petList = bankPetList.second;
		int petIndex = 0;
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

		if (button & BUTTON_NEXT)
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

int Interpreter::withdrawitem(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString itemName;
	checkString(TK, 1, &itemName);
	if (itemName.isEmpty())
		return Parser::kArgError;

	QString memo;
	checkString(TK, 2, &memo);

	QVector<ITEM> bankItemList = injector.server->currentBankItemList;

	int itemIndex = 0;
	bool bret = false;
	for (const ITEM& it : bankItemList)
	{
		if (!itemName.startsWith(kFuzzyPrefix))
		{
			if (it.name == itemName && memo.isEmpty())
				bret = true;
			if (it.name == itemName && !memo.isEmpty() && it.memo.contains(memo))
				bret = true;
		}
		else
		{
			QString newName = itemName.mid(1);
			if (it.name.contains(newName) && memo.isEmpty())
				bret = true;
			else if (it.name.contains(newName) && it.memo.contains(memo))
				bret = true;
		}

		if (bret)
			break;

		++itemIndex;
	}

	if (bret)
	{
		injector.server->IS_WAITFOR_DIALOG_FLAG = true;
		injector.server->withdrawItem(itemIndex);
		waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

		injector.server->IS_WAITFOR_DIALOG_FLAG = true;
		injector.server->press(1);
		waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	}

	return Parser::kNoChange;
}

int Interpreter::addpoint(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString pointName;
	checkString(TK, 1, &pointName);
	if (pointName.isEmpty())
		return Parser::kArgError;

	int max = 0;
	checkInt(TK, 2, &max);
	if (max <= 0)
		return Parser::kArgError;

	static const QHash<QString, int> hash = {
		{ u8"腕力", 0},
		{ u8"體力", 1},
		{ u8"耐力", 2},
		{ u8"速度", 3},

		{ u8"腕力", 0},
		{ u8"体力", 1},
		{ u8"耐力", 2},
		{ u8"速度", 3},

		{ u8"str", 0},
		{ u8"vit", 1},
		{ u8"tgh", 2},
		{ u8"dex", 3},
	};

	int point = hash.value(pointName.toLower(), -1);
	if (point == -1)
		return Parser::kArgError;

	injector.server->addPoint(point, max);

	return Parser::kNoChange;
}

int Interpreter::leftclick(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	injector.server->leftClick(p.x(), p.y());

	return Parser::kNoChange;
}

int Interpreter::rightclick(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	injector.server->rightClick(p.x(), p.y());

	return Parser::kNoChange;
}

int Interpreter::leftdoubleclick(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	injector.server->leftDoubleClick(p.x(), p.y());

	return Parser::kNoChange;
}

int Interpreter::mousedragto(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint pfrom;
	checkInt(TK, 1, &pfrom.rx());
	checkInt(TK, 2, &pfrom.ry());
	QPoint pto;
	checkInt(TK, 3, &pto.rx());
	checkInt(TK, 4, &pto.ry());

	injector.server->dragto(pfrom.x(), pfrom.y(), pto.x(), pto.y());

	return Parser::kNoChange;
}

int Interpreter::trade(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString name;
	checkString(TK, 1, &name);
	if (name.isEmpty())
		return Parser::kArgError;

	QString itemListStr;
	checkString(TK, 2, &itemListStr);

	QString petListStr;
	checkString(TK, 3, &petListStr);

	int gold = 0;
	if (!checkInt(TK, 4, &gold))
	{
		QString tmp;
		if (checkString(TK, 4, &tmp) && tmp == "all")
			gold = injector.server->pc.gold;
	}


	int timeout = 5000;
	checkInt(TK, 5, &timeout);

	mapunit_s unit;
	if (!injector.server->findUnit(name, util::OBJ_HUMAN, &unit))
		return Parser::kNoChange;

	QPoint dst;
	int dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(injector.server->nowFloor, injector.server->nowPoint, unit.p, &dst, true, unit.dir);
	if (dir == -1 || !findPath(dst, 1, 0, timeout))
		return Parser::kNoChange;

	injector.server->setPlayerFaceDirection(dir);

	if (!injector.server->tradeStart(name, timeout))
		return Parser::kNoChange;

	bool ok = false;
	if (!itemListStr.isEmpty())
	{
		QStringList itemIndexList = itemListStr.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (int i = 1; i <= 15; ++i)
				itemIndexList.append(QString::number(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			int min = 1;
			int max = 15;
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError;

			for (int i = min; i <= max; ++i)
				itemIndexList.append(QString::number(i));
		}

		QVector<int> itemIndexVec;
		for (const QString& itemIndex : itemIndexList)
		{
			bool bret = false;
			int index = itemIndex.toInt(&bret);
			--index;
			index += CHAR_EQUIPPLACENUM;
			if (bret && index >= CHAR_EQUIPPLACENUM && index < MAX_ITEM)
			{
				if (injector.server->pc.item[index].useFlag == 1)
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
			ok = true;
		}
		else
			return Parser::kArgError;
	}

	if (!petListStr.isEmpty())
	{
		QStringList petIndexList = petListStr.split(util::rexOR, Qt::SkipEmptyParts);
		if (itemListStr.toLower() == "all")
		{
			for (int i = 1; i <= MAX_PET; ++i)
				petIndexList.append(QString::number(i));
		}
		else if (itemListStr.count("-") == 1)
		{
			int min = 1;
			int max = MAX_PET;
			if (!checkRange(TK, 2, &min, &max))
				return Parser::kArgError;

			for (int i = min; i <= max; ++i)
				petIndexList.append(QString::number(i));
		}

		QVector<int> petIndexVec;
		for (const QString& petIndex : petIndexList)
		{
			bool bret = false;
			int index = petIndex.toInt(&bret);
			--index;

			if (bret && index >= 0 && index < MAX_PET)
			{
				if (injector.server->pet[index].useFlag == 1)
					petIndexVec.append(index);
			}
		}

		std::sort(petIndexVec.begin(), petIndexVec.end());
		auto it = std::unique(petIndexVec.begin(), petIndexVec.end());
		petIndexVec.erase(it, petIndexVec.end());

		if (!petIndexVec.isEmpty())
		{
			injector.server->tradeAppendPets(name, petIndexVec);
			ok = true;
		}
		else
			return Parser::kArgError;
	}

	if (gold > 0 && gold <= injector.server->pc.gold)
	{
		injector.server->tradeAppendGold(name, gold);
		ok = true;
	}

	injector.server->tradeComfirm(name);

	waitfor(timeout, [&injector]()
		{
			return injector.server->pc.trade_confirm >= 3;
		});

	injector.server->tradeComplete(name);

	waitfor(timeout, [&injector]()
		{
			return !injector.server->IS_TRADING;
		});

	return Parser::kNoChange;
}

///////////////////////////////////////////////////////////////
int Interpreter::ocr(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int debugmode = 0;
	checkInt(TK, 1, &debugmode);

	QString ret;
	if (injector.server->postGifCodeImage(&ret))
	{
		qDebug() << ret;
		if (!ret.isEmpty())
		{
			if (debugmode == 0)
				injector.server->inputtext(ret);
		}
	}

	return Parser::kNoChange;
}
