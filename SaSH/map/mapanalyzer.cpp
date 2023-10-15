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
#include "mapanalyzer.h"
#include "astar.h"
#include <net/tcpserver.h>
#include "injector.h"

constexpr const char* kDefaultMapSuffix = ".dat";

util::SafeHash<long long, QPixmap> g_pixMap;
util::SafeHash<long long, map_t> g_maps;

//不可通行地面、物件數據 或 傳點|樓梯
#pragma region StaticTable
//上樓樓梯
QSet<unsigned short> UP = {

};

//下樓樓梯
QSet<unsigned short> DOWN = {

};

QSet<unsigned short> JUMP = {

};

//1 * 1 牆壁 
QSet<unsigned short> WALL = {

};

QSet<unsigned short> WATER = {

};

//不可通行的地型
QSet<unsigned short> GROUND = {

};

//所有類型的 1 * 1 障礙物件
QSet<unsigned short> ROCK = {

};

//可通行 地面 或 物件
QSet<unsigned short> ROAD = {

};

//2號物 或 空區
QSet<unsigned short> EMPTY = {
	2,
};

//超過 1 * 1 的大型障礙物件
static const QHash<long long, QSet<unsigned short>> ROCKEX = {
	{
		2 * 1000 + 2,
		QSet<unsigned short>{
			10007, 10014, 10048, 10049, 10050, 10119, 10141, 10220, 10560, 10561, 10563,
			10564, 10566, 10567, 10671, 10939, 10964, 10965, 10966, 10972, 10976, 10978,
			10979, 11136, 11137, 11138, 11144, 11148, 11150, 11151, 11208, 11488, 11489,
			11492, 11501, 11505, 11571, 11572, 11573, 11574, 11575, 11576, 11577, 11578,
			11579, 11580, 11581, 11582, 11583, 11584, 11585, 11586, 11587, 11588, 11589,
			11597, 11606, 11611, 11612, 11624, 11668, 11669, 11670, 11671, 11672, 11673,
			11674, 11675, 11676, 11677, 11678, 11679, 11680, 11681, 11682, 11683, 11684,
			11685, 11686, 11687, 11688, 11689, 11690, 11691, 11697, 11698, 11699, 11710,
			11722, 11786, 11787, 11788, 11789, 11790, 11791, 11792, 11793, 11794, 11795,
			11796, 11797, 11798, 11799, 11800, 11811, 11820, 11821, 11840, 11841, 11842,
			11843, 11844, 11845, 11846, 11847, 11848, 11849, 11870, 11871, 11872, 11873,
			11909, 12002, 12010, 12019, 12038, 12041, 12048, 12049, 12650, 12670, 12672,
			12673, 12674, 12675, 12688, 12689, 12690, 12691, 12692, 12693, 12817, 12838,
			12908, 13004, 13708, 13804, 15322, 15323, 15422, 15423, 15508, 15509, 15511,
			15524, 15525, 15526, 15527, 15540, 15541, 15542, 15543, 15556, 15557, 15558,
			15559, 15572, 15573, 15574, 15575, 15588, 15589, 15590, 15591, 15604, 15605,
			15606, 15607, 15620, 15621, 15622, 15623, 15723, 24726, 24727, 24729, 24730,
			24744, 24745, 24748, 24749, 24751, 24755, 24756,
		}
	},

	{
		3 * 1000 + 3,
		QSet<unsigned short>{
			10010, 10011, 10056, 10057, 10058, 10059, 10143, 10405, 10624, 10877, 10878,
			10879, 10880, 10881, 10882, 10883, 10884, 10885, 10886, 10887, 10888, 10889,
			10890, 10891, 10892, 10970, 10971, 10974, 10984, 10985, 10989, 11142, 11143,
			11146, 11400, 11401, 11402, 11403, 11404, 11405, 11406, 11407, 11408, 11409,
			11410, 11411, 11412, 11413, 11414, 11415, 11416, 11417, 11418, 11419, 11504,
			11514, 11516, 11604, 11620, 11633, 11707, 11718, 11729, 11851, 11852, 11853,
			12005, 12653, 12909, 12910, 12911, 12912, 13000, 13388, 13389, 13390, 13391,
			13392, 13393, 13394, 13395, 13396, 13397, 13398, 13399, 13400, 13401, 13402,
			13403, 13404, 13405, 13406, 13407, 13408, 13409, 13410, 13411, 13412, 13413,
			13414, 13415, 13416, 13417, 13418, 13419, 13420, 13421, 13422, 13423, 13424,
			13425, 13426, 13427, 13481, 13482, 13483, 13484, 13485, 13486, 13487, 13488,
			13489, 13490, 13491, 13492, 13493, 13494, 13495, 13496, 13497, 13498, 13499,
			13500, 13501, 13502, 13503, 13504, 13505, 13506, 13507, 13508, 13509, 13510,
			13511, 13512, 13513, 15000, 15002, 15003, 15004, 15005, 15006, 15008, 15009,
			15010, 15011, 15100, 15102, 15103, 15104, 15105, 15106, 15108, 15109, 15110,
			15111, 15150, 15152, 15153, 15154, 15155, 15156, 15158, 15159, 15160, 15161,
			15200, 15202, 15203, 15204, 15205, 15206, 15208, 15209, 15210, 15211, 15300,
			15302, 15303, 15304, 15305, 15306, 15308, 15309, 15310, 15311, 15424, 15425,
			15428, 15429, 15430, 15431, 15432, 15433, 15434, 15435, 15436, 15437, 15438,
			15439, 15440, 15441, 15442, 15443, 15448, 15449, 15450, 15451, 15452, 15453,
			15454, 15455, 15456, 15457, 15458, 15459, 15460, 15461, 15462, 15463, 15468,
			15469, 15470, 15471, 15472, 15473, 15474, 15475, 15476, 15477, 15478, 15479,
			15480, 15481, 15482, 15483, 15488, 15489, 15490, 15491, 15492, 15493, 15494,
			15495, 15496, 15497, 15498, 15499, 15500, 15501, 15502, 15503, 15680, 15681,
			15682, 15684, 24736, 24737, 24738, 24739, 24750, 24757, 24758,
		}
	},

	{
		4 * 1000 + 4,
		QSet<unsigned short>{
			10065, 10066, 10067, 10068, 10094, 10542, 10600, 10601, 10783, 10893, 10932,
			11205, 11908, 13711, 15688, 15690, 15691, 15700, 15701, 15703, 15704, 15705,
			15707, 15708, 15709, 15710, 15711, 15712, 15713, 15714, 15715, 15727, 24759,
			24762,
		}
	},

	{
		5 * 1000 + 5,
		QSet<unsigned short>{
			10000, 10001, 10018, 10034, 10035, 10036, 10037, 10038, 10144, 10146, 10151,
			10157, 10158, 10159, 10210, 10211, 10212, 10213, 10217, 10218, 10623, 10784,
			10846, 10895, 10924, 12900, 12901, 12902, 12903, 15320,
		}
	},

	{
		6 * 1000 + 6,
		QSet<unsigned short>{
			10008, 10009, 10043, 10045, 10100, 10101, 10102, 10103, 10104, 10105, 10106,
			10107, 10108, 10109, 10111, 10113, 10115, 10139, 10145, 10148, 10166, 10179,
			10228, 10300, 10502, 10503, 10504, 10505, 10511, 10521, 10541, 10766, 10769,
			10770, 10836, 10838, 10844, 10847,
		}
	},

	{
		7 * 1000 + 7,
		QSet<unsigned short>{
			10227, 10510, 10514, 10515, 10523, 10835, 10848,
		}
	},

	{
		8 * 1000 + 8,
		QSet<unsigned short>{
			10537,
		}
	},

	{
		9 * 1000 + 9,
		QSet<unsigned short>{
			10231, 10232,
		}
	},

	{
		10 * 1000 + 10,
		QSet<unsigned short>{
			10233, 10869, 15402, 15403,
		}
	},

	{
		13 * 1000 + 13,
		QSet<unsigned short>{
			10534,
		}
	},

	{
		15 * 1000 + 15,
		QSet<unsigned short>{
			10237,
		}
	},

	{
		16 * 1000 + 16,
		QSet<unsigned short>{
			10225,
		}
	},

	{
		1 * 1000 + 2,
		QSet<unsigned short>{
			10118, 10128, 10129, 10152, 10201, 10214, 10310, 10314, 10629, 10631, 10674,
			10678, 10808, 10809, 10820, 10821, 10930, 10936, 10937, 11497, 11498, 11552,
			11553, 11554, 11555, 11556, 11557, 11558, 11559, 11560, 11561, 11562, 11563,
			11564, 11565, 11566, 11567, 11568, 11569, 11570, 11605, 11607, 11654, 11655,
			11656, 11657, 11658, 11659, 11660, 11661, 11662, 11663, 11664, 11711, 11712,
			11767, 11768, 11769, 11770, 11771, 11772, 11773, 11774, 11775, 11776, 11777,
			11778, 11779, 11780, 11781, 11782, 11833, 11834, 11835, 11836, 11837, 11838,
			11864, 11865, 11866, 11867, 11868, 12000, 12003, 12004, 12006, 12015, 12016,
			12017, 12018, 12020, 12021, 12026, 12027, 12028, 12029, 12030, 12031, 12040,
			12042, 12043, 12044, 12050, 12051, 12052, 12053, 12054, 12055, 12056, 12064,
			12065, 12066, 12813, 12832, 12835, 12841, 12842, 12846, 14702, 14704, 14706,
			14707, 14709, 15016, 15116, 15166, 15216, 15316, 15484, 15485, 15486, 15487,
			15639,
		}
	},

	{
		1 * 1000 + 3,
		QSet<unsigned short>{
			10022, 10025, 10123, 10126, 10170, 10175, 10176, 10902, 11507, 11511, 11630,
			11721, 11728, 11818, 11819, 11839, 11869, 12400, 12849, 12850, 13713, 13716,
			14705, 14708, 15018, 15118, 15168, 15218, 15318, 15444, 15445, 15446, 15447,
			15464, 15465, 15466, 15467, 15404, 15405, 15406, 15407, 15725,
		}
	},

	{
		1 * 1000 + 4,
		QSet<unsigned short>{
			10901, 10927, 12411, 12412, 12419, 12420,
		}
	},

	{
		1 * 1000 + 5,
		QSet<unsigned short>{
			15726,
		}
	},

	{
		1 * 1000 + 8,
		QSet<unsigned short>{
			10198,
		}
	},

	{
		1 * 1000 + 11,
		QSet<unsigned short>{
			11814, 11817,
		}
	},

	{
		2 * 1000 + 1,
		QSet<unsigned short>{
			10017, 10092, 10093, 10121, 10185, 10192, 10628, 10630, 10633, 10675, 10813,
			10814, 10925, 10938, 10967, 10977, 11139, 11149, 11493, 11494, 11495, 11496,
			11506, 11567, 11608, 11609, 11613, 11622, 11665, 11666, 11667, 11700, 11715,
			11716, 11717, 11720, 11783, 11784, 11785, 12001, 12007, 12008, 12009, 12011,
			12012, 12013, 12014, 12022, 12022, 12032, 12033, 12034, 12035, 12036, 12037,
			12039, 12045, 12046, 12047, 12057, 12058, 12059, 12060, 12061, 12062, 12063,
			12067, 12068, 12069, 12671, 12814, 12839, 12843, 12844, 13707, 14703, 14710,
			14712, 14713, 14715, 15019, 15119, 15169, 15219, 15319, 15638, 24732, 24733,
		}
	},

	{
		2 * 1000 + 3,
		QSet<unsigned short>{
			10125, 10184, 10622, 10636, 10915, 11503, 11513, 11518, 11603, 11623, 11628,
			11632, 11708, 11726, 11730, 11850, 11874, 12603, 12605, 12608, 12610, 12611,
			12612, 12616, 12663, 12665, 12676, 12677, 12678, 12679, 12684, 12685, 12818,
			12819, 12820, 12821, 12828, 12830, 12831, 12840, 12917, 12918, 12919, 12920,
			13712, 14716, 14718, 14720, 15007, 15012, 15013, 15107, 15112, 15113, 15157,
			15162, 15163, 15207, 15212, 15213, 15307, 15312, 15313, 15426, 15696, 15698,
			24725, 24746, 24747,
		}
	},

	{
		2 * 1000 + 4,
		QSet<unsigned short>{
			10202, 10661, 10662, 10664, 10870, 11512, 11625, 11629, 11727, 11875, 12416,
			13802, 13803, 15716, 15718, 15722,
		}
	},

	{
		2 * 1000 + 5,
		QSet<unsigned short>{
			10041, 10607, 15732,
		}
	},

	{
		2 * 1000 + 8,
		QSet<unsigned short>{
			10992,
		}
	},

	{
		2 * 1000 + 9,
		QSet<unsigned short>{
			10986, 10990, 10993,
		}
	},

	{
		2 * 1000 + 10,
		QSet<unsigned short>{
			14410,
		}
	},

	{
		2 * 1000 + 15,
		QSet<unsigned short>{
			10991,
		}
	},

	{
		3 * 1000 + 1,
		QSet<unsigned short>{
			10023, 10024, 10127, 10177, 10188, 11509, 11631, 11724, 11812, 11813, 11816, 12401,
			12847, 12848, 13714, 13715, 14711, 14714, 15017, 15167, 15217, 15317, 15721,
			24728, 24735,
		}
	},

	{
		3 * 1000 + 2,
		QSet<unsigned short>{
			10116, 10117, 10142, 10187, 10196, 10203, 10562, 10565, 10621, 10635, 10787,
			10788, 10913, 10926, 10975, 11147, 11502, 11508, 11510, 11626, 11627, 11709,
			11723, 11725, 12404, 12601, 12602, 12604, 12606, 12607, 12609, 12613, 12614,
			12615, 12662, 12664, 12680, 12681, 12682, 12683, 12686, 12687, 12822, 12823,
			12824, 12825, 12826, 13706, 14717, 14719, 14721, 15001, 15014, 15015, 15101,
			15114, 15115, 15117, 15151, 15164, 15165, 15201, 15214, 15215, 15301, 15314,
			15315, 15427, 15697, 15699,
		}
	},

	{
	3 * 1000 + 4,
	QSet<unsigned short>{
			10013, 10040, 10122, 10182, 10605, 10637, 10669, 10775, 10780, 10781, 10782,
			10785, 10894, 10914, 10982, 10983, 10988, 12905, 12906, 12907, 12913, 12914,
			12915, 12916, 15683, 15687, 15692, 15693, 15702, 24753,
		}
	},

	{
	3 * 1000 + 5,
	QSet<unsigned short>{
			10608, 10609, 10620, 10917, 10919, 10941,
	}
	},

	{
		3 * 1000 + 6,
		QSet<unsigned short>{
			10603, 10604, 10931,
		}
	},

	{
		3 * 1000 + 7,
		QSet<unsigned short>{
			10183,
		}
	},

	{
		3 * 1000 + 8,
		QSet<unsigned short>{
			10304, 10305, 10308,
		}
	},

	{
		3 * 1000 + 9,
		QSet<unsigned short>{
			10180,
		}
	},

	{
		4 * 1000 + 1,
		QSet<unsigned short>{
			10012, 10097, 10171, 10199, 10306, 10900, 10928, 12405, 12406, 12417, 12418,
		}
	},

	{
		4 * 1000 + 2,
		QSet<unsigned short>{
			10016, 10186, 10660, 10663, 10665, 11854, 12415, 13800, 13801, 15717, 15719,
			15724,
		}
	},

	{
	4 * 1000 + 3,
	QSet<unsigned short>{
			10219, 10638, 10668, 10776, 10779, 10786, 10916, 11515, 11517, 11621, 11634,
			11719, 11731, 11855, 11856, 15420, 15421, 15685, 15694, 15706, 24754, 24760,
		}
	},

	{
	4 * 1000 + 5,
	QSet<unsigned short>{
			10865, 15686, 15689,
	}
	},

	{
		4 * 1000 + 6,
		QSet<unsigned short>{
			10181, 10933,
		}
	},

	{
		4 * 1000 + 7,
		QSet<unsigned short>{
			10771,
		}
	},

	{
		4 * 1000 + 8,
		QSet<unsigned short>{
			10768,
		}
	},

	{
		5 * 1000 + 1,
		QSet<unsigned short>{
			10124, 15720,
		}
	},

	{
		5 * 1000 + 2,
		QSet<unsigned short>{
			10165, 10197, 10606, 15731,
		}
	},

	{
		5 * 1000 + 3,
		QSet<unsigned short>{
			10167, 10602, 10918, 11857, 15695,
		}
	},

	{
		5 * 1000 + 4,
		QSet<unsigned short>{
			10673, 10791, 15729, 15730, 24761,
		}
	},

	{
		5 * 1000 + 6,
		QSet<unsigned short>{
			10032, 10039, 10051, 10512, 10520, 10765, 15728,
		}
	},

	{
		5 * 1000 + 7,
		QSet<unsigned short>{
			 10138, 10764, 10773,
		}
	},

	{
		5 * 1000 + 8,
		QSet<unsigned short>{
			10500, 10867,
		}
	},

	{
		5 * 1000 + 9,
		QSet<unsigned short>{
			10164,
		}
	},

	{
		6 * 1000 + 1,
		QSet<unsigned short>{
			10015,
		}
	},

	{
		6 * 1000 + 5,
		QSet<unsigned short>{
			10033, 10147, 10149, 10174, 10522, 10767, 10772, 10843, 10845,
		}
	},

	{
		6 * 1000 + 7,
		QSet<unsigned short>{
			10042, 10046, 10110, 10112, 10114, 10150, 10216, 10229, 10513, 10524, 10839,
			10841,
		}
	},

	{
		6 * 1000 + 9,
		QSet<unsigned short>{
			10160, 10163, 15400,
		}
	},

	{
		6 * 1000 + 16,
		QSet<unsigned short>{
			10234,
		}
	},

	{
		7 * 1000 + 4,
		QSet<unsigned short>{
			10777,
		}
	},

	{
		7 * 1000 + 5,
		QSet<unsigned short>{
			10763, 10833, 10868, 10929,
		}
	},

	{
		7 * 1000 + 6,
		QSet<unsigned short>{
			10226, 10525, 10842, 10861,
		}
	},

	{
		7 * 1000 + 8,
		QSet<unsigned short>{
			10239, 10536, 10539, 10834, 10837, 10840,
		}
	},

	{
		7 * 1000 + 10,
		QSet<unsigned short>{
			10230,
		}
	},

	{
		7 * 1000 + 17,
		QSet<unsigned short>{
			10236,
		}
	},

	{
		8 * 1000 + 2,
		QSet<unsigned short>{
			10862,
		}
	},

	{
		8 * 1000 + 3,
		QSet<unsigned short>{
			10064, 10307,
		}
	},

	{
		8 * 1000 + 4,
		QSet<unsigned short>{
			10863,
		}
	},

	{
		8 * 1000 + 5,
		QSet<unsigned short>{
			10140, 10501, 10871, 10876,
		}
	},

	{
		8 * 1000 + 6,
		QSet<unsigned short>{
			10866, 10872, 10873,
		}
	},

	{
		8 * 1000 + 7,
		QSet<unsigned short>{
			10044, 10532, 10533, 10538, 10874,
		}
	},

	{
		9 * 1000 + 3,
		QSet<unsigned short>{
			10301, 10302, 10303, 10987, 10994, 10995,
		}
	},

	{
		9 * 1000 + 5,
		QSet<unsigned short>{
			10864,
		}
	},

	{
		9 * 1000 + 6,
		QSet<unsigned short>{
			15401,
		}
	},

	{
		9 * 1000 + 7,
		QSet<unsigned short>{
			10762,
		}
	},

	{
		9 * 1000 + 8,
		QSet<unsigned short>{
			10235,
		}
	},

	{
		9 * 1000 + 11,
		QSet<unsigned short>{
			10238,
		}
	},

	{
		10 * 1000 + 8,
		QSet<unsigned short>{
			10530,
		}
	},

	{
		10 * 1000 + 9,
		QSet<unsigned short>{
			10778, 10875,
		}
	},

	{
		10 * 1000 + 11,
		QSet<unsigned short>{
			10531, 10535,
		}
	},

	{
		10 * 1000 + 13,
		QSet<unsigned short>{
			15404,
		}
	},

	{
		13 * 1000 + 6,
		QSet<unsigned short>{
			11815,
		}
	},

	{
		14 * 1000 + 10,
		QSet<unsigned short>{
			10528,
		}
	},

	{
		14 * 1000 + 11,
		QSet<unsigned short>{
			10529,
		}
	},
};

// 大石頭或大型障礙物編號
static const QSet<QPair<QPair<long long, long long >, QSet<unsigned short>>> ROCKEX_SET = {
	{ QPair<long long, long long>{ 2, 2 },   ROCKEX.value(2 * 1000 + 2) }, // 2 * 2
	{ QPair<long long, long long>{ 3, 3 },   ROCKEX.value(3 * 1000 + 3) }, // 3 * 3
	{ QPair<long long, long long>{ 4, 4 },   ROCKEX.value(4 * 1000 + 4) }, // 3 * 2
	{ QPair<long long, long long>{ 5, 5 },   ROCKEX.value(5 * 1000 + 5) }, // 3 * 3
	{ QPair<long long, long long>{ 6, 6 },   ROCKEX.value(6 * 1000 + 6) }, // 2 * 2
	{ QPair<long long, long long>{ 7, 7 },   ROCKEX.value(7 * 1000 + 7) }, // 3 * 2
	{ QPair<long long, long long>{ 8, 8 },   ROCKEX.value(8 * 1000 + 8) }, // 3 * 3
	{ QPair<long long, long long>{ 9, 9 },   ROCKEX.value(9 * 1000 + 9) }, // 2 * 2
	{ QPair<long long, long long>{ 10, 10 }, ROCKEX.value(10 * 1000 + 10) }, // 3 * 2
	{ QPair<long long, long long>{ 13, 13 }, ROCKEX.value(13 * 1000 + 13) }, // 3 * 3
	{ QPair<long long, long long>{ 15, 15 }, ROCKEX.value(15 * 1000 + 15) }, // 2 * 2
	{ QPair<long long, long long>{ 16, 16 }, ROCKEX.value(16 * 1000 + 16) }, // 3 * 2

	{ QPair<long long, long long>{ 1, 2 },   ROCKEX.value(1 * 1000 + 2) }, // 3 * 3
	{ QPair<long long, long long>{ 1, 3 },   ROCKEX.value(1 * 1000 + 3) }, // 2 * 2
	{ QPair<long long, long long>{ 1, 4 },   ROCKEX.value(1 * 1000 + 4) }, // 3 * 2
	{ QPair<long long, long long>{ 1, 5 },   ROCKEX.value(1 * 1000 + 5) }, // 3 * 3
	{ QPair<long long, long long>{ 1, 8 },   ROCKEX.value(1 * 1000 + 8) }, // 2 * 2
	{ QPair<long long, long long>{ 1, 11 },  ROCKEX.value(1 * 1000 + 11) }, // 3 * 2

	{ QPair<long long, long long>{ 2, 1 },   ROCKEX.value(2 * 1000 + 1) }, // 3 * 3
	{ QPair<long long, long long>{ 2, 3 },   ROCKEX.value(2 * 1000 + 3) }, // 2 * 2
	{ QPair<long long, long long>{ 2, 4 },   ROCKEX.value(2 * 1000 + 4) }, // 3 * 2
	{ QPair<long long, long long>{ 2, 5 },   ROCKEX.value(2 * 1000 + 5) }, // 3 * 3
	{ QPair<long long, long long>{ 2, 8 },   ROCKEX.value(2 * 1000 + 8) }, // 2 * 2
	{ QPair<long long, long long>{ 2, 9 },   ROCKEX.value(2 * 1000 + 9) }, // 3 * 2
	{ QPair<long long, long long>{ 2, 10 },  ROCKEX.value(2 * 1000 + 10) }, // 3 * 3
	{ QPair<long long, long long>{ 2, 15 },  ROCKEX.value(2 * 1000 + 15) }, // 2 * 2

	{ QPair<long long, long long>{ 3, 1 },  ROCKEX.value(3 * 1000 + 1) }, // 3 * 2
	{ QPair<long long, long long>{ 3, 2 },  ROCKEX.value(3 * 1000 + 2) }, // 3 * 3
	{ QPair<long long, long long>{ 3, 4 },  ROCKEX.value(3 * 1000 + 4) }, // 2 * 2
	{ QPair<long long, long long>{ 3, 5 },  ROCKEX.value(3 * 1000 + 5) }, // 3 * 2
	{ QPair<long long, long long>{ 3, 6 },  ROCKEX.value(3 * 1000 + 6) }, // 3 * 3
	{ QPair<long long, long long>{ 3, 7 },  ROCKEX.value(3 * 1000 + 7) }, // 3 * 3
	{ QPair<long long, long long>{ 3, 8 },  ROCKEX.value(3 * 1000 + 8) }, // 2 * 2
	{ QPair<long long, long long>{ 3, 9 },  ROCKEX.value(3 * 1000 + 9) }, // 3 * 2

	{ QPair<long long, long long>{ 4, 1 },  ROCKEX.value(4 * 1000 + 1) }, // 3 * 3
	{ QPair<long long, long long>{ 4, 2 },  ROCKEX.value(4 * 1000 + 2) }, // 2 * 2
	{ QPair<long long, long long>{ 4, 3 },  ROCKEX.value(4 * 1000 + 3) }, // 3 * 2
	{ QPair<long long, long long>{ 4, 5 },  ROCKEX.value(4 * 1000 + 5) }, // 3 * 3
	{ QPair<long long, long long>{ 4, 6 },  ROCKEX.value(4 * 1000 + 6) }, // 2 * 2
	{ QPair<long long, long long>{ 4, 7 },  ROCKEX.value(4 * 1000 + 7) }, // 3 * 2
	{ QPair<long long, long long>{ 4, 8 },  ROCKEX.value(4 * 1000 + 8) }, // 3 * 3

	{ QPair<long long, long long>{ 5, 1 },  ROCKEX.value(5 * 1000 + 1) }, // 2 * 2
	{ QPair<long long, long long>{ 5, 2 },  ROCKEX.value(5 * 1000 + 2) }, // 3 * 2
	{ QPair<long long, long long>{ 5, 3 },  ROCKEX.value(5 * 1000 + 3) }, // 3 * 3
	{ QPair<long long, long long>{ 5, 4 },  ROCKEX.value(5 * 1000 + 4) }, // 3 * 3
	{ QPair<long long, long long>{ 5, 6 },  ROCKEX.value(5 * 1000 + 6) }, // 2 * 2
	{ QPair<long long, long long>{ 5, 7 },  ROCKEX.value(5 * 1000 + 7) }, // 3 * 2
	{ QPair<long long, long long>{ 5, 8 },  ROCKEX.value(5 * 1000 + 8) }, // 3 * 3
	{ QPair<long long, long long>{ 5, 9 },  ROCKEX.value(5 * 1000 + 9) }, // 2 * 2

	{ QPair<long long, long long>{ 6, 1 },  ROCKEX.value(6 * 1000 + 1) }, // 3 * 3
	{ QPair<long long, long long>{ 6, 5 },  ROCKEX.value(6 * 1000 + 5) }, // 2 * 2
	{ QPair<long long, long long>{ 6, 7 },  ROCKEX.value(6 * 1000 + 7) }, // 3 * 2
	{ QPair<long long, long long>{ 6, 9 },  ROCKEX.value(6 * 1000 + 9) }, // 3 * 3
	{ QPair<long long, long long>{ 6, 16 }, ROCKEX.value(6 * 1000 + 16) }, // 3 * 3

	{ QPair<long long, long long>{ 7, 4 },  ROCKEX.value(7 * 1000 + 4) }, // 2 * 2
	{ QPair<long long, long long>{ 7, 5 },  ROCKEX.value(7 * 1000 + 5) }, // 3 * 2
	{ QPair<long long, long long>{ 7, 6 },  ROCKEX.value(7 * 1000 + 6) }, // 3 * 3
	{ QPair<long long, long long>{ 7, 8 },  ROCKEX.value(7 * 1000 + 8) }, // 2 * 2
	{ QPair<long long, long long>{ 7, 10 }, ROCKEX.value(7 * 1000 + 10) }, // 3 * 2
	{ QPair<long long, long long>{ 7, 17 }, ROCKEX.value(7 * 1000 + 17) }, // 3 * 3

	{ QPair<long long, long long>{ 8, 2 },  ROCKEX.value(8 * 1000 + 2) }, // 3 * 3
	{ QPair<long long, long long>{ 8, 3 },  ROCKEX.value(8 * 1000 + 3) }, // 2 * 2
	{ QPair<long long, long long>{ 8, 4 },  ROCKEX.value(8 * 1000 + 4) }, // 3 * 2
	{ QPair<long long, long long>{ 8, 5 },  ROCKEX.value(8 * 1000 + 5) }, // 3 * 3
	{ QPair<long long, long long>{ 8, 6 },  ROCKEX.value(8 * 1000 + 6) }, // 2 * 2
	{ QPair<long long, long long>{ 8, 7 },  ROCKEX.value(8 * 1000 + 7) }, // 3 * 2

	{ QPair<long long, long long>{ 9, 3 },  ROCKEX.value(9 * 1000 + 3) }, // 3 * 3
	{ QPair<long long, long long>{ 9, 5 },  ROCKEX.value(9 * 1000 + 5) }, // 2 * 2
	{ QPair<long long, long long>{ 9, 6 },  ROCKEX.value(9 * 1000 + 6) }, // 3 * 2
	{ QPair<long long, long long>{ 9, 7 },  ROCKEX.value(9 * 1000 + 7) }, // 3 * 3
	{ QPair<long long, long long>{ 9, 8 },  ROCKEX.value(9 * 1000 + 8) }, // 3 * 3
	{ QPair<long long, long long>{ 9, 11 }, ROCKEX.value(9 * 1000 + 11) }, // 2 * 2

	{ QPair<long long, long long>{ 10, 8 },  ROCKEX.value(10 * 1000 + 8) }, // 3 * 2
	{ QPair<long long, long long>{ 10, 9 },  ROCKEX.value(10 * 1000 + 9) }, // 3 * 3
	{ QPair<long long, long long>{ 10, 11 }, ROCKEX.value(10 * 1000 + 11) }, // 3 * 2
	{ QPair<long long, long long>{ 10, 13 }, ROCKEX.value(10 * 1000 + 13) }, // 3 * 3

	{ QPair<long long, long long>{ 13, 6 },  ROCKEX.value(13 * 1000 + 6) }, // 3 * 2

	{ QPair<long long, long long>{ 14, 10 }, ROCKEX.value(14 * 1000 + 10) }, // 3 * 3
	{ QPair<long long, long long>{ 14, 11 }, ROCKEX.value(14 * 1000 + 11) }, // 3 * 2
};

#pragma endregion

//檢查地圖大小是否合法
inline constexpr bool CHECKSIZE(long long w, long long h)
{
	if (w < 0 || h < 0 || w > 1500 || h > 1500)
		return false;
	else
		return true;
}

//找大石頭(占用坐標超過1格)並設置標記
void checkAndSetRockEx(map_t& map, const QPoint& p, unsigned short sObject)
{
	//    X = 12220 || 12222 為起點往右上畫6格長方形
	// 
	//     * *       x,y-2  x+1,y-2
	//     * *       x,y-1  x+1,y-1
	//     X *       x,y    x+1,y
	//
	long long x = 0, y = 0;
	for (const QPair<QPair<long long, long long>, QSet<unsigned short>>& it : ROCKEX_SET)
	{
		const QSet<unsigned short> set = it.second;
		if (!set.size() || !set.contains(sObject))
			continue;
		long long max_y = it.first.second;
		long long max_x = it.first.first;
		for (y = 0; y < max_y; ++y)
		{
			for (x = 0; x < max_x; ++x)
			{
				const QPoint point(p.x() + x, p.y() - y);
				map.data.insert(point, util::OBJ_ROCKEX);
			}
		}
	}
};

//重複檢查大石頭
void reCheckAndRockEx(map_t& map, const QPoint& point, unsigned short sObject)
{
	for (const QPair<QPair<long long, long long>, QSet<unsigned short>>& it : ROCKEX_SET)
	{
		const QSet<unsigned short> set = it.second;
		if (!set.size() || !set.contains(sObject))
			continue;
		map.data.insert(point, util::OBJ_ROCKEX);
		return;
	}
};

//用於從映射到內存的數據中取出特定的數據塊
std::vector<unsigned short> load(const UCHAR* pFileMap, long long sectionOffset, long long offest)
{
	std::vector<unsigned short> v = {};
	do
	{
		if (!pFileMap)
		{
			break;
		}

		long long size = sizeof(mapheader_t) + (sectionOffset * offest);
		const UCHAR* p = pFileMap + size;
		for (size_t i = 0u; i < static_cast<size_t>(sectionOffset); i += 2u)
		{
			v.push_back(*reinterpret_cast<const unsigned short*>(p + i));
		}
	} while (false);
	return v;
}

//用於距離排序找到最近的坐標
bool compareDistance(qdistance_t& a, qdistance_t& b)
{
	return (a.distance < b.distance);
}

MapAnalyzer::MapAnalyzer(long long index)
	: Indexer(index)
	, directory(util::toQString(qgetenv("GAME_DIR_PATH")))
{

}

MapAnalyzer::~MapAnalyzer()
{
	qDebug() << "MapAnalyzer distory!!";
}

void MapAnalyzer::loadHotData(Downloader& downloader)
{
	QSet<quint16> d = {};
	QString strdata;
	QVariant vdata;
	if (!downloader.start(Downloader::GiteeMapData, &vdata) && vdata.isValid())
	{
		QString msg = QObject::tr("Map data download failed, please check your network connection!");
		std::wstring wmsg = msg.toStdWString();
		MessageBoxW(nullptr, wmsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
		MINT::NtTerminateProcess(GetCurrentProcess(), 0);
		return;
	}

	strdata = vdata.toString();

	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::os);

	const std::string exprStrUTF8 = strdata.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
	lua.collect_garbage();

	std::vector<std::string> keys = { "UP", "DOWN", "JUMP", "WALL", "WATER", "GROUND", "ROCK", "ROAD", "EMPTY" };

	for (const std::string& key : keys)
	{
		d.clear();

		sol::object retTable = lua[key];
		if (!retTable.valid())
			continue;

		if (!retTable.is<sol::table>())
			continue;

		sol::table table = retTable.as<sol::table>();

		for (const auto& pair : table)
		{
			if (pair.second.is<long long>())
				d.insert(static_cast<quint16>(pair.second.as<long long>()));
		}

		if (key == "UP")
			UP = d;
		else if (key == "DOWN")
			DOWN = d;
		else if (key == "JUMP")
			JUMP = d;
		else if (key == "WALL")
			WALL = d;
		else if (key == "WATER")
			WATER = d;
		else if (key == "ROAD")
			ROAD = d;
		else if (key == "GROUND")
			GROUND = d;
		else if (key == "ROCK")
			ROCK = d;
		else if (key == "EMPTY")
			EMPTY = d;
	}
}

void MapAnalyzer::clear() const { g_maps.clear(); g_pixMap.clear(); }

void MapAnalyzer::clear(long long floor) const { g_maps.remove(floor); g_pixMap.remove(floor); }

QPixmap MapAnalyzer::getPixmapByIndex(long long index) const { return g_pixMap.value(index); }

//查找地形
util::ObjectType MapAnalyzer::getGroundType(const unsigned short data) const
{
	if (UP.contains(data))
		return util::OBJ_UP;

	if (DOWN.contains(data))
		return util::OBJ_DOWN;

	if (JUMP.contains(data))
		return util::OBJ_JUMP;

	if (ROAD.contains(data))
		return util::OBJ_ROAD;

	if (WATER.contains(data))
		return util::OBJ_WATER;

	if (GROUND.contains(data))
		return util::OBJ_WALL;

	if (EMPTY.contains(data))
		return util::OBJ_EMPTY;

	return util::OBJ_UNKNOWN;
}

//查找物件
util::ObjectType MapAnalyzer::getObjectType(const unsigned short data) const
{
	if (UP.contains(data))
		return util::OBJ_UP;

	if (DOWN.contains(data))
		return util::OBJ_DOWN;

	if (JUMP.contains(data))
		return util::OBJ_JUMP;

	if (ROAD.contains(data))
		return util::OBJ_ROAD;

	if (WATER.contains(data))
		return util::OBJ_WATER;

	if ((WALL.contains(data)))
		return util::OBJ_WALL;

	if (ROCK.contains(data))
		return util::OBJ_ROCK;

	if (EMPTY.contains(data))
		return util::OBJ_EMPTY;

	return util::OBJ_UNKNOWN;
}

bool MapAnalyzer::getMapDataByFloor(long long floor, map_t* map)
{
	if (g_maps.contains(floor))
	{
		map_t m = g_maps.value(floor);
		if (m.data.isEmpty())
			return false;

		if (m.refCount == 0)
			m.timer.start();

		++m.refCount;
		*map = m;
		g_maps.insert(floor, m);
		return true;
	}

	return false;
}

void MapAnalyzer::setMapDataByFloor(long long floor, const map_t& map)
{
	g_maps.insert(floor, map);
}

void MapAnalyzer::setPixmapByIndex(long long index, const QPixmap& pix)
{
	if (g_pixMap.contains(index))
	{
		g_pixMap.insert(index, pix);
	}
	else
	{
		g_pixMap.insert(index, pix);
	}
}

//取遊戲原始二進制地圖路徑
QString MapAnalyzer::getCurrentMapPath(long long floor) const
{
	const QString path = directory + "/map/" + util::toQString(floor) + kDefaultMapSuffix;
	return path;
}

QString MapAnalyzer::getCurrentPreHandleMapPath(long long currentIndex, long long floor) const
{
	Injector& injector = Injector::getInstance(currentIndex);
	const QString dirPath(QString("%1/lib/map/%2").arg(util::applicationDirPath()).arg(injector.currentServerListIndex));
	QDir dir(dirPath);
	if (!dir.exists())
		dir.mkdir(dirPath);

	const QString newFileName = QString("%1/%2%3").arg(dirPath).arg(floor).arg(kDefaultMapSuffix);
	return newFileName;
}

bool MapAnalyzer::readFromBinary(long long currentIndex, long long floor, const QString& name, bool enableDraw, bool enableRewrite)
{
	if (floor < 0)
		return false;

	//%(GAME_DIRECTORY)/map/{floor}.dat
	const QString path(getCurrentMapPath(floor));

	map_t map;
	long long width = 0;
	long long height = 0;
	uchar* pFileMap = nullptr;

	{
		//check file exist
		util::ScopedFile file(path);
		if (!file.openRead())
		{
			qDebug() << __FUNCTION__ << " @" << __LINE__ << " Failed to open file.";
			return false;
		}

		if (!file.mmap(pFileMap, 0, sizeof(mapheader_t)))
		{
			qDebug() << __FUNCTION__ << " @" << __LINE__ << " Failed to map file.";
			return false;
		}

		//2個DWORD(4字節)的數據，第1個表示地圖長度 - 東(W)，第2個表示地圖長度 - 南(H)。
		mapheader_t* _header = reinterpret_cast<mapheader_t*>(pFileMap);
		if (_header == nullptr)
		{
			qDebug() << __FUNCTION__ << " Failed to map file.";
			return false;
		}

		//獲取地圖寬高
		width = _header->width;
		height = _header->height;
	}

	if (!CHECKSIZE(width, height))
	{
		qDebug() << __FUNCTION__ << " Invalid map size.";
		return false;
	}

	getMapDataByFloor(floor, &map);
	map.floor = floor;
	map.width = width;
	map.height = height;
	map.name = name;

	if (map.data.size() > 0
		&& map.data.size() == (height * width)
		&& !g_pixMap.value(floor).isNull()
		&& height == map.height
		&& width == map.width && !enableRewrite)
	{
		return true;
	}

	auto draw = [this, &map, &enableDraw, floor]()->void
		{
			if (enableDraw && g_pixMap.value(floor).isNull())
			{
				//QT列表容器<點> 列表 = 地圖.數據.鍵(點) //取指定地圖數據
				const QList<QPoint> list = map.data.keys();//取指定地圖數據

				//QT圖像類 QImage 圖像(QSize(地圖.寬, 地圖.高), 格式32色帶透明)
				QImage img(QSize(map.width, map.height), QImage::Format_ARGB32);//生成圖像
				img.fill(MAP_COLOR_HASH.value(util::OBJ_EMPTY));//填充背景色

				QPainter painter(&img);//實例繪製引擎
				for (const QPoint& it : list) //遍歷地圖數據
				{
					util::ObjectType typeOriginal = map.data.value(it);
					const QBrush brush(MAP_COLOR_HASH.value(typeOriginal), Qt::SolidPattern); //獲取並設置顏色
					const QPen pen(brush, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);  //實例畫筆

					painter.setPen(pen); //設置畫筆
					painter.drawPoint(it); //繪製點
				}
				painter.end(); //結束繪製
				setPixmapByIndex(map.floor, QPixmap::fromImage(img));
			}
		};

	if (!enableRewrite && loadFromBinary(currentIndex, floor, &map))
	{
		draw();
		return true;
	}

	map.data.clear();
	map.stair.clear();
	map.workable.clear();

	long long x = 0, y = 0;
	long long sectionOffset = width * height * 2;

	//隨後W* H * 2字節為地面數據，每2字節為1數據塊，表示地面的地圖編號，以製成基本地形。
	//再隨後W * H * 2字節為地上物件 / 建築物數據，每2字節為1數據塊，表示該點上的物件 / 建築物地圖編號。
	//再隨後 W * H * 2 字節為地圖標誌，每 2 字節為 1 數據塊，

	QList<QFuture<std::vector<unsigned short>>> futures;
	{
		util::ScopedFile file(path);
		if (file.openRead() && file.mmap(pFileMap, 0, file.size()))
		{
			QFutureSynchronizer<std::vector<unsigned short>> sync;
			sync.addFuture(QtConcurrent::run(load, pFileMap, sectionOffset, 0));
			sync.addFuture(QtConcurrent::run(load, pFileMap, sectionOffset, 1));
			sync.addFuture(QtConcurrent::run(load, pFileMap, sectionOffset, 2));
			sync.waitForFinished();
			futures = sync.futures();
		}
	}

	if (futures.size() != 3)
	{
		qDebug() << "map data error" << futures.size();
		return false;
	}

	const std::vector<unsigned short> bGround = futures[0].result();
	const std::vector<unsigned short> bObject = futures[1].result();
	const std::vector<unsigned short> bLabel = futures[2].result();

	if (bGround.size() != bObject.size() || bGround.size() != bLabel.size())
	{
		qDebug() << "map data error" << bGround.size() << bObject.size() << bLabel.size();
		return false;
	}

	auto read = [width](const std::vector<unsigned short>& ba, const QPoint& p)->unsigned short
		{
			size_t offest = static_cast<size_t>(p.x() + (p.y() * width));
			if (offest < ba.size())
				return ba.at(offest);
			else
				return 0;
		};

	bool bret = false;
	unsigned short sGround = 0, sObject = 0, sLabel = 0;
	util::ObjectType typeGround = util::OBJ_UNKNOWN;
	util::ObjectType typeObject = util::OBJ_UNKNOWN;
	util::ObjectType typeOriginal = util::OBJ_UNKNOWN;
	QPoint point(0, 0);

	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			point.setX(x); point.setY(y);

			sGround = read(bGround, point);
			sObject = read(bObject, point);
			sLabel = read(bLabel, point);

			map.ground.insert(point, sGround);
			map.object.insert(point, sObject);
			map.flag.insert(point, sLabel);

			typeGround = getGroundType(sGround);
			typeObject = getObjectType(sObject);

			typeOriginal = map.data.value(point);

			checkAndSetRockEx(map, point, sObject);

			//調試專用
//#ifdef _DEBUG
//			if (point == injector.worker->getPoint())
//			{
//				qDebug() << "ground:" << sGround << "object:" << sObject << "flag: HI[" << HIBYTE(sLabel) << "] LO[" << LOBYTE(sLabel) << "]";
//			}
//#endif

			//排除樓梯或水晶
			if ((util::OBJ_UP == typeOriginal) || (util::OBJ_DOWN == typeOriginal) || (util::OBJ_JUMP == typeOriginal) || (util::OBJ_WARP == typeOriginal) || (util::OBJ_ROCKEX == typeOriginal))
			{
				continue;
			}
			else
				map.data.insert(point, util::OBJ_UNKNOWN);

			if (util::OBJ_ROAD == typeObject || util::OBJ_ROAD == typeGround)
			{
				map.data.insert(point, util::OBJ_ROAD);
				continue;
			}

			//排除水
			if ((util::OBJ_WATER == typeGround))
			{
				map.data.insert(point, util::OBJ_WATER);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}
			//排除牆壁
			else if ((util::OBJ_WALL == typeGround))
			{
				if (typeObject != util::OBJ_ROCK)
					map.data.insert(point, util::OBJ_WALL);
				else
					map.data.insert(point, util::OBJ_ROCK);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}
			//排除石頭
			else if ((util::OBJ_ROCK == typeGround))
			{
				map.data.insert(point, util::OBJ_ROCK);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}
			//排除牆壁
			else if (((6693 == sGround) && (17534 == sObject) && (0xC000 == sLabel)) || (sGround == 0x64))
			{
				if (typeObject != util::OBJ_ROCK)
					map.data.insert(point, util::OBJ_WALL);
				else
					map.data.insert(point, util::OBJ_ROCK);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}
			//排除空白區
			else if (((sGround < 0x64) || (util::OBJ_EMPTY == typeGround)))
			{
				map.data.insert(point, util::OBJ_EMPTY);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}

			//數據塊第 1 字節為 0 或 10，10 表示該坐標能引發場景轉換，否則為 0
			//數據塊第 2 字節為 0、192 或 193，193 表示不能穿越該坐標，反之為 192，0 表示沒地圖。
			if ((0xC003 == sLabel))
			{
				//如果是傳點，但沒有標明是上樓/下樓或水晶，則默認為水晶
				if (((LOBYTE(sLabel) == 3) && (HIBYTE(sLabel) == 192)) && ((typeObject != util::OBJ_JUMP) && (typeObject != util::OBJ_UP) && (typeObject != util::OBJ_DOWN)))
				{
					typeObject = util::OBJ_JUMP;
				}

				if (util::OBJ_ROAD == typeObject)
					map.workable.insert(point);
				else
					map.stair.append(qmappoint_t{ typeObject , point });

				map.data.insert(point, typeObject);//傳點
				continue;
			}
			//找傳點
			else if ((0xC00A == sLabel) || ((LOBYTE(sLabel) == 10) && (HIBYTE(sLabel) == 192)))
			{
				if ((util::OBJ_UP != typeObject) && (util::OBJ_DOWN != typeObject) && (util::OBJ_JUMP != typeObject))
					typeObject = util::OBJ_WARP;
				map.stair.append(qmappoint_t{ typeObject, point });
				map.data.insert(point, typeObject);//可通行
				continue;
			}

			//排除牆壁,障礙
			if ((sObject != 0) && (typeObject != util::OBJ_ROAD))
			{
				if (typeObject != util::OBJ_ROCK)
					map.data.insert(point, util::OBJ_WALL);
				else
					map.data.insert(point, util::OBJ_ROCK);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}

			//排除非通行區塊 193 表示不能穿越該坐標，反之為 192
			if (HIBYTE(sLabel) == 193)
			{
				map.data.insert(point, util::OBJ_EMPTY);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}

			if (typeObject == util::OBJ_ROCK)
			{
				map.data.insert(point, util::OBJ_ROCK);
				reCheckAndRockEx(map, point, sObject);
				continue;
			}

			//不是傳點則強制換成路
			if (((typeObject != util::OBJ_UP) && (typeObject != util::OBJ_DOWN) && (typeObject != util::OBJ_JUMP)) || (util::OBJ_ROAD == typeGround))
				typeObject = util::OBJ_ROAD;

			//如果是路，則加入可通行列表
			if ((util::OBJ_ROAD == typeObject) || (util::OBJ_BOUNDARY == typeObject))
				map.workable.insert(point);

			map.data.insert(point, typeObject);//可通行

			checkAndSetRockEx(map, point, sObject);

			if (!bret)
				bret = true;
		}
	}

	//繪製地圖圖像(只能在PaintEvent中繪製)
	draw();
	saveAsBinary(currentIndex, map, "");
	setMapDataByFloor(floor, map);
	return bret;
}

bool MapAnalyzer::loadFromBinary(long long currentIndex, long long floor, map_t* _map)
{
	QMutexLocker locker(&mutex_);
	if (!floor)
		return false;

	const QString fileName(getCurrentPreHandleMapPath(currentIndex, floor));

	std::string f = fileName.toUtf8().constData();
	std::ifstream ifs(f, std::ios::binary | std::ios::in);
	if (!ifs.is_open())
	{
		return false;
	}

	util::ScopedFileLocker fileLock(fileName);

	map_t map = {};


	ifs.read(reinterpret_cast<char*>(&map.floor), sizeof(short));
	ifs.read(reinterpret_cast<char*>(&map.width), sizeof(short));
	ifs.read(reinterpret_cast<char*>(&map.height), sizeof(short));
	char name[24] = {};
	ifs.read(name, 24);
	map.name = QString(name);
	BYTE type = 0ui8;
	long long y = 0;
	for (long long x = 0; x < map.width; ++x)
	{
		for (y = 0; y < map.height; ++y)
		{
			ifs.read(reinterpret_cast<char*>(&type), sizeof(BYTE));
			if (util::OBJ_MAX >= 0 && type < util::OBJ_MAX)
			{
				map.data.insert(QPoint(x, y), static_cast<util::ObjectType>(type));
			}
		}
	}

	//讀取
	long long stairSize = 0;
	long long i = 0;
	ifs.read(reinterpret_cast<char*>(&stairSize), sizeof(int));
	qmappoint_t qmappoint = {};
	for (i = 0; i < stairSize; ++i)
	{
		qmappoint = {};
		ifs.read(reinterpret_cast<char*>(&qmappoint.type), sizeof(BYTE));
		ifs.read(reinterpret_cast<char*>(&qmappoint.p.rx()), sizeof(short));
		ifs.read(reinterpret_cast<char*>(&qmappoint.p.ry()), sizeof(short));
		map.stair.append(qmappoint);
	}

	// QSet<QPoint> workable = {};
	long long workableSize = 0;
	ifs.read(reinterpret_cast<char*>(&workableSize), sizeof(int));
	QPoint point = {};
	for (i = 0; i < workableSize; ++i)
	{
		point = {};
		ifs.read(reinterpret_cast<char*>(&point.rx()), sizeof(short));
		ifs.read(reinterpret_cast<char*>(&point.ry()), sizeof(short));
		map.workable.insert(point);
	}

	ifs.close();
	setMapDataByFloor(floor, map);
	if (_map)
	{
		*_map = map;
		g_pixMap.remove(floor);
	}
	return true;
}

bool MapAnalyzer::saveAsBinary(long long currentIndex, map_t map, const QString& fileName)
{
	QMutexLocker locker(&mutex_);
	if (!map.floor)
		return false;

	QString newFileName(fileName);
	if (fileName.isEmpty())
	{
		newFileName = getCurrentPreHandleMapPath(currentIndex, map.floor);
	}

	//write to binary file
	std::string f = newFileName.toUtf8().constData();
	std::ofstream ofs(f, std::ios::binary | std::ios::out | std::ios::trunc);
	if (!ofs.is_open())
	{
		return false;
	}

	util::ScopedFileLocker fileLock(newFileName);

	ofs.write(reinterpret_cast<const char*>(&map.floor), sizeof(short));
	ofs.write(reinterpret_cast<const char*>(&map.width), sizeof(short));
	ofs.write(reinterpret_cast<const char*>(&map.height), sizeof(short));
	std::string name(map.name.toUtf8().constData());
	ofs.write(name.c_str(), 24);
	unsigned short x = 0;
	unsigned short y = 0;
	QPoint p = {};
	BYTE type = 0ui8;
	for (x = 0; x < map.width; ++x)
	{
		for (y = 0; y < map.height; ++y)
		{
			p.setX(x); p.setY(y);
			type = static_cast<BYTE>(map.data.value(p));
			ofs.write(reinterpret_cast<const char*>(&type), sizeof(BYTE));
		}
	}

	//寫入
	long long size = map.stair.size();
	long long i = 0;
	ofs.write(reinterpret_cast<const char*>(&size), sizeof(int));
	for (i = 0; i < size; ++i)
	{
		ofs.write(reinterpret_cast<const char*>(&map.stair[i].type), sizeof(BYTE));
		ofs.write(reinterpret_cast<const char*>(&map.stair[i].p.rx()), sizeof(short));
		ofs.write(reinterpret_cast<const char*>(&map.stair[i].p.ry()), sizeof(short));
	}

	// QSet<QPoint> workable = {};
	size = map.workable.size();
	ofs.write(reinterpret_cast<const char*>(&size), sizeof(int));
	QList<QPoint> list = map.workable.values();
	for (i = 0; i < size; ++i)
	{
		x = list[i].x();
		y = list[i].y();
		ofs.write(reinterpret_cast<const char*>(&x), sizeof(short));
		ofs.write(reinterpret_cast<const char*>(&y), sizeof(short));
	}

	ofs.flush();
	ofs.close();

	//cimage img(map.width, map.height);
	//for (unsigned short x = 0; x < map.width; ++x)
	//{
	//	for (unsigned short y = 0; y < map.height; ++y)
	//	{
	//		util::ObjectType type = map.data[QPoint{ x, y }];
	//		QColor color = MAP_COLOR_HASH.value(type, QColor(0, 0, 0));
	//		CRGB fillColor = { (uint8_t)color.red(), (uint8_t)color.green(), (uint8_t)color.blue() };
	//		img.setPixel(QPoint{ x, y }, fillColor);
	//	}
	//}
	//std::string path = "d:/";
	//path += std::to_string(map.floor);
	//path += +".bmp";
	//std::ofstream file(path, std::ios::binary);
	//file << img;

	return true;
}

bool MapAnalyzer::calcNewRoute(long long currentIndex, CAStar& astar, long long floor, const QPoint& src, const QPoint& dst, const QSet<QPoint>& blockList, std::vector<QPoint>* pPaths)
{
	do
	{
		if (pPaths == nullptr)
			break;

		if (src == dst)
		{
			pPaths->push_back(src);
			return true;
		}

		map_t map;
		if (!getMapDataByFloor(floor, &map))
		{
			if (!readFromBinary(currentIndex, floor, map.name))
				break;
			if (!getMapDataByFloor(floor, &map))
				break;
		}

		util::ObjectType dstobj = map.data.value(dst, util::OBJ_UNKNOWN);
		bool isDstAsWarpPoint = (dstobj == util::OBJ_WARP) || (dstobj == util::OBJ_JUMP) || (dstobj == util::OBJ_UP) || (dstobj == util::OBJ_DOWN);

		if (!isDstAsWarpPoint && dstobj != util::OBJ_ROAD)
			break;

		Injector& injector = Injector::getInstance(currentIndex);

		AStarCallback canPassCallback = [&map, &blockList, &injector, isDstAsWarpPoint, &src](const QPoint& point)->bool
			{
				if (point == src)
					return true;

				if (blockList.contains(point))
					return false;

				//村內避免踩NPC
				if (map.floor == 2000)
				{
					if (injector.worker->npcUnitPointHash.contains(point))
					{
						mapunit_t unit = injector.worker->npcUnitPointHash.value(point);
						if (unit.type == util::OBJ_NPC && unit.modelid > 0)
							return false;
					}

					//送貨門口傳點容易誤踩
					if (point == QPoint(102, 80) || point == QPoint(103, 80))
						return false;
				}

				const util::ObjectType obj = map.data.value(point, util::OBJ_UNKNOWN);

				//If the destination coordinates are a teleportation point, treat it as a non-obstacle
				if (isDstAsWarpPoint)
					return (obj == util::OBJ_ROAD) || (obj == util::OBJ_WARP) || (obj == util::OBJ_JUMP) || (obj == util::OBJ_UP) || (obj == util::OBJ_DOWN);
				else
					return  (obj == util::OBJ_ROAD);
			};

		astar.set_canpass(canPassCallback);
		astar.set_corner(true);
		astar.init(map.width, map.height);

		try
		{
			return astar.find(src, dst, pPaths);
		}
		catch (...)
		{
			qDebug() << __FUNCTION__ << " @" << __LINE__ << " Exception.";
		}
	} while (false);

	return false;
}

//快速檢查是否能通行
bool MapAnalyzer::isPassable(long long currentIndex, CAStar& astar, long long floor, const QPoint& src, const QPoint& dst)
{

	bool bret = false;
	do
	{
		map_t map;
		if (!getMapDataByFloor(floor, &map))
		{
			if (!readFromBinary(currentIndex, floor, map.name))
				break;
			if (!getMapDataByFloor(floor, &map))
				break;
		}

		if (src == dst)
			return true;

		AStarCallback canPassCallback = [&map, &src](const QPoint& p)->bool
			{
				if (src == p)
					return true;

				const util::ObjectType& obj = map.data.value(p, util::OBJ_UNKNOWN);
				return obj == util::OBJ_ROAD;
				//return ((obj != util::OBJ_EMPTY) && (obj != util::OBJ_WATER) && (obj != util::OBJ_UNKNOWN) && (obj != util::OBJ_WALL) && (obj != util::OBJ_ROCK) && (obj != util::OBJ_ROCKEX));
			};

		astar.set_canpass(canPassCallback);
		astar.set_corner(true);
		astar.init(map.width, map.height);

		try
		{
			return astar.find(src, dst, nullptr);
		}
		catch (...)
		{
			qDebug() << __FUNCTION__ << " @" << __LINE__ << " Exception.";
		}
	} while (false);

	return bret;
}

QString MapAnalyzer::getGround(long long currentIndex, long long floor, const QString& name, const QPoint& src)
{
	map_t map;
	if (getMapDataByFloor(floor, &map) &&
		(map.ground.isEmpty() || map.object.isEmpty() || map.flag.isEmpty()))
	{
		if (!readFromBinary(currentIndex, floor, name, false, true))
			return 0;

		if (!getMapDataByFloor(floor, &map))
			return 0;
	}

	static const QHash<long long, QString> hash = {
		{ static_cast<long long>(util::OBJ_UNKNOWN), "Unknown" },
		{ static_cast<long long>(util::OBJ_ROAD), "Road" },
		{ static_cast<long long>(util::OBJ_UP), "Up" },
		{ static_cast<long long>(util::OBJ_DOWN), "Down" },
		{ static_cast<long long>(util::OBJ_JUMP), "Jump" },
		{ static_cast<long long>(util::OBJ_WARP), "Warp" },
		{ static_cast<long long>(util::OBJ_WALL), "Wall" },
		{ static_cast<long long>(util::OBJ_ROCK), "Rock" },
		{ static_cast<long long>(util::OBJ_ROCKEX), "RockEx" },
		{ static_cast<long long>(util::OBJ_BOUNDARY), "Boundary" },
		{ static_cast<long long>(util::OBJ_WATER), "Water" },
		{ static_cast<long long>(util::OBJ_EMPTY), "Empty" },
		{ static_cast<long long>(util::OBJ_HUMAN), "Human" },
		{ static_cast<long long>(util::OBJ_NPC), "NPC" },
		{ static_cast<long long>(util::OBJ_BUILDING), "Building" },
		{ static_cast<long long>(util::OBJ_ITEM), "Item" },
		{ static_cast<long long>(util::OBJ_PET), "Pet" },
		{ static_cast<long long>(util::OBJ_GOLD), "Gold" },
		{ static_cast<long long>(util::OBJ_GM), "GM" },
		{ static_cast<long long>(util::OBJ_MAX), "Max" },
	};

	long long flag = map.flag.value(src, 0);
	return QString("%1|%2|%3|%4|MarkAs:%5").arg(map.ground.value(src, 0)).arg(map.object.value(src, 0)).arg(HIBYTE(flag)).arg(LOBYTE(flag)).arg(hash.value(map.data.value(src), "Unknown"));
}

// 取靠近目標的最佳座標和方向
long long MapAnalyzer::calcBestFollowPointByDstPoint(long long currentIndex, CAStar& astar, long long floor, const QPoint& src, const QPoint& dst, QPoint* ret, bool enableExt, long long npcdir)
{

	QVector<qdistance_t> disV;// <distance, point>
	map_t map;
	if (!getMapDataByFloor(floor, &map))
	{
		if (!readFromBinary(currentIndex, floor, map.name))
			return -1;
		if (!getMapDataByFloor(floor, &map))
			return -1;
	}

	long long d = 0;
	long long invalidcount = 0;

	for (const QPoint& it : util::fix_point)
	{
		qdistance_t c = {};
		c.dir = d;
		c.pf = dst + it;
		c.p = dst + it;
		if (src == c.p)//如果已經在目標點
		{
			if (ret)
				*ret = c.p;
			long long n = c.dir + 4;
			return ((n) <= (7)) ? (n) : ((n)-(MAX_DIR));
		}

		if (isPassable(currentIndex, astar, floor, src, dst + it))//確定是否可走
		{
			//計算src 到 c.p直線距離
			c.distance = std::sqrt(std::pow((qreal)src.x() - c.pf.x(), 2) + std::pow((qreal)src.y() - c.pf.y(), 2));
		}
		else//不可走就隨便加個超長距離
		{
			c.distance = std::numeric_limits<double>::max();
			++invalidcount;
		}
		++d;
		disV.append(c);
	}

	if (invalidcount >= MAX_DIR && enableExt && npcdir != -1)//如果周圍8格都不能走搜尋NPC面相方向兩格(中間隔著櫃檯)
	{
		for (long long i = 0; i < 7; ++i)
		{
			QPoint newP;
			switch (i)//找出NPC面相方向的兩格
			{
			case 0://NPC面相北找往北兩格
				newP = dst + QPoint(0, -2);
				break;
			case 2://NPC面相東
				newP = dst + QPoint(2, 0);
				break;
			case 4://NPC面相南
				newP = dst + QPoint(0, 2);
				break;
			case 6://NPC面相西
				newP = dst + QPoint(-2, 0);
				break;
			}

			if (isPassable(currentIndex, astar, floor, src, newP) || src == newP)//確定是否可走
			{
				//qdistance_t c = {};
				//要面相npc的方向  (當前人物要面向newP的方向)
				if (ret)
					*ret = newP;
				long long n = npcdir + 4;
				return ((n) <= (7)) ? (n) : ((n)-(MAX_DIR));
			}
		}
		return -1;
	}
	else if (invalidcount >= 8)// 如果周圍8格都不能走
	{
		return -1;
	}

	std::sort(disV.begin(), disV.end(), compareDistance);

	if (!disV.size()) return -1;
	if (ret)
		*ret = disV.value(0).p;
	//計算方向
	long long n = disV.value(0).dir + 4;
	return ((n) <= (7)) ? (n) : ((n)-(MAX_DIR));//返回方向
}
