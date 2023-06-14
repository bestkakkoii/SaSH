#include "stdafx.h"
#include "lexer.h"

//全局關鍵字映射表 這裡是新增新的命令的第一步，其他需要在interpreter.cpp中新增，這裡不添加的話，腳本分析後會忽略未知的命令
static const QHash<QString, RESERVE> keywords = {
#pragma region BIG5
	//test
	{ u8"測試", TK_CMD },

	//keyword
	{ u8"調用", TK_CALL },
	{ u8"行數", TK_JMP },
	{ u8"跳轉", TK_JMP },
	{ u8"返回", TK_RETURN },
	{ u8"結束", TK_END },
	{ u8"暫停", TK_PAUSE },
	{ u8"功能", TK_LABEL, },
	{ u8"標記", TK_LABEL, },
	{ u8"變數", TK_VARDECL },
	{ u8"變數移除", TK_VARFREE },
	{ u8"變數清空", TK_VARCLR },
	{ u8"格式化", TK_FORMAT },

	//system
	{ u8"執行", TK_CMD },
	{ u8"延時", TK_CMD },
	{ u8"提示", TK_CMD },
	{ u8"消息", TK_CMD },
	{ u8"登出", TK_CMD },
	{ u8"元神歸位", TK_CMD },
	{ u8"回點", TK_CMD },
	{ u8"按鈕", TK_CMD },
	{ u8"說話", TK_CMD },
	{ u8"輸入", TK_CMD },
	{ u8"說出", TK_CMD },
	{ u8"清屏", TK_CMD },
	{ u8"設置", TK_CMD },
	{ u8"判斷", TK_CMD },
	{ u8"讀取設置", TK_CMD },
	{ u8"儲存設置", TK_CMD },


	//check info
	{ u8"戰鬥中", TK_CMD },
	{ u8"查坐標", TK_CMD },
	{ u8"查座標", TK_CMD },
	{ u8"地圖", TK_CMD },
	{ u8"地圖快判", TK_CMD },
	{ u8"對話", TK_CMD },
	{ u8"看見", TK_CMD },
	{ u8"聽見", TK_CMD },
	{ u8"道具", TK_CMD },
	{ u8"道具數量", TK_CMD },
	{ u8"背包滿", TK_CMD },
	{ u8"人物狀態", TK_CMD },
	{ u8"寵物有", TK_CMD },
	{ u8"寵物狀態", TK_CMD },
	{ u8"寵物數量", TK_CMD },
	{ u8"任務狀態", TK_CMD },

	//actions
	{ u8"人物改名", TK_CMD },
	{ u8"使用精靈", TK_CMD },
	{ u8"寵物改名", TK_CMD },
	{ u8"寵物郵件", TK_CMD },
	{ u8"更換寵物", TK_CMD },
	{ u8"丟棄寵物", TK_CMD },
	{ u8"購買", TK_CMD },
	{ u8"售賣", TK_CMD },
	{ u8"使用道具", TK_CMD },
	{ u8"丟棄道具", TK_CMD },
	{ u8"撿物", TK_CMD },
	{ u8"存入道具", TK_CMD },
	{ u8"提出道具", TK_CMD },
	{ u8"存入寵物", TK_CMD },
	{ u8"提出寵物", TK_CMD },
	{ u8"存錢", TK_CMD },
	{ u8"提錢", TK_CMD },
	{ u8"加工", TK_CMD },
	{ u8"料理", TK_CMD },
	{ u8"轉移", TK_CMD },
	{ u8"卸下裝備", TK_CMD },
	{ u8"記錄身上裝備", TK_CMD },
	{ u8"裝上記錄裝備", TK_CMD },
	{ u8"加點", TK_CMD },

	//action with sub cmd
	{ u8"組隊", TK_CMD },
	{ u8"離隊", TK_CMD },
	{ u8"組隊有", TK_CMD },
	{ u8"組隊人數", TK_CMD },

	//move
	{ u8"坐標", TK_CMD },
	{ u8"座標", TK_CMD },
	{ u8"移動", TK_CMD },
	{ u8"封包移動", TK_CMD },
	{ u8"方向", TK_CMD },
	{ u8"最近坐標", TK_CMD },
	{ u8"尋路", TK_CMD },
	{ u8"移動至NPC", TK_CMD },
	{ u8"過點", TK_CMD },

	//mouse
	{ u8"左擊", TK_CMD },
	{ u8"右擊", TK_CMD },
	{ u8"左雙擊", TK_CMD },
	{ u8"拖至", TK_CMD },
#pragma endregion

	////////////////////////////////////////////////////
	////////////// GB2312 //////////////////////////////
	////////////////////////////////////////////////////

#pragma region GB2312

	//test
	{ u8"测试", TK_CMD },

	//keyword
	{ u8"调用", TK_CALL },
	{ u8"行数", TK_JMP },
	{ u8"跳转", TK_JMP },
	{ u8"返回", TK_RETURN },
	{ u8"结束", TK_END },
	{ u8"暂停", TK_PAUSE },
	{ u8"功能", TK_LABEL, },
	{ u8"标记", TK_LABEL, },
	{ u8"变数", TK_VARDECL },
	{ u8"变数移除", TK_VARFREE },
	{ u8"变数清空", TK_VARCLR },
	{ u8"格式化", TK_FORMAT },

	//system
	{ u8"执行", TK_CMD },
	{ u8"延时", TK_CMD },
	{ u8"提示", TK_CMD },
	{ u8"消息", TK_CMD },
	{ u8"登出", TK_CMD },
	{ u8"元神归位", TK_CMD },
	{ u8"回点", TK_CMD },
	{ u8"按钮", TK_CMD },
	{ u8"说话", TK_CMD },
	{ u8"输入", TK_CMD },
	{ u8"说出", TK_CMD },
	{ u8"清屏", TK_CMD },
	{ u8"设置", TK_CMD },
	{ u8"判断", TK_CMD },
	{ u8"读取设置", TK_CMD },
	{ u8"储存设置", TK_CMD },


	//check info
	{ u8"战斗中", TK_CMD },
	{ u8"查坐标", TK_CMD },
	{ u8"查座标", TK_CMD },
	{ u8"地图", TK_CMD },
	{ u8"地图快判", TK_CMD },
	{ u8"对话", TK_CMD },
	{ u8"看见", TK_CMD },
	{ u8"听见", TK_CMD },
	{ u8"道具", TK_CMD },
	{ u8"道具数量", TK_CMD },
	{ u8"背包满", TK_CMD },
	{ u8"人物状态", TK_CMD },
	{ u8"宠物有", TK_CMD },
	{ u8"宠物状态", TK_CMD },
	{ u8"宠物数量", TK_CMD },
	{ u8"任务状态", TK_CMD },

	//actions
	{ u8"人物改名", TK_CMD },
	{ u8"使用精灵", TK_CMD },
	{ u8"宠物改名", TK_CMD },
	{ u8"宠物邮件", TK_CMD },
	{ u8"更换宠物", TK_CMD },
	{ u8"丢弃宠物", TK_CMD },
	{ u8"购买", TK_CMD },
	{ u8"售卖", TK_CMD },
	{ u8"使用道具", TK_CMD },
	{ u8"丢弃道具", TK_CMD },
	{ u8"捡物", TK_CMD },
	{ u8"存入道具", TK_CMD },
	{ u8"提出道具", TK_CMD },
	{ u8"存入宠物", TK_CMD },
	{ u8"提出宠物", TK_CMD },
	{ u8"存钱", TK_CMD },
	{ u8"提钱", TK_CMD },
	{ u8"加工", TK_CMD },
	{ u8"料理", TK_CMD },
	{ u8"转移", TK_CMD },
	{ u8"卸下装备", TK_CMD },
	{ u8"记录身上装备", TK_CMD },
	{ u8"装上记录装备", TK_CMD },
	{ u8"加点", TK_CMD },

	//action with sub cmd
	{ u8"组队", TK_CMD },
	{ u8"离队", TK_CMD },
	{ u8"组队有", TK_CMD },
	{ u8"组队人数", TK_CMD },

	//move
	{ u8"坐标", TK_CMD },
	{ u8"座标", TK_CMD },
	{ u8"移动", TK_CMD },
	{ u8"封包移动", TK_CMD },
	{ u8"方向", TK_CMD },
	{ u8"最近坐标", TK_CMD },
	{ u8"寻路", TK_CMD },
	{ u8"移动至NPC", TK_CMD },
	{ u8"过点", TK_CMD },

	//mouse
	{ u8"左击", TK_CMD },
	{ u8"右击", TK_CMD },
	{ u8"左双击", TK_CMD },
	{ u8"拖至", TK_CMD },

#pragma endregion

#pragma region UTF8

	//test
	{ u8"test", TK_CMD },

	//keyword
	{ u8"call", TK_CALL },
	{ u8"goto", TK_JMP },
	{ u8"end", TK_RETURN },
	{ u8"return", TK_RETURN },
	{ u8"exit", TK_END },
	{ u8"pause", TK_PAUSE },
	{ u8"label", TK_LABEL, },
	{ u8"function", TK_LABEL, },
	{ u8"var", TK_VARDECL },
	{ u8"vardelete", TK_VARFREE },
	{ u8"varclear", TK_VARCLR },
	{ u8"format", TK_FORMAT },

	//system
	{ u8"run", TK_CMD },
	{ u8"sleep", TK_CMD },
	{ u8"print", TK_CMD },
	{ u8"msg", TK_CMD },
	{ u8"logout", TK_CMD },
	{ u8"eo", TK_CMD },
	{ u8"logback", TK_CMD },
	{ u8"button", TK_CMD },
	{ u8"say", TK_CMD },
	{ u8"input", TK_CMD },
	{ u8"talk", TK_CMD },
	{ u8"cls", TK_CMD },
	{ u8"set", TK_CMD },
	{ u8"if", TK_CMD },
	{ u8"saveset", TK_CMD },
	{ u8"loadset", TK_CMD },


	//check info
	{ u8"ifbattle", TK_CMD },
	{ u8"ifpos", TK_CMD },
	{ u8"ifmap", TK_CMD },
	{ u8"ifitem", TK_CMD },
	{ u8"ifitemfull", TK_CMD },
	{ u8"ifplayer", TK_CMD },
	{ u8"ifpet", TK_CMD },
	{ u8"ifpetex", TK_CMD },
	{ u8"ifdaily", TK_CMD },

	{ u8"waitmap", TK_CMD },
	{ u8"waitdlg", TK_CMD },
	{ u8"waitsay", TK_CMD },
	{ u8"waititem", TK_CMD },
	{ u8"waitpet", TK_CMD },

	//actions
	{ u8"chplayername", TK_CMD },
	{ u8"usemagic", TK_CMD },
	{ u8"chpetname", TK_CMD },
	{ u8"chpet", TK_CMD },
	{ u8"droppet", TK_CMD },
	{ u8"buy", TK_CMD },
	{ u8"sell", TK_CMD },
	{ u8"useitem", TK_CMD },
	{ u8"doffitem", TK_CMD },
	{ u8"pickup", TK_CMD },
	{ u8"put", TK_CMD },
	{ u8"get", TK_CMD },
	{ u8"putpet", TK_CMD },
	{ u8"getpet", TK_CMD },
	{ u8"save", TK_CMD },
	{ u8"load", TK_CMD },
	{ u8"make", TK_CMD },
	{ u8"cook", TK_CMD },
	{ u8"unequip", TK_CMD },
	{ u8"recordequip", TK_CMD },
	{ u8"wearrecordequip", TK_CMD },
	{ u8"addpoint", TK_CMD },

	//action with sub cmd
	{ u8"join", TK_CMD },
	{ u8"leave", TK_CMD },
	{ u8"waitteam", TK_CMD },
	{ u8"ifteam", TK_CMD },

	//move
	{ u8"walkpos", TK_CMD },
	{ u8"move", TK_CMD },
	{ u8"w", TK_CMD },
	{ u8"dir", TK_CMD },
	{ u8"findpath", TK_CMD },
	{ u8"movetonpc", TK_CMD },
	{ u8"chmap", TK_CMD },
	{ u8"warp", TK_CMD },

	//mouse
	{ u8"lclick", TK_CMD },
	{ u8"rclick", TK_CMD },
	{ u8"ldbclick", TK_CMD },
	{ u8"dragto", TK_CMD },

	#pragma endregion

	//... 其他後續增加的關鍵字
};

//插入新TOKEN
void Lexer::createToken(int index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken)
{
	ptoken->insert(index, { type, data, raw });
}

//插入空行TOKEN
void Lexer::createEmptyToken(int index, TokenMap* ptoken)
{
	ptoken->insert(index, { TK_WHITESPACE, "", "" });
}

//解析單行內容至多個TOKEN
void Lexer::tokenized(int currentLine, const QString& line, TokenMap* ptoken, QHash<QString, int>* plabel)
{
	if (ptoken == nullptr || plabel == nullptr)
		return;

	int pos = 0;
	QString token;
	QVariant data;
	QString raw = line.trimmed();

	ptoken->clear();

	do
	{
		int commentIndex = raw.indexOf("//");
		if (commentIndex > 0)
		{
			//當前token移除註釋
			raw = raw.mid(0, commentIndex).trimmed();
		}


		if (!getStringToken(raw, " ", token))
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
		if (token.startsWith("//"))
		{
			createToken(pos, TK_COMMENT, "", "", ptoken);
			createToken(pos + 1, TK_COMMENT, data, token, ptoken);
			break;
		}

		RESERVE type = keywords.value(token, TK_UNK);
		if (type == TK_UNK)
		{
			createEmptyToken(pos, ptoken);
			break;
		}


		createToken(pos, type, QVariant::fromValue(token.toLower()), token.toLower(), ptoken);
		++pos;

		for (;;)
		{

			if (!getStringToken(raw, ",", token))
				break;

			if (token.isEmpty())
			{
				break;
			}
			RESERVE prevType = type;
			type = getTokenType(pos, prevType, token, raw);

			if (type == TK_INT)
			{
				bool ok;
				int intValue = token.toInt(&ok);
				if (ok)
				{
					data = QVariant::fromValue(intValue);
				}
			}
			else if (type == TK_DOUBLE)
			{
				bool ok;
				double floatValue = token.toDouble(&ok);
				if (ok)
				{
					data = QVariant::fromValue(floatValue);
				}
			}
			else if (type == TK_STRING)
			{
				if (token.startsWith("\"") && token.endsWith("\""))
					token = token.mid(1, token.length() - 2);
				data = QVariant::fromValue(token);
			}
			else if (type == TK_NAME)
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

//解析整個腳本至多個TOKEN
bool Lexer::tokenized(const QString& script, QHash<int, TokenMap>* ptokens, QHash<QString, int>* plabel)
{
	Lexer lexer;
	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	QStringList lines = script.split("\n");
	int size = lines.size();
	for (int i = 0; i < size; ++i)
	{
		TokenMap tk;
		lexer.tokenized(i, lines.at(i), &tk, &labels);
		tokens.insert(i, tk);
	}

	if (ptokens != nullptr && plabel != nullptr)
	{
		*ptokens = tokens;
		*plabel = labels;
		return true;
	}

	return false;
}

bool Lexer::isDouble(const QString& str) const
{
	if (str.count('.') != 1)
		return false;
	bool ok;
	str.toDouble(&ok);
	return ok;
}

bool Lexer::isInteger(const QString& str) const
{
	bool ok;
	str.toInt(&ok);
	return ok;
}

bool Lexer::isBool(const QString& str) const
{
	return (str == QString(u8"真") || str == QString(u8"假") || str == "true" || str == "false");
}

bool Lexer::isName(const QString& str, RESERVE previousType) const
{
	//check not start from number
	if (str.isEmpty() || str.at(0).isDigit())
		return false;

	return previousType == TK_LABEL || previousType == TK_CALL || previousType == TK_JMP;
}

bool Lexer::isString(const QString& str) const
{
	return (str.startsWith("\"") && str.endsWith("\"")) || (str.startsWith("\'") && str.endsWith("\'"))
		|| (str.startsWith("\'") && str.endsWith("\"")) || (str.startsWith("\"") && str.endsWith("\'"));
}

bool Lexer::isVariable(const QString& str) const
{
	return str.startsWith(kVariablePrefix) && !str.endsWith(kVariablePrefix);
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

bool Lexer::isDelimiter(const QChar& ch) const
{
	return ((ch == ',') || (ch == ' '));
}

//根據容取TOKEN應該定義的類型
RESERVE Lexer::getTokenType(int& pos, RESERVE previous, QString& str, const QString raw) const
{


	int index = 0;

	if (str == "<<")
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
	else if (str == "+")
	{
		return TK_ADD;
	}
	else if (str == "++")
	{
		return TK_INC;
	}
	else if (str == "-")
	{
		return TK_SUB;
	}
	else if (str == "--")
	{
		return TK_DEC;
	}
	else if (str == "*")
	{
		return TK_MUL;
	}
	else if (str == "/")
	{
		return TK_DIV;
	}
	else if (str == "%")
	{
		return TK_MOD;
	}
	else if (str == "&")
	{
		return TK_AND;
	}
	else if (str == "|")
	{
		return TK_OR;
	}
	else if (str == "~")
	{
		return TK_NOT;
	}
	else if (str == "^")
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
	else if (isVariable(str))
	{
		QChar nextChar = next(raw, index);
		if (nextChar.isLetterOrNumber() || nextChar == '\0')
		{
			return TK_REF;
		}
		else
		{
			return TK_UNK;
		}
	}
	else if (previous == TK_NAME || previous == TK_LABELVAR)
	{
		return TK_LABELVAR;
	}
	else if (isString(str))
	{
		return TK_STRING;
	}
	else if (isDouble(str))
	{
		return  TK_DOUBLE;
	}
	else if (isInteger(str))
	{
		return TK_INT;
	}
	else if (isBool(str))
	{
		return TK_BOOL;
	}
	else if (isName(str, previous))
	{
		return TK_NAME;
	}
	else if (str.startsWith("//"))
	{
		return TK_COMMENT;
	}

	return TK_STRING;
}

//根據index取得下一個字元
QChar Lexer::next(const QString& str, int& index) const
{
	if (index < str.length() - 1)
	{
		++index;
		return str[index];
	}
	else
	{
		return QChar();
	}
}

//根據指定分割符號取得字串
bool Lexer::getStringToken(QString& src, const QString& delim, QString& out)
{
	if (src.isEmpty())
		return false;

	if (delim.isEmpty())
		return false;

	QStringList list = src.split(delim);
	if (list.isEmpty())
		return false;

	out = list.first().trimmed();
	int size = out.size();
	//remove frist out and delim from src
	src.remove(0, size + delim.size());
	if (src.startsWith(delim))
		src.remove(0, delim.size());
	src = src.trimmed();
	return true;
}

