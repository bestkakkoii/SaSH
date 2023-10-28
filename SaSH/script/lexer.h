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
constexpr long long kMaxLuaTableDepth = 1024ll;

enum FTK
{
	FTK_EOF,
	FTK_CONTINUE,
};

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

	//基礎類
	TK_CMD,				//(其他 關鍵命令)
	TK_MULTIVAR,		//多個變量
	//TK_FORMAT,			// 格式化後將新數值字符串賦值給變量
	TK_INCDEC,			// 自增自減
	TK_CAOS,			// CAOS命令
	TK_EXPR,			// 表達式

	TK_LABELNAME,			// 標記名稱 不允許使用純數字 或符點數作為標記名稱
	TK_FUNCTIONNAME,
	TK_FUNCTIONARG,
	//其他
	TK_LUASTRING,
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

using TokenMap = QMap<long long, Token>;
Q_DECLARE_METATYPE(TokenMap)

class Node
{
public:
	Node() = default;
	virtual ~Node() = default;

	QString name = "";//函數名稱
	QList<Node*> children; // 指向子節點的指針列表

	long long beginLine = 0ll;
	long long endLine = 0ll;
	Field field = kGlobal;
	long long level = 0ll;
	unsigned long long callCount = 0ui64;
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

	long long callFromLine = 0ll;

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
	explicit Lexer(long long index) : Indexer(index) {}

	static bool __fastcall tokenized(Lexer* pLexer, const QString& script);

	QList<FunctionNode> __fastcall getFunctionNodeList() const { return functionNodeList_; }
	QList<ForNode> __fastcall getForNodeList() const { return forNodeList_; }
	QList<LuaNode> __fastcall getLuaNodeList() const { return luaNodeList_; }
	QHash<QString, long long> __fastcall getLabelList() const { return labelList_; }
	QHash<long long, TokenMap> __fastcall getTokenMaps() const { return tokens_; }

private:
	enum ErrorType
	{
		kTypeError = 6,
		kTypeWarning = 10,
	};

	void __fastcall showError(const QString text, ErrorType type = kTypeError);

	bool __fastcall isTable(const QString& str) const;
	bool __fastcall isDouble(const QString& str) const;
	bool __fastcall isInteger(const QString& str) const;
	bool __fastcall isBool(const QString& str) const;
	bool __fastcall isLabelName(const QString& str, RESERVE previousType) const;
	bool __fastcall isConstString(const QString& str) const;
	bool __fastcall isSpace(const QChar& ch) const;
	bool __fastcall isComment(const QChar& ch) const;
	bool __fastcall isOperator(const QChar& ch) const;
	bool __fastcall isLabel(const QString& str) const;

	void __fastcall recordNode();

	void __fastcall tokenized(long long currentLine, const QString& line, TokenMap* ptoken, QHash<QString, long long>* plabel);

	void __fastcall createToken(long long index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken);

	void __fastcall insertToken(long long index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken);

	void __fastcall createEmptyToken(long long index, TokenMap* ptoken);

	RESERVE __fastcall getTokenType(long long& pos, RESERVE previous, QString& str, const QString raw) const;

	FTK __fastcall getStringToken(QString& src, const QString& delim, QString& out) const;

	void __fastcall checkPairs(const QString& beginstr, const QString& endstr, const QHash<long long, TokenMap>& stokenmaps);

	void __fastcall checkSingleRowPairs(const QString& beginstr, const QString& endstr, const QHash<long long, TokenMap>& stokenmaps);

	void __fastcall checkFunctionPairs(const QHash<long long, TokenMap>& tokenmaps);

	inline void __fastcall clear()
	{
		functionNodeList_.clear();
		forNodeList_.clear();
		labelList_.clear();
		tokens_.clear();
	}

private:
	long long currentLine_ = 0ll;

	LuaNode luaNode_ = {};
	bool beginLuaCode_ = false;

	bool beginCommentChunk_ = false;

	QList<LuaNode> luaNodeList_;
	QList<FunctionNode> functionNodeList_;
	QList<ForNode> forNodeList_;
	QHash<QString, long long> labelList_;
	QHash<long long, TokenMap> tokens_;

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

