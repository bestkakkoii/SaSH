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
				return Parser::kArgError;

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
	return Parser::kNoChange;
}

int Interpreter::dropitem(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

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

int Interpreter::buy(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

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
			injector.server->buy(itemIndex, count, 242, unit.id);
		}
	}
	return Parser::kNoChange;
}

int Interpreter::sell(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

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
			injector.server->sell(itemIndexs, 243, unit.id);
		}
	}

	return Parser::kNoChange;
}

int Interpreter::make(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError;

	injector.server->craft(util::kCraftItem, ingreNameList);

	return Parser::kNoChange;
}

int Interpreter::cook(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString ingreName;
	checkString(TK, 1, &ingreName);

	QStringList ingreNameList = ingreName.split(util::rexOR, Qt::SkipEmptyParts);
	if (ingreNameList.isEmpty())
		return Parser::kArgError;

	injector.server->craft(util::kCraftFood, ingreNameList);

	return Parser::kNoChange;
}

//group
int Interpreter::join(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	injector.server->setTeamState(true);

	return Parser::kNoChange;
}

int Interpreter::leave(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	injector.server->setTeamState(false);

	return Parser::kNoChange;
}

int Interpreter::usemagic(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

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
			return Parser::kArgError;

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

	int magicIndex = injector.server->getMagicIndexByName(magicName);
	if (magicIndex < 0)
		return Parser::kArgError;

	injector.server->useMagic(magicIndex, target);

	return Parser::kNoChange;
}