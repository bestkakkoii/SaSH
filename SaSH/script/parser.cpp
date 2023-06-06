#include "stdafx.h"
#include "parser.h"

enum VarSubCommand
{
	kVarSet,
	kVarCaclulate,
	kVarCancel,
	kVarClear,
};

const QHash<QString, VarSubCommand> VarSubCommandHash = {
	//big5
	{ "設置", kVarSet },
	{ "運算", kVarCaclulate },
	{ "取消", kVarCancel },
	{ "清空", kVarClear },

	//gb2312
	{ "设置", kVarSet },
	{ "运算", kVarCaclulate },
	{ "取消", kVarCancel },
	{ "清空", kVarClear },
};

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
		return a.value<T>() + 1;
	}
	else if (operatorType == TK_DEC)
	{
		return a.value<T>() - 1;
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

void Parser::jump(const QString& name)
{
	int jumpLineCount = 0;
	int jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		return;
	}

	jumpLineCount = jumpLine - lineNumber_;

	jump(jumpLineCount);
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
			case TK_VAR:
				processVariable();
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

void Parser::processVariable()
{
	RESERVE type = getTokenType(1);
	if (type != TK_SUBCMD)
	{
		qDebug() << "Unexpected subcmd type name:";
		return;
	}

	int n = 1;
	VarSubCommand subcmd = VarSubCommandHash.value(getToken<QString>(n));
	switch (subcmd)
	{
	case kVarSet:
	{
		QString varName = getToken<QString>(++n);
		if (varName.isEmpty())
			break;

		QVariant varValue = getToken<QVariant>(++n);
		if (!varValue.isValid())
			break;

		variables_.insert(varName, varValue);
		break;
	}
	case kVarCaclulate:
	{
		QString varName = getToken<QString>(++n);
		if (varName.isEmpty())
			break;

		if (!variables_.contains(varName))
			break;

		RESERVE op = getTokenType(++n);
		if (op == TK_UNK)
			break;

		QVariant varValue = getToken<QVariant>(++n);
		if (!varValue.isValid())
			break;

		QVariant& var = variables_[varName];
		variableCalculate(varName, op, &var, varValue);
		break;
	}
	case kVarCancel:
	{
		QString varName = getToken<QString>(++n);
		if (varName.isEmpty())
			break;

		if (variables_.contains(varName))
			variables_.remove(varName);
		break;
	}
	case kVarClear:
	{
		variables_.clear();
		break;
	}
	}
}

bool Parser::processCall()
{
	if (getTokenType(1) != TK_NAME)
		return false;
	QString functionName = getToken<QString>(1);
	if (functionName.isEmpty())
		return false;

	int jumpLine = matchLineFromLabel(functionName);
	if (jumpLine == -1)
	{
		qDebug() << "Function not found:" << functionName;
		return false;
	}

	callStack_.push(lineNumber_ + 1); // Push the next line index to the call stack
	jump(jumpLine);
	return true;
}

bool Parser::processJump()
{
	RESERVE type = getTokenType(1);
	if (type != TK_NAME && type != TK_INT)
	{
		return false;
	}

	if (type == TK_NAME)
	{
		QString labelName = getToken<QString>(1);
		if (labelName.isEmpty())
			return false;


		jump(labelName);
	}
	else
	{
		int jumpLineCount = getToken<int>(1);
		if (jumpLineCount == -1)
			return false;

		jump(jumpLineCount);
	}
	return true;
}

void Parser::processReturn()
{
	if (!callStack_.isEmpty())
	{
		int returnIndex = callStack_.pop();
		jump(returnIndex);
	}
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
	}
}
