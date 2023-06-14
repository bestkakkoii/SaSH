#include "stdafx.h"
#include "parser.h"

#include "signaldispatcher.h"
#include "injector.h"

//根據token解釋腳本
void Parser::parse(int line)
{
	lineNumber_ = line; //設置當前行號
	callStack_.clear(); //清空調用棧
	jmpStack_.clear();  //清空跳轉棧

	if (tokens_.isEmpty())
		return;

	processTokens();
}

//"調用" 傳參數最小佔位
constexpr int kCallPlaceHoldSize = 2;

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

//行跳轉
void Parser::jump(int line)
{
	//if (line > 0)
	//{
	//	--line;
	//}
	//else
	//{
	//	++line;
	//}
	lineNumber_ += line;
}

//標記跳轉
bool Parser::jump(const QString& name)
{
	int jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		return false;
	}

	int jumpLineCount = jumpLine - lineNumber_;

	jump(jumpLineCount);
	return true;
}

//處理所有的token
void Parser::processTokens()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);
	int oldlineNumber = lineNumber_;
	for (;;)
	{
		if (isEmpty())
			break;

		if (lineNumber_ != 0)
		{
			QThread::yieldCurrentThread();
			QThread::msleep(1);
		}

		emit signalDispatcher.scriptLabelRowTextChanged2(oldlineNumber, NULL, false);
		oldlineNumber = lineNumber_;

		currentLineTokens_ = tokens_.value(lineNumber_);
		if (callBack_)
		{
			int status = callBack_(lineNumber_, currentLineTokens_);
			if (status == kStop)
				break;
		}

		qDebug() << "line:" << lineNumber_ << "tokens:" << currentLineTokens_.value(0).raw;
		RESERVE type = getType();

		try
		{
			switch (type)
			{
			case TK_END:
			{
				lastError_ = kNoError;
				processEnd();
				return;
			}
			case TK_CMD:
			{
				int ret = processCommand();
				if (ret == kHasJump)
					continue;
				else if (ret == kError)
				{
					handleError(ret);
					return;
				}
				else if (ret == kArgError)
				{
					handleError(ret);
				}
				break;
			}
			case TK_VARDECL:
			case TK_VARFREE:
			case TK_VARCLR:
				processVariable(type);
				break;
			case TK_FORMAT:
				processFormation();
				break;
			case TK_CALL:
			{
				if (processCall())
					continue;
				break;
			}
			case TK_JMP:
			{
				if (processJump())
					continue;
				break;
			}
			case TK_RETURN:
				processReturn();
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
			lastError_ = kException;
			break;
		}

		if (isStop_.load(std::memory_order_acquire))
			break;

		emit signalDispatcher.globalVarInfoImport(variables_);
		if (!labalVarStack_.isEmpty())
			emit signalDispatcher.localVarInfoImport(labalVarStack_.top());
		else
			emit signalDispatcher.localVarInfoImport(QHash<QString, QVariant>());
		next();
	}

	processEnd();
}

//處理"標記"
void Parser::processLabel()
{
	QVariantList args = getArgsRef();
	QHash<QString, QVariant> labelVars;
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

	if (!labelVars.isEmpty())
		labalVarStack_.push(labelVars);
}

//處理"結束"
void Parser::processEnd()
{
	variables_.clear();
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

		//取區域變量表
		QHash<QString, QVariant> args = getLabelVars();

		//檢查第二參數是否為邏輯運算符
		RESERVE op = getTokenType(2);
		if (!operatorTypes.contains(op))
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			//如果不是運算符，檢查是否為字符串
			if (varValue.type() == QVariant::Type::String)
			{
				//檢查是否為引用變量
				QString valueStr = varValue.toString();
				if (op == TK_REF)
				{
					valueStr = valueStr.mid(1);
					if (variables_.contains(valueStr))
						varValue = variables_.value(valueStr);
					else
						return;
				}
				else
				{
					//檢查是否為字符串的區域變量
					if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
					{
						varValue = args.value(valueStr).toInt();
					}
					//檢查是否為整數的區域變量
					else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
					{
						bool ok;
						int value = args.value(valueStr).toInt(&ok);
						if (ok)
							varValue = value;
						else
							return;
					}
					else
					{
						//檢查是否使用?讓使用者輸入
						QString msg = getToken<QString>(3);
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
						}
					}
				}
			}

			//插入全局變量表
			variables_.insert(varName, varValue);
			break;
		}

		if (!variables_.contains(varName))
			break;

		if (op == TK_UNK)
			break;

		//取第三個參數
		varValue = getToken<QVariant>(3);
		//檢查第三個是否合法 ，如果不合法 且第二參數不是++或--，則跳出
		if (!varValue.isValid() && op != TK_DEC && op != TK_INC)
			break;

		//取第三個參數的類型
		RESERVE varValueType = getTokenType(3);

		//檢查是否為引用變量
		if (varValueType == TK_REF)
		{
			QString valueStr = varValue.toString().mid(1);
			if (variables_.contains(valueStr))
				varValue = variables_.value(valueStr);
		}
		else if (varValueType == TK_STRING)
		{
			QString valueStr = varValue.toString();
			//檢查是否為字符串區域變量
			if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
			{
				varValue = args.value(valueStr).toInt();
			}
			//檢查是否為整數區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
			{
				bool ok;
				int value = args.value(valueStr).toInt(&ok);
				if (ok)
					varValue = value;
				else
					return;
			}
		}

		//將第一參數，要被運算的變量原數值取出
		QVariant& var = variables_[varName];

		//開始計算新值
		variableCalculate(varName, op, &var, varValue);
		break;
	}
	case TK_VARFREE:
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		if (variables_.contains(varName))
			variables_.remove(varName);
		break;
	}
	case TK_VARCLR:
	{
		variables_.clear();
		break;
	}
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

		QVariant varValue = getToken<QVariant>(2);
		if (!varValue.isValid())
			break;

		QString formatStr = varValue.toString();
		if (formatStr.isEmpty())
			break;

		//查找字符串中包含 {:變數名} 全部替換成變數數值
		for (auto it = variables_.begin(); it != variables_.cend(); ++it)
		{
			QString key = QString("{:%1}").arg(it.key());
			if (formatStr.contains(key))
				formatStr.replace(key, it.value().toString());
		}

		int argsize = tokens_.size();
		for (int i = 3; i < argsize; ++i)
		{
			varValue = getToken<QVariant>(i);
			if (!varValue.isValid())
				continue;

			RESERVE type = getTokenType(i);
			if (type == TK_INT || type == TK_STRING)
			{
				QString key = QString("{:%1}").arg(i - 3 + 1);
				if (formatStr.contains(key))
					formatStr.replace(key, varValue.toString());
			}
			else if (type == TK_REF)
			{
				QString subVarName = varValue.toString();
				if (subVarName.isEmpty() || !subVarName.startsWith(kVariablePrefix))
					continue;

				subVarName = subVarName.mid(1);

				if (!variables_.contains(subVarName))
					continue;

				QVariant subVarValue = variables_.value(subVarName);

				QString key = QString("{:%1}").arg(i - 3 + 1);
				if (formatStr.contains(key))
					formatStr.replace(key, subVarValue.toString());
			}
		}

		variables_[varName] = QVariant::fromValue(formatStr);
	} while (false);
}

//檢查"調用"是否傳入參數
void Parser::checkArgs()
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QVariantList list;
	for (int i = kCallPlaceHoldSize; i < tokens_.value(lineNumber_).size(); ++i)
	{
		Token token = tokens_.value(lineNumber_).value(i);
		QVariant var = token.data;
		if (token.type == TK_REF)
		{
			QString varName = token.data.toString().mid(1);
			if (!variables_.contains(varName))
			{
				list.clear();
				break;
			}

			var = variables_.value(varName);
			if (var.type() == QVariant::String)
			{
				token.type = TK_STRING;
				token.data = var.toString();
			}
			else if (var.type() == QVariant::Int)
			{
				token.type = TK_INT;
				token.data = var.toInt();
			}
			else if (var.type() == QVariant::Double)
			{
				token.type = TK_DOUBLE;
				token.data = var.toDouble();
			}
			else
			{
				list.clear();
				break;
			}

			var = token.data;
		}

		if (token.type != TK_FUZZY)
			list.append(token.data);
		else
			list.append(0);
	}

	callArgsStack_.push(list);
}

//處理"調用"
bool Parser::processCall()
{
	RESERVE type = getTokenType(1);
	do
	{
		if (type != TK_NAME && type != TK_REF)
			break;

		if (type == TK_REF)
		{
			QString varName = getToken<QString>(1);
			if (varName.isEmpty() || !varName.startsWith(kVariablePrefix) || varName == QString(kVariablePrefix))
				break;

			varName = varName.mid(1);

			if (!variables_.contains(varName))
				break;

			QVariant var = variables_.value(varName);
			if (!var.isValid())
				break;

			bool ok;
			int jumpLineCount = var.toInt(&ok);
			if (ok)
			{
				callStack_.push(lineNumber_ + 1);
				checkArgs();
				jump(jumpLineCount);
				return true;
			}
			else
			{
				QString functionName = var.toString();
				if (functionName.isEmpty())
					break;

				int currentLine = lineNumber_;
				checkArgs();
				if (!jump(functionName))
				{
					break;
				}

				callStack_.push(currentLine + 1);
				return true;
			}
		}
		else
		{
			QString functionName = getToken<QString>(1);
			if (functionName.isEmpty())
				break;

			int jumpLine = matchLineFromLabel(functionName);
			if (jumpLine == -1)
				break;

			int currentLine = lineNumber_;
			checkArgs();
			if (!jump(functionName))
			{
				break;
			}
			callStack_.push(currentLine + 1); // Push the next line index to the call stack

			return true;
		}
	} while (false);
	return false;
}

//處理"跳轉"
bool Parser::processJump()
{
	RESERVE type = getTokenType(1);
	do
	{
		if (type != TK_NAME && type != TK_INT && type != TK_REF)
			break;

		if (type == TK_NAME)
		{
			QString labelName = getToken<QString>(1);
			if (labelName.isEmpty())
				break;

			jump(labelName);
		}
		else if (type == TK_REF)
		{
			QString varName = getToken<QString>(1);
			if (varName.isEmpty() || !varName.startsWith(kVariablePrefix) || varName == QString(kVariablePrefix))
				break;

			varName = varName.mid(1);

			if (!variables_.contains(varName))
				break;

			QVariant var = variables_.value(varName);
			if (!var.isValid())
				break;

			bool ok;
			int jumpLineCount = var.toInt(&ok);
			if (ok)
			{
				jump(jumpLineCount);
				return true;
			}
			else
			{
				QString labelName = var.toString();
				if (labelName.isEmpty())
					break;

				if (!jump(labelName))
					break;

				return true;
			}
		}
		else
		{
			int jumpLineCount = getToken<int>(1);
			if (jumpLineCount == 0)
				break;

			jump(jumpLineCount);
			return true;
		}

	} while (false);
	return false;
}

//處理"返回"
void Parser::processReturn()
{
	if (!callArgsStack_.isEmpty())
		callArgsStack_.pop();
	if (!labalVarStack_.isEmpty())
		labalVarStack_.pop();
	if (!callStack_.isEmpty())
	{
		int returnIndex = callStack_.pop();
		int jumpLineCount = returnIndex - lineNumber_;
		jump(jumpLineCount);
		return;
	}
	lineNumber_ = 0;
}

//處理"變量"運算
void Parser::variableCalculate(const QString& varName, RESERVE op, QVariant* pvar, const QVariant& varValue)
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
		Q_UNREACHABLE();
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
	case kNoChange:
		return;
	default:
		break;
	}

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->append(QObject::tr("error occured at line %1. detail:%2").arg(lineNumber_ + 1).arg(msg), 6);
}