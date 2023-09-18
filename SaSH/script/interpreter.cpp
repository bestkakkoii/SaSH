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

#include "map/mapanalyzer.h"
#include "injector.h"
#include "signaldispatcher.h"

//#include "crypto.h"

#include <spdlogger.hpp>

util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> break_markers;//用於標記自訂義中斷點(紅點)
util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> forward_markers;//用於標示當前執行中斷處(黃箭頭)
util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> error_markers;//用於標示錯誤發生行(紅線)
util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> step_markers;//隱式標記中斷點用於單步執行(無)

QString g_logger_name = "";

Interpreter::Interpreter()
	: ThreadPlugin(nullptr)
{
	qDebug() << "Interpreter is created!";
	futureSync_.setCancelOnWait(true);
}

Interpreter::~Interpreter()
{
	requestInterruption();
	futureSync_.waitForFinished();
	if (thread_)
	{
		thread_->quit();
		thread_->wait();
	}
	qDebug() << "Interpreter is destroyed!";
}

//在新線程執行腳本文件
void Interpreter::doFileWithThread(qint64 beginLine, const QString& fileName)
{
	if (thread_ != nullptr)
		return;

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	if (!parser_.loadFile(fileName, nullptr))
		return;

	parser_.setBeginLine(beginLine);

	moveToThread(thread_);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(this, &Interpreter::finished, thread_, &QThread::quit, Qt::UniqueConnection);
	connect(thread_, &QThread::finished, thread_, &QThread::deleteLater, Qt::UniqueConnection);
	connect(thread_, &QThread::started, this, &Interpreter::proc, Qt::UniqueConnection);
	connect(this, &Interpreter::finished, this, [this]()
		{
			thread_ = nullptr;
			qDebug() << "Interpreter::finished";
		}, Qt::UniqueConnection);

	thread_->start();
}

//同線程下執行腳本文件(實例新的interpreter)
bool Interpreter::doFile(qint64 beginLine, const QString& fileName, Interpreter* parent, VarShareMode shareMode, RunFileMode mode)
{
	if (mode == kAsync)
		parser_.setMode(Parser::kAsync);

	if (!parser_.loadFile(fileName, nullptr))
		return false;

	isRunning_.store(true, std::memory_order_release);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	bool isPrivate = parser_.isPrivate();
	if (!isPrivate && mode == kSync)
	{
		emit signalDispatcher.loadFileToTable(fileName);
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(parser_.getTokens()));
	}
	else if (isPrivate && mode == kSync)
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<qint64, TokenMap>{}));
	}

	parser_.setScriptFileName(fileName);

	pCallback = [parent, &signalDispatcher, mode, this](qint64 currentLine, const TokenMap& TK)->qint64
	{
		if (parent == nullptr)
			return 0;

		if (parent->isInterruptionRequested())
			return 0;

		if (isInterruptionRequested())
			return 0;

		RESERVE currentType = TK.value(0).type;

		bool skip = currentType == RESERVE::TK_WHITESPACE
			|| currentType == RESERVE::TK_COMMENT
			|| currentType == RESERVE::TK_UNK;

		if (skip)
			return 1;

		Injector& injector = Injector::getInstance();

		if (mode == kSync)
			emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, this->parser_.getToken().size(), false);

		if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
		{
			parent->paused();
			emit signalDispatcher.scriptPaused();
		}

		parent->checkPause();

		if (mode == kSync)
		{
			QString scriptFileName = parser_.getScriptFileName();
			util::SafeHash<qint64, break_marker_t> breakMarkers = break_markers.value(scriptFileName);
			const util::SafeHash<qint64, break_marker_t> stepMarkers = step_markers.value(scriptFileName);
			if (!(breakMarkers.contains(currentLine) || stepMarkers.contains(currentLine)))
			{
				return 1;//檢查是否有中斷點
			}

			parent->paused();

			if (breakMarkers.contains(currentLine))
			{
				//叫用次數+1
				break_marker_t mark = breakMarkers.value(currentLine);
				++mark.count;

				//重新插入斷下的紀錄
				breakMarkers.insert(currentLine, mark);
				break_markers.insert(scriptFileName, breakMarkers);
				//所有行插入隱式斷點(用於單步)
				emit signalDispatcher.addStepMarker(currentLine, true);
			}

			emit signalDispatcher.addForwardMarker(currentLine, true);

			parent->checkPause();
		}

		return 1;
	};

	if (shareMode == kShare)
	{
		VariantSafeHash* pparentHash = parent->parser_.getGlobalVarPointer();
		QReadWriteLock* pparentLock = parent->parser_.getGlobalVarLockPointer();
		if (pparentHash != nullptr && pparentLock != nullptr)
			parser_.setVariablesPointer(pparentHash, pparentLock);
	}

	parser_.setCallBack(pCallback);
	openLibs();

	parser_.parse(beginLine);

	isRunning_.store(false, std::memory_order_release);
	return true;
}

//新線程下執行一段腳本內容
void Interpreter::doString(const QString& content, Interpreter* parent, VarShareMode shareMode)
{
	parser_.setMode(Parser::kAsync);

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	QString newString = content;
	newString.replace("\\r", "\r");
	newString.replace("\\n", "\n");
	newString.replace("\r\n", "\n");
	newString.replace("\r", "\n");

	setSubScript(true);


	if (!parser_.loadString(content))
		return;

	isRunning_.store(true, std::memory_order_release);

	parser_.setBeginLine(0);

	if (parent)
	{
		pCallback = [parent, this](qint64 currentLine, const TokenMap& TK)->qint64
		{
			if (parent->isInterruptionRequested())
				return 0;

			if (isInterruptionRequested())
				return 0;

			RESERVE currentType = TK.value(0).type;

			bool skip = currentType == RESERVE::TK_WHITESPACE
				|| currentType == RESERVE::TK_COMMENT
				|| currentType == RESERVE::TK_UNK;

			if (skip)
				return 1;

			if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
			{
				parent->paused();
				SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
				emit signalDispatcher.scriptPaused();
			}

			parent->checkPause();

			return 1;
		};

		parser_.setCallBack(pCallback);

		if (shareMode == kShare)
		{
			VariantSafeHash* pparentHash = parent->parser_.getGlobalVarPointer();
			QReadWriteLock* pparentLock = parent->parser_.getGlobalVarLockPointer();
			if (pparentHash != nullptr && pparentLock != nullptr)
				parser_.setVariablesPointer(pparentHash, pparentLock);
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	openLibs();

	QtConcurrent::run([this]()
		{
			isRunning_.store(true, std::memory_order_release);
			parser_.parse(0);
			isRunning_.store(false, std::memory_order_release);
		});
}

//先行解析token並發送給UI顯示
void Interpreter::preview(const QString& fileName)
{
	if (!parser_.loadFile(fileName, nullptr))
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (parser_.isPrivate())
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<qint64, TokenMap>{}));
		return;
	}

	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(parser_.getTokens()));
}

void Interpreter::stop()
{
	emit stoped();
	requestInterruption();
}

//註冊interpreter的成員函數
template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_.registerFunction(functionName, std::bind(fun, this, std::placeholders::_1, std::placeholders::_2));
}

//嘗試取指定位置的token轉為字符串
bool Interpreter::checkString(const TokenMap& TK, qint64 idx, QString* ret)
{
	return parser_.checkString(TK, idx, ret);
}

//嘗試取指定位置的token轉為整數
bool Interpreter::checkInteger(const TokenMap& TK, qint64 idx, qint64* ret)
{
	return parser_.checkInteger(TK, idx, ret);
}

//嘗試取指定位置的token轉為 double
bool Interpreter::checkNumber(const TokenMap& TK, qint64 idx, double* ret)
{
	return parser_.checkNumber(TK, idx, ret);
}

//嘗試取指定位置的token轉為 bool
bool Interpreter::checkBoolean(const TokenMap& TK, qint64 idx, bool* ret)
{
	return parser_.checkBoolean(TK, idx, ret);
}

//嘗試取指定位置的token轉為QVariant
bool Interpreter::toVariant(const TokenMap& TK, qint64 idx, QVariant* ret)
{
	return parser_.toVariant(TK, idx, ret);
}

//檢查跳轉是否滿足，和跳轉的方式
qint64 Interpreter::checkJump(const TokenMap& TK, qint64 idx, bool expr, JumpBehavior behavior)
{
	return parser_.checkJump(TK, idx, expr, behavior);
}

//檢查指定位置開始的兩個參數是否能作為範圍值或指定位置的值
bool Interpreter::checkRange(const TokenMap& TK, qint64 idx, qint64* min, qint64* max)
{
	if (!TK.contains(idx))
		return false;
	if (min == nullptr || max == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	if (type == TK_FUZZY)
	{
		return true;
	}

	QVariant var = parser_.checkValue(TK, idx);
	if (!var.isValid())
		return false;

	QString range;
	if (type == TK_INT)
	{
		bool ok = false;
		qint64 value = var.toInt(&ok);
		if (!ok)
			return false;
		*min = value - 1;
		*max = value - 1;
		return true;
	}
	else if (type == TK_STRING || type == TK_CSTRING)
	{
		qint64 value = 0;
		if (!checkInteger(TK, idx, &value))
		{
			if (!checkString(TK, idx, &range))
				return false;
		}
		else
		{
			*min = value - 1;
			*max = value - 1;
			return true;
		}
	}
	else
		return false;

	if (range.isEmpty())
		return false;


	QStringList list = range.split(util::rexDec);
	if (list.size() != 2)
		return false;

	bool ok = false;
	qint64 valueMin = list.at(0).toInt(&ok);
	if (!ok)
		return false;

	qint64 valueMax = list.at(1).toInt(&ok);
	if (!ok)
		return false;

	if (valueMin > valueMax)
		return false;

	*min = valueMin - 1;
	*max = valueMax - 1;

	return true;
}

//檢查是否為邏輯運算符
bool Interpreter::checkRelationalOperator(const TokenMap& TK, qint64 idx, RESERVE* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	if (!var.isValid())
		return false;

	if (!relationalOperatorTypes.contains(type))
		return false;

	*ret = type;
	return true;
}

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool Interpreter::waitfor(qint64 timeout, std::function<bool()> exprfun)
{
	if (timeout < 0)
		timeout = std::numeric_limits<qint64>::max();

	Injector& injector = Injector::getInstance();
	bool bret = false;
	QElapsedTimer timer; timer.start();
	for (;;)
	{
		checkPause();
		checkOnlineThenWait();

		if (isInterruptionRequested())
			break;

		if (injector.server.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		if (timer.hasExpired(timeout))
			break;

		QThread::msleep(100);
	}
	return bret;
}

void Interpreter::openLibs()
{
	openLibsBIG5();
	openLibsGB2312();
	openLibsUTF8();
}

//新的繁體中文函數註冊
void Interpreter::openLibsBIG5()
{
	/*註冊函數*/

	//core
	registerFunction(u8"正則匹配", &Interpreter::regex);
	registerFunction(u8"查找", &Interpreter::find);
	registerFunction(u8"半角", &Interpreter::half);
	registerFunction(u8"全角", &Interpreter::full);
	registerFunction(u8"轉大寫", &Interpreter::upper);
	registerFunction(u8"轉小寫", &Interpreter::lower);
	registerFunction(u8"替換", &Interpreter::replace);
	registerFunction(u8"轉整", &Interpreter::toint);
	registerFunction(u8"轉字", &Interpreter::tostr);

	//system
	registerFunction(u8"延時", &Interpreter::sleep);
	registerFunction(u8"按鈕", &Interpreter::press);
	registerFunction(u8"元神歸位", &Interpreter::eo);
	registerFunction(u8"提示", &Interpreter::announce);
	registerFunction(u8"輸入", &Interpreter::input);
	registerFunction(u8"消息", &Interpreter::messagebox);
	registerFunction(u8"回點", &Interpreter::logback);
	registerFunction(u8"登出", &Interpreter::logout);
	registerFunction(u8"說話", &Interpreter::talk);
	registerFunction(u8"說出", &Interpreter::talkandannounce);
	registerFunction(u8"清屏", &Interpreter::cleanchat);
	registerFunction(u8"設置", &Interpreter::set);
	registerFunction(u8"儲存設置", &Interpreter::savesetting);
	registerFunction(u8"讀取設置", &Interpreter::loadsetting);

	registerFunction(u8"執行", &Interpreter::run);
	registerFunction(u8"執行代碼", &Interpreter::dostring);
	registerFunction(u8"註冊", &Interpreter::reg);
	registerFunction(u8"計時", &Interpreter::timer);
	registerFunction(u8"菜單", &Interpreter::menu);
	registerFunction(u8"創建人物", &Interpreter::createch);
	registerFunction(u8"刪除人物", &Interpreter::delch);
	registerFunction(u8"發包", &Interpreter::send);

	//check
	registerFunction(u8"任務狀態", &Interpreter::ifdaily);
	registerFunction(u8"戰鬥中", &Interpreter::ifbattle);
	registerFunction(u8"平時中", &Interpreter::ifnormal);
	registerFunction(u8"在線中", &Interpreter::ifonline);
	registerFunction(u8"查坐標", &Interpreter::ifpos);
	registerFunction(u8"查座標", &Interpreter::ifpos);
	registerFunction(u8"地圖快判", &Interpreter::ifmap);
	registerFunction(u8"背包滿", &Interpreter::ifitemfull);

	registerFunction(u8"地圖", &Interpreter::waitmap);
	registerFunction(u8"對話", &Interpreter::waitdlg);
	registerFunction(u8"聽見", &Interpreter::waitsay);
	registerFunction(u8"寵物有", &Interpreter::waitpet);
	registerFunction(u8"道具", &Interpreter::waititem);
	registerFunction(u8"組隊有", &Interpreter::waitteam);

	//move
	registerFunction(u8"方向", &Interpreter::setdir);
	registerFunction(u8"坐標", &Interpreter::move);
	registerFunction(u8"座標", &Interpreter::move);
	registerFunction(u8"移動", &Interpreter::fastmove);
	registerFunction(u8"尋路", &Interpreter::findpath);
	registerFunction(u8"封包移動", &Interpreter::packetmove);
	registerFunction(u8"移動至NPC", &Interpreter::movetonpc);
	registerFunction(u8"轉移", &Interpreter::teleport);
	registerFunction(u8"過點", &Interpreter::warp);

	//action
	registerFunction(u8"使用道具", &Interpreter::useitem);
	registerFunction(u8"丟棄道具", &Interpreter::dropitem);
	registerFunction(u8"交換道具", &Interpreter::swapitem);
	registerFunction(u8"人物改名", &Interpreter::playerrename);
	registerFunction(u8"寵物改名", &Interpreter::petrename);
	registerFunction(u8"更換寵物", &Interpreter::setpetstate);
	registerFunction(u8"丟棄寵物", &Interpreter::droppet);
	registerFunction(u8"購買", &Interpreter::buy);
	registerFunction(u8"售賣", &Interpreter::sell);
	registerFunction(u8"賣寵", &Interpreter::sellpet);
	registerFunction(u8"加工", &Interpreter::make);
	registerFunction(u8"料理", &Interpreter::cook);
	registerFunction(u8"使用精靈", &Interpreter::usemagic);
	registerFunction(u8"撿物", &Interpreter::pickitem);
	registerFunction(u8"存錢", &Interpreter::depositgold);
	registerFunction(u8"提錢", &Interpreter::withdrawgold);
	registerFunction(u8"加點", &Interpreter::addpoint);
	registerFunction(u8"學習", &Interpreter::learn);
	registerFunction(u8"交易", &Interpreter::trade);
	registerFunction(u8"寄信", &Interpreter::mail);
	registerFunction(u8"丟棄石幣", &Interpreter::doffstone);

	registerFunction(u8"記錄身上裝備", &Interpreter::recordequip);
	registerFunction(u8"裝上記錄裝備", &Interpreter::wearequip);
	registerFunction(u8"卸下裝備", &Interpreter::unwearequip);
	registerFunction(u8"卸下寵裝備", &Interpreter::petunequip);
	registerFunction(u8"裝上寵裝備", &Interpreter::petequip);

	registerFunction(u8"存入寵物", &Interpreter::depositpet);
	registerFunction(u8"存入道具", &Interpreter::deposititem);
	registerFunction(u8"提出寵物", &Interpreter::withdrawpet);
	registerFunction(u8"提出道具", &Interpreter::withdrawitem);

	//action->group
	registerFunction(u8"組隊", &Interpreter::join);
	registerFunction(u8"離隊", &Interpreter::leave);
	registerFunction(u8"踢走", &Interpreter::kick);

	registerFunction(u8"左擊", &Interpreter::leftclick);
	registerFunction(u8"右擊", &Interpreter::rightclick);
	registerFunction(u8"左雙擊", &Interpreter::leftdoubleclick);
	registerFunction(u8"拖至", &Interpreter::mousedragto);

}

//新的簡體中文函數註冊
void Interpreter::openLibsGB2312()
{
	/*註册函数*/

	//core
	registerFunction(u8"正则匹配", &Interpreter::regex);
	registerFunction(u8"查找", &Interpreter::find);
	registerFunction(u8"半角", &Interpreter::half);
	registerFunction(u8"全角", &Interpreter::full);
	registerFunction(u8"转大写", &Interpreter::upper);
	registerFunction(u8"转小写", &Interpreter::lower);
	registerFunction(u8"替换", &Interpreter::replace);
	registerFunction(u8"转整", &Interpreter::toint);
	registerFunction(u8"转字", &Interpreter::tostr);

	//system
	registerFunction(u8"延时", &Interpreter::sleep);
	registerFunction(u8"按钮", &Interpreter::press);
	registerFunction(u8"元神归位", &Interpreter::eo);
	registerFunction(u8"提示", &Interpreter::announce);
	registerFunction(u8"输入", &Interpreter::input);
	registerFunction(u8"消息", &Interpreter::messagebox);
	registerFunction(u8"回点", &Interpreter::logback);
	registerFunction(u8"登出", &Interpreter::logout);
	registerFunction(u8"说话", &Interpreter::talk);
	registerFunction(u8"说出", &Interpreter::talkandannounce);
	registerFunction(u8"清屏", &Interpreter::cleanchat);
	registerFunction(u8"设置", &Interpreter::set);
	registerFunction(u8"储存设置", &Interpreter::savesetting);
	registerFunction(u8"读取设置", &Interpreter::loadsetting);

	registerFunction(u8"执行", &Interpreter::run);
	registerFunction(u8"执行代码", &Interpreter::dostring);
	registerFunction(u8"註册", &Interpreter::reg);
	registerFunction(u8"计时", &Interpreter::timer);
	registerFunction(u8"菜单", &Interpreter::menu);
	registerFunction(u8"创建人物", &Interpreter::createch);
	registerFunction(u8"删除人物", &Interpreter::delch);
	registerFunction(u8"发包", &Interpreter::send);

	//check
	registerFunction(u8"任务状态", &Interpreter::ifdaily);
	registerFunction(u8"战斗中", &Interpreter::ifbattle);
	registerFunction(u8"平时中", &Interpreter::ifnormal);
	registerFunction(u8"在线中", &Interpreter::ifonline);
	registerFunction(u8"查坐标", &Interpreter::ifpos);
	registerFunction(u8"查座标", &Interpreter::ifpos);
	registerFunction(u8"地图", &Interpreter::waitmap);
	registerFunction(u8"地图快判", &Interpreter::ifmap);
	registerFunction(u8"对话", &Interpreter::waitdlg);
	registerFunction(u8"听见", &Interpreter::waitsay);

	registerFunction(u8"宠物有", &Interpreter::waitpet);
	registerFunction(u8"道具", &Interpreter::waititem);
	registerFunction(u8"背包满", &Interpreter::ifitemfull);
	//check-group
	registerFunction(u8"组队有", &Interpreter::waitteam);


	//move
	registerFunction(u8"方向", &Interpreter::setdir);
	registerFunction(u8"坐标", &Interpreter::move);
	registerFunction(u8"座标", &Interpreter::move);
	registerFunction(u8"移动", &Interpreter::fastmove);
	registerFunction(u8"寻路", &Interpreter::findpath);
	registerFunction(u8"封包移动", &Interpreter::packetmove);
	registerFunction(u8"移动至NPC", &Interpreter::movetonpc);
	registerFunction(u8"转移", &Interpreter::teleport);
	registerFunction(u8"过点", &Interpreter::warp);

	//action
	registerFunction(u8"使用道具", &Interpreter::useitem);
	registerFunction(u8"丢弃道具", &Interpreter::dropitem);
	registerFunction(u8"交换道具", &Interpreter::swapitem);
	registerFunction(u8"人物改名", &Interpreter::playerrename);
	registerFunction(u8"宠物改名", &Interpreter::petrename);
	registerFunction(u8"更换宠物", &Interpreter::setpetstate);
	registerFunction(u8"丢弃宠物", &Interpreter::droppet);
	registerFunction(u8"购买", &Interpreter::buy);
	registerFunction(u8"售卖", &Interpreter::sell);
	registerFunction(u8"卖宠", &Interpreter::sellpet);
	registerFunction(u8"加工", &Interpreter::make);
	registerFunction(u8"料理", &Interpreter::cook);
	registerFunction(u8"使用精灵", &Interpreter::usemagic);
	registerFunction(u8"捡物", &Interpreter::pickitem);
	registerFunction(u8"存钱", &Interpreter::depositgold);
	registerFunction(u8"提钱", &Interpreter::withdrawgold);
	registerFunction(u8"加点", &Interpreter::addpoint);
	registerFunction(u8"学习", &Interpreter::learn);
	registerFunction(u8"交易", &Interpreter::trade);
	registerFunction(u8"寄信", &Interpreter::mail);
	registerFunction(u8"丢弃石币", &Interpreter::doffstone);

	registerFunction(u8"记录身上装备", &Interpreter::recordequip);
	registerFunction(u8"装上记录装备", &Interpreter::wearequip);
	registerFunction(u8"卸下装备", &Interpreter::unwearequip);
	registerFunction(u8"卸下宠装备", &Interpreter::petunequip);
	registerFunction(u8"装上宠装备", &Interpreter::petequip);

	registerFunction(u8"存入宠物", &Interpreter::depositpet);
	registerFunction(u8"存入道具", &Interpreter::deposititem);
	registerFunction(u8"提出宠物", &Interpreter::withdrawpet);
	registerFunction(u8"提出道具", &Interpreter::withdrawitem);

	//action->group
	registerFunction(u8"组队", &Interpreter::join);
	registerFunction(u8"离队", &Interpreter::leave);
	registerFunction(u8"踢走", &Interpreter::kick);

	registerFunction(u8"左击", &Interpreter::leftclick);
	registerFunction(u8"右击", &Interpreter::rightclick);
	registerFunction(u8"左双击", &Interpreter::leftdoubleclick);
	registerFunction(u8"拖至", &Interpreter::mousedragto);

}

//新的英文函數註冊
void Interpreter::openLibsUTF8()
{
	/*註册函数*/

	//core
	registerFunction(u8"regex", &Interpreter::regex);
	registerFunction(u8"rex", &Interpreter::rex);
	registerFunction(u8"rexg", &Interpreter::rexg);
	registerFunction(u8"find", &Interpreter::find);
	registerFunction(u8"half", &Interpreter::half);
	registerFunction(u8"full", &Interpreter::full);
	registerFunction(u8"upper", &Interpreter::upper);
	registerFunction(u8"lower", &Interpreter::lower);
	registerFunction(u8"replace", &Interpreter::replace);
	registerFunction(u8"toint", &Interpreter::toint);
	registerFunction(u8"tostr", &Interpreter::tostr);

	//system
	registerFunction(u8"sleep", &Interpreter::sleep);
	registerFunction(u8"button", &Interpreter::press);
	registerFunction(u8"eo", &Interpreter::eo);
	registerFunction(u8"print", &Interpreter::announce);
	registerFunction(u8"input", &Interpreter::input);
	registerFunction(u8"msg", &Interpreter::messagebox);
	registerFunction(u8"logback", &Interpreter::logback);
	registerFunction(u8"logout", &Interpreter::logout);
	registerFunction(u8"say", &Interpreter::talk);
	registerFunction(u8"talk", &Interpreter::talkandannounce);
	registerFunction(u8"cls", &Interpreter::cleanchat);
	registerFunction(u8"set", &Interpreter::set);
	registerFunction(u8"saveset", &Interpreter::savesetting);
	registerFunction(u8"loadset", &Interpreter::loadsetting);

	registerFunction(u8"run", &Interpreter::run);
	registerFunction(u8"dostring", &Interpreter::dostring);
	registerFunction(u8"reg", &Interpreter::reg);
	registerFunction(u8"timer", &Interpreter::timer);
	registerFunction(u8"menu", &Interpreter::menu);
	registerFunction(u8"dofile", &Interpreter::dofile);
	registerFunction(u8"createch", &Interpreter::createch);
	registerFunction(u8"delch", &Interpreter::delch);
	registerFunction(u8"send", &Interpreter::send);

	//check
	registerFunction(u8"ifdaily", &Interpreter::ifdaily);
	registerFunction(u8"ifbattle", &Interpreter::ifbattle);
	registerFunction(u8"ifnormal", &Interpreter::ifnormal);
	registerFunction(u8"ifonline", &Interpreter::ifonline);
	registerFunction(u8"ifpos", &Interpreter::ifpos);
	registerFunction(u8"ifmap", &Interpreter::ifmap);
	registerFunction(u8"ifitemfull", &Interpreter::ifitemfull);
	registerFunction(u8"ifitem", &Interpreter::ifitem);

	registerFunction(u8"waitmap", &Interpreter::waitmap);
	registerFunction(u8"waitdlg", &Interpreter::waitdlg);
	registerFunction(u8"waitsay", &Interpreter::waitsay);
	registerFunction(u8"waitpet", &Interpreter::waitpet);
	registerFunction(u8"waititem", &Interpreter::waititem);

	//check-group
	registerFunction(u8"waitteam", &Interpreter::waitteam);


	//move
	registerFunction(u8"dir", &Interpreter::setdir);
	registerFunction(u8"walkpos", &Interpreter::move);
	registerFunction(u8"move", &Interpreter::fastmove);
	registerFunction(u8"findpath", &Interpreter::findpath);
	registerFunction(u8"w", &Interpreter::packetmove);
	registerFunction(u8"movetonpc", &Interpreter::movetonpc);
	registerFunction(u8"chmap", &Interpreter::teleport);
	registerFunction(u8"warp", &Interpreter::warp);

	//action
	registerFunction(u8"useitem", &Interpreter::useitem);
	registerFunction(u8"doffitem", &Interpreter::dropitem);
	registerFunction(u8"swapitem", &Interpreter::swapitem);
	registerFunction(u8"chplayername", &Interpreter::playerrename);
	registerFunction(u8"chpetname", &Interpreter::petrename);
	registerFunction(u8"chpet", &Interpreter::setpetstate);
	registerFunction(u8"doffpet", &Interpreter::droppet);
	registerFunction(u8"buy", &Interpreter::buy);
	registerFunction(u8"sell", &Interpreter::sell);
	registerFunction(u8"sellpet", &Interpreter::sellpet);
	registerFunction(u8"make", &Interpreter::make);
	registerFunction(u8"cook", &Interpreter::cook);
	registerFunction(u8"usemagic", &Interpreter::usemagic);
	registerFunction(u8"pickup", &Interpreter::pickitem);
	registerFunction(u8"save", &Interpreter::depositgold);
	registerFunction(u8"load", &Interpreter::withdrawgold);
	registerFunction(u8"skup", &Interpreter::addpoint);
	registerFunction(u8"learn", &Interpreter::learn);
	registerFunction(u8"trade", &Interpreter::trade);
	registerFunction(u8"mail", &Interpreter::mail);
	registerFunction(u8"doffstone", &Interpreter::doffstone);

	registerFunction(u8"requip", &Interpreter::recordequip);
	registerFunction(u8"wequip", &Interpreter::wearequip);
	registerFunction(u8"uequip", &Interpreter::unwearequip);
	registerFunction(u8"puequip", &Interpreter::petunequip);
	registerFunction(u8"pequip", &Interpreter::petequip);

	registerFunction(u8"putpet", &Interpreter::depositpet);
	registerFunction(u8"put", &Interpreter::deposititem);
	registerFunction(u8"getpet", &Interpreter::withdrawpet);
	registerFunction(u8"get", &Interpreter::withdrawitem);

	//action->group
	registerFunction(u8"join", &Interpreter::join);
	registerFunction(u8"leave", &Interpreter::leave);
	registerFunction(u8"kick", &Interpreter::kick);

	registerFunction(u8"lclick", &Interpreter::leftclick);
	registerFunction(u8"rclick", &Interpreter::rightclick);
	registerFunction(u8"ldbclick", &Interpreter::leftdoubleclick);
	registerFunction(u8"dragto", &Interpreter::mousedragto);
	registerFunction(u8"dlg", &Interpreter::dlg);

	//hide
	registerFunction(u8"ocr", &Interpreter::ocr);

	//battle
	registerFunction(u8"bh", &Interpreter::bh);//atk
	registerFunction(u8"bj", &Interpreter::bj);//magic
	registerFunction(u8"bp", &Interpreter::bp);//skill
	registerFunction(u8"bs", &Interpreter::bs);//switch
	registerFunction(u8"be", &Interpreter::be);//escape
	registerFunction(u8"bd", &Interpreter::bd);//defense
	registerFunction(u8"bi", &Interpreter::bi);//item
	registerFunction(u8"bt", &Interpreter::bt);//catch
	registerFunction(u8"bn", &Interpreter::bn);//nothing
	registerFunction(u8"bw", &Interpreter::bw);//petskill
	registerFunction(u8"bwf", &Interpreter::bwf);//pet nothing
	registerFunction(u8"bwait", &Interpreter::bwait);
	registerFunction(u8"bend", &Interpreter::bend);
}

qint64 Interpreter::mainScriptCallBack(qint64 currentLine, const TokenMap& TK)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (isInterruptionRequested())
		return 0;

	RESERVE currentType = TK.value(0).type;

	bool skip = currentType == RESERVE::TK_WHITESPACE
		|| currentType == RESERVE::TK_COMMENT
		|| currentType == RESERVE::TK_UNK;

	if (skip)
		return 1;

	Injector& injector = Injector::getInstance();

	bool isDebug = injector.isScriptDebugModeEnable.load(std::memory_order_acquire);

	emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, parser_.getLineCount(), false);

	if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
	{
		paused();
		emit signalDispatcher.scriptPaused();
	}

	checkPause();

	if (!isDebug)
		return 1;

	QString scriptFileName = parser_.getScriptFileName();
	util::SafeHash<qint64, break_marker_t> breakMarkers = break_markers.value(scriptFileName);
	const util::SafeHash<qint64, break_marker_t> stepMarkers = step_markers.value(scriptFileName);
	if (!(breakMarkers.contains(currentLine) || stepMarkers.contains(currentLine)))
	{
		return 1;//檢查是否有中斷點
	}

	paused();

	if (breakMarkers.contains(currentLine))
	{
		//叫用次數+1
		break_marker_t mark = breakMarkers.value(currentLine);
		++mark.count;

		//重新插入斷下的紀錄
		breakMarkers.insert(currentLine, mark);
		break_markers.insert(scriptFileName, breakMarkers);
		//所有行插入隱式斷點(用於單步)
		emit signalDispatcher.addStepMarker(currentLine, true);
	}

	emit signalDispatcher.addForwardMarker(currentLine, true);

	checkPause();

	return 1;
}

void Interpreter::proc()
{
	isRunning_.store(true, std::memory_order_release);
	qDebug() << "Interpreter::run()";
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	pCallback = std::bind(&Interpreter::mainScriptCallBack, this, std::placeholders::_1, std::placeholders::_2);

	do
	{
		if (pCallback == nullptr)
			break;

		if (!isSubScript())
		{
			injector.IS_SCRIPT_FLAG.store(true, std::memory_order_release);
			if (!injector.server.isNull())
			{
				PC pc = injector.server->getPC();
				QString logname = QString("%1_%2_3").arg(pc.name).arg(pc.freeName).arg(_getpid());
				if (logname.isEmpty())
					logname = QString("noname_%1").arg(_getpid());

				g_logger_name = SPD_INIT(logname);

			}
			else
				g_logger_name = SPD_INIT(QString("noname_%1").arg(_getpid()));
		}

		parser_.setCallBack(pCallback);

		openLibs();

		parser_.parse();

	} while (false);

	for (auto& it : subInterpreterList_)
	{
		if (it.isNull())
			continue;

		it->requestInterruption();
	}
	futureSync_.waitForFinished();

	if (!isSubScript())
		injector.IS_SCRIPT_FLAG.store(false, std::memory_order_release);
	isRunning_.store(false, std::memory_order_release);

	emit finished();
	emit signalDispatcher.scriptFinished();

	if (!g_logger_name.isEmpty())
		SPD_CLOSE(g_logger_name.toUtf8().constData());
}

//檢查是否戰鬥，如果是則等待，並在戰鬥結束後停滯一段時間
bool Interpreter::checkBattleThenWait()
{
	checkPause();

	Injector& injector = Injector::getInstance();
	bool bret = false;
	if (injector.server->getBattleFlag())
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested())
				break;

			if (!injector.server.isNull())
				break;

			checkPause();

			if (!injector.server->getBattleFlag())
				break;
			if (timer.hasExpired(60000))
				break;
			QThread::msleep(100);
			QThread::yieldCurrentThread();
		}

		QThread::msleep(500UL);
	}
	return bret;
}

bool Interpreter::checkOnlineThenWait()
{
	checkPause();

	Injector& injector = Injector::getInstance();

	if (!injector.server.isNull())
		return false;

	bool bret = false;

	if (!injector.server->getOnlineFlag())
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested())
				break;

			if (!injector.server.isNull())
				break;

			checkPause();

			if (injector.server->getOnlineFlag())
				break;

			QThread::msleep(100);
		}

		QThread::msleep(2000UL);
	}
	return bret;
}

bool Interpreter::findPath(qint64 currneLine, QPoint dst, qint64 steplen, qint64 step_cost, qint64 timeout, std::function<qint64(QPoint& dst)> callback, bool noAnnounce)
{
	Injector& injector = Injector::getInstance();
	qint64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	QString output = "";

	bool isDebug = injector.getEnableHash(util::kScriptDebugModeEnable);
	if (!isDebug)
		noAnnounce = true;

	auto getPos = [hProcess, hModule, &injector]()->QPoint
	{
		if (!injector.server.isNull())
			return injector.server->getPoint();
		else
			return QPoint();
	};

	if (injector.server.isNull())
		return false;

	qint64 floor = injector.server->nowFloor;
	QPoint src(getPos());
	if (src == dst)
		return true;//已經抵達

	map_t _map;
	QSharedPointer<MapAnalyzer> mapAnalyzer = injector.server->mapAnalyzer;
	if (!mapAnalyzer.isNull() && mapAnalyzer->readFromBinary(floor, injector.server->nowFloorName))
	{
		if (mapAnalyzer.isNull() || !mapAnalyzer->getMapDataByFloor(floor, &_map))
			return false;
	}
	else
		return false;

	if (!noAnnounce && !injector.server.isNull())
	{
		output = QObject::tr("<findpath>start searching the path");
		injector.server->announce(output);//"<尋路>開始搜尋路徑"
		logExport(currneLine, output, 4);
	}

	std::vector<QPoint> path;
	QElapsedTimer timer; timer.start();
	if (mapAnalyzer.isNull() || !mapAnalyzer->calcNewRoute(_map, src, dst, &path))
	{
		output = QObject::tr("[error] <findpath>unable to findpath from %1, %2 to %3, %4").arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y());
		injector.server->announce(output);
		logExport(currneLine, output, 4);

		return false;
	}

	qint64 cost = static_cast<qint64>(timer.elapsed());
	if (!noAnnounce && !injector.server.isNull())
	{
		output = QObject::tr("<findpath>path found, from %1, %2 to %3, %4 cost:%5 step:%6")
			.arg(src.x()).arg(src.y()).arg(dst.x()).arg(dst.y()).arg(cost).arg(path.size());
		injector.server->announce(output);
		logExport(currneLine, output, 4);
	}

	QPoint point;
	qint64 steplen_cache = -1;
	qint64 pathsize = path.size();
	qint64 current_floor = floor;

	timer.restart();

	//用於檢測卡點
	QElapsedTimer blockDetectTimer; blockDetectTimer.start();
	QPoint lastPoint = src;

	for (;;)
	{
		checkOnlineThenWait();

		if (injector.server.isNull())
			break;

		if (isInterruptionRequested())
			break;

		src = getPos();

		steplen_cache = steplen;

		for (;;)
		{
			if (!((steplen_cache) >= (pathsize)))
				break;
			--steplen_cache;
		}

		if (steplen_cache >= 0 && (steplen_cache < pathsize))
		{
			if (lastPoint != src)
			{
				blockDetectTimer.restart();
				lastPoint = src;
			}

			point = path.at(steplen_cache);
			injector.server->move(point);
			if (step_cost > 0)
				QThread::msleep(step_cost);
		}

		if (!checkBattleThenWait())
		{
			src = getPos();
			if (src == dst)
			{
				cost = timer.elapsed();
				if (cost > 5000)
				{
					QThread::msleep(500);

					if (injector.server.isNull())
						break;

					if (getPos() != dst)
						continue;

					injector.server->EO();

					if (getPos() != dst)
						continue;

					QThread::msleep(500);

					if (injector.server.isNull())
						break;

					if (getPos() != dst)
						continue;
				}

				injector.server->move(dst);

				QThread::msleep(100);
				if (getPos() != dst)
					continue;

				if (!noAnnounce && !injector.server.isNull())
				{
					output = QObject::tr("<findpath>arrived destination, cost:%1").arg(cost);
					injector.server->announce(output);
					logExport(currneLine, output, 4);
				}
				return true;//已抵達true
			}

			if (mapAnalyzer.isNull() || !mapAnalyzer->calcNewRoute(_map, src, dst, &path))
				break;

			pathsize = path.size();
		}

		if (blockDetectTimer.hasExpired(5000))
		{
			blockDetectTimer.restart();
			if (injector.server.isNull())
				break;

			output = QObject::tr("[warn] <findpath>detedted player ware blocked");
			injector.server->announce(output);
			logExport(currneLine, output, 4);
			injector.server->EO();
			QThread::msleep(500);

			if (injector.server.isNull())
				break;

			//往隨機8個方向移動
			point = getPos();
			lastPoint = point;
			QPoint stockPoint = getPos();
			for (qint64 i = 0; i < 10; ++i)
			{
				point = point + (util::fix_point.at(QRandomGenerator::global()->bounded(0, 7)) * 5);
				injector.server->move(point);
				QThread::msleep(200);
				checkBattleThenWait();
				src = getPos();
				if (stockPoint != src)
					break;
			}

			continue;
		}

		if (injector.server.isNull())
			break;

		if (isInterruptionRequested())
			break;

		if (timer.hasExpired(timeout))
		{
			if (!injector.server.isNull())
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to timeout");
				injector.server->announce(output);
				logExport(currneLine, output, 4);
			}
			break;
		}

		if (injector.server->nowFloor != current_floor)
		{
			if (!injector.server.isNull())
			{
				output = QObject::tr("[warn] <findpath>stop finding path due to floor changed");
				injector.server->announce(output);
				logExport(currneLine, output, 4);
			}
			break;
		}

		if (callback != nullptr)
		{
			QThread::msleep(50);
			if (callback(dst) == 1)
				callback = nullptr;
		}
	}
	return false;
}

//執行子腳本
qint64 Interpreter::run(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	QString fileName;
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
		return Parser::kArgError + 1ll;

	VarShareMode varShareMode = kNotShare;
	qint64 nShared = 0;
	checkInteger(TK, 2, &nShared);
	if (nShared > 0)
		varShareMode = kShare;

	qint64 beginLine = 0;
	checkInteger(TK, 3, &beginLine);
	--beginLine;
	if (beginLine < 0)
		beginLine = 0;

	RunFileMode asyncMode = kSync;
	qint64 nAsync = 0;
	checkInteger(TK, 4, &nAsync);
	if (nAsync > 0)
		asyncMode = kAsync;

	fileName.replace("\\", "/");

	QFileInfo fileInfo(fileName);
	if (!fileInfo.isAbsolute())
	{
		fileName = util::applicationDirPath() + "/script/" + fileName;
		fileName.replace("\\", "/");
		fileName.replace("//", "/");
	}

	fileInfo = QFileInfo(fileName);
	if (fileInfo.isDir())
		return Parser::kArgError + 1ll;

	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".txt";
	else if (suffix != "txt" && suffix != "sac")
	{
		fileName.replace(suffix, "txt");
	}

	if (!QFile::exists(fileName))
		return Parser::kArgError + 1ll;

	if (kSync == asyncMode)
	{
		QScopedPointer<Interpreter> interpreter(new Interpreter());
		if (!interpreter.isNull())
		{
			interpreter->setSubScript(true);

			QString scriptFileName_ = injector.currentScriptFileName;
			injector.currentScriptFileName = fileName;

			if (!interpreter->doFile(beginLine, fileName, this, varShareMode))
			{
				injector.currentScriptFileName = scriptFileName_;
				return Parser::kError;
			}

			injector.currentScriptFileName = scriptFileName_;

			//還原顯示
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			QHash<qint64, TokenMap>currentToken = parser_.getToken();
			emit signalDispatcher.loadFileToTable(scriptFileName_);
			emit signalDispatcher.scriptContentChanged(scriptFileName_, QVariant::fromValue(currentToken));
		}
	}
	else
	{
		futureSync_.addFuture(QtConcurrent::run([this, beginLine, fileName, varShareMode]()->bool
			{
				QSharedPointer<Interpreter> interpreter(new Interpreter());
				if (!interpreter.isNull())
				{
					subInterpreterList_.append(interpreter);
					interpreter->setSubScript(true);
					if (interpreter->doFile(beginLine, fileName, this, varShareMode, kAsync))
						return true;
				}
				return false;
			}));
	}


	return Parser::kNoChange;
}

//執行代碼塊
qint64 Interpreter::dostring(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
		return Parser::kArgError + 1ll;

	QString script = text;
	script.replace("\\r\\n", "\r\n");
	script.replace("\\n", "\n");
	script.replace("\\t", "\t");
	script.replace("\\v", "\v");
	script.replace("\\b", "\b");
	script.replace("\\f", "\f");
	script.replace("\\a", "\a");

	VarShareMode varShareMode = kNotShare;
	qint64 nShared = 0;
	checkInteger(TK, 2, &nShared);
	if (nShared > 0)
		varShareMode = kShare;

	RunFileMode asyncMode = kSync;
	qint64 nAsync = 0;
	checkInteger(TK, 3, &nAsync);
	if (nAsync > 0)
		asyncMode = kAsync;

	QSharedPointer<Interpreter> interpreter(new Interpreter());
	if (!interpreter.isNull())
	{
		subInterpreterList_.append(interpreter);
		interpreter->setSubScript(true);
		interpreter->doString(script, this, varShareMode);
	}

	if (asyncMode == kSync)
	{
		for (;;)
		{
			if (isInterruptionRequested())
				break;

			if (!interpreter->isRunning())
				break;

			QThread::msleep(100);
		}
	}

	return Parser::kNoChange;
}

#include "script_lua/clua.h"
qint64 Interpreter::dofile(qint64 currentline, const TokenMap& TK)
{
	QString fileName = "";
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
		return Parser::kArgError + 1ll;

	fileName.replace("\\", "/");

	fileName = util::applicationDirPath() + "/script/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".lua";
	else if (suffix != "lua")
	{
		fileName.replace(suffix, "lua");
	}

	QString content;
	bool isPrivate = false;
	if (!util::readFile(fileName, &content, &isPrivate))
		return Parser::kArgError + 1ll;

	QSharedPointer<CLua> lua(new CLua(content));
	connect(this, &Interpreter::stoped, lua.get(), &CLua::requestInterruption, Qt::UniqueConnection);

	lua->start();
	lua->wait();

	return Parser::kNoChange;
}

//打印日誌
void Interpreter::logExport(qint64 currentline, const QString& data, qint64 color)
{

	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";


	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline + 1, 3, 10, QLatin1Char(' ')).arg(data));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.appendScriptLog(msg, color);
}