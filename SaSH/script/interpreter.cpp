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

Interpreter::Interpreter(qint64 index)
	: ThreadPlugin(index, nullptr)
	, parser_(index)
{
	qDebug() << "Interpreter is created!";
	futureSync_.setCancelOnWait(true);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, this, &Interpreter::requestInterruption, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllScriptStop, this, &Interpreter::requestInterruption, Qt::UniqueConnection);
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

	beginLine_ = beginLine;

	Injector& injector = Injector::getInstance(getIndex());

	moveToThread(thread_);
	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
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
bool Interpreter::doFile(qint64 beginLine, const QString& fileName, Interpreter* pinterpretter, Parser* pparser, bool issub, Parser::Mode mode)
{
	parser_.setMode(mode);
	parser_.setScriptFileName(fileName);
	parser_.setSubScript(issub);
	parser_.setInterpreter(pinterpretter);
	parser_.setParent(pparser);

	QString content;
	if (!util::readFile(fileName, &content))
		return false;

	if (!parser_.loadString(content))
		return false;

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	bool isPrivate = parser_.isPrivate();
	if (!isPrivate && mode == Parser::kSync)
	{
		emit signalDispatcher.loadFileToTable(fileName);
	}
	else if (isPrivate && mode == Parser::kSync)
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<qint64, TokenMap>{}));
	}

	pCallback = std::bind(&Interpreter::scriptCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	parser_.setCallBack(pCallback);
	openLibs();

	parser_.parse(beginLine);
	return true;
}

//新線程下執行一段腳本內容
void Interpreter::doString(const QString& content, Interpreter* pinterpretter, VarShareMode shareMode)
{
	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	parser_.setMode(Parser::kAsync);
	parser_.setSubScript(true);
	parser_.setInterpreter(this);

	QString newString = content;
	newString.replace("\\r", "\r");
	newString.replace("\\n", "\n");
	newString.replace("\r\n", "\n");
	newString.replace("\r", "\n");

	if (!parser_.loadString(content))
		return;

	isRunning_.store(true, std::memory_order_release);

	parser_.setCurrentLine(0);
	parser_.initialize(&parser_);

	if (pinterpretter != nullptr)
	{
		pCallback = [pinterpretter, this](qint64 currentIndex, qint64 currentLine, const TokenMap& TK)->qint64
		{
			if (pinterpretter->isInterruptionRequested())
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
				pinterpretter->paused();
				SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
				emit signalDispatcher.scriptPaused();
			}

			pinterpretter->checkPause();

			return 1;
		};

		parser_.setCallBack(pCallback);
	}

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	openLibs();

	QtConcurrent::run([this]()
		{
			isRunning_.store(true, std::memory_order_release);
			parser_.parse(0);
			isRunning_.store(false, std::memory_order_release);
			emit finished();
		});
}

//先行解析token並發送給UI顯示
void Interpreter::preview(const QString& fileName)
{
	QString content;
	if (!util::readFile(fileName, &content))
		return;

	if (!parser_.loadString(content))
		return;

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (parser_.isPrivate())
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<qint64, TokenMap>{}));
		return;
	}

	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(parser_.getTokens()));
}

void Interpreter::stop()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.nodifyAllScriptStop();
	Injector& injector = Injector::getInstance(getIndex());
	injector.IS_SCRIPT_INTERRUPT.store(true, std::memory_order_release);
}

//註冊interpreter的成員函數
template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_.registerFunction(functionName, std::bind(fun, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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
		qint64 value = var.toLongLong(&ok);
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
	qint64 valueMin = list.value(0).toLongLong(&ok);
	if (!ok)
		return false;

	qint64 valueMax = list.value(1).toLongLong(&ok);
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

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	bool bret = false;
	QElapsedTimer timer; timer.start();
	qint64 delay = timeout / 10;
	if (delay > 100)
		delay = 100;

	for (;;)
	{
		checkPause();

		if (isInterruptionRequested())
			break;

		if (injector.IS_SCRIPT_INTERRUPT.load(std::memory_order_acquire))
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

		QThread::msleep(delay);
	}
	return bret;
}

void Interpreter::openLibs()
{
	/*註册函数*/

		//system
	registerFunction("button", &Interpreter::press);

	registerFunction("run", &Interpreter::run);
	registerFunction("dostr", &Interpreter::dostr);
	registerFunction("send", &Interpreter::send);

	//check
	registerFunction("checkdaily", &Interpreter::checkdaily);

	registerFunction("waitmap", &Interpreter::waitmap);
	registerFunction("waitdlg", &Interpreter::waitdlg);
	registerFunction("waitsay", &Interpreter::waitsay);
	registerFunction("waitpet", &Interpreter::waitpet);
	registerFunction("waititem", &Interpreter::waititem);
	registerFunction("waitpos", &Interpreter::waitpos);

	//check-group
	registerFunction("waitteam", &Interpreter::waitteam);


	//move
	registerFunction("dir", &Interpreter::setdir);
	registerFunction("walkpos", &Interpreter::walkpos);

	//action
	registerFunction("useitem", &Interpreter::useitem);
	registerFunction("doffitem", &Interpreter::dropitem);
	registerFunction("swapitem", &Interpreter::swapitem);
	registerFunction("chname", &Interpreter::playerrename);
	registerFunction("chpetname", &Interpreter::petrename);
	registerFunction("chpet", &Interpreter::setpetstate);
	registerFunction("doffpet", &Interpreter::droppet);
	registerFunction("buy", &Interpreter::buy);
	registerFunction("sell", &Interpreter::sell);
	registerFunction("sellpet", &Interpreter::sellpet);
	registerFunction("make", &Interpreter::make);
	registerFunction("cook", &Interpreter::cook);
	registerFunction("usemagic", &Interpreter::usemagic);
	registerFunction("pickup", &Interpreter::pickitem);
	registerFunction("putstone", &Interpreter::depositgold);
	registerFunction("getstone", &Interpreter::withdrawgold);
	registerFunction("skup", &Interpreter::addpoint);
	registerFunction("learn", &Interpreter::learn);
	registerFunction("trade", &Interpreter::trade);
	registerFunction("mail", &Interpreter::mail);
	registerFunction("doffstone", &Interpreter::doffstone);

	registerFunction("requip", &Interpreter::recordequip);
	registerFunction("wequip", &Interpreter::wearequip);
	registerFunction("uequip", &Interpreter::unwearequip);
	registerFunction("puequip", &Interpreter::petunequip);
	registerFunction("pequip", &Interpreter::petequip);

	registerFunction("putpet", &Interpreter::depositpet);
	registerFunction("putitem", &Interpreter::deposititem);
	registerFunction("getpet", &Interpreter::withdrawpet);
	registerFunction("getitem", &Interpreter::withdrawitem);

	//action->group
	registerFunction("join", &Interpreter::join);
	registerFunction("leave", &Interpreter::leave);
	registerFunction("kick", &Interpreter::kick);

	//hide
	//registerFunction("ocr", &Interpreter::ocr);

	//battle
	registerFunction("bh", &Interpreter::bh);//atk
	registerFunction("bj", &Interpreter::bj);//magic
	registerFunction("bp", &Interpreter::bp);//skill
	registerFunction("bs", &Interpreter::bs);//switch
	registerFunction("be", &Interpreter::be);//escape
	registerFunction("bd", &Interpreter::bd);//defense
	registerFunction("bi", &Interpreter::bi);//item
	registerFunction("bt", &Interpreter::bt);//catch
	registerFunction("bn", &Interpreter::bn);//nothing
	registerFunction("bw", &Interpreter::bw);//petskill
	registerFunction("bwf", &Interpreter::bwf);//pet nothing
	registerFunction("bwait", &Interpreter::bwait);
	registerFunction("bend", &Interpreter::bend);
}

qint64 Interpreter::scriptCallBack(qint64 currentIndex, qint64 currentLine, const TokenMap& TK)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (isInterruptionRequested())
		return 0;

	Parser* pparent = parser_.getParent();
	if (pparent != &parser_)
	{
		if (pparent != nullptr)
		{
			if (pparent->isInterruptionRequested())
				return 0;

			if (pparent->getInterpreter() != nullptr && pparent->getInterpreter()->isInterruptionRequested())
				return 0;
		}
	}

	Interpreter* interpreter = parser_.getInterpreter();
	if (interpreter != this)
	{
		if (interpreter != nullptr && interpreter->isInterruptionRequested())
			return 0;
	}

	RESERVE currentType = TK.value(0).type;

	bool skip = currentType == RESERVE::TK_WHITESPACE
		|| currentType == RESERVE::TK_COMMENT
		|| currentType == RESERVE::TK_UNK;

	if (skip)
		return 1;

	Injector& injector = Injector::getInstance(currentIndex);

	if (parser_.getMode() == Parser::kSync)
		emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, parser_.getToken().size(), false);

	if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
	{
		paused();
		emit signalDispatcher.scriptPaused();
	}

	checkPause();

	if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		return 1;

	QString scriptFileName = parser_.getScriptFileName();
	util::SafeHash<qint64, break_marker_t> breakMarkers = injector.break_markers.value(scriptFileName);
	const util::SafeHash<qint64, break_marker_t> stepMarkers = injector.step_markers.value(scriptFileName);
	if (!breakMarkers.contains(currentLine) && !stepMarkers.contains(currentLine))
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
		injector.break_markers.insert(scriptFileName, breakMarkers);
		//所有行插入隱式斷點(用於單步)
		emit signalDispatcher.addStepMarker(currentLine, true);
		emit signalDispatcher.breakMarkInfoImport();
	}

	emit signalDispatcher.addForwardMarker(currentLine, true);

	checkPause();

	return 1;
}

void Interpreter::proc()
{
	qDebug() << "Interpreter::run()";
	isRunning_.store(true, std::memory_order_release);

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);


	injector.IS_SCRIPT_FLAG.store(true, std::memory_order_release);
	injector.IS_SCRIPT_INTERRUPT.store(false, std::memory_order_release);

	parser_.initialize(&parser_);

	doFile(beginLine_, injector.currentScriptFileName, this, nullptr, false, Parser::kSync);

	for (auto& it : subInterpreterList_)
	{
		if (it.isNull())
			continue;

		it->requestInterruption();
	}
	futureSync_.waitForFinished();


	injector.IS_SCRIPT_FLAG.store(false, std::memory_order_release);
	injector.IS_SCRIPT_INTERRUPT.store(false, std::memory_order_release);

	isRunning_.store(false, std::memory_order_release);

	emit finished();
	emit signalDispatcher.scriptFinished();
}

//檢查是否戰鬥，如果是則等待，並在戰鬥結束後停滯一段時間
bool Interpreter::checkBattleThenWait()
{
	checkPause();

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return false;

	bool bret = false;
	if (injector.server->getBattleFlag())
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested())
				break;

			if (injector.IS_SCRIPT_INTERRUPT.load(std::memory_order_acquire))
				break;

			if (injector.server.isNull())
				break;

			checkPause();

			if (!injector.server->getBattleFlag())
				break;

			if (timer.hasExpired(60000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(1000UL);
	}

	return bret;
}

bool Interpreter::checkOnlineThenWait()
{
	checkPause();

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
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

			if (injector.server.isNull())
				break;

			if (injector.IS_SCRIPT_INTERRUPT.load(std::memory_order_acquire))
				break;

			checkPause();

			if (injector.server->getOnlineFlag())
				break;

			if (timer.hasExpired(60000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(1000UL);
	}

	return bret;
}

//執行子腳本
qint64 Interpreter::run(qint64 currentIndex, qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	QString fileName;
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
	{
		errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("File name expected but got nothing"));
		return Parser::kArgError + 1ll;
	}

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

	Parser::Mode asyncMode = Parser::kSync;
	qint64 nAsync = 0;
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
		qint64 nret = Parser::kNoChange;

		//紀錄當前數據
		QHash<qint64, TokenMap> tokens = parser_.getTokens();
		QHash<QString, qint64> labels = parser_.getLabels();
		QList<FunctionNode> functionNodeList = parser_.getFunctionNodeList();
		QList<ForNode> forNodeList = parser_.getForNodeList();
		QList<LuaNode> luaNodeList_ = parser_.getLuaNodeList();
		QString currentFileName = parser_.getScriptFileName();
		qint64 currentLine = parser_.getCurrentLine();

		QScopedPointer<Interpreter> interpreter(new Interpreter(currentIndex));
		if (interpreter.isNull())
			return Parser::kError;

		injector.currentScriptFileName = fileName;

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
		injector.currentScriptFileName = currentFileName;
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
		futureSync_.addFuture(QtConcurrent::run([this, beginLine, fileName, varShareMode, asyncMode, currentIndex]()->bool
			{
				QSharedPointer<Interpreter> interpreter(QSharedPointer<Interpreter>::create(currentIndex));
				if (interpreter.isNull())
					return false;

				subInterpreterList_.append(interpreter);
				interpreter->parser_.initialize(&parser_);
				if (interpreter->doFile(beginLine, fileName, this, &parser_, true, asyncMode))
					return true;

				return false;
			}));

		return Parser::kNoChange;
	}
}

//執行代碼塊
qint64 Interpreter::dostr(qint64 currentIndex, qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance(currentIndex);

	if (injector.server.isNull())
		return Parser::kServerNotReady;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
	{
		errorExport(currentIndex, currentline, ERROR_LEVEL, QObject::tr("String expected but got nothing"));
		return Parser::kArgError + 1ll;
	}

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

	Parser::Mode asyncMode = Parser::kSync;
	qint64 nAsync = 0;
	checkInteger(TK, 3, &nAsync);
	if (nAsync > 0)
		asyncMode = Parser::kAsync;

	QSharedPointer<Interpreter> interpreter(QSharedPointer<Interpreter>::create(currentIndex));
	if (interpreter.isNull())
		return Parser::kError;

	if (asyncMode == Parser::kSync)
	{
		if (varShareMode == kShare)
		{
			interpreter->parser_.setLuaMachinePointer(parser_.pLua_);
			interpreter->parser_.setGlobalNameListPointer(parser_.getGlobalNameListPointer());
			interpreter->parser_.setCounterPointer(parser_.getCounterPointer());
			interpreter->parser_.setLuaLocalVarStringListPointer(parser_.getLuaLocalVarStringListPointer());
			interpreter->parser_.setLocalVarStackPointer(parser_.getLocalVarStackPointer());
		}
		else
		{
			interpreter->parser_.initialize(&parser_);
		}
	}
	else
	{
		subInterpreterList_.append(interpreter);
		interpreter->parser_.initialize(nullptr);
	}

	interpreter->doString(script, this, varShareMode);

	if (asyncMode == Parser::kSync)
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

//打印日誌
void Interpreter::logExport(qint64 currentIndex, qint64 currentline, const QString& data, qint64 color)
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

void Interpreter::errorExport(qint64 currentIndex, qint64 currentLine, qint64 level, const QString& data)
{
	QString newText = QString("%1%2").arg(level == WARN_LEVEL ? QObject::tr("[warn]") : QObject::tr("[error]")).arg(data);
	logExport(currentIndex, currentLine, newText, 0);
}