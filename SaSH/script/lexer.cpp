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
#include "injector.h"
#include "signaldispatcher.h"

enum RTK
{
	RTK_EOF,
	RTK_CONTINUE,
};

#pragma region KeyWord
//全局關鍵字映射表 這裡是新增新的命令的第一步，其他需要在interpreter.cpp中新增註冊新函數，這裡不添加的話，腳本分析後會忽略未知的命令
static const QHash<QString, RESERVE> keywords = {
#pragma region zh_TW
	//keyword
	{ "調用", TK_CALL },
	{ "行數", TK_GOTO },
	{ "跳轉", TK_GOTO },
	{ "返回", TK_RETURN },
	{ "返回跳轉", TK_BAK },
	{ "結束", TK_END },
	{ "暫停", TK_PAUSE },
	{ "功能", TK_FUNCTION, },
	{ "標記", TK_LABEL, },
	{ "變量", TK_VARDECL },
	{ "變量移除", TK_VARFREE },
	{ "變量清空", TK_VARCLR },
	{ "格式化", TK_FORMAT },
	{ "隨機數", TK_RND },
	{ "如果", TK_IF },
	{ "遍歷", TK_FOR },
	{ "跳出", TK_BREAK },
	{ "繼續", TK_CONTINUE },

	//system
	{ "執行", TK_CMD },
	{ "延時", TK_CMD },
	{ "提示", TK_CMD },
	{ "消息", TK_CMD },
	{ "登出", TK_CMD },
	{ "元神歸位", TK_CMD },
	{ "回點", TK_CMD },
	{ "按鈕", TK_CMD },
	{ "說話", TK_CMD },
	{ "輸入", TK_CMD },
	{ "說出", TK_CMD },
	{ "清屏", TK_CMD },
	{ "設置", TK_CMD },
	{ "讀取設置", TK_CMD },
	{ "儲存設置", TK_CMD },
	{ "計時", TK_CMD },
	{ "菜單", TK_CMD },
	{ "創建人物", TK_CMD },
	{ "刪除人物", TK_CMD },
	{ "發包", TK_CMD },

	//check info
	{ "戰鬥中", TK_CMD },
	{ "在線中", TK_CMD },
	{ "查坐標", TK_CMD },
	{ "查座標", TK_CMD },
	{ "地圖", TK_CMD },
	{ "地圖快判", TK_CMD },
	{ "對話", TK_CMD },
	{ "看見", TK_CMD },
	{ "聽見", TK_CMD },
	{ "道具", TK_CMD },
	{ "道具數量", TK_CMD },
	{ "背包滿", TK_CMD },
	{ "寵物有", TK_CMD },
	{ "任務狀態", TK_CMD },

	//actions
	{ "人物改名", TK_CMD },
	{ "使用精靈", TK_CMD },
	{ "寵物改名", TK_CMD },
	{ "寵物郵件", TK_CMD },
	{ "更換寵物", TK_CMD },
	{ "丟棄寵物", TK_CMD },
	{ "購買", TK_CMD },
	{ "售賣", TK_CMD },
	{ "賣寵", TK_CMD },
	{ "使用道具", TK_CMD },
	{ "丟棄道具", TK_CMD },
	{ "交換道具", TK_CMD },
	{ "撿物", TK_CMD },
	{ "存入道具", TK_CMD },
	{ "提出道具", TK_CMD },
	{ "存入寵物", TK_CMD },
	{ "提出寵物", TK_CMD },
	{ "存錢", TK_CMD },
	{ "提錢", TK_CMD },
	{ "加工", TK_CMD },
	{ "料理", TK_CMD },
	{ "轉移", TK_CMD },
	{ "卸下裝備", TK_CMD },
	{ "記錄身上裝備", TK_CMD },
	{ "裝上記錄裝備", TK_CMD },
	{ "卸下寵裝備", TK_CMD },
	{ "裝上寵裝備", TK_CMD },
	{ "加點", TK_CMD },
	{ "學習", TK_CMD },
	{ "交易", TK_CMD },
	{ "寄信", TK_CMD },
	{ "丟棄石幣", TK_CMD },

	//action with sub cmd
	{ "組隊", TK_CMD },
	{ "離隊", TK_CMD },
	{ "踢走", TK_CMD },
	{ "組隊有", TK_CMD },
	{ "組隊人數", TK_CMD },

	//move
	{ "坐標", TK_CMD },
	{ "座標", TK_CMD },
	{ "移動", TK_CMD },
	{ "封包移動", TK_CMD },
	{ "方向", TK_CMD },
	{ "最近坐標", TK_CMD },
	{ "尋路", TK_CMD },
	{ "移動至NPC", TK_CMD },
	{ "過點", TK_CMD },

	//mouse
	{ "左擊", TK_CMD },
	{ "右擊", TK_CMD },
	{ "左雙擊", TK_CMD },
	{ "拖至", TK_CMD },
#pragma endregion

#pragma region zh_CN
	//keyword
	{ "调用", TK_CALL },
	{ "行数", TK_GOTO },
	{ "跳转", TK_GOTO },
	{ "返回", TK_RETURN },
	{ "返回跳转", TK_BAK },
	{ "结束", TK_END },
	{ "暂停", TK_PAUSE },
	{ "功能", TK_FUNCTION, },
	{ "标记", TK_LABEL, },
	{ "变量", TK_VARDECL },
	{ "变量移除", TK_VARFREE },
	{ "变量清空", TK_VARCLR },
	{ "格式化", TK_FORMAT },
	{ "随机数", TK_RND },
	{ "如果", TK_IF },
	{ "遍历", TK_FOR },
	{ "跳出", TK_BREAK },
	{ "继续", TK_CONTINUE },

	//system
	{ "执行", TK_CMD },
	{ "延时", TK_CMD },
	{ "提示", TK_CMD },
	{ "消息", TK_CMD },
	{ "登出", TK_CMD },
	{ "元神归位", TK_CMD },
	{ "回点", TK_CMD },
	{ "按钮", TK_CMD },
	{ "说话", TK_CMD },
	{ "输入", TK_CMD },
	{ "说出", TK_CMD },
	{ "清屏", TK_CMD },
	{ "设置", TK_CMD },
	{ "读取设置", TK_CMD },
	{ "储存设置", TK_CMD },
	{ "计时", TK_CMD },
	{ "菜单", TK_CMD },
	{ "创建人物", TK_CMD },
	{ "删除人物", TK_CMD },
	{ "发包", TK_CMD },

	//check info
	{ "战斗中", TK_CMD },
	{ "在线中", TK_CMD },
	{ "查坐标", TK_CMD },
	{ "查座标", TK_CMD },
	{ "地图", TK_CMD },
	{ "地图快判", TK_CMD },
	{ "对话", TK_CMD },
	{ "看见", TK_CMD },
	{ "听见", TK_CMD },
	{ "道具", TK_CMD },
	{ "道具数量", TK_CMD },
	{ "背包满", TK_CMD },
	{ "宠物有", TK_CMD },
	{ "宠物数量", TK_CMD },
	{ "任务状态", TK_CMD },

	//actions
	{ "人物改名", TK_CMD },
	{ "使用精灵", TK_CMD },
	{ "宠物改名", TK_CMD },
	{ "宠物邮件", TK_CMD },
	{ "更换宠物", TK_CMD },
	{ "丢弃宠物", TK_CMD },
	{ "购买", TK_CMD },
	{ "售卖", TK_CMD },
	{ "卖宠", TK_CMD },
	{ "使用道具", TK_CMD },
	{ "丢弃道具", TK_CMD },
	{ "交换道具", TK_CMD },
	{ "捡物", TK_CMD },
	{ "存入道具", TK_CMD },
	{ "提出道具", TK_CMD },
	{ "存入宠物", TK_CMD },
	{ "提出宠物", TK_CMD },
	{ "存钱", TK_CMD },
	{ "提钱", TK_CMD },
	{ "加工", TK_CMD },
	{ "料理", TK_CMD },
	{ "转移", TK_CMD },
	{ "卸下装备", TK_CMD },
	{ "记录身上装备", TK_CMD },
	{ "装上记录装备", TK_CMD },
	{ "卸下宠装备", TK_CMD },
	{ "装上宠装备", TK_CMD },
	{ "加点", TK_CMD },
	{ "学习", TK_CMD },
	{ "交易", TK_CMD },
	{ "寄信", TK_CMD },
	{ "丢弃石币", TK_CMD },

	//action with sub cmd
	{ "组队", TK_CMD },
	{ "离队", TK_CMD },
	{ "踢走", TK_CMD },
	{ "组队有", TK_CMD },
	{ "组队人数", TK_CMD },

	//move
	{ "坐标", TK_CMD },
	{ "座标", TK_CMD },
	{ "移动", TK_CMD },
	{ "封包移动", TK_CMD },
	{ "方向", TK_CMD },
	{ "最近坐标", TK_CMD },
	{ "寻路", TK_CMD },
	{ "移动至NPC", TK_CMD },
	{ "过点", TK_CMD },

	//mouse
	{ "左击", TK_CMD },
	{ "右击", TK_CMD },
	{ "左双击", TK_CMD },
	{ "拖至", TK_CMD },

#pragma endregion
	{ "callwithname", TK_CALLWITHNAME },
#pragma region en_US
	//keyword
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
	{ "var", TK_VARDECL },
	{ "local", TK_LOCAL },
	{ "delete", TK_VARFREE },
	{ "releaseall", TK_VARCLR },
	{ "format", TK_FORMAT },
	{ "rnd", TK_RND },
	{ "if", TK_IF },
	{ "for", TK_FOR },
	{ "break", TK_BREAK },
	{ "continue", TK_CONTINUE },

	//system
	{ "run", TK_CMD },
	{ "sleep", TK_CMD },
	{ "print", TK_CMD },
	{ "msg", TK_CMD },
	{ "logout", TK_CMD },
	{ "eo", TK_CMD },
	{ "logback", TK_CMD },
	{ "button", TK_CMD },
	{ "say", TK_CMD },
	{ "input", TK_CMD },
	{ "talk", TK_CMD },
	{ "cls", TK_CMD },
	{ "set", TK_CMD },
	{ "saveset", TK_CMD },
	{ "loadset", TK_CMD },
	{ "reg", TK_CMD },
	{ "timer", TK_CMD },
	{ "menu", TK_CMD },
	{ "dofile", TK_CMD },
	{ "createch", TK_CMD },
	{ "delch", TK_CMD },
	{ "send", TK_CMD },

	//check info
	{ "ifbattle", TK_CMD },
	{ "ifnormal", TK_CMD },
	{ "ifonline", TK_CMD },
	{ "ifpos", TK_CMD },
	{ "ifmap", TK_CMD },
	{ "ifitem", TK_CMD },
	{ "ifitemfull", TK_CMD },
	{ "ifpet", TK_CMD },
	{ "ifdaily", TK_CMD },

	{ "waitmap", TK_CMD },
	{ "waitdlg", TK_CMD },
	{ "waitsay", TK_CMD },
	{ "waititem", TK_CMD },
	{ "waitpet", TK_CMD },

	//actions
	{ "chplayername", TK_CMD },
	{ "usemagic", TK_CMD },
	{ "chpetname", TK_CMD },
	{ "chpet", TK_CMD },
	{ "doffpet", TK_CMD },
	{ "buy", TK_CMD },
	{ "sell", TK_CMD },
	{ "sellpet", TK_CMD },
	{ "useitem", TK_CMD },
	{ "doffitem", TK_CMD },
	{ "swapitem", TK_CMD },
	{ "pickup", TK_CMD },
	{ "put", TK_CMD },
	{ "get", TK_CMD },
	{ "putpet", TK_CMD },
	{ "getpet", TK_CMD },
	{ "save", TK_CMD },
	{ "load", TK_CMD },
	{ "make", TK_CMD },
	{ "cook", TK_CMD },
	{ "uequip", TK_CMD },
	{ "requip", TK_CMD },
	{ "wequip", TK_CMD },
	{ "puequip", TK_CMD },
	{ "pequip", TK_CMD },
	{ "skup", TK_CMD },
	{ "learn", TK_CMD },
	{ "trade", TK_CMD },
	{ "dostring", TK_CMD },
	{ "mail", TK_CMD },
	{ "doffstone", TK_CMD },

	//action with sub cmd
	{ "join", TK_CMD },
	{ "leave", TK_CMD },
	{ "kick", TK_CMD },
	{ "waitteam", TK_CMD },
	{ "ifteam", TK_CMD },

	//move
	{ "walkpos", TK_CMD },
	{ "move", TK_CMD },
	{ "w", TK_CMD },
	{ "dir", TK_CMD },
	{ "findpath", TK_CMD },
	{ "movetonpc", TK_CMD },
	{ "chmap", TK_CMD },
	{ "warp", TK_CMD },

	//mouse
	{ "lclick", TK_CMD },
	{ "rclick", TK_CMD },
	{ "ldbclick", TK_CMD },
	{ "dragto", TK_CMD },

	//hide
	{ "ocr", TK_CMD },
	{ "dlg", TK_CMD },
	{ "regex", TK_CMD },
	{ "rex", TK_CMD },
	{ "rexg", TK_CMD },
	{ "find", TK_CMD },
	{ "half", TK_CMD },
	{ "full", TK_CMD },
	{ "upper", TK_CMD },
	{ "lower", TK_CMD },
	{ "replace", TK_CMD },
	{ "todb", TK_CMD },

	//battle
	{ "bh", TK_CMD },//atk
	{ "bj", TK_CMD },//magic
	{ "bp", TK_CMD },//skill
	{ "bs", TK_CMD },//switch
	{ "be", TK_CMD },//escape
	{ "bd", TK_CMD },//defense
	{ "bi", TK_CMD },//item
	{ "bt", TK_CMD },//catch
	{ "bn", TK_CMD },//nothing
	{ "bw", TK_CMD },//petskill
	{ "bwf", TK_CMD },//pet nothing
	{ "bwait", TK_CMD },
	{ "bend", TK_CMD },
	#pragma endregion

	//... 其他後續增加的關鍵字
};

#ifdef TEST_LEXER
static const QHash<QString, RESERVE> keyHash = {
	{ "local", TK_LOCAL }, { "局", TK_LOCAL },

	{ "or", TK_OROR }, { "或", TK_OROR },
	{ "and", TK_ANDAND }, { "與", TK_ANDAND },

	{ "not", TK_NOT }, { "否", TK_ANDAND },

	{ "any", TK_ANY }, { "任意", TK_ANY },
	{ "double", TK_DOUBLE }, { "浮點數", TK_DOUBLE }, { "浮点数", TK_DOUBLE },
	{ "bool", TK_BOOL }, { "布爾值", TK_BOOL }, { "布尔值", TK_BOOL },
	{ "int", TK_INT }, { "整數", TK_INT }, { "整数", TK_INT },
	{ "string", TK_STRING }, { "字符串", TK_STRING },
	{ "table", TK_TABLE }, { "表", TK_TABLE },
	{ "nil", TK_NIL }, { "空", TK_NIL },

	{ "in", TK_IN },
	{ "do", TK_DO },
	{ "then", TK_THEN },

	{ "while", TK_THEN }, { "循環", TK_THEN }, { "循环", TK_THEN },

	{ "function", TK_FUNCTION }, { "功能", TK_FUNCTION },
	{ "end", TK_END }, { "結束", TK_END }, { "结束", TK_END },

	{ "repeat", TK_REPEAT }, { "重複", TK_REPEAT },
	{ "until", TK_UNTIL }, { "直到", TK_UNTIL },

	{ "if", TK_IF }, { "如果", TK_IF },
	{ "else", TK_ELSE }, { "否則", TK_ELSE }, { "否则", TK_ELSE },
	{ "elseif", TK_ELSEIF }, { "否則如果", TK_ELSEIF }, { "否则如果", TK_ELSEIF },

	{ "for", TK_FOR }, { "遍歷", TK_FOR }, { "遍历", TK_FOR },
	{ "function", TK_FUNCTION },

	{ "return", TK_RETURN }, { "返回", TK_RETURN },
	{ "break", TK_BREAK }, { "跳出", TK_BREAK },
	{ "continue", TK_CONTINUE }, { "繼續", TK_CONTINUE }, { "继续", TK_CONTINUE },

	{ "true", TK_TRUE }, { "真", TK_TRUE },
	{ "false", TK_FALSE }, { "假", TK_FALSE },

	{ "goto", TK_GOTO }, { "跳轉", TK_GOTO }, { "跳转", TK_GOTO },

	{ "jmp", TK_JMP }, { "跳至", TK_JMP },

	{ "label", TK_LABEL }, { "標記", TK_LABEL }, { "标记", TK_LABEL },
};
#endif
#pragma endregion

#pragma region  Tool
void Lexer::showError(const QString text, ErrorType type)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	if (type == kTypeError)
		emit signalDispatcher.appendScriptLog(QObject::tr("[error]") + text, 0);
	else
		emit signalDispatcher.appendScriptLog(QObject::tr("[warn]") + text, 0);
}

bool Lexer::isTable(const QString& str) const
{
	return str.startsWith("{") && str.endsWith("}") && !str.startsWith("{:");
}

bool Lexer::isDouble(const QString& str) const
{
	if (str.count('.') != 1)
		return false;

	qint64 size = str.size();
	for (qint64 i = 0; i < size; ++i)
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
	qint64 size = str.size();
	for (qint64 i = 0; i < size; ++i)
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

#pragma region TEST
#ifdef TEST_LEXER
#ifdef _DEBUG
void debugOutput(TokenMap* ptoken)
{
	for (auto it = ptoken->constBegin(); it != ptoken->constEnd(); ++it)
	{
		qint64 key = it.key();
		Token value = it.value();

		qDebug() << "No:" << key << "type:" << value.type << "value:" << value.data;
	}
}
#endif

//根據index取得當前字元
QChar Lexer::Reader::peek()
{
	qint64 index = nowIndex_;
	if (index < 0 || index >= nowTokenLength_)
	{
		return QChar();
	}

	return nowToken_.at(index);
}

//根據index取得下一個字元
QChar Lexer::Reader::next()
{
	qint64 index = nowIndex_;
	++index;
	if (index <= 0ll || index >= nowTokenLength_)
	{
		return QChar();
	}

	return nowToken_.at(index);
}

//根據index取得上一個字元
QChar Lexer::Reader::prev()
{
	qint64 index = nowIndex_;
	--index;
	if (index <= 0ll || index >= nowTokenLength_)
	{
		return QChar();
	}

	return nowToken_.at(index);
}

bool Lexer::Reader::checkNext(QChar ch)
{
	return next() == ch;
}

bool Lexer::Reader::checkPrev(QChar ch)
{
	return prev() == ch;
}

bool Lexer::tokenized(Lexer* pLexer, const QString& script)
{
	if (script.isEmpty())
		return false;

	if (pLexer == nullptr)
		return false;

	pLexer->clear();
	pLexer->reader_.clear();

	Injector& injector = Injector::getInstance();

	RESERVE controlReserve = TK_NONE;
	qint64 currentIndentLevel = 0;

	QStringList tokenStringList;
	pLexer->splitToStringToken(script, &tokenStringList);
	qDebug() << tokenStringList;
	RESERVE previous = TK_NONE;
	qint64 size = tokenStringList.size();
	qint64 i, j, n = 0ll;
	qint64 line = 0ll;
	QList<Token> tokenList;
	for (;;)
	{
		Token tk = pLexer->getNextToken(previous, tokenStringList);
		previous = tk.type;

		tokenList.append(tk);
		if (tokenStringList.isEmpty())
			break;
	}

	size = tokenList.size();

	for (i = 0; i < size; ++i)
	{
		Token prevTk;
		Token& nowTk = tokenList[i];
		Token nextTk;
		if (i > 0)
			prevTk = tokenList.at(i - 1);
		if (i < size - 1)
			nextTk = tokenList.at(i + 1);

		if (prevTk.type != TK_NEWLINE && prevTk.type != TK_COLON)
		{
			continue;
		}

		if (nowTk.type == TK_UNK && nextTk.type == TK_LPAREN)
			nowTk.type = TK_CALLNAME;

	}

	for (i = 0; i < size; ++i)
	{
		const Token nowTk = tokenList.at(i);
		if (nowTk.type == TK_NEWLINE)
		{
			++line;
			qDebug() << "[type]:" << nowTk.type << "[data]:" << nowTk.data;
			continue;
		}

		switch (nowTk.type)
		{
		case TK_LABEL:
		{
			pLexer->labelList_.insert(nowTk.data.toString(), line);
			break;
		}
		case TK_FUNCTIONNAME:
		{
			controlReserve = TK_FUNCTION;
			++currentIndentLevel;

			FunctionNode functionNode;
			functionNode.name = nowTk.data.toString();
			functionNode.beginLine = line;
			functionNode.endLine = -1; // 初始结束行号设为 -1
			functionNode.level = currentIndentLevel;
			functionNode.field = currentIndentLevel == 1 ? kGlobal : kLocal;

			functionNode.argList = {};

			bool isRef = false;
			RESERVE typeReserve = TK_ANY;
			n = 1;
			for (;;)
			{
				j = i + n;
				if (j >= size)
					break;

				Token& tmpTk = tokenList[j];
				if (tmpTk.type == TK_RPAREN)//新行或右括號結束
					break;

				do
				{
					if (tmpTk.type == TK_COMMA)
						break;

					if (tmpTk.type == TK_LPAREN)
						break;

					if (tmpTk.data.toString() == "&")
					{
						isRef = true;
						typeReserve = TK_REF;
						break;
					}


					if (keyHash.contains(tmpTk.data.toString()))
					{
						typeReserve = keyHash.value(tmpTk.data.toString());
						break;
					}

					if (isRef)
					{
						tmpTk.type = typeReserve;
						functionNode.argList.append(qMakePair(typeReserve, QString("&" + tmpTk.data.toString())));
						typeReserve = TK_ANY;
						isRef = false;
						break;
					}

					tmpTk.type = typeReserve;
					functionNode.argList.append(qMakePair(typeReserve, tmpTk.data));
					typeReserve = TK_ANY;
				} while (false);

				++n;
			}

			pLexer->functionNodeList_.append(functionNode);

			break;
		}
		case TK_FOR:
		{
			controlReserve = TK_FOR;
			++currentIndentLevel;

			ForNode forNode;
			forNode.beginLine = line;
			forNode.endLine = -1; // 初始结束行号设为 -1
			forNode.level = currentIndentLevel;
			forNode.field = currentIndentLevel == 1 ? kGlobal : kLocal;

			n = 1;
			for (;;)
			{
				j = i + n;
				if (j >= size)
					break;

				Token tmpTk = tokenList.at(j);
				if (tmpTk.data.toString() == "do")//取到 do 為止
					break;

				do
				{
					if (tmpTk.type == TK_COMMA)
						break;

					if (tmpTk.type == TK_LPAREN)
						break;

					if (forNode.varName.isEmpty() && tmpTk.data.type() == QVariant::String && tmpTk.data.toString() != "do")
					{
						forNode.varName = tmpTk.data.toString();
						break;
					}

					if (tmpTk.type == TK_ASSIGN)
						break;

					if (!forNode.beginValue.isValid() && tmpTk.data.toString() != "do")
					{
						forNode.beginValue = tmpTk.data;
						break;
					}

					if ((!forNode.endValue.isValid() || forNode.endValue == forNode.beginValue) && tmpTk.data.toString() != "do")
					{
						forNode.endValue = tmpTk.data;
						break;
					}

					if ((!forNode.stepValue.isValid() || forNode.stepValue.toLongLong() == 0) && tmpTk.data.toString() != "do")
					{
						forNode.stepValue = tmpTk.data;
						break;
					}

				} while (false);

				++n;
			}

			pLexer->forNodeList_.append(forNode);

			break;
		}
		case TK_RETURN:
		{
			//QStringList returnTypes;

			//for (qint64 i = 1; i < tk.size(); ++i)
			//{
			//	Token token = tk.value(i);
			//	if (token.type == TK_COMMENT)
			//		continue;
			//	returnTypes.append(token.data.toString());
			//}

			//if (!pLexer->functionNodeList_.isEmpty())
			//{
			//	pLexer->functionNodeList_.last().returnTypes.insert(i, returnTypes);
			//}
			break;
		}
		case TK_END:
		{
			if (controlReserve == TK_FUNCTION)
			{
				if (!pLexer->functionNodeList_.isEmpty())
				{
					pLexer->functionNodeList_.last().endLine = line;
					qDebug() << "function:" << pLexer->functionNodeList_.last().name << pLexer->functionNodeList_.last().argList;
				}
			}
			else if (controlReserve == TK_FOR)
			{
				if (!pLexer->forNodeList_.isEmpty())
				{
					pLexer->forNodeList_.last().endLine = line;
					qDebug() << "for:" << pLexer->forNodeList_.last().varName << pLexer->forNodeList_.last().beginValue << pLexer->forNodeList_.last().endValue << pLexer->forNodeList_.last().stepValue;
				}
			}
			else if (controlReserve == TK_WHILE)
			{

			}
			else if (controlReserve == TK_REPEAT)
			{

			}

			controlReserve = TK_END;
			--currentIndentLevel;
			break;
		}
		default:
			break;
		}

		qDebug() << "[type]:" << nowTk.type << "[data]:" << nowTk.data;
	}

	QStringList content;
	for (const Token& it : tokenList)
	{
		content.append(it.raw);
	}

	QString c = content.join(" ");

	qDebug() << c;

#if 0
	for (qint64 i = 0; i < size; ++i)
	{
		TokenMap tk;
		//單獨處理每一行的TOKEN
		if (pLexer->useNewLexer)
			pLexer->tokenizedNew(lines.at(i), &tk);
		else
			pLexer->tokenized(i, lines.at(i), &tk, &pLexer->labelList_);
		tokens.insert(i, tk);

		if (!pLexer->useNewLexer)
			continue;

		RESERVE reserve = tk.value(0).type;

		switch (reserve)
		{
		case TK_LABEL:
		{
			pLexer->labelList_.insert(tk.value(0).data.toString(), i);
			break;
		}
		case TK_FUNCTION:
		{
			controlReserve = TK_FUNCTION;
			++currentIndentLevel;

			FunctionNode functionNode;
			functionNode.name = tk.value(1).data.toString();
			functionNode.beginLine = i;
			functionNode.endLine = -1; // 初始结束行号设为 -1
			functionNode.level = currentIndentLevel;
			functionNode.field = currentIndentLevel == 1 ? kGlobal : kLocal;

			functionNode.argList = tk.value(2).data.value<VarPairList>();

			pLexer->functionNodeList_.append(functionNode);

			break;
		}
		case TK_FOR:
		{
			controlReserve = TK_FOR;
			++currentIndentLevel;

			ForNode forNode;
			forNode.beginLine = i;
			forNode.endLine = -1; // 初始结束行号设为 -1
			forNode.level = currentIndentLevel;
			forNode.field = currentIndentLevel == 1 ? kGlobal : kLocal;

			forNode.varName = tk.value(1).data.toString();
			//----------------------------2 是 = 符號
			forNode.beginValue = tk.value(3).data.toLongLong();
			forNode.endValue = tk.value(4).data.toLongLong();

			pLexer->forNodeList_.append(forNode);

			break;
		}
		case TK_WHILE:
		{
			controlReserve = TK_WHILE;
			++currentIndentLevel;
			break;
		}
		case TK_REPEAT:
		{
			controlReserve = TK_REPEAT;
			++currentIndentLevel;
			break;
		}
		case TK_RETURN:
		{
			QStringList returnTypes;

			for (qint64 i = 1; i < tk.size(); ++i)
			{
				Token token = tk.value(i);
				if (token.type == TK_COMMENT)
					continue;
				returnTypes.append(token.data.toString());
			}

			if (!pLexer->functionNodeList_.isEmpty())
			{
				pLexer->functionNodeList_.last().returnTypes.insert(i, returnTypes);
			}
			break;
		}
		case TK_END:
		{
			if (controlReserve == TK_FUNCTION)
			{
				if (!pLexer->functionNodeList_.isEmpty())
				{
					pLexer->functionNodeList_.last().endLine = i;
				}
			}
			else if (controlReserve == TK_FOR)
			{
				if (!pLexer->forNodeList_.isEmpty())
				{
					pLexer->forNodeList_.last().endLine = i;
				}
			}
			else if (controlReserve == TK_WHILE)
			{

			}
			else if (controlReserve == TK_REPEAT)
			{

			}

			controlReserve = TK_END;
			--currentIndentLevel;
			break;
		}
		default:
			break;
		}
	}

	pLexer->tokens_ = tokens;
#endif
	return true;
}

bool Lexer::splitToStringToken(QString src, QStringList* pTokenStringList)
{
	//markers
	qint64 sQuote = 0i64; // 是否在單引號內
	qint64 dQuote = 0i64; // 是否在雙引號內
	qint64 waitforCommantEnd = 0i64; // 是否在註解內
	qint64 waitforBlockCommantEnd = 0i64; // 是否在區塊註解內

	QStringList tokenStringList;

	reader_.resetIndex();
	reader_.resetToken(src);

	QChar u = 0ui16;
	QChar ch = 0ui16;

	qint64 line = 0;

	for (;;)
	{
		u = reader_.peek();

		switch (u.unicode())
		{
		case L'\r':
		{
			if (!waitforBlockCommantEnd)
				reader_.takeStored();
			else
				reader_.store(u);
			break;
		}
		case L'\n':
		{
			if (sQuote > 0 || dQuote > 0)
				break;

			if (waitforCommantEnd)
			{
				waitforCommantEnd = false;
				reader_.takeStored();
			}

			if (!waitforBlockCommantEnd)
			{
				reader_.takeStored();
				reader_.store(u);
				reader_.takeStored();
				++line;
			}
			else
				reader_.store(u);

			break;
		}
		case L'\\':
		{
			if (sQuote == 0 && dQuote == 0)
			{
				showError(QObject::tr("[error] @ %1 | Escape character is not allowed outside of string literal.").arg(line));
				return RTK_EOF;
			}

			reader_.store(u);
			break;
		}
		case L'\'':
		case L'"':
		{
			if (waitforCommantEnd || waitforBlockCommantEnd)
			{
				reader_.store(u);
				break;
			}

			if (sQuote > 0 || dQuote > 0)
			{
				sQuote = 0;
				dQuote = 0;

				reader_.store(u);
				reader_.takeStored();
				break;
			}

			reader_.takeStored();
			++dQuote;
			++sQuote;
			reader_.store(u);

			break;
		}
		case L'*': // *, *=
		{
			if (waitforCommantEnd)
			{
				reader_.store(u);
				break;
			}

			if (sQuote > 0 || dQuote > 0)
			{
				reader_.store(u);
				break;
			}

			ch = reader_.next();
			if (ch == L'/')
			{
				if (waitforBlockCommantEnd)
				{
					waitforBlockCommantEnd = false;
					reader_.store(u);
					reader_.movenext();
					reader_.store(reader_.peek());
					reader_.takeStored();
				}
			}

			if (waitforBlockCommantEnd)
			{
				reader_.store(u);
				break;
			}

			reader_.takeStored();
			reader_.store(u);
			reader_.takeStored();
			break;
		}
		case L'/': // /, /=
		{
			if (sQuote > 0 || dQuote > 0)
			{
				reader_.store(u);
				break;
			}

			ch = reader_.next();
			if (ch == L'/' && !waitforCommantEnd) // //, /*
			{
				waitforCommantEnd = true;
				reader_.takeStored();
				reader_.store(u);
				reader_.movenext();
				reader_.store(reader_.peek());
				break;
			}
			else if (ch == L'*' && !waitforBlockCommantEnd) // //, /*
			{
				waitforBlockCommantEnd = true;
				reader_.takeStored();
				reader_.store(u);
				reader_.movenext();
				reader_.store(reader_.peek());
				break;
			}

			if (waitforCommantEnd || waitforBlockCommantEnd)
			{
				reader_.store(u);
				break;
			}

			reader_.takeStored();
			reader_.store(u);
			reader_.takeStored();
			break;
		}
		case L' ':
		{
			if (waitforCommantEnd || waitforBlockCommantEnd)
			{
				reader_.store(u);
				break;
			}

			if (sQuote > 0 || dQuote > 0)
			{
				reader_.store(u);
				break;
			}

			reader_.takeStored();
			break;
		}
		case L'(':
		case L')':
		case L'}':
		case L'{':
		case L'[':
		case L']':
		case L',':
		case L'#': // #
		case L'?': // ?:
		case L':': // :
		case L';':
		case L'+':
		case L'-':
		case L'%':
		case L'&':
		case L'^':
		case L'<':
		case L'>':
		case L'=':
		case L'~':
		case L'_':
		case L'.':
		{
			if (waitforCommantEnd || waitforBlockCommantEnd)
			{
				reader_.store(u);
				break;
			}

			if (sQuote > 0 || dQuote > 0)
			{
				reader_.store(u);
				break;
			}

			reader_.takeStored();
			reader_.store(u);
			reader_.takeStored();
			break;
		}
		case L'\0':
		{
			if (pTokenStringList != nullptr)
				*pTokenStringList = reader_.getList();
			return true;
		}
		default:
			reader_.store(u);
			break;
		}

		reader_.movenext();
	}

	return false;
}

bool Lexer::checkOperator(RESERVE previous, QString& tokenStr, RESERVE* pReserve)
{
	if (tokenStr.isEmpty())
		return false;

	if (pReserve == nullptr)
		return false;

	QChar u = tokenStr.front();
	QChar ch = L'\0';
	bool bret = true;

	reader_.resetIndex();
	reader_.resetToken(tokenStr);

	switch (u.unicode())
	{
	case L'+': // +, ++, +=
	{
		ch = reader_.next();
		if (ch == L'+') // ++
			*pReserve = TK_INC;
		else if (ch == L'=') // +=
			*pReserve = TK_ADD_ASSIGN;
		else // +
		{
			if (previous == TK_CSTRING)
				*pReserve = TK_DOTDOT;
			else
				*pReserve = TK_ADD;
		}

		break;
	}
	case L'-': //  -, --, -=
	{
		ch = reader_.next();
		if (ch == L'-') // --
			*pReserve = TK_DEC;
		else if (ch == L'=') // -=
			*pReserve = TK_SUB_ASSIGN;
		else // -
			*pReserve = TK_SUB;

		break;
	}
	case L'*': // *, *=
	{
		ch = reader_.next();
		if (ch == L'=') // *=
			*pReserve = TK_MUL_ASSIGN;
		else if (ch == L'/') // */
			*pReserve = TK_COMMENT;
		else // *
			*pReserve = TK_MUL;

		break;
	}
	case L'/': // /, /=
	{
		ch = reader_.next();
		if (ch == L'=') // /=
			*pReserve = TK_DIV_ASSIGN;
		else if (ch == L'/' || ch == '*') // //, /*
			*pReserve = TK_COMMENT;
		else // /
			*pReserve = TK_DIV;

		break;
	}
	case L'%': // %, %=
	{
		ch = reader_.next();
		if (ch == L'=') // %=
			*pReserve = TK_MOD_ASSIGN;
		else // %
			*pReserve = TK_MOD;

		break;
	}
	case L'!': // !, !=
	{
		ch = reader_.next();
		if (ch == L'=')
			*pReserve = TK_NEQ;
		else
			*pReserve = TK_NOT;

		break;
	}
	case L'=': // =, ==
	{
		ch = reader_.next();
		if (ch == L'=') // ==
			*pReserve = TK_EQ;
		else // =
			*pReserve = TK_ASSIGN;

		break;
	}
	case L'>': // >, >=, >>, >>=
	{
		ch = reader_.next();
		if (ch == L'=') // >=
			*pReserve = TK_GEQ;
		else if (ch == L'>') // >>
		{
			ch = reader_.next();
			if (ch == L'=') // >>=
				*pReserve = TK_SHR_ASSIGN;
			else
				*pReserve = TK_SHR;
		}
		else
			*pReserve = TK_GT;

		break;
	}
	case L'<': // <, <=, <<, <<=
	{
		ch = reader_.next();
		if (ch == L'=') // <=
			*pReserve = TK_LEQ;
		else if (ch == L'<') // <<
		{
			ch = reader_.next();
			if (ch == L'=') // <<=
				*pReserve = TK_SHL_ASSIGN;
			else
				*pReserve = TK_SHL;
		}
		else
			*pReserve = TK_LT;

		break;
	}
	case L'&': // &, &&, &=
	{
		ch = reader_.next();
		if (ch == L'=') // &=
			*pReserve = TK_AND_ASSIGN;
		else if (ch == L'&') // &&
			*pReserve = TK_ANDAND;
		else if (ch != L'\0' && ch != L',') // &xxxx
			*pReserve = TK_REF;
		else // &
			*pReserve = TK_AND;

		break;
	}
	case L'|': // |, |=
	{
		ch = reader_.next();
		if (ch == L'=') // |=
			*pReserve = TK_OR_ASSIGN;
		else if (ch == L'|') // ||
			*pReserve = TK_OROR;
		else // |
			*pReserve = TK_OR;

		break;
	}
	case L'^': // ^, ^=
	{
		ch = reader_.next();
		if (ch == L'=')
			*pReserve = TK_XOR_ASSIGN;
		else
			*pReserve = TK_XOR;

		break;
	}
	case L'~': // ~, ~=
	{
		ch = reader_.next();
		if (ch == L'=') // ~=
			*pReserve = TK_NEQ;
		else // ~
			*pReserve = TK_NOT;

		break;
	}
	case L'?': // ?:
	{
		*pReserve = TK_FUZZY;
		break;
	}
	case L':': // :
	{
		*pReserve = TK_COLON;
		break;
	}
	case L'.': // ., ..
	{
		ch = reader_.next();
		if (ch == L'.')
			*pReserve = TK_DOTDOT;
		else
			*pReserve = TK_DOT;
		break;
	}
	case L'#': // #
	{
		*pReserve = TK_SHARP;
		break;
	}
	case L',':
	{
		*pReserve = TK_COMMA;
		break;
	}
	case L'(':
	{
		if (previous == TK_FUNCTIONNAME)
			beginFunctionArgsDeclaration_ = true;

		if (previous == TK_FOR)
			beginForArgs_ = true;

		*pReserve = TK_LPAREN;
		break;
	}
	case L')':
	{
		if (beginFunctionArgsDeclaration_)
		{
			beginFunctionArgsDeclaration_ = false;
			functionArgList_.clear();
		}

		if (beginForArgs_)
			beginForArgs_ = false;

		*pReserve = TK_RPAREN;
		break;
	}
	case L'[':
	{
		*pReserve = TK_LBRACKET;
		break;
	}
	case L']':
	{
		*pReserve = TK_RBRACKET;
		break;
	}
	case L'{':
	{
		*pReserve = TK_LBRACE;
		break;
	}
	case L'}':
	{
		*pReserve = TK_RBRACE;
		break;
	}
	default:
		bret = false;
		break;
	}

	reader_.resetIndex();
	return  bret;
}

Token Lexer::getNextToken(RESERVE previous, QStringList& refTokenStringList)
{
	if (refTokenStringList.isEmpty())
		return Token();

	Token token;
	QString front = refTokenStringList.front();
	refTokenStringList.pop_front();

	if (keyHash.contains(front))
	{
		return Token{ keyHash.value(front), front, front };
	}

	//標記名稱
	if (previous == TK_LABEL)
	{
		return Token{ TK_LABELNAME, front, front };
	}

	//跳轉目標標記
	if (previous == TK_GOTO)
	{
		return  Token{ TK_LABELNAME, front, front };
	}

	RESERVE opReserve = TK_UNK;
	if (checkOperator(previous, front, &opReserve))
	{
		return Token{ opReserve, front, front };
	}

	//函數名稱
	if (previous == TK_FUNCTION)
	{
		beginFunctionDeclaration_ = false;
		beginFunctionNameDeclaration_ = true;
		return Token{ TK_FUNCTIONNAME, front, front };
	}

	//函數傳參
	if (beginFunctionNameDeclaration_ || beginFunctionArgsDeclaration_)
	{
		beginFunctionNameDeclaration_ = false;
		beginFunctionArgsDeclaration_ = true;

		token = Token{ TK_FUNCTIONARG, front, front };
		functionArgList_.append(token);
		return token;
	}

	//返回值
	if (previous == TK_RETURN)
	{
		return Token{ TK_RETURNVALUE, front, front };
	}

	//更多返回值
	if (previous == TK_RETURNVALUE)
	{
		return Token{ TK_RETURNVALUE, front, front };
	}

	if (beginForArgs_)
	{
		bool ok = false;
		qint64 value = front.toLongLong(&ok);
		if (ok && !front.startsWith("'") && !front.startsWith("\""))
			token = Token{ TK_FORARG, value, front };
		else
			token = Token{ TK_FORARG, front, front };
		forArgList_.append(token);
		return token;
	}

	if (isConstString(front))
	{
		return Token{ TK_STRINGVALUE, front, front };
	}

	if (isBool(front))
	{
		QVariant value = front == "true" || front == "真" ? true : false;
		return Token{ TK_BOOLVALUE, value, front };
	}

	if (isDouble(front))
	{
		QVariant value = front.toDouble();
		return Token{ TK_DOUBLEVALUE, value, front };
	}

	if (isInteger(front))
	{
		QVariant value = front.toLongLong();
		return Token{ TK_INTVALUE, value, front };
	}

	if (front == "nil")
	{
		return Token{ TK_NIL, QVariant(), front };
	}

	if (front == "\n")
	{
		return Token{ TK_NEWLINE, front, front };
	}

	return Token{ TK_UNK, front, front };
}


//將lua table轉換成字串
QString Lexer::getLuaTableString(const sol::table& t, int& depth)
{
	if (depth <= 0)
		return "";

	--depth;

	QString ret = "{\n";

	QStringList results;
	QStringList strKeyResults;

	QString nowIndent = "";
	for (int i = 0; i <= 10 - depth + 1; ++i)
	{
		nowIndent += "    ";
	}

	for (const auto& pair : t)
	{
		qint64 nKey = 0;
		QString key = "";
		QString value = "";

		if (pair.first.is<qint64>())
		{
			nKey = pair.first.as<qint64>() - 1;
		}
		else
			key = QString::fromUtf8(pair.first.as<std::string>().c_str());

		if (pair.second.is<sol::table>())
			value = getLuaTableString(pair.second.as<sol::table>(), depth);
		else if (pair.second.is<std::string>())
			value = QString("'%1'").arg(QString::fromUtf8(pair.second.as<std::string>().c_str()));
		else if (pair.second.is<qint64>())
			value = QString::number(pair.second.as<qint64>());
		else if (pair.second.is<double>())
			value = QString::number(pair.second.as<double>(), 'f', 16);
		else if (pair.second.is<bool>())
			value = pair.second.as<bool>() ? "true" : "false";
		else
			value = "nil";

		if (key.isEmpty())
		{
			if (nKey >= results.size())
			{
				for (qint64 i = results.size(); i <= nKey; ++i)
					results.append("nil");
			}

			results[nKey] = nowIndent + value;
		}
		else
			strKeyResults.append(nowIndent + QString("%1 = %2").arg(key).arg(value));
	}

	std::sort(strKeyResults.begin(), strKeyResults.end(), [](const QString& a, const QString& b)
		{
			static const QLocale locale;
			static const QCollator collator(locale);
			return collator.compare(a, b) < 0;
		});

	results.append(strKeyResults);

	ret += results.join(",\n");
	ret += "\n}";

	return ret;
}

//將字符串解析成lua table
sol::object Lexer::getLuaTableFromString(const QString& str)
{
	const QString expr(QString("return %1;").arg(str));
	const std::string exprStrUTF8 = expr.toUtf8().constData();
	sol::protected_function_result loaded_chunk = lua_.safe_script(exprStrUTF8.c_str(), sol::script_pass_on_error);
	lua_.collect_garbage();

	if (!loaded_chunk.valid())
	{
		sol::error err = loaded_chunk;
		qDebug() << err.what();
		return sol::lua_nil;
	}

	sol::object retObject;
	try
	{
		retObject = loaded_chunk;
	}
	catch (...)
	{
		return sol::lua_nil;
	}

	if (retObject.is<sol::table>())
		return retObject;

	return sol::lua_nil;
}

#endif
#pragma endregion

#pragma region OLD

//解析整個腳本至多個TOKEN
bool Lexer::tokenized(Lexer* pLexer, const QString& script)
{
	Injector& injector = Injector::getInstance();
	pLexer->clear();

	QStringList lines = script.split("\n");
	qint64 size = lines.size();
	for (qint64 i = 0; i < size; ++i)
	{
		TokenMap tk;
		pLexer->tokenized(i, lines.at(i), &tk, &pLexer->labelList_);
		pLexer->tokens_.insert(i, tk);
	}

	pLexer->recordNode();

	pLexer->checkFunctionPairs(pLexer->tokens_);

	return true;
}

//解析單行內容至多個TOKEN
void Lexer::tokenized(qint64 currentLine, const QString& line, TokenMap* ptoken, QHash<QString, qint64>* plabel)
{
	if (ptoken == nullptr || plabel == nullptr)
		return;

	qint64 pos = 0;
	QString token;
	QVariant data;
	QString raw = line.trimmed();
	QString originalRaw = raw;
	RESERVE type = TK_UNK;

	ptoken->clear();

	do
	{
		qint64 commentIndex = raw.indexOf("//");
		if (commentIndex == -1)
			commentIndex = raw.indexOf("/*");

		//處理整行註釋
		if (commentIndex == 0 && (raw.trimmed().indexOf("//") == 0 || raw.trimmed().indexOf("/*") == 0))
		{
			createToken(pos, TK_COMMENT, raw, raw, ptoken);
			break;
		}
		//當前token移除註釋
		else if (commentIndex > 0)
			raw = raw.mid(0, commentIndex).trimmed();

		bool doNotLowerCase = false;
		//a,b,c = 1,2,3
		static const QRegularExpression rexMultiVar(R"(^\s*([_a-zA-Z\p{Han}][\w\p{Han}]*(?:\s*,\s*[_a-zA-Z\p{Han}][\w\p{Han}]*)*)\s*=\s*([^,]+(?:\s*,\s*[^,]+)*)$\;*)");
		//local a,b,c = 1,2,3
		static const QRegularExpression rexMultiLocalVar(R"(^\s*[lL][oO][cC][aA][lL]\s+([_a-zA-Z\p{Han}][\w\p{Han}]*(?:\s*,\s*[_a-zA-Z\p{Han}][\w\p{Han}]*)*)\s*=\s*([^,]+(?:\s*,\s*[^,]+)*)$\;*)");
		//var++, var--
		static const QRegularExpression varIncDec(R"(^\s*([\p{Han}\w]+)(\+\+|--)\;*)");
		//+= -= *= /= &= |= ^= %=
		static const QRegularExpression varCAOs(R"(^\s*(?!\d)([\p{Han}\w]+)\s+([+\-*\/&|^%]\=)\s+([\W\w\s\p{Han}]+)\;*)");
		//x = expr
		static const QRegularExpression varExpr(R"(^\s*([\w\p{Han}]+)\s*\=\s*([\W\w\s\p{Han}]+)\;*)");
		//+ - * / % & | ^ ( )
		static const QRegularExpression varAnyOp(R"([+\-*\/%&|^\(\)])");
		//if expr op expr
		static const QRegularExpression varIf(R"(^\s*[iI][fF][\s*\(\s*|\s+]([\w\W\p{Han}]+\s*[<|>|\=|!][\=]*\s*[\w\W\p{Han}]+)\s*,\s*([\w\W\p{Han}]+))");
		//if expr
		static const QRegularExpression varIf2(R"(^\s*[iI][fF][\s*\(\s*|\s+]\s*([\w\W\p{Han}]+)\s*,\s*([\w\W\p{Han}]+))");
		//function name()
		static const QRegularExpression rexFunction(R"(^\s*[fF][uU][nN][cC][tT][iI][oO][nN]\s+([\w\p{Han}]+)\s*\(([\w\W\p{Han}]*)\))");
		//xxx()
		static const QRegularExpression rexCallFunction(R"(^\s*^([\w\p{Han}]+)\s*\(([\w\W\p{Han}]*)\)$\;*)");
		//for i = 1, 10 or for i = 1, 10, 2 or for (i = 1, 10) or for (i = 1, 10, 2)
		static const QRegularExpression rexCallFor(R"(^\s*[fF][oO][rR]\s*\(*([\w\p{Han}]+)\s*=\s*([^,]+)\s*,\s*([^,]+)\s*[\)]*)");
		static const QRegularExpression rexCallForForever(R"(^\s*[fF][oO][rR]\s*\(*\s*,\s*,\s*\)*\s*[do]*)");
		static const QRegularExpression rexCallForWithStep(R"(^\s*[fF][oO][rR]\s*\(*([\w\p{Han}]+)\s*=\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^,\)]+)\s*[\)]*)");
		static const QRegularExpression rexTable(R"(^\s*([\w\p{Han}]+)\s*=\s*(\{[\s\S]*\})\;*)");
		static const QRegularExpression rexLocalTable(R"(^\s*[lL][oO][cC][aA][lL]\s+([\w\p{Han}]+)\s*=\s*(\{[\s\S]*\})\;*)");
		//處理if
		if (raw.startsWith("if") && !raw.startsWith("ifitem"))
		{
			if (raw.startsWith("ifpet"))
			{
				raw.replace("ifpet", "if pet.count");
			}
			else if (raw.startsWith("ifteam"))
			{
				raw.replace("ifteam", "if team.count");
			}

			QRegularExpressionMatch match = varIf.match(raw);
			if (!match.hasMatch())
				match = varIf2.match(raw);

			if (match.hasMatch())
			{
				QString cmd = "if";
				QString expr = match.captured(1).simplified();//表達式
				QString jmpExpr = match.captured(2).simplified();//跳轉表達式
				createToken(pos, TK_IF, cmd, cmd, ptoken);
				createToken(pos + 1, TK_EXPR, expr, expr, ptoken);
				if (isInteger(jmpExpr))
					createToken(pos + 2, TK_INT, jmpExpr.toLongLong(), jmpExpr, ptoken);
				else
					createToken(pos + 2, TK_STRING, jmpExpr, jmpExpr, ptoken);
				break;
			}
		}
		//處理for 3參
		if (raw.contains(rexCallForWithStep))
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
		else if (raw.contains(varCAOs) && !raw.front().isDigit())
		{
			QRegularExpressionMatch match = varCAOs.match(raw);
			if (match.hasMatch())
			{
				QString notuse;
				QString varName = match.captured(1).simplified();
				QString op = match.captured(2).simplified();
				QString value = match.captured(3).simplified();
				qint64 p = pos + 1;
				RESERVE optype = getTokenType(p, TK_CAOS, op, op);
				++p;
				RESERVE valuetype = getTokenType(p, optype, value, value);

				createToken(pos, TK_CAOS, varName, varName, ptoken);
				createToken(pos + 1, optype, op, op, ptoken);
				createToken(pos + 2, valuetype, value, value, ptoken);
			}
			break;
		}
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
				for (int i = 1; i <= 3; ++i)
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
					token = "callwithname";
				}
			}
			else
			{
				//以空格為分界分離出第一個TOKEN(命令)
				if (!getStringCommandToken(raw, " ", token))
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
				static const QRegularExpression rexCallLuaFunction(R"(^\s*([\w\p{Han}]+\.?[\w\p{Han}]*)\s*\([\w\W]+\)\s*;*\s*[\W\w\p{Han}]*;*)");
				static const QRegularExpression rexTableSet(R"(^\s*([\w\p{Han}]+)[^\(](?:\['*"*(\w+\p{Han}*)'*"*\])?)");
				if (token.contains(rexCallLuaFunction))
				{
					QRegularExpressionMatch match = rexCallLuaFunction.match(token);
					if (match.hasMatch())
					{
						QString varName = match.captured(1).simplified();
						createToken(pos, TK_CALLLUA, varName, varName, ptoken);
						createToken(pos + 1, TK_STRING, originalRaw, originalRaw, ptoken);
						break;
					}
				}
				else if (token.contains(rexTableSet))
				{
					QRegularExpressionMatch match = rexTableSet.match(token);
					if (match.hasMatch())
					{
						QString varName = match.captured(1).simplified();
						createToken(pos, TK_TABLESET, varName, varName, ptoken);
						createToken(pos + 1, TK_STRING, originalRaw, originalRaw, ptoken);
						break;
					}
				}

				showError(QObject::tr("@ %1 | Unknown command '%2' has been ignored").arg(currentLine + 1).arg(token), kTypeWarning);
				createEmptyToken(pos, ptoken);
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
				if (getStringCommandToken(raw, ",", token) == RTK_EOF)
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
				qint64 intValue = token.toLongLong(&ok);
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
	QHash<qint64, FunctionNode> chunkHash;
	QMap<qint64, TokenMap> map;
	for (auto it = tokens_.cbegin(); it != tokens_.cend(); ++it)
		map.insert(it.key(), it.value());

	//這裡是為了避免沒有透過call調用函數的情況
	QStack<RESERVE> reserveStack;
	QStack<FunctionNode> functionNodeStack;
	QStack<ForNode> forNodeStack;

	QList<Node> extraEndNodeList;
	qint64 currentIndentLevel = 0;
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		qint64 row = it.key();
		TokenMap tk = it.value();

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

			for (qint64 i = 1; i < tk.size(); ++i)
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
	for (auto it = map.cbegin(); it != map.cend(); ++it)
	{
		qint64 row = it.key();
		TokenMap tk = it.value();

		switch (tk.value(0).type)
		{
		case TK_CONTINUE:
		case TK_BREAK:
		{
			bool ok = false;

			for (auto subit = forNodes.constBegin(); subit != forNodes.constEnd(); ++subit)
			{
				ForNode node = *subit;
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
			for (int i = tk.size() - 1; i >= 0; --i)
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

				for (auto subit = forNodes.constBegin(); subit != forNodes.constEnd(); ++subit)
				{
					ForNode node = *subit;
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

				for (auto subit = functionNodes.constBegin(); subit != functionNodes.constEnd(); ++subit)
				{
					FunctionNode node = *subit;
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
void Lexer::createToken(qint64 index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken)
{
	if (ptoken != nullptr)
		ptoken->insert(index, Token{ type, data, raw });
}

//插入新TOKEN到index位置，將原本的TOKEN後移
void Lexer::insertToken(qint64 index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken)
{
	TokenMap tokenMap;
	tokenMap.insert(index, Token{ type, data, raw });

	for (auto it = ptoken->constBegin(); it != ptoken->constEnd(); ++it)
	{
		qint64 key = it.key();
		if (key + 1 <= index)
			continue;

		Token token = it.value();
		tokenMap.insert(key + 1, token);
	}

	*ptoken = tokenMap;
};

//插入空行TOKEN
void Lexer::createEmptyToken(qint64 index, TokenMap* ptoken)
{
	ptoken->insert(index, Token{ TK_WHITESPACE, "", "" });
}

//根據容取TOKEN應該定義的類型
RESERVE Lexer::getTokenType(qint64& pos, RESERVE previous, QString& str, const QString raw) const
{
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
void Lexer::checkPairs(const QString& beginstr, const QString& endstr, const QHash<qint64, TokenMap>& stokenmaps)
{
	QMap<qint64, QString> unpairedFunctions;
	QMap<qint64, QString> unpairedEnds;
	QStack<qint64> functionStack;

	QMap <qint64, TokenMap> tokenmaps;//轉成有序容器方便按順序遍歷
	for (auto it = stokenmaps.cbegin(); it != stokenmaps.cend(); ++it)
		tokenmaps.insert(it.key(), it.value());

	for (auto it = tokenmaps.cbegin(); it != tokenmaps.cend(); ++it)
	{
		qint64 row = it.key();
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
		qint64 row = functionStack.pop();
		QString statement = tokenmaps.value(row).cbegin().value().data.toString().simplified();
		unpairedFunctions.insert(row, statement);
	}

	// 打印所有不成對的 "function" 語句
	for (auto it = unpairedFunctions.cbegin(); it != unpairedFunctions.cend(); ++it)
	{
		qint64 row = it.key();
		QString statement = it.value();
		QString errorMessage = QObject::tr("@ %1 | Missing '%2' for statement '%3'").arg(row + 1).arg(endstr).arg(statement);
		showError(errorMessage, kTypeWarning);
	}

	// 打印所有不成對的 "end" 語句
	for (auto it = unpairedEnds.cbegin(); it != unpairedEnds.cend(); ++it)
	{
		qint64 row = it.key();
		QString statement = it.value();
		QString errorMessage = QObject::tr("@ %1 | Extra '%2' for statement '%3'").arg(row + 1).arg(endstr).arg(statement);
		showError(errorMessage, kTypeWarning);
	}
}

//檢查單行字符配對
void Lexer::checkSingleRowPairs(const QString& beginstr, const QString& endstr, const QHash<qint64, TokenMap>& stokenmaps)
{
	QMap<qint64, QVector<int>> unpairedFunctions; // <Row, Vector of unpaired start indices>
	QMap<qint64, QVector<int>> unpairedEnds;      // <Row, Vector of unpaired end indices>

	QMap<qint64, TokenMap> tokenmaps;
	for (auto it = stokenmaps.cbegin(); it != stokenmaps.cend(); ++it)
		tokenmaps.insert(it.key(), it.value());

	for (auto it = tokenmaps.cbegin(); it != tokenmaps.cend(); ++it)
	{
		qint64 row = it.key();
		QStringList tmp;
		for (auto it2 = it.value().cbegin(); it2 != it.value().cend(); ++it2)
			tmp.append(it2.value().data.toString().simplified());

		QString statement = tmp.join(" ");



		QVector<int> startIndices;
		QVector<int> endIndices;

		for (int index = 0; index < statement.length(); ++index)
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
		qint64 row = it.key();
		QVector<int> unpairedIndices = it.value();

		for (int index : unpairedIndices)
		{
			QString statement = tokenmaps[row].value(0).data.toString().simplified();
			QString errorMessage = QString(QObject::tr("@ %1 | Unpaired '%2' index %3: '%4'")).arg(row + 1).arg(beginstr).arg(index).arg(statement);
			showError(errorMessage, kTypeWarning);
		}
	}

	// 打印所有不成對的 endstr 語句
	for (auto it = unpairedEnds.cbegin(); it != unpairedEnds.cend(); ++it)
	{
		qint64 row = it.key();
		QVector<int> unpairedIndices = it.value();

		for (int index : unpairedIndices)
		{
			QString statement = tokenmaps[row].value(0).data.toString().simplified();
			QString errorMessage = QString(QObject::tr("@ %1 | Unpaired '%2' index %3: '%4'")).arg(row + 1).arg(endstr).arg(index).arg(statement);
			showError(errorMessage, kTypeWarning);
		}
	}
}

void Lexer::checkFunctionPairs(const QHash<qint64, TokenMap>& stokenmaps)
{
	checkSingleRowPairs("(", ")", stokenmaps);
	checkSingleRowPairs("[", "]", stokenmaps);
	checkSingleRowPairs("{", "}", stokenmaps);
}

//根據指定分割符號取得字串
bool Lexer::getStringCommandToken(QString& src, const QString& delim, QString& out) const
{
	if (src.isEmpty() || delim.isEmpty())
		return false;

	enum class State { Normal, DoubleQuoted, SingleQuoted, InParentheses, InBraces };
	State state = State::Normal;

	qint64 i = 0;
	qint64 start = 0;
	qint64 srcSize = src.size();

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
				return true;
			}
		}
	}

	return false;

	//if (src.isEmpty())
	//	return false;

	//if (delim.isEmpty())
	//	return false;

	//if (src.startsWith("\""))
	//{
	//	// Find the closing quote
	//	qint64 closingQuoteIndex = src.indexOf("\"", 1);
	//	if (closingQuoteIndex == -1)
	//		return false;

	//	qint64 srcSize = src.size();
	//	for (qint64 i = closingQuoteIndex + 1; i < srcSize; ++i)
	//	{
	//		if (src.at(i) == '"')
	//		{
	//			if (i > 0 && src.at(i - 1) == '\\')
	//				continue;
	//			closingQuoteIndex = i;
	//			break;
	//		}
	//	}

	//	// Extract the quoted token
	//	out = src.mid(0, closingQuoteIndex + 1).trimmed();
	//	src.remove(0, closingQuoteIndex + 1);
	//	src = src.trimmed();
	//	if (src.startsWith(delim))
	//		src.remove(0, delim.size());
	//}
	//else if (src.startsWith("'"))
	//{
	//	// Find the closing single quote
	//	qint64 closingSingleQuoteIndex = src.indexOf("'", 1);
	//	if (closingSingleQuoteIndex == -1)
	//		return false;

	//	qint64 srcSize = src.size();
	//	for (qint64 i = closingSingleQuoteIndex + 1; i < srcSize; ++i)
	//	{
	//		if (src.at(i) == '\'')
	//		{
	//			if (i > 0 && src.at(i - 1) == '\\')
	//				continue;
	//			closingSingleQuoteIndex = i;
	//			break;
	//		}
	//	}

	//	// Extract the quoted token
	//	out = src.mid(0, closingSingleQuoteIndex + 1);
	//	src.remove(0, closingSingleQuoteIndex + 1);
	//	src = src.trimmed();
	//	if (src.startsWith(delim))
	//		src.remove(0, delim.size());
	//}
	//else
	//{
	//	QStringList list = src.split(delim);
	//	if (list.isEmpty())
	//	{
	//		// Empty token, treat it as an empty string
	//		out = "";
	//	}
	//	else
	//	{
	//		out = list.first().trimmed();
	//		qint64 size = out.size();

	//		// Remove the first 'out' and 'delim' from src
	//		src.remove(0, size + delim.size());
	//		if (src.startsWith(delim))
	//			src.remove(0, delim.size());
	//		src = src.trimmed();
	//	}
	//}

	//if (src.endsWith(";"))
	//	src.remove(src.size() - 1, 1);

	//if (out.endsWith(";"))
	//	out.remove(out.size() - 1, 1);

	//return true;
}

qint64 Lexer::findClosingQuoteIndex(const QString& src, QChar quoteChar, int startIndex) const
{
	QChar escapeChar = '\\';
	bool escaped = false;

	for (int i = startIndex + 1; i < src.size(); ++i)
	{
		QChar currentChar = src.at(i);

		if (escaped)
		{
			escaped = false;
		}
		else if (currentChar == escapeChar)
		{
			escaped = true;
		}
		else if (currentChar == quoteChar)
		{
			return i;
		}
	}

	return -1;
}

void Lexer::extractAndRemoveToken(QString& src, const QString& delim, int startIndex, int endIndex, QString& out) const
{
	out = src.mid(startIndex, endIndex - startIndex + 1).trimmed();
	src.remove(startIndex, endIndex - startIndex + 1);

	src = src.trimmed();
	if (src.startsWith(delim))
		src.remove(0, delim.size());
}

bool Lexer::isInsideQuotes(const QString& src, int index) const
{
	QChar singleQuote = '\'';
	QChar doubleQuote = '"';
	bool insideSingleQuotes = false;
	bool insideDoubleQuotes = false;

	for (int i = 0; i < src.size(); ++i)
	{
		if (i == index)
			break;

		QChar currentChar = src.at(i);
		QChar previousChar = i > 0 ? src.at(i - 1) : QChar();

		if (currentChar == singleQuote && previousChar != '\\')
		{
			insideSingleQuotes = !insideSingleQuotes;
		}
		else if (currentChar == doubleQuote && previousChar != '\\')
		{
			insideDoubleQuotes = !insideDoubleQuotes;
		}
	}

	return insideSingleQuotes || insideDoubleQuotes;
}

#pragma endregion