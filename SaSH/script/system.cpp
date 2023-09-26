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

qint64 Interpreter::reg(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	QString text;
	if (!checkString(TK, 1, &text))
		return Parser::kArgError + 1ll;

	QString typeStr;
	if (!checkString(TK, 2, &typeStr))
		return Parser::kArgError + 2ll;

	QHash<QString, qint64> hash = parser_.getLabels();
	if (!hash.contains(text))
		return Parser::kArgError + 1ll;

	parser_.insertUserCallBack(text, typeStr);
	return Parser::kNoChange;
}

qint64 Interpreter::timer(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	QString varName = TK.value(2).data.toString();
	qint64 pointer = 0;
	if (!checkInteger(TK, 1, &pointer))
	{
		return Parser::kArgError + 1ll;
	}
	else if (pointer == 0)
	{
		varName = TK.value(1).data.toString();
		if (!varName.isEmpty())
		{
			QSharedPointer<QElapsedTimer> timer(new QElapsedTimer());
			if (!timer.isNull())
			{
				customTimer_.insert(util::toQString(reinterpret_cast<qint64>(timer.data())), timer);
				timer->start();
				parser_.insertVar(varName, reinterpret_cast<qint64>(timer.data()));
			}
		}
	}
	else if (!varName.isEmpty() && pointer > 0)
	{
		if (customTimer_.contains(util::toQString(pointer)))
		{
			QElapsedTimer* timer = reinterpret_cast<QElapsedTimer*>(pointer);
			qint64 time = 0;

			time = timer->elapsed();

			if (time >= 1000ll)
			{
				parser_.insertVar(varName, time / 1000ll);
				return Parser::kNoChange;
			}
		}
		parser_.insertVar(varName, 0ll);
	}
	else if (pointer > 0)
	{
		if (customTimer_.contains(util::toQString(pointer)))
		{
			customTimer_.remove(util::toQString(pointer));
		}
	}
	else
		return Parser::kArgError;

	return Parser::kNoChange;
}

qint64 Interpreter::sleep(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	qint64 t;
	if (!checkInteger(TK, 1, &t))
		return Parser::kArgError + 1ll;

	if (t >= 1000)
	{
		qint64 i = 0;
		qint64 size = t / 1000;
		for (; i < size; ++i)
		{
			QThread::msleep(1000UL);
			if (isInterruptionRequested())
				break;
		}

		if (i % 1000 > 0)
			QThread::msleep(static_cast<DWORD>(i) % 1000UL);
	}
	else if (t > 0)
		QThread::msleep(static_cast<DWORD>(t));

	return Parser::kNoChange;
}

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
					if (!isExact && textList.at(i).toLower().contains(newText))
					{
						injector.server->press(i + 1, dialogid, npcId + ext);
						break;
					}
					else if (isExact && textList.at(i).toLower() == newText)
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

qint64 Interpreter::eo(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkBattleThenWait();

	QString varName = TK.value(1).data.toString();

	QElapsedTimer timer; timer.start();
	injector.server->EO();
	if (!varName.isEmpty())
	{
		bool bret = waitfor(5000, [currentIndex]() { return !Injector::getInstance(currentIndex).server->isEOTTLSend.load(std::memory_order_acquire); });

		qint64 result = bret ? injector.server->lastEOTime.load(std::memory_order_acquire) : -1;

		parser_.insertVar(varName, result);
	}


	return Parser::kNoChange;
}

qint64 Interpreter::announce(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	QString text;
	qreal number = 0.0;
	qint64 nnumber = 0;
	bool boolean = false;

	if (checkNumber(TK, 1, &number))
	{
		text = util::toQString(number);
	}
	else if (checkBoolean(TK, 1, &boolean))
	{
		text = util::toQString(boolean);
	}
	else if (!checkString(TK, 1, &text))
	{
		if (checkInteger(TK, 1, &nnumber))
			text = util::toQString(nnumber);
		else
			text = TK.value(1).data.toString();
	}

	qint64 color = 4;
	checkInteger(TK, 2, &color);
	if (color == -1)
		color = QRandomGenerator::global()->bounded(0, 10);
	else if (color < -1)
	{
		logExport(currentIndex, currentLine, text, 0);
		return Parser::kNoChange;
	}

	if (!injector.server.isNull())
	{
		injector.server->announce(text, color);
		logExport(currentIndex, currentLine, text, color);
	}
	else if (color != -2)
		logExport(currentIndex, currentLine, text, 0);

	return Parser::kNoChange;
}

qint64 Interpreter::input(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString text;
	qreal number = 0.0;
	qint64 nnumber = 0;
	bool boolean = false;

	if (checkNumber(TK, 1, &number))
	{
		text = util::toQString(number);
	}
	else if (checkBoolean(TK, 1, &boolean))
	{
		text = util::toQString(boolean);
	}
	else if (!checkString(TK, 1, &text))
	{
		if (checkInteger(TK, 1, &nnumber))
			text = util::toQString(nnumber);
		else
			text = TK.value(1).data.toString();
	}

	QString npcName;
	qint64 npcId = -1;
	checkString(TK, 2, &npcName);
	mapunit_t unit;
	if (!npcName.isEmpty() && injector.server->findUnit(npcName, util::OBJ_NPC, &unit))
	{
		npcId = unit.id;
	}

	qint64 dialogid = -1;
	checkInteger(TK, 3, &dialogid);

	injector.server->inputtext(text, dialogid, npcId);

	return Parser::kNoChange;
}

qint64 Interpreter::messagebox(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	QString text;
	qreal number = 0.0;
	qint64 nnumber = 0;
	bool boolean = false;

	if (checkNumber(TK, 1, &number))
	{
		text = util::toQString(number);
	}
	else if (checkBoolean(TK, 1, &boolean))
	{
		text = util::toQString(boolean);
	}
	else if (!checkString(TK, 1, &text))
	{
		if (checkInteger(TK, 1, &nnumber))
			text = util::toQString(nnumber);
		else
			text = TK.value(1).data.toString();
	}

	qint64 type = 0;
	checkInteger(TK, 2, &type);

	QString varName;
	checkString(TK, 3, &varName);


	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	if (varName.isEmpty())
		emit signalDispatcher.messageBoxShow(text, type, nullptr);
	else
	{
		qint64 nret = QMessageBox::StandardButton::NoButton;
		emit signalDispatcher.messageBoxShow(text, type, &nret);
		if (nret != QMessageBox::StandardButton::NoButton)
		{
			parser_.insertVar(varName, nret == QMessageBox::StandardButton::Yes ? "yes" : "no");
		}
	}

	return Parser::kNoChange;
}

qint64 Interpreter::talk(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString text;
	qreal number = 0.0;
	qint64 nnumber = 0;
	bool boolean = false;

	if (checkNumber(TK, 1, &number))
	{
		text = util::toQString(number);
	}
	else if (checkBoolean(TK, 1, &boolean))
	{
		text = util::toQString(boolean);
	}
	else if (!checkString(TK, 1, &text))
	{
		if (checkInteger(TK, 1, &nnumber))
			text = util::toQString(nnumber);
		else
			text = TK.value(1).data.toString();
	}

	qint64 color = 4;
	checkInteger(TK, 2, &color);
	if (color < 0)
		color = QRandomGenerator::global()->bounded(0, 10);

	TalkMode talkmode = kTalkNormal;
	qint64 nTalkMode = 0;
	checkInteger(TK, 3, &nTalkMode);
	if (nTalkMode > 0 && nTalkMode < kTalkModeMax)
	{
		--nTalkMode;
		talkmode = static_cast<TalkMode>(nTalkMode);
	}

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->talk(text, color, talkmode);

	return Parser::kNoChange;
}

qint64 Interpreter::talkandannounce(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	checkOnlineThenWait();
	checkBattleThenWait();

	announce(currentIndex, currentLine, TK);
	return talk(currentIndex, currentLine, TK);
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

qint64 Interpreter::logout(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();

	injector.server->logOut();

	return Parser::kNoChange;
}

qint64 Interpreter::logback(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return Parser::kServerNotReady;

	checkOnlineThenWait();
	checkBattleThenWait();

	injector.server->logBack();

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

qint64 Interpreter::cleanchat(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.server.isNull())
		injector.server->cleanChatHistory();

	return Parser::kNoChange;
}

qint64 Interpreter::savesetting(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	QString fileName;
	if (!checkString(TK, 1, &fileName))
	{
		if (TK.value(1).type == TK_FUZZY)
		{
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return Parser::kServerNotReady;

			fileName = injector.server->getPC().name;
		}
	}

	fileName.replace("\\", "/");

	fileName = util::applicationDirPath() + "/settings/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".json";
	else if (suffix != "json")
	{
		fileName.replace(suffix, "json");
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.saveHashSettings(fileName, true);

	return Parser::kNoChange;
}

qint64 Interpreter::loadsetting(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	QString fileName;
	if (!checkString(TK, 1, &fileName))
	{
		if (TK.value(1).type == TK_FUZZY)
		{
			Injector& injector = Injector::getInstance(currentIndex);
			if (injector.server.isNull())
				return Parser::kServerNotReady;

			fileName = injector.server->getPC().name;
		}
	}

	fileName.replace("\\", "/");

	fileName = util::applicationDirPath() + "/settings/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".json";
	else if (suffix != "json")
	{
		fileName.replace(suffix, "json");
	}

	if (!QFile::exists(fileName))
		return Parser::kArgError + 1ll;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.loadHashSettings(fileName, true);

	return Parser::kNoChange;
}

///////////////////////////////////////////////////////////////

qint64 Interpreter::dlg(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString varName = TK.value(1).data.toString();
	if (varName.isEmpty())
		return Parser::kArgError + 1ll;

	QString buttonStrs;
	checkString(TK, 2, &buttonStrs);
	if (buttonStrs.isEmpty())
		return Parser::kArgError + 2ll;

	QString text;
	if (!checkString(TK, 3, &text))
		return Parser::kArgError + 3ll;

	qint64 timeout = DEFAULT_FUNCTION_TIMEOUT;
	checkInteger(TK, 4, &timeout);

	checkOnlineThenWait();
	checkBattleThenWait();

	text.replace("\\n", "\n");

	buttonStrs = buttonStrs.toUpper();
	QStringList buttonStrList = buttonStrs.split(util::rexOR, Qt::SkipEmptyParts);
	util::SafeVector<qint64> buttonVec;
	quint32 buttonFlag = 0;
	for (const QString& str : buttonStrList)
	{
		if (!buttonMap.contains(str))
			return Parser::kArgError + 2ll;
		quint32 value = buttonMap.value(str);
		buttonFlag |= value;
	}

	injector.server->IS_WAITFOR_CUSTOM_DIALOG_FLAG = true;
	injector.server->createRemoteDialog(buttonFlag, text);
	bool bret = waitfor(timeout, [&injector]() { return !injector.server->IS_WAITFOR_CUSTOM_DIALOG_FLAG; });
	QHash<QString, BUTTON_TYPE> big5 = {
		{ "OK", BUTTON_OK},
		{ "CANCEL", BUTTON_CANCEL },
		//big5
		{ u8"確定", BUTTON_YES },
		{ u8"取消", BUTTON_NO },
		{ u8"上一頁", BUTTON_PREVIOUS },
		{ u8"下一頁", BUTTON_NEXT },
	};

	QHash<QString, BUTTON_TYPE> gb2312 = {
		{ "OK", BUTTON_OK},
		{ "CANCEL", BUTTON_CANCEL },
		//gb2312
		{ u8"确定", BUTTON_YES },
		{ u8"取消", BUTTON_NO },
		{ u8"上一页", BUTTON_PREVIOUS },
		{ u8"下一页", BUTTON_NEXT },
	};
	UINT acp = GetACP();

	customdialog_t dialog = injector.server->customDialog;
	QString type;
	if (acp == 950)
		type = big5.key(dialog.button, "");
	else
		type = gb2312.key(dialog.button, "");

	QVariant result;
	if (type.isEmpty() && dialog.row > 0)
		result = dialog.row;
	else
		result = type;

	parser_.insertVar(varName, result);
	injector.server->IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;
	return checkJump(TK, 6, bret, FailedJump);
}

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