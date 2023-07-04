#pragma once
#pragma execution_character_set("utf-8")
#include <Windows.h>
#include <unordered_map>
#include <string>
#include <QPoint>
#include <QString>
#include <util.h>

static const QHash<util::ObjectType, QColor> MAP_COLOR_HASH = {
	{ util::OBJ_UNKNOWN,  QColor(0, 0, 1) },		 //黑
	{ util::OBJ_ROAD,     QColor(64, 74, 41) },	     //墨綠
	{ util::OBJ_UP,       QColor(255, 128, 128) },   //乳紅
	{ util::OBJ_DOWN,     QColor(128, 128, 255) },   //乳紫
	{ util::OBJ_JUMP,     QColor(200, 200, 65) },	 //乳黃
	{ util::OBJ_WARP,     QColor(200, 137, 48) },    //乳橘
	{ util::OBJ_WALL,     QColor(35, 35, 35) },	     //灰黑
	{ util::OBJ_ROCK,     QColor(46, 55, 25) },		 //灰
	{ util::OBJ_ROCKEX,   QColor(81, 53, 28) },		 //咖啡
	{ util::OBJ_BOUNDARY, QColor(112, 146, 190) },   //湛藍
	{ util::OBJ_WATER,    QColor(29, 73, 97) },		 //深湛藍
	{ util::OBJ_EMPTY,    QColor(0, 0, 1) },		 //黑
	{ util::OBJ_NPC,      QColor(198, 211, 255) },	 //淺紫
	{ util::OBJ_ITEM,     QColor(32, 255, 141) },	 //青綠
	{ util::OBJ_HUMAN,    QColor(255, 194, 194) },   //淺粉
	{ util::OBJ_PET,      QColor(149, 153, 124) },   //亞麻
	{ util::OBJ_GOLD,     QColor(247, 255, 0) },     //黃
	{ util::OBJ_GM,       QColor(212, 25, 25) },     //紅
};

//取靠近目標的最佳座標和方向
typedef struct qdistance_s
{
	int dir;
	qreal distance;//for euclidean distance
	QPoint p;
	QPointF pf;
}qdistance_t;

typedef struct qmappoint_s
{
	util::ObjectType type = util::OBJ_UNKNOWN;
	QPoint p = {};
} qmappoint_t;

//魔力包含3字節的'MAP'和 9個'\0' 石器沒有
typedef struct mapheader_s
{
	//char head[3] = { '\0', '\0', '\0' }; // +0
	//char padding[9] = {};                // +3
	DWORD width = 0UL;                     // +12
	DWORD height = 0UL;                    // +16
} mapheader_t;

typedef struct map_s
{
	int floor = 0;
	int width = 0;
	int height = 0;
	QString name = "";
	QVector<qmappoint_t> stair = {};
	QSet<QPoint> workable = {};

	util::SafeHash<QPoint, util::ObjectType> data;
}map_t;

inline uint qHash(const QPoint& key, uint seed) Q_DECL_NOTHROW
{
	const uint val = (key.x() * 10000) + key.y();
	return qHash<uint>(val, seed);
}

inline uint qHash(const map_t& key, uint seed) Q_DECL_NOTHROW
{
	const uint val = (key.width * 10000) + (key.height) + (key.floor * 1000) + (key.data.size());
	return qHash<uint>(val, seed);
}

class MapAnalyzer
{
public:
	MapAnalyzer();
	virtual ~MapAnalyzer() = default;
	bool __fastcall readFromBinary(int floor, const QString& name, bool enableDraw = false);
	bool __fastcall getMapDataByFloor(int floor, map_t* map);
	bool __fastcall calcNewRoute(const map_t& map, const QPoint& src, const QPoint& dst, QVector<QPoint>* path);
	void clear() { maps_.clear(); pixMap_.clear(); }
	void clear(int floor) { maps_.remove(floor); pixMap_.remove(floor); }
	bool __fastcall saveAsBinary(map_t map, const QString& fileName);
	QPixmap __fastcall getPixmapByIndex(int index) const { return pixMap_.value(index); }
	int __fastcall calcBestFollowPointByDstPoint(int floor, const QPoint& src, const QPoint& dst, QPoint* ret, bool enableExt, int npcdir);
	bool __fastcall isPassable(int floor, const QPoint& src, const QPoint& dst);

private:
	inline QString __fastcall getCurrentMapPath(int floor) const;

	inline void __fastcall setMapDataByFloor(int floor, const map_t& map);
	void __fastcall setPixmapByIndex(int index, const QPixmap& pix);

	bool __fastcall loadFromBinary(int floor, map_t* _map);

	util::ObjectType __fastcall getGroundType(const uint16_t data) const;
	util::ObjectType __fastcall getObjectType(const uint16_t data) const;

public:
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

private:
	QString directory = "";
	util::SafeHash<int, QPixmap> pixMap_;
	util::SafeHash<int, map_t> maps_;
	QMutex mutex_;

};

template <class Stream>
Stream& operator<<(Stream& out, MapAnalyzer::cimage const& img)
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
