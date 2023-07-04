#pragma once
#include "lexer.h"
#include <QStack>
#include <functional>

#include "threadplugin.h"
#include "util.h"

using CommandRegistry = std::function<int(int currentLine, const TokenMap& token)>;

//callbak
using ParserCallBack = std::function<int(int currentLine, const TokenMap& token)>;

using VariantSafeHash = util::SafeHash<QString, QVariant>;

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

enum JumpBehavior
{
	SuccessJump,
	FailedJump,
};

class Parser : public ThreadPlugin
{
	Q_OBJECT
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
		kLabelError,
		kArgError = 10,

	};

	enum Mode
	{
		kSync,
		kAsync,
	};

public:
	inline ParserError lastCriticalError() const { return lastCriticalError_; }

	void setLastErrorMessage(const QString& msg) { lastErrorMesssage_ = msg; }
	QString getLastErrorMessage() const { return lastErrorMesssage_; }

	//設置所有標籤所在行號數據
	inline void setLabels(const util::SafeHash<QString, int>& labels) { labels_ = labels; }

	//設置腳本所有Token數據
	inline void setTokens(const util::SafeHash<int, TokenMap>& tokens) { tokens_ = tokens; }

	Q_REQUIRED_RESULT inline bool hasToken() const { return !tokens_.isEmpty(); }

	Q_REQUIRED_RESULT inline const util::SafeHash<int, TokenMap> getToken() const { return tokens_; }

	inline void setCallBack(const ParserCallBack& callBack) { callBack_ = callBack; }

	inline void setCurrentLine(int line) { lineNumber_ = line; }

	inline void setMode(Mode mode) { mode_ = mode; }

	inline void insertUserCallBack(const QString& name, const QString& type)
	{
		//確保一種類型只能被註冊一次
		for (auto it = userRegCallBack_.begin(); it != userRegCallBack_.end(); ++it)
		{
			if (it.value() == type)
				return;
			if (it.key() == name)
				return;
		}

		userRegCallBack_.insert(name, type);
	}

	inline bool getErrorCallBackLabelName(QString* pname) const
	{
		if (pname == nullptr)
			return false;

		for (auto it = userRegCallBack_.begin(); it != userRegCallBack_.end(); ++it)
		{
			if (it.value() == "err")
			{
				*pname = it.key();
				return true;
			}
		}
		return false;
	}

	inline bool getDtorCallBackLabelName(QString* pname)
	{
		if (pname == nullptr)
			return false;

		if (dtorCallBackFlag_)
			return false;

		util::SafeHash<QString, int>& hash = getLabels();
		constexpr const char* DTOR = "dtor";
		if (hash.contains(DTOR))
		{
			dtorCallBackFlag_ = true;
			*pname = DTOR;
			return true;
		}

		return false;
	}

	inline void registerFunction(const QString& commandName, const CommandRegistry& function)
	{
		commandRegistry_.insert(commandName, static_cast<CommandRegistry>(function));
	}

	template <typename T>
	Q_REQUIRED_RESULT inline T getVar(const QString& name)
	{
		QString newName = name;
		if (variables_->contains(newName))
		{
			return variables_->value(newName).value<T>();
		}
		else
		{
			return QVariant::fromValue(name).value<T>();
		}
	}

	void jump(int line, bool noStack);
	void jumpto(int line, bool noStack);
	bool jump(const QString& name, bool noStack);

	bool checkString(const TokenMap& TK, int idx, QString* ret);
	bool checkInt(const TokenMap& TK, int idx, int* ret);
#if 0
	bool checkDouble(const TokenMap& TK, int idx, double* ret);
#endif
	bool toVariant(const TokenMap& TK, int idx, QVariant* ret);
	bool compare(const QVariant& a, const QVariant& b, RESERVE type) const;

	QVariant checkValue(const TokenMap TK, int idx, QVariant::Type);
	int checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior);
public:
	Parser();
	virtual ~Parser() = default;

	//解析腳本
	void parse(int line = 0);
	util::SafeHash<QString, int>& getLabels() { return labels_; }
	QSharedPointer<VariantSafeHash> getVariablesPointer() const { return variables_; }
	void setPVariables(const QSharedPointer<VariantSafeHash>& variables) { variables_ = variables; }
	//Q_REQUIRED_RESULT inline QVariant& getVarRef(const QString& name) { return (*variables_)[name]; }
	Q_REQUIRED_RESULT inline VariantSafeHash& getVarsRef() { return *variables_; }
	Q_REQUIRED_RESULT inline VariantSafeHash& getLabelVarsRef()
	{
		if (!labalVarStack_.isEmpty())
			return labalVarStack_.top();
		else
		{
			labalVarStack_.push(VariantSafeHash{});
			return labalVarStack_.top();
		}
	}
	Q_REQUIRED_RESULT inline VariantSafeHash getLabelVars()
	{
		if (!labalVarStack_.isEmpty())
			return labalVarStack_.top();
		else
		{
			labalVarStack_.push(VariantSafeHash{});
			return labalVarStack_.top();
		}
	}
private:
	void processTokens();
	int processCommand();
	void processVariable(RESERVE type);
	void processLocalVariable();
	void processVariableIncDec();
	void processVariableCAOs();
	void processVariableExpr();
	void processMultiVariable();
	void processFormation();
	void processRandom();
	bool processCall();
	bool processGoto();
	bool processJump();
	void processReturn();
	void processBack();
	void processLabel();
	void processClean();
	bool processGetSystemVarValue(const QString& varName, QString& valueStr, QVariant& varValue);
	bool processIfCompare();

	void replaceToVariable(QString& str);
	bool checkCallStack();
	bool checkFuzzyValue(const QString& varName, QVariant varValue, QVariant* pvalue);

	template <typename T>
	bool exprTo(QString expr, T* ret);

	template <typename T>
	bool exprTo(T value, QString expr, T* ret);


	void handleError(int err);
	void checkArgs();


	inline void next() { ++lineNumber_; }

	Q_REQUIRED_RESULT inline bool isEmpty() const { return !tokens_.contains(lineNumber_); }
	Q_REQUIRED_RESULT inline RESERVE getCurrentFirstTokenType() const { return currentLineTokens_.value(0).type; }
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
			return 0;
		return T();
	}
	Q_REQUIRED_RESULT inline RESERVE getTokenType(int index) const { return currentLineTokens_.value(index).type; }
	Q_REQUIRED_RESULT TokenMap getCurrentTokens() const { return currentLineTokens_; }
	void variableCalculate(RESERVE op, QVariant* var, const QVariant& varValue);
	int matchLineFromLabel(const QString& label) const { return labels_.value(label, -1); }

	Q_REQUIRED_RESULT inline QVariantList& getArgsRef()
	{
		if (!callArgsStack_.isEmpty())
			return callArgsStack_.top();
		else
			return emptyArgs_;
	}

	void generateStackInfo(int type);


private:
	typedef struct tagFunctionChunk
	{
		QString name;
		int begin = -1;
		int end = -1;
	} FunctionChunk;

	bool usestate = false;

	util::SafeHash<int, TokenMap> tokens_;							//當前運行腳本的每一行token
	QSharedPointer<VariantSafeHash> variables_;						//所有用腳本變量
	util::SafeHash<QString, int> labels_;							//所有標記所在行記錄
	util::SafeHash<QString, FunctionChunk> functionChunks_;         //函數代碼塊
	util::SafeHash<QString, QString> userRegCallBack_;				//用戶註冊的回調函數
	util::SafeHash<QString, CommandRegistry> commandRegistry_;		//所有命令的函數指針
	QStack<int> callStack_;											//"調用"命令所在行棧
	QStack<int> jmpStack_;											//"跳轉"命令所在行棧
	QStack<QVariantList> callArgsStack_;							//"調用"命令參數棧
	QVariantList emptyArgs_;										//空參數
	QStack<VariantSafeHash> labalVarStack_;							//標籤變量名
	VariantSafeHash emptyLabelVars_;								//空標籤變量名

	TokenMap currentLineTokens_;									//當前行token
	RESERVE currentType_ = TK_UNK;									//當前行第一個token類型
	int lineNumber_ = 0;											//當前行號

	ParserError lastCriticalError_ = kNoError;						//最後一次錯誤

	util::SafeData<QString> lastErrorMesssage_;						//最後一次錯誤信息

	ParserCallBack callBack_ = nullptr;								//解析腳本回調函數

	Mode mode_ = kSync;												//解析模式

	bool dtorCallBackFlag_ = false;									//析構回調函數標記
	bool ctorCallBackFlag_ = false;									//建構回調函數標記
};