#pragma once
#include "lexer.h"
#include <QStack>
#include <functional>

using CommandRegistry = std::function<int(const QMap<int, Token>&)>;

//callbak
using ParserCallBack = std::function<int(int currentLine, const QMap<int, Token>&)>;

static const QSet<RESERVE> operatorTypes = {
	TK_ADD, //"+"
	TK_SUB, // '-'
	TK_MUL, // '*'
	TK_DIV, // '/'
	TK_MOD, // '%'
	TK_SHL, // "<<"
	TK_SHR, // ">>"
	TK_AND, // "&"
	TK_OR, // "|"
	TK_NOT, // "~"
	TK_XOR, // "^"
	TK_NEG, //"!" (與C++的 ! 同義)
	TK_INC, // "++" (與C++的 ++ 同義)
	TK_DEC, // "--" (與C++的 -- 同義)
};

static const QSet<RESERVE> relationalOperatorTypes = {
	TK_EQ, // "=="
	TK_NEQ, // "!="
	TK_GT, // ">"
	TK_LT, // "<"
	TK_GEQ, // ">="
	TK_LEQ, // "<="
};

class Parser
{
public:
	enum
	{
		kStop = 0,
		kContinue = 1,
		kPause,
		kResume,
	};

	enum ParserError
	{
		kNoError = 0,
		kException,
		kUndefinedVariable,
	};

	enum ParserCommandStatus
	{
		kHasJump = 0,
		kNoChange,
		kError,
		kArgError,
	};

public:
	inline ParserError lastError() const { return lastError_; }

	//設置所有標籤所在行號數據
	inline void setLabels(const QHash<QString, int>& labels) { labels_ = labels; }

	//設置腳本所有Token數據
	inline void setTokens(const QHash<int, QMap<int, Token>>& tokens) { tokens_ = tokens; }

	inline bool hasToken() const { return !tokens_.isEmpty(); }

	inline void setCallBack(const ParserCallBack& callBack) { callBack_ = callBack; }

	inline void setCurrentLine(int line) { lineNumber_ = line; }

	//設置腳本所有全局變量數據
	void setGlobalVariables(const QHash<QString, QVariant>& variables) { globalVariables_ = variables; }

	inline void registerFunction(const QString& commandName, const CommandRegistry& function)
	{
		commandRegistry_.insert(commandName, static_cast<CommandRegistry>(function));
	}

	template <typename T>
	T getVar(const QString& name)
	{
		QString newName = name;
		if (newName.startsWith(kVariablePrefix))
			newName.remove(0, 1);

		if (variables_.contains(newName))
		{
			return variables_.value(newName).value<T>();
		}
		else
		{
			lastError_ = kUndefinedVariable;
			isStop_.store(true, std::memory_order_release);
			return T();
		}
	}

	void jump(int line);
	bool jump(const QString& name);
public:

	//解析腳本
	void parse(int line = 0);

private:
	void processTokens();
	int processCommand();
	void processVariable(RESERVE type);
	void processFormation();
	bool processCall();
	bool processJump();
	void processReturn();
	//void processLabel();

	inline void next() { ++lineNumber_; }

	Q_REQUIRED_RESULT inline bool isEmpty() const { return !tokens_.contains(lineNumber_); }
	Q_REQUIRED_RESULT inline RESERVE getType() const { return currentLineTokens_.value(0).type; }
	template <typename T>
	Q_REQUIRED_RESULT inline T getToken(int index) const
	{
		if (currentLineTokens_.contains(index))
		{
			QVariant data = currentLineTokens_.value(index).data;
			if (data.isValid())
				return currentLineTokens_.value(index).data.value<T>();
		}
		//如果是整數返回 -1
		if (std::is_same<T, int>::value)
			return -1;
		return T();
	}
	Q_REQUIRED_RESULT inline RESERVE getTokenType(int index) const { return currentLineTokens_.value(index).type; }
	Q_REQUIRED_RESULT QMap<int, Token> getCurrentTokens() const { return currentLineTokens_; }
	void variableCalculate(const QString& varName, RESERVE op, QVariant* var, const QVariant& varValue);
	int matchLineFromLabel(const QString& label) const { return labels_.value(label, -1); }

private:
	std::atomic_bool isStop_ = false;
	QHash<int, QMap<int, Token>> tokens_;
	QHash<QString, QVariant> variables_;
	QHash<QString, QVariant> globalVariables_;
	QHash<QString, int> labels_;
	QHash<QString, CommandRegistry> commandRegistry_;

	QStack<int> callStack_;
	QStack<int> jmpStack_;
	QMap<int, Token> currentLineTokens_;
	RESERVE currentType_ = TK_UNK;
	int lineNumber_ = 0;

	ParserError lastError_ = kNoError;

	//CallBack
	ParserCallBack callBack_ = nullptr;

};