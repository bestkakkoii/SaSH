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

qint64 Interpreter::walkpos(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
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