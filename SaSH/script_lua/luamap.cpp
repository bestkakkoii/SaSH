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

	injector.server->setPlayerFaceDirection(--dir);

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

	injector.server->setPlayerFaceToPoint(pos);

	return TRUE;
}

qint64 CLuaMap::setDir(std::string sdir, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QString qdir = QString::fromUtf8(sdir.c_str());

	injector.server->setPlayerFaceDirection(qdir);

	return TRUE;
}

qint64 CLuaMap::move(qint64 x, qint64 y, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	if (injector.server.isNull())
		return FALSE;

	luadebug::checkBattleThenWait(s);

	QPoint pos(static_cast<int>(x), static_cast<int>(y));

	injector.server->move(pos);

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

	QString qdir = QString::fromUtf8(sdir.c_str());

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
		//補上map
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

qint64 CLuaMap::findPath(qint64 x, qint64 y, qint64 steplen, qint64 timeout, sol::object ofunction, sol::object ocallbackSpeed, sol::this_state s)
{
	sol::state_view lua(s);
	Injector& injector = Injector::getInstance(lua["_INDEX"].get<qint64>());
	qint64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	QPoint dst(static_cast<int>(x), static_cast<int>(y));

	bool noAnnounce = false;
	bool isDebug = injector.getEnableHash(util::kScriptDebugModeEnable);
	if (!isDebug)
		noAnnounce = true;

	qint64 callbackSpeed = 50;
	if (ocallbackSpeed.valid() && ocallbackSpeed.is<qint64>())
	{
		callbackSpeed = ocallbackSpeed.as<qint64>();
	}


	auto getPos = [hProcess, hModule, &injector]()->QPoint
	{
		if (!injector.server.isNull())
			return injector.server->getPoint();
		else
			return QPoint();
	};

	if (injector.server.isNull())
		return FALSE;

	qint64 floor = injector.server->nowFloor;
	QPoint src(getPos());
	if (src == dst)
		return true;//已經抵達

	map_t _map;
	QSharedPointer<MapAnalyzer> mapAnalyzer = injector.server->mapAnalyzer;
	if (!mapAnalyzer.isNull() && mapAnalyzer->readFromBinary(floor, injector.server->nowFloorName))
	{
		if (mapAnalyzer.isNull() || !mapAnalyzer->getMapDataByFloor(floor, &_map))
			return FALSE;
	}
	else
		return FALSE;

	if (!noAnnounce && !injector.server.isNull())
		injector.server->announce(QObject::tr("<findpath>start searching the path"));//"<尋路>開始搜尋路徑"

	std::vector<QPoint> path;
	QElapsedTimer timer; timer.start();
	if (mapAnalyzer.isNull() || !mapAnalyzer->calcNewRoute(_map, src, dst, &path))
	{
		if (!noAnnounce && !injector.server.isNull())
			injector.server->announce(QObject::tr("<findpath>unable to findpath"));//"<尋路>找不到路徑"
		return FALSE;
	}

	qint64 cost = static_cast<qint64>(timer.elapsed());
	if (!noAnnounce && !injector.server.isNull())
		injector.server->announce(QObject::tr("<findpath>path found, cost:%1 step:%2").arg(cost).arg(path.size()));//"<尋路>成功找到路徑，耗時：%1"

	sol::safe_function callBack;
	if (ofunction.valid() && ofunction.is< sol::safe_function>())
	{
		callBack = ofunction.as< sol::safe_function>();
	}

	QPoint point;
	qint64 steplen_cache = -1;
	qint64 pathsize = path.size();
	qint64 current_floor = floor;

	timer.restart();

	//用於檢測卡點
	QElapsedTimer blockDetectTimer; blockDetectTimer.start();
	QPoint lastPoint = src;

	for (;;)
	{
		if (injector.server.isNull())
			break;

		luadebug::checkStopAndPause(s);

		src = getPos();

		steplen_cache = steplen;

		for (;;)
		{
			if (!((steplen_cache) >= (pathsize)))
				break;
			--steplen_cache;
		}

		if (steplen_cache >= 0 && (steplen_cache < pathsize))
		{
			if (lastPoint != src)
			{
				blockDetectTimer.restart();
				lastPoint = src;
			}

			point = path.at(steplen_cache);
			injector.server->move(point);
		}

		if (!luadebug::checkBattleThenWait(s))
		{
			src = getPos();
			if (src == dst)
			{
				cost = timer.elapsed();
				if (cost > 5000)
				{
					QThread::msleep(500);

					if (injector.server.isNull())
						break;

					if (getPos() != dst)
						continue;

					injector.server->EO();

					if (getPos() != dst)
						continue;

					QThread::msleep(500);

					if (injector.server.isNull())
						break;

					if (getPos() != dst)
						continue;
				}

				injector.server->move(dst);

				QThread::msleep(100);
				if (getPos() != dst)
					continue;

				if (!noAnnounce && !injector.server.isNull())
					injector.server->announce(QObject::tr("<findpath>arrived destination, cost:%1").arg(timer.elapsed()));//"<尋路>已到達目的地，耗時：%1"
				return TRUE;//已抵達true
			}

			if (mapAnalyzer.isNull() || !mapAnalyzer->calcNewRoute(_map, src, dst, &path))
				break;

			pathsize = path.size();
		}

		if (blockDetectTimer.hasExpired(5000))
		{
			blockDetectTimer.restart();
			if (injector.server.isNull())
				break;

			injector.server->announce(QObject::tr("<findpath>detedted player ware blocked"));
			injector.server->EO();
			QThread::msleep(500);

			if (injector.server.isNull())
				break;

			//往隨機8個方向移動
			point = getPos();
			lastPoint = point;
			point = point + util::fix_point.at(QRandomGenerator::global()->bounded(0, 7));
			injector.server->move(point);
			QThread::msleep(100);

			continue;
		}

		if (injector.server.isNull())
			break;

		luadebug::checkStopAndPause(s);

		if (timer.hasExpired(timeout))
		{
			if (!injector.server.isNull())
				injector.server->announce(QObject::tr("<findpath>stop finding path due to timeout"));//"<尋路>超時，放棄尋路"
			break;
		}

		if (injector.server->nowFloor != current_floor)
		{
			injector.server->announce(QObject::tr("<findpath>stop finding path due to map changed"));//"<尋路>地圖已變更，放棄尋路"
			break;
		}

		if (callBack.valid())
		{
			QThread::msleep(callbackSpeed);
			sol::table tpos = lua.create_table();
			tpos["x"] = src.x();
			tpos["y"] = src.y();
			auto result = callBack(tpos);
			if (result.valid() && result.get_type() == sol::type::number)
			{
				if (tpos.is<sol::table>() && tpos["x"].is<qint64>() && tpos["y"].is<qint64>())
					dst = QPoint(tpos["x"], tpos["y"]);
				qint64 r = result.get<qint64>();
				if (r == -1)
				{
					return FALSE;
				}
				else if (r == -2)
				{
					//銷毀callback
					callBack = sol::safe_function();
				}
			}
		}
	}
	return FALSE;
}