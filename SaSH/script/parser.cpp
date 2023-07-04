#include "stdafx.h"
#include "parser.h"

#include "signaldispatcher.h"
#include "injector.h"
#include "interpreter.h"

#ifdef _DEBUG
#define exprtk_enable_debugging
#endif
#include <exprtk/exprtk.hpp>

Parser::Parser()
	: ThreadPlugin(nullptr)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &Parser::requestInterruption, Qt::UniqueConnection);
}

//根據token解釋腳本
void Parser::parse(int line)
{

	lineNumber_ = line; //設置當前行號
	callStack_.clear(); //清空調用棧
	jmpStack_.clear();  //清空跳轉棧

	if (tokens_.isEmpty())
		return;

	variables_.reset(new VariantSafeHash());
	if (variables_.isNull())
		return;

	processTokens();
}

//"調用" 傳參數最小佔位
constexpr int kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr int kFormatPlaceHoldSize = 3;

//變量運算
template<typename T>
T calc(const QVariant& a, const QVariant& b, RESERVE operatorType)
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
		if constexpr (std::is_integral<T>::value)
			return a.value<T>() + 1;
		else
			return a.value<T>() + 1.0;
	}
	else if (operatorType == TK_DEC)
	{
		if constexpr (std::is_integral<T>::value)
			return a.value<T>() - 1;
		else
			return a.value<T>() - 1.0;
	}

	Q_UNREACHABLE();
}

//比較兩個QVariant以 a 的類型為主
bool Parser::compare(const QVariant& a, const QVariant& b, RESERVE type) const
{
	QVariant::Type aType = a.type();

	if (aType == QVariant::String)
	{
		QString strA = a.toString();
		QString strB = b.toString();

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
	//else if (aType == QVariant::Double)
	//{
	//	double numA = a.toDouble();
	//	double numB = b.toDouble();

	//	// 根据 type 进行相应的比较操作
	//	switch (type)
	//	{
	//	case TK_EQ: // "=="
	//		return (numA == numB);

	//	case TK_NEQ: // "!="
	//		return (numA != numB);

	//	case TK_GT: // ">"
	//		return (numA > numB);

	//	case TK_LT: // "<"
	//		return (numA < numB);

	//	case TK_GEQ: // ">="
	//		return (numA >= numB);

	//	case TK_LEQ: // "<="
	//		return (numA <= numB);

	//	default:
	//		return false; // 不支持的比较类型，返回 false
	//	}
	//}
	else// if (aType == QVariant::Int)
	{
		int numA = a.toInt();
		int numB = b.toInt();

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

	return false; // 不支持的类型，返回 false
}

template <typename T>
bool Parser::exprTo(QString expr, T* ret)
{
	auto makeValue = [](const QString& expr, T* ret)->bool
	{
		using TKSymbolTable = exprtk::symbol_table<double>;
		using TKExpression = exprtk::expression<double>;
		using TKParser = exprtk::parser<double>;

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
	};

	bool result = makeValue(expr, ret);
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

template <typename T>
bool Parser::exprTo(T value, QString expr, T* ret)
{
	using TKSymbolTable = exprtk::symbol_table<double>;
	using TKExpression = exprtk::expression<double>;
	using TKParser = exprtk::parser<double>;

	constexpr auto varName = "A";

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

//嘗試取指定位置的token轉為字符串
bool Parser::checkString(const TokenMap& TK, int idx, QString* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	VariantSafeHash args = getLabelVars();
	if (!var.isValid())
		return false;

	if (type == TK_CSTRING)
	{
		*ret = var.toString();
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::Int || vtype == QVariant::String)
				*ret = args.value(name).toString();
			//else if (vtype == QVariant::Double)
			//	*ret = QString::number(args.value(name).toDouble(), 'f', 16);
			else
				return false;
		}
		else if (variables_->contains(name))
		{
			*ret = variables_->value(name).toString();
		}
		else
			*ret = var.toString();
	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為整數
bool Parser::checkInt(const TokenMap& TK, int idx, int* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	VariantSafeHash args = getLabelVars();
	if (!var.isValid())
		return false;
	if (type == TK_CSTRING)
		return false;

	if (type == TK_INT)
	{
		bool ok = false;
		int value = var.toInt(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name) && args.value(name).type() == QVariant::Int)
		{
			*ret = args.value(name).toInt();
		}
		else if (args.contains(name) && args.value(name).type() == QVariant::String)
		{
			bool ok;
			int value = args.value(name).toInt(&ok);
			if (ok)
				*ret = value;
			else
				return false;
		}
		else if (variables_->contains(name))
		{
			if (variables_->value(name).type() == QVariant::Int)
			{
				bool ok;
				int value = variables_->value(name).toInt(&ok);
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
			QString varValueStr = var.toString();
			replaceToVariable(varValueStr);

			int value = 0;
			if (!exprTo(varValueStr, &value))
				return false;

			*ret = value;
		}
	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為雙精度浮點數
#if 0
bool Parser::checkDouble(const TokenMap& TK, int idx, double* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	VariantSafeHash args = getLabelVars();
	if (!var.isValid())
		return false;

	if (type == TK_CSTRING)
		return false;

	if (type == TK_DOUBLE)
	{
		bool ok = false;
		double value = var.toDouble(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name) && args.value(name).type() == QVariant::Double)
		{
			*ret = args.value(name).toDouble();
		}
		else if (args.contains(name) && args.value(name).type() == QVariant::String)
		{
			bool ok;
			double value = args.value(name).toDouble(&ok);
			if (ok && args.value(name).toString().count(".") == 1)
				*ret = value;
			else
				return false;
		}
		else if (variables_->contains(name))
		{
			bool ok;
			double value = variables_->value(name).toDouble(&ok);
			if (ok && variables_->value(name).toString().count(".") == 1)
				*ret = value;
			else
				return false;
		}
		else
		{
			QString varValueStr = var.toString();
			replaceToVariable(varValueStr);

			double value = 0;
			if (!exprTo(varValueStr, &value))
				return false;

			*ret = value;
		}
	}
	else
		return false;

	return true;
}
#endif

//嘗試取指定位置的token轉為QVariant
bool Parser::toVariant(const TokenMap& TK, int idx, QVariant* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	VariantSafeHash args = getLabelVars();
	if (!var.isValid())
		return false;

	if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::Int || vtype == QVariant::String)
				*ret = args.value(name).toString();
			//else if (vtype == QVariant::Double)
			//	*ret = QString::number(args.value(name).toDouble(), 'f', 8);
			else
				return false;
		}
		else if (variables_->contains(name))
		{
			bool ok;
			int value = variables_->value(name).toInt(&ok);
			if (ok)
				*ret = value;
			else
			{
				//double value = variables_->value(name).toDouble(&ok);
				//if (ok)
				//	*ret = value;
				//else
				*ret = variables_->value(name).toString();
			}
		}
		else
			*ret = var.toString();
	}
	else
	{
		*ret = var;
	}

	return true;
}

//嘗試取指定位置的token轉為按照double -> int -> string順序檢查
QVariant Parser::checkValue(const TokenMap TK, int idx, QVariant::Type type)
{
	QVariant varValue;
	int ivalue;
	//double dvalue;
	QString text;
	//if (checkDouble(currentLineTokens_, idx, &dvalue))
	//{
		//varValue = dvalue;
	//}
	if (checkInt(currentLineTokens_, idx, &ivalue))
	{
		varValue = ivalue;
	}
	else if (checkString(currentLineTokens_, idx, &text))
	{
		varValue = text;
	}
	else
	{
		if (type == QVariant::Int)
			varValue = 0;
		//else if (type == QVariant::Double)
			//varValue = 0.0;
		else if (type == QVariant::String)
			varValue = "";
	}

	return varValue;
}

//檢查跳轉是否滿足，和跳轉的方式
int Parser::checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior)
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
		int line = 0;
		if (TK.contains(idx))
		{
			QVariant var = checkValue(TK, idx, QVariant::Double);//這裡故意用Double，這樣最後沒有結果時強制報參數錯誤
			QVariant::Type type = var.type();
			if (type == QVariant::String)
			{
				label = var.toString();
			}
			else if (type == QVariant::Int)
			{
				bool ok = false;
				int value = 0;
				value = var.toInt(&ok);
				if (ok)
					line = value;
			}
			else
				return Parser::kArgError;

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
			return Parser::kArgError;

		return Parser::kHasJump;
	}

	return Parser::kNoChange;
}

//將表達式中所有變量替換成實際數值
void Parser::replaceToVariable(QString& expr)
{
	QVector<QPair<QString, QVariant>> tmpvec;

	VariantSafeHash args = getLabelVars();
	for (auto it = args.cbegin(); it != args.cend(); ++it)
		tmpvec.append(qMakePair(it.key(), it.value()));

	//按照長度排序，這樣才能先替換長的變量
	std::sort(tmpvec.begin(), tmpvec.end(), [](const QPair<QString, QVariant>& a, const QPair<QString, QVariant>& b)
		{
			if (a.first.length() == b.first.length())
			{
				//長度依樣則按照字母順序排序
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
		if (value.type() == QVariant::String)
			expr.replace(name, QString(R"('%1')").arg(value.toString().trimmed()));
		else
			expr.replace(name, QString::number(value.toInt()));
	}

	tmpvec.clear();

	VariantSafeHash variables = *variables_;
	for (auto it = variables.cbegin(); it != variables.cend(); ++it)
	{
		tmpvec.append(qMakePair(it.key(), it.value()));
	}

	//按照長度排序，這樣才能先替換長的變量
	std::sort(tmpvec.begin(), tmpvec.end(), [](const QPair<QString, QVariant>& a, const QPair<QString, QVariant>& b)
		{
			return a.first.length() > b.first.length();
		});

	for (const QPair<QString, QVariant> it : tmpvec)
	{
		QString name = it.first;
		QVariant value = it.second;
		if (value.type() == QVariant::String)
			expr.replace(name, QString(R"('%1')").arg(value.toString().trimmed()));
		else
			expr.replace(name, QString::number(value.toInt()));
	}

	expr.replace("^", " xor ");
	expr.replace("~", " nor ");
	expr.replace("&&", " and ");
	expr.replace("||", " or ");

	expr = expr.trimmed();
}

//行跳轉
void Parser::jump(int line, bool noStack)
{
	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);
	lineNumber_ += line;
}

//指定行跳轉
void Parser::jumpto(int line, bool noStack)
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
			int returnIndex = jmpStack_.pop();//jump行號出棧
			int jumpLineCount = returnIndex - lineNumber_;
			jump(jumpLineCount, true);
			return true;
		}
		return false;
	}
	else if (name.toLower() == "return")
	{
		if (!callArgsStack_.isEmpty())
			callArgsStack_.pop();//call行號出棧
		if (!labalVarStack_.isEmpty())
			labalVarStack_.pop();//label局變量出棧
		if (!callStack_.isEmpty())
		{
			int returnIndex = callStack_.pop();
			int jumpLineCount = returnIndex - lineNumber_;
			jump(jumpLineCount, true);
			return true;
		}
		return false;
	}

	int jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		handleError(kLabelError);
		return false;
	}

	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);

	int jumpLineCount = jumpLine - lineNumber_;
	jump(jumpLineCount, true);
	return true;
}

//檢查是否需要跳過整個function代碼塊
bool Parser::checkCallStack()
{
	const QString cmd = currentLineTokens_.value(0).data.toString();
	if (cmd != "function")
		return false;

	const QString funname = currentLineTokens_.value(1).data.toString();
	do
	{
		//棧為空時直接跳過整個代碼塊
		if (callStack_.isEmpty())
			break;

		int lastRow = callStack_.top() - 1;

		//匹配棧頂紀錄的function名稱與當前執行名稱是否一致
		QString oldname = tokens_.value(lastRow).value(1).data.toString();
		if (oldname != funname)
			break;

		return false;
	} while (false);

	if (!functionChunks_.contains(funname))
		return false;

	FunctionChunk chunk = functionChunks_.value(funname);
	jump(chunk.end - chunk.begin + 1, true);
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
		generateStackInfo(0);
		generateStackInfo(1);
	}

	//這裡是防止人為設置過長的延時導致腳本無法停止
	auto delay = [this, &injector]()
	{
		int extraDelay = injector.getValueHash(util::kScriptSpeedValue);
		if (extraDelay > 1000)
		{
			//將超過1秒的延時分段
			int i = 0;
			for (i = 0; i < extraDelay / 1000; ++i)
			{
				if (isInterruptionRequested())
					return;
				QThread::msleep(1000);
			}
			QThread::msleep(extraDelay % 1000);
		}
		else if (extraDelay >= 0)
		{
			QThread::msleep(extraDelay);
		}
		QThread::msleep(1);
	};


	QHash<int, FunctionChunk> chunkHash;
	QMap<int, TokenMap> map;
	for (auto it = tokens_.cbegin(); it != tokens_.cend(); ++it)
		map.insert(it.key(), it.value());

	//這裡是為了避免沒有透過call調用函數的情況
	int indentLevel = 0;
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		int row = it.key();
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
		if (cmd == "function")
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

	int oldlineNumber = lineNumber_;
	bool skip = false;
	RESERVE currentType = TK_UNK;
	QString name;
	for (;;)
	{
		if (isEmpty())
		{
			//檢查是否存在腳本析構函數
			QString name;
			if (!getDtorCallBackLabelName(&name))
				break;
			else
			{
				callStack_.push(tokens_.size() - 1);
				jump(name, true);
			}
		}

		if (!ctorCallBackFlag_)
		{
			//檢查是否存在腳本建構函數
			util::SafeHash<QString, int> hash = getLabels();
			constexpr const char* CTOR = "ctor";
			if (hash.contains(CTOR))
			{
				ctorCallBackFlag_ = true;
				callStack_.push(0);
				jump(CTOR, true);
			}
		}

		currentLineTokens_ = tokens_.value(lineNumber_);

		currentType = getCurrentFirstTokenType();
		skip = currentType == RESERVE::TK_WHITESPACE || currentType == RESERVE::TK_SPACE || currentType == RESERVE::TK_COMMENT || currentType == RESERVE::TK_UNK;

		oldlineNumber = lineNumber_;

		if (currentType == TK_LABEL)
		{
			if (checkCallStack())
				continue;
		}

		if (!skip)
		{
			delay();
			if (callBack_ != nullptr)
			{
				int status = callBack_(lineNumber_, currentLineTokens_);
				if (status == kStop)
					break;
			}
		}

		//qDebug() << "line:" << lineNumber_ << "tokens:" << currentLineTokens_.value(0).raw;

		switch (currentType)
		{
		case TK_END:
		{
			lastCriticalError_ = kNoError;
			processClean();
			name.clear();
			if (!getDtorCallBackLabelName(&name))
				return;
			else
			{
				callStack_.push(tokens_.size() - 1);
				jump(name, true);
				continue;
			}
			break;
		}
		case TK_CMP:
		{
			if (processIfCompare())
				continue;
			break;
		}
		case TK_CMD:
		{
			int ret = processCommand();
			if (ret == kHasJump)
			{
				generateStackInfo(1);
				continue;
			}
			else if (ret == kError)
			{
				handleError(ret);
				name.clear();
				if (!getErrorCallBackLabelName(&name))
				{
					return;
				}
				else
				{
					jump(name, true);
					continue;
				}
			}
			else if (ret == kArgError)
			{
				handleError(ret);
				name.clear();
				if (getErrorCallBackLabelName(&name))
				{
					jump(name, true);
					continue;
				}
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
		case TK_CALL:
		{
			if (processCall())
			{
				generateStackInfo(0);
				continue;
			}
			break;
		}
		case TK_GOTO:
		{
			if (processGoto())
			{
				generateStackInfo(1);
				continue;
			}
			break;
		}
		case TK_JMP:
		{
			if (processJump())
			{
				generateStackInfo(1);
				continue;
			}
			break;
		}
		case TK_RETURN:
		{
			processReturn();
			generateStackInfo(0);
			continue;
		}
		case TK_BAK:
		{
			processBack();
			generateStackInfo(1);
			continue;
		}
		case TK_LABEL:
		{
			processLabel();
			break;
		}
		case TK_COMMENT:
		case TK_WHITESPACE:
		case TK_SPACE:
			break;
		default:
			qDebug() << "Unexpected token type:" << currentType;
			break;
		}

		if (isInterruptionRequested())
		{
			name.clear();
			if (!getDtorCallBackLabelName(&name))
				break;
			else
			{
				callStack_.push(tokens_.size() - 1);
				jump(name, true);
				continue;
			}
		}

		//導出變量訊息
		if (mode_ == kSync)
		{
			if (!skip)
			{
				QHash<QString, QVariant> varhash;
				QHash<QString, QVariant> globalHash = variables_->toHash();
				QHash<QString, QVariant> localHash;
				if (!labalVarStack_.isEmpty())
					localHash = labalVarStack_.top().toHash();

				QString key;
				for (auto it = globalHash.cbegin(); it != globalHash.cend(); ++it)
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
		QThread::yieldCurrentThread();
	}

	processClean();
}

//處理"標記"，檢查是否有聲明局變量
void Parser::processLabel()
{
	QString token = currentLineTokens_.value(0).data.toString();
	if (token == "label")
		return;

	QVariantList args = getArgsRef();
	VariantSafeHash labelVars;
	for (int i = kCallPlaceHoldSize; i < currentLineTokens_.size(); ++i)
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

	labalVarStack_.push(labelVars);

}

//處理"結束"
void Parser::processClean()
{
	callStack_.clear();
	jmpStack_.clear();
	callArgsStack_.clear();
	labalVarStack_.clear();
}

//處理所有核心命令之外的所有命令
int Parser::processCommand()
{
	TokenMap tokens = getCurrentTokens();
	Token commandToken = tokens.value(0);
	QString commandName = commandToken.data.toString();
	int status = kNoChange;
	if (commandRegistry_.contains(commandName))
	{
		CommandRegistry function = commandRegistry_.value(commandName);
		if (function == nullptr)
		{
			qDebug() << "Command not registered:" << commandName;
			return status;
		}

		status = function(lineNumber_, tokens);
	}
	else
	{
		qDebug() << "Command not found:" << commandName;
	}
	return status;
}

void Parser::processLocalVariable()
{
	QString varNameStr = getToken<QString>(0);
	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	int varCount = varNames.count();

	//取區域變量表
	VariantSafeHash& args = getLabelVarsRef();

	for (int i = 0; i < varCount; ++i)
	{
		QString varName = varNames.at(i);
		if (varName.isEmpty())
		{
			args.insert(varName, 0);
			continue;
		}

		if (i + 1 >= currentLineTokens_.size())
		{
			args.insert(varName, 0);
			continue;
		}

		QVariant varValue = checkValue(currentLineTokens_, i + 1, QVariant::Int);

		args.insert(varName, varValue);
	}
}

bool Parser::checkFuzzyValue(const QString& varName, QVariant varValue, QVariant* pvalue)
{
	QString opStr = varValue.toString().simplified();
	if (!opStr.startsWith(kFuzzyPrefix))
		return false;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString valueStr = varValue.toString().simplified();

	//檢查是否使用?讓使用者輸入
	QString msg;
	checkString(currentLineTokens_, 3, &msg);

	if (valueStr.startsWith(kFuzzyPrefix + QString("2")))//整數輸入框
	{
		varValue.clear();
		emit signalDispatcher.inputBoxShow(msg, QInputDialog::IntInput, &varValue);
		if (!varValue.isValid())
			return false;
		varValue = varValue.toInt();
	}
	//else if (valueStr.startsWith(kFuzzyPrefix + QString("3"))) //雙精度浮點數輸入框
	//{
	//	varValue.clear();
	//	emit signalDispatcher.inputBoxShow(msg, QInputDialog::DoubleInput, &varValue);
	//	if (!varValue.isValid())
	//		return false;
	//	varValue = varValue.toDouble();
	//}
	else if ((valueStr.startsWith(kFuzzyPrefix) && valueStr.endsWith(kFuzzyPrefix)) || valueStr.startsWith(kFuzzyPrefix + QString("1")))// 字串輸入框
	{
		varValue.clear();
		emit signalDispatcher.inputBoxShow(msg, QInputDialog::TextInput, &varValue);
		if (!varValue.isValid())
			return false;
		varValue = varValue.toString();
	}

	if (!varValue.isValid())
		return false;

	if (pvalue == nullptr)
		return false;

	*pvalue = varValue;
	return true;
}

//處理"變量"
void Parser::processVariable(RESERVE type)
{

	switch (type)
	{
	case TK_VARDECL:
	{
		//取第一個參數
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		//取第二個參數
		QVariant varValue = getToken<QVariant>(2);
		if (!varValue.isValid())
			break;

		//檢查第二參數是否為邏輯運算符
		RESERVE op = getTokenType(2);
		if (!operatorTypes.contains(op))
		{

			//如果不是運算符，檢查是否為字符串
			//bool pass = false;

			QString text;
			int ivalue = 0;
			if (checkString(currentLineTokens_, 2, &text))
			{
				varValue = text;
			}
			else if (checkInt(currentLineTokens_, 2, &ivalue))
			{
				varValue = ivalue;
			}

			if (checkFuzzyValue(varName, varValue, &varValue))
			{
				variables_->insert(varName, varValue);
				break;
			}

			QString varStr = varValue.toString();
			if (processGetSystemVarValue(varName, varStr, varValue))
			{
				variables_->insert(varName, varValue);
				break;
			}

			//插入全局變量表
			variables_->insert(varName, varValue);
			break;
		}

		//下面是變量比較數值，先檢查變量是否存在
		if (!variables_->contains(varName))
		{
			if (op != TK_DEC && op != TK_INC)
				break;
			else
				variables_->insert(varName, 0);
		}


		if (op == TK_UNK)
			break;

		//取第三個參數
		varValue = getToken<QVariant>(3);
		//檢查第三個是否合法 ，如果不合法 且第二參數不是++或--，則跳出
		if (!varValue.isValid() && op != TK_DEC && op != TK_INC)
			break;

		//取第三個參數的類型
		//RESERVE varValueType = getTokenType(3);




		//將第一參數，要被運算的變量原數值取出
		QVariant var = variables_->value(varName, 0);

		//開始計算新值
		variableCalculate(op, &var, varValue);
		if (variables_->contains(varName))
			variables_->insert(varName, var);
		break;
	}
	case TK_VARFREE:
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		if (variables_->contains(varName))
			variables_->remove(varName);
		break;
	}
	case TK_VARCLR:
	{
		variables_->clear();
		break;
	}
	default:
		break;
	}
}

//處理多變量聲明
void Parser::processMultiVariable()
{
	QString varNameStr = getToken<QString>(0);
	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	int varCount = varNames.count();

	//取區域變量表
	VariantSafeHash args = getLabelVars();


	for (int i = 0; i < varCount; ++i)
	{
		QString varName = varNames.at(i);
		if (varName.isEmpty())
		{
			variables_->insert(varName, 0);
			continue;
		}

		if (i + 1 >= currentLineTokens_.size())
		{
			variables_->insert(varName, 0);
			continue;
		}

		QVariant varValue = checkValue(currentLineTokens_, i + 1, QVariant::Int);

		variables_->insert(varName, varValue);
	}
}

//處理變量自增自減
void Parser::processVariableIncDec()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())
		return;

	RESERVE op = getTokenType(1);


	VariantSafeHash& args = getLabelVarsRef();

	int value = 0;
	if (args.contains(varName))
		value = args.value(varName, 0).toInt();
	else
		value = getVar<int>(varName);

	if (op == TK_DEC)
		--value;
	else if (op == TK_INC)
		++value;

	if (args.contains(varName))
		args.insert(varName, value);
	else
		variables_->insert(varName, value);
}

//處理變量計算
void Parser::processVariableCAOs()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())
		return;

	RESERVE op = getTokenType(1);

	QVariant var = checkValue(currentLineTokens_, 0, QVariant::Int);
	QVariant::Type type = var.type();
	QString opStr = getToken<QString>(1);

	QVariant varValue = checkValue(currentLineTokens_, 2, QVariant::Int);

	VariantSafeHash& args = getLabelVarsRef();
	QString varValueStr = varValue.toString();
	replaceToVariable(varValueStr);

	if (type == QVariant::String)
	{
		if (varValue.type() == QVariant::String && op == TK_ADD)
		{
			QString str = var.toString() + varValue.toString();
			if (args.contains(varName))
				args.insert(varName, str);
			else
				variables_->insert(varName, str);
		}
		return;
	}
	else
	{

		if (type == QVariant::Int)
		{
			int value = 0;
			if (!exprTo(var.toInt(), QString("%1 %2").arg(opStr).arg(QString::number(varValue.toDouble(), 'f', 16)), &value))
				return;
			var = value;
		}
		//else if (type == QVariant::Double)
		//{
		//	double value = 0;
		//	if (!exprTo(var.toDouble(), QString("%1 %2").arg(opStr).arg(QString::number(varValue.toDouble(), 'f', 16)), &value))
		//		return;
		//	var = value;
		//}
		else
			return;
	}

	if (args.contains(varName))
		args.insert(varName, var);
	else
		variables_->insert(varName, var);
}

//處理變量表達式
void Parser::processVariableExpr()
{
	QString varName = getToken<QString>(0);
	if (varName.isEmpty())
		return;

	QVariant varValue = checkValue(currentLineTokens_, 0, QVariant::Int);

	QString expr;
	if (!checkString(currentLineTokens_, 1, &expr))
		return;

	replaceToVariable(expr);

	QVariant::Type type = varValue.type();

	QVariant result;
	if (type == QVariant::Int)
	{
		int value = 0;
		if (!exprTo(expr, &value))
			return;
		result = value;
	}
	//else if (type == QVariant::Double)
	//{
	//	double value = 0;
	//	if (!exprTo(expr, &value))
	//		return;
	//	result = value;
	//}
	else
		return;

	VariantSafeHash& args = getLabelVarsRef();
	if (args.contains(varName))
		args.insert(varName, result);
	else
		variables_->insert(varName, result);
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
		kPartyInfo,
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
		{ "party", kPartyInfo },
		{ "chat", kChatInfo },
		{ "dialog", kDialogInfo },
		{ "point", kPointInfo },
		{ "battle", kBattleInfo },

	};

	if (!systemVarNameHash.contains(trimmedStr))
		return false;

	SystemVarName index = systemVarNameHash.value(trimmedStr);
	bool bret = false;
	switch (index)
	{
	case kPlayerInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		VariantSafeHash& hashpc = injector.server->hashpc;
		if (!hashpc.contains(typeStr))
			break;

		varValue = hashpc.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kMagicInfo:
	{
		int magicIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &magicIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hashmagic = injector.server->hashmagic;
		if (!hashmagic.contains(magicIndex))
			break;

		QHash<QString, QVariant> hash = hashmagic.value(magicIndex);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kSkillInfo:
	{
		int skillIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &skillIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hashskill = injector.server->hashskill;
		if (!hashskill.contains(skillIndex))
			break;

		QHash<QString, QVariant> hash = hashskill.value(skillIndex);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPetInfo:
	{
		int petIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &petIndex))
		{
			QString typeStr;
			if (!checkString(currentLineTokens_, 3, &typeStr))
				break;

			if (typeStr == "count")
			{
				varValue = injector.server->hashpet.size();
				bret = true;
			}


			break;
		}

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hashpet = injector.server->hashpet;
		if (!hashpet.contains(petIndex))
			break;

		QHash<QString, QVariant> hash = hashpet.value(petIndex);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPetSkillInfo:
	{
		int petIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &petIndex))
			break;

		int skillIndex = -1;
		if (!checkInt(currentLineTokens_, 4, &skillIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 5, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<int, QHash<QString, QVariant>>>& hashpetskill = injector.server->hashpetskill;
		if (!hashpetskill.contains(petIndex))
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hash = hashpetskill.value(petIndex);
		if (!hash.contains(skillIndex))
			break;

		QHash<QString, QVariant> hash2 = hash.value(skillIndex);
		if (!hash2.contains(typeStr))
			break;

		varValue = hash2.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kMapInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		VariantSafeHash& hashmap = injector.server->hashmap;
		if (!hashmap.contains(typeStr))
			break;

		varValue = hashmap.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kItemInfo:
	{
		int itemIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &itemIndex))
		{
			QString typeStr;
			if (checkString(currentLineTokens_, 3, &typeStr))
			{
				typeStr = typeStr.simplified().toLower();
				if (typeStr == "space")
				{
					QVector<int> itemIndexs;
					int size = 0;
					if (injector.server->getItemEmptySpotIndexs(&itemIndexs))
						size = itemIndexs.size();

					varValue = size;
					bret = varValue.isValid();
					break;
				}
				else if (typeStr == "count")
				{
					QString itemName;
					if (!checkString(currentLineTokens_, 4, &itemName))
						break;
					QVector<int> v;
					int count = 0;
					if (injector.server->getItemIndexsByName(itemName, "", &v))
					{
						for (const int it : v)
							count += injector.server->pc.item[it].pile;
					}

					varValue = count;
					bret = varValue.isValid();
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
						bret = varValue.isValid();
					}
				}
			}
		}

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hashitem = injector.server->hashitem;
		if (!hashitem.contains(itemIndex))
			break;

		QHash<QString, QVariant> hash = hashitem.value(itemIndex);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kEquipInfo:
	{
		int itemIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &itemIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hashitem = injector.server->hashequip;
		if (!hashitem.contains(itemIndex))
			break;

		QHash<QString, QVariant> hash = hashitem.value(itemIndex);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPetEquipInfo:
	{
		int petIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &petIndex))
			break;

		int itemIndex = -1;
		if (!checkInt(currentLineTokens_, 4, &itemIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 5, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<int, QHash<QString, QVariant>>>& hashpetequip = injector.server->hashpetequip;
		if (!hashpetequip.contains(petIndex))
			break;

		QHash<int, QHash<QString, QVariant>> hash = hashpetequip.value(petIndex);
		if (!hash.contains(itemIndex))
			break;

		QHash<QString, QVariant> hash2 = hash.value(itemIndex);
		if (!hash2.contains(typeStr))
			break;

		varValue = hash2.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPartyInfo:
	{
		int partyIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &partyIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, QHash<QString, QVariant>> hashparty = injector.server->hashparty;
		if (!hashparty.contains(partyIndex))
			break;

		QHash<QString, QVariant> hash = hashparty.value(partyIndex);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kChatInfo:
	{
		int chatIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &chatIndex))
			break;

		util::SafeHash<int, QVariant>& hashchat = injector.server->hashchat;
		if (!hashchat.contains(chatIndex))
			break;

		varValue = hashchat.value(chatIndex);
		bret = varValue.isValid();
		break;
	}
	case kDialogInfo:
	{
		int dialogIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &dialogIndex))
			break;

		util::SafeHash<int, QVariant>& hashdialog = injector.server->hashdialog;

		if (dialogIndex == -1)
		{
			QStringList texts;
			for (int i = 0; i < MAX_DIALOG_LINE; ++i)
			{
				if (!hashdialog.contains(i))
					break;

				texts << hashdialog.value(i).toString().simplified();
			}

			varValue = texts.join("\n");
		}
		else
		{
			if (!hashdialog.contains(dialogIndex))
				break;

			varValue = hashdialog.value(dialogIndex);
		}


		bret = varValue.isValid();
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
		//int rep = 0;   // 聲望
		//int ene = 0;   // 氣勢
		//int shl = 0;   // 貝殼
		//int vit = 0;   // 活力
		//int pts = 0;   // 積分
		//int vip = 0;   // 會員點
		currencydata_t currency = injector.server->currencyData;
		if (typeStr == "exp")
		{
			varValue = currency.expbufftime;
			bret = varValue.isValid();
		}
		else if (typeStr == "rep")
		{
			varValue = currency.prestige;
			bret = varValue.isValid();
		}
		else if (typeStr == "ene")
		{
			varValue = currency.energy;
			bret = varValue.isValid();
		}
		else if (typeStr == "shl")
		{
			varValue = currency.shell;
			bret = varValue.isValid();
		}
		else if (typeStr == "vit")
		{
			varValue = currency.vitality;
			bret = varValue.isValid();
		}
		else if (typeStr == "pts")
		{
			varValue = currency.points;
			bret = varValue.isValid();
		}
		else if (typeStr == "vip")
		{
			varValue = currency.VIPPoints;
			bret = varValue.isValid();
		}

		if (bret)
			injector.server->press(BUTTON_CANCEL);

		break;
	}
	case kBattleInfo:
	{
		int index = -1;
		if (!checkInt(currentLineTokens_, 3, &index))
		{
			QString typeStr;
			checkString(currentLineTokens_, 3, &typeStr);
			if (typeStr.isEmpty())
				break;

			if (typeStr == "round")
			{
				varValue = injector.server->BattleCliTurnNo;
				bret = varValue.isValid();
			}
			else if (typeStr == "field")
			{
				varValue = injector.server->hashbattlefield.get();
				bret = varValue.isValid();
			}
			break;
		}

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);

		util::SafeHash<int, QHash<QString, QVariant>> hashbattle = injector.server->hashbattle;
		if (!hashbattle.contains(index))
			break;

		QHash<QString, QVariant> hash = hashbattle.value(index);
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	default:
		break;
	}

	return bret;
}

//處理if
bool Parser::processIfCompare()
{
	QString expr = getToken<QString>(1);

	replaceToVariable(expr);

	double dvalue = 0.0;
	bool result = false;
	if (exprTo(expr, &dvalue))
	{
		result = dvalue > 0.0;
	}

	//RESERVE op;
	//if (!checkRelationalOperator(TK, 2, &op))
	//	return Parser::kArgError;

	//if (!toVariant(TK, 3, &b))
	//	return Parser::kArgError;

	//return checkJump(TK, 4, compare(a, b, op), SuccessJump);

	return checkJump(currentLineTokens_, 2, result, FailedJump) == kHasJump;
}

//處理隨機數
void Parser::processRandom()
{
	do
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		int min = 0;
		checkInt(currentLineTokens_, 2, &min);
		int max = 0;
		checkInt(currentLineTokens_, 3, &max);

		if (min == 0 && max == 0)
			break;

		VariantSafeHash& args = getLabelVarsRef();

		if (min > 0 && max == 0)
		{
			if (args.contains(varName))
				args.insert(varName, QRandomGenerator::global()->bounded(0, min));
			else
				variables_->insert(varName, QRandomGenerator::global()->bounded(0, min));
			break;
		}
		else if (min > max)
		{
			if (args.contains(varName))
				args.insert(varName, 0);
			else
				variables_->insert(varName, 0);
			break;
		}

		if (args.contains(varName))
			args.insert(varName, QRandomGenerator::global()->bounded(min, max));
		else
			variables_->insert(varName, QRandomGenerator::global()->bounded(min, max));
	} while (false);
}

//處理"格式化"
void Parser::processFormation()
{

	auto formatTime = [](int seconds)->QString
	{
		int hours = seconds / 3600;
		int minutes = (seconds % 3600) / 60;
		int remainingSeconds = seconds % 60;

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

		VariantSafeHash& args = getLabelVarsRef();

		QString key;
		QString keyWithTime;
		constexpr int MAX_NESTING_DEPTH = 10;//最大嵌套深度
		for (int i = 0; i < MAX_NESTING_DEPTH; ++i)
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
					int seconds = it.value().toInt();
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
					int seconds = it.value().toInt();
					formatStr.replace(keyWithTime, formatTime(seconds));
				}
			}
		}

		QVariant varValue;
		int argsize = tokens_.size();
		for (int i = kFormatPlaceHoldSize; i < argsize; ++i)
		{
			varValue = checkValue(currentLineTokens_, i + 1, QVariant::String);

			key = QString("{:%1}").arg(i - kFormatPlaceHoldSize + 1);

			if (formatStr.contains(key))
			{
				formatStr.replace(key, varValue.toString());
				continue;
			}

			keyWithTime = QString("{T:%1}").arg(i - kFormatPlaceHoldSize + 1);
			if (formatStr.contains(keyWithTime))
				formatStr.replace(key, formatTime(varValue.toInt()));
		}

		static const QRegularExpression rexOut(R"(\[(\d+)\])");
		if ((varName.startsWith("out", Qt::CaseInsensitive) && varName.contains(rexOut)) || varName.toLower() == "out")
		{
			QRegularExpressionMatch match = rexOut.match(varName);
			int color = QRandomGenerator::global()->bounded(0, 10);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				int nColor = str.toInt();
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
			int color = QRandomGenerator::global()->bounded(0, 10);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				int nColor = str.toInt();
				if (nColor >= 0 && nColor <= 10)
					color = nColor;
			}

			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->talk(formatStr, color);
		}
		else
		{
			if (args.contains(varName))
				args.insert(varName, QVariant::fromValue(formatStr));
			else
				variables_->insert(varName, QVariant::fromValue(formatStr));
		}
	} while (false);
}

//檢查"調用"是否傳入參數
void Parser::checkArgs()
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QVariantList list;
	for (int i = kCallPlaceHoldSize; i < tokens_.value(lineNumber_).size(); ++i)
	{
		TokenMap map = tokens_.value(lineNumber_);
		Token token = map.value(i);
		QVariant var = checkValue(map, i, QVariant::String);

		if (token.type != TK_FUZZY)
			list.append(var);
		else
			list.append(0);
	}

	//無論如何都要在調用call之後壓入新的參數堆棧
	callArgsStack_.push(list);
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

		int jumpLine = matchLineFromLabel(functionName);
		if (jumpLine == -1)
			break;

		int currentLine = lineNumber_;
		checkArgs();
		if (!jump(functionName, true))
		{
			break;
		}
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
			QVariant var = checkValue(currentLineTokens_, 1, QVariant::Int);
			QVariant::Type type = var.type();
			if (type == QVariant::Int)
			{
				int jumpLineCount = var.toInt();
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
	//RESERVE type = getTokenType(1);
	do
	{
		QVariant var = checkValue(currentLineTokens_, 1, QVariant::Int);

		int line = var.toInt();
		if (line <= 0)
			break;

		jumpto(line, false);
		return true;

	} while (false);

	return false;
}

//處理"返回"
void Parser::processReturn()
{
	if (!callArgsStack_.isEmpty())
		callArgsStack_.pop();//call行號出棧
	if (!labalVarStack_.isEmpty())
		labalVarStack_.pop();//label局變量出棧
	if (!callStack_.isEmpty())
	{
		int returnIndex = callStack_.pop();
		int jumpLineCount = returnIndex - lineNumber_;
		jump(jumpLineCount, true);
		return;
	}
	lineNumber_ = 0;
}

//處理"返回跳轉"
void Parser::processBack()
{
	if (!jmpStack_.isEmpty())
	{
		int returnIndex = jmpStack_.pop();//jump行號出棧
		int jumpLineCount = returnIndex - lineNumber_;
		jump(jumpLineCount, true);
		return;
	}
	lineNumber_ = 0;
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
		//if (type == QVariant::Double)
		//	*pvar = calc<double>(*pvar, varValue, op);
		//else
		*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_SUB:
		//if (type == QVariant::Double)
		//	*pvar = calc<double>(*pvar, varValue, op);
		//else
		*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_MUL:
		//if (type == QVariant::Double)
		//	*pvar = calc<double>(*pvar, varValue, op);
		//else
		*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_DIV:
		//if (type == QVariant::Double)
		//	*pvar = calc<double>(*pvar, varValue, op);
		//else
		*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_INC:
		//if (type == QVariant::Double)
		//	*pvar = calc<double>(*pvar, varValue, op);
		//else
		*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_DEC:
		//if (type == QVariant::Double)
		//	*pvar = calc<double>(*pvar, varValue, op);
		//else
		*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_MOD:
		*pvar = pvar->toInt() % varValue.toInt();
		break;
	case TK_AND:
		*pvar = pvar->toInt() & varValue.toInt();
		break;
	case TK_OR:
		*pvar = pvar->toInt() | varValue.toInt();
		break;
	case TK_XOR:
		*pvar = pvar->toInt() ^ varValue.toInt();
		break;
	case TK_SHL:
		*pvar = pvar->toInt() << varValue.toInt();
		break;
	case TK_SHR:
		*pvar = pvar->toInt() >> varValue.toInt();
		break;
	default:
		//Q_UNREACHABLE();//基於lexer事前處理過不可能會執行到這裡
		break;
	}
}

//處理錯誤
void Parser::handleError(int err)
{
	if (err == kNoChange)
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (err == kError)
		emit signalDispatcher.addErrorMarker(lineNumber_, err);

	QString msg;
	switch (err)
	{
	case kError:
		msg = QObject::tr("unknown error");
		break;
	case kArgError:
		msg = QObject::tr("argument error");
		break;
	case kLabelError:
		msg = QObject::tr("label incorrect or not exist");
		break;
	case kNoChange:
		return;
	default:
		break;
	}

	emit signalDispatcher.appendScriptLog(QObject::tr("error occured at line %1. detail:%2").arg(lineNumber_ + 1).arg(msg), 6);
}

//解析跳轉棧或調用棧相關訊息並發送信號
void Parser::generateStackInfo(int type)
{
	if (mode_ != kSync)
		return;

	const QStack<int> stack = (type == 0) ? callStack_ : jmpStack_;
	QList<int> list = stack.toList();
	QHash <int, QString> hash;
	if (!list.isEmpty())
	{
		for (const int it : list)
		{
			if (!tokens_.contains(it - 1))
				continue;

			TokenMap tokens = tokens_.value(it - 1);
			if (tokens.isEmpty())
				continue;

			QStringList l;
			QString cmd = tokens.value(0).raw.trimmed() + " ";

			int size = tokens.size();
			for (int i = 1; i < size; ++i)
				l.append(tokens.value(i).raw.trimmed());

			hash.insert(it, cmd + l.join(", "));
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (type == 0)
		emit signalDispatcher.callStackInfoChanged(hash);
	else
		emit signalDispatcher.jumpStackInfoChanged(hash);
}