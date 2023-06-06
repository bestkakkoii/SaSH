#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"
#include "map/mapanalyzer.h"

//move
int Interpreter::setdir(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

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

int Interpreter::move(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	injector.server->move(p);

	waitfor(1000, [&injector, &p]()->bool
		{
			return injector.server->nowPoint == p;
		});

	return Parser::kNoChange;
}

int Interpreter::fastmove(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	injector.server->move(p);

	return Parser::kNoChange;
}

int Interpreter::packetmove(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());
	QString dirStr;
	checkString(TK, 3, &dirStr);

	if (p.x() < 0 || p.x() >= 1500 || p.y() < 0 || p.y() >= 1500)
		return Parser::kArgError;

	injector.server->move(p, dirStr);

	return Parser::kNoChange;
}

int Interpreter::findpath(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	int steplen = 3;
	checkInt(TK, 3, &steplen);

	//findpath 不允許接受為0的xy座標
	if (p.x() <= 0 || p.x() >= 1500 || p.y() <= 0 || p.y() >= 1500)
		return Parser::kArgError;

	findPath(p, steplen);

	return Parser::kNoChange;
}

int Interpreter::movetonpc(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

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
	if (findPath(p, 1, 0, timeout, findNpcCallBack) && dir != -1)
	{
		injector.server->setPlayerFaceDirection(dir);
		bret = true;
	}

	return checkJump(TK, 6, bret, FailedJump);
}