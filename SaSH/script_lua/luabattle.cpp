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

long long CLuaBattle::count()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->battle_total.get();
}

long long CLuaBattle::dura()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->battleDurationTimer.cost() / 1000.0;
}

long long CLuaBattle::time()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->battle_total_time.get() / 1000.0 / 60.0;

}

long long CLuaBattle::cost()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->battle_one_round_time.get() / 1000.0;
}

long long CLuaBattle::round()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	if (!gamedevice.worker->getOnlineFlag())
		return -1;

	if (!gamedevice.worker->getBattleFlag())
		return 0;

	return gamedevice.worker->battleCurrentRound.get() + 1;
}

std::string CLuaBattle::field()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return "";

	return util::toConstData(gamedevice.worker->getFieldString(gamedevice.worker->battleField.get()));
}

long long CLuaBattle::charpos()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	return static_cast<long long>(gamedevice.worker->battleCharCurrentPos.get() + 1);
}

long long CLuaBattle::petpos()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	long long petIndex = gamedevice.worker->battleCharCurrentPos.get() + 5;

	sa::battle_object_t obj = gamedevice.worker->getBattleData().objects.value(petIndex);

	return obj.level > 0 && obj.modelid > 0 ? petIndex + 1 : -1;
}

long long CLuaBattle::size()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->getBattleData().objects.size();
}

long long CLuaBattle::enemycount()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->getBattleData().enemies.size();
}

long long CLuaBattle::alliecount()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	return gamedevice.worker->getBattleData().allies.size();
}

sa::battle_object_t CLuaBattle::operator[](long long index)
{
	--index;

	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return sa::battle_object_t();

	return gamedevice.worker->getBattleData().objects.value(index);
}

long long CLuaBattle::charUseAttack(long long objIndex, sol::this_state s)//atk
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharAttackAct(--objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::charUseMagic(long long magicIndex, long long objIndex, sol::this_state s)//magic
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	--objIndex;
	if (objIndex < 0)
		objIndex = 0;

	return gamedevice.worker->sendBattleCharMagicAct(--magicIndex, objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::charUseMagic(std::string smagig, long long objIndex, sol::this_state s)//magic
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	long long index = gamedevice.worker->getMagicIndexByName(util::toQString(smagig));
	if (index < 0)
		return FALSE;

	--objIndex;
	if (objIndex < 0)
		objIndex = 0;

	return gamedevice.worker->sendBattleCharMagicAct(index, objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::charUseSkill(long long skillIndex, long long objIndex, sol::this_state s)//skill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	--objIndex;
	if (objIndex < 0)
		objIndex = 0;

	return gamedevice.worker->sendBattleCharJobSkillAct(--skillIndex, objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::charUseSkill(std::string sskill, long long objIndex, sol::this_state s)//skill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull() || !gamedevice.worker->getBattleFlag())
		return FALSE;

	long long index = gamedevice.worker->getSkillIndexByName(util::toQString(sskill));
	if (index < 0)
		return FALSE;

	--objIndex;
	if (objIndex < 0)
		objIndex = 0;

	return gamedevice.worker->sendBattleCharJobSkillAct(index, objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::switchPet(long long petIndex, sol::this_state s)//switch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharSwitchPetAct(--petIndex) ? TRUE : FALSE;
}

long long CLuaBattle::escape(sol::this_state s)//escape
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharEscapeAct() ? TRUE : FALSE;
}

long long CLuaBattle::defense(sol::this_state s)//defense
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharDefenseAct() ? TRUE : FALSE;
}

long long CLuaBattle::useItem(long long itemIndex, long long objIndex, sol::this_state s)//item
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	--objIndex;
	if (objIndex < 0)
		objIndex = 0;

	return gamedevice.worker->sendBattleCharItemAct(--itemIndex, objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::useItem(std::string sitem, long long objIndex, sol::this_state s)//item
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull() || !gamedevice.worker->getBattleFlag())
		return FALSE;

	long long index = gamedevice.worker->getItemIndexByName(util::toQString(sitem));
	if (index < 0)
		return FALSE;

	--objIndex;
	if (objIndex < 0)
		objIndex = 0;

	return gamedevice.worker->sendBattleCharItemAct(index, objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::catchPet(long long objIndex, sol::this_state s)//catch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharCatchPetAct(--objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::catchPet(std::string sname, sol::object olevel, sol::object omaxhp, sol::object omodelid, sol::this_state s)//catch
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;


	QString name = util::toQString(sname);

	long long maxhp = -1;
	if (omaxhp.is<long long>())
		maxhp = omaxhp.as<long long>();

	long long modelid = -1;
	if (omodelid.is<long long>())
		modelid = omodelid.as<long long>();

	long long level = -1;
	if (olevel.is<long long>())
		level = olevel.as<long long>();

	QVector<sa::battle_object_t> battleObjects = gamedevice.worker->getBattleData().enemies;
	sa::battle_object_t obj = {};
	for (const sa::battle_object_t& obj : battleObjects)
	{
		if (!name.isEmpty() && obj.name != name)
			continue;

		if (level > 0 && obj.level != level)
			continue;

		if (maxhp > 0 && obj.maxHp != maxhp)
			continue;

		if (modelid > 0 && obj.modelid != modelid)
			continue;

		return gamedevice.worker->sendBattleCharCatchPetAct(obj.pos) ? TRUE : FALSE;
	}

	return FALSE;
}

long long CLuaBattle::nothing(sol::this_state s)//nothing
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattleCharDoNothing() ? TRUE : FALSE;
}

long long CLuaBattle::petUseSkill(long long petSkillIndex, long long objIndex, sol::this_state s)//petskill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattlePetSkillAct(--petSkillIndex, --objIndex) ? TRUE : FALSE;
}

long long CLuaBattle::petUseSkill(std::string spetskill, long long objIndex, sol::this_state s)//petskill
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (!gamedevice.worker.isNull() && gamedevice.worker->getBattleFlag())
	{
		long long petIndex = gamedevice.worker->getCharacter().battlePetNo;
		if (-1 == petIndex)
			return FALSE;

		long long index = gamedevice.worker->getPetSkillIndexByName(petIndex, util::toQString(spetskill));
		if (index < 0)
			return FALSE;

		return gamedevice.worker->sendBattlePetSkillAct(index, --objIndex) ? TRUE : FALSE;
	}
	return FALSE;
}

long long CLuaBattle::petNothing(sol::this_state s)//pet nothing
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	return gamedevice.worker->sendBattlePetDoNothing() ? TRUE : FALSE;
}

long long CLuaBattle::bwait(sol::object otimeout, sol::object jump, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	if (gamedevice.sendMessage(kEnableBattleDialog, false, NULL) <= 0)
		return FALSE;

	bool bret = luadebug::waitfor(s, timeout, [&gamedevice]()
		{
			if (!gamedevice.worker->getBattleFlag())
				return true;
			long long G = gamedevice.worker->getGameStatus();
			long long W = gamedevice.worker->getWorldStatus();

			return W == 10 && G == 4;
		});

	if (gamedevice.worker->getBattleFlag())
	{
		if (gamedevice.sendMessage(kEnableBattleDialog, true, NULL) <= 0)
			return FALSE;
	}
	else
		bret = false;

	return bret ? TRUE : FALSE;
}

long long CLuaBattle::bend(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	long long G = gamedevice.worker->getGameStatus();
	if (G == 4)
	{
		if (!mem::write<short>(gamedevice.getProcess(), gamedevice.getProcessModule() + 0xE21E8, 1))
			return FALSE;

		gamedevice.worker->setGameStatus(5);
		gamedevice.worker->isBattleDialogReady.off();
		return TRUE;
	}
	return FALSE;
}