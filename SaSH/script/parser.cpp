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
#include <gamedevice.h>
#include "interpreter.h"

//"調用" 傳參數最小佔位
constexpr long long kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr long long kFormatPlaceHoldSize = 3;

#pragma region  LuaTools
static void __fastcall makeTable(sol::state& lua, const char* name, long long i, long long j)
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

static void __fastcall makeTable(sol::state& lua, const char* name, long long i)
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

static void __fastcall makeTable(sol::state& lua, const char* name)
{
	if (!lua[name].valid())
		lua[name] = lua.create_table();
	else if (!lua[name].is<sol::table>())
		lua[name] = lua.create_table();
}
#pragma endregion

static void hookProc(lua_State* L, lua_Debug* ar)
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

		if (!lua["__THIS_PARSER"].valid())
			return;

		Parser* pparser = lua["__THIS_PARSER"].get<Parser*>();
		if (pparser == nullptr)
			return;

		lua.set("LINE", pparser->getCurrentLine() + 1);

		//獲取區域變量數值
		long long i;
		const char* name = nullptr;

		long long tmpIndex = 1;
		long long ctmpIndex = 1;
		for (i = 1; (name = lua_getlocal(L, ar, i)) != nullptr; ++i)
		{
			QString key = util::toQString(name);
			if (g_sysConstVarName.contains(key))
				continue;

			QVariant value;
			long long depth = kMaxLuaTableDepth;
			QPair<QString, QVariant> pair = luadebug::getVars(L, i, depth);
			if (key == "(temporary)")
				key = QString("temporary_%1").arg(tmpIndex++);
			else if (key == "(C temporary)")
				key = QString("c_temporary_%1").arg(ctmpIndex++);
			else if (key.startsWith("_"))
				continue;
			else if (key.startsWith("("))
				continue;
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
	, counter_(q_check_ptr(new Counter()))
	, globalNames_(q_check_ptr(new QStringList()))
	, localVarStack_(q_check_ptr(new QStack<QHash<QString, QVariant>>()))
	, luaLocalVarStringList_(q_check_ptr(new QStringList()))
	, pLua_(q_check_ptr(new CLua(index)))
{
	sash_assume(counter_ != nullptr);
	sash_assume(globalNames_ != nullptr);
	sash_assume(localVarStack_ != nullptr);
	sash_assume(luaLocalVarStringList_ != nullptr);
	sash_assume(pLua_ != nullptr);
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

	if (nullptr == counter_)
		counter_.reset(q_check_ptr(new Counter()));

	sash_assume(counter_ != nullptr);

	if (nullptr == globalNames_)
		globalNames_.reset(q_check_ptr(new QStringList()));

	sash_assume(globalNames_ != nullptr);

	if (nullptr == localVarStack_)
		localVarStack_.reset(q_check_ptr(new QStack<QHash<QString, QVariant>>()));

	sash_assume(localVarStack_ != nullptr);

	if (nullptr == luaLocalVarStringList_)
		luaLocalVarStringList_.reset(q_check_ptr(new QStringList()));

	sash_assume(luaLocalVarStringList_ != nullptr);

	if (nullptr == pLua_)
		pLua_.reset(q_check_ptr(new CLua(index)));

	sash_assume(pLua_ != nullptr);

	pLua_->setHookEnabled(false);

	pLua_->openlibs();

	pLua_->setHookForStop(true);

	sol::state& lua_ = pLua_->getLua();

	makeTable(lua_, "unit", 2000);
	makeTable(lua_, "magic", sa::MAX_MAGIC);
	makeTable(lua_, "petskill", sa::MAX_PET, sa::MAX_PET_SKILL);
	makeTable(lua_, "petequip", sa::MAX_PET, sa::MAX_PET_ITEM);
	makeTable(lua_, "point");
	makeTable(lua_, "mails", sa::MAX_ADDRESS_BOOK, sa::MAIL_MAX_HISTORY);

	insertGlobalVar("INDEX", index);

	if (globalNames_->isEmpty())
	{
		*globalNames_ = g_sysConstVarName;
	}

#pragma region init

	lua_.set("FILE", util::toConstData(getScriptFileName()));

	if (!callStack_.isEmpty())
		lua_.set("FUNCTION", util::toConstData(callStack_.top().name));
	else
		lua_.set("FUNCTION", sol::lua_nil);

	QString path = getScriptFileName();
	QFileInfo info = QFileInfo(path);
	lua_.set("CURRENTSCRIPTDIR", util::toConstData(info.absolutePath()));

	lua_.set_function("format", [this](std::string sformat, sol::this_state s)->std::string
		{
			QString formatStr = util::toQString(sformat);
			if (formatStr.isEmpty())
				return sformat;

			static const QRegularExpression rexFormat(R"(\{\s*(?:([CT]?))\s*:\s*([^}]+)\s*\})");
			if (!formatStr.contains(rexFormat))
				return sformat;

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
					QString varStr = match.captured(2);
					QVariant varValue;

					if (type == "T")
					{
						varValue = luaDoString(QString("return %1;").arg(varStr));
						if (varValue.toString() != "nil")
							formatVarList.insert(varStr, qMakePair(FormatType::kTime, util::formatSeconds(varValue.toLongLong())));
						else
							formatVarList.insert(varStr, qMakePair(FormatType::kNothing, QString()));
					}
					else if (type == "C")
					{
						QString str;
						if (varStr.toLower() == "date")
						{
							const QDateTime dt = QDateTime::currentDateTime();
							str = dt.toString("yyyy-MM-dd");

						}
						else if (varStr.toLower() == "time")
						{
							const QDateTime dt = QDateTime::currentDateTime();
							str = dt.toString("hh:mm:ss:zzz");
						}

						formatVarList.insert(varStr, qMakePair(FormatType::kConst, str));
					}
					else
					{
						varValue = luaDoString(QString("return %1;").arg(varStr));
						if (varValue.toString() != "nil")
							formatVarList.insert(varStr, qMakePair(FormatType::kNothing, varValue.toString()));
						else
							formatVarList.insert(varStr, qMakePair(FormatType::kNothing, QString()));
					}
				}

				if (formatVarList.isEmpty())
					continue;

				for (auto it = formatVarList.constBegin(); it != formatVarList.constEnd(); ++it)
				{
					const QString key = it.key();
					const QPair<FormatType, QString> varValue = it.value();
					QString preReplace = "";
					switch (varValue.first)
					{
					case FormatType::kNothing:
						preReplace = QString("{:%1}").arg(key);
						break;
					case FormatType::kTime:
						preReplace = QString("{T:%1}").arg(key);
						break;
					case FormatType::kConst:
						preReplace = QString("{C:%1}").arg(key);
						break;
					default:
						continue;
					}

					formatStr.replace(preReplace, varValue.second);
				}
			}

			return util::toConstData(formatStr);

		});

	lua_State* L = lua_.lua_state();
	lua_["__THIS_PARSER"] = pparent;
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
		long long currentLine = getCurrentLine() + 1;
		if (extMsg.contains("FLAG_DETECT_STOP"))
		{
			emit signalDispatcher.appendScriptLog(QObject::tr("[info]") + QObject::tr("@ %1 | detail:%2").arg(currentLine).arg("stop by user"), 6);
			return;
		}
		--counter_->validCommand;
		++counter_->error;
		QString cmd = currentLineTokens_.value(0).data.toString();
		constexpr long long kMaxLuaErrorLength = 128;
		msg = cmd + extMsg;
		if (msg.size() < kMaxLuaErrorLength)
			emit signalDispatcher.appendScriptLog(QObject::tr("[error]") + QObject::tr("@ %1 | detail:%2").arg(currentLine).arg(msg), 6);
		else
		{
			//split to multi line each kMaxLuaErrorLength char
			QString left = msg;
			while (left.size() > kMaxLuaErrorLength)
			{
				QString line = left.left(kMaxLuaErrorLength);
				emit signalDispatcher.appendScriptLog(QObject::tr("[error]") + QObject::tr("@ %1 | detail:%2").arg(currentLine).arg(line), 6);
				left = left.mid(kMaxLuaErrorLength);
			}

			if (!left.isEmpty())
				emit signalDispatcher.appendScriptLog(QObject::tr("[error]") + QObject::tr("@ %1 | detail:%2").arg(currentLine).arg(left), 6);
		}
		return;
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

	if (nullptr == pLua_)
		return "nil";

	importLocalVariablesToPreLuaList();//將局變量倒出成lua格式列表
	try { updateSysConstKeyword(expr); } //更新系統變量
	catch (...) { return "nil"; }
	checkConditionalOperator(expr); //替換三目語法

	QStringList localVars = *luaLocalVarStringList_;
	localVars.append(expr);
	QString exprStr = localVars.join("\n");

	QString luaFunction = QString(R"(
			local __success, __result = pcall(function()
				%1
			end)
			if not __success then
				error(__result)
			else
				return __result
			end
	)").arg(exprStr);

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
		GameDevice& gamedevice = GameDevice::getInstance(getIndex());
		gamedevice.stopScript();
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
	else if (retObject.is<sol::table>() && !retObject.is<sol::userdata>() && !retObject.is<sol::function>() && !retObject.is<sol::lightuserdata>())
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
	if (nullptr == pLua_)
		return false;

	std::string sname(util::toConstData(name));
	sol::state& lua_ = pLua_->getLua();
	if (lua_[sname.c_str()].valid())
		return true;

	return false;
}

QVariant Parser::getGlobalVarValue(const QString& name)
{
	if (nullptr == pLua_)
		return QVariant("nil");

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
	else if (obj.is<sol::table>() && !obj.is<sol::userdata>() && !obj.is<sol::function>() && !obj.is<sol::lightuserdata>())
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

		if (g_sysConstVarName.contains(key))
			continue;

		strvalue = it.value().toString();
		type = it.value().type();
		if (type == QVariant::String && !strvalue.startsWith("{") && !strvalue.endsWith("}"))
			strvalue = QString("[[%1]]").arg(strvalue);
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
	if (g_lua_exception_var_list.contains(name) || name.startsWith("__"))
		return;

	if (nullptr == pLua_)
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
				GameDevice& gamedevice = GameDevice::getInstance(getIndex());
				gamedevice.stopScript();
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
	if (nullptr == pLua_)
		return;

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

	if (g_lua_exception_var_list.contains(name) || name.startsWith("_"))
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
		if (!isSubScript())
		{
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			gamedevice.stopScript();
		}

		stopByExit_ = true;
		return false;
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

		if (pair.second.is<sol::table>() && !pair.second.is<sol::userdata>() && !pair.second.is<sol::function>() && !pair.second.is<sol::lightuserdata>())
			value = getLuaTableString(pair.second.as<sol::table>(), depth);
		else if (pair.second.is<std::string>())
			value = QString("[[%1]]").arg(util::toQString(pair.second));
		else if (pair.second.is<long long>())
			value = util::toQString(pair.second.as<long long>());
		else if (pair.second.is<double>())
			value = util::toQString(pair.second.as<double>());
		else if (pair.second.is<bool>())
			value = util::toQString(pair.second.as<bool>());
		else
			value = "nil";

		if (value.isEmpty())
			value = "[[]]";

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
	currentField.clear();
	callStack_.clear();
	jmpStack_.clear();
	forStack_.clear();
	callArgsStack_.clear();

	if (isSubScript() && getMode() == Parser::kSync)
		return;

	counter_.reset();
	globalNames_.reset();
	localVarStack_.reset();
	luaLocalVarStringList_.reset();

	if (pLua_ != nullptr)
	{
		sol::state& lua = pLua_->getLua();
		lua.collect_garbage();
	}

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
		opStr.remove("=");

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
	QVariant value = luaDoString(exprStr);

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
		varNameStr.remove("local");
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
			long long color;
			util::rnd::get(&color, 0LL, 10LL);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				long long nColor = str.toLongLong();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			long long currentIndex = getIndex();
			GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
			if (gamedevice.log.isOpen())
				gamedevice.log.write(formatedStr, getCurrentLine());

			if (!gamedevice.worker.isNull())
				gamedevice.worker->announce(formatedStr, color);

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
			long long color;
			util::rnd::get(&color, 0LL, 10LL);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				long long nColor = str.toLongLong();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			long long currentIndex = getIndex();
			GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
			if (!gamedevice.worker.isNull())
				gamedevice.worker->talk(formatedStr, color);
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
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long extraDelay = gamedevice.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000ll)
	{
		//將超過1秒的延時分段
		long long i = 0ll;
		long long size = extraDelay / 1000ll;
		for (i = 0; i < size; ++i)
		{
			if (gamedevice.IS_SCRIPT_INTERRUPT.get())
				return;

			QThread::msleep(1000);
		}
		if (extraDelay % 1000ll > 0ll)
			QThread::msleep(extraDelay % 1000);
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
	if (nullptr == pLua_)
		return false;

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

		if (g_lua_exception_var_list.contains(key) || key.startsWith("_"))
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
		local __success, __result = pcall(function()
			%1
		end)
		if not __success then
			error(__result)
		else
			return __result
		end
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
	if (nullptr == pLua_)
		return;

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

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

	util::timer timer;

	for (;;)
	{
		//用戶中斷
		if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		{
			break;
		}

		//到結尾
		if (empty())
		{
			break;
		}

		//經由命令而停止
		if (stopByExit_)
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
			if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get() && gamedevice.IS_SCRIPT_EDITOR_OPENED.get())
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
			if (!isSubScript())
			{
				GameDevice& gamedevice = GameDevice::getInstance(getIndex());
				gamedevice.stopScript();
			}
			stopByExit_ = true;
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
				gamedevice.stopScript();
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

		if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		{
			break;
		}

		if (stopByExit_)
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
		if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get() && !isSubScript())
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
				.arg("'" + path + "' " + (isSubScript() ? QObject::tr("sub-ok") : QObject::tr("main-ok"))).arg(util::formatMilliseconds(timer.cost())));
			gamedevice.log.close();
		}
	}

	processClean();
}

//導出變量訊息
void Parser::exportVarInfo()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())//檢查是否為調試模式
		return;

	if (!gamedevice.IS_SCRIPT_EDITOR_OPENED.get())//檢查編輯器是否打開
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

	if (expr.contains("FILE"))
	{
		lua_.set("FILE", util::toConstData(getScriptFileName()));
	}

	if (expr.contains("FUNCTION"))
	{
		if (!callStack_.isEmpty())
			lua_.set("FUNCTION", util::toConstData(callStack_.top().name));
		else
			lua_.set("FUNCTION", sol::lua_nil);
	}

	if (expr.contains("CURRENTSCRIPTDIR"))
	{
		QString path = getScriptFileName();
		QFileInfo info = QFileInfo(path);
		lua_.set("CURRENTSCRIPTDIR", util::toConstData(info.absolutePath()));
	}

	if (expr.contains("SCRIPTDIR"))
	{
		lua_.set("SCRIPTDIR", util::toConstData(QString("%1/script").arg(util::applicationDirPath())));
	}

	if (expr.contains("SETTINGDIR"))
	{
		lua_.set("SETTINGDIR", util::toConstData(QString("%1/settings").arg(util::applicationDirPath())));
	}

	if (expr.contains("CURRENTDIR"))
	{
		lua_.set("CURRENTDIR", util::toConstData(util::applicationDirPath()));
	}

	if (expr.contains("PID"))
	{
		lua_.set("PID", static_cast<long long>(_getpid()));
	}

	if (expr.contains("THREADID"))
	{
		lua_.set("THREADID", reinterpret_cast<long long>(QThread::currentThreadId()));
	}

	long long currentIndex = getIndex();
	if (lua_["__INDEX"].valid() && lua_["__INDEX"].is<long long>())
	{
		long long tempIndex = lua_["__INDEX"].get<long long>();

		if (tempIndex >= 0 && tempIndex < SASH_MAX_THREAD)
			currentIndex = tempIndex;
	}

	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (expr.contains("HWND"))
	{
		lua_.set("HWND", reinterpret_cast<long long>(gamedevice.getParentWidget()));
	}

	if (expr.contains("GAMEPID"))
	{
		lua_.set("GAMEPID", gamedevice.getProcessId());
	}

	if (expr.contains("GAMEHWND"))
	{
		lua_.set("GAMEPID", reinterpret_cast<long long>(gamedevice.getProcessWindow()));
	}

	if (expr.contains("GAMEHANDLE"))
	{
		lua_.set("GAMEHANDLE", reinterpret_cast<long long>(gamedevice.getProcess()));
	}

	if (expr.contains("INDEX"))
	{
		lua_.set("INDEX", gamedevice.getIndex());
	}

	/////////////////////////////////////////////////////////////////////////////////////
	if (gamedevice.worker.isNull())
		return;

	//unit\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("unit") || expr.contains("unit["))
	{
		QList<sa::map_unit_t> units = gamedevice.worker->mapUnitHash.values();

		long long size = units.size();

		sol::meta::unqualified_t<sol::table> unit = lua_["unit"];

		unit["count"] = size;

		for (long long i = 0; i < size; ++i)
		{
			sa::map_unit_t u = units.value(i);
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

		unit["find"] = [this, currentIndex](sol::object oname, sol::object ofname, sol::this_state s)->sol::object
			{
				sol::state_view lua(s);
				GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
				if (gamedevice.worker.isNull())
					return sol::lua_nil;

				QString name = "";
				long long modelid = -1;
				if (oname.is<std::string>())
					name = util::toQString(oname);
				if (oname.is<long long>())
					modelid = oname.as<long long>();

				QString freeName = "";
				if (ofname.is<std::string>())
					freeName = util::toQString(ofname);

				if (name.isEmpty() && modelid == 0 && freeName.isEmpty())
					return sol::lua_nil;

				sa::map_unit_t _unit = {};
				if (!gamedevice.worker->findUnit(name, sa::kObjectNPC, &_unit, freeName, modelid))
				{
					if (!gamedevice.worker->findUnit(name, sa::kObjectHuman, &_unit, freeName, modelid))
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

				return sol::make_object(lua, t);
			};
	}

	//magic\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("magic") || expr.contains("magic["))
	{
		sol::meta::unqualified_t<sol::table> mg = lua_["magic"];

		for (long long i = 0; i < sa::MAX_MAGIC; ++i)
		{
			long long index = i + 1;
			sa::magic_t magic = gamedevice.worker->getMagic(i);

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
				GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
				if (gamedevice.worker.isNull())
					return 0;

				QString name = "";
				if (oname.is<std::string>())
					name = util::toQString(oname);

				if (name.isEmpty())
					return sol::lua_nil;


				long long index = gamedevice.worker->getMagicIndexByName(name);
				if (index == -1)
					return sol::lua_nil;

				sa::magic_t _magic = gamedevice.worker->getMagic(index);

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

	//petskill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petskill") || expr.contains("petskill["))
	{
		sol::meta::unqualified_t<sol::table> psk = lua_["petskill"];

		long long petIndex = -1;
		long long index = -1;
		long long i, j;
		for (i = 0; i < sa::MAX_PET; ++i)
		{
			petIndex = i + 1;

			for (j = 0; j < sa::MAX_PET_SKILL; ++j)
			{
				index = j + 1;
				sa::pet_skill_t skill = gamedevice.worker->getPetSkill(i, j);

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
				GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
				if (gamedevice.worker.isNull())
					return 0;

				QString name = "";
				if (oname.is<std::string>())
					name = util::toQString(oname);

				if (name.isEmpty())
					return sol::lua_nil;

				long long index = gamedevice.worker->getPetSkillIndexByName(petIndex, name);
				if (index == -1)
					return sol::lua_nil;

				sa::pet_skill_t _skill = gamedevice.worker->getPetSkill(petIndex, index);

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
	if (expr.contains("petequip") || expr.contains("petequip["))
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

				sa::item_t item = gamedevice.worker->getPetEquip(i, j);

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

	if (expr.contains("point"))
	{
		sa::currency_data_t point = gamedevice.worker->currencyData.get();

		sol::meta::unqualified_t<sol::table> pt = lua_["point"];

		pt["exp"] = point.expbufftime;
		pt["rep"] = point.prestige;
		pt["ene"] = point.energy;
		pt["shl"] = point.shell;
		pt["vit"] = point.vitality;
		pt["pts"] = point.points;
		pt["vip"] = point.VIPPoints;
	}

	if (expr.contains("mails") || expr.contains("mails["))
	{
		sol::meta::unqualified_t<sol::table> mails = lua_["mails"];
		long long j = 0;
		for (long long i = 0; i < sa::MAX_ADDRESS_BOOK; ++i)
		{
			long long card = i + 1;
			sa::mail_history_t mail = gamedevice.worker->getMailHistory(i);
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