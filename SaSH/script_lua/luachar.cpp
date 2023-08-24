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

qint64 CLuaChar::rename(std::string sfname, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString name = QString::fromUtf8(sfname.c_str());

	injector.server->setPlayerFreeName(name);

	return TRUE;
}

qint64 CLuaChar::useMagic(qint64 magicIndex, qint64 target, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->useMagic(--magicIndex, target);

	return TRUE;
}

qint64 CLuaChar::depositGold(qint64 gold, sol::object oispublic, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->depositGold(gold, oispublic.is<bool>() ? oispublic.as<bool>() : false);

	return TRUE;
}

qint64 CLuaChar::withdrawGold(qint64 gold, sol::object oispublic, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->withdrawGold(gold, oispublic.is<bool>() ? oispublic.as<bool>() : false);

	return TRUE;
}

qint64 CLuaChar::dropGold(qint64 gold, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->dropGold(gold);

	return TRUE;
}

qint64 CLuaChar::mail(qint64 cardIndex, std::string stext, qint64 petIndex, std::string sitemName, std::string sitemMemo, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = QString::fromUtf8(stext.c_str());

	QString itemName = QString::fromUtf8(sitemName.c_str());

	QString itemMemo = QString::fromUtf8(sitemMemo.c_str());

	injector.server->mail(--cardIndex, text, petIndex, itemName, itemMemo);

	return TRUE;
}

qint64 CLuaChar::mail(qint64 cardIndex, std::string stext, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString text = QString::fromUtf8(stext.c_str());

	injector.server->mail(--cardIndex, text, -1, "", "");

	return TRUE;
}

qint64 CLuaChar::skillUp(qint64 abilityIndex, qint64 amount, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->addPoint(--abilityIndex, amount);

	return TRUE;
}

//action-group
qint64 CLuaChar::setTeamState(bool join, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->setTeamState(join);

	return TRUE;
}

qint64 CLuaChar::kick(qint64 teammateIndex, sol::this_state s)\
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->kickteam(--teammateIndex);

	return TRUE;
}