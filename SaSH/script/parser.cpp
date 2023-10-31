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
constexpr long long kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr long long kFormatPlaceHoldSize = 3;

//不輸出到列表也不做更動的全局變量名稱
static const QStringList exceptionList = {
	"TARGET", "_THIS_PARSER", "_INDEX_", "_THIS", "_G", "_HOOKFORSTOP",
	"BattleClass", "CharClass", "InfoClass", "ItemClass", "MapClass", "PetClass", "SystemClass", "TARGET", "Timer", "_G", "_HOOKFORSTOP", "_INDEX", "_INDEX_", "_ROWCOUNT_",
	"_THIS", "_print", "assert", "base",  "collectgarbage",
	"contains", "copy", "coroutine", "dbclick", "debug", "dofile", "dragto", "error", "find", "format", "full", "getmetatable",
	"half", "input", "io", "ipairs", "lclick", "load", "loadfile", "loadset", "lower", "math", "mkpath", "mktable", "dlg", "ocr",
	"msg", "next", "os", "package", "pairs", "pcall",  "print", "printf", "rawequal",
	"rawget", "rawlen", "rawset", "rclick", "regex", "replace", "require", "rex", "rexg", "rnd", "saveset", "select", "set", "setmetatable",
	"skill", "sleep", "split", "string", "table", "tadd", "tback", "tfront", "timer", "tjoin", "tmerge", "todb", "toint", "tonumber", "tostr",
	"tostring", "tpadd", "tpopback", "tpopfront", "trim", "trotate", "trsort", "tshuffle", "tsleft", "tsort", "tsright", "tswap", "tunique", "type",
	"unit", "upper", "utf8", "warn", "xpcall"
};

#pragma region  LuaTools
void __fastcall makeTable(sol::state& lua, const char* name, long long i, long long j)
{
	if (!lua[name].valid())
		lua[name] = lua.create_table();
	else if (!lua[name].is<sol::table>())
		lua[name] = lua.create_table();

	long long k, l;

	for (k = 1; k <= i + 1; ++k)
	{
		if (!lua[name][k].valid())
			lua[name][k] = lua.create_table();
		else if (!lua[name][k].is<sol::table>())
			lua[name][k] = lua.create_table();

		for (l = 1; l <= j + 1; ++l)
		{
			if (!lua[name][k][l].valid())
				lua[name][k][l] = lua.create_table();
			else if (!lua[name][k][l].is<sol::table>())
				lua[name][k][l] = lua.create_table();
		}
	}
}

void __fastcall makeTable(sol::state& lua, const char* name, long long i)
{
	if (!lua[name].valid())
		lua[name] = lua.create_table();
	else if (!lua[name].is<sol::table>())
		lua[name] = lua.create_table();

	long long k;
	for (k = 1; k <= i + 1; ++k)
	{
		if (!lua[name][k].valid())
			lua[name][k] = lua.create_table();
		else if (!lua[name][k].is<sol::table>())
			lua[name][k] = lua.create_table();
	}
}

void __fastcall makeTable(sol::state& lua, const char* name)
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

std::vector<long long> Unique(const std::vector<long long>& v)
{
	std::vector<long long> result = v;
#if _MSVC_LANG > 201703L
	std::ranges::stable_sort(result, std::less<long long>());
#else
	std::sort(result.begin(), result.end(), std::less<long long>());
#endif
	result.erase(std::unique(result.begin(), result.end()), result.end());
	return result;
}

template<typename T>
std::vector<T> ShiftLeft(const std::vector<T>& v, long long i)
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
std::vector<T> ShiftRight(const std::vector<T>& v, long long i)
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
std::vector<T> Rotate(const std::vector<T>& v, long long len)//true = right, false = left
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

void hookProc(lua_State* L, lua_Debug* ar)
{
	if (!L)
		return;

	if (!ar)
		return;

	sol::this_state s = L;
	sol::state_view lua(L);

	if (ar->event == LUA_MASKRET || ar->event == LUA_MASKLINE || ar->event == LUA_MASKCALL)
	{
		luadebug::checkStopAndPause(s);

		if (!lua["_THIS_PARSER"].valid())
			return;

		Parser* pparser = lua["_THIS_PARSER"].get<Parser*>();
		if (pparser == nullptr)
			return;

		lua.set("_LINE_", pparser->getCurrentLine() + 1);

		//獲取區域變量數值
		long long i;
		const char* name = nullptr;

		long long tmpIndex = 1;
		long long ctmpIndex = 1;
		for (i = 1; (name = lua_getlocal(L, ar, i)) != nullptr; ++i)
		{
			QString key = util::toQString(name);
			QVariant value;
			long long depth = kMaxLuaTableDepth;
			QPair<QString, QVariant> pair = luadebug::getVars(L, i, depth);
			if (key == "(temporary)")
				key = QString("temporary_%1").arg(tmpIndex++);
			else if (key == "(C temporary)")
				key = QString("c_temporary_%1").arg(ctmpIndex++);
			else
			{
				if (pair.first == "(boolean)")
					value = pair.second.toBool();
				else if (pair.first == "(integer)")
					value = pair.second.toLongLong();
				else if (pair.first == "(number)")
					value = pair.second.toDouble();
				else
					value = pair.second.toString();
				pparser->insertLocalVar(key, value);
			}
			lua_pop(L, 1);
		}

		luadebug::checkStopAndPause(s);
	}
}

Parser::Parser(long long index)
	: Indexer(index)
	, lexer_(index)
	, counter_(new Counter())
	, globalNames_(new QStringList())
	, localVarStack_(new QStack<QHash<QString, QVariant>>())
	, luaLocalVarStringList_(new QStringList())
	, pLua_(new CLua(index))
{
	qDebug() << "Parser is created!!";
}

Parser::~Parser()
{
	processClean();
	qDebug() << "Parser is distory!!";
}

void Parser::initialize(Parser* pparent)
{
	long long index = getIndex();

	if (counter_.isNull())
		counter_.reset(q_check_ptr(new Counter()));

	if (globalNames_.isNull())
		globalNames_.reset(q_check_ptr(new QStringList()));

	if (localVarStack_.isNull())
		localVarStack_.reset(q_check_ptr(new QStack<QHash<QString, QVariant>>()));

	if (luaLocalVarStringList_.isNull())
		luaLocalVarStringList_.reset(q_check_ptr(new QStringList()));

	if (pLua_.isNull())
		pLua_.reset(q_check_ptr(new CLua(index)));

	pLua_->setHookEnabled(false);

	pLua_->openlibs();

	pLua_->setHookForStop(true);

	sol::state& lua_ = pLua_->getLua();

	makeTable(lua_, "battle", sa::MAX_ENEMY);
	makeTable(lua_, "unit", 2000);
	makeTable(lua_, "chat", sa::MAX_CHAT_HISTORY);
	makeTable(lua_, "card", sa::MAX_ADDRESS_BOOK);
	makeTable(lua_, "map");
	makeTable(lua_, "team", sa::MAX_PARTY);
	makeTable(lua_, "item", sa::MAX_ITEM + 200);
	makeTable(lua_, "pet", sa::MAX_PET);
	makeTable(lua_, "char");
	makeTable(lua_, "timer");
	makeTable(lua_, "dialog", sa::MAX_DIALOG_LINE);
	makeTable(lua_, "magic", sa::MAX_MAGIC);
	makeTable(lua_, "skill", sa::MAX_PROFESSION_SKILL);
	makeTable(lua_, "petskill", sa::MAX_PET, sa::MAX_SKILL);
	makeTable(lua_, "petequip", sa::MAX_PET, sa::MAX_PET_ITEM);
	makeTable(lua_, "point");
	makeTable(lua_, "mails", sa::MAX_ADDRESS_BOOK, sa::MAIL_MAX_HISTORY);

	insertGlobalVar("INDEX", index);

	if (globalNames_->isEmpty())
	{
		*globalNames_ = g_sysConstVarName;
	}

#pragma region init
	lua_.set_function("checkdaily", [this](std::string smisson, sol::object otimeout)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			QString mission = util::toQString(smisson);

			long long timeout = 5000;
			if (otimeout.is<long long>())
				timeout = otimeout.as<long long>();

			if (!injector.worker.isNull())
				return injector.worker->checkJobDailyState(mission, timeout);
			else
				return -1;
		});


	lua_.set_function("getgamestate", [this](long long id)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.GetGameState(id);
		});

	lua_.set_function("rungame", [this](long long id)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.RunGame(id);
		});

	lua_.set_function("closegame", [this](long long id)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.CloseGame(id);
		});

	lua_.set_function("openwindow", [this](long long id)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.OpenNewWindow(id);
		});

	lua_.set_function("setlogin", [this](long long id, long long server, long long subserver, long long position, std::string account, std::string password)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			QString acc = util::toQString(account);
			QString pwd = util::toQString(password);

			return sender.SetAutoLogin(id, server - 1, subserver - 1, position - 1, acc, pwd);
		});

	lua_.set_function("runex", [this](long long id, std::string path)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.RunFile(id, util::toQString(path));
		});

	lua_.set_function("stoprunex", [this](long long id)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.StopFile(id);
		});

	lua_.set_function("dostrex", [this](long long id, std::string content)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.RunScript(id, util::toQString(content));
		});

	lua_.set_function("loadsetex", [this](long long id, std::string content)->long long
		{
			Injector& injector = Injector::getInstance(getIndex());
			InterfaceSender sender(injector.getParentWidget());

			return sender.LoadSettings(id, util::toQString(content));
		});

	lua_.set_function("msg", [this](sol::object otext, sol::object otitle, sol::object otype, sol::this_state s)->std::string
		{
			QString text;
			if (otext.is<std::string>())
				text = util::toQString(otext);
			else if (otext.is<long long>())
				text = util::toQString(otext.as<long long>());
			else if (otext.is<double>())
				text = util::toQString(otext.as<double>());
			else if (otext.is<bool>())
				text = util::toQString(otext.as<bool>());
			else if (otext.is<sol::table>())
			{
				long long depth = kMaxLuaTableDepth;
				text = getLuaTableString(otext.as<sol::table>(), depth);
			}

			QString title;
			if (otitle.is<std::string>())
				title = util::toQString(otitle);
			else if (otitle.is<long long>())
				title = util::toQString(otitle.as<long long>());
			else if (otitle.is<double>())
				title = util::toQString(otitle.as<double>());
			else if (otitle.is<bool>())
				title = util::toQString(otitle.as<bool>());
			else if (otitle.is<sol::table>())
			{
				long long depth = kMaxLuaTableDepth;
				title = getLuaTableString(otitle.as<sol::table>(), depth);
			}

			long long type = 1;
			if (otype.is<long long>())
				type = otype.as<long long>();

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
			long long nret = QMessageBox::StandardButton::NoButton;
			emit signalDispatcher.messageBoxShow(text, type, title, &nret);
			return nret == QMessageBox::StandardButton::Yes ? "yes" : "no";
		});

	lua_.set_function("dlg", [this](std::string buttonstr, std::string stext, sol::object otype, sol::object otimeout, sol::this_state s)->sol::object
		{
			Injector& injector = Injector::getInstance(getIndex());
			sol::state_view lua_(s);

			if (injector.worker.isNull())
				return sol::lua_nil;

			QString buttonStrs = util::toQString(buttonstr);

			QString text = util::toQString(stext);

			long long timeout = DEFAULT_FUNCTION_TIMEOUT;
			if (otype.is<long long>())
				timeout = otype.as<long long>();

			if (otimeout.is<long long>())
				timeout = otimeout.as<long long>();

			unsigned long long type = 2;
			if (otype.is<unsigned long long>())
				type = otype.as<unsigned long long>();


			luadebug::checkOnlineThenWait(s);
			luadebug::checkBattleThenWait(s);

			text.replace("\\n", "\n");

			buttonStrs = buttonStrs.toUpper();
			QStringList buttonStrList = buttonStrs.split(util::rexOR, Qt::SkipEmptyParts);
			safe::Vector<long long> buttonVec;
			unsigned long long buttonFlag = 0;
			for (const QString& str : buttonStrList)
			{
				if (!sa::buttonMap.contains(str))
				{
					luadebug::showErrorMsg(s, luadebug::ERROR_LEVEL, QObject::tr("invalid button string: %1").arg(str));
					return sol::lua_nil;
				}
				unsigned long long value = sa::buttonMap.value(str);
				buttonFlag |= value;
			}

			injector.worker->IS_WAITFOR_CUSTOM_DIALOG_FLAG.on();
			injector.worker->createRemoteDialog(type, buttonFlag, text);
			bool bret = false;
			QElapsedTimer timer; timer.start();
			for (;;)
			{
				if (injector.IS_SCRIPT_INTERRUPT.get())
					return sol::lua_nil;
				if (timer.hasExpired(timeout))
					break;

				if (!injector.worker->IS_WAITFOR_CUSTOM_DIALOG_FLAG.get())
				{
					bret = true;
					break;
				}
				QThread::msleep(100);
			}
			injector.worker->IS_WAITFOR_CUSTOM_DIALOG_FLAG.off();

			if (!bret)
			{
				insertGlobalVar("vret", "nil");
				return sol::lua_nil;
			}

			QHash<QString, sa::BUTTON_TYPE> big5 = {
				{ "OK", sa::BUTTON_OK},
				{ "CANCEL", sa::BUTTON_CANCEL },
				{ "CLOSE",sa::BUTTON_CLOSE },
				{ "關閉",sa::BUTTON_CLOSE },
				//big5
				{ "確定", sa::BUTTON_YES },
				{ "取消", sa::BUTTON_NO },
				{ "上一頁",sa::BUTTON_PREVIOUS },
				{ "下一頁",sa::BUTTON_NEXT },
				{ "取消",sa::BUTTON_NO },
			};

			QHash<QString, sa::BUTTON_TYPE> gb2312 = {
				{ "OK",sa::BUTTON_OK},
				{ "CANCEL",sa::BUTTON_CANCEL },
				{ "关闭", sa::BUTTON_CLOSE },
				//gb2312
				{ "确定", sa::BUTTON_YES },
				{ "取消", sa::BUTTON_NO },
				{ "上一页",sa::BUTTON_PREVIOUS },
				{ "下一页", sa::BUTTON_NEXT },
			};

			sa::customdialog_t dialog = injector.worker->customDialog.get();

			QString sbtype;
			sbtype = big5.key(dialog.button, "");
			if (sbtype.isEmpty())
				sbtype = gb2312.key(dialog.button, "");

			if (sbtype.isEmpty())
				sbtype = "0x" + util::toQString(static_cast<long long>(dialog.rawbutton), 16);

			//insertGlobalVar("vret", (sbtype.isEmpty() && dialog.row > 0) ? QVariant(QString("{%1,''}").arg(dialog.row)) : QVariant(QString("{'%1','%2'}").arg(sbtype).arg(dialog.rawdata)));

			sol::table t = lua_.create_table();

			t["row"] = dialog.row;

			t["button"] = util::toConstData(sbtype);

			t["data"] = util::toConstData(dialog.rawdata);

			return t;
		});


	lua_.set_function("contains", [this](sol::object data, sol::object ocmp_data, sol::this_state s)->bool
		{
			auto toVariant = [](const sol::object& o)->QVariant
				{
					QVariant out;
					if (o.is<std::string>())
						out = util::toQString(o).simplified();
					else if (o.is<long long>())
						out = o.as<long long>();
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

				if (b.isEmpty())
					return false;

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

			if (adata.toString().isEmpty() && !bdata.toString().isEmpty())
				return false;
			else if (!adata.toString().isEmpty() && bdata.toString().isEmpty())
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

	timer["new"] = [this](sol::this_state s)->long long
		{
			QSharedPointer<QElapsedTimer> timer(QSharedPointer<QElapsedTimer>::create());
			if (timer.isNull())
				return 0;

			timer->start();
			unsigned long long id = 0;
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

	timer["get"] = [this](long long id, sol::this_state s)->long long
		{
			if (id == 0)
				return 0;

			QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
			if (timer == nullptr)
				return 0;

			return timer->elapsed();
		};

	timer["gets"] = [this](long long id, sol::this_state s)->long long
		{
			if (id == 0)
				return 0;

			QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
			if (timer == nullptr)
				return 0;

			return timer->elapsed() / 1000;
		};

	timer["getstr"] = [this](long long id, sol::this_state s)->std::string
		{
			if (id == 0)
				return "";

			QSharedPointer<QElapsedTimer> timer = timerMap_.value(id);
			if (timer == nullptr)
				return "";

			long long time = timer->elapsed();
			QString formated = util::formatMilliseconds(time);
			return util::toConstData(formated);
		};

	timer["del"] = [this](long long id, sol::this_state s)->bool
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
			long long type = QInputDialog::InputMode::TextInput;
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
				msg = argList.value(1);
			}

			if (argList.size() > 2)
			{
				if (type == QInputDialog::IntInput)
					var = QVariant(argList.value(2).toLongLong(&ok));
				else if (type == QInputDialog::DoubleInput)
					var = QVariant(argList.value(2).toDouble(&ok));
				else
					var = QVariant(argList.value(2));
			}

			emit signalDispatcher.inputBoxShow(msg, type, &var);

			if (var.toLongLong() == 987654321ll)
			{
				insertGlobalVar("vret", "nil");
				luadebug::showErrorMsg(s, luadebug::WARN_LEVEL, QObject::tr("force stop by user input stop code"));
				Injector& injector = Injector::getInstance(getIndex());
				injector.stopScript();
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
				return sol::make_object(s, util::toConstData(var.toString()));
			}
		});

	lua_.set_function("format", [this](std::string sformat, sol::this_state s)->sol::object
		{
			QString formatStr = util::toQString(sformat);
			if (formatStr.isEmpty())
				return sol::lua_nil;

			static const QRegularExpression rexFormat(R"(\{\s*(?:([CT]?))\s*:\s*([^}]+)\s*\})");
			if (!formatStr.contains(rexFormat))
				return sol::make_object(s, sformat);

			enum FormatType
			{
				kNothing,
				kTime,
				kConst,
			};

			constexpr long long MAX_FORMAT_DEPTH = 10;

			for (long long i = 0; i < MAX_FORMAT_DEPTH; ++i)
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
			return sol::make_object(s, util::toConstData(formatStr));

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
			long long size = FullWidth.size();
			for (long long i = 0; i < size; ++i)
			{
				result.replace(FullWidth.at(i), HalfWidth.at(i));
			}

			insertGlobalVar("vret", result);
			return util::toConstData(result);
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
			long long size = FullWidth.size();
			for (long long i = 0; i < size; ++i)
			{
				result.replace(HalfWidth.at(i), FullWidth.at(i));
			}

			insertGlobalVar("vret", result);
			return util::toConstData(result);
		});

	lua_.set_function("lower", [this](std::string sstr, sol::this_state s)->std::string
		{
			QString result = util::toQString(sstr).toLower();
			insertGlobalVar("vret", result);
			return util::toConstData(result);
		});

	lua_.set_function("upper", [this](std::string sstr, sol::this_state s)->std::string
		{
			QString result = util::toQString(sstr).toUpper();
			insertGlobalVar("vret", result);
			return util::toConstData(result);
		});

	lua_.set_function("trim", [this](std::string sstr, sol::object oisSimplified, sol::this_state s)->std::string
		{
			bool isSimplified = false;

			if (oisSimplified.is<bool>())
				isSimplified = oisSimplified.as<bool>();
			else if (oisSimplified.is<long long>())
				isSimplified = oisSimplified.as<long long>() > 0;
			else if (oisSimplified.is<double>())
				isSimplified = oisSimplified.as<double>() > 0.0;

			QString result = util::toQString(sstr).trimmed();

			if (isSimplified)
				result = result.simplified();

			insertGlobalVar("vret", result);
			return util::toConstData(result);
		});

	lua_.set_function("todb", [this](sol::object ovalue, sol::object len, sol::this_state s)->double
		{
			double result = 0.0;
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue).toDouble();
			else if (ovalue.is<long long>())
				result = static_cast<double>(ovalue.as<long long>() * 1.0);
			else if (ovalue.is<double>())
				result = ovalue.as<double>();

			if (len.is<long long>() && len.as<long long>() >= 0 && len.as<long long>() <= 16)
			{
				QString str = QString::number(result, 'f', len.as<long long>());
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
			else if (ovalue.is<long long>())
				result = util::toQString(ovalue.as<long long>());
			else if (ovalue.is<double>())
			{
				result = util::toQString(ovalue.as<double>());
				while (!result.isEmpty() && result.back() == '0')
					result.chop(1);
			}

			insertGlobalVar("vret", result);
			return util::toConstData(result);
		});

	lua_.set_function("toint", [this](sol::object ovalue, sol::this_state s)->long long
		{
			long long result = 0.0;
			if (ovalue.is<std::string>())
				result = util::toQString(ovalue).toLongLong();
			else if (ovalue.is<long long>())
				result = ovalue.as<long long>();
			else if (ovalue.is<double>())
				result = static_cast<long long>(qFloor(ovalue.as<double>()));

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
			else if (oisRex.is<long long>())
				isRex = oisRex.as<long long>() > 0;
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

			return util::toConstData(result);
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

			long long pos1 = varValue.indexOf(text1);
			if (pos1 < 0)
				pos1 = 0;

			long long pos2 = -1;
			if (text2.isEmpty())
				pos2 = varValue.length();
			else
			{
				pos2 = static_cast<long long>(varValue.indexOf(text2, pos1 + text1.length()));
				if (pos2 < 0)
					pos2 = varValue.length();
			}

			result = varValue.mid(pos1 + text1.length(), pos2 - pos1 - text1.length());

			insertGlobalVar("vret", result);
			return util::toConstData(result);
		});

	//参数1:字符串内容, 参数2:正则表达式, 参数3(选填):第几个匹配项, 参数4(选填):是否为全局匹配, 参数5(选填):第几组
	lua_.set_function("regex", [this](std::string ssrc, std::string rexstr, sol::object oidx, sol::object oisglobal, sol::object ogidx, sol::this_state s)->std::string
		{
			QString varValue = util::toQString(ssrc);

			QString result = varValue;

			QString text = util::toQString(rexstr);

			long long capture = 1;
			if (oidx.is<long long>())
				capture = oidx.as<long long>();

			bool isGlobal = false;
			if (oisglobal.is<long long>())
				isGlobal = oisglobal.as<long long>() > 0;
			else if (oisglobal.is<bool>())
				isGlobal = oisglobal.as<bool>();

			long long maxCapture = 0;
			if (ogidx.is<long long>())
				maxCapture = ogidx.as<long long>();

			const QRegularExpression regex(text);

			if (!isGlobal)
			{
				QRegularExpressionMatch match = regex.match(varValue);
				if (match.hasMatch())
				{
					if (capture < 0 || capture > match.lastCapturedIndex())
					{
						insertGlobalVar("vret", result);
						return util::toConstData(result);
					}

					result = match.captured(capture);
				}
			}
			else
			{
				QRegularExpressionMatchIterator matchs = regex.globalMatch(varValue);
				long long n = 0;
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
			return util::toConstData(result);
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

			long long n = 1;
			if (match.hasMatch())
			{
				for (long long i = 0; i <= match.lastCapturedIndex(); ++i)
				{
					result[n] = util::toConstData(match.captured(i));
				}
			}

			long long maxdepth = kMaxLuaTableDepth;
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

			long long n = 1;
			while (matchs.hasNext())
			{
				QRegularExpressionMatch match = matchs.next();

				if (!match.hasMatch())
					continue;

				for (const auto& capture : match.capturedTexts())
				{
					result[n] = util::toConstData(capture);
					++n;
				}
			}

			long long maxdepth = kMaxLuaTableDepth;
			insertGlobalVar("vret", getLuaTableString(result, maxdepth));
			return result;
		});

	lua_.set_function("rnd", [this](sol::object omin, sol::object omax, sol::this_state s)->long long
		{
			std::random_device rd;
			std::mt19937_64 gen(rd());
			long long result = 0;
			if (omin == sol::lua_nil && omax == sol::lua_nil)
			{
				result = gen();
				insertGlobalVar("vret", result);
				return result;
			}

			long long min = 0;
			if (omin.is<long long>())
				min = omin.as<long long>();


			long long max = 0;
			if (omax.is<long long>())
				max = omax.as<long long>();

			if ((min > 0 && max == 0) || (min == max))
			{
				std::uniform_int_distribution<long long> distribution(0, min);
				result = distribution(gen);
			}
			else if (min > max)
			{
				std::uniform_int_distribution<long long> distribution(max, min);
				result = distribution(gen);
			}
			else
			{
				std::uniform_int_distribution<long long> distribution(min, max);
				result = distribution(gen);
			}

			insertGlobalVar("vret", result);
			return result;
		});

	lua_["mkpath"] = [](std::string sfilename, sol::object obj, sol::this_state s)->std::string
		{
			QString retstring = "\0";
			QString fileName = util::toQString(sfilename);
			QFileInfo fileinfo(fileName);
			QString suffix = fileinfo.suffix();
			if (suffix.isEmpty())
				fileName += ".txt";
			else if (suffix != "txt")
				fileName.replace(suffix, "txt");

			if (obj == sol::lua_nil)
			{
				retstring = util::findFileFromName(fileName);
			}
			else if (obj.is<std::string>())
			{
				retstring = util::findFileFromName(fileName, util::toQString(obj));
			}

			return util::toConstData(retstring);
		};

	lua_["mktable"] = [](long long a, sol::object ob, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);

			sol::table t = lua.create_table();

			if (ob.is<long long>() && a > ob.as<long long>())
			{
				for (long long i = ob.as<long long>(); i < (a + 1); ++i)
				{
					t.add(i);
				}
			}
			else if (ob.is<long long>() && a < ob.as<long long>())
			{
				for (long long i = a; i < (ob.as<long long>() + 1); ++i)
				{
					t.add(i);
				}
			}
			else if (ob.is<long long>() && a == ob.as<long long>())
			{
				t.add(a);
			}
			else if (ob == sol::lua_nil && a >= 0)
			{
				for (long long i = 1; i < a + 1; ++i)
				{
					t.add(i);
				}
			}
			else if (ob == sol::lua_nil && a < 0)
			{
				for (long long i = a; i < 2; ++i)
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

			long long len = 1;
			if (oside == sol::lua_nil)
				len = 1;
			else if (oside.is<long long>())
				len = oside.as<long long>();
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

	lua_["tsleft"] = [](sol::object t, long long i, sol::this_state s)->sol::object
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

	lua_["tsright"] = [](sol::object t, long long i, sol::this_state s)->sol::object
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
						if (!i.second.is<long long>())
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
				std::vector<long long> v = t.as<std::vector<long long>>();
				std::vector<long long> v2 = Unique(v);
				for (const long long& i : v2) { t2.add(i); }
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

	lua_.set_function("split", [](std::string src, std::string del, sol::object skipEmpty, sol::object orex, sol::this_state s)->sol::object
		{
			sol::state_view lua(s);
			sol::table t = lua.create_table();
			QString qsrc = util::toQString(src);
			QString qdel = util::toQString(del);
			QStringList v;
			bool useRex = false;
			if (orex.is<bool>())
				useRex = orex.as<bool>();
			else if (orex.is<long long>())
				useRex = orex.as<long long>() > 0;
			else if (orex.is<double>())
				useRex = orex.as<double>() > 0.0;

			bool skip = true;
			if (skipEmpty.is<bool>())
				skip = skipEmpty.as<bool>();
			else if (skipEmpty.is<long long>())
				skip = skipEmpty.as<long long>() > 0;
			else if (skipEmpty.is<double>())
				skip = skipEmpty.as<double>() > 0.0;

			if (useRex)
			{
				const QRegularExpression re(qdel);
				v = qsrc.split(re, skip ? Qt::SkipEmptyParts : Qt::KeepEmptyParts);
			}
			else
				v = qsrc.split(qdel, skip ? Qt::SkipEmptyParts : Qt::KeepEmptyParts);

			if (v.size() > 1)
			{
				for (const QString& i : v)
				{
					t.add(util::toConstData(i));
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
				if (i.second.is<long long>())
				{
					l.append(util::toQString(i.second.as<long long>()));
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
			QString result = l.join(util::toQString(del));
			insertGlobalVar("vret", result);
			return  util::toConstData(result);
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
#pragma endregion

	lua_State* L = lua_.lua_state();
	lua_["_THIS_PARSER"] = pparent;
	lua_sethook(L, &hookProc, LUA_MASKRET | LUA_MASKCALL | LUA_MASKLINE, NULL);//
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
void Parser::parse(long long line)
{
	setCurrentLine(line); //設置當前行號
	callStack_.clear(); //清空調用棧
	jmpStack_.clear();  //清空跳轉棧

	if (tokens_.isEmpty())
		return;

	//解析腳本
	processTokens();
}

//處理錯誤
void Parser::handleError(long long err, const QString& addition)
{
	if (err == kNoChange)
		return;

	QString extMsg;
	if (!addition.isEmpty())
		extMsg = " " + addition;

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	QString msg;
	switch (err)
	{
	case kNoChange:
		return;
	case kError://程序級別的錯誤
	{
		--counter_->validCommand;
		++counter_->error;
		msg = QObject::tr("unknown error") + extMsg;
		break;
	}
	case kServerNotReady://遊戲未開的錯誤
	{
		--counter_->validCommand;
		++counter_->error;
		msg = QObject::tr("server not ready") + extMsg;
		break;
	}
	case kLabelError://標記不存在的錯誤
	{
		--counter_->validCommand;
		++counter_->error;
		msg = QObject::tr("label incorrect or not exist") + extMsg;
		break;
	}
	case kUnknownCommand://無法識別的指令錯誤
	{
		--counter_->validCommand;
		++counter_->error;
		QString cmd = currentLineTokens_.value(0).data.toString();
		msg = QObject::tr("unknown command: %1").arg(cmd) + extMsg;
		break;
	}
	case kLuaError://lua錯誤
	{
		if (extMsg.contains("FLAG_DETECT_STOP"))
		{
			emit signalDispatcher.appendScriptLog(QObject::tr("[info]") + QObject::tr("@ %1 | detail:%2").arg(getCurrentLine() + 1).arg("stop by user"), 6);
			return;
		}
		--counter_->validCommand;
		++counter_->error;
		QString cmd = currentLineTokens_.value(0).data.toString();
		msg = QObject::tr("[lua]:%1").arg(cmd) + extMsg;
		break;
	}
	default:
	{
		//參數錯誤
		if (err >= kArgError && err <= kArgError + 20ll)
		{
			--counter_->validCommand;
			++counter_->error;
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
void Parser::checkConditionalOperator(QString& expr)
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
		return "nil";

	importLocalVariablesToPreLuaList();//將局變量倒出成lua格式列表
	try { updateSysConstKeyword(expr); } //更新系統變量
	catch (...) { return "nil"; }
	checkConditionalOperator(expr); //替換三目語法

	QStringList localVars = *luaLocalVarStringList_;
	localVars.append(expr);
	QString exprStr = localVars.join("\n");

	sol::state& lua_ = pLua_->getLua();

	const std::string exprStrUTF8(util::toConstData(exprStr));
	//執行lua
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8, sol::script_pass_on_error);
	lua_.collect_garbage();
	if (!loaded_chunk.valid())
	{
		long long currentLine = getCurrentLine();
		sol::error err = loaded_chunk;
		QString errStr = util::toQString(err.what());
		handleError(kLuaError, errStr);
		Injector& injector = Injector::getInstance(getIndex());
		injector.stopScript();
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
	else if (retObject.is<long long>())
		return retObject.as<long long>();
	else if (retObject.is<double>())
		return retObject.as<double>();
	else if (retObject.is<sol::table>())
	{
		long long depth = kMaxLuaTableDepth;
		return getLuaTableString(retObject.as<sol::table>(), depth);
	}

	return "nil";
}

//取表達式結果
template <typename T>
typename std::enable_if<
	std::is_same<T, QString>::value ||
	std::is_same<T, QVariant>::value ||
	std::is_same<T, bool>::value ||
	std::is_same<T, long long>::value ||
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
		|| expr == QString("返回") || expr == QString("跳回") || expr == QString("繼續") || expr == QString("跳出")
		|| expr == QString("继续") || expr == "exit")
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
	else if constexpr (std::is_same<T, long long>::value)
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

//嘗試取指定位置的token轉為字符串
bool Parser::checkString(const TokenMap& TK, long long idx, QString* ret)
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
	}
	else if (type == TK_STRING || type == TK_CAOS)
	{
		QString expr = var.toString();

		QString resultStr;
		if (!exprTo(expr, &resultStr))
			return false;

		*ret = resultStr;

	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為整數
bool Parser::checkInteger(const TokenMap& TK, long long idx, long long* ret)
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
		long long value = var.toLongLong(&ok);
		if (!ok)
			return false;

		*ret = value;
		return true;
	}
	else if (type == TK_STRING || type == TK_CAOS)
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
	else if (type == TK_STRING)
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

bool Parser::checkNumber(const TokenMap& TK, long long idx, double* ret)
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
	else if (type == TK_STRING || type == TK_CAOS)
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

bool Parser::checkBoolean(const TokenMap& TK, long long idx, bool* ret)
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
	else if (type == TK_STRING || type == TK_CAOS)
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

//嘗試取指定位置的token轉為按照double -> long long -> string順序檢查
QVariant Parser::checkValue(const TokenMap TK, long long idx, QVariant::Type type)
{
	QVariant varValue("nil");
	long long ivalue;
	QString text;
	bool bvalue;
	double dvalue;

	if (checkBoolean(TK, idx, &bvalue))
	{
		varValue = bvalue;
	}
	else if (checkNumber(TK, idx, &dvalue))
	{
		varValue = dvalue;
	}
	else if (checkInteger(TK, idx, &ivalue))
	{
		varValue = ivalue;
	}
	else if (checkString(TK, idx, &text))
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
long long Parser::checkJump(const TokenMap& TK, long long idx, bool expr, JumpBehavior behavior)
{
	bool okJump = false;
	if (behavior == JumpBehavior::FailedJump)
		okJump = !expr;
	else
		okJump = expr;

	if (!okJump)
		return Parser::kNoChange;

	QString label;
	long long line = 0;
	if (TK.contains(idx))
	{
		QString preCheck = TK.value(idx).data.toString().simplified();

		if (preCheck == "return" || preCheck == "back" || preCheck == "continue" || preCheck == "break"
			|| preCheck == QString("返回") || preCheck == QString("跳回") || preCheck == QString("繼續") || preCheck == QString("跳出")
			|| preCheck == QString("继续") || preCheck == "exit")
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
				long long value = 0;
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
void Parser::checkCallArgs(long long n)
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QVariantList list;
	long long currentLine = getCurrentLine();
	if (tokens_.contains(currentLine))
	{
		TokenMap TK = tokens_.value(currentLine);
		long long size = TK.size();
		for (long long i = kCallPlaceHoldSize + n; i < size; ++i)
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

		long long lastRow = node.callFromLine;

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
	std::string sname(util::toConstData(name));
	sol::state& lua_ = pLua_->getLua();
	if (lua_[sname.c_str()].valid())
		return true;

	return false;
}

QVariant Parser::getGlobalVarValue(const QString& name)
{
	std::string sname(util::toConstData(name));
	sol::state& lua_ = pLua_->getLua();

	if (!lua_[sname.c_str()].valid())
		return QVariant("nil");

	QVariant var;
	sol::object obj = lua_[sname.c_str()];
	if (obj.is<bool>())
		var = obj.as<bool>();
	else if (obj.is<std::string>())
		var = util::toQString(obj);
	else if (obj.is<long long>())
		var = obj.as<long long>();
	else if (obj.is<double>())
		var = obj.as<double>();
	else if (obj.is<sol::table>())
	{
		long long depth = kMaxLuaTableDepth;
		var = getLuaTableString(obj.as<sol::table>(), depth);
	}
	else
		var = "nil";

	return var;
}

//局變量保存到預備列表
void Parser::importLocalVariablesToPreLuaList()
{
	//將局變量保存到預備列表、每次執行表達式時都會將局變量列表轉換lua語句插入表達式前
	QVariantHash localVars = getLocalVars();
	luaLocalVarStringList_->clear();
	QString key, strvalue;
	QVariant::Type type = QVariant::Invalid;

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
		luaLocalVarStringList_->append(strvalue);
	}
}

//插入全局變量
void Parser::insertGlobalVar(const QString& name, const QVariant& value)
{
	if (exceptionList.contains(name))
		return;

	QVariant var = value;

	if (value.toString() == "nil")
	{
		removeGlobalVar(name);
		return;
	}

	if (!globalNames_->contains(name))
	{
		globalNames_->append(name);
		globalNames_->removeDuplicates();

		QCollator collator = util::getCollator();
		std::sort(globalNames_->begin(), globalNames_->end(), collator);
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
			lua_.set(keyBytes.constData(), util::toConstData(str));
		}
		else//table
		{
			QString content = QString("%1 = %2;").arg(name).arg(str);
			std::string contentBytes(util::toConstData(content));
			sol::protected_function_result loaded_chunk = lua_.safe_script(contentBytes, sol::script_pass_on_error);
			lua_.collect_garbage();
			if (!loaded_chunk.valid())
			{
				sol::error err = loaded_chunk;
				QString errStr = util::toQString(err.what());
				handleError(kLuaError, errStr);
				Injector& injector = Injector::getInstance(getIndex());
				injector.stopScript();
				return;
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
	QByteArray ba = name.toUtf8();
	lua_[ba.constData()] = sol::lua_nil;
	if (!globalNames_->contains(name))
		return;

	globalNames_->removeOne(name);
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

	QVariant::Type type = value.type();

	if (type != QVariant::String
		&& type != QVariant::Int && type != QVariant::LongLong
		&& type != QVariant::Double
		&& type != QVariant::Bool)
	{
		hash.insert(name, "nil");
		return;
	}

	hash.insert(name, value);
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
	if (!localVarStack_->isEmpty())
		return localVarStack_->top();

	return emptyLocalVars_;
}

QVariantHash& Parser::getLocalVarsRef()
{
	if (!localVarStack_->isEmpty())
		return localVarStack_->top();

	localVarStack_->push(emptyLocalVars_);
	return localVarStack_->top();
}
#pragma endregion

long long Parser::matchLineFromLabel(const QString& label) const
{
	if (!labels_.contains(label))
		return -1;

	return labels_.value(label, -1);
}

long long Parser::matchLineFromFunction(const QString& funcName) const
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

ForNode Parser::getForNodeByLineIndex(long long line) const
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

//行數跳轉
bool Parser::jump(long long line, bool noStack)
{
	long long currentLine = getCurrentLine();
	if (!noStack)
		jmpStack_.push(currentLine + 1);
	currentLine += line;
	setCurrentLine(currentLine);
	return true;
}

//指定行跳轉
void Parser::jumpto(long long line, bool noStack)
{
	long long currentLine = getCurrentLine();
	if (line - 1 < 0)
		line = 1;
	if (!noStack)
		jmpStack_.push(currentLine + 1);
	currentLine = line - 1;
	setCurrentLine(currentLine);
}

//標記、函數跳轉或執行關鍵命令
bool Parser::jump(const QString& name, bool noStack)
{
	QString newName = name.toLower();
	if (newName == "back" || newName == QString("跳回"))
	{
		if (!jmpStack_.isEmpty())
		{
			long long returnIndex = jmpStack_.pop();//jump行號出棧
			long long jumpLineCount = returnIndex - getCurrentLine();

			return jump(jumpLineCount, true);
		}
		return false;
	}
	else if (newName == "return" || newName == QString("返回"))
	{
		return processReturn(3);//從第三個參數開始取返回值
	}
	else if (newName == "continue" || newName == QString("繼續") || newName == QString("继续"))
	{
		return processContinue();
	}
	else if (newName == "break" || newName == QString("跳出"))
	{
		return processBreak();
	}
	else if (newName == "exit")
	{
		Injector& injector = Injector::getInstance(getIndex());
		injector.stopScript();
		return true;
	}

	//從跳轉位調用函數
	long long jumpLine = matchLineFromFunction(name);
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

	long long currentLine = getCurrentLine();
	if (!noStack)
		jmpStack_.push(currentLine + 1);//行號入棧

	long long jumpLineCount = jumpLine - currentLine;

	return jump(jumpLineCount, true);
}

QString Parser::getLuaTableString(const sol::table& t, long long& depth)
{
	if (depth < 0)
		return "{}";

	if (t.size() == 0)
		return "{}";

	--depth;

	QString ret = "{";

	QStringList results;
	QStringList strKeyResults;

	QString nowIndent = "";
	for (long long i = 0; i <= 10 - depth + 1; ++i)
	{
		nowIndent += "    ";
	}

	for (const auto& pair : t)
	{
		long long nKey = 0;
		QString key = "";
		QString value = "";

		if (pair.first.is<long long>())
		{
			nKey = pair.first.as<long long>() - 1;
		}
		else
			key = util::toQString(pair.first);

		if (pair.second.is<sol::table>())
			value = getLuaTableString(pair.second.as<sol::table>(), depth);
		else if (pair.second.is<std::string>())
			value = QString("'%1'").arg(util::toQString(pair.second));
		else if (pair.second.is<long long>())
			value = util::toQString(pair.second.as<long long>());
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
				for (long long i = results.size(); i <= nKey; ++i)
					results.append("nil");
			}

			results[nKey] = nowIndent + value;
			//strKeyResults.append(nowIndent + QString("[%1] = %2").arg(nKey + 1).arg(value));
		}
		else
			strKeyResults.append(nowIndent + QString("%1=%2").arg(key).arg(value));
	}

	QCollator collator = util::getCollator();
	std::sort(strKeyResults.begin(), strKeyResults.end(), collator);

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

	for (long long i = kCallPlaceHoldSize; i < currentLineTokens_.size(); ++i)
	{
		Token token = currentLineTokens_.value(i);
		if (token.type == TK_FUNCTIONARG)
		{
			QString labelName = token.data.toString().simplified();
			if (labelName.isEmpty())
				continue;

			if (i - kCallPlaceHoldSize >= args.size())
				break;

			QVariant currnetVar = args.value(i - kCallPlaceHoldSize);
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
						handleError(kArgError + i - kCallPlaceHoldSize + 1, QString(QObject::tr("@ %1 | Invalid local variable type expacted '%2' but got '%3'"))
							.arg(getCurrentLine()).arg(expectedType).arg(currnetVar.typeName()));
						break;
					}

					if (expectedType == labelName)
						labelVars.insert(labelName, currnetVar);
				}
			}
		}
	}

	currentField.push(TK_FUNCTION);//作用域標誌入棧
	localVarStack_->push(labelVars);//聲明局變量入棧
}

//處理"標記"
void Parser::processLabel()
{

}

//處理"結束"
void Parser::processClean()
{
	if (!pLua_.isNull())
	{
		sol::state& lua = pLua_->getLua();
		lua.collect_garbage();
	}

	currentField.clear();
	callStack_.clear();
	jmpStack_.clear();
	forStack_.clear();
	callArgsStack_.clear();

	if (isSubScript() && getMode() == Parser::kSync)
		return;

	if (!counter_.isNull())
		counter_.reset();

	if (!globalNames_.isNull())
		globalNames_.reset();

	if (!localVarStack_.isNull())
		localVarStack_.reset();

	if (!luaLocalVarStringList_.isNull())
		luaLocalVarStringList_.reset();

	if (!pLua_.isNull())
		pLua_.reset();
}

//處理所有核心命令之外的所有命令
long long Parser::processCommand()
{
	TokenMap tokens = getCurrentTokens();
	Token commandToken = tokens.value(0);
	QVariant varValue = commandToken.data;
	if (!varValue.isValid())
	{
		return kError;
	}

	QString commandName = commandToken.data.toString();
	long long status = kNoChange;

	if (commandRegistry_.contains(commandName))
	{
		CommandRegistry function = commandRegistry_.value(commandName, nullptr);
		if (function == nullptr)
		{
			return kError;
		}

		long long currentIndex = getIndex();
		try
		{
			status = function(currentIndex, getCurrentLine(), tokens);
		}
		catch (...)
		{
			return kError;
		}
	}
	else
	{
		return kError;
	}

	return status;
}

//處理變量自增自減
void Parser::processVariableIncDec()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())
		return;

	RESERVE op = getTokenType(1);

	QString expr = QString("%1 = %1 %2 1; return %1").arg(varName).arg(op == TK_INC ? "+" : "-");

	QVariant result = luaDoString(expr);
	if (!result.isValid())
		return;

	insertVar(varName, result);
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

	QString followedExprStr = getToken<QString>(2);
	QString expr;

	if (typeFirst == QVariant::String)
	{
		if (op != TK_ADD || followedExprStr == "nil" || varFirst.toString() == "nil")
			return;

		expr = QString("%1 = %1 .. tostring(%2); return %1").arg(varName).arg(followedExprStr);

	}
	else if (typeFirst == QVariant::Int || typeFirst == QVariant::LongLong || typeFirst == QVariant::Double || typeFirst == QVariant::Bool)
	{
		QString opStr = getToken<QString>(1).simplified();
		opStr.replace("=", "");

		expr = QString("%1 = %1 %2 %3; return %1").arg(varName).arg(opStr).arg(followedExprStr);
	}
	else
		return;

	QVariant result = luaDoString(expr);
	if (!result.isValid())
		return;

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

	QString exprStr = QString("if (%1) then return true; else return false; end").arg(getToken<QString>(1).simplified());
	insertGlobalVar("_IFEXPR", exprStr);

	QVariant value = luaDoString(exprStr);

	insertGlobalVar("_IFRESULT", util::toQString(value.toBool()));

	return checkJump(currentLineTokens_, 2, value.toBool(), SuccessJump) == kHasJump;
}

//處理多變量賦予單值
void Parser::processMultiVariable()
{
	//QString fieldStr = getToken<QString>(0);

	//QString names = getToken<QString>(1);
	//QStringList nameList = names.split(util::rexComma, Qt::SkipEmptyParts);
	//for (QString& name : nameList)
	//	name = name.simplified();

	//QVariant varValue = checkValue(currentLineTokens_, 2);

	//if (fieldStr == "[global]")
	//{
	//	for (const QString& name : nameList)
	//		insertGlobalVar(name, varValue);
	//}
	//else
	//{
	//	for (const QString& name : nameList)
	//		insertLocalVar(name, varValue);
	//}

	QString varNameStr = getToken<QString>(0);
	QString field = "global";
	if (varNameStr.contains("local"))
	{
		field = "local";
		varNameStr.replace("local", "");
		varNameStr = varNameStr.simplified();
	}

	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	long long varCount = varNames.count();

	QVariant firstValue;

	QString preExpr = getToken<QString>(1);

	firstValue = checkValue(currentLineTokens_, 1);

	if (varCount == 1)
	{
		//只有一個有可能是局變量改值。使用綜合的insert
		if (field == "local")
			insertLocalVar(varNames.front(), firstValue);
		else
			insertGlobalVar(varNames.front(), firstValue);
		return;
	}

	//下面是多個變量聲明和初始化必定是全局
	for (long long i = 0; i < varCount; ++i)
	{
		QString varName = varNames.value(i);
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

		if (field == "local")
			insertLocalVar(varName, varValue);
		else
			insertGlobalVar(varName, varValue);
		firstValue = varValue;
	}
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
		if (!formatStr.startsWith("'") || !formatStr.startsWith("\""))
			formatStr = QString("'%1").arg(formatStr);
		if (!formatStr.endsWith("'") || !formatStr.endsWith("\""))
			formatStr = QString("%1'").arg(formatStr);

		QVariant var = luaDoString(QString("return format(%1);").arg(formatStr));

		QString formatedStr = var.toString();

		static const QRegularExpression rexOut(R"(\[(\d+)\])");
		if ((varName.startsWith("out", Qt::CaseInsensitive) && varName.contains(rexOut)) || varName.toLower() == "out")
		{
			QRegularExpressionMatch match = rexOut.match(varName);
			long long color = QRandomGenerator::global()->bounded(0, 10);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				long long nColor = str.toLongLong();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			long long currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.log.isOpen())
				injector.log.write(formatedStr, getCurrentLine());

			if (!injector.worker.isNull())
				injector.worker->announce(formatedStr, color);

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
			long long color = QRandomGenerator::global()->bounded(0, 10);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				long long nColor = str.toLongLong();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			long long currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			if (!injector.worker.isNull())
				injector.worker->talk(formatedStr, color);
		}
		else
		{
			insertVar(varName, QVariant::fromValue(formatedStr));
		}
	} while (false);
}

//處理"調用"
bool Parser::processCall(RESERVE reserve)
{
	do
	{
		QString functionName;
		if (reserve == TK_CALL)//call xxxx
			checkString(currentLineTokens_, 1, &functionName);
		else //xxxx()
			functionName = getToken<QString>(1);

		if (functionName.isEmpty())
			break;

		long long jumpLine = matchLineFromFunction(functionName);
		if (jumpLine == -1)
		{
			QString expr = getToken<QString>(100);
			if (!expr.isEmpty())
			{
				QVariant var = luaDoString("return " + expr);
				insertGlobalVar("vret", var);
				sol::state& lua_ = pLua_->getLua();
				if (lua_["_JUMP"].valid() && lua_["_JUMP"] != sol::lua_nil)
				{
					if (lua_["_JUMP"].is<long long>())
					{
						long long nvalue = lua_["_JUMP"].get<long long>();
						TokenMap TK;
						TK.insert(1, Token{ TK_INT, nvalue, util::toQString(nvalue) });
						if (checkJump(TK, 1, false, JumpBehavior::FailedJump) == kHasJump)
							return true;
					}
					else if (lua_["_JUMP"].is<std::string>())
					{
						QString str = util::toQString(lua_["_JUMP"].get<std::string>());
						TokenMap TK;
						TK.insert(1, Token{ TK_STRING, str, str });
						if (checkJump(TK, 1, false, JumpBehavior::FailedJump) == kHasJump)
							return true;
					}
				}
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
	do
	{
		QVariant var = checkValue(currentLineTokens_, 1);
		QVariant::Type type = var.type();
		if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double || type == QVariant::Bool)
		{
			long long jumpLineCount = var.toLongLong();
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

	long long line = var.toLongLong();
	if (line <= 0)
		return false;

	jumpto(line, false);
	return true;
}

//處理"返回"
bool Parser::processReturn(long long takeReturnFrom)
{
	QVariantList list;
	for (long long i = takeReturnFrom; i < currentLineTokens_.size(); ++i)
		list.append(checkValue(currentLineTokens_, i));

	long long size = list.size();

	if (size == 1)
		insertGlobalVar("vret", list.value(0));
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

		str += v.join(",");
		str += "}";

		insertGlobalVar("vret", str);
	}
	else
		insertGlobalVar("vret", "nil");

	if (!callArgsStack_.isEmpty())
		callArgsStack_.pop();//callArgNames出棧

	if (!localVarStack_->isEmpty())
		localVarStack_->pop();//callALocalArgs出棧

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
	long long returnIndex = node.callFromLine + 1;
	long long jumpLineCount = returnIndex - getCurrentLine();
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

	long long returnIndex = jmpStack_.pop();//jump行號出棧
	long long jumpLineCount = returnIndex - getCurrentLine();
	jump(jumpLineCount, true);
}

//這裡是防止人為設置過長的延時導致腳本無法停止
void Parser::processDelay()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	long long extraDelay = injector.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000ll)
	{
		//將超過1秒的延時分段
		long long i = 0ll;
		long long size = extraDelay / 1000ll;
		for (i = 0; i < size; ++i)
		{
			if (injector.IS_SCRIPT_INTERRUPT.get())
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
	long long currentLine = getCurrentLine();
	if (forStack_.isEmpty() || forStack_.top().beginLine != currentLine)
	{
		//init var value
		QVariant initValue = checkValue(currentLineTokens_, 2);
		if (initValue.type() != QVariant::LongLong)
		{
			initValue = QVariant("nil");
		}

		long long nBeginValue = initValue.toLongLong();

		//end var value
		QVariant endValue = checkValue(currentLineTokens_, 3);
		if (endValue.type() != QVariant::LongLong)
		{
			endValue = QVariant("nil");
		}

		long long nEndValue = endValue.toLongLong();

		//step var value
		QVariant stepValue = checkValue(currentLineTokens_, 4);
		if (stepValue.type() != QVariant::LongLong || stepValue.toLongLong() == 0)
			stepValue = 1ll;

		ForNode node = getForNodeByLineIndex(currentLine);
		node.beginValue = nBeginValue;
		node.endValue = nEndValue;
		node.stepValue = stepValue;

		if (!node.varName.isEmpty())
		{
			if (isLocalVarContains(node.varName))
				insertLocalVar(node.varName, nBeginValue);
			else if (isGlobalVarContains(node.varName))
				insertGlobalVar(node.varName, nBeginValue);
			else
				insertLocalVar(node.varName, nBeginValue);
		}

		++node.callCount;
		forStack_.push(node);
		currentField.push(TK_FOR);
		return false;
	}
	else
	{
		ForNode node = forStack_.pop();

		do
		{
			//變量空則無限循環
			if (node.varName.isEmpty())
			{
				++node.callCount;
				forStack_.push(node);
				return false;
			}

			QVariant varValue;
			if (isLocalVarContains(node.varName))
				varValue = getLocalVarValue(node.varName);
			else if (isGlobalVarContains(node.varName))
				varValue = getGlobalVarValue(node.varName);
			else
				break;

			if (varValue.type() != QVariant::LongLong)
				break;

			long long n = varValue.toLongLong();
			long long endValue = node.endValue.toLongLong();
			if (node.endValue.type() == QVariant::String)
			{
				if (!exprTo(node.endValue.toString(), &endValue))
				{
					++node.callCount;
					forStack_.push(node);
					return false;
				}
			}

			node.endValue = endValue;

			if ((n >= node.endValue.toLongLong() && node.stepValue.toLongLong() >= 0)
				|| (n <= node.endValue.toLongLong() && node.stepValue.toLongLong() < 0))
			{
				break;
			}
			else
			{
				n += node.stepValue.toLongLong();
				if (isLocalVarContains(node.varName))
					insertLocalVar(node.varName, n);
				else if (isGlobalVarContains(node.varName))
					insertGlobalVar(node.varName, n);
				else
					insertLocalVar(node.varName, n);

				//繼續循環
				++node.callCount;
				forStack_.push(node);
				return false;
			}
		} while (false);

		//結束循環
		++node.callCount;
		forStack_.push(node);
		return processBreak();
	}
}

//處理for, function結尾
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
		long long endline = node.endLine + 1;

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

//處理純Lua語句(所有無法辨識的命令都會直接當作lua執行)
bool Parser::processLuaString()
{
	bool bret = false;
	QString exprStr = getToken<QString>(1);
	luaDoString(exprStr);


	sol::state& lua_ = pLua_->getLua();
	sol::meta::unqualified_t<sol::table> table = lua_["_G"];
	if (!table.valid())
		return bret;

	if (table.empty())
		return bret;

	for (const std::pair<sol::object, sol::object>& pairMain : table)
	{
		QString key;
		if (pairMain.first.is<std::string>())
			key = util::toQString(pairMain.first).simplified();
		else if (pairMain.first.is<long long>())
			key = util::toQString(pairMain.first.as<long long>()).simplified();
		else
			continue;

		if (key.isEmpty())
			continue;

		if (key.startsWith("sol."))
			continue;

		if (exceptionList.contains(key))
			continue;

		if (!globalNames_->contains(key))
		{
			globalNames_->append(key);
			globalNames_->removeDuplicates();
		}
	}

	QCollator collator = util::getCollator();
	std::sort(globalNames_->begin(), globalNames_->end(), collator);
	return bret;
}

//處理整塊的lua代碼
bool Parser::processLuaCode()
{
	const long long currentLine = getCurrentLine();
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

	luaBegin_ = true;

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
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	Injector& injector = Injector::getInstance(currentIndex);

	pLua_->setMax(size());

	//同步模式時清空所有非break marker
	if (mode_ == kSync)
	{
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addStepMarker(-1, false);
	}

	bool skip = false;
	RESERVE currentType = TK_UNK;
	QString name;

	QElapsedTimer timer; timer.start();

	for (;;)
	{
		if (injector.IS_SCRIPT_INTERRUPT.get())
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

		long long currentLine = getCurrentLine();
		currentLineTokens_ = tokens_.value(currentLine);

		currentType = getCurrentFirstTokenType();

		skip = currentType == RESERVE::TK_WHITESPACE
			|| currentType == RESERVE::TK_COMMENT
			|| currentType == RESERVE::TK_UNK
			|| (luaBegin_ && currentType != RESERVE::TK_LUAEND);

		if (currentType == TK_FUNCTION)
		{
			if (checkCallStack())
				continue;
		}

		if (!skip)
		{
			processDelay();
			if (injector.IS_SCRIPT_DEBUG_ENABLE.get() && injector.IS_SCRIPT_EDITOR_OPENED.get())
			{
				QThread::msleep(1);
			}

			if (callBack_ != nullptr)
			{
				long long status = callBack_(currentIndex, currentLine, currentLineTokens_);
				if (status == kStop)
					break;
			}
		}

		++counter_->validCommand;

		switch (currentType)
		{
		case TK_COMMENT:
		{
			--counter_->validCommand;
			++counter_->comment;
			break;
		}
		case TK_WHITESPACE:
		{
			--counter_->validCommand;
			++counter_->space;
			break;
		}
		case TK_EXIT:
		{
			injector.stopScript();
			break;
		}
		case TK_MULTIVAR:
		{
			processMultiVariable();
			break;
		}
		case TK_LUABEGIN:
		{
			if (!processLuaCode())
			{
				name.clear();
				injector.stopScript();
			}
			break;
		}
		case TK_LUAEND:
		{
			luaBegin_ = false;
			break;
		}
		case TK_LUASTRING:
		{
			if (processLuaString())
				continue;
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
			long long ret = processCommand();
			switch (ret)
			{
			case kHasJump:
			{
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
		//case TK_FORMAT:
		//{
		//	processFormation();
		//	break;
		//}
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
				continue;
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
			if (processReturn())
				continue;

			break;
		}
		case TK_BAK:
		{
			processBack();
			continue;
		}
		default:
		{
			qDebug() << "Unexpected token type:" << currentType;
			break;
		}
		}

		if (injector.IS_SCRIPT_INTERRUPT.get())
		{
			break;
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

	if (&signalDispatcher != nullptr)
	{
		if (injector.IS_SCRIPT_DEBUG_ENABLE.get() && !isSubScript())
		{
			emit signalDispatcher.appendScriptLog(QObject::tr(" ========== script report : valid %1，error %2，comment %3，space %4 ==========")
				.arg(counter_->validCommand).arg(counter_->error).arg(counter_->comment).arg(counter_->space));
		}

		if (!isSubScript())
		{
			QString path = getScriptFileName();
			QString dirName = "script/";
			long long indexScript = path.indexOf(dirName);
			if (indexScript != -1)
				path = path.mid(indexScript + dirName.size());

			emit signalDispatcher.appendScriptLog(QObject::tr(" ========== script result : %1，cost %2 ==========")
				.arg("'" + path + "' " + (isSubScript() ? QObject::tr("sub-ok") : QObject::tr("main-ok"))).arg(util::formatMilliseconds(timer.elapsed())));
			injector.log.close();
		}
	}

	processClean();
}

//導出變量訊息
void Parser::exportVarInfo()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.IS_SCRIPT_DEBUG_ENABLE.get())//檢查是否為調試模式
		return;

	if (!injector.IS_SCRIPT_EDITOR_OPENED.get())//檢查編輯器是否打開
		return;

	QVariantHash varhash;
	QVariantHash localHash = getLocalVars();

	QString key;
	for (auto it = localHash.constBegin(); it != localHash.constEnd(); ++it)
	{
		key = QString("local|%1").arg(it.key());
		varhash.insert(key, it.value());
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (&signalDispatcher != nullptr)
	{
		emit signalDispatcher.varInfoImported(this, varhash, *globalNames_);
	}
}

#pragma region SystemVariable
//根據表達式更新lua內的變量值
void Parser::updateSysConstKeyword(const QString& expr)
{
	bool bret = false;
	sol::state& lua_ = pLua_->getLua();

	if (expr.contains("_FILE_"))
	{
		lua_.set("_FILE_", util::toConstData(getScriptFileName()));
	}

	if (expr.contains("_FUNCTION_"))
	{
		if (!callStack_.isEmpty())
			lua_.set("_FUNCTION_", util::toConstData(callStack_.top().name));
		else
			lua_.set("_FUNCTION_", sol::lua_nil);
	}

	if (expr.contains("PID"))
	{
		lua_.set("PID", static_cast<long long>(_getpid()));
	}

	if (expr.contains("THREADID"))
	{
		lua_.set("THREADID", reinterpret_cast<long long>(QThread::currentThreadId()));
	}

	if (expr.contains("INFINITE"))
	{
		lua_.set("INFINITE", std::numeric_limits<long long>::max());
	}
	if (expr.contains("MAXTHREAD"))
	{
		lua_.set("MAXTHREAD", SASH_MAX_THREAD);
	}
	if (expr.contains("MAXCHAR"))
	{
		lua_.set("MAXCHAR", sa::MAX_CHARACTER);
	}
	if (expr.contains("MAXDIR"))
	{
		lua_.set("MAXDIR", sa::MAX_DIR);
	}
	if (expr.contains("MAXITEM"))
	{
		lua_.set("MAXITEM", sa::MAX_ITEM - sa::CHAR_EQUIPPLACENUM);
	}
	if (expr.contains("MAXEQUIP"))
	{
		lua_.set("MAXEQUIP", sa::CHAR_EQUIPPLACENUM);
	}
	if (expr.contains("MAXCARD"))
	{
		lua_.set("MAXCARD", sa::MAX_ADDRESS_BOOK);
	}
	if (expr.contains("MAXMAGIC"))
	{
		lua_.set("MAXMAGIC", sa::MAX_MAGIC);
	}
	if (expr.contains("MAXSKILL"))
	{
		lua_.set("MAXSKILL", sa::MAX_PROFESSION_SKILL);
	}
	if (expr.contains("MAXPET"))
	{
		lua_.set("MAXPET", sa::MAX_PET);
	}
	if (expr.contains("MAXPETSKILL"))
	{
		lua_.set("MAXPETSKILL", sa::MAX_SKILL);
	}
	if (expr.contains("MAXCHAT"))
	{
		lua_.set("MAXCHAT", sa::MAX_CHAT_HISTORY);
	}
	if (expr.contains("MAXDLG"))
	{
		lua_.set("MAXDLG", sa::MAX_DIALOG_LINE);
	}
	if (expr.contains("MAXENEMY"))
	{
		lua_.set("MAXENEMY", sa::MAX_ENEMY);
	}

	long long currentIndex = getIndex();
	if (lua_["_INDEX"].valid() && lua_["_INDEX"].is<long long>())
	{
		long long tempIndex = lua_["_INDEX"].get<long long>();

		if (tempIndex >= 0 && tempIndex < SASH_MAX_THREAD)
			currentIndex = tempIndex;
	}

	Injector& injector = Injector::getInstance(currentIndex);

	if (expr.contains("HWND"))
	{
		lua_.set("HWND", reinterpret_cast<long long>(injector.getParentWidget()));
	}

	if (expr.contains("GAMEPID"))
	{
		lua_.set("GAMEPID", injector.getProcessId());
	}

	if (expr.contains("GAMEHWND"))
	{
		lua_.set("GAMEPID", reinterpret_cast<long long>(injector.getProcessWindow()));
	}

	if (expr.contains("GAMEHANDLE"))
	{
		lua_.set("GAMEHANDLE", reinterpret_cast<long long>(injector.getProcess()));
	}

	if (expr.contains("INDEX"))
	{
		lua_.set("INDEX", injector.getIndex());
	}


	/////////////////////////////////////////////////////////////////////////////////////
	if (injector.worker.isNull())
		return;

	if (expr.contains("GAME"))
	{
		bret = true;
		lua_.set("GAME", injector.worker->getGameStatus());
	}

	if (expr.contains("WORLD"))
	{
		bret = true;
		lua_.set("WORLD", injector.worker->getWorldStatus());
	}

	if (expr.contains("isonline"))
	{
		lua_.set("isonline", injector.worker->getOnlineFlag());
	}

	if (expr.contains("isbattle"))
	{
		lua_.set("isbattle", injector.worker->getBattleFlag());
	}

	if (expr.contains("isnormal"))
	{
		lua_.set("isnormal", !injector.worker->getBattleFlag());
	}

	if (expr.contains("isdialog"))
	{
		lua_.set("isdialog", injector.worker->isDialogVisible());
	}

	if (expr.contains("gtime"))
	{
		static const QHash<long long, QString> hash = {
			{ sa::LS_NOON, QObject::tr("noon") },
			{ sa::LS_EVENING, QObject::tr("evening") },
			{ sa::LS_NIGHT , QObject::tr("night") },
			{ sa::LS_MORNING, QObject::tr("morning") },
		};

		long long satime = injector.worker->saCurrentGameTime.get();
		QString timeStr = hash.value(satime, QObject::tr("unknown"));
		lua_.set("gtime", util::toConstData(timeStr));
	}

	//char\.(\w+)
	if (expr.contains("char."))
	{
		injector.worker->updateDatasFromMemory();

		sol::meta::unqualified_t<sol::table> ch = lua_["char"];

		sa::PC _pc = injector.worker->getPC();

		ch["name"] = util::toConstData(_pc.name);

		ch["fname"] = util::toConstData(_pc.freeName);

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

		ch["family"] = util::toConstData(_pc.family);

		ch["battlepet"] = _pc.battlePetNo + 1;

		ch["ridepet"] = _pc.ridePetNo + 1;

		ch["mailpet"] = _pc.mailPetNo + 1;

		ch["luck"] = _pc.luck;

		ch["point"] = _pc.point;
	}

	//pet\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("pet.") || expr.contains("pet["))
	{
		injector.worker->updateDatasFromMemory();

		const QHash<QString, sa::PetState> hash = {
			{ "battle", sa::kBattle },
			{ "standby", sa::kStandby },
			{ "mail",sa::kMail },
			{ "rest",sa::kRest },
			{ "ride",sa::kRide },
		};

		sol::meta::unqualified_t<sol::table> pet = lua_["pet"];

		pet["count"] = injector.worker->getPetSize();

		QHash<long long, sa::PET> pets = injector.worker->getPets();
		for (long long i = 0; i < sa::MAX_PET; ++i)
		{
			sa::PET p = pets.value(i);
			long long index = i + 1;

			pet[index]["valid"] = p.valid;

			pet[index]["name"] = util::toConstData(p.name);

			pet[index]["fname"] = util::toConstData(p.freeName);

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

			pet[index]["pos"] = p.index;//寵物顯示的位置

			pet[index]["index"] = index;//寵物實際的索引

			sa::PetState state = p.state;
			QString str = hash.key(state, "");
			pet[index]["state"] = util::toConstData(str);

			pet[index]["power"] = p.power;
		}
	}

	//item\[(\d+)\]\.(\w+)
	if (expr.contains("item.") || expr.contains("item["))
	{
		injector.worker->updateItemByMemory();

		sol::meta::unqualified_t<sol::table> item = lua_["item"];

		QHash<long long, sa::ITEM> items = injector.worker->getItems();
		for (long long i = 0; i < sa::MAX_ITEM; ++i)
		{
			sa::ITEM it = items.value(i);
			long long index = i + 1;
			if (i < sa::CHAR_EQUIPPLACENUM)
				index += 100;
			else
				index = index - sa::CHAR_EQUIPPLACENUM;

			item[index]["valid"] = it.valid;

			item[index]["index"] = index;

			item[index]["name"] = util::toConstData(it.name);

			item[index]["memo"] = util::toConstData(it.memo);

			item[index]["count"] = it.count;

			item[index]["dura"] = it.damage;

			item[index]["lv"] = it.level;

			item[index]["stack"] = it.stack;

			item[index]["lv"] = it.level;
			item[index]["field"] = it.field;
			item[index]["target"] = it.target;
			item[index]["type"] = it.type;
			item[index]["modelid"] = it.modelid;
			item[index]["name2"] = util::toConstData(it.name2);
		}


		QVector<long long> itemIndexs;
		injector.worker->getItemEmptySpotIndexs(&itemIndexs);

		item["space"] = itemIndexs.size();

		item["isfull"] = itemIndexs.size() == 0;

		auto getIndexs = [this, currentIndex](sol::object oitemnames, sol::object oitemmemos, bool includeEequip, sol::this_state s)->QVector<long long>
			{
				QVector<long long> itemIndexs;
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
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

				long long min = sa::CHAR_EQUIPPLACENUM;
				long long max = sa::MAX_ITEM;
				if (includeEequip)
					min = 0;

				if (!injector.worker->getItemIndexsByName(itemnames, itemmemos, &itemIndexs, min, max))
				{
					return itemIndexs;
				}
				return itemIndexs;
			};


		item.set_function("count", [this, currentIndex, getIndexs](sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::this_state s)->long long
			{
				insertGlobalVar("vret", 0);
				long long count = 0;
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return count;

				bool includeEequip = true;
				if (oitemmemos.is<bool>())
					includeEequip = oincludeEequip.as<bool>();

				QVector<long long> itemIndexs = getIndexs(oitemnames, oitemmemos, includeEequip, s);
				if (itemIndexs.isEmpty())
					return count;

				QHash<long long, sa::ITEM> items = injector.worker->getItems();
				for (const long long itemIndex : itemIndexs)
				{
					sa::ITEM item = items.value(itemIndex);
					if (item.valid)
						count += item.stack;
				}

				insertGlobalVar("vret", count);
				return count;
			});

		item.set_function("indexof", [this, currentIndex, getIndexs](sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::this_state s)->long long
			{
				insertGlobalVar("vret", -1);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return -1;

				bool includeEequip = true;
				if (oincludeEequip.is<bool>())
					includeEequip = oincludeEequip.as<bool>();

				QVector<long long> itemIndexs = getIndexs(oitemnames, oitemmemos, includeEequip, s);
				if (itemIndexs.isEmpty())
					return -1;

				long long index = itemIndexs.front();
				if (index < sa::CHAR_EQUIPPLACENUM)
					index += 100LL;
				else
					index -= static_cast<long long>(sa::CHAR_EQUIPPLACENUM);

				++index;

				insertGlobalVar("vret", index);
				return index;
			});

		item.set_function("find", [this, currentIndex, getIndexs](sol::object oitemnames, sol::object oitemmemos, sol::object oincludeEequip, sol::this_state s)->sol::object
			{
				insertGlobalVar("vret", -1);
				sol::state_view lua(s);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return sol::lua_nil;

				bool includeEequip = true;
				if (oincludeEequip.is<bool>())
					includeEequip = oincludeEequip.as<bool>();

				QVector<long long> itemIndexs = getIndexs(oitemnames, oitemmemos, includeEequip, s);
				if (itemIndexs.isEmpty())
					return sol::lua_nil;

				long long index = itemIndexs.front();
				if (index < sa::CHAR_EQUIPPLACENUM)
					index += 100LL;
				else
					index -= static_cast<long long>(sa::CHAR_EQUIPPLACENUM);

				++index;

				if (index < 0 || index >= sa::MAX_ITEM)
					return sol::lua_nil;

				sa::ITEM item = injector.worker->getItem(index);

				sol::table t = lua.create_table();
				t["valid"] = item.valid;
				t["index"] = index;
				t["name"] = util::toConstData(item.name);
				t["memo"] = util::toConstData(item.memo);
				t["name2"] = util::toConstData(item.name2);
				t["dura"] = item.damage;
				t["lv"] = item.level;
				t["stack"] = item.stack;
				t["field"] = item.field;
				t["target"] = item.target;
				t["type"] = item.type;
				t["modelid"] = item.modelid;

				return t;
			});
	}

	//team\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("team.") || expr.contains("team["))
	{
		sol::meta::unqualified_t<sol::table> team = lua_["team"];

		team["count"] = static_cast<long long>(injector.worker->getPartySize());

		sa::mapunit_s unit = {};
		sa::PARTY party = {};
		long long index = -1;

		for (long long i = 0; i < sa::MAX_PARTY; ++i)
		{
			party = injector.worker->getParty(i);
			index = i + 1;

			team[index]["valid"] = party.valid;

			team[index]["index"] = index;

			team[index]["id"] = party.id;

			team[index]["name"] = util::toConstData(party.name);

			if (injector.worker->mapUnitHash.contains(party.id))
				team[index]["fname"] = util::toConstData(injector.worker->mapUnitHash.value(party.id).freeName);

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

		map["name"] = util::toConstData(injector.worker->getFloorName());

		map["floor"] = injector.worker->getFloor();

		map["x"] = injector.worker->getPoint().x();

		map["y"] = injector.worker->getPoint().y();

		if (expr.contains("ground"))
			map["ground"] = util::toConstData(injector.worker->getGround());

		map.set_function("isxy", [this, currentIndex](long long x, long long y, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return false;
				QPoint pos = injector.worker->getPoint();
				return pos == QPoint(x, y);
			});

		map.set_function("isrect", [this, currentIndex](long long x1, long long y1, long long x2, long long y2, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return false;
				QPoint pos = injector.worker->getPoint();
				return pos.x() >= x1 && pos.x() <= x2 && pos.y() >= y1 && pos.y() <= y2;

			});

		map.set_function("ismap", [this, currentIndex](sol::object omap, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return false;

				if (omap.is<long long>())
				{
					return injector.worker->getFloor() == omap.as<long long>();
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
					if (ok && injector.worker->getFloor() == floor)
						return true;

					newName = it;
					if (newName.startsWith(kFuzzyPrefix))
					{
						newName = newName.mid(1);
						isExact = false;
					}

					if (newName.isEmpty())
						continue;

					if (isExact && injector.worker->getFloorName() == newName)
						return true;
					else if (injector.worker->getFloorName().contains(newName))
						return true;
				}

				return false;
			});
	}

	//card\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("card.") || expr.contains("card["))
	{
		sol::meta::unqualified_t<sol::table> card = lua_["card"];

		for (long long i = 0; i < sa::MAX_ADDRESS_BOOK; ++i)
		{
			sa::ADDRESS_BOOK addressBook = injector.worker->getAddressBook(i);
			long long index = i + 1;

			card[index]["valid"] = addressBook.valid;

			card[index]["index"] = index;

			card[index]["name"] = util::toConstData(addressBook.name);

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

		for (long long i = 0; i < sa::MAX_CHAT_HISTORY; ++i)
		{
			QString text = injector.worker->getChatHistory(i);
			long long index = i + 1;

			chat[index] = util::toConstData(text);
		}

		chat["contains"] = [this, currentIndex](std::string str, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return false;

				if (str.empty())
					return false;

				QStringList list = util::toQString(str).split(util::rexOR, Qt::SkipEmptyParts);
				QString text = util::toQString(str);
				for (long long i = 0; i < sa::MAX_CHAT_HISTORY; ++i)
				{
					QString cmptext = injector.worker->getChatHistory(i);
					for (const QString& it : list)
					{
						if (cmptext.contains(it))
							return true;
					}
				}

				return false;
			};
	}

	//unit\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("unit.") || expr.contains("unit["))
	{
		QList<sa::mapunit_t> units = injector.worker->mapUnitHash.values();

		long long size = units.size();

		sol::meta::unqualified_t<sol::table> unit = lua_["unit"];

		unit["count"] = size;

		for (long long i = 0; i < size; ++i)
		{
			sa::mapunit_t u = units.value(i);
			if (!u.isVisible)
				continue;

			long long index = i + 1;

			unit[index]["valid"] = u.isVisible;

			unit[index]["index"] = index;

			unit[index]["id"] = u.id;

			unit[index]["name"] = util::toConstData(u.name);

			unit[index]["fname"] = util::toConstData(u.freeName);

			unit[index]["family"] = util::toConstData(u.family);

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
				if (injector.worker.isNull())
					return 0;

				QString _name = "";
				long long modelid = 0;
				if (name.is<std::string>())
					_name = util::toQString(name);
				if (ofname.is<std::string>())
					_name = util::toQString(ofname);

				QString freeName = "";
				if (ofname.is<std::string>())
					freeName = util::toQString(ofname);

				if (_name.isEmpty() && modelid == 0 && freeName.isEmpty())
					return sol::lua_nil;

				sa::mapunit_t _unit = {};
				if (!injector.worker->findUnit(_name, sa::OBJ_NPC, &_unit, freeName, modelid))
				{
					if (!injector.worker->findUnit(_name, sa::OBJ_HUMAN, &_unit, freeName, modelid))
						return sol::lua_nil;
				}

				sol::table t = lua.create_table();
				t["valid"] = _unit.isVisible;

				t["index"] = -1;

				t["id"] = _unit.id;

				t["name"] = util::toConstData(_unit.name);

				t["fname"] = util::toConstData(_unit.freeName);

				t["family"] = util::toConstData(_unit.family);

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
	if ((expr.contains("battle.") || expr.contains("battle[")) && !expr.contains("isbattle"))
	{
		sol::meta::unqualified_t<sol::table> battle = lua_["battle"];

		battle["count"] = injector.worker->battle_total.get();
		battle["dura"] = injector.worker->battleDurationTimer.elapsed() / 1000.0;
		battle["time"] = injector.worker->battle_total_time.get() / 1000.0 / 60.0;
		battle["cost"] = injector.worker->battle_one_round_time.get() / 1000.0;
		battle["round"] = injector.worker->battleCurrentRound.get() + 1;
		battle["field"] = util::toConstData(injector.worker->getFieldString(injector.worker->battleField.get()));
		battle["charpos"] = static_cast<long long>(injector.worker->battleCharCurrentPos.get() + 1);
		battle["petpos"] = -1;
		battle["size"] = 0;
		battle["enemycount"] = 0;
		battle["alliecount"] = 0;

		if (injector.worker->getBattleFlag())
		{
			sa::battledata_t bt = injector.worker->getBattleData();

			battle["petpos"] = static_cast<long long>(bt.objects.value(injector.worker->battleCharCurrentPos.get() + 5).pos + 1);

			long long size = bt.objects.size();
			battle["size"] = size;
			battle["enemycount"] = bt.enemies.size();
			battle["alliecount"] = bt.allies.size();

			for (long long i = 0; i < size; ++i)
			{
				sa::battleobject_t obj = bt.objects.value(i);
				long long index = i + 1;

				battle[index]["valid"] = obj.maxHp > 0 && obj.level > 0 && obj.modelid > 0;

				battle[index]["index"] = static_cast<long long>(obj.pos + 1);

				battle[index]["name"] = util::toConstData(obj.name);

				battle[index]["fname"] = util::toConstData(obj.freeName);

				battle[index]["modelid"] = obj.modelid;

				battle[index]["lv"] = obj.level;

				battle[index]["hp"] = obj.hp;

				battle[index]["maxhp"] = obj.maxHp;

				battle[index]["hpp"] = obj.hpPercent;

				battle[index]["status"] = util::toConstData(injector.worker->getBadStatusString(obj.status));

				battle[index]["ride"] = obj.rideFlag > 0;

				battle[index]["ridename"] = util::toConstData(obj.rideName);

				battle[index]["ridelv"] = obj.rideLevel;

				battle[index]["ridehp"] = obj.rideHp;

				battle[index]["ridemaxhp"] = obj.rideMaxHp;

				battle[index]["ridehpp"] = obj.rideHpPercent;
			}
		}
	}

	//dialog\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	if (expr.contains("dialog.") || expr.contains("dialog["))
	{
		QStringList dialogstrs = injector.worker->currentDialog.get().linedatas;

		sol::meta::unqualified_t<sol::table> dlg = lua_["dialog"];

		long long size = dialogstrs.size();

		bool visible = injector.worker->isDialogVisible();

		for (long long i = 0; i < sa::MAX_DIALOG_LINE; ++i)
		{
			QString text;
			if (i >= size || !visible)
				text = "";
			else
				text = dialogstrs.value(i);

			long long index = i + 1;

			if (visible)
				dlg[index] = util::toConstData(text);
			else
				dlg[index] = "";
		}

		sa::dialog_t dialog = injector.worker->currentDialog.get();
		dlg["id"] = dialog.dialogid;
		dlg["unitid"] = dialog.unitid;
		dlg["type"] = dialog.windowtype;
		dlg["buttontext"] = util::toConstData(dialog.linebuttontext.join("|"));
		dlg["button"] = dialog.buttontype;
		dlg["data"] = util::toConstData(dialog.data);

		dlg["contains"] = [this, currentIndex](std::string str, sol::this_state s)->bool
			{
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return false;

				if (str.empty())
					return false;

				if (!injector.worker->isDialogVisible())
					return false;

				QString text = util::toQString(str);
				QStringList list = text.split(util::rexOR, Qt::SkipEmptyParts);
				QStringList dialogstrs = injector.worker->currentDialog.get().linedatas;
				long long size = dialogstrs.size();
				for (long long i = 0; i < size; ++i)
				{
					QString cmptext = dialogstrs.value(i);
					for (const QString& it : list)
					{
						if (cmptext.contains(it))
							return true;
					}
				}

				return false;
			};
	}

	//magic\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("magic.") || expr.contains("magic["))
	{
		sol::meta::unqualified_t<sol::table> mg = lua_["magic"];

		for (long long i = 0; i < sa::MAX_MAGIC; ++i)
		{
			long long index = i + 1;
			sa::MAGIC magic = injector.worker->getMagic(i);

			mg[index]["valid"] = magic.valid;
			mg[index]["index"] = index;
			mg[index]["costmp"] = magic.costmp;
			mg[index]["field"] = magic.field;
			mg[index]["name"] = util::toConstData(magic.name);
			mg[index]["memo"] = util::toConstData(magic.memo);
			mg[index]["target"] = magic.target;
		}

		mg["find"] = [this, currentIndex](sol::object oname, sol::this_state s)->sol::object
			{
				sol::state_view lua(s);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return 0;

				QString name = "";
				if (oname.is<std::string>())
					name = util::toQString(oname);

				if (name.isEmpty())
					return sol::lua_nil;


				long long index = injector.worker->getMagicIndexByName(name);
				if (index == -1)
					return sol::lua_nil;

				sa::MAGIC _magic = injector.worker->getMagic(index);

				sol::table t = lua.create_table();

				t["valid"] = _magic.valid;
				t["index"] = index;
				t["costmp"] = _magic.costmp;
				t["field"] = _magic.field;
				t["name"] = util::toConstData(_magic.name);
				t["memo"] = util::toConstData(_magic.memo);
				t["target"] = _magic.target;

				return t;
			};
	}

	//skill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("skill.") || expr.contains("skill["))
	{
		sol::meta::unqualified_t<sol::table> sk = lua_["skill"];

		for (long long i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
		{
			long long index = i + 1;
			sa::PROFESSION_SKILL skill = injector.worker->getSkill(i);

			sk[index]["valid"] = skill.valid;
			sk[index]["index"] = index;
			sk[index]["costmp"] = skill.costmp;
			sk[index]["modelid"] = skill.icon;
			sk[index]["type"] = skill.kind;
			sk[index]["lv"] = skill.skill_level;
			sk[index]["id"] = skill.skillId;
			sk[index]["name"] = util::toConstData(skill.name);
			sk[index]["memo"] = util::toConstData(skill.memo);
			sk[index]["target"] = skill.target;
		}

		sk["find"] = [this, currentIndex](sol::object oname, sol::this_state s)->sol::object
			{
				sol::state_view lua(s);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return 0;

				QString name = "";
				if (oname.is<std::string>())
					name = util::toQString(oname);

				if (name.isEmpty())
					return sol::lua_nil;

				long long index = injector.worker->getSkillIndexByName(name);
				if (index == -1)
					return sol::lua_nil;

				sa::PROFESSION_SKILL _skill = injector.worker->getSkill(index);

				sol::table t = lua.create_table();

				t["valid"] = _skill.valid;
				t["index"] = index;
				t["costmp"] = _skill.costmp;
				t["modelid"] = _skill.icon;
				t["type"] = _skill.kind;
				t["lv"] = _skill.skill_level;
				t["id"] = _skill.skillId;
				t["name"] = util::toConstData(_skill.name);
				t["memo"] = util::toConstData(_skill.memo);
				t["target"] = _skill.target;

				return t;
			};
	}

	//petskill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petskill.") || expr.contains("petskill["))
	{
		sol::meta::unqualified_t<sol::table> psk = lua_["petskill"];

		long long petIndex = -1;
		long long index = -1;
		long long i, j;
		for (i = 0; i < sa::MAX_PET; ++i)
		{
			petIndex = i + 1;

			for (j = 0; j < sa::MAX_SKILL; ++j)
			{
				index = j + 1;
				sa::PET_SKILL skill = injector.worker->getPetSkill(i, j);

				psk[petIndex][index]["valid"] = skill.valid;
				psk[petIndex][index]["index"] = index;
				psk[petIndex][index]["id"] = skill.skillId;
				psk[petIndex][index]["field"] = skill.field;
				psk[petIndex][index]["target"] = skill.target;
				psk[petIndex][index]["name"] = util::toConstData(skill.name);
				psk[petIndex][index]["memo"] = util::toConstData(skill.memo);
			}
		}

		psk["find"] = [this, currentIndex](long long petIndex, sol::object oname, sol::this_state s)->sol::object
			{
				sol::state_view lua(s);
				Injector& injector = Injector::getInstance(currentIndex);
				if (injector.worker.isNull())
					return 0;

				QString name = "";
				if (oname.is<std::string>())
					name = util::toQString(oname);

				if (name.isEmpty())
					return sol::lua_nil;

				long long index = injector.worker->getPetSkillIndexByName(petIndex, name);
				if (index == -1)
					return sol::lua_nil;

				sa::PET_SKILL _skill = injector.worker->getPetSkill(petIndex, index);

				sol::table t = lua.create_table();

				t["valid"] = _skill.valid;
				t["index"] = index;
				t["id"] = _skill.skillId;
				t["field"] = _skill.field;
				t["target"] = _skill.target;
				t["name"] = util::toConstData(_skill.name);
				t["memo"] = util::toConstData(_skill.memo);

				return t;
			};
	}

	//petequip\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petequip.") || expr.contains("petequip["))
	{
		sol::meta::unqualified_t<sol::table> peq = lua_["petequip"];

		long long petIndex = -1;
		long long index = -1;
		long long i, j;
		for (i = 0; i < sa::MAX_PET; ++i)
		{
			petIndex = i + 1;

			for (j = 0; j < sa::MAX_PET_ITEM; ++j)
			{
				index = j + 1;

				sa::ITEM item = injector.worker->getPetEquip(i, j);

				peq[petIndex][index]["valid"] = item.valid;
				peq[petIndex][index]["index"] = index;
				peq[petIndex][index]["lv"] = item.level;
				peq[petIndex][index]["field"] = item.field;
				peq[petIndex][index]["target"] = item.target;
				peq[petIndex][index]["type"] = item.type;
				peq[petIndex][index]["modelid"] = item.modelid;
				peq[petIndex][index]["dura"] = item.damage;
				peq[petIndex][index]["name"] = util::toConstData(item.name);
				peq[petIndex][index]["name2"] = util::toConstData(item.name2);
				peq[petIndex][index]["memo"] = util::toConstData(item.memo);
			}
		}
	}

	if (expr.contains("point."))
	{
		sa::currencydata_t point = injector.worker->currencyData.get();

		sol::meta::unqualified_t<sol::table> pt = lua_["point"];

		pt["exp"] = point.expbufftime;
		pt["rep"] = point.prestige;
		pt["ene"] = point.energy;
		pt["shl"] = point.shell;
		pt["vit"] = point.vitality;
		pt["pts"] = point.points;
		pt["vip"] = point.VIPPoints;
	}

	if (expr.contains("mails["))
	{
		sol::meta::unqualified_t<sol::table> mails = lua_["mails"];
		long long j = 0;
		for (long long i = 0; i < sa::MAX_ADDRESS_BOOK; ++i)
		{
			long long card = i + 1;
			sa::MAIL_HISTORY mail = injector.worker->getMailHistory(i);
			for (j = 0; j < sa::MAIL_MAX_HISTORY; ++j)
			{
				long long index = j + 1;
				mails[card][index] = util::toConstData(mail.dateStr[j]);
			}
		}
	}
}
#pragma endregion

#pragma region deprecated
#if 0
//處理單個或多個局變量聲明
void Parser::processLocalVariable()
{
	QString varNameStr = getToken<QString>(0);
	if (varNameStr.isEmpty())
		return;

	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	if (varNames.isEmpty())
		return;

	long long varCount = varNames.count();
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

	for (long long i = 0; i < varCount; ++i)
	{
		QString varName = varNames.value(i);
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
	long long varCount = varNames.count();
	long long value = 0;

	QVariant firstValue;

	QString preExpr = getToken<QString>(1);
	static const QRegularExpression rexCallFun(R"([\w\p{Han}]+\((.+)\))");
	if (preExpr.contains("input("))
	{
		exprTo(preExpr, &firstValue);
		if (firstValue.toLongLong() == 987654321ll)
		{
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
	for (long long i = 0; i < varCount; ++i)
	{
		QString varName = varNames.value(i);
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
		long long value = 0;
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
#endif
#pragma endregion