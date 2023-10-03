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
#include "interpreter.h"
#include "util.h"
#include "injector.h"
#include "map/mapanalyzer.h"

//move
qint64 Interpreter::setdir(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString dirStr = "";
	qint64 dir = -1;
	checkInteger(TK, 1, &dir);
	checkString(TK, 1, &dirStr);

	dirStr = dirStr.toUpper().simplified();

	if (dir != -1 && dirStr.isEmpty() && dir >= 1 && dir <= 8)
	{
		--dir;
		injector.server->setCharFaceDirection(dir);
	}
	else if (dir == -1 && !dirStr.isEmpty())
	{
		injector.server->setCharFaceDirection(dirStr);
	}
	else
		return Parser::kArgError + 1ll;

	return Parser::kNoChange;
}

qint64 Interpreter::move(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 1, &x);
	checkInteger(TK, 2, &y);
	QPoint p(x, y);

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

	if (p.x() < 0 || p.x() >= 1500)
		return Parser::kArgError + 1ll;

	if (p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError + 2ll;

	injector.server->move(p);
	QThread::msleep(1);

	waitfor(timeout, [this, &injector, &p]()->bool
		{
			checkBattleThenWait();
			bool result = injector.server->getPoint() == p;
			if (result)
			{
				injector.server->move(p);
			}
			return result;
		});

	return Parser::kNoChange;
}

qint64 Interpreter::fastmove(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 x = 0;
	qint64 y = 0;
	QPoint p;
	QString dirStr;
	if (checkInteger(TK, 1, &x))
	{
		if (checkInteger(TK, 2, &y))
		{
			if (x < 0 || x >= 1500)
				return Parser::kArgError + 1ll;

			if (y < 0 || y >= 1500)
				return Parser::kArgError + 2ll;

			p = QPoint(x, y);
		}
		else if (x >= 1 && x <= 8)
		{
			qint64 dir = x - 1;
			p = injector.server->getPoint() + util::fix_point.value(dir) * 10;
		}
		else
			return Parser::kArgError;
	}
	else if (checkString(TK, 1, &dirStr))
	{
		if (!dirMap.contains(dirStr.toUpper().simplified()))
			return Parser::kArgError + 1ll;

		DirType dir = dirMap.value(dirStr.toUpper().simplified());
		//計算出往該方向10格的坐標
		p = injector.server->getPoint() + util::fix_point.value(dir) * 10;

	}

	injector.server->move(p);
	QThread::msleep(1);

	return Parser::kNoChange;
}

qint64 Interpreter::packetmove(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 1, &x);
	checkInteger(TK, 2, &y);
	QPoint p(x, y);
	QString dirStr;
	checkString(TK, 3, &dirStr);

	if (p.x() < 0 || p.x() >= 1500)
		return Parser::kArgError + 1ll;

	if (p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError + 2ll;

	injector.server->move(p, dirStr);

	return Parser::kNoChange;
}

qint64 Interpreter::teleport(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->warp();

	return Parser::kNoChange;
}