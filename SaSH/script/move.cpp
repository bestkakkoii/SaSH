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
		injector.server->setPlayerFaceDirection(dir);
	}
	else if (dir == -1 && !dirStr.isEmpty())
	{
		injector.server->setPlayerFaceDirection(dirStr);
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
			p = injector.server->getPoint() + util::fix_point.at(dir) * 10;
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
		p = injector.server->getPoint() + util::fix_point.at(dir) * 10;

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

qint64 Interpreter::findpath(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 x = 0;
	qint64 y = 0;
	QPoint p;
	qint64 steplen = 3;
	qint64 step_cost = 10;
	qint64 timeout = 180000;
	QString name;
	if (!checkInteger(TK, 1, &x))
	{
		if (!checkString(TK, 1, &name))
			return Parser::kArgError + 1ll;
	}
	else
		checkInteger(TK, 2, &y);

	p = QPoint(x, y);

	auto findNpcCallBack = [&injector](const QString& name, QPoint& dst, qint64* pdir)->bool
	{
		mapunit_s unit;
		if (!injector.server->findUnit(name, util::OBJ_NPC, &unit, ""))
		{
			if (!injector.server->findUnit(name, util::OBJ_HUMAN, &unit, ""))
				return 0;//沒找到
		}

		qint64 dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(injector.server->nowFloor, injector.server->getPoint(), unit.p, &dst, true, unit.dir);
		if (pdir)
			*pdir = dir;
		return dir != -1;//找到了
	};

	qint64 dir = -1;
	if (p.isNull() && !name.isEmpty())
	{
		QString key = QString::number(injector.server->nowFloor);
		util::Config config(injector.getPointFileName());
		QList<util::MapData> datas = config.readMapData(key);
		if (datas.isEmpty())
			return Parser::kNoChange;

		QPoint point;
		QStringList strList = name.split(util::rexOR, Qt::SkipEmptyParts);
		if (strList.size() == 2)
		{
			bool ok = false;

			point.setX(strList.at(0).toLongLong(&ok));
			if (ok)
				point.setY(strList.at(1).toLongLong(&ok));
			if (!ok)
				return Parser::kArgError;
		}

		for (const util::MapData& d : datas)
		{
			if (d.name.isEmpty())
				return Parser::kNoChange;

			if (point == QPoint(d.x, d.y))
			{
				p = QPoint(d.x, d.y);
				checkInteger(TK, 2, &steplen);
			}
			else if (d.name == name)
			{
				p = QPoint(d.x, d.y);
				checkInteger(TK, 2, &steplen);
				break;
			}
			else if (name.startsWith(kFuzzyPrefix))
			{
				QString newName = name.mid(0);
				if (d.name.contains(newName))
				{
					p = QPoint(d.x, d.y);
					checkInteger(TK, 2, &steplen);
					break;
				}
			}
		}

		if (p.isNull())
			return Parser::kArgError + 1ll;

		checkInteger(TK, 3, &step_cost);

		checkInteger(TK, 4, &timeout);
	}
	else
	{
		checkInteger(TK, 3, &steplen);

		checkInteger(TK, 4, &step_cost);

		checkInteger(TK, 5, &timeout);
	}

	//findpath 不允許接受為0的xy座標
	if (p.x() < 0 || p.x() >= 1500)
		return Parser::kArgError + 1ll;

	if (p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError + 2ll;

	if (findPath(currentIndex, currentLine, p, steplen, step_cost, timeout))
	{
		if (!name.isEmpty() && (findNpcCallBack(name, p, &dir)) && dir != -1)
			injector.server->setPlayerFaceDirection(dir);
	}

	return Parser::kNoChange;
}

qint64 Interpreter::movetonpc(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString cmpNpcName;
	qint64 modelid = -1;
	if (!checkInteger(TK, 1, &modelid))
	{
		if (!checkString(TK, 1, &cmpNpcName))
			return Parser::kArgError + 1ll;
	}

	QString cmpFreeName;
	checkString(TK, 2, &cmpFreeName);
	if (cmpFreeName.isEmpty() && cmpNpcName.isEmpty() && modelid == -1)
		return Parser::kArgError + 2ll;

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 3, &x);
	checkInteger(TK, 4, &y);
	QPoint p(x, y);

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT * 36;
	checkInteger(TK, 5, &timeout);

	//findpath 不允許接受為0的xy座標
	if (p.x() <= 0 || p.x() >= 1500)
		return Parser::kArgError + 1ll;

	if (p.y() <= 0 || p.y() >= 1500)
		return Parser::kArgError + 2ll;

	mapunit_s unit;
	qint64 dir = -1;
	auto findNpcCallBack = [&injector, &unit, cmpNpcName, cmpFreeName, modelid, &dir](QPoint& dst)->bool
	{
		if (modelid > 0)
		{
			if (!injector.server->findUnit("", util::OBJ_NPC, &unit, "", modelid))
			{
				return 0;//沒找到
			}
		}
		else if (!injector.server->findUnit(cmpNpcName, util::OBJ_NPC, &unit, cmpFreeName))
		{
			if (!injector.server->findUnit(cmpNpcName, util::OBJ_HUMAN, &unit, cmpFreeName))
				return 0;//沒找到
		}

		dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(injector.server->nowFloor, injector.server->getPoint(), unit.p, &dst, true, unit.dir);
		return dir != -1 ? 1 : 0;//找到了
	};

	bool bret = false;
	if (findPath(currentIndex, currentLine, p, 1, 0, timeout, findNpcCallBack, true) && dir != -1)
	{
		injector.server->setPlayerFaceDirection(dir);
		bret = true;
	}

	return checkJump(TK, 6, bret, FailedJump);
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

qint64 Interpreter::warp(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 xfrom = 0;
	qint64 yfrom = 0;
	checkInteger(TK, 1, &xfrom);
	checkInteger(TK, 2, &yfrom);
	QPoint pfrom(xfrom, yfrom);
	if (pfrom.x() < 0 || pfrom.x() > 1500)
		return Parser::kArgError + 1ll;

	if (pfrom.y() < 0 || pfrom.y() > 1500)
		return Parser::kArgError + 2ll;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 xto = 0;
	qint64 yto = 0;
	checkInteger(TK, 3, &xto);
	checkInteger(TK, 4, &yto);
	QPoint pto(xto, yto);
	if (pto.x() < 0 || pto.x() > 1500)
		return Parser::kArgError + 3ll;

	if (pto.y() < 0 || pto.y() > 1500)
		return Parser::kArgError + 4ll;

	qint64 floor = 0;
	QString floorName;
	if (!checkInteger(TK, 5, &floor))
	{
		if (!checkString(TK, 5, &floorName))
			return Parser::kArgError + 5ll;
	}

	if (floor == 0 && floorName.isEmpty())
		return Parser::kArgError + 5ll;

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 6, &timeout);

	bool bret = false;

	do
	{
		if (!findPath(currentIndex, currentLine, pfrom, 1, 0, timeout))
			break;

		bret = waitfor(timeout, [&injector, pto, floor, floorName]()->bool
			{
				if (injector.server->getPoint() == pto)
				{
					if (floorName.isEmpty())
					{
						if (injector.server->nowFloor == floor)
							return true;
					}
					else
					{
						if (floorName.startsWith(kFuzzyPrefix))
						{
							QString newFloorName = floorName.mid(1);
							if (injector.server->nowFloorName.contains(newFloorName))
								return true;
						}
						else if (injector.server->nowFloorName == floorName)
							return true;
					}
				}
				return false;
			});

	} while (false);

	return checkJump(TK, 7, bret, FailedJump);
}