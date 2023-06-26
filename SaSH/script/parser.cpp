#include "stdafx.h"
#include "parser.h"

#include "signaldispatcher.h"
#include "injector.h"
#include "interpreter.h"

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
	if (type == TK_STRING || type == TK_CMD || type == TK_NAME || type == TK_LABELVAR || type == TK_CAOS)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::Int || vtype == QVariant::String)
				*ret = args.value(name).toString();
			else if (vtype == QVariant::Double)
				*ret = QString::number(args.value(name).toDouble(), 'f', 16);
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
		return false;

	return true;
}

//嘗試取指定位置的token轉為雙精度浮點數
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
			return false;
	}
	else
		return false;

	return true;
}

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
			else if (vtype == QVariant::Double)
				*ret = QString::number(args.value(name).toDouble(), 'f', 8);
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
				double value = variables_->value(name).toDouble(&ok);
				if (ok)
					*ret = value;
				else
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
	double dvalue;
	QString text;
	if (checkDouble(currentLineTokens_, idx, &dvalue))
	{
		varValue = dvalue;
	}
	else if (checkInt(currentLineTokens_, idx, &ivalue))
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
		else if (type == QVariant::Double)
			varValue = 0.0;
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

//處理所有的token
void Parser::processTokens()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	Injector& injector = Injector::getInstance();

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
	};

	int oldlineNumber = lineNumber_;
	for (;;)
	{
		QThread::msleep(1);
		delay();

		if (isEmpty())
		{
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
			util::SafeHash<QString, int>& hash = getLabels();
			constexpr const char* CTOR = "ctor";
			if (hash.contains(CTOR))
			{
				ctorCallBackFlag_ = true;
				callStack_.push(0);
				jump(CTOR, true);
			}
		}

		currentLineTokens_ = tokens_.value(lineNumber_);

		RESERVE currentType = currentLineTokens_.value(0).type;
		bool skip = currentType == RESERVE::TK_WHITESPACE || currentType == RESERVE::TK_SPACE;

		if (!skip)
		{
			if (mode_ == Mode::kSync)
				emit signalDispatcher.scriptLabelRowTextChanged2(oldlineNumber, NULL, false);
		}
		oldlineNumber = lineNumber_;

		if (callBack_)
		{
			if (!skip)
			{
				int status = callBack_(lineNumber_, currentLineTokens_);
				if (status == kStop)
					break;
			}
		}

		//qDebug() << "line:" << lineNumber_ << "tokens:" << currentLineTokens_.value(0).raw;
		RESERVE type = getType();

		try
		{
			switch (type)
			{
			case TK_END:
			{
				lastCriticalError_ = kNoError;
				processClean();
				QString name;
				if (!getDtorCallBackLabelName(&name))
					return;
				else
				{
					callStack_.push(tokens_.size() - 1);
					jump(name, true);
					continue;
				}
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
					QString name;
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
					QString name;
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
				processVariable(type);
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
				processFormation();
				break;
			case TK_RND:
				processRandom();
				break;
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
				processReturn();
				generateStackInfo(0);
				continue;
			case TK_BAK:
				processBack();
				generateStackInfo(1);
				continue;
			case TK_LABEL:
			{
				processLabel();
				break;
			}

			case TK_WHITESPACE:
				break;
			default:
				qDebug() << "Unexpected token type:" << type;
				break;
			}
		}
		catch (...)
		{
			lastCriticalError_ = kException;
			break;
		}

		if (isInterruptionRequested())
		{
			QString name;
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
				emit signalDispatcher.globalVarInfoImport(variables_->toHash());
				if (!labalVarStack_.isEmpty())
					emit signalDispatcher.localVarInfoImport(labalVarStack_.top().toHash());//導出局變量訊息
				else
					emit signalDispatcher.localVarInfoImport(QHash<QString, QVariant>());
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
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			//如果不是運算符，檢查是否為字符串
			bool pass = false;

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

			if (text.startsWith(kFuzzyPrefix))
			{

				QString valueStr = text;


				//檢查是否使用?讓使用者輸入
				QString msg;
				checkString(currentLineTokens_, 3, &msg);

				if (valueStr.startsWith(kFuzzyPrefix + QString("2")))//整數輸入框
				{
					varValue.clear();
					emit signalDispatcher.inputBoxShow(msg, QInputDialog::IntInput, &varValue);
					if (!varValue.isValid())
						break;
					varValue = varValue.toInt();
				}
				else if (valueStr.startsWith(kFuzzyPrefix + QString("3"))) //雙精度浮點數輸入框
				{
					varValue.clear();
					emit signalDispatcher.inputBoxShow(msg, QInputDialog::DoubleInput, &varValue);
					if (!varValue.isValid())
						break;
					varValue = varValue.toDouble();
				}
				else if (valueStr.startsWith(kFuzzyPrefix) && valueStr.endsWith(kFuzzyPrefix) || valueStr.startsWith(kFuzzyPrefix + QString("1")))// 字串輸入框
				{
					varValue.clear();
					emit signalDispatcher.inputBoxShow(msg, QInputDialog::TextInput, &varValue);
					if (!varValue.isValid())
						break;
					varValue = varValue.toString();
				}
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
		RESERVE varValueType = getTokenType(3);




		//將第一參數，要被運算的變量原數值取出
		QVariant& var = (*variables_)[varName];

		//開始計算新值
		variableCalculate(op, &var, varValue);
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
		value = args[varName].toInt();
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

	QVariant varValue = checkValue(currentLineTokens_, 2, QVariant::Int);

	VariantSafeHash& args = getLabelVarsRef();
	QString varValueStr = varValue.toString();
	for (auto it = args.cbegin(); it != args.cend(); ++it)
	{
		QString key = it.key();
		QString value = it.value().toString();
		varValueStr.replace(key, value);
	}

	VariantSafeHash variables = *variables_;
	for (auto it = variables.cbegin(); it != variables.cend(); ++it)
	{
		QString key = it.key();
		QString value = it.value().toString();
		varValueStr.replace(key, value);
	}

	varValueStr.replace("^", "xor");
	varValueStr.replace("~", "nor ");

	if (var.type() == QVariant::String)
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
		using TKSymbolTable = exprtk::symbol_table<double>;
		using TKExpression = exprtk::expression<double>;
		using TKParser = exprtk::parser<double>;

		TKSymbolTable symbolTable;
		symbolTable.add_constants();

		TKParser parser;
		TKExpression expression;
		expression.register_symbol_table(symbolTable);

		const std::string exprStr = varValueStr.toStdString();
		if (!parser.compile(exprStr, expression))
			return;

		if (var.type() == QVariant::Int)
			var = var.toInt() + static_cast<int>(expression.value());
		else if (var.type() == QVariant::Double)
			var = var.toDouble() + expression.value();
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

	VariantSafeHash& args = getLabelVarsRef();
	for (auto it = args.cbegin(); it != args.cend(); ++it)
	{
		QString name = it.key();
		QVariant value = it.value();
		if (value.type() == QVariant::String)
			expr.replace(name, value.toString().simplified());
		else if (value.type() == QVariant::Double)
			expr.replace(name, QString::number(value.toDouble(), 'f', 16));
		else
			expr.replace(name, QString::number(value.toInt()));
	}

	VariantSafeHash variables = *variables_;
	for (auto it = variables.cbegin(); it != variables.cend(); ++it)
	{
		QString name = it.key();
		QVariant value = it.value();
		if (value.type() == QVariant::String)
			expr.replace(name, value.toString().simplified());
		else if (value.type() == QVariant::Double)
			expr.replace(name, QString::number(value.toDouble(), 'f', 16));
		else
			expr.replace(name, QString::number(value.toInt()));
	}

	QVariant::Type type = varValue.type();
	using TKSymbolTable = exprtk::symbol_table<double>;
	using TKExpression = exprtk::expression<double>;
	using TKParser = exprtk::parser<double>;

	TKSymbolTable symbolTable;
	symbolTable.add_constants();

	TKParser parser;
	TKExpression expression;
	expression.register_symbol_table(symbolTable);

	const std::string exprStr = expr.toStdString();
	if (!parser.compile(exprStr, expression))
		return;

	QVariant result;
	if (type == QVariant::Int)
		result = static_cast<int>(expression.value());
	else if (type == QVariant::Double)
		result = expression.value();
	else
		return;

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

		util::SafeHash<int, VariantSafeHash>& hashmagic = injector.server->hashmagic;
		if (!hashmagic.contains(magicIndex))
			break;

		VariantSafeHash& hash = hashmagic[magicIndex];
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

		util::SafeHash<int, VariantSafeHash>& hashskill = injector.server->hashskill;
		if (!hashskill.contains(skillIndex))
			break;

		VariantSafeHash& hash = hashskill[skillIndex];
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

		util::SafeHash<int, VariantSafeHash>& hashpet = injector.server->hashpet;
		if (!hashpet.contains(petIndex))
			break;

		VariantSafeHash& hash = hashpet[petIndex];
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

		util::SafeHash<int, util::SafeHash<int, VariantSafeHash>>& hashpetskill = injector.server->hashpetskill;
		if (!hashpetskill.contains(petIndex))
			break;

		util::SafeHash<int, VariantSafeHash>& hash = hashpetskill[petIndex];
		if (!hash.contains(skillIndex))
			break;

		VariantSafeHash& hash2 = hash[skillIndex];
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

		util::SafeHash<int, VariantSafeHash>& hashitem = injector.server->hashitem;
		if (!hashitem.contains(itemIndex))
			break;

		VariantSafeHash& hash = hashitem[itemIndex];
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

		util::SafeHash<int, VariantSafeHash>& hashitem = injector.server->hashequip;
		if (!hashitem.contains(itemIndex))
			break;

		VariantSafeHash& hash = hashitem[itemIndex];
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

		util::SafeHash<int, util::SafeHash<int, VariantSafeHash>>& hashpetequip = injector.server->hashpetequip;
		if (!hashpetequip.contains(petIndex))
			break;

		util::SafeHash<int, VariantSafeHash>& hash = hashpetequip[petIndex];
		if (!hash.contains(itemIndex))
			break;

		VariantSafeHash& hash2 = hash[itemIndex];
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

		util::SafeHash<int, VariantSafeHash>& hashparty = injector.server->hashparty;
		if (!hashparty.contains(partyIndex))
			break;

		VariantSafeHash& hash = hashparty[partyIndex];
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
			for (int i = 0; i < 10; ++i)
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

		util::SafeHash<int, VariantSafeHash>& hashbattle = injector.server->hashbattle;
		if (!hashbattle.contains(index))
			break;

		VariantSafeHash& hash = hashbattle[index];
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
	QString varValue = getToken<QString>(1);

	VariantSafeHash& args = getLabelVarsRef();

	for (auto it = args.cbegin(); it != args.cend(); ++it)
	{
		QString key = it.key();
		QString value = it.value().toString();
		varValue.replace(key, value);
	}

	VariantSafeHash variables = *getPVariables();
	for (auto it = variables.cbegin(); it != variables.cend(); ++it)
	{
		QString key = it.key();
		QString value = it.value().toString();
		varValue.replace(key, value);
	}

	QString expr = varValue.simplified();

	using TKExpression = exprtk::expression<double>;
	using TKParser = exprtk::parser<double>;

	TKExpression expression;
	TKParser parser;

	std::string expressionStr = expr.toStdString();

	bool result = false;
	if (parser.compile(expressionStr, expression))
	{
		result = expression.value() > 0.0;
	}

	//RESERVE op;
	//if (!checkRelationalOperator(TK, 2, &op))
	//	return Parser::kArgError;

	//if (!toVariant(TK, 3, &b))
	//	return Parser::kArgError;

	//return checkJump(TK, 4, compare(a, b, op), SuccessJump);

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

		//查找字符串中包含 {:變數名} 全部替換成變數數值
		for (auto it = variables_->cbegin(); it != variables_->cend(); ++it)
		{
			QString key = QString("{:%1}").arg(it.key());
			QString keyWithTime = QString("{T:%1}").arg(it.key());
			if (formatStr.contains(key))
				formatStr.replace(key, it.value().toString());
			else if (formatStr.contains(keyWithTime))
			{
				int seconds = it.value().toInt();
				formatStr.replace(keyWithTime, formatTime(seconds));
			}
		}

		for (auto it = args.cbegin(); it != args.cend(); ++it)
		{
			QString key = QString("{L:%1}").arg(it.key());
			QString keyWithTime = QString("{LT:%1}").arg(it.key());
			if (formatStr.contains(key))
				formatStr.replace(key, it.value().toString());
			else if (formatStr.contains(keyWithTime))
			{
				int seconds = it.value().toInt();
				formatStr.replace(keyWithTime, formatTime(seconds));
			}
		}

		int argsize = tokens_.size();
		for (int i = kFormatPlaceHoldSize; i < argsize; ++i)
		{
			QVariant varValue = checkValue(currentLineTokens_, i + 1, QVariant::String);

			QString key = QString("{:%1}").arg(i - kFormatPlaceHoldSize + 1);
			QString keyWithTime = QString("{T:%1}").arg(i - kFormatPlaceHoldSize + 1);
			if (formatStr.contains(key))
				formatStr.replace(key, varValue.toString());
			else if (formatStr.contains(keyWithTime))
				formatStr.replace(key, formatTime(varValue.toInt()));
		}

		if (varName.toLower() == "out")
		{
			const QDateTime time(QDateTime::currentDateTime());
			const QString timeStr(time.toString("hh:mm:ss:zzz"));
			QString src = "\0";

			QString msg = (QString("[%1 | @%2]: %3\0") \
				.arg(timeStr)
				.arg(lineNumber_ + 1, 3, 10, QLatin1Char(' ')).arg(formatStr));

			int color = QRandomGenerator::global()->bounded(0, 10);

			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->announce(formatStr, color);
			if (!injector.scriptLogModel.isNull())
				injector.scriptLogModel->append(msg, color);
		}
		else if (varName.toLower() == "say")
		{
			int color = QRandomGenerator::global()->bounded(0, 10);
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
	RESERVE type = getTokenType(1);
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
		int returnIndex = jmpStack_.pop();
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
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_SUB:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_MUL:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_DIV:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_INC:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_DEC:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
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

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->append(QObject::tr("error occured at line %1. detail:%2").arg(lineNumber_ + 1).arg(msg), 6);
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