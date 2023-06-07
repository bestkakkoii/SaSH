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
	checkString(TK, 1, &text);

	int status = injector.server->checkJobDailyState(text);
	if (status == 1)//已完成
		return Parser::kNoChange;
	else if (status == 2)//進行中
		return checkJump(TK, 2, false, FailedJump);//使用第2參數跳轉
	else//沒接過任務
		return checkJump(TK, 3, false, FailedJump);//使用第3參數跳轉
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

	return checkJump(TK, 3, injector.server->nowPoint == p, FailedJump);
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
			{
				if (mapname.startsWith("?"))
				{
					QString newName = mapname.mid(1);
					return injector.server->nowFloorName.contains(newName);
				}
				else
					return mapname == injector.server->nowFloorName;
			}
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

	QString cmpStr;
	checkString(TK, 1, &cmpStr);
	if (cmpStr.isEmpty())
		return Parser::kArgError;

	int min = 0;
	int max = 7;
	if (!checkRange(TK, 2, &min, &max))
		return Parser::kArgError;

	QThread::msleep(100);

	bool bret = waitfor(5000, [&injector, min, max, cmpStr]()->bool
		{
			QStringList dialogList = injector.server->currentDialog.linedatas;
			for (int i = min; i <= max; i++)
			{
				if (i >= dialogList.size())
					break;

				QString text = dialogList.at(i);
				if (text.isEmpty())
					continue;

				if (text.contains(cmpStr))
				{
					QThread::msleep(100);
					return true;
				}
			}

			return false;
		});


	return checkJump(TK, 3, bret, FailedJump);
}

int Interpreter::checkchathistory(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int min = 0;
	int max = 19;
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError;


	QString cmpStr;
	checkString(TK, 2, &cmpStr);
	if (cmpStr.isEmpty())
		return Parser::kArgError;

	QVector<QPair<int, QString>> chatHistory = injector.server->chatQueue.values();
	bool bret = false;
	for (int i = min; i <= max; i++)
	{
		if (i >= chatHistory.size())
			break;

		QPair<int, QString> text = chatHistory.at(i);
		if (text.second.isEmpty())
			continue;

		if (text.second.contains(cmpStr))
		{
			bret = true;
			break;
		}
	}

	return checkJump(TK, 3, bret, FailedJump);
}

int Interpreter::checkunit(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return Parser::kNoChange;
}

int Interpreter::checkplayerstatus(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaPlayer, TK);

	return checkJump(TK, 4, bret, FailedJump);
}

int Interpreter::checkpetstatus(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaPet, TK);

	return checkJump(TK, 5, bret, FailedJump);
}

int Interpreter::checkitemcount(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaItem, TK);

	return checkJump(TK, 4, bret, SuccessJump);
}

int Interpreter::checkteamcount(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaCount, TK);

	return checkJump(TK, 3, bret, FailedJump);
}


int Interpreter::checkpetcount(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaCount, TK);

	return checkJump(TK, 3, bret, FailedJump);
}

//check->group
int Interpreter::checkteam(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	PC pc = injector.server->pc;
	return checkJump(TK, 1, (pc.status & CHR_STATUS_LEADER) || (pc.status & CHR_STATUS_PARTY), FailedJump);
}

int Interpreter::checkitemfull(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = false;
	for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		ITEM item = injector.server->pc.item[i];
		if (item.useFlag == 0 || item.name.isEmpty())
		{
			bret = true;
			break;
		}
	}

	return checkJump(TK, 1, bret, SuccessJump);
}

int Interpreter::checkitem(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int min = 0, max = 14;
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError;

	min += CHAR_EQUIPPLACENUM;
	max += CHAR_EQUIPPLACENUM;

	QString itemName;
	checkString(TK, 2, &itemName);
	if (itemName.isEmpty())
		return Parser::kArgError;

	QStringList itemNames = itemName.split(util::rexOR, Qt::SkipEmptyParts);
	if (itemNames.isEmpty())
		return Parser::kArgError;

	QString itemMemo;
	checkString(TK, 3, &itemMemo);

	bool bret = false;
	for (int i = min; i <= max; ++i)
	{
		ITEM item = injector.server->pc.item[i];
		if (item.useFlag == 0 || item.name.isEmpty())
			continue;

		for (const QString& it : itemNames)
		{
			if (item.name == it)
			{
				bret = true;
				break;
			}
			else if (it.startsWith(kFuzzyPrefix))
			{
				QString newName = it.mid(1);
				if (item.name.contains(newName))
				{
					bret = true;
					break;
				}
			}
		}

		if (bret)
			break;
	}

	return checkJump(TK, 4, bret, FailedJump);
}

int Interpreter::checkpet(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError;

	QVector<int> v;
	return checkJump(TK, 4, injector.server->getPetIndexsByName(petName, &v), FailedJump);
}

int Interpreter::cmp(const TokenMap& TK)
{
	QVariant a;
	QVariant b;

	if (!toVariant(TK, 1, &a))
		return Parser::kArgError;

	RESERVE op;
	if (checkRelationalOperator(TK, 2, &op))
		return Parser::kArgError;

	if (!toVariant(TK, 3, &b))
		return Parser::kArgError;

	return checkJump(TK, 4, compare(a, b, op), SuccessJump);
}