#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"
#include "map/mapanalyzer.h"

//move
int Interpreter::setdir(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString dirStr = "";
	int dir = -1;
	checkInt(TK, 1, &dir);
	checkString(TK, 1, &dirStr);

	if (dir != -1 && dirStr.isEmpty())
	{
		injector.server->setPlayerFaceDirection(dir);
	}
	else if (dir == -1 && !dirStr.isEmpty())
	{
		injector.server->setPlayerFaceDirection(dirStr);
	}
	else
		return Parser::kArgError;

	return Parser::kNoChange;
}

int Interpreter::move(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	int timeout = 2000;
	checkInt(TK, 3, &timeout);

	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	injector.server->move(p);
	QThread::msleep(1);

	waitfor(timeout, [&injector, &p]()->bool
		{
			bool result = injector.server->nowPoint == p;
			if (result)
			{
				injector.server->move(p);
			}
			return result;
		});

	return Parser::kNoChange;
}

int Interpreter::fastmove(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	injector.server->move(p);
	QThread::msleep(1);

	return Parser::kNoChange;
}

int Interpreter::packetmove(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());
	QString dirStr;
	checkString(TK, 3, &dirStr);

	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	injector.server->move(p, dirStr);
	QThread::msleep(1);

	return Parser::kNoChange;
}

int Interpreter::findpath(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	int steplen = 3;
	checkInt(TK, 3, &steplen);

	//findpath 不允許接受為0的xy座標
	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	findPath(p, steplen);

	return Parser::kNoChange;
}

int Interpreter::movetonpc(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString cmpNpcName;
	checkString(TK, 1, &cmpNpcName);

	QString cmpFreeName;
	checkString(TK, 2, &cmpFreeName);

	QPoint p;
	checkInt(TK, 3, &p.rx());
	checkInt(TK, 4, &p.ry());

	int timeout = 180000;
	checkInt(TK, 5, &timeout);

	//findpath 不允許接受為0的xy座標
	if (p.x() <= 0 || p.x() >= 1500 || p.y() <= 0 || p.y() >= 1500)
		return Parser::kArgError;

	mapunit_s unit;
	int dir = -1;
	auto findNpcCallBack = [&injector, &unit, cmpNpcName, cmpFreeName, &dir](QPoint& dst)->bool
	{
		if (!injector.server->findUnit(cmpNpcName, util::OBJ_NPC, &unit, cmpFreeName))
		{
			if (!injector.server->findUnit(cmpNpcName, util::OBJ_HUMAN, &unit, cmpFreeName))
				return 0;//沒找到
		}

		dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(injector.server->nowFloor, injector.server->nowPoint, unit.p, &dst, true, unit.dir);
		return dir != -1 ? 1 : 0;//找到了
	};

	bool bret = false;
	if (findPath(p, 1, 0, timeout, findNpcCallBack, true) && dir != -1)
	{
		injector.server->setPlayerFaceDirection(dir);
		bret = true;
	}

	return checkJump(TK, 6, bret, FailedJump);
}

int Interpreter::teleport(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	injector.server->warp();

	return Parser::kNoChange;
}

int Interpreter::warp(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint pfrom;
	checkInt(TK, 1, &pfrom.rx());
	checkInt(TK, 2, &pfrom.ry());
	if (pfrom.isNull() || pfrom.x() < 0 || pfrom.x() > 1500 || pfrom.y() < 0 || pfrom.y() > 1500)
		return Parser::kArgError;

	QPoint pto;
	checkInt(TK, 3, &pto.rx());
	checkInt(TK, 4, &pto.ry());
	if (pto.isNull() || pto.x() < 0 || pto.x() > 1500 || pto.y() < 0 || pto.y() > 1500)
		return Parser::kArgError;

	int floor = 0;
	QString floorName;
	if (!checkInt(TK, 5, &floor))
	{
		if (!checkString(TK, 5, &floorName))
			return Parser::kArgError;
	}

	if (floor == 0 && floorName.isEmpty())
		return Parser::kArgError;

	int timeout = 5000;
	checkInt(TK, 6, &timeout);

	bool bret = false;

	do
	{
		if (!findPath(pfrom, 1, 0, timeout))
			break;

		bret = waitfor(timeout, [&injector, pto, floor, floorName]()->bool
			{
				if (injector.server->nowPoint == pto)
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