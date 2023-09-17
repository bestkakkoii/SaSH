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
#include "injector.h"
#include "signaldispatcher.h"

bool CLuaUtil::getSys(sol::table dstTable, sol::this_state)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	dstTable.clear();

	dstTable["game"] = injector.server->getGameStatus();
	dstTable["world"] = injector.server->getWorldStatus();

	return TRUE;
}

bool CLuaUtil::getMap(sol::table dstTable, sol::this_state)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	dstTable.clear();

	dstTable["floor"] = injector.server->nowFloor;
	dstTable["name"] = injector.server->nowFloorName.toUtf8().constData();

	QPoint pos = injector.server->getPoint();
	dstTable["x"] = pos.x();
	dstTable["y"] = pos.y();

	return TRUE;
}

bool CLuaUtil::getChar(sol::table dstTable, sol::this_state)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	PC pc = injector.server->getPC();


	//clear table
	dstTable.clear();

	//get data
	dstTable["modelid"] = pc.modelid;
	dstTable["faceid"] = pc.faceid;
	dstTable["unitid"] = pc.id;
	dstTable["dir"] = pc.dir + 1;
	dstTable["hp"] = pc.hp;
	dstTable["maxhp"] = pc.maxHp;
	dstTable["hpp"] = pc.hpPercent;
	dstTable["mp"] = pc.mp;
	dstTable["maxmp"] = pc.maxMp;
	dstTable["mpp"] = pc.mpPercent;
	dstTable["vit"] = pc.vit;
	dstTable["str"] = pc.str;
	dstTable["tgh"] = pc.tgh;
	dstTable["dex"] = pc.dex;
	dstTable["exp"] = pc.exp;
	dstTable["maxexp"] = pc.maxExp;
	dstTable["lv"] = pc.level;
	dstTable["atk"] = pc.atk;
	dstTable["def"] = pc.def;
	dstTable["agi"] = pc.agi;
	dstTable["chasma"] = pc.chasma;
	dstTable["luck"] = pc.luck;
	dstTable["earth"] = pc.earth;
	dstTable["water"] = pc.water;
	dstTable["fire"] = pc.fire;
	dstTable["wind"] = pc.wind;
	dstTable["gold"] = pc.gold;
	dstTable["title"] = pc.titleNo;
	dstTable["dp"] = pc.dp;
	dstTable["name"] = pc.name.toUtf8().constData();
	dstTable["freename"] = pc.freeName.toUtf8().constData();
	dstTable["namecolor"] = pc.nameColor;
	dstTable["status"] = pc.status;
	dstTable["etc"] = pc.etcFlag;
	dstTable["battlepet"] = pc.battlePetNo + 1;
	dstTable["mailpet"] = pc.mailPetNo + 1;
	dstTable["battleside"] = pc.sideNo + 1;
	dstTable["help"] = pc.helpMode;
	dstTable["turn"] = pc.transmigration;
	dstTable["family"] = pc.family.toUtf8().constData();
	dstTable["familyleader"] = pc.familyleader;
	dstTable["channel"] = pc.channel;
	dstTable["quickchannel"] = pc.quickChannel;
	dstTable["bankgold"] = pc.personal_bankgold;
	dstTable["ridepet"] = pc.ridePetNo;
	dstTable["learnride"] = pc.learnride;
	dstTable["lowstride"] = pc.lowsride;
	dstTable["ridepetname"] = pc.ridePetName.toUtf8().constData();
	dstTable["ridepetlv"] = pc.ridePetLevel;
	dstTable["familysprite"] = pc.familySprite;
	dstTable["basegrano"] = pc.baseGraNo;
	dstTable["big4fm"] = pc.big4fm;
	dstTable["tradeconfirm"] = pc.trade_confirm;
	dstTable["job"] = pc.profession_class;
	dstTable["skilllv"] = pc.profession_level;
	dstTable["skillpoint"] = pc.profession_skill_point;
	dstTable["jobname"] = pc.profession_class_name.toUtf8().constData();
	dstTable["maxload"] = pc.maxload;

	return TRUE;
}

bool CLuaUtil::getPet(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_PET; ++i)
	{
		PET pet = injector.server->getPet(i);

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["modelid"] = pet.modelid;
		dstTable[i + 1]["hp"] = pet.hp;
		dstTable[i + 1]["maxhp"] = pet.maxHp;
		dstTable[i + 1]["hpp"] = pet.hpPercent;
		dstTable[i + 1]["mp"] = pet.mp;
		dstTable[i + 1]["maxmp"] = pet.maxMp;
		dstTable[i + 1]["mpp"] = pet.mpPercent;
		dstTable[i + 1]["exp"] = pet.exp;
		dstTable[i + 1]["maxexp"] = pet.maxExp;
		dstTable[i + 1]["lv"] = pet.level;
		dstTable[i + 1]["atk"] = pet.atk;
		dstTable[i + 1]["def"] = pet.def;
		dstTable[i + 1]["agi"] = pet.agi;
		dstTable[i + 1]["loyal"] = pet.loyal;
		dstTable[i + 1]["earth"] = pet.earth;
		dstTable[i + 1]["water"] = pet.water;
		dstTable[i + 1]["fire"] = pet.fire;
		dstTable[i + 1]["wind"] = pet.wind;
		dstTable[i + 1]["maxskill"] = pet.maxSkill;
		dstTable[i + 1]["turn"] = pet.transmigration;
		dstTable[i + 1]["fusion"] = pet.fusion;
		dstTable[i + 1]["status"] = pet.status;
		dstTable[i + 1]["name"] = pet.name.toUtf8().constData();
		dstTable[i + 1]["freename"] = pet.freeName.toUtf8().constData();
		dstTable[i + 1]["valid"] = pet.valid;
		dstTable[i + 1]["changename"] = pet.changeNameFlag == 1;
		dstTable[i + 1]["state"] = pet.state + 1;
		dstTable[i + 1]["power"] = pet.power;
	}
	return TRUE;
}

bool CLuaUtil::getTeam(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_PARTY; ++i)
	{
		PARTY party = injector.server->getParty(i);

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["valid"] = party.valid;
		dstTable[i + 1]["id"] = party.id;
		dstTable[i + 1]["lv"] = party.level;
		dstTable[i + 1]["maxhp"] = party.maxHp;
		dstTable[i + 1]["hp"] = party.hp;
		dstTable[i + 1]["hpp"] = party.hpPercent;
		dstTable[i + 1]["mp"] = party.mp;
		dstTable[i + 1]["name"] = party.name.toUtf8().constData();
	}

	return TRUE;
}

bool CLuaUtil::getCard(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_ADDRESS_BOOK; ++i)
	{
		ADDRESS_BOOK adrBook = injector.server->getAddressBook(i);

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["valid"] = adrBook.valid;
		dstTable[i + 1]["online"] = adrBook.onlineFlag == 1;
		dstTable[i + 1]["lv"] = adrBook.level;
		dstTable[i + 1]["turn"] = adrBook.transmigration;
		dstTable[i + 1]["dp"] = adrBook.dp;
		dstTable[i + 1]["modelid"] = adrBook.modelid;
		dstTable[i + 1]["name"] = adrBook.name.toUtf8().constData();
	}

	return TRUE;
}

bool CLuaUtil::getChat(sol::table dstTable, sol::this_state)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_CHAT_HISTORY; ++i)
	{
		QString text = injector.server->getChatHistory(i);

		dstTable[i + 1] = text.toUtf8().constData();
	}

	return TRUE;
}

bool CLuaUtil::getDialog(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	QStringList dialogList = injector.server->currentDialog.get().linedatas;

	for (int i = 0; i < MAX_DIALOG_LINE; ++i)
	{
		if (i >= dialogList.size())
			break;

		dstTable[i + 1] = dialogList[i].toUtf8().constData();
	}

	dstTable["id"] = injector.server->currentDialog.get().dialogid;
	dstTable["unitid"] = injector.server->currentDialog.get().unitid;

	return TRUE;
}

bool CLuaUtil::getUnit(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	QList<mapunit_t> units = injector.server->mapUnitHash.values();

	int size = units.size();

	for (int i = 0; i < size; ++i)
	{
		mapunit_t unit = units[i];

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["unitid"] = unit.id;
		dstTable[i + 1]["modelid"] = unit.modelid;
		dstTable[i + 1]["x"] = unit.x;
		dstTable[i + 1]["y"] = unit.y;
		dstTable[i + 1]["dir"] = unit.dir + 1;
		dstTable[i + 1]["lv"] = unit.level;
		dstTable[i + 1]["namecolor"] = unit.nameColor;
		dstTable[i + 1]["name"] = unit.name.toUtf8().constData();
		dstTable[i + 1]["freename"] = unit.freeName.toUtf8().constData();
		dstTable[i + 1]["walkable"] = unit.walkable;
		dstTable[i + 1]["height"] = unit.height;
		dstTable[i + 1]["charnamecolor"] = unit.charNameColor;
		dstTable[i + 1]["family"] = unit.family.toUtf8().constData();
		dstTable[i + 1]["petname"] = unit.petname.toUtf8().constData();
		dstTable[i + 1]["petlv"] = unit.petlevel;
		dstTable[i + 1]["class"] = unit.classNo;
		dstTable[i + 1]["itemname"] = unit.item_name.toUtf8().constData();
		dstTable[i + 1]["gold"] = unit.gold;
		dstTable[i + 1]["job"] = unit.profession_class;
		dstTable[i + 1]["skilllv"] = unit.profession_level;
		dstTable[i + 1]["skillpoint"] = unit.profession_skill_point;
		dstTable[i + 1]["type"] = unit.objType;
		dstTable[i + 1]["visible"] = unit.isVisible;
	}

	return TRUE;
}

bool CLuaUtil::getBattleUnit(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	battledata_t battleData = injector.server->getBattleData();

	QVector<battleobject_t> battleObjects = battleData.objects;

	dstTable["field"] = battleData.fieldAttr;

	dstTable["count"] = battleData.enemies.size();

	dstTable["round"] = injector.server->battleCurrentRound.load(std::memory_order_acquire) + 1;

	dstTable["dura"] = static_cast<qint64>(injector.server->battleDurationTimer.elapsed() / 1000ll);

	dstTable["totaldura"] = static_cast<qint64>(injector.server->battle_total_time.load(std::memory_order_acquire) / 1000 / 60);

	dstTable["totalcombat"] = injector.server->battle_total.load(std::memory_order_acquire);

	for (int i = 0; i < MAX_ENEMY; ++i)
	{
		if (i >= battleObjects.size())
			break;

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		battleobject_t battleObject = battleObjects[i];

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["pos"] = battleObject.pos;
		dstTable[i + 1]["name"] = battleObject.name.toUtf8().constData();
		dstTable[i + 1]["freename"] = battleObject.freeName.toUtf8().constData();
		dstTable[i + 1]["modelid"] = battleObject.modelid;
		dstTable[i + 1]["lv"] = battleObject.level;
		dstTable[i + 1]["hp"] = battleObject.hp;
		dstTable[i + 1]["maxhp"] = battleObject.maxHp;
		dstTable[i + 1]["hpp"] = battleObject.hpPercent;
		dstTable[i + 1]["status"] = battleObject.status;
		dstTable[i + 1]["rideflag"] = battleObject.rideFlag > 0;
		dstTable[i + 1]["ridename"] = battleObject.rideName.toUtf8().constData();
		dstTable[i + 1]["ridelv"] = battleObject.rideLevel;
		dstTable[i + 1]["ridehp"] = battleObject.rideHp;
		dstTable[i + 1]["ridemaxhp"] = battleObject.rideMaxHp;
		dstTable[i + 1]["ridehpp"] = battleObject.rideHpPercent;
		dstTable[i + 1]["ready"] = battleObject.ready;
	}

	return TRUE;
}

bool CLuaUtil::getDaily(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_MISSION; ++i)
	{
		JOBDAILY jobDaily = injector.server->getJobDaily(i);

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["id"] = jobDaily.JobId;
		dstTable[i + 1]["explain"] = jobDaily.explain.toUtf8().constData();
		dstTable[i + 1]["state"] = jobDaily.state.toUtf8().constData();
	}

	return TRUE;
}

bool CLuaUtil::getItem(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	PC pc = injector.server->getPC();

	for (int i = 0; i < MAX_ITEM; ++i)
	{
		ITEM item = pc.item[i];

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["color"] = item.color;
		dstTable[i + 1]["modelid"] = item.modelid;
		dstTable[i + 1]["lv"] = item.level;
		dstTable[i + 1]["stack"] = item.stack;
		dstTable[i + 1]["alch"] = item.alch.toUtf8().constData();
		dstTable[i + 1]["valid"] = item.valid;
		dstTable[i + 1]["field"] = item.field;
		dstTable[i + 1]["target"] = item.target;
		dstTable[i + 1]["deadtarget"] = item.deadTargetFlag;
		dstTable[i + 1]["sendflag"] = item.sendFlag;
		dstTable[i + 1]["name"] = item.name.toUtf8().constData();
		dstTable[i + 1]["name2"] = item.name2.toUtf8().constData();
		dstTable[i + 1]["memo"] = item.memo.toUtf8().constData();

		QString damage = item.damage;
		damage.replace("%", "");
		damage.replace("％", "");
		bool ok = false;
		qint64 nDamage = damage.toLongLong(&ok);
		if (!ok && !damage.isEmpty())
			nDamage = 100;
		dstTable[i + 1]["damage"] = nDamage;

		dstTable[i + 1]["type"] = item.type;
		dstTable[i + 1]["jigsaw"] = item.jigsaw.toUtf8().constData();
		dstTable[i + 1]["maxstack"] = item.maxStack;
	}

	return TRUE;
}

bool CLuaUtil::getSkill(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_PROFESSION_SKILL; ++i)
	{
		PROFESSION_SKILL skill = injector.server->getSkill(i);

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["valid"] = skill.valid;
		dstTable[i + 1]["id"] = skill.skillId;
		dstTable[i + 1]["target"] = skill.target;
		dstTable[i + 1]["kind"] = skill.kind;
		dstTable[i + 1]["name"] = skill.name.toUtf8().constData();
		dstTable[i + 1]["memo"] = skill.memo.toUtf8().constData();
		dstTable[i + 1]["icon"] = skill.icon;
		dstTable[i + 1]["costmp"] = skill.costmp;
		dstTable[i + 1]["lv"] = skill.skill_level;
	}

	return TRUE;
}

bool CLuaUtil::getMagic(sol::table dstTable, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	//clear table
	dstTable.clear();

	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		MAGIC magic = injector.server->getMagic(i);

		if (!dstTable[i + 1].is<sol::table>())
			dstTable[i + 1] = lua.create_table();

		dstTable[i + 1]["index"] = i + 1;
		dstTable[i + 1]["valid"] = magic.valid;
		dstTable[i + 1]["costmp"] = magic.costmp;
		dstTable[i + 1]["field"] = magic.field;
		dstTable[i + 1]["target"] = magic.target;
		dstTable[i + 1]["deadtarget"] = magic.deadTargetFlag;
		dstTable[i + 1]["name"] = magic.name.toUtf8().constData();
		dstTable[i + 1]["memo"] = magic.memo.toUtf8().constData();
	}

	return TRUE;
}
