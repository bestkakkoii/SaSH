#include "stdafx.h"
#include "lexer.h"
#include "injector.h"
#include "signaldispatcher.h"

//全局關鍵字映射表 這裡是新增新的命令的第一步，其他需要在interpreter.cpp中新增，這裡不添加的話，腳本分析後會忽略未知的命令
static const QHash<QString, RESERVE> keywords = {
#pragma region BIG5
	//test
	{ u8"測試", TK_CMD },

	//keyword
	{ u8"調用", TK_CALL },
	{ u8"行數", TK_GOTO },
	{ u8"跳轉", TK_GOTO },
	{ u8"返回", TK_RETURN },
	{ u8"返回跳轉", TK_BAK },
	{ u8"結束", TK_END },
	{ u8"暫停", TK_PAUSE },
	{ u8"功能", TK_LABEL, },
	{ u8"標記", TK_LABEL, },
	{ u8"變量", TK_VARDECL },
	{ u8"變量移除", TK_VARFREE },
	{ u8"變量清空", TK_VARCLR },
	{ u8"格式化", TK_FORMAT },
	{ u8"隨機數", TK_RND },
	{ u8"如果", TK_CMP },

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
	{ u8"讀取設置", TK_CMD },
	{ u8"儲存設置", TK_CMD },
	{ u8"計時", TK_CMD },

	//check info
	{ u8"戰鬥中", TK_CMD },
	{ u8"在線中", TK_CMD },
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
	{ u8"賣寵", TK_CMD },
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
	{ u8"卸下寵裝備", TK_CMD },
	{ u8"裝上寵裝備", TK_CMD },
	{ u8"加點", TK_CMD },
	{ u8"學習", TK_CMD },
	{ u8"交易", TK_CMD },
	{ u8"寄信", TK_CMD },

	//action with sub cmd
	{ u8"組隊", TK_CMD },
	{ u8"離隊", TK_CMD },
	{ u8"踢走", TK_CMD },
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
	{ u8"行数", TK_GOTO },
	{ u8"跳转", TK_GOTO },
	{ u8"返回", TK_RETURN },
	{ u8"返回跳转", TK_BAK },
	{ u8"结束", TK_END },
	{ u8"暂停", TK_PAUSE },
	{ u8"功能", TK_LABEL, },
	{ u8"标记", TK_LABEL, },
	{ u8"变量", TK_VARDECL },
	{ u8"变量移除", TK_VARFREE },
	{ u8"变量清空", TK_VARCLR },
	{ u8"格式化", TK_FORMAT },
	{ u8"随机数", TK_RND },
	{ u8"如果", TK_CMP },

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
	{ u8"读取设置", TK_CMD },
	{ u8"储存设置", TK_CMD },
	{ u8"计时", TK_CMD },


	//check info
	{ u8"战斗中", TK_CMD },
	{ u8"在线中", TK_CMD },
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
	{ u8"卖宠", TK_CMD },
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
	{ u8"卸下宠装备", TK_CMD },
	{ u8"装上宠装备", TK_CMD },
	{ u8"加点", TK_CMD },
	{ u8"学习", TK_CMD },
	{ u8"交易", TK_CMD },
	{ u8"寄信", TK_CMD },

	//action with sub cmd
	{ u8"组队", TK_CMD },
	{ u8"离队", TK_CMD },
	{ u8"踢走", TK_CMD },
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
	{ u8"goto", TK_GOTO },
	{ u8"jmp", TK_JMP },
	{ u8"end", TK_RETURN },
	{ u8"return", TK_RETURN },
	{ u8"back", TK_BAK },
	{ u8"exit", TK_END },
	{ u8"pause", TK_PAUSE },
	{ u8"label", TK_LABEL, },
	{ u8"function", TK_LABEL, },
	{ u8"var", TK_VARDECL },
	{ u8"local", TK_LOCAL },
	{ u8"delete", TK_VARFREE },
	{ u8"releaseall", TK_VARCLR },
	{ u8"format", TK_FORMAT },
	{ u8"rnd", TK_RND },
	{ u8"if", TK_CMP },

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
	{ u8"saveset", TK_CMD },
	{ u8"loadset", TK_CMD },
	{ u8"reg", TK_CMD },
	{ u8"timer", TK_CMD },

	//check info
	{ u8"ifbattle", TK_CMD },
	{ u8"ifnormal", TK_CMD },
	{ u8"ifonline", TK_CMD },
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
	{ u8"sellpet", TK_CMD },
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
	{ u8"uequip", TK_CMD },
	{ u8"requip", TK_CMD },
	{ u8"wequip", TK_CMD },
	{ u8"puequip", TK_CMD },
	{ u8"pequip", TK_CMD },
	{ u8"skup", TK_CMD },
	{ u8"learn", TK_CMD },
	{ u8"trade", TK_CMD },
	{ u8"dostring", TK_CMD },
	{ u8"mail", TK_CMD },

	//action with sub cmd
	{ u8"join", TK_CMD },
	{ u8"leave", TK_CMD },
	{ u8"kick", TK_CMD },
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

	//hide
	{ u8"ocr", TK_CMD },
	{ u8"dlg", TK_CMD },
	{ u8"regex", TK_CMD },
	{ u8"find", TK_CMD },
	{ u8"half", TK_CMD },
	{ u8"full", TK_CMD },
	{ u8"upper", TK_CMD },
	{ u8"lower", TK_CMD },
	{ u8"replace", TK_CMD },
	{ u8"toint", TK_CMD },
	{ u8"tostr", TK_CMD },
	{ u8"todb", TK_CMD },

	//battle
	{ u8"bh", TK_CMD },//atk
	{ u8"bj", TK_CMD },//magic
	{ u8"bp", TK_CMD },//skill
	{ u8"bs", TK_CMD },//switch
	{ u8"be", TK_CMD },//escape
	{ u8"bd", TK_CMD },//defense
	{ u8"bi", TK_CMD },//item
	{ u8"bt", TK_CMD },//catch
	{ u8"bn", TK_CMD },//nothing
	{ u8"bw", TK_CMD },//petskill
	{ u8"bwf", TK_CMD },//pet nothing
	{ u8"bwait", TK_CMD },
	{ u8"bend", TK_CMD },
	#pragma endregion

	//... 其他後續增加的關鍵字
};

void Lexer::showError(const QString text, ErrorType type)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.appendScriptLog(text, type);
}

//插入新TOKEN
void Lexer::createToken(qint64 index, RESERVE type, const QVariant& data, const QString& raw, TokenMap* ptoken)
{
	ptoken->insert(index, { type, data, raw });
}

//插入空行TOKEN
void Lexer::createEmptyToken(qint64 index, TokenMap* ptoken)
{
	ptoken->insert(index, { TK_WHITESPACE, "", "" });
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
	RESERVE type = TK_UNK;

	ptoken->clear();

	do
	{
		qint64 commentIndex = raw.indexOf("//");
		if (commentIndex > 0)
		{
			//當前token移除註釋
			raw = raw.mid(0, commentIndex).trimmed();
		}

		bool doNotLowerCase = false;
		static const QRegularExpression rexMultiVar(R"(^\s*([_a-zA-Z\p{Han}][_a-zA-Z0-9\p{Han}]*(?:\s*,\s*[_a-zA-Z\p{Han}][_a-zA-Z0-9\p{Han}]*)*)\s*=\s*([^,]+(?:\s*,\s*[^,]+)*)$)");//a,b,c = 1,2,3
		static const QRegularExpression rexMultiLocalVar(R"([lL][oO][cC][aA][lL]\s+([_a-zA-Z\p{Han}][_a-zA-Z0-9\p{Han}]*(?:\s*,\s*[_a-zA-Z\p{Han}][_a-zA-Z0-9\p{Han}]*)*)\s*=\s*([^,]+(?:\s*,\s*[^,]+)*)$)");//local a,b,c = 1,2,3
		static const QRegularExpression varIncDec(R"(([\p{Han}\w]+)(\+\+|--))");//++ --
		static const QRegularExpression varCAOs(R"((?!\d)([\p{Han}\w]+)\s+([+\-*\/&|^%]\=)\s+([\W\w\s\p{Han}]+))");//+= -= *= /= &= |= ^= %=
		static const QRegularExpression varExpr(R"(([\w\p{Han}]+)\s+\=\s+([\W\w\s\p{Han}]+))");//x = expr
		static const QRegularExpression varAnyOp(R"([+\-*\/%&|^\(\)])");//+ - * / % & | ^ ( )
		static const QRegularExpression varIf(R"([iI][fF][\(|\s+]([\d\w\W\p{Han}]+\s*[<|>|\=|!][\=]*\s*[\d\w\W\p{Han}]+))");//if (expr)
		static const QRegularExpression rexFunction(R"(function\s+([\w\p{Han}\d]+)\s*\(([\w\W\p{Han}]*)\))");
		static const QRegularExpression rexCallFunction(R"((\w+)\s*\(([\w\W\p{Han}]*)\))");
		if (raw.contains(varIf))
		{
			QRegularExpressionMatch match = varIf.match(raw);
			if (match.hasMatch())
			{
				QString cmd = "if";
				QString expr = match.captured(1).simplified();
				QStringList exprList = expr.split(util::rexComma);
				createToken(pos, TK_CMP, cmd, cmd, ptoken);
				createToken(pos + 1, TK_STRING, exprList.at(0), exprList.at(0), ptoken);
				if (exprList.size() > 1)
				{
					bool ok;
					qint64 value = exprList.at(1).toLongLong(&ok);
					if (ok)
						createToken(pos + 2, TK_INT, value, exprList.at(1), ptoken);
					else
						createToken(pos + 2, TK_STRING, exprList.at(1), exprList.at(1), ptoken);
				}
				break;
			}
		}
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
				break;
			}
		}
		else if (raw.contains(varCAOs) && !raw.front().isDigit())
		{
			QRegularExpressionMatch match = varCAOs.match(raw);
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
			break;
		}
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
			bool isCommand = raw.trimmed().startsWith("//");

			if (!isCommand && raw.contains(rexFunction))
			{
				QRegularExpressionMatch match = rexFunction.match(raw);
				if (!match.hasMatch())
					break;
				token = "function";
				raw = QString("%1,%2,%3").arg(match.captured(1).simplified(), match.captured(2).trimmed(), match.captured(3).trimmed());
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
					token = "call";
				}
			}
			else
			{
				//以空格為分界分離出第一個TOKEN(命令)
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
			}

			//檢查第一個TOKEN是否存在於關鍵字表，否則視為空行
			type = keywords.value(token, TK_UNK);
			if (type == TK_UNK)
			{
				showError(QObject::tr("<Warning>Unknown command '%1' has been ignored at line: %2").arg(token).arg(currentLine + 1), kTypeWarning);
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
				if (!getStringToken(raw, ",", token))
					break;
			}
			else if (raw.isEmpty())
				break;
			else
			{
				token = raw;
				raw.clear();
			}

			//忽略空白TOKEN
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
				{
					data = QVariant::fromValue(intValue);
				}
			}
			else if (type == TK_BOOL)
			{
				type = TK_INT;
				//真為1，假為0, true為1，false為0
				if (token.toLower() == "true" || token == "真")
					data = QVariant::fromValue(1ll);
				else if (token.toLower() == "false" || token == "假")
					data = QVariant::fromValue(0ll);
			}
			else if (type == TK_DOUBLE)//對雙精度浮點數進行強制轉INT處理
			{
				bool ok;
				qint64 floatValue = token.toLongLong(&ok);
				if (ok)
				{
					data = QVariant::fromValue(floatValue);
					type = TK_INT;
				}
			}
			else if (type == TK_STRING)//對字串進行轉換處理，去除首尾單引號、雙引號，並標記為常量
			{
				if ((token.startsWith("\"") || token.startsWith("\'")) && (token.endsWith("\"") || token.endsWith("\'")))
				{
					token = token.mid(1, token.length() - 2);
					type = TK_CSTRING;
				}

				data = QVariant::fromValue(token);
			}
			else if (type == TK_LABELVAR)
			{
				if ((token.startsWith("\"") || token.startsWith("\'")) && (token.endsWith("\"") || token.endsWith("\'")))
				{
					token = token.mid(1, token.length() - 2);
					type = TK_CSTRING;
				}

				data = QVariant::fromValue(token);
			}
			else if (type == TK_NAME)//保存標記名稱
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
bool Lexer::tokenized(const QString& script, QHash<qint64, TokenMap>* ptokens, QHash<QString, qint64>* plabel)
{

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->clear();

	Lexer lexer;
	QHash<qint64, TokenMap> tokens;
	QHash<QString, qint64> labels;
	QStringList lines = script.split("\n");
	qint64 size = lines.size();
	for (qint64 i = 0; i < size; ++i)
	{
		TokenMap tk;
		lexer.tokenized(i, lines.at(i), &tk, &labels);
		tokens.insert(i, tk);
	}

	lexer.checkInvalidReadVariable(tokens);
	lexer.checkFunctionPairs(tokens);

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
	str.toLongLong(&ok);
	return ok;
}

//預留的類型，但一般用戶容易混淆，所以實際上並沒有使用
bool Lexer::isBool(const QString& str) const
{
	return (str == QString(u8"真") || str == QString(u8"假") || str.toLower() == "true" || str.toLower() == "false");
}

bool Lexer::isName(const QString& str, RESERVE previousType) const
{
	//check not start from number
	if (str.isEmpty() || str.at(0).isDigit())
		return false;

	return previousType == TK_LABEL || previousType == TK_CALL || previousType == TK_GOTO;
}

bool Lexer::isString(const QString& str) const
{
	return (str.startsWith("\"") && str.endsWith("\"")) || (str.startsWith("\'") && str.endsWith("\'"))
		|| (str.startsWith("\'") && str.endsWith("\"")) || (str.startsWith("\"") && str.endsWith("\'"));
}

//bool Lexer::isVariable(const QString& str) const
//{
//	return str.startsWith(kVariablePrefix) && !str.endsWith(kVariablePrefix);
//}

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
RESERVE Lexer::getTokenType(qint64& pos, RESERVE previous, QString& str, const QString raw) const
{
	//findex = 0;

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
	else if (previous == TK_NAME || previous == TK_LABELVAR)
	{
		//如果前一個TOKEN是label名或區域變量名，那麼接下來的TOKEN都視為區域變量名
		return TK_LABELVAR;
	}
	else if (isString(str))
	{
		return TK_STRING;
	}
	else if (isDouble(str))
	{
		return TK_DOUBLE;
	}
	else if (isInteger(str))
	{
		return TK_INT;
	}
	else if (isName(str, previous))
	{
		return TK_NAME;
	}
	else if (str.startsWith("//"))
	{
		return TK_COMMENT;
	}

	//其他的都默認為字符串
	return TK_STRING;
}

//根據index取得下一個字元
QChar Lexer::next(const QString& str, qint64& index) const
{
	if (index < static_cast<qint64>(str.length()) - 1ll)
	{
		++index;
		return str.at(index);
	}
	else
	{
		return QChar();
	}
}

//根據指定分割符號取得字串
bool Lexer::getStringToken(QString& src, const QString& delim, QString& out)
{
	//if (src.isEmpty())
	//	return false;

	//if (delim.isEmpty())
	//	return false;

	//QStringList list = src.split(delim);
	//if (list.isEmpty())
	//	return false;

	//out = list.first().trimmed();
	//qint64 size = out.size();

	////remove frist 'out' and 'delim' from src
	//src.remove(0, size + delim.size());
	//if (src.startsWith(delim))
	//	src.remove(0, delim.size());
	//src = src.trimmed();
	//return true;

	if (src.isEmpty())
		return false;

	if (delim.isEmpty())
		return false;

	QString openingQuote = "\"";
	QString closingQuote = "\"";

	if (src.startsWith(openingQuote))
	{
		// Find the closing quote
		qint64 closingQuoteIndex = src.indexOf(closingQuote, openingQuote.size());
		if (closingQuoteIndex == -1)
			return false;

		// Extract the quoted token
		out = src.mid(0, closingQuoteIndex + closingQuote.size()).trimmed();
		src.remove(0, closingQuoteIndex + closingQuote.size());
		src = src.trimmed();
		if (src.startsWith(delim))
			src.remove(0, delim.size());
	}
	else if (src.startsWith("'"))
	{
		// Find the closing single quote
		qint64 closingSingleQuoteIndex = src.indexOf("'", 1);
		if (closingSingleQuoteIndex == -1)
			return false;

		// Extract the quoted token
		out = src.mid(0, closingSingleQuoteIndex + 1);
		src.remove(0, closingSingleQuoteIndex + 1);
		src = src.trimmed();
		if (src.startsWith(delim))
			src.remove(0, delim.size());
	}
	else
	{
		QStringList list = src.split(delim);
		if (list.isEmpty())
		{
			// Empty token, treat it as an empty string
			out = "";
		}
		else
		{
			out = list.first().trimmed();
			qint64 size = out.size();

			// Remove the first 'out' and 'delim' from src
			src.remove(0, size + delim.size());
			if (src.startsWith(delim))
				src.remove(0, delim.size());
			src = src.trimmed();
		}
	}

	return true;
}

//檢查引用變量前面是否缺少&符號
void Lexer::checkInvalidReadVariable(const QHash<qint64, TokenMap>& stokenmaps)
{
	QMap<qint64, QString> invalidReadVariables;
	QStringList varNameList;

	QMap <qint64, TokenMap> tokenmaps;//轉成有序容器方便按順序遍歷
	for (auto it = stokenmaps.cbegin(); it != stokenmaps.cend(); ++it)
		tokenmaps.insert(it.key(), it.value());

	//首次先把所有變量聲明找出來紀錄變量名稱
	for (auto it = tokenmaps.cbegin(); it != tokenmaps.cend(); ++it)
	{
		//const qint64 row = it.key();
		const TokenMap& tokenmap = it.value();
		RESERVE type = tokenmap.value(0).type;

		if (type == TK_VARDECL || type == TK_FORMAT || type == TK_RND)
		{
			QString varName = tokenmap.value(1).data.toString().simplified();
			if (type == TK_FORMAT)
			{
				if (varName == "out" || varName == "say")
					continue;
			}

			if (!varNameList.contains(varName))
				varNameList.append(varName);
		}
		else if (type == TK_MULTIVAR)
		{
			QStringList varNames = tokenmap.value(0).data.toString().simplified().split(util::rexComma, Qt::SkipEmptyParts);
			if (varNames.isEmpty())
				continue;

			for (const QString& varName : varNames)
			{
				if (!varNameList.contains(varName))
					varNameList.append(varName);
			}
		}
	}

	static const QStringList systemVarList = {
		//utf8
		"tick", "stick", "chname", "chfname", "chlv", "chhp", "chmp", "chdp", "stone", "px", "py",
			"floor", "frname", "date", "time", "earnstone", "expbuff", "dlgid", "bt"
	};

	varNameList.append(systemVarList);


	//for (auto it = tokenmaps.cbegin(); it != tokenmaps.cend(); ++it)
	//{
	//	const qint64 row = it.key();
	//	const TokenMap tokenmap = it.value();
	//	for (auto subit = std::next(tokenmap.cbegin()); subit != tokenmap.cend(); ++subit)
	//	{
	//		RESERVE curtype = subit.value().type;

	//		QString varName = subit.value().data.toString().simplified();
	//		if (varNameList.contains(varName))
	//			continue;

	//		QString errorMessage = QObject::tr("<Syntax Error>Unexpected using undeclared variable name '%1' at line: %2").arg(varName.mid(0)).arg(row + 1);
	//		showError(errorMessage);
	//	}
	//}

}

void Lexer::checkFunctionPairs(const QHash<qint64, TokenMap>& stokenmaps)
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
		//RESERVE type = it.value().value(0).type;
		QString statement = it.value().value(0).data.toString().simplified();

		if (statement != "function" && statement != "end")
			continue;

		if (statement == "function") // "function" statement
		{
			functionStack.push(row);
		}
		else if (statement == "end") // "end" statement
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

	// 打印所有不成对的 "function" 语句
	for (auto it = unpairedFunctions.cbegin(); it != unpairedFunctions.cend(); ++it)
	{
		qint64 row = it.key();
		QString statement = it.value();
		QString errorMessage = QObject::tr("<Syntax Error>Missing 'end' for statement '%1' at line: %2").arg(statement).arg(row + 1);
		showError(errorMessage);
	}

	// 打印所有不成对的 "end" 语句
	for (auto it = unpairedEnds.cbegin(); it != unpairedEnds.cend(); ++it)
	{
		qint64 row = it.key();
		QString statement = it.value();
		QString errorMessage = QObject::tr("<Syntax Error>Extra 'end' for statement '%1' at line: %2").arg(statement).arg(row + 1);
		showError(errorMessage);
	}
}

