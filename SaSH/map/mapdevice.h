/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

#pragma once
#if _MSC_VER >= 1600 
#pragma execution_character_set("utf-8") 
#endif

#include <util.h>
#include <indexer.h>
#include "update/downloader.h"

class MapDevice : public Indexer
{
	Q_DISABLE_COPY_MOVE(MapDevice)
public:
	explicit MapDevice(long long index);
	virtual ~MapDevice();

	static void __fastcall loadHotData(Downloader& downloder);

	bool __fastcall readFromBinary(long long currentIndex, long long  floor, const QString& name, bool enableDraw = false, bool enableRewrite = false);

	bool __fastcall getMapDataByFloor(long long  floor, sa::map_t* map);

	bool __fastcall calcNewRoute(long long currentIndex, AStarDevice* pastar,
		long long floor, const QPoint& src, const QPoint& dst, const QSet<QPoint>& blockList, std::vector<QPoint>* pPaths);

	void __fastcall clear() const;

	void __fastcall clear(long long floor) const;

	[[nodiscard]] QPixmap __fastcall getPixmapByIndex(long long index) const;

	bool __fastcall saveAsBinary(long long currentIndex, sa::map_t map, const QString& fileName) const;

	long long __fastcall calcBestFollowPointByDstPoint(long long currentIndex, AStarDevice* pastar,
		long long floor, const QPoint& src, const QPoint& dst, QPoint* ret, bool enableExt, long long npcdir);

	bool __fastcall isPassable(long long currentIndex, AStarDevice* pastar, long long floor, const QPoint& src, const QPoint& dst);

	QString __fastcall getGround(long long currentIndex, long long floor, const QString& name, const QPoint& src);

	[[nodiscard]] QString __fastcall getCurrentPreHandleMapPath(long long currentIndex, long long floor) const;

private:
	[[nodiscard]] QString __fastcall getCurrentMapPath(long long floor) const;


	void __fastcall setMapDataByFloor(long long floor, const sa::map_t& map);
	void __fastcall setPixmapByIndex(long long index, const QPixmap& pix);

	bool __fastcall loadFromBinary(long long currentIndex, long long floor, sa::map_t* _map);

	[[nodiscard]] sa::ObjectType __fastcall getGroundType(const unsigned short data) const;
	[[nodiscard]] sa::ObjectType __fastcall getObjectType(const unsigned short data) const;

public:
#if 0
	struct CRGB
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		CRGB(uint8_t _r, uint8_t _g, uint8_t _b)
			: r(_r), g(_g), b(_b)
		{
		}

		bool operator==(const CRGB& rhs) const
		{
			return (r == rhs.r) && (g == rhs.g) && (b == rhs.b);
		}
	};

	struct cimage
	{
	public:
		cimage(const int width, const int height)
			: wp(width), hp(height), rgb(wp* hp * 3)
		{
		}
		uint8_t& r(int x, int y) { return rgb[(x + y * wp) * 3 + 2]; }
		uint8_t& g(int x, int y) { return rgb[(x + y * wp) * 3 + 1]; }
		uint8_t& b(int x, int y) { return rgb[(x + y * wp) * 3 + 0]; }

		void setPixel(const QPoint& p, const CRGB& color)
		{
			if (CHECKRANGE(p.y()))
			{
				rgb.at((p.x() + (hp - p.y()) * wp) * 3 + 2) = color.r;
				rgb.at((p.x() + (hp - p.y()) * wp) * 3 + 1) = color.g;
				rgb.at((p.x() + (hp - p.y()) * wp) * 3 + 0) = color.b;
			}
		}

		constexpr uint32_t w() const
		{
			return wp;
		}

		constexpr uint32_t h() const
		{
			return hp;
		}

		const uint8_t* data() const
		{
			return rgb.data();
		}

	private:
		int wp;
		int hp;
		std::vector<uint8_t> rgb;

		bool CHECKRANGE(int y) const
		{
			return (((h()) - (y)) > 0) && (((h()) - (y)) < h());
		};
	};
#endif

private:
	QString directory = "";
	mutable QMutex mutex_;
};

#if 0
template <class Stream>
Stream& operator<<(Stream& out, MapDevice::cimage const& img)
{
	uint32_t w = img.w(), h = img.h();
	uint32_t pad = w * -3 & 3;
	uint32_t total = 54 + 3 * w * h + pad * h;
	uint32_t head[13] = { total, 0, 54, 40, w, h, (24 << 16) | 1 };
	const char* rgb = reinterpret_cast<char const*>(img.data());

	out.write("BM", 2);
	out.write(reinterpret_cast<char*>(head), 52);
	for (uint32_t i = 0; i < h; ++i)
	{
		out.write(rgb + (3 * w * i), 3 * w);
		out.write(reinterpret_cast<char*>(&pad), pad);
	}
	return out;
}
#endif