#include "stdafx.h"
#include "interpreter.h"

#include "map/mapanalyzer.h"
#include "injector.h"
#include "signaldispatcher.h"

#include "crypto.h"

#include "spdloger.hpp"

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

	QString content;
	bool isPrivate = false;
	if (!readFile(fileName, &content, &isPrivate))
		return;

	scriptFileName_ = fileName;

	QHash<qint64, TokenMap> tokens;
	QHash<QString, qint64> labels;
	if (!loadString(content, &tokens, &labels))
		return;

	parser_.setTokens(tokens);
	parser_.setLabels(labels);

	beginLine_ = beginLine;

	moveToThread(thread_);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(this, &Interpreter::finished, thread_, &QThread::quit);
	connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
	connect(thread_, &QThread::started, this, &Interpreter::proc);
	connect(this, &Interpreter::finished, this, [this]()
		{
			thread_ = nullptr;
			qDebug() << "Interpreter::finished";
		});

	thread_->start();
}

//同線程下執行腳本文件(實例新的interpreter)
bool Interpreter::doFile(qint64 beginLine, const QString& fileName, Interpreter* parent, VarShareMode shareMode, RunFileMode mode)
{
	if (mode == kAsync)
		parser_.setMode(Parser::kAsync);

	QString content;
	bool isPrivate = false;
	if (!readFile(fileName, &content, &isPrivate))
		return false;

	QHash<qint64, TokenMap> tokens;
	QHash<QString, qint64> labels;
	if (!loadString(content, &tokens, &labels))
		return false;

	isRunning_.store(true, std::memory_order_release);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (!isPrivate && mode == kSync)
	{
		emit signalDispatcher.loadFileToTable(fileName);
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(tokens));
	}
	else if (isPrivate && mode == kSync)
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<qint64, TokenMap>{}));
	}

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
			|| currentType == RESERVE::TK_SPACE
			|| currentType == RESERVE::TK_COMMENT
			|| currentType == RESERVE::TK_UNK;

		if (skip)
			return 1;

		if (mode == kSync)
			emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, this->parser_.getToken().size(), false);

		if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
		{
			parent->paused();
			emit signalDispatcher.scriptPaused();
		}

		parent->checkPause();

		return 1;
	};

	if (shareMode == kShare)
	{
		QVariantHash* pparentHash = parent->parser_.getGlobalVarPointer();
		QReadWriteLock* pparentLock = parent->parser_.getGlobalVarLockPointer();
		if (pparentHash != nullptr && pparentLock != nullptr)
			parser_.setVariablesPointer(pparentHash, pparentLock);
	}

	parser_.setTokens(tokens);
	parser_.setLabels(labels);
	parser_.setCallBack(pCallback);
	openLibs();

	parser_.parse(beginLine);

	isRunning_.store(false, std::memory_order_release);
	return true;
}

//新線程下執行一段腳本內容
void Interpreter::doString(const QString& script, Interpreter* parent, VarShareMode shareMode)
{
	parser_.setMode(Parser::kAsync);

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	QString newString = script;
	newString.replace("\\r", "\r");
	newString.replace("\\n", "\n");
	newString.replace("\r\n", "\n");
	newString.replace("\r", "\n");

	setSubScript(true);

	QHash<qint64, TokenMap> tokens;
	QHash<QString, qint64> labels;
	if (!loadString(script, &tokens, &labels))
		return;

	isRunning_.store(true, std::memory_order_release);

	parser_.setTokens(tokens);
	parser_.setLabels(labels);

	beginLine_ = 0;

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
				|| currentType == RESERVE::TK_SPACE
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
			QVariantHash* pparentHash = parent->parser_.getGlobalVarPointer();
			QReadWriteLock* pparentLock = parent->parser_.getGlobalVarLockPointer();
			if (pparentHash != nullptr && pparentLock != nullptr)
				parser_.setVariablesPointer(pparentHash, pparentLock);
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	parser_.setTokens(tokens);
	parser_.setLabels(labels);

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
	QString content;
	bool isPrivate = false;
	if (!readFile(fileName, &content, &isPrivate))
		return;

	QHash<qint64, TokenMap> tokens;
	QHash<QString, qint64> labels;
	if (!loadString(content, &tokens, &labels))
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (isPrivate)
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<qint64, TokenMap>{}));
		return;
	}
	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(tokens));
}

//將腳本內容轉換成Token
bool Interpreter::loadString(const QString& script, QHash<qint64, TokenMap>* ptokens, QHash<QString, qint64>* plabel)
{
	return Lexer::tokenized(script, ptokens, plabel);
}

//讀入文件內容
bool Interpreter::readFile(const QString& fileName, QString* pcontent, bool* isPrivate)
{
	util::QScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return false;

	QString c;
	if (fileName.endsWith(util::SCRIPT_SUFFIX_DEFAULT))
	{
		QTextStream in(&f);
		in.setCodec(util::CODEPAGE_DEFAULT);
		c = in.readAll();
		c.replace("\r\n", "\n");
		if (isPrivate != nullptr)
			*isPrivate = false;
	}
	else if (fileName.endsWith(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT))
	{
		Crypto crypto;
		if (!crypto.decodeScript(fileName, c))
			return false;

		if (isPrivate != nullptr)
			*isPrivate = true;
	}

	if (pcontent != nullptr)
	{
		*pcontent = c;
		return true;
	}

	return false;
}

void Interpreter::stop()
{
	requestInterruption();
}

//註冊interpreter的成員函數
template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_.registerFunction(functionName, std::bind(fun, this, std::placeholders::_1, std::placeholders::_2));
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

bool Interpreter::findPath(QPoint dst, qint64 steplen, qint64 step_cost, qint64 timeout, std::function<qint64(QPoint& dst)> callback, bool noAnnounce)
{
	Injector& injector = Injector::getInstance();
	qint64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	bool isDebug = injector.getEnableHash(util::kScriptDebugModeEnable);
	if (isDebug)
		noAnnounce = true;

	auto getPos = [hProcess, hModule]()
	{
		return QPoint(mem::read<qint64>(hProcess, hModule + kOffestNowX), mem::read<qint64>(hProcess, hModule + kOffestNowY));
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
		injector.server->announce(QObject::tr("<findpath>start searching the path"));//"<尋路>開始搜尋路徑"

	QVector<QPoint> path;
	QElapsedTimer timer; timer.start();
	if (mapAnalyzer.isNull() || !mapAnalyzer->calcNewRoute(_map, src, dst, &path))
	{
		if (!noAnnounce && !injector.server.isNull())
			injector.server->announce(QObject::tr("<findpath>unable to findpath"));//"<尋路>找不到路徑"
		return false;
	}

	qint64 cost = static_cast<qint64>(timer.elapsed());
	if (!noAnnounce && !injector.server.isNull())
		injector.server->announce(QObject::tr("<findpath>path found, cost:%1 step:%2").arg(cost).arg(path.size()));//"<尋路>成功找到路徑，耗時：%1"

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
			QThread::msleep(50);
		}

		if (!checkBattleThenWait())
		{
			src = getPos();
			if (src == dst)
			{
				cost = timer.elapsed();
				if (cost > 2000)
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

				if (!noAnnounce && !injector.server.isNull())
					injector.server->announce(QObject::tr("<findpath>arrived destination, cost:%1").arg(timer.elapsed()));//"<尋路>已到達目的地，耗時：%1"
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

			injector.server->announce(QObject::tr("<findpath>detedted player ware blocked"));
			injector.server->EO();
			QThread::msleep(500);

			if (injector.server.isNull())
				break;

			//往隨機8個方向移動
			point = getPos();
			lastPoint = point;
			point = point + util::fix_point.at(QRandomGenerator::global()->bounded(0, 7));
			injector.server->move(point);
			QThread::msleep(100);

			continue;
		}

		if (injector.server.isNull())
			break;

		if (isInterruptionRequested())
			break;

		if (timer.hasExpired(timeout))
		{
			if (!injector.server.isNull())
				injector.server->announce(QObject::tr("<findpath>stop finding path due to timeout"));//"<尋路>超時，放棄尋路"
			break;
		}

		if (injector.server->nowFloor != current_floor)
		{
			injector.server->announce(QObject::tr("<findpath>stop finding path due to map changed"));//"<尋路>地圖已變更，放棄尋路"
			break;
		}

		if (callback != nullptr)
		{
			if (callback(dst) == 1)
				callback = nullptr;
		}
	}
	return false;
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

	QVariant var = parser_.checkValue(TK, idx, QVariant::Double);
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

//比較兩個QVariant以 a 的類型為主
bool Interpreter::compare(const QVariant& a, const QVariant& b, RESERVE type) const
{
	return parser_.compare(a, b, type);
}

bool Interpreter::compare(CompareArea area, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return false;

	RESERVE op;
	QVariant b;
	QVariant a;

	if (area == kAreaPlayer)
	{
		QString cmpTypeStr;
		checkString(TK, 1, &cmpTypeStr);
		if (cmpTypeStr.isEmpty())
			return false;

		CompareType cmpType = comparePcTypeMap.value(cmpTypeStr.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		if (!checkRelationalOperator(TK, 2, &op))
			return false;

		if (!TK.contains(3))
			return false;

		b = parser_.checkValue(TK, 3, QVariant::String);
		PC _pc = injector.server->getPC();
		switch (cmpType)
		{
		case kPlayerName:
			a = _pc.name;
			break;
		case kPlayerFreeName:
			a = _pc.freeName;
			break;
		case kPlayerLevel:
			a = _pc.level;
			break;
		case kPlayerHp:
			a = _pc.hp;
			break;
		case kPlayerMaxHp:
			a = _pc.maxHp;
			break;
		case kPlayerHpPercent:
			a = _pc.hpPercent;
			break;
		case kPlayerMp:
			a = _pc.mp;
			break;
		case kPlayerMaxMp:
			a = _pc.maxMp;
			break;
		case kPlayerMpPercent:
			a = _pc.mpPercent;
			break;
		case kPlayerExp:
			a = _pc.exp;
			break;
		case kPlayerMaxExp:
			a = _pc.maxExp;
			break;
		case kPlayerStone:
			a = _pc.gold;
			break;
		case kPlayerAtk:
			a = _pc.atk;
			break;
		case kPlayerDef:
			a = _pc.def;
			break;
		default:
			return false;
		}
	}
	else if (area == kAreaPet)
	{
		qint64 petIndex = 0;

		checkInteger(TK, 1, &petIndex);
		--petIndex;
		if (petIndex < 0)
		{
			QString petTypeName;
			checkString(TK, 1, &petTypeName);
			if (petTypeName.isEmpty())
				return false;

			PC _pc = injector.server->getPC();

			QHash<QString, qint64> hash = {
				{ u8"戰寵", _pc.battlePetNo },
				{ u8"騎寵", _pc.ridePetNo },
				{ u8"战宠", _pc.battlePetNo },
				{ u8"骑宠", _pc.ridePetNo },
				{ u8"battlepet", _pc.battlePetNo },
				{ u8"ride", _pc.ridePetNo },
			};

			if (!hash.contains(petTypeName))
				return false;

			petIndex = hash.value(petTypeName, -1);
			if (petIndex < 0)
				return false;
		}

		QString cmpTypeStr;
		checkString(TK, 2, &cmpTypeStr);
		if (cmpTypeStr.isEmpty())
			return false;

		CompareType cmpType = comparePetTypeMap.value(cmpTypeStr.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;


		if (!checkRelationalOperator(TK, 3, &op))
			return false;

		if (!TK.contains(4))
			return false;

		b = parser_.checkValue(TK, 4, QVariant::String);

		switch (cmpType)
		{
		case kPetName:
			a = injector.server->pet[petIndex].name;
			break;
		case kPetFreeName:
			a = injector.server->pet[petIndex].freeName;
			break;
		case kPetLevel:
			a = injector.server->pet[petIndex].level;
			break;
		case kPetHp:
			a = injector.server->pet[petIndex].hp;
			break;
		case kPetMaxHp:
			a = injector.server->pet[petIndex].maxHp;
			break;
		case kPetHpPercent:
			a = injector.server->pet[petIndex].hpPercent;
			break;
		case kPetExp:
			a = injector.server->pet[petIndex].exp;
			break;
		case kPetMaxExp:
			a = injector.server->pet[petIndex].maxExp;
			break;
		case kPetAtk:
			a = injector.server->pet[petIndex].atk;
			break;
		case kPetDef:
			a = injector.server->pet[petIndex].def;
			break;
		case kPetState:
		{
			const QHash<QString, PetState> hash = {
				{ u8"戰鬥", kBattle },
				{ u8"等待", kStandby },
				{ u8"郵件", kMail },
				{ u8"休息", kRest },
				{ u8"騎乘", kRide },

				{ u8"战斗", kBattle },
				{ u8"等待", kStandby },
				{ u8"邮件", kMail },
				{ u8"休息", kRest },
				{ u8"骑乘", kRide },

				{ u8"battle", kBattle },
				{ u8"standby", kStandby },
				{ u8"mail", kMail },
				{ u8"rest", kRest },
				{ u8"ride", kRide },
			};
			PetState state = injector.server->pet[petIndex].state;
			PetState cmpstate = hash.value(b.toString().toLower(), kNoneState);
			if (cmpstate == 0)
				return false;

			a = state;
			b = cmpstate;
			break;
		}
		default:
			return false;
		}
	}
	else if (area == kAreaItem)
	{
		QString cmpTypeStr;
		checkString(TK, 0, &cmpTypeStr);
		if (cmpTypeStr.isEmpty())
			return false;

		CompareType cmpType = compareAmountTypeMap.value(cmpTypeStr, kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		QString itemName;
		checkString(TK, 1, &itemName);
		if (itemName.isEmpty())
			return false;

		QString itemMemo;
		checkString(TK, 2, &itemMemo);

		if (!checkRelationalOperator(TK, 3, &op))
			return false;

		if (!TK.contains(4))
			return false;

		b = parser_.checkValue(TK, 4, QVariant::String);

		switch (cmpType)
		{
		case kitemCount:
		{
			QVector<int> v;
			qint64 count = 0;
			PC _pc = injector.server->getPC();
			if (injector.server->getItemIndexsByName(itemName, itemMemo, &v))
			{
				for (const int it : v)
					count += _pc.item[it].pile;
			}

			a = count;
			break;
		}
		default:
			return false;
		}
	}
	else if (area == kAreaCount)
	{
		QString cmpTypeStr;
		checkString(TK, 0, &cmpTypeStr);
		if (cmpTypeStr.isEmpty())
			return false;

		CompareType cmpType = compareAmountTypeMap.value(cmpTypeStr, kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		if (!checkRelationalOperator(TK, 1, &op))
			return false;

		if (!TK.contains(2))
			return false;

		b = parser_.checkValue(TK, 2, QVariant::String);

		switch (cmpType)
		{
		case kTeamCount:
		{
			qint64 count = injector.server->getPartySize();
			a = count;
			break;
		}
		case kPetCount:
		{
			qint64 count = injector.server->getPetSize();
			a = count;
			break;
		}
		default:
			return false;
		}
	}
	else
		return false;

	if (!a.isValid() || !b.isValid())
		return false;

	return compare(a, b, op);
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

		if (isInterruptionRequested())
			break;

		if (timer.hasExpired(timeout))
			break;

		if (injector.server.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		QThread::msleep(100);
	}
	return bret;
}

qint64 Interpreter::mainScriptCallBack(qint64 currentLine, const TokenMap& TK)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (isInterruptionRequested())
		return 0;

	RESERVE currentType = TK.value(0).type;

	bool skip = currentType == RESERVE::TK_WHITESPACE
		|| currentType == RESERVE::TK_SPACE
		|| currentType == RESERVE::TK_COMMENT
		|| currentType == RESERVE::TK_UNK;

	if (skip)
		return 1;

	SPD_LOG(g_logger_name, QString("main script callback: line: %1").arg(currentLine + 1));

	emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, parser_.getToken().size(), false);

	if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
	{
		paused();
		emit signalDispatcher.scriptPaused();
	}

	checkPause();

	SPD_LOG(g_logger_name, "check break markers");

	util::SafeHash<qint64, break_marker_t> markers = break_markers.value(scriptFileName_);
	const util::SafeHash<qint64, break_marker_t> step_marker = step_markers.value(scriptFileName_);
	if (!(markers.contains(currentLine) || step_marker.contains(currentLine)))
	{
		SPD_LOG(g_logger_name, "no markers, return now");
		return 1;//檢查是否有中斷點
	}

	paused();

	if (markers.contains(currentLine))
	{
		//叫用次數+1
		break_marker_t mark = markers.value(currentLine);
		++mark.count;

		//重新插入斷下的紀錄
		markers.insert(currentLine, mark);
		break_markers.insert(scriptFileName_, markers);

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
				QString pcname = injector.server->getPC().name;
				if (pcname.isEmpty())
					pcname = "noname";

				g_logger_name = SPD_INIT(pcname);

			}
			else
				g_logger_name = SPD_INIT("noname");
		}

		parser_.setCallBack(pCallback);
		openLibs();
		parser_.parse(beginLine_);

		if (!g_logger_name.isEmpty())
			SPD_CLOSE(g_logger_name.toStdString());

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
	registerFunction(u8"測試", &Interpreter::test);
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

	//check
	registerFunction(u8"任務狀態", &Interpreter::checkdaily);
	registerFunction(u8"戰鬥中", &Interpreter::isbattle);
	registerFunction(u8"平時中", &Interpreter::isnormal);
	registerFunction(u8"在線中", &Interpreter::isonline);
	registerFunction(u8"查坐標", &Interpreter::checkcoords);
	registerFunction(u8"查座標", &Interpreter::checkcoords);
	registerFunction(u8"地圖", &Interpreter::checkmap);
	registerFunction(u8"地圖快判", &Interpreter::checkmapnowait);
	registerFunction(u8"對話", &Interpreter::checkdialog);
	registerFunction(u8"看見", &Interpreter::checkunit);
	registerFunction(u8"聽見", &Interpreter::checkchathistory);

	registerFunction(u8"人物狀態", &Interpreter::checkplayerstatus);
	registerFunction(u8"寵物狀態", &Interpreter::checkpetstatus);
	registerFunction(u8"道具數量", &Interpreter::checkitemcount);
	registerFunction(u8"組隊人數", &Interpreter::checkteamcount);
	registerFunction(u8"寵物數量", &Interpreter::checkpetcount);
	registerFunction(u8"寵物有", &Interpreter::checkpet);
	registerFunction(u8"道具", &Interpreter::checkitem);
	registerFunction(u8"背包滿", &Interpreter::checkitemfull);
	//check-group
	registerFunction(u8"組隊有", &Interpreter::checkteam);


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
	registerFunction(u8"测试", &Interpreter::test);
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

	//check
	registerFunction(u8"任务状态", &Interpreter::checkdaily);
	registerFunction(u8"战斗中", &Interpreter::isbattle);
	registerFunction(u8"平时中", &Interpreter::isnormal);
	registerFunction(u8"在线中", &Interpreter::isonline);
	registerFunction(u8"查坐标", &Interpreter::checkcoords);
	registerFunction(u8"查座标", &Interpreter::checkcoords);
	registerFunction(u8"地图", &Interpreter::checkmap);
	registerFunction(u8"地图快判", &Interpreter::checkmapnowait);
	registerFunction(u8"对话", &Interpreter::checkdialog);
	registerFunction(u8"看见", &Interpreter::checkunit);
	registerFunction(u8"听见", &Interpreter::checkchathistory);

	registerFunction(u8"人物状态", &Interpreter::checkplayerstatus);
	registerFunction(u8"宠物状态", &Interpreter::checkpetstatus);
	registerFunction(u8"道具数量", &Interpreter::checkitemcount);
	registerFunction(u8"组队人数", &Interpreter::checkteamcount);
	registerFunction(u8"宠物数量", &Interpreter::checkpetcount);
	registerFunction(u8"宠物有", &Interpreter::checkpet);
	registerFunction(u8"道具", &Interpreter::checkitem);
	registerFunction(u8"背包满", &Interpreter::checkitemfull);
	//check-group
	registerFunction(u8"组队有", &Interpreter::checkteam);


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
	registerFunction(u8"find", &Interpreter::find);
	registerFunction(u8"half", &Interpreter::half);
	registerFunction(u8"full", &Interpreter::full);
	registerFunction(u8"upper", &Interpreter::upper);
	registerFunction(u8"lower", &Interpreter::lower);
	registerFunction(u8"replace", &Interpreter::replace);
	registerFunction(u8"toint", &Interpreter::toint);
	registerFunction(u8"tostr", &Interpreter::tostr);

	//system
	registerFunction(u8"test", &Interpreter::test);
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

	//check
	registerFunction(u8"ifdaily", &Interpreter::checkdaily);
	registerFunction(u8"ifbattle", &Interpreter::isbattle);
	registerFunction(u8"ifnormal", &Interpreter::isnormal);
	registerFunction(u8"ifonline", &Interpreter::isonline);
	registerFunction(u8"ifpos", &Interpreter::checkcoords);
	registerFunction(u8"ifmap", &Interpreter::checkmapnowait);
	registerFunction(u8"ifplayer", &Interpreter::checkplayerstatus);
	registerFunction(u8"ifpetex", &Interpreter::checkpetstatus);
	registerFunction(u8"ifitem", &Interpreter::checkitemcount);
	registerFunction(u8"ifteam", &Interpreter::checkteamcount);
	registerFunction(u8"ifpet", &Interpreter::checkpetcount);
	registerFunction(u8"ifitemfull", &Interpreter::checkitemfull);

	registerFunction(u8"waitmap", &Interpreter::checkmap);
	registerFunction(u8"waitdlg", &Interpreter::checkdialog);
	registerFunction(u8"waitsay", &Interpreter::checkchathistory);
	registerFunction(u8"waitpet", &Interpreter::checkpet);
	registerFunction(u8"waititem", &Interpreter::checkitem);

	//check-group
	registerFunction(u8"waitteam", &Interpreter::checkteam);


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
	registerFunction(u8"chplayername", &Interpreter::playerrename);
	registerFunction(u8"chpetname", &Interpreter::petrename);
	registerFunction(u8"chpet", &Interpreter::setpetstate);
	registerFunction(u8"droppet", &Interpreter::droppet);
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

	//hide
	registerFunction(u8"ocr", &Interpreter::ocr);
	registerFunction(u8"dlg", &Interpreter::dlg);

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

qint64 Interpreter::test(qint64 currentline, const TokenMap& TK) const
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	injector.server->announce("Hello World!");
	return Parser::kNoChange;
}

//執行子腳本
qint64 Interpreter::run(qint64 currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString fileName;
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
		return Parser::kArgError;

	VarShareMode varShareMode = kNotShare;
	qint64 nShared = 0;
	checkInteger(TK, 2, &nShared);
	if (nShared > 0)
		varShareMode = kShare;

	qint64 beginLine = 0;
	checkInteger(TK, 3, &beginLine);

	RunFileMode asyncMode = kSync;
	qint64 nAsync = 0;
	checkInteger(TK, 4, &nAsync);
	if (nAsync > 0)
		asyncMode = kAsync;

	fileName.replace("\\", "/");

	fileName = QCoreApplication::applicationDirPath() + "/script/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".txt";
	else if (suffix != "txt" && suffix != "sac")
	{
		fileName.replace(suffix, "txt");
	}

	if (!QFile::exists(fileName))
		return Parser::kArgError;

	if (kSync == asyncMode)
	{
		QScopedPointer<Interpreter> interpreter(new Interpreter());
		if (!interpreter.isNull())
		{
			interpreter->setSubScript(true);
			if (!interpreter->doFile(beginLine, fileName, this, varShareMode))
				return Parser::kError;

			//還原顯示
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			QHash<qint64, TokenMap>currentToken = parser_.getToken();
			emit signalDispatcher.loadFileToTable(scriptFileName_);
			emit signalDispatcher.scriptContentChanged(scriptFileName_, QVariant::fromValue(currentToken));
		}
	}
	else
	{
		futureSync_.addFuture(QtConcurrent::run([this, beginLine, fileName]()->bool
			{
				QSharedPointer<Interpreter> interpreter(new Interpreter());
				if (!interpreter.isNull())
				{
					subInterpreterList_.append(interpreter);
					interpreter->setSubScript(true);
					if (interpreter->doFile(beginLine, fileName, this, kNotShare, kAsync))
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
		return Parser::kError;

	QString text;
	checkString(TK, 1, &text);
	if (text.isEmpty())
		return Parser::kArgError;

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