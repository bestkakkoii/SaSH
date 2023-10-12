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

import Utility;
#include "stdafx.h"
#include "interpreter.h"
#include "injector.h"

//check
__int64 Interpreter::checkdaily(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString text;
	checkString(TK, 1, &text);
	text = text.simplified();

	if (text.isEmpty())
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("mission name cannot be empty"));
		return Parser::kArgError + 1ll;
	}

	__int64 status = injector.server->checkJobDailyState(text);
	if (status == 1)//已完成
		return Parser::kNoChange;
	else if (status == 2)//進行中
		return checkJump(TK, 2, true, SuccessJump);//使用第2參數跳轉
	else//沒接過任務
		return checkJump(TK, 3, true, SuccessJump);//使用第3參數跳轉
}

__int64 Interpreter::waitpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString petName;
	checkString(TK, 1, &petName);
	petName = petName.simplified();

	if (petName.isEmpty())
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("pet name cannot be empty"));
		return Parser::kArgError + 1ll;
	}

	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	checkOnlineThenWait();

	bool bret = false;
	if (timeout == 0)
	{
		QVector<__int64> v;
		bret = injector.server->getPetIndexsByName(petName, &v);
	}
	else
	{
		bret = waitfor(timeout, [&injector, petName]()->bool
			{
				QVector<__int64> v;
				return injector.server->getPetIndexsByName(petName, &v);
			});
	}

	return checkJump(TK, 3, bret, FailedJump);
}

__int64 Interpreter::waitmap(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	QElapsedTimer timer; timer.start();

	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString mapname = "";
	__int64 floor = 0;
	checkString(TK, 1, &mapname);
	mapname = mapname.simplified();

	QStringList mapnames = mapname.split(rexOR, Qt::SkipEmptyParts);
	checkInteger(TK, 1, &floor);

	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	if (floor == 0 && mapname.isEmpty())
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("invalid map name or floor number"));
		return Parser::kServerNotReady;
	}

	auto check = [&injector, floor, mapnames]()
	{
		if (floor != 0)
			return floor == injector.server->getFloor();
		else
		{
			QString currentFloorName = injector.server->getFloorName();
			__int64 currentFloor = injector.server->getFloor();

			for (const QString& mapname : mapnames)
			{
				bool ok;
				__int64 fr = mapname.toLongLong(&ok);
				if (ok)
				{
					if (fr == currentFloor)
						return true;
				}
				else
				{
					if (mapname.startsWith("?"))
					{
						QString newName = mapname.mid(1);
						return currentFloorName.contains(newName);
					}
					else
						return mapname == currentFloorName;
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
	qDebug() << "init cost" << timer.elapsed() << "ms";
	return checkJump(TK, 3, bret, FailedJump);
}

__int64 Interpreter::waitdlg(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString cmpStr;
	__int64 dlgid = -1;
	if (!checkString(TK, 1, &cmpStr))
	{
		if (!checkInteger(TK, 1, &dlgid))
			return Parser::kArgError + 1ll;
	}
	else
		cmpStr = cmpStr.simplified();

	bool bret = false;
	if (dlgid != -1)
	{
		__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
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
		QStringList cmpStrs = cmpStr.split(rexOR, Qt::SkipEmptyParts);

		__int64 min = 1;
		__int64 max = MAX_DIALOG_LINE;
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError + 2ll;
		if (min == max)
		{
			++min;
			++max;
		}

		__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
		checkInteger(TK, 3, &timeout);

		auto check = [&injector, min, max, cmpStrs]()->bool
		{
			if (!injector.server->isDialogVisible())
				return false;

			if (cmpStrs.isEmpty() || cmpStrs.front().isEmpty())
				return true;

			QStringList dialogStrList = injector.server->currentDialog.get().linedatas;
			for (__int64 i = min; i <= max; ++i)
			{
				int index = i - 1;
				if (index < 0 || index >= dialogStrList.size())
					break;

				QString text = dialogStrList.value(index).simplified();
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

__int64 Interpreter::waitsay(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	__int64 min = 1;
	__int64 max = MAX_CHAT_HISTORY;
	if (!checkRange(TK, 1, &min, &max))
		return Parser::kArgError + 1ll;
	if (min == max)
	{
		++min;
		++max;
	}

	QString cmpStr;
	checkString(TK, 2, &cmpStr);
	cmpStr = cmpStr.simplified();
	if (cmpStr.isEmpty())
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("pre-compare string cannot be empty"));
		return Parser::kArgError + 2ll;
	}

	QStringList cmpStrs = cmpStr.split(rexOR, Qt::SkipEmptyParts);

	bool bret = false;
	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

	auto check = [&injector, min, max, cmpStrs]()->bool
	{
		for (__int64 i = min; i <= max; ++i)
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

		QVector<QPair<__int64, QString>> list = injector.server->chatQueue.values();

		for (__int64 i = min; i <= max; ++i)
		{
			if (i < 0)
				continue;
			if (i > list.size())
				break;
			QString text = list.value(i - 1).second.simplified();
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

__int64 Interpreter::waitpos(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString posStrs;
	QStringList posStrList;
	QList<QPoint> posList;
	__int64 x = 0, y = 0;

	__int64 timeoutIndex = 3;
	__int64 jumpIndex = 4;

	if (checkInteger(TK, 1, &x))
	{
		if (!checkInteger(TK, 2, &y))
			return Parser::kArgError + 2ll;

		posList.push_back(QPoint(x, y));
	}
	else if (checkString(TK, 1, &posStrs))
	{
		posStrs = posStrs.simplified();

		if (posStrs.isEmpty())
			return Parser::kArgError + 1ll;

		posStrList = posStrs.split(rexOR, Qt::SkipEmptyParts);
		if (posStrList.isEmpty())
			return Parser::kArgError + 1ll;

		for (const QString& posStr : posStrList)
		{
			QStringList pos = posStr.split(rexComma, Qt::SkipEmptyParts);
			if (pos.size() != 2)
				continue;

			bool ok1, ok2;
			__int64 x = pos.value(0).toLongLong(&ok1);
			__int64 y = pos.value(1).toLongLong(&ok2);
			if (ok1 && ok2)
				posList.push_back(QPoint(x, y));
		}

		if (posList.isEmpty())
			return Parser::kArgError + 1ll;

		timeoutIndex = 2;
		jumpIndex = 3;
	}
	else
		return Parser::kArgError + 1ll;

	auto check = [&injector, posList]()
	{
		QPoint pos = injector.server->getPoint();
		for (const QPoint& p : posList)
		{
			if (p == pos)
				return true;
		}
		return false;
	};

	bool bret = false;
	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

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
__int64 Interpreter::waitteam(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	PC pc = injector.server->getPC();

	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
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

__int64 Interpreter::waititem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	__int64 min = 0, max = static_cast<__int64>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
	bool isEquip = false;
	if (!checkRange(TK, 1, &min, &max))
	{
		QString partStr;
		checkString(TK, 1, &partStr);
		partStr = partStr.simplified();

		if (partStr.isEmpty())
			return Parser::kArgError + 1ll;

		if (partStr.toLower() == "all" || partStr.toLower() == QString("全部"))
		{
			min = 101;
			max = static_cast<__int64>(101 + CHAR_EQUIPPLACENUM);
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
	else if (min >= 101 && max <= static_cast<__int64>(100 + CHAR_EQUIPPLACENUM))
	{
		min -= 101;
		max -= 101;
	}

	QString itemName;
	checkString(TK, 2, &itemName);
	itemName = itemName.simplified();

	QString itemMemo;
	checkString(TK, 3, &itemMemo);
	itemMemo = itemMemo.simplified();

	if (itemName.isEmpty() && itemMemo.isEmpty())
		return Parser::kArgError + 1ll;

	__int64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 4, &timeout);

	injector.server->updateItemByMemory();

	bool bret = false;
	if (timeout == 0)
	{
		QVector<__int64> vec;
		bret = injector.server->getItemIndexsByName(itemName, itemMemo, &vec, min, max);
	}
	else
	{
		bret = waitfor(timeout, [&injector, itemName, itemMemo, min, max]()->bool
			{
				QVector<__int64> vec;
				return injector.server->getItemIndexsByName(itemName, itemMemo, &vec, min, max);
			});
	}

	return checkJump(TK, 5, bret, FailedJump);
}
