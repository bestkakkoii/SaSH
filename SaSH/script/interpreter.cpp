#include "stdafx.h"
#include "interpreter.h"

#include "map/mapanalyzer.h"
#include "injector.h"
#include "signaldispatcher.h"

#include "crypto.h"

util::SafeHash<QString, util::SafeHash<int, break_marker_t>> break_markers;//用於標記自訂義中斷點(紅點)
util::SafeHash < QString, util::SafeHash<int, break_marker_t>> forward_markers;//用於標示當前執行中斷處(黃箭頭)
util::SafeHash < QString, util::SafeHash<int, break_marker_t>> error_markers;//用於標示錯誤發生行(紅線)
util::SafeHash < QString, util::SafeHash<int, break_marker_t>> step_markers;//隱式標記中斷點用於單步執行(無)


Interpreter::Interpreter(QObject* parent)
	: QObject(parent)
	, parser_(new Parser())
{
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
	qDebug() << "Interpreter::~Interpreter()";
}

//在新線程執行腳本文件
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
	bool isPrivate = false;
	if (!readFile(fileName, &content, &isPrivate))
		return;

	scriptFileName_ = fileName;

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

//同線程下執行腳本文件(實例新的interpreter)
bool Interpreter::doFile(int beginLine, const QString& fileName, Interpreter* parent, VarShareMode shareMode, RunFileMode mode)
{
	if (parser_.isNull())
		parser_.reset(new Parser());

	if (shareMode == kAsync)
		parser_->setMode(Parser::kAsync);

	QString content;
	bool isPrivate = false;
	if (!readFile(fileName, &content, &isPrivate))
		return false;

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
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
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<int, TokenMap>{}));
	}

	pCallback = [parent, &signalDispatcher, mode, this](int currentLine, const TokenMap& TK)->int
	{
		if (parent->isInterruptionRequested())
			return 0;

		if (isInterruptionRequested())
			return 0;

		if (mode == kSync)
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

	if (shareMode == kShare)
	{
		QSharedPointer<VariantSafeHash> pparentHash = parent->parser_->getPVariables();
		if (!pparentHash.isNull())
			parser_->setPVariables(pparentHash);
	}

	parser_->setTokens(tokens);
	parser_->setLabels(labels);
	parser_->setCallBack(pCallback);
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

	//if (shareMode == kShare && mode == kSync)
	//{
	//	VariantSafeHash vars = parser_->getVarsRef();
	//	VariantSafeHash& parentVars = parent->parser_->getVarsRef();
	//	for (auto it = vars.cbegin(); it != vars.cend(); ++it)
	//	{
	//		parentVars.insert(it.key(), it.value());
	//	}
	//}

	isRunning_.store(false, std::memory_order_release);
	return true;
}

//新線程下執行一段腳本內容
void Interpreter::doString(const QString& script, Interpreter* parent, VarShareMode shareMode)
{
	if (parser_.isNull())
		parser_.reset(new Parser());

	isSub = true;
	parser_->setMode(Parser::kAsync);

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	if (!loadString(script, &tokens, &labels))
		return;

	isRunning_.store(true, std::memory_order_release);

	parser_->setTokens(tokens);
	parser_->setLabels(labels);

	beginLine_ = 0;

	thread_ = new QThread();
	if (nullptr == thread_)
		return;

	if (parent)
	{
		pCallback = [parent, this](int currentLine, const TokenMap& TK)->int
		{
			if (parent->isInterruptionRequested())
				return 0;

			if (isInterruptionRequested())
				return 0;

			if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
			{
				parent->pause();
				SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
				emit signalDispatcher.scriptPaused();
			}

			parent->checkPause();

			updateGlobalVariables();

			return 1;
		};
		parser_->setCallBack(pCallback);

		if (shareMode == kShare)
		{
			QSharedPointer<VariantSafeHash> pparentHash = parent->parser_->getPVariables();
			if (!pparentHash.isNull())
				parser_->setPVariables(pparentHash);
		}
	}

	parser_->setTokens(tokens);
	parser_->setLabels(labels);
	openLibs();

	QtConcurrent::run([this]()
		{
			try
			{
				parser_->parse(0);
			}
			catch (...)
			{
				qDebug() << "parse exception --- Sub DoString";
				isRunning_.store(false, std::memory_order_release);
				return;
			}

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

	QHash<int, TokenMap> tokens;
	QHash<QString, int> labels;
	if (!loadString(content, &tokens, &labels))
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (isPrivate)
	{
		emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(QHash<int, TokenMap>{}));
		return;
	}
	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(tokens));
}

//將腳本內容轉換成Token
bool Interpreter::loadString(const QString& script, QHash<int, TokenMap>* ptokens, QHash<QString, int>* plabel)
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

//註冊interpreter的成員函數
template<typename Func>
void Interpreter::registerFunction(const QString functionName, Func fun)
{
	parser_->registerFunction(functionName, std::bind(fun, this, std::placeholders::_1, std::placeholders::_2));
}

//檢查是否戰鬥，如果是則等待，並在戰鬥結束後停滯一段時間
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
			QThread::msleep(100);
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

bool Interpreter::findPath(QPoint dst, int steplen, int step_cost, int timeout, std::function<int(QPoint& dst)> callback, bool noAnnounce)
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
	if (!noAnnounce)
		injector.server->announce(QObject::tr("<findpath>start searching the path"));//"<尋路>開始搜尋路徑"
	QElapsedTimer timer; timer.start();
	if (!injector.server->mapAnalyzer->calcNewRoute(_map, src, dst, &path))
	{
		if (!noAnnounce)
			injector.server->announce(QObject::tr("<findpath>unable to findpath"));//"<尋路>找不到路徑"
		return false;
	}
	int cost = static_cast<int>(timer.elapsed());
	if (!noAnnounce)
		injector.server->announce(QObject::tr("<findpath>path found, cost:%1").arg(cost));//"<尋路>成功找到路徑，耗時：%1"

	int size = path.size();
	int first_size = size;
	timer.restart();

	int current_floor = floor;

	int n = 0;

	QElapsedTimer blockDetectTimer; blockDetectTimer.start();
	QPoint lastPoint = src;

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
			if (lastPoint != src)
			{
				blockDetectTimer.restart();
				lastPoint = src;
			}

			injector.server->move(point);
			QThread::msleep(step_cost);
		}

		if (!checkBattleThenWait())
		{
			src = injector.server->nowPoint;
			if (src == dst)
			{
				cost = timer.elapsed();
				if (cost > 2000)
				{
					QThread::msleep(500);
					if (injector.server.isNull())
						break;
					injector.server->EO();
					QThread::msleep(500);
					if (injector.server.isNull())
						break;
					if (injector.server->nowPoint != dst)
						continue;
				}
				injector.server->move(dst);
				if (!noAnnounce)
					injector.server->announce(QObject::tr("<findpath>arrived destination, cost:%1").arg(timer.elapsed()));//"<尋路>已到達目的地，耗時：%1"
				return true;//已抵達true
			}

			if (!injector.server->mapAnalyzer->calcNewRoute(_map, src, dst, &path))
				break;
			size = path.size();
		}

		if (blockDetectTimer.hasExpired(5000))
		{
			blockDetectTimer.restart();
			injector.server->announce(QObject::tr("<findpath>detedted player ware blocked"));
			injector.server->EO();
			QThread::msleep(500);
			//往隨機8個方向移動
			QPoint point = injector.server->nowPoint;
			lastPoint = point;
			point = point + util::fix_point.at(QRandomGenerator::global()->bounded(0, 7));
			injector.server->move(point);
			QThread::msleep(100);
			continue;
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

//嘗試取指定位置的token轉為字符串
bool Interpreter::checkString(const TokenMap& TK, int idx, QString* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = parser_->getLabelVars();
	if (!var.isValid())
		return false;
	if (type == TK_REF)
	{
		*ret = parser_->getVar<QString>(var.toString());
	}
	else if (type == TK_STRING || type == TK_CMD)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::Int || vtype == QVariant::String)
				*ret = args.value(name).toString();
			else if (vtype == QVariant::Double)
				*ret = QString::number(args.value(name).toDouble(), 'f', 8);
			else
				return false;
		}
		else
			*ret = var.toString();
	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為整數
bool Interpreter::checkInt(const TokenMap& TK, int idx, int* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = parser_->getLabelVars();
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
	else if (type == TK_STRING)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name) && args.value(name).type() == QVariant::Int)
		{
			*ret = args.value(name).toInt();
		}
		else if (args.contains(name) && args.value(name).type() == QVariant::String)
		{
			bool ok;
			int value = args.value(name).toInt(&ok);
			if (ok)
				*ret = value;
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為雙精度浮點數
bool Interpreter::checkDouble(const TokenMap& TK, int idx, double* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = parser_->getLabelVars();
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
	else if (type == TK_STRING)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name) && args.value(name).type() == QVariant::Double)
		{
			*ret = args.value(name).toDouble();
		}
		else if (args.contains(name) && args.value(name).type() == QVariant::String)
		{
			bool ok;
			double value = args.value(name).toDouble(&ok);
			if (ok)
				*ret = value;
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

//嘗試取指定位置的token轉為QVariant
bool Interpreter::toVariant(const TokenMap& TK, int idx, QVariant* ret) const
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = parser_->getLabelVars();
	if (!var.isValid())
		return false;

	if (type == TK_REF)
	{
		*ret = parser_->getVar<QVariant>(var.toString());
	}
	else if (type == TK_STRING || type == TK_CMD)
	{
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::Int || vtype == QVariant::String)
				*ret = args.value(name).toString();
			else if (vtype == QVariant::Double)
				*ret = QString::number(args.value(name).toDouble(), 'f', 8);
			else
				return false;
		}
		else
			*ret = var.toString();
	}
	else
	{
		*ret = var;
	}

	return true;
}

//檢查跳轉是否滿足，和跳轉的方式
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
			parser_->jump(label, false);
		}
		else if (line != 0)
		{
			parser_->jump(line, false);
		}
		else
			return Parser::kArgError;

		return Parser::kHasJump;
	}

	return Parser::kNoChange;
}

//檢查指定位置開始的兩個參數是否能作為範圍值或指定位置的值
bool Interpreter::checkRange(const TokenMap& TK, int idx, int* min, int* max) const
{
	if (!TK.contains(idx))
		return false;
	if (min == nullptr || max == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = parser_->getLabelVars();
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
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::String)
				range = args.value(name).toString();
			else if (vtype == QVariant::Int)
			{
				bool ok = false;
				int value = args.value(name).toInt(&ok);
				if (ok)
				{
					*min = value - 1;
					*max = value - 1;
					return true;
				}
			}
			else
				return false;
		}
		else
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

//檢查是否為邏輯運算符
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

//比較兩個QVariant以 a 的類型為主
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

		CompareType cmpType = compareTypeMap.value(QObject::tr("player") + cmpTypeStr.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;

		if (!checkRelationalOperator(TK, 2, &op))
			return false;

		if (!TK.contains(3))
			return false;

		auto bType = TK.value(3).type;
		b = TK.value(3).data;
		if (bType == TK_REF)
		{
			int value;
			QString bStr;
			if (!checkInt(TK, 3, &value))
			{
				if (!checkString(TK, 3, &bStr))
					return false;
				else
					b = bStr;
			}
			else
			{
				b = value;
			}
		}

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
				{ u8"戰寵", injector.server->pc.battlePetNo},
				{ u8"騎寵", injector.server->pc.battlePetNo},
				{ u8"战宠", injector.server->pc.battlePetNo},
				{ u8"骑宠", injector.server->pc.battlePetNo},
				{ u8"battlepet", injector.server->pc.battlePetNo},
				{ u8"ride", injector.server->pc.battlePetNo},
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

		CompareType cmpType = compareTypeMap.value(QObject::tr("pet") + cmpTypeStr.toLower(), kCompareTypeNone);
		if (cmpType == kCompareTypeNone)
			return false;


		if (!checkRelationalOperator(TK, 3, &op))
			return false;

		if (!TK.contains(4))
			return false;

		auto bType = TK.value(4).type;
		b = TK.value(4).data;
		if (bType == TK_REF)
		{
			int value;
			QString bStr;
			if (!checkInt(TK, 4, &value))
			{
				if (!checkString(TK, 4, &bStr))
					return false;
				else
					b = bStr;
			}
			else
			{
				b = value;
			}
		}

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

		CompareType cmpType = compareTypeMap.value(cmpTypeStr, kCompareTypeNone);
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

		auto bType = TK.value(4).type;
		b = TK.value(4).data;
		if (bType == TK_REF)
		{
			int value;
			QString bStr;
			if (!checkInt(TK, 4, &value))
			{
				if (!checkString(TK, 4, &bStr))
					return false;
				else
					b = bStr;
			}
			else
			{
				b = value;
			}
		}

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

		if (!checkRelationalOperator(TK, 1, &op))
			return false;

		if (!TK.contains(2))
			return false;

		auto bType = TK.value(2).type;
		b = TK.value(2).data;
		if (bType == TK_REF)
		{
			int value;
			QString bStr;
			if (!checkInt(TK, 2, &value))
			{
				if (!checkString(TK, 2, &bStr))
					return false;
				else
					b = bStr;
			}
			else
			{
				b = value;
			}
		}

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

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool Interpreter::waitfor(int timeout, std::function<bool()> exprfun) const
{
	Injector& injector = Injector::getInstance();
	bool bret = false;
	QElapsedTimer timer; timer.start();
	for (;;)
	{
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

//更新系統預定變量的值
void Interpreter::updateGlobalVariables()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;
	PC pc = injector.server->pc;
	VariantSafeHash& hash = parser_->getVarsRef();
	QPoint nowPoint = injector.server->nowPoint;
	dialog_t dialog = injector.server->currentDialog.get();

	//big5
	hash["毫秒時間戳"] = static_cast<int>(QDateTime::currentMSecsSinceEpoch());
	hash["秒時間戳"] = static_cast<int>(QDateTime::currentSecsSinceEpoch());

	hash["人物主名"] = pc.name;
	hash["人物副名"] = pc.freeName;
	hash["人物等級"] = pc.level;
	hash["人物耐久力"] = pc.hp;
	hash["人物氣力"] = pc.mp;
	hash["人物DP"] = pc.dp;
	hash["石幣"] = pc.gold;

	hash["東坐標"] = nowPoint.x();
	hash["南坐標"] = nowPoint.y();
	hash["地圖編號"] = injector.server->nowFloor;
	hash["地圖"] = injector.server->nowFloorName;
	hash["日期"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");
	hash["時間"] = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");
	hash["對話框ID"] = dialog.seqno;

	//gb2312
	hash["毫秒时间戳"] = static_cast<int>(QDateTime::currentMSecsSinceEpoch());
	hash["秒时间戳"] = static_cast<int>(QDateTime::currentSecsSinceEpoch());

	hash["人物主名"] = pc.name;
	hash["人物副名"] = pc.freeName;
	hash["人物等级"] = pc.level;
	hash["人物耐久力"] = pc.hp;
	hash["人物气力"] = pc.mp;
	hash["人物DP"] = pc.dp;
	hash["石币"] = pc.gold;

	hash["东坐标"] = nowPoint.x();
	hash["南坐标"] = nowPoint.y();
	hash["地图编号"] = injector.server->nowFloor;
	hash["地图"] = injector.server->nowFloorName;
	hash["日期"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");
	hash["时间"] = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");
	hash["对话框ID"] = dialog.seqno;

	//utf8
	hash["tick"] = static_cast<int>(QDateTime::currentMSecsSinceEpoch());
	hash["stick"] = static_cast<int>(QDateTime::currentSecsSinceEpoch());

	hash["chname"] = pc.name;
	hash["chfname"] = pc.freeName;
	hash["chlv"] = pc.level;
	hash["chhp"] = pc.hp;
	hash["chmp"] = pc.mp;
	hash["chdp"] = pc.dp;
	hash["stone"] = pc.gold;

	hash["px"] = nowPoint.x();
	hash["py"] = nowPoint.y();
	hash["floor"] = injector.server->nowFloor;
	hash["frname"] = injector.server->nowFloorName;
	hash["date"] = QDateTime::currentDateTime().toString("yyyy-MM-dd");
	hash["time"] = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");

	hash["earnstone"] = injector.server->recorder[0].goldearn;

	hash["dlgid"] = dialog.seqno;

}

void Interpreter::proc()
{
	isRunning_.store(true, std::memory_order_release);
	qDebug() << "Interpreter::run()";
	Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	pCallback = [this, &injector, &signalDispatcher](int currentLine, const TokenMap& TK)->int
	{
		if (isInterruptionRequested())
			return 0;

		RESERVE currentType = parser_->getToken().value(currentLine).value(0).type;
		if (currentType != RESERVE::TK_WHITESPACE && currentType != RESERVE::TK_SPACE)
			emit signalDispatcher.scriptLabelRowTextChanged(currentLine + 1, parser_->getToken().size(), false);
		if (TK.contains(0) && TK.value(0).type == TK_PAUSE)
		{
			pause();
			emit signalDispatcher.scriptPaused();
		}

		checkPause();

		updateGlobalVariables();

		const util::SafeHash<int, break_marker_t> markers = break_markers.value(scriptFileName_);
		const util::SafeHash<int, break_marker_t> step_marker = step_markers.value(scriptFileName_);
		if (!(markers.contains(currentLine) || step_marker.contains(currentLine)))
			return 1;//檢查是否有中斷點

		pause();

		if (markers.contains(currentLine))
		{
			//叫用次數+1
			break_marker_t mark = markers.value(currentLine);
			++mark.count;

			//重新插入斷下的紀錄
			break_markers[scriptFileName_].insert(currentLine, mark);

			//所有行插入隱式斷點(用於單步)
			emit signalDispatcher.addStepMarker(currentLine, true);
		}
		emit signalDispatcher.addForwardMarker(currentLine, true);

		updateGlobalVariables();

		checkPause();

		return 1;
	};

	do
	{
		if (parser_.isNull())
			break;

		if (!isSub)
			injector.IS_SCRIPT_FLAG = true;

		parser_->setCallBack(pCallback);
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

	for (auto& it : subInterpreterList_)
	{
		if (it.isNull())
			continue;

		it->requestInterruption();
	}
	futureSync_.waitForFinished();

	if (!isSub)
		injector.IS_SCRIPT_FLAG = false;
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
	registerFunction(u8"執行代碼", &Interpreter::dostring);

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
	registerFunction(u8"判断", &Interpreter::cmp);
	registerFunction(u8"执行", &Interpreter::run);
	registerFunction(u8"执行代码", &Interpreter::dostring);

	//check
	registerFunction(u8"任务状态", &Interpreter::checkdaily);
	registerFunction(u8"战斗中", &Interpreter::isbattle);
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
	registerFunction(u8"if", &Interpreter::cmp);
	registerFunction(u8"run", &Interpreter::run);
	registerFunction(u8"dostring", &Interpreter::dostring);

	//check
	registerFunction(u8"ifdaily", &Interpreter::checkdaily);
	registerFunction(u8"ifbattle", &Interpreter::isbattle);
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
	registerFunction(u8"addpoint", &Interpreter::addpoint);
	registerFunction(u8"learn", &Interpreter::learn);
	registerFunction(u8"trade", &Interpreter::trade);
	registerFunction(u8"mail", &Interpreter::mail);

	registerFunction(u8"recordequip", &Interpreter::recordequip);
	registerFunction(u8"wearrecordequip", &Interpreter::wearequip);
	registerFunction(u8"unequip", &Interpreter::unwearequip);
	registerFunction(u8"petunequip", &Interpreter::petunequip);
	registerFunction(u8"petequip", &Interpreter::petequip);

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
}

int Interpreter::test(int currentline, const TokenMap&) const
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	injector.server->announce("Hello World!");
	return Parser::kNoChange;
}

//執行子腳本
int Interpreter::run(int currentline, const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	QString fileName;
	checkString(TK, 1, &fileName);
	if (fileName.isEmpty())
		return Parser::kArgError;

	VarShareMode varShareMode = kNotShare;
	int nShared = 0;
	checkInt(TK, 2, &nShared);
	if (nShared > 0)
		varShareMode = kShare;

	int beginLine = 0;
	checkInt(TK, 3, &beginLine);

	RunFileMode asyncMode = kSync;
	int nAsync = 0;
	checkInt(TK, 4, &nAsync);
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
			interpreter->isSub = true;
			if (!interpreter->doFile(beginLine, fileName, this, varShareMode))
				return Parser::kError;

			//還原顯示
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			QHash<int, TokenMap>currentToken = parser_->getToken();
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
					interpreter->isSub = true;
					if (interpreter->doFile(beginLine, fileName, this, kNotShare, kAsync))
						return true;
				}
				return false;
			}));
	}


	return Parser::kNoChange;
}

//執行代碼塊
int Interpreter::dostring(int currentline, const TokenMap& TK)
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
	int nShared = 0;
	checkInt(TK, 2, &nShared);
	if (nShared > 0)
		varShareMode = kShare;

	RunFileMode asyncMode = kSync;
	int nAsync = 0;
	checkInt(TK, 3, &nAsync);
	if (nAsync > 0)
		asyncMode = kAsync;

	QSharedPointer<Interpreter> interpreter(new Interpreter());
	if (!interpreter.isNull())
	{
		subInterpreterList_.append(interpreter);
		interpreter->isSub = true;
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
void Interpreter::logExport(int currentline, const QString& data, int color)
{

	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";


	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline + 1, 3, 10, QLatin1Char(' ')).arg(data));

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->append(msg, color);
}