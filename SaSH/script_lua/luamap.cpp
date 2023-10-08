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
#include "clua.h"
#include "injector.h"
#include "signaldispatcher.h"
#include "map/mapanalyzer.h"

qint64 CLuaMap::setDir(qint64 dir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->setCharFaceDirection(--dir);

	return TRUE;
}

qint64 CLuaMap::setDir(qint64 x, qint64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	injector.server->setCharFaceToPoint(pos);

	return TRUE;
}

qint64 CLuaMap::setDir(std::string sdir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString qdir = util::toQString(sdir);

	injector.server->setCharFaceDirection(qdir);

	return TRUE;
}

qint64 CLuaMap::move(sol::object obj, qint64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	qint64 x = 0;
	QPoint p;
	QString dirStr;
	if (obj.is<qint64>() || obj.is<double>())
	{
		x = obj.as<qint64>();
		p = QPoint(static_cast<int>(x), static_cast<int>(y));
	}
	else if (obj.is<std::string>())
	{
		if (!dirMap.contains(dirStr.toUpper().simplified()))
			return FALSE;

		DirType dir = dirMap.value(dirStr.toUpper().simplified());
		//計算出往該方向10格的坐標
		p = injector.server->getPoint() + util::fix_point.value(dir) * 10;
	}

	injector.server->move(p);
	QThread::msleep(100);
	return TRUE;
}

qint64 CLuaMap::packetMove(qint64 x, qint64 y, std::string sdir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	QString qdir = util::toQString(sdir);

	injector.server->move(pos, qdir);

	return TRUE;
}

qint64 CLuaMap::teleport(sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	injector.server->warp();

	return TRUE;
}

qint64 CLuaMap::downLoad(sol::object ofloor, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	qint64 floor = -1;
	if (ofloor.is<qint64>())
	{
		floor = ofloor.as<qint64>();
	}

	if (floor >= 0 || floor == -1)
	{
		injector.server->downloadMap(floor);
	}
	else if (floor == -2)
	{
		QString exePath = injector.currentGameExePath;
		exePath = exePath.replace(QChar('\\'), QChar('/'));
		//去掉sa_8001.exe
		exePath = exePath.left(exePath.lastIndexOf(QChar('/')));
		//補上map 這裡是遊戲的地圖目錄
		exePath += "/map";

		//遍歷.dat
		struct map
		{
			qint64 floor;
			qint64 size;
		};

		QList<map> list;
		QDir dir(exePath);
		dir.setFilter(QDir::Files | QDir::NoSymLinks);
		QFileInfoList fileInfoList = dir.entryInfoList();
		static const QRegularExpression reg("(\\d+).dat");
		for (QFileInfo& fileInfo : fileInfoList)
		{
			QString fileName = fileInfo.fileName();
			QRegularExpressionMatch match = reg.match(fileName.toLower());
			if (match.hasMatch())
			{
				QString floor = match.captured(1);
				int size = fileInfo.size();
				list.append(map{ floor.toLongLong(), size });
			}
		}

		//文件小到大排序
		std::sort(list.begin(), list.end(), [](const map& ma, const map& mb)
			{
				return ma.size < mb.size;
			});

		for (const map& m : list)
		{
			injector.server->downloadMap(m.floor);
		}
	}
	else
		return FALSE;

	return TRUE;
}

bool __fastcall findPathProcess(
	qint64 currentIndex,
	QPoint dst,
	qint64 steplen,
	qint64 step_cost,
	qint64 timeout,
	std::function<qint64(QPoint& dst)> callback,
	sol::this_state s)
{
	Injector& injector = Injector::getInstance(currentIndex);
	qint64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	QString output = "";

	auto getPos = [hProcess, hModule, &injector]()->QPoint
	{
		if (!injector.server.isNull())
			return injector.server->getPoint();
		else
			return QPoint();
	};

	if (injector.server.isNull())
		return false;

	qint64 floor = injector.server->getFloor();
	QPoint src(getPos());
	if (src == dst)
		return true;//已經抵達

	QSharedPointer<MapAnalyzer> mapAnalyzer = injector.server->mapAnalyzer;
	if (!mapAnalyzer.isNull())
	{
		if (mapAnalyzer.isNull())
			return false;
	}
	else
		return false;

	if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
	{
		output = QObject::tr("<findpath>start searching the path");
		luadebug::logExport(s, output, 4);
	}

	CAStar astar;
	std::vector<QPoint> path;
	QElapsedTimer timer; timer.start();
	QSet<QPoint> blockList;
	qint64 nret = -1;
	if (mapAnalyzer.isNull())
		return false;

	if (!mapAnalyzer->calcNewRoute(astar, floor, src, dst, blockList, &path))
	{
		output = QObject::tr("[error] <findpath>unable to findpath from %1, %2 to %3, %4").arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y());
		luadebug::logExport(s, output, 6);
		return false;
	}

	qint64 cost = static_cast<qint64>(timer.elapsed());
	if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
	{
		output = QObject::tr("<findpath>path found, from %1, %2 to %3, %4 cost:%5 step:%6")
			.arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y()).arg(cost).arg(path.size());
		luadebug::logExport(s, output, 4);
	}

	QPoint point;
	qint64 steplen_cache = -1;
	qint64 pathsize = path.size();
	qint64 current_floor = floor;

	timer.restart();

	//用於檢測卡點
	QElapsedTimer blockDetectTimer; blockDetectTimer.start();
	QPoint lastPoint = src;
	QPoint lastTryPoint;
	qint64 recordedStep = -1;

	for (;;)
	{
		luadebug::checkStopAndPause(s);
		luadebug::checkOnlineThenWait(s);

		if (injector.server.isNull())
		{
			if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		src = getPos();

		steplen_cache = steplen;

		for (;;)
		{
			if (!((steplen_cache) >= (pathsize)))
				break;
			--steplen_cache;
		}

		if (recordedStep >= 0)
			steplen_cache = recordedStep;

		if (steplen_cache >= 0 && (steplen_cache < pathsize))
		{
			if (lastPoint != src)
			{
				blockDetectTimer.restart();
				lastPoint = src;
			}

			point = path.at(steplen_cache);
			injector.server->move(point);
			lastTryPoint = point;
			if (step_cost > 0)
				QThread::msleep(step_cost);
		}

		if (!luadebug::checkBattleThenWait(s))
		{
			src = getPos();
			if (!src.isNull() && src == dst)
			{
				cost = timer.elapsed();
				if (cost > 5000)
				{
					QThread::msleep(500);

					luadebug::checkStopAndPause(s);
					if (injector.server.isNull())
					{
						if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
						{
							output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
							luadebug::logExport(s, output, 10);
						}
						break;
					}

					src = getPos();
					if (src.isNull() || src != dst)
						continue;

					injector.server->EO();

					src = getPos();
					if (src.isNull() || src != dst)
						continue;

					QThread::msleep(500);

					luadebug::checkStopAndPause(s);
					if (injector.server.isNull())
					{
						if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
						{
							output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
							luadebug::logExport(s, output, 10);
						}
						break;
					}

					injector.server->EO();

					src = getPos();
					if (src.isNull() || src != dst)
						continue;
				}

				injector.server->move(dst);

				QThread::msleep(200);
				src = getPos();
				if (src.isNull() || src != dst)
					continue;

				if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
				{
					output = QObject::tr("<findpath>arrived destination, cost:%1").arg(cost);
					luadebug::logExport(s, output, 4);
				}
				return true;//已抵達true
			}

			luadebug::checkStopAndPause(s);

			if (mapAnalyzer.isNull())
				break;

			if (!mapAnalyzer->calcNewRoute(astar, floor, src, dst, blockList, &path))
			{
				output = QObject::tr("[error] <findpath>unable to findpath from %1, %2 to %3, %4").arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y());
				luadebug::logExport(s, output, 6);
				break;
			}

			pathsize = path.size();
		}
		else
		{
			src = getPos();
		}

		if (blockDetectTimer.hasExpired(5000))
		{
			blockDetectTimer.restart();
			luadebug::checkStopAndPause(s);
			if (injector.server.isNull())
			{
				if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
				{
					output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
					luadebug::logExport(s, output, 10);
				}
				break;
			}

			if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			{
				output = QObject::tr("[warn] <findpath>detedted player ware blocked");
				luadebug::logExport(s, output, 10);
			}
			injector.server->EO();
			QThread::msleep(500);
			luadebug::checkStopAndPause(s);

			//查看前方是否存在NPC阻擋
			QPoint point;
			mapunit_t unit; bool hasNPCBlock = false;
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
			bool isAutoEscape = injector.getEnableHash(util::kAutoEscapeEnable);
			bool isKNPC = injector.getEnableHash(util::kKNPCEnable);

			for (const QPoint& it : util::fix_point)
			{
				point = src + it;
				if (!injector.server->findUnit(QString("%1|%2").arg(point.x()).arg(point.y()), util::OBJ_NPC, &unit) || !unit.isVisible)
					continue;
				hasNPCBlock = true;
				break;
			}

			if (hasNPCBlock && (unit.name.contains("願藏") || unit.name.contains("近藏")
				|| unit.name.contains("愿藏") || unit.name.contains("近藏")
				|| unit.name.startsWith("宮本")))
			{
				injector.setEnableHash(util::kAutoEscapeEnable, false);
				injector.setEnableHash(util::kKNPCEnable, true);
				emit signalDispatcher.applyHashSettingsToUI();
				injector.server->move(point);
				QThread::msleep(2000);
				luadebug::checkBattleThenWait(s);
				blockDetectTimer.restart();
				injector.setEnableHash(util::kAutoEscapeEnable, isAutoEscape);
				injector.setEnableHash(util::kKNPCEnable, isKNPC);
				emit signalDispatcher.applyHashSettingsToUI();
				continue;
			}

			luadebug::checkStopAndPause(s);
			if (injector.server.isNull())
			{
				if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
				{
					output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
					luadebug::logExport(s, output, 10);
				}
				break;
			}

			//將正前方的坐標加入黑名單
			src = getPos();
			qint64 dir = injector.server->setCharFaceToPoint(lastTryPoint);
			if (dir == -1)
				dir = injector.server->getDir();
			point = src + util::fix_point.value(dir);
			blockList.insert(point);
			blockList.insert(lastTryPoint);
			if (recordedStep == -1)
				recordedStep = steplen_cache;
			else
				--recordedStep;
			continue;
		}

		luadebug::checkStopAndPause(s);
		if (injector.server.isNull())
		{
			if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		if (timer.hasExpired(timeout))
		{
			if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to timeout");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		if (injector.server->getFloor() != current_floor)
		{
			if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to floor changed");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		if (callback != nullptr)
		{
			QThread::msleep(50);
			qint64 r = callback(dst);
			if (r == 1)
				callback = nullptr;
			else if (r == 2)
				break;
		}
	}
	return false;
}


qint64 CLuaMap::findPath(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object ofunction, sol::object jump, sol::this_state s)
{
	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	Injector& injector = Injector::getInstance(currentIndex);
	if (jump.is<qint64>() || jump.is<std::string>())
		lua["_JUMP"] = jump;
	else
		lua["_JUMP"] = sol::lua_nil;
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkStopAndPause(s);
	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	qint64 x = -1;
	qint64 y = -1;
	QPoint p;
	qint64 steplen = 3;
	qint64 step_cost = 0;
	qint64 timeout = 180000;
	QString name;

	sol::protected_function func;

	if (p1.is<qint64>())
		x = p1.as<qint64>();
	else if (p1.is<std::string>())
		name = util::toQString(p1.as<std::string>());
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	if (x != -1)
	{
		if (p2.is<qint64>())
			y = p2.as<qint64>();
		else
			luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value type"));
		if (p3.is<qint64>())
			steplen = p3.as<qint64>();
		if (p4.is<qint64>())
			step_cost = p4.as<qint64>();
		if (p5.is<qint64>())
			timeout = p5.as<qint64>();

		if (ofunction.is<sol::protected_function>())
			func = ofunction.as<sol::protected_function>();

		p = QPoint(x, y);
	}
	else
	{
		if (p2.is<qint64>())
			steplen = p2.as<qint64>();
		if (p3.is<qint64>())
			step_cost = p3.as<qint64>();
		if (p4.is<qint64>())
			timeout = p4.as<qint64>();
		if (p5.is<sol::protected_function>())
			func = p5.as<sol::protected_function>();
	}

	CAStar astar;
	auto findNpcCallBack = [&injector, &astar, &s](const QString& name, QPoint& dst, qint64* pdir)->bool
	{
		luadebug::checkStopAndPause(s);

		mapunit_s unit;
		if (!injector.server->findUnit(name, util::OBJ_NPC, &unit, ""))
		{
			if (!injector.server->findUnit(name, util::OBJ_HUMAN, &unit, ""))
				return 0;//沒找到
		}

		qint64 dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(astar, injector.server->getFloor(), injector.server->getPoint(), unit.p, &dst, true, unit.dir);
		if (pdir)
			*pdir = dir;
		return dir != -1;//找到了
	};

	qint64 dir = -1;
	if (!name.isEmpty())
	{
		QString key = util::toQString(injector.server->getFloor());
		util::Config config(injector.getPointFileName());
		QList<util::MapData> datas = config.readMapData(key);
		if (datas.isEmpty())
			return FALSE;

		QPoint point;
		QStringList strList = name.split(util::rexOR, Qt::SkipEmptyParts);
		if (strList.size() == 2)
		{
			bool ok = false;

			point.setX(strList.value(0).toLongLong(&ok));
			if (ok)
				point.setY(strList.value(1).toLongLong(&ok));
			if (!ok)
				return FALSE;
		}

		for (const util::MapData& d : datas)
		{
			if (d.name.isEmpty())
				return FALSE;

			if (point == QPoint(d.x, d.y))
				p = QPoint(d.x, d.y);
			else if (d.name == name)
				p = QPoint(d.x, d.y);
			else if (name.startsWith("?"))
			{
				QString newName = name.mid(0);
				if (d.name.contains(newName))
				{
					p = QPoint(d.x, d.y);
					break;
				}
			}
		}

		if (p.isNull())
			return FALSE;
	}

	if (func.valid())
	{
		auto stdfun = [&injector, &func, &p, steplen, step_cost, timeout, &findNpcCallBack, &dir](QPoint& dst)->bool
		{
			return func(dst.x(), dst.y());
		};

		if (findPathProcess(currentIndex, p, steplen, step_cost, timeout, stdfun, s))
		{
			if (!name.isEmpty() && (findNpcCallBack(name, p, &dir)) && dir != -1)
				injector.server->setCharFaceDirection(dir);
			lua["_JUMP"] = sol::lua_nil;
			return TRUE;
		}
		return FALSE;
	}

	if (findPathProcess(currentIndex, p, steplen, step_cost, timeout, nullptr, s))
	{
		if (!name.isEmpty() && (findNpcCallBack(name, p, &dir)) && dir != -1)
			injector.server->setCharFaceDirection(dir);
		lua["_JUMP"] = sol::lua_nil;
		return TRUE;
	}

	return FALSE;
}

qint64 CLuaMap::moveToNPC(sol::object p1, sol::object nicknames, qint64 x, qint64 y, qint64 otimeout, sol::object jump, sol::this_state s)
{
	sol::state_view lua(s);
	qint64 currentIndex = lua["_INDEX"].get<qint64>();
	Injector& injector = Injector::getInstance(currentIndex);
	if (jump.is<qint64>() || jump.is<std::string>())
		lua["_JUMP"] = jump;
	else
		lua["_JUMP"] = sol::lua_nil;

	if (injector.server.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString cmpNpcName;
	qint64 modelid = -1;
	if (p1.is<std::string>())
	{
		cmpNpcName = util::toQString(p1.as<std::string>());
	}
	else if (p1.is<qint64>())
	{
		modelid = p1.as<qint64>();
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));


	QString cmpFreeName;
	if (nicknames.is<std::string>())
		cmpFreeName = util::toQString(nicknames.as<std::string>());

	if (cmpFreeName.isEmpty() && cmpNpcName.isEmpty() && modelid == -1)
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value type"));

	QPoint p(x, y);

	qint64 timeout = 5000LL * 36;

	mapunit_s unit;
	qint64 dir = -1;
	CAStar astar;
	auto findNpcCallBack = [&injector, &unit, cmpNpcName, cmpFreeName, modelid, &dir, &astar](QPoint& dst)->bool
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

		dir = injector.server->mapAnalyzer->calcBestFollowPointByDstPoint(astar, injector.server->getFloor(), injector.server->getPoint(), unit.p, &dst, true, unit.dir);
		return dir != -1 ? 1 : 0;//找到了
	};

	if (findPathProcess(currentIndex, p, 1, 0, timeout, findNpcCallBack, s) && dir != -1)
	{
		injector.server->setCharFaceDirection(dir);
		lua["_JUMP"] = sol::lua_nil;
		return TRUE;
	}

	return FALSE;
}