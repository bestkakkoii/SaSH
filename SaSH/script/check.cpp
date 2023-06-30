#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

//check
int Interpreter::checkdaily(int, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
		return Parser::kArgError;

	int status = injector.server->checkJobDailyState(text);
	if (status == 1)//已完成
		return Parser::kNoChange;
	else if (status == 2)//進行中
		return checkJump(TK, 2, true, SuccessJump);//使用第2參數跳轉
	else//沒接過任務
		return checkJump(TK, 3, true, SuccessJump);//使用第3參數跳轉
}

int Interpreter::isbattle(int, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, injector.server->getBattleFlag(), FailedJump);
}

int Interpreter::isonline(int, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, injector.server->getOnlineFlag(), FailedJump);
}

int Interpreter::isnormal(int, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return checkJump(TK, 1, !injector.server->getBattleFlag(), FailedJump);
}

int Interpreter::checkcoords(int, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QPoint p;
	checkInt(TK, 1, &p.rx());
	checkInt(TK, 2, &p.ry());

	return checkJump(TK, 3, injector.server->nowPoint == p, FailedJump);
}

int Interpreter::checkmap(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString mapname = "";
	int floor = 0;
	checkString(TK, 1, &mapname);
	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);
	checkInt(TK, 1, &floor);

	int timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInt(TK, 2, &timeout);

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
					int fr = mapname.toInt(&ok);
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

int Interpreter::checkmapnowait(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	checkBattleThenWait();

	QString mapname = "";
	int floor = 0;
	checkString(TK, 1, &mapname);
	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);
	checkInt(TK, 1, &floor);

	auto check = [floor, mapnames, &injector]()
	{
		if (floor != 0)
			return floor == injector.server->nowFloor;
		else
		{
			for (const QString& mapname : mapnames)
			{
				bool ok;
				int floor = mapname.toInt(&ok);
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

int Interpreter::checkdialog(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString cmpStr;
	checkString(TK, 1, &cmpStr);
	if (cmpStr.isEmpty())
		return Parser::kArgError;
	QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

	int min = 1;
	int max = 10;
	if (!checkRange(TK, 2, &min, &max))
		return Parser::kArgError;
	if (min == max)
	{
		++min;
		++max;
	}

	int timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInt(TK, 3, &timeout);

	bool bret = waitfor(timeout, [&injector, min, max, cmpStrs]()->bool
		{
			util::SafeHash<int, QVariant> hashdialog = injector.server->hashdialog;
			for (int i = min; i <= max; ++i)
			{
				if (!hashdialog.contains(i))
					break;

				QString text = hashdialog.value(i).toString();
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

int Interpreter::checkchathistory(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int min = 1;
	int max = 20;
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

	int timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInt(TK, 3, &timeout);

	bool bret = waitfor(timeout, [&injector, min, max, cmpStrs]()->bool
		{
			util::SafeHash<int, QVariant> hashchat = injector.server->hashchat;
			for (int i = min; i <= max; ++i)
			{
				if (!hashchat.contains(i))
					break;

				QString text = hashchat.value(i).toString();
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

int Interpreter::checkunit(int, const TokenMap&)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	return Parser::kNoChange;
}

int Interpreter::checkplayerstatus(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaPlayer, TK);

	return checkJump(TK, 4, bret, SuccessJump);
}

int Interpreter::checkpetstatus(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaPet, TK);

	return checkJump(TK, 5, bret, SuccessJump);
}

int Interpreter::checkitemcount(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaItem, TK);

	return checkJump(TK, 5, bret, SuccessJump);
}

int Interpreter::checkteamcount(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaCount, TK);

	return checkJump(TK, 3, bret, SuccessJump);
}

int Interpreter::checkpetcount(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = compare(kAreaCount, TK);

	return checkJump(TK, 3, bret, SuccessJump);
}

//check->group
int Interpreter::checkteam(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	PC pc = injector.server->pc;

	int timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInt(TK, 1, &timeout);

	bool bret = waitfor(timeout, [&pc]()->bool
		{
			return (pc.status & CHR_STATUS_LEADER) || (pc.status & CHR_STATUS_PARTY);
		});

	return checkJump(TK, 2, bret, FailedJump);
}

int Interpreter::checkitemfull(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	bool bret = true;
	for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		ITEM item = injector.server->pc.item[i];
		if (item.useFlag == 0 || item.name.isEmpty())
		{
			bret = false;
			break;
		}
	}

	return checkJump(TK, 1, bret, SuccessJump);
}

int Interpreter::checkitem(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	int min = 0, max = 14;
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
			max = 101 + CHAR_EQUIPPLACENUM;
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
	else if (min >= 101 && max <= 100 + CHAR_EQUIPPLACENUM)
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


	int timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInt(TK, 4, &timeout);

	bool bret = waitfor(timeout, [&injector, min, max, itemNames, itemMemos]()->bool
		{
			for (int i = min; i <= max; ++i)
			{
				ITEM item = injector.server->pc.item[i];
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
					for (int j = 0; j < itemNames.size(); ++j)
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

int Interpreter::checkpet(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError;

	int timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInt(TK, 2, &timeout);

	QElapsedTimer timer; timer.start();
	bool bret = waitfor(timeout, [&injector, petName]()->bool
		{
			util::SafeVector<int> v;
			return injector.server->getPetIndexsByName(petName, &v);
		});

	return checkJump(TK, 3, bret, FailedJump);
}

