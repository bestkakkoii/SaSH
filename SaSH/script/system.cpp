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
#include "interpreter.h"
#include "util.h"
#include "injector.h"

#include "signaldispatcher.h"

qint64 Interpreter::press(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkBattleThenWait();

	QString text;
	qint64 row = 0;
	if (!checkInteger(TK, 1, &row))
	{
		if (!checkString(TK, 1, &text))
			return Parser::kArgError + 1ll;
		if (text.isEmpty())
			return Parser::kArgError + 1ll;
	}

	QString npcName;
	qint64 npcId = -1;
	checkString(TK, 2, &npcName);

	mapunit_t unit;
	if (!npcName.isEmpty() && injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
		npcId = unit.id;

	qint64 dialogid = -1;
	checkInteger(TK, 3, &dialogid);

	qint64 ext = 0;
	checkInteger(TK, 4, &ext);

	if (text.isEmpty() && row == 0)
		return Parser::kArgError + 1ll;

	if (!text.isEmpty())
	{
		BUTTON_TYPE button = buttonMap.value(text.toUpper(), BUTTON_NOTUSED);
		if (button != BUTTON_NOTUSED)
			injector.server->press(button, dialogid, npcId + ext);
		else
		{
			dialog_t dialog = injector.server->currentDialog;
			QStringList textList = dialog.linebuttontext;
			if (!textList.isEmpty())
			{
				bool isExact = true;
				QString newText = text.toLower();
				if (newText.startsWith(kFuzzyPrefix))
				{
					newText = newText.mid(1);
					isExact = false;
				}
				else if (newText.startsWith("#"))
				{
					newText = newText.mid(1);
					injector.server->inputtext(newText, dialogid, npcId + ext);
					return Parser::kNoChange;
				}

				for (qint64 i = 0; i < textList.size(); ++i)
				{
					if (!isExact && textList.value(i).toLower().contains(newText))
					{
						injector.server->press(i + 1, dialogid, npcId + ext);
						break;
					}
					else if (isExact && textList.value(i).toLower() == newText)
					{
						injector.server->press(i + 1, dialogid, npcId + ext);
						break;
					}
				}
			}
		}
	}
	else if (row > 0)
		injector.server->press(row, dialogid, npcId + ext);

	return Parser::kNoChange;
}

qint64 Interpreter::menu(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	qint64 index = 0;
	if (!checkInteger(TK, 1, &index))
		return Parser::kArgError + 1ll;
	--index;
	if (index < 0)
		return Parser::kArgError + 1ll;

	qint64 type = 1;
	checkInteger(TK, 2, &type);
	--type;
	if (type < 0 || type > 1)
		return Parser::kArgError + 2ll;

	if (type == 0)
	{
		injector.server->saMenu(index);
	}
	else
	{
		injector.server->shopOk(index);
	}

	return Parser::kNoChange;
}

qint64 Interpreter::createch(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 dataplacenum = -1;
	if (!checkInteger(TK, 1, &dataplacenum))
	{
		QString dataplace;
		if (!checkString(TK, 1, &dataplace))
			return Parser::kArgError + 1ll;
		if (dataplace == "左")
			dataplacenum = 0;
		else if (dataplace == "右")
			dataplacenum = 1;
		else
			return Parser::kArgError + 1ll;
	}
	else
	{
		if (dataplacenum != 1 && dataplacenum != 2)
			return Parser::kArgError + 1ll;
		--dataplacenum;
	}

	QString charname;
	if (!checkString(TK, 2, &charname))
		return Parser::kArgError + 2ll;
	if (charname.isEmpty())
		return Parser::kArgError + 2ll;

	qint64 imgno = 0;
	if (!checkInteger(TK, 3, &imgno))
		imgno = 100050;
	if (imgno <= 0)
		imgno = 100050;

	qint64 faceimgno = 0;
	if (!checkInteger(TK, 4, &faceimgno))
		faceimgno = 30250;
	if (faceimgno <= 0)
		faceimgno = 30250;

	qint64 vit = 0;
	if (!checkInteger(TK, 5, &vit))
		return Parser::kArgError + 5ll;
	if (vit < 0)
		return Parser::kArgError + 5ll;

	qint64 str = 0;
	if (!checkInteger(TK, 6, &str))
		return Parser::kArgError + 6ll;
	if (str < 0)
		return Parser::kArgError + 6ll;

	qint64 tgh = 0;
	if (!checkInteger(TK, 7, &tgh))
		return Parser::kArgError + 7ll;
	if (tgh < 0)
		return Parser::kArgError + 7ll;

	qint64 dex = 0;
	if (!checkInteger(TK, 8, &dex))
		return Parser::kArgError + 8ll;
	if (dex < 0)
		return Parser::kArgError + 8ll;

	if (vit + str + tgh + dex != 20)
		return Parser::kArgError;

	qint64 earth = 0;
	if (!checkInteger(TK, 9, &earth))
		return Parser::kArgError + 9ll;
	if (earth < 0)
		return Parser::kArgError + 9ll;

	qint64 water = 0;
	if (!checkInteger(TK, 10, &water))
		return Parser::kArgError + 10ll;
	if (water < 0)
		return Parser::kArgError + 10ll;

	qint64 fire = 0;
	if (!checkInteger(TK, 11, &fire))
		return Parser::kArgError + 11ll;
	if (fire < 0)
		return Parser::kArgError + 11ll;
	qint64 wind = 0;
	if (!checkInteger(TK, 12, &wind))
		return Parser::kArgError + 12ll;
	if (wind < 0)
		return Parser::kArgError + 12ll;

	if (earth + water + fire + wind != 10)
		return Parser::kArgError;

	qint64 hometown = -1;
	if (!checkInteger(TK, 13, &hometown))
	{
		static const QHash<QString, qint64> hash = {
			{ "薩姆吉爾",0 }, { "瑪麗娜絲", 1 }, { "加加", 2 }, { "卡魯它那", 3 },
			{ "萨姆吉尔",0 }, { "玛丽娜丝", 1 }, { "加加", 2 }, { "卡鲁它那", 3 },

			{ "薩姆吉爾村",0 }, { "瑪麗娜絲村", 1 }, { "加加村", 2 }, { "卡魯它那村", 3 },
			{ "萨姆吉尔村",0 }, { "玛丽娜丝村", 1 }, { "加加村", 2 }, { "卡鲁它那村", 3 },
		};

		QString hometownstr;
		if (!checkString(TK, 13, &hometownstr))
			return Parser::kArgError + 13ll;
		if (hometownstr.isEmpty())
			return Parser::kArgError + 13ll;

		hometown = hash.value(hometownstr, -1);
		if (hometown == -1)
			hometown = 1;
	}
	else
	{
		if (hometown <= 0 || hometown > 4)
			hometown = 1;
		else
			--hometown;
	}

	injector.server->createCharacter(static_cast<int>(dataplacenum)
		, charname
		, static_cast<int>(imgno)
		, static_cast<int>(faceimgno)
		, static_cast<int>(vit)
		, static_cast<int>(str)
		, static_cast<int>(tgh)
		, static_cast<int>(dex)
		, static_cast<int>(earth)
		, static_cast<int>(water)
		, static_cast<int>(fire)
		, static_cast<int>(wind)
		, static_cast<int>(hometown));

	return Parser::kNoChange;
}

qint64 Interpreter::delch(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 index = -1;
	if (!checkInteger(TK, 1, &index))
		return Parser::kArgError + 1ll;
	--index;
	if (index < 0 || index > MAX_CHARACTER)
		return Parser::kArgError + 1ll;

	QString password;
	if (!checkString(TK, 2, &password))
		return Parser::kArgError + 2ll;
	if (password.isEmpty())
		return Parser::kArgError + 2ll;

	qint64 backtofirst = 0;
	checkInteger(TK, 3, &backtofirst);

	injector.server->deleteCharacter(index, password, backtofirst > 0);

	return Parser::kNoChange;
}

///////////////////////////////////////////////////////////////

qint64 Interpreter::ocr(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 debugmode = 0;
	checkInteger(TK, 1, &debugmode);

	QString key = "";
	checkString(TK, 2, &key);
	if (key.isEmpty())
		return Parser::kArgError + 2ll;

	constexpr const char* keyword = "qwertyuiopasdfghjklzxcvbnmQERTYUIOPASDFGHJKLZXCVBNM";
	if (key != keyword)
		return Parser::kArgError + 2ll;

#ifdef OCR_ENABLE
	QString ret;
	if (injector.server->captchaOCR(&ret))
	{
		if (!ret.isEmpty())
		{
			if (debugmode == 0)
				injector.server->inputtext(ret);
		}
	}
#endif

	return Parser::kNoChange;
}

#include "net/autil.h"
qint64 Interpreter::send(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	qint64 size = TK.size();

	qint64 funId = -1;
	if (!checkInteger(TK, 1, &funId))
		return Parser::kArgError + 1ll;
	if (funId <= 0)
		return Parser::kArgError + 1ll;

	std::vector<std::variant<int, std::string>> args;

	for (qint64 i = 2; i < size; ++i)
	{
		qint64 varIntValue;
		QString varStringValue;
		std::variant<int, std::string> var;

		if (!checkInteger(TK, i, &varIntValue))
		{
			if (!checkString(TK, i, &varStringValue))
				return Parser::kArgError + i;
			args.emplace_back(util::fromUnicode(varStringValue));
		}
		else
		{
			args.emplace_back(static_cast<int>(varIntValue));
		}
	}

	injector.autil.util_SendArgs(static_cast<int>(funId), args);

	return Parser::kNoChange;
}