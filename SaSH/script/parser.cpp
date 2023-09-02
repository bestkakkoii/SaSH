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

//#ifdef _DEBUG
//#define exprtk_enable_debugging
//#endif
//#include <exprtk/exprtk.hpp>

#include <spdlogger.hpp>
extern QString g_logger_name;

//"調用" 傳參數最小佔位
constexpr qint64 kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr qint64 kFormatPlaceHoldSize = 3;

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
				return sol::make_object(s, static_cast<qint64>(qFloor(var.toDouble())));
			else
				return sol::make_object(s, var.toString().toUtf8().constData());
		});
}

Parser::~Parser()
{
	qDebug() << "Parser is distory!!";
}

//根據token解釋腳本
void Parser::parse(qint64 line)
{
	lineNumber_ = line; //設置當前行號
	callStack_.clear(); //清空調用棧
	jmpStack_.clear();  //清空跳轉棧

	if (tokens_.isEmpty())
		return;

	if (variables_ == nullptr)
	{
		variables_ = new VariantSafeHash();
		if (variables_ == nullptr)
			return;
	}

	if (globalVarLock_ == nullptr)
	{
		globalVarLock_ = new QReadWriteLock();
		if (globalVarLock_ == nullptr)
			return;
	}

	try
	{
		processTokens();
	}
	catch (const std::exception& e)
	{
		SPD_LOG(g_logger_name, QString("[parser] processTokens exception: %1").arg(e.what()), SPD_ERROR);
	}
	catch (...)
	{
		SPD_LOG(g_logger_name, "[parser] processTokens unknown exception:", SPD_ERROR);
	}

	if (!isSubScript_)
	{
		if (variables_ != nullptr)
		{
			delete variables_;
			variables_ = nullptr;
		}

		if (globalVarLock_ != nullptr)
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
		emit signalDispatcher.addErrorMarker(lineNumber_, err);

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
		msg = QObject::tr("lua error: %1").arg(cmd) + extMsg;
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

	emit signalDispatcher.appendScriptLog(QString("[error] ") + QObject::tr("error occured at line %1. detail:%2").arg(lineNumber_ + 1).arg(msg), 6);
}

//比較兩個 QVariant 以 a 的類型為主
bool Parser::compare(const QVariant& a, const QVariant& b, RESERVE type) const
{
	QVariant::Type aType = a.type();

	if (aType == QVariant::Int || aType == QVariant::LongLong || aType == QVariant::Double)
	{
		qint64 numA = a.toLongLong();
		qint64 numB = b.toLongLong();

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

bool Parser::exprTo(QString expr, QString* ret)
{
	if (nullptr == ret)
		return false;

	expr = expr.simplified();

	if (expr == "return" || expr == "back" || expr == "continue" || expr == "break"
		|| expr == QString(u8"返回") || expr == QString(u8"跳回") || expr == QString(u8"繼續") || expr == QString(u8"跳出")
		|| expr == QString(u8"继续"))
	{
		*ret = expr;
		return true;
	}

	checkConditionalOp(expr);

	QString exprStr = QString("%1\nreturn %2;").arg(localVarList.join("\n")).arg(expr);
	insertGlobalVar("_LUAEXPR", exprStr);

	const std::string exprStrUTF8 = exprStr.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
	lua_.collect_garbage();
	if (!loaded_chunk.valid())
	{
		sol::error err = loaded_chunk;
		QString errStr = QString::fromUtf8(err.what());
		handleError(kLuaError, errStr);
		return false;
	}

	sol::object retObject;
	try
	{
		retObject = loaded_chunk;
	}
	catch (...)
	{
		return false;
	}

	if (retObject.is<std::string>())
	{
		*ret = QString::fromUtf8(retObject.as<std::string>().c_str());
		return true;
	}
	else if (retObject.is<sol::table>())
	{
		int deep = 10;
		*ret = getLuaTableString(retObject.as<sol::table>(), deep);
		return true;
	}
	return false;
}

//取表達式結果
template <typename T>
typename std::enable_if<std::is_same<T, qint64>::value || std::is_same<T, qreal>::value || std::is_same<T, QVariant>::value, bool>::type
Parser::exprTo(QString expr, T* ret)
{
	if (nullptr == ret)
		return false;

	expr = expr.simplified();

	checkConditionalOp(expr);

	QString exprStr = QString("%1\nreturn %2;").arg(localVarList.join("\n")).arg(expr);
	insertGlobalVar("_LUAEXPR", exprStr);

	const std::string exprStrUTF8 = exprStr.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
	lua_.collect_garbage();
	if (!loaded_chunk.valid())
	{
		sol::error err = loaded_chunk;
		QString errStr = QString::fromUtf8(err.what());
		handleError(kLuaError, errStr);
		return false;
	}

	sol::object retObject;
	try
	{
		retObject = loaded_chunk;
	}
	catch (...)
	{
		return false;
	}

	if (retObject.is<bool>())
	{
		*ret = retObject.as<bool>() ? 1ll : 0ll;
		return true;
	}
	else if (retObject.is<std::string>())
	{
		if constexpr (std::is_same<T, QVariant>::value)
		{
			*ret = QString::fromUtf8(retObject.as<std::string>().c_str());
			return true;
		}
	}
	else if (retObject.is<qint64>())
	{
		*ret = retObject.as<qint64>();
		return true;
	}
	else if (retObject.is<double>())
	{
		*ret = static_cast<qint64>(qFloor(retObject.as<double>()));
		return true;
	}

	return false;
}

//取表達式結果 += -= *= /= %= ^=
template <typename T>
typename std::enable_if<std::is_same<T, qint64>::value || std::is_same<T, qreal>::value, bool>::type
Parser::exprCAOSTo(const QString& varName, QString expr, T* ret)
{
	if (nullptr == ret)
		return false;

	expr = expr.simplified();

	checkConditionalOp(expr);

	QString exprStr = QString("%1\n%2 = %3 %4;\nreturn %5;")
		.arg(localVarList.join("\n")).arg(varName).arg(varName).arg(expr).arg(varName);

	insertGlobalVar("_LUAEXPR", exprStr);

	const std::string exprStrUTF8 = exprStr.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
	lua_.collect_garbage();
	if (!loaded_chunk.valid())
	{
		sol::error err = loaded_chunk;
		QString errStr = QString::fromUtf8(err.what());
		handleError(kLuaError, errStr);
		return false;
	}

	sol::object retObject;
	try
	{
		retObject = loaded_chunk;
	}
	catch (...)
	{
		return false;
	}

	if (retObject.is<bool>())
	{
		*ret = retObject.as<bool>() ? 1ll : 0ll;
		return true;
	}
	else if (retObject.is<qint64>())
	{
		*ret = retObject.as<qint64>();
		return true;
	}
	else if (retObject.is<double>())
	{
		*ret = static_cast<qint64>(qFloor(retObject.as<double>()));
		return true;
	}

	return false;
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
	}
	else if ((var.toString().startsWith("\'") || var.toString().startsWith("\""))
		&& (var.toString().endsWith("\'") || var.toString().endsWith("\"")))
	{
		*ret = var.toString();
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		////檢查是否為區域變量
		//QVariantHash args = getLocalVars();
		//QString varName = var.toString();
		//if (args.contains(varName))
		//{
		//	QVariant::Type vtype = args.value(varName).type();
		//	if (vtype != QVariant::Int && vtype != QVariant::LongLong && vtype != QVariant::Double && vtype != QVariant::String)
		//		return false;
		//	*ret = args.value(varName).toString();
		//	return true;
		//}
		//else if (isGlobalVarContains(varName))
		//{
		//	var = getGlobalVarValue(varName);
		//	if (var.type() == QVariant::List)
		//		return false;
		//	*ret = var.toString();
		//}
		//else
		//{
		//}

		* ret = var.toString();

		importVariablesToLua(*ret);

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
	else if ((var.toString().startsWith("\'") || var.toString().startsWith("\""))
		&& (var.toString().endsWith("\'") || var.toString().endsWith("\"")))
		return false;

	if (type == TK_INT)
	{
		bool ok = false;
		qint64 value = var.toLongLong(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		////檢查是否為區域變量
		//QVariantHash args = getLocalVars();
		//QString varName = var.toString();
		//if (args.contains(varName)
		//	&& (args.value(varName).type() == QVariant::Int
		//		|| args.value(varName).type() == QVariant::LongLong
		//		|| args.value(varName).type() == QVariant::Double))
		//{
		//	*ret = args.value(varName).toLongLong();
		//}
		//else if (args.contains(varName) && args.value(varName).type() == QVariant::String)
		//{
		//	bool ok;
		//	qint64 value = args.value(varName).toLongLong(&ok);
		//	if (ok)
		//		*ret = value;
		//	else
		//		return false;
		//	return true;
		//}
		//else if (isGlobalVarContains(varName))
		//{
		//	QVariant gVar = getGlobalVarValue(varName);
		//	if (gVar.type() == QVariant::Int || gVar.type() == QVariant::LongLong || gVar.type() == QVariant::Double)
		//	{
		//		bool ok;
		//		qint64 value = gVar.toLongLong(&ok);
		//		if (ok)
		//			*ret = value;
		//		else
		//			return false;
		//	}
		//	else
		//		return false;
		//}
		//else
		//{
		//}

		QString varValueStr = var.toString();

		importVariablesToLua(varValueStr);

		qint64 value = 0;
		if (!exprTo(varValueStr, &value))
			return false;

		*ret = value;
	}
	else
		return false;


	return true;
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

	if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		//QVariantHash args = getLocalVars();
		//QString varName = var.toString();
		//if (args.contains(varName))
		//{
		//	QVariant::Type vtype = args.value(varName).type();
		//	if (vtype == QVariant::Int || vtype == QVariant::LongLong || vtype == QVariant::Double || vtype == QVariant::String)
		//		*ret = args.value(varName).toString();
		//	else
		//		return false;
		//	return true;
		//}
		//else if (isGlobalVarContains(varName))
		//{
		//	QVariant gVar = getGlobalVarValue(varName);
		//	bool ok;
		//	qint64 value = gVar.toLongLong(&ok);
		//	if (ok)
		//		*ret = value;
		//	else
		//	{
		//		*ret = gVar.toString();
		//	}
		//}
		//else
		//{
		//}

		QString s = var.toString();

		importVariablesToLua(s);

		QString exprStrUTF8;
		qint64 value = 0;
		if (exprTo(s, &value))
			*ret = value;
		else if (exprTo(s, &exprStrUTF8))
			*ret = exprStrUTF8;
		else
			*ret = var;
	}
	else
	{
		*ret = var;
	}

	return true;
}

//嘗試取指定位置的token轉為按照double -> qint64 -> string順序檢查
QVariant Parser::checkValue(const TokenMap TK, qint64 idx, QVariant::Type type)
{
	QVariant varValue;
	qint64 ivalue;
	QString text;

	if (checkInteger(currentLineTokens_, idx, &ivalue))
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
		else
			varValue = 0ll;
	}

	return varValue;
}

//檢查跳轉是否滿足，和跳轉的方式
qint64 Parser::checkJump(const TokenMap& TK, qint64 idx, bool expr, JumpBehavior behavior)
{
	bool okJump = false;
	if (behavior == JumpBehavior::FailedJump)
	{
		okJump = !expr;
	}
	else
	{
		okJump = expr;
	}

	if (okJump)
	{
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
				QVariant var = checkValue(TK, idx, QVariant::LongLong);
				QVariant::Type type = var.type();
				if (type == QVariant::String)
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
					return Parser::kArgError + idx;
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

	return Parser::kNoChange;
}

//檢查"調用"是否傳入參數
void Parser::checkArgs()
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QVariantList list;
	if (tokens_.contains(lineNumber_))
	{
		TokenMap TK = tokens_.value(lineNumber_);
		qint64 size = TK.size();
		for (qint64 i = kCallPlaceHoldSize; i < size; ++i)
		{
			Token token = TK.value(i);
			QVariant var = checkValue(TK, i, QVariant::Int);
			if (!var.isValid())
				var = 0ll;

			if (token.type != TK_FUZZY)
				list.append(var);
			else
				list.append(0ll);
		}
	}

	//無論如何都要在調用call之後壓入新的參數堆棧
	callArgsStack_.push(list);
}

//檢查是否使用問號關鍵字
bool Parser::checkFuzzyValue(const QString& varName, QVariant* pvalue)
{
	QVariant varValue = currentLineTokens_.value(1).data;
	if (!varValue.isValid())
		return false;

	QString opStr = varValue.toString().simplified();
	if (!opStr.startsWith(kFuzzyPrefix))
		return false;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString valueStr = varValue.toString().simplified();

	int type = 1;

	QRegularExpressionMatch match = util::rexSquareBrackets.match(valueStr);
	if (match.hasMatch())
		type = match.captured(1).toInt();

	QString msg;
	if (valueStr.contains(util::rexQuoteWithParentheses))
	{
		match = util::rexQuoteWithParentheses.match(valueStr);
		if (match.hasMatch())
		{
			msg = match.captured(1);
			QVariantHash args = getLocalVars();
			QString varName = msg;
			if (args.contains(varName))
			{
				QVariant::Type vtype = args.value(varName).type();
				if (vtype == QVariant::Int || vtype == QVariant::LongLong || vtype == QVariant::Double || vtype == QVariant::String)
					msg = args.value(varName).toString();
			}
			else if (isGlobalVarContains(varName))
			{
				QVariant var = getGlobalVarValue(varName);
				if (var.type() == QVariant::List)
					return false;

				msg = var.toString();
			}
			else
			{
				if (msg.startsWith('"') || msg.startsWith('\''))
					msg = msg.mid(1);
				if (msg.endsWith('"') || msg.endsWith('\''))
					msg = msg.mid(0, msg.length() - 1);
			}
		}
	}

	varValue.clear();
	varValue = getLocalVarValue(varName);
	if (!varValue.isValid())
		varValue = getGlobalVarValue(varName);

	if (2 == type)//整數輸入框
	{

		emit signalDispatcher.inputBoxShow(msg, QInputDialog::IntInput, &varValue);
		if (!varValue.isValid())
			return false;
		varValue = varValue.toLongLong();
		if (varValue.toInt() == 987654321)
		{
			requestInterruption();
			return false;
		}
	}
	else if (1 == type)// 字串輸入框
	{
		varValue.clear();
		emit signalDispatcher.inputBoxShow(msg, QInputDialog::TextInput, &varValue);
		if (!varValue.isValid())
			return false;
		varValue = varValue.toString();
		if (varValue.toInt() == 987654321)
		{
			requestInterruption();
			return false;
		}
	}

	if (!varValue.isValid())
		return false;

	if (pvalue == nullptr)
		return false;

	*pvalue = varValue;
	insertVar(varName, varValue);
	return true;
}

//檢查是否需要跳過整個function代碼塊
bool Parser::checkCallStack()
{
	const QString cmd = currentLineTokens_.value(0).data.toString();
	if (cmd != "function")
		return false;

	const QString funname = currentLineTokens_.value(1).data.toString();
	if (ctorCallBackFlag_ == 1 && funname == "ctor")
	{
		ctorCallBackFlag_ = 2;
		return false;
	}

	if (dtorCallBackFlag_ == 1 && funname == "dtor")
	{
		ctorCallBackFlag_ = 2;
		return false;
	}

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

		qint64 lastRow = callStack_.top() - 1;

		//匹配棧頂紀錄的function名稱與當前執行名稱是否一致
		QString oldname = tokens_.value(lastRow).value(1).data.toString();
		if (oldname != funname)
		{
			if (!checkString(tokens_.value(lastRow), 1, &oldname))
				break;
		}

		return false;
	} while (false);

	if (!functionChunks_.contains(funname))
		return false;

	FunctionChunk chunk = functionChunks_.value(funname);
	jump(chunk.end - chunk.begin + 1, true);
	return true;
}

//插入局變量
void Parser::insertLocalVar(const QString& name, const QVariant& value)
{
	QVariantHash& hash = getLocalVarsRef();
	if (value.isValid())
	{
		QVariant::Type type = value.type();
		if (type != QVariant::LongLong && type != QVariant::String && type != QVariant::List)
		{
			bool ok = false;
			qint64 val = value.toLongLong(&ok);
			if (ok)
				hash.insert(name, val);
			else
				hash.insert(name, 0ll);
			return;
		}
		hash.insert(name, value);
	}

}

//插入全局變量
void Parser::insertGlobalVar(const QString& name, const QVariant& value)
{
	if (globalVarLock_ == nullptr)
		return;

	QWriteLocker locker(globalVarLock_);
	if (variables_ != nullptr)
	{
		if (value.isValid())
		{
			QVariant::Type type = value.type();
			if (type != QVariant::LongLong && type != QVariant::String && type != QVariant::List)
			{
				bool ok = false;
				qint64 val = value.toLongLong(&ok);
				if (ok)
					variables_->insert(name, val);
				else
					variables_->insert(name, 0ll);
				return;
			}
			variables_->insert(name, value);
		}
	}
}

//檢查是否被keyword包裹
bool Parser::isTextWrapped(const QString& text, const QString& keyword)
{
	int singleQuoteIndex = text.indexOf("'");
	int doubleQuoteIndex = text.indexOf('"');

	if (singleQuoteIndex != -1 && singleQuoteIndex < text.indexOf(keyword))
		return true;

	if (singleQuoteIndex != -1 && doubleQuoteIndex < text.indexOf(keyword))
		return true;

	if (doubleQuoteIndex != -1 && doubleQuoteIndex < text.indexOf(keyword))
		return true;

	if (doubleQuoteIndex != -1 && singleQuoteIndex < text.indexOf(keyword))
		return true;

	return false;
}

//表達式替換內容
bool Parser::importVariablesToLua(const QString& expr)
{
	QVariantHash globalVars = getGlobalVars();
	for (auto it = globalVars.cbegin(); it != globalVars.cend(); ++it)
	{
		QString key = it.key();
		QByteArray keyBytes = key.toUtf8();
		QString value = it.value().toString();
		if (it.value().type() == QVariant::String)
		{
			if (!value.startsWith("{") && !value.endsWith("}"))
			{
				bool ok = false;
				it.value().toInt(&ok);
				if (!ok || value.isEmpty())
					lua_.set(keyBytes.constData(), value.toUtf8().constData());
				else
					lua_.set(keyBytes.constData(), it.value().toLongLong());
			}
			else//table
			{
				QString content = QString("%1 = %2;").arg(key).arg(value);
				std::string contentBytes = content.toUtf8().constData();
				lua_.safe_script(contentBytes);
			}
		}
		else
			lua_.set(keyBytes.constData(), it.value().toLongLong());
	}

	QVariantHash localVars = getLocalVars();
	localVarList.clear();
	for (auto it = localVars.cbegin(); it != localVars.cend(); ++it)
	{
		QString key = it.key();
		QString value = it.value().toString();
		if (it.value().type() == QVariant::String && !value.startsWith("{") && !value.endsWith("}"))
		{
			bool ok = false;
			it.value().toInt(&ok);

			if (!ok || value.isEmpty())
				value = QString("'%1'").arg(value);
		}
		QString local = QString("local %1 = %2;").arg(key).arg(value);
		localVarList.append(local);
	}

	return updateSysConstKeyword(expr);
}

//表達式替換系統常量
bool Parser::updateSysConstKeyword(const QString& expr)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return false;

	bool bret = false;
	QString rexStart = "pet";

	//這裡寫成一串會出現VS語法bug，所以分開寫
	const QString rexMiddleStart = R"(\[(?:'([^']*)'|")";
	const QString rexMiddleMid = R"(([^ "]*))";
	const QString rexMEnd = R"("|([\w\p{Han}]+))\])";
	const QString rexExtra = R"(\.(\w+))";

	static const QRegularExpression rexPlayer(R"(char\.(\w+))");
	const QRegularExpression rexPet(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	static const QRegularExpression rexPetEx(R"(pet\.(\w+))");

	static const QRegularExpression rexItem(R"(item\[([\w\p{Han}]+)\]\.(\w+))");

	rexStart = "item";
	const QRegularExpression rexItemEx(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	static const QRegularExpression rexItemSup(R"(item\.(\w+))");

	rexStart = "team";
	const QRegularExpression rexTeam(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	static const QRegularExpression rexTeamEx(R"(team\.(\w+))");

	static const QRegularExpression rexMap(R"(map\.(\w+))");

	rexStart = "card";
	const QRegularExpression rexCard(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "chat";
	const QRegularExpression rexChat(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd);

	rexStart = "unit";
	const QRegularExpression rexUnit(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "battle";
	const QRegularExpression rexBattle(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	static const QRegularExpression rexBattleEx(R"(battle\.(\w+))");

	rexStart = "dialog";
	const QRegularExpression rexDialog(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd);

	static const QRegularExpression rexDialogEx(R"(dialog\.(\w+))");

	PC _pc = injector.server->getPC();

	QRegularExpressionMatch match = rexPlayer.match(expr);
	//char\.(\w+)
	if (match.hasMatch())
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
	}

	//pet\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexPet.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["pet"].valid())
			lua_["pet"] = lua_.create_table();
		else
		{
			if (!lua_["pet"].is<sol::table>())
				lua_["pet"] = lua_.create_table();
		}

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

			if (!lua_["pet"][index].valid())
				lua_["pet"][index] = lua_.create_table();
			else
			{
				if (!lua_["pet"][index].is<sol::table>())
					lua_["pet"][index] = lua_.create_table();
			}

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

			PetState state = pet.state;
			QString str = hash.key(state, "");
			lua_["pet"][index]["state"] = str.toUtf8().constData();

			lua_["pet"][index]["power"] = qFloor((static_cast<double>(pet.atk + pet.def + pet.agi) + (static_cast<double>(pet.maxHp) / 4.0)));
		}
	}

	//pet\.(\w+)
	match = rexPetEx.match(expr);
	if (match.hasMatch())
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
	match = rexItem.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["item"].valid())
			lua_["item"] = lua_.create_table();
		else
		{
			if (!lua_["item"].is<sol::table>())
				lua_["item"] = lua_.create_table();
		}

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

			lua_["item"][index]["stack"] = item.stack;
		}
	}

	//item\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexItemEx.match(expr);
	if (match.hasMatch())
	{
		if (!lua_["item"].valid())
			lua_["item"] = lua_.create_table();
		else
		{
			if (!lua_["item"].is<sol::table>())
				lua_["item"] = lua_.create_table();
		}

		for (const ITEM& it : _pc.item)
		{
			QVector<int> vWithNameAndMemo;
			QVector<int> vWithName;
			QVector<int> vWithMemo;
			qint64 countWithNameAndMemo = 0;
			qint64 countWithName = 0;
			qint64 countWithMemo = 0;
			QString itemName = it.name.simplified();
			QString itemMemo = it.memo.simplified();

			if (!injector.server->getItemIndexsByName(itemName, itemMemo, &vWithNameAndMemo) &&
				!injector.server->getItemIndexsByName(itemName, "", &vWithName) &&
				!injector.server->getItemIndexsByName("", itemMemo, &vWithMemo))
				continue;

			for (const int it : vWithNameAndMemo)
				countWithNameAndMemo += _pc.item[it].stack;

			for (const int it : vWithName)
				countWithName += _pc.item[it].stack;

			for (const int it : vWithMemo)
				countWithMemo += _pc.item[it].stack;

			QString keyWithNameAndMemo = QString("%1|%2").arg(itemName).arg(itemMemo);
			QString keyWithName = QString("%1").arg(itemName);
			QString keyWithMemo = QString("|%1").arg(itemMemo);

			if (!lua_["item"][keyWithNameAndMemo.toUtf8().constData()].valid())
				lua_["item"][keyWithNameAndMemo.toUtf8().constData()] = lua_.create_table();
			else
			{
				if (!lua_["item"][keyWithNameAndMemo.toUtf8().constData()].is<sol::table>())
					lua_["item"][keyWithNameAndMemo.toUtf8().constData()] = lua_.create_table();
			}

			if (!lua_["item"][keyWithName.toUtf8().constData()].valid())
				lua_["item"][keyWithName.toUtf8().constData()] = lua_.create_table();
			else
			{
				if (!lua_["item"][keyWithName.toUtf8().constData()].is<sol::table>())
					lua_["item"][keyWithName.toUtf8().constData()] = lua_.create_table();
			}

			if (!lua_["item"][keyWithMemo.toUtf8().constData()].valid())
				lua_["item"][keyWithMemo.toUtf8().constData()] = lua_.create_table();
			else
			{
				if (!lua_["item"][keyWithMemo.toUtf8().constData()].is<sol::table>())
					lua_["item"][keyWithMemo.toUtf8().constData()] = lua_.create_table();
			}

			lua_["item"][keyWithNameAndMemo.toUtf8().constData()]["count"] = countWithNameAndMemo;
			lua_["item"][keyWithName.toUtf8().constData()]["count"] = countWithName;
			lua_["item"][keyWithMemo.toUtf8().constData()]["count"] = countWithMemo;
			bret = true;
		}
	}

	//item\.(\w+)
	match = rexItemSup.match(expr);
	if (match.hasMatch())
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
	match = rexTeam.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["team"].valid())
			lua_["team"] = lua_.create_table();
		else
		{
			if (!lua_["team"].is<sol::table>())
				lua_["team"] = lua_.create_table();
		}

		for (int i = 0; i < MAX_PARTY; ++i)
		{
			PARTY party = injector.server->getParty(i);
			int index = i + 1;
			if (!lua_["team"][index].valid())
				lua_["team"][index] = lua_.create_table();
			else
			{
				if (!lua_["team"][index].is<sol::table>())
					lua_["team"][index] = lua_.create_table();
			}

			lua_["team"][index]["name"] = party.name.toUtf8().constData();

			lua_["team"][index]["lv"] = party.level;

			lua_["team"][index]["hp"] = party.hp;

			lua_["team"][index]["maxhp"] = party.maxHp;

			lua_["team"][index]["hpp"] = party.hpPercent;

			lua_["team"][index]["mp"] = party.mp;
		}
	}

	//team\.(\w+)
	match = rexTeamEx.match(expr);
	if (match.hasMatch())
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
	match = rexMap.match(expr);
	if (match.hasMatch())
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
	match = rexCard.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["card"].valid())
			lua_["card"] = lua_.create_table();
		else
		{
			if (!lua_["card"].is<sol::table>())
				lua_["card"] = lua_.create_table();
		}

		for (int i = 0; i < MAX_ADR_BOOK; ++i)
		{
			ADDRESS_BOOK addressBook = injector.server->getAddressBook(i);
			int index = i + 1;
			if (!lua_["card"][index].valid())
				lua_["card"][index] = lua_.create_table();
			else
			{
				if (!lua_["card"][index].is<sol::table>())
					lua_["card"][index] = lua_.create_table();
			}

			lua_["card"][index]["name"] = addressBook.name.toUtf8().constData();

			lua_["card"][index]["online"] = addressBook.onlineFlag ? 1 : 0;

			lua_["card"][index]["turn"] = addressBook.transmigration;

			lua_["card"][index]["dp"] = addressBook.dp;

			lua_["card"][index]["lv"] = addressBook.level;
		}
	}

	//chat\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexChat.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["chat"].valid())
			lua_["chat"] = lua_.create_table();
		else
		{
			if (!lua_["chat"].is<sol::table>())
				lua_["chat"] = lua_.create_table();
		}

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
	match = rexUnit.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["unit"].valid())
			lua_["unit"] = lua_.create_table();
		else
		{
			if (!lua_["unit"].is<sol::table>())
				lua_["unit"] = lua_.create_table();
		}

		QList<mapunit_t> units = injector.server->mapUnitHash.values();

		int size = units.size();
		for (int i = 0; i < size; ++i)
		{
			mapunit_t unit = units.at(i);
			if (!unit.isVisible)
				continue;

			int index = i + 1;

			if (!lua_["unit"][index].valid())
				lua_["unit"][index] = lua_.create_table();
			else
			{
				if (!lua_["unit"][index].is<sol::table>())
					lua_["unit"][index] = lua_.create_table();
			}

			lua_["unit"][index]["name"] = unit.name.toUtf8().constData();

			lua_["unit"][index]["fname"] = unit.freeName.toUtf8().constData();

			lua_["unit"][index]["family"] = unit.family.toUtf8().constData();

			lua_["unit"][index]["lv"] = unit.level;

			lua_["unit"][index]["dir"] = unit.dir;

			lua_["unit"][index]["x"] = unit.p.x();

			lua_["unit"][index]["y"] = unit.p.y();

			lua_["unit"][index]["gold"] = unit.gold;

			lua_["unit"][index]["model"] = unit.modelid;
		}
	}

	//battle\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexBattle.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		battledata_t battle = injector.server->getBattleData();

		if (!lua_["battle"].valid())
			lua_["battle"] = lua_.create_table();
		else
		{
			if (!lua_["battle"].is<sol::table>())
				lua_["battle"] = lua_.create_table();
		}

		int size = battle.objects.size();
		for (int i = 0; i < size; ++i)
		{
			battleobject_t obj = battle.objects.at(i);
			int index = i + 1;

			if (!lua_["battle"][index].valid())
				lua_["battle"][index] = lua_.create_table();
			else
			{
				if (!lua_["battle"][index].is<sol::table>())
					lua_["battle"][index] = lua_.create_table();
			}

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
	}

	//battle\.(\w+)
	match = rexBattleEx.match(expr);
	if (match.hasMatch())
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
	match = rexDialog.match(expr);
	if (match.hasMatch())
	{
		bret = true;
		if (!lua_["dialog"].valid())
			lua_["dialog"] = lua_.create_table();
		else
		{
			if (!lua_["dialog"].is<sol::table>())
				lua_["dialog"] = lua_.create_table();
		}

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

			if (!lua_["dialog"][index].valid())
				lua_["dialog"][index] = lua_.create_table();
			else
			{
				if (!lua_["dialog"][index].is<sol::table>())
					lua_["dialog"][index] = lua_.create_table();
			}

			lua_["dialog"][index] = text.toUtf8().constData();
		}
	}

	//dialog\.(\w+)
	match = rexDialogEx.match(expr);
	if (match.hasMatch())
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

//將表達式中所有變量替換成實際數值
void Parser::replaceToVariable(QString& expr)
{
	auto replaceToPlaceHold = [&expr](const QString& checkStr, QString replaceToStr, Qt::CaseSensitivity sen)->bool
	{
		if (replaceToStr.isEmpty())
			replaceToStr = checkStr;

		//不允許對帶有'對象'運算符且'不含方括號'的字串操作
		if (expr.contains(".") && !expr.contains("[") && !expr.contains("]"))
			return false;

		int index = -1;
		bool skippart = false;
		if (expr.contains("[") && expr.contains("]"))
		{
			//index從 [ 開始算起
			index = expr.indexOf(checkStr, expr.indexOf("[", 0, sen), sen);
			if (index == -1)
				return false;

			skippart = true;
		}
		else
		{
			index = expr.indexOf(checkStr, 0, sen);
			if (index == -1)
				return false;

			if (index > 0)
			{
				QChar ch = expr.at(index - 1);
				if (ch != ' ' && ch != ',' && ch != '(' && ch != '[' && ch != '{' && ch != '<' && ch != '.')
					return false;
			}
		}

		int countCheckStr = expr.count(checkStr, sen);

		bool bret = false;
		for (;;)
		{
			//取到空格逗號結尾或) 確保是一個獨立的詞
			int end = index + checkStr.length();
			while (end < expr.length())
			{
				QChar ch = expr.at(end);
				if (ch == '\'' || ch == '"')
					return false;

				if (ch == ' ' || ch == ',' || ch == ')' || ch == ']' || ch == '}' || ch == '>' || ch == '.')
					break;
				++end;
			}

			//index 到 end 之間的字符串
			QString endStr = expr.mid(index, end - index);
			if (endStr != checkStr)
				break;

			//replace to VAR_REPLACE_PLACEHOLD
			expr.replace(index, checkStr.length(), replaceToStr);

			bret = true;
			--countCheckStr;
			if (countCheckStr == 0)
				break;

			//next
			if (skippart)
			{
				index = expr.indexOf(checkStr, expr.indexOf("]", index, sen), sen) - 1;
				if (index == -1)
					break;
			}
			else
			{
				skippart = false;
				index += replaceToStr.length();
			}
		};

		return bret;
	};

	replaceToPlaceHold("true", "1", Qt::CaseInsensitive);
	replaceToPlaceHold("false", "0", Qt::CaseInsensitive);

	QVector<QPair<QString, QVariant>> tmpvec;

	QVariantHash args = getLocalVars();
	for (auto it = args.cbegin(); it != args.cend(); ++it)
		tmpvec.append(qMakePair(it.key(), it.value()));

	//按照長度排序，這樣才能先替換長的變量
	std::sort(tmpvec.begin(), tmpvec.end(), [](const QPair<QString, QVariant>& a, const QPair<QString, QVariant>& b)
		{
			if (a.first.length() == b.first.length())
			{
				//長度一樣則按照字母順序排序
				static const QLocale locale;
				static const QCollator collator(locale);
				return collator.compare(a.first, b.first) < 0;
			}

			return a.first.length() > b.first.length();
		});

	constexpr const char* CONST_STR_PLACEHOLD = "##########";//很重要，用於將被引號包圍的字符串替換成常量佔位，避免替換時出現問題
	constexpr const char* VAR_REPLACE_PLACEHOLD = "@@@@@@@@@@";//很重要，用於將表達式中的變量替換成常量佔位，避免替換時出現問題

	for (const QPair<QString, QVariant> it : tmpvec)
	{
		QString name = it.first;
		QVariant value = it.second;

		if (name == CONST_STR_PLACEHOLD)
			continue;

		if (!replaceToPlaceHold(name, VAR_REPLACE_PLACEHOLD, Qt::CaseSensitive))
			continue;

		QString oldText;
		if (isTextWrapped(expr, name))
		{
			//將引號包裹的部分暫時替換成棧位字
			int quoteIndex = expr.indexOf("'");
			if (quoteIndex == -1)
				quoteIndex = expr.indexOf('"');

			if (quoteIndex != -1)
			{
				int quoteIndex2 = expr.indexOf("'", quoteIndex + 1);
				if (quoteIndex2 == -1)
					quoteIndex2 = expr.indexOf('"', quoteIndex + 1);

				if (quoteIndex2 != -1)
				{
					oldText = expr.mid(quoteIndex, quoteIndex2 - quoteIndex + 1);
					expr.replace(quoteIndex, quoteIndex2 - quoteIndex + 1, CONST_STR_PLACEHOLD);
				}
			}
		}

		if (value.type() == QVariant::String)
			expr.replace(VAR_REPLACE_PLACEHOLD, QString(R"('%1')").arg(value.toString().trimmed()));
		else if (value.type() == QVariant::Int || value.type() == QVariant::LongLong || value.type() == QVariant::Double)
			expr.replace(VAR_REPLACE_PLACEHOLD, QString::number(value.toLongLong()));

		if (!oldText.isEmpty())
		{
			//將引號包裹的部分恢復
			expr.replace(CONST_STR_PLACEHOLD, oldText);
		}
	}

	tmpvec.clear();

	VariantSafeHash* pglobalhash = getGlobalVarPointer();

	for (auto it = pglobalhash->cbegin(); it != pglobalhash->cend(); ++it)
	{
		tmpvec.append(qMakePair(it.key(), it.value()));
	}

	//按照長度排序，這樣才能先替換長的變量
	std::sort(tmpvec.begin(), tmpvec.end(), [](const QPair<QString, QVariant>& a, const QPair<QString, QVariant>& b)
		{
			if (a.first.length() == b.first.length())
			{
				//長度一樣則按照字母順序排序
				static const QLocale locale;
				static const QCollator collator(locale);
				return collator.compare(a.first, b.first) < 0;
			}

			return a.first.length() > b.first.length();
		});

	for (const QPair<QString, QVariant> it : tmpvec)
	{
		QString name = it.first;
		QVariant value = it.second;

		if (name == CONST_STR_PLACEHOLD)
			continue;

		if (!replaceToPlaceHold(name, VAR_REPLACE_PLACEHOLD, Qt::CaseSensitive))
			continue;

		QString oldText;
		if (isTextWrapped(expr, name))
		{
			//將引號包裹的部分暫時替換成棧位字 __PLACEHOLD__
			int quoteIndex = expr.indexOf("'");
			if (quoteIndex == -1)
				quoteIndex = expr.indexOf('"');

			if (quoteIndex != -1)
			{
				int quoteIndex2 = expr.indexOf("'", quoteIndex + 1);
				if (quoteIndex2 == -1)
					quoteIndex2 = expr.indexOf('"', quoteIndex + 1);

				if (quoteIndex2 != -1)
				{
					oldText = expr.mid(quoteIndex, quoteIndex2 - quoteIndex + 1);
					expr.replace(quoteIndex, quoteIndex2 - quoteIndex + 1, CONST_STR_PLACEHOLD);
				}
			}
		}

		if (value.type() == QVariant::String && !isTextWrapped(expr, name))
			expr.replace(VAR_REPLACE_PLACEHOLD, QString(R"('%1')").arg(value.toString().trimmed()));
		else if (value.type() == QVariant::Int || value.type() == QVariant::LongLong || value.type() == QVariant::Double)
			expr.replace(VAR_REPLACE_PLACEHOLD, QString::number(value.toLongLong()));

		if (!oldText.isEmpty())
		{
			//將引號包裹的部分恢復
			expr.replace(CONST_STR_PLACEHOLD, oldText);
		}
	}

	expr = expr.trimmed();
}

//行跳轉
bool Parser::jump(qint64 line, bool noStack)
{
	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);
	lineNumber_ += line;
	return true;
}

//指定行跳轉
void Parser::jumpto(qint64 line, bool noStack)
{
	if (line - 1 < 0)
		line = 1;
	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);
	lineNumber_ = line - 1;
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
			qint64 jumpLineCount = returnIndex - lineNumber_;

			return jump(jumpLineCount, true);
		}
		return false;
	}
	else if (newName == "return" || newName == QString(u8"返回"))
	{
		bool bret = false;
		if (!callStack_.isEmpty())
		{
			qint64 returnIndex = callStack_.pop();
			qint64 jumpLineCount = returnIndex - lineNumber_;

			bret = jump(jumpLineCount, true);
		}

		if (!callArgsStack_.isEmpty())
			callArgsStack_.pop();//call行號出棧
		if (!localVarStack_.isEmpty())
			localVarStack_.pop();//label局變量出棧
		return bret;
	}
	else if (newName == "continue" || newName == QString(u8"繼續") || newName == QString(u8"继续"))
	{
		return processContinue();
	}
	else if (newName == "break" || newName == QString(u8"跳出"))
	{
		return processBreak();
	}

	qint64 jumpLine = matchLineFromFunction(name);
	if (jumpLine != -1 && name != "ctor" && name != "dtor")
	{
		callStack_.push(lineNumber_ + 1);
		jumpto(jumpLine, true);
		skipFunctionChunkDisable_ = true;
		return true;
	}

	jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		handleError(kLabelError, QString("'%1'").arg(name));
		return false;
	}

	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);

	qint64 jumpLineCount = jumpLine - lineNumber_;

	return jump(jumpLineCount, true);
}

//更新並記錄每個函數塊的開始行和結束行
void Parser::recordFunctionChunks()
{
	QHash<qint64, FunctionChunk> chunkHash;
	QMap<qint64, TokenMap> map;
	for (auto it = tokens_.cbegin(); it != tokens_.cend(); ++it)
		map.insert(it.key(), it.value());

	//這裡是為了避免沒有透過call調用函數的情況
	qint64 indentLevel = 0;
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		qint64 row = it.key();
		TokenMap tokens = it.value();
		QString cmd = tokens.value(0).data.toString();
		FunctionChunk chunk = chunkHash.value(indentLevel, FunctionChunk{});
		if (!chunk.name.isEmpty() && chunk.begin >= 0 && chunk.end > 0)
		{
			chunkHash.remove(indentLevel);
			functionChunks_.insert(chunk.name, chunk);
		}

		//紀錄function結束行
		if (cmd == "end")
		{
			--indentLevel;
			chunk = chunkHash.value(indentLevel, FunctionChunk{});
			chunk.end = row;
			if (!chunk.name.isEmpty() && chunk.begin >= 0 && chunk.end > 0)
			{
				chunkHash.remove(indentLevel);
				functionChunks_.insert(chunk.name, chunk);
				continue;
			}

			chunkHash.insert(indentLevel, chunk);
		}
		//紀錄function起始行
		else if (cmd == "function")
		{
			QString name = tokens.value(1).data.toString();
			chunk.name = name;
			chunk.begin = row;
			chunkHash.insert(indentLevel, chunk);
			++indentLevel;
		}
	}

#ifdef _DEBUG
	for (auto it = functionChunks_.cbegin(); it != functionChunks_.cend(); ++it)
		qDebug() << it.key() << it.value().name << it.value().begin << it.value().end;
#endif
}

//更新並記錄每個 for 塊的開始行和結束行
void Parser::recordForChunks()
{
	QHash<qint64, ForChunk> chunkHash;
	QMap<qint64, TokenMap> map;
	for (auto it = tokens_.cbegin(); it != tokens_.cend(); ++it)
		map.insert(it.key(), it.value());

	//這裡是為了避免沒有透過call調用函數的情況
	qint64 indentLevel = 0;
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		qint64 row = it.key();
		TokenMap tokens = it.value();
		QString cmd = tokens.value(0).data.toString().simplified();
		ForChunk chunk = chunkHash.value(indentLevel, ForChunk{});
		QString scopeKey = "";

		if (!chunk.name.isEmpty() && chunk.begin >= 0 && chunk.end > 0)
		{
			chunkHash.remove(indentLevel);
			if (scopeKey.isEmpty() && !chunk.name.isEmpty())
				scopeKey = QString("%1_%2").arg(chunk.name).arg(chunk.begin);
			forChunks_.insert(scopeKey, chunk);
		}

		//紀錄function結束行
		if (cmd == "endfor")
		{
			--indentLevel;
			chunk = chunkHash.value(indentLevel, ForChunk{});
			chunk.end = row;
			if (!chunk.name.isEmpty() && chunk.begin >= 0 && chunk.end > 0)
			{
				chunkHash.remove(indentLevel);

				if (scopeKey.isEmpty() && !chunk.name.isEmpty())
					scopeKey = QString("%1_%2").arg(chunk.name).arg(chunk.begin);
				forChunks_.insert(scopeKey, chunk);
				continue;
			}

			chunkHash.insert(indentLevel, chunk);
		}
		//紀錄function起始行
		else if (cmd == "for")
		{
			QString name = tokens.value(1).data.toString().simplified();
			chunk.name = name;
			chunk.begin = row;
			chunkHash.insert(indentLevel, chunk);
			++indentLevel;
		}
	}
}

//解析跳轉棧或調用棧相關訊息並發送信號
void Parser::generateStackInfo(qint64 type)
{
	if (mode_ != kSync)
		return;

	const QStack<qint64> stack = (type == 0) ? callStack_ : jmpStack_;
	QList<qint64> list = stack.toList();
	QVector<QPair<int, QString>> hash;
	if (!list.isEmpty())
	{
		for (const qint64 it : list)
		{
			if (!tokens_.contains(it - 1))
				continue;

			TokenMap tokens = tokens_.value(it - 1);
			if (tokens.isEmpty())
				continue;

			QStringList l;
			QString cmd = tokens.value(0).raw.trimmed() + " ";

			int size = tokens.size();
			for (qint64 i = 1; i < size; ++i)
				l.append(tokens.value(i).raw.trimmed());

			hash.append(qMakePair(it, cmd + l.join(", ")));
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (type == kModeCallStack)
		emit signalDispatcher.callStackInfoChanged(QVariant::fromValue(hash));
	else
		emit signalDispatcher.jumpStackInfoChanged(QVariant::fromValue(hash));
}

//處理"標記"，檢查是否有聲明局變量
void Parser::processLabel()
{
	QString token = currentLineTokens_.value(0).data.toString();
	if (token == "label")
		return;

	QVariantList args = getArgsRef();
	QVariantHash labelVars;
	const QStringList typeList = { "int", "double", "bool", "string", "table" };

	for (qint64 i = kCallPlaceHoldSize; i < currentLineTokens_.size(); ++i)
	{
		Token token = currentLineTokens_.value(i);
		if (token.type == TK_LABELVAR)
		{
			QString labelName = token.data.toString().simplified();
			if (labelName.isEmpty())
				continue;

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
					labelVars.insert(labelName.replace("double ", "", Qt::CaseInsensitive), static_cast<qint64>(qFloor(currnetVar.toDouble())));
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
					labelVars.insert(labelName.replace("bool ", "", Qt::CaseInsensitive), currnetVar.toLongLong() > 0 ? 1ll : 0ll);
				}
				else
				{
					QString expectedType = labelName;
					for (const QString& typeStr : typeList)
					{
						if (expectedType.startsWith(typeStr))
						{
							expectedType = type;
							labelVars.insert(labelName, "nil");
							handleError(kArgError + i - kCallPlaceHoldSize + 1, QString(QObject::tr("Invalid local variable type: %1")).arg(labelName));
							break;
						}
					}

					if (expectedType == labelName)
						labelVars.insert(labelName, currnetVar);
				}
			}
		}
	}

	localVarStack_.push(labelVars);

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
		return kError;
	}

	QString commandName = commandToken.data.toString();
	qint64 status = kNoChange;

	if (commandRegistry_.contains(commandName))
	{
		CommandRegistry function = commandRegistry_.value(commandName, nullptr);
		if (function == nullptr)
		{
			SPD_LOG(g_logger_name, QString("[parser] Command pointer is nullptr: %1").arg(commandName), SPD_WARN);
			return kError;
		}

		try
		{
			status = function(lineNumber_, tokens);
		}
		catch (const std::exception& e)
		{
			SPD_LOG(g_logger_name, QString("[parser] Command exception: %1").arg(e.what()), SPD_ERROR);
			return kError;
		}
		catch (...)
		{
			SPD_LOG(g_logger_name, QString("[parser] Command unknown exception: %1").arg(commandName), SPD_ERROR);
			return kError;
		}
	}
	else
	{
		SPD_LOG(g_logger_name, QString("[parser] Command not found: %1").arg(commandName), SPD_WARN);
		return kError;
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

//處理局變量
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

	QVariant firstValue = checkValue(currentLineTokens_, 1, QVariant::LongLong);
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

		QVariant varValue = checkValue(currentLineTokens_, i + 1, QVariant::LongLong);
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

//處理多變量聲明
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
	}
	else
		firstValue = checkValue(currentLineTokens_, 1, QVariant::LongLong);

	if (varCount == 1)
	{
		insertVar(varNames.front(), firstValue);
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
			insertVar(varName, firstValue);
			continue;
		}

		QVariant varValue;
		preExpr = getToken<QString>(i + 1);
		if (preExpr.contains("input("))
			exprTo(preExpr, &varValue);
		else
			varValue = checkValue(currentLineTokens_, i + 1, QVariant::LongLong);

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

		insertVar(varName, varValue);
		firstValue = varValue;
	}
}

QString Parser::getLuaTableString(const sol::table& t, int& deepth)
{
	if (deepth <= 0)
		return "";

	QString ret = "{ ";

	QStringList results;
	QStringList strKeyResults;

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
			key = QString::fromUtf8(pair.first.as<std::string>().c_str());

		if (pair.second.is<sol::table>())
			value = getLuaTableString(pair.second.as<sol::table>(), ++deepth);
		else if (pair.second.is<std::string>())
			value = QString("'%1'").arg(QString::fromUtf8(pair.second.as<std::string>().c_str()));
		else if (pair.second.is<qint64>())
			value = QString::number(pair.second.as<qint64>());
		else if (pair.second.is<double>())
			value = QString::number(qFloor(pair.second.as<double>()));
		else if (pair.second.is<bool>())
			value = pair.second.as<bool>() ? "true" : "false";
		else
			value = "nil";

		if (key.isEmpty())
		{
			if (nKey >= results.size())
			{
				for (qint64 i = results.size(); i <= nKey; ++i)
					results.append("nil");
			}

			results[nKey] = value;
		}
		else
			strKeyResults.append(QString("%1 = %2").arg(key).arg(value));
	}

	std::sort(strKeyResults.begin(), strKeyResults.end(), [](const QString& a, const QString& b)
		{
			static const QLocale locale;
			static const QCollator collator(locale);
			return collator.compare(a, b) < 0;
		});

	results.append(strKeyResults);

	ret += results.join(", ");
	ret += " }";

	return ret;
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
void Parser::processTableSet()
{
	QString varName = getToken<QString>(0);
	QString expr = getToken<QString>(1);
	if (expr.isEmpty())
		return;

	importVariablesToLua(expr);

	RESERVE type = getTokenType(0);

	if (isLocalVarContains(varName))
	{
		QString localVar = getLocalVarValue(varName).toString();
		if (!localVar.startsWith("{") || !localVar.endsWith("}"))
			return;

		QString exprStr = QString("%1\n%2;\nreturn %3;").arg(localVarList.join("\n")).arg(expr).arg(varName);
		insertGlobalVar("_LUAEXPR", exprStr);

		const std::string exprStrUTF8 = exprStr.toUtf8().constData();
		sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();
		if (!loaded_chunk.valid())
		{
			sol::error err = loaded_chunk;
			QString errStr = QString::fromUtf8(err.what());
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

		int deepth = 10;
		QString resultStr = getLuaTableString(retObject.as<sol::table>(), deepth);
		if (resultStr.isEmpty())
			return;

		insertLocalVar(varName, resultStr);
	}
	else if (type == TK_TABLESET && isGlobalVarContains(varName))
	{
		QString globalVar = getGlobalVarValue(varName).toString();
		if (!globalVar.startsWith("{") || !globalVar.endsWith("}"))
			return;

		QString exprStr = QString("%1\n%2 = %3;\n%4;\nreturn %5;").arg(localVarList.join("\n")).arg(varName).arg(globalVar).arg(expr).arg(varName);
		insertGlobalVar("_LUAEXPR", exprStr);

		const std::string exprStrUTF8 = exprStr.toUtf8().constData();
		sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();
		if (!loaded_chunk.valid())
		{
			sol::error err = loaded_chunk;
			QString errStr = QString::fromUtf8(err.what());
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

		int deepth = 10;
		QString resultStr = getLuaTableString(retObject.as<sol::table>(), deepth);
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

	QVariant varFirst = checkValue(currentLineTokens_, 0, QVariant::LongLong);
	QVariant::Type typeFirst = varFirst.type();

	RESERVE op = getTokenType(1);

	QVariant varSecond = checkValue(currentLineTokens_, 2, QVariant::LongLong);
	QVariant::Type typeSecond = varSecond.type();

	if (typeFirst == QVariant::String)
	{
		if (typeSecond == QVariant::String && op == TK_ADD)
		{
			QString exprStr = QString("%1 .. %2").arg(varFirst.toString()).arg(varSecond.toString());
			insertGlobalVar("_LUAEXPR", exprStr);

			const std::string exprStrUTF8 = exprStr.toUtf8().constData();
			sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
			lua_.collect_garbage();
			if (!loaded_chunk.valid())
			{
				sol::error err = loaded_chunk;
				QString errStr = QString::fromUtf8(err.what());
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
	QString expr = QString("%1 %2").arg(opStr).arg(varSecond.toLongLong());
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

	QVariant varValue = checkValue(currentLineTokens_, 0, QVariant::LongLong);

	QString expr;
	if (!checkString(currentLineTokens_, 1, &expr))
		return;

	QVariant::Type type = varValue.type();

	QVariant result;
	if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double)
	{
		qint64 value = 0;
		if (!exprTo(expr, &value))
			return;
		result = value;
	}
	else
		return;

	insertVar(varName, result);
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
			{ "battlepet", _pc.battlePetNo },
			{ "mailpet", _pc.mailPetNo },
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
					if (!checkString(currentLineTokens_, 4, &itemName))
						break;
					QVector<int> v;
					qint64 count = 0;
					if (injector.server->getItemIndexsByName(itemName, "", &v))
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

//處理if
bool Parser::processIfCompare()
{
	QString expr = getToken<QString>(1).simplified();

	importVariablesToLua(expr);

	if (expr.contains("\""))
	{
		expr = expr.replace("\"", "'");
	}

	insertGlobalVar("_IFEXPR", expr);

	qint64 nvalue = 0;
	bool result = false;
	if (exprTo(expr, &nvalue))
	{
		result = nvalue != 0;
	}

	insertGlobalVar("_IFRESULT", nvalue);

	return checkJump(currentLineTokens_, 2, result, SuccessJump) == kHasJump;
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
			insertVar(varName, distribution(gen));
			break;
		}
		else if (min > max)
		{
			insertVar(varName, 0);
			break;
		}

		std::uniform_int_distribution<qint64> distribution(min, max);
		insertVar(varName, distribution(gen));
	} while (false);
}

//處理"格式化"
void Parser::processFormation()
{

	auto formatTime = [](qint64 seconds)->QString
	{
		qint64 day = seconds / 86400ll;
		qint64 hours = (seconds % 86400ll) / 3600ll;
		qint64 minutes = (seconds % 3600ll) / 60ll;
		qint64 remainingSeconds = seconds % 60ll;

		return QString(QObject::tr("%1 day %2 hour %3 min %4 sec")).arg(day).arg(hours).arg(minutes).arg(remainingSeconds);
	};

	do
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		QString formatStr;
		checkString(currentLineTokens_, 2, &formatStr);
		if (formatStr.isEmpty())
			break;

		QVariantHash args = getLocalVars();
		QString key;
		QString keyWithTime;
		constexpr qint64 MAX_NESTING_DEPTH = 10;//最大嵌套深度
		static const QStringList constList = { "time", "date" };

		for (qint64 i = 0; i < MAX_NESTING_DEPTH; ++i)
		{
			for (auto it = args.cbegin(); it != args.cend(); ++it)
			{
				key = QString("{:%1}").arg(it.key());

				if (formatStr.contains(key))
				{
					formatStr.replace(key, it.value().toString());
					continue;
				}

				keyWithTime = QString("{T:%1}").arg(it.key());
				if (formatStr.contains(keyWithTime))
				{
					qint64 seconds = it.value().toLongLong();
					formatStr.replace(keyWithTime, formatTime(seconds));
				}
			}

			//查找字符串中包含 {:變數名} 全部替換成變數數值
			for (auto it = variables_->cbegin(); it != variables_->cend(); ++it)
			{
				key = QString("{:%1}").arg(it.key());

				if (formatStr.contains(key))
				{
					formatStr.replace(key, it.value().toString());
					continue;
				}

				keyWithTime = QString("{T:%1}").arg(it.key());
				if (formatStr.contains(keyWithTime))
				{
					qint64 seconds = it.value().toLongLong();
					formatStr.replace(keyWithTime, formatTime(seconds));
				}
			}

			for (const QString& str : constList)
			{
				key = QString("{C:%1}").arg(str);
				if (!formatStr.contains(key))
					continue;

				QString replaceStr;
				if (str == "date")
				{
					const QDateTime dt = QDateTime::currentDateTime();
					replaceStr = dt.toString("yyyy-MM-dd");

				}
				else if (str == "time")
				{
					const QDateTime dt = QDateTime::currentDateTime();
					replaceStr = dt.toString("hh:mm:ss:zzz");
				}
				else
					continue;

				formatStr.replace(key, replaceStr);
			}
		}

		QVariant varValue;
		qint64 argsize = tokens_.size();
		for (qint64 i = kFormatPlaceHoldSize; i < argsize; ++i)
		{
			varValue = checkValue(currentLineTokens_, i, QVariant::String);

			key = QString("{:%1}").arg(i - kFormatPlaceHoldSize + 1);

			if (formatStr.contains(key))
			{
				formatStr.replace(key, varValue.toString());
				continue;
			}

			keyWithTime = QString("{T:%1}").arg(i - kFormatPlaceHoldSize + 1);
			if (formatStr.contains(keyWithTime))
				formatStr.replace(keyWithTime, formatTime(varValue.toLongLong()));
		}

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

			const QDateTime time(QDateTime::currentDateTime());
			const QString timeStr(time.toString("hh:mm:ss:zzz"));
			QString src = "\0";

			QString msg = (QString("[%1 | @%2]: %3\0") \
				.arg(timeStr)
				.arg(lineNumber_ + 1, 3, 10, QLatin1Char(' ')).arg(formatStr));



			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->announce(formatStr, color);
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
				injector.server->talk(formatStr, color);
		}
		else
		{
			insertVar(varName, QVariant::fromValue(formatStr));
		}
	} while (false);
}

//處理"調用"
bool Parser::processCall()
{
	RESERVE type = getTokenType(1);
	do
	{
		if (type != TK_NAME)
			break;

		QString functionName;
		checkString(currentLineTokens_, 1, &functionName);
		if (functionName.isEmpty())
			break;

		qint64 jumpLine = matchLineFromFunction(functionName);
		if (jumpLine == -1)
			break;

		qint64 currentLine = lineNumber_;
		checkArgs();
		jumpto(jumpLine + 1, true);
		callStack_.push(currentLine + 1); // Push the next line index to the call stack

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
		if (type == TK_NAME)
		{
			QString labelName = getToken<QString>(1);
			if (labelName.isEmpty())
				break;

			jump(labelName, false);
		}
		else
		{
			QVariant var = checkValue(currentLineTokens_, 1, QVariant::LongLong);
			QVariant::Type type = var.type();
			if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double)
			{
				qint64 jumpLineCount = var.toLongLong();
				if (jumpLineCount == 0)
					break;

				jump(jumpLineCount, false);
				return true;
			}
			else if (type == QVariant::String)
			{
				QString labelName = var.toString();
				if (labelName.isEmpty())
					break;

				jump(labelName, false);
				return true;
			}
		}

	} while (false);
	return false;
}

//處理"指定行跳轉"
bool Parser::processJump()
{
	QVariant var = checkValue(currentLineTokens_, 1, QVariant::LongLong);

	qint64 line = var.toLongLong();
	if (line <= 0)
		return false;

	jumpto(line, false);
	return true;
}

//處理"返回"
void Parser::processReturn()
{
	retValue_ = checkValue(currentLineTokens_, 1, QVariant::LongLong);
	insertGlobalVar("vret", retValue_);

	if (!callArgsStack_.isEmpty())
		callArgsStack_.pop();//call行號出棧
	if (!localVarStack_.isEmpty())
		localVarStack_.pop();//label局變量出棧

	if (callStack_.isEmpty())
	{
		lineNumber_ = 0;
		return;
	}

	qint64 returnIndex = callStack_.pop();
	qint64 jumpLineCount = returnIndex - lineNumber_;
	jump(jumpLineCount, true);
}

//處理"返回跳轉"
void Parser::processBack()
{
	if (jmpStack_.isEmpty())
	{
		lineNumber_ = 0;
		return;
	}

	qint64 returnIndex = jmpStack_.pop();//jump行號出棧
	qint64 jumpLineCount = returnIndex - lineNumber_;
	jump(jumpLineCount, true);
}

//這裡是防止人為設置過長的延時導致腳本無法停止
void Parser::processDelay()
{
	Injector& injector = Injector::getInstance();
	qint64 extraDelay = injector.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000)
	{
		//將超過1秒的延時分段
		qint64 i = 0;
		qint64 size = extraDelay / 1000;
		for (i = 0; i < size; ++i)
		{
			if (isInterruptionRequested())
				return;
			QThread::msleep(1000);
		}
		if (extraDelay % 1000 > 0)
			QThread::msleep(extraDelay % 1000);
	}
	else if (extraDelay > 0)
	{
		QThread::msleep(extraDelay);
	}

	QThread::msleep(1);
}

//處理"遍歷"
bool Parser::processFor()
{
	QString varName = getToken<QString>(1);
	if (varName.isEmpty())
		return false;

	//init var value
	QVariant var = checkValue(currentLineTokens_, 2, QVariant::LongLong);
	if (var.type() != QVariant::LongLong)
		return false;

	//break condition
	QVariant breakVar = checkValue(currentLineTokens_, 3, QVariant::LongLong);
	if (breakVar.type() != QVariant::LongLong)
		return false;
	qint64 breakValue = breakVar.toLongLong();

	//step var value
	QVariant stepVar = checkValue(currentLineTokens_, 4, QVariant::LongLong);
	if (stepVar.type() != QVariant::LongLong || stepVar.toLongLong() == 0)
		stepVar = 1ll;
	qint64 step = stepVar.toLongLong();

	QString scopedKey = QString("%1_%2").arg(varName).arg(lineNumber_);

	QString cmpKey = "";
	if (!forStack_.isEmpty())
		cmpKey = QString("%1_%2").arg(forStack_.top().first).arg(forStack_.top().second);

	if (forStack_.isEmpty() || cmpKey != scopedKey)
	{
		insertLocalVar(varName, var.toLongLong());
		forStack_.push(qMakePair(varName, lineNumber_));//for行號入棧
	}
	else
	{
		QVariant value = getLocalVarValue(varName);
		if (value.type() != QVariant::LongLong)
			return false;

		qint64 nvalue = value.toLongLong();
		if ((nvalue >= breakValue && step >= 0) || (nvalue <= breakValue && step < 0))
		{
			if (!forStack_.isEmpty() && forChunks_.contains(scopedKey))
			{
				qint64 endline = forChunks_.value(scopedKey).end + 1;
				forStack_.pop();//for行號出棧
				jumpto(endline + 1, true);
				return true;
			}
		}
		else
		{
			nvalue += step;
			insertLocalVar(varName, nvalue);
		}
	}
	return false;
}

//處理"遍歷結束"
bool Parser::processEndFor()
{
	if (forStack_.isEmpty())
		return false;

	qint64 line = forStack_.top().second + 1;//跳for
	jumpto(line, true);
	return true;
}

//處理"繼續"
bool Parser::processContinue()
{
	if (forStack_.isEmpty())
		return false;

	qint64 line = forStack_.top().second + 1;//跳for
	jumpto(line, true);
	return true;
}

//處理"跳出"
bool Parser::processBreak()
{
	if (forStack_.isEmpty())
		return false;

	QString varName = forStack_.top().first;
	QString scopedKey = QString("%1_%2").arg(varName).arg(forStack_.top().second);

	if (!forChunks_.contains(scopedKey))
		return false;

	qint64 endline = forChunks_.value(scopedKey).end + 1;

	forStack_.pop();//for行號出棧

	jumpto(endline + 1, true);
	return true;
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

	recordFunctionChunks();
	recordForChunks();

	bool skip = false;
	RESERVE currentType = TK_UNK;
	QString name;

	for (;;)
	{
		if (empty())
		{
			//檢查是否存在腳本析構函數
			QString name;
			if (!getDtorCallBackLabelName(&name))
				break;
			else
			{
				callStack_.push(static_cast<qint64>(tokens_.size() - 1));
				jump(name, true);
			}
		}

		if (ctorCallBackFlag_ == 0)
		{
			//檢查是否存在腳本建構函數
			QHash<QString, qint64> hash = getLabels();
			constexpr const char* CTOR = "ctor";
			if (hash.contains(CTOR))
			{
				ctorCallBackFlag_ = 1;
				callStack_.push(0);
				jump(CTOR, true);
			}
		}

		currentLineTokens_ = tokens_.value(lineNumber_);

		currentType = getCurrentFirstTokenType();

		skip = currentType == RESERVE::TK_WHITESPACE || currentType == RESERVE::TK_SPACE || currentType == RESERVE::TK_COMMENT || currentType == RESERVE::TK_UNK;

		if (currentType == TK_LABEL)
		{
			if (checkCallStack())
				continue;
		}

		if (!skip)
		{
			processDelay();

			if (callBack_ != nullptr)
			{
				qint64 status = callBack_(lineNumber_, currentLineTokens_);
				if (status == kStop)
					break;
			}
		}

		switch (currentType)
		{
		case TK_COMMENT:
		case TK_WHITESPACE:
		case TK_SPACE:
		{
			break;
		}
		case TK_END:
		{
			lastCriticalError_ = kNoError;
			processClean();
			name.clear();
			if (!getDtorCallBackLabelName(&name))
				return;
			else
			{
				callStack_.push(static_cast<qint64>(tokens_.size() - 1));
				jump(name, true);
				continue;
			}
			break;
		}
		case TK_CMP:
		{
			if (processIfCompare())
			{
				continue;
			}
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
				if (getErrorCallBackLabelName(&name))
				{
					SPD_LOG(g_logger_name, "[parser] Command has error, has user error callback", SPD_WARN);
					jump(name, true);
					continue;
				}
				SPD_LOG(g_logger_name, "[parser] Command has error, not user callback was found", SPD_WARN);
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
		case TK_LOCALTABLESET:
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
		case TK_FOR:
		{
			if (processFor())
				continue;
			break;
		}
		case TK_ENDFOR:
		{
			if (processEndFor())
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
		case TK_CALL:
		{
			if (processCall())
			{
				generateStackInfo(kModeCallStack);
				continue;
			}
			break;
		}
		case TK_GOTO:
		{
			if (processGoto())
			{
				generateStackInfo(kModeJumpStack);
				continue;
			}
			break;
		}
		case TK_JMP:
		{
			if (processJump())
			{
				generateStackInfo(kModeJumpStack);
				continue;
			}
			break;
		}
		case TK_RETURN:
		{
			processReturn();
			generateStackInfo(kModeCallStack);
			continue;
		}
		case TK_BAK:
		{
			processBack();
			generateStackInfo(kModeJumpStack);
			continue;
		}
		case TK_LABEL:
		{
			processLabel();
			break;
		}
		default:
		{
			qDebug() << "Unexpected token type:" << currentType;
			SPD_LOG(g_logger_name, "[parser] Unexpected token type", SPD_WARN);
			break;
		}
		}

		if (isInterruptionRequested())
		{
			name.clear();
			if (!getDtorCallBackLabelName(&name))
				break;
			else
			{
				callStack_.push(static_cast<qint64>(tokens_.size() - 1));
				jump(name, true);
				continue;
			}
		}

		//導出變量訊息
		if (mode_ == kSync)
		{
			if (!skip)
			{
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

				emit signalDispatcher.varInfoImported(varhash);
			}
		}

		//移動至下一行
		next();
	}

	processClean();
}