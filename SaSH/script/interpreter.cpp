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

#include "map/mapdevice.h"
#include <gamedevice.h>
#include "signaldispatcher.h"

//#include "crypto.h"

Interpreter::Interpreter(long long index)
	: Indexer(index)
	, parser_(index)
{
	qDebug() << "Interpreter is created!";
	subThreadFutureSync_.setCancelOnWait(true);
}

Interpreter::~Interpreter()
{
	subThreadFutureSync_.waitForFinished();
	qDebug() << "Interpreter is destroyed!";
}

bool Interpreter::isRunning() const
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	return isRunning_.get()
		&& gamedevice.IS_SCRIPT_FLAG.get() && !gamedevice.IS_SCRIPT_INTERRUPT.get();
}

//在新線程執行腳本文件
void Interpreter::doFileWithThread(long long beginLine, const QString& fileName)
{
	std::ignore = fileName;
	beginLine_ = beginLine;

	moveToThread(&thread_);
	connect(this, &Interpreter::finished, &thread_, &QThread::quit, Qt::QueuedConnection);
	connect(&thread_, &QThread::started, this, &Interpreter::proc, Qt::QueuedConnection);
	thread_.start();
}

//同線程下執行腳本文件(實例新的interpreter)
bool Interpreter::doFile(long long beginLine, const QString& fileName, Interpreter* pinterpretter, Parser* pparser, bool issub, Parser::Mode mode)
{
	parser_.setMode(mode);
	parser_.setScriptFileName(fileName);
	parser_.setSubScript(issub);
	parser_.setInterpreter(pinterpretter);
	parser_.setParent(pparser);

	QString content;
	bool isPrivate = false;
	if (!util::readFile(fileName, &content, &isPrivate))
		return false;

	parser_.setPrivate(isPrivate);

	if (!parser_.loadString(content))
		return false;

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	isPrivate = parser_.isPrivate();
	if (!isPrivate && mode == Parser::kSync)
	{
		emit signalDispatcher.loadFileToTable(fileName);
	}
	else if (isPrivate && mode == Parser::kSync)
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<long long, TokenMap>{}));
	}

	pCallback = std::bind(&Interpreter::scriptCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	parser_.setCallBack(pCallback);
	openLibs();

	parser_.parse(beginLine);
	return true;
}

//新線程下執行一段腳本內容
void Interpreter::doString(QString content)
{
	parser_.initialize(pParentParser_);
	parser_.setMode(Parser::kAsync);
	parser_.setScriptFileName("");
	parser_.setSubScript(true);
	parser_.setInterpreter(pParentInterpreter_);
	parser_.setParent(pParentParser_);

	content.replace("\r\n", "\n");
	content.replace("\\r\\n", "\n");
	content.replace("\\n", "\n");
	content.replace("\\t", "\t");
	content.replace("\\v", "\v");
	content.replace("\\b", "\b");
	content.replace("\\f", "\f");
	content.replace("\\a", "\a");

	if (!parser_.loadString(content))
		return;

	openLibs();

	moveToThread(&thread_);
	connect(this, &Interpreter::finished, &thread_, &QThread::quit, Qt::QueuedConnection);
	connect(&thread_, &QThread::started, this, &Interpreter::onRunString, Qt::QueuedConnection);
	thread_.start();
}

void Interpreter::onRunString()
{
	safe::auto_flag autoSafeflag(&isRunning_);
	parser_.parse(0);
	emit finished();
}

//先行解析token並發送給UI顯示
void Interpreter::preview(const QString& fileName)
{
	QString content;
	bool isPrivate = false;
	if (!util::readFile(fileName, &content, &isPrivate))
		return;

	parser_.setPrivate(isPrivate);

	if (!parser_.loadString(content))
		return;

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (parser_.isPrivate())
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<long long, TokenMap>{}));
		return;
	}

	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(parser_.getTokens()));
}

//註冊interpreter的成員函數
template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_.registerFunction(functionName, std::bind(fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

//嘗試取指定位置的token轉為字符串
bool Interpreter::checkString(const TokenMap& TK, long long idx, QString* ret)
{
	return parser_.checkString(TK, idx, ret);
}

//嘗試取指定位置的token轉為整數
bool Interpreter::checkInteger(const TokenMap& TK, long long idx, long long* ret)
{
	return parser_.checkInteger(TK, idx, ret);
}

//檢查跳轉是否滿足，和跳轉的方式
long long Interpreter::checkJump(const TokenMap& TK, long long idx, bool expr, JumpBehavior behavior)
{
	return parser_.checkJump(TK, idx, expr, behavior);
}

//檢查指定位置開始的兩個參數是否能作為範圍值或指定位置的值
bool Interpreter::checkRange(const TokenMap& TK, long long idx, long long* min, long long* max)
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
		long long value = var.toLongLong(&ok);
		if (!ok)
			return false;
		*min = value - 1;
		*max = value - 1;
		return true;
	}
	else if (type == TK_STRING || type == TK_CSTRING)
	{
		long long value = 0;
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
	long long valueMin = list.value(0).toLongLong(&ok);
	if (!ok)
		return false;

	long long valueMax = list.value(1).toLongLong(&ok);
	if (!ok)
		return false;

	if (valueMin > valueMax)
		return false;

	*min = valueMin - 1;
	*max = valueMax - 1;

	return true;
}

//檢查是否為邏輯運算符
bool Interpreter::checkRelationalOperator(const TokenMap& TK, long long idx, RESERVE* ret) const
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
bool Interpreter::waitfor(long long timeout, std::function<bool()> exprfun) const
{
	if (nullptr == exprfun)
		return false;

	if (timeout < 0)
		timeout = std::numeric_limits<long long>::max();
	else if (timeout == 0)
		return exprfun();

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	bool bret = false;
	util::timer timer;
	long long delay = timeout / 10;
	if (delay > 100)
		delay = 100;

	for (;;)
	{
		gamedevice.checkScriptPause();

		if (gamedevice.IS_SCRIPT_INTERRUPT.get())
			break;

		if (gamedevice.worker.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		if (timer.hasExpired(timeout))
			break;

		QThread::msleep(delay);
	}
	return bret;
}

void Interpreter::openLibs()
{
	/*註册函数*/

	//system
	registerFunction("run", &Interpreter::run);
	registerFunction("dostr", &Interpreter::dostr);

	registerFunction("執行", &Interpreter::run);
	registerFunction("執行代碼", &Interpreter::dostr);

	registerFunction("执行", &Interpreter::run);
	registerFunction("执行代码", &Interpreter::dostr);
}

long long Interpreter::scriptCallBack(long long currentIndex, long long currentLine, const TokenMap& TK)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		return 0;

	RESERVE currentType = TK.value(0).type;

	bool skip = currentType == RESERVE::TK_WHITESPACE
		|| currentType == RESERVE::TK_COMMENT
		|| currentType == RESERVE::TK_UNK;

	if (skip)
	{
		QThread::yieldCurrentThread();
		return 1;
	}

	if (parser_.getMode() == Parser::kSync)
		emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, parser_.getToken().size(), false);

	if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
	{
		gamedevice.paused();
		emit signalDispatcher.scriptPaused();
	}

	gamedevice.checkScriptPause();

	if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		return 0;

	if (!gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
	{
		QThread::yieldCurrentThread();
		return 1;
	}

	QString scriptFileName = parser_.getScriptFileName();
	safe::hash<long long, break_marker_t> breakMarkers = gamedevice.break_markers.value(scriptFileName);
	const safe::hash<long long, break_marker_t> stepMarkers = gamedevice.step_markers.value(scriptFileName);
	if (!breakMarkers.contains(currentLine) && !stepMarkers.contains(currentLine))
	{
		QThread::yieldCurrentThread();
		return 1;//檢查是否有中斷點
	}

	gamedevice.paused();

	if (breakMarkers.contains(currentLine))
	{
		//叫用次數+1
		break_marker_t mark = breakMarkers.value(currentLine);
		++mark.count;

		//重新插入斷下的紀錄
		breakMarkers.insert(currentLine, mark);
		gamedevice.break_markers.insert(scriptFileName, breakMarkers);
		//所有行插入隱式斷點(用於單步)
		emit signalDispatcher.addStepMarker(currentLine, true);
		emit signalDispatcher.breakMarkInfoImport();
	}

	emit signalDispatcher.addForwardMarker(currentLine, true);

	gamedevice.checkScriptPause();

	if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		return 0;

	QThread::yieldCurrentThread();
	return 1;
}

void Interpreter::proc()
{
	safe::auto_flag autoSafeflag(&isRunning_);
	qDebug() << "Interpreter::run()";

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	gamedevice.IS_SCRIPT_FLAG.on();
	gamedevice.IS_SCRIPT_INTERRUPT.off();

	parser_.initialize(&parser_);

	doFile(beginLine_, gamedevice.currentScriptFileName.get(), this, nullptr, false, Parser::kSync);

	gamedevice.IS_SCRIPT_FLAG.off();

	subInterpreterList_.clear();
	subThreadFutureSync_.waitForFinished();

	emit finished();
	emit signalDispatcher.scriptFinished();
}

//檢查是否戰鬥，如果是則等待，並在戰鬥結束後停滯一段時間
bool Interpreter::checkBattleThenWait()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	gamedevice.checkScriptPause();

	if (gamedevice.worker.isNull())
		return false;

	bool bret = false;
	if (gamedevice.worker->getBattleFlag())
	{
		util::timer timer;
		bret = true;
		for (;;)
		{
			if (gamedevice.IS_SCRIPT_INTERRUPT.get())
				break;

			if (gamedevice.worker.isNull())
				break;

			gamedevice.checkScriptPause();

			if (!gamedevice.worker->getBattleFlag())
				break;

			if (timer.hasExpired(180000))
				break;

			QThread::msleep(200);
		}

		QThread::msleep(1000);
	}

	return bret;
}

bool Interpreter::checkOnlineThenWait()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	gamedevice.checkScriptPause();

	if (gamedevice.worker.isNull())
		return false;

	bool bret = false;

	if (!gamedevice.worker->getOnlineFlag())
	{
		util::timer timer;
		bret = true;
		for (;;)
		{
			if (gamedevice.IS_SCRIPT_INTERRUPT.get())
				break;

			if (gamedevice.worker.isNull())
				break;

			gamedevice.checkScriptPause();

			if (gamedevice.worker->getOnlineFlag())
				break;

			if (timer.hasExpired(180000))
				break;

			QThread::msleep(200);
		}

		QThread::msleep(1000);
	}

	return bret;
}

//執行子腳本
long long Interpreter::run(long long currentIndex, long long currentline, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	QString fileName;
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
	{
		errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("File name expected but got nothing"));
		return Parser::kArgError + 1ll;
	}

	VarShareMode varShareMode = kNotShare;
	long long nShared = 0;
	checkInteger(TK, 2, &nShared);
	if (nShared > 0)
		varShareMode = kShare;

	long long beginLine = 0;
	checkInteger(TK, 3, &beginLine);
	--beginLine;
	if (beginLine < 0)
		beginLine = 0;

	Parser::Mode asyncMode = Parser::kSync;
	long long nAsync = 0;
	checkInteger(TK, 4, &nAsync);
	if (nAsync > 0)
		asyncMode = Parser::kAsync;

	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	if (!fileInfo.isAbsolute())
	{
		//取主腳本路徑
		QString currentFileName = parser_.getScriptFileName();
		QFileInfo mainScriptFileInfo(currentFileName);

		//take directory only
		QString currentDir = mainScriptFileInfo.absolutePath();

		fileName = currentDir + "/" + fileName;
		fileName.replace("\\", "/");
		fileName.replace("//", "/");
	}

	fileInfo = QFileInfo(fileName);
	if (fileInfo.isDir())
	{
		errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("Invalid path of file '%1' expected a file but got a directory").arg(fileName));
		return Parser::kArgError + 1ll;
	}

	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".txt";
	else if (suffix != "txt" && suffix != "sac")
	{
		errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("Invalid suffix of file '%1'").arg(suffix));
		return Parser::kArgError + 1ll;
	}

	//如果文件不存在則嘗試遍歷搜索腳本
	if (!QFile::exists(fileName))
	{
		QStringList paths;
		fileName = fileInfo.fileName();
		QString withoutsuffix = fileName;
		withoutsuffix.remove(fileInfo.suffix());
		util::searchFiles(util::applicationDirPath(), fileName, "txt", &paths, false);
		if (paths.isEmpty())
		{
			errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("original path '%1' of file not exist, try to auto search but found nothing").arg(fileName));
			return Parser::kArgError + 1ll;
		}

		fileName = paths.first();
		errorExport(currentIndex, currentline, WARN_LEVEL, QObject::tr("[warn]original path '%1' of file not exist, auto search and found file at '%2'").arg(fileName));
	}

	if (Parser::kSync == asyncMode)
	{
		long long nret = Parser::kNoChange;

		//紀錄當前數據
		QHash<long long, TokenMap> tokens = parser_.getTokens();
		QHash<QString, long long> labels = parser_.getLabels();
		QList<FunctionNode> functionNodeList = parser_.getFunctionNodeList();
		QList<ForNode> forNodeList = parser_.getForNodeList();
		QList<LuaNode> luaNodeList_ = parser_.getLuaNodeList();
		QString currentFileName = parser_.getScriptFileName();
		long long currentLine = parser_.getCurrentLine();

		std::unique_ptr<Interpreter> interpreter(q_check_ptr(new Interpreter(currentIndex)));
		sash_assume(interpreter != nullptr);
		if (interpreter == nullptr)
			return Parser::kError;

		interpreter->parser_.setSubScript(true);

		gamedevice.currentScriptFileName.set(fileName);

		if (varShareMode == kShare)
		{
			interpreter->parser_.initialize(&parser_);
			interpreter->parser_.setLuaMachinePointer(parser_.pLua_);
			interpreter->parser_.setGlobalNameListPointer(parser_.getGlobalNameListPointer());
			interpreter->parser_.setCounterPointer(parser_.getCounterPointer());
			interpreter->parser_.setLuaLocalVarStringListPointer(parser_.getLuaLocalVarStringListPointer());
			interpreter->parser_.setLocalVarStackPointer(parser_.getLocalVarStackPointer());
		}
		else
			interpreter->parser_.initialize(&parser_);

		if (!interpreter->doFile(beginLine, fileName, this, &parser_, true, asyncMode))
		{
			nret = Parser::kError;
		}

		//還原顯示
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.loadFileToTable(currentFileName);

		//還原數據
		gamedevice.currentScriptFileName.set(currentFileName);
		parser_.setTokens(tokens);
		parser_.setLabels(labels);
		parser_.setFunctionNodeList(functionNodeList);
		parser_.setForNodeList(forNodeList);
		parser_.setLuaNodeList(luaNodeList_);
		parser_.setCurrentLine(currentLine);
		return nret;
	}
	else
	{
		subThreadFutureSync_.addFuture(QtConcurrent::run([this, beginLine, fileName, varShareMode, asyncMode, currentIndex]()->bool
			{
				std::unique_ptr<Interpreter> interpreter(q_check_ptr(new Interpreter(currentIndex)));
				sash_assume(interpreter != nullptr);
				if (nullptr == interpreter)
					return false;

				interpreter->parser_.setSubScript(true);
				interpreter->parser_.initialize(&parser_);
				if (interpreter->doFile(beginLine, fileName, this, &parser_, true, asyncMode))
					return true;

				return false;
			}));

		return Parser::kNoChange;
	}
}

//執行代碼塊
long long Interpreter::dostr(long long currentIndex, long long currentline, const TokenMap& TK)
{
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.worker.isNull())
		return Parser::kServerNotReady;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
	{
		errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("String expected but got nothing"));
		return Parser::kArgError + 1ll;
	}

	std::unique_ptr<Interpreter> interpreter(q_check_ptr(new Interpreter(currentIndex)));
	sash_assume(interpreter != nullptr);
	if (nullptr == interpreter)
		return Parser::kError;

	interpreter->setParentParser(&parser_);
	interpreter->setParentInterpreter(this);
	interpreter->doString(text);
	subInterpreterList_.append(std::move(interpreter));
	return Parser::kNoChange;
}

//打印日誌
void Interpreter::logExport(long long currentIndex, long long currentline, const QString& data, long long color)
{
	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";


	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline + 1, 3, 10, QLatin1Char(' ')).arg(data));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendScriptLog(msg, color);
}

void Interpreter::errorExport(long long currentIndex, long long currentLine, long long level, const QString& data)
{
	QString newText = QString("%1%2").arg(level == WARN_LEVEL ? QObject::tr("[warn]") : QObject::tr("[error]")).arg(data);
	logExport(currentIndex, currentLine, newText, 0);
}