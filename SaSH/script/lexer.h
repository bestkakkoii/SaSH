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

#pragma once
#include <QObject>
#include <QHash>
#include <QMap>
#include <QVariant>
#include "util.h"
#include <indexer.h>

constexpr const char* kFuzzyPrefix = "?";
constexpr qint64 kMaxLuaTableDepth = 10ll;

//必須使用此枚舉名稱 RESERVE 請不要刪除我的任何註釋
enum RESERVE
{
	//其他未知
	TK_UNK = -1,	//未知的TOKEN 將直接忽略 如果TK_UNK位於參數中則直接將當前行視為空行忽略

	//文件
	TK_WHITESPACE,		//任和 ' ', '\t', '\r', '\n', '\v', '\f' 有關的空白字符

	//註釋
	TK_COMMENT,		// "//", /**/ (註釋)

	//數值類型
	TK_ANY,
	TK_STRING,		//任何字符串
	TK_CSTRING,		//被 ' ' 或 " " 包圍的常量字符串
	TK_INT,			//任何整數
	TK_DOUBLE,		//任何符點數
	TK_BOOL,		//"真" "假" "true" 或 "false"
	TK_TABLE,
	TK_NIL,
	TK_LOCALTABLE,

	//邏輯符號
	TK_GT,  //">"  (大於)
	TK_LT,  //"<"  (小於)
	TK_GEQ, //">=" (大於等於)
	TK_LEQ, //"<=" (小於等於)
	TK_NEQ, //"!=" (不等於)
	TK_EQ,  //"==" (等於)

	//運算符號
	TK_ADD, //"+"
	TK_SUB, // '-'
	TK_MUL, // '*'
	TK_DIV, // '/'
	TK_MOD, // '%'
	TK_SHL, // "<<"
	TK_SHR, // ">>"
	TK_AND, // "&"
	TK_OR,  // "|"
	TK_NOT, // "~"
	TK_XOR, // "^"
	TK_NEG, //"!" (與C++的 ! 同義)
	TK_INC, // "++" (與C++的 ++ 同義)
	TK_DEC, // "--" (與C++的 -- 同義)

	//自訂運算符
	TK_FUZZY,			//如果是單獨問號 '?' 後面沒有接續變量名

	//關鍵命令 (所有關鍵命令必定是中文)
	TK_IF,
	TK_WHILE,
	TK_REPEAT,
	TK_UNTIL,
	TK_IN,
	TK_DO,
	TK_THEN,
	TK_CALL,			// "調用" 調用標記
	TK_CALLWITHNAME,
	TK_GOTO,			// "跳轉" 跳轉至標記
	TK_JMP,				// 跳轉到指定行號
	TK_RETURN,			// "返回" 用於返回調用處的下一行
	TK_BAK,				// "返回" 用於返回跳轉處的下一行
	TK_EXIT,				// "結束" 直接結束腳本
	TK_PAUSE,			// "暫停" 暫停腳本
	TK_LABEL,			//"標記" 提供給跳轉命令使用 
	TK_FUNCTION,		// "函數" 定義函數
	TK_END,
	TK_FOR,				// for循環
	TK_BREAK,
	TK_CONTINUE,
	TK_LOCAL,
	TK_GLOBAL,
	TK_ELSE,
	TK_ELSEIF,
	TK_TABLESET,

	//基礎類
	TK_CMD,				//(其他 關鍵命令)
	TK_MULTIVAR,		//多個變量
	TK_FORMAT,			// 格式化後將新數值字符串賦值給變量
	TK_INCDEC,			// 自增自減
	TK_CAOS,			// CAOS命令
	TK_EXPR,			// 表達式

	TK_LABELNAME,			// 標記名稱 不允許使用純數字 或符點數作為標記名稱
	TK_FUNCTIONNAME,
	TK_FUNCTIONARG,
	//其他
	TK_LUABEGIN,
	TK_LUACONTENT,
	TK_LUAEND,

};
Q_DECLARE_METATYPE(RESERVE)

enum Field
{
	kGlobal = 0,
	kLocal = 1,
};
Q_DECLARE_METATYPE(Field)

//單個Token的其他解析數據、和原始數據
struct Token
{
	RESERVE type = TK_UNK;
	QVariant data = "";
	QString raw = "";
};
Q_DECLARE_METATYPE(Token)

using TokenMap = QMap<qint64, Token>;
Q_DECLARE_METATYPE(TokenMap)

class Node
{
public:
	Node() = default;
	virtual ~Node() = default;

	QString name = "";//函數名稱
	QList<Node*> children; // 指向子節點的指針列表

	qint64 beginLine = 0ll;
	qint64 endLine = 0ll;
	Field field = kGlobal;
	qint64 level = 0ll;
	quint64 callCount = 0ui64;
};
Q_DECLARE_METATYPE(Node)

class LuaNode : public Node
{
public:
	QString content = "";

	void clear()
	{
		name.clear();
		children.clear();
		beginLine = 0ll;
		endLine = 0ll;
		field = kGlobal;
		level = 0ll;
		callCount = 0ui64;
		content.clear();
	}
};
Q_DECLARE_METATYPE(LuaNode)

class FunctionNode : public Node
{
public:
	FunctionNode() = default;
	virtual ~FunctionNode() = default;

	QList<QPair<RESERVE, QVariant>> argList = {};
	QHash<int, QStringList> returnTypes = {};

	qint64 callFromLine = 0ll;

	void clear()
	{
		name.clear();
		children.clear();
		beginLine = 0ll;
		endLine = 0ll;
		field = kGlobal;
		level = 0ll;
		callCount = 0ui64;
		argList.clear();
		returnTypes.clear();
	}
};
Q_DECLARE_METATYPE(FunctionNode)

class ForNode : public Node
{
public:
	ForNode() = default;
	virtual ~ForNode() = default;

	QString varName = "";
	QVariant stepValue;
	QVariant beginValue;
	QVariant endValue;

	void clear()
	{
		name.clear();
		children.clear();
		beginLine = 0ll;
		endLine = 0ll;
		field = kGlobal;
		level = 0ll;
		callCount = 0ui64;
		varName.clear();
		stepValue = QVariant("nil");
		beginValue = QVariant("nil");
		endValue = QVariant("nil");
	}
};
Q_DECLARE_METATYPE(ForNode)

class Lexer : public Indexer
{
public:
	Lexer(qint64 index) { setIndex(index); }

	static bool tokenized(Lexer* pLexer, const QString& script);

	QList<FunctionNode> getFunctionNodeList() const { return functionNodeList_; }
	QList<ForNode> getForNodeList() const { return forNodeList_; }
	QList<LuaNode> getLuaNodeList() const { return luaNodeList_; }
	QHash<QString, qint64> getLabelList() const { return labelList_; }
	QHash<qint64, TokenMap> getTokenMaps() const { return tokens_; }

private:
	enum ErrorType
	{
		kTypeError = 6,
		kTypeWarning = 10,
	};

	void showError(const QString text, ErrorType type = kTypeError);

	bool isTable(const QString& str) const;
	bool isDouble(const QString& str) const;
	bool isInteger(const QString& str) const;
	bool isBool(const QString& str) const;
	bool isLabelName(const QString& str, RESERVE previousType) const;
	bool isConstString(const QString& str) const;
	bool isSpace(const QChar& ch) const;
	bool isComment(const QChar& ch) const;
	bool isOperator(const QChar& ch) const;
	bool isLabel(const QString& str) const;

	void recordNode();

	void tokenized(qint64 currentLine, const QString& line, TokenMap* ptoken, QHash<QString, qint64>* plabel);

	void createToken(qint64 index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken);

	void insertToken(qint64 index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken);

	void createEmptyToken(qint64 index, TokenMap* ptoken);

	RESERVE getTokenType(qint64& pos, RESERVE previous, QString& str, const QString raw) const;

	bool getStringCommandToken(QString& src, const QString& delim, QString& out) const;

	void checkPairs(const QString& beginstr, const QString& endstr, const QHash<qint64, TokenMap>& stokenmaps);

	void checkSingleRowPairs(const QString& beginstr, const QString& endstr, const QHash<qint64, TokenMap>& stokenmaps);

	void checkFunctionPairs(const QHash<qint64, TokenMap>& tokenmaps);

#ifdef TEST_LEXER
	// new lexer build by 09052023
public:


private:

	class Reader
	{
	public:
		Reader() = default;
		Reader(const QString& str) : nowToken_(str), nowTokenLength_(str.length()) {}
		inline void resetIndex(qint64 index = 0ll) { nowIndex_ = index; }
		inline void resetToken(const QString& str) { nowToken_ = str; nowTokenLength_ = str.length(); }
		inline void movenext(qint64 step = 1) { nowIndex_ += step; }
		inline void moveprev(qint64 step = 1) { nowIndex_ -= step; }
		QChar next();
		QChar peek();
		QChar prev();
		bool checkNext(QChar ch);
		bool checkPrev(QChar ch);

		void store(QChar ch) { storedString_.append(ch); }
		qint64 storedLength() const { return storedString_.length(); }
		bool isStoredEmpty() const { return storedString_.simplified().isEmpty(); }
		void takeStored()
		{
			const QString ret = storedString_;

			if (!ret.isEmpty())
				list_.append(ret);

			storedString_.clear();
		}

		void moveToNextLine() { ++currentLine_; }

		QStringList getList() const { return list_; }

		void clear()
		{
			nowIndex_ = 0;
			nowToken_ = "";
			nowTokenLength_ = 0;
			storedString_ = "";
			list_.clear();
			currentLine_ = 0ll;
		}
	private:
		qint64 nowIndex_ = 0;

		QString nowToken_ = "";

		qint64 nowTokenLength_ = 0;

		QString storedString_ = "";

		QStringList list_;

		qint64 currentLine_ = 0ll;
	};

	RESERVE getTokenType(qint64 currentPos, RESERVE previous, const QString& token);

	QString getLuaTableString(const sol::table& t, int& depth);
	sol::object getLuaTableFromString(const QString& str);

	bool splitToStringToken(QString src, QStringList* pTokenStringList);
	bool checkOperator(RESERVE previous, QString& tokenStr, RESERVE* pReserve);
	Token getNextToken(RESERVE previous, QStringList& refTokenStringList);
#endif

	inline void clear()
	{
		functionNodeList_.clear();
		forNodeList_.clear();
		labelList_.clear();
		tokens_.clear();

#ifdef TEST_LEXER
		beginFunctionDeclaration_ = false;
		beginFunctionNameDeclaration_ = false;
		beginFunctionArgsDeclaration_ = false;
		functionArgList_.clear();

		beginForArgs_ = false;
		forArgList_.clear();
#endif
	}

private:
	qint64 currentLine_ = 0ll;

	LuaNode luaNode_ = {};
	bool beginLuaCode_ = false;

	QList<LuaNode> luaNodeList_;
	QList<FunctionNode> functionNodeList_;
	QList<ForNode> forNodeList_;
	QHash<QString, qint64> labelList_;
	QHash<qint64, TokenMap> tokens_;

#ifdef TEST_LEXER
	sol::state lua_;
	Reader reader_;

	bool beginFunctionDeclaration_ = false;
	bool beginFunctionNameDeclaration_ = false;
	bool beginFunctionArgsDeclaration_ = false;
	QList<Token> functionArgList_;

	bool beginForArgs_ = false;
	QList<Token> forArgList_;
#endif
};

