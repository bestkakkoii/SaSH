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

constexpr const char* kFuzzyPrefix = "?";

//必須使用此枚舉名稱 RESERVE 請不要刪除我的任何註釋
enum RESERVE
{
	//其他未知
	TK_UNK = -1,	//未知的TOKEN 將直接忽略 如果TK_UNK位於參數中則直接將當前行視為空行忽略

	//標準分隔符
	TK_SPACE,		//(用於分隔第一個TOKEN)

	//數值類型
	TK_STRING,		//任何字符串
	TK_CSTRING,		//被 ' ' 或 " " 包圍的常量字符串
	TK_INT,			//任何整數
	TK_DOUBLE,		//任何符點數
	TK_BOOL,		//"真" "假" "true" 或 "false"

	//邏輯符號
	TK_GT,  //">"
	TK_LT,  //"<"
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
	TK_COMMENT,			// "//" (註釋)

	//關鍵命令 (所有關鍵命令必定是中文)
	TK_CALL,			// "調用" 調用標記
	TK_GOTO,			// "跳轉" 跳轉至標記
	TK_JMP,				// 跳轉到指定行號
	TK_RETURN,			// "返回" 用於返回調用處的下一行
	TK_BAK,				// "返回" 用於返回跳轉處的下一行
	TK_END,				// "結束" 直接結束腳本
	TK_PAUSE,			// "暫停" 暫停腳本
	TK_LABEL,			//"標記" 提供給調用 和 跳轉命令使用 

	//文件
	TK_WHITESPACE,		//任和 ' ', '\t', '\r', '\n', '\v', '\f' 有關的空白字符
	TK_EOF,				//"結尾"

	//基礎類
	TK_CMD,				//(其他 關鍵命令)
	TK_VARDECL,			// 變量聲明名稱或賦值 (任何使用 "變量 設置" 宣告過的變量 lexser只負責解析出那些是變量)
	TK_VARFREE,			// 變量釋放
	TK_VARCLR,			// 變量清空
	TK_MULTIVAR,		//多個變量
	TK_LABELVAR,		//標籤設置的傳參變量
	TK_LOCAL,
	TK_FORMAT,			// 格式化後將新數值字符串賦值給變量
	TK_INCDEC,			// 自增自減
	TK_CAOS,			// CAOS命令
	TK_EXPR,			// 表達式
	TK_CMP,				// 比較
	TK_FOR,				// for循環
	TK_ENDFOR,			// for循環結束
	TK_BREAK,
	TK_CONTINUE,

	TK_RND,
	TK_NAME,			// 標記名稱 不允許使用純數字 或符點數作為標記名稱

	//其他

};

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

class Lexer
{
public:
	static bool tokenized(const QString& script, QHash<qint64, TokenMap>* tokens, QHash<QString, qint64>* plabel);

private:
	enum ErrorType
	{
		kTypeError = 6,
		kTypeWarning = 10,
	};

	bool isDouble(const QString& str) const;
	bool isInteger(const QString& str) const;
	bool isBool(const QString& str) const;
	bool isName(const QString& str, RESERVE previousType) const;
	bool isConstString(const QString& str) const;
	//bool isVariable(const QString& str) const;
	bool isSpace(const QChar& ch) const;
	bool isComment(const QChar& ch) const;
	bool isOperator(const QChar& ch) const;
	bool isLabel(const QString& str) const;

	QChar next(const QString& str, qint64& index) const;
	RESERVE getTokenType(qint64& pos, RESERVE previous, QString& str, const QString raw) const;
	bool getStringToken(QString& src, const QString& delim, QString& out);

	void createToken(qint64 index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken);
	void createEmptyToken(qint64 index, TokenMap* ptoken);

	void tokenized(qint64 currentLine, const QString& line, TokenMap* ptoken, QHash<QString, qint64>* plabel);

	void checkPairs(const QString& beginstr, const QString& endstr, const QHash<qint64, TokenMap>& stokenmaps);
	void checkSingleRowPairs(const QString& beginstr, const QString& endstr, const QHash<qint64, TokenMap>& stokenmaps);
	void checkFunctionPairs(const QHash<qint64, TokenMap>& tokenmaps);

	void showError(const QString text, ErrorType type = kTypeError);
};

