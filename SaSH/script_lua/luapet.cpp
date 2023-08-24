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

qint64 CLuaPet::setState(qint64 petIndex, qint64 state, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->setPetState(--petIndex, state);

	return TRUE;
}

qint64 CLuaPet::drop(qint64 petIndex, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->dropPet(--petIndex);

	return TRUE;
}

qint64 CLuaPet::rename(qint64 petIndex, std::string name, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString qname = QString::fromUtf8(name.c_str());

	injector.server->setPetFreeName(--petIndex, qname);

	return TRUE;
}

qint64 CLuaPet::learn(qint64 fromSkillIndex, qint64 petIndex, qint64 toSkillIndex, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->learn(--fromSkillIndex, --petIndex, --toSkillIndex, unitid, dialogid);

	return TRUE;
}

qint64 CLuaPet::swap(qint64 petIndex, qint64 from, qint64 to, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->petitemswap(--petIndex, --from, --to);

	return TRUE;
}

qint64 CLuaPet::deposit(qint64 petIndex, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->depositPet(--petIndex, unitid, dialogid);

	return TRUE;
}

qint64 CLuaPet::withdraw(qint64 petIndex, qint64 unitid, qint64 dialogid, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->withdrawPet(--petIndex, unitid, dialogid);

	return TRUE;
}