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
#include "parser.h"

#include "signaldispatcher.h"
#include "injector.h"
#include "interpreter.h"


//"調用" 傳參數最小佔位
constexpr qint64 kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr qint64 kFormatPlaceHoldSize = 3;

#pragma region  LuaTools
void makeTable(sol::state& lua, const char* name, qint64 i, qint64 j)
{
	if (!lua[name].valid())
		lua[name] = lua.create_table();
	else if (!lua[name].is<sol::table>())
		lua[name] = lua.create_table();

	qint64 k, l;

	for (k = 1; k <= i; ++k)
	{
		if (!lua[name][k].valid())
			lua[name][k] = lua.create_table();
		else if (!lua[name][k].is<sol::table>())
			lua[name][k] = lua.create_table();

		for (l = 1; l <= j; ++l)
		{
			if (!lua[name][k][l].valid())
				lua[name][k][l] = lua.create_table();
			else if (!lua[name][k][l].is<sol::table>())
				lua[name][k][l] = lua.create_table();
		}
	}
}

void makeTable(sol::state& lua, const char* name, qint64 i)
{
	if (!lua[name].valid())
		lua[name] = lua.create_table();
	else if (!lua[name].is<sol::table>())
		lua[name] = lua.create_table();

	qint64 k;
	for (k = 1; k <= i; ++k)
	{
		if (!lua[name][k].valid())
			lua[name][k] = lua.create_table();
		else if (!lua[name][k].is<sol::table>())
			lua[name][k] = lua.create_table();
	}
}

void makeTable(sol::state& lua, const char* name)
{
	if (!lua[name].valid())
		lua[name] = lua.create_table();
	else if (!lua[name].is<sol::table>())
		lua[name] = lua.create_table();
}

std::vector<std::string> Unique(const std::vector<std::string>& v)
{
	std::vector<std::string> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::stable_sort(result, std::less<std::string>());
#else
	std::sort(result.begin(), result.end(), std::less<std::string>());
#endif
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

std::vector<qint64> Unique(const std::vector<qint64>& v)
{
	std::vector<qint64> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::stable_sort(result, std::less<qint64>());
#else
	std::sort(result.begin(), result.end(), std::less<qint64>());
#endif
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

template<typename T>
std::vector<T> ShiftLeft(const std::vector<T>& v, qint64 i)
{
	std::vector<T> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::shift_left(result, i);
#else
	std::rotate(result.begin(), result.begin() + i, result.end());
#endif
	return result;

}

template<typename T>
std::vector<T> ShiftRight(const std::vector<T>& v, qint64 i)
{
	std::vector<T> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::shift_right(result, i);
#else
	std::rotate(result.begin(), result.end() - i, result.end());
#endif
	return result;
}

template<typename T>
std::vector<T> Shuffle(const std::vector<T>& v)
{
	std::vector<T> result = v;
	std::random_device rd;
	std::mt19937_64 gen(rd());
	//std::default_random_engine eng(rd());
#if _MSVC_LANG > 201703L
	std::ranges::shuffle(result, gen);
#else
	std::shuffle(result.begin(), result.end(), gen);
#endif
	return result;
}

template<typename T>
std::vector<T> Rotate(const std::vector<T>& v, qint64 len)//true = right, false = left
{
	std::vector<T> result = v;
	if (len >= 0)
#if _MSVC_LANG > 201703L
		std::ranges::rotate(result, result.begin() + len);
#else
		std::rotate(result.begin(), result.begin() + len, result.end());
#endif
	else
#if _MSVC_LANG > 201703L
		std::ranges::rotate(result, result.end() + len);
#else
		std::rotate(result.begin(), result.end() + len, result.end());
#endif
	return result;
}
#pragma endregion

Parser::Parser(qint64 index)
	: ThreadPlugin(index, nullptr)
	, lexer_(index)
{
	qDebug() << "Parser is created!!";
	setIndex(index);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &Parser::requestInterruption, Qt::UniqueConnection);

	pLua_.reset(new CLua(index));

	pLua_->openlibs();

	pLua_->setHookForStop(true);

	sol::state& lua_ = pLua_->getLua();

	const QStringList exceptionList = { "char", "pet", "item", "map", "magic", "skill", "petskill", "petequip", "dialog", "chat", "battle", "point", "team", "card", "unit" };
	globalNames_.append(exceptionList);

	makeTable(lua_, "battle", MAX_ENEMY);
	makeTable(lua_, "unit", 2000);
	makeTable(lua_, "chat", MAX_CHAT_HISTORY);
	makeTable(lua_, "card", MAX_ADDRESS_BOOK);
	makeTable(lua_, "map");
	makeTable(lua_, "team", MAX_PARTY);
	makeTable(lua_, "item", MAX_ITEM + 200);
	makeTable(lua_, "pet", MAX_PET);
	makeTable(lua_, "char");
	makeTable(lua_, "timer");
	makeTable(lua_, "dialog", MAX_DIALOG_LINE);
	makeTable(lua_, "magic", MAX_MAGIC);
	makeTable(lua_, "skill", MAX_PROFESSION_SKILL);
	makeTable(lua_, "petksill", MAX_PET, MAX_SKILL);
	makeTable(lua_, "petequip", MAX_PET, MAX_PET_ITEM);
	makeTable(lua_, "point");

	lua_.set_function("contains", [this](sol::object data, sol::object ocmp_data, sol::this_state s)->bool
		{
			auto toVariant = [](const sol::object& o)->QVariant
			{
				QVariant out;
				if (o.is<std::string>())
					out = util::toQString(o).simplified();
				else if (o.is<qint64>())
					out = o.as<qint64>();
				else if (o.is<double>())
					out = o.as<double>();
				else if (o.is<bool>())
					out = o.as<bool>();

				return out;
			};

			if (data.is<sol::table>() && ocmp_data.is<sol::table>())
			{
				sol::table table = data.as<sol::table>();
				sol::table table2 = ocmp_data.as<sol::table>();
				QStringList a;
				QStringList b;

				for (const std::pair<sol::object, sol::object>& pair : table)
					a.append(toVariant(pair.second).toString().simplified());

				for (const std::pair<sol::object, sol::object>& pair : table2)
					b.append(toVariant(pair.second).toString().simplified());

				return QSet<QString>(a.begin(), a.end()).contains(QSet<QString>(b.begin(), b.end())) || QSet<QString>(b.begin(), b.end()).contains(QSet<QString>(a.begin(), a.end()));
			}
			else if (data.is<sol::table>() && !ocmp_data.is<sol::table>())
			{
				QStringList a;
				QString b = toVariant(ocmp_data).toString().simplified();
				sol::table table = data.as<sol::table>();
				for (const std::pair<sol::object, sol::object>& pair : table)
					a.append(toVariant(pair.second).toString().simplified());

				return a.contains(b);
			}

			QString qcmp_data;
			QVariant adata = toVariant(ocmp_data);
			QVariant bdata = toVariant(data);

			if (adata.type() == QVariant::Type::String)
				qcmp_data = adata.toString().simplified();
			else if (adata.type() == QVariant::Type::LongLong)
				qcmp_data = util::toQString(adata.toLongLong());
			else if (adata.type() == QVariant::Type::Double)
				qcmp_data = util::toQString(adata.toDouble());
			else if (adata.type() == QVariant::Type::Bool)
				qcmp_data = util::toQString(adata.toBool());
			else
				return false;

			if (bdata.type() == QVariant::Type::String)
				return bdata.toString().simplified().contains(qcmp_data) || qcmp_data.contains(bdata.toString().simplified());
			else if (bdata.type() == QVariant::Type::LongLong)
				return util::toQString(bdata.toLongLong()).contains(qcmp_data) || qcmp_data.contains(util::toQString(bdata.toLongLong()));
			else if (bdata.type() == QVariant::Type::Double)
				return util::toQString(bdata.toDouble()).contains(qcmp_data) || qcmp_data.contains(util::toQString(bdata.toDouble()));
			else if (bdata.type() == QVariant::Type::Bool)
				return util::toQString(bdata.toBool()).contains(qcmp_data) || qcmp_data.contains(util::toQString(bdata.toBool()));
			else
				return false;

			return false;

		}
	);

	sol::meta::unqualified_t<sol::table> timer = lua_["timer"];

	timer["new"] = [this](sol::this_state s)->qint64
	{
		QSharedPointer<QElapsedTimer> timer(new QElapsedTimer());
		if (timer == nullptr)
			return 0;

		timer->start();
		quint64 id = 0;
		for (;;)
		{
			id = QRandomGenerator64::global()->generate64();
			if (timerMap_.contains(id))
				continue;

			timerMap_.insert(id, timer);
			break;
		}

		return id;
	};

	timer["get"] = [this](qint64 id, sol::this_state s)->qint64
	{
		if (id == 0)
			return 0;

		QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
		if (timer == nullptr)
			return 0;

		return timer->elapsed();
	};

	timer["gets"] = [this](qint64 id, sol::this_state s)->qint64
	{
		if (id == 0)
			return 0;

		QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
		if (timer == nullptr)
			return 0;

		return timer->elapsed() / 1000;
	};

	timer["getstr"] = [this](qint64 id, sol::this_state s)->std::string
	{
		if (id == 0)
			return "";

		QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
		if (timer == nullptr)
			return "";

		qint64 time = timer->elapsed();
		QString formated = util::formatMilliseconds(time);
		return formated.toUtf8().constData();
	};

	timer["del"] = [this](qint64 id, sol::this_state s)->bool
	{
		if (id == 0)
			return "";

		QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
		if (timer == nullptr)
			return false;

		timerMap_.remove(id);
		return true;
	};

	lua_.set_function("input", [this, index](sol::object oargs, sol::this_state s)->sol::object
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);

			std::string sargs = "";
			if (oargs.is<std::string>())
				sargs = oargs.as<std::string>();

			QString args = util::toQString(sargs);

			QStringList argList = args.split(util::rexOR, Qt::SkipEmptyParts);
			qint64 type = QInputDialog::InputMode::TextInput;
			QString msg;
			QVariant var;
			bool ok = false;
			if (argList.size() > 0)
			{
				type = argList.front().toLongLong(&ok) - 1ll;
				if (type < 0 || type > 2)
					type = QInputDialog::InputMode::TextInput;
			}

			if (argList.size() > 1)
			{
				msg = argList.at(1);
			}

			if (argList.size() > 2)
			{
				if (type == QInputDialog::IntInput)
					var = QVariant(argList.at(2).toLongLong(&ok));
				else if (type == QInputDialog::DoubleInput)
					var = QVariant(argList.at(2).toDouble(&ok));
				else
					var = QVariant(argList.at(2));
			}

			emit signalDispatcher.inputBoxShow(msg, type, &var);

			if (var.toLongLong() == 987654321ll)
			{
				requestInterruption();
				insertGlobalVar("vret", "nil");
				return sol::lua_nil;
			}

			if (type == QInputDialog::IntInput)
			{
				insertGlobalVar("vret", var.toLongLong());
				return sol::make_object(s, var.toLongLong());
			}
			else if (type == QInputDialog::DoubleInput)
			{
				insertGlobalVar("vret", var.toDouble());
				return sol::make_object(s, var.toDouble());
			}
			else
			{
				QString str = var.toString();
				if (str.toLower() == "true" || str.toLower() == "false" || str.toLower() == "真" || str.toLower() == "假")
					return sol::make_object(s, var.toBool());

				insertGlobalVar("vret", var.toString());
				return sol::make_object(s, var.toString().toUtf8().constData());
			}
		});

	lua_.set_function("format", [this](std::string sformat, sol::this_state s)->sol::object
		{
			QString formatStr = util::toQString(sformat);
			if (formatStr.isEmpty())
				return sol::lua_nil;

			static const QRegularExpression rexFormat(R"(\{([T|C])?\s*\:([\w\p{Han}]+(\['*"*[\w\p{Han}]+'*"*\])*)\})");
			if (!formatStr.contains(rexFormat))
				return sol::make_object(s, sformat);

			enum FormatType
			{
				kNothing,
				kTime,
				kConst,
			};

			constexpr qint64 MAX_FORMAT_DEPTH = 10;

			for (qint64 i = 0; i < MAX_FORMAT_DEPTH; ++i)
			{
				QRegularExpressionMatchIterator matchIter = rexFormat.globalMatch(formatStr);
				//Group 1: T or C or nothing
				//Group 2: var or var table

				QMap<QString, QPair<FormatType, QString>> formatVarList;

				for (;;)
				{
					if (!matchIter.hasNext())
						break;

					const QRegularExpressionMatch match = matchIter.next();
					if (!match.hasMatch())
						break;

					QString type = match.captured(1);
					QString var = match.captured(2);
					QVariant varValue;

					if (type == "T")
					{
						varValue = luaDoString(QString("return %1").arg(var));
						formatVarList.insert(var, qMakePair(FormatType::kTime, util::formatSeconds(varValue.toLongLong())));
					}
					else if (type == "C")
					{
						QString str;
						if (var.toLower() == "date")
						{
							const QDateTime dt = QDateTime::currentDateTime();
							str = dt.toString("yyyy-MM-dd");

						}
						else if (var.toLower() == "time")
						{
							const QDateTime dt = QDateTime::currentDateTime();
							str = dt.toString("hh:mm:ss:zzz");
						}

						formatVarList.insert(var, qMakePair(FormatType::kConst, str));
					}
					else
					{
						varValue = luaDoString(QString("return %1").arg(var));
						formatVarList.insert(var, qMakePair(FormatType::kNothing, varValue.toString()));
					}
				}

				for (auto it = formatVarList.constBegin(); it != formatVarList.constEnd(); ++it)
				{
					const QString var = it.key();
					const QPair<FormatType, QString> varValue = it.value();

					if (varValue.first == FormatType::kNothing)
					{
						formatStr.replace(QString("{:%1}").arg(var), varValue.second);
					}
					else if (varValue.first == FormatType::kTime)
					{
						formatStr.replace(QString("{T:%1}").arg(var), varValue.second);
					}
					else if (varValue.first == FormatType::kConst)
					{
						formatStr.replace(QString("{C:%1}").arg(var), varValue.second);
					}
				}
			}

			insertGlobalVar("vret", formatStr);
			return sol::make_object(s, formatStr.toUtf8().constData());

		});

	lua_.set_function("half", [this](std::string sstr, sol::this_state s)->std::string
		{
			const QString FullWidth = "０１２３４５６７８９"
				"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
				"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
				"、～！＠＃＄％︿＆＊（）＿－＝＋［］｛｝＼｜；：’＂，＜．＞／？【】《》　";
			const QString HalfWidth = "0123456789"
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"'~!@#$%^&*()_-+=[]{}\\|;:'\",<.>/? []<>";

			if (sstr.empty())
				return sstr;

			QString result = util::toQString(sstr);
			qint64 size = FullWidth.size();
			for (qint64 i = 0; i < size; ++i)
			{
				result.replace(FullWidth.at(i), HalfWidth.at(i));
			}

			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	lua_.set_function("full", [this](std::string sstr, sol::this_state s)->std::string
		{
			const QString FullWidth = "０１２３４５６７８９"
				"ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
				"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ"
				"、～！＠＃＄％︿＆＊（）＿－＝＋［］｛｝＼｜；：’＂，＜．＞／？【】《》　";
			const QString HalfWidth = "0123456789"
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"'~!@#$%^&*()_-+=[]{}\\|;:'\",<.>/?[]<> ";

			if (sstr.empty())
				return sstr;

			QString result = util::toQString(sstr);
			qint64 size = FullWidth.size();
			for (qint64 i = 0; i < size; ++i)
			{
				result.replace(HalfWidth.at(i), FullWidth.at(i));
			}

			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	lua_.set_function("lower", [this](std::string sstr, sol::this_state s)->std::string
		{
			QString result = util::toQString(sstr).toLower();
			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	lua_.set_function("upper", [this](std::string sstr, sol::this_state s)->std::string
		{
			QString result = util::toQString(sstr).toUpper();
			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	lua_.set_function("trim", [this](std::string sstr, sol::object oisSimplified, sol::this_state s)->std::string
		{
			bool isSimplified = false;

			if (oisSimplified.is<bool>())
				isSimplified = oisSimplified.as<bool>();
			else if (oisSimplified.is<qint64>())
				isSimplified = oisSimplified.as<qint64>() > 0;
			else if (oisSimplified.is<double>())
				isSimplified = oisSimplified.as<double>() > 0.0;

			QString result = util::toQString(sstr).trimmed();

			if (isSimplified)
				result = result.simplified();

			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	lua_.set_function("todb", [this](sol::object ovalue, sol::object len, sol::this_state s)->double
		{
			double result = 0.0;
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue).toDouble();
			else if (ovalue.is<qint64>())
				result = static_cast<double>(ovalue.as<qint64>() * 1.0);
			else if (ovalue.is<double>())
				result = ovalue.as<double>();

			if (len.is<qint64>() && len.as<qint64>() >= 0 && len.as<qint64>() <= 16)
			{
				QString str = QString::number(result, 'f', len.as<qint64>());
				result = str.toDouble();
			}

			insertGlobalVar("vret", result);
			return result;
		});

	lua_.set_function("tostr", [this](sol::object ovalue, sol::this_state s)->std::string
		{
			QString result = "";
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue);
			else if (ovalue.is<qint64>())
				result = util::toQString(ovalue.as<qint64>());
			else if (ovalue.is<double>())
				result = util::toQString(ovalue.as<double>());

			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	lua_.set_function("toint", [this](sol::object ovalue, sol::this_state s)->qint64
		{
			qint64 result = 0.0;
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue).toLongLong();
			else if (ovalue.is<qint64>())
				result = ovalue.as<qint64>();
			else if (ovalue.is<double>())
				result = static_cast<qint64>(qFloor(ovalue.as<double>()));

			insertGlobalVar("vret", result);
			return result;
		});

	lua_.set_function("replace", [this](std::string ssrc, std::string sfrom, std::string sto, sol::object oisRex, sol::this_state s)->std::string
		{
			QString src = util::toQString(ssrc);
			QString from = util::toQString(sfrom);
			QString to = util::toQString(sto);

			bool isRex = false;
			if (oisRex.is<bool>())
				isRex = oisRex.as<bool>();
			else if (oisRex.is<qint64>())
				isRex = oisRex.as<qint64>() > 0;
			else if (oisRex.is<double>())
				isRex = oisRex.as<double>() > 0.0;

			QString result;
			if (!isRex)
				result = src.replace(from, to);
			else
			{
				const QRegularExpression regex(from);
				result = src.replace(regex, to);
			}

			insertGlobalVar("vret", result);

			return result.toUtf8().constData();
		});

	lua_.set_function("find", [this](std::string ssrc, std::string sfrom, sol::object osto, sol::this_state s)->std::string
		{
			QString varValue = util::toQString(ssrc);
			QString text1 = util::toQString(sfrom);
			QString text2 = "";
			if (osto.is<std::string>())
				text2 = util::toQString(osto);

			QString result = varValue;

			//查找 src 中 text1 到 text2 之间的文本 如果 text2 为空 则查找 text1 到行尾的文本

			qint64 pos1 = varValue.indexOf(text1);
			if (pos1 < 0)
				pos1 = 0;

			qint64 pos2 = -1;
			if (text2.isEmpty())
				pos2 = varValue.length();
			else
			{
				pos2 = static_cast<qint64>(varValue.indexOf(text2, pos1 + text1.length()));
				if (pos2 < 0)
					pos2 = varValue.length();
			}

			result = varValue.mid(pos1 + text1.length(), pos2 - pos1 - text1.length());

			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	//参数1:字符串内容, 参数2:正则表达式, 参数3(选填):第几个匹配项, 参数4(选填):是否为全局匹配, 参数5(选填):第几组
	lua_.set_function("regex", [this](std::string ssrc, std::string rexstr, sol::object oidx, sol::object oisglobal, sol::object ogidx, sol::this_state s)->std::string
		{
			QString varValue = util::toQString(ssrc);

			QString result = varValue;

			QString text = util::toQString(rexstr);

			qint64 capture = 1;
			if (oidx.is<qint64>())
				capture = oidx.as<qint64>();

			bool isGlobal = false;
			if (oisglobal.is<qint64>())
				isGlobal = oisglobal.as<qint64>() > 0;
			else if (oisglobal.is<bool>())
				isGlobal = oisglobal.as<bool>();

			qint64 maxCapture = 0;
			if (ogidx.is<qint64>())
				maxCapture = ogidx.as<qint64>();

			const QRegularExpression regex(text);

			if (!isGlobal)
			{
				QRegularExpressionMatch match = regex.match(varValue);
				if (match.hasMatch())
				{
					if (capture < 0 || capture > match.lastCapturedIndex())
					{
						insertGlobalVar("vret", result);
						return result.toUtf8().constData();
					}

					result = match.captured(capture);
				}
			}
			else
			{
				QRegularExpressionMatchIterator matchs = regex.globalMatch(varValue);
				qint64 n = 0;
				while (matchs.hasNext())
				{
					QRegularExpressionMatch match = matchs.next();
					if (++n != maxCapture)
						continue;

					if (capture < 0 || capture > match.lastCapturedIndex())
						continue;

					result = match.captured(capture);
					break;
				}
			}

			insertGlobalVar("vret", result);
			return result.toUtf8().constData();
		});

	//rex 参数1:来源字符串, 参数2:正则表达式
	lua_.set_function("rex", [this](std::string ssrc, std::string rexstr, sol::this_state s)->sol::table
		{
			QString src = util::toQString(ssrc);
			sol::state_view lua(s);
			sol::table result = lua.create_table();

			QString expr = util::toQString(rexstr);

			const QRegularExpression regex(expr);

			QRegularExpressionMatch match = regex.match(src);

			qint64 n = 1;
			if (match.hasMatch())
			{
				for (qint64 i = 0; i <= match.lastCapturedIndex(); ++i)
				{
					result[n] = match.captured(i).toUtf8().constData();
				}
			}

			qint64 maxdepth = kMaxLuaTableDepth;
			insertGlobalVar("vret", getLuaTableString(result, maxdepth));
			return result;
		});

	lua_.set_function("rexg", [this](std::string ssrc, std::string rexstr, sol::this_state s)->sol::table
		{
			QString src = util::toQString(ssrc);
			sol::state_view lua(s);
			sol::table result = lua.create_table();

			QString expr = util::toQString(rexstr);

			const QRegularExpression regex(expr);

			QRegularExpressionMatchIterator matchs = regex.globalMatch(src);

			qint64 n = 1;
			while (matchs.hasNext())
			{
				QRegularExpressionMatch match = matchs.next();

				if (!match.hasMatch())
					continue;

				for (const auto& capture : match.capturedTexts())
				{
					result[n] = capture.toUtf8().constData();
					++n;
				}
			}

			qint64 maxdepth = kMaxLuaTableDepth;
			insertGlobalVar("vret", getLuaTableString(result, maxdepth));
			return result;
		});

	lua_.set_function("rnd", [this](sol::object omin, sol::object omax, sol::this_state s)->qint64
		{
			std::random_device rd;
			std::mt19937_64 gen(rd());
			qint64 result = 0;
			if (omin == sol::lua_nil && omax == sol::lua_nil)
			{
				result = gen();
				insertGlobalVar("vret", result);
				return result;
			}

			qint64 min = 0;
			if (omin.is<qint64>())
				min = omin.as<qint64>();


			qint64 max = 0;
			if (omax.is<qint64>())
				max = omax.as<qint64>();

			if ((min > 0 && max == 0) || (min == max))
			{
				std::uniform_int_distribution<qint64> distribution(0, min);
				result = distribution(gen);
			}
			else if (min > max)
			{
				std::uniform_int_distribution<qint64> distribution(max, min);
				result = distribution(gen);
			}
			else
			{
				std::uniform_int_distribution<qint64> distribution(min, max);
				result = distribution(gen);
			}

			insertGlobalVar("vret", result);
			return result;
		});

	lua_["mkpath"] = [](std::string filename, sol::object obj, sol::this_state s)->std::string
	{
		QString retstring = "\0";
		if (obj == sol::lua_nil)
		{
			retstring = util::findFileFromName(util::toQString(filename) + ".txt");
		}
		else if (obj.is<std::string>())
		{
			retstring = util::findFileFromName(util::toQString(filename) + ".txt", util::toQString(obj));
		}

		return retstring.toUtf8().constData();
	};

	lua_["mktable"] = [](qint64 a, sol::object ob, sol::this_state s)->sol::object
	{
		sol::state_view lua(s);

		sol::table t = lua.create_table();

		if (ob.is<qint64>() && a > ob.as<qint64>())
		{
			for (qint64 i = ob.as<qint64>(); i < (a + 1); ++i)
			{
				t.add(i);
			}
		}
		else if (ob.is<qint64>() && a < ob.as<qint64>())
		{
			for (qint64 i = a; i < (ob.as<qint64>() + 1); ++i)
			{
				t.add(i);
			}
		}
		else if (ob.is<qint64>() && a == ob.as<qint64>())
		{
			t.add(a);
		}
		else if (ob == sol::lua_nil && a >= 0)
		{
			for (qint64 i = 1; i < a + 1; ++i)
			{
				t.add(i);
			}
		}
		else if (ob == sol::lua_nil && a < 0)
		{
			for (qint64 i = a; i < 2; ++i)
			{
				t.add(i);
			}
		}
		else
			t.add(a);

		return t;
	};

	static const auto Is_1DTable = [](sol::table t)->bool
	{
		for (const std::pair<sol::object, sol::object>& i : t)
		{
			if (i.second.is<sol::table>())
			{
				return false;
			}
		}
		return true;
	};

	lua_["tshuffle"] = [](sol::object t, sol::this_state s)->sol::object
	{
		if (!t.is<sol::table>())
			return sol::lua_nil;
		//檢查是否為1維
		sol::table test = t.as<sol::table>();
		if (test.size() == 0)
			return sol::lua_nil;
		if (!Is_1DTable(test))
			return sol::lua_nil;

		sol::state_view lua(s);
		std::vector<sol::object> v = t.as<std::vector<sol::object>>();
		std::vector<sol::object> v2 = Shuffle(v);
		sol::table t2 = lua.create_table();
		for (const sol::object& i : v2) { t2.add(i); }
		auto copy = [&t](sol::table src)
		{
			//清空原表
			t.as<sol::table>().clear();
			//將篩選後的表複製到原表
			for (const std::pair<sol::object, sol::object>& i : src)
			{
				t.as<sol::table>().add(i.second);
			}
		};
		copy(t2);
		return t2;
	};

	lua_["trotate"] = [](sol::object t, sol::object oside, sol::this_state s)->sol::object
	{
		if (!t.is<sol::table>())
			return sol::lua_nil;

		qint64 len = 1;
		if (oside == sol::lua_nil)
			len = 1;
		else if (oside.is<qint64>())
			len = oside.as<qint64>();
		else
			return sol::lua_nil;

		sol::table test = t.as<sol::table>();
		if (test.size() == 0)
			return sol::lua_nil;
		if (!Is_1DTable(test))
			return sol::lua_nil;

		sol::state_view lua(s);
		std::vector<sol::object> v = t.as<std::vector<sol::object>>();
		std::vector<sol::object> v2 = Rotate(v, len);
		sol::table t2 = lua.create_table();
		for (const sol::object& i : v2) { t2.add(i); }
		auto copy = [&t](sol::table src)
		{
			//清空原表
			t.as<sol::table>().clear();
			//將篩選後的表複製到原表
			for (const std::pair<sol::object, sol::object>& i : src)
			{
				t.as<sol::table>().add(i.second);
			}
		};
		copy(t2);
		return t2;
	};

	lua_["tsleft"] = [](sol::object t, qint64 i, sol::this_state s)->sol::object
	{
		if (!t.is<sol::table>())
			return sol::lua_nil;
		if (i < 0)
			return sol::lua_nil;

		sol::table test = t.as<sol::table>();
		if (test.size() == 0)
			return sol::lua_nil;
		if (!Is_1DTable(test))
			return sol::lua_nil;

		sol::state_view lua(s);
		std::vector<sol::object> v = t.as<std::vector<sol::object>>();
		std::vector<sol::object> v2 = ShiftLeft(v, i);
		sol::table t2 = lua.create_table();
		for (const sol::object& it : v2) { t2.add(it); }
		auto copy = [&t](sol::table src)
		{
			//清空原表
			t.as<sol::table>().clear();
			//將篩選後的表複製到原表
			for (const std::pair<sol::object, sol::object>& i : src)
			{
				t.as<sol::table>().add(i.second);
			}
		};
		copy(t2);
		return t2;
	};

	lua_["tsright"] = [](sol::object t, qint64 i, sol::this_state s)->sol::object
	{
		if (!t.is<sol::table>())
			return sol::lua_nil;
		if (i < 0)
			return sol::lua_nil;

		sol::table test = t.as<sol::table>();
		if (test.size() == 0)
			return sol::lua_nil;
		if (!Is_1DTable(test))
			return sol::lua_nil;

		sol::state_view lua(s);
		std::vector<sol::object> v = t.as<std::vector<sol::object>>();
		std::vector<sol::object> v2 = ShiftRight(v, i);
		sol::table t2 = lua.create_table();
		for (const sol::object& o : v2) { t2.add(o); }
		auto copy = [&t](sol::table src)
		{
			//清空原表
			t.as<sol::table>().clear();
			//將篩選後的表複製到原表
			for (const std::pair<sol::object, sol::object>& o : src)
			{
				t.as<sol::table>().add(o.second);
			}
		};
		copy(t2);
		return t2;
	};

	lua_["tunique"] = [](sol::object t, sol::this_state s)->sol::object
	{
		if (!t.is<sol::table>())
			return sol::lua_nil;

		sol::table test = t.as<sol::table>();
		if (test.size() == 0)
			return sol::lua_nil;

		auto isIntTable = [&test]()->bool
		{
			for (const std::pair<sol::object, sol::object>& i : test)
			{
				if (!i.second.is<qint64>())
					return false;
			}
			return true;
		};

		auto isStringTable = [&test]()->bool
		{
			for (const std::pair<sol::object, sol::object>& i : test)
			{
				if (!i.second.is<std::string>())
					return false;
			}
			return true;
		};

		auto copy = [&t](sol::table src)
		{
			//清空原表
			t.as<sol::table>().clear();
			//將篩選後的表複製到原表
			for (const std::pair<sol::object, sol::object>& i : src)
			{
				t.as<sol::table>().add(i.second);
			}
		};

		sol::state_view lua(s);
		sol::table t2 = lua.create_table();
		if (isIntTable())
		{
			std::vector<qint64> v = t.as<std::vector<qint64>>();
			std::vector<qint64> v2 = Unique(v);
			for (const qint64& i : v2) { t2.add(i); }
			copy(t2);
			return t2;
		}
		else if (isStringTable())
		{
			std::vector<std::string> v = t.as<std::vector<std::string>>();
			std::vector<std::string> v2 = Unique(v);
			for (const std::string& i : v2) { t2.add(i); }
			copy(t2);
			return t2;
		}
		else
			return sol::lua_nil;
	};

	lua_["tsort"] = [](sol::object t, sol::this_state s)->sol::object
	{
		sol::state_view lua(s);
		if (!t.is<sol::table>())
			return sol::lua_nil;

		sol::protected_function sort = lua["table"]["sort"];
		if (sort.valid())
		{
			sort(t);
		}
		return t;
	};

	lua_.safe_script(R"(
		trsort = function(t)
			local function reverseSort(a, b)
				if type(a) == "string" and type(b) == "string" then
					return utf8.codepoint(a) > utf8.codepoint(b)
				elseif type(a) == "number" and type(b) == "number" then
					return a > b
				elseif type(a) == "boolean" and type(b) == "boolean" then
					return a > b
				elseif type(a) == "table" and type(b) == "table" then
					return #a > #b
				end

				return a > b
			end

			table.sort(t, reverseSort)
			return t
		end

	)");

#pragma region _copyfunstr
	std::string _copyfunstr = R"(
		function copy(object)
			local lookup_table = {};
			local function _copy(object)
				if (type(object) ~= ("table")) then
					
					return object;
				elseif (lookup_table[object]) then
					return lookup_table[object];
				end
				local newObject = {};
				lookup_table[object] = newObject;
				for key, value in pairs(object) do
					newObject[_copy(key)] = _copy(value);
				end
				return setmetatable(newObject, getmetatable(object));
			end

			local ret = _copy(object);
			return ret;
		end
	)";

	sol::load_result lr = lua_.load(_copyfunstr);
	if (lr.valid())
	{
		sol::protected_function target = lr.get<sol::protected_function>();
		sol::bytecode target_bc = target.dump();
		lua_.safe_script(target_bc.as_string_view(), sol::script_pass_on_error);
		lua_.collect_garbage();
	}
#pragma endregion

	//表合併
	lua_["tmerge"] = [](sol::object t1, sol::object t2, sol::this_state s)->sol::object
	{
		if (!t1.is<sol::table>() || !t2.is<sol::table>())
			return sol::lua_nil;

		sol::state_view lua(s);
		sol::table t3 = lua.create_table();
		sol::table lookup_table_1 = lua.create_table();
		sol::table lookup_table_2 = lua.create_table();
		//sol::protected_function _copy = lua["copy"];
		sol::table test1 = t1.as<sol::table>();//_copy(t1, lookup_table_1, lua);
		sol::table test2 = t2.as<sol::table>();//_copy(t2, lookup_table_2, lua);
		if (!test1.valid() || !test2.valid())
			return sol::lua_nil;
		for (const std::pair<sol::object, sol::object>& i : test1)
		{
			t3.add(i.second);
		}
		for (const std::pair<sol::object, sol::object>& i : test2)
		{
			t3.add(i.second);
		}
		auto copy = [&t1](sol::table src)
		{
			//清空原表
			t1.as<sol::table>().clear();
			//將篩選後的表複製到原表
			for (const std::pair<sol::object, sol::object>& i : src)
			{
				t1.as<sol::table>().add(i.second);
			}
		};
		copy(t3);
		return t3;
	};

	lua_.set_function("split", [](std::string src, std::string del, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			sol::table t = lua.create_table();
			QString qsrc = util::toQString(src);
			QString qdel = util::toQString(del);
			const QRegularExpression re(qdel);
			QRegularExpression::NoMatchOption;
			QStringList v = qsrc.split(re);
			if (v.size() > 1)
			{
				for (const QString& i : v)
				{
					t.add(i.toUtf8().constData());
				}
				return t;
			}
			else
				return sol::lua_nil;
		});

	lua_["tjoin"] = [this](sol::table t, std::string del, sol::this_state s)->std::string
	{
		QStringList l = {};
		for (const std::pair<sol::object, sol::object>& i : t)
		{
			if (i.second.is<int>())
			{
				l.append(util::toQString(i.second.as<int>()));
			}
			else if (i.second.is<double>())
			{
				l.append(util::toQString(i.second.as<double>()));
			}
			else if (i.second.is<bool>())
			{
				l.append(util::toQString(i.second.as<bool>()));
			}
			else if (i.second.is<std::string>())
			{
				l.append(util::toQString(i.second));
			}
		}
		QString ret = l.join(util::toQString(del));
		insertGlobalVar("vret", ret);
		return  ret.toUtf8().constData();
	};

	//根據key交換表中的兩個元素
	lua_["tswap"] = [](sol::table t, sol::object key1, sol::object key2, sol::this_state s)->sol::object
	{
		if (!t.valid())
			return sol::lua_nil;
		if (!t[key1].valid() || !t[key2].valid())
			return sol::lua_nil;
		sol::object temp = t[key1];
		t[key1] = t[key2];
		t[key2] = temp;
		return t;
	};

	lua_["tadd"] = [](sol::table t, sol::object value, sol::this_state s)->sol::object
	{
		if (!t.valid())
			return sol::lua_nil;
		t.add(value);
		return t;
	};

	lua_["tpadd"] = [](sol::table t, sol::object value, sol::this_state s)->sol::object
	{
		sol::state_view lua(s);
		if (!t.valid())
			return sol::lua_nil;
		sol::function insert = lua["table"]["insert"];
		insert(t, 1, value);
		return t;
	};

	lua_["tpopback"] = [](sol::table t, sol::this_state s)->sol::object
	{
		sol::state_view lua(s);
		if (!t.valid())
			return sol::lua_nil;
		sol::function remove = lua["table"]["remove"];
		sol::object temp = t[t.size()];
		remove(t, t.size());
		return t;
	};

	lua_["tpopfront"] = [](sol::table t, sol::this_state s)->sol::object
	{
		sol::state_view lua(s);
		if (!t.valid())
			return sol::lua_nil;
		sol::function remove = lua["table"]["remove"];
		sol::object temp = t[1];
		remove(t, 1);
		return t;
	};

	lua_["tfront"] = [](sol::table t, sol::this_state s)->sol::object
	{
		if (!t.valid())
			return sol::lua_nil;
		return t[1];
	};

	lua_["tback"] = [](sol::table t, sol::this_state s)->sol::object
	{
		if (!t.valid())
			return sol::lua_nil;
		return t[t.size()];
	};
}

Parser::~Parser()
{
	qDebug() << "Parser is distory!!";
}

//讀取腳本文件並轉換成Tokens
bool Parser::loadFile(const QString& fileName, QString* pcontent)
{
	util::ScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return false;

	const QFileInfo fi(fileName);
	const QString suffix = "." + fi.suffix();

	QString c;
	if (suffix == util::SCRIPT_DEFAULT_SUFFIX)
	{
		QTextStream in(&f);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		in.setCodec(util::DEFAULT_CODEPAGE);
#else
		in.setEncoding(QStringConverter::Utf8);
#endif
		in.setGenerateByteOrderMark(true);
		c = in.readAll();
		c.replace("\r\n", "\n");
		isPrivate_ = false;
	}
#ifdef CRYPTO_H
	else if (suffix == util::SCRIPT_PRIVATE_SUFFIX_DEFAULT)
	{
		Crypto crypto;
		if (!crypto.decodeScript(fileName, c))
			return false;

		isPrivate_ = true;
	}
#endif
	else
	{
		return false;
	}

	if (pcontent != nullptr)
	{
		*pcontent = c;
	}

	bool bret = loadString(c);
	if (bret)
		scriptFileName_ = fileName;
	return bret;
}

//將腳本內容轉換成Tokens
bool Parser::loadString(const QString& content)
{
	bool bret = Lexer::tokenized(&lexer_, content);

	if (bret)
	{
		tokens_ = lexer_.getTokenMaps();
		labels_ = lexer_.getLabelList();
		functionNodeList_ = lexer_.getFunctionNodeList();
		forNodeList_ = lexer_.getForNodeList();
		luaNodeList_ = lexer_.getLuaNodeList();
	}

	return bret;
}

void Parser::insertUserCallBack(const QString& name, const QString& type)
{
	//確保一種類型只能被註冊一次
	for (auto it = userRegCallBack_.cbegin(); it != userRegCallBack_.cend(); ++it)
	{
		if (it.value() == type)
			return;
		if (it.key() == name)
			return;
	}

	userRegCallBack_.insert(name, type);
}

//根據token解釋腳本
void Parser::parse(qint64 line)
{
	setCurrentLine(line); //設置當前行號
	callStack_.clear(); //清空調用棧
	jmpStack_.clear();  //清空跳轉棧

	if (tokens_.isEmpty())
		return;

	processTokens();
}

//處理錯誤
void Parser::handleError(qint64 err, const QString& addition)
{
	if (err == kNoChange)
		return;

	QString extMsg;
	if (!addition.isEmpty())
		extMsg = " " + addition;

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (err == kError)
		emit signalDispatcher.addErrorMarker(getCurrentLine(), err);

	QString msg;
	switch (err)
	{
	case kNoChange:
		return;
	case kError:
		msg = QObject::tr("unknown error") + extMsg;
		break;
	case kServerNotReady:
	{
		msg = QObject::tr("server not ready") + extMsg;
		break;
	}
	case kLabelError:
		msg = QObject::tr("label incorrect or not exist") + extMsg;
		break;
	case kUnknownCommand:
	{
		QString cmd = currentLineTokens_.value(0).data.toString();
		msg = QObject::tr("unknown command: %1").arg(cmd) + extMsg;
		break;
	}
	case kLuaError:
	{
		QString cmd = currentLineTokens_.value(0).data.toString();
		msg = QObject::tr("[lua]:%1").arg(cmd) + extMsg;
		break;
	}
	default:
	{
		if (err >= kArgError && err <= kArgError + 20ll)
		{
			if (err == kArgError)
				msg = QObject::tr("argument error") + extMsg;
			else
				msg = QObject::tr("argument error") + QString(" idx:%1").arg(err - kArgError) + extMsg;
			break;
		}
		break;
	}
	}

	emit signalDispatcher.appendScriptLog(QObject::tr("[error]") + QObject::tr("@ %1 | detail:%2").arg(getCurrentLine() + 1).arg(msg), 6);
}

//三目運算表達式替換
void Parser::checkConditionalOp(QString& expr)
{
	static const QRegularExpression rexConditional(R"(([^\,]+)\s+\?\s+([^\,]+)\s+\:\s+([^\,]+))");
	if (expr.contains(rexConditional))
	{
		QRegularExpressionMatch match = rexConditional.match(expr);
		if (match.hasMatch())
		{
			//left ? mid : right
			QString left = match.captured(1);
			QString mid = match.captured(2);
			QString right = match.captured(3);

			//to lua style: (left and { mid } or { right })[1]

			QString luaExpr = QString("(((((%1) ~= nil) and ((%1) == true) or (type(%1) == 'number' and (%1) > 0)) and { %2 } or { %3 })[1])").arg(left).arg(mid).arg(right);
			expr = luaExpr;
		}
	}
}

//執行指定lua語句
QVariant Parser::luaDoString(QString expr)
{
	if (expr.isEmpty())
	{
		return "nil";
	}

	importVariablesToLua(expr);
	checkConditionalOp(expr);

	QString exprStr = QString("%1\n%2").arg(luaLocalVarStringList.join("\n")).arg(expr);

	sol::state& lua_ = pLua_->getLua();

	const std::string exprStrUTF8 = exprStr.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8, sol::script_pass_on_error);
	lua_.collect_garbage();
	if (!loaded_chunk.valid())
	{
		qint64 currentLine = getCurrentLine();
		sol::error err = loaded_chunk;
		QString errStr = util::toQString(err.what());
		//if (!isGlobalVarContains(QString("_LUAERR_AT_%1").arg(currentLine)))
		//	insertGlobalVar(QString("_LUA_DO_STR_ERR_AT_%1").arg(currentLine), QString("[[%1]]").arg(exprStr));
		//else
		//{
		//	for (qint64 i = 0; i < 1000; ++i)
		//	{
		//		if (!isGlobalVarContains(QString("_LUA_DO_STR_ERR_AT_%1_%2").arg(currentLine).arg(i)))
		//		{
		//			insertGlobalVar(QString("_LUA_DO_STR_ERR_AT_%1_%2").arg(currentLine).arg(i), QString("[[%1]]").arg(exprStr));
		//			break;
		//		}
		//	}
		//}
		handleError(kLuaError, errStr);
		return "nil";
	}

	sol::object retObject;
	try
	{
		retObject = loaded_chunk;
	}
	catch (...)
	{
		return "nil";
	}

	if (retObject.is<bool>())
		return retObject.as<bool>();
	else if (retObject.is<std::string>())
		return util::toQString(retObject);
	else if (retObject.is<qint64>())
		return retObject.as<qint64>();
	else if (retObject.is<double>())
		return retObject.as<double>();
	else if (retObject.is<sol::table>())
	{
		qint64 deep = kMaxLuaTableDepth;
		return getLuaTableString(retObject.as<sol::table>(), deep);
	}

	return "nil";
}

//取表達式結果
template <typename T>
typename std::enable_if<
	std::is_same<T, QString>::value ||
	std::is_same<T, QVariant>::value ||
	std::is_same<T, bool>::value ||
	std::is_same<T, qint64>::value ||
	std::is_same<T, double>::value
	, bool>::type
	Parser::exprTo(QString expr, T* ret)
{
	if (nullptr == ret)
		return false;

	expr = expr.simplified();

	if (expr.isEmpty())
	{
		*ret = T();
		return true;
	}

	if (expr == "return" || expr == "back" || expr == "continue" || expr == "break"
		|| expr == QString(u8"返回") || expr == QString(u8"跳回") || expr == QString(u8"繼續") || expr == QString(u8"跳出")
		|| expr == QString(u8"继续"))
	{
		if constexpr (std::is_same<T, QVariant>::value || std::is_same<T, QString>::value)
			*ret = expr;
		return true;
	}

	QVariant var = luaDoString(QString("return %1").arg(expr));

	if constexpr (std::is_same<T, QString>::value)
	{
		*ret = var.toString();
		return true;
	}
	else if constexpr (std::is_same<T, QVariant>::value)
	{
		*ret = var;
		return true;
	}
	else if constexpr (std::is_same<T, bool>::value)
	{
		*ret = var.toBool();
		return true;
	}
	else if constexpr (std::is_same<T, qint64>::value)
	{
		*ret = var.toLongLong();
		return true;
	}
	else if constexpr (std::is_same<T, double>::value)
	{
		*ret = var.toDouble();
		return true;
	}

	return false;
}

//取表達式結果 += -= *= /= %= ^=
template <typename T>
typename std::enable_if<std::is_same<T, qint64>::value || std::is_same<T, double>::value || std::is_same<T, bool>::value, bool>::type
Parser::exprCAOSTo(const QString& varName, QString expr, T* ret)
{
	if (nullptr == ret)
		return false;

	expr = expr.simplified();

	if (expr.isEmpty())
	{
		*ret = T();
		return true;
	}

	QString exprStr = QString("%1 = %2 %3;\nreturn %4;").arg(varName).arg(varName).arg(expr).arg(varName);

	QVariant var = luaDoString(exprStr);

	if constexpr (std::is_same<T, bool>::value)
	{
		*ret = var.toBool();
		return true;
	}
	else if constexpr (std::is_same<T, qint64>::value)
	{
		*ret = var.toLongLong();
		return true;
	}
	else if constexpr (std::is_same<T, double>::value)
	{
		*ret = var.toDouble();
		return true;
	}

	return false;
}

void Parser::removeEscapeChar(QString* str) const
{
	if (nullptr == str)
		return;

	str->replace("\\\"", "@@@@@@@@@@");
	str->replace("\"", "");
	str->replace("@@@@@@@@@@", "\"");

	str->replace("'", "@@@@@@@@@@");
	str->replace("\\'", "");
	str->replace("@@@@@@@@@@", "'");

	str->replace("\\\"", "\"");
	str->replace("\\\'", "\'");
	str->replace("\\\\", "\\");
	str->replace("\\n", "\n");
	str->replace("\\r", "\r");
	str->replace("\\t", "\t");
	str->replace("\\b", "\b");
	str->replace("\\f", "\f");
	str->replace("\\v", "\v");
	str->replace("\\a", "\a");
	str->replace("\\?", "\?");
	str->replace("\\0", "\0");
}

//嘗試取指定位置的token轉為QVariant
bool Parser::toVariant(const TokenMap& TK, qint64 idx, QVariant* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	if (!var.isValid())
		return false;

	if (type == TK_STRING || type == TK_CMD)
	{
		QString s = var.toString();

		QString exprStrUTF8;
		qint64 value = 0;
		if (exprTo(s, &value))
			*ret = value;
		else if (exprTo(s, &exprStrUTF8))
			*ret = exprStrUTF8;
		else
			*ret = var;
	}
	else if (type == TK_BOOL)
	{
		bool ok = false;
		qint64 value = var.toLongLong(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else if (type == TK_DOUBLE)
	{
		bool ok = false;
		double value = var.toDouble(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else
	{
		*ret = var;
	}

	return true;
}

//嘗試取指定位置的token轉為字符串
bool Parser::checkString(const TokenMap& TK, qint64 idx, QString* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx, Token{}).type;
	QVariant var = TK.value(idx, Token{}).data;

	if (!var.isValid())
		return false;

	if (type == TK_CSTRING)
	{
		*ret = var.toString();
		removeEscapeChar(ret);
	}
	else if (type == TK_STRING || type == TK_CMD)
	{
		*ret = var.toString();
		removeEscapeChar(ret);

		QString exprStrUTF8;
		if (!exprTo(*ret, &exprStrUTF8))
			return false;

		*ret = exprStrUTF8;

	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為整數
bool Parser::checkInteger(const TokenMap& TK, qint64 idx, qint64* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;

	if (!var.isValid())
		return false;
	if (type == TK_CSTRING)
		return false;

	if (type == TK_INT)
	{
		bool ok = false;
		qint64 value = var.toLongLong(&ok);
		if (!ok)
			return false;

		*ret = value;
		return true;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_CAOS)
	{
		QString varValueStr = var.toString();

		QVariant value;
		if (!exprTo(varValueStr, &value))
			return false;

		if (value.type() == QVariant::LongLong || value.type() == QVariant::Int)
		{
			*ret = value.toLongLong();
			return true;
		}
		else if (value.type() == QVariant::Double)
		{
			*ret = value.toDouble();
			return true;
		}
		else if (value.type() == QVariant::Bool)
		{
			*ret = value.toBool() ? 1ll : 0ll;
			return true;
		}
		else
			return false;
	}

	if (type == TK_BOOL)
	{
		bool value = var.toBool();

		*ret = value ? 1ll : 0ll;
		return true;
	}
	else if (type == TK_DOUBLE)
	{
		bool ok = false;
		double value = var.toDouble(&ok);
		if (!ok)
			return false;

		*ret = value;
		return true;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_CAOS)
	{
		QString varValueStr = var.toString();

		QVariant value;
		if (!exprTo(varValueStr, &value))
			return false;

		if (value.type() == QVariant::LongLong || value.type() == QVariant::Int)
		{
			*ret = value.toLongLong();
			return true;
		}
	}

	return false;
}

bool Parser::checkNumber(const TokenMap& TK, qint64 idx, double* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;

	if (!var.isValid())
		return false;
	if (type == TK_CSTRING)
		return false;

	if (type == TK_DOUBLE && var.type() == QVariant::Double)
	{
		bool ok = false;
		double value = var.toDouble(&ok);
		if (!ok)
			return false;

		*ret = value;
		return true;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_CAOS)
	{
		QString varValueStr = var.toString();

		QVariant value;
		if (!exprTo(varValueStr, &value))
			return false;

		if (value.type() == QVariant::Double)
		{
			*ret = value.toDouble();
			return true;
		}
	}

	return false;
}

bool Parser::checkBoolean(const TokenMap& TK, qint64 idx, bool* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;

	if (!var.isValid())
		return false;
	if (type == TK_CSTRING)
		return false;

	if (type == TK_BOOL && var.type() == QVariant::Bool)
	{
		bool value = var.toBool();

		*ret = value;
		return true;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_CAOS)
	{
		QString varValueStr = var.toString();

		QVariant value;
		if (!exprTo(varValueStr, &value))
			return false;

		if (value.type() == QVariant::Bool)
		{
			*ret = value.toBool();
			return true;
		}
	}

	return false;
}

//嘗試取指定位置的token轉為按照double -> qint64 -> string順序檢查
QVariant Parser::checkValue(const TokenMap TK, qint64 idx, QVariant::Type type)
{
	QVariant varValue("nil");
	qint64 ivalue;
	QString text;
	bool bvalue;
	double dvalue;

	if (checkBoolean(currentLineTokens_, idx, &bvalue))
	{
		varValue = bvalue;
	}
	else if (checkNumber(currentLineTokens_, idx, &dvalue))
	{
		varValue = dvalue;
	}
	else if (checkInteger(currentLineTokens_, idx, &ivalue))
	{
		varValue = ivalue;
	}
	else if (checkString(currentLineTokens_, idx, &text))
	{
		varValue = text;
	}
	else
	{
		if (type == QVariant::String)
			varValue = "";
		else if (type == QVariant::Double)
			varValue = 0.0;
		else if (type == QVariant::Bool)
			varValue = false;
		else if (type == QVariant::LongLong)
			varValue = 0ll;
		else
			varValue = "nil";
	}

	return varValue;
}

//檢查跳轉是否滿足，和跳轉的方式
qint64 Parser::checkJump(const TokenMap& TK, qint64 idx, bool expr, JumpBehavior behavior)
{
	bool okJump = false;
	if (behavior == JumpBehavior::FailedJump)
		okJump = !expr;
	else
		okJump = expr;

	if (!okJump)
		return Parser::kNoChange;

	QString label;
	qint64 line = 0;
	if (TK.contains(idx))
	{
		QString preCheck = TK.value(idx).data.toString().simplified();

		if (preCheck == "return" || preCheck == "back" || preCheck == "continue" || preCheck == "break"
			|| preCheck == QString(u8"返回") || preCheck == QString(u8"跳回") || preCheck == QString(u8"繼續") || preCheck == QString(u8"跳出")
			|| preCheck == QString(u8"继续"))
		{

			label = preCheck;
		}
		else
		{
			QVariant var = checkValue(TK, idx);
			QVariant::Type type = var.type();
			if (type == QVariant::String && var.toString() != "nil")
			{
				label = var.toString();
				if (label.startsWith("'") || label.startsWith("\""))
					label = label.mid(1);
				if (label.endsWith("'") || label.endsWith("\""))
					label = label.left(label.length() - 1);
			}
			else if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double)
			{
				bool ok = false;
				qint64 value = 0;
				value = var.toLongLong(&ok);
				if (ok)
					line = value;
			}
			else
			{
				label = preCheck;
			}
		}
	}

	if (label.isEmpty() && line == 0)
		line = -1;

	if (!label.isEmpty())
	{
		jump(label, false);
	}
	else if (line != 0)
	{
		jump(line, false);
	}
	else
		return Parser::kArgError + idx;

	return Parser::kHasJump;
}

//檢查"調用"是否傳入參數
void Parser::checkCallArgs(qint64 n)
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QVariantList list;
	qint64 currentLine = getCurrentLine();
	if (tokens_.contains(currentLine))
	{
		TokenMap TK = tokens_.value(currentLine);
		qint64 size = TK.size();
		for (qint64 i = kCallPlaceHoldSize + n; i < size; ++i)
		{
			Token token = TK.value(i);
			QVariant var = checkValue(TK, i);
			if (!var.isValid())
				var = "nil";

			if (token.type != TK_FUZZY)
				list.append(var);
			else
				list.append(0ll);
		}
	}

	//無論如何都要在調用call之後壓入新的參數堆棧
	callArgsStack_.push(list);
}

//檢查是否需要跳過整個function代碼塊
bool Parser::checkCallStack()
{
	const QString cmd = currentLineTokens_.value(0).data.toString();
	if (cmd != "function")
		return false;

	const QString funname = currentLineTokens_.value(1).data.toString();

	if (skipFunctionChunkDisable_)
	{
		skipFunctionChunkDisable_ = false;
		return false;
	}

	do
	{
		//棧為空時直接跳過整個代碼塊
		if (callStack_.isEmpty())
			break;

		FunctionNode node = callStack_.top();

		qint64 lastRow = node.callFromLine;

		//匹配棧頂紀錄的function名稱與當前執行名稱是否一致
		QString oldname = tokens_.value(lastRow).value(1).data.toString();
		if (oldname != funname)
		{
			if (!checkString(tokens_.value(lastRow), 1, &oldname))
				break;

			if (oldname != funname)
				break;
		}

		return false;
	} while (false);

	if (matchLineFromFunction(funname) == -1)
		return false;

	FunctionNode node = getFunctionNodeByName(funname);
	jump(std::abs(node.endLine - node.beginLine) + 1, true);
	return true;
}

#pragma region GlobalVar
bool Parser::isGlobalVarContains(const QString& name)
{
	std::string sname = name.toUtf8().constData();
	sol::state& lua_ = pLua_->getLua();
	if (lua_[sname.c_str()].valid())
		return true;

	return false;
}

QVariant Parser::getGlobalVarValue(const QString& name)
{
	std::string sname = name.toUtf8().constData();
	sol::state& lua_ = pLua_->getLua();

	if (!lua_[sname.c_str()].valid())
		return QVariant("nil");

	QVariant var;
	sol::object obj = lua_[sname.c_str()];
	if (obj.is<bool>())
		var = obj.as<bool>();
	else if (obj.is<std::string>())
		var = util::toQString(obj);
	else if (obj.is<qint64>())
		var = obj.as<qint64>();
	else if (obj.is<double>())
		var = obj.as<double>();
	else if (obj.is<sol::table>())
	{
		qint64 deep = kMaxLuaTableDepth;
		var = getLuaTableString(obj.as<sol::table>(), deep);
	}
	else
		var = "nil";

	return var;
}

//表達式替換內容
void Parser::importVariablesToLua(const QString& expr)
{
	sol::state& lua_ = pLua_->getLua();

	//將局變量保存倒預備列表、每次執行表達式時都會將局變量列表轉換lua語句插入表達式前
	QVariantHash localVars = getLocalVars();
	luaLocalVarStringList.clear();
	QString key, strvalue;
	QVariant::Type type = QVariant::Invalid;
	bool ok = false;
	for (auto it = localVars.cbegin(); it != localVars.cend(); ++it)
	{
		key = it.key();
		if (key.isEmpty())
			continue;

		strvalue = it.value().toString();
		type = it.value().type();
		if (type == QVariant::String && !strvalue.startsWith("{") && !strvalue.endsWith("}"))
			strvalue = QString("'%1'").arg(strvalue);
		else if (strvalue.startsWith("{") && strvalue.endsWith("}"))
			strvalue = strvalue;
		else if (type == QVariant::Double)
			strvalue = util::toQString(it.value().toDouble());
		else if (type == QVariant::Bool)
			strvalue = util::toQString(it.value().toBool());
		else if (type == QVariant::Int || type == QVariant::LongLong)
			strvalue = util::toQString(it.value().toLongLong());
		else
			strvalue = "nil";

		strvalue = QString("local %1 = %2;").arg(key).arg(strvalue);
		luaLocalVarStringList.append(strvalue);
	}

	try
	{
		updateSysConstKeyword(expr);
	}
	catch (...)
	{

	}
}

//插入全局變量
void Parser::insertGlobalVar(const QString& name, const QVariant& value)
{
	const QStringList exceptionList = { "char", "pet", "item", "map", "magic", "skill", "petskill", "petequip", "dialog", "chat", "battle", "point", "team", "card", "unit" };
	if (exceptionList.contains(name))
		return;

	QVariant var = value;

	if (value.toString() == "nil")
	{
		removeGlobalVar(name);
		return;
	}

	if (!globalNames_.contains(name))
	{
		globalNames_.append(name);
		globalNames_.removeDuplicates();
		std::sort(globalNames_.begin(), globalNames_.end());
	}

	sol::state& lua_ = pLua_->getLua();

	QVariant::Type type = value.type();
	if (type != QVariant::String
		&& type != QVariant::Int && type != QVariant::LongLong
		&& type != QVariant::Double
		&& type != QVariant::Bool)
	{
		var = "nil";
	}

	//每次插入變量都順便更新lua變量
	QByteArray keyBytes = name.toUtf8();
	QString str = var.toString();
	if (type == QVariant::String)
	{
		if (!str.startsWith("{") && !str.endsWith("}"))
		{
			lua_.set(keyBytes.constData(), str.toUtf8().constData());
		}
		else//table
		{
			QString content = QString("%1 = %2;").arg(name).arg(str);
			std::string contentBytes = content.toUtf8().constData();
			sol::protected_function_result loaded_chunk = lua_.safe_script(contentBytes, sol::script_pass_on_error);
			lua_.collect_garbage();
			if (!loaded_chunk.valid())
			{
				sol::error err = loaded_chunk;
				QString errStr = util::toQString(err.what());
				handleError(kLuaError, errStr);
			}
		}
	}
	else if (type == QVariant::Double)
		lua_.set(keyBytes.constData(), var.toDouble());
	else if (type == QVariant::Bool)
		lua_.set(keyBytes.constData(), var.toBool());
	else if (type == QVariant::Int || type == QVariant::LongLong)
		lua_.set(keyBytes.constData(), var.toLongLong());
	else
		lua_[keyBytes.constData()] = sol::lua_nil;
}

void Parser::insertVar(const QString& name, const QVariant& value)
{
	if (isLocalVarContains(name))
		insertLocalVar(name, value);
	else
		insertGlobalVar(name, value);
}

void Parser::removeGlobalVar(const QString& name)
{
	sol::state& lua_ = pLua_->getLua();
	lua_[name.toUtf8().constData()] = sol::lua_nil;
	if (!globalNames_.contains(name))
		return;

	globalNames_.removeOne(name);
}
#pragma endregion

#pragma region LocalVar
//插入局變量
void Parser::insertLocalVar(const QString& name, const QVariant& value)
{
	if (name.isEmpty())
		return;

	QVariantHash& hash = getLocalVarsRef();

	if (!value.isValid())
		return;

	QVariant var = value;

	QVariant::Type type = value.type();

	if (type != QVariant::String
		&& type != QVariant::Int && type != QVariant::LongLong
		&& type != QVariant::Double
		&& type != QVariant::Bool)
	{
		hash.insert(name, "nil");
	}

	hash.insert(name, var);
}

bool Parser::isLocalVarContains(const QString& name)
{
	QVariantHash hash = getLocalVars();
	if (hash.contains(name))
		return true;

	return false;
}

QVariant Parser::getLocalVarValue(const QString& name)
{
	QVariantHash hash = getLocalVars();
	if (hash.contains(name))
		return hash.value(name);

	return QVariant("nil");
}

void Parser::removeLocalVar(const QString& name)
{
	QVariantHash& hash = getLocalVarsRef();
	if (hash.contains(name))
		hash.remove(name);
}

QVariantHash Parser::getLocalVars() const
{
	if (!localVarStack_.isEmpty())
		return localVarStack_.top();

	return emptyLocalVars_;
}

QVariantHash& Parser::getLocalVarsRef()
{
	if (!localVarStack_.isEmpty())
		return localVarStack_.top();

	localVarStack_.push(emptyLocalVars_);
	return localVarStack_.top();
}
#pragma endregion

qint64 Parser::matchLineFromLabel(const QString& label) const
{
	if (!labels_.contains(label))
		return -1;

	return labels_.value(label, -1);
}

qint64 Parser::matchLineFromFunction(const QString& funcName) const
{
	for (const FunctionNode& node : functionNodeList_)
	{
		if (node.name == funcName)
			return node.beginLine;
	}

	return -1;
}

FunctionNode Parser::getFunctionNodeByName(const QString& funcName) const
{
	for (const FunctionNode& node : functionNodeList_)
	{
		if (node.name == funcName)
			return node;
	}

	return FunctionNode{};
}

ForNode Parser::getForNodeByLineIndex(qint64 line) const
{
	for (const ForNode& node : forNodeList_)
	{
		if (node.beginLine == line)
			return node;
	}

	return ForNode{};
}

QVariantList& Parser::getArgsRef()
{
	if (!callArgsStack_.isEmpty())
		return callArgsStack_.top();
	else
		return emptyArgs_;
}

//根據表達式更新lua內的變量值
void Parser::updateSysConstKeyword(const QString& expr)
{
	bool bret = false;
	sol::state& lua_ = pLua_->getLua();
	if (expr.contains("_LINE_"))
	{
		lua_.set("_LINE_", getCurrentLine() + 1);
	}

	if (expr.contains("_FILE_"))
	{
		lua_.set("_FILE_", getScriptFileName().toUtf8().constData());
	}

	if (expr.contains("_FUNCTION_"))
	{
		if (!callStack_.isEmpty())
			lua_.set("_FUNCTION_", callStack_.top().name.toUtf8().constData());
		else
			lua_.set("_FUNCTION_", sol::lua_nil);
	}

	if (expr.contains("PID"))
	{
		lua_.set("PID", static_cast<qint64>(_getpid()));
	}

	if (expr.contains("THREADID"))
	{
		lua_.set("THREADID", reinterpret_cast<qint64>(QThread::currentThreadId()));
	}

	if (expr.contains("INFINITE"))
	{
		lua_.set("INFINITE", std::numeric_limits<qint64>::max());
	}
	if (expr.contains("MAXTHREAD"))
	{
		lua_.set("MAXTHREAD", SASH_MAX_THREAD);
	}
	if (expr.contains("MAXCHAR"))
	{
		lua_.set("MAXCHAR", MAX_CHARACTER);
	}
	if (expr.contains("MAXDIR"))
	{
		lua_.set("MAXDIR", MAX_DIR);
	}
	if (expr.contains("MAXITEM"))
	{
		lua_.set("MAXITEM", MAX_ITEM - CHAR_EQUIPPLACENUM);
	}
	if (expr.contains("MAXEQUIP"))
	{
		lua_.set("MAXEQUIP", CHAR_EQUIPPLACENUM);
	}
	if (expr.contains("MAXCARD"))
	{
		lua_.set("MAXCARD", MAX_ADDRESS_BOOK);
	}
	if (expr.contains("MAXMAGIC"))
	{
		lua_.set("MAXMAGIC", MAX_MAGIC);
	}
	if (expr.contains("MAXSKILL"))
	{
		lua_.set("MAXSKILL", MAX_PROFESSION_SKILL);
	}
	if (expr.contains("MAXPET"))
	{
		lua_.set("MAXPET", MAX_PET);
	}
	if (expr.contains("MAXPETSKILL"))
	{
		lua_.set("MAXPETSKILL", MAX_SKILL);
	}
	if (expr.contains("MAXCHAT"))
	{
		lua_.set("MAXCHAT", MAX_CHAT_HISTORY);
	}
	if (expr.contains("MAXDLG"))
	{
		lua_.set("MAXDLG", MAX_DIALOG_LINE);
	}
	if (expr.contains("MAXENEMY"))
	{
		lua_.set("MAXENEMY", MAX_ENEMY);
	}

	qint64 currentIndex = getIndex();
	if (lua_["_INDEX"].valid() && lua_["_INDEX"].is<qint64>())
	{
		qint64 tempIndex = lua_["_INDEX"].get<qint64>();

		if (tempIndex >= 0 && tempIndex < SASH_MAX_THREAD)
			currentIndex = tempIndex;
	}

	Injector& injector = Injector::getInstance(currentIndex);

	if (expr.contains("HWND"))
	{
		lua_.set("HWND", reinterpret_cast<qint64>(injector.getParentWidget()));
	}

	if (expr.contains("GAMEPID"))
	{
		lua_.set("GAMEPID", injector.getProcessId());
	}

	if (expr.contains("GAMEHWND"))
	{
		lua_.set("GAMEPID", reinterpret_cast<qint64>(injector.getProcessWindow()));
	}

	if (expr.contains("GAMEHANDLE"))
	{
		lua_.set("GAMEHANDLE", reinterpret_cast<qint64>(injector.getProcess()));
	}

	if (expr.contains("INDEX"))
	{
		lua_.set("INDEX", injector.getIndex());
	}


	/////////////////////////////////////////////////////////////////////////////////////
	if (injector.server.isNull())
		return;

	if (expr.contains("GAME"))
	{
		bret = true;
		lua_.set("GAME", injector.server->getGameStatus());
	}

	if (expr.contains("WORLD"))
	{
		bret = true;
		lua_.set("WORLD", injector.server->getWorldStatus());
	}

	if (expr.contains("isonline"))
	{
		lua_.set("isonline", injector.server->getOnlineFlag());
	}

	if (expr.contains("isbattle"))
	{
		lua_.set("isbattle", injector.server->getBattleFlag());
	}

	if (expr.contains("isnormal"))
	{
		lua_.set("isnormal", !injector.server->getBattleFlag());
	}

	if (expr.contains("isdialog"))
	{
		lua_.set("isdialog", injector.server->isDialogVisible());
	}

	//char\.(\w+)
	if (expr.contains("char."))
	{
		injector.server->updateDatasFromMemory();

		sol::meta::unqualified_t<sol::table> ch = lua_["char"];

		PC _pc = injector.server->getPC();

		ch["name"] = _pc.name.toUtf8().constData();

		ch["fname"] = _pc.freeName.toUtf8().constData();

		ch["lv"] = _pc.level;

		ch["hp"] = _pc.hp;

		ch["maxhp"] = _pc.maxHp;

		ch["hpp"] = _pc.hpPercent;

		ch["mp"] = _pc.mp;

		ch["maxmp"] = _pc.maxMp;

		ch["mpp"] = _pc.mpPercent;

		ch["exp"] = _pc.exp;

		ch["maxexp"] = _pc.maxExp;

		ch["stone"] = _pc.gold;

		ch["vit"] = _pc.vit;

		ch["str"] = _pc.str;

		ch["tgh"] = _pc.tgh;

		ch["dex"] = _pc.dex;

		ch["atk"] = _pc.atk;

		ch["def"] = _pc.def;

		ch["chasma"] = _pc.chasma;

		ch["turn"] = _pc.transmigration;

		ch["earth"] = _pc.earth;

		ch["water"] = _pc.water;

		ch["fire"] = _pc.fire;

		ch["wind"] = _pc.wind;

		ch["modelid"] = _pc.modelid;

		ch["faceid"] = _pc.faceid;

		ch["family"] = _pc.family.toUtf8().constData();

		ch["battlepet"] = _pc.battlePetNo + 1;

		ch["ridepet"] = _pc.ridePetNo + 1;

		ch["mailpet"] = _pc.mailPetNo + 1;

		ch["luck"] = _pc.luck;

		ch["point"] = _pc.point;
	}

	//pet\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("pet.") || expr.contains("pet["))
	{
		injector.server->updateDatasFromMemory();

		const QHash<QString, PetState> hash = {
			{ u8"battle", kBattle },
			{ u8"standby", kStandby },
			{ u8"mail", kMail },
			{ u8"rest", kRest },
			{ u8"ride", kRide },
		};

		sol::meta::unqualified_t<sol::table> pet = lua_["pet"];

		pet["count"] = injector.server->getPetSize();

		for (qint64 i = 0; i < MAX_PET; ++i)
		{
			PET p = injector.server->getPet(i);
			qint64 index = i + 1;

			pet[index]["valid"] = p.valid;

			pet[index]["name"] = p.name.toUtf8().constData();

			pet[index]["fname"] = p.freeName.toUtf8().constData();

			pet[index]["lv"] = p.level;

			pet[index]["hp"] = p.hp;

			pet[index]["maxhp"] = p.maxHp;

			pet[index]["hpp"] = p.hpPercent;

			pet[index]["exp"] = p.exp;

			pet[index]["maxexp"] = p.maxExp;

			pet[index]["atk"] = p.atk;

			pet[index]["def"] = p.def;

			pet[index]["agi"] = p.agi;

			pet[index]["loyal"] = p.loyal;

			pet[index]["turn"] = p.transmigration;

			pet[index]["earth"] = p.earth;

			pet[index]["water"] = p.water;

			pet[index]["fire"] = p.fire;

			pet[index]["wind"] = p.wind;

			pet[index]["modelid"] = p.modelid;

			pet[index]["pos"] = p.index;

			pet[index]["index"] = index;

			PetState state = p.state;
			QString str = hash.key(state, "");
			pet[index]["state"] = str.toUtf8().constData();

			pet[index]["power"] = p.power;
		}
	}

	//item\[(\d+)\]\.(\w+)
	if (expr.contains("item.") || expr.contains("item["))
	{
		injector.server->updateItemByMemory();

		PC _pc = injector.server->getPC();

		sol::meta::unqualified_t<sol::table> item = lua_["item"];

		for (qint64 i = 0; i < MAX_ITEM; ++i)
		{
			ITEM it = _pc.item[i];
			qint64 index = i + 1;
			if (i < CHAR_EQUIPPLACENUM)
				index += 100;
			else
				index = index - CHAR_EQUIPPLACENUM;

			item[index]["valid"] = it.valid;

			item[index]["index"] = index;

			item[index]["name"] = it.name.toUtf8().constData();

			item[index]["memo"] = it.memo.toUtf8().constData();

			if (it.name == "惡魔寶石" || it.name == "恶魔宝石")
			{
				static QRegularExpression rex("(\\d+)");
				QRegularExpressionMatch match = rex.match(it.memo);
				if (match.hasMatch())
				{
					QString str = match.captured(1);
					bool ok = false;
					qint64 dura = str.toLongLong(&ok);
					if (ok)
						item[index]["count"] = dura;
				}
			}

			QString damage = it.damage.simplified();
			if (damage.contains("%"))
				damage.replace("%", "");
			if (damage.contains("％"))
				damage.replace("％", "");

			bool ok = false;
			qint64 dura = damage.toLongLong(&ok);
			if (!ok && !damage.isEmpty())
				item[index]["dura"] = 100;
			else
				item[index]["dura"] = dura;

			item[index]["lv"] = it.level;

			if (it.valid && it.stack == 0)
				it.stack = 1;
			else if (!it.valid)
				it.stack = 0;

			item[index]["stack"] = it.stack;

			item[index]["lv"] = it.level;
			item[index]["field"] = it.field;
			item[index]["target"] = it.target;
			item[index]["type"] = it.type;
			item[index]["modelid"] = it.modelid;
			item[index]["name2"] = it.name2.toUtf8().constData();
		}


		QVector<qint64> itemIndexs;
		injector.server->getItemEmptySpotIndexs(&itemIndexs);

		item["space"] = itemIndexs.size();

		item["isfull"] = itemIndexs.size() == 0;

		auto getIndexs = [this, currentIndex](sol::object oitemnames, sol::object oitemmemos, bool includeEequip, sol::this_state s)->QVector<qint64>
		{
			QVector<qint64> itemIndexs;
			qint64 count = 0;
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return itemIndexs;

			QString itemnames;
			if (oitemnames.is<std::string>())
				itemnames = util::toQString(oitemnames);
			QString itemmemos;
			if (oitemmemos.is<std::string>())
				itemmemos = util::toQString(oitemmemos);

			if (itemnames.isEmpty() && itemmemos.isEmpty())
			{
				return itemIndexs;
			}

			if (!injector.server->getItemIndexsByName(itemnames, itemmemos, &itemIndexs))
			{
				return itemIndexs;
			}
			return itemIndexs;
		};


		item.set_function("count", [this, currentIndex, getIndexs](sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::this_state s)->qint64
			{
				insertGlobalVar("vret", 0);
				qint64 count = 0;
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.server.isNull())
					return count;

				bool includeEequip = true;
				if (oitemmemos.is<bool>())
					includeEequip = oincludeEequip.as<bool>();

				QVector<qint64> itemIndexs = getIndexs(oitemnames, oitemmemos, includeEequip, s);
				if (itemIndexs.isEmpty())
					return count;

				qint64 size = itemIndexs.size();
				PC pc = injector.server->getPC();
				for (qint64 i = 0; i < size; ++i)
				{
					qint64 itemIndex = itemIndexs.at(i);
					if (!includeEequip && i < CHAR_EQUIPPLACENUM)
						continue;

					ITEM item = pc.item[itemIndex];
					if (item.valid)
						count += item.stack;
				}

				insertGlobalVar("vret", count);
				return count;
			});

		item.set_function("indexof", [this, currentIndex, getIndexs](sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::this_state s)->qint64
			{
				insertGlobalVar("vret", -1);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.server.isNull())
					return -1;

				bool includeEequip = true;
				if (oincludeEequip.is<bool>())
					includeEequip = oincludeEequip.as<bool>();

				QVector<qint64> itemIndexs = getIndexs(oitemnames, oitemmemos, includeEequip, s);
				if (itemIndexs.isEmpty())
					return -1;

				qint64 index = itemIndexs.front();
				if (index < CHAR_EQUIPPLACENUM)
					index += 100LL;
				else
					index -= static_cast<qint64>(CHAR_EQUIPPLACENUM);

				++index;

				insertGlobalVar("vret", index);
				return index;
			});

		item.set_function("find", [this, currentIndex, getIndexs](sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::this_state s)->sol::object
			{
				insertGlobalVar("vret", -1);
				sol::state_view lua(s);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.server.isNull())
					return sol::lua_nil;

				bool includeEequip = true;
				if (oincludeEequip.is<bool>())
					includeEequip = oincludeEequip.as<bool>();

				QVector<qint64> itemIndexs = getIndexs(oitemnames, oitemmemos, includeEequip, s);
				if (itemIndexs.isEmpty())
					return sol::lua_nil;

				qint64 index = itemIndexs.front();
				if (index < CHAR_EQUIPPLACENUM)
					index += 100LL;
				else
					index -= static_cast<qint64>(CHAR_EQUIPPLACENUM);

				++index;

				if (index < 0 || index >= MAX_ITEM)
					return sol::lua_nil;

				ITEM item = injector.server->getPC().item[index];

				sol::table t = lua.create_table();
				t["valid"] = item.valid;
				t["index"] = index;
				t["name"] = item.name.toUtf8().constData();
				t["memo"] = item.memo.toUtf8().constData();
				t["dura"] = item.damage.toUtf8().constData();
				t["lv"] = item.level;
				t["stack"] = item.stack;
				t["field"] = item.field;
				t["target"] = item.target;
				t["type"] = item.type;
				t["modelid"] = item.modelid;
				t["name2"] = item.name2.toUtf8().constData();

				return t;
			});
	}

	//team\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("team.") || expr.contains("team["))
	{
		sol::meta::unqualified_t<sol::table> team = lua_["team"];

		team["count"] = static_cast<qint64>(injector.server->getPartySize());

		for (qint64 i = 0; i < MAX_PARTY; ++i)
		{
			PARTY party = injector.server->getParty(i);
			qint64 index = i + 1;

			team[index]["valid"] = party.valid;

			team[index]["index"] = index;

			team[index]["id"] = party.id;

			team[index]["name"] = party.name.toUtf8().constData();

			team[index]["lv"] = party.level;

			team[index]["hp"] = party.hp;

			team[index]["maxhp"] = party.maxHp;

			team[index]["hpp"] = party.hpPercent;

			team[index]["mp"] = party.mp;
		}
	}

	//map\.(\w+)
	if (expr.contains("map."))
	{
		sol::meta::unqualified_t<sol::table> map = lua_["map"];

		map["name"] = injector.server->getFloorName().toUtf8().constData();

		map["floor"] = injector.server->getFloor();

		map["x"] = injector.server->getPoint().x();

		map["y"] = injector.server->getPoint().y();

		if (expr.contains("ground"))
			map["ground"] = injector.server->getGround().toUtf8().constData();

		map.set_function("isxy", [this, currentIndex](qint64 x, qint64 y, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.server.isNull())
					return false;
				QPoint pos = injector.server->getPoint();
				return pos == QPoint(x, y);
			});

		map.set_function("isrect", [this, currentIndex](qint64 x1, qint64 y1, qint64 x2, qint64 y2, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.server.isNull())
					return false;
				QPoint pos = injector.server->getPoint();
				return pos.x() >= x1 && pos.x() <= x2 && pos.y() >= y1 && pos.y() <= y2;

			});

		map.set_function("ismap", [this, currentIndex](sol::object omap, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.server.isNull())
					return false;

				if (omap.is<qint64>())
				{
					return injector.server->getFloor() == omap.as<qint64>();
				}

				QString mapNames = util::toQString(omap);
				QStringList mapNameList = mapNames.split(util::rexOR, Qt::SkipEmptyParts);
				bool ok = false;
				bool isExact = true;
				qint64 floor = 0;
				QString newName;
				for (const QString& it : mapNameList)
				{
					floor = it.toLongLong(&ok);
					if (ok && injector.server->getFloor() == floor)
						return true;

					newName = it;
					if (newName.startsWith(kFuzzyPrefix))
					{
						newName = newName.mid(1);
						isExact = false;
					}

					if (newName.isEmpty())
						continue;

					if (isExact && injector.server->getFloorName() == newName)
						return true;
					else if (injector.server->getFloorName().contains(newName))
						return true;
				}

				return false;
			});
	}

	//card\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("card.") || expr.contains("card["))
	{
		sol::meta::unqualified_t<sol::table> card = lua_["card"];

		for (qint64 i = 0; i < MAX_ADDRESS_BOOK; ++i)
		{
			ADDRESS_BOOK addressBook = injector.server->getAddressBook(i);
			qint64 index = i + 1;

			card[index]["valid"] = addressBook.valid;

			card[index]["index"] = index;

			card[index]["name"] = addressBook.name.toUtf8().constData();

			card[index]["online"] = addressBook.onlineFlag;

			card[index]["turn"] = addressBook.transmigration;

			card[index]["dp"] = addressBook.dp;

			card[index]["lv"] = addressBook.level;
		}
	}

	//chat\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	if (expr.contains("chat.") || expr.contains("chat["))
	{
		sol::meta::unqualified_t<sol::table> chat = lua_["chat"];

		for (qint64 i = 0; i < MAX_CHAT_HISTORY; ++i)
		{
			QString text = injector.server->getChatHistory(i);
			qint64 index = i + 1;

			chat[index] = text.toUtf8().constData();
		}

		chat["contains"] = [this, currentIndex](std::string str, sol::this_state s)->bool
		{
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return false;

			if (str.empty())
				return false;

			QString text = util::toQString(str);
			for (qint64 i = 0; i < MAX_CHAT_HISTORY; ++i)
			{
				QString cmptext = injector.server->getChatHistory(i);
				if (cmptext.contains(text))
					return true;
			}

			return false;
		};
	}

	//unit\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("unit.") || expr.contains("unit["))
	{
		QList<mapunit_t> units = injector.server->mapUnitHash.values();

		qint64 size = units.size();

		sol::meta::unqualified_t<sol::table> unit = lua_["unit"];

		unit["count"] = size;

		for (qint64 i = 0; i < size; ++i)
		{
			mapunit_t u = units.at(i);
			if (!u.isVisible)
				continue;

			qint64 index = i + 1;

			unit[index]["valid"] = u.isVisible;

			unit[index]["index"] = index;

			unit[index]["id"] = u.id;

			unit[index]["name"] = u.name.toUtf8().constData();

			unit[index]["fname"] = u.freeName.toUtf8().constData();

			unit[index]["family"] = u.family.toUtf8().constData();

			unit[index]["lv"] = u.level;

			unit[index]["dir"] = u.dir;

			unit[index]["x"] = u.p.x();

			unit[index]["y"] = u.p.y();

			unit[index]["gold"] = u.gold;

			unit[index]["modelid"] = u.modelid;
		}

		unit["find"] = [this, currentIndex](sol::object name, sol::object ofname, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return 0;

			QString _name = "";
			qint64 modelid = 0;
			if (name.is<std::string>())
				_name = util::toQString(name);
			if (ofname.is<std::string>())
				_name = util::toQString(ofname);

			QString freeName = "";
			if (ofname.is<std::string>())
				freeName = util::toQString(ofname);

			if (_name.isEmpty() && modelid == 0 && freeName.isEmpty())
				return sol::lua_nil;

			mapunit_t _unit = {};
			if (!injector.server->findUnit(_name, util::OBJ_NPC, &_unit, freeName, modelid))
			{
				if (!injector.server->findUnit(_name, util::OBJ_HUMAN, &_unit, freeName, modelid))
					return sol::lua_nil;
			}

			sol::table t = lua.create_table();
			t["valid"] = _unit.isVisible;

			t["index"] = -1;

			t["id"] = _unit.id;

			t["name"] = _unit.name.toUtf8().constData();

			t["fname"] = _unit.freeName.toUtf8().constData();

			t["family"] = _unit.family.toUtf8().constData();

			t["lv"] = _unit.level;

			t["dir"] = _unit.dir;

			t["x"] = _unit.p.x();

			t["y"] = _unit.p.y();

			t["gold"] = _unit.gold;

			t["modelid"] = _unit.modelid;

			return t;
		};
	}

	//battle\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if ((expr.contains("battle.") || expr.contains("battle[")) && !expr.contains("isbattle") && injector.server->getBattleFlag())
	{
		sol::meta::unqualified_t<sol::table> battle = lua_["battle"];

		battledata_t bt = injector.server->getBattleData();

		battle["playerpos"] = static_cast<qint64>(bt.player.pos + 1);
		battle["petpos"] = static_cast<qint64>(bt.pet.pos + 1);

		qint64 size = bt.objects.size();
		battle["size"] = size;
		battle["enemycount"] = bt.enemies.size();

		battle["round"] = injector.server->battleCurrentRound.load(std::memory_order_acquire) + 1;

		battle["field"] = injector.server->getFieldString(bt.fieldAttr).toUtf8().constData();

		battle["dura"] = static_cast<qint64>(injector.server->battleDurationTimer.elapsed() / 1000ll);

		battle["totaldura"] = static_cast<qint64>(injector.server->battle_total_time.load(std::memory_order_acquire) / 1000 / 60);

		battle["totalcombat"] = injector.server->battle_total.load(std::memory_order_acquire);

		for (qint64 i = 0; i < size; ++i)
		{
			battleobject_t obj = bt.objects.at(i);
			qint64 index = i + 1;

			battle[index]["valid"] = obj.maxHp > 0 && obj.level > 0 && obj.modelid > 0;

			battle[index]["index"] = static_cast<qint64>(obj.pos + 1);

			battle[index]["name"] = obj.name.toUtf8().constData();

			battle[index]["fname"] = obj.freeName.toUtf8().constData();

			battle[index]["modelid"] = obj.modelid;

			battle[index]["lv"] = obj.level;

			battle[index]["hp"] = obj.hp;

			battle[index]["maxhp"] = obj.maxHp;

			battle[index]["hpp"] = obj.hpPercent;

			battle[index]["status"] = injector.server->getBadStatusString(obj.status).toUtf8().constData();

			battle[index]["ride"] = obj.rideFlag > 0;

			battle[index]["ridename"] = obj.rideName.toUtf8().constData();

			battle[index]["ridelv"] = obj.rideLevel;

			battle[index]["ridehp"] = obj.rideHp;

			battle[index]["ridemaxhp"] = obj.rideMaxHp;

			battle[index]["ridehpp"] = obj.rideHpPercent;
		}
	}

	//dialog\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	if (expr.contains("dialog.") || expr.contains("dialog["))
	{
		QStringList dialogstrs = injector.server->currentDialog.get().linedatas;

		sol::meta::unqualified_t<sol::table> dlg = lua_["dialog"];

		qint64 size = dialogstrs.size();

		bool visible = injector.server->isDialogVisible();

		for (qint64 i = 0; i < MAX_DIALOG_LINE; ++i)
		{
			QString text;
			if (i >= size || !visible)
				text = "";
			else
				text = dialogstrs.at(i);

			qint64 index = i + 1;

			dlg[index] = text.toUtf8().constData();
		}

		dialog_t dialog = injector.server->currentDialog.get();

		dlg["id"] = dialog.dialogid;
		dlg["unitid"] = dialog.unitid;
		dlg["type"] = dialog.windowtype;
		dlg["buttontext"] = dialog.linebuttontext.join("|").toUtf8().constData();
		dlg["button"] = dialog.buttontype;

		dlg["contains"] = [this, currentIndex](std::string str, sol::this_state s)->bool
		{
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return false;

			if (str.empty())
				return false;

			QString text = util::toQString(str);
			QStringList dialogstrs = injector.server->currentDialog.get().linedatas;
			for (qint64 i = 0; i < MAX_DIALOG_LINE; ++i)
			{
				QString cmptext = dialogstrs.at(i);
				if (cmptext.contains(text))
					return true;
			}

			return false;
		};
	}

	//magic\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("magic.") || expr.contains("magic["))
	{
		sol::meta::unqualified_t<sol::table> mg = lua_["magic"];

		for (qint64 i = 0; i < MAX_MAGIC; ++i)
		{
			qint64 index = i + 1;
			MAGIC magic = injector.server->getMagic(i);

			mg[index]["valid"] = magic.valid;
			mg[index]["index"] = index;
			mg[index]["costmp"] = magic.costmp;
			mg[index]["field"] = magic.field;
			mg[index]["name"] = magic.name.toUtf8().constData();
			mg[index]["memo"] = magic.memo.toUtf8().constData();
			mg[index]["target"] = magic.target;
		}

		mg["find"] = [this, currentIndex](sol::object oname, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return 0;

			QString name = "";
			if (oname.is<std::string>())
				name = util::toQString(oname);

			if (name.isEmpty())
				return sol::lua_nil;


			qint64 index = injector.server->getMagicIndexByName(name);
			if (index == -1)
				return sol::lua_nil;

			MAGIC _magic = injector.server->getMagic(index);

			sol::table t = lua.create_table();

			t["valid"] = _magic.valid;
			t["index"] = index;
			t["costmp"] = _magic.costmp;
			t["field"] = _magic.field;
			t["name"] = _magic.name.toUtf8().constData();
			t["memo"] = _magic.memo.toUtf8().constData();
			t["target"] = _magic.target;

			return t;
		};
	}

	//skill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("skill.") || expr.contains("skill["))
	{
		sol::meta::unqualified_t<sol::table> sk = lua_["skill"];

		for (qint64 i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			qint64 index = i + 1;
			PROFESSION_SKILL skill = injector.server->getSkill(i);

			sk[index]["valid"] = skill.valid;
			sk[index]["index"] = index;
			sk[index]["costmp"] = skill.costmp;
			sk[index]["modelid"] = skill.icon;
			sk[index]["type"] = skill.kind;
			sk[index]["lv"] = skill.skill_level;
			sk[index]["id"] = skill.skillId;
			sk[index]["name"] = skill.name.toUtf8().constData();
			sk[index]["memo"] = skill.memo.toUtf8().constData();
			sk[index]["target"] = skill.target;
		}

		sk["find"] = [this, currentIndex](sol::object oname, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return 0;

			QString name = "";
			if (oname.is<std::string>())
				name = util::toQString(oname);

			if (name.isEmpty())
				return sol::lua_nil;

			qint64 index = injector.server->getSkillIndexByName(name);
			if (index == -1)
				return sol::lua_nil;

			PROFESSION_SKILL _skill = injector.server->getSkill(index);

			sol::table t = lua.create_table();

			t["valid"] = _skill.valid;
			t["index"] = index;
			t["costmp"] = _skill.costmp;
			t["modelid"] = _skill.icon;
			t["type"] = _skill.kind;
			t["lv"] = _skill.skill_level;
			t["id"] = _skill.skillId;
			t["name"] = _skill.name.toUtf8().constData();
			t["memo"] = _skill.memo.toUtf8().constData();
			t["target"] = _skill.target;

			return t;
		};
	}

	//petskill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petksill.") || expr.contains("petksill["))
	{
		sol::meta::unqualified_t<sol::table> psk = lua_["petskill"];

		qint64 petIndex = -1;
		qint64 index = -1;
		qint64 i, j;
		for (i = 0; i < MAX_PET; ++i)
		{
			petIndex = i + 1;

			for (j = 0; j < MAX_SKILL; ++j)
			{
				index = j + 1;
				PET_SKILL skill = injector.server->getPetSkill(i, j);

				psk[petIndex][index]["valid"] = skill.valid;
				psk[petIndex][index]["index"] = index;
				psk[petIndex][index]["id"] = skill.skillId;
				psk[petIndex][index]["field"] = skill.field;
				psk[petIndex][index]["target"] = skill.target;
				psk[petIndex][index]["name"] = skill.name.toUtf8().constData();
				psk[petIndex][index]["memo"] = skill.memo.toUtf8().constData();
			}
		}

		psk["find"] = [this, currentIndex](qint64 petIndex, sol::object oname, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return 0;

			QString name = "";
			if (oname.is<std::string>())
				name = util::toQString(oname);

			if (name.isEmpty())
				return sol::lua_nil;

			qint64 index = injector.server->getPetSkillIndexByName(petIndex, name);
			if (index == -1)
				return sol::lua_nil;

			PET_SKILL _skill = injector.server->getPetSkill(petIndex, index);

			sol::table t = lua.create_table();

			t["valid"] = _skill.valid;
			t["index"] = index;
			t["id"] = _skill.skillId;
			t["field"] = _skill.field;
			t["target"] = _skill.target;
			t["name"] = _skill.name.toUtf8().constData();
			t["memo"] = _skill.memo.toUtf8().constData();

			return t;
		};
	}

	//petequip\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petequip.") || expr.contains("petequip["))
	{
		sol::meta::unqualified_t<sol::table> peq = lua_["petequip"];

		qint64 petIndex = -1;
		qint64 index = -1;
		qint64 i, j;
		for (i = 0; i < MAX_PET; ++i)
		{
			petIndex = i + 1;

			for (j = 0; j < MAX_PET_ITEM; ++j)
			{
				index = j + 1;

				ITEM item = injector.server->getPetEquip(i, j);

				QString damage = item.damage;
				damage = damage.replace("%", "");
				damage = damage.replace("％", "");
				bool ok = false;
				qint64 damageValue = damage.toLongLong(&ok);
				if (!ok)
					damageValue = 100;

				peq[petIndex][index]["valid"] = item.valid;
				peq[petIndex][index]["index"] = index;
				peq[petIndex][index]["lv"] = item.level;
				peq[petIndex][index]["field"] = item.field;
				peq[petIndex][index]["target"] = item.target;
				peq[petIndex][index]["type"] = item.type;
				peq[petIndex][index]["modelid"] = item.modelid;
				peq[petIndex][index]["dura"] = damageValue;
				peq[petIndex][index]["name"] = item.name.toUtf8().constData();
				peq[petIndex][index]["name2"] = item.name2.toUtf8().constData();
				peq[petIndex][index]["memo"] = item.memo.toUtf8().constData();
			}
		}
	}

	if (expr.contains("point."))
	{
		currencydata_t point = injector.server->currencyData.get();

		sol::meta::unqualified_t<sol::table> pt = lua_["point"];

		pt["exp"] = point.expbufftime;
		pt["rep"] = point.prestige;
		pt["ene"] = point.energy;
		pt["shl"] = point.shell;
		pt["vit"] = point.vitality;
		pt["pts"] = point.points;
		pt["vip"] = point.VIPPoints;
	}
}

//行跳轉
bool Parser::jump(qint64 line, bool noStack)
{
	qint64 currentLine = getCurrentLine();
	if (!noStack)
		jmpStack_.push(currentLine + 1);
	currentLine += line;
	setCurrentLine(currentLine);
	return true;
}

//指定行跳轉
void Parser::jumpto(qint64 line, bool noStack)
{
	qint64 currentLine = getCurrentLine();
	if (line - 1 < 0)
		line = 1;
	if (!noStack)
		jmpStack_.push(currentLine + 1);
	currentLine = line - 1;
	setCurrentLine(currentLine);
}

//標記跳轉
bool Parser::jump(const QString& name, bool noStack)
{
	QString newName = name.toLower();
	if (newName == "back" || newName == QString(u8"跳回"))
	{
		if (!jmpStack_.isEmpty())
		{
			qint64 returnIndex = jmpStack_.pop();//jump行號出棧
			qint64 jumpLineCount = returnIndex - getCurrentLine();

			return jump(jumpLineCount, true);
		}
		return false;
	}
	else if (newName == "return" || newName == QString(u8"返回"))
	{
		return processReturn(3);//從第三個參數開始取返回值
	}
	else if (newName == "continue" || newName == QString(u8"繼續") || newName == QString(u8"继续"))
	{
		return processContinue();
	}
	else if (newName == "break" || newName == QString(u8"跳出"))
	{
		return processBreak();
	}

	//從跳轉位調用函數
	qint64 jumpLine = matchLineFromFunction(name);
	if (jumpLine != -1)
	{
		FunctionNode node = getFunctionNodeByName(name);
		node.callFromLine = getCurrentLine();
		checkCallArgs(1);
		jumpto(jumpLine + 1, true);
		callStack_.push(node);
		skipFunctionChunkDisable_ = true;
		return true;
	}

	//跳轉標記檢查
	jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		handleError(kLabelError, QString("'%1'").arg(name));
		return false;
	}

	qint64 currentLine = getCurrentLine();
	if (!noStack)
		jmpStack_.push(currentLine + 1);

	qint64 jumpLineCount = jumpLine - currentLine;

	return jump(jumpLineCount, true);
}

//解析跳轉棧或調用棧相關訊息並發送信號
void Parser::generateStackInfo(qint64 type)
{
	if (mode_ != kSync)
		return;

	if (type != 0)
		return;
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		return;

	QList<FunctionNode> list = callStack_.toList();
	QVector<QPair<int, QString>> hash;
	if (!list.isEmpty())
	{
		for (const FunctionNode it : list)
		{

			QStringList l;
			QString cmd = QString("name: %1, form: %2, level:%3").arg(it.name).arg(it.callFromLine).arg(it.level);

			hash.append(qMakePair(it.beginLine, cmd));
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (type == kModeCallStack)
		emit signalDispatcher.callStackInfoChanged(QVariant::fromValue(hash));
	else
		emit signalDispatcher.jumpStackInfoChanged(QVariant::fromValue(hash));
}

QString Parser::getLuaTableString(const sol::table& t, qint64& depth)
{
	if (depth <= 0)
		return "{}";

	if (t.size() == 0)
		return "{}";

	--depth;

	QString ret = "{";

	QStringList results;
	QStringList strKeyResults;

	QString nowIndent = "";
	for (qint64 i = 0; i <= 10 - depth + 1; ++i)
	{
		nowIndent += "    ";
	}

	for (const auto& pair : t)
	{
		qint64 nKey = 0;
		QString key = "";
		QString value = "";

		if (pair.first.is<qint64>())
		{
			nKey = pair.first.as<qint64>() - 1;
		}
		else
			key = util::toQString(pair.first);

		if (pair.second.is<sol::table>())
			value = getLuaTableString(pair.second.as<sol::table>(), depth);
		else if (pair.second.is<std::string>())
			value = QString("'%1'").arg(util::toQString(pair.second));
		else if (pair.second.is<qint64>())
			value = util::toQString(pair.second.as<qint64>());
		else if (pair.second.is<double>())
			value = util::toQString(pair.second.as<double>());
		else if (pair.second.is<bool>())
			value = util::toQString(pair.second.as<bool>());
		else
			value = "nil";

		if (value.isEmpty())
			value = "''";

		if (key.isEmpty())
		{
			if (nKey >= results.size())
			{
				for (qint64 i = results.size(); i <= nKey; ++i)
					results.append("nil");
			}

			results[nKey] = nowIndent + value;
			//strKeyResults.append(nowIndent + QString("[%1] = %2").arg(nKey + 1).arg(value));
		}
		else
			strKeyResults.append(nowIndent + QString("%1=%2").arg(key).arg(value));
	}

	std::sort(strKeyResults.begin(), strKeyResults.end(), [](const QString& a, const QString& b)
		{
			static const QLocale locale;
			static const QCollator collator(locale);
			return collator.compare(a, b) < 0;
		});

	results.append(strKeyResults);

	ret += results.join(",");
	ret += "}";

	return ret.simplified();
}

//處理"功能"，檢查是否有聲明局變量 這裡要注意只能執行一次否則會重複壓棧
void Parser::processFunction()
{
	QString functionName = currentLineTokens_.value(1).data.toString();

	//避免因錯誤跳轉到這裡後又重複定義
	if (!callStack_.isEmpty() && callStack_.top().name != functionName)
	{
		return;
	}

	//檢查是否有強制類型
	QVariantList args = getArgsRef();
	QVariantHash labelVars;
	const QStringList typeList = { "int", "double", "bool", "string", "table" };

	for (qint64 i = kCallPlaceHoldSize; i < currentLineTokens_.size(); ++i)
	{
		Token token = currentLineTokens_.value(i);
		if (token.type == TK_FUNCTIONARG)
		{
			QString labelName = token.data.toString().simplified();
			if (labelName.isEmpty())
				continue;

			if (i - kCallPlaceHoldSize >= args.size())
				break;

			QVariant currnetVar = args.at(i - kCallPlaceHoldSize);
			QVariant::Type type = currnetVar.type();

			if (!args.isEmpty() && (args.size() > (i - kCallPlaceHoldSize)) && (currnetVar.isValid()))
			{
				if (labelName.contains("int ", Qt::CaseInsensitive)
					&& (type == QVariant::Type::Int || type == QVariant::Type::LongLong))
				{
					labelVars.insert(labelName.replace("int ", "", Qt::CaseInsensitive), currnetVar.toLongLong());
				}
				else if (labelName.contains("double ", Qt::CaseInsensitive) && (type == QVariant::Type::Double))
				{
					labelVars.insert(labelName.replace("double ", "", Qt::CaseInsensitive), currnetVar.toDouble());
				}
				else if (labelName.contains("string ", Qt::CaseInsensitive) && type == QVariant::Type::String)
				{
					labelVars.insert(labelName.replace("string ", "", Qt::CaseInsensitive), currnetVar.toString());
				}
				else if (labelName.contains("table ", Qt::CaseInsensitive)
					&& type == QVariant::Type::String
					&& currnetVar.toString().startsWith("{")
					&& currnetVar.toString().endsWith("}"))
				{
					labelVars.insert(labelName.replace("table ", "", Qt::CaseInsensitive), currnetVar.toString());
				}
				else if (labelName.contains("bool ", Qt::CaseInsensitive)
					&& (type == QVariant::Type::Int || type == QVariant::Type::LongLong || type == QVariant::Type::Bool))
				{
					labelVars.insert(labelName.replace("bool ", "", Qt::CaseInsensitive), currnetVar.toBool());
				}
				else
				{
					QString expectedType = labelName;
					for (const QString& typeStr : typeList)
					{
						if (!expectedType.startsWith(typeStr))
							continue;

						expectedType = typeStr;
						labelVars.insert(labelName, "nil");
						handleError(kArgError + i - kCallPlaceHoldSize + 1, QString(QObject::tr("@ %1 | Invalid local variable type expacted '%2' but got '%3'")).arg(getCurrentLine()).arg(expectedType).arg(currnetVar.typeName()));
						break;
					}

					if (expectedType == labelName)
						labelVars.insert(labelName, currnetVar);
				}
			}
		}
	}

	currentField.push(TK_FUNCTION);//作用域標誌入棧
	localVarStack_.push(labelVars);//聲明局變量入棧
}

//處理"標記"
void Parser::processLabel()
{

}

//處理"結束"
void Parser::processClean()
{
	callStack_.clear();
	jmpStack_.clear();
	callArgsStack_.clear();
	localVarStack_.clear();
}

//處理所有核心命令之外的所有命令
qint64 Parser::processCommand()
{
	TokenMap tokens = getCurrentTokens();
	Token commandToken = tokens.value(0);
	QVariant varValue = commandToken.data;
	if (!varValue.isValid())
	{
		return kError;
	}

	QString commandName = commandToken.data.toString();
	qint64 status = kNoChange;

	if (commandRegistry_.contains(commandName))
	{
		CommandRegistry function = commandRegistry_.value(commandName, nullptr);
		if (function == nullptr)
		{
			return kError;
		}

		qint64 currentIndex = getIndex();
		status = function(currentIndex, getCurrentLine(), tokens);
	}
	else
	{
		return kError;
	}

	return status;
}

//處理單個或多個局變量聲明
void Parser::processLocalVariable()
{
	QString varNameStr = getToken<QString>(0);
	if (varNameStr.isEmpty())
		return;

	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	if (varNames.isEmpty())
		return;

	qint64 varCount = varNames.count();
	if (varCount == 0)
		return;

	//取區域變量表
	QVariantHash args = getLocalVars();

	QVariant firstValue;
	QString preExpr = getToken<QString>(1);
	if (preExpr.contains("input("))
	{
		exprTo(preExpr, &firstValue);
		if (firstValue.toLongLong() == 987654321ll)
		{
			requestInterruption();
			return;
		}

		if (!firstValue.isValid())
			firstValue = 0;
	}
	else
		firstValue = checkValue(currentLineTokens_, 1);

	if (varCount == 1)
	{
		insertLocalVar(varNames.front(), firstValue);
		return;
	}

	for (qint64 i = 0; i < varCount; ++i)
	{
		QString varName = varNames.at(i);
		if (varName.isEmpty())
		{
			continue;
		}

		if (i + 1 >= currentLineTokens_.size())
		{
			insertLocalVar(varName, firstValue);
			continue;
		}

		QVariant varValue = checkValue(currentLineTokens_, i + 1);
		if (i > 0 && !varValue.isValid())
		{
			varValue = firstValue;
		}

		if (varValue.type() == QVariant::String && currentLineTokens_.value(i + 1).type != TK_CSTRING)
		{
			QVariant varValue2 = varValue;
			QString expr = varValue.toString();
			if (exprTo(expr, &varValue2) && varValue2.isValid())
				varValue = varValue2.toString();
		}

		insertLocalVar(varName, varValue);
		firstValue = varValue;
	}
}

//處理單個或多個全局變量聲明，或單個局變量重新賦值
void Parser::processVariable()
{
	QString varNameStr = getToken<QString>(0);
	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	qint64 varCount = varNames.count();
	qint64 value = 0;

	QVariant firstValue;

	QString preExpr = getToken<QString>(1);
	static const QRegularExpression rexCallFun(R"([\w\p{Han}]+\((.+)\))");
	if (preExpr.contains("input("))
	{
		exprTo(preExpr, &firstValue);
		if (firstValue.toLongLong() == 987654321ll)
		{
			requestInterruption();
			return;
		}

		if (!firstValue.isValid())
			firstValue = 0;
	}
	else if (preExpr.contains(rexCallFun))
	{
		QString expr = preExpr;
		exprTo(expr, &firstValue);
		if (!firstValue.isValid())
			firstValue = 0;
	}
	else
		firstValue = checkValue(currentLineTokens_, 1);

	if (varCount == 1)
	{
		//只有一個有可能是局變量改值。使用綜合的insert
		insertVar(varNames.front(), firstValue);
		return;
	}

	//下面是多個變量聲明和初始化必定是全局
	for (qint64 i = 0; i < varCount; ++i)
	{
		QString varName = varNames.at(i);
		if (varName.isEmpty())
		{
			continue;
		}

		if (i + 1 >= currentLineTokens_.size())
		{
			insertGlobalVar(varName, firstValue);
			continue;
		}

		QVariant varValue;
		preExpr = getToken<QString>(i + 1);
		if (preExpr.contains("input("))
		{
			exprTo(preExpr, &varValue);
			if (firstValue.toLongLong() == 987654321ll)
			{
				requestInterruption();
				return;
			}

			if (!varValue.isValid())
				varValue = 0;
		}
		else
			varValue = checkValue(currentLineTokens_, i + 1);

		if (i > 0 && !varValue.isValid())
		{
			varValue = firstValue;
		}

		if (varValue.type() == QVariant::String && currentLineTokens_.value(i + 1).type != TK_CSTRING)
		{
			QVariant varValue2 = varValue;
			QString expr = varValue.toString();

			if (exprTo(expr, &varValue2) && varValue2.isValid())
				varValue = varValue2.toLongLong();
		}

		insertGlobalVar(varName, varValue);
		firstValue = varValue;
	}
}

//處理數組(表)
void Parser::processTable()
{
	QString varName = getToken<QString>(0);
	QString expr = getToken<QString>(1);
	RESERVE type = getTokenType(1);
	if (type == TK_LOCALTABLE)
		insertLocalVar(varName, expr);
	else
		insertGlobalVar(varName, expr);
}

//處理表元素設值
void Parser::processTableSet(const QString& preVarName, const QVariant& value)
{
	static const QRegularExpression rexTableSet(R"(([\w\p{Han}]+)(?:\['*"*(\w+\p{Han}*)'*"*\])?)");
	QString varName = preVarName;
	QString expr;
	sol::state& lua_ = pLua_->getLua();

	if (varName.isEmpty())
		varName = getToken<QString>(0);
	else
	{
		QRegularExpressionMatch match = rexTableSet.match(varName);
		if (match.hasMatch())
		{
			varName = match.captured(1).simplified();
			if (!value.isValid())
				return;

			expr = QString("%1[%2] = %3").arg(varName).arg(match.captured(2)).arg(value.toString());
		}
		else
			return;
	}

	if (expr.isEmpty())
		expr = getToken<QString>(1);
	if (expr.isEmpty())
		return;

	importVariablesToLua(expr);
	checkConditionalOp(expr);

	RESERVE type = getTokenType(0);

	if (isLocalVarContains(varName))
	{
		QString localVar = getLocalVarValue(varName).toString();
		if (!localVar.startsWith("{") || !localVar.endsWith("}"))
			return;

		QString exprStr = QString("%1\n%2;\nreturn %3;").arg(luaLocalVarStringList.join("\n")).arg(expr).arg(varName);
		//insertGlobalVar("_LUAEXPR", QString("[[%1]]").arg(exprStr));

		const std::string exprStrUTF8 = exprStr.toUtf8().constData();
		sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8, sol::script_pass_on_error);
		lua_.collect_garbage();
		if (!loaded_chunk.valid())
		{
			sol::error err = loaded_chunk;
			QString errStr = util::toQString(err.what());
			//insertGlobalVar(QString("_LUAERR_AT_%1").arg(getCurrentLine()), QString("[[%1]]").arg(exprStr));
			handleError(kLuaError, errStr);
			return;
		}

		sol::object retObject;
		try
		{
			retObject = loaded_chunk;
		}
		catch (...)
		{
			return;
		}

		if (!retObject.is<sol::table>())
			return;

		qint64 depth = kMaxLuaTableDepth;
		QString resultStr = getLuaTableString(retObject.as<sol::table>(), depth);
		if (resultStr.isEmpty())
			return;

		insertLocalVar(varName, resultStr);
	}
	else
	{
		QString exprStr = QString("%1\n%2;\nreturn %3;").arg(luaLocalVarStringList.join("\n")).arg(expr).arg(varName);
		//insertGlobalVar("_LUAEXPR", QString("[[%1]]").arg(exprStr));

		const std::string exprStrUTF8 = exprStr.toUtf8().constData();
		sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8, sol::script_pass_on_error);
		lua_.collect_garbage();
		if (!loaded_chunk.valid())
		{
			sol::error err = loaded_chunk;
			QString errStr = util::toQString(err.what());
			//insertGlobalVar(QString("_LUAERR_AT_%1").arg(getCurrentLine()), QString("[[%1]]").arg(exprStr));
			handleError(kLuaError, errStr);
			return;
		}
	}
}

//處理變量自增自減
void Parser::processVariableIncDec()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())
		return;

	RESERVE op = getTokenType(1);
	QVariantHash args = getLocalVars();

	qint64 value = 0;
	double dvalue = 0;
	if (args.contains(varName))
	{

		QVariant var = args.value(varName);
		QVariant::Type type = var.type();
		if (type == QVariant::LongLong)
		{
			value = args.value(varName, 0ll).toLongLong();
			value += (op == TK_INC ? 1 : -1);
			insertLocalVar(varName, value);
		}
		else if (type == QVariant::Double)
		{
			dvalue = args.value(varName, 0.0).toDouble();
			dvalue += (op == TK_INC ? 1.0 : -1.0);
			insertLocalVar(varName, dvalue);
		}
		else
			return;
	}

	std::string name = varName.toUtf8().constData();

	sol::state& lua_ = pLua_->getLua();

	if (!isGlobalVarContains(varName) && (!lua_[name.c_str()].is<qint64>() && !lua_[name.c_str()].is<double>()))
		return;

	if (lua_[name.c_str()].is<qint64>())
	{
		value = lua_[name.c_str()].get<qint64>();
		value += (op == TK_INC ? 1 : -1);
		insertGlobalVar(varName, value);
	}
	else if (lua_[name.c_str()].is<double>())
	{
		dvalue = lua_[name.c_str()].get<double>();
		dvalue += (op == TK_INC ? 1.0 : -1.0);
		insertGlobalVar(varName, dvalue);
	}
}

//處理變量計算
void Parser::processVariableCAOs()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())

		return;

	QVariant varFirst = checkValue(currentLineTokens_, 0);
	QVariant::Type typeFirst = varFirst.type();

	RESERVE op = getTokenType(1);

	QVariant varSecond = checkValue(currentLineTokens_, 2);
	QVariant::Type typeSecond = varSecond.type();

	sol::state& lua_ = pLua_->getLua();

	if (typeFirst == QVariant::String && varFirst.toString() != "nil" && varSecond.toString() != "nil")
	{
		if (typeSecond == QVariant::String && op == TK_ADD)
		{
			QString strA = varFirst.toString();
			QString strB = varSecond.toString();
			if (strB.startsWith("'") || strB.startsWith("\""))
				strB = strB.mid(1);
			if (strB.endsWith("'") || strB.endsWith("\""))
				strB = strB.left(strB.length() - 1);

			QString exprStr = QString("return '%1' .. '%2';").arg(strA).arg(strB);
			//insertGlobalVar("_LUAEXPR", QString("[[%1]]").arg(exprStr));

			importVariablesToLua(exprStr);
			checkConditionalOp(exprStr);

			const std::string exprStrUTF8 = exprStr.toUtf8().constData();
			sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8, sol::script_pass_on_error);
			lua_.collect_garbage();
			if (!loaded_chunk.valid())
			{
				sol::error err = loaded_chunk;
				QString errStr = util::toQString(err.what());
				//insertGlobalVar(QString("_LUAERR_AT_%1").arg(getCurrentLine()), QString("[[%1]]").arg(exprStr));
				handleError(kLuaError, errStr);
				return;
			}

			sol::object retObject;
			try
			{
				retObject = loaded_chunk;
			}
			catch (...)
			{
				return;
			}

			if (retObject.is<std::string>())
			{
				insertVar(varName, util::toQString(retObject));
			}
		}
		return;
	}

	if (typeFirst != QVariant::Int && typeFirst != QVariant::LongLong && typeFirst != QVariant::Double && typeFirst != QVariant::Bool)
		return;

	QString opStr = getToken<QString>(1).simplified();
	opStr.replace("=", "");

	double dvalue = 0;
	QString expr = "";

	expr = QString("%1 %2").arg(opStr).arg(util::toQString(varSecond.toDouble()));
	if (!exprCAOSTo(varName, expr, &dvalue))
		return;

	insertVar(varName, dvalue);
}

//處理變量表達式
void Parser::processVariableExpr()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())
		return;

	QVariant varValue = checkValue(currentLineTokens_, 0);

	QString expr;
	if (!checkString(currentLineTokens_, 1, &expr))
		return;

	QVariant::Type type = varValue.type();

	QVariant result;
	if ((type == QVariant::Int || type == QVariant::LongLong) && !expr.contains("/"))
	{
		qint64 value = 0;
		if (!exprTo(expr, &value))
			return;
		result = value;
	}
	else if (type == QVariant::Double || expr.contains("/"))
	{
		double value = 0;
		if (!exprTo(expr, &value))
			return;
		result = value;
	}
	else if (type == QVariant::Bool)
	{
		bool value = false;
		if (!exprTo(expr, &value))
			return;
		result = value;
	}
	else
	{
		result = expr;
	}

	insertVar(varName, result);
}

//處理if
bool Parser::processIfCompare()
{
	/*
	if (expr) then
		return true;
	else
		return false;
	end
	*/
	bool result = false;
	QString exprStr = QString("if (%1) then return true; else return false; end").arg(getToken<QString>(1).simplified());
	insertGlobalVar("_IFEXPR", exprStr);

	QVariant value = luaDoString(exprStr);

	insertGlobalVar("_IFRESULT", util::toQString(value.toBool()));

	return checkJump(currentLineTokens_, 2, value.toBool(), SuccessJump) == kHasJump;
}

//處理"格式化"
void Parser::processFormation()
{
	do
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		QString formatStr = checkValue(currentLineTokens_, 2).toString();

		QVariant var = luaDoString(QString("return format(tostring('%1'));").arg(formatStr));

		QString formatedStr = var.toString();

		static const QRegularExpression rexOut(R"(\[(\d+)\])");
		if ((varName.startsWith("out", Qt::CaseInsensitive) && varName.contains(rexOut)) || varName.toLower() == "out")
		{
			QRegularExpressionMatch match = rexOut.match(varName);
			qint64 color = QRandomGenerator::global()->bounded(0, 10);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				qint64 nColor = str.toLongLong();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			qint64 currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			if (!injector.server.isNull())
				injector.server->announce(formatedStr, color);

			const QDateTime time(QDateTime::currentDateTime());
			const QString timeStr(time.toString("hh:mm:ss:zzz"));
			QString src = "\0";

			QString msg = (QString("[%1 | @%2]: %3\0") \
				.arg(timeStr)
				.arg(getCurrentLine() + 1, 3, 10, QLatin1Char(' ')).arg(formatedStr));

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
			emit signalDispatcher.appendScriptLog(msg, color);
		}
		else if ((varName.startsWith("say", Qt::CaseInsensitive) && varName.contains(rexOut)) || varName.toLower() == "say")
		{
			QRegularExpressionMatch match = rexOut.match(varName);
			qint64 color = QRandomGenerator::global()->bounded(0, 10);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				qint64 nColor = str.toLongLong();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			qint64 currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			if (!injector.server.isNull())
				injector.server->talk(formatedStr, color);
		}
		else
		{
			if (varName.contains("[") && varName.contains("]"))
				processTableSet(varName, QVariant::fromValue(formatedStr));
			else
				insertVar(varName, QVariant::fromValue(formatedStr));
		}
	} while (false);
}

//處理"調用"
bool Parser::processCall(RESERVE reserve)
{
	RESERVE type = getTokenType(1);
	do
	{
		QString functionName;
		if (reserve == TK_CALL)//call xxxx
			checkString(currentLineTokens_, 1, &functionName);
		else //xxxx()
			functionName = getToken<QString>(1);

		if (functionName.isEmpty())
			break;

		qint64 jumpLine = matchLineFromFunction(functionName);
		if (jumpLine == -1)
		{
			QString expr = getToken<QString>(100);
			if (!expr.isEmpty())
			{
				QVariant var = luaDoString("return " + expr);
				insertGlobalVar("vret", var);
			}
			break;
		}

		FunctionNode node = getFunctionNodeByName(functionName);
		++node.callCount;
		node.callFromLine = getCurrentLine();
		checkCallArgs(0);
		jumpto(jumpLine + 1, true);
		callStack_.push(node);

		return true;

	} while (false);
	return false;
}

//處理"跳轉"
bool Parser::processGoto()
{
	RESERVE type = getTokenType(1);
	do
	{
		QVariant var = checkValue(currentLineTokens_, 1);
		QVariant::Type type = var.type();
		if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double || type == QVariant::Bool)
		{
			qint64 jumpLineCount = var.toLongLong();
			if (jumpLineCount == 0)
				break;

			jump(jumpLineCount, false);
			return true;
		}
		else if (type == QVariant::String && var.toString() != "nil")
		{
			QString labelName = var.toString();
			if (labelName.isEmpty())
				break;

			jump(labelName, false);
			return true;
		}

	} while (false);
	return false;
}

//處理"指定行跳轉"
bool Parser::processJump()
{
	QVariant var = checkValue(currentLineTokens_, 1);
	if (var.toString() == "nil")
		return false;

	qint64 line = var.toLongLong();
	if (line <= 0)
		return false;

	jumpto(line, false);
	return true;
}

//處理"返回"
bool Parser::processReturn(qint64 takeReturnFrom)
{
	QVariantList list;
	for (qint64 i = takeReturnFrom; i < currentLineTokens_.size(); ++i)
		list.append(checkValue(currentLineTokens_, i));

	qint64 size = list.size();

	lastReturnValue_ = list;
	if (size == 1)
		insertGlobalVar("vret", list.at(0));
	else if (size > 1)
	{
		QString str = "{";
		QStringList v;

		for (const QVariant& var : list)
		{
			QString s = var.toString();
			if (s.isEmpty())
				continue;

			if (var.type() == QVariant::String && s != "nil")
				s = "'" + s + "'";

			v.append(s);
		}

		str += v.join(", ");
		str += "}";

		insertGlobalVar("vret", str);
	}
	else
		insertGlobalVar("vret", "nil");

	if (!callArgsStack_.isEmpty())
		callArgsStack_.pop();//callArgNames出棧

	if (!localVarStack_.isEmpty())
		localVarStack_.pop();//callALocalArgs出棧

	if (!currentField.isEmpty())
	{
		//清空所有forNode
		for (;;)
		{
			if (currentField.isEmpty())
				break;

			auto field = currentField.pop();
			if (field == TK_FOR && !forStack_.isEmpty())
				forStack_.pop();

			//只以fucntino作為主要作用域
			else if (field == TK_FUNCTION)
				break;
		}
	}

	if (callStack_.isEmpty())
	{
		return false;
	}

	FunctionNode node = callStack_.pop();
	qint64 returnIndex = node.callFromLine + 1;
	qint64 jumpLineCount = returnIndex - getCurrentLine();
	jump(jumpLineCount, true);
	return true;
}

//處理"返回跳轉"
void Parser::processBack()
{
	if (jmpStack_.isEmpty())
	{
		setCurrentLine(0);
		return;
	}

	qint64 returnIndex = jmpStack_.pop();//jump行號出棧
	qint64 jumpLineCount = returnIndex - getCurrentLine();
	jump(jumpLineCount, true);
}

//這裡是防止人為設置過長的延時導致腳本無法停止
void Parser::processDelay()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	qint64 extraDelay = injector.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000ll)
	{
		//將超過1秒的延時分段
		qint64 i = 0ll;
		qint64 size = extraDelay / 1000ll;
		for (i = 0; i < size; ++i)
		{
			if (isInterruptionRequested())
				return;
			QThread::msleep(1000L);
		}
		if (extraDelay % 1000ll > 0ll)
			QThread::msleep(extraDelay % 1000ll);
	}
	else if (extraDelay > 0ll)
	{
		QThread::msleep(extraDelay);
	}
}

//處理"遍歷"
bool Parser::processFor()
{
	//init var value
	QVariant initValue = checkValue(currentLineTokens_, 2);
	if (initValue.type() != QVariant::LongLong)
	{
		initValue = QVariant("nil");
	}

	qint64 nBeginValue = initValue.toLongLong();

	//break condition
	QVariant breakValue = checkValue(currentLineTokens_, 3);
	if (breakValue.type() != QVariant::LongLong)
	{
		breakValue = QVariant("nil");
	}

	qint64 nEndValue = breakValue.toLongLong();

	//step var value
	QVariant stepValue = checkValue(currentLineTokens_, 4);
	if (stepValue.type() != QVariant::LongLong || stepValue.toLongLong() == 0)
		stepValue = 1ll;

	qint64 nStepValue = stepValue.toLongLong();

	qint64 currentLine = getCurrentLine();
	if (forStack_.isEmpty() || forStack_.top().beginLine != currentLine)
	{
		ForNode node = getForNodeByLineIndex(currentLine);
		node.beginValue = nBeginValue;
		node.endValue = nEndValue;
		node.stepValue = stepValue;

		++node.callCount;
		if (isLocalVarContains(node.varName))
			insertLocalVar(node.varName, nBeginValue);
		else if (isGlobalVarContains(node.varName))
			insertGlobalVar(node.varName, nBeginValue);
		else
			insertLocalVar(node.varName, nBeginValue);
		forStack_.push(node);
		currentField.push(TK_FOR);
	}
	else
	{
		ForNode node = forStack_.top();

		if (node.varName.isEmpty())
			return false;

		QVariant localValue;
		if (isLocalVarContains(node.varName))
			localValue = getLocalVarValue(node.varName);
		else if (isGlobalVarContains(node.varName))
			localValue = getGlobalVarValue(node.varName);
		else
			return processBreak();

		if (localValue.type() != QVariant::LongLong)
			return false;

		if (!initValue.isValid() && !breakValue.isValid())
			return false;

		if (initValue.toString() == "nil" || breakValue.toString() == "nil")
			return false;

		qint64 nNowLocal = localValue.toLongLong();

		if ((nNowLocal >= nEndValue && nStepValue >= 0)
			|| (nNowLocal <= nEndValue && nStepValue < 0))
		{
			return processBreak();
		}
		else
		{
			nNowLocal += nStepValue;
			if (isLocalVarContains(node.varName))
				insertLocalVar(node.varName, nNowLocal);
			else if (isGlobalVarContains(node.varName))
				insertGlobalVar(node.varName, nNowLocal);
			else
				insertLocalVar(node.varName, nNowLocal);
		}
	}
	return false;
}

bool Parser::processEnd()
{
	if (currentField.isEmpty())
	{
		if (!forStack_.isEmpty())
			return processEndFor();
		else
			return processReturn();
	}
	else if (currentField.top() == TK_FUNCTION)
	{
		return processReturn();
	}
	else if (currentField.top() == TK_FOR)
	{
		return processEndFor();
	}

	return false;
}

//處理"遍歷結束"
bool Parser::processEndFor()
{
	if (forStack_.isEmpty())
		return false;

	ForNode node = forStack_.top();//跳回 for
	jumpto(node.beginLine + 1, true);
	return true;
}

//處理"繼續"
bool Parser::processContinue()
{
	if (currentField.isEmpty())
		return false;

	RESERVE reserve = currentField.top();
	switch (reserve)
	{
	case TK_FOR:
	{
		if (forStack_.isEmpty())
			break;

		ForNode node = forStack_.top();//跳到 end
		jumpto(node.endLine + 1, true);
		return true;
		break;
	}
	case TK_WHILE:
	case TK_REPEAT:
	default:
		break;
	}

	return false;
}

//處理"跳出"
bool Parser::processBreak()
{
	if (currentField.isEmpty())
		return false;

	RESERVE reserve = currentField.pop();

	switch (reserve)
	{
	case TK_FOR:
	{
		if (forStack_.isEmpty())
			break;

		//for行號出棧
		ForNode node = forStack_.pop();
		qint64 endline = node.endLine + 1;

		jumpto(endline + 1, true);
		return true;
	}
	case TK_WHILE:
	case TK_REPEAT:
	default:
		break;
	}

	return false;
}

bool Parser::processLuaCode()
{
	const qint64 currentLine = getCurrentLine();
	QString luaCode;
	for (const LuaNode& it : luaNodeList_)
	{
		if (currentLine != it.beginLine)
			continue;

		luaCode = it.content;
		break;
	}

	if (luaCode.isEmpty())
		return false;

	const QString luaFunction = QString(R"(
local chunk <const> = function()
	%1
end

return chunk();
	)").arg(luaCode);

	QVariant result = luaDoString(luaFunction);
	bool isValid = result.isValid();
	if (isValid)
		insertGlobalVar("vret", result);
	else
		insertGlobalVar("vret", "nil");
	return isValid;
}

//處理所有的token
void Parser::processTokens()
{
	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	Injector& injector = Injector::getInstance(currentIndex);
	sol::state& lua_ = pLua_->getLua();
	pLua_->setMax(size());

	//同步模式時清空所有marker並重置UI顯示的堆棧訊息
	if (mode_ == kSync)
	{
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addStepMarker(-1, false);
		generateStackInfo(kModeCallStack);
		generateStackInfo(kModeJumpStack);
	}

	bool skip = false;
	RESERVE currentType = TK_UNK;
	QString name;

	QElapsedTimer timer; timer.start();

	for (;;)
	{
		if (isInterruptionRequested())
		{
			break;
		}

		if (empty())
		{
			break;
		}

		//導出變量訊息
		if (mode_ == kSync && !skip)
			exportVarInfo();

		qint64 currentLine = getCurrentLine();
		currentLineTokens_ = tokens_.value(currentLine);

		currentType = getCurrentFirstTokenType();

		skip = currentType == RESERVE::TK_WHITESPACE || currentType == RESERVE::TK_COMMENT || currentType == RESERVE::TK_UNK;

		if (currentType == TK_FUNCTION)
		{
			if (checkCallStack())
				continue;
		}

		if (!skip)
		{
			processDelay();
			if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
			{
				QThread::msleep(1);
			}

			if (callBack_ != nullptr)
			{
				qint64 status = callBack_(currentIndex, currentLine, currentLineTokens_);
				if (status == kStop)
					break;
			}
		}

		switch (currentType)
		{
		case TK_COMMENT:
		case TK_WHITESPACE:
		{
			break;
		}
		case TK_EXIT:
		{
			name.clear();
			requestInterruption();
			break;
		}
		case TK_LUABEGIN:
		{
			if (!processLuaCode())
			{
				name.clear();
				requestInterruption();
			}
			break;
		}
		case TK_LUAEND:
		{
			break;
		}
		case TK_IF:
		{
			if (processIfCompare())
				continue;

			break;
		}
		case TK_CMD:
		{
			qint64 ret = processCommand();
			switch (ret)
			{
			case kHasJump:
			{
				generateStackInfo(kModeJumpStack);
				continue;
			}
			case kError:
			case kArgError:
			case kUnknownCommand:
			default:
			{
				handleError(ret);
				name.clear();
				break;
			}
			break;
			}

			break;
		}
		case TK_LOCAL:
		{
			processLocalVariable();
			break;
		}
		case TK_TABLESET:
		{
			processTableSet();
			break;
		}
		case TK_TABLE:
		case TK_LOCALTABLE:
		{
			processTable();
			break;
		}
		case TK_MULTIVAR:
		{
			processVariable();
			break;
		}
		case TK_INCDEC:
		{
			processVariableIncDec();
			break;
		}
		case TK_CAOS:
		{
			processVariableCAOs();
			break;
		}
		case TK_EXPR:
		{
			processVariableExpr();
			break;
		}
		case TK_FORMAT:
		{
			processFormation();
			break;
		}
		case TK_GOTO:
		{
			if (processGoto())
				continue;

			break;
		}
		case TK_JMP:
		{
			if (processJump())
				continue;

			break;
		}
		case TK_CALLWITHNAME:
		case TK_CALL:
		{
			if (processCall(currentType))
			{
				generateStackInfo(kModeCallStack);
				continue;
			}

			generateStackInfo(kModeCallStack);

			break;
		}
		case TK_FUNCTION:
		{
			processFunction();
			break;
		}
		case TK_LABEL:
		{
			processLabel();
			break;
		}
		case TK_FOR:
		{
			if (processFor())
				continue;

			break;
		}
		case TK_BREAK:
		{
			if (processBreak())
				continue;

			break;
		}
		case TK_CONTINUE:
		{
			if (processContinue())
				continue;

			break;
		}
		case TK_END:
		{
			if (processEnd())
				continue;

			break;
		}
		case TK_RETURN:
		{
			bool bret = processReturn();
			generateStackInfo(kModeCallStack);
			if (bret)
				continue;

			break;
		}
		case TK_BAK:
		{
			processBack();
			generateStackInfo(kModeJumpStack);
			continue;
		}
		default:
		{
			qDebug() << "Unexpected token type:" << currentType;
			break;
		}
		}

		//導出變量訊息
		if (mode_ == kSync && !skip)
			exportVarInfo();

		//移動至下一行
		next();
	}

	//最後再輸出一次變量訊息
	if (mode_ == kSync && !skip)
		exportVarInfo();

	processClean();
	lua_.collect_garbage();

	/*==========全部重建 : 成功 2 個，失敗 0 個，略過 0 個==========
	========== 重建 開始於 1:24 PM 並使用了 01:04.591 分鐘 ==========*/
	if (&signalDispatcher != nullptr)
		emit signalDispatcher.appendScriptLog(QObject::tr(" ========== script result : %1，cost %2 ==========")
			.arg(isSubScript() ? QObject::tr("sub-ok") : QObject::tr("main-ok")).arg(util::formatMilliseconds(timer.elapsed())));
}

//導出變量訊息
void Parser::exportVarInfo()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		return;

	QVariantHash varhash;
	QVariantHash localHash;
	if (!localVarStack_.isEmpty())
		localHash = localVarStack_.top();

	QString key;
	for (auto it = localHash.cbegin(); it != localHash.cend(); ++it)
	{
		key = QString("local|%1").arg(it.key());
		varhash.insert(key, it.value());
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (&signalDispatcher != nullptr)
		emit signalDispatcher.varInfoImported(this, varhash, globalNames_);
}