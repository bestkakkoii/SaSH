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

import Utility;
import String;
#include "stdafx.h"
#include "interpreter.h"
#include "injector.h"

#include "signaldispatcher.h"

__int64 Interpreter::press(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkBattleThenWait();

	QString text;
	__int64 row = 0;
	if (!checkInteger(TK, 1, &row))
	{
		if (!checkString(TK, 1, &text))
			return Parser::kArgError + 1ll;
		if (text.isEmpty())
			return Parser::kArgError + 1ll;
	}

	QString npcName;
	__int64 npcId = -1;
	checkString(TK, 2, &npcName);

	mapunit_t unit;
	if (!npcName.isEmpty() && injector.server->findUnit(npcName, OBJ_NPC, &unit))
		npcId = unit.id;

	__int64 dialogid = -1;
	checkInteger(TK, 3, &dialogid);

	__int64 ext = 0;
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

				for (__int64 i = 0; i < textList.size(); ++i)
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

///////////////////////////////////////////////////////////////

__int64 Interpreter::ocr(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 debugmode = 0;
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
__int64 Interpreter::send(__int64 currentIndex, __int64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	__int64 size = TK.size();

	__int64 funId = -1;
	if (!checkInteger(TK, 1, &funId))
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("function type must be a number"));
		return Parser::kArgError + 1ll;
	}
	if (funId <= 0)
	{
		errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("function type must be a positive number"));
		return Parser::kArgError + 1ll;
	}

	std::vector<std::variant<int, std::string>> args;

	for (__int64 i = 2; i < size; ++i)
	{
		__int64 varIntValue;
		QString varStringValue;
		std::variant<int, std::string> var;

		if (!checkInteger(TK, i, &varIntValue))
		{
			if (!checkString(TK, i, &varStringValue))
			{
				errorExport(currentIndex, currentLine, ERROR_LEVEL, QObject::tr("argument %1 must be a number or a string").arg(i - 1));
				return Parser::kArgError + i;
			}
			args.emplace_back(fromQUnicode(varStringValue));
		}
		else
		{
			args.emplace_back(static_cast<int>(varIntValue));
		}
	}

	injector.autil.util_SendArgs(static_cast<int>(funId), args);

	return Parser::kNoChange;
}