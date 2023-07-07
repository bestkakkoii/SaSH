#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

//check
qint64 Interpreter::checkdaily(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	qint64 status = injector.server->checkJobDailyState(text);
	if (status == 1)//已完成
		return Parser::kNoChange;
	else if (status == 2)//進行中
		return checkJump(TK, 2, true, SuccessJump);//使用第2參數跳轉
	else//沒接過任務
		return checkJump(TK, 3, true, SuccessJump);//使用第3參數跳轉
}

qint64 Interpreter::isbattle(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, injector.server->getBattleFlag(), FailedJump);
}

qint64 Interpreter::isonline(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, injector.server->getOnlineFlag(), FailedJump);
}

qint64 Interpreter::isnormal(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, !injector.server->getBattleFlag(), FailedJump);
}

qint64 Interpreter::checkcoords(qint64, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	qint64 x = 0;
	qint64 y = 0;
	checkInteger(TK, 1, &x);
	checkInteger(TK, 2, &y);
	QPoint p(x, y);

	return checkJump(TK, 3, injector.server->getPoint() == p, FailedJump);
}

qint64 Interpreter::checkmap(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString mapname = "";
	qint64 floor = 0;
	checkString(TK, 1, &mapname);
	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);
	checkInteger(TK, 1, &floor);

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	if (floor == 0 && mapname.isEmpty())
		return Parser::kError;

	bool bret = waitfor(timeout, [&injector, floor, mapnames]()->bool
		{
			if (floor != 0)
				return floor == injector.server->nowFloor;
			else
			{
				for (const QString& mapname : mapnames)
				{
					bool ok;
					qint64 fr = mapname.toLongLong(&ok);
					if (ok)
					{
						if (fr == injector.server->nowFloor)
							return true;
					}
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
				}
			}
			return false;
		});

	if (!bret && timeout > 2000)
		injector.server->EO();

	return checkJump(TK, 3, bret, FailedJump);
}

qint64 Interpreter::checkmapnowait(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString mapname = "";
	qint64 floor = 0;
	checkString(TK, 1, &mapname);
	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);
	checkInteger(TK, 1, &floor);

	auto check = [floor, mapnames, &injector]()
	{
		if (floor != 0)
			return floor == injector.server->nowFloor;
		else
		{
			for (const QString& mapname : mapnames)
			{
				bool ok;
				qint64 floor = mapname.toLongLong(&ok);
				if (ok)
				{
					if (floor == injector.server->nowFloor)
						return true;
				}
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
			}

		}
		return false;
	};

	return checkJump(TK, 2, check(), FailedJump);
}

qint64 Interpreter::checkdialog(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString cmpStr;
	checkString(TK, 1, &cmpStr);
	if (cmpStr.isEmpty())
		return Parser::kArgError;
	QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

	qint64 min = 1;
	qint64 max = MAX_DIALOG_LINE;
	if (!checkRange(TK, 2, &min, &max))
		return Parser::kArgError;
	if (min == max)
	{
		++min;
		++max;
	}

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

	bool bret = waitfor(timeout, [&injector, min, max, cmpStrs]()->bool
		{
			QStringList dialogStrList = injector.server->currentDialog.linedatas;
			for (qint64 i = min; i <= max; ++i)
			{
				int index = i - 1;
				if (index < 0 || index >= dialogStrList.size())
					break;

				QString text = dialogStrList.at(index);
				if (text.isEmpty())
					continue;

				for (const QString& cmpStr : cmpStrs)
				{
					if (text.contains(cmpStr, Qt::CaseInsensitive))
					{
						return true;
					}
				}
			}

			return false;
		});


	return checkJump(TK, 4, bret, FailedJump);
}

qint64 Interpreter::checkchathistory(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	qint64 min = 1;
	qint64 max = MAX_CHAT_HISTORY;
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError;
	if (min == max)
	{
		++min;
		++max;
	}

	QString cmpStr;
	checkString(TK, 2, &cmpStr);
	if (cmpStr.isEmpty())
		return Parser::kArgError;
	QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

	bool bret = waitfor(timeout, [&injector, min, max, cmpStrs]()->bool
		{
			for (qint64 i = min; i <= max; ++i)
			{
				QString text = injector.server->getChatHistory(i - 1);
				if (text.isEmpty())
					continue;

				for (const QString& cmpStr : cmpStrs)
				{
					if (text.contains(cmpStr, Qt::CaseInsensitive))
					{
						return true;
					}
				}
			}

			return false;
		});

	return checkJump(TK, 4, bret, FailedJump);
}

qint64 Interpreter::checkunit(qint64, const TokenMap&)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return Parser::kNoChange;
}

qint64 Interpreter::checkplayerstatus(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaPlayer, TK);

	return checkJump(TK, 4, bret, SuccessJump);
}

qint64 Interpreter::checkpetstatus(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaPet, TK);

	return checkJump(TK, 5, bret, SuccessJump);
}

qint64 Interpreter::checkitemcount(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaItem, TK);

	return checkJump(TK, 5, bret, SuccessJump);
}

qint64 Interpreter::checkteamcount(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaCount, TK);

	return checkJump(TK, 3, bret, SuccessJump);
}

qint64 Interpreter::checkpetcount(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaCount, TK);

	return checkJump(TK, 3, bret, SuccessJump);
}

//check->group
qint64 Interpreter::checkteam(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	PC pc = injector.server->getPC();

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 1, &timeout);

	bool bret = waitfor(timeout, [&pc]()->bool
		{
			return (pc.status & CHR_STATUS_LEADER) || (pc.status & CHR_STATUS_PARTY);
		});

	return checkJump(TK, 2, bret, FailedJump);
}

qint64 Interpreter::checkitemfull(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = true;
	for (qint64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		ITEM item = injector.server->getPC().item[i];
		if (item.useFlag == 0 || item.name.isEmpty())
		{
			bret = false;
			break;
		}
	}

	return checkJump(TK, 1, bret, SuccessJump);
}

qint64 Interpreter::checkitem(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	qint64 min = 0, max = static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	bool isEquip = false;
	if (!checkRange(TK, 1, &min, &max))
	{
		QString partStr;
		checkString(TK, 1, &partStr);
		if (partStr.isEmpty())
			return Parser::kArgError;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			min = 101;
			max = static_cast<qint64>(101 + CHAR_EQUIPPLACENUM);
		}
		else
		{
			min = equipMap.value(partStr.toLower(), CHAR_EQUIPNONE);
			if (min == CHAR_EQUIPNONE)
				return Parser::kArgError;
			max = min;
			isEquip = true;
		}
	}

	if (min < 101 && max < 101 && !isEquip)
	{
		min += CHAR_EQUIPPLACENUM;
		max += CHAR_EQUIPPLACENUM;
	}
	else if (min >= 101 && max <= static_cast<qint64>(100 + CHAR_EQUIPPLACENUM))
	{
		min -= 101;
		max -= 101;
	}

	QString itemName;
	checkString(TK, 2, &itemName);
	if (itemName.isEmpty())
		return Parser::kArgError;

	QStringList itemNames = itemName.split(util::rexOR, Qt::SkipEmptyParts);
	if (itemNames.isEmpty())
		return Parser::kArgError;

	QString itemMemo;
	checkString(TK, 3, &itemMemo);
	QStringList itemMemos = itemMemo.split(util::rexOR);


	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 4, &timeout);

	bool bret = waitfor(timeout, [&injector, min, max, itemNames, itemMemos]()->bool
		{
			for (qint64 i = min; i <= max; ++i)
			{
				ITEM item = injector.server->getPC().item[i];
				if (item.useFlag == 0 || item.name.isEmpty())
					continue;

				if (itemMemos.isEmpty())
				{
					for (const QString& it : itemNames)
					{

						if (item.name == it)
						{
							return true;
						}
						else if (it.startsWith(kFuzzyPrefix))
						{
							QString newName = it.mid(1);
							if (item.name.contains(newName))
							{
								return true;
							}
						}
					}
				}
				else if (itemMemos.size() == itemNames.size())
				{
					for (qint64 j = 0; j < itemNames.size(); ++j)
					{
						if (item.name == itemNames[j] && item.memo.contains(itemMemos[j]))
						{
							return true;
						}
						else if (itemNames[j].startsWith(kFuzzyPrefix))
						{
							QString newName = itemNames[j].mid(1);
							if (item.name.contains(newName) && item.memo.contains(itemMemos[j]))
							{
								return true;
							}
						}
					}
				}

			}

			return false;
		});

	return checkJump(TK, 5, bret, FailedJump);
}

qint64 Interpreter::checkpet(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError;

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	QElapsedTimer timer; timer.start();
	bool bret = waitfor(timeout, [&injector, petName]()->bool
		{
			QVector<int> v;
			return injector.server->getPetIndexsByName(petName, &v);
		});

	return checkJump(TK, 3, bret, FailedJump);
}

