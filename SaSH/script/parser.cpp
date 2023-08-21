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

#ifdef _DEBUG
#define exprtk_enable_debugging
#endif
#include <exprtk/exprtk.hpp>

#include <spdloger.hpp>
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
	case kLabelError:
		msg = QObject::tr("label incorrect or not exist") + extMsg;
		break;
	case kUnknownCommand:
	{
		QString cmd = currentLineTokens_.value(0).data.toString();
		msg = QObject::tr("unknown command: %1").arg(cmd) + extMsg;
		break;
	}
	default:
	{
		if (err >= kArgError && err <= kArgError + 20ll)
		{
			if (err == kArgError)
				msg = QObject::tr("argument error");
			else
				msg = QObject::tr("argument error") + QString(" idx:%1").arg(err - kArgError);
			break;
		}
		break;
	}
	}

	emit signalDispatcher.appendScriptLog(QObject::tr("error occured at line %1. detail:%2").arg(lineNumber_ + 1).arg(msg), 6);
}

//變量運算
template<typename T>
T Parser::calc(const QVariant& a, const QVariant& b, RESERVE operatorType)
{
	if constexpr (std::is_integral<T>::value)
	{
		if (operatorType == TK_ADD)
		{
			return a.value<T>() + b.value<T>();
		}
		else if (operatorType == TK_SUB)
		{
			return a.value<T>() - b.value<T>();
		}
		else if (operatorType == TK_MUL)
		{
			return a.value<T>() * b.value<T>();
		}
		else if (operatorType == TK_DIV)
		{
			return a.value<T>() / b.value<T>();
		}
		else if (operatorType == TK_INC)
		{
			return a.value<T>() + 1;
		}
		else if (operatorType == TK_DEC)
		{
			return a.value<T>() - 1;
		}
	}

	return T();
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

//取表達式結果
template <typename T>
bool Parser::exprMakeValue(const QString& expr, T* ret)
{
	using TKSymbolTable = exprtk::symbol_table<double>;
	using TKExpression = exprtk::expression<double>;
	using TKParser = exprtk::parser<double>;

	try
	{
		TKSymbolTable symbolTable;
		symbolTable.add_constants();

		TKParser parser;
		TKExpression expression;
		expression.register_symbol_table(symbolTable);

		const QByteArray exprStrUTF8 = expr.toUtf8();
		const std::string exprStr = std::string(exprStrUTF8.constData(), exprStrUTF8.size());
		if (!parser.compile(exprStr, expression))
			return false;

		if (ret != nullptr)
			*ret = static_cast<T>(expression.value());
		return true;
	}
	catch (const std::exception& e)
	{
		SPD_LOG(g_logger_name, QString("[parser] exprTo exception: %1").arg(e.what()), SPD_ERROR);
	}
	catch (...)
	{
		SPD_LOG(g_logger_name, "[parser] exprTo unknown exception", SPD_ERROR);
	}

	return false;
}

//取表達式結果
template <typename T>
bool Parser::exprTo(QString expr, T* ret)
{
	bool result = exprMakeValue(expr, ret);
	if (result)
		return true;

	static const QStringList logicOp = { "==", "!=", "<=", ">=", "<", ">" };
	//檢查表達式是否包含邏輯運算符
	QString opStr;
	for (const auto& op : logicOp)
	{
		if (expr.contains(op))
		{
			opStr = op;
			break;
		}
	}

	static const QHash<QString, RESERVE> hash = {
		{ "==", TK_EQ }, // "=="
		{ "!=", TK_NEQ }, // "!="
		{ ">", TK_GT }, // ">"
		{ "<", TK_LT }, // "<"
		{ ">=", TK_GEQ }, // ">="
		{ "<=", TK_LEQ }, // "<="
	};

	//以邏輯符號為分界分割左右邊
	QStringList exprList;
	QVariant A;
	QVariant B;
	if (opStr.isEmpty())
		return false;

	exprList = expr.split(opStr, Qt::SkipEmptyParts);

	if (exprList.size() != 2)
		return false;

	A = exprList.at(0).trimmed();
	B = exprList.at(1).trimmed();

	RESERVE type = hash.value(opStr, TK_UNK);
	if (type == TK_UNK)
		return false;

	result = compare(A, B, type);

	if (ret != nullptr)
		*ret = static_cast<T>(result);

	return true;
}

//取表達式結果
template <typename T>
bool Parser::exprTo(T value, QString expr, T* ret)
{
	using TKSymbolTable = exprtk::symbol_table<double>;
	using TKExpression = exprtk::expression<double>;
	using TKParser = exprtk::parser<double>;

	constexpr auto varName = "A";
	try
	{
		TKSymbolTable symbolTable;
		symbolTable.add_constants();
		double dvalue = static_cast<double>(value);
		symbolTable.add_variable(varName, dvalue);


		TKParser parser;
		TKExpression expression;
		expression.register_symbol_table(symbolTable);

		QString newExpr = QString("%1 %2").arg(varName).arg(expr);

		const std::string exprStr = newExpr.toStdString();
		if (!parser.compile(exprStr, expression))
			return false;

		expression.value();
		dvalue = symbolTable.variable_ref(varName);

		if (ret != nullptr)
			*ret = static_cast<T>(dvalue);
		return true;
	}
	catch (const std::exception& e)
	{
		SPD_LOG(g_logger_name, QString("[parser] exprTo2 exception: %1").arg(e.what()), SPD_ERROR);
	}
	catch (...)
	{
		SPD_LOG(g_logger_name, "[parser] exprTo2 unknown exception", SPD_ERROR);
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
		//檢查是否為區域變量
		QVariantHash args = getLocalVars();
		QString varName = var.toString();
		if (args.contains(varName))
		{
			QVariant::Type vtype = args.value(varName).type();
			if (vtype != QVariant::Int && vtype != QVariant::LongLong && vtype != QVariant::Double && vtype != QVariant::String)
				return false;

			*ret = args.value(varName).toString();

			return true;
		}
		else if (isGlobalVarContains(varName))
		{
			var = getGlobalVarValue(varName);
			if (var.type() == QVariant::List)
				return false;

			*ret = var.toString();
		}
		else
		{
			if (var.type() == QVariant::List)
				return false;

			*ret = var.toString();
			cycleReplace(*ret);

			//所有數學運算符號
			static const QRegularExpression regOp(R"(\+|\-|\*|\/|\%)");
			if (ret->contains(regOp))
			{
				//以運算符分割後檢查是否所有值都是整數，如果是 return false否則不處理
				QStringList values = ret->split(regOp, Qt::SkipEmptyParts);
				for (const QString& value : values)
				{
					if (value.isEmpty())
						continue;

					bool ok = false;
					value.simplified().toLongLong(&ok);
					if (!ok)
						return true;
				}

				return false;
			}
		}
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
		//檢查是否為區域變量
		QVariantHash args = getLocalVars();
		QString varName = var.toString();

		if (args.contains(varName)
			&& (args.value(varName).type() == QVariant::Int
				|| args.value(varName).type() == QVariant::LongLong
				|| args.value(varName).type() == QVariant::Double))
		{
			*ret = args.value(varName).toLongLong();
		}
		else if (args.contains(varName) && args.value(varName).type() == QVariant::String)
		{
			bool ok;
			qint64 value = args.value(varName).toLongLong(&ok);
			if (ok)
				*ret = value;
			else
				return false;

			return true;
		}
		else if (isGlobalVarContains(varName))
		{
			QVariant gVar = getGlobalVarValue(varName);
			if (gVar.type() == QVariant::Int || gVar.type() == QVariant::LongLong || gVar.type() == QVariant::Double)
			{
				bool ok;
				qint64 value = gVar.toLongLong(&ok);
				if (ok)
					*ret = value;
				else
					return false;
			}
			else
				return false;
		}
		else
		{
			if (var.type() == QVariant::List)
				return false;

			QString varValueStr = var.toString();

			cycleReplace(varValueStr);

			qint64 value = 0;
			if (!exprTo(varValueStr, &value))
				return false;

			if (-0x7fffffffffffffff >= value)
				return false;

			*ret = value;
		}
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
		QVariantHash args = getLocalVars();
		QString varName = var.toString();
		if (args.contains(varName))
		{
			QVariant::Type vtype = args.value(varName).type();
			if (vtype == QVariant::Int || vtype == QVariant::LongLong || vtype == QVariant::Double || vtype == QVariant::String)
				*ret = args.value(varName).toString();
			else
				return false;

			return true;
		}
		else if (isGlobalVarContains(varName))
		{
			QVariant gVar = getGlobalVarValue(varName);
			bool ok;
			qint64 value = gVar.toLongLong(&ok);
			if (ok)
				*ret = value;
			else
			{
				*ret = gVar.toString();
			}
		}
		else
		{
			QString s = var.toString();

			cycleReplace(s);

			*ret = s;
		}
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
		if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double)
			varValue = 0ll;
		else if (type == QVariant::String)
			varValue = "";
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
			QVariant var = checkValue(TK, idx, QVariant::Double);//這裡故意用Double，這樣最後沒有結果時強制報參數錯誤
			QVariant::Type type = var.type();
			if (type == QVariant::String)
			{
				label = var.toString();
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
void Parser::cycleReplace(QString& expr)
{
	//為了保證可以交互嵌套所以多幾次循環替換
	for (int i = 0; i < 4; ++i)
	{
		replaceSysConstKeyword(expr);
		replaceToVariable(expr);
	}
}

//表達式替換系統常量
void Parser::replaceSysConstKeyword(QString& expr)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	QString rexStart = "pet";

	//這裡寫成一串會出現VS語法bug，所以分開寫
	const QString rexMiddleStart = R"(\[(?:'([^']*)'|")";
	const QString rexMiddleMid = R"(([^ "]*))";
	const QString rexMEnd = R"("|(\d+))\])";
	const QString rexExtra = R"(\.(\w+))";

	static const QRegularExpression rexPlayer(R"(char\.(\w+))");
	const QRegularExpression rexPet(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "pet";
	const QRegularExpression rexPetEx(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd);

	static const QRegularExpression rexItem(R"(item\[(\d+)\]\.(\w+))");

	rexStart = "item";
	const QRegularExpression rexItemEx(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "team";
	const QRegularExpression rexTeamEx(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd);

	rexStart = "team";
	const QRegularExpression rexTeam(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	static const QRegularExpression rexMap(R"(map\.(\w+))");

	rexStart = "card";
	const QRegularExpression rexCard(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "chat";
	const QRegularExpression rexChatEx(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd);

	rexStart = "unit";
	const QRegularExpression rexUnit(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "battle";
	const QRegularExpression rexBattle(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd + rexExtra);

	rexStart = "battle";
	const QRegularExpression rexBattleEx(rexStart + rexMiddleStart + rexMiddleMid + rexMEnd);

	PC _pc = injector.server->getPC();

	QVariant a = 0;


	QRegularExpressionMatch match = rexPlayer.match(expr);
	//char\.(\w+)
	if (match.hasMatch())
	{
		QString strType = match.captured(1).simplified().toLower();
		if (strType.isEmpty())
			return;

		CompareType cmpType = comparePcTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		switch (cmpType)
		{
		case kPlayerName:
			a = _pc.name;
			break;
		case kPlayerFreeName:
			a = _pc.freeName;
			break;
		case kPlayerLevel:
			a = _pc.level;
			break;
		case kPlayerHp:
			a = _pc.hp;
			break;
		case kPlayerMaxHp:
			a = _pc.maxHp;
			break;
		case kPlayerHpPercent:
			a = _pc.hpPercent;
			break;
		case kPlayerMp:
			a = _pc.mp;
			break;
		case kPlayerMaxMp:
			a = _pc.maxMp;
			break;
		case kPlayerMpPercent:
			a = _pc.mpPercent;
			break;
		case kPlayerExp:
			a = _pc.exp;
			break;
		case kPlayerMaxExp:
			a = _pc.maxExp;
			break;
		case kPlayerStone:
			a = _pc.gold;
			break;
		case kPlayerVit:
			a = _pc.vital;
			break;
		case kPlayerStr:
			a = _pc.str;
			break;
		case kPlayerTgh:
			a = _pc.tgh;
			break;
		case kPlayerDex:
			a = _pc.dex;
			break;
		case kPlayerAtk:
			a = _pc.atk;
			break;
		case kPlayerDef:
			a = _pc.def;
			break;
		case kPlayerChasma:
			a = _pc.charm;
			break;
		case kPlayerTurn:
			a = _pc.transmigration;
			break;
		case kPlayerEarth:
			a = _pc.earth;
			break;
		case kPlayerWater:
			a = _pc.water;
			break;
		case kPlayerFire:
			a = _pc.fire;
			break;
		case kPlayerWind:
			a = _pc.wind;
			break;
		default:
			return;
		}

		expr.replace(QString("char.%1").arg(strType), a.toString());
	}

	//pet\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexPet.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		QString strType = match.captured(4).simplified().toLower();
		if (strIndex.isEmpty() || strType.isEmpty())
			return;

		CompareType cmpType = comparePetTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		int index = strIndex.toInt() - 1;
		if (index < 0 || index >= MAX_PET)
		{
			QHash<QString, qint64> hash = {
				{ u8"戰寵", _pc.battlePetNo },
				{ u8"騎寵", _pc.ridePetNo },
				{ u8"战宠", _pc.battlePetNo },
				{ u8"骑宠", _pc.ridePetNo },
				{ u8"battlepet", _pc.battlePetNo },
				{ u8"ride", _pc.ridePetNo },
			};

			if (!hash.contains(strIndex))
				return;

			index = hash.value(strIndex);
		}

		PET pet = injector.server->getPet(index);

		switch (cmpType)
		{
		case kPetName:
			a = pet.name;
			break;
		case kPetFreeName:
			a = pet.freeName;
			break;
		case kPetLevel:
			a = pet.level;
			break;
		case kPetHp:
			a = pet.hp;
			break;
		case kPetMaxHp:
			a = pet.maxHp;
			break;
		case kPetHpPercent:
			a = pet.hpPercent;
			break;
		case kPetExp:
			a = pet.exp;
			break;
		case kPetMaxExp:
			a = pet.maxExp;
			break;
		case kPetAtk:
			a = pet.atk;
			break;
		case kPetDef:
			a = pet.def;
			break;
		case kPetLoyal:
			a = pet.ai;
			break;
		case kPetTurn:
			a = pet.trn;
			break;
		case kPetEarth:
			a = pet.earth;
			break;
		case kPetWater:
			a = pet.water;
			break;
		case kPetFire:
			a = pet.fire;
			break;
		case kPetWind:
			a = pet.wind;
			break;
		case kPetState:
		{
			const QHash<QString, PetState> hash = {
				{ u8"battle", kBattle },
				{ u8"standby", kStandby },
				{ u8"mail", kMail },
				{ u8"rest", kRest },
				{ u8"ride", kRide },
			};

			PetState state = pet.state;
			QString str = hash.key(state, "");
			if (str.isEmpty())
				return;

			a = str;
			break;
		}
		case kPetPower:
			a = qFloor((static_cast<double>(pet.atk + pet.def + pet.quick) + (static_cast<double>(pet.maxHp) / 4.0)));
		default:
			return;
		}

		expr.replace(QString("pet['%1'].%2").arg(strIndex).arg(strType), a.toString());
		expr.replace(QString("pet[\"%1\"].%2").arg(strIndex).arg(strType), a.toString());
		expr.replace(QString("pet[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//pet\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexPetEx.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		if (strIndex == "count")
		{
			a = injector.server->getPartySize();
			expr.replace(QString("pet['%1']").arg(strIndex), a.toString());
			expr.replace(QString("pet[\"%1\"]").arg(strIndex), a.toString());
			expr.replace(QString("pet[%1]").arg(strIndex), a.toString());
		}
	}

	//item\[(\d+)\]\.(\w+)
	match = rexItem.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		QString strType = match.captured(2).simplified().toLower();

		if (strIndex.isEmpty() || strType.isEmpty())
			return;

		CompareType cmpType = compareItemTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		int index = strIndex.toInt() - 1;
		if (index >= 100 && index < 100 + CHAR_EQUIPPLACENUM)
		{
			index -= 100;
		}
		else if (index >= 0 && index <= MAX_ITEM - CHAR_EQUIPPLACENUM)
		{
			index += CHAR_EQUIPPLACENUM;
		}
		else
			return;

		if (index < 0 || index >= MAX_ITEM)
			return;

		ITEM item = _pc.item[index];

		switch (cmpType)
		{
		case kItemName:
			a = item.name;
			break;
		case kItemMemo:
			a = item.memo;
			break;
		case kItemDura:
		{
			QString damage = item.damage.simplified();
			if (damage.contains("%"))
				damage.replace("%", "");
			if (damage.contains("％"))
				damage.replace("％", "");

			bool ok = false;
			int dura = damage.toLongLong(&ok);
			if (!ok)
				a = 100;
			else
				a = dura;
			break;
		}
		case kItemLevel:
			a = item.level;
			break;
		case kItemStack:
			a = item.pile;
			break;
		default:
			return;
		}

		expr.replace(QString("item[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//item\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexItemEx.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		QString strType = match.captured(4).simplified().toLower();

		CompareType cmpType = compareItemTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		switch (cmpType)
		{
		case kitemCount:
		{
			QVector<int> v;
			qint64 count = 0;
			QStringList args = strIndex.split(util::rexOR);
			QString itemName = args.at(0).simplified();
			QString itemMemo;
			if (args.size() > 1)
				itemMemo = args.at(1).simplified();

			if (injector.server->getItemIndexsByName(itemName, itemMemo, &v))
			{
				for (const int it : v)
					count += _pc.item[it].pile;
			}

			a = count;
			break;
		}
		default:
			return;
		}

		expr.replace(QString("item['%1'].%2").arg(strIndex).arg(strType), a.toString());
		expr.replace(QString("item[\"%1\"].%2").arg(strIndex).arg(strType), a.toString());
		expr.replace(QString("item[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//team\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexTeam.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		QString strType = match.captured(4).simplified().toLower();

		CompareType cmpType = compareTeamTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		int index = strIndex.toInt() - 1;
		if (index < 0 || index >= MAX_PARTY)
			return;

		switch (cmpType)
		{
		case kTeamName:
			a = injector.server->getParty(index).name;
			break;
		case kTeamLevel:
			a = injector.server->getParty(index).level;
			break;
		case kTeamHp:
			a = injector.server->getParty(index).hp;
			break;

		case kTeamMaxHp:
			a = injector.server->getParty(index).maxHp;
			break;
		case kTeamHpPercent:
			a = injector.server->getParty(index).hpPercent;
			break;
		case kTeamMp:
			a = injector.server->getParty(index).mp;
			break;
		default:
			return;
		}

		expr.replace(QString("team['%1'].%2").arg(strIndex).arg(strType), a.toString());
		expr.replace(QString("team[\"%1\"].%2").arg(strIndex).arg(strType), a.toString());
		expr.replace(QString("team[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//team\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexTeamEx.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		if (strIndex == "count")
		{
			a = injector.server->getPartySize();
			expr.replace(QString("team['%1']").arg(strIndex), a.toString());
			expr.replace(QString("team[\"%1\"]").arg(strIndex), a.toString());
			expr.replace(QString("team[%1]").arg(strIndex), a.toString());
		}
	}

	//map\.(\w+)
	match = rexMap.match(expr);
	if (match.hasMatch())
	{
		QString strType = match.captured(1).simplified().toLower();
		if (strType.isEmpty())
			return;

		CompareType cmpType = compareMapTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		switch (cmpType)
		{
		case kMapName:
			a = injector.server->nowFloorName;
			break;
		case kMapFloor:
			a = injector.server->nowFloor;
			break;
		case kMapX:
			a = injector.server->getPoint().x();
			break;
		case kMapY:
			a = injector.server->getPoint().y();
			break;
		default:
			return;
		}

		expr.replace(QString("map.%1").arg(strType), a.toString());
	}

	//card\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexCard.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		QString strType = match.captured(4).simplified().toLower();
		if (strIndex.isEmpty() || strType.isEmpty())
			return;

		CompareType cmpType = compareCardTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		int index = strIndex.toInt() - 1;
		if (index < 0 || index > MAX_ADR_BOOK)
			return;

		switch (cmpType)
		{
		case kCardName:
			a = injector.server->getAddressBook(index).name;
			break;
		case kCardOnlineState:
			a = injector.server->getAddressBook(index).onlineFlag > 0 ? 1 : 0;
			break;
		case kCardTurn:
			a = injector.server->getAddressBook(index).transmigration;
			break;
		case kCardDp:
			a = injector.server->getAddressBook(index).dp;
			break;
		case kCardLevel:
			a = injector.server->getAddressBook(index).level;
			break;
		default:
			return;
		}

		expr.replace(QString("card[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//chat\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexTeamEx.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		int index = strIndex.toInt() - 1;
		if (index < 0 || index >= MAX_CHAT_HISTORY)
			return;

		a = injector.server->getChatHistory(index);
		expr.replace(QString("chat[%1]").arg(strIndex), a.toString());
	}

	//unit\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexUnit.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		QString strType = match.captured(4).simplified().toLower();
		if (strIndex.isEmpty() || strType.isEmpty())
			return;

		CompareType cmpType = compareUnitTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		QList<mapunit_t> units = injector.server->mapUnitHash.values();

		int index = strIndex.toInt() - 1;
		if (index < 0 || index >= units.size())
			return;

		mapunit_t unit = units.at(index);
		if (!unit.isvisible)
			return;

		switch (cmpType)
		{
		case kUnitName:
			a = unit.name;
			break;
		case kUnitFreeName:
			a = unit.freeName;
			break;
		case kUnitFamilyName:
			a = unit.fmname;
			break;
		case kUnitLevel:
			a = unit.level;
			break;
		case kUnitDir:
			a = unit.dir;
			break;
		case kUnitX:
			a = unit.p.x();
			break;
		case kUnitY:
			a = unit.p.y();
			break;
		case kUnitGold:
			a = unit.gold;
			break;
		case kUnitModelId:
			a = unit.graNo;
			break;
		default:
			return;
		}

		expr.replace(QString("unit[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//battle\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]\.(\w+)
	match = rexBattle.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		QString strType = match.captured(4).simplified().toLower();
		if (strIndex.isEmpty() || strType.isEmpty())
			return;

		CompareType cmpType = compareBattleUnitTypeMap.value(strType.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		int index = strIndex.toInt() - 1;
		if (index < 0 || index >= MAX_ENEMY)
			return;

		battledata_t battle = injector.server->getBattleData();
		int pos = -1;
		for (int i = 0; i < battle.objects.size(); i++)
		{
			if (battle.objects.at(i).pos == index)
			{
				pos = i;
				break;
			}
		}

		if (pos < 0)
			return;

		battleobject_t obj = battle.objects.at(pos);

		switch (cmpType)
		{
		case kBattleUnitPos:
			a = obj.pos + 1;
			break;
		case kBattleUnitName:
			a = obj.name;
			break;
		case kBattleUnitFreeName:
			a = obj.freename;
			break;
		case kBattleUnitModelId:
			a = obj.faceid;
			break;
		case kBattleUnitLevel:
			a = obj.level;
			break;
		case kBattleUnitHp:
			a = obj.hp;
			break;
		case kBattleUnitMaxHp:
			a = obj.maxHp;
			break;
		case kBattleUnitHpPercent:
			a = obj.hpPercent;
			break;
		case kBattleUnitStatus:
			a = injector.server->getBadStatusString(obj.status);
			break;
		case kBattleUnitRideFlag:
			a = obj.rideFlag > 0;
			break;
		case kBattleUnitRideName:
			a = obj.rideName;
			break;
		case kBattleUnitRideLevel:
			a = obj.rideLevel;
			break;
		case kBattleUnitRideHp:
			a = obj.rideHp;
			break;
		case kBattleUnitRideMaxHp:
			a = obj.rideMaxHp;
			break;
		case kBattleUnitRideHpPercent:
			a = obj.rideHpPercent;
			break;
		}

		expr.replace(QString("battle[%1].%2").arg(strIndex).arg(strType), a.toString());
	}

	//battle\[(?:'([^']*)'|"([^ "]*)"|(\d+))\]
	match = rexBattleEx.match(expr);
	if (match.hasMatch())
	{
		QString strIndex = match.captured(1).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(2).simplified().toLower();
		if (strIndex.isEmpty())
			strIndex = match.captured(3).simplified().toLower();
		if (strIndex.isEmpty())
			return;

		CompareType cmpType = compareBattleTypeMap.value(strIndex.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return;

		battledata_t battle = injector.server->getBattleData();

		switch (cmpType)
		{
		case kBattleRound:
			a = injector.server->BattleCliTurnNo + 1;
			break;
		case kBattleField:
			a = injector.server->getFieldString(battle.fieldAttr);
			break;
		case kBattleDuration:
			a = injector.server->battleDurationTimer.elapsed() / 1000;
			break;
		case kBattleTotalDuration:
			a = injector.server->battle_total_time / 1000 / 60;
			break;
		case kBattleTotalCombat:
			a = injector.server->battle_totol;
			break;
		default:
			return;
		}

		expr.replace(QString("battle['%1']").arg(strIndex), a.toString());
		expr.replace(QString("battle[\"%1\"]").arg(strIndex), a.toString());
		expr.replace(QString("battle[%1]").arg(strIndex), a.toString());
	}

	if (expr.contains("_GAME_"))
	{
		expr.replace("_GAME_", QString::number(injector.server->getGameStatus()));
	}

	if (expr.contains("_WORLD_"))
	{
		expr.replace("_WORLD_", QString::number(injector.server->getWorldStatus()));
	}
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

	expr.replace("^", " xor ");
	expr.replace("~", " nor ");
	expr.replace("&&", " and ");
	expr.replace("||", " or ");

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
	if (name.toLower() == "back")
	{
		if (!jmpStack_.isEmpty())
		{
			qint64 returnIndex = jmpStack_.pop();//jump行號出棧
			qint64 jumpLineCount = returnIndex - lineNumber_;

			return jump(jumpLineCount, true);
		}
		return false;
	}
	else if (name.toLower() == "return")
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
	else if (name.toLower() == "continue")
	{
		return processContinue();
	}
	else if (name.toLower() == "break")
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

//處理"變量"運算
void Parser::variableCalculate(RESERVE op, QVariant* pvar, const QVariant& varValue)
{
	if (nullptr == pvar)
		return;

	QVariant::Type type = pvar->type();

	switch (op)
	{
	case TK_ADD:
		*pvar = calc<qint64>(*pvar, varValue, op);
		break;
	case TK_SUB:
		*pvar = calc<qint64>(*pvar, varValue, op);
		break;
	case TK_MUL:
		*pvar = calc<qint64>(*pvar, varValue, op);
		break;
	case TK_DIV:
		*pvar = calc<qint64>(*pvar, varValue, op);
		break;
	case TK_INC:
		*pvar = calc<qint64>(*pvar, varValue, op);
		break;
	case TK_DEC:
		*pvar = calc<qint64>(*pvar, varValue, op);
		break;
	case TK_MOD:
		*pvar = pvar->toLongLong() % varValue.toLongLong();
		break;
	case TK_AND:
		*pvar = pvar->toLongLong() & varValue.toLongLong();
		break;
	case TK_OR:
		*pvar = pvar->toLongLong() | varValue.toLongLong();
		break;
	case TK_XOR:
		*pvar = pvar->toLongLong() ^ varValue.toLongLong();
		break;
	case TK_SHL:
		*pvar = pvar->toLongLong() << varValue.toLongLong();
		break;
	case TK_SHR:
		*pvar = pvar->toLongLong() >> varValue.toLongLong();
		break;
	default:
		break;
	}
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
	for (qint64 i = kCallPlaceHoldSize; i < currentLineTokens_.size(); ++i)
	{
		Token token = currentLineTokens_.value(i);
		if (token.type == TK_LABELVAR)
		{
			QString labelName = token.data.toString();
			if (labelName.isEmpty())
				continue;

			if (!args.isEmpty() && (args.size() > (i - kCallPlaceHoldSize)) && (args.at(i - kCallPlaceHoldSize).isValid()))
				labelVars.insert(labelName, args.at(i - kCallPlaceHoldSize));
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
		QString typeStr = getToken<QString>(2);
		if (typeStr.isEmpty())
			break;

		QVariant varValue;
		if (processGetSystemVarValue(varName, typeStr, varValue))
		{
			insertVar(varName, varValue);
			break;
		}

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

		if (checkFuzzyValue(varName, &varValue))
		{
			continue;
		}

		if (varValue.type() == QVariant::String && currentLineTokens_.value(i + 1).type != TK_CSTRING)
		{
			QVariant varValue2 = varValue;
			QString expr = varValue.toString();
			replaceToVariable(expr);
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
			insertVar(varName, firstValue);
			continue;
		}

		QVariant varValue = checkValue(currentLineTokens_, i + 1, QVariant::LongLong);
		if (i > 0 && !varValue.isValid())
		{
			varValue = firstValue;
		}

		if (checkFuzzyValue(varName, &varValue))
		{
			continue;
		}

		if (varValue.type() == QVariant::String && currentLineTokens_.value(i + 1).type != TK_CSTRING)
		{
			QVariant varValue2 = varValue;
			QString expr = varValue.toString();
			replaceToVariable(expr);
			if (exprTo(expr, &varValue2) && varValue2.isValid())
				varValue = varValue2.toLongLong();
		}

		insertVar(varName, varValue);
		firstValue = varValue;
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

	RESERVE op = getTokenType(1);

	QVariant var = checkValue(currentLineTokens_, 0, QVariant::LongLong);
	QVariant::Type type = var.type();
	QString opStr = getToken<QString>(1);

	QVariant varValue = checkValue(currentLineTokens_, 2, QVariant::LongLong);

	QString varValueStr = varValue.toString();
	replaceToVariable(varValueStr);

	if (type == QVariant::String)
	{
		if (varValue.type() == QVariant::String && op == TK_ADD)
		{
			QString newStr = varValue.toString();
			//前後如果包含單引號 或雙引號則去除
			if (newStr.startsWith('\'') || newStr.startsWith('"'))
				newStr = newStr.mid(1);
			if (newStr.endsWith('\'') || newStr.endsWith('"'))
				newStr = newStr.left(newStr.length() - 1);

			QString str = var.toString() + newStr;
			insertVar(varName, str);
		}
		return;
	}
	else
	{
		if (type == QVariant::Int || type == QVariant::LongLong || type == QVariant::Double)
		{
			qint64 value = 0;
			if (!exprTo(var.toLongLong(), QString("%1 %2").arg(opStr).arg(QString::number(varValue.toDouble(), 'f', 16)), &value))
				return;
			var = value;
		}
		else
			return;
	}

	insertVar(varName, var);
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

	replaceToVariable(expr);

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
			{ "vit", _pc.vital },
			{ "str", _pc.str }, { "tgh", _pc.tgh }, { "dex", _pc.dex },
			{ "exp", _pc.exp }, { "maxexp", _pc.maxExp },
			{ "lv", _pc.level },
			{ "atk", _pc.atk }, { "def", _pc.def },
			{ "agi", _pc.quick }, { "chasma", _pc.charm }, { "luck", _pc.luck },
			{ "earth", _pc.earth }, { "water", _pc.water }, { "fire", _pc.fire }, { "wind", _pc.wind },
			{ "stone", _pc.gold },

			{ "titleNo", _pc.titleNo },
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
			{ "familyname", _pc.familyName },
			//{ "", _pc.familyleader },
			{ "ridename", _pc.ridePetName },
			{ "ridelv", _pc.ridePetLevel },
			{ "earnstone", injector.server->recorder[0].goldearn },
			{ "earnrep", injector.server->recorder[0].repearn > 0 ? (injector.server->recorder[0].repearn / (injector.server->repTimer.elapsed() / 1000)) * 3600 : 0 },
			//{ "", _pc.familySprite },
			//{ "", _pc.baseGraNo },
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
			{ "valid", m.useFlag ? 1 : 0 },
			{ "cost", m.mp },
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
			{ "valid", s.useFlag ? 1 : 0 },
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
			{ "graNo", _pet.graNo },						//圖號
			{ "hp", _pet.hp }, { "maxhp", _pet.maxHp }, { "hpp", _pet.hpPercent },					//血量
			{ "mp", _pet.mp }, { "maxmp", _pet.maxMp }, { "mpp", _pet.mpPercent },					//魔力
			{ "exp", _pet.exp }, { "maxexp", _pet.maxExp },				//經驗值
			{ "lv", _pet.level },						//等級
			{ "atk", _pet.atk },						//攻擊力
			{ "def", _pet.def },						//防禦力
			{ "agi", _pet.quick },						//速度
			{ "loyal", _pet.ai },							//AI
			{ "earth", _pet.earth }, { "water", _pet.water }, { "fire", _pet.fire }, { "wind", _pet.wind },
			{ "maxskill", _pet.maxSkill },
			{ "turn", _pet.trn },						// 寵物轉生數
			{ "name", _pet.name },
			{ "fname", _pet.freeName },
			{ "valid", _pet.useFlag ? 1ll : 0ll },
			{ "turn", _pet.trn },
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
			{ "valid", _petskill.useFlag ? 1 : 0 },
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
							count += injector.server->getPC().item[it].pile;
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

		QHash<QString, QVariant> hash = {
			//{ "", item.color },
			{ "grano", item.graNo },
			{ "lv", item.level },
			{ "stack", item.pile },
			{ "valid", item.useFlag ? 1 : 0 },
			{ "field", item.field },
			{ "target", item.target },
			//{ "", item.deadTargetFlag },
			//{ "", item.sendFlag },
			{ "name", item.name },
			{ "name2", item.name2 },
			{ "memo", item.memo },
			{ "dura", item.damage },
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

		QHash<QString, QVariant> hash = {
			//{ "", item.color },
			{ "grano", item.graNo },
			{ "lv", item.level },
			{ "stack", item.pile },
			{ "valid", item.useFlag ? 1 : 0 },
			{ "field", item.field },
			{ "target", item.target },
			//{ "", item.deadTargetFlag },
			//{ "", item.sendFlag },
			{ "name", item.name },
			{ "name2", item.name2 },
			{ "memo", item.memo },
			{ "dura", item.damage },
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
		if (!ok)
			damageValue = 100;
		else
			damageValue = dura;

		QHash<QString, QVariant> hash = {
			//{ "", item.color },
			{ "grano", item.graNo },
			{ "level", item.level },
			{ "stack", item.pile },
			{ "valid", item.useFlag ? 1 : 0 },
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
			{ "valid", _party.useFlag ? 1 : 0 },
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
				varValue = dialog.seqno;
			}
			else if (typeStr == "unitid")
			{
				if (!valid)
				{
					varValue = 0;
					break;
				}

				qint64 unitid = injector.server->currentDialog.get().objindex;
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
				varValue = injector.server->BattleCliTurnNo;
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
	QString expr = getToken<QString>(1);

	cycleReplace(expr);

	if (expr.contains("\""))
	{
		expr = expr.replace("\"", "'");
	}

	static const QRegularExpression re(R"([A-Za-z\d\p{Han}_'"]+)");
	auto matchit = re.globalMatch(expr);
	while (matchit.hasNext())
	{
		//兩側加上引號
		auto match = matchit.next();
		QString text = match.captured(0);

		bool ok = false;
		text.toInt(&ok);
		if (ok)
			continue;

		if (text == "and" || text == "or")
			continue;

		QString newText = text;
		if (!newText.startsWith("'"))
			newText = "'" + newText;
		if (!newText.endsWith("'"))
			newText = newText + "'";

		expr.replace(text, newText);
	}

	insertGlobalVar("_IFEXPR", expr);

	double dvalue = 0.0;
	bool result = false;
	if (exprTo(expr, &dvalue))
	{
		result = dvalue > 0.0;
	}

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
		qint64 hours = seconds / 3600ll;
		qint64 minutes = (seconds % 3600ll) / 60ll;
		qint64 remainingSeconds = seconds % 60ll;

		return QString(QObject::tr("%1 hour %2 min %3 sec")).arg(hours).arg(minutes).arg(remainingSeconds);
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
		if (nvalue >= breakValue)
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