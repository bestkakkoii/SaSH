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
import String;
#include "stdafx.h"
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"

__int64 CLuaPet::setState(__int64 petIndex, __int64 state, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->setPetState(--petIndex, static_cast<PetState>(state));

	return TRUE;
}

__int64 CLuaPet::drop(__int64 petIndex, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->dropPet(--petIndex);

	return TRUE;
}

__int64 CLuaPet::rename(__int64 petIndex, std::string name, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString qname = toQString(name);

	injector.server->setPetFreeName(--petIndex, qname);

	return TRUE;
}

__int64 CLuaPet::learn(__int64 fromSkillIndex, __int64 petIndex, __int64 toSkillIndex, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->learn(--fromSkillIndex, --petIndex, --toSkillIndex, dialogid, unitid);

	return TRUE;
}

__int64 CLuaPet::swap(__int64 petIndex, __int64 from, __int64 to, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->petitemswap(--petIndex, --from, --to);

	return TRUE;
}

__int64 CLuaPet::deposit(__int64 petIndex, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->depositPet(--petIndex, dialogid, unitid);

	return TRUE;
}

__int64 CLuaPet::withdraw(__int64 petIndex, __int64 unitid, __int64 dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<__int64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->withdrawPet(--petIndex, dialogid, unitid);

	return TRUE;
}