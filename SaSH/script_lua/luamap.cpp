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

qint64 CLuaMap::setDir(qint64 dir, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->setPlayerFaceDirection(--dir);

	return TRUE;
}

qint64 CLuaMap::setDir(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	injector.server->setPlayerFaceToPoint(pos);

	return TRUE;
}

qint64 CLuaMap::setDir(std::string sdir, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString qdir = QString::fromUtf8(sdir.c_str());

	injector.server->setPlayerFaceDirection(qdir);

	return TRUE;
}

qint64 CLuaMap::move(qint64 x, qint64 y, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	injector.server->move(pos);

	return TRUE;
}

qint64 CLuaMap::packetMove(qint64 x, qint64 y, std::string sdir, sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	QString qdir = QString::fromUtf8(sdir.c_str());

	injector.server->move(pos, qdir);

	return TRUE;
}

qint64 CLuaMap::teleport(sol::this_state s)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->warp();

	return TRUE;
}