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

//private
bool Interpreter::checkBattleThenWait()
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

//private
bool Interpreter::findPath(const QPoint& dst, int steplen, int step_cost, int timeout, std::function<int()> callback)
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
			if (callback() == 1)
				return true;
		}
	}
	return false;
}

//private
bool Interpreter::checkString(const TokenMap& TK, int idx, QString* ret)
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
bool Interpreter::checkInt(const TokenMap& TK, int idx, int* ret)
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
		*ret = var.toInt();
	}
	else
		return false;

	return true;
}

//private
int Interpreter::checkJump(const TokenMap& TK, int idx, bool expr, JumpBehavior behavior)
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
			line = var.toInt();
		}
		else
			return Parser::kError;

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
			return Parser::kError;

		return Parser::kHasJump;
	}

	return Parser::kNoChange;
}


bool Interpreter::waitfor(int timeout, std::function<bool()> exprfun)
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
	registerFunction(u8"回點", &Interpreter::logback);
	registerFunction(u8"登出", &Interpreter::logout);
	registerFunction(u8"說話", &Interpreter::talk);
	registerFunction(u8"說出", &Interpreter::talkandannounce);
	registerFunction(u8"清屏", &Interpreter::cleanchat);

	//check
	registerFunction(u8"任務狀態", &Interpreter::checkdaily);
	registerFunction(u8"戰鬥中", &Interpreter::isbattle);
	registerFunction(u8"查坐標", &Interpreter::checkcoords);
	registerFunction(u8"地圖", &Interpreter::checkmap);
	registerFunction(u8"地圖快判", &Interpreter::checkmapnowait);
	registerFunction(u8"對話", &Interpreter::checkdialog);
	registerFunction(u8"看見", &Interpreter::checkunit);
	registerFunction(u8"聽見", &Interpreter::checkchathistory);

	//move
	registerFunction(u8"方向", &Interpreter::setdir);
	registerFunction(u8"座標", &Interpreter::move);
	registerFunction(u8"坐標", &Interpreter::move);
	registerFunction(u8"移動", &Interpreter::fastmove);
	registerFunction(u8"尋路", &Interpreter::findpath);
	registerFunction(u8"封包移動", &Interpreter::packetmove);
	registerFunction(u8"移動至NPC", &Interpreter::movetonpc);
	registerFunction(u8"尋找NPC", &Interpreter::findnpc);

	//action
	registerFunction(u8"使用道具", &Interpreter::useitem);
	registerFunction(u8"丟棄道具", &Interpreter::dropitem);
}

int Interpreter::test(const TokenMap&)
{
	qDebug() << "Hello World!";
	return Parser::kNoChange;
}