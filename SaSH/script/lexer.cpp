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

#include "stdafx.h"
#include "lexer.h"
#include <gamedevice.h>
#include "signaldispatcher.h"

#pragma region KeyWord
//全局關鍵字映射表 這裡是新增新的命令的第一步，其他需要在interpreter.cpp中新增註冊新函數，這裡不添加的話，腳本分析後會忽略未知的命令
static const QHash<QString, RESERVE> keywords = {
	{ "[call]", TK_CALLWITHNAME },
	//... 其他後續增加的關鍵字
	{ "#lua", TK_LUABEGIN },
	{ "#endlua", TK_LUAEND },

	//keyword，關鍵字 全權交由parser處理
	{ "call", TK_CALL },
	{ "goto", TK_GOTO },
	{ "jmp", TK_JMP },
	{ "end", TK_END },
	{ "return", TK_RETURN },
	{ "back", TK_BAK },
	{ "exit", TK_EXIT },
	{ "pause", TK_PAUSE },
	{ "function", TK_FUNCTION, },
	{ "label", TK_LABEL, },
	{ "if", TK_IF },
	{ "for", TK_FOR },
	{ "break", TK_BREAK },
	{ "continue", TK_CONTINUE },

	//system 系統命令 交由interpreter處理
	{ "run", TK_CMD },
	{ "dostr", TK_CMD },
};
#pragma endregion

#pragma region  Tool
void Lexer::showError(const QString text, ErrorType type)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	if (type == kTypeError)
		emit signalDispatcher.appendScriptLog(QObject::tr("[error]") + text, 0);
	else
		emit signalDispatcher.appendScriptLog(QObject::tr("[warn]") + text, 0);
}

bool Lexer::isTable(const QString& str) const
{
	return str.startsWith("{") && str.endsWith("}");
}

bool Lexer::isDouble(const QString& str) const
{
	if (str.count('.') != 1)
		return false;

	long long size = str.size();
	for (long long i = 0; i < size; ++i)
	{
		if (!str.at(i).isDigit() && str.at(i) != '.')
			return false;
	}

	bool ok;
	str.toDouble(&ok);
	return ok;
}

bool Lexer::isInteger(const QString& str) const
{
	long long size = str.size();
	for (long long i = 0; i < size; ++i)
	{
		if (i == 0 && (str.at(i) == '+' || str.at(i) == '-'))
		{
			continue;
		}
		if (!str.at(i).isDigit())
			return false;
	}

	bool ok;
	str.toLongLong(&ok);
	return ok;
}

bool Lexer::isBool(const QString& str) const
{
	return (str == QString("真") || str == QString("假") || str.toLower() == "true" || str.toLower() == "false");
}

bool Lexer::isLabelName(const QString& str, RESERVE previousType) const
{
	//check not start from number
	if (str.isEmpty() || str.at(0).isDigit())
		return false;

	return previousType == TK_LABEL;
}

bool Lexer::isConstString(const QString& str) const
{
	static const QRegularExpression re(R"('\s*+\s*')");
	return (str.startsWith("\'") || str.startsWith("\"")) && (str.endsWith("\'") || str.endsWith("\"")) && !str.contains(re);
}

bool Lexer::isLabel(const QString& str) const
{
	return  keywords.value(str, TK_UNK) == TK_LABEL;
}

bool Lexer::isSpace(const QChar& ch) const
{
	static const QRegularExpression re("\\s?");
	return re.match(ch).hasMatch();
}

bool Lexer::isComment(const QChar& ch) const
{
	return (ch == '/');
}

bool Lexer::isOperator(const QChar& ch) const
{
	return (
		(ch == '<') || (ch == '>') ||
		(ch == '+') || (ch == '-') || (ch == '*') || (ch == '/') ||
		(ch == '%') ||
		(ch == '&') || (ch == '|') || (ch == '~') || (ch == '^') ||
		(ch == '!')
		);
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////

#pragma region OLD

//解析整個腳本至多個TOKEN
bool Lexer::tokenized(Lexer* pLexer, const QString& script)
{
	pLexer->clear();

	QStringList lines = script.split("\n");
	long long size = lines.size();
	for (long long i = 0; i < size; ++i)
	{
		TokenMap tk;
		pLexer->tokenized(i, lines.value(i), &tk, &pLexer->labelList_);
		pLexer->tokens_.insert(i, tk);
	}

	pLexer->recordNode();

	pLexer->checkFunctionPairs(pLexer->tokens_);

	return true;
}

//解析單行內容至多個TOKEN
void Lexer::tokenized(long long currentLine, const QString& line, TokenMap* ptoken, QHash<QString, long long>* plabel)
{
	if (ptoken == nullptr || plabel == nullptr)
		return;

	long long pos = 0;
	QString token;
	QVariant data;
	QString raw = line.trimmed();
	QString originalRaw = raw;
	RESERVE type = TK_UNK;

	ptoken->clear();

	do
	{
		if (!beginCommentChunk_ && raw.startsWith("#lua") && !beginLuaCode_)
		{
			beginLuaCode_ = true;
			luaNode_.clear();
			luaNode_.beginLine = currentLine;
			luaNode_.field = kGlobal;
			luaNode_.level = 0;
			createToken(pos, TK_LUABEGIN, line, line, ptoken);
			break;
		}
		else if (!beginCommentChunk_ && raw.startsWith("#endlua") && beginLuaCode_)
		{
			beginLuaCode_ = false;
			luaNode_.endLine = currentLine;
			luaNodeList_.append(luaNode_);
			luaNode_.clear();
			createToken(pos, TK_LUAEND, line, line, ptoken);
			break;
		}
		else if (!beginCommentChunk_ && beginLuaCode_)
		{
			luaNode_.content.append(line + "\n");
			createToken(pos, TK_LUACONTENT, "[lua]", "[lua]", ptoken);
			createToken(pos + 1, TK_LUACONTENT, line, line, ptoken);
			break;
		}
		else if (raw.startsWith("--[[") && !beginCommentChunk_ && !beginLuaCode_)
		{
			beginCommentChunk_ = true;
			createToken(0, TK_COMMENT, "", "", ptoken);
			createToken(pos + 1, TK_COMMENT, originalRaw, originalRaw, ptoken);
			break;
		}
		else if ((raw.startsWith("]]") || raw.endsWith("]]")) && beginCommentChunk_ && !beginLuaCode_)
		{
			beginCommentChunk_ = false;
			createToken(0, TK_COMMENT, "", "", ptoken);
			createToken(pos + 1, TK_COMMENT, originalRaw, originalRaw, ptoken);
			break;
		}
		else if (beginCommentChunk_ && !beginLuaCode_)
		{
			createToken(pos, TK_COMMENT, "", "", ptoken);
			createToken(pos + 1, TK_COMMENT, originalRaw, originalRaw, ptoken);
			break;
		}

		long long commentIndex = raw.indexOf("//");


		//處理整行註釋
		if (commentIndex == 0 && (raw.trimmed().indexOf("//") == 0))
		{
			createToken(pos, TK_COMMENT, "", "", ptoken);
			createToken(pos + 1, TK_COMMENT, originalRaw, originalRaw, ptoken);
			break;
		}
		//當前token移除註釋
		else if (commentIndex > 0)
			raw = raw.mid(0, commentIndex).trimmed();

		//自動補齊括號列表，此處為用戶調用函數補齊括號
		static const QStringList tempReplacementList = {
			"set", "print", "printf", "msg", "dlg", "findpath", "findnpc", "rex", "regex", "rexg", "format", "run",
			"say", "sleep", "saveset", "loadset", "lclick", "rclick", "dbclick", "dragto", "chmap", "w", "download",
			"move", "cls", "eo", "logout", "logback", "runex", "openwindow", "rungame", "closegame", "setlogin", "dostrex",
			"getgamestate", "loadsetex", "createch", "delch", "menu", "checkdaily", "button", "join", "leave", "kick", "send",
			"chname", "chpetname", "doffstone", "dir", "walkpos", "chpet",
			"bh", "bj", "bp", "bs", "be", "bd", "bi", "bn", "bw", "bwn", "bwait", "bend",
			"waitpos", "waitmap", "waititem", "waitteam", "waitpet", "waitsay", "waitdlg",
			"usemagic", "doffpet", "buy", "sell",  "sellpet","useitem", "doffitem",
			"swapitem", "pickup", "putitem","getitem", "putpet", "getpet", "putstone",  "getstone", "make", "cook", "uequip",
			"requip", "wequip", "puequip", "pequip","skup", "learn", "trade", "mail",
		};

		for (const QString& it : tempReplacementList)
		{
			if (raw.startsWith(it + " "))//有參數
			{
				//將xxx 改為set(
				raw.replace(it + " ", it + "(");
				raw.append(")");
				originalRaw = raw;
				break;
			}
			else if (raw == it)//無參數
			{
				raw.append("()");
				originalRaw = raw;
				break;
			}
		}

		bool doNotLowerCase = false;
		//a,b,c = 1,2,3
		static const QRegularExpression rexMultiVar(R"(([local]*\s*[_a-zA-Z\p{Han}][\w\p{Han}]*(?:\s*,\s*[_a-zA-Z\p{Han}][\w\p{Han}]*)*)\s*=\s*([^,]+(?:\s*,\s*[^,]+)*)\;*$)");
		//local a,b,c = 1,2,3
		//static const QRegularExpression rexMultiLocalVar(R"([lL][oO][cC][aA][lL]\s+([_a-zA-Z\p{Han}][\w\p{Han}]*(?:\s*,\s*[_a-zA-Z\p{Han}][\w\p{Han}]*)*)\s*=\s*([^,]+(?:\s*,\s*[^,]+)*)$\;*)");
		//static const QRegularExpression rexMultiLocalVar(R"(^(.*\,.+?)\s*=\s*(.*?)$)");
		//var++, var--
		static const QRegularExpression varIncDec(R"((?!\d)([\p{Han}\W\w]+)(\+\+|--)\;*$)");
		//+= -= *= /= &= |= ^= %=
		static const QRegularExpression varCAOs(R"((?!\d)([\p{Han}\W\w]+)\s*([+\-*\/&\|\^%]\=)\s*([\W\w\s\p{Han}]+)\;*$)");
		//>>= <<=
		static const QRegularExpression varCAOs2(R"((?!\d)([\p{Han}\W\w]+)\s*(>>\=|<<=)\s*([\W\w\s\p{Han}]+)\;*$)");
		//x = expr
		//static const QRegularExpression varExpr(R"(([\w\p{Han}]+)\s*\=\s*([\W\w\s\p{Han}]+)\;*)");
		//+ - * / % & | ^ ( )
		static const QRegularExpression varAnyOp(R"([+\-*\/%&|^\(\)])");
		//if expr op expr
		static const QRegularExpression varIf(R"([iI][fF][\s*\(\s*|\s+]([\w\W\p{Han}]+\s*[<|>|\=|!][\=]*\s*[\w\W\p{Han}]+)\s*,\s*([\w\W\p{Han}]+))");
		//if expr
		static const QRegularExpression varIf2(R"([iI][fF][\s*\(\s*|\s+]\s*([\w\W\p{Han}]+)\s*,\s*([\w\W\p{Han}]+)\s*([\w\W\p{Han}]*)$)");
		//function name()
		static const QRegularExpression rexFunction(R"([fF][uU][nN][cC][tT][iI][oO][nN]\s+([\w\p{Han}]+)\s*\(([\w\W\p{Han}]*)\))");
		//xxx()
		static const QRegularExpression rexCallFunction(R"(^([\w\p{Han}]+)\s*\(([\w\W\p{Han}]*)\)$\;*)");
		//for i = 1, 10 or for i = 1, 10, 2 or for (i = 1, 10) or for (i = 1, 10, 2)
		static const QRegularExpression rexCallFor(R"([fF][oO][rR]\s*\(*([\w\p{Han}]+)\s*=\s*([^,]+)\s*,\s*([^,]+)\s*[\)]*)");
		static const QRegularExpression rexCallForForever(R"([fF][oO][rR]\s*\(*\s*,\s*,\s*\)*\s*[do]*)");
		static const QRegularExpression rexCallForWithStep(R"([fF][oO][rR]\s*\(*([\w\p{Han}]+)\s*=\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,\)]+)\s*[\)]*)");
		//static const QRegularExpression rexTable(R"(([\w\p{Han}]+)\s*=\s*(\{[\s\S]*\})\;*)");
		//static const QRegularExpression rexLocalTable(R"([lL][oO][cC][aA][lL]\s+([\w\p{Han}]+)\s*=\s*(\{[\s\S]*\})\;*)");
		//處理if
		if (raw.startsWith("if"))
		{
			QRegularExpressionMatch match = varIf.match(raw);
			if (!match.hasMatch())
				match = varIf2.match(raw);

			if (match.hasMatch())
			{
				token = "if";
				raw = raw.mid(2).trimmed();
				type = TK_IF;
			}
		}
		//處理for 3參
		else if (raw.contains(rexCallForWithStep))
		{
			QRegularExpressionMatch match = rexCallForWithStep.match(raw);
			if (match.hasMatch())
			{
				QString varName = match.captured(1).simplified();
				QString exprA = match.captured(2).simplified();
				QString exprB = match.captured(3).simplified();
				QString exprC = match.captured(4).simplified();

				const QString cmd = "for";
				createToken(pos, TK_FOR, cmd, cmd, ptoken);
				createToken(pos + 1, TK_STRING, varName, varName, ptoken);
				createToken(pos + 2, TK_STRING, exprA, exprA, ptoken);
				createToken(pos + 3, TK_STRING, exprB, exprB, ptoken);
				createToken(pos + 4, TK_STRING, exprC, exprC, ptoken);
			}
			break;
		}
		//處理for 2參
		else if (raw.contains(rexCallFor))
		{
			QRegularExpressionMatch match = rexCallFor.match(raw);
			if (match.hasMatch())
			{
				QString varName = match.captured(1).simplified();
				QString exprA = match.captured(2).simplified();
				QString exprB = match.captured(3).simplified();

				const QString cmd = "for";
				createToken(pos, TK_FOR, cmd, cmd, ptoken);
				createToken(pos + 1, TK_STRING, varName, varName, ptoken);
				createToken(pos + 2, TK_STRING, exprA, exprA, ptoken);
				createToken(pos + 3, TK_STRING, exprB, exprB, ptoken);
			}
			break;
		}
		//處理for forver
		else if (raw.contains(rexCallForForever))
		{
			QRegularExpressionMatch match = rexCallForForever.match(raw);
			if (match.hasMatch())
			{
				const QString varName = "";
				QString cmd = "for";
				createToken(pos, TK_FOR, cmd, cmd, ptoken);
				createToken(pos + 1, TK_STRING, varName, varName, ptoken);
				createToken(pos + 2, TK_STRING, "", "", ptoken);
				createToken(pos + 3, TK_STRING, "", "", ptoken);
			}
			break;
		}
		//處理自增自減
		else if ((raw.count("++") == 1 || raw.count("--") == 1) && raw.contains(varIncDec) && !raw.front().isDigit())
		{
			//拆分出變數名稱 和 運算符
			QRegularExpressionMatch match = varIncDec.match(raw);
			if (match.hasMatch())
			{
				QString varName = match.captured(1).simplified();
				QString op = match.captured(2).simplified();

				createToken(pos, TK_INCDEC, varName, varName, ptoken);
				createToken(pos + 1, op == "++" ? TK_INC : TK_DEC, op, op, ptoken);
			}
			break;
		}
		//處理+= -+ *= /= %= |= &= 
		else if ((raw.contains(varCAOs) || raw.contains(varCAOs2)) && !raw.front().isDigit())
		{
			QRegularExpressionMatch match = varCAOs.match(raw);
			if (!match.hasMatch())
				match = varCAOs2.match(raw);

			if (match.hasMatch())
			{
				QString notuse;
				QString varName = match.captured(1).simplified();
				QString op = match.captured(2).simplified();
				QString value = match.captured(3).simplified();
				long long p = pos + 1;
				RESERVE optype = getTokenType(p, TK_CAOS, op, op);
				++p;
				RESERVE valuetype = getTokenType(p, optype, value, value);

				createToken(pos, TK_CAOS, varName, varName, ptoken);  //變量
				createToken(pos + 1, optype, op, op, ptoken);         //運算符
				createToken(pos + 2, valuetype, value, value, ptoken);//值
			}
			break;
		}
		////處理單一或多個全局變量聲明+初始化 或 已存在的局變量重新賦值
		//else if (raw.count("=") == 1 && raw.contains(rexMultiVar)
		//	&& !raw.front().isDigit())
		//{
		//	QRegularExpressionMatch match = rexMultiVar.match(raw);
		//	if (match.hasMatch())
		//	{
		//		token = match.captured(1).simplified();
		//		raw = match.captured(2).simplified();
		//		type = TK_MULTIVAR;
		//		doNotLowerCase = true;
		//	}
		//}
#if 0
		//處理單一局表
		else if (raw.count("=") == 1 && raw.contains(rexLocalTable) && !raw.front().isDigit() && raw.contains("{") && raw.contains("}"))
		{
			QRegularExpressionMatch match = rexLocalTable.match(raw);
			if (match.hasMatch())
			{
				QString varName = match.captured(1).simplified();
				QString expr = match.captured(2).simplified();
				createToken(pos, TK_LOCALTABLE, varName, varName, ptoken);
				createToken(pos + 1, TK_LOCALTABLE, expr, expr, ptoken);
			}
			break;
		}
		//處理單一全局表
		else if (raw.count("=") == 1 && raw.contains(rexTable) && !raw.front().isDigit() && raw.contains("{") && raw.contains("}"))
		{
			QRegularExpressionMatch match = rexTable.match(raw);
			if (match.hasMatch())
			{
				QString varName = match.captured(1).simplified();
				QString expr = match.captured(2).simplified();
				createToken(pos, TK_TABLE, varName, varName, ptoken);
				createToken(pos + 1, TK_TABLE, expr, expr, ptoken);
			}
			break;
		}
		//處理單一或多個局變量聲明+初始化
		else if (raw.count("=") == 1 && raw.contains(rexMultiLocalVar)
			&& (!raw.contains(varAnyOp) || raw.indexOf(varAnyOp) > raw.indexOf("\'") || raw.indexOf(varAnyOp) > raw.indexOf("\""))
			&& !raw.front().isDigit())
		{
			QRegularExpressionMatch match = rexMultiLocalVar.match(raw);
			if (match.hasMatch())
			{
				token = match.captured(1).simplified();
				raw = match.captured(2).simplified();
				type = TK_LOCAL;
				doNotLowerCase = true;
			}
		}
		//處理單一或多個全局變量聲明+初始化 或 已存在的局變量重新賦值
		else if (raw.count("=") == 1 && raw.contains(rexMultiVar)
			&& (!raw.contains(varAnyOp) || raw.indexOf(varAnyOp) > raw.indexOf("\'") || raw.indexOf(varAnyOp) > raw.indexOf("\""))
			&& !raw.front().isDigit())
		{
			QRegularExpressionMatch match = rexMultiVar.match(raw);
			if (match.hasMatch())
			{
				token = match.captured(1).simplified();
				raw = match.captured(2).simplified();
				type = TK_MULTIVAR;
				doNotLowerCase = true;
			}
		}
		//處理變量賦值數學表達式
		else if (raw.contains(varExpr) && raw.contains(varAnyOp) && !raw.front().isDigit())
		{
			QRegularExpressionMatch match = varExpr.match(raw);
			if (match.hasMatch())
			{
				QString varName = match.captured(1).simplified();
				QString expr = match.captured(2).simplified();

				createToken(pos, TK_EXPR, varName, varName, ptoken);
				createToken(pos + 1, TK_STRING, expr, expr, ptoken);
			}
			break;
		}
#endif
		else
		{
			//處理整行註釋
			bool isCommand = raw.trimmed().startsWith("//");
			if (!isCommand)
				isCommand = raw.trimmed().startsWith("/*");

			//處理使用正規function(...)聲明的語法
			if (!isCommand && raw.contains(rexFunction))
			{
				QRegularExpressionMatch match = rexFunction.match(raw);
				if (!match.hasMatch())
					break;
				token = "function";
				QStringList args;
				for (long long i = 1; i <= 3; ++i)
				{
					args.append(match.captured(i).simplified());
				}
				raw = args.join(",").simplified();
				if (raw.endsWith(","))
					raw.chop(1);
			}
			else if (!isCommand && !raw.contains("function", Qt::CaseInsensitive) && raw.contains(rexCallFunction))
			{
				QRegularExpressionMatch match = rexCallFunction.match(raw);
				if (!match.hasMatch())
					break;

				token = match.captured(1).simplified();
				raw = match.captured(2).trimmed();
				type = keywords.value(token, TK_UNK);
				if (type == TK_UNK)
				{
					raw = QString("%1,%2").arg(token, raw);
					token = "[call]";
					createToken(100, TK_STRING, originalRaw.trimmed(), originalRaw.trimmed(), ptoken);
				}
			}
			else
			{
				//以空格為分界分離出第一個TOKEN(命令)
				if (getStringToken(raw, " ", token) == FTK_EOF)
				{
					createEmptyToken(pos, ptoken);
					break;
				}

				if (token.isEmpty())
				{
					createEmptyToken(pos, ptoken);
					break;
				}

				//遇到註釋
				if (token.startsWith("//") || token.startsWith("/*"))
				{
					createToken(pos, TK_COMMENT, "", "", ptoken);
					createToken(pos + 1, TK_COMMENT, data, token, ptoken);
					break;
				}
			}
			//檢查第一個TOKEN是否存在於關鍵字表，否則視為空行
			type = keywords.value(token, TK_UNK);
			if (type == TK_UNK)
			{
				createToken(pos, TK_LUASTRING, "[lua]", "[lua]", ptoken);
				createToken(pos + 1, TK_STRING, originalRaw, originalRaw, ptoken);
				//showError(QObject::tr("@ %1 | Unknown command '%2' has been ignored").arg(currentLine + 1).arg(token), kTypeWarning);
				//createEmptyToken(pos, ptoken);
				break;
			}
		}

		if (!doNotLowerCase)
			createToken(pos, type, QVariant::fromValue(token.toLower()), token.toLower(), ptoken);//命令必定是小寫
		else
			createToken(pos, type, QVariant::fromValue(token), token, ptoken);//一個或多個變量名不轉換大小寫
		++pos;

		//以","分界取TOKEN
		for (;;)
		{
			raw = raw.trimmed();

			if (raw.contains(","))
			{
				if (getStringToken(raw, ",", token) == FTK_EOF)
					break;
			}
			else if (raw.isEmpty())
				break;
			else
			{
				token = raw;
				raw.clear();
			}

			//忽略空白TOKEN (用戶省略的參數以空字符串佔位)
			if (token.isEmpty())
			{
				createToken(pos, TK_STRING, "", "", ptoken);
				++pos;
				continue;
			}

			//檢查TOKEN類型
			RESERVE prevType = type;
			type = getTokenType(pos, prevType, token, raw);

			if (type == TK_INT)//對整數進行轉換處理
			{
				bool ok;
				long long intValue = token.toLongLong(&ok);
				if (ok)
					data = QVariant::fromValue(intValue);
			}
			else if (type == TK_BOOL)
			{
				type = TK_BOOL;
				//真為1，假為0, true為1，false為0
				if (token.toLower() == "true" || token == "真")
					data = QVariant::fromValue(true);
				else if (token.toLower() == "false" || token == "假")
					data = QVariant::fromValue(false);
			}
			else if (type == TK_DOUBLE)//對雙精度浮點數進行強制轉INT處理
			{
				bool ok;
				double floatValue = token.toDouble(&ok);
				if (ok)
				{
					data = QVariant::fromValue(floatValue);
					type = TK_DOUBLE;
				}
			}
			else if (type == TK_CSTRING)//對常量字串進行轉換處理，去除首尾單引號、雙引號，並標記為常量字串
			{
				token = token.mid(1, token.length() - 2);
				data = QVariant::fromValue(token);
			}
			else if (type == TK_STRING)//對普通字串進行轉換處理
			{
				data = QVariant::fromValue(token);
			}
			else if (type == TK_FUNCTIONARG)//處理調用傳參
			{
				if ((token.startsWith("\"") || token.startsWith("\'")) && (token.endsWith("\"") || token.endsWith("\'")))
				{
					token = token.mid(1, token.length() - 2);
					type = TK_CSTRING;
				}

				data = QVariant::fromValue(token);
			}
			else if (type == TK_LABELNAME)//保存標記名稱
			{
				data = QVariant::fromValue(token);
				if (prevType == TK_LABEL)
					plabel->insert(token, currentLine);
			}
			else
			{
				data = QVariant::fromValue(token);
			}

			createToken(pos, type, data, token, ptoken);
			++pos;
		}
	} while (false);
}

//更新並記錄每個函數塊的開始行和結束行
void Lexer::recordNode()
{
	QHash<long long, FunctionNode> chunkHash;
	QMap<long long, TokenMap> map;
	for (auto it = tokens_.cbegin(); it != tokens_.cend(); ++it)
		map.insert(it.key(), it.value());

	//這裡是為了避免沒有透過call調用函數的情況
	QStack<RESERVE> reserveStack;
	QStack<FunctionNode> functionNodeStack;
	QStack<ForNode> forNodeStack;

	QList<Node> extraEndNodeList;
	long long currentIndentLevel = 0;
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		long long row = it.key();
		TokenMap tk = it.value();

		if (TK_LUABEGIN == tk.value(0).type)
		{
			beginLuaCode_ = true;
			continue;
		}
		else if (TK_LUAEND == tk.value(0).type)
		{
			beginLuaCode_ = false;
			continue;
		}
		else if (beginLuaCode_)
			continue;

		switch (tk.value(0).type)
		{
		case TK_FUNCTION:
		{
			reserveStack.push(TK_FUNCTION);
			++currentIndentLevel;

			FunctionNode functionNode;
			functionNode.name = tk.value(1).data.toString();
			functionNode.beginLine = row;
			functionNode.endLine = -1; // 初始结束行号设为 -1
			functionNode.level = currentIndentLevel;
			functionNode.field = currentIndentLevel == 1 ? kGlobal : kLocal;

			functionNode.argList = {};

			functionNodeStack.push(functionNode);
			break;
		}
		case TK_FOR:
		{
			reserveStack.push(TK_FOR);
			++currentIndentLevel;

			ForNode forNode;
			forNode.beginLine = row;
			forNode.endLine = -1; // 初始结束行号设为 -1
			forNode.level = currentIndentLevel;
			forNode.field = currentIndentLevel == 1 ? kGlobal : kLocal;

			forNode.varName = tk.value(1).data.toString();
			//----------------------------2 是 = 符號
			forNode.beginValue = tk.value(3).data.toLongLong();
			forNode.endValue = tk.value(4).data.toLongLong();

			forNodeStack.push(forNode);

			break;
		}
		case TK_RETURN:
		{
			QStringList returnTypes;

			for (long long i = 1; i < tk.size(); ++i)
			{
				Token token = tk.value(i);
				if (token.type == TK_COMMENT)
					continue;
				returnTypes.append(token.data.toString());
			}

			if (!functionNodeStack.isEmpty())
			{
				FunctionNode functionNode = functionNodeStack.pop();
				functionNode.returnTypes.insert(row, returnTypes);
				functionNodeStack.push(functionNode);
			}
			break;
		}
		case TK_WHILE:
		{
			reserveStack.push(TK_WHILE);
			++currentIndentLevel;
			break;
		}
		case TK_REPEAT:
		{
			reserveStack.push(TK_REPEAT);
			++currentIndentLevel;
			break;
		}
		case TK_UNTIL:
		{
			reserveStack.pop();
			--currentIndentLevel;
			break;
		}
		case TK_END:
		{
			if (reserveStack.isEmpty())
			{
				Node node;
				node.beginLine = row;
				node.endLine = row;
				node.level = currentIndentLevel;
				node.field = currentIndentLevel <= 1 ? kGlobal : kLocal;
				extraEndNodeList.append(node);
				break;
			}

			RESERVE reserve = reserveStack.pop();

			if (reserve == TK_FUNCTION)
			{
				if (!functionNodeStack.isEmpty())
				{
					FunctionNode functionNode = functionNodeStack.pop();
					functionNode.endLine = row;
					functionNodeList_.append(functionNode);
				}
			}
			else if (reserve == TK_FOR)
			{
				if (!forNodeStack.isEmpty())
				{
					ForNode forNode = forNodeStack.pop();
					forNode.endLine = row;
					forNodeList_.append(forNode);
				}
			}
			else if (reserve == TK_WHILE)
			{

			}
			else if (reserve == TK_REPEAT)
			{

			}

			--currentIndentLevel;
			break;
		}
		}
	}

	QList<ForNode> forNodes = forNodeList_;
	QList<FunctionNode> functionNodes = functionNodeList_;

	beginLuaCode_ = false;

	for (auto subit = map.cbegin(); subit != map.cend(); ++subit)
	{
		long long row = subit.key();
		TokenMap tk = subit.value();

		if (TK_LUABEGIN == tk.value(0).type)
		{
			beginLuaCode_ = true;
			continue;
		}
		else if (TK_LUAEND == tk.value(0).type)
		{
			beginLuaCode_ = false;
			continue;
		}
		else if (beginLuaCode_)
			continue;

		switch (tk.value(0).type)
		{
		case TK_CONTINUE:
		case TK_BREAK:
		{
			bool ok = false;

			for (auto ssubit = forNodes.constBegin(); ssubit != forNodes.constEnd(); ++ssubit)
			{
				ForNode node = *ssubit;
				if (row > node.beginLine && row < node.endLine)
				{
					ok = true;
					break;
				}
			}

			if (!ok)
			{
				QString errorMessage = QObject::tr("@ %1 | '%2' must be used in a loop").arg(row + 1).arg(tk.value(0).type == TK_BREAK ? "break" : "continue");
				showError(errorMessage, kTypeError);
			}

			break;
		}
		case TK_RETURN:
		{
			bool ok = false;

			for (auto it = functionNodes.constBegin(); it != functionNodes.constEnd(); ++it)
			{
				FunctionNode node = *it;
				if (row > node.beginLine && row < node.endLine)
				{
					ok = true;
					break;
				}
			}

			if (!ok)
			{
				QString errorMessage = QObject::tr("@ %1 | '%2' must be used in a function").arg(row + 1).arg("return");
				showError(errorMessage, kTypeError);
			}

			break;
		}
		default:
		{
			RESERVE reserve = TK_UNK;
			for (long long i = static_cast<long long>(tk.size()) - 1; i >= 0; --i)
			{
				Token token = tk.value(i);
				if (token.raw == "break")
				{
					reserve = TK_BREAK;
					break;
				}
				else if (token.raw == "continue")
				{
					reserve = TK_CONTINUE;
					break;
				}
				else if (token.raw == "return")
				{
					reserve = TK_RETURN;
					break;
				}
			}

			if (reserve == TK_UNK)
				break;

			switch (reserve)
			{
			case TK_CONTINUE:
			case TK_BREAK:
			{
				bool ok = false;

				for (auto ssubit = forNodes.constBegin(); ssubit != forNodes.constEnd(); ++ssubit)
				{
					ForNode node = *ssubit;
					if (row > node.beginLine && row < node.endLine)
					{
						ok = true;
						break;
					}
				}

				if (!ok)
				{
					QString errorMessage = QObject::tr("@ %1 | '%2' must be used in a loop").arg(row + 1).arg(reserve == TK_BREAK ? "break" : "continue");
					showError(errorMessage, kTypeError);
				}

				break;
			}
			case TK_RETURN:
			{
				bool ok = false;

				for (auto ssubit = functionNodes.constBegin(); ssubit != functionNodes.constEnd(); ++ssubit)
				{
					FunctionNode node = *ssubit;
					if (row > node.beginLine && row < node.endLine)
					{
						ok = true;
						break;
					}
				}

				if (!ok)
				{
					QString errorMessage = QObject::tr("@ %1 | '%2' must be used in a function").arg(row + 1).arg("return");
					showError(errorMessage, kTypeError);
				}

				break;
			}
			}

			break;
		}
		}
	}

	for (const auto& it : functionNodeList_)
	{
		if (it.endLine == -1)
		{
			QString errorMessage = QObject::tr("@ %1 | Missing '%2' for statement '%3'").arg(it.beginLine + 1).arg("end").arg("function");
			showError(errorMessage, kTypeError);
		}
	}

	for (const auto& it : forNodeList_)
	{
		if (it.endLine == -1)
		{
			QString errorMessage = QObject::tr("@ %1 | Missing '%2' for statement '%3'").arg(it.beginLine + 1).arg("end").arg("for");
			showError(errorMessage, kTypeError);
		}
	}

	for (const auto& it : extraEndNodeList)
	{
		QString errorMessage = QObject::tr("@ %1 | extra '%2' were found").arg(it.beginLine + 1).arg("end");
		showError(errorMessage, kTypeError);
	}
}

//插入新TOKEN
void Lexer::createToken(long long index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken)
{
	if (ptoken != nullptr)
		ptoken->insert(index, Token{ type, data, raw });
}

//插入新TOKEN到index位置，將原本的TOKEN後移
void Lexer::insertToken(long long index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken)
{
	TokenMap tokenMap;
	tokenMap.insert(index, Token{ type, data, raw });

	for (auto it = ptoken->constBegin(); it != ptoken->constEnd(); ++it)
	{
		long long key = it.key();
		if (key + 1 <= index)
			continue;

		Token token = it.value();
		tokenMap.insert(key + 1, token);
	}

	*ptoken = tokenMap;
};

//插入空行TOKEN
void Lexer::createEmptyToken(long long index, TokenMap* ptoken)
{
	ptoken->insert(index, Token{ TK_WHITESPACE, "", "" });
}

//根據容取TOKEN應該定義的類型
RESERVE Lexer::getTokenType(long long& pos, RESERVE previous, QString& str, const QString raw) const
{
	std::ignore = pos;
	//findex = 0;

	if (str.startsWith("//") || str.startsWith("/*"))
	{
		return TK_COMMENT;
	}
	else if (isConstString(str))
	{
		return TK_CSTRING;
	}
	else if (str == "<<")
	{
		return TK_SHL;
	}
	else if (str == ">>")
	{
		return TK_SHR;
	}
	else if (str == "<")
	{
		return TK_LT;
	}
	else if (str == ">")
	{
		return TK_GT;
	}
	else if (str == "==")
	{
		return TK_EQ;
	}
	else if (str == "!=")
	{
		return TK_NEQ;
	}
	else if (str == ">=")
	{
		return TK_GEQ;
	}
	else if (str == "<=")
	{
		return TK_LEQ;
	}
	else if (str == "+" || str == "+=")
	{
		return TK_ADD;
	}
	else if (str == "++")
	{
		return TK_INC;
	}
	else if (str == "-" || str == "-=")
	{
		return TK_SUB;
	}
	else if (str == "--")
	{
		return TK_DEC;
	}
	else if (str == "*" || str == "*=")
	{
		return TK_MUL;
	}
	else if (str == "/" || str == "/=")
	{
		return TK_DIV;
	}
	else if (str == "%" || str == "%=")
	{
		return TK_MOD;
	}
	else if (str == "&" || str == "&=")
	{
		return TK_AND;
	}
	else if (str == "|" || str == "|=")
	{
		return TK_OR;
	}
	else if (str == "~")
	{
		return TK_NOT;
	}
	else if (str == "^" || str == "^=")
	{
		return TK_XOR;
	}
	else if (str == "!")
	{
		return TK_NEG;
	}
	else if (str == kFuzzyPrefix)
	{
		return TK_FUZZY;
	}
	else if (isBool(str))
	{
		str = str.toLower();
		return TK_BOOL;
	}
	else if (previous == TK_FUNCTIONNAME || previous == TK_FUNCTIONARG)
	{
		//如果前一個TOKEN是function名或區域變量名，那麼接下來的TOKEN都視為區域變量名
		return TK_FUNCTIONARG;
	}
	else if (isDouble(str))
	{
		return TK_DOUBLE;
	}
	else if (isInteger(str))
	{
		return TK_INT;
	}
	else if (isLabelName(str, previous))
	{
		return TK_LABELNAME;
	}
	else if (previous == TK_FUNCTION)
	{
		return TK_FUNCTIONNAME;
	}

	//其他的都默認為字符串
	return TK_STRING;
}

//檢查指定詞組配對
void Lexer::checkPairs(const QString& beginstr, const QString& endstr, const QHash<long long, TokenMap>& stokenmaps)
{
	QMap<long long, QString> unpairedFunctions;
	QMap<long long, QString> unpairedEnds;
	QStack<long long> functionStack;

	QMap <long long, TokenMap> tokenmaps;//轉成有序容器方便按順序遍歷
	for (auto it = stokenmaps.cbegin(); it != stokenmaps.cend(); ++it)
		tokenmaps.insert(it.key(), it.value());

	for (auto it = tokenmaps.cbegin(); it != tokenmaps.cend(); ++it)
	{
		long long row = it.key();
		QString statement = it.value().value(0).data.toString().simplified();

		if (statement != beginstr && statement != endstr)
			continue;

		if (statement == beginstr) // "function" statement
		{
			functionStack.push(row);
		}
		else if (statement == endstr) // "end" statement
		{
			if (functionStack.isEmpty()) // "end" without preceding "function"
			{
				unpairedEnds.insert(row, statement);
			}
			else // "end" with matching "function"
			{
				functionStack.pop();
			}
		}
	}

	// Any remaining functions on the stack are missing an "end"
	while (!functionStack.isEmpty())
	{
		long long row = functionStack.pop();
		QString statement = tokenmaps.value(row).cbegin().value().data.toString().simplified();
		unpairedFunctions.insert(row, statement);
	}

	// 打印所有不成對的 "function" 語句
	for (auto it = unpairedFunctions.cbegin(); it != unpairedFunctions.cend(); ++it)
	{
		long long row = it.key();
		QString statement = it.value();
		QString errorMessage = QObject::tr("@ %1 | Missing '%2' for statement '%3'").arg(row + 1).arg(endstr).arg(statement);
		showError(errorMessage, kTypeWarning);
	}

	// 打印所有不成對的 "end" 語句
	for (auto it = unpairedEnds.cbegin(); it != unpairedEnds.cend(); ++it)
	{
		long long row = it.key();
		QString statement = it.value();
		QString errorMessage = QObject::tr("@ %1 | Extra '%2' for statement '%3'").arg(row + 1).arg(endstr).arg(statement);
		showError(errorMessage, kTypeWarning);
	}
}

//檢查單行字符配對
void Lexer::checkSingleRowPairs(const QString& beginstr, const QString& endstr, const QHash<long long, TokenMap>& stokenmaps)
{
	QMap<long long, QVector<long long>> unpairedFunctions; // <Row, Vector of unpaired start indices>
	QMap<long long, QVector<long long>> unpairedEnds;      // <Row, Vector of unpaired end indices>

	QMap<long long, TokenMap> tokenmaps;
	for (auto it = stokenmaps.cbegin(); it != stokenmaps.cend(); ++it)
		tokenmaps.insert(it.key(), it.value());

	bool beginLuaCode = false;
	for (auto it = tokenmaps.cbegin(); it != tokenmaps.cend(); ++it)
	{
		long long row = it.key();
		TokenMap tokenMap = it.value();
		Token firstToken = tokenMap.value(0);
		QStringList tmp;

		if (TK_LUABEGIN == firstToken.type && !beginLuaCode)
		{
			beginLuaCode = true;
			continue;
		}
		else if (TK_LUAEND == firstToken.type && beginLuaCode)
		{
			beginLuaCode = false;
			continue;
		}
		else if (beginLuaCode)
			continue;

		for (auto it2 = tokenMap.cbegin(); it2 != tokenMap.cend(); ++it2)
		{
			tmp.append(it2.value().data.toString().simplified());
		}

		QString statement = tmp.join(" ");

		QVector<long long> startIndices;
		QVector<long long> endIndices;

		for (long long index = 0; index < statement.length(); ++index)
		{
			QChar currentChar = statement.at(index);

			if (currentChar == beginstr)
			{
				startIndices.append(index);
			}
			else if (currentChar == endstr)
			{
				if (!startIndices.isEmpty())
				{
					startIndices.removeLast(); // Matched, remove the last start index
				}
				else
				{
					endIndices.append(index); // Unpaired end index
				}
			}
		}

		// Remaining unpaired start indices are stored as unpairedFunctions
		unpairedFunctions[row] = startIndices;

		// Remaining unpaired end indices are stored as unpairedEnds
		unpairedEnds[row] = endIndices;
	}

	// 打印所有不成對的 beginstr 語句
	for (auto it = unpairedFunctions.cbegin(); it != unpairedFunctions.cend(); ++it)
	{
		long long row = it.key();
		QVector<long long> unpairedIndices = it.value();

		for (long long index : unpairedIndices)
		{
			QString statement = tokenmaps[row].value(0).data.toString().simplified();
			QString errorMessage = QString(QObject::tr("@ %1 | Unpaired '%2' index %3: '%4'")).arg(row + 1).arg(beginstr).arg(index).arg(statement);
			showError(errorMessage, kTypeWarning);
		}
	}

	// 打印所有不成對的 endstr 語句
	for (auto it = unpairedEnds.cbegin(); it != unpairedEnds.cend(); ++it)
	{
		long long row = it.key();
		QVector<long long> unpairedIndices = it.value();

		for (long long index : unpairedIndices)
		{
			QString statement = tokenmaps[row].value(0).data.toString().simplified();
			QString errorMessage = QString(QObject::tr("@ %1 | Unpaired '%2' index %3: '%4'")).arg(row + 1).arg(endstr).arg(index).arg(statement);
			showError(errorMessage, kTypeWarning);
		}
	}
}

void Lexer::checkFunctionPairs(const QHash<long long, TokenMap>& stokenmaps)
{
	checkSingleRowPairs("(", ")", stokenmaps);
	//checkSingleRowPairs("[", "]", stokenmaps);
	checkSingleRowPairs("{", "}", stokenmaps);
}

//根據指定分割符號取得字串
FTK Lexer::getStringToken(QString& src, const QString& delim, QString& out) const
{
	if (src.isEmpty() || delim.isEmpty())
		return FTK_EOF;

	enum class State { Normal, DoubleQuoted, SingleQuoted, InParentheses, InBraces };
	State state = State::Normal;

	long long i = 0;
	long long start = 0;
	long long srcSize = src.size();

	while (i < srcSize)
	{
		QChar c = src.at(i);

		if (state == State::Normal)
		{
			if (c == '"')
				state = State::DoubleQuoted;
			else if (c == '\'')
				state = State::SingleQuoted;
			else if (c == '(')
				state = State::InParentheses;
			else if (c == '{')
				state = State::InBraces;
			else if (c == delim)
				break;
		}
		else if (state == State::DoubleQuoted)
		{
			if (c == '"' && (i == 0 || src.at(i - 1) != '\\'))
				state = State::Normal;
		}
		else if (state == State::SingleQuoted)
		{
			if (c == '\'' && (i == 0 || src.at(i - 1) != '\\'))
				state = State::Normal;
		}
		else if (state == State::InParentheses)
		{
			if (c == ')')
				state = State::Normal;
		}
		else if (state == State::InBraces)
		{
			if (c == '}')
				state = State::Normal;
		}

		++i;

		if (state == State::Normal)
		{
			if (i == srcSize || src.at(i) == delim)
			{
				// 提取标记
				out = src.mid(start, i - start).trimmed();
				src.remove(0, i + delim.size());
				return FTK_CONTINUE;
			}
		}
	}

	return FTK_EOF;
}
#pragma endregion