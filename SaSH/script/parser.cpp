#include "stdafx.h"
#include "parser.h"

#include "signaldispatcher.h"

void Parser::parse(int line)
{
	lineNumber_ = line;
	callStack_.clear();
	jmpStack_.clear();

	if (!tokens_.isEmpty())
		processTokens();
}

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

bool Parser::jump(const QString& name)
{
	int jumpLineCount = 0;
	int jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		return false;
	}

	jumpLineCount = jumpLine - lineNumber_;

	jump(jumpLineCount);
	return true;
}

void Parser::processTokens()
{
	while (!isEmpty())
	{
		if (lineNumber_ != 0)
		{
			QThread::yieldCurrentThread();
			QThread::msleep(1);
		}

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
				return;
			}
			case TK_CMD:
			{
				int ret = processCommand();
				if (ret == kHasJump)
					continue;
				else if (ret == kError)
				{
					qDebug() << "Command error:" << currentLineTokens_.value(0).raw;
					return;
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
				break;
			case TK_LABEL:
				break;
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

		next();
	}
}

int Parser::processCommand()
{
	QMap<int, Token> tokens = getCurrentTokens();
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

		status = function(tokens);
	}
	else
	{
		qDebug() << "Command not found:" << commandName;
	}
	return status;
}

void Parser::processVariable(RESERVE type)
{

	switch (type)
	{
	case TK_VARDECL:
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		QVariant varValue = getToken<QVariant>(2);
		if (!varValue.isValid())
			break;

		RESERVE op = getTokenType(2);
		if (!operatorTypes.contains(op))
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			if (varValue.type() == QVariant::Type::String)
			{
				QString valueStr = varValue.toString();
				QString msg = QObject::tr("set var [%1] value").arg(varName);
				if (valueStr.startsWith(kFuzzyPrefix + QString("2")))
				{
					varValue.clear();
					emit signalDispatcher.inputBoxShow(msg, QInputDialog::IntInput, &varValue);
					if (!varValue.isValid())
						break;
				}
				else if (valueStr.startsWith(kFuzzyPrefix + QString("3")))
				{
					varValue.clear();
					emit signalDispatcher.inputBoxShow(msg, QInputDialog::DoubleInput, &varValue);
					if (!varValue.isValid())
						break;
				}
				else if (valueStr.startsWith(kFuzzyPrefix) && valueStr.endsWith(kFuzzyPrefix) || valueStr.startsWith(kFuzzyPrefix + QString("1")))
				{
					varValue.clear();
					emit signalDispatcher.inputBoxShow(msg, QInputDialog::TextInput, &varValue);
					if (!varValue.isValid())
						break;
				}
			}

			variables_.insert(varName, varValue);
			break;
		}

		if (!variables_.contains(varName))
			break;

		if (op == TK_UNK)
			break;

		varValue = getToken<QVariant>(3);
		if (!varValue.isValid() && op != TK_DEC && op != TK_INC)
			break;

		QVariant& var = variables_[varName];
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
				QString subVarName = varValue.toString().mid(1);
				if (subVarName.isEmpty() || !subVarName.startsWith(kVariablePrefix))
					continue;

				subVarName = subVarName.mid(1);

				if (!variables_.contains(subVarName))
					continue;

				QVariant subVarValue = variables_.value(subVarName);

				QString key = QString("{:%1}").arg(subVarName);
				if (formatStr.contains(key))
					formatStr.replace(key, subVarValue.toString());
			}
		}

		variables_[varName] = QVariant::fromValue(formatStr);
	} while (false);
}

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
				jump(jumpLineCount);
				return true;
			}
			else
			{
				QString functionName = var.toString();
				if (functionName.isEmpty())
					break;

				if (!jump(functionName))
					break;

				callStack_.push(lineNumber_ + 1);
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

			callStack_.push(lineNumber_ + 1); // Push the next line index to the call stack
			jump(jumpLine);

			return true;
		}
	} while (false);
	return false;
}

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
			if (jumpLineCount == -1)
				break;

			jump(jumpLineCount);
			return true;
		}

	} while (false);
	return false;
}

void Parser::processReturn()
{
	if (!callStack_.isEmpty())
	{
		int returnIndex = callStack_.pop();
		jump(returnIndex);
	}
	else
		lineNumber_ = 0;
}

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
