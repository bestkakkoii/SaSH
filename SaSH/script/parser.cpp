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

extern QString g_logger_name;

//"調用" 傳參數最小佔位
constexpr qint64 kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr qint64 kFormatPlaceHoldSize = 3;

void makeTable(sol::state& lua, const char* name, qint64 i, qint64 j)
{
	if (!lua[name].valid() || !lua[name].is<sol::table>())
		lua[name] = lua.create_table();
	qint64 k, l;

	for (k = i; k <= i; ++k)
	{
		if (!lua[name][k].valid() || !lua[name][k].is<sol::table>())
			lua[name][k] = lua.create_table();

		for (l = i; l <= j; ++l)
		{
			if (!lua[name][k][l].valid() || !lua[name][k][l].is<sol::table>())
				lua[name][k][l] = lua.create_table();
		}
	}
}

void makeTable(sol::state& lua, const char* name, qint64 i)
{
	if (!lua[name].valid() || !lua[name].is<sol::table>())
		lua[name] = lua.create_table();
	qint64 k;
	for (k = 1; k <= i; ++k)
	{
		if (!lua[name][k].valid() || !lua[name][k].is<sol::table>())
			lua[name][k] = lua.create_table();

		lua[name][k] = lua.create_table();
	}
}

Parser::Parser()
	: ThreadPlugin(nullptr)
{
	qDebug() << "Parser is created!!";
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &Parser::requestInterruption, Qt::UniqueConnection);

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

	lua_.set_function("input", [this](sol::object oargs, sol::this_state s)->sol::object
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

			std::string sargs = "";
			if (oargs.is<std::string>())
				sargs = oargs.as<std::string>();

			QString args = QString::fromUtf8(sargs.c_str());

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
				return sol::lua_nil;
			}

			if (type == QInputDialog::IntInput)
				return sol::make_object(s, var.toLongLong());
			else if (type == QInputDialog::DoubleInput)
				return sol::make_object(s, var.toDouble());
			else
			{
				QString str = var.toString();
				if (str.toLower() == "true" || str.toLower() == "false" || str.toLower() == "真" || str.toLower() == "假")
					return sol::make_object(s, var.toBool());

				return sol::make_object(s, var.toString().toUtf8().constData());
			}
		});


	lua_.set_function("format", [this](std::string sformat, sol::this_state s)->sol::object
		{
			QString formatStr = QString::fromUtf8(sformat.c_str());
			if (formatStr.isEmpty())
				return sol::lua_nil;

			static const QRegularExpression rexFormat(R"(\{([T|C])?\s*\:([\w\p{Han}]+(\['*"*[\w\p{Han}]+'*"*\])*)\})");
			if (!formatStr.contains(rexFormat))
				return sol::make_object(s, sformat.c_str());

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

			return sol::make_object(s, formatStr.toUtf8().constData());

		});
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
		in.setCodec(util::DEFAULT_CODEPAGE);
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

	bool hasLocalHash = false;
	if (variables_ == nullptr)
	{
		variables_ = new VariantSafeHash();
		if (variables_ == nullptr)
			return;
		hasLocalHash = true;
	}

	bool hasGlobalHash = false;
	if (globalVarLock_ == nullptr)
	{
		globalVarLock_ = new QReadWriteLock();
		if (globalVarLock_ == nullptr)
			return;
		hasGlobalHash = true;
	}

	processTokens();

	if (!isSubScript_ || hasLocalHash || hasGlobalHash)
	{
		if (variables_ != nullptr && hasLocalHash)
		{
			delete variables_;
			variables_ = nullptr;
		}

		if (globalVarLock_ != nullptr && hasGlobalHash)
		{
			delete globalVarLock_;
			globalVarLock_ = nullptr;
		}
	}
}

//處理錯誤
void Parser::handleError(qint64 err, const QString& addition)
{
	if (err == kNoChange)
		return;

	QString extMsg;
	if (!addition.isEmpty())
		extMsg = " " + addition;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (err == kError)
		emit signalDispatcher.addErrorMarker(getCurrentLine(), err);

	QString msg;
	switch (err)
	{
	case kError:
	{
		msg = QObject::tr("unknown error") + extMsg;
		break;
	}
	case kServerNotReady:
	{
		msg = QObject::tr("server not ready") + extMsg;
		break;
	}
	case kLabelError:
	{
		msg = QObject::tr("label incorrect or not exist") + extMsg;
		break;
	}
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

//比較兩個 QVariant 以 a 的類型為主
bool Parser::compare(const QVariant& a, const QVariant& b, RESERVE type) const
{
	QVariant::Type aType = a.type();

	if (aType == QVariant::Int || aType == QVariant::LongLong || aType == QVariant::Double)
	{
		qint64 numA = a.toDouble();
		qint64 numB = b.toDouble();

		// 根据 type 进行相应的比较操作
		switch (type)
		{
		case TK_EQ: // "=="
			return (numA == numB);

		case TK_NEQ: // "!="
			return (numA != numB);

		case TK_GT: // ">"
			return (numA > numB);

		case TK_LT: // "<"
			return (numA < numB);

		case TK_GEQ: // ">="
			return (numA >= numB);

		case TK_LEQ: // "<="
			return (numA <= numB);

		default:
			return false; // 不支持的比较类型，返回 false
		}
	}
	else
	{
		QString strA = a.toString().simplified();
		QString strB = b.toString().simplified();

		// 根据 type 进行相应的比较操作
		switch (type)
		{
		case TK_EQ: // "=="
			return (strA == strB);

		case TK_NEQ: // "!="
			return (strA != strB);

		case TK_GT: // ">"
			return (strA.compare(strB) > 0);

		case TK_LT: // "<"
			return (strA.compare(strB) < 0);

		case TK_GEQ: // ">="
			return (strA.compare(strB) >= 0);

		case TK_LEQ: // "<="
			return (strA.compare(strB) <= 0);

		default:
			return false; // 不支持的比较类型，返回 false
		}
	}

	return false; // 不支持的类型，返回 false
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

	const std::string exprStrUTF8 = exprStr.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
	lua_.collect_garbage();
	if (!loaded_chunk.valid())
	{
		sol::error err = loaded_chunk;
		QString errStr = QString::fromUtf8(err.what());
		if (!isGlobalVarContains(QString("_LUAERR@%1").arg(getCurrentLine())))
			insertGlobalVar(QString("_LUA_DO_STR_ERR@%1").arg(getCurrentLine()), exprStr);
		else
		{
			for (qint64 i = 0; i < 1000; ++i)
			{
				if (!isGlobalVarContains(QString("_LUA_DO_STR_ERR@%1%2").arg(getCurrentLine()).arg(i)))
				{
					insertGlobalVar(QString("_LUA_DO_STR_ERR@%1_%2").arg(getCurrentLine()).arg(i), exprStr);
					break;
				}
			}
		}
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
		return QString::fromUtf8(retObject.as<std::string>().c_str());
	else if (retObject.is<qint64>())
		return retObject.as<qint64>();
	else if (retObject.is<double>())
		return retObject.as<double>();
	else if (retObject.is<sol::table>())
	{
		int deep = kMaxLuaTableDepth;
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
	QVariant varValue;
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
			|| preCheck == QString(u8"返回") || expr == QString(u8"跳回") || preCheck == QString(u8"繼續") || preCheck == QString(u8"跳出")
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
void Parser::checkCallArgs(qint64 line)
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QVariantList list;
	if (tokens_.contains(getCurrentLine()))
	{
		TokenMap TK = tokens_.value(getCurrentLine());
		qint64 size = TK.size();
		for (qint64 i = kCallPlaceHoldSize; i < size; ++i)
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
VariantSafeHash* Parser::getGlobalVarPointer() const
{
	if (globalVarLock_ == nullptr)
		return nullptr;

	QReadLocker locker(globalVarLock_);
	return variables_;
}

void Parser::setVariablesPointer(VariantSafeHash* pvariables, QReadWriteLock* plock)
{
	if (!isSubScript())
		return;

	variables_ = pvariables;
	globalVarLock_ = plock;
}

bool Parser::isGlobalVarContains(const QString& name) const
{
	if (globalVarLock_ == nullptr)
		return false;

	QReadLocker locker(globalVarLock_);
	if (variables_ != nullptr)
		return variables_->contains(name);
	return false;
}

QVariant Parser::getGlobalVarValue(const QString& name) const
{
	if (globalVarLock_ == nullptr)
		return QVariant();

	QReadLocker locker(globalVarLock_);
	if (variables_ != nullptr)
		return variables_->value(name, 0);
	return QVariant();
}

//插入全局變量
void Parser::insertGlobalVar(const QString& name, const QVariant& value)
{
	if (globalVarLock_ == nullptr)
		return;

	QWriteLocker locker(globalVarLock_);
	if (variables_ == nullptr)
		return;

	if (!value.isValid())
		return;

	QVariant var = value;

	QVariant::Type type = value.type();
	if (type != QVariant::LongLong && type != QVariant::String
		&& type != QVariant::Double
		&& type != QVariant::Int && type != QVariant::Bool)
	{
		bool ok = false;
		qint64 val = value.toLongLong(&ok);
		if (ok)
			var = val;
		else
			var = false;
	}

	variables_->insert(name, var);
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
	if (globalVarLock_ == nullptr)
		return;

	QWriteLocker locker(globalVarLock_);
	if (variables_ != nullptr)
	{
		variables_->remove(name);
		lua_[name.toUtf8().constData()] = sol::lua_nil;
	}
}

QVariantHash Parser::getGlobalVars()
{
	if (globalVarLock_ == nullptr)
		return QVariantHash();

	QReadLocker locker(globalVarLock_);
	if (variables_ != nullptr)
		return variables_->toHash();
	return QVariantHash();
}

void Parser::clearGlobalVar()
{
	if (globalVarLock_ == nullptr)
		return;

	QWriteLocker locker(globalVarLock_);
	if (variables_ != nullptr)
		variables_->clear();
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

	if (type != QVariant::LongLong && type != QVariant::Int
		&& type != QVariant::Double
		&& type != QVariant::Bool
		&& type != QVariant::String)
	{
		bool ok = false;
		qint64 val = value.toLongLong(&ok);
		if (ok)
			hash.insert(name, val);
		else
			hash.insert(name, false);
		return;
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

	return QVariant();
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

//表達式替換內容
bool Parser::importVariablesToLua(const QString& expr)
{
	QVariantHash globalVars = getGlobalVars();
	for (auto it = globalVars.cbegin(); it != globalVars.cend(); ++it)
	{
		QString key = it.key();
		if (key.isEmpty())
			continue;
		QByteArray keyBytes = key.toUtf8();
		QString value = it.value().toString();
		if (it.value().type() == QVariant::String)
		{
			if (!value.startsWith("{") && !value.endsWith("}"))
			{
				bool ok = false;
				it.value().toLongLong(&ok);
				if (!ok || value.isEmpty())
					lua_.set(keyBytes.constData(), value.toUtf8().constData());
				else
					lua_.set(keyBytes.constData(), it.value().toLongLong());
			}
			else//table
			{
				QString content = QString("%1 = %2;").arg(key).arg(value);
				std::string contentBytes = content.toUtf8().constData();
				sol::protected_function_result loaded_chunk = lua_.safe_script(contentBytes, sol::script_pass_on_error);
				lua_.collect_garbage();
				if (!loaded_chunk.valid())
				{
					sol::error err = loaded_chunk;
					QString errStr = QString::fromUtf8(err.what());
					handleError(kLuaError, errStr);
					return false;
				}
			}
		}
		else if (it.value().type() == QVariant::Double)
			lua_.set(keyBytes.constData(), it.value().toDouble());
		else if (it.value().type() == QVariant::Bool)
			lua_.set(keyBytes.constData(), it.value().toBool());
		else
			lua_.set(keyBytes.constData(), it.value().toLongLong());
	}

	QVariantHash localVars = getLocalVars();
	luaLocalVarStringList.clear();
	for (auto it = localVars.cbegin(); it != localVars.cend(); ++it)
	{
		QString key = it.key();
		if (key.isEmpty())
			continue;
		QString value = it.value().toString();
		QVariant::Type type = it.value().type();
		if (type == QVariant::String && !value.startsWith("{") && !value.endsWith("}"))
		{
			bool ok = false;
			it.value().toInt(&ok);

			if (!ok || value.isEmpty())
				value = QString("'%1'").arg(value);
		}
		else if (type == QVariant::Double)
			value = QString("%1").arg(QString::number(it.value().toDouble(), 'f', 16));
		else if (type == QVariant::Bool)
			value = QString("%1").arg(it.value().toBool() ? "true" : "false");

		QString local = QString("local %1 = %2;").arg(key).arg(value);
		luaLocalVarStringList.append(local);
	}

	return updateSysConstKeyword(expr);
}

//根據表達式更新lua內的變量值
bool Parser::updateSysConstKeyword(const QString& expr)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return false;

	bool bret = false;

	PC _pc = injector.server->getPC();

	//char\.(\w+)
	if (expr.contains("char"))
	{
		bret = true;
		if (!lua_["char"].valid())
			lua_["char"] = lua_.create_table();
		else
		{
			if (!lua_["char"].is<sol::table>())
				lua_["char"] = lua_.create_table();
		}

		lua_["char"]["name"] = _pc.name.toUtf8().constData();

		lua_["char"]["fname"] = _pc.freeName.toUtf8().constData();

		lua_["char"]["lv"] = _pc.level;

		lua_["char"]["hp"] = _pc.hp;

		lua_["char"]["maxhp"] = _pc.maxHp;

		lua_["char"]["hpp"] = _pc.hpPercent;

		lua_["char"]["mp"] = _pc.mp;

		lua_["char"]["maxmp"] = _pc.maxMp;

		lua_["char"]["mpp"] = _pc.mpPercent;

		lua_["char"]["exp"] = _pc.exp;

		lua_["char"]["maxexp"] = _pc.maxExp;

		lua_["char"]["stone"] = _pc.gold;

		lua_["char"]["vit"] = _pc.vit;

		lua_["char"]["str"] = _pc.str;

		lua_["char"]["tgh"] = _pc.tgh;

		lua_["char"]["dex"] = _pc.dex;

		lua_["char"]["atk"] = _pc.atk;

		lua_["char"]["def"] = _pc.def;

		lua_["char"]["chasma"] = _pc.chasma;

		lua_["char"]["turn"] = _pc.transmigration;

		lua_["char"]["earth"] = _pc.earth;

		lua_["char"]["water"] = _pc.water;

		lua_["char"]["fire"] = _pc.fire;

		lua_["char"]["wind"] = _pc.wind;

		lua_["char"]["modelid"] = _pc.modelid;

		lua_["char"]["faceid"] = _pc.faceid;

		lua_["char"]["family"] = _pc.family.toUtf8().constData();

		lua_["char"]["battlepet"] = _pc.battlePetNo + 1;

		lua_["char"]["ridepet"] = _pc.ridePetNo + 1;

		lua_["char"]["mailpet"] = _pc.mailPetNo + 1;

		lua_["char"]["luck"] = _pc.luck;

		lua_["char"]["point"] = _pc.point;
	}

	//pet\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("pet"))
	{
		bret = true;
		makeTable(lua_, "pet", MAX_PET);

		const QHash<QString, PetState> hash = {
			{ u8"battle", kBattle },
			{ u8"standby", kStandby },
			{ u8"mail", kMail },
			{ u8"rest", kRest },
			{ u8"ride", kRide },
		};

		for (int i = 0; i < MAX_PET; ++i)
		{
			PET pet = injector.server->getPet(i);
			int index = i + 1;

			lua_["pet"][index]["valid"] = pet.valid;

			lua_["pet"][index]["name"] = pet.name.toUtf8().constData();

			lua_["pet"][index]["fname"] = pet.freeName.toUtf8().constData();

			lua_["pet"][index]["lv"] = pet.level;

			lua_["pet"][index]["hp"] = pet.hp;

			lua_["pet"][index]["maxhp"] = pet.maxHp;

			lua_["pet"][index]["hpp"] = pet.hpPercent;

			lua_["pet"][index]["exp"] = pet.exp;

			lua_["pet"][index]["maxexp"] = pet.maxExp;

			lua_["pet"][index]["atk"] = pet.atk;

			lua_["pet"][index]["def"] = pet.def;

			lua_["pet"][index]["agi"] = pet.agi;

			lua_["pet"][index]["loyal"] = pet.loyal;

			lua_["pet"][index]["turn"] = pet.transmigration;

			lua_["pet"][index]["earth"] = pet.earth;

			lua_["pet"][index]["water"] = pet.water;

			lua_["pet"][index]["fire"] = pet.fire;

			lua_["pet"][index]["wind"] = pet.wind;

			lua_["pet"][index]["modelid"] = pet.modelid;

			lua_["pet"][index]["pos"] = pet.index;

			lua_["pet"][index]["index"] = index;

			PetState state = pet.state;
			QString str = hash.key(state, "");
			lua_["pet"][index]["state"] = str.toUtf8().constData();

			lua_["pet"][index]["power"] = (static_cast<double>(pet.atk + pet.def + pet.agi) + (static_cast<double>(pet.maxHp) / 4.0));
		}
	}

	//pet\.(\w+)
	if (expr.contains("pet"))
	{
		bret = true;
		if (!lua_["pet"].valid())
			lua_["pet"] = lua_.create_table();
		else
		{
			if (!lua_["pet"].is<sol::table>())
				lua_["pet"] = lua_.create_table();
		}

		lua_["pet"]["count"] = injector.server->getPetSize();
	}

	//item\[(\d+)\]\.(\w+)
	if (expr.contains("item"))
	{
		bret = true;
		makeTable(lua_, "item", MAX_ITEM);

		for (int i = 0; i < MAX_ITEM; ++i)
		{
			ITEM item = _pc.item[i];
			int index = i + 1;
			if (i < CHAR_EQUIPPLACENUM)
				index += 100;
			else
				index = index - CHAR_EQUIPPLACENUM;

			if (!lua_["item"][index].valid())
				lua_["item"][index] = lua_.create_table();
			else
			{
				if (!lua_["item"][index].is<sol::table>())
					lua_["item"][index] = lua_.create_table();
			}

			lua_["item"][index]["valid"] = item.valid;

			lua_["item"][index]["index"] = index;

			lua_["item"][index]["name"] = item.name.toUtf8().constData();

			lua_["item"][index]["memo"] = item.memo.toUtf8().constData();

			if (item.name == "惡魔寶石" || item.name == "恶魔宝石")
			{
				static QRegularExpression rex("(\\d+)");
				QRegularExpressionMatch match = rex.match(item.memo);
				if (match.hasMatch())
				{
					QString str = match.captured(1);
					bool ok = false;
					qint64 dura = str.toLongLong(&ok);
					if (ok)
						lua_["item"][index]["count"] = dura;
				}
			}

			QString damage = item.damage.simplified();
			if (damage.contains("%"))
				damage.replace("%", "");
			if (damage.contains("％"))
				damage.replace("％", "");

			bool ok = false;
			qint64 dura = damage.toLongLong(&ok);
			if (!ok && !damage.isEmpty())
				lua_["item"][index]["dura"] = 100;
			else
				lua_["item"][index]["dura"] = dura;

			lua_["item"][index]["lv"] = item.level;

			if (item.valid && item.stack == 0)
				item.stack = 1;
			else if (!item.valid)
				item.stack = 0;

			lua_["item"][index]["stack"] = item.stack;

			lua_["item"][index]["lv"] = item.level;
			lua_["item"][index]["field"] = item.field;
			lua_["item"][index]["target"] = item.target;
			lua_["item"][index]["type"] = item.type;
			lua_["item"][index]["modelid"] = item.modelid;
			lua_["item"][index]["name2"] = item.name2.toUtf8().constData();
		}
	}

	//item\.(\w+)
	if (expr.contains("item"))
	{
		bret = true;
		if (!lua_["item"].valid())
			lua_["item"] = lua_.create_table();
		else
		{
			if (!lua_["item"].is<sol::table>())
				lua_["item"] = lua_.create_table();
		}

		QVector<int> itemIndexs;
		injector.server->getItemEmptySpotIndexs(&itemIndexs);
		lua_["item"]["space"] = itemIndexs.size();
	}

	//team\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("team"))
	{
		bret = true;
		makeTable(lua_, "team", MAX_PARTY);

		for (int i = 0; i < MAX_PARTY; ++i)
		{
			PARTY party = injector.server->getParty(i);
			int index = i + 1;

			lua_["team"][index]["valid"] = party.valid;

			lua_["team"][index]["index"] = index;

			lua_["team"][index]["id"] = party.id;

			lua_["team"][index]["name"] = party.name.toUtf8().constData();

			lua_["team"][index]["lv"] = party.level;

			lua_["team"][index]["hp"] = party.hp;

			lua_["team"][index]["maxhp"] = party.maxHp;

			lua_["team"][index]["hpp"] = party.hpPercent;

			lua_["team"][index]["mp"] = party.mp;
		}
	}

	//team\.(\w+)
	if (expr.contains("team"))
	{
		bret = true;
		if (!lua_["team"].valid())
			lua_["team"] = lua_.create_table();
		else
		{
			if (!lua_["team"].is<sol::table>())
				lua_["team"] = lua_.create_table();
		}

		lua_["team"]["count"] = static_cast<qint64>(injector.server->getPartySize());
	}

	//map\.(\w+)
	if (expr.contains("map"))
	{
		bret = true;
		if (!lua_["map"].valid())
			lua_["map"] = lua_.create_table();
		else
		{
			if (!lua_["map"].is<sol::table>())
				lua_["map"] = lua_.create_table();
		}

		lua_["map"]["name"] = injector.server->nowFloorName.toUtf8().constData();

		lua_["map"]["floor"] = injector.server->nowFloor;

		lua_["map"]["x"] = injector.server->getPoint().x();

		lua_["map"]["y"] = injector.server->getPoint().y();
	}

	//card\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("card"))
	{
		bret = true;
		makeTable(lua_, "card", MAX_ADDRESS_BOOK);
		for (int i = 0; i < MAX_ADDRESS_BOOK; ++i)
		{
			ADDRESS_BOOK addressBook = injector.server->getAddressBook(i);
			int index = i + 1;

			lua_["card"][index]["valid"] = addressBook.valid;

			lua_["card"][index]["index"] = index;

			lua_["card"][index]["name"] = addressBook.name.toUtf8().constData();

			lua_["card"][index]["online"] = addressBook.onlineFlag ? 1 : 0;

			lua_["card"][index]["turn"] = addressBook.transmigration;

			lua_["card"][index]["dp"] = addressBook.dp;

			lua_["card"][index]["lv"] = addressBook.level;
		}
	}

	//chat\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	if (expr.contains("chat"))
	{
		bret = true;
		makeTable(lua_, "chat", MAX_CHAT_HISTORY);

		for (int i = 0; i < MAX_CHAT_HISTORY; ++i)
		{
			QString text = injector.server->getChatHistory(i);
			int index = i + 1;

			if (!lua_["chat"][index].valid())
				lua_["chat"][index] = lua_.create_table();
			else
			{
				if (!lua_["chat"][index].is<sol::table>())
					lua_["chat"][index] = lua_.create_table();
			}

			lua_["chat"][index] = text.toUtf8().constData();
		}
	}

	//unit\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("unit"))
	{
		bret = true;


		QList<mapunit_t> units = injector.server->mapUnitHash.values();

		int size = units.size();
		makeTable(lua_, "unit", size);
		for (int i = 0; i < size; ++i)
		{
			mapunit_t unit = units.at(i);
			if (!unit.isVisible)
				continue;

			int index = i + 1;

			lua_["unit"][index]["valid"] = unit.isVisible;

			lua_["unit"][index]["index"] = index;

			lua_["unit"][index]["id"] = unit.id;

			lua_["unit"][index]["name"] = unit.name.toUtf8().constData();

			lua_["unit"][index]["fname"] = unit.freeName.toUtf8().constData();

			lua_["unit"][index]["family"] = unit.family.toUtf8().constData();

			lua_["unit"][index]["lv"] = unit.level;

			lua_["unit"][index]["dir"] = unit.dir;

			lua_["unit"][index]["x"] = unit.p.x();

			lua_["unit"][index]["y"] = unit.p.y();

			lua_["unit"][index]["gold"] = unit.gold;

			lua_["unit"][index]["modelid"] = unit.modelid;
		}

		lua_["unit"]["count"] = size;
	}

	//battle\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("battle"))
	{
		bret = true;
		battledata_t battle = injector.server->getBattleData();

		makeTable(lua_, "battle", MAX_ENEMY);

		int size = battle.objects.size();
		for (int i = 0; i < size; ++i)
		{
			battleobject_t obj = battle.objects.at(i);
			int index = i + 1;

			lua_["battle"][index]["valid"] = obj.maxHp > 0 && obj.level > 0 && obj.modelid > 0;

			lua_["battle"][index]["index"] = static_cast<qint64>(obj.pos + 1);

			lua_["battle"][index]["name"] = obj.name.toUtf8().constData();

			lua_["battle"][index]["fname"] = obj.freeName.toUtf8().constData();

			lua_["battle"][index]["modelid"] = obj.modelid;

			lua_["battle"][index]["lv"] = obj.level;

			lua_["battle"][index]["hp"] = obj.hp;

			lua_["battle"][index]["maxhp"] = obj.maxHp;

			lua_["battle"][index]["hpp"] = obj.hpPercent;

			lua_["battle"][index]["status"] = injector.server->getBadStatusString(obj.status).toUtf8().constData();

			lua_["battle"][index]["ride"] = obj.rideFlag > 0 ? 1 : 0;

			lua_["battle"][index]["ridename"] = obj.rideName.toUtf8().constData();

			lua_["battle"][index]["ridelv"] = obj.rideLevel;

			lua_["battle"][index]["ridehp"] = obj.rideHp;

			lua_["battle"][index]["ridemaxhp"] = obj.rideMaxHp;

			lua_["battle"][index]["ridehpp"] = obj.rideHpPercent;
		}

		lua_["battle"]["playerpos"] = static_cast<qint64>(battle.player.pos + 1);
		lua_["battle"]["petpos"] = static_cast<qint64>(battle.pet.pos + 1);
	}

	//battle\.(\w+)
	if (expr.contains("battle"))
	{
		bret = true;
		if (!lua_["battle"].valid())
			lua_["battle"] = lua_.create_table();
		else
		{
			if (!lua_["battle"].is<sol::table>())
				lua_["battle"] = lua_.create_table();
		}

		battledata_t battle = injector.server->getBattleData();

		lua_["battle"]["round"] = injector.server->battleCurrentRound.load(std::memory_order_acquire) + 1;

		lua_["battle"]["field"] = injector.server->getFieldString(battle.fieldAttr).toUtf8().constData();

		lua_["battle"]["dura"] = static_cast<qint64>(injector.server->battleDurationTimer.elapsed() / 1000ll);

		lua_["battle"]["totaldura"] = static_cast<qint64>(injector.server->battle_total_time.load(std::memory_order_acquire) / 1000 / 60);

		lua_["battle"]["totalcombat"] = injector.server->battle_total.load(std::memory_order_acquire);
	}

	//dialog\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	if (expr.contains("dialog"))
	{
		bret = true;
		makeTable(lua_, "dialog", MAX_PET, MAX_PET_ITEM);

		QStringList dialog = injector.server->currentDialog.get().linedatas;
		int size = dialog.size();
		bool visible = injector.server->isDialogVisible();
		for (int i = 0; i < MAX_DIALOG_LINE; ++i)
		{
			QString text;
			if (i >= size || !visible)
				text = "";
			else
				text = dialog.at(i);

			int index = i + 1;

			lua_["dialog"][index] = text.toUtf8().constData();
		}
	}

	//dialog\.(\w+)
	if (expr.contains("dialog"))
	{
		bret = true;
		if (!lua_["dialog"].valid())
			lua_["dialog"] = lua_.create_table();
		else
		{
			if (!lua_["dialog"].is<sol::table>())
				lua_["dialog"] = lua_.create_table();
		}

		dialog_t dialog = injector.server->currentDialog.get();

		lua_["dialog"]["id"] = dialog.dialogid;
		lua_["dialog"]["unitid"] = dialog.unitid;
		lua_["dialog"]["type"] = dialog.windowtype;
		lua_["dialog"]["buttontext"] = dialog.linebuttontext.join("|").toUtf8().constData();
		lua_["dialog"]["button"] = dialog.buttontype;
	}


	//magic\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("magic"))
	{
		bret = true;

		makeTable(lua_, "magic", MAX_MAGIC);

		for (int i = 0; i < MAX_MAGIC; ++i)
		{
			int index = i + 1;
			MAGIC magic = injector.server->getMagic(i);

			lua_["magic"][index]["valid"] = magic.valid;
			lua_["magic"][index]["index"] = index;
			lua_["magic"][index]["costmp"] = magic.costmp;
			lua_["magic"][index]["field"] = magic.field;
			lua_["magic"][index]["name"] = magic.name.toUtf8().constData();
			lua_["magic"][index]["memo"] = magic.memo.toUtf8().constData();
			lua_["magic"][index]["target"] = magic.target;
		}
	}

	//skill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("skill"))
	{
		bret = true;

		makeTable(lua_, "skill", MAX_PROFESSION_SKILL);

		for (int i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			int index = i + 1;
			PROFESSION_SKILL skill = injector.server->getSkill(i);

			lua_["skill"][index]["valid"] = skill.valid;
			lua_["skill"][index]["index"] = index;
			lua_["skill"][index]["costmp"] = skill.costmp;
			lua_["skill"][index]["modelid"] = skill.icon;
			lua_["skill"][index]["type"] = skill.kind;
			lua_["skill"][index]["lv"] = skill.skill_level;
			lua_["skill"][index]["id"] = skill.skillId;
			lua_["skill"][index]["name"] = skill.name.toUtf8().constData();
			lua_["skill"][index]["memo"] = skill.memo.toUtf8().constData();
			lua_["skill"][index]["target"] = skill.target;
		}
	}

	//petskill\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petksill"))
	{
		bret = true;

		makeTable(lua_, "petksill", MAX_PET, MAX_SKILL);

		int petIndex = -1;
		int index = -1;
		int i, j;
		for (i = 0; i < MAX_PET; ++i)
		{
			petIndex = i + 1;

			if (!lua_["petskill"][petIndex].valid())
				lua_["petskill"][petIndex] = lua_.create_table();
			else
			{
				if (!lua_["petskill"][petIndex].is<sol::table>())
					lua_["petskill"][petIndex] = lua_.create_table();
			}

			for (j = 0; j < MAX_SKILL; ++j)
			{
				index = j + 1;
				PET_SKILL skill = injector.server->getPetSkill(i, j);

				if (!lua_["petskill"][petIndex][index].valid())
					lua_["petskill"][petIndex][index] = lua_.create_table();
				else
				{
					if (!lua_["petskill"][petIndex][index].is<sol::table>())
						lua_["petskill"][petIndex][index] = lua_.create_table();
				}

				lua_["petskill"][petIndex][index]["valid"] = skill.valid;
				lua_["petskill"][petIndex][index]["index"] = index;
				lua_["petskill"][petIndex][index]["id"] = skill.skillId;
				lua_["petskill"][petIndex][index]["field"] = skill.field;
				lua_["petskill"][petIndex][index]["target"] = skill.target;
				lua_["petskill"][petIndex][index]["name"] = skill.name.toUtf8().constData();
				lua_["petskill"][petIndex][index]["memo"] = skill.memo.toUtf8().constData();

			}
		}
	}

	//petequip\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	if (expr.contains("petequip"))
	{
		bret = true;

		makeTable(lua_, "petequip", MAX_PET, MAX_PET_ITEM);

		int petIndex = -1;
		int index = -1;
		int i, j;
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

				lua_["petequip"][petIndex][index]["valid"] = item.valid;
				lua_["petequip"][petIndex][index]["index"] = index;
				lua_["petequip"][petIndex][index]["lv"] = item.level;
				lua_["petequip"][petIndex][index]["field"] = item.field;
				lua_["petequip"][petIndex][index]["target"] = item.target;
				lua_["petequip"][petIndex][index]["type"] = item.type;
				lua_["petequip"][petIndex][index]["modelid"] = item.modelid;
				lua_["petequip"][petIndex][index]["dura"] = damageValue;
				lua_["petequip"][petIndex][index]["name"] = item.name.toUtf8().constData();
				lua_["petequip"][petIndex][index]["name2"] = item.name2.toUtf8().constData();
				lua_["petequip"][petIndex][index]["memo"] = item.memo.toUtf8().constData();

			}
		}
	}

	if (expr.contains("_GAME_"))
	{
		bret = true;
		lua_.set("_GAME_", injector.server->getGameStatus());
	}

	if (expr.contains("_WORLD_"))
	{
		bret = true;
		lua_.set("_WORLD_", injector.server->getWorldStatus());
	}

	return bret;
}

//行跳轉
bool Parser::jump(qint64 line, bool noStack)
{
	qint64 currnetLine = getCurrentLine();
	if (!noStack)
		jmpStack_.push(currnetLine + 1);
	currnetLine += line;
	setCurrentLine(currnetLine);
	return true;
}

//指定行跳轉
void Parser::jumpto(qint64 line, bool noStack)
{
	if (line - 1 < 0)
		line = 1;
	if (!noStack)
		jmpStack_.push(getCurrentLine() + 1);

	setCurrentLine(line - 1);
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
		callStack_.push(node);
		jumpto(jumpLine, true);
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

	if (!noStack)
		jmpStack_.push(getCurrentLine() + 1);

	qint64 jumpLineCount = jumpLine - getCurrentLine();

	return jump(jumpLineCount, true);
}

//解析跳轉棧或調用棧相關訊息並發送信號
void Parser::generateStackInfo(qint64 type)
{
	if (mode_ != kSync)
		return;

	if (type != 0)
		return;

	Injector& injector = Injector::getInstance();

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

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (type == kModeCallStack)
		emit signalDispatcher.callStackInfoChanged(QVariant::fromValue(hash));
	else
		emit signalDispatcher.jumpStackInfoChanged(QVariant::fromValue(hash));
}

QString Parser::getLuaTableString(const sol::table& t, int& depth)
{
	if (depth <= 0)
		return "";

	--depth;

	QString ret = "{\n";

	QStringList results;
	QStringList strKeyResults;

	QString nowIndent = "";
	for (int i = kMaxLuaTableDepth - 1 - depth; i > 0; --i)
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
		{
			key = QString::fromUtf8(pair.first.as<std::string>().c_str());
		}

		if (pair.second.is<sol::table>())
		{
			value = getLuaTableString(pair.second.as<sol::table>(), depth);
		}
		else if (pair.second.is<std::string>())
		{
			value = QString("'%1'").arg(QString::fromUtf8(pair.second.as<std::string>().c_str()));
		}
		else if (pair.second.is<qint64>())
		{
			value = QString::number(pair.second.as<qint64>());
		}
		else if (pair.second.is<double>())
		{
			value = QString::number(pair.second.as<double>(), 'f', 16);
		}
		else if (pair.second.is<bool>())
		{
			value = pair.second.as<bool>() ? "true" : "false";
		}
		else
		{
			value = "nil";
		}

		if (key.isEmpty())
		{
			if (nKey >= results.size())
			{
				for (qint64 i = results.size(); i <= nKey; ++i)
				{
					results.append("nil");
				}
			}

			results[nKey] = nowIndent + value;
		}
		else
		{
			strKeyResults.append(nowIndent + QString("%1 = %2").arg(key).arg(value));
		}
	}

	std::sort(strKeyResults.begin(), strKeyResults.end(), [](const QString& a, const QString& b)
		{
			static const QLocale locale;
			static const QCollator collator(locale);
			return collator.compare(a, b) < 0;
		});

	results.append(strKeyResults);

	ret += results.join(",\n");
	ret += "\n}";

	return ret;
}

//根據關鍵字取值保存到變量
bool Parser::processGetSystemVarValue(const QString& varName, QString& valueStr, QVariant& varValue)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return false;

	QString trimmedStr = valueStr.simplified().toLower();

	enum SystemVarName
	{
		kPlayerInfo,
		kMagicInfo,
		kSkillInfo,
		kPetInfo,
		kPetSkillInfo,
		kMapInfo,
		kItemInfo,
		kEquipInfo,
		kPetEquipInfo,
		kTeamInfo,
		kChatInfo,
		kDialogInfo,
		kPointInfo,
		kBattleInfo,
	};

	const QHash<QString, SystemVarName> systemVarNameHash = {
		{ "player", kPlayerInfo },
		{ "magic", kMagicInfo },
		{ "skill", kSkillInfo },
		{ "pet", kPetInfo },
		{ "petskill", kPetSkillInfo },
		{ "map", kMapInfo },
		{ "item", kItemInfo },
		{ "equip", kEquipInfo },
		{ "petequip", kPetEquipInfo },
		{ "team", kTeamInfo },
		{ "chat", kChatInfo },
		{ "dialog", kDialogInfo },
		{ "point", kPointInfo },
		{ "battle", kBattleInfo },
	};

	if (!systemVarNameHash.contains(trimmedStr))
		return false;

	SystemVarName index = systemVarNameHash.value(trimmedStr);
	bool bret = true;
	varValue = "";
	switch (index)
	{
	case kPlayerInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
		{
			break;
		}

		PC _pc = injector.server->getPC();
		QHash<QString, QVariant> hash = {
			{ "dir", _pc.dir },
			{ "hp", _pc.hp }, { "maxhp", _pc.maxHp }, { "hpp", _pc.hpPercent },
			{ "mp", _pc.mp }, { "maxmp", _pc.maxMp }, { "mpp", _pc.mpPercent },
			{ "vit", _pc.vit },
			{ "str", _pc.str }, { "tgh", _pc.tgh }, { "dex", _pc.dex },
			{ "exp", _pc.exp }, { "maxexp", _pc.maxExp },
			{ "lv", _pc.level },
			{ "atk", _pc.atk }, { "def", _pc.def },
			{ "agi", _pc.agi }, { "chasma", _pc.chasma }, { "luck", _pc.luck },
			{ "earth", _pc.earth }, { "water", _pc.water }, { "fire", _pc.fire }, { "wind", _pc.wind },
			{ "stone", _pc.gold },

			{ "title", _pc.titleNo },
			{ "dp", _pc.dp },
			{ "name", _pc.name },
			{ "fname", _pc.freeName },
			{ "namecolor", _pc.nameColor },
			{ "battlepet", _pc.battlePetNo + 1  },
			{ "mailpet", _pc.mailPetNo + 1 },
			//{ "", _pc.standbyPet },
			//{ "", _pc.battleNo },
			//{ "", _pc.sideNo },
			//{ "", _pc.helpMode },
			//{ "", _pc._pcNameColor },
			{ "turn", _pc.transmigration },
			//{ "", _pc.chusheng },
			{ "family", _pc.family },
			//{ "", _pc.familyleader },
			{ "ridename", _pc.ridePetName },
			{ "ridelv", _pc.ridePetLevel },
			{ "earnstone", injector.server->recorder[0].goldearn },
			{ "earnrep", injector.server->recorder[0].repearn > 0 ? (injector.server->recorder[0].repearn / (injector.server->repTimer.elapsed() / 1000)) * 3600 : 0 },
			//{ "", _pc.familySprite },
			//{ "", _pc.basemodelid },
		};

		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kMagicInfo:
	{
		qint64 magicIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &magicIndex))
			break;
		--magicIndex;

		if (magicIndex < 0 || magicIndex > MAX_MAGIC)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		MAGIC m = injector.server->getMagic(magicIndex);

		const QVariantHash hash = {
			{ "valid", m.valid ? 1 : 0 },
			{ "cost", m.costmp },
			{ "field", m.field },
			{ "target", m.target },
			{ "deadTargetFlag", m.deadTargetFlag },
			{ "name", m.name },
			{ "memo", m.memo },
		};

		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		break;
	}
	case kSkillInfo:
	{
		qint64 skillIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &skillIndex))
			break;
		--skillIndex;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		PROFESSION_SKILL s = injector.server->getSkill(skillIndex);

		const QHash<QString, QVariant> hash = {
			{ "valid", s.valid ? 1 : 0 },
			{ "cost", s.costmp },
			//{ "field", s. },
			{ "target", s.target },
			//{ "", s.deadTargetFlag },
			{ "name", s.name },
			{ "memo", s.memo },
		};

		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		break;
	}
	case kPetInfo:
	{
		qint64 petIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &petIndex))
		{
			QString typeStr;
			if (!checkString(currentLineTokens_, 3, &typeStr))
				break;

			if (typeStr == "count")
			{
				varValue = injector.server->getPetSize();
				bret = true;
			}

			break;
		}
		--petIndex;

		if (petIndex < 0 || petIndex >= MAX_PET)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		const QHash<PetState, QString> petStateHash = {
			{ kBattle, u8"battle" },
			{ kStandby , u8"standby" },
			{ kMail, u8"mail" },
			{ kRest, u8"rest" },
			{ kRide, u8"ride" },
		};

		PET _pet = injector.server->getPet(petIndex);

		QVariantHash hash = {
			{ "index", _pet.index + 1 },						//位置
			{ "modelid", _pet.modelid },						//圖號
			{ "hp", _pet.hp }, { "maxhp", _pet.maxHp }, { "hpp", _pet.hpPercent },					//血量
			{ "mp", _pet.mp }, { "maxmp", _pet.maxMp }, { "mpp", _pet.mpPercent },					//魔力
			{ "exp", _pet.exp }, { "maxexp", _pet.maxExp },				//經驗值
			{ "lv", _pet.level },						//等級
			{ "atk", _pet.atk },						//攻擊力
			{ "def", _pet.def },						//防禦力
			{ "agi", _pet.agi },						//速度
			{ "loyal", _pet.loyal },							//AI
			{ "earth", _pet.earth }, { "water", _pet.water }, { "fire", _pet.fire }, { "wind", _pet.wind },
			{ "maxskill", _pet.maxSkill },
			{ "turn", _pet.transmigration },						// 寵物轉生數
			{ "name", _pet.name },
			{ "fname", _pet.freeName },
			{ "valid", _pet.valid ? 1ll : 0ll },
			{ "turn", _pet.transmigration },
			{ "state", petStateHash.value(_pet.state) },
			{ "power", static_cast<qint64>(qFloor(_pet.power)) }
		};

		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		break;
	}
	case kPetSkillInfo:
	{
		qint64 petIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &petIndex))
			break;

		if (petIndex < 0 || petIndex >= MAX_PET)
			break;

		qint64 skillIndex = -1;
		if (!checkInteger(currentLineTokens_, 4, &skillIndex))
			break;

		if (skillIndex < 0 || skillIndex >= MAX_SKILL)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 5, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		PET_SKILL _petskill = injector.server->getPetSkill(petIndex, skillIndex);

		QHash<QString, QVariant> hash = {
			{ "valid", _petskill.valid ? 1 : 0 },
			//{ "", _petskill.field },
			{ "target", _petskill.target },
			//{ "", _petskill.deadTargetFlag },
			{ "name", _petskill.name },
			{ "memo", _petskill.memo },
		};

		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		break;
	}
	case kMapInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		injector.server->reloadHashVar("map");
		VariantSafeHash hashmap = injector.server->hashmap;
		if (!hashmap.contains(typeStr))
			break;

		varValue = hashmap.value(typeStr);
		break;
	}
	case kItemInfo:
	{
		qint64 itemIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &itemIndex))
		{
			QString typeStr;
			if (checkString(currentLineTokens_, 3, &typeStr))
			{
				typeStr = typeStr.simplified().toLower();
				if (typeStr == "space")
				{
					QVector<int> itemIndexs;
					qint64 size = 0;
					if (injector.server->getItemEmptySpotIndexs(&itemIndexs))
						size = itemIndexs.size();

					varValue = size;
					break;
				}
				else if (typeStr == "count")
				{
					QString itemName;
					checkString(currentLineTokens_, 4, &itemName);


					QString memo;
					checkString(currentLineTokens_, 5, &memo);

					if (itemName.isEmpty() && memo.isEmpty())
						break;

					QVector<int> v;
					qint64 count = 0;
					if (injector.server->getItemIndexsByName(itemName, memo, &v))
					{
						for (const int it : v)
							count += injector.server->getPC().item[it].stack;
					}

					varValue = count;
				}
				else
				{
					QString cmpStr = typeStr;
					if (cmpStr.isEmpty())
						break;

					QString memo;
					checkString(currentLineTokens_, 4, &memo);

					QVector<int> itemIndexs;
					if (injector.server->getItemIndexsByName(cmpStr, memo, &itemIndexs))
					{
						int index = itemIndexs.front();
						if (itemIndexs.front() >= CHAR_EQUIPPLACENUM)
							varValue = index + 1 - CHAR_EQUIPPLACENUM;
						else
							varValue = index + 1 + 100;
					}
				}
			}
		}

		int index = itemIndex + CHAR_EQUIPPLACENUM - 1;
		if (index < CHAR_EQUIPPLACENUM || index >= MAX_ITEM)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		PC pc = injector.server->getPC();
		ITEM item = pc.item[index];

		QString damage = item.damage.simplified();
		qint64 damageValue = 0;
		if (damage.contains("%"))
			damage.replace("%", "");
		if (damage.contains("％"))
			damage.replace("％", "");

		bool ok = false;
		qint64 dura = damage.toLongLong(&ok);
		if (!ok && !damage.isEmpty())
			damageValue = 100;
		else
			damageValue = dura;

		qint64 countdura = 0;
		if (item.name == "惡魔寶石" || item.name == "恶魔宝石")
		{
			static QRegularExpression rex("(\\d+)");
			QRegularExpressionMatch match = rex.match(item.memo);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				bool ok = false;
				dura = str.toLongLong(&ok);
				if (ok)
					countdura = dura;
			}
		}

		QHash<QString, QVariant> hash = {
			//{ "", item.color },
			{ "modelid", item.modelid },
			{ "lv", item.level },
			{ "stack", item.stack },
			{ "valid", item.valid ? 1 : 0 },
			{ "field", item.field },
			{ "target", item.target },
			//{ "", item.deadTargetFlag },
			//{ "", item.sendFlag },
			{ "name", item.name },
			{ "name2", item.name2 },
			{ "memo", item.memo },
			{ "dura", damageValue },
			{ "count", countdura }
		};

		varValue = hash.value(typeStr);
		break;
	}
	case kEquipInfo:
	{
		qint64 itemIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &itemIndex))
			break;

		int index = itemIndex - 1;
		if (index < 0 || index >= CHAR_EQUIPPLACENUM)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		PC pc = injector.server->getPC();
		ITEM item = pc.item[index];

		QString damage = item.damage.simplified();
		qint64 damageValue = 0;
		if (damage.contains("%"))
			damage.replace("%", "");
		if (damage.contains("％"))
			damage.replace("％", "");

		bool ok = false;
		int dura = damage.toLongLong(&ok);
		if (!ok && !damage.isEmpty())
			damageValue = 100;
		else
			damageValue = dura;

		QHash<QString, QVariant> hash = {
			//{ "", item.color },
			{ "modelid", item.modelid },
			{ "lv", item.level },
			{ "stack", item.stack },
			{ "valid", item.valid ? 1 : 0 },
			{ "field", item.field },
			{ "target", item.target },
			//{ "", item.deadTargetFlag },
			//{ "", item.sendFlag },
			{ "name", item.name },
			{ "name2", item.name2 },
			{ "memo", item.memo },
			{ "dura", damageValue },
		};

		varValue = hash.value(typeStr);
		break;
	}
	case kPetEquipInfo:
	{
		qint64 petIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &petIndex))
			break;
		--petIndex;

		if (petIndex < 0 || petIndex >= MAX_PET)
			break;

		qint64 itemIndex = -1;
		if (!checkInteger(currentLineTokens_, 4, &itemIndex))
			break;
		--itemIndex;

		if (itemIndex < 0 || itemIndex >= MAX_PET_ITEM)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 5, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		ITEM item = injector.server->getPetEquip(petIndex, itemIndex);

		QString damage = item.damage.simplified();
		qint64 damageValue = 0;
		if (damage.contains("%"))
			damage.replace("%", "");
		if (damage.contains("％"))
			damage.replace("％", "");

		bool ok = false;
		int dura = damage.toLongLong(&ok);
		if (!ok && !damage.isEmpty())
			damageValue = 100;
		else
			damageValue = dura;

		QHash<QString, QVariant> hash = {
			//{ "", item.color },
			{ "modelid", item.modelid },
			{ "lv", item.level },
			{ "stack", item.stack },
			{ "valid", item.valid ? 1 : 0 },
			{ "field", item.field },
			{ "target", item.target },
			//{ "", item.deadTargetFlag },
			//{ "", item.sendFlag },
			{ "name", item.name },
			{ "name2", item.name2 },
			{ "memo", item.memo },
			{ "dura", damageValue },
		};

		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		break;
	}
	case kTeamInfo:
	{
		qint64 partyIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &partyIndex))
			break;
		--partyIndex;

		if (partyIndex < 0 || partyIndex >= MAX_PARTY)
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		PARTY _party = injector.server->getParty(partyIndex);
		QHash<QString, QVariant> hash = {
			{ "valid", _party.valid ? 1 : 0 },
			{ "id", _party.id },
			{ "lv", _party.level },
			{ "maxhp", _party.maxHp },
			{ "hp", _party.hp },
			{ "hpp", _party.hpPercent },
			{ "mp", _party.mp },
			{ "name", _party.name },
		};

		varValue = hash.value(typeStr);
		break;
	}
	case kChatInfo:
	{
		qint64 chatIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &chatIndex))
			break;

		if (chatIndex < 1 || chatIndex > 20)
			break;

		varValue = injector.server->getChatHistory(chatIndex - 1);
		break;
	}
	case kDialogInfo:
	{
		bool valid = injector.server->isDialogVisible();

		qint64 dialogIndex = -1;
		if (!checkInteger(currentLineTokens_, 3, &dialogIndex))
		{
			QString typeStr;
			if (!checkString(currentLineTokens_, 3, &typeStr))
				break;

			if (typeStr == "id")
			{
				if (!valid)
				{
					varValue = 0;
					break;
				}

				dialog_t dialog = injector.server->currentDialog;
				varValue = dialog.dialogid;
			}
			else if (typeStr == "unitid")
			{
				if (!valid)
				{
					varValue = 0;
					break;
				}

				qint64 unitid = injector.server->currentDialog.get().unitid;
				varValue = unitid;
			}
			else if (typeStr == "type")
			{
				if (!valid)
				{
					varValue = 0;
					break;
				}

				qint64 type = injector.server->currentDialog.get().windowtype;
				varValue = type;
			}
			else if (typeStr == "button")
			{
				if (!valid)
				{
					varValue = "";
					break;
				}

				QStringList list = injector.server->currentDialog.get().linebuttontext;
				varValue = list.join("|");
			}
			break;
		}

		if (!valid)
		{
			varValue = "";
			break;
		}

		QStringList dialogStrList = injector.server->currentDialog.get().linedatas;

		if (dialogIndex == -1)
		{
			QStringList texts;
			for (qint64 i = 0; i < MAX_DIALOG_LINE; ++i)
			{
				if (i >= dialogStrList.size())
					break;

				texts.append(dialogStrList.at(i).simplified());
			}

			varValue = texts.join("\n");
		}
		else
		{
			int index = dialogIndex - 1;
			if (index < 0 || index >= dialogStrList.size())
				break;

			varValue = dialogStrList.at(index);
		}
		break;
	}
	case kPointInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		injector.server->IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG = true;
		injector.server->shopOk(2);

		QElapsedTimer timer; timer.start();
		for (;;)
		{
			if (isInterruptionRequested())
				return false;

			if (timer.hasExpired(5000))
				break;

			if (!injector.server->IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG)
				break;

			QThread::msleep(100);
		}

		//qint64 rep = 0;   // 聲望
		//qint64 ene = 0;   // 氣勢
		//qint64 shl = 0;   // 貝殼
		//qint64 vit = 0;   // 活力
		//qint64 pts = 0;   // 積分
		//qint64 vip = 0;   // 會員點
		currencydata_t currency = injector.server->currencyData;
		if (typeStr == "exp")
		{
			varValue = currency.expbufftime;
		}
		else if (typeStr == "rep")
		{
			varValue = currency.prestige;
		}
		else if (typeStr == "ene")
		{
			varValue = currency.energy;
		}
		else if (typeStr == "shl")
		{
			varValue = currency.shell;
		}
		else if (typeStr == "vit")
		{
			varValue = currency.vitality;
		}
		else if (typeStr == "pts")
		{
			varValue = currency.points;
		}
		else if (typeStr == "vip")
		{
			varValue = currency.VIPPoints;
		}
		else
			break;

		injector.server->press(BUTTON_CANCEL);

		break;
	}
	case kBattleInfo:
	{
		qint64 index = -1;
		if (!checkInteger(currentLineTokens_, 3, &index))
		{
			QString typeStr;
			checkString(currentLineTokens_, 3, &typeStr);
			if (typeStr.isEmpty())
				break;

			if (typeStr == "round")
			{
				varValue = injector.server->battleCurrentRound.load(std::memory_order_acquire);
			}
			else if (typeStr == "field")
			{
				varValue = injector.server->hashbattlefield.get();
			}
			break;
		}

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);

		injector.server->reloadHashVar("battle");
		util::SafeHash<int, QVariantHash> hashbattle = injector.server->hashbattle;
		if (!hashbattle.contains(index))
			break;

		QVariantHash hash = hashbattle.value(index);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		break;
	}
	default:
	{
		break;
	}
	}

	return bret;
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
		SPD_LOG(g_logger_name, QString("[parser] Invalid command: %1").arg(commandToken.data.toString()), SPD_WARN);
		return kUnknownCommand;
	}

	QString commandName = commandToken.data.toString();
	qint64 status = kNoChange;

	if (commandRegistry_.contains(commandName))
	{
		CommandRegistry function = commandRegistry_.value(commandName, nullptr);
		if (function == nullptr)
		{
			SPD_LOG(g_logger_name, QString("[parser] Command pointer is nullptr: %1").arg(commandName), SPD_WARN);
			return kUnknownCommand;
		}

		status = function(getCurrentLine(), tokens);
	}
	else
	{
		SPD_LOG(g_logger_name, QString("[parser] Command not found: %1").arg(commandName), SPD_WARN);
		return kUnknownCommand;
	}

	return status;
}

//處理"變量"
void Parser::processVariable(RESERVE type)
{
	switch (type)
	{
	case TK_VARDECL:
	{
		//取第一個參數(變量名)
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		//取第二個參數(類別字符串)
		QString typeStr;
		if (!checkString(currentLineTokens_, 2, &typeStr))
			break;

		if (typeStr.isEmpty())
			break;

		QVariant varValue;
		if (!processGetSystemVarValue(varName, typeStr, varValue))
			break;

		//插入全局變量表
		if (varName.contains("[") && varName.contains("]"))
			processTableSet(varName, varValue);
		else
			insertVar(varName, varValue);
		break;
	}
	case TK_VARFREE:
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		if (isGlobalVarContains(varName))
			removeGlobalVar(varName);
		else
			removeLocalVar(varName);
		break;
	}
	case TK_VARCLR:
	{
		clearGlobalVar();
		break;
	}
	default:
		break;
	}
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
void Parser::processMultiVariable()
{
	QString varNameStr = getToken<QString>(0);
	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	qint64 varCount = varNames.count();
	qint64 value = 0;

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
		insertGlobalVar("_LUAEXPR", exprStr);

		const std::string exprStrUTF8 = exprStr.toUtf8().constData();
		sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();
		if (!loaded_chunk.valid())
		{
			sol::error err = loaded_chunk;
			QString errStr = QString::fromUtf8(err.what());
			insertGlobalVar(QString("_LUAERR@%1").arg(getCurrentLine()), exprStr);
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

		int depth = kMaxLuaTableDepth;
		QString resultStr = getLuaTableString(retObject.as<sol::table>(), depth);
		if (resultStr.isEmpty())
			return;

		insertLocalVar(varName, resultStr);
	}
	else if (isGlobalVarContains(varName))
	{
		QString globalVar = getGlobalVarValue(varName).toString();
		if (!globalVar.startsWith("{") || !globalVar.endsWith("}"))
			return;

		QString exprStr = QString("%1\n%2 = %3;\n%4;\nreturn %5;").arg(luaLocalVarStringList.join("\n")).arg(varName).arg(globalVar).arg(expr).arg(varName);
		insertGlobalVar("_LUAEXPR", exprStr);

		const std::string exprStrUTF8 = exprStr.toUtf8().constData();
		sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();
		if (!loaded_chunk.valid())
		{
			sol::error err = loaded_chunk;
			QString errStr = QString::fromUtf8(err.what());
			insertGlobalVar(QString("_LUAERR@%1").arg(getCurrentLine()), exprStr);
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

		int depth = kMaxLuaTableDepth;
		QString resultStr = getLuaTableString(retObject.as<sol::table>(), depth);
		if (resultStr.isEmpty())
			return;

		insertGlobalVar(varName, resultStr);
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
	if (args.contains(varName))
		value = args.value(varName, 0ll).toLongLong();
	else
		value = getVar<qint64>(varName);

	if (op == TK_DEC)
		--value;
	else if (op == TK_INC)
		++value;
	else
		return;

	insertVar(varName, value);
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
			insertGlobalVar("_LUAEXPR", exprStr);

			importVariablesToLua(exprStr);
			checkConditionalOp(exprStr);

			const std::string exprStrUTF8 = exprStr.toUtf8().constData();
			sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
			lua_.collect_garbage();
			if (!loaded_chunk.valid())
			{
				sol::error err = loaded_chunk;
				QString errStr = QString::fromUtf8(err.what());
				insertGlobalVar(QString("_LUAERR@%1").arg(getCurrentLine()), exprStr);
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
				insertVar(varName, QString::fromUtf8(retObject.as<std::string>().c_str()));
			}
		}
		return;
	}

	if (typeFirst != QVariant::Int && typeFirst != QVariant::LongLong && typeFirst != QVariant::Double)
		return;

	QString opStr = getToken<QString>(1).simplified();
	opStr.replace("=", "");

	qint64 value = 0;
	QString expr = "";
	if (typeFirst == QVariant::Int || typeFirst == QVariant::LongLong)
		expr = QString("%1 %2").arg(opStr).arg(varSecond.toLongLong());
	else if (typeFirst == QVariant::Double)
		expr = QString("%1 %2").arg(opStr).arg(varSecond.toDouble());
	else if (typeFirst == QVariant::Bool)
		expr = QString("%1 %2").arg(opStr).arg(varSecond.toBool() ? 1 : 0);
	else
		return;

	if (!exprCAOSTo(varName, expr, &value))
		return;
	insertVar(varName, value);
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
	if (type == QVariant::Int || type == QVariant::LongLong)
	{
		qint64 value = 0;
		if (!exprTo(expr, &value))
			return;
		result = value;
	}
	else if (type == QVariant::Double)
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

	insertGlobalVar("_IFRESULT", value.toString());

	return checkJump(currentLineTokens_, 2, value.toBool(), SuccessJump) == kHasJump;
}

//處理隨機數
void Parser::processRandom()
{
	do
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		qint64 min = 0;
		checkInteger(currentLineTokens_, 2, &min);
		qint64 max = 0;
		checkInteger(currentLineTokens_, 3, &max);

		if (min == 0 && max == 0)
			break;

		std::random_device rd;
		std::mt19937_64 gen(rd());
		if (min > 0 && max == 0)
		{
			std::uniform_int_distribution<qint64> distribution(0, min);

			if (varName.contains("[") && varName.contains("]"))
				processTableSet(varName, distribution(gen));
			else
				insertVar(varName, distribution(gen));
			break;
		}
		else if (min > max)
		{
			if (varName.contains("[") && varName.contains("]"))
				processTableSet(varName, 0);
			else
				insertVar(varName, 0);
			break;
		}

		std::uniform_int_distribution<qint64> distribution(min, max);
		if (varName.contains("[") && varName.contains("]"))
			processTableSet(varName, distribution(gen));
		else
			insertVar(varName, distribution(gen));
	} while (false);
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

			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->announce(formatedStr, color);

			const QDateTime time(QDateTime::currentDateTime());
			const QString timeStr(time.toString("hh:mm:ss:zzz"));
			QString src = "\0";

			QString msg = (QString("[%1 | @%2]: %3\0") \
				.arg(timeStr)
				.arg(getCurrentLine() + 1, 3, 10, QLatin1Char(' ')).arg(formatedStr));

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
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

			Injector& injector = Injector::getInstance();
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
			break;

		FunctionNode node = getFunctionNodeByName(functionName);
		++node.callCount;
		node.callFromLine = getCurrentLine();
		checkCallArgs(jumpLine);
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
	Injector& injector = Injector::getInstance();
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
		initValue = QVariant();
	}

	qint64 nBeginValue = initValue.toLongLong();

	//break condition
	QVariant breakValue = checkValue(currentLineTokens_, 3);
	if (breakValue.type() != QVariant::LongLong)
	{
		breakValue = QVariant();
	}

	qint64 nEndValue = breakValue.toLongLong();

	//step var value
	QVariant stepValue = checkValue(currentLineTokens_, 4);
	if (stepValue.type() != QVariant::LongLong || stepValue.toLongLong() == 0)
		stepValue = 1ll;

	qint64 nStepValue = stepValue.toLongLong();

	if (forStack_.isEmpty() || forStack_.top().beginLine != getCurrentLine())
	{
		ForNode node = getForNodeByLineIndex(getCurrentLine());
		node.beginValue = nBeginValue;
		node.endValue = nEndValue;
		node.stepValue = stepValue;

		++node.callCount;
		if (!node.varName.isEmpty())
			insertLocalVar(node.varName, nBeginValue);
		forStack_.push(node);
		currentField.push(TK_FOR);
	}
	else
	{
		ForNode node = forStack_.top();

		if (node.varName.isEmpty())
			return false;

		QVariant localValue = getLocalVarValue(node.varName);

		if (localValue.type() != QVariant::LongLong)
			return false;

		if (!initValue.isValid() && !breakValue.isValid())
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

//處理所有的token
void Parser::processTokens()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	Injector& injector = Injector::getInstance();

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

		currentLineTokens_ = tokens_.value(getCurrentLine());

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
			if (!lua_["_DEBUG"].is<bool>() || lua_["_DEBUG"].get<bool>())
			{
				lua_["_DEBUG"] = true;
				if (injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
					QThread::msleep(1);
			}
			else
				lua_["_DEBUG"] = false;

			if (callBack_ != nullptr)
			{
				qint64 status = callBack_(getCurrentLine(), currentLineTokens_);
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
			lastCriticalError_ = kNoError;
			name.clear();
			requestInterruption();
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
				SPD_LOG(g_logger_name, "[parser] Command has error", SPD_WARN);
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
		case TK_VARDECL:
		case TK_VARFREE:
		case TK_VARCLR:
		{
			processVariable(currentType);
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
			processMultiVariable();
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
		case TK_RND:
		{
			processRandom();
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
			SPD_LOG(g_logger_name, "[parser] Unexpected token type", SPD_WARN);
			break;
		}
		}

		//導出變量訊息
		if (mode_ == kSync && !skip)
			exportVarInfo();

		//移動至下一行
		next();
	}

	processClean();
	lua_.collect_garbage();

	/*==========全部重建 : 成功 2 個，失敗 0 個，略過 0 個==========
	========== 重建 開始於 1:24 PM 並使用了 01:04.591 分鐘 ==========*/
	emit signalDispatcher.appendScriptLog(QObject::tr(" ========== script result : %1，cost %2 ==========").arg("success").arg(util::formatMilliseconds(timer.elapsed())));

}

//導出變量訊息
void Parser::exportVarInfo()
{
	Injector& injector = Injector::getInstance();
	if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		return;


	QVariantHash varhash;
	VariantSafeHash* pglobalHash = getGlobalVarPointer();
	QVariantHash localHash;
	if (!localVarStack_.isEmpty())
		localHash = localVarStack_.top();

	QString key;
	for (auto it = pglobalHash->cbegin(); it != pglobalHash->cend(); ++it)
	{
		key = QString("global|%1").arg(it.key());
		varhash.insert(key, it.value());
	}

	for (auto it = localHash.cbegin(); it != localHash.cend(); ++it)
	{
		key = QString("local|%1").arg(it.key());
		varhash.insert(key, it.value());
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.varInfoImported(varhash);
}