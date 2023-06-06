#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

//check
int Interpreter::checkdaily(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	if (!checkString(TK, 1, &text))
		return Parser::kError;

	QString state;
	if (!checkString(TK, 2, &state))
		return Parser::kError;

	return checkJump(TK, 3, injector.server->checkJobDailyState(text, state), FailedJump);
}

int Interpreter::isbattle(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, injector.server->IS_BATTLE_FLAG, SuccessJump);
}

int Interpreter::checkcoords(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	return checkJump(TK, 1, injector.server->nowPoint == p, FailedJump);
}

int Interpreter::checkmap(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString mapname = "";
	int floor = 0;
	checkString(TK, 1, &mapname);
	checkInt(TK, 1, &floor);

	if (floor == 0 && mapname.isEmpty())
		return Parser::kError;

	bool bret = waitfor(5000, [&injector, floor, mapname]()->bool
		{
			if (floor != 0)
				return floor == injector.server->nowFloor;
			else
				return mapname == injector.server->nowFloorName;
		});

	if (!bret)
		injector.server->EO();

	return checkJump(TK, 2, bret, FailedJump);
}

int Interpreter::checkmapnowait(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString mapname = "";
	int floor = 0;
	checkString(TK, 1, &mapname);
	checkInt(TK, 1, &floor);

	if (floor == 0 && mapname.isEmpty())
		return Parser::kError;

	bool bret = false;
	if (floor != 0)
		bret = floor == injector.server->nowFloor;
	else
		bret = mapname == injector.server->nowFloorName;

	return checkJump(TK, 2, bret, FailedJump);
}

int Interpreter::checkdialog(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;


}

int Interpreter::checkchathistory(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;


	QString text;


}

int Interpreter::checkunit(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;
}
