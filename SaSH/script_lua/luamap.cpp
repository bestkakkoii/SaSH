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
#include <gamedevice.h>
#include "signaldispatcher.h"
#include "map/mapdevice.h"

long long CLuaMap::x()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	QPoint p = gamedevice.worker->getPoint();
	return p.x();

}
long long CLuaMap::y()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	QPoint p = gamedevice.worker->getPoint();
	return p.y();
}

std::tuple<long long, long long> CLuaMap::xy()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return std::make_tuple(-1, -1);

	QPoint p = gamedevice.worker->getPoint();
	return std::make_tuple(p.x(), p.y());
}

long long CLuaMap::floor()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return -1;

	return gamedevice.worker->getFloor();
}

std::string CLuaMap::getName()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return "";

	return util::toConstData(gamedevice.worker->getFloorName());
}

std::string CLuaMap::getGround()
{
	GameDevice& gamedevice = GameDevice::getInstance(index_);
	if (gamedevice.worker.isNull())
		return "";

	return util::toConstData(gamedevice.worker->getGround());
}

bool CLuaMap::isxy(long long x, long long y, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return false;

	QPoint pos = gamedevice.worker->getPoint();
	return pos == QPoint(x, y);
}

bool CLuaMap::isrect(long long x1, long long y1, long long x2, long long y2, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return false;
	QPoint pos = gamedevice.worker->getPoint();
	return pos.x() >= x1 && pos.x() <= x2 && pos.y() >= y1 && pos.y() <= y2;
}

bool CLuaMap::ismap(sol::object omap, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return false;

	if (omap.is<long long>())
	{
		return gamedevice.worker->getFloor() == omap.as<long long>();
	}

	QString mapNames = util::toQString(omap);
	QStringList mapNameList = mapNames.split(util::rexOR, Qt::SkipEmptyParts);
	bool ok = false;
	bool isExact = true;
	long long floor = 0;
	QString newName;
	for (const QString& it : mapNameList)
	{
		floor = it.toLongLong(&ok);
		if (ok && gamedevice.worker->getFloor() == floor)
			return true;

		newName = it;
		if (newName.startsWith("?"))
		{
			newName = newName.mid(1);
			isExact = false;
		}

		if (newName.isEmpty())
			continue;

		if (isExact && gamedevice.worker->getFloorName() == newName)
			return true;
		else if (gamedevice.worker->getFloorName().contains(newName))
			return true;
	}

	return false;
}

long long CLuaMap::setdir(sol::object p1, sol::object p2, sol::object p3, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString dirStr = "";
	if (p1.is<std::string>())
		dirStr = util::toQString(p1.as<std::string>());

	long long dir = -1;
	long long x = -1;
	if (p1.is<long long>())
	{
		dir = p1.as<long long>() - 1;
		x = p1.as<long long>();
	}

	long long y = -1;
	bool noWindow = false;
	if (p2.is<long long>())
	{
		y = p2.as<long long>();
		if (p3.is<bool>())
			noWindow = p3.as<bool>();
	}
	else
	{
		if (p2.is<bool>())
			noWindow = p2.as<bool>();
	}


	if (x != -1 && y != -1)
	{
		return gamedevice.worker->setCharFaceToPoint(QPoint(x, y));
	}

	dirStr = dirStr.toUpper().simplified();

	if (dir != -1 && dirStr.isEmpty() && dir >= 0 && dir < sa::MAX_DIR)
	{
		return gamedevice.worker->setCharFaceDirection(dir, noWindow);
	}
	else if (dir == -1 && !dirStr.isEmpty())
	{
		return gamedevice.worker->setCharFaceDirection(dirStr, noWindow);
	}

	return FALSE;
}

long long CLuaMap::walkpos(long long x, long long y, sol::object otimeout, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QPoint p(x, y);

	long long timeout = 5000;
	if (otimeout.is<long long>())
		timeout = otimeout.as<long long>();

	if (p.x() < 0 || p.x() >= 1500)
		return FALSE;

	if (p.y() < 0 || p.y() >= 1500)
		return FALSE;

	if (!gamedevice.worker->move(p))
		return FALSE;

	QThread::msleep(1);

	bool bret = luadebug::waitfor(s, timeout, [&s, this, &gamedevice, &p]()->bool
		{
			luadebug::checkBattleThenWait(s);
			bool result = gamedevice.worker->getPoint() == p;
			if (result)
			{
				gamedevice.worker->move(p);
			}
			return result;
		});

	return bret;
}

long long CLuaMap::move(sol::object obj, long long y, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long x = 0;
	QPoint p;
	QString dirStr;
	if (obj.is<long long>() || obj.is<double>())
	{
		x = obj.as<long long>();
		p = QPoint(static_cast<int>(x), static_cast<int>(y));
	}
	else if (obj.is<std::string>())
	{
		if (!sa::dirMap.contains(dirStr.toUpper().simplified()))
		{
			luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("invalid direction"));
			return FALSE;
		}

		sa::DirType dir = sa::dirMap.value(dirStr.toUpper().simplified());
		//計算出往該方向10格的坐標
		p = gamedevice.worker->getPoint() + util::fix_point.value(dir) * 10;
	}

	if (!gamedevice.worker->move(p))
		return FALSE;

	QThread::msleep(100);;
	return TRUE;
}

long long CLuaMap::packetMove(long long x, long long y, std::string sdir, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	QString qdir = util::toQString(sdir);

	return gamedevice.worker->move(pos, qdir);
}

long long CLuaMap::teleport(sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->warp();
}

long long CLuaMap::downLoad(sol::object ofloor, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long floor = -1;
	if (ofloor.is<long long>())
	{
		floor = ofloor.as<long long>();
	}

	if (floor >= 0 || floor == -1)
	{
		return gamedevice.worker->downloadMap(floor);
	}
	else if (floor == -2)
	{
		QString exePath = gamedevice.currentGameExePath;
		exePath = exePath.replace(QChar('\\'), QChar('/'));
		//去掉sa_8001.exe
		exePath = exePath.left(exePath.lastIndexOf(QChar('/')));
		//補上map 這裡是遊戲的地圖目錄
		exePath += "/map";

		//遍歷.dat
		struct map
		{
			long long floor;
			long long size;
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
				QString szfl = match.captured(1);
				int size = fileInfo.size();
				list.append(map{ szfl.toLongLong(), size });
			}
		}

		//文件小到大排序
		std::sort(list.begin(), list.end(), [](const map& ma, const map& mb)
			{
				return ma.size < mb.size;
			});

		for (const map& m : list)
		{
			gamedevice.worker->downloadMap(m.floor);
		}
	}
	else
	{
		luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("invalid floor"));
		return FALSE;
	}

	return TRUE;
}

bool __fastcall findPathProcess(
	long long currentIndex,
	AStarDevice* pastar,
	QPoint dst,
	long long steplen,
	long long step_cost,
	long long timeout,
	std::function<long long(QPoint& dst)> callback,
	long long callback_step_cost,
	sol::this_state s)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	HANDLE hProcess = gamedevice.getProcess();

	QString output = "";

	auto getPos = [hProcess, hModule, &gamedevice]()->QPoint
		{
			if (!gamedevice.worker.isNull())
				return gamedevice.worker->getPoint();
			else
				return QPoint();
		};

	if (gamedevice.worker.isNull())
		return false;

	long long floor = gamedevice.worker->getFloor();
	QPoint src(getPos());
	if (src == dst)
		return true;//已經抵達

	if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
	{
		output = QObject::tr("<findpath>start searching the path");
		luadebug::logExport(s, output, 4);
	}

	std::vector<QPoint> path;
	util::timer timer;
	QSet<QPoint> blockList;

	if (!gamedevice.worker->mapDevice.calcNewRoute(currentIndex, pastar, floor, src, dst, blockList, &path))
	{
		output = QObject::tr("[error] <findpath>unable to findpath from %1, %2 to %3, %4").arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y());
		luadebug::logExport(s, output, 6);
		return false;
	}

	long long cost = static_cast<long long>(timer.cost());
	if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
	{
		output = QObject::tr("<findpath>path found, from %1, %2 to %3, %4 cost:%5 step:%6")
			.arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y()).arg(cost).arg(path.size());
		luadebug::logExport(s, output, 4);
	}

	QPoint point;
	long long steplen_cache = -1;
	long long pathsize = path.size();
	long long current_floor = floor;

	timer.restart();

	//用於檢測卡點
	util::timer blockDetectTimer;
	QPoint lastPoint = src;
	QPoint lastTryPoint;
	long long recordedStep = -1;

	for (;;)
	{
		if (luadebug::checkStopAndPause(s))
			return false;
		luadebug::checkOnlineThenWait(s);

		if (gamedevice.worker.isNull())
		{
			if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
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

			if (!gamedevice.worker->move(point))
				continue;

			lastTryPoint = point;
			if (step_cost > 0)
				QThread::msleep(step_cost);
		}

		if (!luadebug::checkBattleThenWait(s))
		{
			src = getPos();
			if (!src.isNull() && src == dst)
			{
				cost = timer.cost();
				if (cost > 5000)
				{
					QThread::msleep(500);

					if (luadebug::checkStopAndPause(s))
						return false;
					if (gamedevice.worker.isNull())
					{
						if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
						{
							output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
							luadebug::logExport(s, output, 10);
						}
						break;
					}

					src = getPos();
					if (src.isNull() || src != dst)
						continue;

					gamedevice.worker->EO();

					src = getPos();
					if (src.isNull() || src != dst)
						continue;

					QThread::msleep(500);

					if (luadebug::checkStopAndPause(s))
						return false;

					if (gamedevice.worker.isNull())
					{
						if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
						{
							output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
							luadebug::logExport(s, output, 10);
						}
						break;
					}

					gamedevice.worker->EO();

					src = getPos();
					if (src.isNull() || src != dst)
						continue;
				}

				gamedevice.worker->move(dst);

				QThread::msleep(200);
				src = getPos();
				if (src.isNull() || src != dst)
					continue;

				if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
				{
					output = QObject::tr("<findpath>arrived destination, cost:%1").arg(cost);
					luadebug::logExport(s, output, 4);
				}
				return true;//已抵達true
			}

			if (luadebug::checkStopAndPause(s))
				return false;

			if (!gamedevice.worker->mapDevice.calcNewRoute(currentIndex, pastar, floor, src, dst, blockList, &path))
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

		if (blockDetectTimer.hasExpired(sa::MAX_TIMEOUT))
		{
			blockDetectTimer.restart();
			if (luadebug::checkStopAndPause(s))
				return false;

			if (gamedevice.worker.isNull())
			{
				if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
				{
					output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
					luadebug::logExport(s, output, 10);
				}
				break;
			}

			if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
			{
				output = QObject::tr("[warn] <findpath>detedted player ware blocked");
				luadebug::logExport(s, output, 10);
			}

			gamedevice.worker->EO();
			QThread::msleep(500);
			if (luadebug::checkStopAndPause(s))
				return false;

			//查看前方是否存在NPC阻擋
			QPoint blockPoint;
			sa::map_unit_t unit; bool hasNPCBlock = false;
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
			bool isAutoEscape = gamedevice.getEnableHash(util::kAutoEscapeEnable);
			bool isKNPC = gamedevice.getEnableHash(util::kKNPCEnable);

			for (const QPoint& it : util::fix_point)
			{
				blockPoint = src + it;
				if (!gamedevice.worker->findUnit(QString("%1|%2").arg(point.x()).arg(point.y()), sa::kObjectNPC, &unit) || !unit.isVisible)
					continue;
				hasNPCBlock = true;
				break;
			}

			if (hasNPCBlock && (unit.name.contains("願藏") || unit.name.contains("近藏")
				|| unit.name.contains("愿藏") || unit.name.contains("近藏")
				|| unit.name.startsWith("宮本")))
			{
				gamedevice.setEnableHash(util::kAutoEscapeEnable, false);
				gamedevice.setEnableHash(util::kKNPCEnable, true);
				emit signalDispatcher.applyHashSettingsToUI();
				gamedevice.worker->move(blockPoint);
				QThread::msleep(2000);
				luadebug::checkBattleThenWait(s);
				blockDetectTimer.restart();
				gamedevice.setEnableHash(util::kAutoEscapeEnable, isAutoEscape);
				gamedevice.setEnableHash(util::kKNPCEnable, isKNPC);
				emit signalDispatcher.applyHashSettingsToUI();
				continue;
			}

			if (luadebug::checkStopAndPause(s))
				return false;

			if (gamedevice.worker.isNull())
			{
				if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
				{
					output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
					luadebug::logExport(s, output, 10);
				}
				break;
			}

			//將正前方的坐標加入黑名單
			src = getPos();
			long long dir = gamedevice.worker->setCharFaceToPoint(lastTryPoint);
			if (dir == -1)
				dir = gamedevice.worker->getDir();
			blockPoint = src + util::fix_point.value(dir);
			blockList.insert(blockPoint);
			blockList.insert(lastTryPoint);
			if (recordedStep == -1)
				recordedStep = steplen_cache;
			else
				--recordedStep;

			continue;
		}

		if (luadebug::checkStopAndPause(s))
			return false;

		if (gamedevice.worker.isNull())
		{
			if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to interruption");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		if (timer.hasExpired(timeout))
		{
			if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to timeout");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		if (gamedevice.worker->getFloor() != current_floor)
		{
			if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to floor changed");
				luadebug::logExport(s, output, 10);
			}
			break;
		}

		if (callback != nullptr)
		{
			QThread::msleep(callback_step_cost);
			long long r = callback(dst);
			if (r == 1)
				callback = nullptr;
			else if (r == 2)
				break;
		}
	}
	return false;
}


long long CLuaMap::findPath(sol::object p1, sol::object p2, sol::object p3, sol::object p4, sol::object p5, sol::object ofunction, sol::object jump, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return FALSE;

	if (luadebug::checkStopAndPause(s))
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long x = -1;
	long long y = -1;
	QPoint p;
	long long steplen = 3;
	long long step_cost = 0;
	long long timeout = 180000;
	QString name;

	sol::protected_function func;

	if (p1.is<long long>())
		x = p1.as<long long>();
	else if (p1.is<std::string>())
		name = util::toQString(p1.as<std::string>());
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));

	if (x != -1)
	{
		if (p2.is<long long>())
			y = p2.as<long long>();
		else
			luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value type"));
		if (p3.is<long long>())
			steplen = p3.as<long long>();
		if (p4.is<long long>())
			step_cost = p4.as<long long>();
		if (p5.is<long long>() && p5.as<long long>() > 0)
			timeout = p5.as<long long>();

		if (ofunction.is<sol::protected_function>())
			func = ofunction.as<sol::protected_function>();

		p = QPoint(x, y);
	}
	else
	{
		if (p2.is<long long>())
			steplen = p2.as<long long>();
		if (p3.is<long long>())
			step_cost = p3.as<long long>();
		if (p4.is<long long>() && p4.as<long long>() > 0)
			timeout = p4.as<long long>();
		if (p5.is<sol::protected_function>())
			func = p5.as<sol::protected_function>();
	}

	AStarDevice astar;
	auto findNpcCallBack = [&gamedevice, &astar, currentIndex, &s](const QString& name, QPoint& dst, long long* pdir)->bool
		{
			if (luadebug::checkStopAndPause(s))
				return false;

			sa::map_unit_t unit;
			if (!gamedevice.worker->findUnit(name, sa::kObjectNPC, &unit, ""))
			{
				if (!gamedevice.worker->findUnit(name, sa::kObjectHuman, &unit, ""))
					return 0;//沒找到
			}

			long long dir = gamedevice.worker->mapDevice.calcBestFollowPointByDstPoint(
				currentIndex, &astar, gamedevice.worker->getFloor(), gamedevice.worker->getPoint(), unit.p, &dst, true, unit.dir);
			if (pdir)
				*pdir = dir;
			return dir != -1;//找到了
		};

	long long dir = -1;
	if (!name.isEmpty())
	{
		QString key = util::toQString(gamedevice.worker->getFloor());
		util::Config config(gamedevice.getPointFileName(), QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		QList<util::Config::MapData> datas = config.readMapData(key);
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
			{
				return FALSE;
			}
		}

		for (const util::Config::MapData& d : datas)
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
		auto stdfun = [&gamedevice, &func, &p, steplen, step_cost, timeout, &findNpcCallBack, &dir](QPoint& dst)->bool
			{
				return func(dst.x(), dst.y());
			};

		if (findPathProcess(currentIndex, &astar, p, steplen, step_cost, timeout, stdfun, 50, s))
		{
			if (!name.isEmpty() && (findNpcCallBack(name, p, &dir)) && dir != -1)
				gamedevice.worker->setCharFaceDirection(dir);
			return TRUE;
		}
		return FALSE;
	}

	if (findPathProcess(currentIndex, &astar, p, steplen, step_cost, timeout, nullptr, 0, s))
	{
		if (!name.isEmpty() && (findNpcCallBack(name, p, &dir)) && dir != -1)
			gamedevice.worker->setCharFaceDirection(dir);
		return TRUE;
	}

	return FALSE;
}

long long CLuaMap::findNPC(sol::object p1, sol::object nicknames, long long x, long long y, sol::object otimeout, sol::object ostepcost, sol::object oenableCrossWall, sol::this_state s)
{
	sol::state_view lua(s);
	long long currentIndex = lua["__INDEX"].get<long long>();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString cmpNpcName;
	long long modelid = -1;
	if (p1.is<std::string>())
	{
		cmpNpcName = util::toQString(p1.as<std::string>());
	}
	else if (p1.is<long long>())
	{
		modelid = p1.as<long long>();
	}
	else
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 1, QObject::tr("invalid value type"));


	QString cmpFreeName;
	if (nicknames.is<std::string>())
		cmpFreeName = util::toQString(nicknames.as<std::string>());

	if (cmpFreeName.isEmpty() && cmpNpcName.isEmpty() && modelid == -1)
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_PARAM_TYPE, false, 2, QObject::tr("invalid value type"));

	QPoint p(x, y);

	long long timeout = 5000LL * 36;
	if (otimeout.is<long long>() && otimeout.as<long long>() > 0)
		timeout = otimeout.as<long long>();

	bool enableCrossWall = true;
	if (oenableCrossWall.is<bool>())
		enableCrossWall = oenableCrossWall.as<bool>();

	long long callback_step_cost = 50;
	if (ostepcost.is<long long>() && ostepcost.as<long long>() > 0)
		callback_step_cost = ostepcost.as<long long>();

	sa::map_unit_t unit;
	long long dir = -1;
	AStarDevice astar;
	auto findNpcCallBack = [&gamedevice, &unit, currentIndex, cmpNpcName, cmpFreeName, modelid, enableCrossWall, &dir, &astar](QPoint& dst)->bool
		{
			if (modelid > 0)
			{
				if (!gamedevice.worker->findUnit("", sa::kObjectNPC, &unit, "", modelid))
				{
					return 0;//沒找到
				}
			}
			else if (!gamedevice.worker->findUnit(cmpNpcName, sa::kObjectNPC, &unit, cmpFreeName))
			{
				if (!gamedevice.worker->findUnit(cmpNpcName, sa::kObjectHuman, &unit, cmpFreeName))
					return 0;//沒找到
			}

			//找到了則計算最加靠近座標和面像方位
			dir = gamedevice.worker->mapDevice.calcBestFollowPointByDstPoint(
				currentIndex, &astar, gamedevice.worker->getFloor(), gamedevice.worker->getPoint(), unit.p, &dst, enableCrossWall, unit.dir);
			return dir != -1 ? 1 : 0;
		};

	if (findPathProcess(currentIndex, &astar, p, 1, 0, timeout, findNpcCallBack, callback_step_cost, s) && dir != -1)
	{
		gamedevice.worker->setCharFaceDirection(dir);
		return TRUE;
	}

	return FALSE;
}