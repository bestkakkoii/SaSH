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

//check
qint64 Interpreter::checkdaily(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
		return Parser::kArgError + 1ll;

	qint64 status = injector.server->checkJobDailyState(text);
	if (status == 1)//已完成
		return Parser::kNoChange;
	else if (status == 2)//進行中
		return checkJump(TK, 2, true, SuccessJump);//使用第2參數跳轉
	else//沒接過任務
		return checkJump(TK, 3, true, SuccessJump);//使用第3參數跳轉
}

qint64 Interpreter::waitpet(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString petName;
	checkString(TK, 1, &petName);
	if (petName.isEmpty())
		return Parser::kArgError + 1ll;

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	checkOnlineThenWait();

	bool bret = false;
	if (timeout == 0)
	{
		QVector<qint64> v;
		bret = injector.server->getPetIndexsByName(petName, &v);
	}
	else
	{
		bret = waitfor(timeout, [&injector, petName]()->bool
			{
				QVector<qint64> v;
				return injector.server->getPetIndexsByName(petName, &v);
			});
	}

	return checkJump(TK, 3, bret, FailedJump);
}

qint64 Interpreter::waitmap(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString mapname = "";
	qint64 floor = 0;
	checkString(TK, 1, &mapname);
	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);
	checkInteger(TK, 1, &floor);

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	if (floor == 0 && mapname.isEmpty())
		return Parser::kServerNotReady;

	auto check = [&injector, floor, mapnames]()
	{
		if (floor != 0)
			return floor == injector.server->getFloor();
		else
		{
			for (const QString& mapname : mapnames)
			{
				bool ok;
				qint64 fr = mapname.toLongLong(&ok);
				if (ok)
				{
					if (fr == injector.server->getFloor())
						return true;
				}
				else
				{
					if (mapname.startsWith("?"))
					{
						QString newName = mapname.mid(1);
						return injector.server->getFloorName().contains(newName);
					}
					else
						return mapname == injector.server->getFloorName();
				}
			}
		}
		return false;
	};

	bool bret = false;
	if (timeout == 0)
	{
		bret = check();
	}
	else
	{
		bret = waitfor(timeout, [&check]()->bool
			{
				return check();
			});
	}

	if (!bret && timeout > 2000)
		injector.server->EO();

	return checkJump(TK, 3, bret, FailedJump);
}

qint64 Interpreter::waitdlg(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString cmpStr;
	qint64 dlgid = -1;
	if (!checkString(TK, 1, &cmpStr))
	{
		if (!checkInteger(TK, 1, &dlgid))
			return Parser::kArgError + 1ll;
	}

	bool bret = false;
	if (dlgid != -1)
	{
		qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
		checkInteger(TK, 2, &timeout);

		if (timeout == 0)
			bret = injector.server->currentDialog.get().dialogid == dlgid;
		else
		{
			bret = waitfor(timeout, [&injector, dlgid]()->bool
				{
					if (!injector.server->isDialogVisible())
						return false;

					return injector.server->currentDialog.get().dialogid == dlgid;
				});
		}

		return checkJump(TK, 3, bret, FailedJump);
	}
	else
	{
		QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

		qint64 min = 1;
		qint64 max = MAX_DIALOG_LINE;
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError + 2ll;
		if (min == max)
		{
			++min;
			++max;
		}

		qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
		checkInteger(TK, 3, &timeout);

		auto check = [&injector, min, max, cmpStrs]()->bool
		{
			if (!injector.server->isDialogVisible())
				return false;

			if (cmpStrs.isEmpty() || cmpStrs.front().isEmpty())
				return true;

			QStringList dialogStrList = injector.server->currentDialog.get().linedatas;
			for (qint64 i = min; i <= max; ++i)
			{
				int index = i - 1;
				if (index < 0 || index >= dialogStrList.size())
					break;

				QString text = dialogStrList.at(index).simplified();
				if (text.isEmpty())
					continue;

				for (const QString& cmpStr : cmpStrs)
				{
					if (text.contains(cmpStr.simplified(), Qt::CaseInsensitive))
					{
						return true;
					}
				}
			}

			return false;
		};

		if (timeout == 0)
		{
			bret = check();
		}
		else
		{
			bret = waitfor(timeout, [&check]()->bool
				{
					return check();
				});

		}

		return checkJump(TK, 4, bret, FailedJump);
	}
}

qint64 Interpreter::waitsay(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 min = 1;
	qint64 max = MAX_CHAT_HISTORY;
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError + 1ll;
	if (min == max)
	{
		++min;
		++max;
	}

	QString cmpStr;
	checkString(TK, 2, &cmpStr);
	if (cmpStr.isEmpty())
		return Parser::kArgError + 2ll;
	QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

	bool bret = false;
	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

	auto check = [&injector, min, max, cmpStrs]()->bool
	{
		for (qint64 i = min; i <= max; ++i)
		{
			QString text = injector.server->getChatHistory(i - 1).simplified();
			if (text.isEmpty())
				continue;

			for (const QString& cmpStr : cmpStrs)
			{
				if (text.contains(cmpStr.simplified(), Qt::CaseInsensitive))
				{
					return true;
				}
			}
		}

		return false;
	};

	if (timeout == 0)
	{
		bret = check();
	}
	else
	{
		bret = waitfor(timeout, [&check]()->bool
			{
				return check();
			});
	}

	return checkJump(TK, 4, bret, FailedJump);
}

//check->group
qint64 Interpreter::waitteam(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	PC pc = injector.server->getPC();

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 1, &timeout);

	bool bret = false;
	if (timeout == 0)
	{
		bret = (pc.status & CHR_STATUS_LEADER) || (pc.status & CHR_STATUS_PARTY);;
	}
	else
	{
		bret = waitfor(timeout, [&pc]()->bool
			{
				return (pc.status & CHR_STATUS_LEADER) || (pc.status & CHR_STATUS_PARTY);
			});
	}

	return checkJump(TK, 2, bret, FailedJump);
}

qint64 Interpreter::waititem(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 min = 0, max = static_cast<qint64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	bool isEquip = false;
	if (!checkRange(TK, 1, &min, &max))
	{
		QString partStr;
		checkString(TK, 1, &partStr);
		if (partStr.isEmpty())
			return Parser::kArgError + 1ll;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			min = 101;
			max = static_cast<qint64>(101 + CHAR_EQUIPPLACENUM);
		}
		else
		{
			min = equipMap.value(partStr.toLower(), CHAR_EQUIPNONE);
			if (min == CHAR_EQUIPNONE)
				return Parser::kArgError + 2ll;
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

	QString itemMemo;
	checkString(TK, 3, &itemMemo);

	if (itemName.isEmpty() && itemMemo.isEmpty())
		return Parser::kArgError + 1ll;

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 4, &timeout);

	injector.server->updateItemByMemory();

	bool bret = false;
	if (timeout == 0)
	{
		QVector<qint64> vec;
		bret = injector.server->getItemIndexsByName(itemName, itemMemo, &vec, min, max);
	}
	else
	{
		bret = waitfor(timeout, [&injector, itemName, itemMemo, min, max]()->bool
			{
				QVector<qint64> vec;
				return injector.server->getItemIndexsByName(itemName, itemMemo, &vec, min, max);
			});
	}

	return checkJump(TK, 5, bret, FailedJump);
}
