#include "stdafx.h"
#include "clua.h"

#include "signaldispatcher.h"
#include "injector.h"


#if QT_NO_DEBUG
#pragma comment(lib, "lua-5.4.4.lib")
#else
#pragma comment(lib, "lua-5.4.4d.lib")
#endif

#define OPEN_HOOK

void luadebug::tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1, const QVariant& p2, const QVariant& p3, const QVariant& p4)
{
	Q_UNUSED(p4);
	lua_State* L = s;
	////檢查當前行是否包含在#param中
	//if (QLuaDebuger::CHECK_PROGMA(L, "argument"))
		//return;

	switch (element)
	{
	case ERROR_FLAG_DETECT_STOP:
	case ERROR_REQUEST_STOP_FROM_USER:
	case ERROR_REQUEST_STOP_FROM_SCRIP:
	case ERROR_REQUEST_STOP_FROM_DISTRUCTOR:
	case ERROR_REQUEST_STOP_FROM_PARENT_SCRIP:
	{
		//用戶請求停止
		const QString qmsgstr(errormsg_str.value(element));
		const std::string str(qmsgstr.toStdString());
		luaL_error(L, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE://固定參數數量錯誤
	{
		int topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toInt()).arg(topsize));
		const std::string str(qmsgstr.toStdString());
		luaL_argcheck(L, topsize == p1.toInt(), topsize, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_NONE://無參數數量錯誤
	{
		int topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(topsize));
		const std::string str(qmsgstr.toStdString());
		luaL_argcheck(L, topsize == 0, 1, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_RANGE://範圍參數數量錯誤
	{
		int topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toInt()).arg(p2.toInt()).arg(topsize));
		const std::string str(qmsgstr.toStdString());
		luaL_argcheck(L, topsize >= p1.toInt() && topsize <= p2.toInt(), 1, str.c_str());
		break;
	}
	case ERROR_PARAM_VALUE://數值錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(qmsgstr.toStdString());
		luaL_argcheck(L, p2.toBool(), p1.toInt(), str.c_str());
		break;
	}
	case ERROR_PARAM_TYPE://參數預期錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(qmsgstr.toStdString());
		luaL_argexpected(L, p2.toBool(), p1.toInt(), str.c_str());
		break;
	}
	default:
	{
		luaL_error(L, "UNKNOWN ERROR TYPE");
		break;
	}
	}
}

QString luadebug::getErrorMsgLocatedLine(const QString& str, int* retline)
{
	const QString cmpstr(str.simplified());

	QRegularExpressionMatch match = rexGetLine.match(cmpstr);
	QRegularExpressionMatch match2 = reGetLineEx.match(cmpstr);
	static const auto matchies = [](const QRegularExpressionMatch& m, int* retline)->void
	{
		int size = m.capturedTexts().size();
		if (size > 1)
		{
			for (int i = (size - 1); i >= 1; --i)
			{
				const QString s = m.captured(i).simplified();
				if (!s.isEmpty())
				{
					bool ok = false;
					int row = s.simplified().toInt(&ok);
					if (ok)
					{
						if (retline)
							*retline = row - 1;

						break;
					}
				}
			}
		}
	};

	if (match.hasMatch())
	{
		matchies(match, retline);
	}
	else if (match2.hasMatch())
	{
		matchies(match2, retline);
	}

	return cmpstr;
}

bool luadebug::isInterruptionRequested(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	CLua* pLua = lua["_THIS"].get<CLua*>();
	if (pLua == nullptr)
		return false;

	return pLua->isInterruptionRequested();
}

void luadebug::checkStopAndPause(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	lua_State* L = s.lua_state();
	CLua* pLua = lua["_THIS"].get<CLua*>();
	if (pLua == nullptr)
		return;

	if (pLua->isInterruptionRequested())
	{
		luadebug::tryPopCustomErrorMsg(s, luadebug::ERROR_FLAG_DETECT_STOP);
		return;
	}
	pLua->checkPause();
}

bool luadebug::checkBattleThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	Injector& injector = Injector::getInstance();
	bool bret = false;
	if (injector.server->getBattleFlag())
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested(s))
				break;

			if (!injector.server.isNull())
				break;

			checkStopAndPause(s);

			if (!injector.server->getBattleFlag())
				break;
			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(500);
	}
	return bret;
}

//lua函數鉤子 這裡主要用於控制 暫停、終止腳本、獲取棧數據、變量數據...或其他操作
void luadebug::hookProc(lua_State* L, lua_Debug* ar)
{
	if (!L) return;
	if (!ar) return;
	sol::this_state s = L;
	sol::state_view lua(L);

	//LUA_HOOKLINE://每執行一行
	//LUA_HOOKCALL://函數開頭
	//LUA_MASKCOUNT://每 n 個函數執行一次
	//LUA_HOOKRET://函數結尾

	switch (ar->event)
	{
	case LUA_HOOKCALL:
	{
		//函數入口
		break;
	}
	case LUA_MASKCOUNT:
	{
		//每 n 個函數執行一次
		break;
	}
	case LUA_HOOKRET:
	{
		//函數返回
		break;
	}
	case LUA_HOOKLINE:
	{
		//進入新行
		luadebug::checkStopAndPause(s);

		break;
	}
	default:
		break;
	}
}

//遞歸獲取每一層目錄
void luadebug::getPackagePath(const QString base, QStringList* result)
{
	QDir dir(base);
	if (!dir.exists())
		return;
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	QFileInfoList list = dir.entryInfoList();
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);
		result->append(fileInfo.filePath());
		getPackagePath(fileInfo.filePath(), result);
	}
}

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool luadebug::waitfor(const sol::this_state& s, qint64 timeout, std::function<bool()> exprfun)
{
	if (timeout < 0)
		timeout = std::numeric_limits<qint64>::max();

	Injector& injector = Injector::getInstance();
	bool bret = false;
	QElapsedTimer timer; timer.start();
	for (;;)
	{
		checkStopAndPause(s);

		if (isInterruptionRequested(s))
			break;

		if (timer.hasExpired(timeout))
			break;

		if (injector.server.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		QThread::msleep(100);
	}
	return bret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLua::CLua(QObject* parent)
	: ThreadPlugin(parent)
{

}

CLua::CLua(const QString& content, QObject* parent)
	: ThreadPlugin(parent)
	, scriptContent_(content)
{
	qDebug() << "CLua";
}

CLua::~CLua()
{
	qDebug() << "~CLua";
}

void CLua::start()
{
	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	moveToThread(thread_);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(this, &CLua::finished, thread_, &QThread::quit);
	connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
	connect(thread_, &QThread::started, this, &CLua::proc);
	connect(this, &CLua::finished, this, [this]()
		{
			requestInterruption();
			thread_->requestInterruption();
			thread_->quit();
			thread_->wait();
			thread_ = nullptr;
			qDebug() << "CLua::finished";
		});


	thread_->start();
}

void CLua::wait()
{
	if (nullptr != thread_)
		thread_->wait();
}

void CLua::open_enumlibs()
{
	//添加lua枚舉值示範
	enum TemplateEnum
	{
		EnumValue1,
		EnumValue2,
		EnumValue3
	};

	lua_.new_enum<TemplateEnum>("TEST",
		{
			{ "EnumValue1", TemplateEnum::EnumValue1 },
			{ "EnumValue2", TemplateEnum::EnumValue2 },
			{ "EnumValue3", TemplateEnum::EnumValue3 }
		}
	);

	//在lua中的使用方法: TEST.EnumValue1
}

//以Metatable方式註冊函數 支援函數多載、運算符重載，面向對象，常量
void CLua::open_testlibs()
{
	sol::usertype<CLuaTest> test = lua_.new_usertype<CLuaTest>("Test",
		sol::call_constructor,
		sol::constructors<
		/*建構函數多載*/
		CLuaTest(sol::this_state),
		CLuaTest(lua_Integer a, sol::this_state)>(),

		//成員函數多載
		"add", sol::overload(
			sol::resolve<lua_Integer(lua_Integer a, lua_Integer b)>(&CLuaTest::add),
			sol::resolve<lua_Integer(lua_Integer b)>(&CLuaTest::add)
		),

		//成員函數聲明
		"sub", &CLuaTest::sub
	);

	/*
	在lua中的使用方法:

	local test = Test(1)
	print(test:add(1, 2))
	print(test:add(2))
	print(test:sub(1, 2))

	*/
}

//luasystem.cpp
void CLua::open_systemlibs()
{
	sol::usertype<CLuaSystem> sys = lua_.new_usertype<CLuaSystem>("System",
		sol::call_constructor,
		sol::constructors<CLuaSystem()>(),

		"sleep", &CLuaSystem::sleep,
		"logout", &CLuaSystem::logout,
		"logback", &CLuaSystem::logback,
		"eo", &CLuaSystem::eo,
		"print", sol::overload(
			sol::resolve<lua_Integer(sol::object ostr, sol::this_state s)>(&CLuaSystem::announce),
			sol::resolve<lua_Integer(sol::object ostr, lua_Integer color, sol::this_state s)>(&CLuaSystem::announce),
			sol::resolve<lua_Integer(std::string format, sol::object ostr, lua_Integer color, sol::this_state s)>(&CLuaSystem::announce)
		),

		"msg", &CLuaSystem::messagebox,
		"say", &CLuaSystem::talk,
		"cls", &CLuaSystem::cleanchat,
		"menu", sol::overload(
			sol::resolve<lua_Integer(lua_Integer index, sol::this_state s)>(&CLuaSystem::menu),
			sol::resolve<lua_Integer(lua_Integer type, lua_Integer index, sol::this_state s)>(&CLuaSystem::menu)
		),

		"saveset", &CLuaSystem::savesetting,
		"loadset", &CLuaSystem::loadsetting,
		"button", sol::overload(
			sol::resolve<lua_Integer(const std::string & buttonStr, sol::object onpcName, sol::object odlgId, sol::this_state s)>(&CLuaSystem::press),
			sol::resolve<lua_Integer(lua_Integer row, sol::object onpcName, sol::object odlgId, sol::this_state s)>(&CLuaSystem::press)
		),

		"input", &CLuaSystem::input,
		"set", &CLuaSystem::set
	);
}

void CLua::proc()
{
	do
	{
		if (scriptContent_.simplified().isEmpty())
			break;

		lua_State* L = lua_.lua_state();

		lua_.set("_THIS", this);// 將this指針傳給lua設置全局變量
		lua_.set("_THIS_PARENT", parent_);// 將父類指針傳給lua設置全局變量
		lua_.set("_INDEX", index_);

		//打開lua原生庫
		lua_.open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::os,
			sol::lib::string,
			sol::lib::math,
			sol::lib::table,
			sol::lib::debug,
			sol::lib::utf8,
			sol::lib::coroutine,
			sol::lib::io
		);

		open_enumlibs();
		open_testlibs();
		open_systemlibs();

		//執行短腳本
		lua_.safe_script(R"(
			collectgarbage("setpause", 100)
			collectgarbage("setstepmul", 100);
			collectgarbage("step", 1024);
		)");

		//回收垃圾
		lua_.collect_garbage();

		//Add additional package path.
		QStringList paths;
		std::string package_path = lua_["package"]["path"];
		paths.append(QString::fromUtf8(package_path.c_str()).replace("\\", "/"));

		QStringList dirs;
		luadebug::getPackagePath(util::applicationDirPath() + "/", &dirs);
		for (const QString& it : dirs)
		{
			QString path = it + "/?.lua";
			paths.append(path.replace("\\", "/"));
		}

		lua_["package"]["path"] = std::string(paths.join(";").toUtf8().constData());

#ifdef OPEN_HOOK
		lua_sethook(L, &luadebug::hookProc, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, NULL);// | LUA_MASKCOUNT 
#endif

		QStringList tableStrs;
		std::string luaCode = scriptContent_.toUtf8();

		//安全模式下執行lua腳本
		sol::protected_function_result loaded_chunk = lua_.safe_script(luaCode.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();

		if (!loaded_chunk.valid())
		{
			sol::error err("\0");
			QString qstrErr("\0");
			bool errOk = false;
			try
			{
				err = loaded_chunk;
				qstrErr = QString::fromUtf8(err.what());
				errOk = true;
			}
			catch (...)
			{
				if (!isSubScript_)
					tableStrs << "[fatal]:" + tr("========== lua error result with an EXCEPTION ==========");
				else
					tableStrs << "[fatal]: SUBTHREAD | " + tr("========== lua error result with an EXCEPTION ==========");

				if (errOk)
				{
					int retline = -1;
					QString msg(luadebug::getErrorMsgLocatedLine(qstrErr, &retline));

					if (msg.contains("FLAG_DETECT_STOP")
						|| msg.contains("REQUEST_STOP_FROM_USER")
						|| msg.contains("REQUEST_STOP_FROM_SCRIP")
						|| msg.contains("REQUEST_STOP_FROM_PARENT_SCRIP")
						|| msg.contains("REQUEST_STOP_FROM_DISTRUCTOR"))
					{
						if (msg.contains("FLAG_DETECT_STOP"))
							tableStrs << tr("> lua script stop by flag change to false");
						else if (msg.contains("REQUEST_STOP_FROM_USER"))
							tableStrs << tr("> lua script stop with user request");
						else if (msg.contains("REQUEST_STOP_FROM_SCRIP"))
							tableStrs << tr("> lua script stop from script request");
						else if (msg.contains("REQUEST_STOP_FROM_PARENT_SCRIP"))
							tableStrs << tr("> lua script stop from parent script request");
						else if (msg.contains("REQUEST_STOP_FROM_DISTRUCTOR"))
							tableStrs << tr("> lua script stop from it's distructor");
						//SPD_LOG(GLOBAL_LOG_ID, QString("%1 [index:%2] Lua Info:%3").arg(__FUNCTION__).arg(m_index).arg(msg));
						//emit this->addErrorMarker(retline, true);
						if (isDebug_)
							tableStrs << tr("> message: ");
						tableStrs << "> [info]:" + msg;
					}
					else
					{
						tableStrs << tr("========== lua script stop with an ERROR ==========");
						//SPD_LOG(GLOBAL_LOG_ID, QString("%1 [index:%2] Lua Warn:%3").arg(__FUNCTION__).arg(m_index).arg(msg), SPD_WARN);
						if (isDebug_)
							tableStrs << tr("> reason: ");
						tableStrs << "> [error]:" + msg;
					}

					//emit this->logMessageExport(s, );
					if (isDebug_ && !isSubScript_)
						tableStrs << ">";
				}
			}
		}
		else
		{
			if (isDebug_)
			{
				//get script return value
				tableStrs << tr("========== lua script normally end ==========");
				tableStrs << tr("> return value:");
				sol::object retObject;

				try
				{
					retObject = loaded_chunk;
				}
				catch (...)
				{
					if (!isSubScript_)
						tableStrs << "[fatal]:" + tr("========== lua normal result with EXCEPTION ==========");
					else
						tableStrs << "[fatal]: SUBTHREAD | " + tr("========== lua normal result with EXCEPTION ==========");
				}
				if (retObject.is<bool>())
				{
					tableStrs << QString("> (boolean)%1").arg(retObject.as<bool>() ? "true" : "false");
				}
				else if (retObject.is<int>())
				{
					tableStrs << "> (integer)" + QString::number(retObject.as<int>());

				}
				else if (retObject.is<double>())
				{
					tableStrs << "> (number)" + QString::number(retObject.as<double>(), 'f', 16);
				}
				else if (retObject.is<std::string>())
				{
					tableStrs << "> (string)" + QString::fromUtf8(retObject.as<const char*>());
				}
				else if (retObject == sol::lua_nil)
				{
					tableStrs << "> (nil)";
				}
				else if (retObject.is<sol::table>())
				{
					sol::table t = retObject.as<sol::table>();

					tableStrs << "> {";
					for (const std::pair<sol::object, sol::object>& it : t)
					{
						sol::object key = it.first;
						sol::object val = it.second;
						if (!key.valid() || !val.valid()) continue;
						if (!key.is<std::string>() && !key.is<int>()) continue;
						QString qkey = key.is<std::string>() ? QString::fromUtf8(key.as<const char*>()) : QString::number(key.as<int>());

						if (val.is<bool>())
						{
							tableStrs << QString(R"(>     ["%1"] = (boolean)%2,)").arg(qkey).arg(val.as<bool>() ? "true" : "false");
						}
						else if (val.is<int>())
						{
							tableStrs << QString(R"(>     ["%1"] = (integer)%2,)").arg(qkey).arg(val.as<int>());
						}
						else if (val.is<double>())
						{
							tableStrs << QString(R"(>     ["%1"] = (number)%2,)").arg(qkey).arg(val.as<double>());
						}
						else if (val.is<std::string>())
						{
							tableStrs << QString(R"(>     ["%1"] = (string)%2,)").arg(qkey).arg(QString::fromUtf8(val.as<const char*>()));
						}
						else if (val.is<sol::table>())
						{
							tableStrs << QString(R"(>     ["%1"] = %2,)").arg(qkey).arg("table");
						}
						else
						{
							tableStrs << QString(R"(>     ["%1"] = %2,)").arg(qkey).arg("unknown");
						}

					}
					tableStrs << "> }";
				}
				else
				{
					tableStrs << tr("> (unknown type of data)");
				}
				tableStrs << ">";
			}
		}

		//QLuaDebuger::logMessageExportEx(s, table);
	} while (false);

	emit finished();
}