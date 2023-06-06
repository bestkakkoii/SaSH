#include "stdafx.h"
#include "interpreter.h"

#include "map/mapanalyzer.h"
#include "injector.h"
#include "signaldispatcher.h"

Interpreter::Interpreter(QObject* parent)
	: QObject(parent)
	, lexer_(new Lexer())
	, parser_(new Parser())
{

}

Interpreter::~Interpreter()
{
	requestInterruption();
	if (thread_)
	{
		thread_->quit();
		thread_->wait();
	}
	qDebug() << "Interpreter::~Interpreter()";
}

void Interpreter::start(int beginLine, const QString& fileName)
{
	if (thread_ != nullptr)
		return;

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	beginLine_ = beginLine;
	currentMainScriptFileName_ = fileName;
	moveToThread(thread_);
	connect(this, &Interpreter::finished, thread_, &QThread::quit);
	connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
	connect(thread_, &QThread::started, this, &Interpreter::run);
	connect(this, &Interpreter::finished, this, [this]()
		{
			thread_ = nullptr;
			qDebug() << "Interpreter::finished";
		});

	thread_->start();
}

void Interpreter::stop()
{
	requestInterruption();
}

void Interpreter::pause()
{
	if (parser_.isNull())
		return;

	mutex_.lock();
	isPaused_.store(true, std::memory_order_release);
	mutex_.unlock();
}

void Interpreter::resume()
{
	if (parser_.isNull())
		return;

	mutex_.lock();
	isPaused_.store(false, std::memory_order_release);
	waitCondition_.wakeAll();
	mutex_.unlock();

}

void Interpreter::doString(const QString& script)
{
	if (parser_.isNull())
		parser_.reset(new Parser());

	QHash<int, TokenMap> tokens;
	if (!loadString(script, &tokens))
		return;

	parser_->setTokens(tokens);
	beginLine_ = 0;
	currentMainScriptFileName_ = "";
	currentMainScriptString_ = script;

	thread_ = new QThread();
	if (nullptr == thread_)
		return;
	moveToThread(thread_);
	connect(this, &Interpreter::finished, thread_, &QThread::quit);
	connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
	connect(thread_, &QThread::started, this, &Interpreter::run);
	connect(this, &Interpreter::finished, this, [this]()
		{
			thread_ = nullptr;
			qDebug() << "Interpreter::finished";
		});

	thread_->start();
}

bool Interpreter::loadString(const QString& script, QHash<int, TokenMap>* phash)
{
	if (lexer_.isNull())
		lexer_.reset(new Lexer());

	QStringList lines = script.split("\n");
	int rowCount = lines.size();

	QElapsedTimer timer; timer.start();
	QHash<int, TokenMap > tokens;
	for (int row = 0; row < rowCount; ++row)
	{
		TokenMap lineTokens = lexer_->tokenized(row, lines.at(row));
		tokens.insert(row, lineTokens);

		QStringList params;
		int size = lineTokens.size();
		for (int i = 1; i < size; ++i)
		{
			params.append(lineTokens.value(i).raw);
		}

	}
	qDebug() << "tokenized" << timer.elapsed() << "ms";
	if (phash != nullptr)
		*phash = tokens;

	return true;
}

bool Interpreter::loadFile(const QString& fileName, QHash<int, TokenMap>* phash)
{
	if (lexer_.isNull())
		lexer_.reset(new Lexer());

	int rowCount = 0;

	util::QScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return false;

	QTextStream in(&f);
	in.setCodec(util::CODEPAGE_DEFAULT);
	QString c = in.readAll();
	c.replace("\r\n", "\n");

	QHash<int, TokenMap> hash;
	if (!loadString(c, &hash))
		return false;

	alltokens_.insert(fileName, hash);

	if (phash != nullptr)
		*phash = hash;

	return true;
}

template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_->registerFunction(functionName, std::bind(fun, this, std::placeholders::_1));
}

bool Interpreter::checkBattleThenWait() const
{
	Injector& injector = Injector::getInstance();
	bool bret = false;
	if (injector.server->IS_BATTLE_FLAG)
	{
		QElapsedTimer timer; timer.start();
		bret = true;
		for (;;)
		{
			if (isInterruptionRequested())
				break;

			if (!injector.server->IS_BATTLE_FLAG)
				break;
			if (timer.hasExpired(60000))
				break;
			QThread::msleep(100UL);
			QThread::yieldCurrentThread();
		}

		QThread::msleep(500UL);
	}
	return bret;
}

bool Interpreter::findPath(QPoint dst, int steplen, int step_cost, int timeout, std::function<int(QPoint& dst)> callback) const
{
	//print() << L"MapName:" << Util::S2W(g_mapinfo->name) << "(" << g_mapinfo->floor << ")";
	Injector& injector = Injector::getInstance();
	int floor = injector.server->nowFloor;
	QPoint src(injector.server->nowPoint);
	if (src == dst)
	{
		return true;//已經抵達
	}

	map_t _map;

	QVector<QPoint> path;

	if (injector.server->mapAnalyzer->readFromBinary(floor, injector.server->nowFloorName))
	{
		if (!injector.server->mapAnalyzer->getMapDataByFloor(floor, &_map))
		{

			return false;
		}

	}
	else
	{
		return false;
	}

	//print() << "walkable size:" << _map.walkable.size();
	injector.server->announce(QObject::tr("<findpath>start searching the path"));//u8"<尋路>開始搜尋路徑"
	QElapsedTimer timer; timer.start();
	if (!injector.server->mapAnalyzer->calcNewRoute(_map, src, dst, &path))
	{
		injector.server->announce(QObject::tr("<findpath>unable to findpath"));//u8"<尋路>找不到路徑"
		return false;
	}
	int cost = static_cast<int>(timer.elapsed());
	injector.server->announce(QObject::tr("<findpath>path found, cost:%1").arg(cost));//u8"<尋路>成功找到路徑，耗時：%1"

	int size = path.size();
	int first_size = size;
	timer.restart();

	int current_floor = floor;

	int n = 0;
	for (;;)
	{
		if (injector.server.isNull())
			break;

		src = injector.server->nowPoint;

		int steplen_cache = steplen;

		for (;;)
		{
			if (!((steplen_cache) >= (size)))
				break;
			--steplen_cache;
		}

		if (steplen_cache >= 0 && (steplen_cache < static_cast<int>(path.size())))
		{
			QPoint point(path.at(steplen_cache));
			injector.server->move(point);
			QThread::msleep(step_cost);
		}

		if (!checkBattleThenWait())
		{
			src = injector.server->nowPoint;
			if (src == dst)
			{
				cost = timer.elapsed();
				if (cost > 15000)
				{
					injector.server->EO();
				}
				injector.server->move(dst);
				injector.server->announce(QObject::tr("<findpath>arrived destination, cost:%1").arg(timer.elapsed()));//u8"<尋路>已到達目的地，耗時：%1"
				return true;//已抵達true
			}

			if (!injector.server->mapAnalyzer->calcNewRoute(_map, src, dst, &path))
				break;
			size = path.size();
		}

		if (timer.hasExpired(timeout))
		{
			injector.server->announce(QObject::tr("<findpath>stop finding path due to timeout"));//u8"<尋路>超時，放棄尋路"
			break;
		}

		if (injector.server.isNull())
			break;

		if (injector.server->nowFloor != current_floor)
		{
			injector.server->announce(QObject::tr("<findpath>stop finding path due to map changed"));//u8"<尋路>地圖已變更，放棄尋路"
			break;
		}

		if (isInterruptionRequested())
			break;

		if (callback != nullptr)
		{
			if (callback(dst) == 1)
				callback = nullptr;
		}
	}
	return false;
}

//private
bool Interpreter::checkString(const TokenMap& TK, int idx, QString* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	if (!var.isValid())
		return false;
	if (type == TK_REF)
	{
		*ret = parser_->getVar<QString>(var.toString());
	}
	else if (type == TK_STRING)
	{
		*ret = var.toString();
	}
	else
		return false;

	return true;
}

//private
bool Interpreter::checkInt(const TokenMap& TK, int idx, int* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	if (!var.isValid())
		return false;

	if (type == TK_REF)
	{
		*ret = parser_->getVar<int>(var.toString());
	}
	else if (type == TK_INT)
	{
		bool ok = false;
		int value = var.toInt(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else
		return false;

	return true;
}

//private
int Interpreter::checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior) const
{
	if (!TK.contains(idx))
		return Parser::kError;

	bool okJump = false;
	if (behavior == JumpBehavior::FailedJump)
	{
		okJump = !expr;
	}
	else
	{
		okJump = expr;
	}
	if (okJump)
	{
		RESERVE type = TK.value(idx).type;
		QVariant var = TK.value(idx).data;
		QString label;
		int line = 0;
		if (type == TK_REF)
		{
			line = parser_->getVar<int>(var.toString());
			if (line == 0)
				label = parser_->getVar<QString>(var.toString());
		}
		else if (type == TK_STRING)
		{
			label = var.toString();
		}
		else if (type == TK_INT)
		{
			bool ok = false;
			int value = 0;
			value = var.toInt(&ok);
			if (ok)
				line = value;
		}
		else
			return Parser::kArgError;

		if (label.isEmpty() && line == 0)
			line = -1;

		if (!label.isEmpty())
		{
			parser_->jump(label);
		}
		else if (line != 0)
		{
			parser_->jump(line);
		}
		else
			return Parser::kArgError;

		return Parser::kHasJump;
	}

	return Parser::kNoChange;
}

//private
bool Interpreter::checkRange(const TokenMap& TK, int idx, int* min, int* max) const
{
	if (!TK.contains(idx))
		return false;
	if (min == nullptr || max == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	if (!var.isValid())
		return false;

	QString range;
	if (type == TK_REF)
	{
		range = parser_->getVar<QString>(var.toString());
		bool ok = false;
		int value = range.toInt(&ok);
		if (ok)
		{
			*min = value - 1;
			*max = value - 1;
			return true;
		}
	}
	else if (type == TK_STRING)
	{
		range = var.toString();
	}
	else if (type == TK_INT)
	{
		bool ok = false;
		int value = var.toInt(&ok);
		if (!ok)
			return false;
		*min = value - 1;
		*max = value - 1;
		return true;
	}
	else if (type == TK_FUZZY)
	{
		return true;
	}
	else
		return false;

	if (range.isEmpty())
		return false;


	QStringList list = range.split(util::rexDec);
	if (list.size() != 2)
		return false;

	bool ok = false;
	int valueMin = list.at(0).toInt(&ok);
	if (!ok)
		return false;

	int valueMax = list.at(1).toInt(&ok);
	if (!ok)
		return false;

	if (valueMin > valueMax)
		return false;

	*min = valueMin - 1;
	*max = valueMax - 1;

	return true;
}

bool Interpreter::checkRelationalOperator(const TokenMap& TK, int idx, RESERVE* ret) const
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

bool Interpreter::compare(const QVariant& a, const QVariant& b, RESERVE type) const
{
	QVariant::Type aType = a.type();

	if (aType == QVariant::String)
	{
		QString strA = a.toString();
		QString strB = b.toString();

		// 根据 type 进行相应的比较操作
		switch (type)
		{
		case TK_EQ: // "=="
			return (strA == strB);

		case TK_NEQ: // "!="
			return (strA != strB);

		case TK_GT: // ">"
			return (strA.compare(strB) > 0);

		case TK_LT: // "<"
			return (strA.compare(strB) < 0);

		case TK_GEQ: // ">="
			return (strA.compare(strB) >= 0);

		case TK_LEQ: // "<="
			return (strA.compare(strB) <= 0);

		default:
			return false; // 不支持的比较类型，返回 false
		}
	}
	else if (aType == QVariant::Double)
	{
		double numA = a.toDouble();
		double numB = b.toDouble();

		// 根据 type 进行相应的比较操作
		switch (type)
		{
		case TK_EQ: // "=="
			return (numA == numB);

		case TK_NEQ: // "!="
			return (numA != numB);

		case TK_GT: // ">"
			return (numA > numB);

		case TK_LT: // "<"
			return (numA < numB);

		case TK_GEQ: // ">="
			return (numA >= numB);

		case TK_LEQ: // "<="
			return (numA <= numB);

		default:
			return false; // 不支持的比较类型，返回 false
		}
	}
	else if (aType == QVariant::Int)
	{
		int numA = a.toInt();
		int numB = b.toInt();

		// 根据 type 进行相应的比较操作
		switch (type)
		{
		case TK_EQ: // "=="
			return (numA == numB);

		case TK_NEQ: // "!="
			return (numA != numB);

		case TK_GT: // ">"
			return (numA > numB);

		case TK_LT: // "<"
			return (numA < numB);

		case TK_GEQ: // ">="
			return (numA >= numB);

		case TK_LEQ: // "<="
			return (numA <= numB);

		default:
			return false; // 不支持的比较类型，返回 false
		}

	}

	return false; // 不支持的类型，返回 false
}

bool Interpreter::compare(CompareArea area, const TokenMap& TK) const
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
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

		CompareType cmpType = compareTypeMap.value(QObject::tr("player") + cmpTypeStr, kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		if (checkRelationalOperator(TK, 2, &op))
			return false;

		if (!TK.contains(3))
			return false;

		b = TK.value(3).data;

		switch (cmpType)
		{
		case kPlayerName:
			a = injector.server->pc.name;
			break;
		case kPlayerFreeName:
			a = injector.server->pc.freeName;
			break;
		case kPlayerLevel:
			a = injector.server->pc.level;
			break;
		case kPlayerHp:
			a = injector.server->pc.hp;
			break;
		case kPlayerMaxHp:
			a = injector.server->pc.maxHp;
			break;
		case kPlayerHpPercent:
			a = injector.server->pc.hpPercent;
			break;
		case kPlayerMp:
			a = injector.server->pc.mp;
			break;
		case kPlayerMaxMp:
			a = injector.server->pc.maxMp;
			break;
		case kPlayerMpPercent:
			a = injector.server->pc.mpPercent;
			break;
		case kPlayerExp:
			a = injector.server->pc.exp;
			break;
		case kPlayerMaxExp:
			a = injector.server->pc.maxExp;
			break;
		case kPlayerStone:
			a = injector.server->pc.gold;
			break;
		case kPlayerAtk:
			a = injector.server->pc.atk;
			break;
		case kPlayerDef:
			a = injector.server->pc.def;
			break;
		default:
			return false;
		}
	}
	else if (area == kAreaPet)
	{
		int petIndex = 0;

		checkInt(TK, 1, &petIndex);
		--petIndex;
		if (petIndex < 0)
		{
			QString petTypeName;
			checkString(TK, 1, &petTypeName);
			if (petTypeName.isEmpty())
				return false;

			QHash<QString, int> hash = {
				{ QObject::tr("battlepet"), injector.server->pc.battlePetNo },
				{ QObject::tr("ride"), injector.server->pc.battlePetNo },
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

		CompareType cmpType = compareTypeMap.value(QObject::tr("pet") + cmpTypeStr, kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;


		if (checkRelationalOperator(TK, 3, &op))
			return false;

		if (!TK.contains(4))
			return false;

		b = TK.value(4).data;

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
			const QHash<PetState, QString> hash = {
				{ kBattle, QObject::tr("battle") },
				{ kStandby, QObject::tr("standby") },
				{ kMail, QObject::tr("mail") },
				{ kRest, QObject::tr("rest") },
				{ kRide, QObject::tr("ride") },
			};
			PetState state = injector.server->pet[petIndex].state;
			QString stateStr = hash.value(state, "");
			if (stateStr.isEmpty())
				return false;

			a = stateStr;
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

		CompareType cmpType = compareTypeMap.value(cmpTypeStr, kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		QString itemName;
		checkString(TK, 1, &itemName);
		if (itemName.isEmpty())
			return false;

		QString itemMemo;
		checkString(TK, 2, &itemMemo);

		if (checkRelationalOperator(TK, 3, &op))
			return false;

		if (!TK.contains(4))
			return false;

		b = TK.value(4).data;

		switch (cmpType)
		{
		case itemCount:
		{
			QVector<int> v;
			int count = 0;
			if (injector.server->getItemIndexsByName(itemName, itemMemo, &v))
			{
				int size = v.size();
				for (const int it : v)
					count += injector.server->pc.item[it].pile;
			}

			a = count;
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

		CompareType cmpType = compareTypeMap.value(cmpTypeStr, kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		if (checkRelationalOperator(TK, 1, &op))
			return false;

		if (!TK.contains(2))
			return false;

		b = TK.value(2).data;

		switch (cmpType)
		{
		case kTeamCount:
		{
			int count = 0;
			if ((injector.server->pc.status & CHR_STATUS_LEADER) || (injector.server->pc.status & CHR_STATUS_PARTY))
			{
				for (int i = 0; i < MAX_PARTY; ++i)
				{
					if (injector.server->party[i].name.isEmpty())
						continue;

					if (injector.server->party[i].useFlag == 0)
						continue;

					if (injector.server->party[i].level == 0)
						continue;

					++count;
				}
			}

			a = count;
			break;
		}
		case kPetCount:
		{
			int count = 0;
			for (int i = 0; i < MAX_PET; ++i)
			{
				if (injector.server->pet[i].useFlag == 0)
					continue;

				if (injector.server->pet[i].level == 0)
					continue;

				if (injector.server->pet[i].name.isEmpty())
					continue;

				++count;
			}

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

bool Interpreter::waitfor(int timeout, std::function<bool()> exprfun) const
{
	Injector& injector = Injector::getInstance();
	bool bret = false;
	QElapsedTimer timer; timer.start();
	for (;;)
	{
		QThread::msleep(100);

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
	}
	return bret;
}

void Interpreter::run()
{
	isRunning_.store(true, std::memory_order_release);
	qDebug() << "Interpreter::run()";
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	ParserCallBack callback = [this, &injector, &signalDispatcher](int currentLine, const TokenMap& TK)->int
	{
		if (isInterruptionRequested())
			return 0;

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, false);

		if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
		{
			pause();
			emit signalDispatcher.scriptPaused();
		}

		if (isPaused_.load(std::memory_order_acquire))
		{
			mutex_.lock();
			waitCondition_.wait(&mutex_);
			mutex_.unlock();
		}

		return 1;
	};

	do
	{
		if (isInterruptionRequested())
			break;

		if (lexer_.isNull())
			break;

		if (parser_.isNull())
			break;

		if (currentMainScriptFileName_.isEmpty() && currentMainScriptString_.isEmpty())
			break;

		if (!injector.server.isNull())
			injector.server->IS_SCRIPT_FLAG = true;

		if (!parser_->hasToken())
		{
			if (!alltokens_.contains(currentMainScriptFileName_))
			{
				if (!loadFile(currentMainScriptFileName_, nullptr))
					break;
			}

			parser_->setTokens(alltokens_.value(currentMainScriptFileName_));
		}

		openLibs();
		parser_->setCallBack(callback);
		parser_->setLabels(lexer_->getLabels());

		qDebug() << "beginLine_" << beginLine_;

		try
		{
			parser_->parse(beginLine_);
		}
		catch (...)
		{
			qDebug() << "parse exception";
		}

	} while (false);

	if (!injector.server.isNull())
		injector.server->IS_SCRIPT_FLAG = false;
	isRunning_.store(false, std::memory_order_release);
	emit finished();
}

void Interpreter::openLibs()
{
	/*註冊函數*/

	//system
	registerFunction(u8"測試", &Interpreter::test);
	registerFunction(u8"延時", &Interpreter::sleep);
	registerFunction(u8"按鈕", &Interpreter::press);
	registerFunction(u8"元神歸位", &Interpreter::eo);
	registerFunction(u8"提示", &Interpreter::announce);
	registerFunction(u8"消息", &Interpreter::messagebox);
	registerFunction(u8"回點", &Interpreter::logback);
	registerFunction(u8"登出", &Interpreter::logout);
	registerFunction(u8"說話", &Interpreter::talk);
	registerFunction(u8"說出", &Interpreter::talkandannounce);
	registerFunction(u8"清屏", &Interpreter::cleanchat);
	registerFunction(u8"設置", &Interpreter::set);
	registerFunction(u8"儲存設置", &Interpreter::savesetting);
	registerFunction(u8"讀取設置", &Interpreter::loadsetting);

	//check
	registerFunction(u8"任務狀態", &Interpreter::checkdaily);
	registerFunction(u8"戰鬥中", &Interpreter::isbattle);
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

	//action
	registerFunction(u8"使用道具", &Interpreter::useitem);
	registerFunction(u8"丟棄道具", &Interpreter::dropitem);
	registerFunction(u8"人物改名", &Interpreter::playerrename);
	registerFunction(u8"寵物改名", &Interpreter::petrename);
	registerFunction(u8"更換寵物", &Interpreter::setpetstate);
	registerFunction(u8"丟棄寵物", &Interpreter::droppet);
	registerFunction(u8"購買", &Interpreter::buy);
	registerFunction(u8"售賣", &Interpreter::sell);
	registerFunction(u8"加工", &Interpreter::make);
	registerFunction(u8"料理", &Interpreter::cook);
	registerFunction(u8"使用精靈", &Interpreter::usemagic);

	//action->group
	registerFunction(u8"組隊", &Interpreter::join);
	registerFunction(u8"離隊", &Interpreter::leave);

}

int Interpreter::test(const TokenMap&) const
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	injector.server->announce("Hello World!");
	return Parser::kNoChange;
}