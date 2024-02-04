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

sa::pet_t CLuaPet::operator[](long long index)
{
	--index;

	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return sa::pet_t();

	gamedevice.worker->updateItemByMemory();

	return gamedevice.worker->getPet(index);
}

long long CLuaPet::count()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	gamedevice.worker->updateItemByMemory();
	return gamedevice.worker->getPetSize();
}

long long CLuaPet::count(std::string sname)
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return 0;

	gamedevice.worker->updateItemByMemory();

	QString name = util::toQString(sname);

	QVector< long long> indexs;
	if (!gamedevice.worker->getPetIndexsByName(name, &indexs))
		return 0;

	return indexs.size();
}


long long CLuaPet::learn(long long petIndex, long long fromSkillIndex, long long toSkillIndex, sol::object ounitid, sol::object odialogid, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long modelid = -1;
	long long unitid = -1;
	if (ounitid.is<long long>())
		modelid = ounitid.as<long long>();

	QString npcname = "";
	if (ounitid.is<std::string>())
		npcname = util::toQString(ounitid);

	if (modelid != -1 || !npcname.isEmpty())
	{
		sa::map_unit_t unit = {};
		if (gamedevice.worker->findUnit(npcname, sa::kObjectNPC, &unit, "", modelid))
		{
			unitid = unit.id;
		}
	}

	long long dialogid = 0;
	if (odialogid.is<long long>())
		dialogid = odialogid.as<long long>();

	return gamedevice.worker->learn(--petIndex, --fromSkillIndex, --toSkillIndex, dialogid, unitid);
}