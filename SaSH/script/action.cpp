#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

//action
int Interpreter::useitem(const TokenMap& TK)
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
					{ QObject::tr("self"), 0 },
					{ QObject::tr("battlepet"), injector.server->pc.battlePetNo },
					{ QObject::tr("ride"), injector.server->pc.ridePetNo },
					{ QObject::tr("leader"), 6},
				};

				for (int i = 0; i < MAX_PET; ++i)
					hash.insert(QObject::tr("pet") + QString::number(i + 1), i + 1);

				for (int i = 1; i < MAX_PARTY; ++i)
					hash.insert(QObject::tr("teammate") + QString::number(i), i + 1 + MAX_PET);

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
		return Parser::kArgError;

	injector.server->useItem(itemIndex, target);
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::dropitem(const TokenMap& TK)
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

		QThread::msleep(200);
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

		QThread::msleep(200);
	}
	return Parser::kNoChange;
}

int Interpreter::playerrename(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString newName;
	checkString(TK, 1, &newName);

	injector.server->setPlayerFreeName(newName);

	return Parser::kNoChange;
}

int Interpreter::petrename(const TokenMap& TK)
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

int Interpreter::setpetstate(const TokenMap& TK)
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

int Interpreter::droppet(const TokenMap& TK)
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

	QThread::msleep(200);

	return Parser::kNoChange;
}

int Interpreter::buy(const TokenMap& TK)
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

	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::sell(const TokenMap& TK)
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

	QThread::msleep(200);

	return Parser::kNoChange;
}

int Interpreter::make(const TokenMap& TK)
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
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::cook(const TokenMap& TK)
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
	QThread::msleep(200);
	return Parser::kNoChange;
}

//group
int Interpreter::join(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->setTeamState(true);

	return Parser::kNoChange;
}

int Interpreter::leave(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->setTeamState(false);

	return Parser::kNoChange;
}

int Interpreter::usemagic(const TokenMap& TK)
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
				{ QObject::tr("self"), 0 },
				{ QObject::tr("battlepet"), injector.server->pc.battlePetNo },
				{ QObject::tr("ride"), injector.server->pc.ridePetNo },
				{ QObject::tr("leader"), 6},
			};

			for (int i = 0; i < MAX_PET; ++i)
				hash.insert(QObject::tr("pet") + QString::number(i + 1), i + 1);

			for (int i = 1; i < MAX_PARTY; ++i)
				hash.insert(QObject::tr("teammate") + QString::number(i), i + 1 + MAX_PET);

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

int Interpreter::pickitem(const TokenMap& TK)
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

int Interpreter::depositgold(const TokenMap& TK)
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

int Interpreter::withdrawgold(const TokenMap& TK)
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

int Interpreter::warp(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->warp();

	return Parser::kNoChange;
}

int Interpreter::leftclick(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	injector.server->leftCLick(p.x(), p.y());

	return Parser::kNoChange;
}

util::SafeHash<int, ITEM> recordedEquip_;
int Interpreter::recordequip(const TokenMap& TK)
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

int Interpreter::wearequip(const TokenMap& TK)
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
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::unwearequip(const TokenMap& TK)
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

		part = equipMap.value(partStr, CHAR_EQUIPNONE);
		if (part == CHAR_EQUIPNONE)
			return Parser::kArgError;
	}
	else
		--part;

	int spotIndex = injector.server->getItemEmptySpotIndex();
	if (spotIndex == -1)
		return Parser::kNoChange;

	injector.server->swapItem(part, spotIndex);
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::depositpet(const TokenMap& TK)
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

	//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
	injector.server->depositPet(petIndex);
	//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
	injector.server->press(BUTTON_YES);
	//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
	injector.server->press(BUTTON_OK);
	//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::deposititem(const TokenMap& TK)
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

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->depositItem(it);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->press(1);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
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
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::withdrawpet(const TokenMap& TK)
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

	QThread::msleep(100);

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
			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->withdrawPet(petIndex);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->press(BUTTON_YES);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

			//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
			injector.server->press(BUTTON_OK);
			//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });
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
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::withdrawitem(const TokenMap& TK)
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

	QThread::msleep(100);

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
		//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
		injector.server->withdrawItem(itemIndex);
		//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

		//injector.server->IS_WAITFOR_DIALOG_FLAG = true;
		injector.server->press(1);
		//waitfor(1000, [&injector]()->bool { return !injector.server->IS_WAITFOR_DIALOG_FLAG; });

	}
	QThread::msleep(200);
	return Parser::kNoChange;
}

int Interpreter::addpoint(const TokenMap& TK)
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
		{ QObject::tr("str"), 0 },
		{ QObject::tr("vit"), 1 },
		{ QObject::tr("tgh"), 2 },
		{ QObject::tr("dex"), 3 },
	};

	int point = hash.value(pointName, -1);
	if (point == -1)
		return Parser::kArgError;

	injector.server->addPoint(point, max);
	QThread::msleep(200);
	return Parser::kNoChange;
}