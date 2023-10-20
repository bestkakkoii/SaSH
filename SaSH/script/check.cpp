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
long long Interpreter::waitpet(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	QString petName;
	checkString(TK, 1, &petName);
	petName = petName.simplified();

	if (petName.isEmpty())
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("pet name cannot be empty"));
		return Parser::kArgError + 1ll;
	}

	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	checkOnlineThenWait();

	bool bret = false;
	if (timeout == 0)
	{
		QVector<long long> v;
		bret = injector.worker->getPetIndexsByName(petName, &v);
	}
	else
	{
		bret = waitfor(timeout, [&injector, petName]()->bool
			{
				QVector<long long> v;
				return injector.worker->getPetIndexsByName(petName, &v);
			});
	}

	return checkJump(TK, 3, bret, FailedJump);
}

long long Interpreter::waitmap(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	QElapsedTimer timer; timer.start();

	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString mapname = "";
	long long floor = 0;
	checkString(TK, 1, &mapname);
	mapname = mapname.simplified();

	QStringList mapnames = mapname.split(util::rexOR, Qt::SkipEmptyParts);
	checkInteger(TK, 1, &floor);

	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 2, &timeout);

	if (floor == 0 && mapname.isEmpty())
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("invalid map name or floor number"));
		return Parser::kServerNotReady;
	}

	auto check = [&injector, floor, mapnames]()
		{
			if (floor != 0)
				return floor == injector.worker->getFloor();
			else
			{
				QString currentFloorName = injector.worker->getFloorName();
				long long currentFloor = injector.worker->getFloor();

				for (const QString& mapname : mapnames)
				{
					bool ok;
					long long fr = mapname.toLongLong(&ok);
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
		injector.worker->EO();
	qDebug() << "init cost" << timer.elapsed() << "ms";
	return checkJump(TK, 3, bret, FailedJump);
}

long long Interpreter::waitdlg(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString cmpStr;
	long long dlgid = -1;
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
		long long timeout = DEFAULT_FUNCTION_TIMEOUT;
		checkInteger(TK, 2, &timeout);

		if (timeout == 0)
			bret = injector.worker->currentDialog.get().dialogid == dlgid;
		else
		{
			bret = waitfor(timeout, [&injector, dlgid]()->bool
				{
					if (!injector.worker->isDialogVisible())
						return false;

					return injector.worker->currentDialog.get().dialogid == dlgid;
				});
		}

		return checkJump(TK, 3, bret, FailedJump);
	}
	else
	{
		QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

		long long min = 1;
		long long max = MAX_DIALOG_LINE;
		if (!checkRange(TK, 2, &min, &max))
			return Parser::kArgError + 2ll;
		if (min == max)
		{
			++min;
			++max;
		}

		long long timeout = DEFAULT_FUNCTION_TIMEOUT;
		checkInteger(TK, 3, &timeout);

		auto check = [&injector, min, max, cmpStrs]()->bool
			{
				if (!injector.worker->isDialogVisible())
					return false;

				if (cmpStrs.isEmpty() || cmpStrs.front().isEmpty())
					return true;

				QStringList dialogStrList = injector.worker->currentDialog.get().linedatas;
				for (long long i = min; i <= max; ++i)
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

long long Interpreter::waitsay(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	long long min = 1;
	long long max = MAX_CHAT_HISTORY;
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

	QStringList cmpStrs = cmpStr.split(util::rexOR, Qt::SkipEmptyParts);

	bool bret = false;
	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 3, &timeout);

	auto check = [&injector, min, max, cmpStrs]()->bool
		{
			for (long long i = min; i <= max; ++i)
			{
				QString text = injector.worker->getChatHistory(i - 1).simplified();
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

			QVector<QPair<long long, QString>> list = injector.worker->chatQueue.values();

			for (long long i = min; i <= max; ++i)
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

long long Interpreter::waitpos(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	QString posStrs;
	QStringList posStrList;
	QList<QPoint> posList;
	long long x = 0, y = 0;

	long long timeoutIndex = 3;
	long long jumpIndex = 4;

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

		posStrList = posStrs.split(util::rexOR, Qt::SkipEmptyParts);
		if (posStrList.isEmpty())
			return Parser::kArgError + 1ll;

		for (const QString& posStr : posStrList)
		{
			QStringList pos = posStr.split(util::rexComma, Qt::SkipEmptyParts);
			if (pos.size() != 2)
				continue;

			bool ok1, ok2;
			long long x = pos.value(0).toLongLong(&ok1);
			long long y = pos.value(1).toLongLong(&ok2);
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
			QPoint pos = injector.worker->getPoint();
			for (const QPoint& p : posList)
			{
				if (p == pos)
					return true;
			}
			return false;
		};

	bool bret = false;
	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
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
long long Interpreter::waitteam(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	PC pc = injector.worker->getPC();

	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
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

long long Interpreter::waititem(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.worker.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	long long min = 0, max = static_cast<long long>(MAX_ITEM - CHAR_EQUIPPLACENUM - 1);
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
			max = static_cast<long long>(101 + CHAR_EQUIPPLACENUM);
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
	else if (min >= 101 && max <= static_cast<long long>(100 + CHAR_EQUIPPLACENUM))
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

	long long timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 4, &timeout);

	injector.worker->updateItemByMemory();

	bool bret = false;
	if (timeout == 0)
	{
		QVector<long long> vec;
		bret = injector.worker->getItemIndexsByName(itemName, itemMemo, &vec, min, max);
	}
	else
	{
		bret = waitfor(timeout, [&injector, itemName, itemMemo, min, max]()->bool
			{
				QVector<long long> vec;
				return injector.worker->getItemIndexsByName(itemName, itemMemo, &vec, min, max);
			});
	}

	return checkJump(TK, 5, bret, FailedJump);
}
