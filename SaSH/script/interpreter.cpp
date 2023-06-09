#include "stdafx.h"
#include "interpreter.h"

#include "map/mapanalyzer.h"
#include "injector.h"
#include "signaldispatcher.h"

Interpreter::Interpreter(QObject* parent)
	: QObject(parent)
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

void Interpreter::doFileWithThread(int beginLine, const QString& fileName)
{
	if (thread_ != nullptr)
		return;

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	if (parser_.isNull())
		parser_.reset(new Parser());

	QString content;
	if (!readFile(fileName, &content))
		return;

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	if (!loadString(content, &tokens, &labels))
		return;

	parser_->setTokens(tokens);
	parser_->setLabels(labels);

	beginLine_ = beginLine;

	moveToThread(thread_);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, [this]() { requestInterruption(); });
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

bool Interpreter::doFile(int beginLine, const QString& fileName, Interpreter* parent)
{
	if (parser_.isNull())
		parser_.reset(new Parser());

	QString content;
	if (!readFile(fileName, &content))
		return false;

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	if (!loadString(content, &tokens, &labels))
		return false;

	isRunning_.store(true, std::memory_order_release);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.loadFileToTable(fileName);
	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(tokens));

	ParserCallBack callback = [parent, &signalDispatcher, this](int currentLine, const TokenMap& TK)->int
	{
		if (parent->isInterruptionRequested())
			return 0;

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, this->parser_->getToken().size(), false);

		if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
		{
			parent->pause();
			emit signalDispatcher.scriptPaused();
		}

		parent->checkPause();

		updateGlobalVariables();

		return 1;
	};

	parser_->setTokens(tokens);
	parser_->setLabels(labels);
	parser_->setCallBack(callback);
	openLibs();
	try
	{
		parser_->parse(beginLine);
	}
	catch (...)
	{
		qDebug() << "parse exception --- Sub";
		isRunning_.store(false, std::memory_order_release);
		return false;
	}
	isRunning_.store(false, std::memory_order_release);
	return true;
}

void Interpreter::doString(const QString& script)
{
	if (parser_.isNull())
		parser_.reset(new Parser());

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	if (!loadString(script, &tokens, &labels))
		return;

	parser_->setTokens(tokens);
	parser_->setLabels(labels);

	beginLine_ = 0;

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	moveToThread(thread_);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::nodifyAllStop, [this]() { requestInterruption(); });
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

void Interpreter::preview(const QString& fileName)
{
	QString content;
	if (!readFile(fileName, &content))
		return;

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	if (!loadString(content, &tokens, &labels))
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(tokens));
}

bool Interpreter::loadString(const QString& script, QHash<int, TokenMap>* ptokens, QHash<QString, int>* plabel)
{
	return Lexer::tokenized(script, ptokens, plabel);
}

bool Interpreter::readFile(const QString& fileName, QString* pcontent)
{
	util::QScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return false;

	QTextStream in(&f);
	in.setCodec(util::CODEPAGE_DEFAULT);
	QString c = in.readAll();
	c.replace("\r\n", "\n");
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

template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_->registerFunction(functionName, std::bind(fun, this, std::placeholders::_1));
}

bool Interpreter::checkBattleThenWait()
{
	checkPause();

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

			if (!injector.server.isNull())
				break;

			checkPause();

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

void Interpreter::checkPause()
{
	if (isPaused_.load(std::memory_order_acquire))
	{
		mutex_.lock();
		waitCondition_.wait(&mutex_);
		mutex_.unlock();
	}
}

bool Interpreter::findPath(QPoint dst, int steplen, int step_cost, int timeout, std::function<int(QPoint& dst)> callback)
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
	injector.server->announce(QObject::tr("<findpath>start searching the path"));//"<尋路>開始搜尋路徑"
	QElapsedTimer timer; timer.start();
	if (!injector.server->mapAnalyzer->calcNewRoute(_map, src, dst, &path))
	{
		injector.server->announce(QObject::tr("<findpath>unable to findpath"));//"<尋路>找不到路徑"
		return false;
	}
	int cost = static_cast<int>(timer.elapsed());
	injector.server->announce(QObject::tr("<findpath>path found, cost:%1").arg(cost));//"<尋路>成功找到路徑，耗時：%1"

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
				if (cost > 5000)
				{
					QThread::msleep(500);
					injector.server->EO();
					if (injector.server->nowPoint != dst)
						continue;
				}
				injector.server->move(dst);
				injector.server->announce(QObject::tr("<findpath>arrived destination, cost:%1").arg(timer.elapsed()));//"<尋路>已到達目的地，耗時：%1"
				return true;//已抵達true
			}

			if (!injector.server->mapAnalyzer->calcNewRoute(_map, src, dst, &path))
				break;
			size = path.size();
		}

		if (timer.hasExpired(timeout))
		{
			injector.server->announce(QObject::tr("<findpath>stop finding path due to timeout"));//"<尋路>超時，放棄尋路"
			break;
		}

		if (injector.server.isNull())
			break;

		if (injector.server->nowFloor != current_floor)
		{
			injector.server->announce(QObject::tr("<findpath>stop finding path due to map changed"));//"<尋路>地圖已變更，放棄尋路"
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

bool Interpreter::checkDouble(const TokenMap& TK, int idx, double* ret) const
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
		*ret = parser_->getVar<double>(var.toString());
	}
	else if (type == TK_DOUBLE)
	{
		bool ok = false;
		double value = var.toDouble(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else
		return false;

	return true;
}

bool Interpreter::toVariant(const TokenMap& TK, int idx, QVariant* ret) const
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
		*ret = parser_->getVar<QVariant>(var.toString());
	}
	else
	{
		*ret = var;
	}

	return true;
}

//private
int Interpreter::checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior) const
{
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
		QString label;
		int line = 0;
		if (TK.contains(idx))
		{
			RESERVE type = TK.value(idx).type;
			QVariant var = TK.value(idx).data;

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

		}

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

void Interpreter::updateGlobalVariables()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;
	PC pc = injector.server->pc;
	QVariantHash& hash = parser_->getrefs();

	hash["毫秒時間戳"] = GetTickCount64();
	hash["秒時間戳"] = GetTickCount64() / 1000;


	hash["人物主名"] = pc.name;
	hash["人物副名"] = pc.freeName;
	hash["人物等級"] = pc.level;
	hash["人物耐久力"] = pc.hp;
	hash["人物氣力"] = pc.mp;
	hash["人物DP"] = pc.dp;
	hash["石幣"] = pc.gold;

	QPoint nowPoint = injector.server->nowPoint;
	hash["東坐標"] = nowPoint.x();
	hash["南坐標"] = nowPoint.y();
	hash["地圖編號"] = injector.server->nowFloor;
	hash["地圖"] = injector.server->nowFloorName;
	hash["日期"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");
	hash["時間"] = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");

}

void Interpreter::proc()
{
	isRunning_.store(true, std::memory_order_release);
	qDebug() << "Interpreter::run()";
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	ParserCallBack callback = [this, &injector, &signalDispatcher](int currentLine, const TokenMap& TK)->int
	{
		if (isInterruptionRequested())
			return 0;

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, parser_->getToken().size(), false);

		if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
		{
			pause();
			emit signalDispatcher.scriptPaused();
		}

		checkPause();

		updateGlobalVariables();

		return 1;
	};

	do
	{
		if (parser_.isNull())
			break;

		if (!isSub)
			injector.IS_SCRIPT_FLAG = true;

		parser_->setCallBack(callback);
		openLibs();
		try
		{
			parser_->parse(beginLine_);
		}
		catch (...)
		{
			qDebug() << "parse exception --- Main";
		}

	} while (false);

	if (!isSub)
		injector.IS_SCRIPT_FLAG = false;
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
	registerFunction(u8"判斷", &Interpreter::cmp);
	registerFunction(u8"執行", &Interpreter::run);

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
	registerFunction(u8"撿物", &Interpreter::pickitem);
	registerFunction(u8"存錢", &Interpreter::depositgold);
	registerFunction(u8"提錢", &Interpreter::withdrawgold);
	registerFunction(u8"轉移", &Interpreter::warp);
	registerFunction(u8"左擊", &Interpreter::leftclick);
	registerFunction(u8"加點", &Interpreter::addpoint);

	registerFunction(u8"記錄身上裝備", &Interpreter::recordequip);
	registerFunction(u8"裝上記錄裝備", &Interpreter::wearequip);
	registerFunction(u8"卸下裝備", &Interpreter::unwearequip);

	registerFunction(u8"存入寵物", &Interpreter::depositpet);
	registerFunction(u8"存入道具", &Interpreter::deposititem);
	registerFunction(u8"提出寵物", &Interpreter::withdrawpet);
	registerFunction(u8"提出道具", &Interpreter::withdrawitem);

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

int Interpreter::run(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString fileName;
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
		return Parser::kArgError;

	fileName.replace("\\", "/");

	fileName = QCoreApplication::applicationDirPath() + "/script/" + fileName;
	fileName.replace("\\", "/");
	fileName.replace("//", "/");

	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();
	if (suffix.isEmpty())
		fileName += ".txt";
	else if (suffix != "txt")
	{
		fileName.replace(suffix, "txt");
	}

	if (!QFile::exists(fileName))
		return Parser::kArgError;

	int beginLine = 0;
	checkInt(TK, 2, &beginLine);

	QScopedPointer<Interpreter> interpreter(new Interpreter());
	if (!interpreter.isNull())
	{
		if (!interpreter->doFile(beginLine, fileName, this))
			return Parser::kError;

		//還原顯示
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		QHash<int, TokenMap>currentToken = parser_->getToken();
		emit signalDispatcher.loadFileToTable(fileName);
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(currentToken));

	}
	return Parser::kNoChange;
}