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
#include "tcpserver.h"
#include "autil.h"
#include <injector.h>
#include "signaldispatcher.h"
#include "map/mapanalyzer.h"
#include "script/interpreter.h"

#pragma comment(lib, "winmm.lib")

#include <spdloger.hpp>
extern QString g_logger_name;//parser.cpp

#pragma region StringControl
// 0-9,a-z(10-35),A-Z(36-61)
int Server::a62toi(const QString& a) const
{
	int ret = 0;
	int sign = 1;
	int size = a.length();
	for (int i = 0; i < size; ++i)
	{
		ret *= 62;
		if ('0' <= a[i] && a[i] <= '9')
			ret += a[i].unicode() - '0';
		else if ('a' <= a[i] && a[i] <= 'z')
			ret += a[i].unicode() - 'a' + 10;
		else if ('A' <= a[i] && a[i] <= 'Z')
			ret += a[i].unicode() - 'A' + 36;
		else if (a[i] == '-')
			sign = -1;
		else
			return 0;
	}
	return ret * sign;
}

int Server::getStringToken(const QString& src, const QString& delim, int count, QString& out) const
{
	if (src.isEmpty() || delim.isEmpty() || count < 0)
	{
		out.clear();
		return 1;
	}

	int c = 1;
	int i = 0;

	while (c < count)
	{
		i = src.indexOf(delim, i);
		if (i == -1)
		{
			out = "";
			return 1;
		}
		i += delim.length();
		c++;
	}

	int j = src.indexOf(delim, i);
	if (j == -1)
	{
		out = src.mid(i);
		return 0;
	}

	out = src.mid(i, j - i);
	return 0;
}

int Server::getIntegerToken(const QString& src, const QString& delim, int count) const
{
	if (src.isEmpty() || delim.isEmpty() || count < 0)
	{
		return -1;
	}

	QString s;
	if (getStringToken(src, delim, count, s) == 1)
		return -1;

	bool ok = false;
	int value = s.toInt(&ok);
	if (ok)
		return value;
	return -1;
}

int Server::getInteger62Token(const QString& src, const QString& delim, int count) const
{
	QString s;
	getStringToken(src, delim, count, s);
	if (s.isEmpty())
		return -1;
	return a62toi(s);
}

QString Server::makeStringFromEscaped(const QString& src) const
{
	struct EscapeChar
	{
		QChar escapechar;
		QChar escapedchar;
	};

	static const EscapeChar escapeChar[] = {
		{ QChar('n'), QChar('\n') },
		{ QChar('c'), QChar(',')},
		{ QChar('z'), QChar('|')},
		{ QChar('y'), QChar('\\') },
	};

	QString result;
	int srclen = src.length();

	for (int i = 0; i < srclen; ++i)
	{
		if (src[i] == '\\' && i + 1 < srclen)
		{
			QChar nextChar = src[i + 1];

			bool isDBCSLeadByte = false;
			if (nextChar.isHighSurrogate() && i + 2 < srclen && src[i + 2].isLowSurrogate())
			{
				result.append(src.midRef(i, 2));
				i += 2;
				isDBCSLeadByte = true;
			}

			if (!isDBCSLeadByte)
			{
				for (int j = 0; j < sizeof(escapeChar) / sizeof(escapeChar[0]); ++j)
				{
					if (escapeChar[j].escapedchar == nextChar)
					{
						result.append(escapeChar[j].escapechar);
						i++;
						break;
					}
				}
			}
		}
		else
		{
			result.append(src[i]);
		}
	}

	return result;
}

#if 0
// 0-9,a-z(10-35),A-Z(36-61)
int Server::a62toi(char* a) const
{
	int ret = 0;
	int fugo = 1;

	while (*a != NULL)
	{
		ret *= 62;
		if ('0' <= (*a) && (*a) <= '9')
			ret += (*a) - '0';
		else
		{
			if ('a' <= (*a) && (*a) <= 'z')
				ret += (*a) - 'a' + 10;
			else
			{
				if ('A' <= (*a) && (*a) <= 'Z')
					ret += (*a) - 'A' + 36;
				else
				{
					if (*a == '-')
						fugo = -1;
					else
						return 0;
				}
			}
		}
		a++;
	}
	return ret * fugo;
}
#endif

void Server::clearNetBuffer()
{
	//memset(net_readbuf, 0, sizeof(net_readbuf));
	net_readbuf.clear();
	//memset(rpc_linebuffer, 0, sizeof(rpc_linebuffer));
	Autil::util_Clear();
}

int Server::appendReadBuf(const QByteArray& data)
{
	net_readbuf.append(data);
	return 0;
}

QByteArrayList Server::splitLinesFromReadBuf()
{
	QByteArrayList lines = net_readbuf.split('\n'); // Split net_readbuf into lines

	if (!net_readbuf.endsWith('\n'))
	{
		// The last line is incomplete, remove it from the list and keep it in net_readbuf
		int lastIndex = lines.size() - 1;
		net_readbuf = lines[lastIndex];
		lines.removeAt(lastIndex);
	}
	else
	{
		// net_readbuf does not contain any incomplete line
		net_readbuf.clear();
	}

	for (int i = 0; i < lines.size(); ++i)
	{
		// Remove '\r' from each line
		lines[i] = lines[i].replace('\r', "");
	}

	return lines;
}

#pragma endregion

inline constexpr bool checkAND(unsigned int a, unsigned int b)
{
	return (a & b) == b;
}

#pragma region Net
Server::Server(QObject* parent)
	: ThreadPlugin(parent)
	, chatQueue(MAX_CHAT_HISTORY)
{
	loginTimer.start();
	battleDurationTimer.start();
	normalDurationTimer.start();
	eottlTimer.start();
	connectingTimer.start();
	repTimer.start();

	Autil::util_Init();
	clearNetBuffer();

	sync_.setCancelOnWait(true);
	ayncBattleCommandSync.setCancelOnWait(true);

	mapAnalyzer.reset(new MapAnalyzer);
}

Server::~Server()
{
	requestInterruption();
	clearNetBuffer();

	server_->close();
	for (QTcpSocket* clientSocket : clientSockets_)
	{
		clientSocket->close();
	}

	for (QTcpSocket* clientSocket : clientSockets_)
	{
		if (clientSocket == nullptr)
			continue;

		if (clientSocket->state() == QAbstractSocket::ConnectedState)
			clientSocket->waitForDisconnected();
		delete clientSocket;
	}

	clientSockets_.clear();

	ayncBattleCommandFlag.store(true, std::memory_order_release);
	ayncBattleCommandSync.waitForFinished();
	ayncBattleCommandSync.clearFutures();
	ayncBattleCommandFlag.store(false, std::memory_order_release);
	sync_.waitForFinished();
	mapAnalyzer.reset(nullptr);
	qDebug() << "Server is distroyed";
}

//用於清空部分數據 主要用於登出後清理數據避免數據混亂，每次登出後都應該清除大部分的基礎訊息
void Server::clear()
{
	enemyNameListCache.clear();
	mapUnitHash.clear();
	chatQueue.clear();
	for (int i = 0; i < MAX_PET + 1; ++i)
		recorder[i] = {};
	battleData = {};
	currentDialog = dialog_t{};

	// 清理 MAIL_HISTORY 数组中的每个元素
	for (int i = 0; i < MAX_ADR_BOOK; ++i)
	{
		MailHistory[i] = {};
	}

	// 清理 PET 数组中的每个元素
	for (int i = 0; i < MAX_PET; ++i)
	{
		pet[i] = {};
	}

	// 清理 PROFESSION_SKILL 数组中的每个元素
	for (int i = 0; i < MAX_PROFESSION_SKILL; ++i)
	{
		profession_skill[i] = {};
	}

	// 清理 MAGIC 数组中的每个元素
	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		magic[i] = {};
	}

	// 清理 PARTY 数组中的每个元素
	for (int i = 0; i < MAX_PARTY; ++i)
	{
		party[i] = {};
	}

	// 清理 CHARLISTTABLE 数组中的每个元素
	for (int i = 0; i < MAXCHARACTER; ++i)
	{
		chartable[i] = {};
	}

	// 清理 ADDRESS_BOOK 数组中的每个元素
	for (int i = 0; i < MAX_ADR_BOOK; ++i)
	{
		addressBook[i] = {};
	}

	battleResultMsg = {};

	// 清理 PET_SKILL 数组中的每个元素
	for (int i = 0; i < MAX_PET; ++i)
	{
		for (int j = 0; j < MAX_SKILL; ++j)
		{
			petSkill[i][j] = {};
		}
	}

	// 清理 JOBDAILY 数组中的每个元素
	for (int i = 0; i < MAXMISSION; ++i)
	{
		jobdaily[i] = {};
	}
}

//啟動TCP服務端，並監聽系統自動配發的端口
bool Server::start(QObject* parent)
{
	server_.reset(new QTcpServer(parent));
	if (server_.isNull())
		return false;

	connect(server_.data(), &QTcpServer::newConnection, this, &Server::onNewConnection);

	if (!server_->listen(QHostAddress::AnyIPv6, 0))
	{
		qDebug() << "Failed to listen on socket";
		return false;
	}

	port_ = server_->serverPort();
	return true;
}

//異步接收客戶端連入通知
void Server::onNewConnection()
{
	QTcpSocket* clientSocket = server_->nextPendingConnection();
	if (!clientSocket)
		return;

	clientSockets_.append(clientSocket);
	clientSocket->setReadBufferSize(8191);

	//clientSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
	clientSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	clientSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 8191);
	clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 8191);

	connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onClientReadyRead);
	connect(this, &Server::write, this, &Server::onWrite, Qt::QueuedConnection);
}

//異步接收客戶端數據
void Server::onClientReadyRead()
{
	QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
	if (!clientSocket)
		return;
	QByteArray badata = clientSocket->readAll();

	sync_.addFuture(QtConcurrent::run([this, clientSocket, badata]()
		{
			QMutexLocker lock(&net_mutex);

			QString preStr = QString::fromUtf8(badata);
			if (preStr.startsWith("dc"))
			{
				setBattleFlag(false);
				setOnlineFlag(false);
				return;
			}
			else if (preStr.startsWith("bPK"))
			{
				Injector& injector = Injector::getInstance();
				int value = mem::read<short>(injector.getProcess(), injector.getProcessModule() + 0xE21E4);
				announce(QString("[async battle] 战斗面板生成了 类别:%1").arg(value));
				isBattleDialogReady.store(true, std::memory_order_release);
				doBattleWork(false);//sync
				return;
			}
			else if (preStr.startsWith("dk|"))
			{
				//remove dk|s
				preStr = preStr.mid(3);
				lssproto_CustomWN_recv(preStr);
				return;
			}
			else if (preStr.startsWith("tk|"))
			{
				//remove tk|s
				preStr = preStr.mid(3);
				lssproto_CustomTK_recv(preStr);
				return;
			}

			handleData(clientSocket, badata);
		}));
}

//異步發送數據
void Server::onWrite(QTcpSocket* clientSocket, QByteArray ba, int size)
{
	if (clientSocket && clientSocket->state() != QAbstractSocket::UnconnectedState)
	{
		clientSocket->write(ba, size);
		clientSocket->flush();
	}
}

//異步處理數據
void Server::handleData(QTcpSocket*, QByteArray badata)
{
	//char rpc_linebuffer[65536] = {};//Autil::NETBUFSIZ
	//int len = badata.size();
	//memset(rpc_linebuffer, 0, sizeof(rpc_linebuffer));
	//memcpy_s(rpc_linebuffer, sizeof(rpc_linebuffer), badata, len);
	//_snprintf_s(rpc_linebuffer, sizeof(rpc_linebuffer), _TRUNCATE, "%s", badata);

	appendReadBuf(badata);

	if (net_readbuf.isEmpty())
	{
		//emit write(clientSocket, badata, len);

		return;
	}

	//qDebug() << "Received " << badata.size() << " bytes from client but actual len is:" << badata.trimmed().size();
	SPD_LOG(g_logger_name, QString("[proto] Received %1 bytes from client but actual len is: %2").arg(badata.size()).arg(badata.trimmed().size()));

	Injector& injector = Injector::getInstance();
	QString key = mem::readString(injector.getProcess(), injector.getProcessModule() + kOffestPersonalKey, Autil::PERSONALKEYSIZE, true, true);
	if (key != Autil::PersonalKey)
		Autil::PersonalKey = key;


	QByteArrayList dataList = splitLinesFromReadBuf();
	if (dataList.isEmpty())
		return;

	SPD_LOG(g_logger_name, QString("[proto] Received %1 lines from client").arg(dataList.size()));

	for (QByteArray& ba : dataList)
	{
		if (isInterruptionRequested())
			break;

		//memset(rpc_linebuffer, 0, sizeof(rpc_linebuffer));
		// get line from read buffer
		if (!ba.isEmpty())
		{
			int ret = saDispatchMessage(ba.data());

			if (ret < 0)
			{
				qDebug() << "************************ LSSPROTO_END ************************";
				//代表此段數據已到結尾
				Autil::util_Clear();
				break;
			}
			else if (ret == BC_NEED_TO_CLEAN)
			{
				qDebug() << "************************ CLEAR_BUFFER ************************";
				//錯誤的數據 或 需要清除緩存
				clearNetBuffer();
				break;
			}
		}
		else
		{
			qDebug() << "************************ DONE_BUFFER ************************";
			//數據讀完了
			Autil::util_Clear();
		}
	}

	//emit write(clientSocket, sendBuf.data(), len);
}

//經由 handleData 調用同步解析數據
int Server::saDispatchMessage(char* encoded)
{
	int	func = 0, fieldcount = 0;
	int	iChecksum = 0, iChecksumrecv = 0;

	QByteArray raw(Autil::NETBUFSIZ, '\0');
	QByteArray data(Autil::NETDATASIZE, '\0');
	QByteArray result(Autil::NETDATASIZE, '\0');
	//char raw[NETBUFSIZ] = {};
	//memset(raw, 0, sizeof(raw));

	Autil::util_DecodeMessage(raw.data(), Autil::NETBUFSIZ, encoded);
	Autil::util_SplitMessage(raw.data(), Autil::NETBUFSIZ, const_cast<char*>(Autil::SEPARATOR));
	if (Autil::util_GetFunctionFromSlice(&func, &fieldcount) != 1)
	{
		return 0;
	}

	//qDebug() << "fun" << func << "fieldcount" << fieldcount;

	if (func == LSSPROTO_ERROR_RECV)
	{
		return -1;
	}

	SPD_LOG(g_logger_name, QString("[proto] lssproto func: %1").arg(func));

	switch (func)
	{
	case LSSPROTO_XYD_RECV: /*戰後刷新人物座標、方向2*/
	{
		int x = 0;
		int y = 0;
		int dir = 0;
		iChecksum += Autil::util_deint(2, &x);
		iChecksum += Autil::util_deint(3, &y);
		iChecksum += Autil::util_deint(4, &dir);

		Autil::util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_XYD_RECV" << "x" << x << "y" << y << "dir" << dir;
		lssproto_XYD_recv(QPoint(x, y), dir);
		break;
	}
	case LSSPROTO_EV_RECV: /*WRAP 4*/
	{
		int seqno = 0;
		int result = 0;
		iChecksum += Autil::util_deint(2, &seqno);
		iChecksum += Autil::util_deint(3, &result);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_EV_RECV" << "seqno" << seqno << "result" << result;
		lssproto_EV_recv(seqno, result);
		break;
	}
	case LSSPROTO_EN_RECV: /*Battle EncountFlag //開始戰鬥 7*/
	{
		int result = 0;
		int field = 0;
		iChecksum += Autil::util_deint(2, &result);
		iChecksum += Autil::util_deint(3, &field);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_EN_RECV" << "result" << result << "field" << field;
		lssproto_EN_recv(result, field);
		break;
	}
	case LSSPROTO_RS_RECV: /*戰後獎勵 12*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_RS_RECV" << util::toUnicode(data.data());
		lssproto_RS_recv(data.data());
		break;
	}
	case LSSPROTO_RD_RECV:/*戰後經驗 13*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_RD_RECV" << util::toUnicode(data.data());
		lssproto_RD_recv(data.data());
		break;
	}
	case LSSPROTO_B_RECV: /*每回合開始的戰場資訊 15*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_B_RECV" << util::toUnicode(data.data());
		lssproto_B_recv(data.data());
		break;
	}
	case LSSPROTO_I_RECV: /*物品變動 22*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_I_RECV" << util::toUnicode(data.data());
		lssproto_I_recv(data.data());
		break;
	}
	case LSSPROTO_SI_RECV:/* 道具位置交換24*/
	{
		int fromindex;
		int toindex;

		iChecksum += Autil::util_deint(2, &fromindex);
		iChecksum += Autil::util_deint(3, &toindex);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_SI_RECV" << "fromindex" << fromindex << "toindex" << toindex;
		lssproto_SI_recv(fromindex, toindex);
		break;
	}
	case LSSPROTO_MSG_RECV:/*收到郵件26*/
	{
		int aindex;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		int color;

		iChecksum += Autil::util_deint(2, &aindex);
		iChecksum += Autil::util_destring(3, data.data());
		iChecksum += Autil::util_deint(4, &color);
		Autil::util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_MSG_RECV" << util::toUnicode(data.data());
		lssproto_MSG_recv(aindex, data.data(), color);
		break;
	}
	case LSSPROTO_PME_RECV:/*28*/
	{
		int objindex;
		int graphicsno;
		int x;
		int y;
		int dir;
		int flg;
		int no;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &objindex);
		iChecksum += Autil::util_deint(3, &graphicsno);
		iChecksum += Autil::util_deint(4, &x);
		iChecksum += Autil::util_deint(5, &y);
		iChecksum += Autil::util_deint(6, &dir);
		iChecksum += Autil::util_deint(7, &flg);
		iChecksum += Autil::util_deint(8, &no);
		iChecksum += Autil::util_destring(9, data.data());
		Autil::util_deint(10, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_PME_RECV" << "objindex" << objindex << "graphicsno" << graphicsno <<
			"x" << x << "y" << y << "dir" << dir << "flg" << flg << "no" << no << "cdata" << util::toUnicode(data.data());
		lssproto_PME_recv(objindex, graphicsno, QPoint(x, y), dir, flg, no, data.data());
		break;
	}
	case LSSPROTO_AB_RECV:/* 30*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_AB_RECV" << util::toUnicode(data.data());
		lssproto_AB_recv(data.data());
		break;
	}
	case LSSPROTO_ABI_RECV:/*名片數據31*/
	{
		int num;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_deint(2, &num);
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_ABI_RECV" << "num" << num << "data" << util::toUnicode(data.data());
		lssproto_ABI_recv(num, data.data());
		break;
	}
	case LSSPROTO_TK_RECV: /*收到對話36*/
	{
		int index;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		int color;

		iChecksum += Autil::util_deint(2, &index);
		iChecksum += Autil::util_destring(3, data.data());
		iChecksum += Autil::util_deint(4, &color);
		Autil::util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_TK_RECV" << "index" << index << "message" << util::toUnicode(data.data()) << "color" << color;
		lssproto_TK_recv(index, data.data(), color);
		break;
	}
	case LSSPROTO_MC_RECV: /*地圖數據更新，重新繪製地圖37*/
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		int tilesum;
		int objsum;
		int eventsum;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &fl);
		iChecksum += Autil::util_deint(3, &x1);
		iChecksum += Autil::util_deint(4, &y1);
		iChecksum += Autil::util_deint(5, &x2);
		iChecksum += Autil::util_deint(6, &y2);
		iChecksum += Autil::util_deint(7, &tilesum);
		iChecksum += Autil::util_deint(8, &objsum);
		iChecksum += Autil::util_deint(9, &eventsum);
		iChecksum += Autil::util_destring(10, data.data());
		Autil::util_deint(11, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_MC_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 <<
			"tilesum" << tilesum << "objsum" << objsum << "eventsum" << eventsum << "data" << util::toUnicode(data.data());
		lssproto_MC_recv(fl, x1, y1, x2, y2, tilesum, objsum, eventsum, data.data());
		//m_map.name = qmap;
		break;
	}
	case LSSPROTO_M_RECV: /*地圖數據更新，重新寫入地圖2 39*/
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &fl);
		iChecksum += Autil::util_deint(3, &x1);
		iChecksum += Autil::util_deint(4, &y1);
		iChecksum += Autil::util_deint(5, &x2);
		iChecksum += Autil::util_deint(6, &y2);
		iChecksum += Autil::util_destring(7, data.data());
		Autil::util_deint(8, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_M_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 << "data" << util::toUnicode(data.data());
		lssproto_M_recv(fl, x1, y1, x2, y2, data.data());
		//m_map.floor = fl;
		break;
	}
	case LSSPROTO_C_RECV: /*服務端發送的靜態信息，可用於顯示玩家，其它玩家，公交，寵物等信息 41*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_C_RECV" << util::toUnicode(data.data());
		lssproto_C_recv(data.data());
		break;
	}
	case LSSPROTO_CA_RECV: /*//周圍人、NPC..等等狀態改變必定是 _C_recv已經新增過的單位 42*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CA_RECV" << util::toUnicode(data.data());
		lssproto_CA_recv(data.data());
		break;
	}
	case LSSPROTO_CD_RECV: /*刪除指定一個或多個周圍人、NPC單位 43*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CD_RECV" << util::toUnicode(data.data());
		lssproto_CD_recv(data.data());
		break;
	}
	case LSSPROTO_R_RECV:
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_R_RECV" << util::toUnicode(data.data());
		lssproto_R_recv(data.data());
		break;
	}
	case LSSPROTO_S_RECV: /*更新所有基礎資訊 46*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_S_RECV" << util::toUnicode(data.data());
		lssproto_S_recv(data.data());
		break;
	}
	case LSSPROTO_D_RECV:/*47*/
	{
		int category;
		int dx;
		int dy;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &category);
		iChecksum += Autil::util_deint(3, &dx);
		iChecksum += Autil::util_deint(4, &dy);
		iChecksum += Autil::util_destring(5, data.data());
		Autil::util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_D_RECV" << "category" << category << "dx" << dx << "dy" << dy << "data" << util::toUnicode(data.data());
		lssproto_D_recv(category, dx, dy, data.data());
		break;
	}
	case LSSPROTO_FS_RECV:/*開關切換 49*/
	{
		int flg;

		iChecksum += Autil::util_deint(2, &flg);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_FS_RECV" << "flg" << flg;
		lssproto_FS_recv(flg);
		break;
	}
	case LSSPROTO_HL_RECV:/*51*/
	{
		int flg;

		iChecksum += Autil::util_deint(2, &flg);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_HL_RECV" << "flg" << flg;
		lssproto_HL_recv(flg);
		break;
	}
	case LSSPROTO_PR_RECV:/*組隊變化 53*/
	{
		int request;
		int result;

		iChecksum += Autil::util_deint(2, &request);
		iChecksum += Autil::util_deint(3, &result);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_PR_RECV" << "request" << request << "result" << result;
		lssproto_PR_recv(request, result);
		break;
	}
	case LSSPROTO_KS_RECV: /*寵物更換狀態55*/
	{
		int petarray;
		int result;

		iChecksum += Autil::util_deint(2, &petarray);
		iChecksum += Autil::util_deint(3, &result);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_KS_RECV" << "petarray" << petarray << "result" << result;
		lssproto_KS_recv(petarray, result);
		break;
	}
	case LSSPROTO_PS_RECV:
	{
		int result;
		int havepetindex;
		int havepetskill;
		int toindex;

		iChecksum += Autil::util_deint(2, &result);
		iChecksum += Autil::util_deint(3, &havepetindex);
		iChecksum += Autil::util_deint(4, &havepetskill);
		iChecksum += Autil::util_deint(5, &toindex);
		Autil::util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_PS_RECV" << "result" << result << "havepetindex" << havepetindex << "havepetskill" << havepetskill << "toindex" << toindex;
		lssproto_PS_recv(result, havepetindex, havepetskill, toindex);
		break;
	}
	case LSSPROTO_SKUP_RECV: /*更新點數 63*/
	{
		int point;

		iChecksum += Autil::util_deint(2, &point);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_SKUP_RECV" << "point" << point;
		lssproto_SKUP_recv(point);
		break;
	}
	case LSSPROTO_WN_RECV:/*NPC對話框 66*/
	{
		int windowtype;
		int buttontype;
		int seqno;
		int objindex;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &windowtype);
		iChecksum += Autil::util_deint(3, &buttontype);
		iChecksum += Autil::util_deint(4, &seqno);
		iChecksum += Autil::util_deint(5, &objindex);
		iChecksum += Autil::util_destring(6, data.data());
		Autil::util_deint(7, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_WN_RECV" << "windowtype" << windowtype << "buttontype" << buttontype << "seqno" << seqno << "objindex" << objindex << "data" << util::toUnicode(data.data());
		lssproto_WN_recv(windowtype, buttontype, seqno, objindex, data.data());
		break;
	}
	case LSSPROTO_EF_RECV: /*天氣68*/
	{
		int effect;
		int level;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &effect);
		iChecksum += Autil::util_deint(3, &level);
		iChecksum += Autil::util_destring(4, data.data());
		Autil::util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_EF_RECV" << "effect" << effect << "level" << level << "option" << util::toUnicode(data.data());
		lssproto_EF_recv(effect, level, data.data());
		break;
	}
	case LSSPROTO_SE_RECV:/*69*/
	{
		int x;
		int y;
		int senumber;
		int sw;

		iChecksum += Autil::util_deint(2, &x);
		iChecksum += Autil::util_deint(3, &y);
		iChecksum += Autil::util_deint(4, &senumber);
		iChecksum += Autil::util_deint(5, &sw);
		Autil::util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_SE_RECV" << "x" << x << "y" << y << "senumber" << senumber << "sw" << sw;
		lssproto_SE_recv(QPoint(x, y), senumber, sw);
		break;
	}
	case LSSPROTO_CLIENTLOGIN_RECV:/*選人畫面 72*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CLIENTLOGIN_RECV" << util::toUnicode(data.data());
		lssproto_ClientLogin_recv(data.data());

		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CREATENEWCHAR_RECV:/*人物新增74*/
	{
		//char result[Autil::NETDATASIZE] = {}; memset(result, 0, sizeof(result));
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, result.data());
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CREATENEWCHAR_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CreateNewChar_recv(result.data(), data.data());
		break;
	}
	case LSSPROTO_CHARDELETE_RECV:/*人物刪除 76*/
	{
		//char result[Autil::NETDATASIZE] = {}; memset(result, 0, sizeof(result));
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, result.data());
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CHARDELETE_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharDelete_recv(result.data(), data.data());
		break;
	}
	case LSSPROTO_CHARLOGIN_RECV: /*成功登入 78*/
	{
		//char result[Autil::NETDATASIZE] = {}; memset(result, 0, sizeof(result));
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, result.data());
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CHARLOGIN_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharLogin_recv(result.data(), data.data());
		break;
	}
	case LSSPROTO_CHARLIST_RECV:/*選人頁面資訊 80*/
	{
		//char result[Autil::NETDATASIZE] = {}; memset(result, 0, sizeof(result));
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, result.data());
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_CHARLIST_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharList_recv(result.data(), data.data());

		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARLOGOUT_RECV:/*登出 82*/
	{
		//char result[Autil::NETDATASIZE] = {}; memset(result, 0, sizeof(result));
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, result.data());
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_CHARLOGOUT_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharLogout_recv(result.data(), data.data());
		break;
	}
	case LSSPROTO_PROCGET_RECV:/*84*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_PROCGET_RECV" << util::toUnicode(data.data());
		lssproto_ProcGet_recv(data.data());
		break;
	}
	case LSSPROTO_PLAYERNUMGET_RECV:/*86*/
	{
		int logincount;
		int player;

		iChecksum += Autil::util_deint(2, &logincount);
		iChecksum += Autil::util_deint(3, &player);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_PLAYERNUMGET_RECV" << "logincount:" << logincount << "player:" << player; //"logincount:%d player:%d\n
		lssproto_PlayerNumGet_recv(logincount, player);
		break;
	}
	case LSSPROTO_ECHO_RECV: /*伺服器定時ECHO "hoge" 88*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_ECHO_RECV" << util::toUnicode(data.data());
		lssproto_Echo_recv(data.data());
		break;
	}
	case LSSPROTO_NU_RECV: /*不知道幹嘛的 90*/
	{
		int AddCount;

		iChecksum += Autil::util_deint(2, &AddCount);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_NU_RECV" << "AddCount:" << AddCount;
		lssproto_NU_recv(AddCount);
		break;
	}
	case LSSPROTO_TD_RECV:/*92*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_TD_RECV" << util::toUnicode(data.data());
		lssproto_TD_recv(data.data());
		break;
	}
	case LSSPROTO_FM_RECV:/*家族頻道93*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_FM_RECV" << util::toUnicode(data.data());
		lssproto_FM_recv(data.data());
		break;
	}
	case LSSPROTO_WO_RECV:/*95*/
	{
		int effect;

		iChecksum += Autil::util_deint(2, &effect);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_WO_RECV" << "effect:" << effect;
		lssproto_WO_recv(effect);
		break;
	}
	case LSSPROTO_NC_RECV: /*沈默? 101* 戰鬥結束*/
	{
		int flg = 0;
		iChecksum += Autil::util_deint(2, &flg);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_NC_RECV" << "flg:" << flg;
		lssproto_NC_recv(flg);
		break;
	}
	case LSSPROTO_CS_RECV:/*固定客戶端的速度104*/
	{
		int deltimes = 0;
		iChecksum += Autil::util_deint(2, &deltimes);
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_CS_RECV" << "deltimes:" << deltimes;
		lssproto_CS_recv(deltimes);
		break;
	}
	case LSSPROTO_PETST_RECV: /*寵物狀態改變 107*/
	{
		int petarray;
		int result;

		iChecksum += Autil::util_deint(2, &petarray);
		iChecksum += Autil::util_deint(3, &result);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_PETST_RECV" << "petarray:" << petarray << "result:" << result;
		lssproto_PETST_recv(petarray, result);
		break;
	}
	case LSSPROTO_SPET_RECV: /*寵物更換狀態115*/
	{
		int standbypet;
		int result;

		iChecksum += Autil::util_deint(2, &standbypet);
		iChecksum += Autil::util_deint(3, &result);
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_SPET_RECV" << "standbypet:" << standbypet << "result:" << result;
		lssproto_SPET_recv(standbypet, result);
		break;
	}
	case LSSPROTO_JOBDAILY_RECV:/*任務日誌120*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_JOBDAILY_RECV" << util::toUnicode(data.data());
		lssproto_JOBDAILY_recv(data.data());
		break;
	}
	case LSSPROTO_TEACHER_SYSTEM_RECV:/*導師系統123*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_TEACHER_SYSTEM_RECV" << util::toUnicode(data.data());
		lssproto_TEACHER_SYSTEM_recv(data.data());
		break;
	}
	case LSSPROTO_FIREWORK_RECV:/*煙火?126*/
	{
		iChecksum = 0;
		iChecksumrecv = 0;
		int iCharaindex, iType, iActionNum;

		iChecksum += Autil::util_deint(2, &iCharaindex);
		iChecksum += Autil::util_deint(3, &iType);
		iChecksum += Autil::util_deint(4, &iActionNum);
		Autil::util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_FIREWORK_RECV" << "iCharaindex:" << iCharaindex << "iType:" << iType << "iActionNum:" << iActionNum;
		lssproto_Firework_recv(iCharaindex, iType, iActionNum);
		break;
	}
	case LSSPROTO_CHAREFFECT_RECV:/*146*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_destring(2, data.data());
		Autil::util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_CHAREFFECT_RECV" << util::toUnicode(data.data());
		lssproto_CHAREFFECT_recv(data.data());
		break;
	}
	case LSSPROTO_IMAGE_RECV:/*151*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		int x = 0;
		int y = 0;
		int z = 0;
		//int iChecksumrecv;

		iChecksum += Autil::util_destring(2, data.data());
		iChecksum += Autil::util_deint(3, &x);
		iChecksum += Autil::util_deint(4, &y);
		iChecksum += Autil::util_deint(5, &z);
		Autil::util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		//base64解碼
		//QByteArray str = QByteArray::fromBase64(data.data());
		//QImage image = QImage::fromData(str);
		//image.save("demo.png");
		break;
	}
	case LSSPROTO_DENGON_RECV:/*200*/
	{
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));
		int coloer;
		int num;

		iChecksum += Autil::util_destring(2, data.data());
		iChecksum += Autil::util_deint(3, &coloer);
		iChecksum += Autil::util_deint(4, &num);
		Autil::util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;

		qDebug() << "LSSPROTO_DENGON_RECV" << util::toUnicode(data.data()) << "coloer:" << coloer << "num:" << num;
		lssproto_DENGON_recv(data.data(), coloer, num);
		break;
	}
	case LSSPROTO_SAMENU_RECV:/*201*/
	{
		int count;
		//char data[Autil::NETDATASIZE] = {}; memset(data, 0, sizeof(data.data()));

		iChecksum += Autil::util_deint(2, &count);
		iChecksum += Autil::util_destring(3, data.data());
		Autil::util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
			return 0;


		qDebug() << "LSSPROTO_SAMENU_RECV" << "count:" << count << util::toUnicode(data.data());
		break;
	}
	case 220://SE SO驗證圖
	{
		break;
	}
	default:
	{
		qDebug() << "-------------------fun" << func << "fieldcount" << fieldcount;
		break;
	}
	}
	Autil::SliceCount = 0;
	return BC_ABOUT_TO_END;

}
#pragma endregion

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region GET
//用於判斷畫面的狀態的數值 (9平時 10戰鬥 <8非登入)
int Server::getWorldStatus()
{
	QReadLocker lock(&worldStateLocker);
	Injector& injector = Injector::getInstance();
	return mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestWorldStatus);
}

//用於判斷畫面或動畫狀態的數值 (平時一般是3 戰鬥中選擇面板是4 戰鬥動作中是5或6，平時還有很多其他狀態值)
int Server::getGameStatus()
{
	QReadLocker lock(&gameStateLocker);
	Injector& injector = Injector::getInstance();
	return mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestGameStatus);
}

bool Server::checkWG(int w, int g)
{
	return getWorldStatus() == w && getGameStatus() == g;
}

//檢查非登入時所在頁面
int Server::getUnloginStatus()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	int W = getWorldStatus();
	int G = getGameStatus();

	if (0 == W && 0 == G)
	{
		return util::kStatusDisappear;//窗口不存在
	}
	if (11 == W && 2 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusDisconnected);
		return util::kStatusDisconnect;//斷線
	}
	else if ((2 == W && 5 == G) || (6 == W && 1 == G))//|| (9 == W && 102 == G) || (9 == W && 0 == G) || (9 == W && 103 == G)
	{
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusConnecting);
		return util::kStatusConnecting;//連線中
	}
	else if (3 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusBusy);
		return util::kStatusBusy;//忙碌
	}
	else if (2 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusTimeout);
		return util::kStatusTimeout;//逾時
	}
	else if (1 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelNoUserNameOrPassword);
		return util::kNoUserNameOrPassword;//無帳號密碼
	}
	else if ((1 == W && 2 == G) || (1 == W && 3 == G))
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLogining);
		return util::kStatusInputUser;//輸入帳號密碼
	}
	else if (2 == W && 2 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSelectServer);
		return util::kStatusSelectServer;//選擇伺服器
	}
	else if (2 == W && 3 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSelectSubServer);
		return util::kStatusSelectSubServer;//選擇子伺服器(分流)
	}
	else if ((3 == W && 11 == G) || (3 == W && 1 == G))
	{
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSelectPosition);
		return util::kStatusSelectCharacter;//選擇人物
	}
	else if ((9 == W && 200 == G) || (9 == W && 201 == G) || (9 == W && 202 == G) || (9 == W && 203 == G) || (9 == W && 204 == G))//刷新
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 1 == G)//切回平時
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 2 == G)//切回平時2
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 20 == G)//切換場景
	{
		return util::kStatusUnknown;
	}
	else if ((3 == W && 0 == G) || (3 == W && 10 == G))//????選人畫面->登入
	{
		return util::kStatusUnknown;
	}
	else if (5 == W && 0 == G)//????登入後->平時
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 100 == G)//????登入後->平時2
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 101 == G)//????登入後->平時3
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 103 == G)//????登入後->平時3
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 4 == G)//轉移入戰鬥
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 0 == G)//轉移入戰鬥
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 1 == G)//轉移入戰鬥2
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 2 == G)//戰鬥中動作之後?
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 3 == G)//戰鬥前置
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 4 == G)//戰鬥切出面板
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 5 == G)//戰鬥面板結束
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 6 == G)//戰鬥動作中
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 7 == G)//????戰鬥->平時
	{
		return util::kStatusUnknown;
	}
	else if (10 == W && 8 == G)//????戰鬥->平時2
	{
		return util::kStatusUnknown;
	}
	else if (9 == W && 3 == G)
		return util::kStatusLogined;//已丁入(平時且無其他對話框或特殊場景)

	qDebug() << "getUnloginStatus: " << W << " " << G;
	return util::kStatusUnknown;
}

//計算人物最單物品大堆疊數(負重量)
void Server::getPlayerMaxCarryingCapacity()
{
	PC pc = getPC();
	int nowMaxload = pc.maxload;
	switch (pc.transmigration)
	{
	case 0:
		pc.maxload = 3;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		pc.maxload = 3 + pc.transmigration;
		break;
	case 5:
		pc.maxload = 10;
		break;
	case 6:
		pc.maxload = 11;
		break;
	}

	//取腰带的负重
	ITEM item = pc.item[CHAR_EQBELT];
	if (!item.name.isEmpty())
	{
		//負重|负重
		static const QRegularExpression re("負重|负重");
		int index = item.memo.indexOf(re);
		if (index != -1)
		{

			QString buf = item.memo.mid(index + 3);
			bool ok = false;
			int value = buf.toInt(&ok);
			if (ok && value > 0)
				pc.maxload += value;
		}
	}

	if (pc.maxload < nowMaxload)
		pc.maxload = nowMaxload;

	setPC(pc);
}

int Server::getPartySize() const
{
	int count = 0;
	PC pc = getPC();
	if ((pc.status & CHR_STATUS_LEADER) || (pc.status & CHR_STATUS_PARTY))
	{
		for (int i = 0; i < MAX_PARTY; ++i)
		{
			PARTY party = getParty(i);
			if (party.useFlag <= 0)
				continue;
			if (party.level <= 0)
				continue;

			++count;
		}
	}
	return count;
}

QString Server::getChatHistory(int index)
{
	if (index < 0 || index > 19)
		return "\0";

	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();

	int total = mem::read<int>(hProcess, hModule + kOffestChatBufferMaxCount);
	if (index > total)
		return "\0";

	//int maxptr = mem::read<int>(hProcess, hModule + 0x146278);

	constexpr int MAX_CHAT_BUFFER = 0x10C;
	int ptr = hModule + kOffestChatBuffer + ((total - index) * MAX_CHAT_BUFFER);

	return mem::readString(hProcess, ptr, 0x10C, false);
}

//獲取周圍玩家名稱列表
QStringList Server::getJoinableUnitList() const
{
	Injector& injector = Injector::getInstance();
	QString leader = injector.getStringHash(util::kAutoFunNameString).simplified();
	QStringList unitNameList;
	if (!leader.isEmpty())
		unitNameList.append(leader);

	PC pc = getPC();
	for (const mapunit_t& unit : mapUnitHash)
	{
		QString newNpcName = unit.name.simplified();
		if (newNpcName.isEmpty() || (unit.objType != util::OBJ_HUMAN))
			continue;

		if (!leader.isEmpty() && newNpcName == leader)
			continue;

		if (newNpcName == pc.name.simplified())
			continue;

		unitNameList.append(newNpcName);
	}
	return unitNameList;
};

//使用道具名稱枚舉所有道具索引
bool Server::getItemIndexsByName(const QString& name, const QString& memo, QVector<int>* pv, int from, int to)
{
	bool isExact = true;
	QString newName = name.simplified();
	QString newMemo = name.simplified();

	if (name.startsWith("?"))
	{
		isExact = false;
		newName = name.mid(1).simplified();
	}

	PC pc = getPC();

	QVector<int> v;
	for (int i = 0; i < MAX_ITEM; ++i)
	{
		QString itemName = pc.item[i].name.simplified();
		if (itemName.isEmpty() || pc.item[i].useFlag == 0)
			continue;

		QString itemMemo = pc.item[i].memo.simplified();

		if (memo.isEmpty())
		{
			if (isExact && (newName == itemName))
				v.append(i);
			else if (!isExact && (itemName.contains(newName)))
				v.append(i);
		}
		else if (name.isEmpty())
		{
			if (itemMemo.contains(newMemo))
				v.append(i);
		}
		else
		{
			if (isExact && (newName == itemName) && (itemMemo.contains(newMemo)))
				v.append(i);
			else if (!isExact && (itemName.contains(newName)) && (itemMemo.contains(newMemo)))
				v.append(i);
		}
	}

	if (pv)
		*pv = v;
	return !v.isEmpty();
}

//根據道具名稱(或包含說明文)獲取模糊或精確匹配道具索引
int Server::getItemIndexByName(const QString& name, bool isExact, const QString& memo, int from, int to) const
{
	if (name.isEmpty())
		return -1;

	QString newStr = name.simplified();
	QString newMemo = memo.simplified();

	if (newStr.startsWith("?"))
	{
		newStr.remove(0, 1);
		isExact = false;
	}

	PC pc = getPC();

	for (int i = from; i < to; ++i)
	{
		QString itemName = pc.item[i].name.simplified();
		if (itemName.isEmpty() || pc.item[i].useFlag == 0)
			continue;

		QString itemMemo = pc.item[i].memo.simplified();

		if (isExact && newMemo.isEmpty() && (newStr == itemName))
			return i;
		else if (!isExact && newMemo.isEmpty() && itemName.contains(newStr))
			return i;
		else if (isExact && !newMemo.isEmpty() && (itemMemo.contains(newMemo)) && (newStr == itemName))
			return i;
		else if (!isExact && !newMemo.isEmpty() && (itemMemo.contains(newMemo)) && itemName.contains(newStr))
			return i;
	}

	return -1;
}

//使用名稱匹配寵物技能索引和寵物索引
int Server::getPetSkillIndexByName(int& petIndex, const QString& name) const
{
	QString newName = name.simplified();
	if (petIndex == -1)
	{
		for (int j = 0; j < MAX_PET; ++j)
		{
			for (int i = 0; i < MAX_SKILL; ++i)
			{
				QString petSkillName = petSkill[j][i].name.simplified();
				if (petSkillName.isEmpty() || petSkill[j][i].useFlag == 0)
					continue;

				if (petSkillName == newName)
				{
					petIndex = j;
					return i;
				}
				else if (name.startsWith("?"))
				{
					newName.remove(0, 1);
					if (petSkillName.contains(newName))
					{
						petIndex = j;
						return i;
					}
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < MAX_SKILL; ++i)
		{
			QString petSkillName = petSkill[petIndex][i].name.simplified();
			if (petSkillName.isEmpty() || petSkill[petIndex][i].useFlag == 0)
				continue;

			if (petSkillName == newName)
				return i;
			else if (newName.startsWith("?"))
			{
				newName.remove(0, 1);
				if (petSkillName.contains(newName))
					return i;
			}
		}
	}

	return -1;
}

//使用名稱枚舉所有寵物索引
bool Server::getPetIndexsByName(const QString& name, QVector<int>* pv) const
{
	QVector<int> v;
	QStringList nameList = name.simplified().split(util::rexOR, Qt::SkipEmptyParts);

	for (int i = 0; i < MAX_PET; ++i)
	{
		if (v.contains(i))
			continue;

		PET _pet = pet[i];
		QString newPetName = _pet.name.simplified();
		if (newPetName.isEmpty() || _pet.useFlag == 0)
			continue;

		for (QString name : nameList)
		{
			name = name.simplified();
			if (name == newPetName)
			{
				v.append(i);
				break;
			}
			else if (name.startsWith("?"))
			{
				QString newName = name;
				newName.remove(0, 1);
				if (newPetName.contains(newName))
				{
					v.append(i);
					break;
				}
			}
		}
	}

	if (pv)
		*pv = v;

	return !v.isEmpty();
}

//取背包空格索引
int Server::getItemEmptySpotIndex() const
{
	PC pc = getPC();
	for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		QString name = pc.item[i].name.simplified();
		if (name.isEmpty() || pc.item[i].useFlag == 0)
			return i;
	}

	return -1;
}

bool Server::getItemEmptySpotIndexs(QVector<int>* pv) const
{
	PC pc = getPC();
	QVector<int> v;
	for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		QString name = pc.item[i].name.simplified();
		if (name.isEmpty() || pc.item[i].useFlag == 0)
			v.append(i);
	}
	if (pv)
		*pv = v;

	return !v.isEmpty();
}

QString Server::getBadStatusString(unsigned int status)
{
	QStringList temp;
	if (checkAND(status, BC_FLG_DEAD))
		temp.append(QObject::tr("dead")); // 死亡
	if (checkAND(status, BC_FLG_POISON))
		temp.append(QObject::tr("poisoned")); // 中毒
	if (checkAND(status, BC_FLG_PARALYSIS))
		temp.append(QObject::tr("paralyzed")); // 麻痹
	if (checkAND(status, BC_FLG_SLEEP))
		temp.append(QObject::tr("sleep")); // 昏睡
	if (checkAND(status, BC_FLG_STONE))
		temp.append(QObject::tr("petrified")); // 石化
	if (checkAND(status, BC_FLG_DRUNK))
		temp.append(QObject::tr("dizzy")); // 眩晕
	if (checkAND(status, BC_FLG_CONFUSION))
		temp.append(QObject::tr("confused")); // 混乱
	if (checkAND(status, BC_FLG_HIDE))
		temp.append(QObject::tr("hidden")); // 是否隐藏，地球一周
	if (checkAND(status, BC_FLG_REVERSE))
		temp.append(QObject::tr("reverse")); // 反轉
	return temp.join(" ");
}

QString Server::getFieldString(unsigned int field)
{
	switch (field)
	{
	case 1:
		return QObject::tr("earth");
	case 2:
		return QObject::tr("water");
	case 3:
		return QObject::tr("fire");
	case 4:
		return QObject::tr("wind");
	default:
		return QString();
	}
}

QPoint Server::getPoint()
{
	Injector& injector = Injector::getInstance();
	int hModule = injector.getProcessModule();
	if (hModule == 0)
		return QPoint{};

	HANDLE hProcess = injector.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return QPoint{};

	QReadLocker locker(&pointMutex_);
	int x = mem::read<int>(hProcess, hModule + kOffestNowX);
	int y = mem::read<int>(hProcess, hModule + kOffestNowY);
	return QPoint(x, y);
}

//檢查指定任務狀態，並同步等待封包返回
int Server::checkJobDailyState(const QString& missionName)
{
	if (!getOnlineFlag())
		return false;

	if (getBattleFlag())
		return false;

	QString newMissionName = missionName.simplified();
	if (newMissionName.isEmpty())
		return false;

	IS_WAITFOR_JOBDAILY_FLAG = true;
	lssproto_JOBDAILY_send(const_cast<char*>("dyedye"));
	QElapsedTimer timer; timer.start();

	for (;;)
	{
		if (timer.hasExpired(5000))
			break;

		if (!IS_WAITFOR_JOBDAILY_FLAG)
			break;

		QThread::msleep(100);
	}

	bool isExact = true;

	if (newMissionName.startsWith("?"))
	{
		isExact = false;
		newMissionName = newMissionName.mid(1);
	}

	for (const JOBDAILY& it : jobdaily)
	{
		if (!isExact && (it.explain == newMissionName))
			return it.state == QString(u8"已完成") ? 1 : 2;
		else if (!isExact && it.explain.contains(newMissionName))
			return it.state == QString(u8"已完成") ? 1 : 2;
	}

	return 0;
}

//查找指定類型和名稱的單位
bool Server::findUnit(const QString& nameSrc, int type, mapunit_t* punit, const QString& freenameSrc, int modelid)
{
	QList<mapunit_t> units = mapUnitHash.values();

	QString newSrcName = nameSrc.simplified();
	QStringList nameSrcList = newSrcName.split(util::rexOR, Qt::SkipEmptyParts);

	QString newSrcFreeName = freenameSrc.simplified();
	QStringList freeNameSrcList = newSrcFreeName.split(util::rexOR);

	//coord
	if (nameSrcList.size() == 2)
	{
		bool ok = false;
		QPoint point;
		point.setX(nameSrcList.at(0).simplified().toInt(&ok));
		if (!ok)
			return false;

		point.setY(nameSrcList.at(1).simplified().toInt(&ok));
		if (!ok)
			return false;

		for (const mapunit_t& it : units)
		{
			if (it.graNo == 0 || it.graNo == 9999)
				continue;

			if (it.objType != type)
				continue;

			if (it.p == point)
			{
				*punit = it;
				setPlayerFaceToPoint(point);
				return true;
			}
		}
		return false;
	}
	else if (modelid != -1)
	{
		for (const mapunit_t& it : units)
		{
			if (it.graNo == 0 || it.graNo == 9999)
				continue;

			if (it.graNo == modelid)
			{
				*punit = it;
				return true;
			}
		}
	}
	else
	{
		auto check = [&punit, &units, type](QString name, QString freeName)
		{
			name = name.simplified();
			freeName = freeName.simplified();

			for (const mapunit_t& it : units)
			{
				if (it.graNo == 0)
					continue;

				if (it.objType != type)
					continue;

				QString newUnitName = it.name.simplified();
				QString newUnitFreeName = it.freeName.simplified();

				if (freeName.isEmpty())
				{
					if (newUnitName == name)
					{
						*punit = it;
						return true;
					}
					else if (name.startsWith("?"))
					{
						QString newName = name.mid(1).simplified();
						if (newUnitName.contains(newName))
						{
							*punit = it;
							return true;
						}
					}
				}
				else if (name.isEmpty())
				{
					if (newUnitFreeName.contains(freeName))
					{
						*punit = it;
						return true;
					}
				}
				else
				{
					if (newUnitFreeName.isEmpty())
						continue;

					if ((newUnitName == name) && (newUnitFreeName.contains(freeName)))
					{
						*punit = it;
						return true;
					}
					else if (name.startsWith("?"))
					{
						QString newName = name.mid(1).simplified();
						if (newUnitName.contains(newName) && (newUnitFreeName.contains(freeName)))
						{
							*punit = it;
							return true;
						}
					}
				}
			}

			return false;
		};

		for (const auto& tmpName : nameSrcList)
		{
			QString tmpFreeName;
			if (freeNameSrcList.size() > 0)
				tmpFreeName = freeNameSrcList.takeFirst();

			if (check(tmpName, tmpFreeName))
				return true;
		}

		for (const auto& tmpFreeName : nameSrcList)
		{
			QString tmpName;
			if (freeNameSrcList.size() > 0)
				tmpName = nameSrcList.takeFirst();

			if (check(tmpName, tmpFreeName))
				return true;
		}
	}
	return false;
}

//查找非滿血自己寵物或隊友的索引 (主要用於自動吃肉)
int Server::findInjuriedAllie()
{
	PC pc = getPC();
	if (pc.hp < pc.maxHp)
		return 0;

	for (int i = 0; i < MAX_PET; ++i)
	{
		if ((pet[i].hp > 0) && (pet[i].hp < pet[i].maxHp))
			return i + 1;
	}

	for (int i = 0; i < MAX_PARTY; ++i)
	{
		if ((party[i].hp > 0) && (party[i].hp < party[i].maxHp))
			return i + 1 + MAX_PET;
	}

	return 0;
}

//根據名稱和索引查找寵物是否存在
bool Server::matchPetNameByIndex(int index, const QString& cmpname)
{
	if (index < 0 || index >= MAX_PET)
		return false;

	QString newCmpName = cmpname.simplified();
	if (newCmpName.isEmpty())
		return false;

	QString name = pet[index].name.simplified();
	QString freename = pet[index].freeName.simplified();

	if (!name.isEmpty() && newCmpName == name)
		return true;
	if (!freename.isEmpty() && newCmpName == freename)
		return true;

	return false;
}
#pragma endregion

#pragma region SET
//更新敵我索引範圍
void Server::updateCurrentSideRange(battledata_t& bt)
{
	if (BattleMyNo < 10)
	{
		bt.alliemin = 0;
		bt.alliemax = 9;
		bt.enemymin = 10;
		bt.enemymax = 19;
	}
	else
	{
		bt.alliemin = 10;
		bt.alliemax = 19;
		bt.enemymin = 0;
		bt.enemymax = 9;
	}
	setBattleData(bt);
}

//根據索引刷新道具資訊
void Server::refreshItemInfo(int i)
{
	QVariant var;
	QVariantList varList;

	if (i < 0 || i >= MAX_ITEM)
		return;

	PC pc = getPC();
	if (i < CHAR_EQUIPPLACENUM)
	{
		QStringList equipVHeaderList = {
			QObject::tr("head"), QObject::tr("body"), QObject::tr("righthand"), QObject::tr("leftacc"),
			QObject::tr("rightacc"), QObject::tr("belt"), QObject::tr("lefthand"), QObject::tr("shoes"),
			QObject::tr("gloves")
		};

		if (!pc.item[i].name.isEmpty() && (pc.item[i].useFlag != 0))
		{
			if (pc.item[i].name2.isEmpty())
				varList = { equipVHeaderList.at(i), pc.item[i].name, pc.item[i].damage,	pc.item[i].memo };
			else
				varList = { equipVHeaderList.at(i), QString("%1(%2)").arg(pc.item[i].name).arg(pc.item[i].name2), pc.item[i].damage,	pc.item[i].memo };
		}
		else
		{
			varList = { equipVHeaderList.at(i), "", "",	"" };
		}


		var = QVariant::fromValue(varList);

	}
	else
	{
		if (!pc.item[i].name.isEmpty() && (pc.item[i].useFlag != 0))
		{
			if (pc.item[i].name2.isEmpty())
				varList = { i - CHAR_EQUIPPLACENUM + 1, pc.item[i].name, pc.item[i].pile, pc.item[i].damage, pc.item[i].level, pc.item[i].memo };
			else
				varList = { i - CHAR_EQUIPPLACENUM + 1, QString("%1(%2)").arg(pc.item[i].name).arg(pc.item[i].name2), pc.item[i].pile, pc.item[i].damage, pc.item[i].level, pc.item[i].memo };
		}
		else
		{
			varList = { i - CHAR_EQUIPPLACENUM + 1, "", "", "", "", "" };
		}
		var = QVariant::fromValue(varList);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	if (i < CHAR_EQUIPPLACENUM)
	{
		equipInfoRowContents.insert(i, var);
		emit signalDispatcher.updateEquipInfoRowContents(i, var);
	}
	else
	{
		itemInfoRowContents.insert(i, var);
		emit signalDispatcher.updateItemInfoRowContents(i, var);
	}
}

//刷新所有道具資訊
void Server::refreshItemInfo()
{
	for (int i = 0; i < MAX_ITEM; ++i)
	{
		refreshItemInfo(i);
	}
}

void Server::updateItemByMemory()
{
	//本来应该一次性读取整个结构体的，但我们不需要这麽多讯息

	QMutexLocker locker(&swapItemMutex_);
	Injector& injector = Injector::getInstance();
	int hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();
	PC pc = getPC();
	for (int i = 0; i < MAX_ITEM; ++i)
	{
		constexpr int item_offest = 0x184;
		pc.item[i].useFlag = mem::read<short>(hProcess, hModule + 0x422C028 + i * item_offest);
		if (pc.item[i].useFlag != 1)
		{
			pc.item[i] = {};
			continue;
		}

		pc.item[i].name = mem::readString(hProcess, hModule + 0x422C032 + i * item_offest, ITEM_NAME_LEN, true, false);
		pc.item[i].memo = mem::readString(hProcess, hModule + 0x422C060 + i * item_offest, ITEM_MEMO_LEN, true, false);
		if (i >= CHAR_EQUIPPLACENUM)
			pc.item[i].pile = mem::read<short>(hProcess, hModule + 0x422BF58 + i * item_offest);
		else
			pc.item[i].pile = 1;

		if (pc.item[i].pile == 0)
			pc.item[i].pile = 1;
	}
	setPC(pc);
}

//讀取內存刷新各種基礎數據，有些封包數據不明確、或不確定，用來補充不足的部分
void Server::updateDatasFromMemory()
{
	Injector& injector = Injector::getInstance();
	if (!getOnlineFlag())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	int hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	//地圖數據 原因同上
	int floor = mem::read<int>(hProcess, hModule + kOffestNowFloor);
	QString map = mem::readString(hProcess, hModule + kOffestNowFloorName, FLOOR_NAME_LEN, true);
	if (nowFloor != floor)
		emit signalDispatcher.updateNpcList(floor);

	nowFloor = floor;

	PC pc = getPC();
	pc.dir = mem::read<int>(hProcess, hModule + kOffestDir) + 5 % 8;

	emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(nowFloorName).arg(nowFloor));

	//人物坐標 (因為伺服器端不會經常刷新這個數值大多是在遊戲客戶端修改的)
	QPoint point(mem::read<int>(hProcess, hModule + kOffestNowX), mem::read<int>(hProcess, hModule + kOffestNowY));
	nowPoint = point;
	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2").arg(point.x()).arg(point.y()));

	updateItemByMemory();

	//每隻寵物如果處於等待或戰鬥則為1
	mem::read(hProcess, hModule + kOffestSelectPetArray, sizeof(pc.selectPetNo), pc.selectPetNo);

	//郵件寵物索引
	int mailPetIndex = mem::read<int>(hProcess, hModule + kOffestMailPetIndex);
	if (mailPetIndex < 0 || mailPetIndex >= MAX_PET)
		mailPetIndex = -1;

	//騎乘寵物索引
	int ridePetIndex = mem::read<int>(hProcess, hModule + kOffestRidePetIndex);
	if (ridePetIndex < 0 || ridePetIndex >= MAX_PET)
		ridePetIndex = -1;

	pc.ridePetNo = ridePetIndex;
	//short battlePetIndex = pc.battlePetNo;

	//人物狀態 (是否組隊或其他..)
	pc.status = mem::read<short>(hProcess, hModule + kOffestPlayerStatus);

	short isInTeam = mem::read<short>(hProcess, hModule + kOffestTeamState);
	if (isInTeam == 1 && !(pc.status & CHR_STATUS_PARTY))
		pc.status |= CHR_STATUS_PARTY;
	else if (isInTeam == 0 && (pc.status & CHR_STATUS_PARTY))
		pc.status &= (~CHR_STATUS_PARTY);

	setPC(pc);

	for (int i = 0; i < MAX_PET; ++i)
	{
		//休息 郵件 騎乘
		if (pc.selectPetNo[i] == FALSE)
		{
			if (pet[i].state == kStandby || pet[i].state == kBattle || pet[i].state == kNoneState)
				pet[i].state = kRest;

			if (i == mailPetIndex && pc.ridePetNo != i)
			{
				if (pet[i].state == kRest)
				{
					pet[i].state = kMail;
					pc.mailPetNo = i;
				}
			}
			else if (mailPetIndex == -1 && pet[i].state == kMail)
			{
				pet[i].state = kRest;
				pc.mailPetNo = -1;
			}
			else if (pc.ridePetNo == i)
			{
				pet[i].state = kRide;
			}
			else if (pc.ridePetNo == -1 && pet[i].state == kRide)
			{
				pet[i].state = kRest;
			}
		}
		//戰鬥 等待
		else
		{
			if (pc.ridePetNo == i)
			{
				pet[i].state = kRide;
			}
			else if (pc.battlePetNo != i && pet[i].state != kStandby)
			{
				pet[i].state = kStandby;
			}
			else if (pc.ridePetNo == -1 && pet[i].state == kRide)
			{
				pet[i].state = kRest;
			}
		}

		emit signalDispatcher.updatePlayerInfoPetState(i, pet[i].state);
	}
}

void Server::reloadHashVar(const QString& typeStr)
{
	QString newTypeStr = typeStr.simplified().toLower();
	if (newTypeStr == "map")
	{
		QPoint point = getPoint();
		const util::SafeHash<QString, QVariant> _hashmap = {
			{ "floor", nowFloor },
			{ "name", nowFloorName },
			{ "x", point.x() },
			{ "y", point.y() },
			{ "time", SaTimeZoneNo }
		};
		hashmap = _hashmap;
	}
	else if (newTypeStr == "battle")
	{
		util::SafeHash<int, QHash<QString, QVariant>> _hashbattle;
		battledata_t bt = getBattleData();
		QVector<battleobject_t> objects = bt.objects;

		for (const battleobject_t& battle : objects)
		{
			const QHash<QString, QVariant> hash = {
				{ "pos", battle.pos + 1 },
				{ "name", battle.name },
				{ "fname", battle.freename },
				{ "modelid", battle.faceid },
				{ "lv", battle.level },
				{ "hp", battle.hp },
				{ "maxhp", battle.maxHp },
				{ "hpp", battle.hpPercent },
				{ "status", getBadStatusString(battle.status) },
				{ "rideflag", battle.rideFlag },
				{ "ridename", battle.rideName },
				{ "ridelv", battle.rideLevel },
				{ "ridehp", battle.rideHp },
				{ "ridemaxhp", battle.rideMaxHp },
				{ "ridehpp", battle.rideHpPercent },
			};
			_hashbattle.insert(battle.pos + 1, hash);
		}

		hashbattle = _hashbattle;

		hashbattlefield = getFieldString(bt.fieldAttr);
	}
}

void Server::swapItemLocal(int from, int to)
{
	if (from < 0 || to < 0)
		return;
	QMutexLocker lock(&pcMutex_);
	std::swap(pc_.item[from], pc_.item[to]);
}

void Server::setWorldStatus(int w)
{
#ifdef _DEBUG
	announce(QString("[async battle] 將 W值從 %1 改為 %2").arg(getWorldStatus()).arg(w));
#endif
	QWriteLocker lock(&worldStateLocker);
	Injector& injector = Injector::getInstance();
	mem::write<int>(injector.getProcess(), injector.getProcessModule() + kOffestWorldStatus, w);
}

void Server::setGameStatus(int g)
{
#ifdef _DEBUG
	announce(QString("[async battle] 將 G值從 %1 改為 %2").arg(getGameStatus()).arg(g));
#endif
	QWriteLocker lock(&gameStateLocker);
	Injector& injector = Injector::getInstance();
	mem::write<int>(injector.getProcess(), injector.getProcessModule() + kOffestGameStatus, g);
}

//切換是否在戰鬥中的標誌
void Server::setBattleFlag(bool enable)
{
	QWriteLocker lock(&battleStateLocker);

	IS_BATTLE_FLAG.store(enable, std::memory_order_release);
	isBattleDialogReady.store(false, std::memory_order_release);

	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	DWORD hModule = injector.getProcessModule();

	//這裡關乎頭上是否會出現V.S.圖標
	int status = mem::read<short>(hProcess, hModule + kOffestPlayerStatus);
	if (enable)
	{
		if (!checkAND(status, CHR_STATUS_BATTLE))
			status |= CHR_STATUS_BATTLE;
	}
	else
	{
		if (checkAND(status, CHR_STATUS_BATTLE))
			status &= ~CHR_STATUS_BATTLE;
	}

	mem::write<int>(hProcess, hModule + kOffestPlayerStatus, status);
	mem::write<int>(hProcess, hModule + kOffestBattleStatus, enable ? 1 : 0);

	mem::write<int>(hProcess, hModule + 0x4169B6C, 0);

	mem::write<int>(hProcess, hModule + 0x4181198, 0);
	mem::write<int>(hProcess, hModule + 0x41829FC, 0);

	mem::write<int>(hProcess, hModule + 0x415EF9C, 0);
	mem::write<int>(hProcess, hModule + 0x4181D44, 0);
	mem::write<int>(hProcess, hModule + 0x41829F8, 0);

	mem::write<int>(hProcess, hModule + 0x415F4EC, 30);
}

void Server::setWindowTitle()
{
	Injector& injector = Injector::getInstance();
	int subServer = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestSubServerIndex);//injector.getValueHash(util::kServerValue);
	//int subserver = 0;//injector.getValueHash(util::kSubServerValue);
	int position = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestPositionIndex);//injector.getValueHash(util::kPositionValue);

	QString subServerName;
	int size = injector.subServerNameList.get().size();
	if (subServer >= 0 && subServer < size)
		subServerName = injector.subServerNameList.get().at(subServer);
	else
		subServerName = "0";

	QString positionName;
	if (position >= 0 && position < 1)
		positionName = position == 0 ? QObject::tr("left") : QObject::tr("right");
	else
		positionName = QString::number(position);

	PC pc = getPC();
	QString title = QString("SaSH [%1:%2] - %3 Lv:%4 HP:%5/%6 MP:%7/%8 $:%9") \
		.arg(subServerName).arg(positionName).arg(pc.name).arg(pc.level).arg(pc.hp).arg(pc.maxHp).arg(pc.mp).arg(pc.maxMp).arg(pc.gold);
	std::wstring wtitle = title.toStdWString();
	SetWindowTextW(injector.getProcessWindow(), wtitle.c_str());
}

void Server::setPoint(const QPoint& pos)
{
	Injector& injector = Injector::getInstance();
	int hModule = injector.getProcessModule();
	if (hModule == 0)
		return;

	HANDLE hProcess = injector.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return;

	QWriteLocker locker(&pointMutex_);
	mem::write<int>(hProcess, hModule + kOffestNowX, pos.x());
	mem::write<int>(hProcess, hModule + kOffestNowY, pos.y());
}

//清屏 (實際上就是 char數組置0)
void Server::cleanChatHistory()
{
	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kCleanChatHistory, NULL, NULL);
	chatQueue.clear();
	if (!injector.chatLogModel.isNull())
		injector.chatLogModel->clear();
}
#pragma endregion

#pragma region System
//公告
void Server::announce(const QString& msg, int color)
{
	Injector& injector = Injector::getInstance();

	if (msg.contains("[async battle]"))
	{
		if (!injector.getEnableHash(util::kScriptDebugModeEnable))
			return;
		SPD_LOG(protoBattleLogName, msg);
		return;
	}

	if (!getOnlineFlag())
		return;

	HANDLE hProcess = injector.getProcess();
	if (!msg.isEmpty())
	{
		std::string str = util::fromUnicode(msg);
		util::VirtualMemory ptr(hProcess, str.size(), true);
		mem::write(hProcess, ptr, const_cast<char*>(str.c_str()), str.size());
		injector.sendMessage(Injector::kSendAnnounce, ptr, color);
	}
	else
	{
		util::VirtualMemory ptr(hProcess, "", util::VirtualMemory::kAnsi, true);
		injector.sendMessage(Injector::kSendAnnounce, ptr, color);
	}
	chatQueue.enqueue(QPair<int, QString>(color, msg));
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.appendChatLog(msg, color);
}

//喊話
void Server::talk(const QString& text, int color, TalkMode mode)
{
	if (!getOnlineFlag())
		return;

	if (text.startsWith("//skup"))
	{
		lssproto_CustomTK_recv(text);
		return;
	}

	if (color < 0 || color > 10)
		color = 0;
	bool bRecover = false;
	PC pc = getPC();
	int flg = pc.etcFlag;
	QString msg;
	if (mode == kTalkGlobal)
		msg = ("P|/XJ ");
	else if (mode == kTalkFamily)
		msg = ("P|/FM ");
	else if (mode == kTalkWorld)
		msg = ("P|/WD ");
	else if (mode == kTalkTeam)
	{
		int newflg = flg;
		if (!(newflg & PC_ETCFLAG_PARTY_CHAT))
		{
			newflg |= PC_ETCFLAG_PARTY_CHAT;
			setSwitcher(newflg);
			bRecover = true;
		}
		msg = ("P|");

	}
	else
		msg = ("P|");
	msg += text;
	std::string str = util::fromUnicode(msg);
	lssproto_TK_send(getPoint(), const_cast<char*>(str.c_str()), color, 3);
	if (bRecover)
		setSwitcher(flg);
}

void Server::createCharacter(int dataplacenum
	, const QString& charname
	, int imgno
	, int faceimgno
	, int vit
	, int str
	, int tgh
	, int dex
	, int earth
	, int water
	, int fire
	, int wind
	, int hometown)
{
	if (dataplacenum != 0 && dataplacenum != 1)
		return;

	if (chartable[dataplacenum].valid)
		return;

	//hometown: 薩姆0 漁村1 加加2 卡魯3
	if (!checkWG(3, 11))
		return;

	std::string sname = util::fromUnicode(charname);
	lssproto_CreateNewChar_send(dataplacenum, const_cast<char*>(sname.c_str()), imgno, faceimgno, vit, str, tgh, dex, earth, water, fire, wind, hometown);

	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();
	mem::write<int>(hProcess, hModule + 0x421C000, 1);
	int time = timeGetTime();
	mem::write<int>(hProcess, hModule + 0x421C004, time);
	mem::write<int>(hProcess, hModule + 0x4152B44, 2);

	setWorldStatus(2);
	setGameStatus(2);
}

void Server::deleteCharacter(int index, const QString password, bool backtofirst)
{
	if (index < 0 || index > MAXCHARACTER)
		return;

	if (!checkWG(3, 11))
		return;

	CHARLISTTABLE table = chartable[index];

	if (!table.valid)
		return;

	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();

	mem::write<int>(hProcess, hModule + 0x4230A88, index);
	mem::writeString(hProcess, hModule + 0x421BF74, table.name);

	std::string sname = util::fromUnicode(table.name);
	std::string spassword = util::fromUnicode(password);
	lssproto_CharDelete_send(const_cast<char*>(sname.c_str()), const_cast<char*>(spassword.c_str()));

	mem::write<int>(hProcess, hModule + 0x421C000, 1);
	mem::write<int>(hProcess, hModule + 0x415EF6C, 2);

	int time = timeGetTime();
	mem::write<int>(hProcess, hModule + 0x421C004, time);

	setGameStatus(21);

	if (backtofirst)
	{
		setWorldStatus(1);
		setGameStatus(2);
	}
}

//老菜單
void Server::shopOk(int n)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	//SE 1隨身倉庫 2查看聲望氣勢
	lssproto_ShopOk_send(n);
}

//新菜單
void Server::saMenu(int n)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_SaMenu_send(n);
}

//切換單一開關
void Server::setSwitcher(int flg, bool enable)
{
	PC pc = getPC();
	if (enable)
		pc.etcFlag |= flg;
	else
		pc.etcFlag &= ~flg;
	setPC(pc);
	setSwitcher(pc.etcFlag);
}

//切換全部開關
void Server::setSwitcher(int flg)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_FS_send(flg);
}

bool Server::isDialogVisible()
{
	if (!getOnlineFlag())
		return false;

	if (getBattleFlag())
		return false;

	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();

	bool bret = mem::read<int>(hProcess, hModule + 0xB83EC) != -1;
	return bret;
}
#pragma endregion

#pragma region Connection
//元神歸位
void Server::EO()
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_EO_send(0);
	lastEOTime.store(-1, std::memory_order_release);
	isEOTTLSend.store(true, std::memory_order_release);
	eottlTimer.restart();
	lssproto_Echo_send(const_cast<char*>("hoge"));

	//石器私服SE SO專用
	Injector& injector = Injector::getInstance();
	QString cmd = injector.getStringHash(util::kEOCommandString);
	if (!cmd.isEmpty())
		talk(cmd);

	//setBattleEnd();
}
//登出
void Server::logOut()
{
	if (!getOnlineFlag())
		return;

	lssproto_CharLogout_send(0);
	setWorldStatus(7);
	setGameStatus(0);
}

//回點
void Server::logBack()
{
	if (!getOnlineFlag())
		return;

	lssproto_CharLogout_send(1);
}

//登入
bool Server::login(int s)
{
	util::UnLoginStatus status = static_cast<util::UnLoginStatus>(s);
	Injector& injector = Injector::getInstance();

	int server = injector.getValueHash(util::kServerValue);
	int subserver = injector.getValueHash(util::kSubServerValue);
	int position = injector.getValueHash(util::kPositionValue);
	QString account = injector.getStringHash(util::kGameAccountString);
	QString password = injector.getStringHash(util::kGamePasswordString);

	bool enableReconnect = injector.getEnableHash(util::kAutoReconnectEnable);
	bool enableAutoLogin = injector.getEnableHash(util::kAutoLoginEnable);

	if (status == util::kStatusDisconnect)
	{
		if (enableReconnect)
		{
			injector.leftDoubleClick(315, 270);
			disconnectflag = true;
		}
		return false;
	}

	if (!enableAutoLogin && enableReconnect && !disconnectflag)
		return false;

	QElapsedTimer timer; timer.start();

	const QString fileName(qgetenv("JSON_PATH"));
	util::Config config(fileName);
	switch (status)
	{
	case util::kStatusBusy:
	{
		QList<int> list = config.readArray<int>("System", "Login", "Busy");
		if (list.size() == 2)
			injector.leftDoubleClick(list.at(0), list.at(1));
		else
		{
			injector.leftDoubleClick(315, 255);
			config.writeArray<int>("System", "Login", "Busy", { 315, 255 });
		}
		break;
	}
	case util::kStatusTimeout:
	{
		QList<int> list = config.readArray<int>("System", "Login", "Timeout");
		if (list.size() == 2)
			injector.leftDoubleClick(list.at(0), list.at(1));
		else
		{
			injector.leftDoubleClick(315, 253);
			config.writeArray<int>("System", "Login", "Timeout", { 315, 253 });
		}
		break;
	}
	case util::kNoUserNameOrPassword:
	{
		QList<int> list = config.readArray<int>("System", "Login", "NoUserNameOrPassword");
		if (list.size() == 2)
			injector.leftDoubleClick(list.at(0), list.at(1));
		else
		{
			injector.leftDoubleClick(315, 253);
			config.writeArray<int>("System", "Login", "NoUserNameOrPassword", { 315, 253 });
		}
		break;
	}
	case util::kStatusInputUser:
	{
		Injector& injector = Injector::getInstance();
		if (!account.isEmpty())
			mem::writeString(injector.getProcess(), injector.getProcessModule() + kOffestAccount, account);

		if (!password.isEmpty())
			mem::writeString(injector.getProcess(), injector.getProcessModule() + kOffestPassword, password);

		QList<int> list = config.readArray<int>("System", "Login", "OK");
		if (list.size() == 2)
			injector.leftDoubleClick(list.at(0), list.at(1));
		else
		{
			injector.leftDoubleClick(380, 310);
			config.writeArray<int>("System", "Login", "OK", { 380, 310 });
		}
		break;
	}
	case util::kStatusSelectServer:
	{
		constexpr int table[48] = {
			0, 0, 0,
			1, 0, 1,
			2, 0, 2,
			3, 0, 3,

			4, 1, 0,
			5, 1, 1,
			6, 1, 2,
			7, 1, 3,

			8,  2, 0,
			9,  2, 1,
			10, 2, 2,
			11, 2, 3,

			12, 3, 0,
			13, 3, 1,
			14, 3, 2,
			15, 3, 3,
		};

		if (server >= 0 && server < 15)
		{
			const int a = table[server * 3 + 1];
			const int b = table[server * 3 + 2];

			int x = 160 + (a * 125);
			int y = 165 + (b * 25);

			QList<int> list = config.readArray<int>("System", "Login", "SelectServer");
			if (list.size() == 4)
			{
				x = list.at(0) + (a * list.at(1));
				y = list.at(2) + (b * list.at(3));
			}
			else
			{
				config.writeArray<int>("System", "Login", "SelectServer", { 160, 125, 165, 25 });
			}

			for (;;)
			{
				injector.mouseMove(x, y);
				int value = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestMousePointedIndex);
				if (value != -1)
				{
					injector.leftDoubleClick(x, y);
					break;
				}
				x -= 5;
				if (x <= 0)
					break;

				if (timer.hasExpired(1000))
					break;

			}
		}
		break;
	}
	case util::kStatusSelectSubServer:
	{
		int serverIndex = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestServerIndex);
		if (server != serverIndex)
		{
			int x = 500;
			int y = 340;

			QList<int> list = config.readArray<int>("System", "Login", "SelectSubServerGoBack");
			if (list.size() == 2)
			{
				x = list.at(0);
				y = list.at(1);
			}
			else
			{
				config.writeArray<int>("System", "Login", "SelectSubServer", QList<int>{ 500, 340 });
			}

			for (;;)
			{
				injector.mouseMove(x, y);
				int value = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestMousePointedIndex);
				if (value != -1)
				{
					injector.leftDoubleClick(x, y);
					break;
				}

				x -= 10;
				if (x <= 0)
					break;

				if (timer.hasExpired(1000))
					break;

			}
			break;
		}

		if (subserver >= 0 && subserver < 15)
		{
			int x = 250;
			int y = 265 + (subserver * 20);

			QList<int> list = config.readArray<int>("System", "Login", "SelectSubServer");
			if (list.size() == 3)
			{
				x = list.at(0);
				y = list.at(1) + (subserver * list.at(2));
			}
			else
			{
				config.writeArray<int>("System", "Login", "SelectSubServer", { 250, 265, 20 });
			}

			for (;;)
			{
				injector.mouseMove(x, y);
				int value = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestMousePointedIndex);
				if (value != -1)
				{
					injector.leftDoubleClick(x, y);
					break;
				}

				x += 5;
				if (x >= 640)
					break;

				if (timer.hasExpired(1000))
					break;

			}
			connectingTimer.restart();
		}
		break;
	}
	case util::kStatusSelectCharacter:
	{
		if (position >= 0 && position <= 1)
		{

			int x = 100 + (position * 300);
			int y = 340;

			QList<int> list = config.readArray<int>("System", "Login", "SelectCharacter");
			if (list.size() == 3)
			{
				x = list.at(0) + (position * list.at(1));
				y = list.at(2);
			}
			else
			{
				config.writeArray<int>("System", "Login", "SelectCharacter", { 100, 300, 340 });
			}

			if (!chartable[position].valid)
				break;

			injector.leftDoubleClick(x, y);
		}
		break;
	}
	case util::kStatusLogined:
	{
		disconnectflag = false;
		return true;
	}
	case util::kStatusConnecting:
	{
		if (connectingTimer.hasExpired(5000))
		{
			setWorldStatus(7);
			setGameStatus(0);
			connectingTimer.restart();
		}
		break;
	}
	default:
		break;
	}
	disconnectflag = false;
	return false;
}

#pragma endregion

#pragma region WindowPacket
//創建對話框
void Server::createRemoteDialog(int button, const QString& text)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	Injector& injector = Injector::getInstance();

	util::VirtualMemory ptr(injector.getProcess(), text, util::VirtualMemory::kAnsi, true);

	injector.sendMessage(Injector::kCreateDialog, button, ptr);
}

void Server::press(BUTTON_TYPE select, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	char data[2] = { '\0', '\0' };
	if (BUTTON_BUY == select)
	{
		data[0] = { '1' };
		select = BUTTON_NOTUSED;
	}
	else if (BUTTON_SELL == select)
	{
		data[0] = { '2' };
		select = BUTTON_NOTUSED;
	}
	else if (BUTTON_OUT == select)
	{
		data[0] = { '3' };
		select = BUTTON_NOTUSED;
	}
	else if (BUTTON_BACK == select)
	{
		select = BUTTON_NOTUSED;
	}
	else if (BUTTON_AUTO == select)
	{
		if (dialog.buttontype & BUTTON_OK)
			select = BUTTON_OK;
		else if (dialog.buttontype & BUTTON_YES)
			select = BUTTON_YES;
		else if (dialog.buttontype & BUTTON_NEXT)
			select = BUTTON_NEXT;
		else if (dialog.buttontype & BUTTON_PREVIOUS)
			select = BUTTON_PREVIOUS;
		else if (dialog.buttontype & BUTTON_NO)
			select = BUTTON_NO;
		else if (dialog.buttontype & BUTTON_CANCEL)
			select = BUTTON_CANCEL;
	}

	lssproto_WN_send(getPoint(), seqno, objindex, select, const_cast<char*>(data));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

void Server::press(int row, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;
	QString qrow = QString::number(row);
	std::string srow = qrow.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//買東西
void Server::buy(int index, int amt, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (index < 0)
		return;

	if (amt <= 0)
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QString qrow = QString("%1\\z%2").arg(index + 1).arg(amt);
	std::string srow = qrow.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//賣東西
void Server::sell(const QString& name, const QString& memo, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (name.isEmpty())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QVector<int> indexs;
	if (!getItemIndexsByName(name, memo, &indexs, CHAR_EQUIPPLACENUM))
		return;

	sell(indexs, seqno, objindex);
}

//賣東西
void Server::sell(int index, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (index < 0 || index >= MAX_ITEM)
		return;

	PC pc = getPC();
	if (pc.item[index].name.isEmpty() || pc.item[index].useFlag == 0)
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QString qrow = QString("%1\\z%2").arg(index).arg(pc.item[index].pile);
	std::string srow = qrow.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//賣東西
void Server::sell(const QVector<int>& indexs, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (indexs.isEmpty())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QStringList list;
	for (const int it : indexs)
	{
		if (it < 0 || it >= MAX_ITEM)
			continue;

		sell(it, seqno, objindex);
	}
}

//寵物學技能
void Server::learn(int skillIndex, int petIndex, int spot, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (spot < 0 || spot >= 7)
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;
	//8\z3\z3\z1000 技能
	QString qrow = QString("%1\\z%3\\z%3\\z%4").arg(skillIndex + 1).arg(petIndex + 1).arg(spot + 1).arg(0);
	std::string srow = qrow.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

void Server::depositItem(int itemIndex, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QString qstr = QString::number(itemIndex + 1);
	std::string srow = qstr.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::withdrawItem(int itemIndex, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QString qstr = QString::number(itemIndex + 1);
	std::string srow = qstr.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::depositPet(int petIndex, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QString qstr = QString::number(petIndex + 1);
	std::string srow = qstr.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::withdrawPet(int petIndex, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;

	if (objindex == -1)
		objindex = dialog.objindex;

	QString qstr = QString::number(petIndex + 1);
	std::string srow = qstr.toStdString();
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

//遊戲對話框輸入文字送出
void Server::inputtext(const QString& text, int seqno, int objindex)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (seqno == -1)
		seqno = dialog.seqno;
	if (objindex == -1)
		objindex = dialog.objindex;
	std::string s = util::fromUnicode(text);
	//if (dialog.buttontype & BUTTON_YES)
	//	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_YES, const_cast<char*>(s.c_str()));
	//else if (dialog.buttontype & BUTTON_OK)
	//	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_OK, const_cast<char*>(s.c_str()));
	//else
	lssproto_WN_send(getPoint(), seqno, objindex, BUTTON_OK, const_cast<char*>(s.c_str()));
	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//解除安全瑪
void Server::unlockSecurityCode(const QString& code)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (code.isEmpty())
		return;

	//6-15个字符（必须大小写字母加数字)
	//static const QRegularExpression regex("[A-Za-z\\d]{6,25}");

	//if (!regex.match(code).hasMatch())
	//{
	//	return;
	//}

	std::string scode = code.toStdString();
	lssproto_WN_send(getPoint(), kDialogSecurityCode, -1, NULL, const_cast<char*>(scode.c_str()));
}

void Server::windowPacket(const QString& command, int seqno, int objindex)
{
	//SI|itemIndex(0-15)|Stack(-1)
	//TI|itemIndex(0-59)|Stack(-1)
	//SP|petIndex(0-4)|
	//TP|petIndex(0-?)|
	//SG|gold|
	//TG|gold|
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	std::string s = util::fromUnicode(command);
	lssproto_WN_send(getPoint(), seqno, objindex, NULL, const_cast<char*>(s.c_str()));
}
#pragma endregion

#pragma region CHAR
//使用精靈
void Server::useMagic(int magicIndex, int target)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (magicIndex < 0 || magicIndex >= MAX_ITEM)
		return;

	if (target < 0 || target >= (MAX_PET + MAX_PARTY))
		return;

	if (magicIndex < MAX_MAGIC && !isPlayerMpEnoughForMagic(magicIndex))
		return;
	else if (getPC().item[magicIndex].useFlag == 0)
		return;

	lssproto_MU_send(getPoint(), magicIndex, target);
}

//組隊或離隊 true 組隊 false 離隊
void Server::setTeamState(bool join)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_PR_send(getPoint(), join ? 1 : 0);
}

void Server::kickteam(int n)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (n < 0 || n >= MAX_PARTY)
		return;

	if (n == 0)
	{
		setTeamState(false);
		return;
	}

	if (party[n].useFlag == 1)
		lssproto_KTEAM_send(n);
}

void Server::mail(const QVariant& card, const QString& text, int petIndex, const QString& itemName, const QString& itemMemo)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	int index = -1;
	if (card.type() == QVariant::Type::Int || card.type() == QVariant::Type::LongLong)
		index = card.toInt();
	else if (card.type() == QVariant::Type::String)
	{
		bool isExact = false;
		QString name = card.toString();
		if (name.startsWith("?"))
		{
			name = name.mid(1);
			isExact = true;
		}

		for (int i = 0; i < MAX_ADR_BOOK; i++)
		{
			if (addressBook[i].useFlag == 0)
				continue;

			if (isExact)
			{
				if (name == addressBook[i].name)
				{
					index = i;
					break;
				}
			}
			else
			{
				if (addressBook[i].name.contains(name))
				{
					index = i;
					break;
				}
			}
		}
	}

	if (index < 0 || index > MAX_ADR_BOOK)
		return;

	if (addressBook[index].useFlag == 0)
		return;

	std::string sstr = util::fromUnicode(text);
	if (itemName.isEmpty() && itemMemo.isEmpty() && (petIndex < 0 || petIndex > MAX_PET))
	{
		lssproto_MSG_send(index, const_cast<char*>(sstr.c_str()), NULL);
	}
	else
	{
		if (addressBook[index].onlineFlag == 0)
			return;

		int itemIndex = getItemIndexByName(itemName, true, itemMemo, CHAR_EQUIPPLACENUM);
		if (itemIndex < 0 || itemIndex >= MAX_ITEM)
			return;

		PET pet = getPet(petIndex);
		if (pet.useFlag == 0)
			return;

		if (pet.state != kMail)
			setPetState(petIndex, kMail);

		lssproto_PMSG_send(index, petIndex, itemIndex, const_cast<char*>(sstr.c_str()), NULL);
	}
}

//加點
bool Server::addPoint(int skillid, int amt)
{
	if (!getOnlineFlag())
		return false;

	if (getBattleFlag())
		return false;

	if (skillid < 0 || skillid > 4)
		return false;

	if (amt < 1)
		return false;

	if (amt > StatusUpPoint)
		amt = StatusUpPoint;
	QElapsedTimer timer; timer.start();
	for (int i = 0; i < amt; ++i)
	{
		IS_WAITFOT_SKUP_RECV = true;
		lssproto_SKUP_send(skillid);
		for (;;)
		{
			if (timer.hasExpired(1000))
				break;

			if (getBattleFlag())
				return false;

			if (!getOnlineFlag())
				return false;

			if (!IS_WAITFOT_SKUP_RECV)
				break;
		}
		timer.restart();
	}
	return true;
}

//人物改名
void Server::setPlayerFreeName(const QString& name)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	std::string sname = util::fromUnicode(name);
	lssproto_FT_send(const_cast<char*> (sname.c_str()));
}

//寵物改名
void Server::setPetFreeName(int petIndex, const QString& name)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	std::string sname = util::fromUnicode(name);
	lssproto_KN_send(petIndex, const_cast<char*> (sname.c_str()));
}
#pragma endregion

#pragma region PET
//設置寵物狀態 (戰鬥 | 等待 | 休息 | 郵件 | 騎乘)
void Server::setPetState(int petIndex, PetState state)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (getBattleFlag())
		return;

	if (!getOnlineFlag())
		return;

	Injector& injector = Injector::getInstance();
	QMutexLocker locker(&net_mutex);

	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	PC pc = getPC();
	switch (state)
	{
	case kBattle:
	{
		//if (pc.ridePetNo == petIndex)
		//	setRidePet(-1);

		setFightPet(petIndex);

		//if (pc.ridePetNo == petIndex)
		//{
		//	pc.ridePetNo = -1;
		//	mem::write<int>(hProcess, hModule + kOffestRidePetIndex, -1);
		//	emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		//}

		//if (pc.mailPetNo == petIndex)
		//{
		//	pc.mailPetNo = -1;
		//	mem::write<short>(hProcess, hModule + kOffestMailPetIndex, -1);
		//}

		//pet[petIndex].state = kBattle;
		//pc.battleNo = petIndex;
		//pc.selectPetNo[petIndex] = TRUE;
		//mem::write<short>(hProcess, hModule + kOffestSelectPetArray + (petIndex * sizeof(short)), TRUE);
		//PET _pet = pet[petIndex];
		//emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		break;
	}
	case kStandby:
	{
		if (pc.ridePetNo == petIndex)
			setRidePet(-1);

		if (pet[petIndex].state == kMail)
			setPetState(petIndex, 0);

		setPetState(petIndex, 1);

		if (pc.ridePetNo == petIndex)
		{
			pc.ridePetNo = -1;
			mem::write<int>(hProcess, hModule + kOffestRidePetIndex, -1);
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		}

		if (pc.mailPetNo == petIndex)
		{
			pc.mailPetNo = -1;
			mem::write<short>(hProcess, hModule + kOffestMailPetIndex, -1);
		}

		if (pc.battlePetNo == petIndex)
		{
			pc.battlePetNo = -1;
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		}

		pet[petIndex].state = kStandby;
		pc.selectPetNo[petIndex] = TRUE;
		mem::write<short>(hProcess, hModule + kOffestSelectPetArray + (petIndex * sizeof(short)), TRUE);
		break;
	}
	case kMail:
	{
		if (pc.ridePetNo == petIndex)
			setRidePet(-1);

		setPetState(petIndex, 4);

		if (pc.ridePetNo == petIndex)
		{
			pc.ridePetNo = -1;
			mem::write<int>(hProcess, hModule + kOffestRidePetIndex, -1);
		}

		if (pc.battlePetNo == petIndex)
			pc.battlePetNo = -1;

		pet[petIndex].state = kMail;
		pc.mailPetNo = petIndex;
		pc.selectPetNo[petIndex] = FALSE;
		mem::write<short>(hProcess, hModule + kOffestMailPetIndex, petIndex);
		mem::write<short>(hProcess, hModule + kOffestSelectPetArray + (petIndex * sizeof(short)), FALSE);
		break;
	}
	case kRest:
	{
		if (pc.ridePetNo == petIndex)
			setRidePet(-1);

		setPetState(petIndex, 0);

		if (pc.ridePetNo == petIndex)
		{
			pc.ridePetNo = -1;
			mem::write<int>(hProcess, hModule + kOffestRidePetIndex, -1);
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		}

		if (pc.mailPetNo == petIndex)
		{
			pc.mailPetNo = -1;
			mem::write<short>(hProcess, hModule + kOffestMailPetIndex, -1);
		}

		if (pc.battlePetNo == petIndex)
		{
			pc.battlePetNo = -1;
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		}

		pet[petIndex].state = kRest;
		pc.selectPetNo[petIndex] = FALSE;
		mem::write<short>(hProcess, hModule + kOffestSelectPetArray + (petIndex * sizeof(short)), FALSE);

		break;
	}
	case kRide:
	{
		if (pet[petIndex].state == kRide || pc.ridePetNo == petIndex)
		{
			break;
		}

		//if (pc.battlePetNo == petIndex)
		//	setFightPet(-1);

		if (pc.ridePetNo != petIndex)
			setRidePet(petIndex);

		setRidePet(petIndex);

		//if (pc.mailPetNo == petIndex)
		//{
		//	pc.mailPetNo = -1;
		//	mem::write<short>(hProcess, hModule + kOffestMailPetIndex, -1);
		//}

		//if (pc.battlePetNo == petIndex)
		//{
		//	pc.battlePetNo = -1;
		//	emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		//}

		//pet[petIndex].state = kRide;
		//pc.ridePetNo = petIndex;
		//pc.selectPetNo[petIndex] = FALSE;
		//mem::write<int>(hProcess, hModule + kOffestRidePetIndex, petIndex);
		//mem::write<short>(hProcess, hModule + kOffestSelectPetArray + (petIndex * sizeof(short)), FALSE);
		//PET _pet = pet[petIndex];
		//emit signalDispatcher.updateRideHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		break;
	}
	default:
		break;
	}
	//setPC(pc);
}

void Server::setAllPetState()
{
	for (int i = 0; i < MAX_PET; i++)
	{
		int state = 0;
		switch (pet[i].state)
		{
		case kBattle:
			state = 1;
			break;
		case kStandby:
			state = 1;
			break;
		case kMail:
			state = 4;
			break;
		case kRest:
			break;
		case kRide:
			setRidePet(i);
			return;
		}

		setPetState(i, pet[i].state);
	}
}

//設置戰鬥寵物
void Server::setFightPet(int petIndex)
{
	lssproto_KS_send(petIndex);
}

//設置騎乘寵物
void Server::setRidePet(int petIndex)
{
	QString str = QString("R|P|%1").arg(petIndex);
	std::string sstr = str.toStdString();
	lssproto_FM_send(const_cast<char*>(sstr.c_str()));
}

//設置寵物等待狀態
void Server::setStandbyPet(int standbypets)
{
	lssproto_SPET_send(standbypets);
}

//設置寵物狀態封包  0:休息 1:戰鬥或等待 4:郵件
void Server::setPetState(int petIndex, int state)
{
	PC pc = getPC();
	if (petIndex == pc.battlePetNo)
		lssproto_KS_send(-1);

	lssproto_PETST_send(petIndex, state);

	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();
	if (state == 0)
	{
		if (pet[petIndex].state == kMail)
			mem::write<short>(hProcess, hModule + kOffestMailPetIndex, -1);
		pet[petIndex].state = kRest;
	}

	else if (state == 4)
	{
		pet[petIndex].state = kMail;
		mem::write<short>(hProcess, hModule + kOffestMailPetIndex, petIndex);
	}

	int standby = 0;
	for (int i = 0; i < MAX_PET; i++)
	{
		if (pet[i].state == kStandby)
			standby += 1 << i;
	}
	lssproto_SPET_send(standby);
}

//丟棄寵物
void Server::dropPet(int petIndex)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_DP_send(getPoint(), petIndex);
}
#pragma endregion

#pragma region MAP
//下載指定坐標 24 * 24 大小的地圖塊
void Server::downloadMap(int x, int y)
{
	lssproto_M_send(nowFloor, x, y, x + 24, y + 24);
}

//下載全部地圖塊
void Server::downloadMap()
{
	if (!getOnlineFlag())
		return;

	bool IsDownloadingMap = true;

	int floor = nowFloor;

	map_t map;
	if (!mapAnalyzer->readFromBinary(nowFloor, nowFloorName))
		return;

	if (!mapAnalyzer->getMapDataByFloor(floor, &map))
		return;

	int downloadMapXSize = map.width;
	int downloadMapYSize = map.height;

	if (!downloadMapXSize || !downloadMapYSize) return;

	int downloadMapX = 0;
	int downloadMapY = 0;

	do
	{
		lssproto_M_send(floor, downloadMapX, downloadMapY, downloadMapX + 24, downloadMapY + 24);

		downloadMapX += 24;

		if (downloadMapX > downloadMapXSize)
		{
			downloadMapX = 0;
			downloadMapY += 24;
		}
		if (downloadMapY > downloadMapYSize)
		{
			//IsDownloadingMap = false;
			break;
		}
	} while (IsDownloadingMap);

	//清空尋路地圖數據、數據重讀、圖像重繪
	mapAnalyzer->clear(nowFloor);
	mapAnalyzer->readFromBinary(nowFloor, nowFloorName);
}

//轉移
void Server::warp()
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	Injector& injector = Injector::getInstance();
	struct
	{
		short ev = 0;
		short seqno = 0;
	}ev;

	if (mem::read(injector.getProcess(), injector.getProcessModule() + kOffestEV, sizeof(ev), &ev))
		lssproto_EV_send(ev.ev, ev.seqno, getPoint(), -1);
}

//計算方向
QString calculateDirection(int currentX, int currentY, int targetX, int targetY)
{
	QString table = "abcdefgh";
	QPoint src(currentX, currentY);
	for (const QPoint& it : util::fix_point)
	{
		if (it + src == QPoint(targetX, targetY))
		{
			int index = util::fix_point.indexOf(it);
			return table.mid(index, 1);
		}
	}

	return "";
}

//移動(封包) [a-h]
void Server::move(const QPoint& p, const QString& dir)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (p.x() < 0 || p.x() > 1500 || p.y() < 0 || p.y() > 1500)
		return;

	QWriteLocker locker(&pointMutex_);

	std::string sdir = dir.toStdString();
	lssproto_W2_send(p, const_cast<char*>(sdir.c_str()));
}

//移動(記憶體)
void Server::move(const QPoint& p)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (p.x() < 0 || p.x() > 1500 || p.y() < 0 || p.y() > 1500)
		return;

	QWriteLocker locker(&pointMutex_);

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kSetMove, p.x(), p.y());
}

//轉向指定坐標
void Server::setPlayerFaceToPoint(const QPoint& pos)
{
	int dir = -1;
	QPoint current = getPoint();
	for (const QPoint& it : util::fix_point)
	{
		if (it + current == pos)
		{
			dir = util::fix_point.indexOf(it);
			setPlayerFaceDirection(dir);
			break;
		}
	}
}

//轉向 (根據方向索引自動轉換成A-H)
void Server::setPlayerFaceDirection(int dir)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (dir < 0 || dir >= MAX_DIR)
		return;

	const QString dirchr = "ABCDEFGH";
	QString dirStr = dirchr[dir];
	std::string sdirStr = dirStr.toUpper().toStdString();
	lssproto_W2_send(getPoint(), const_cast<char*>(sdirStr.c_str()));

	//這裡是用來使遊戲動畫跟著轉向
	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();
	int newdir = (dir + 3) % 8;
	int p = mem::read<int>(hProcess, hModule + 0x422E3AC);
	if (p)
	{
		mem::write<int>(hProcess, hModule + 0x422BE94, newdir);
		mem::write<int>(hProcess, p + 0x150, newdir);
	}
}

//轉向 使用方位字符串
void Server::setPlayerFaceDirection(const QString& dirStr)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	static const QHash<QString, QString> dirhash = {
		{ u8"北", "A" }, { u8"東北", "B" }, { u8"東", "C" }, { u8"東南", "D" },
		{ u8"南", "E" }, { u8"西南", "F" }, { u8"西", "G" }, { u8"西北", "H" },
		{ u8"北", "A" }, { u8"东北", "B" }, { u8"东", "C" }, { u8"东南", "D" },
		{ u8"南", "E" }, { u8"西南", "F" }, { u8"西", "G" }, { u8"西北", "H" }
	};

	int dir = -1;
	QString qdirStr;
	const QString dirchr = "ABCDEFGH";
	if (!dirhash.contains(dirStr))
	{
		QRegularExpression re(u8"[A-H]");
		QRegularExpressionMatch match = re.match(dirStr);
		if (!match.hasMatch())
			return;
		qdirStr = match.captured(0);
		dir = dirchr.indexOf(qdirStr, 0, Qt::CaseInsensitive);
	}
	else
	{

		dir = dirchr.indexOf(dirhash.value(dirStr), 0, Qt::CaseInsensitive);
		qdirStr = dirhash.value(dirStr);
	}
	std::string sdirStr = qdirStr.toUpper().toStdString();
	lssproto_W2_send(getPoint(), const_cast<char*>(sdirStr.c_str()));

	//這裡是用來使遊戲動畫跟著轉向
	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	int hModule = injector.getProcessModule();
	int newdir = (dir + 3) % 8;
	int p = mem::read<int>(hProcess, hModule + 0x422E3AC);
	if (p)
	{
		mem::write<int>(hProcess, hModule + 0x422BE94, newdir);
		mem::write<int>(hProcess, p + 0x150, newdir);
	}
}
#pragma endregion

#pragma region ITEM
//物品排序
void Server::sortItem(bool deepSort)
{
	getPlayerMaxCarryingCapacity();
	PC pc = getPC();
	int j = 0;
	int i = 0;

	if (swapitemModeFlag == 0)
	{
		QMutexLocker lock(&swapItemMutex_);
		if (deepSort)
			swapitemModeFlag = 1;

		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (pc.item[i].useFlag == 0)
					continue;

				//if (!isItemStackable(pc.item[i].sendFlag))
				//	continue;

				if (pc.item[i].name.isEmpty())
					continue;

				if (pc.item[i].name != pc.item[j].name)
					continue;

				if (pc.item[i].memo != pc.item[j].memo)
					continue;

				if (pc.item[i].graNo != pc.item[j].graNo)
					continue;

				if (pc.maxload <= 0)
					continue;

				QString key = QString("%1|%2|%3").arg(pc.item[j].name).arg(pc.item[j].memo).arg(pc.item[j].graNo);
				if (pc.item[j].pile > 1 && !itemStackFlagHash.contains(key))
					itemStackFlagHash.insert(key, true);
				else
				{
					swapItem(i, j);
					itemStackFlagHash.insert(key, false);
					continue;
				}

				if (itemStackFlagHash.contains(key) && !itemStackFlagHash.value(key) && pc.item[j].pile == 1)
					continue;

				if (pc.item[j].pile >= pc.maxload)
				{
					pc.maxload = pc.item[j].pile;
					if (pc.item[j].pile != pc.item[j].maxStack)
					{
						pc.item[j].maxStack = pc.item[j].pile;
						swapItem(i, j);
					}
					continue;
				}

				if (pc.item[i].pile > pc.item[j].pile)
					continue;

				pc.item[j].maxStack = pc.item[j].pile;

				swapItem(i, j);
			}
		}
	}
	else if (swapitemModeFlag == 1 && deepSort)
	{
		QMutexLocker lock(&swapItemMutex_);
		swapitemModeFlag = 2;

		//補齊  item[i].useFlag == 0 的空格
		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (pc.item[i].useFlag == 0)
					continue;

				if (pc.item[j].useFlag == 0)
				{
					swapItem(i, j);
				}
			}
		}
	}
	else if (deepSort)
	{
		QMutexLocker lock(&swapItemMutex_);
		swapitemModeFlag = 0;

		static const QLocale locale;
		static const QCollator collator(locale);

		//按 pc.item[i].name 名稱排序
		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (pc.item[i].useFlag == 0)
					continue;

				if (pc.item[j].useFlag == 0)
					continue;

				if (pc.item[i].name.isEmpty())
					continue;

				if (pc.item[j].name.isEmpty())
					continue;

				if (pc.item[i].name != pc.item[j].name)
				{
					if (collator.compare(pc.item[i].name, pc.item[j].name) < 0)
					{
						swapItem(i, j);
					}
				}
				else if (pc.item[i].memo != pc.item[j].memo)
				{
					if (collator.compare(pc.item[i].memo, pc.item[j].memo) < 0)
					{
						swapItem(i, j);
					}
				}
				else if (pc.item[i].graNo != pc.item[j].graNo)
				{
					if (pc.item[i].graNo < pc.item[j].graNo)
					{
						swapItem(i, j);
					}
				}
				//數量少的放前面
				else if (pc.item[i].pile < pc.item[j].pile)
				{
					swapItem(i, j);
				}
			}
		}
	}

	setPC(pc);
	refreshItemInfo();
}

//丟棄道具
void Server::dropItem(int index)
{
	if (index < CHAR_EQUIPPLACENUM || index >= MAX_ITEM)
		return;

	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	PC pc = getPC();
	if (pc.item[index].name.isEmpty() || pc.item[index].useFlag == 0)
		return;

	lssproto_DI_send(getPoint(), index);
}

void Server::dropItem(QVector<int> indexs)
{
	for (const int it : indexs)
		dropItem(it);
}

void Server::dropGold(int gold)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	PC pc = getPC();
	if (gold >= pc.gold)
		gold = pc.gold;

	lssproto_DG_send(getPoint(), gold);
}

//使用道具
void Server::useItem(int itemIndex, int target)
{
	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	if (target < 0 || target > 9)
		return;

	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_ID_send(getPoint(), itemIndex, target);
}

//交換道具
void Server::swapItem(int from, int to)
{
	if (from < 0 || from >= MAX_ITEM)
		return;

	if (to < 0 || to >= MAX_ITEM)
		return;

	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_MI_send(from, to);
}

//撿道具
void Server::pickItem(int dir)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	lssproto_PI_send(getPoint(), (dir + 3) % 8);
}

//穿裝 to = -1 丟裝 to = -2 脫裝 to = itemspotindex
void Server::petitemswap(int petIndex, int from, int to)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (to != -1)
	{
		if ((from >= CHAR_EQUIPPLACENUM) || (to >= CHAR_EQUIPPLACENUM))
			return;
	}

	lssproto_PetItemEquip_send(getPoint(), petIndex, from, to);
}

//料理/加工
void Server::craft(util::CraftType type, const QStringList& ingres)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (ingres.size() < 2 || ingres.size() > 5)
		return;

	QStringList itemIndexs;
	int petIndex = -1;

	//int i = 0;
	//int j = 0;

	QString skillName;
	if (type == util::CraftType::kCraftFood)
		skillName = QString(u8"料理");
	else
		skillName = QString(u8"加工");

	int skillIndex = getPetSkillIndexByName(petIndex, skillName);
	if (petIndex == -1 || skillIndex == -1)
		return;

	for (const QString& it : ingres)
	{
		int index = getItemIndexByName(it, true, "", CHAR_EQUIPPLACENUM);
		if (index == -1)
			return;

		itemIndexs.append(QString::number(index));
	}

	QString qstr = itemIndexs.join("|");
	std::string str = qstr.toStdString();
	lssproto_PS_send(petIndex, skillIndex, NULL, const_cast<char*>(str.c_str()));
}

void Server::depositGold(int gold, bool isPublic)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (gold <= 0)
		return;

	QString qstr = QString("B|%1|%2").arg(!isPublic ? "G" : "T").arg(gold);
	std::string str = qstr.toStdString();
	lssproto_FM_send(const_cast<char*>(str.c_str()));
}

void Server::withdrawGold(int gold, bool isPublic)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (gold <= 0)
		return;

	QString qstr = QString("B|%1|%2").arg(!isPublic ? "G" : "T").arg(-gold);
	std::string str = qstr.toStdString();
	lssproto_FM_send(const_cast<char*>(str.c_str()));
}

void Server::tradeComfirm(const QString name)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (!IS_TRADING)
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	QString cmd = QString("T|%1|%2|C|confirm").arg(opp_sockfd).arg(opp_name);
	std::string scmd = util::fromUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));
}

void Server::tradeCancel()
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (!IS_TRADING)
		return;

	QString cmd = QString("W|%1|%2").arg(opp_sockfd).arg(opp_name);
	std::string scmd = util::fromUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));
	PC pc = getPC();
	pc.trade_confirm = 1;
	setPC(pc);
	tradeStatus = 0;
}

bool Server::tradeStart(const QString& name, int timeout)
{
	if (!getOnlineFlag())
		return false;

	if (getBattleFlag())
		return false;

	if (IS_TRADING)
		return false;

	lssproto_TD_send(const_cast<char*>("D|D"));

	QElapsedTimer timer; timer.start();
	for (;;)
	{
		if (!getOnlineFlag())
			return false;

		if (getBattleFlag())
			return false;

		if (timer.hasExpired(timeout))
			return false;

		if (IS_TRADING)
			break;

		QThread::msleep(100);
	}

	return opp_name == name;
}

void Server::tradeAppendItems(const QString& name, const QVector<int>& itemIndexs)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (!IS_TRADING)
		return;

	if (itemIndexs.isEmpty())
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	PC pc = getPC();
	for (int i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		bool bret = false;
		int stack = pc.item[i].pile;
		for (int j = 0; j < stack; ++j)
		{
			QString cmd = QString("T|%1|%2|I|1|%3").arg(opp_sockfd).arg(opp_name).arg(i);
			std::string scmd = util::fromUnicode(cmd);
			lssproto_TD_send(const_cast<char*>(scmd.c_str()));
			bret = true;
		}

		if (bret)
		{
			myitem_tradeList.append(QString("I|%1").arg(i));
		}
		else
		{
			myitem_tradeList.append("I|-1");
		}
	}
}

void Server::tradeAppendGold(const QString& name, int gold)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (!IS_TRADING)
		return;

	PC pc = getPC();
	if (gold < 0 || gold > pc.gold)
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	if (mygoldtrade != 0)
		return;

	QString cmd = QString("T|%1|%2|G|%3|%4").arg(opp_sockfd).arg(opp_name).arg(3).arg(gold);
	std::string scmd = util::fromUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));
	mygoldtrade = gold;
}

void Server::tradeAppendPets(const QString& name, const QVector<int>& petIndexs)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (!IS_TRADING)
		return;

	if (petIndexs.isEmpty())
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	//T|87| 02020202|P|3| 3 |  攻击|忠犬|料理|||||乌力斯坦|嘿嘿嘿嘿
	//T|%s| %s      |P|3| %d | %s
	QStringList list = mypet_tradeList;
	for (const int index : petIndexs)
	{
		if (index < 0 || index >= MAX_PET)
			continue;

		PET _pet = pet[index];
		QStringList list;
		for (const auto& it : petSkill[index])
		{
			if (it.useFlag == 0)
				list.append("");
			else
				list.append(it.name);
		}
		list.append(_pet.name);
		list.append(_pet.freeName);

		QString cmd = QString("T|%1|%2|P|3|%3|%4").arg(opp_sockfd).arg(opp_name).arg(index).arg(list.join("|"));
		std::string scmd = util::fromUnicode(cmd);
		lssproto_TD_send(const_cast<char*>(scmd.c_str()));
		list[index] = QString("P|%1").arg(index);
	}
	mypet_tradeList = list;
}

void Server::tradeComplete(const QString& name)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (!IS_TRADING)
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	QStringList mytradeList;
	mytradeList.append(myitem_tradeList.join("|"));
	mytradeList.append(mypet_tradeList.join("|"));
	mytradeList.append(QString("G|%1").arg(mygoldtrade));

	QStringList opptradelist;
	for (const showitem& it : opp_item)
	{
		if (it.name.isEmpty())
			opptradelist.append("I|-1");
		else
			opptradelist.append(QString("I|%1").arg(it.itemindex));
	}

	for (const showpet& it : opp_pet)
	{
		if (it.opp_petname.isEmpty())
			opptradelist.append("P|-1");
		else
			opptradelist.append(QString("P|%1").arg(it.opp_petindex));
	}

	opptradelist.append(QString("G|%1").arg(tradeWndDropGoldGet));

	QString cmd = QString("T|%1|%2|K|%3|%4").arg(opp_sockfd).arg(opp_name).arg(mytradeList.join("|")).arg(opptradelist.join("|"));
	std::string scmd = util::fromUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));

	//T|87|02020202|K|
	//I|-1|I|-1|I|-1|I|-1|I|9|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|
	//P|-1|P|-1|P|-1|P|-1|P|-1|
	//G|6556|
	//I|9|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|I|-1|
	//P|-1|P|-1|P|2|P|-1|P|-1|
	//G|142|
}
#pragma endregion

#pragma region SAOriginal
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
void Server::setPcParam(const QString& name
	, const QString& freeName
	, int level
	, const QString& petname
	, int petlevel
	, int nameColor, int walk
	, int height
	, int profession_class
	, int profession_level
	, int profession_skill_point
	, int herofloor)
#else
void Server::setPcParam(const QString& name
	, const QString& freeName
	, int level
	, const QString& petname
	, int petlevel
	, int nameColor
	, int walk
	, int height
	, int profession_class
	, int profession_level
	, int profession_skill_point)
#endif
	// 	#endif
#else
void Server::setPcParam(const QString& name
	, const QString& freeName
	, int level
	, const QString& petname
	, int petlevel
	, int nameColor
	, int walk
	, int height)
#endif
{
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
	int gmnameLen;
#endif

	PC pc = getPC();
	pc.name = name;

	pc.freeName = freeName;

	pc.level = level;

	pc.ridePetName = petname;

	pc.ridePetLevel = petlevel;

	pc.nameColor = nameColor;
	if (walk != 0)
	{
		//pc.status |= CHR_STATUS_W;
	}
	if (height != 0)
	{
		//pc.status |= CHR_STATUS_H;
	}

	//if (pc.ptAct == NULL)
	//	return;

	/*if (nameLen <= CHAR_NAME_LEN)
	{
		strcpy(pc.ptAct->name, name);
	}
	if (freeNameLen <= CHAR_FREENAME_LEN)
	{
		strcpy(pc.ptAct->freeName, freeName);
	}
	pc.ptAct->level = level;

	if (petnameLen <= CHAR_FREENAME_LEN)
	{
		strcpy(pc.ptAct->petName, petname);
	}*/
	//pc.ptAct->petLevel = petlevel;

	//pc.ptAct->itemNameColor = nameColor;

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	pc.profession_class = profession_class;
	//pc.ptAct->profession_class = profession_class;
	pc.profession_level = profession_level;
	//	pc.profession_exp = profession_exp;
	pc.profession_skill_point = profession_skill_point;
#endif
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	pc.herofloor = herofloor;
#endif
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
	gmnameLen = strlen(gm_name);
	if (gmnameLen <= 33)
	{
		strcpy(pc.ptAct->gm_name, gm_name);
	}
#endif

#ifdef _CHANNEL_MODIFY
#ifdef _CHAR_PROFESSION
	if (pc.profession_class == 0)
	{
		//pc.etcFlag &= ~PC_ETCFLAG_CHAT_OCC;
		//TalkMode = 0;
	}
#endif
#endif

	setPC(pc);
}

void Server::realTimeToSATime(LSTIME* lstime)
{
	constexpr unsigned long long era = 912766409ULL + 5400ULL;
	//cary 十五
	unsigned long long lsseconds = (TimeGetTime() - FirstTime) / 1000ULL + serverTime - era;

	lstime->year = static_cast<int>(lsseconds / (LSTIME_SECONDS_PER_DAY * LSTIME_DAYS_PER_YEAR));

	unsigned long long lsdays = lsseconds / LSTIME_SECONDS_PER_DAY;
	lstime->day = static_cast<int>(lsdays % LSTIME_DAYS_PER_YEAR);


	/*(750*12)*/
	lstime->hour = static_cast<int>((lsseconds % LSTIME_SECONDS_PER_DAY) * LSTIME_HOURS_PER_DAY / LSTIME_SECONDS_PER_DAY);

	return;
}

LSTIME_SECTION getLSTime(LSTIME* lstime)
{
	if (NIGHT_TO_MORNING < lstime->hour && lstime->hour <= MORNING_TO_NOON)
	{
		return LS_MORNING;
	}
	else if (NOON_TO_EVENING < lstime->hour && lstime->hour <= EVENING_TO_NIGHT)
	{
		return LS_EVENING;
	}
	else if (EVENING_TO_NIGHT < lstime->hour && lstime->hour <= NIGHT_TO_MORNING)
	{
		return LS_NIGHT;
	}
	else
		return LS_NOON;
}

#pragma endregion

#pragma region BattleAction

//設置戰鬥結束
void Server::setBattleEnd()
{
	bool battleState = getBattleFlag();
	if (battleState)
	{
		battle_total_time += battleDurationTimer.elapsed();
		normalDurationTimer.restart();
		ayncBattleCommandFlag.store(true, std::memory_order_release);
		ayncBattleCommandSync.clearFutures();
		ayncBattleCommandFlag.store(false, std::memory_order_release);
	}

	lssproto_EO_send(0);
	//lssproto_Echo_send(const_cast<char*>("hoge"));

	setBattleFlag(false);

	if (getWorldStatus() == 10)
	{
		setGameStatus(7);
	}
}

inline bool Server::checkFlagState(int pos)
{
	if (pos < 0 || pos >= 20)
		return false;
	return checkAND(BattleAnimFlag, 1 << pos);
}

//異步處理自動/快速戰鬥邏輯和發送封包
void Server::doBattleWork(bool async)
{
	Injector& injector = Injector::getInstance();
	bool fastChecked = injector.getEnableHash(util::kFastBattleEnable);
	bool normalChecked = injector.getEnableHash(util::kAutoBattleEnable) || (fastChecked && getWorldStatus() == 10);
	if (async || (normalChecked && checkWG(10, 4)))
		QtConcurrent::run(this, &Server::asyncBattleAction);
	else if (!async && (!normalChecked && getWorldStatus() == 9))
	{
		QtConcurrent::run([this]()
			{
				playerDoBattleWork();
				petDoBattleWork();
				isEnemyAllReady.store(false, std::memory_order_release);
			});
	}
}

void Server::syncBattleAction()
{
	Injector& injector = Injector::getInstance();

	if (!isEnemyAllReady.load(std::memory_order_acquire))
	{
		announce("[async battle] 敌方尚未准备完成，忽略动作", 7);
		return;
	}

	auto delay = [this, &injector](const QString& name)
	{
		//战斗延时
		int delay = injector.getValueHash(util::kBattleActionDelayValue);
		if (delay < 100)
		{
			QThread::msleep(100);
			return;
		}

		announce(QString("[async battle] 战斗 %1 开始延时 %2 毫秒").arg(name).arg(delay), 6);

		if (delay > 1000)
		{
			for (int i = 0; i < delay / 1000; ++i)
			{
				QThread::msleep(1000);
				if (isInterruptionRequested())
					return;
			}

			QThread::msleep(delay % 1000);
		}
		else
		{
			QThread::msleep(delay);
		}
	};

	auto setCurrentRoundEnd = [this]()
	{
		//这里不发的话一般战斗、和快战都不会再收到后续的封包 (应该?)
		lssproto_Echo_send(const_cast<char*>("hoge"));
		isEnemyAllReady.store(false, std::memory_order_release);
	};

	//人物和宠物分开发 TODO 修正多个BA人物多次发出战斗指令的问题
	if (!checkFlagState(BattleMyNo))
	{
		delay(u8"人物");
		//解析人物战斗逻辑并发送指令
		playerDoBattleWork();

	}

	int battlePetIndex = pc_.battlePetNo;
	if (battlePetIndex < 0 || battlePetIndex >= MAX_PET)
	{
		setCurrentRoundEnd();
		return;
	}

	//TODO 修正宠物指令在多个BA时候重复发送的问题
	if (!checkFlagState(BattleMyNo + 5))
	{
		petDoBattleWork();
		setCurrentRoundEnd();
	}
}

void Server::asyncBattleAction()
{
	QMutexLocker lock(&ayncBattleCommandMutex);

	constexpr int MAX_DELAY = 100;

	Injector& injector = Injector::getInstance();

	auto checkAllFlags = [this, &injector]()->bool
	{
		if (ayncBattleCommandFlag.load(std::memory_order_acquire))
		{
			announce("[async battle] 从外部中断的战斗等待", 7);
			return false;
		}

		if (!getOnlineFlag())
		{
			announce("[async battle] 人物不在线上，忽略动作", 7);
			return false;
		}

		if (!getBattleFlag())
		{
			announce("[async battle] 人物不在战斗中，忽略动作", 7);
			return false;
		}

		if (!isEnemyAllReady.load(std::memory_order_acquire))
		{
			announce("[async battle] 敌方尚未准备完成，忽略动作", 7);
			return false;
		}

		if (!injector.getEnableHash(util::kAutoBattleEnable) && !injector.getEnableHash(util::kFastBattleEnable))
		{
			announce("[async battle] 快战或自动战斗没有开启，忽略动作", 7);
			return false;
		}

		return true;
	};

	if (!checkAllFlags())
		return;

	//自动战斗打开 或 快速战斗打开且处于战斗场景
	bool fastChecked = injector.getEnableHash(util::kFastBattleEnable);
	bool normalChecked = injector.getEnableHash(util::kAutoBattleEnable) || (fastChecked && getWorldStatus() == 10);
	if (normalChecked && !checkWG(10, 4))
	{
		announce("[async battle] 画面不对,当前游戏状态[%1]，画面状态[%2].arg(getWorldStatus()).arg(getGameStatus())", 7);
		return;
	}

	auto delay = [this, &injector, &checkAllFlags](const QString& name)
	{
		//战斗延时
		int delay = injector.getValueHash(util::kBattleActionDelayValue);
		if (delay < 100)
		{
			QThread::msleep(100);
			return;
		}

		announce(QString("[async battle] 战斗 %1 开始延时 %2 毫秒").arg(name).arg(delay), 6);

		if (delay > 1000)
		{
			for (int i = 0; i < delay / 1000; ++i)
			{
				QThread::msleep(1000);
				if (!checkAllFlags())
					return;
			}

			QThread::msleep(delay % 1000);
		}
		else
		{
			QThread::msleep(delay);
		}
	};

	auto setCurrentRoundEnd = [this, normalChecked]()
	{
		//通知结束这一回合
		if (normalChecked)
		{
			//if (Checked)
			//mem::write<short>(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1);
			int G = getGameStatus();
			if (G == 4)
			{
				setGameStatus(5);
				isBattleDialogReady.store(false, std::memory_order_release);
			}
		}

		//这里不发的话一般战斗、和快战都不会再收到后续的封包 (应该?)
		//lssproto_EO_send(0);
		//lssproto_Echo_send(const_cast<char*>("hoge"));
		isEnemyAllReady.store(false, std::memory_order_release);
	};

	battledata_t bt = getBattleData();
	//人物和宠物分开发 TODO 修正多个BA人物多次发出战斗指令的问题
	if (!checkFlagState(BattleMyNo) && !bt.charAlreadyAction)
	{
		bt.charAlreadyAction = true;
		//announce(QString("[async battle] 准备发出人物战斗指令"));
		delay(u8"人物");
		//解析人物战斗逻辑并发送指令
		playerDoBattleWork();
		//announce("[async battle] 人物战斗指令发送完毕");

	}

	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
	{
		bt.petAlreadyAction = true;
		//announce("[async battle] 无战宠，忽略战宠动作", 7);
		setCurrentRoundEnd();
		return;
	}

	//TODO 修正宠物指令在多个BA时候重复发送的问题
	if (!checkFlagState(BattleMyNo + 5) && !bt.petAlreadyAction)
	{
		bt.petAlreadyAction = true;
		//announce(QString("[async battle] 准备发出宠物战斗指令"));
		petDoBattleWork();
		//announce("[async battle] 宠物战斗指令发送完毕");
		setCurrentRoundEnd();
	}
}

//人物戰鬥
int Server::playerDoBattleWork()
{
	battledata_t bt = getBattleData();
	if (hasUnMoveableStatue(bt.player.status))
	{
		sendBattlePlayerDoNothing();
		//announce("[async battle] 人物在不可动作的异常状态中，指令发送完毕", 6);
		return 1;
	}
	Injector& injector = Injector::getInstance();
	do
	{
		//自動逃跑
		if (injector.getEnableHash(util::kAutoEscapeEnable))
		{
			sendBattlePlayerEscapeAct();
			break;
		}

		handlePlayerBattleLogics();

	} while (false);

	//if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
	//	mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//else
	//

	//
	return 1;
}

//寵物戰鬥
int Server::petDoBattleWork()
{
	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return 0;
	battledata_t bt = getBattleData();
	Injector& injector = Injector::getInstance();
	do
	{
		//自動逃跑
		if (hasUnMoveableStatue(bt.pet.status) || injector.getEnableHash(util::kAutoEscapeEnable) || petEscapeEnableTempFlag)
		{
			sendBattlePetDoNothing();
			break;
		}

		handlePetBattleLogics();

	} while (false);
	//mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1, sizeof(short));
	return 1;
}

//檢查人物血量
bool Server::checkPlayerHp(int cmpvalue, int* target, bool useequal)
{
	PC pc = getPC();
	if (useequal && (pc.hpPercent <= cmpvalue))
	{
		if (target)
			*target = BattleMyNo;
		return true;
	}
	else if (!useequal && (pc.hpPercent < cmpvalue))
	{
		if (target)
			*target = BattleMyNo;
		return true;
	}

	return false;
};

//檢查人物氣力
bool Server::checkPlayerMp(int cmpvalue, int* target, bool useequal)
{
	PC pc = getPC();
	if (useequal && (pc.mpPercent <= cmpvalue))
	{
		if (target)
			*target = BattleMyNo;
		return true;
	}
	else if (!useequal && (pc.mpPercent < cmpvalue))
	{
		if (target)
			*target = BattleMyNo;
		return true;
	}

	return  false;
};

//檢測戰寵血量
bool Server::checkPetHp(int cmpvalue, int* target, bool useequal)
{
	PC pc = getPC();
	int i = pc.battlePetNo;
	if (i < 0 || i >= MAX_PET)
		return false;

	if (useequal && (pet[i].hpPercent <= cmpvalue) && (pet[i].level > 0) && (pet[i].maxHp > 0))
	{
		if (target)
			*target = BattleMyNo + 5;
		return true;
	}
	else if (!useequal && (pet[i].hpPercent < cmpvalue) && (pet[i].level > 0) && (pet[i].maxHp > 0))
	{
		if (target)
			*target = BattleMyNo + 5;
		return true;
	}

	return false;
};

//檢測騎寵血量
bool Server::checkRideHp(int cmpvalue, int* target, bool useequal)
{
	PC pc = getPC();
	int i = pc.ridePetNo;
	if (i < 0 || i >= MAX_PET)
		return false;

	if (useequal && (pet[i].hpPercent <= cmpvalue) && (pet[i].level > 0) && (pet[i].maxHp > 0))
	{
		if (target)
			*target = BattleMyNo + 5;
		return true;
	}
	else if (!useequal && (pet[i].hpPercent < cmpvalue) && (pet[i].level > 0) && (pet[i].maxHp > 0))
	{
		if (target)
			*target = BattleMyNo + 5;
		return true;
	}

	return false;
};

//檢測隊友血量
bool Server::checkPartyHp(int cmpvalue, int* target)
{
	if (!target)
		return false;

	PC pc = getPC();
	if (!(pc.status & CHR_STATUS_PARTY) && !(pc.status & CHR_STATUS_LEADER))
		return false;

	for (int i = 0; i < MAX_PARTY; ++i)
	{
		if (party[i].hpPercent < cmpvalue && party[i].level > 0 && party[i].maxHp > 0 && party[i].useFlag == 1)
		{
			*target = i;
			return true;
		}
	}

	return false;
}

//檢查是否寵物是否已滿
bool Server::isPetSpotEmpty() const
{
	for (int i = 0; i < MAX_PET; ++i)
	{
		if ((pet[i].level <= 0) || (pet[i].maxHp <= 0) || (pet[i].useFlag <= 0))
			return false;
	}

	return true;
}

//人物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
void Server::handlePlayerBattleLogics()
{
	using namespace util;
	Injector& injector = Injector::getInstance();

	tempCatchPetTargetIndex = -1;
	petEscapeEnableTempFlag = false; //用於在必要的時候切換戰寵動作為逃跑模式

	QStringList items;
	battledata_t bt = getBattleData();
	QVector<battleobject_t> battleObjects = bt.enemies;
	QVector<battleobject_t> tempbattleObjects;
	sortBattleUnit(battleObjects);

	int target = -1;

	auto checkAllieHp = [this, &bt](int cmpvalue, int* target, bool useequal)->bool
	{
		if (!target)
			return false;

		int min = 0;
		int max = 9;
		if (BattleMyNo >= 10)
		{
			min = 10;
			max = 19;
		}

		QVector<battleobject_t> battleObjects = bt.objects;
		for (const battleobject_t& obj : battleObjects)
		{
			if (obj.pos < min || obj.pos > max)
				continue;

			if (!useequal && (obj.hpPercent < cmpvalue))
			{
				*target = obj.pos;
				return true;
			}
			else if (useequal && (obj.hpPercent <= cmpvalue))
			{
				*target = obj.pos;
				return true;
			}
		}

		return false;
	};

	//自動捉寵
	do
	{
		bool autoCatch = injector.getEnableHash(util::kAutoCatchEnable);
		if (!autoCatch)
			break;

		if (isPetSpotEmpty())
		{
			petEscapeEnableTempFlag = true;
			sendBattlePlayerEscapeAct();
			return;
		}

		int tempTarget = -1;

		//檢查等級條件
		bool levelLimitEnable = injector.getEnableHash(util::kBattleCatchTargetLevelEnable);
		if (levelLimitEnable)
		{
			int levelLimit = injector.getValueHash(util::kBattleCatchTargetLevelValue);
			if (levelLimit <= 0 || levelLimit > 255)
				levelLimit = 1;

			if (matchBattleEnemyByLevel(levelLimit, battleObjects, &tempbattleObjects))
			{
				battleObjects = tempbattleObjects;
			}
			else
				battleObjects.clear();
		}

		//檢查最大血量條件
		bool maxHpLimitEnable = injector.getEnableHash(util::kBattleCatchTargetMaxHpEnable);
		if (maxHpLimitEnable && !battleObjects.isEmpty())
		{
			int maxHpLimit = injector.getValueHash(util::kBattleCatchTargetMaxHpValue);
			if (matchBattleEnemyByMaxHp(maxHpLimit, battleObjects, &tempbattleObjects))
			{
				battleObjects = tempbattleObjects;
			}
			else
				battleObjects.clear();
		}

		//檢查名稱條件
		QStringList targetList = injector.getStringHash(util::kBattleCatchPetNameString).split(util::rexOR, Qt::SkipEmptyParts);
		if (!targetList.isEmpty() && !battleObjects.isEmpty())
		{
			bool bret = false;
			for (const QString& it : targetList)
			{
				if (matchBattleEnemyByName(it, true, battleObjects, &tempbattleObjects))
				{
					if (!bret)
						bret = true;
					battleObjects = tempbattleObjects;
				}
			}

			if (!bret)
				battleObjects.clear();
		}

		//目標不存在的情況下
		if (battleObjects.isEmpty())
		{
			int catchMode = injector.getValueHash(util::kBattleCatchModeValue);
			if (0 == catchMode)
			{
				//遇敵逃跑
				petEscapeEnableTempFlag = true;
				sendBattlePlayerEscapeAct();
				return;
			}

			//遇敵攻擊 (跳出while 往下檢查其他常規動作)
			break;
		}

		battleobject_t obj = battleObjects.first();
		battleObjects.pop_front();
		tempTarget = obj.pos;
		tempCatchPetTargetIndex = tempTarget;

		//允許人物動作降低血量
		bool allowPlayerAction = injector.getEnableHash(util::kBattleCatchPlayerMagicEnable);
		int hpLimit = injector.getValueHash(util::kBattleCatchTargetMagicHpValue);
		if (allowPlayerAction && (obj.hpPercent >= hpLimit))
		{
			int actionType = injector.getValueHash(util::kBattleCatchPlayerMagicValue);
			if (actionType == 1)
			{
				sendBattlePlayerDefenseAct();
				return;
			}
			else if (actionType == 2)
			{
				sendBattlePlayerEscapeAct();
				return;
			}

			if (actionType == 0)
			{
				if (tempTarget >= 0 && tempTarget < MAX_ENEMY)
				{
					sendBattlePlayerAttackAct(tempTarget);
					return;
				}
			}
			else
			{
				int magicIndex = actionType - 3;
				bool isProfession = magicIndex > (MAX_MAGIC - 1);
				if (isProfession) //0 ~ MAX_PROFESSION_SKILL
				{
					magicIndex -= MAX_MAGIC;
					target = -1;
					if (fixPlayerTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
					{
						if (isPlayerMpEnoughForSkill(magicIndex))
						{
							sendBattlePlayerJobSkillAct(magicIndex, target);
							return;
						}
						else
						{
							tempTarget = getBattleSelectableEnemyTarget(bt);
							sendBattlePlayerAttackAct(tempTarget);
						}
					}
				}
				else
				{
					//如果敵方已經中狀態，則尋找下一個目標
					for (;;)
					{
						if (hasBadStatus(obj.status))
						{
							if (!battleObjects.isEmpty())
							{
								obj = battleObjects.front();
								tempTarget = obj.pos;
								battleObjects.pop_front();
								continue;
							}
							else
							{
								sendBattlePlayerDefenseAct();
								return;
							}
						}
						else
							break;
					}

					target = -1;
					if (fixPlayerTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
					{
						if (isPlayerMpEnoughForMagic(magicIndex))
						{
							sendBattlePlayerMagicAct(magicIndex, target);
							return;
						}
						else
							break;
					}
				}
			}
		}

		//允許人物道具降低血量
		bool allowPlayerItem = injector.getEnableHash(util::kBattleCatchPlayerItemEnable);
		hpLimit = injector.getValueHash(util::kBattleCatchTargetItemHpValue);
		if (allowPlayerItem && (obj.hpPercent >= hpLimit))
		{
			int itemIndex = -1;
			QString text = injector.getStringHash(util::kBattleCatchPlayerItemString).simplified();
			items = text.split(util::rexOR, Qt::SkipEmptyParts);
			for (const QString& str : items)
			{
				itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
				if (itemIndex != -1)
					break;
			}

			target = -1;
			if (itemIndex != -1)
			{
				if (fixPlayerTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0 && target <= 21))
				{
					sendBattlePlayerItemAct(itemIndex, target);
					return;
				}
			}
		}

		sendBattlePlayerCatchPetAct(tempTarget);
		return;
	} while (false);

	//鎖定逃跑
	do
	{
		bool lockEscapeEnable = injector.getEnableHash(util::kLockEscapeEnable);
		if (!lockEscapeEnable)
			break;

		QString text = injector.getStringHash(util::kLockEscapeString);
		QStringList targetList = text.split(util::rexOR, Qt::SkipEmptyParts);
		if (targetList.isEmpty())
			break;

		for (const QString& it : targetList)
		{
			if (matchBattleEnemyByName(it, true, battleObjects, &tempbattleObjects))
			{
				sendBattlePlayerEscapeAct();
				petEscapeEnableTempFlag = true;
				return;
			}
		}
	} while (false);

	//鎖定攻擊
	do
	{
		bool lockAttackEnable = injector.getEnableHash(util::kLockAttackEnable);
		if (!lockAttackEnable)
			break;

		QString text = injector.getStringHash(util::kLockAttackString);
		QStringList targetList = text.split(util::rexOR, Qt::SkipEmptyParts);
		if (targetList.isEmpty())
			break;

		bool bret = false;
		for (const QString& it : targetList)
		{
			if (matchBattleEnemyByName(it, true, battleObjects, &tempbattleObjects))
			{
				bret = true;
				battleObjects = tempbattleObjects;
				target = battleObjects.front().pos;
				break;
			}
		}

		if (bret)
		{
			IS_LOCKATTACK_ESCAPE_DISABLE = true;
			break;
		}

		if (IS_LOCKATTACK_ESCAPE_DISABLE)
			break;

		if (injector.getEnableHash(util::kBattleNoEscapeWhileLockPetEnable))
			break;

		sendBattlePlayerEscapeAct();
		petEscapeEnableTempFlag = true;
		return;
	} while (false);

	//落馬逃跑
	do
	{
		bool fallEscapeEnable = injector.getEnableHash(util::kFallDownEscapeEnable);
		if (!fallEscapeEnable)
			break;

		if (bt.player.rideFlag != 0)
			break;

		sendBattlePlayerEscapeAct();
		petEscapeEnableTempFlag = true;
		return;
	} while (false);

	//道具復活
	do
	{
		bool itemRevive = injector.getEnableHash(util::kBattleItemReviveEnable);
		if (!itemRevive)
			break;

		int tempTarget = -1;
		bool ok = false;
		unsigned int targetFlags = injector.getValueHash(util::kBattleItemReviveTargetValue);
		if (targetFlags & kSelectPet)
		{
			if (checkPetHp(NULL, &tempTarget, true))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if ((targetFlags & kSelectAllieAny) || (targetFlags & kSelectAllieAll))
			{
				if (checkAllieHp(NULL, &tempTarget, true))
				{
					ok = true;
				}
			}
		}


		if (!ok)
			break;

		QString text = injector.getStringHash(util::kBattleItemReviveItemString).simplified();
		if (text.isEmpty())
			break;

		items = text.split(util::rexOR, Qt::SkipEmptyParts);
		if (items.isEmpty())
			break;

		int itemIndex = -1;
		for (const QString& str : items)
		{
			itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
			if (itemIndex != -1)
				break;
		}

		if (itemIndex == -1)
			break;

		target = -1;
		if (fixPlayerTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0 && target < MAX_ENEMY))
		{
			sendBattlePlayerItemAct(itemIndex, target);
			return;
		}
	} while (false);

	//精靈復活
	do
	{
		bool magicRevive = injector.getEnableHash(util::kBattleMagicReviveEnable);
		if (!magicRevive)
			break;

		int tempTarget = -1;
		bool ok = false;
		unsigned int targetFlags = injector.getValueHash(util::kBattleMagicReviveTargetValue);
		if (targetFlags & kSelectPet)
		{
			if (checkPetHp(NULL, &tempTarget, true))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if ((targetFlags & kSelectAllieAny) || (targetFlags & kSelectAllieAll))
			{
				if (checkAllieHp(NULL, &tempTarget, true))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		int magicIndex = injector.getValueHash(util::kBattleMagicReviveMagicValue);
		if (magicIndex <0 || magicIndex > MAX_MAGIC)
			break;

		bool isProfession = magicIndex > (3 + MAX_MAGIC - 1);
		if (isProfession) //0 ~ MAX_PROFESSION_SKILL
		{
			magicIndex -= (3 + MAX_MAGIC);

		}
		else
		{
			target = -1;
			if (fixPlayerTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target < MAX_ENEMY))
			{
				if (isPlayerMpEnoughForMagic(magicIndex))
				{
					sendBattlePlayerMagicAct(magicIndex, target);
					return;
				}
				else
				{
					break;
				}
			}
		}
	} while (false);

	//嗜血补气
	do
	{
		bool skillMp = injector.getEnableHash(util::kBattleSkillMpEnable);
		if (!skillMp)
			break;

		int tempTarget = -1;
		int charMpPercent = injector.getValueHash(util::kBattleSkillMpValue);
		if (!checkPlayerMp(charMpPercent, &tempTarget, true) && (BattleMyMp > 0))
		{
			break;
		}

		int magicIndex = injector.getValueHash(util::kBattleSkillMpSkillValue);
		if (magicIndex < 0)
			break;
		target = 1;
		if (fixPlayerTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
		{
			if (isPlayerHpEnoughForSkill(magicIndex))
			{
				sendBattlePlayerJobSkillAct(magicIndex, target);
				return;
			}
			else
			{
				break;
			}
		}

	} while (false);

	//道具補氣
	do
	{
		bool itemHealMp = injector.getEnableHash(util::kBattleItemHealMpEnable);
		if (!itemHealMp)
			break;

		int tempTarget = -1;
		//bool ok = false;
		int charMpPercent = injector.getValueHash(util::kBattleItemHealMpValue);
		if (!checkPlayerMp(charMpPercent, &tempTarget, true) && (BattleMyMp > 0))
		{
			break;
		}

		QString text = injector.getStringHash(util::kBattleItemHealMpItemString).simplified();
		if (text.isEmpty())
			break;

		items = text.split(util::rexOR, Qt::SkipEmptyParts);
		if (items.isEmpty())
			break;

		int itemIndex = -1;
		for (const QString& str : items)
		{
			itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
			if (itemIndex != -1)
				break;
		}

		if (itemIndex == -1)
			break;

		target = 0;
		if (fixPlayerTargetByItemIndex(itemIndex, tempTarget, &target) && (target == BattleMyNo))
		{
			sendBattlePlayerItemAct(itemIndex, target);
			return;
		}
	} while (false);

	//指定回合
	do
	{
		int atRoundIndex = injector.getValueHash(util::kBattleCharRoundActionRoundValue);
		if (atRoundIndex <= 0)
			break;

		if (atRoundIndex != BattleCliTurnNo + 1)
			break;

		int tempTarget = -1;
		///bool ok = false;

		int enemy = injector.getValueHash(util::kBattleCharRoundActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		int level = injector.getValueHash(util::kBattleCharRoundActionLevelValue);
		if (level != 0)
		{
			auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const battleobject_t& obj1, const battleobject_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		int actionType = injector.getValueHash(util::kBattleCharRoundActionTypeValue);
		if (actionType == 1)
		{
			sendBattlePlayerDefenseAct();
			return;
		}
		else if (actionType == 2)
		{
			sendBattlePlayerEscapeAct();
			return;
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattleCharRoundActionTargetValue);
		if (targetFlags & kSelectEnemyAny)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyAll)
		{
			tempTarget = 21;
		}
		else if (targetFlags & kSelectEnemyFront)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target + 20;

		}
		else if (targetFlags & kSelectEnemyBack)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target + 20;
		}
		else if (targetFlags & kSelectSelf)
		{
			tempTarget = BattleMyNo;
		}
		else if (targetFlags & kSelectPet)
		{
			tempTarget = BattleMyNo + 5;
		}
		else if ((targetFlags & kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (targetFlags & kSelectAllieAll)
		{
			tempTarget = 20;
		}
		else if (targetFlags & kSelectLeader) {}
		else if (targetFlags & kSelectLeaderPet) {}
		else if (targetFlags & kSelectTeammate1) {}
		else if (targetFlags & kSelectTeammate1Pet) {}
		else if (targetFlags & kSelectTeammate2) {}
		else if (targetFlags & kSelectTeammate2Pet) {}
		else if (targetFlags & kSelectTeammate3) {}
		else if (targetFlags & kSelectTeammate3Pet) {}
		else if (targetFlags & kSelectTeammate4) {}
		else if (targetFlags & kSelectTeammate4Pet) {}

		if (actionType == 0)
		{
			if (tempTarget >= 0 && tempTarget < MAX_ENEMY)
			{
				sendBattlePlayerAttackAct(tempTarget);
				return;
			}
			else
			{
				tempTarget = getBattleSelectableEnemyTarget(bt);
				if (tempTarget == -1)
					break;

				sendBattlePlayerAttackAct(tempTarget);
				return;
			}
		}
		else
		{
			int magicIndex = actionType - 3;
			bool isProfession = magicIndex > (MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= MAX_MAGIC;

				if (fixPlayerTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
				{
					if (isPlayerMpEnoughForSkill(magicIndex))
					{
						sendBattlePlayerJobSkillAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattlePlayerAttackAct(tempTarget);
					}
				}
			}
			else
			{

				if (fixPlayerTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
				{
					if (isPlayerMpEnoughForMagic(magicIndex))
					{
						sendBattlePlayerMagicAct(magicIndex, target);
						return;
					}
					else
					{
						break;
					}
				}
			}
		}
	} while (false);

	//間隔回合
	do
	{
		bool crossActionEnable = injector.getEnableHash(util::kCrossActionCharEnable);
		if (!crossActionEnable)
			break;

		int tempTarget = -1;

		int round = injector.getValueHash(util::kBattleCharCrossActionRoundValue) + 1;
		if ((BattleCliTurnNo + 1) % round)
		{
			break;
		}

		int actionType = injector.getValueHash(util::kBattleCharCrossActionTypeValue);
		if (actionType == 1)
		{
			sendBattlePlayerDefenseAct();
			return;
		}
		else if (actionType == 2)
		{
			sendBattlePlayerEscapeAct();
			return;
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattleCharCrossActionTargetValue);
		if (targetFlags & kSelectEnemyAny)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyAll)
		{
			tempTarget = 21;
		}
		else if (targetFlags & kSelectEnemyFront)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target + 20;
		}
		else if (targetFlags & kSelectEnemyBack)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target + 20;
		}
		else if (targetFlags & kSelectSelf)
		{
			tempTarget = BattleMyNo;
		}
		else if (targetFlags & kSelectPet)
		{
			tempTarget = BattleMyNo + 5;
		}
		else if ((targetFlags & kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (targetFlags & kSelectAllieAll)
		{
			tempTarget = 20;
		}
		else if (targetFlags & kSelectLeader) {}
		else if (targetFlags & kSelectLeaderPet) {}
		else if (targetFlags & kSelectTeammate1) {}
		else if (targetFlags & kSelectTeammate1Pet) {}
		else if (targetFlags & kSelectTeammate2) {}
		else if (targetFlags & kSelectTeammate2Pet) {}
		else if (targetFlags & kSelectTeammate3) {}
		else if (targetFlags & kSelectTeammate3Pet) {}
		else if (targetFlags & kSelectTeammate4) {}
		else if (targetFlags & kSelectTeammate4Pet) {}

		if (actionType == 0)
		{
			if (tempTarget >= 0 && tempTarget < MAX_ENEMY)
			{
				sendBattlePlayerAttackAct(tempTarget);
				return;
			}
			else
			{
				tempTarget = getBattleSelectableEnemyTarget(bt);
				if (tempTarget == -1)
					break;

				sendBattlePlayerAttackAct(tempTarget);
				return;
			}
		}
		else
		{
			int magicIndex = actionType - 3;
			bool isProfession = magicIndex > (MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= MAX_MAGIC;

				if (fixPlayerTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
				{
					if (isPlayerMpEnoughForSkill(magicIndex))
					{
						sendBattlePlayerJobSkillAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattlePlayerAttackAct(tempTarget);
					}
				}
			}
			else
			{

				if (fixPlayerTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
				{
					if (isPlayerMpEnoughForMagic(magicIndex))
					{
						sendBattlePlayerMagicAct(magicIndex, target);
						return;
					}
					else
					{
						break;
					}
				}
			}
		}
	} while (false);

	//精靈補血
	do
	{
		bool magicHeal = injector.getEnableHash(util::kBattleMagicHealEnable);
		if (!magicHeal)
			break;

		int tempTarget = -1;
		bool ok = false;
		unsigned int targetFlags = injector.getValueHash(util::kBattleMagicHealTargetValue);
		int charPercent = injector.getValueHash(util::kBattleMagicHealCharValue);
		int petPercent = injector.getValueHash(util::kBattleMagicHealPetValue);
		int alliePercent = injector.getValueHash(util::kBattleMagicHealAllieValue);

		if (targetFlags & kSelectSelf)
		{
			if (checkPlayerHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (targetFlags & kSelectPet)
			{
				if (checkPetHp(petPercent, &tempTarget))
				{
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if ((targetFlags & kSelectAllieAny) || (targetFlags & kSelectAllieAll))
			{
				if (checkAllieHp(alliePercent, &tempTarget, false))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		int magicIndex = injector.getValueHash(util::kBattleMagicHealMagicValue);
		if (magicIndex < 0 || magicIndex > MAX_MAGIC)
			break;

		bool isProfession = magicIndex > (MAX_MAGIC - 1);
		if (!isProfession) // ifMagic
		{
			target = -1;
			if (fixPlayerTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 21))
			{
				if (isPlayerMpEnoughForMagic(magicIndex))
				{
					sendBattlePlayerMagicAct(magicIndex, target);
					return;
				}
				else
				{
					break;
				}
			}
		}
	} while (false);

	//道具補血
	do
	{
		bool itemHeal = injector.getEnableHash(util::kBattleItemHealEnable);
		if (!itemHeal)
			break;

		int tempTarget = -1;
		bool ok = false;

		unsigned int targetFlags = injector.getValueHash(util::kBattleItemHealTargetValue);
		int charPercent = injector.getValueHash(util::kBattleItemHealCharValue);
		int petPercent = injector.getValueHash(util::kBattleItemHealPetValue);
		int alliePercent = injector.getValueHash(util::kBattleItemHealAllieValue);
		if (targetFlags & kSelectSelf)
		{
			if (checkPlayerHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (targetFlags & kSelectPet)
			{
				if (checkPetHp(petPercent, &tempTarget))
				{
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if ((targetFlags & kSelectAllieAny) || (targetFlags & kSelectAllieAll))
			{
				if (checkAllieHp(alliePercent, &tempTarget, false))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		int itemIndex = -1;
		bool meatProiory = injector.getEnableHash(util::kBattleItemHealMeatPriorityEnable);
		if (meatProiory)
		{
			itemIndex = getItemIndexByName(u8"肉", false, u8"耐久力", CHAR_EQUIPPLACENUM);
		}

		if (itemIndex == -1)
		{
			QString text = injector.getStringHash(util::kBattleItemHealItemString).simplified();
			if (text.isEmpty())
				break;

			items = text.split(util::rexOR, Qt::SkipEmptyParts);
			if (items.isEmpty())
				break;

			for (const QString& str : items)
			{
				itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
				if (itemIndex != -1)
					break;
			}
		}

		if (itemIndex == -1)
			break;

		target = -1;
		if (fixPlayerTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0 && target <= 21))
		{
			sendBattlePlayerItemAct(itemIndex, target);
			return;
		}
	} while (false);

	//一般動作
	do
	{
		int tempTarget = -1;
		//bool ok = false;

		int enemy = injector.getValueHash(util::kBattleCharNormalActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		int level = injector.getValueHash(util::kBattleCharNormalActionLevelValue);
		if (level != 0)
		{
			auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const battleobject_t& obj1, const battleobject_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		int actionType = injector.getValueHash(util::kBattleCharNormalActionTypeValue);
		if (actionType == 1)
		{
			sendBattlePlayerDefenseAct();
			return;
		}
		else if (actionType == 2)
		{
			sendBattlePlayerEscapeAct();
			return;
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattleCharNormalActionTargetValue);
		if (targetFlags & kSelectEnemyAny)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyAll)
		{
			tempTarget = 21;
		}
		else if (targetFlags & kSelectEnemyFront)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target + 20;
		}
		else if (targetFlags & kSelectEnemyBack)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target + 20;
		}
		else if (targetFlags & kSelectSelf)
		{
			tempTarget = BattleMyNo;
		}
		else if (targetFlags & kSelectPet)
		{
			tempTarget = BattleMyNo + 5;
		}
		else if ((targetFlags & kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (targetFlags & kSelectAllieAll)
		{
			tempTarget = 20;
		}
		else if (targetFlags & kSelectLeader) {}
		else if (targetFlags & kSelectLeaderPet) {}
		else if (targetFlags & kSelectTeammate1) {}
		else if (targetFlags & kSelectTeammate1Pet) {}
		else if (targetFlags & kSelectTeammate2) {}
		else if (targetFlags & kSelectTeammate2Pet) {}
		else if (targetFlags & kSelectTeammate3) {}
		else if (targetFlags & kSelectTeammate3Pet) {}
		else if (targetFlags & kSelectTeammate4) {}
		else if (targetFlags & kSelectTeammate4Pet) {}

		if (actionType == 0)
		{
			if (tempTarget >= 0 && tempTarget < MAX_ENEMY)
			{
				sendBattlePlayerAttackAct(tempTarget);
				return;
			}
			else
			{
				tempTarget = getBattleSelectableEnemyTarget(bt);
				if (tempTarget == -1)
					break;

				sendBattlePlayerAttackAct(tempTarget);
				return;
			}
		}
		else
		{
			int magicIndex = actionType - 3;
			bool isProfession = magicIndex > (MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= MAX_MAGIC;

				if (fixPlayerTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
				{
					if (isPlayerMpEnoughForSkill(magicIndex))
					{
						sendBattlePlayerJobSkillAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattlePlayerAttackAct(tempTarget);
					}
				}
			}
			else
			{

				if (fixPlayerTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= 22))
				{
					if (isPlayerMpEnoughForMagic(magicIndex))
					{
						sendBattlePlayerMagicAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattlePlayerAttackAct(tempTarget);
					}
				}
			}
		}
	} while (false);

	sendBattlePlayerDoNothing();
}

//寵物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
void Server::handlePetBattleLogics()
{
	using namespace util;
	Injector& injector = Injector::getInstance();
	battledata_t bt = getBattleData();
	QVector<battleobject_t> battleObjects = bt.enemies;
	QVector<battleobject_t> tempbattleObjects;
	sortBattleUnit(battleObjects);
	int target = -1;

	//自動捉寵
	do
	{
		bool autoCatch = injector.getEnableHash(util::kAutoCatchEnable);
		if (!autoCatch)
			break;

		//允許寵物動作
		bool allowPetAction = injector.getEnableHash(util::kBattleCatchPetSkillEnable);
		if (!allowPetAction)
		{
			sendBattlePetDoNothing(); //避免有人會忘記改成防禦，默認只要打開捉寵且沒設置動作就什麼都不做
			return;
		}

		int actionType = injector.getValueHash(util::kBattleCatchPetSkillValue);

		int skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		int tempTarget = tempCatchPetTargetIndex;
		if ((tempTarget != -1) && fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
		{
			sendBattlePetSkillAct(skillIndex, target);
			return;
		}
		sendBattlePetDoNothing(); //避免有人會忘記改成防禦，默認只要打開捉寵且動作失敗就什麼都不做
		return;
	} while (false);

	//鎖定攻擊
	do
	{
		bool lockAttackEnable = injector.getEnableHash(util::kLockAttackEnable);
		if (!lockAttackEnable)
			break;

		QString text = injector.getStringHash(util::kLockAttackString);
		QStringList targetList = text.split(util::rexOR, Qt::SkipEmptyParts);
		if (targetList.isEmpty())
			break;

		bool bret = false;
		for (const QString& it : targetList)
		{
			if (matchBattleEnemyByName(it, true, battleObjects, &tempbattleObjects))
			{
				bret = true;
				battleObjects = tempbattleObjects;
				target = battleObjects.front().pos;
				break;
			}
		}

		if (bret)
			break;

		sendBattlePetDoNothing();
		return;
	} while (false);

	//指定回合
	do
	{
		int atRoundIndex = injector.getValueHash(util::kBattlePetRoundActionEnemyValue);
		if (atRoundIndex <= 0)
			break;

		int tempTarget = -1;
		//bool ok = false;

		int enemy = injector.getValueHash(util::kBattlePetRoundActionLevelValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		int level = injector.getValueHash(util::kBattleCharRoundActionLevelValue);
		if (level != 0)
		{
			auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const battleobject_t& obj1, const battleobject_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattlePetRoundActionTargetValue);
		if (targetFlags & kSelectEnemyAny)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyAll)
		{
			tempTarget = 21;
		}
		else if (targetFlags & kSelectEnemyFront)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyBack)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectSelf)
		{
			tempTarget = BattleMyNo;
		}
		else if (targetFlags & kSelectPet)
		{
			tempTarget = BattleMyNo + 5;
		}
		else if ((targetFlags & kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (targetFlags & kSelectAllieAll)
		{
			tempTarget = 20;
		}
		else if (targetFlags & kSelectLeader) {}
		else if (targetFlags & kSelectLeaderPet) {}
		else if (targetFlags & kSelectTeammate1) {}
		else if (targetFlags & kSelectTeammate1Pet) {}
		else if (targetFlags & kSelectTeammate2) {}
		else if (targetFlags & kSelectTeammate2Pet) {}
		else if (targetFlags & kSelectTeammate3) {}
		else if (targetFlags & kSelectTeammate3Pet) {}
		else if (targetFlags & kSelectTeammate4) {}
		else if (targetFlags & kSelectTeammate4Pet) {}

		int actionType = injector.getValueHash(util::kBattlePetRoundActionTypeValue);

		int skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
		{
			sendBattlePetSkillAct(skillIndex, target);
			return;
		}
	} while (false);

	//間隔回合
	do
	{
		bool crossActionEnable = injector.getEnableHash(util::kCrossActionPetEnable);
		if (!crossActionEnable)
			break;

		int tempTarget = -1;

		int round = injector.getValueHash(util::kBattlePetCrossActionRoundValue) + 1;
		if ((BattleCliTurnNo + 1) % round)
		{
			break;
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattlePetCrossActionTargetValue);
		if (targetFlags & kSelectEnemyAny)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyAll)
		{
			tempTarget = 21;
		}
		else if (targetFlags & kSelectEnemyFront)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyBack)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectSelf)
		{
			tempTarget = BattleMyNo;
		}
		else if (targetFlags & kSelectPet)
		{
			tempTarget = BattleMyNo + 5;
		}
		else if ((targetFlags & kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (targetFlags & kSelectAllieAll)
		{
			tempTarget = 20;
		}
		else if (targetFlags & kSelectLeader) {}
		else if (targetFlags & kSelectLeaderPet) {}
		else if (targetFlags & kSelectTeammate1) {}
		else if (targetFlags & kSelectTeammate1Pet) {}
		else if (targetFlags & kSelectTeammate2) {}
		else if (targetFlags & kSelectTeammate2Pet) {}
		else if (targetFlags & kSelectTeammate3) {}
		else if (targetFlags & kSelectTeammate3Pet) {}
		else if (targetFlags & kSelectTeammate4) {}
		else if (targetFlags & kSelectTeammate4Pet) {}

		int actionType = injector.getValueHash(util::kBattlePetCrossActionTypeValue);

		int skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
		{
			sendBattlePetSkillAct(skillIndex, target);
			return;
		}
		else
		{
			tempTarget = getBattleSelectableEnemyTarget(bt);
			if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
			{
				sendBattlePetSkillAct(skillIndex, target);
				return;
			}
		}
	} while (false);

	//一般動作
	do
	{
		int tempTarget = -1;
		//bool ok = false;

		int enemy = injector.getValueHash(util::kBattlePetNormalActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		int level = injector.getValueHash(util::kBattlePetNormalActionLevelValue);
		if (level != 0)
		{
			auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const battleobject_t& obj1, const battleobject_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattlePetNormalActionTargetValue);
		if (targetFlags & kSelectEnemyAny)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyAll)
		{
			tempTarget = 21;
		}
		else if (targetFlags & kSelectEnemyFront)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectEnemyBack)
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target;
		}
		else if (targetFlags & kSelectSelf)
		{
			tempTarget = BattleMyNo;
		}
		else if (targetFlags & kSelectPet)
		{
			tempTarget = BattleMyNo + 5;
		}
		else if ((targetFlags & kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (targetFlags & kSelectAllieAll)
		{
			tempTarget = 20;
		}
		else if (targetFlags & kSelectLeader) {}
		else if (targetFlags & kSelectLeaderPet) {}
		else if (targetFlags & kSelectTeammate1) {}
		else if (targetFlags & kSelectTeammate1Pet) {}
		else if (targetFlags & kSelectTeammate2) {}
		else if (targetFlags & kSelectTeammate2Pet) {}
		else if (targetFlags & kSelectTeammate3) {}
		else if (targetFlags & kSelectTeammate3Pet) {}
		else if (targetFlags & kSelectTeammate4) {}
		else if (targetFlags & kSelectTeammate4Pet) {}

		int actionType = injector.getValueHash(util::kBattlePetNormalActionTypeValue);

		int skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
		{
			sendBattlePetSkillAct(skillIndex, target);
			return;
		}
		else
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
			{
				sendBattlePetSkillAct(skillIndex, target);
				return;
			}
		}
	} while (false);

	sendBattlePetDoNothing();
}

//精靈名稱匹配精靈索引
int Server::getMagicIndexByName(const QString& name, bool isExact) const
{
	if (name.isEmpty())
		return -1;

	QString newName = name.simplified();
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
		isExact = false;
	}

	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		if (magic[i].name.isEmpty())
			continue;

		if (isExact && (magic[i].name == newName))
			return i;
		else if (!isExact && magic[i].name.contains(newName))
			return i;
	}
	return -1;
}

//根據target判斷文字
QString Server::getAreaString(int target)
{
	if (target == 20)
		return QObject::tr("all allies");
	else if (target == 21)
		return QObject::tr("all enemies");
	else if (target == 22)
		return QObject::tr("all field");

	return QObject::tr("unknown");
}

//寵物名稱查找寵物索引
int Server::getGetPetSkillIndexByName(int petIndex, const QString& name) const
{
	if (!getOnlineFlag())
		return -1;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return -1;

	if (name.isEmpty())
		return -1;

	int petSkillIndex = -1;
	for (int i = 0; i < MAX_SKILL; ++i)
	{
		const QString petSkillName = petSkill[petIndex][i].name;
		if (petSkillName.isEmpty())
			continue;
		if (petSkillName == name)
		{
			petSkillIndex = i;
			break;
		}
	}

	return petSkillIndex;
}

//戰鬥檢查MP是否足夠施放精靈
bool Server::isPlayerMpEnoughForMagic(int magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return false;

	PC pc = getPC();
	if (magic[magicIndex].mp > pc.mp)
		return false;

	return true;
}

//戰鬥檢查MP是否足夠施放技能
bool Server::isPlayerMpEnoughForSkill(int magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= MAX_PROFESSION_SKILL)
		return false;

	PC pc = getPC();
	if (profession_skill[magicIndex].costmp > pc.mp)
		return false;

	return true;
}

//戰鬥檢查HP是否足夠施放技能
bool Server::isPlayerHpEnoughForSkill(int magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= MAX_PROFESSION_SKILL)
		return false;

	PC pc = getPC();
	if (MIN_HP_PERCENT > pc.hpPercent)
		return false;

	return true;
}

//戰場上單位排序
void Server::sortBattleUnit(QVector<battleobject_t>& v) const
{
	QVector<battleobject_t> newv;
	QVector<battleobject_t> dstv = v;
	if (dstv.isEmpty())
		return;

	constexpr int maxorder = 20;
	constexpr int order[maxorder] = { 19, 17, 15, 16, 18, 14, 12, 10, 11, 13, 8, 6, 5, 7, 9, 3, 1, 0, 2, 4 };

	for (const int it : order)
	{
		for (const battleobject_t& obj : dstv)
		{
			if ((obj.hp > 0) && (obj.level > 0) && (obj.maxHp > 0) && (obj.faceid > 0) && (obj.pos == it)
				&& !(obj.status & BC_FLG_DEAD) && !(obj.status & BC_FLG_HIDE))
			{
				newv.append(obj);
				break;
			}
		}
	}


	v = newv;
}

//取戰鬥敵方可選編號
int Server::getBattleSelectableEnemyTarget(const battledata_t& bt) const
{
	int defaultTarget = 19;
	if (BattleMyNo >= 10)
		defaultTarget = 5;

	QVector<battleobject_t> enemies(bt.enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	sortBattleUnit(enemies);
	if (enemies.size())
		return enemies[0].pos;
	else
		return defaultTarget;
}

//取戰鬥一排可選編號
int Server::getBattleSelectableEnemyOneRowTarget(const battledata_t& bt, bool front) const
{
	int defaultTarget = 15;
	if (BattleMyNo >= 10)
		defaultTarget = 5;

	QVector<battleobject_t> enemies(bt.enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	sortBattleUnit(enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	int targetIndex = -1;

	if (front)
	{
		if (BattleMyNo < 10)
		{
			// 只取 pos 在 15~19 之间的，取最前面的
			for (int i = 0; i < enemies.size(); ++i)
			{
				if (enemies[i].pos >= 15 && enemies[i].pos < MAX_ENEMY)
				{
					targetIndex = i;
					break;
				}
			}
		}
		else
		{
			// 只取 pos 在 5~9 之间的，取最前面的
			for (int i = 0; i < enemies.size(); ++i)
			{
				if (enemies[i].pos >= 5 && enemies[i].pos <= 9)
				{
					targetIndex = i;
					break;
				}
			}
		}
	}
	else
	{
		if (BattleMyNo < 10)
		{
			// 只取 pos 在 10~14 之间的，取最前面的
			for (int i = 0; i < enemies.size(); ++i)
			{
				if (enemies[i].pos >= 10 && enemies[i].pos <= 14)
				{
					targetIndex = i;
					break;
				}
			}
		}
		else
		{
			// 只取 pos 在 0~4 之间的，取最前面的
			for (int i = 0; i < enemies.size(); ++i)
			{
				if (enemies[i].pos >= 0 && enemies[i].pos <= 4)
				{
					targetIndex = i;
					break;
				}
			}
		}
	}

	if (targetIndex >= 0 && targetIndex < enemies.size())
	{
		return enemies[targetIndex].level;
	}

	return defaultTarget;
}

//取戰鬥隊友可選目標編號
int Server::getBattleSelectableAllieTarget(const battledata_t& bt) const
{
	int defaultTarget = 5;
	if (BattleMyNo >= 10)
		defaultTarget = 15;

	QVector<battleobject_t> allies(bt.allies);
	if (allies.isEmpty() || !allies.size())
		return defaultTarget;

	sortBattleUnit(allies);
	if (allies.size())
		return allies[0].pos;
	else
		return defaultTarget;
}

//戰鬥匹配敵方名稱
bool Server::matchBattleEnemyByName(const QString& name, bool isExact, QVector<battleobject_t> src, QVector<battleobject_t>* v) const
{
	QVector<battleobject_t> tempv;
	if (name.isEmpty())
		return false;

	QString newName = name;
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
		isExact = false;
	}

	QVector<battleobject_t> enemies = src;

	if (enemies.isEmpty())
		return false;

	sortBattleUnit(enemies);

	for (const battleobject_t& enemy : enemies)
	{
		if (isExact)
		{
			if (enemy.name == newName)
				tempv.append(enemy);
		}
		else
		{
			if (enemy.name.contains(newName))
				tempv.append(enemy);
		}
	}

	if (v)
	{
		*v = tempv;
	}
	return !tempv.isEmpty();
}

//戰鬥匹配敵方最低等級
bool Server::matchBattleEnemyByLevel(int level, QVector<battleobject_t> src, QVector<battleobject_t>* v) const
{
	QVector<battleobject_t> tempv;
	if (level <= 0 || level > 255)
		return false;

	QVector<battleobject_t> enemies = src;

	if (enemies.isEmpty())
		return false;

	sortBattleUnit(enemies);

	for (const battleobject_t& enemy : enemies)
	{
		if (enemy.level == level)
			tempv.append(enemy);
	}

	if (v)
	{
		*v = tempv;
	}
	return !tempv.isEmpty();
}

//戰鬥匹配敵方最大血量
bool Server::matchBattleEnemyByMaxHp(int maxHp, QVector<battleobject_t> src, QVector<battleobject_t>* v) const
{
	QVector<battleobject_t> tempv;
	if (maxHp <= 0 || maxHp > 100000)
		return false;

	QVector<battleobject_t> enemies = src;

	if (enemies.isEmpty())
		return false;

	sortBattleUnit(enemies);

	for (const battleobject_t& enemy : enemies)
	{
		if (enemy.maxHp > maxHp)
			tempv.append(enemy);
	}

	if (v)
	{
		*v = tempv;
	}
	return !tempv.isEmpty();
}

//戰鬥人物修正精靈目標範圍
bool Server::fixPlayerTargetByMagicIndex(int magicIndex, int oldtarget, int* target) const
{
	if (!target)
		return false;

	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return false;

	int magicType = magic[magicIndex].target;

	switch (magicType)
	{
	case MAGIC_TARGET_MYSELF:
	{
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case MAGIC_TARGET_OTHER://雙方任意單體
	{
		if (oldtarget < 0 || oldtarget >= 20)
		{
			if (BattleMyNo < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ALLMYSIDE://我方全體
	{
		if (BattleMyNo < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case MAGIC_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (BattleMyNo < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	case MAGIC_TARGET_ALL://雙方全體同時(場地)
	{
		oldtarget = 22;
		break;
	}
	case MAGIC_TARGET_NONE://無
	{
		oldtarget = -1;
		break;
	}
	case MAGIC_TARGET_OTHERWITHOUTMYSELF://我方任意除了自己
	{
		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{

		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		else if (oldtarget == (BattleMyNo + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WHOLEOTHERSIDE://敵方全體 或 我方全體
	{
		if (oldtarget != 20 && oldtarget != 21)
		{
			if (oldtarget >= 0 && oldtarget <= 9)
			{
				if (BattleMyNo < 10)
					oldtarget = 20;
				else
					oldtarget = 21;
			}
			else
			{
				if (BattleMyNo < 10)
					oldtarget = 21;
				else
					oldtarget = 20;
			}
		}

		break;
	}
	case MAGIC_TARGET_SINGLE:				// 針對敵方某一方
	{
		if (oldtarget < 10 || oldtarget >= MAX_ENEMY)
		{
			if (BattleMyNo < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ONE_ROW:				// 針對敵方某一列
	{
		if (BattleMyNo < 10)
		{
			if (oldtarget < 5)
				oldtarget = 20 + 5;
			else if (oldtarget < 10)
				oldtarget = 20 + 6;
			else if (oldtarget < 15)
				oldtarget = 20 + 3;
			else
				oldtarget = 20 + 4;
		}
		else
		{
			if (oldtarget < 5)
				oldtarget = 20 + 5 - 10;
			else if (oldtarget < 10)
				oldtarget = 20 + 6 - 10;
			else if (oldtarget < 15)
				oldtarget = 20 + 3 - 10;
			else
				oldtarget = 20 + 4 - 10;
		}
		break;
	}
	case MAGIC_TARGET_ALL_ROWS:				// 針對敵方所有人
	{
		if (BattleMyNo < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	default:
		break;
	}

	*target = oldtarget;
	return true;
}

//戰鬥人物修正職業技能目標範圍
bool Server::fixPlayerTargetBySkillIndex(int magicIndex, int oldtarget, int* target) const
{
	if (!target)
		return false;

	if (magicIndex < 0 || magicIndex >= MAX_PROFESSION_SKILL)
		return false;

	int magicType = profession_skill[magicIndex].target;

	switch (magicType)
	{
	case MAGIC_TARGET_MYSELF:
	{
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case MAGIC_TARGET_OTHER://雙方任意單體
	{
		if (oldtarget < 0 || oldtarget >= 20)
		{
			if (BattleMyNo < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ALLMYSIDE://我方全體
	{
		if (BattleMyNo < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case MAGIC_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (BattleMyNo < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	case MAGIC_TARGET_ALL://雙方全體同時(場地)
	{
		oldtarget = 22;
		break;
	}
	case MAGIC_TARGET_NONE://無
	{
		//oldtarget = -1;
		//break;
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case MAGIC_TARGET_OTHERWITHOUTMYSELF://我方任意除了自己
	{
		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{
		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		else if (oldtarget == (BattleMyNo + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WHOLEOTHERSIDE://敵方全體 或 我方全體
	{
		if (oldtarget != 20 && oldtarget != 21)
		{
			if (oldtarget >= 0 && oldtarget <= 9)
			{
				if (BattleMyNo < 10)
					oldtarget = 20;
				else
					oldtarget = 21;
			}
			else
			{
				if (BattleMyNo < 10)
					oldtarget = 21;
				else
					oldtarget = 20;
			}
		}

		break;
	}
	case MAGIC_TARGET_SINGLE:				// 針對敵方某一方
	{
		if (oldtarget < 10 || oldtarget >= MAX_ENEMY)
		{
			if (BattleMyNo < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ONE_ROW:				// 針對敵方某一列
	{
		if (BattleMyNo < 10)
		{
			if (oldtarget < 5)
				oldtarget = 20 + 5;
			else if (oldtarget < 10)
				oldtarget = 20 + 6;
			else if (oldtarget < 15)
				oldtarget = 20 + 3;
			else
				oldtarget = 20 + 4;
		}
		else
		{
			if (oldtarget < 5)
				oldtarget = 20 + 5 - 10;
			else if (oldtarget < 10)
				oldtarget = 20 + 6 - 10;
			else if (oldtarget < 15)
				oldtarget = 20 + 3 - 10;
			else
				oldtarget = 20 + 4 - 10;
		}
		break;
	}
	case MAGIC_TARGET_ALL_ROWS:				// 針對敵方所有人
	{
		if (BattleMyNo < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	default:
		break;
	}

	*target = oldtarget;
	return true;
}

//戰鬥人物修正物品目標範圍
bool Server::fixPlayerTargetByItemIndex(int itemIndex, int oldtarget, int* target) const
{
	if (!target)
		return false;

	if (itemIndex < CHAR_EQUIPPLACENUM || itemIndex >= MAX_ITEM)
		return false;

	PC pc = getPC();
	int itemType = pc.item[itemIndex].target;

	switch (itemType)
	{
	case ITEM_TARGET_MYSELF:
	{
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case ITEM_TARGET_OTHER://雙方任意單體
	{
		if (oldtarget < 0 || oldtarget >= 20)
		{
			if (BattleMyNo < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case ITEM_TARGET_ALLMYSIDE://我方全體
	{
		if (BattleMyNo < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case ITEM_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (BattleMyNo < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	case ITEM_TARGET_ALL://雙方全體同時(場地)
	{
		oldtarget = 22;
		break;
	}
	case ITEM_TARGET_NONE://無
	{
		oldtarget = -1;
		break;
	}
	case ITEM_TARGET_OTHERWITHOUTMYSELF://我方任意除了自己
	{
		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		break;
	}
	case ITEM_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{
		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		else if (oldtarget == (BattleMyNo + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case ITEM_TARGET_PET://戰寵
	{
		oldtarget = BattleMyNo + 5;
		break;
	}

	default:
		break;
	}

	*target = oldtarget;
	return true;
}

//戰鬥修正寵物技能目標範圍
bool Server::fixPetTargetBySkillIndex(int skillIndex, int oldtarget, int* target) const
{
	if (!target)
		return false;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return false;

	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return false;

	int skillType = petSkill[pc.battlePetNo][skillIndex].target;

	switch (skillType)
	{
	case PETSKILL_TARGET_MYSELF:
	{
		if (oldtarget != (BattleMyNo + 5))
			oldtarget = BattleMyNo + 5;
		break;
	}
	case PETSKILL_TARGET_OTHER:
	{
		break;
	}
	case PETSKILL_TARGET_ALLMYSIDE:
	{
		if (BattleMyNo < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case PETSKILL_TARGET_ALLOTHERSIDE:
	{
		if (BattleMyNo < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	case PETSKILL_TARGET_ALL:
	{
		oldtarget = 22;
		break;
	}
	case PETSKILL_TARGET_NONE:
	{
		oldtarget = 0;
		break;
	}
	case PETSKILL_TARGET_OTHERWITHOUTMYSELF:
	{
		if (oldtarget == (BattleMyNo + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case PETSKILL_TARGET_WITHOUTMYSELFANDPET:
	{
		int max = 9;
		int min = 0;
		if (BattleMyNo >= 10)
		{
			max = 19;
			min = 10;
		}

		if (oldtarget < min || oldtarget > max)
		{
			oldtarget = -1;
		}
		else if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		else if (oldtarget == (BattleMyNo + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	}

	*target = oldtarget;

	return true;
}

//戰鬥人物普通攻擊
void Server::sendBattlePlayerAttackAct(int target)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	if (target < 0 || target >= MAX_ENEMY)
		return;

	const QString qcmd = QString("H|%1").arg(QString::number(target, 16));
	lssproto_B_send(qcmd);


	battledata_t bt = getBattleData();
	battleobject_t obj = bt.objects.value(target, battleobject_t{});

	const QString name = !obj.name.isEmpty() ? obj.name : obj.freename;
	const QString text(QObject::tr("use attack [%1]%2").arg(target + 1).arg(name));
	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物使用精靈
void Server::sendBattlePlayerMagicAct(int magicIndex, int target)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return;

	const QString qcmd = QString("J|%1|%2").arg(QString::number(magicIndex, 16)).arg(QString::number(target, 16));
	lssproto_B_send(qcmd);

	const QString magicName = magic[magicIndex].name;

	QString text("");
	if (target < MAX_ENEMY)
	{
		battledata_t bt = getBattleData();
		battleobject_t obj = bt.objects.value(target, battleobject_t{});
		QString name = !obj.name.isEmpty() ? obj.name : obj.freename;
		text = QObject::tr("use magic %1 to [%2]%3").arg(magicName).arg(target + 1).arg(name);
	}
	else
	{
		text = QObject::tr("use %1 to %2").arg(magicName).arg(getAreaString(target));
	}

	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物使用職業技能
void Server::sendBattlePlayerJobSkillAct(int skillIndex, int target)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (skillIndex < 0 || skillIndex >= MAX_PROFESSION_SKILL)
		return;

	const QString qcmd = QString("P|%1|%2").arg(QString::number(skillIndex, 16)).arg(QString::number(target, 16));
	lssproto_B_send(qcmd);

	const QString skillName = profession_skill[skillIndex].name.simplified();
	QString text("");
	if (target < MAX_ENEMY)
	{
		battledata_t bt = getBattleData();
		battleobject_t obj = bt.objects.value(target, battleobject_t{});
		QString name = !obj.name.isEmpty() ? obj.name : obj.freename;
		text = QObject::tr("use skill %1 to [%2]%3").arg(skillName).arg(target + 1).arg(name);
	}
	else
	{
		text = QObject::tr("use %1 to %2").arg(skillName).arg(getAreaString(target));
	}

	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物使用道具
void Server::sendBattlePlayerItemAct(int itemIndex, int target)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	const QString qcmd = QString("I|%1|%2").arg(QString::number(itemIndex, 16)).arg(QString::number(target, 16));
	lssproto_B_send(qcmd);

	PC pc = getPC();
	const QString itemName = pc.item[itemIndex].name.simplified();

	QString text("");
	if (target < MAX_ENEMY)
	{
		battledata_t bt = getBattleData();
		battleobject_t obj = bt.objects.value(target, battleobject_t{});
		QString name = !obj.name.isEmpty() ? obj.name : obj.freename;
		text = QObject::tr("use item %1 to [%2]%3").arg(itemName).arg(target + 1).arg(name);
	}
	else
	{
		text = QObject::tr("use %1 to %2").arg(itemName).arg(getAreaString(target));
	}

	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物防禦
void Server::sendBattlePlayerDefenseAct()
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	const QString qcmd("G");
	lssproto_B_send(qcmd);

	const QString text = QObject::tr("defense");
	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物逃跑
void Server::sendBattlePlayerEscapeAct()
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	const QString qcmd("E");
	lssproto_B_send(qcmd);

	const QString text(QObject::tr("escape"));
	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物捉寵
void Server::sendBattlePlayerCatchPetAct(int target)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	if (target < 0 || target >= MAX_ENEMY)
		return;

	const QString qcmd = QString("T|%1").arg(QString::number(target, 16));
	lssproto_B_send(qcmd);

	battledata_t bt = getBattleData();
	battleobject_t obj = bt.objects.value(target, battleobject_t{});
	QString name = !obj.name.isEmpty() ? obj.name : obj.freename;
	const QString text(QObject::tr("catch [%1]%2").arg(target).arg(name));

	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物切換戰寵
void Server::sendBattlePlayerSwitchPetAct(int petIndex)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (pet[petIndex].useFlag == 0)
		return;

	if (pet[petIndex].hp <= 0)
		return;

	const QString qcmd = QString("S|%1").arg(QString::number(petIndex, 16));
	lssproto_B_send(qcmd);

	QString text(QObject::tr("switch pet to %1") \
		.arg(!pet[petIndex].freeName.isEmpty() ? pet[petIndex].freeName : pet[petIndex].name));

	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥人物什麼都不做
void Server::sendBattlePlayerDoNothing()
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	const QString qcmd("N");
	lssproto_B_send(qcmd);

	QString text(QObject::tr("do nothing"));
	if (labelPlayerAction != text)
	{
		labelPlayerAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

		emit signalDispatcher.updateLabelPlayerAction(text);
	}
}

//戰鬥戰寵技能
void Server::sendBattlePetSkillAct(int skillIndex, int target)
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return;

	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return;

	const QString qcmd = QString("W|%1|%2").arg(QString::number(skillIndex, 16)).arg(QString::number(target, 16));
	lssproto_B_send(qcmd);

	QString text("");
	if (target < MAX_ENEMY)
	{
		QString name;
		if (petSkill[pc.battlePetNo][skillIndex].name != u8"防禦" && petSkill[pc.battlePetNo][skillIndex].name != u8"防御")
		{
			battledata_t bt = getBattleData();
			battleobject_t obj = bt.objects.value(target, battleobject_t{});
			name = !obj.name.isEmpty() ? obj.name : obj.freename;
		}
		else
			name = QObject::tr("self");

		text = QObject::tr("use %1 to [%2]%3").arg(petSkill[pc.battlePetNo][skillIndex].name).arg(target + 1).arg(name);
	}
	else
	{
		text = QObject::tr("use %1 to %2").arg(petSkill[pc.battlePetNo][skillIndex].name).arg(getAreaString(target));
	}

	if (labelPetAction != text)
	{
		labelPetAction = text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPetAction(text);
	}
}

//戰鬥戰寵什麼都不做
void Server::sendBattlePetDoNothing()
{
	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return;

	const QString qcmd("W|FF|FF");

	lssproto_B_send(qcmd);

	QString text(QObject::tr("do nothing"));
	if (labelPetAction != text)
	{
		labelPetAction = text;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateLabelPetAction(text);
	}
}

#pragma endregion

#pragma region Lssproto_Recv
//人物刪除
void Server::lssproto_CharDelete_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (data.isEmpty() && result.isEmpty())
		return;

	if (result == SUCCESSFULSTR || data == SUCCESSFULSTR)
	{
		charDelStatus = 1;
	}
}

//組隊變化
void Server::lssproto_PR_recv(int request, int result)
{
	QStringList teamInfoList;
	PC pc = getPC();
	if (request == 1 && result == 1)
	{
		pc.status |= CHR_STATUS_PARTY;

		for (int i = 0; i < MAX_PARTY; ++i)
		{
			if (party[i].name.isEmpty() || (party[i].useFlag == 0) || (party[i].maxHp <= 0))
			{
				teamInfoList.append("");
				continue;
			}
			QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(party[i].name).arg(party[i].level)
				.arg(party[i].hp).arg(party[i].maxHp).arg(party[i].hpPercent);
			teamInfoList.append(text);
		}
	}
	else
	{
		if (request == 0 && result == 1)
		{
			partyModeFlag = 0;

			int i;
#ifdef	MAX_AIRPLANENUM
			for (i = 0; i < MAX_AIRPLANENUM; ++i)
#else
			for (i = 0; i < MAX_PARTY; ++i)
#endif
			{
				if (party[i].useFlag != 0)
				{
					if (party[i].id == pc.id)
					{
						pc.status &= (~CHR_STATUS_PARTY);
					}
				}
				party[i] = {};
				teamInfoList.append("");
			}
			pc.status &= (~CHR_STATUS_LEADER);
		}
	}
	setPC(pc);
	prSendFlag = 0;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateTeamInfo(teamInfoList);
}

//地圖轉移
void Server::lssproto_EV_recv(int seqno, int result)
{
	Injector& injector = Injector::getInstance();
	int floor = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestNowFloor);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateNpcList(floor);

}

//開關切換
void Server::lssproto_FS_recv(int flg)
{
	PC pc = getPC();
	pc.etcFlag = static_cast<unsigned short>(flg);
	setPC(pc);

	Injector& injector = Injector::getInstance();

	injector.setEnableHash(util::kSwitcherTeamEnable, (flg & PC_ETCFLAG_GROUP) != 0);//組隊開關
	injector.setEnableHash(util::kSwitcherPKEnable, (flg & PC_ETCFLAG_PK) != 0);//決鬥開關
	injector.setEnableHash(util::kSwitcherCardEnable, (flg & PC_ETCFLAG_CARD) != 0);//名片開關
	injector.setEnableHash(util::kSwitcherTradeEnable, (flg & PC_ETCFLAG_TRADE) != 0);//交易開關
	injector.setEnableHash(util::kSwitcherWorldEnable, (flg & PC_ETCFLAG_WORLD) != 0);//世界頻道開關
	injector.setEnableHash(util::kSwitcherFamilyEnable, (flg & PC_ETCFLAG_FM) != 0);//家族頻道開關
	injector.setEnableHash(util::kSwitcherJobEnable, (flg & PC_ETCFLAG_JOB) != 0);//職業頻道開關

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.applyHashSettingsToUI();
}

void Server::lssproto_AB_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i;
	int no;
	QString name;
	int flag;
	int useFlag;
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	QString planetid;
	int j;
#endif

	if (!getOnlineFlag())
		return;

	for (i = 0; i < MAX_ADR_BOOK; ++i)
	{
		//no = i * 6; //the second
		no = i * 8;
		useFlag = getIntegerToken(data, "|", no + 1);
		if (useFlag == -1)
		{
			useFlag = 0;
		}
		if (useFlag <= 0)
		{
#if 0
			if (addressBook[i].useFlag == 1)
#else
			if (!MailHistory[i].dateStr[MAIL_MAX_HISTORY - 1].isEmpty())
#endif
			{
				MailHistory[i] = MailHistory[i];

			}
			addressBook[i].useFlag = 0;
			addressBook[i].name.clear();
			addressBook[i] = {};
			continue;
		}

#ifdef _EXTEND_AB
		if (i == MAX_ADR_BOOK - 1)
			addressBook[i].useFlag = useFlag;
		else
			addressBook[i].useFlag = 1;
#else
		addressBook[i].useFlag = 1;
#endif

		flag = getStringToken(data, "|", no + 2, name);

		if (flag == 1)
			break;

		makeStringFromEscaped(name);
		addressBook[i].name = name.simplified();
		addressBook[i].level = getIntegerToken(data, "|", no + 3);
		addressBook[i].dp = getIntegerToken(data, "|", no + 4);
		addressBook[i].onlineFlag = (short)getIntegerToken(data, "|", no + 5);
		addressBook[i].graNo = getIntegerToken(data, "|", no + 6);
		addressBook[i].transmigration = getIntegerToken(data, "|", no + 7);
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
		for (j = 0; j < MAX_GMSV; ++j)
		{
			if (gmsv[j].used == '1')
			{
				getStringToken(gmsv[j].ipaddr, '.', 4, sizeof(planetid) - 1, planetid);
				if (addressBook[i].onlineFlag == atoi(planetid))
				{
					sprintf_s(addressBook[i].planetname, "%s", gmsv[j].name);
					break;
				}
			}
		}
#endif
	}
}

//名片數據
void Server::lssproto_ABI_recv(int num, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString name;
	//int nameLen;
	int useFlag;
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	QString planetid[8];
	int j;
#endif

	if (num >= MAX_ADR_BOOK)
		return;

	useFlag = getIntegerToken(data, "|", 1);
	if (useFlag < 0)
	{
		useFlag = 0;
	}
	if (useFlag == 0)
	{
		if (MailHistory[num].dateStr[MAIL_MAX_HISTORY - 1][0] != 0)
		{
			MailHistory[num].clear();
		}
		addressBook[num].useFlag = useFlag;
		addressBook[num].name[0] = '\0';
		return;
	}

#ifdef _EXTEND_AB
	if (num == MAX_ADR_BOOK - 1)
		addressBook[num].useFlag = useFlag;
	else
		addressBook[num].useFlag = 1;
#else
	addressBook[num].useFlag = useFlag;
#endif

	getStringToken(data, "|", 2, name);
	makeStringFromEscaped(name);
	name = name.simplified();

	addressBook[num].name = name;
	addressBook[num].level = getIntegerToken(data, "|", 3);
	addressBook[num].dp = getIntegerToken(data, "|", 4);
	addressBook[num].onlineFlag = (short)getIntegerToken(data, "|", 5);
	addressBook[num].graNo = getIntegerToken(data, "|", 6);
	addressBook[num].transmigration = getIntegerToken(data, "|", 7);

#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	if (addressBook[num].onlineFlag == 0)
		sprintf_s(addressBook[num].planetname, " ");
	for (j = 0; j < MAX_GMSV; ++j)
	{
		if (gmsv[j].used == '1')
		{
			getStringToken(gmsv[j].ipaddr, '.', 4, sizeof(planetid) - 1, planetid);
			if (addressBook[num].onlineFlag == atoi(planetid))
			{
				sprintf_s(addressBook[num].planetname, 64, "%s", gmsv[j].name);
				break;
			}
		}
	}
#endif
}

//戰後獎勵 (逃跑或被打死不會有)
void Server::lssproto_RS_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i;
	QString token;
	QString item;
	QStringList texts;

	//cary 確定 欄位 數
	int cols = RESULT_CHR_EXP;
	getStringToken(data, ",", RESULT_CHR_EXP + 1, token);
	if (token[0] == 0)
	{
		cols = RESULT_CHR_EXP - 1;
		battleResultMsg.resChr[RESULT_CHR_EXP - 1].petNo = -1;
		battleResultMsg.resChr[RESULT_CHR_EXP - 1].levelUp = -1;
		battleResultMsg.resChr[RESULT_CHR_EXP - 1].exp = -1;
	}
	//end cary
	QString playerExp = QObject::tr("player exp:");
	QString rideExp = QObject::tr("ride exp:");
	QString petExp = QObject::tr("pet exp:");

	PC pc = getPC();
	for (i = 0; i < cols; ++i)
	{
		if (i >= 5)
			break;
		getStringToken(data, ",", i + 1, token);

		int index = getIntegerToken(token, "|", 1);

		int isLevelUp = getIntegerToken(token, "|", 2);

		QString temp;
		getStringToken(token, "|", 3, temp);
		int exp = a62toi(temp);

		if (index == -2)
		{
			if (isLevelUp)
				++recorder[0].leveldifference;

			recorder[0].expdifference += exp;
			texts.append(playerExp + QString::number(exp));
		}
		else if (pc.ridePetNo == index)
		{
			if (isLevelUp)
				++recorder[index].leveldifference;

			recorder[index].expdifference += exp;
			texts.append(rideExp + QString::number(exp));
		}
		else if (pc.battlePetNo == index)
		{
			if (isLevelUp)
				++recorder[index].leveldifference;
			recorder[index].expdifference += exp;
			texts.append(petExp + QString::number(exp));
		}
	}

	QStringList itemsList;
	auto appendList = [&itemsList](const QString& str)->void
	{
		if (!str.isEmpty())
		{
			itemsList.append(str);
		}
	};

	getStringToken(data, ",", i + 1, token);
	getStringToken(token, "|", 1, item);
	makeStringFromEscaped(item);
	item = item.simplified();

	appendList(item);
	getStringToken(token, "|", 2, item);
	makeStringFromEscaped(item);
	item = item.simplified();

	appendList(item);
	getStringToken(token, "|", 3, item);
	makeStringFromEscaped(item);
	item = item.simplified();

	appendList(item);

	if (!itemsList.isEmpty())
	{
		texts.append(QObject::tr("rewards:"));
		texts.append(itemsList);
	}

	Injector& injector = Injector::getInstance();
	if (texts.size() > 1 && injector.getEnableHash(util::kShowExpEnable))
		announce(texts.join(" "));
}

//戰後經驗 (逃跑或被打死不會有)
void Server::lssproto_RD_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//battleResultMsg.resChr[0].exp = getInteger62Token(data, "|", 1);
	//battleResultMsg.resChr[1].exp = getInteger62Token(data, "|", 2);
}

//道具位置交換
void Server::lssproto_SI_recv(int from, int to)
{
	swapItemLocal(from, to);
	refreshItemInfo();
}

//道具數據改變
void Server::lssproto_I_recv(char* cdata)
{
	PC pc = getPC();
	QMutexLocker locker(&swapItemMutex_);
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i, j;
	int no;
	QString name;
	QString name2;
	QString memo;
	//char *data = "9|烏力斯坦的肉||0|耐久力10前後回覆|24002|0|1|0|7|不會損壞|1|肉|20||10|烏力斯坦的肉||0|耐久力10前後回覆|24002|0|1|0|7|不會損壞|1|肉|20|";
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	for (j = 0; ; ++j)
	{
#ifdef _ITEM_JIGSAW
#ifdef _NPC_ITEMUP
#ifdef _ITEM_COUNTDOWN
		no = j * 17;
#else
		no = j * 16;
#endif
#else
		no = j * 15;
#endif
#else
#ifdef _PET_ITEM
		no = j * 14;
#else
#ifdef _ITEM_PILENUMS
#ifdef _ALCHEMIST
#ifdef _MAGIC_ITEM_
		no = j * 15;
#else
		no = j * 13;
#endif
#else
		no = j * 12;
#endif
#else

		no = j * 11;
#endif
#endif//_PET_ITEM
#endif//_ITEM_JIGSAW
		i = getIntegerToken(data, "|", no + 1);//道具位
		if (getStringToken(data, "|", no + 2, name) == 1)//道具名
			break;

		if (i < 0 || i >= MAX_ITEM)
			break;

		makeStringFromEscaped(name);
		name = name.simplified();
		if (name.isEmpty())
		{
			pc.item[i].useFlag = 0;
			pc.item[i].name.clear();
			pc.item[i] = {};
			refreshItemInfo(i);
			continue;
		}
		pc.item[i].useFlag = 1;
		pc.item[i].name = name;
		getStringToken(data, "|", no + 3, name2);//第二個道具名
		makeStringFromEscaped(name2);
		name2 = name2.simplified();
		pc.item[i].name2 = name2;
		pc.item[i].color = getIntegerToken(data, "|", no + 4);//顏色
		if (pc.item[i].color < 0)
			pc.item[i].color = 0;
		getStringToken(data, "|", no + 5, memo);//道具介紹
		makeStringFromEscaped(memo);
		memo = memo.simplified();
		pc.item[i].memo = memo;
		pc.item[i].graNo = getIntegerToken(data, "|", no + 6);//道具形像
		pc.item[i].field = getIntegerToken(data, "|", no + 7);//
		pc.item[i].target = getIntegerToken(data, "|", no + 8);
		if (pc.item[i].target >= 100)
		{
			pc.item[i].target %= 100;
			pc.item[i].deadTargetFlag = 1;
		}
		else
		{
			pc.item[i].deadTargetFlag = 0;
		}
		pc.item[i].level = getIntegerToken(data, "|", no + 9);//等級
		pc.item[i].sendFlag = getIntegerToken(data, "|", no + 10);

		{
			// 顯示物品耐久度
			QString damage;
			getStringToken(data, "|", no + 11, damage);
			makeStringFromEscaped(damage);
			damage = damage.simplified();
			if (damage.size() <= 16)
			{
				pc.item[i].damage = damage;
			}
		}
#ifdef _ITEM_PILENUMS
		{
			QString pile;
			getStringToken(data, "|", no + 12, pile);
			makeStringFromEscaped(pile);
			pile = pile.simplified();
			pc.item[i].pile = pile.toInt();
			if (pc.item[i].useFlag == 1 && pc.item[i].pile == 0)
				pc.item[i].pile = 1;
		}
#endif

#ifdef _ALCHEMIST //_ITEMSET7_TXT
		{
			QString alch;
			getStringToken(data, "|", no + 13, alch);
			makeStringFromEscaped(alch);
			alch = alch.simplified();
			pc.item[i].alch = alch;
		}
#endif
#ifdef _PET_ITEM
		{
			QString type;
			getStringToken(data, "|", no + 14, type);
			makeStringFromEscaped(type);
			type = type.simplified();
			pc.item[i].type = type.toInt();
		}

		refreshItemInfo(i);
#else
#ifdef _MAGIC_ITEM_
		pc.item[i].道具類型 = getIntegerToken(data, "|", no + 14);
#endif
#endif
		/*
#ifdef _ITEM_JIGSAW
		{
			char jigsaw[10];
			getStringToken(data, "|", no + 15, sizeof(jigsaw) - 1, jigsaw);
			makeStringFromEscaped(jigsaw);
			strcpy( pc.item[i].jigsaw, jigsaw );
			if( i == JigsawIdx ){
				SetJigsaw( pc.item[i].graNo, pc.item[i].jigsaw );
			}
		}
#endif
#ifdef _NPC_ITEMUP
			pc.item[i].itemup = getIntegerToken(data, "|", no + 16);
#endif
#ifdef _ITEM_COUNTDOWN
			pc.item[i].counttime = getIntegerToken(data, "|", no + 17);
#endif
			*/

	}

	setPC(pc);

	QStringList itemList;
	for (const ITEM& it : pc.item)
	{
		if (it.name.isEmpty())
			continue;
		itemList.append(it.name);
	}
	emit signalDispatcher.updateComboBoxItemText(util::kComboBoxItem, itemList);
}

//對話框
void Server::lssproto_WN_recv(int windowtype, int buttontype, int seqno, int objindex, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty() && buttontype == 0)
		return;


	IS_WAITFOR_DIALOG_FLAG = false;

	//第一部分是把按鈕都拆出來
	QString newText = data.trimmed();
	newText.replace("\\n", "\n");
	QStringList strList = newText.split("\n", Qt::SkipEmptyParts);
	if (!strList.isEmpty())
	{
		QString first = strList.first();
		if (!first.isEmpty() && first.at(0).isDigit())
			strList.removeFirst();

		QStringList removeList = {
			u8"：", u8"？", u8"『", u8"～", u8"☆", u8"、", u8"，", u8"。", u8"歡迎",
			u8"欢迎",  u8"選", u8"选", u8"請問", u8"请问"
		};

		for (int i = 0; i < strList.size(); ++i)
		{
			for (const QString& str : removeList)
			{
				if (strList[i].contains(str, Qt::CaseInsensitive))
				{
					strList.removeAt(i);
					i--;
					break;
				}
			}
		}

		for (int i = 0; i < strList.size(); ++i)
		{
			strList[i] = strList[i].simplified();
			strList[i].remove(u8"　");
			strList[i].remove(u8"》");
			strList[i].remove(u8"《");
		}
	}

	//下面是開始檢查寄放處, 寵店
	data.replace("\\n", "\n");
	data.replace("\\c", ",");
	data = data.trimmed();

	static const QStringList BankPetList = {
		u8"2\n　　　　請選擇寵物　　　　\n\n", u8"2\n　　　　请选择宠物　　　　\n\n"
	};
	static const QStringList BankItemList = {
		 u8"1|寄放店|要領取什么呢？|項目有很多|這樣子就可以了嗎？|", u8"1|寄放店|要领取什么呢？|项目有很多|这样子就可以了吗？|"
	};
	static const QStringList FirstWarningList = {
		u8"上一次離線時間", u8"上一次离线时间"
	};
	static const QStringList SecurityCodeList = {
		 u8"安全密碼進行解鎖", u8"安全密码进行解锁"
	};
	static const QStringList KNPCList = {
		//zh_TW
		u8"如果能贏過我的"/*院藏*/, u8"如果想通過"/*近藏*/, u8"吼"/*紅暴*/, u8"你想找麻煩"/*七兄弟*/, u8"多謝～。",
		u8"轉以上確定要出售？", u8"再度光臨", u8"已經太多",

		//zh_CN
		u8"如果能赢过我的"/*院藏*/, u8"如果想通过"/*近藏*/, u8"吼"/*红暴*/, u8"你想找麻烦"/*七兄弟*/, u8"多謝～。",
		u8"转以上确定要出售？", u8"再度光临", u8"已经太多",
	};
	static const QRegularExpression rexBankPet(u8R"(LV\.\s*(\d+)\s*MaxHP\s*(\d+)\s*(\S+))");

	//這個是特殊訊息
	static const QRegularExpression rexExtraInfoBig5(u8R"((?:.*?豆子倍率：\d*(?:\.\d+)?剩余(\d+(?:\.\d+)?)分鐘)?\s+\S+\s+聲望：(\d+)\s+氣勢：(\d+)\s+貝殼：(\d+)\s+活力：(\d+)\s+積分：(\d+)\s+會員點：(\d+))");
	static const QRegularExpression rexExtraInfoGb2312(u8R"((?:.*?豆子倍率：\d*(?:\.\d+)?剩余(\d+(?:\.\d+)?)分钟)?\s+\S+\s+声望：(\d+)\s+气势：(\d+)\s+贝壳：(\d+)\s+活力：(\d+)\s+积分：(\d+)\s+会员点：(\d+))");



	QStringList linedatas;
	if (data.count("|") > 1)
	{
		linedatas = data.split(util::rexOR);
		for (QString& it : linedatas)
			it = it.simplified();
	}
	else
	{
		linedatas = data.split("\n");
		for (QString& it : linedatas)
			it = it.simplified();
	}

	currentDialog.set(dialog_t{ windowtype, buttontype, seqno, objindex, data, linedatas, strList });

	for (const QString& it : BankPetList)
	{
		if (!data.contains(it, Qt::CaseInsensitive))
			continue;

		data.remove(it);
		data = data.trimmed();

		currentBankPetList.first = buttontype;
		currentBankPetList.second.clear();

		//QRegularExpressionMatch itMatch = rexBankPet.match(data);
		QStringList petList = data.split("\n");
		for (const QString& petStr : petList)
		{
			QRegularExpressionMatch itMatch = rexBankPet.match(petStr);
			if (itMatch.hasMatch())
			{
				bankpet_t bankPet;
				bankPet.level = itMatch.captured(1).toInt();
				bankPet.maxHp = itMatch.captured(2).toInt();
				bankPet.name = itMatch.captured(3);
				currentBankPetList.second.append(bankPet);
			}
		}
		IS_WAITFOR_BANK_FLAG = false;
		return;
	}

	for (const QString& it : BankItemList)
	{
		if (!data.contains(it, Qt::CaseInsensitive))
			continue;

		data.remove(it);
		currentBankItemList.clear();
		int index = 0;
		for (;;)
		{
			ITEM item = {};
			QString temp;
			getStringToken(data, "|", 1 + index * 7, temp);
			if (temp.isEmpty())
				break;
			item.useFlag = 1;
			item.name = temp;

			getStringToken(data, "|", 6 + index * 7, temp);
			item.memo = temp;
			currentBankItemList.append(item);
			++index;
		}
		IS_WAITFOR_BANK_FLAG = false;
		return;

	}

	//匹配額外訊息
	QRegularExpressionMatch extraInfoMatch = rexExtraInfoBig5.match(data);
	if (extraInfoMatch.hasMatch())
	{
		currencydata_t currency;
		int n = 1;
		if (extraInfoMatch.lastCapturedIndex() == 7)
			currency.expbufftime = qFloor(extraInfoMatch.captured(n++).toDouble() * 60.0);
		else
			currency.expbufftime = 0;

		currency.prestige = extraInfoMatch.captured(n++).toInt();
		currency.energy = extraInfoMatch.captured(n++).toInt();
		currency.shell = extraInfoMatch.captured(n++).toInt();
		currency.vitality = extraInfoMatch.captured(n++).toInt();
		currency.points = extraInfoMatch.captured(n++).toInt();
		currency.VIPPoints = extraInfoMatch.captured(n++).toInt();

		if (recorder[0].reprecord == 0)
		{
			recorder[0].reprecord = currency.prestige;
			repTimer.restart();
		}
		else
		{
			recorder[0].repearn = currency.prestige - recorder[0].reprecord;
			if (recorder[0].repearn <= 0)
			{
				recorder[0].repearn = 0;
				recorder[0].reprecord = currency.prestige;
				repTimer.restart();
			}
		}

		currencyData = currency;
		IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG = false;
	}
	else
	{
		extraInfoMatch = rexExtraInfoGb2312.match(data);
		if (extraInfoMatch.hasMatch())
		{
			currencydata_t currency;
			int n = 1;
			if (extraInfoMatch.lastCapturedIndex() == 7)
				currency.expbufftime = qFloor(extraInfoMatch.captured(n++).toDouble() * 60.0);
			else
				currency.expbufftime = 0;
			currency.prestige = extraInfoMatch.captured(n++).toInt();
			currency.energy = extraInfoMatch.captured(n++).toInt();
			currency.shell = extraInfoMatch.captured(n++).toInt();
			currency.vitality = extraInfoMatch.captured(n++).toInt();
			currency.points = extraInfoMatch.captured(n++).toInt();
			currency.VIPPoints = extraInfoMatch.captured(n++).toInt();

			if (recorder[0].reprecord == 0)
			{
				recorder[0].reprecord = currency.prestige;
				repTimer.restart();
			}
			else
			{
				recorder[0].repearn = currency.prestige - recorder[0].reprecord;
				if (recorder[0].repearn <= 0)
				{
					recorder[0].repearn = 0;
					recorder[0].reprecord = currency.prestige;
					repTimer.restart();
				}
			}

			currencyData = currency;
			IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG = false;
		}
	}

	//這裡開始是 KNPC
	Injector& injector = Injector::getInstance();

	data = data.simplified();
	for (const QString& it : FirstWarningList)
	{
		if (data.contains(it, Qt::CaseInsensitive))
		{
			press(BUTTON_AUTO, seqno, objindex);
			return;
		}
	}

	for (const QString& it : SecurityCodeList)
	{
		if (!data.contains(it, Qt::CaseInsensitive))
			continue;

		QString securityCode = injector.getStringHash(util::kGameSecurityCodeString);
		if (!securityCode.isEmpty())
		{
			injector.server->unlockSecurityCode(securityCode);
			return;
		}
	}


	if (injector.getEnableHash(util::kKNPCEnable))
	{
		for (const QString& str : KNPCList)
		{
			if (data.contains(str, Qt::CaseInsensitive))
			{
				press(BUTTON_AUTO, seqno, objindex);
				break;
			}
		}
	}


}

void Server::lssproto_PME_recv(int objindex,
	int graphicsno, const QPoint& pos, int dir, int flg, int no, char* cdata)
{
	if (getBattleFlag())
		return;

	if (flg == 0)
	{
		switch (no)
		{
		case 0:
			//createPetAction(graphicsno, x, y, dir, 0, dir, -1);
			break;
		case 1:
			//createPetAction(graphicsno, x, y, dir, 2, 0, -1);
			break;
		}
	}
	else
	{
		QString data = util::toUnicode(cdata);
		if (data.isEmpty())
			return;

		QString smalltoken;
		int id;
		int x;
		int y;
		int dir;
		int graNo;
		int level;
		int nameColor;
		QString name;
		QString freeName;
		int walkable;
		int height;
		int charType;
		int ps = 1;
#ifdef _OBJSEND_C
		ps = 2;
#endif
		charType = getIntegerToken(data, "|", ps++);
		getStringToken(data, "|", ps++, smalltoken);
		id = a62toi(smalltoken);
		getStringToken(data, "|", ps++, smalltoken);
		x = smalltoken.toInt();
		getStringToken(data, "|", ps++, smalltoken);
		y = smalltoken.toInt();
		getStringToken(data, "|", ps++, smalltoken);
		dir = (smalltoken.toInt() + 3) % 8;
		getStringToken(data, "|", ps++, smalltoken);
		graNo = smalltoken.toInt();
		getStringToken(data, "|", ps++, smalltoken);
		level = smalltoken.toInt();
		nameColor = getIntegerToken(data, "|", ps++);
		getStringToken(data, "|", ps++, name);
		makeStringFromEscaped(name);
		name = name.simplified();
		getStringToken(data, "|", ps++, freeName);
		makeStringFromEscaped(freeName);
		freeName = freeName.simplified();
		getStringToken(data, "|", ps++, smalltoken);
		walkable = smalltoken.toInt();
		getStringToken(data, "|", ps++, smalltoken);
		height = smalltoken.toInt();
	}
}

void Server::lssproto_EF_recv(int effect, int level, char* coption)
{
	//char* pCommand = NULL;
	//DWORD dwDiceTimer;
	//QString option = util::toUnicode(coption);

	Injector& injector = Injector::getInstance();
	if (!getOnlineFlag())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	int floor = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffestNowFloor);
	emit signalDispatcher.updateNpcList(floor);
}

//開始戰鬥
void Server::lssproto_EN_recv(int result, int field)
{
	//開始戰鬥為1，未開始戰鬥為0
	if (result > 0)
	{
		announce("[async battle] 收到战斗开始封包");
		setBattleFlag(true);
		IS_LOCKATTACK_ESCAPE_DISABLE = false;
		normalDurationTimer.restart();
		battleDurationTimer.restart();
		++battle_totol;
	}

}

//求救
void Server::lssproto_HL_recv(int flg)
{

}


//戰鬥每回合資訊
void Server::lssproto_B_recv(char* ccommand)
{
	QString command = util::toUnicode(ccommand);
	if (command.isEmpty())
		return;

	QString first = command.left(2);
	first.remove("B");

	QString data = command.mid(3);
	if (data.isEmpty())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	battledata_t bt = getBattleData();
	PC pc = getPC();

	if (first == "C")
	{
		QVector<QStringList> topList;
		QVector<QStringList> bottomList;

		announce("[async battle] ----------------------------------------------");
		announce("[async battle] 收到新的战斗 C 数据");
		/*
		//BC|戰場屬性（0:無屬性,1:地,2:水,3:火,4:風）|人物在組隊中的位置|人物名稱|人物稱號|人物形象編號|人物等級(16進制)|當前HP|最大HP|人物狀態（死亡，中毒等）|是否騎乘標志(0:未騎，1騎,-1落馬)|騎寵名稱|騎寵等級|騎寵HP|騎寵最大HP|戰寵在隊伍中的位置|戰寵名稱|未知|戰寵形象|戰寵等級|戰寵HP|戰寵最大HP|戰寵異常狀態（昏睡，死亡，中毒等）|0||0|0|0|
		//敵1位置|敵1名稱|未知|敵1形象|敵1等級|敵1HP|敵1最大HP|敵人異常狀態（死亡，中毒等）|0||0|0|0|
		BC|0|
		0|01010101|AAAAAAAAAAAA|18A9E|97|555|555|5|1|布洛多斯|94|783|783|
		1|02020202|1010101|18A9E|97|552|552|5|1|布洛多斯|96|748|748|
		5|帖拉所伊朵||18816|7E|546|551|1|0||0|0|0|
		6|帖拉所伊朵||18816|7F|55E|55E|1|0||0|0|0|
		F|貝恩達斯||187F0|2|37|37|1|0||0|0|0|
		10|貝恩達斯||187F0|2|36|36|1|0||0|0|0|
		11|貝恩達斯||187F0|4|4F|4F|1|0||0|0|0|
		*/

		QString temp;
		//fl = getIntegerToken(data, "|", 1);

		bt.allies.clear();
		bt.enemies.clear();
		bt.fieldAttr = 0;
		bt.pet = {};
		bt.player = {};
		bt.objects.clear();
		bt.objects.resize(MAX_ENEMY);
		bt.charAlreadyAction = false;
		bt.charAlreadyAction = false;
		int i = 0;
		int n = 0;
		bt.fieldAttr = getIntegerToken(data, "|", 1);

		bool ok = false;

		for (;;)
		{
			//16進制使用 a62toi
			//string 使用 getStringToken(data, "|", n, var); 而後使用 makeStringFromEscaped(var) 處理轉譯
			//int 使用 getIntegerToken(data, "|", n);
			getStringToken(data, "|", i * 13 + 2, temp);
			int pos = temp.toInt(&ok, 16);
			if (!ok)
				break;

			if (pos < 0 || pos >= MAX_ENEMY)
				break;

			battleobject_t obj = {};

			obj.pos = pos;

			getStringToken(data, "|", i * 13 + 3, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			obj.name = temp;

			getStringToken(data, "|", i * 13 + 4, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			obj.freename = temp;

			getStringToken(data, "|", i * 13 + 5, temp);
			obj.faceid = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 6, temp);
			obj.level = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 7, temp);
			obj.hp = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 8, temp);
			obj.maxHp = temp.toInt(nullptr, 16);

			obj.hpPercent = util::percent(obj.hp, obj.maxHp);

			getStringToken(data, "|", i * 13 + 9, temp);
			obj.status = temp.toInt(nullptr, 16);

			obj.rideFlag = getIntegerToken(data, "|", i * 13 + 10);

			getStringToken(data, "|", i * 13 + 11, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			obj.rideName = temp;

			getStringToken(data, "|", i * 13 + 12, temp);
			obj.rideLevel = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 13, temp);
			obj.rideHp = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 14, temp);
			obj.rideMaxHp = temp.toInt(nullptr, 16);

			obj.rideHpPercent = util::percent(obj.rideHp, obj.rideMaxHp);

			if ((pos >= bt.enemymin) && (pos <= bt.enemymax) && obj.rideFlag == 0 && obj.faceid > 0 && !obj.name.isEmpty())
			{
				if (!enemyNameListCache.contains(obj.name))
				{
					enemyNameListCache.append(obj.name);

					if (enemyNameListCache.size() > 1)
					{
						enemyNameListCache.removeDuplicates();
						std::sort(enemyNameListCache.begin(), enemyNameListCache.end(), util::customStringCompare);
					}
				}
			}

			if (BattleMyNo == pos)
			{
				bt.player.pos = pos;
				bt.player.name = obj.name;
				bt.player.freename = obj.freename;
				bt.player.faceid = obj.faceid;
				bt.player.level = obj.level;
				bt.player.hp = obj.hp;
				bt.player.maxHp = obj.maxHp;
				bt.player.hpPercent = obj.hpPercent;
				bt.player.status = obj.status;
				bt.player.rideFlag = obj.rideFlag;
				bt.player.rideName = obj.rideName;
				bt.player.rideLevel = obj.rideLevel;
				bt.player.rideHp = obj.rideHp;
				bt.player.rideMaxHp = obj.rideMaxHp;

				pc.hp = obj.hp;
				pc.maxHp = obj.maxHp;
				pc.hpPercent = util::percent(pc.hp, pc.maxHp);

				if (pc.hp == 0)
					++recorder[0].deadthcount;
				emit signalDispatcher.updateCharHpProgressValue(obj.level, obj.hp, obj.maxHp);

				if (obj.rideFlag == 1)
				{
					n = -1;
					for (int j = 0; j < MAX_PET; ++j)
					{
						if ((pet[j].maxHp == obj.rideMaxHp) &&
							(pet[j].level == obj.rideLevel) &&
							matchPetNameByIndex(j, obj.rideName))
						{
							n = j;
							break;
						}
					}

					if (pc.ridePetNo != n)
						pc.ridePetNo = n;

					if ((pc.ridePetNo > 0) && (pc.ridePetNo < MAX_PET))
					{
						pet[pc.ridePetNo].hp = obj.rideHp;
						pet[pc.ridePetNo].maxHp = obj.rideMaxHp;

						if (pet[pc.ridePetNo].hp == 0)
							++recorder[pc.ridePetNo + 1].deadthcount;
						emit signalDispatcher.updateRideHpProgressValue(obj.rideLevel, obj.rideHp, obj.rideMaxHp);
					}
				}
				else
				{
					pc.ridePetNo = -1;
				}

				if ((pc.ridePetNo < 0) || (pc.ridePetNo >= MAX_PET))
				{
					emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
				}
			}

			//分開記錄敵我數據
			if (pos >= bt.alliemin && pos <= bt.alliemax)
			{
				if (obj.faceid > 0 && obj.maxHp > 0 && obj.level > 0 && !checkAND(obj.status, BC_FLG_HIDE))
					bt.allies.append(obj);
			}

			if (pos >= bt.enemymin && pos <= bt.enemymax)
			{
				if (obj.faceid > 0 && obj.maxHp > 0 && obj.level > 0 && !checkAND(obj.status, BC_FLG_HIDE))
					bt.enemies.append(obj);
			}

			if (pos < bt.objects.size())
				bt.objects.insert(pos, obj);

			QStringList tempList = {};
			QString temp;
			tempList.append(QString::number(obj.pos));

			QString statusStr = getBadStatusString(obj.status);
			if (!statusStr.isEmpty())
				statusStr = QString(" (%1)").arg(statusStr);

			if (obj.pos == BattleMyNo)
			{
				temp = QString("%1[%2]%3 Lv:%4 HP:%5/%6 (%7) MP:%8%9").arg(obj.ready ? "*" : "").arg(obj.pos + 1).arg(obj.name).arg(obj.level)
					.arg(obj.hp).arg(obj.maxHp).arg(QString::number(obj.hpPercent) + "%").arg(BattleMyMp).arg(statusStr);
			}
			else
			{
				temp = QString("%1[%2]%3 Lv:%4 HP:%5/%6 (%7)%8").arg(obj.ready ? "*" : "").arg(obj.pos + 1).arg(obj.name).arg(obj.level)
					.arg(obj.hp).arg(obj.maxHp).arg(QString::number(obj.hpPercent) + "%").arg(statusStr);
			}

			tempList.append(temp);
			temp.clear();

			if (obj.rideFlag == 1)
			{
				temp = QString("%1[%2]%3 Lv:%4 HP:%5/%6 (%7)%8").arg(obj.ready ? "*" : "").arg(obj.pos + 1).arg(obj.rideName).arg(obj.rideLevel)
					.arg(obj.rideHp).arg(obj.rideMaxHp).arg(QString::number(obj.rideHpPercent) + "%").arg(statusStr);
			}

			tempList.append(temp);

			if (BattleMyNo < 10)
			{
				if ((obj.pos >= 0) && (obj.pos <= 9))
				{
					qDebug() << "allie pos:" << obj.pos << "value:" << tempList;
					bottomList.append(tempList);
				}
				else if ((obj.pos >= 10) && (obj.pos < MAX_ENEMY))
				{
					qDebug() << "enemy pos:" << obj.pos << "value:" << tempList;
					topList.append(tempList);
				}
			}
			else
			{
				if ((obj.pos >= 0) && (obj.pos <= 9))
				{
					qDebug() << "enemy pos:" << obj.pos << "value:" << tempList;
					topList.append(tempList);
				}
				else if ((obj.pos >= 10) && (obj.pos < MAX_ENEMY))
				{
					qDebug() << "allie pos:" << obj.pos << "value:" << tempList;
					bottomList.append(tempList);
				}
			}

			++i;
		}

		//更新戰場動態UI
		QVariant top = QVariant::fromValue(topList);
		if (top.isValid())
		{
			topInfoContents = top;
			emit signalDispatcher.updateTopInfoContents(top);
		}

		QVariant bottom = QVariant::fromValue(bottomList);
		if (bottom.isValid())
		{
			bottomInfoContents = bottom;
			emit signalDispatcher.updateBottomInfoContents(bottom);
		}

		//更新戰寵數據
		if (BattleMyNo >= 0 && BattleMyNo < bt.objects.size())
		{
			battleobject_t obj = bt.objects.value(BattleMyNo + 5, battleobject_t{});
			if (obj.level <= 0 || obj.maxHp <= 0 || obj.faceid <= 0)
			{
				int petIndex = pc.battlePetNo;
				if (petIndex >= 0)
				{
					setFightPet(-1);
					pet[petIndex].state = kRest;
					pc.selectPetNo[petIndex] = FALSE;
					Injector& injector = Injector::getInstance();
					mem::write<short>(injector.getProcess(), injector.getProcessModule() + kOffestSelectPetArray + (petIndex * sizeof(short)), FALSE);
					++recorder[petIndex + 1].deadthcount;
					pc.battlePetNo = -1;
				}
				emit signalDispatcher.updateLabelPetAction("");
				emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
			}
			else
			{
				n = -1;
				for (int j = 0; j < MAX_PET; ++j)
				{
					if ((pet[j].maxHp == obj.maxHp) && (pet[j].level == obj.level)
						&& (matchPetNameByIndex(j, obj.name)))
					{
						n = j;
						break;
					}
				}

				if (pc.battlePetNo != n)
					pc.battlePetNo = n;

				if (pc.battlePetNo >= 0 && pc.battlePetNo < MAX_PET)
				{
					bt.pet.pos = obj.pos;
					bt.pet.name = obj.name;
					bt.pet.freename = obj.freename;
					bt.pet.faceid = obj.faceid;
					bt.pet.level = obj.level;
					bt.pet.hp = obj.hp;
					bt.pet.maxHp = obj.maxHp;
					bt.pet.hpPercent = obj.hpPercent;
					bt.pet.status = obj.status;
					bt.pet.rideFlag = obj.rideFlag;
					bt.pet.rideName = obj.rideName;
					bt.pet.rideLevel = obj.rideLevel;
					bt.pet.rideHp = obj.rideHp;
					bt.pet.rideMaxHp = obj.rideMaxHp;

					pet[pc.battlePetNo].hp = obj.hp;
					pet[pc.battlePetNo].maxHp = obj.maxHp;
					pet[pc.battlePetNo].hpPercent = obj.hpPercent;

					if (pet[pc.battlePetNo].hp == 0)
						++recorder[pc.battlePetNo + 1].deadthcount;
					emit signalDispatcher.updatePetHpProgressValue(obj.level, obj.hp, obj.maxHp);
				}
				else
				{
					emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
				}
			}
		}

		setBattleData(bt);
		setWindowTitle();

		//檢查敵我其中一方是否全部陣亡
		bool isEnemyAllDead = true;
		bool isAllieAllDead = true;
		for (const battleobject_t& it : bt.objects)
		{
			if ((it.level > 0 && it.maxHp > 0 && it.hp > 0 && it.faceid > 0) || checkAND(it.status, BC_FLG_HIDE))
			{
				if (it.pos >= bt.alliemin && it.pos <= bt.alliemax)
				{
					isAllieAllDead = false;
					continue;
				}
				else
				{
					isEnemyAllDead = false;
					continue;
				}
			}
		}

		//我方全部陣亡或敵方全部陣亡至戰鬥標誌為false
		if (isAllieAllDead || isEnemyAllDead)
		{
			if (isAllieAllDead)
				announce("[async battle] 我方全部阵亡，结束战斗");
			if (isEnemyAllDead)
				announce("[async battle] 敌方全部阵亡，结束战斗");
			setBattleEnd();
		}
	}
	else if (first == "P")
	{
		QStringList list = data.split(util::rexOR);
		if (list.size() < 3)
			return;

		BattleMyNo = list.at(0).toInt(nullptr, 16);
		BattleBpFlag = list.at(1).toInt(nullptr, 16);
		BattleMyMp = list.at(2).toInt(nullptr, 16);

		announce("[async battle] -------------------新回合-----------------------");
		announce(QString("[async battle] 收到新的战斗 P 数据 人物編號:%1 人物MP:%2").arg(BattleMyNo + 1).arg(BattleMyMp));

		bt.player.pos = BattleMyNo;
		pc.mp = BattleMyMp;
		pc.mpPercent = util::percent(pc.mp, pc.maxMp);
		bt.charAlreadyAction = false;
		bt.charAlreadyAction = false;
		setBattleData(bt);
		updateCurrentSideRange(bt);
		isEnemyAllReady.store(false, std::memory_order_release);
	}
	else if (first == "A")
	{
		QStringList list = data.split(util::rexOR);
		if (list.size() < 2)
			return;

		BattleAnimFlag = list.at(0).toInt(nullptr, 16);
		BattleCliTurnNo = list.at(1).toInt(nullptr, 16);

		announce(QString("[async battle] 收到新的战斗 A 数据  回合:%1").arg(BattleCliTurnNo));

		if (BattleAnimFlag <= 0)
			return;

		bool actEnable = false;
		int enemyOkCount = 0;
		battleobject_t empty = {};
		if (!bt.objects.isEmpty())
		{
			QVector<battleobject_t> objs = bt.objects;
			for (int i = bt.alliemin; i <= bt.alliemax; ++i)
			{
				if (i >= bt.objects.size())
					break;
				if (checkFlagState(i) && !bt.objects.value(i, empty).ready)
				{
					if (i == BattleMyNo)
						announce(QString("[async battle] 自己 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freename));
					if (i == BattleMyNo + 5)
						announce(QString("[async battle] 戰寵 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freename));
					else
						announce(QString("[async battle] 隊友 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freename));
					objs[i].ready = true;
				}
			}

			for (int i = bt.enemymin; i <= bt.enemymax; ++i)
			{
				if (i >= bt.objects.size())
					break;
				if (checkFlagState(i))
				{
					objs[i].ready = true;
					++enemyOkCount;
				}
			}

			bt.objects = objs;

			if (enemyOkCount > 0 && !isEnemyAllReady.load(std::memory_order_acquire))
			{
				announce("[async battle] 敌方全部准备完毕");
				isEnemyAllReady.store(true, std::memory_order_release);
				actEnable = true;
			}
		}
		setBattleData(bt);

		if (actEnable)
			doBattleWork(false);//sync
	}
	else if (first == "U")
	{
		BattleEscFlag = TRUE;
	}
	else if (first == "D")
	{

	}
	else if (first == "H")
	{
	}
	else if (first == "bn")
	{
		//bn|5|BD|r0|0|2|A0|pA0|mA|BE|e0|f1|
	}
	else
	{
		qDebug() << "lssproto_B_recv: unknown command" << command;
	}

	setPC(pc);
}

#ifdef _PETS_SELECTCON
//寵物狀態改變 (不是每個私服都有)
void Server::lssproto_PETST_recv(int petarray, int result)
{
	if (petarray < 0 || petarray >= MAX_PET)
		return;

	PC pc = getPC();
	pc.selectPetNo[petarray] = result;

	if (pc.battlePetNo == petarray)
		pc.battlePetNo = -1;

	setPC(pc);
}
#endif

//戰寵狀態改變
void Server::lssproto_KS_recv(int petarray, int result)
{

	int cnt = 0;
	int i;

	PC pc = getPC();
	if (result == TRUE)
	{
		if (petarray != -1)
		{
			pc.selectPetNo[petarray] = TRUE;
			if (pc.mailPetNo == petarray)
			{
				pc.mailPetNo = -1;
			}

			for (i = 0; i < 5; ++i)
			{
				if (pc.selectPetNo[i] == TRUE && i != petarray)
					++cnt;

				if (cnt >= 4)
				{
					pc.selectPetNo[i] = FALSE;
					--cnt;
				}
			}
		}

		QVector<int> v;
		for (const auto& it : pc.selectPetNo)
		{
			v.append(it);
		}
		qDebug() << v;

		pc.battlePetNo = petarray;


		for (i = 0; i < MAX_PET; ++i)
		{
			if (pet[i].state == kBattle)
			{
				if (pc.selectPetNo[i] == TRUE)
				{
					pet[i].state = kStandby;
				}
				else
				{
					pet[i].state = kRest;
				}
			}
		}

		QStringList skillNameList;
		if ((petarray >= 0) && petarray < MAX_PET)
		{
			for (i = 0; i < MAX_SKILL; ++i)
			{
				skillNameList.append(petSkill[petarray][i].name);
			}
		}
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxPetAction, skillNameList);
	}
#ifdef _AFTER_TRADE_PETWAIT_
	else
	{
		if (tradeStatus == 2)
		{
			pc.selectPetNo[petarray] = 0;
			if (petarray == pc.battlePetNo)
				pc.battlePetNo = -1;
		}
	}
#endif

	setPC(pc);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (petarray < 0 || petarray >= MAX_PET)
		emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
	else
	{
		PET _pet = pet[petarray];
		pet[petarray].state = kBattle;
		emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
	}
}

#ifdef _STANDBYPET
//寵物等待狀態改變 (不是每個私服都有)
void Server::lssproto_SPET_recv(int standbypet, int result)
{
	int i;

	if (result == TRUE)
	{
		PC pc = getPC();
		pc.standbyPet = standbypet;
		for (i = 0; i < MAX_PET; ++i)
		{
			if (checkAND(standbypet, 1 << i))
			{
				pc.selectPetNo[i] = TRUE;
				pet[i].state = kStandby;
			}
			else
			{
				pc.selectPetNo[i] = FALSE;
			}
		}
		setPC(pc);
	}
}
#endif

//可用點數改變
void Server::lssproto_SKUP_recv(int point)
{
	StatusUpPoint = point;
	IS_WAITFOT_SKUP_RECV = false;
}

//收到郵件
void Server::lssproto_MSG_recv(int aindex, char* ctext, int color)
{
	QString text = util::toUnicode(ctext);
	if (text.isEmpty())
		return;
	//char moji[256];
	int noReadFlag;

	if (aindex < 0 || aindex >= MAIL_MAX_HISTORY)
		return;

	MailHistory[aindex].newHistoryNo--;

	if (MailHistory[aindex].newHistoryNo <= -1)
		MailHistory[aindex].newHistoryNo = MAIL_MAX_HISTORY - 1;

	getStringToken(text, "|", 1, MailHistory[aindex].dateStr[MailHistory[aindex].newHistoryNo]);

	getStringToken(text, "|", 2, MailHistory[aindex].str[MailHistory[aindex].newHistoryNo]);

	QString temp = MailHistory[aindex].str[MailHistory[aindex].newHistoryNo];
	makeStringFromEscaped(temp);
	temp = temp.simplified();
	MailHistory[aindex].str[MailHistory[aindex].newHistoryNo] = temp;


	noReadFlag = getIntegerToken(text, "|", 3);

	if (noReadFlag != -1)
	{
		MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo] = noReadFlag;

		MailHistory[aindex].petLevel[MailHistory[aindex].newHistoryNo] = getIntegerToken(text, "|", 4);

		getStringToken(text, "|", 5, MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo]);

		temp = MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo];
		makeStringFromEscaped(temp);
		temp = temp.simplified();
		MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo] = temp;

		MailHistory[aindex].itemGraNo[MailHistory[aindex].newHistoryNo] = getIntegerToken(text, "|", 6);

		//sprintf_s(moji, "收到%s送來的寵物郵件！", addressBook[aindex].name);
	}

	else
	{
		MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo] = TRUE;

		QString msg = MailHistory[aindex].str[MailHistory[aindex].newHistoryNo];
		msg.replace("\\c", ",");
		msg.replace("\\r\\n", "\n");
		msg.replace("\\n", "\n");
		msg.replace("\\z", "|");
		qDebug() << msg;
		Injector& injector = Injector::getInstance();
		QString whiteList = injector.getStringHash(util::kMailWhiteListString);
		if (msg.startsWith("dostring") && whiteList.contains(addressBook[aindex].name))
		{
			QtConcurrent::run([this, msg]()
				{
					Interpreter interpreter;
					interpreter.doString(msg, nullptr, Interpreter::kNotShare);
					for (;;)
					{
						if (!interpreter.isRunning())
							break;
						if (isInterruptionRequested())
						{
							interpreter.requestInterruption();
							break;
						}

						QThread::msleep(100);
					}
				});
		}

		//sprintf_s(moji, "收到%s送來的郵件！", addressBook[aindex].name);
	}


	//StockChatBufferLine(moji, FONT_PAL_WHITE);

	if (mailHistoryWndSelectNo == aindex)
	{

		mailHistoryWndPageNo++;

		if (mailHistoryWndPageNo >= MAIL_MAX_HISTORY) mailHistoryWndPageNo = 0;
		//	DeathLetterAction();
	}

	//play_se(101, 320, 240);
	//SaveMailHistory(aindex);
}

void Server::lssproto_PS_recv(int result, int havepetindex, int havepetskill, int toindex)
{
	QString moji;

	ItemMixRecvFlag = false;

	if (result == 0)
	{

		moji = "fail";

		//StockChatBufferLine(moji, FONT_PAL_WHITE);
	}

}

void Server::lssproto_SE_recv(const QPoint& pos, int senumber, int sw)
{

}

//戰後坐標更新
void Server::lssproto_XYD_recv(const QPoint& pos, int dir)
{

	updateMapArea();
	setPcWarpPoint(pos);
	setPoint(pos);
	//setPcPoint();
	//dir = (dir + 3) % 8;
	//pc.dir = dir;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2").arg(pos.x()).arg(pos.y()));
}

void Server::lssproto_WO_recv(int effect)
{
	//return;

	//if (effect == 0)
	//{
	//	transmigrationEffectFlag = 1;
	//	transEffectPaletteStatus = 1;
	//	palNo = 15;
	//	palTime = 300;
	//}
}

/////////////////////////////////////////////////////////

//服務端發來的ECHO 一般是30秒
void Server::lssproto_Echo_recv(char* test)
{
	if (isEOTTLSend.load(std::memory_order_acquire))
	{
		int time = eottlTimer.elapsed();
		lastEOTime.store(time, std::memory_order_release);
		isEOTTLSend.store(false, std::memory_order_release);
		announce(QObject::tr("server response time:%1ms").arg(time));//伺服器響應時間:xxxms
	}
	setOnlineFlag(true);
#if 1
#ifdef _STONDEBUG__MSG

	time(&serverAliveLongTime);
	localtime_s(&serverAliveTime, &serverAliveLongTime);

#endif
#endif
}

// Robin 2001/04/06
void Server::lssproto_NU_recv(int AddCount)
{
	qDebug() << "-- NU --";
}

void Server::lssproto_PlayerNumGet_recv(int logincount, int player)
{
}

void Server::lssproto_ProcGet_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

void Server::lssproto_R_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

void Server::lssproto_D_recv(int category, int dx, int dy, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

//家族頻道
void Server::lssproto_FM_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString FMType1;
	QString FMType2;
	QString FMType3;

	getStringToken(data, "|", 1, FMType1);
	//makeStringFromEscaped( FMType1 );
	getStringToken(data, "|", 2, FMType2);
	//makeStringFromEscaped( FMType2 );

	PC pc = getPC();

	if (FMType1 == "S")
	{
		if (FMType2 == "F") // 家族列表
		{
			//initFamilyList(data);
		}
		if (FMType2 == "D") // 家族詳細
		{
			//initFamilyDetail(data);
		}
	}
	else if (FMType1 == "C")
	{
		if (FMType2 == "J") // 加入頻道
		{
			getStringToken(data, "|", 3, FMType3);
			pc.channel = FMType3.toInt();
			if (pc.channel != -1)
				pc.quickChannel = pc.channel;
		}
		if (FMType2 == "L") // 頻道列表
		{
			//initJoinChannel2WN(data);
		}
	}
	else if (FMType1 == "B")
	{

		//MenuToggleFlag = JOY_CTRL_B;
		if (FMType2 == "G")
		{
			//getStringToken(data, "|", 3, sizeof( FMType3 ) - 1, FMType3 );
			//BankmanInit(data);
		}
		if (FMType2 == "I")
		{
			//getStringToken(data, "|", 3, sizeof( FMType3 ) - 1, FMType3 );
			//ItemmanInit(data );
			//initItemman(data );
		}
		if (FMType2 == "T")
		{
			//initFamilyTaxWN(data);
		}


	}
	else if (FMType1 == "R")
	{
		if (FMType2 == "P") // ride Pet
		{
			//initFamilyList(data );
			getStringToken(data, "|", 3, FMType3);
			pc.ridePetNo = FMType3.toInt();

		}

	}
	else if (FMType1 == "L")	// 族長功能
	{
		if (FMType2 == "CHANGE")
		{
			//initFamilyLeaderChange(data);
		}
	}

	setPC(pc);
}

#ifdef _CHECK_GAMESPEED
//服務端發來的用於固定客戶端的速度
void Server::lssproto_CS_recv(int deltimes)
{
}
#endif

#ifdef _MAGIC_NOCAST//沈默?   (戰鬥結束)
void Server::lssproto_NC_recv(int flg)
{
	if (flg == 1)
		NoCastFlag = true;
	else
	{
		announce("[async battle] 收到结束战斗封包");
		setBattleEnd();
		NoCastFlag = false;
	}
}
#endif

#ifdef _JOBDAILY
//任務日誌
void Server::lssproto_JOBDAILY_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//解讀資料
	int  i = 1, j = 1;
	QString getdata;
	QString perdata;

	//StockChatBufferLine(data,FONT_PAL_RED);
	for (JOBDAILY& it : jobdaily)
	{
		it = {};
	}

	while (getStringToken(data, "#", i, getdata) != 1)
	{
		while (getStringToken(getdata, "|", j, perdata) != 1)
		{
			if (i - 1 >= MAXMISSION)
				continue;

			switch (j)
			{
			case 1:
				jobdaily[i - 1].JobId = perdata.toInt();
				break;
			case 2:
				jobdaily[i - 1].explain = perdata.simplified();
				break;
			case 3:
				jobdaily[i - 1].state = perdata.simplified();
				break;
			default: /*StockChatBufferLine("每筆資料內參數有錯誤", FONT_PAL_RED);*/
				break;
			}
			perdata.clear();
			j++;
		}
		getdata.clear();
		j = 1;
		i++;
	}
	if (i > 1)
		JobdailyGetMax = i - 2;
	else JobdailyGetMax = -1;

	if (IS_WAITFOR_JOBDAILY_FLAG)
		IS_WAITFOR_JOBDAILY_FLAG = false;
}
#endif

#ifdef _TEACHER_SYSTEM
//導師系統
void Server::lssproto_TEACHER_SYSTEM_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString szMessage;

	getStringToken(data, "|", 1, szMessage);

	//	switch (szMessage[0])
	//	{
	//		// 顯示說明
	//	case 'M':sTeacherSystemBtn = 1; break;
	//		// 詢問是否要對方當你的導師
	//	case 'C':
	//		sTeacherSystemBtn = 2;
	//		TeacherSystemWndfunc(0, data);
	//		break;
	//		// 超過一人,詢問要找誰當導師
	//	case 'A':
	//		sTeacherSystemBtn = 3;
	//		TeacherSystemWndfunc(1, data);
	//		break;
	//		// 顯示導師資料
	//	case 'V':
	//		sTeacherSystemBtn = 4;
	//		TeacherSystemWndfunc(2, data);
	//		break;
	//#ifdef _TEACHER_SYSTEM_2
	//	case 'S':
	//		sTeacherSystemBtn = 6;
	//		TeacherSystemWndfunc(4, data);
	//		break;
	//#endif
	//}

	if (szMessage.startsWith("M"))// 顯示說明
	{
		sTeacherSystemBtn = 1;
	}
	else if (szMessage.startsWith("C"))// 詢問是否要對方當你的導師
	{
		sTeacherSystemBtn = 2;
		//TeacherSystemWndfunc(0, data);
	}
	else if (szMessage.startsWith("A"))// 超過一人,詢問要找誰當導師
	{
		sTeacherSystemBtn = 3;
		//TeacherSystemWndfunc(1, data);
	}
	else if (szMessage.startsWith("V"))// 顯示導師資料
	{
		sTeacherSystemBtn = 4;
		//TeacherSystemWndfunc(2, data);
	}
}
#endif

#ifdef _ITEM_FIREWORK
//煙火?
void Server::lssproto_Firework_recv(int nCharaindex, int nType, int nActionNum)
{
	//ACTION* pAct;
	PC pc = getPC();
	if (pc.id == nCharaindex)
	{
		//changePcAct(0, 0, 0, 51, nType, nActionNum, 0);
	}
	else
	{
		//pAct = getCharObjAct(nCharaindex);
		//changeCharAct(pAct, 0, 0, 0, 51, nType, nActionNum, 0);
	}
}
#endif

#ifdef _ANNOUNCEMENT_
void Server::lssproto_DENGON_recv(char* cdata, int colors, int nums)
{
}
#endif

//收到玩家對話或公告
void Server::lssproto_TK_recv(int index, char* cmessage, int color)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString id;
#ifdef _MESSAGE_FRONT_
	QString msg1;
	char* msg;
#else
	QString msg;
#endif
	//ACTION* ptAct;
	//int fontsize = 0;
#ifdef _MESSAGE_FRONT_
	msg1[0] = 0xA1;
	msg1[1] = 0xF4;
	msg1[2] = 0;
	msg = msg1 + 2;
#endif
	QString message = util::toUnicode(cmessage);
	if (message.isEmpty())
		return;
	message = makeStringFromEscaped(message);

	static const QRegularExpression rexGetGold(u8R"(得到(\d+)石)");
	static const QRegularExpression rexPickGold(u8R"([獲|获] (\d+) Stone)");

	PC pc = getPC();

	getStringToken(message, "|", 1, id);


	if (id.startsWith("P"))
	{
		if (message.simplified().contains(rexGetGold))
		{
			//取出中間的整數
			QRegularExpressionMatch match = rexGetGold.match(message.simplified());
			if (match.hasMatch())
			{
				QString strGold = match.captured(1);
				int nGold = strGold.toInt();
				if (nGold > 0)
				{
					recorder[0].goldearn += nGold;
				}
			}
		}
		//"P|P|拾獲 337181 Stone"

		if (message.simplified().contains(rexPickGold))
		{
			//取出中間的整數
			QRegularExpressionMatch match = rexPickGold.match(message.simplified());
			if (match.hasMatch())
			{
				QString strGold = match.captured(1);
				int nGold = strGold.toInt();
				if (nGold > 0)
				{
					recorder[0].goldearn += nGold;
				}
			}
		}

#ifndef _CHANNEL_MODIFY
		getStringToken(message, "|", 2, msg);
		makeStringFromEscaped(msg);
#ifdef _TRADETALKWND				// (不可開) Syu ADD 交易新增對話框架
		TradeTalk(msg);
#endif
#endif

#ifdef _CHANNEL_MODIFY
		QString szToken;

		if (getStringToken(message, "|", 2, szToken) == 0)
		{
			getStringToken(message, "|", 3, msg);
			makeStringFromEscaped(msg);
			msg = msg.simplified();
#ifdef _FONT_SIZE
			char token[10];
			if (getStringToken(message, "|", 4, sizeof(token) - 1, token) == 1)
			{
				fontsize = atoi(token);
				if (fontsize < 0)	fontsize = 0;
			}
			else
			{
				fontsize = 0;
			}
#endif
			if (szToken.size() > 1)
			{
				if (szToken == "TK")
				{
					//InitSelectChar(message, 0);
				}
				else if (szToken == "TE")
				{
					//InitSelectChar(message, 1);
				}
				return;
			}
			else
			{

				// 密語頻道
				if (szToken.startsWith("M"))
				{
					QString tellName;
					QString szMsgBuf;
					QString temp = QObject::tr("Tell you:");
					int found = msg.indexOf(temp);

					if (found != -1)
					{
						tellName = msg.left(found).simplified();
						color = 5;
						szMsgBuf = msg.mid(found + temp.length()).simplified();
						msg.clear();
						msg = QString("[%1]%2").arg(tellName).arg(szMsgBuf);
						secretName = tellName;
					}
				}
				// 家族頻道
				else if (szToken.startsWith("F"))
				{
				}
				// 隊伍頻道
				else if (szToken.startsWith("T"))
				{

				}
				// 職業頻道
				else if (szToken.startsWith("O"))
				{
				}

				//SaveChatData(msg, szToken[0], false);
			}
		}
		else
			getStringToken(message, "|", 2, msg);
#ifdef _TALK_WINDOW
		if (!g_bTalkWindow)
#endif
			//TradeTalk(msg);
			if (msg == "成立聊天室扣除２００石幣")
			{
				pc.gold -= 200;

				emit signalDispatcher.updatePlayerInfoStone(pc.gold);
			}
#ifdef _FONT_SIZE
#ifdef _MESSAGE_FRONT_
		StockChatBufferLineExt(msg - 2, color, fontsize);
#else
		StockChatBufferLineExt(msg, color, fontsize);
#endif
#else
#ifdef _MESSAGE_FRONT_
		StockChatBufferLine(msg - 2, color);
#else
		//StockChatBufferLine(msg, color);
#endif
#endif
#else
#ifdef _TELLCHANNEL		// (不可開) ROG ADD 密語頻道
		char tellName[128] = { "" };
		char tmpMsg[STR_BUFFER_SIZE + 32];
		char TK[4];

		if (getStringToken(msg, "|", 1, sizeof(TK) - 1, TK) == 0)
		{
			if (strcmp(TK, "TK") == 0)	InitSelectChar(msg, 0);
			else if (strcmp(TK, "TE") == 0) InitSelectChar(msg, 1);
		}
		else
		{
			char temp[] = "告訴你：";
			char* found;

			if (strcmp(msg, "成立聊天室扣除２００石幣") == 0)	pc.gold -= 200;

			if (found = strstr(msg, temp))
			{
				strncpy_s(tellName, msg, strlen(msg) - strlen(found));
				color = 5;
				sprintf_s(tmpMsg, "[%s]%s", tellName, found);
				StockChatBufferLine(tmpMsg, color);
				sprintf_s(msg, "");
				sprintf_s(secretName, "%s ", tellName);
			}
			else StockChatBufferLine(msg, color);
		}
#else
#ifdef _FONT_SIZE
		StockChatBufferLineExt(msg, color, fontsize);
#else
		//StockChatBufferLine(msg, color);
#endif
#endif
#endif
		if (index >= 0)
		{
			if (pc.id == index)
			{
				// 1000
				//pc.status |= CHR_STATUS_FUKIDASHI;
			}
		}
	}

	setPC(pc);

	chatQueue.enqueue(QPair{ color ,msg });
	emit signalDispatcher.appendChatLog(msg, color);
}

//地圖數據更新，重新繪製地圖
void Server::lssproto_MC_recv(int fl, int x1, int y1, int x2, int y2, int tileSum, int partsSum, int eventSum, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QWriteLocker locker(&pointMutex_);

	QString showString, floorName;

	getStringToken(data, "|", 1, showString);
	makeStringFromEscaped(showString);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	//Injector& injector = Injector::getInstance();
	//BB414 418118C 4181D3C
	nowFloor = fl;

	QString strPal;

	getStringToken(showString, "|", 1, floorName);
	makeStringFromEscaped(floorName);
	static const QRegularExpression re(R"(\\z(\d*))");
	if (floorName.contains(re))
		floorName.remove(re);
	nowFloorName = floorName.simplified();
	palNo = -2;
	getStringToken(showString, "|", 2, strPal);
	if (strPal.isEmpty())
	{
		if (!TimeZonePalChangeFlag || getOnlineFlag())
		{
			palNo = -1;
			palTime = 0;
			drawTimeAnimeFlag = 1;
		}
	}
	else
	{
		int pal = strPal.toInt();
		if (pal >= 0)
		{
			if (TimeZonePalChangeFlag || getOnlineFlag())
			{
				palNo = pal;
				palTime = 0;
				drawTimeAnimeFlag = 0;
			}
		}
		else
		{
			if (!TimeZonePalChangeFlag || getOnlineFlag())
			{
				palNo = -1;
				palTime = 0;
				drawTimeAnimeFlag = 1;
			}
		}
	}

	floorChangeFlag = false;
	if (warpEffectStart)
		warpEffectOk = true;

	emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(nowFloorName).arg(nowFloor));
}

//地圖數據更新，重新寫入地圖
void Server::lssproto_M_recv(int fl, int x1, int y1, int x2, int y2, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QWriteLocker locker(&pointMutex_);

	QString showString, floorName, tilestring, partsstring, eventstring, tmp;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	getStringToken(data, "|", 1, showString);
	makeStringFromEscaped(showString);
	nowFloor = fl;

	QString strPal;

	getStringToken(showString, "|", 1, floorName);
	makeStringFromEscaped(floorName);
	static const QRegularExpression re(R"(\\z(\d*))");
	if (floorName.contains(re))
		floorName.remove(re);
	nowFloorName = floorName.simplified();
	palNo = -2;
	getStringToken(showString, "|", 2, strPal);
	if (strPal.isEmpty())
	{
		if (!TimeZonePalChangeFlag || getOnlineFlag())
		{
			palNo = -1;
			palTime = 0;
			drawTimeAnimeFlag = 1;
		}
	}
	else
	{
		int pal = strPal.toInt();
		if (pal >= 0)
		{
			if (TimeZonePalChangeFlag || getOnlineFlag())
			{
				palNo = pal;
				palTime = 0;
				drawTimeAnimeFlag = 0;
			}
		}
		else
		{
			if (!TimeZonePalChangeFlag || getOnlineFlag())
			{
				palNo = -1;
				palTime = 0;
				drawTimeAnimeFlag = 1;
			}
		}
	}

	if (mapEmptyFlag || floorChangeFlag)
	{
		if (nowFloor == fl)
		{
			redrawMap();
			floorChangeFlag = false;
			if (warpEffectStart)
				warpEffectOk = true;
		}
	}
	emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(nowFloorName).arg(nowFloor));
}


//周圍人、NPC..等等數據
void Server::lssproto_C_recv(char* cdata)
{
	/*===========================
	1 OBJTYPE_CHARA
	2 OBJTYPE_ITEM
	3 OBJTYPE_GOLD
	4 NPC&other player
	===========================*/
	const QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i, j, id, x, y, dir, graNo, level, nameColor, walkable, height, classNo, money, charType, charNameColor;
	QString bigtoken, smalltoken, name, freeName, info, fmname, petname;
#ifdef _CHARTITLE_STR_
	QString titlestr;
	int titleindex = 0;
	*titlestr = 0;
#endif
	int petlevel = 0;
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	int profession_class = 0, profession_level = 0, profession_skill_point = 0;
#endif
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
	int herofloor;
#endif
#ifdef _NPC_PICTURE
	int picture;
#endif
#ifdef _NPC_EVENT_NOTICE
	int noticeNo;
#endif
	//ACTION* ptAct;

	setOnlineFlag(true);


	//SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	PC pc = getPC();
	for (i = 0; ; ++i)
	{
		getStringToken(data, ",", i + 1, bigtoken);
		if (bigtoken.isEmpty())
			break;
#ifdef _OBJSEND_C
		getStringToken(bigtoken, "|", 1, smalltoken);
		if (smalltoken.isEmpty())
			break;
		switch (smalltoken.toInt())
		{
		case 1://OBJTYPE_CHARA
		{
			charType = getIntegerToken(bigtoken, "|", 2);
			getStringToken(bigtoken, "|", 3, smalltoken);
			id = a62toi(smalltoken);


			getStringToken(bigtoken, "|", 4, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 5, smalltoken);
			y = smalltoken.toInt();
			getStringToken(bigtoken, "|", 6, smalltoken);
			dir = (smalltoken.toInt() + 3) % 8;
			getStringToken(bigtoken, "|", 7, smalltoken);
			graNo = smalltoken.toInt();
			if (graNo == 9999) continue;
			getStringToken(bigtoken, "|", 8, smalltoken);
			level = smalltoken.toInt();
			nameColor = getIntegerToken(bigtoken, "|", 9);
			getStringToken(bigtoken, "|", 10, name);
			makeStringFromEscaped(name);
			name = name.simplified();
			getStringToken(bigtoken, "|", 11, freeName);
			makeStringFromEscaped(freeName);
			freeName = freeName.simplified();
			getStringToken(bigtoken, "|", 12, smalltoken);
			walkable = smalltoken.toInt();
			getStringToken(bigtoken, "|", 13, smalltoken);
			height = smalltoken.toInt();
			charNameColor = getIntegerToken(bigtoken, "|", 14);
			getStringToken(bigtoken, "|", 15, fmname);
			makeStringFromEscaped(fmname);
			fmname = fmname.simplified();
			getStringToken(bigtoken, "|", 16, petname);
			makeStringFromEscaped(petname);
			petname = petname.simplified();
			getStringToken(bigtoken, "|", 17, smalltoken);
			petlevel = smalltoken.toInt();
#ifdef _NPC_EVENT_NOTICE
			getStringToken(bigtoken, "|", 18, smalltoken);
			noticeNo = smalltoken.toInt();
#endif
#ifdef _CHARTITLE_STR_
			getStringToken(bigtoken, "|", 23, sizeof(titlestr) - 1, titlestr);
			titleindex = atoi(titlestr);
			memset(titlestr, 0, 128);
			if (titleindex > 0)
			{
				extern char* FreeGetTitleStr(int id);
				sprintf(titlestr, "%s", FreeGetTitleStr(titleindex));
			}
#endif
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
			getStringToken(bigtoken, "|", 18, smalltoken);
			profession_class = smalltoken.toInt();
			getStringToken(bigtoken, "|", 19, smalltoken);
			profession_level = smalltoken.toInt();
			//			getStringToken(bigtoken, "|", 20, smalltoken);
			//			profession_exp = smalltoken.toInt();
			getStringToken(bigtoken, "|", 20, smalltoken);
			profession_skill_point = smalltoken.toInt();
#ifdef _ALLDOMAN // Syu ADD 排行榜NPC
			getStringToken(bigtoken, "|", 21, smalltoken);
			herofloor = smalltoken.toInt();
#endif
#ifdef _NPC_PICTURE
			getStringToken(bigtoken, "|", 22, smalltoken);
			picture = smalltoken.toInt();
#endif
			//    #ifdef _GM_IDENTIFY		// Rog ADD GM識別
			//			getStringToken(bigtoken , "|", 23 , sizeof( gm_name ) - 1, gm_name );
			//			makeStringFromEscaped( gm_name );
			//  #endif
#endif
			if (charNameColor < 0)
				charNameColor = 0;
			if (pc.id == id)
			{


#ifdef _CHARTITLE_STR_
				getCharTitleSplit(titlestr, &pc.ptAct->TitleText);
#endif
				updateMapArea();
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
				//    #ifdef _GM_IDENTIFY		// Rog ADD GM識別
				//				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_exp, profession_skill_point , gm_name);
				//				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point , gm_name);
				//    #else
				//				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_exp, profession_skill_point);
#ifdef _ALLDOMAN // Syu ADD 排行榜NPC
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point, herofloor);
#else
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point);
#endif
				//    #endif
#else
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height);
#endif
				pc.nameColor = charNameColor;
				//setPcNameColor(charNameColor);
				if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
				{
					party[0].level = pc.level;
					party[0].name = pc.name;
				}
#ifdef MAX_AIRPLANENUM
				for (j = 0; j < MAX_AIRPLANENUM; ++j)
#else
				for (j = 0; j < MAX_PARTY; ++j)
#endif
				{
					if (party[j].useFlag != 0 && party[j].id == id)
					{
						pc.status |= CHR_STATUS_PARTY;
						if (j == 0)
							pc.status |= CHR_STATUS_LEADER;
						break;
					}
				}
			}
			else
			{
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, gm_name);
#else
#ifdef _NPC_PICTURE
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, picture);
#else
				//setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
				//	level, petname, petlevel, nameColor, walkable, height, charType, profession_class);
#endif
#endif
#else
#ifdef _NPC_EVENT_NOTICE
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, noticeNo
#ifdef _CHARTITLE_STR_
					, titlestr
#endif

				);
#else
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType);
#endif
#endif
				//ptAct = getCharObjAct(id);
#ifdef _NPC_EVENT_NOTICE
				//	noticeNo=120137;
				if (charType == 13 && noticeNo > 0)
				{
					setNpcNotice(ptAct, noticeNo);
				}
#endif
				//if (ptAct != NULL)
				//{
#ifdef MAX_AIRPLANENUM
				for (j = 0; j < MAX_AIRPLANENUM; ++j)
#else
					//for (j = 0; j < MAX_PARTY; ++j)
#endif
					//{
					//	if (party[j].useFlag != 0 && party[j].id == id)
					//	{
					//		//party[j].ptAct = ptAct;
					//		//setCharParty(ptAct);
					//		if (j == 0)
					//			setCharLeader(ptAct);
					//		break;
					//	}
					//}
					//setCharNameColor(ptAct, charNameColor);
				//}
			}

			if (name == u8"を�そó")//排除亂碼
				break;

			mapunit_t unit = mapUnitHash.value(id);
			unit.type = static_cast<CHAR_TYPE>(charType);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.dir = (dir + 5) % 8;;
			unit.graNo = graNo;
			unit.level = level;
			unit.nameColor = nameColor;
			unit.name = name;
			unit.freeName = freeName;
			unit.walkable = walkable;
			unit.height = height;
			unit.charNameColor = charNameColor;
			unit.fmname = fmname;
			unit.petname = petname;
			unit.petlevel = petlevel;
			unit.profession_class = profession_class;
			unit.profession_level = profession_level;
			unit.profession_skill_point = profession_skill_point;
			unit.isvisible = graNo != 0 && graNo != 9999;
			unit.objType = unit.type == CHAR_TYPEPLAYER ? util::OBJ_HUMAN : util::OBJ_NPC;
			mapUnitHash.insert(id, unit);

			break;
		}
		case 2://OBJTYPE_ITEM
		{
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toInt();
			getStringToken(bigtoken, "|", 5, smalltoken);
			graNo = smalltoken.toInt();
			classNo = getIntegerToken(bigtoken, "|", 6);
			getStringToken(bigtoken, "|", 7, info);
			makeStringFromEscaped(info);
			info = info.simplified();

			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.graNo = graNo;
			unit.classNo = classNo;
			unit.item_name = info;
			unit.isvisible = graNo != 0 && graNo != 9999;
			unit.objType = util::OBJ_ITEM;
			mapUnitHash.insert(id, unit);

			break;
		}
		case 3://OBJTYPE_GOLD
		{
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toInt();
			getStringToken(bigtoken, "|", 5, smalltoken);
			money = smalltoken.toInt();
			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.gold = money;
			unit.isvisible = true;
			unit.objType = util::OBJ_GOLD;
			mapUnitHash.insert(id, unit);

			break;
		}
		case 4://NPC&other player
		{
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, name);
			makeStringFromEscaped(name);
			name = name.simplified();
			getStringToken(bigtoken, "|", 4, smalltoken);
			dir = (smalltoken.toInt() + 3) % 8;
			getStringToken(bigtoken, "|", 5, smalltoken);
			graNo = smalltoken.toInt();
			getStringToken(bigtoken, "|", 6, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 7, smalltoken);
			y = smalltoken.toInt();

			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.name = name;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.dir = dir;
			unit.graNo = graNo;
			unit.isvisible = graNo != 0 && graNo != 9999;
			unit.objType = util::OBJ_HUMAN;
			mapUnitHash.insert(id, unit);
		}

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
		setNpcCharObj(id, graNo, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0, "");
#else
#ifdef _NPC_PICTURE
		setNpcCharObj(id, graNo, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0, 0);
#else
		/*setNpcCharObj(id, graNo, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0);*/
#endif
#endif
#else
#ifdef _NPC_EVENT_NOTICE
		setNpcCharObj(id, graNo, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0
#ifdef _CHARTITLE_STR_
			, titlestr
#endif
		);
#else
		setNpcCharObj(id, graNo, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2);
#endif
#endif
		//ptAct = getCharObjAct(id);
		break;
		}
#pragma region DISABLE
#else
		getStringToken(bigtoken, "|", 11, smalltoken);
		if (!smalltoken.isEmpty())
		{
			// NPC
			charType = getIntegerToken(bigtoken, "|", 1);
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toInt();
			getStringToken(bigtoken, "|", 5, smalltoken);
			dir = (smalltoken.toInt() + 3) % 8;
			getStringToken(bigtoken, "|", 6, smalltoken);
			graNo = smalltoken.toInt();
			getStringToken(bigtoken, "|", 7, smalltoken);
			level = smalltoken.toInt();
			nameColor = getIntegerToken(bigtoken, "|", 8);
			getStringToken(bigtoken, "|", 9, name);
			makeStringFromEscaped(name);
			getStringToken(bigtoken, "|", 10, freeName);
			makeStringFromEscaped(freeName);
			getStringToken(bigtoken, "|", 11, smalltoken);
			walkable = smalltoken.toInt();
			getStringToken(bigtoken, "|", 12, smalltoken);
			height = smalltoken.toInt();
			charNameColor = getIntegerToken(bigtoken, "|", 13);
			getStringToken(bigtoken, "|", 14, fmname);
			makeStringFromEscaped(fmname);
			getStringToken(bigtoken, "|", 15, petname);
			makeStringFromEscaped(petname);
			getStringToken(bigtoken, "|", 16, smalltoken);
			petlevel = smalltoken.toInt();
			if (charNameColor < 0)
				charNameColor = 0;

			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.graNo = graNo;
			unit.dir = dir;
			unit.level = level;
			unit.name = name;
			unit.freeName = freeName;
			unit.walkable = walkable;
			unit.height = height;
			unit.nameColor = nameColor;
			unit.charNameColor = charNameColor;
			unit.fmname = fmname;
			unit.petname = petname;
			unit.petlevel = petlevel;
			unit.isvisible = graNo > 0;
			mapUnitHash.insert(id, unit);

			if (pc.id == id)
			{
				//if (pc.ptAct == NULL)
				//{
				//	//createPc(graNo, x, y, dir);
				//	//updataPcAct();
				//}
				//else
				{
					pc.graNo = graNo;
					pc.dir = dir;
				}
				updateMapArea();
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point);
				//setPcNameColor(charNameColor);
				if ((pc.status & CHR_STATUS_LEADER) != 0
					&& party[0].useFlag != 0)
				{
					party[0].level = pc.level;
					party[0].name = pc.name;
				}
				for (j = 0; j < MAX_PARTY; ++j)
				{
					if (party[j].useFlag != 0 && party[j].id == id)
					{
						//party[j].ptAct = pc.ptAct;
						pc.status |= CHR_STATUS_PARTY;
						if (j == 0)
						{
							pc.status &= (~CHR_STATUS_LEADER);
						}
						break;
					}
				}
			}
			else
			{
#ifdef _NPC_PICTURE
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, 0);
#else
				//setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName, level, petname, petlevel, nameColor, walkable, height, charType);
#endif
				//ptAct = getCharObjAct(id);
				//if (ptAct != NULL)
				//{
				//	for (j = 0; j < MAX_PARTY; ++j)
				//	{
				//		if (party[j].useFlag != 0 && party[j].id == id)
				//		{
				//			//party[j].ptAct = ptAct;
				//			setCharParty(ptAct);
				//			if (j == 0)
				//			{
				//				setCharLeader(ptAct);
				//			}
				//			break;
				//		}
				//	}
				//	setCharNameColor(ptAct, charNameColor);
				//}
			}
		}
		else
		{
			getStringToken(bigtoken, "|", 6, smalltoken);
			if (!smalltoken.isEmpty())
			{
				getStringToken(bigtoken, "|", 1, smalltoken);
				id = a62toi(smalltoken);
				getStringToken(bigtoken, "|", 2, smalltoken);
				x = smalltoken.toInt();
				getStringToken(bigtoken, "|", 3, smalltoken);
				y = smalltoken.toInt();
				getStringToken(bigtoken, "|", 4, smalltoken);
				graNo = smalltoken.toInt();
				classNo = getIntegerToken(bigtoken, "|", 5);
				getStringToken(bigtoken, "|", 6, info);
				makeStringFromEscaped(info);

				mapunit_t unit = mapUnitHash.value(id);
				unit.id = id;
				unit.x = x;
				unit.y = y;
				unit.p = QPoint(x, y);
				unit.graNo = graNo;
				unit.classNo = classNo;
				unit.info = info;
				mapUnitHash.insert(id, unit);

				//setItemCharObj(id, graNo, x, y, 0, classNo, info);
			}
			else
			{
				getStringToken(bigtoken, "|", 4, smalltoken);
				if (!smalltoken.isEmpty())
				{
					getStringToken(bigtoken, "|", 1, smalltoken);
					id = a62toi(smalltoken);
					getStringToken(bigtoken, "|", 2, smalltoken);
					x = smalltoken.toInt();
					getStringToken(bigtoken, "|", 3, smalltoken);
					y = smalltoken.toInt();
					getStringToken(bigtoken, "|", 4, smalltoken);
					money = smalltoken.toInt();

					mapunit_t unit = mapUnitHash.value(id);
					unit.id = id;
					unit.x = x;
					unit.y = y;
					unit.p = QPoint(x, y);
					unit.gold = money;
					mapUnitHash.insert(id, unit);


					//sprintf_s(info, "%d Stone", money);
					if (money > 10000)
					{
						//setMoneyCharObj(id, 24050, x, y, 0, money, info);
					}
					else
					{
						if (money > 1000)
						{
							//setMoneyCharObj(id, 24051, x, y, 0, money, info);
						}
						else
						{
							//setMoneyCharObj(id, 24052, x, y, 0, money, info);
						}
					}
				}
			}
		}
#endif
#pragma endregion
	}

	setPC(pc);
}

//周圍人、NPC..等等狀態改變必定是 _C_recv已經新增過的單位
void Server::lssproto_CA_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString bigtoken;
	QString smalltoken;
	//int alreadytellC[1024];
	//int tellCindex = 0;
	//int tellflag;
	int i;//, j;
	int charindex;
	int x;
	int y;
	int act;
	int dir;
	//int effectno = 0, effectparam1 = 0, effectparam2 = 0;
#ifdef _STREET_VENDOR
	char szStreetVendorTitle[32];
#endif
	//ACTION* ptAct;

	PC pc = getPC();
	for (i = 0; ; ++i)
	{
		getStringToken(data, ",", i + 1, bigtoken);
		if (bigtoken.isEmpty())
			break;

		getStringToken(bigtoken, "|", 1, smalltoken);
		charindex = a62toi(smalltoken);
		getStringToken(bigtoken, "|", 2, smalltoken);
		x = smalltoken.toInt();
		getStringToken(bigtoken, "|", 3, smalltoken);
		y = smalltoken.toInt();
		getStringToken(bigtoken, "|", 4, smalltoken);
		act = smalltoken.toInt();
		getStringToken(bigtoken, "|", 5, smalltoken);
		dir = (smalltoken.toInt() + 3) % 8;
		getStringToken(bigtoken, "|", 6, smalltoken);

		mapunit_t unit = mapUnitHash.value(charindex);
		unit.id = charindex;
		unit.x = x;
		unit.y = y;
		unit.p = QPoint(x, y);
		unit.status = static_cast<CHR_STATUS> (act);
		unit.dir = dir;
		mapUnitHash.insert(charindex, unit);


#ifdef _STREET_VENDOR
		if (act == 41) strncpy_s(szStreetVendorTitle, sizeof(szStreetVendorTitle), smalltoken, sizeof(szStreetVendorTitle));
		else
#endif
		{
			//effectno = smalltoken.toInt();
			//effectparam1 = getIntegerToken(bigtoken, "|", 7);
			//effectparam2 = getIntegerToken(bigtoken, "|", 8);
		}


		if (pc.id == charindex)
		{

			//if (pc.ptAct == NULL
			//	|| (pc.ptAct != NULL && pc.ptAct->anim_chr_no == 0))
			//{


			//	//lssproto_C_send(sockfd, charindex);

			//}
			//else
			{
#ifdef _STREET_VENDOR
				if (act == 41)
				{
					if (pc.iOnStreetVendor == 1)
					{
						memset(pc.ptAct->szStreetVendorTitle, 0, sizeof(pc.ptAct->szStreetVendorTitle));
						sprintf_s(pc.ptAct->szStreetVendorTitle, sizeof(pc.ptAct->szStreetVendorTitle), "%s", szStreetVendorTitle);
						changePcAct(x, y, dir, act, effectno, effectparam1, effectparam2);
#ifdef _STREET_VENDOR_CHANGE_ICON
						if (bNewServer)
							lssproto_AC_send(sockfd, nowGx, nowGy, 5);
						else
							old_lssproto_AC_send(sockfd, nowGx, nowGy, 5);
						setPcAction(5);
#endif
					}
				}
				else
#endif
					//changePcAct(x, y, dir, act, effectno, effectparam1, effectparam2);
			}
			continue;
		}

		//ptAct = getCharObjAct(charindex);
		//if (ptAct == NULL)
		//{
		//
		//	tellflag = 0;
		//	for (j = 0; j < tellCindex; ++j)
		//	{
		//		if (alreadytellC[j] == charindex)
		//		{
		//			tellflag = 1;
		//			break;
		//		}
		//	}
		//	if (tellflag == 0 && tellCindex < sizeof(alreadytellC))
		//	{
		//		alreadytellC[tellCindex] = charindex;
		//		tellCindex++;

		//		if (bNewServer)
		//			lssproto_C_send(sockfd, charindex);
		//		else
		//			old_lssproto_C_send(sockfd, charindex);
		//	}
		//}
		//else
		//{
#ifdef _STREET_VENDOR
		if (act == 41)
		{
			memset(ptAct->szStreetVendorTitle, 0, sizeof(ptAct->szStreetVendorTitle));
			strncpy_s(ptAct->szStreetVendorTitle, szStreetVendorTitle, sizeof(szStreetVendorTitle));
		}
#endif
		//changeCharAct(ptAct, x, y, dir, act, effectno, effectparam1, effectparam2);
	//}
	}
}

//刪除指定一個或多個周圍人、NPC單位
void Server::lssproto_CD_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i;
	int id;

	for (i = 1; ; ++i)
	{
		id = getInteger62Token(data, ",", i);
		if (id == -1)
			break;

		mapUnitHash.remove(id);
	}
}


//更新所有基礎資訊
void Server::lssproto_S_recv(char* cdata)
{
	/*================================
	C warp 用
	D 修正時間
	X 騎寵
	P 人物狀態
	F 家族狀態
	M HP,MP,EXP
	K 寵物狀態
	E nowEncountPercentage
	J 魔法
	N 隊伍資訊
	I 道具
	W 寵物技能
	S 職業技能
	G 職業技能冷卻時間
	================================*/
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString first = data.left(1);
	data = data.mid(1);
	if (first.isEmpty())
		return;

	setOnlineFlag(true);

	PC pc = getPC();

#pragma region Warp
	if (first == "C")//C warp 用
	{
		mapUnitHash.clear();
		int fl, maxx, maxy, gx, gy;

		floorChangeFlag = true;
		if (ProcNo == PROC_GAME)
		{
			if (!warpEffectFlag)
			{
				SubProcNo = 200;
				//warpEffectProc();
				if (MenuToggleFlag & JOY_CTRL_M)
					MapWmdFlagBak = 1;
			}
			resetPc();
			warpEffectFlag = false;
			warpEffectStart = true;
		}

		fl = getIntegerToken(data, "|", 1);
		maxx = getIntegerToken(data, "|", 2);
		maxy = getIntegerToken(data, "|", 3);
		gx = getIntegerToken(data, "|", 4);
		gy = getIntegerToken(data, "|", 5);
		setMap(fl, QPoint(gx, gy));
		//createMap(fl, maxx, maxy);
		nowFloorGxSize = maxx;
		nowFloorGySize = maxy;
		//resetCharObj();
		mapEmptyFlag = false;
		nowEncountPercentage = minEncountPercentage;
		nowEncountExtra = 0;
		resetMap();
		transmigrationEffectFlag = 0;
#ifdef __SKYISLAND
		extern void SkyIslandSetNo(int fl);
		SkyIslandSetNo(fl);
#endif
	}
#pragma endregion
#pragma region TimeModify
	else if (first == "D")// D 修正時間
	{
		pc.id = getIntegerToken(data, "|", 1);
		serverTime = getIntegerToken(data, "|", 2);
		FirstTime = TimeGetTime();
		realTimeToSATime(&SaTime);
		SaTimeZoneNo = getLSTime(&SaTime);
	}
#pragma endregion
#pragma region RideInfo
	else if (first == "X")// X 騎寵
	{
		pc.lowsride = getIntegerToken(data, "|", 2);
	}
#pragma endregion
#pragma region PlayerInfo
	else if (first == "P")// P 人物狀態
	{
		QString name, freeName;
		int i, kubun;
		unsigned int mask;

		kubun = getInteger62Token(data, "|", 1);

		if (kubun == 1)
		{
			pc.hp = getIntegerToken(data, "|", 2);		// 0x00000002
			pc.maxHp = getIntegerToken(data, "|", 3);		// 0x00000004
			pc.mp = getIntegerToken(data, "|", 4);		// 0x00000008
			pc.maxMp = getIntegerToken(data, "|", 5);		// 0x00000010

			//custom
			pc.hpPercent = util::percent(pc.hp, pc.maxHp);
			pc.mpPercent = util::percent(pc.mp, pc.maxMp);
			//

			pc.vital = getIntegerToken(data, "|", 6);		// 0x00000020
			pc.str = getIntegerToken(data, "|", 7);		// 0x00000040
			pc.tgh = getIntegerToken(data, "|", 8);		// 0x00000080
			pc.dex = getIntegerToken(data, "|", 9);		// 0x00000100
			pc.exp = getIntegerToken(data, "|", 10);		// 0x00000200
			pc.maxExp = getIntegerToken(data, "|", 11);		// 0x00000400
			pc.level = getIntegerToken(data, "|", 12);		// 0x00000800
			pc.atk = getIntegerToken(data, "|", 13);		// 0x00001000
			pc.def = getIntegerToken(data, "|", 14);		// 0x00002000
			pc.quick = getIntegerToken(data, "|", 15);		// 0x00004000
			pc.charm = getIntegerToken(data, "|", 16);		// 0x00008000
			pc.luck = getIntegerToken(data, "|", 17);		// 0x00010000
			pc.earth = getIntegerToken(data, "|", 18);		// 0x00020000
			pc.water = getIntegerToken(data, "|", 19);		// 0x00040000
			pc.fire = getIntegerToken(data, "|", 20);		// 0x00080000
			pc.wind = getIntegerToken(data, "|", 21);		// 0x00100000
			pc.gold = getIntegerToken(data, "|", 22);		// 0x00200000
			pc.titleNo = getIntegerToken(data, "|", 23);		// 0x00400000
			pc.dp = getIntegerToken(data, "|", 24);		// 0x00800000
			pc.transmigration = getIntegerToken(data, "|", 25);// 0x01000000
			pc.ridePetNo = getIntegerToken(data, "|", 26);	// 0x02000000
			pc.learnride = getIntegerToken(data, "|", 27);	// 0x04000000
			pc.baseGraNo = getIntegerToken(data, "|", 28);	// 0x08000000
#ifdef _NEW_RIDEPETS
			pc.lowsride = getIntegerToken(data, "|", 29);		// 0x08000000
#endif
#ifdef _SFUMATO
			pc.sfumato = 0xff0000;
#endif
			getStringToken(data, "|", 30, name);
			makeStringFromEscaped(name);
			name = name.simplified();
			pc.name = name;
			getStringToken(data, "|", 31, freeName);
			makeStringFromEscaped(freeName);
			freeName = freeName.simplified();
			pc.freeName = freeName;
#ifdef _NEW_ITEM_
			pc.道具欄狀態 = getIntegerToken(data, "|", 32);
#endif
#ifdef _SA_VERSION_25
			int pointindex = getIntegerToken(data, "|", 33);
			char pontname[][32] = {
				"薩姆吉爾村",
				"瑪麗娜絲村",
				"加加村",
				"卡魯它那村",
			};
			sprintf(pc.chusheng, "%s", pontname[pointindex]);
#ifdef _MAGIC_ITEM_
			pc.法寶道具狀態 = getIntegerToken(data, "|", 34);
			pc.道具光環效果 = getIntegerToken(data, "|", 35);
#endif
#endif

		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (kubun & mask)
				{
					if (mask == 0x00000002) // ( 1 << 1 )
					{
						pc.hp = getIntegerToken(data, "|", i);// 0x00000002
						//custom
						pc.hpPercent = util::percent(pc.hp, pc.maxHp);

						//
						i++;
					}
					else if (mask == 0x00000004) // ( 1 << 2 )
					{
						pc.maxHp = getIntegerToken(data, "|", i);// 0x00000004
						//custom
						pc.hpPercent = util::percent(pc.hp, pc.maxHp);

						//
						i++;
					}
					else if (mask == 0x00000008)
					{
						pc.mp = getIntegerToken(data, "|", i);// 0x00000008
						//custom
						pc.mpPercent = util::percent(pc.mp, pc.maxMp);

						//
						i++;
					}
					else if (mask == 0x00000010)
					{
						pc.maxMp = getIntegerToken(data, "|", i);// 0x00000010
						//custom
						pc.mpPercent = util::percent(pc.mp, pc.maxMp);

						//
						i++;
					}
					else if (mask == 0x00000020)
					{
						pc.vital = getIntegerToken(data, "|", i);// 0x00000020
						i++;
					}
					else if (mask == 0x00000040)
					{
						pc.str = getIntegerToken(data, "|", i);// 0x00000040
						i++;
					}
					else if (mask == 0x00000080)
					{
						pc.tgh = getIntegerToken(data, "|", i);// 0x00000080
						i++;
					}
					else if (mask == 0x00000100)
					{
						pc.dex = getIntegerToken(data, "|", i);// 0x00000100
						i++;
					}
					else if (mask == 0x00000200)
					{
						pc.exp = getIntegerToken(data, "|", i);// 0x00000200
						i++;
					}
					else if (mask == 0x00000400)
					{
						pc.maxExp = getIntegerToken(data, "|", i);// 0x00000400
						i++;
					}
					else if (mask == 0x00000800)
					{
						pc.level = getIntegerToken(data, "|", i);// 0x00000800
						i++;
					}
					else if (mask == 0x00001000)
					{
						pc.atk = getIntegerToken(data, "|", i);// 0x00001000
						i++;
					}
					else if (mask == 0x00002000)
					{
						pc.def = getIntegerToken(data, "|", i);// 0x00002000
						i++;
					}
					else if (mask == 0x00004000)
					{
						pc.quick = getIntegerToken(data, "|", i);// 0x00004000
						i++;
					}
					else if (mask == 0x00008000)
					{
						pc.charm = getIntegerToken(data, "|", i);// 0x00008000
						i++;
					}
					else if (mask == 0x00010000)
					{
						pc.luck = getIntegerToken(data, "|", i);// 0x00010000
						i++;
					}
					else if (mask == 0x00020000)
					{
						pc.earth = getIntegerToken(data, "|", i);// 0x00020000
						i++;
					}
					else if (mask == 0x00040000)
					{
						pc.water = getIntegerToken(data, "|", i);// 0x00040000
						i++;
					}
					else if (mask == 0x00080000)
					{
						pc.fire = getIntegerToken(data, "|", i);// 0x00080000
						i++;
					}
					else if (mask == 0x00100000)
					{
						pc.wind = getIntegerToken(data, "|", i);// 0x00100000
						i++;
					}
					else if (mask == 0x00200000)
					{
						pc.gold = getIntegerToken(data, "|", i);// 0x00200000
						i++;
					}
					else if (mask == 0x00400000)
					{
						pc.titleNo = getIntegerToken(data, "|", i);// 0x00400000
						i++;
					}
					else if (mask == 0x00800000)
					{
						pc.dp = getIntegerToken(data, "|", i);// 0x00800000
						i++;
					}
					else if (mask == 0x01000000)
					{
						pc.transmigration = getIntegerToken(data, "|", i);// 0x01000000
						i++;
					}
					else if (mask == 0x02000000)
					{
						getStringToken(data, "|", i, name);// 0x01000000
						makeStringFromEscaped(name);
						name = name.simplified();
						pc.name = name;
						i++;
					}
					else if (mask == 0x04000000)
					{
						getStringToken(data, "|", i, freeName);// 0x02000000
						makeStringFromEscaped(freeName);
						freeName = freeName.simplified();
						pc.freeName = freeName;
						i++;
					}
					else if (mask == 0x08000000) // ( 1 << 27 )
					{
						pc.ridePetNo = getIntegerToken(data, "|", i);// 0x08000000
						i++;
					}
					else if (mask == 0x10000000) // ( 1 << 28 )
					{
						pc.learnride = getIntegerToken(data, "|", i);// 0x10000000
						i++;
					}
					else if (mask == 0x20000000) // ( 1 << 29 )
					{
						pc.baseGraNo = getIntegerToken(data, "|", i);// 0x20000000
						i++;
					}
					else if (mask == 0x40000000) // ( 1 << 30 )
					{
						pc.skywalker = getIntegerToken(data, "|", i);// 0x40000000
						i++;
					}
#ifdef _CHARSIGNADY_NO_
					else if (mask == 0x80000000) // ( 1 << 31 )
					{
						pc.簽到標記 = getIntegerToken(data, "|", i);// 0x80000000
						i++;
					}
#endif
				}
			}
		}

		//updataPcAct();
		if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
		{
			party[0].level = pc.level;
			party[0].maxHp = pc.maxHp;
			party[0].hp = pc.hp;
			party[0].name = pc.name;
		}

		getPlayerMaxCarryingCapacity();

		emit signalDispatcher.updateCharHpProgressValue(pc.level, pc.hp, pc.maxHp);
		emit signalDispatcher.updateCharMpProgressValue(pc.level, pc.mp, pc.maxMp);

		if (pc.ridePetNo < 0)
		{
			for (int i = 0; i < MAX_PET; ++i)
			{
				if (pet[i].state == kRide)
				{
					if (pc.selectPetNo[i] == TRUE)
					{
						pet[i].state = kStandby;
					}
					else
					{
						pet[i].state = kRest;
					}
				}
			}
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		}
		else
		{
			PET _pet = pet[pc.ridePetNo];
			pet[pc.ridePetNo].state = kRide;
			emit signalDispatcher.updateRideHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}

		if (pc.battlePetNo < 0)
		{
			for (int i = 0; i < MAX_PET; ++i)
			{
				if (pet[i].state == kBattle)
				{
					if (pc.selectPetNo[i] == TRUE)
					{
						pet[i].state = kStandby;
					}
					else
					{
						pet[i].state = kRest;
					}
				}
			}
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		}
		else
		{
			PET _pet = pet[pc.battlePetNo];
			emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}

		emit signalDispatcher.updatePlayerInfoStone(pc.gold);
		const QVariantList varList = {
			pc.name, pc.freeName, "",
			QObject::tr("%1(%2tr)").arg(pc.level).arg(pc.transmigration), pc.exp, pc.maxExp, pc.exp > pc.maxExp ? 0 : pc.maxExp - pc.exp, "",
			QString("%1/%2").arg(pc.hp).arg(pc.maxHp), QString("%1/%2").arg(pc.mp).arg(pc.maxMp),
			pc.charm, pc.atk, pc.def, pc.quick, pc.luck
		};

		QVariant var = QVariant::fromValue(varList);

		playerInfoColContents.insert(0, var);
		emit signalDispatcher.updatePlayerInfoColContents(0, var);
		setWindowTitle();
	}
#pragma endregion
#pragma region FamilyInfo
	else if (first == "F") // F 家族狀態
	{
		QString familyName;

		getStringToken(data, "|", 1, familyName);
		makeStringFromEscaped(familyName);
		familyName = familyName.simplified();
		pc.familyName = familyName;

		pc.familyleader = getIntegerToken(data, "|", 2);
		pc.channel = getIntegerToken(data, "|", 3);
		pc.familySprite = getIntegerToken(data, "|", 4);
		pc.big4fm = getIntegerToken(data, "|", 5);
#ifdef _CHANNEL_MODIFY
		if (pc.familyleader == FMMEMBER_NONE)
		{
			//pc.etcFlag &= ~PC_ETCFLAG_CHAT_FM;
			talkMode = 0;
		}
#endif
		// HP,MP,EXP
	}
#pragma endregion
#pragma region PlayerModify
	else if (first == "M") // M HP,MP,EXP
	{
		pc.hp = getIntegerToken(data, "|", 1);
		pc.mp = getIntegerToken(data, "|", 2);
		pc.exp = getIntegerToken(data, "|", 3);
		//updataPcAct();
		if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
			party[0].hp = pc.hp;

		//custom
		pc.hpPercent = util::percent(pc.hp, pc.maxHp);
		pc.mpPercent = util::percent(pc.mp, pc.maxMp);

		emit signalDispatcher.updateCharHpProgressValue(pc.level, pc.hp, pc.maxHp);
		emit signalDispatcher.updateCharMpProgressValue(pc.level, pc.mp, pc.maxMp);
	}
#pragma endregion
#pragma region PetInfo
	else if (first == "K") // K 寵物狀態
	{
		QString name, freeName;
		int no, kubun, i;
		unsigned int mask;

		no = data.left(1).toUInt();
		data = data.mid(2);
		if (data.isEmpty())
			return;

		if (no < 0 || no >= MAX_PET)
			return;

		kubun = getInteger62Token(data, "|", 1);
		if (kubun == 0)
		{
			if (pet[no].useFlag)
			{
				if (no == pc.battlePetNo)
					pc.battlePetNo = -1;
				if (no == pc.mailPetNo)
					pc.mailPetNo = -1;
				pc.selectPetNo[no] = FALSE;
			}
			pet[no] = {};
		}
		else
		{
			pet[no].useFlag = 1;
			if (kubun == 1)
			{
				pet[no].graNo = getIntegerToken(data, "|", 2);		// 0x00000002
				pet[no].hp = getIntegerToken(data, "|", 3);		// 0x00000004
				pet[no].maxHp = getIntegerToken(data, "|", 4);		// 0x00000008
				pet[no].mp = getIntegerToken(data, "|", 5);		// 0x00000010
				pet[no].maxMp = getIntegerToken(data, "|", 6);		// 0x00000020

				//custom
				pet[no].hpPercent = util::percent(pet[no].hp, pet[no].maxHp);
				pet[no].mpPercent = util::percent(pet[no].mp, pet[no].maxMp);

				pet[no].exp = getIntegerToken(data, "|", 7);		// 0x00000040
				pet[no].maxExp = getIntegerToken(data, "|", 8);		// 0x00000080
				pet[no].level = getIntegerToken(data, "|", 9);		// 0x00000100
				pet[no].atk = getIntegerToken(data, "|", 10);		// 0x00000200
				pet[no].def = getIntegerToken(data, "|", 11);		// 0x00000400
				pet[no].quick = getIntegerToken(data, "|", 12);		// 0x00000800
				pet[no].ai = getIntegerToken(data, "|", 13);		// 0x00001000
				pet[no].earth = getIntegerToken(data, "|", 14);		// 0x00002000
				pet[no].water = getIntegerToken(data, "|", 15);		// 0x00004000
				pet[no].fire = getIntegerToken(data, "|", 16);		// 0x00008000
				pet[no].wind = getIntegerToken(data, "|", 17);		// 0x00010000
				pet[no].maxSkill = getIntegerToken(data, "|", 18);		// 0x00020000
				pet[no].changeNameFlag = getIntegerToken(data, "|", 19);// 0x00040000
				pet[no].trn = getIntegerToken(data, "|", 20);
#ifdef _SHOW_FUSION
				pet[no].fusion = getIntegerToken(data, "|", 21);
				getStringToken(data, "|", 22, name);// 0x00080000
				makeStringFromEscaped(name);
				name = name.simplified();
				pet[no].name = name;
				getStringToken(data, "|", 23, freeName);// 0x00100000
				makeStringFromEscaped(freeName);
				freeName = freeName.simplified();
				pet[no].freeName = freeName;
#else
				getStringToken(data, "|", 21, name);// 0x00080000
				makeStringFromEscaped(name);
				pet[no].name = name;

				getStringToken(data, "|", 22, freeName);// 0x00100000
				makeStringFromEscaped(freeName);
				pet[no].freeName = freeName;
#endif
#ifdef _PETCOM_
				pet[no].oldhp = getIntegerToken(data, "|", 24);
				pet[no].oldatk = getIntegerToken(data, "|", 25);
				pet[no].olddef = getIntegerToken(data, "|", 26);
				pet[no].oldquick = getIntegerToken(data, "|", 27);
				pet[no].oldlevel = getIntegerToken(data, "|", 28);
#endif
#ifdef _RIDEPET_
				pet[no].rideflg = getIntegerToken(data, "|", 29);
#endif
#ifdef _PETBLESS_
				pet[no].blessflg = getIntegerToken(data, "|", 30);
				pet[no].blesshp = getIntegerToken(data, "|", 31);
				pet[no].blessatk = getIntegerToken(data, "|", 32);
				pet[no].blessdef = getIntegerToken(data, "|", 33);
				pet[no].blessquick = getIntegerToken(data, "|", 34);
#endif
			}
			else
			{
				mask = 2;
				i = 2;
				for (; mask > 0; mask <<= 1)
				{
					if (kubun & mask)
					{
						if (mask == 0x00000002)
						{
							pet[no].graNo = getIntegerToken(data, "|", i);// 0x00000002
							i++;
						}
						else if (mask == 0x00000004)
						{
							pet[no].hp = getIntegerToken(data, "|", i);// 0x00000004
							pet[no].hpPercent = util::percent(pet[no].hp, pet[no].maxHp);
							i++;
						}
						else if (mask == 0x00000008)
						{
							pet[no].maxHp = getIntegerToken(data, "|", i);// 0x00000008
							pet[no].hpPercent = util::percent(pet[no].hp, pet[no].maxHp);
							i++;
						}
						else if (mask == 0x00000010)
						{
							pet[no].mp = getIntegerToken(data, "|", i);// 0x00000010
							pet[no].mpPercent = util::percent(pet[no].mp, pet[no].maxMp);
							i++;
						}
						else if (mask == 0x00000020)
						{
							pet[no].maxMp = getIntegerToken(data, "|", i);// 0x00000020
							pet[no].mpPercent = util::percent(pet[no].mp, pet[no].maxMp);
							i++;
						}
						else if (mask == 0x00000040)
						{
							pet[no].exp = getIntegerToken(data, "|", i);// 0x00000040
							i++;
						}
						else if (mask == 0x00000080)
						{
							pet[no].maxExp = getIntegerToken(data, "|", i);// 0x00000080
							i++;
						}
						else if (mask == 0x00000100)
						{
							pet[no].level = getIntegerToken(data, "|", i);// 0x00000100
							i++;
						}
						else if (mask == 0x00000200)
						{
							pet[no].atk = getIntegerToken(data, "|", i);// 0x00000200
							i++;
						}
						else if (mask == 0x00000400)
						{
							pet[no].def = getIntegerToken(data, "|", i);// 0x00000400
							i++;
						}
						else if (mask == 0x00000800)
						{
							pet[no].quick = getIntegerToken(data, "|", i);// 0x00000800
							i++;
						}
						else if (mask == 0x00001000)
						{
							pet[no].ai = getIntegerToken(data, "|", i);// 0x00001000
							i++;
						}
						else if (mask == 0x00002000)
						{
							pet[no].earth = getIntegerToken(data, "|", i);// 0x00002000
							i++;
						}
						else if (mask == 0x00004000)
						{
							pet[no].water = getIntegerToken(data, "|", i);// 0x00004000
							i++;
						}
						else if (mask == 0x00008000)
						{
							pet[no].fire = getIntegerToken(data, "|", i);// 0x00008000
							i++;
						}
						else if (mask == 0x00010000)
						{
							pet[no].wind = getIntegerToken(data, "|", i);// 0x00010000
							i++;
						}
						else if (mask == 0x00020000)
						{
							pet[no].maxSkill = getIntegerToken(data, "|", i);// 0x00020000
							i++;
						}
						else if (mask == 0x00040000)
						{
							pet[no].changeNameFlag = getIntegerToken(data, "|", i);// 0x00040000
							i++;
						}
						else if (mask == 0x00080000)
						{
							getStringToken(data, "|", i, name);// 0x00080000
							makeStringFromEscaped(name);
							name = name.simplified();
							pet[no].name = name;
							i++;
						}
						else if (mask == 0x00100000)
						{
							getStringToken(data, "|", i, freeName);// 0x00100000
							makeStringFromEscaped(freeName);
							freeName = freeName.simplified();
							pet[no].freeName = freeName;
							i++;
						}
#ifdef _PETCOM_
						else if (mask == 0x200000)
						{
							pet[no].oldhp = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x400000)
						{
							pet[no].oldatk = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x800000)
						{
							pet[no].olddef = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x1000000)
						{
							pet[no].oldquick = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x2000000)
						{
							pet[no].oldlevel = getIntegerToken(data, "|", i);
							i++;
						}
#endif
#ifdef _PETBLESS_
						else if (mask == 0x4000000)
						{
							pet[no].blessflg = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x8000000)
						{
							pet[no].blesshp = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x10000000)
						{
							pet[no].blessatk = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x20000000)
						{
							pet[no].blessquick = getIntegerToken(data, "|", i);
							i++;
						}
						else if (mask == 0x40000000)
						{
							pet[no].blessdef = getIntegerToken(data, "|", i);
							i++;
						}
#endif
					}
				}
			}
		}

		if (pc.ridePetNo >= 0 && pc.ridePetNo < MAX_PET)
		{
			PET _pet = pet[pc.ridePetNo];
			pet[pc.ridePetNo].state = kRide;
			emit signalDispatcher.updateRideHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		}

		if (pc.battlePetNo >= 0 && pc.battlePetNo < MAX_PET)
		{
			PET _pet = pet[pc.battlePetNo];
			pet[pc.battlePetNo].state = kBattle;
			emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		}

		for (int i = 0; i < MAX_PET; ++i)
		{
			PET _pet = pet[i];
			QVariantList varList;
			if (_pet.useFlag == 1)
			{
				pet[i].power = (((static_cast<double>(_pet.atk + _pet.def + _pet.quick) + (static_cast<double>(_pet.maxHp) / 4.0)) / static_cast<double>(_pet.level)) * 100.0);
				varList = QVariantList{
					_pet.name, _pet.freeName, "",
					QObject::tr("%1(%2tr)").arg(_pet.level).arg(_pet.trn), _pet.exp, _pet.maxExp, _pet.maxExp - _pet.exp, "",
					QString("%1/%2").arg(_pet.hp).arg(_pet.maxHp), "",
					_pet.ai, _pet.atk, _pet.def, _pet.quick, "", pet[i].power

				};
			}
			else
			{
				for (int i = 0; i < 16; ++i)
					varList.append("");

			}

			QVariant var = QVariant::fromValue(varList);
			playerInfoColContents.insert(i + 1, var);
			emit signalDispatcher.updatePlayerInfoColContents(i + 1, var);
		}

	}
#pragma endregion
#pragma region EncountPercentage
	else if (first == "E") // E nowEncountPercentage
	{
		//minEncountPercentage = getIntegerToken(data, "|", 1);
		//maxEncountPercentage = getIntegerToken(data, "|", 2);
		//nowEncountPercentage = minEncountPercentage;
	}
#pragma endregion
#pragma region MagicInfo
	else if (first == "J") //J 精靈
	{
		QString name, memo;
		int no;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= MAX_MAGIC)
			return;

		if (data.isEmpty())
			return;

		magic[no].useFlag = getIntegerToken(data, "|", 1);
		if (magic[no].useFlag != 0)
		{
			magic[no].mp = getIntegerToken(data, "|", 2);
			magic[no].field = getIntegerToken(data, "|", 3);
			magic[no].target = getIntegerToken(data, "|", 4);
			if (magic[no].target >= 100)
			{
				magic[no].target %= 100;
				magic[no].deadTargetFlag = 1;
			}
			else
				magic[no].deadTargetFlag = 0;
			getStringToken(data, "|", 5, name);
			makeStringFromEscaped(name);
			name = name.simplified();
			magic[no].name = name;
			getStringToken(data, "|", 6, memo);
			makeStringFromEscaped(memo);
			memo = memo.simplified();
			magic[no].memo = memo;
		}
		else
		{
			magic[no] = {};
		}

		QStringList magicNameList;
		for (int i = 0; i < MAX_MAGIC; ++i)
		{
			magicNameList.append(magic[i].name);
		}

		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxCharAction, magicNameList);
	}
#pragma endregion
#pragma region TeamInfo
	else if (first == "N")  // N 隊伍資訊
	{
		auto updateTeamInfo = [this, &signalDispatcher]()
		{
			QStringList teamInfoList;
			for (int i = 0; i < MAX_PARTY; ++i)
			{
				if (party[i].name.isEmpty() || (party[i].useFlag == 0) || (party[i].maxHp <= 0))
				{
					party[i] = {};
					teamInfoList.append("");
					continue;
				}
				QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(party[i].name).arg(party[i].level)
					.arg(party[i].hp).arg(party[i].maxHp).arg(party[i].hpPercent);
				teamInfoList.append(text);
			}
			emit signalDispatcher.updateTeamInfo(teamInfoList);
		};

		QString name;
		int no, kubun, i, checkPartyCount, no2;
		//int gx, gy;
		unsigned int mask;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= MAX_PARTY)
			return;

		if (data.isEmpty())
			return;

		kubun = getInteger62Token(data, "|", 1);
		if (kubun == 0)
		{
			if (party[no].useFlag != 0 && party[no].id != pc.id)
			{

			}

			party[no].useFlag = 0;
			party[no] = {};
			checkPartyCount = 0;
			no2 = -1;
#ifdef MAX_AIRPLANENUM
			for (i = 0; i < MAX_AIRPLANENUM; ++i)
#else
			for (i = 0; i < MAX_PARTY; ++i)
#endif
			{
				if (party[i].useFlag != 0)
				{
					checkPartyCount++;
					if (no2 == -1 && i > no)
						no2 = i;
				}
			}
			if (checkPartyCount <= 1)
			{
				partyModeFlag = 0;
#ifdef _CHANNEL_MODIFY
				//pc.etcFlag &= ~PC_ETCFLAG_CHAT_MODE;
				if (talkMode == 2)
					talkMode = 0;
#endif
			}
			else
			{
				//if (no2 >= 0 || gx >= 0 || gy >= 0)
					//goFrontPartyCharacter(no2, gx, gy);
			}
			updateTeamInfo();
			return;
		}

		partyModeFlag = 1;
		prSendFlag = 0;
		party[no].useFlag = 1;

		if (kubun == 1)
		{
			party[no].id = getIntegerToken(data, "|", 2);	// 0x00000002
			party[no].level = getIntegerToken(data, "|", 3);	// 0x00000004
			party[no].maxHp = getIntegerToken(data, "|", 4);	// 0x00000008
			party[no].hp = getIntegerToken(data, "|", 5);	// 0x00000010
			party[no].mp = getIntegerToken(data, "|", 6);	// 0x00000020
			getStringToken(data, "|", 7, name);	// 0x00000040
			makeStringFromEscaped(name);
			name = name.simplified();
			party[no].name = name;
		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (kubun & mask)
				{
					if (mask == 0x00000002)
					{
						party[no].id = getIntegerToken(data, "|", i);// 0x00000002
						i++;
					}
					else if (mask == 0x00000004)
					{
						party[no].level = getIntegerToken(data, "|", i);// 0x00000004
						i++;
					}
					else if (mask == 0x00000008)
					{
						party[no].maxHp = getIntegerToken(data, "|", i);// 0x00000008
						i++;
					}
					else if (mask == 0x00000010)
					{
						party[no].hp = getIntegerToken(data, "|", i);// 0x00000010
						i++;
					}
					else if (mask == 0x00000020)
					{
						party[no].mp = getIntegerToken(data, "|", i);// 0x00000020
						i++;
					}
					else if (mask == 0x00000040)
					{
						getStringToken(data, "|", i, name);// 0x00000040
						makeStringFromEscaped(name);
						name = name.simplified();
						party[no].name = name;
						i++;
					}
				}
			}
		}
		if (party[no].id != pc.id)
		{
			//ptAct = getCharObjAct(party[no].id);
			//if (ptAct != NULL)
			//{
			//	party[no].ptAct = ptAct;
			//	setCharParty(ptAct);
			//	// NPC
			//	if (no == 0)
			//		setCharLeader(ptAct);
			//}
			//else
			//	party[no].ptAct = NULL;
		}
		else
		{
			//party[no].ptAct = pc.ptAct;
			pc.status |= CHR_STATUS_PARTY;
			// PC
			if (no == 0)
				pc.status &= (~CHR_STATUS_LEADER);
		}
		party[no].hpPercent = util::percent(party[no].hp, party[no].maxHp);
		updateTeamInfo();
	}
#pragma endregion
#pragma region ItemInfo
	else if (first == "I") //I 道具
	{
		int i, no;
		QString temp;
		QMutexLocker lock(&swapItemMutex_);

		for (i = 0; i < MAX_ITEM; ++i)
		{
#ifdef _ITEM_JIGSAW
#ifdef _NPC_ITEMUP
#ifdef _ITEM_COUNTDOWN
			no = i * 16;
#else
			no = i * 15;
#endif
#else
			no = i * 14;
#endif
#else
#ifdef _PET_ITEM
			no = i * 13;
#else
#ifdef _ITEM_PILENUMS
#ifdef _ALCHEMIST //#ifdef _ITEMSET7_TXT
			no = i * 14;
#else

			no = i * 11;

#endif//_ALCHEMIST
#else

			no = i * 10;

			//end modified by lsh

#endif//_ITEM_PILENUMS
#endif//_PET_ITEM
#endif//_ITEM_JIGSAW
			getStringToken(data, "|", no + 1, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			if (temp.isEmpty())
			{
				pc.item[i].useFlag = 0;
				pc.item[i].name.clear();
				pc.item[i] = {};
				refreshItemInfo(i);
				continue;
			}
			pc.item[i].useFlag = 1;
			pc.item[i].name = temp.simplified();
			getStringToken(data, "|", no + 2, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			pc.item[i].name2 = temp;
			pc.item[i].color = getIntegerToken(data, "|", no + 3);
			if (pc.item[i].color < 0)
				pc.item[i].color = 0;
			getStringToken(data, "|", no + 4, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			pc.item[i].memo = temp;
			pc.item[i].graNo = getIntegerToken(data, "|", no + 5);
			pc.item[i].field = getIntegerToken(data, "|", no + 6);
			pc.item[i].target = getIntegerToken(data, "|", no + 7);
			if (pc.item[i].target >= 100)
			{
				pc.item[i].target %= 100;
				pc.item[i].deadTargetFlag = 1;
			}
			else
				pc.item[i].deadTargetFlag = 0;
			pc.item[i].level = getIntegerToken(data, "|", no + 8);
			pc.item[i].sendFlag = getIntegerToken(data, "|", no + 9);

			// 顯示物品耐久度
			getStringToken(data, "|", no + 10, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			pc.item[i].damage = temp;
#ifdef _ITEM_PILENUMS
			getStringToken(data, "|", no + 11, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			pc.item[i].pile = temp.toInt();
#endif
#ifdef _ALCHEMIST //_ITEMSET7_TXT
			getStringToken(data, "|", no + 12, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			pc.item[i].alch = temp;
#endif
#ifdef _PET_ITEM
			pc.item[i].type = getIntegerToken(data, "|", no + 13);
#else
#ifdef _MAGIC_ITEM_
			pc.item[i].道具類型 = getIntegerToken(data, "|", no + 13);
#endif
#endif
#ifdef _ITEM_JIGSAW
			getStringToken(data, "|", no + 14, temp);
			pc.item[i].jigsaw = temp.simplified();

#endif
#ifdef _NPC_ITEMUP
			pc.item[i].itemup = getIntegerToken(data, "|", no + 15);
#endif
#ifdef _ITEM_COUNTDOWN
			pc.item[i].counttime = getIntegerToken(data, "|", no + 16);
#endif

			refreshItemInfo(i);
		}

		QStringList itemList;
		for (const ITEM& it : pc.item)
		{
			if (it.name.isEmpty())
				continue;
			itemList.append(it.name);
		}
		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxItem, itemList);
	}
#pragma endregion
#pragma region PetSkill
	else if (first == "W")//接收到的寵物技能
	{
		int i, no, no2;
		QString temp;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= MAX_SKILL)
			return;

		if (data.isEmpty())
			return;


		for (i = 0; i < MAX_SKILL; ++i)
		{
			petSkill[no][i].useFlag = 0;
		}

		QStringList skillNameList;
		for (i = 0; i < MAX_SKILL; ++i)
		{
			no2 = i * 5;
			getStringToken(data, "|", no2 + 4, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			if (temp.isEmpty())
				continue;
			petSkill[no][i].useFlag = 1;
			petSkill[no][i].name = temp;
			petSkill[no][i].skillId = getIntegerToken(data, "|", no2 + 1);
			petSkill[no][i].field = getIntegerToken(data, "|", no2 + 2);
			petSkill[no][i].target = getIntegerToken(data, "|", no2 + 3);
			getStringToken(data, "|", no2 + 5, temp);
			makeStringFromEscaped(temp);
			temp = temp.simplified();
			petSkill[no][i].memo = temp;

			if ((pc.battlePetNo >= 0) && pc.battlePetNo < MAX_PET)
				skillNameList.append(petSkill[no][i].name);
		}

		if ((pc.battlePetNo >= 0) && pc.battlePetNo < MAX_PET)
			emit signalDispatcher.updateComboBoxItemText(util::kComboBoxPetAction, skillNameList);
	}
#pragma endregion
#pragma region PlayerSkill
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	else if (first == "S") // S 職業技能
	{
		QString name;
		QString memo;
		int i, count = 0;

		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			profession_skill[i].useFlag = 0;
			profession_skill[i].kind = 0;
		}
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			count = i * 9;
			profession_skill[i].useFlag = getIntegerToken(data, "|", 1 + count);
			profession_skill[i].skillId = getIntegerToken(data, "|", 2 + count);
			profession_skill[i].target = getIntegerToken(data, "|", 3 + count);
			profession_skill[i].kind = getIntegerToken(data, "|", 4 + count);
			profession_skill[i].icon = getIntegerToken(data, "|", 5 + count);
			profession_skill[i].costmp = getIntegerToken(data, "|", 6 + count);
			profession_skill[i].skill_level = getIntegerToken(data, "|", 7 + count);

			getStringToken(data, "|", 8 + count, name);
			makeStringFromEscaped(name);
			name = name.simplified();
			profession_skill[i].name = name;

			getStringToken(data, "|", 9 + count, memo);
			makeStringFromEscaped(memo);
			memo = memo.simplified();
			profession_skill[i].memo = memo;
		}

		QStringList magicNameList;
		for (i = 0; i < MAX_MAGIC; ++i)
		{
			magicNameList.append(magic[i].name);
		}
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			magicNameList.append(profession_skill[i].name);
		}
		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxCharAction, magicNameList);

#ifdef _SKILLSORT
		SortSkill();
#endif
	}
#endif
#pragma endregion
#pragma region PRO3_ADDSKILL
#ifdef _PRO3_ADDSKILL
	case 'G':
	{
		int i, count = 0;
		data++;
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
			profession_skill[i].cooltime = 0;
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			count = i * 1;
			profession_skill[i].cooltime = getIntegerToken(data, "|", 1 + count);
		}
		break;
	}
#endif
#pragma endregion
#pragma region PetEquip
#ifdef _PET_ITEM
	else if (first == "B") // B 寵物道具
	{
		int i, no, nPetIndex;
		QString szData;

		nPetIndex = data.left(1).toUInt();
		data = data.mid(2);

		if (nPetIndex < 0 || nPetIndex >= MAX_PET)
			return;

		if (data.isEmpty())
			return;

		for (i = 0; i < MAX_PET_ITEM; ++i)
		{
#ifdef _ITEM_JIGSAW
#ifdef _NPC_ITEMUP
#ifdef _ITEM_COUNTDOWN
			no = i * 16;
#else
			no = i * 15;
#endif
#else
			no = i * 14;
#endif
#else
			no = i * 13;
#endif
			getStringToken(data, "|", no + 1, szData);
			makeStringFromEscaped(szData);
			szData = szData.simplified();
			if (szData.isEmpty())	// 沒道具
			{
				pet[nPetIndex].item[i] = {};
				continue;
			}
			pet[nPetIndex].item[i].useFlag = 1;
			pet[nPetIndex].item[i].name = szData;
			getStringToken(data, "|", no + 2, szData);
			makeStringFromEscaped(szData);
			szData = szData.simplified();
			pet[nPetIndex].item[i].name2 = szData;
			pet[nPetIndex].item[i].color = getIntegerToken(data, "|", no + 3);
			if (pet[nPetIndex].item[i].color < 0)
				pet[nPetIndex].item[i].color = 0;
			getStringToken(data, "|", no + 4, szData);
			makeStringFromEscaped(szData);
			szData = szData.simplified();
			pet[nPetIndex].item[i].memo = szData.simplified();
			pet[nPetIndex].item[i].graNo = getIntegerToken(data, "|", no + 5);
			pet[nPetIndex].item[i].field = getIntegerToken(data, "|", no + 6);
			pet[nPetIndex].item[i].target = getIntegerToken(data, "|", no + 7);
			if (pet[nPetIndex].item[i].target >= 100)
			{
				pet[nPetIndex].item[i].target %= 100;
				pet[nPetIndex].item[i].deadTargetFlag = 1;
			}
			else
				pet[nPetIndex].item[i].deadTargetFlag = 0;
			pet[nPetIndex].item[i].level = getIntegerToken(data, "|", no + 8);
			pet[nPetIndex].item[i].sendFlag = getIntegerToken(data, "|", no + 9);

			// 顯示物品耐久度
			getStringToken(data, "|", no + 10, szData);
			makeStringFromEscaped(szData);
			szData = szData.simplified();
			pet[nPetIndex].item[i].damage = szData;
			pet[nPetIndex].item[i].pile = getIntegerToken(data, "|", no + 11);
#ifdef _ALCHEMIST //_ITEMSET7_TXT
			getStringToken(data, "|", no + 12, szData);
			makeStringFromEscaped(szData);
			szData = szData.simplified();
			pet[nPetIndex].item[i].alch = szData;
#endif
			pet[nPetIndex].item[i].type = getIntegerToken(data, "|", no + 13);
#ifdef _ITEM_JIGSAW
			getStringToken(data, "|", no + 14, szData);
			makeStringFromEscaped(szData);
			szData = szData.simplified();
			pet[nPetIndex].item[i].jigsaw = szData;
			//可拿給寵物裝備的道具,就不會是拼圖了,以下就免了
			//if( i == JigsawIdx )
			//	SetJigsaw( pc.item[i].graNo, pc.item[i].jigsaw );
#endif
#ifdef _NPC_ITEMUP
			pet[nPetIndex].item[i].itemup = getIntegerToken(data, "|", no + 15);
#endif
#ifdef _ITEM_COUNTDOWN
			pet[nPetIndex].item[i].counttime = getIntegerToken(data, "|", no + 16);
#endif
		}
	}
#endif
#pragma endregion
#pragma region S_recv_Unknown
	else if (first == "U")
	{
		//*新手兜|DEF+25|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//*新手铠|DEF+53 QUICK-14|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//*新手棍棒|ATK+60|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//白狼之眼|QUICK+40 MP+80|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//双蛇之戒|ATK+20 DEF+20 HP+80 MP+80|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//*新手腰带|DEF+10 HP+60|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//*新手盾|DEF+5 HP+5|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//*新手鞋|DEF+10 QUICK+10|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//*新手手套|ATK+20 QUICK+10|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//风的石头||0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//小块肉||0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//小块肉||0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
		//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	}
	else if (first == "H")
	{
		//H0|0|  //0~19
	}
	else if (first == "O")
	{
		//O0|||||||||||||
		//O1|||||||||||||
	}
	else if (first == "R")
	{
		//R|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
	}
	else if (first == "V")
	{
	}
	else if (first == "Z")
	{
		//1|-1|4542819|
	}
	else if (first == "!")
	{
		/*
			!0|0|
			!1|0|
			!2|0|
			!3|0|
			!4|0|
			!5|0|
		*/
	}
#pragma endregion
	else
	{
		qDebug() << "[" << first << "]:" << data;
	}

	setPC(pc);
}

//客戶端登入(進去選人畫面)
void Server::lssproto_ClientLogin_recv(char* cresult)
{
	QString result = util::toUnicode(cresult);
	if (result.isEmpty())
		return;
	//netproc_sending = NETPROC_SENDING;
	//if (netproc_sending == NETPROC_SENDING)
	//{
		//netproc_sending = NETPROC_RECEIVED;
	if (result.contains(OKSTR, Qt::CaseInsensitive))
	{
		time(&serverAliveLongTime);
		localtime_s(&serverAliveTime, &serverAliveLongTime);
	}
	else if (result.contains(CANCLE, Qt::CaseInsensitive))
	{

	}
}

//新增人物
void Server::lssproto_CreateNewChar_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains(SUCCESSFULSTR, Qt::CaseInsensitive) || data.contains(SUCCESSFULSTR, Qt::CaseInsensitive))
	{

	}
	else
	{
		//創建人物內容提示
	}
}

//更新人物列表
void Server::lssproto_CharList_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;
	//char LoginErrorMessage[1024];
	//memset(LoginErrorMessage, 0, 1024);
	if (result.contains("failed", Qt::CaseInsensitive))
	{
		//_snprintf_s(LoginErrorMessage, sizeof(LoginErrorMessage), _TRUNCATE, "%s", data);
#ifdef _AIDENGLU_
		PcLanded.登陸延時時間 = TimeGetTime() + 2000;
#endif
		return;
	}

	//if (netproc_sending == NETPROC_SENDING)
	//{
	QString nm, opt;
	int i;

	//netproc_sending = NETPROC_RECEIVED;
	if (!result.contains(SUCCESSFULSTR, Qt::CaseInsensitive) && !data.contains(SUCCESSFULSTR, Qt::CaseInsensitive))
	{
		//if (result.contains("OUTOFSERVICE", Qt::CaseInsensitive))
		return;
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusGettingPlayerList);

	for (i = 0; i < MAXCHARACTER; ++i)
		chartable[i] = {};

	QVector<CHARLISTTABLE> vec;
	for (i = 0; i < MAXCHARACTER; ++i)
	{
		CHARLISTTABLE table = {};
		nm.clear();
		opt.clear();
		getStringToken(data, "|", i * 2 + 1, nm);
		makeStringFromEscaped(nm);
		nm = nm.simplified();

		getStringToken(data, "|", i * 2 + 2, opt);
		makeStringFromEscaped(opt);
		opt.replace("\\yz", "|");
		opt.replace("\\z", "|");
		opt = opt.simplified();

		if (opt.startsWith("0") || opt.startsWith("1"))
		{
			QStringList args = opt.split(util::rexOR);
			if (args.size() < 13)
				continue;
			int index = args.at(0).toInt();
			if (index >= 0 && index < MAXCHARACTER)
			{
				table.valid = true;
				table.name = nm;
				table.faceGraNo = args.at(1).toInt();
				table.level = args.at(2).toInt();
				table.hp = args.at(3).toInt();
				table.str = args.at(4).toInt();
				table.def = args.at(5).toInt();
				table.agi = args.at(6).toInt();
				table.chasma = args.at(7).toInt();
				table.dp = args.at(8).toInt();
				table.attr[0] = args.at(9).toInt();
				if (table.attr[0] < 0 || table.attr[0] > 100)
					table.attr[0] = 0;
				table.attr[1] = args.at(10).toInt();
				if (table.attr[1] < 0 || table.attr[1] > 100)
					table.attr[1] = 0;
				table.attr[2] = args.at(11).toInt();
				if (table.attr[2] < 0 || table.attr[2] > 100)
					table.attr[2] = 0;
				table.attr[3] = args.at(12).toInt();
				if (table.attr[3] < 0 || table.attr[3] > 100)
					table.attr[3] = 0;

				table.pos = index;
				vec.append(table);
			}
		}
	}

	int size = vec.size();
	for (i = 0; i < size; ++i)
	{
		if (i < 0 || i >= MAXCHARACTER)
			continue;

		int index = vec.at(i).pos;
		chartable[index] = vec.at(i);
	}
}

//人物登出(不是每個私服都有，有些是直接切斷後跳回帳號密碼頁)
void Server::lssproto_CharLogout_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains(SUCCESSFULSTR, Qt::CaseInsensitive) || data.contains(SUCCESSFULSTR, Qt::CaseInsensitive))
	{
		setBattleFlag(false);
		setOnlineFlag(false);
	}
}

//人物登入
void Server::lssproto_CharLogin_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;

#ifdef __NEW_CLIENT
	if (strcmp(result, SUCCESSFULSTR) == 0 && !hPing)
#else
	if (result.contains(SUCCESSFULSTR, Qt::CaseInsensitive) || data.contains(SUCCESSFULSTR, Qt::CaseInsensitive))
#endif
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSignning);
		loginTimer.restart();
		setOnlineFlag(true);
#ifdef __NEW_CLIENT
		hPing = CreateThread(NULL, 0, PingFunc, &sin_server.sin_addr, 0, &dwPingID);
#endif
	}

#ifdef __NEW_CLIENT
#ifdef _NEW_WGS_MSG				// WON ADD WGS的新視窗
	if (strcmp(result, "failed") == 0 && !hPing)
		ERROR_MESSAGE = atoi(data);
#endif
#endif
#ifdef _ANGEL_SUMMON
	angelFlag = FALSE;
	angelMsg[0] = NULL;
#endif
}

void Server::lssproto_TD_recv(char* cdata)//交易
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString Head = "";
	QString buf_sockfd = "";
	QString buf_name = "";
	QString buf = "";

	getStringToken(data, "|", 1, Head);

	PC pc = getPC();

	// 交易开启资料初始化
	if (Head.startsWith("C"))
	{

		getStringToken(data, "|", 2, opp_sockfd);
		getStringToken(data, "|", 3, opp_name);
		getStringToken(data, "|", 4, trade_command);

		if (trade_command.startsWith("0"))
		{
			return;
		}
		else if (trade_command.startsWith("1"))
		{
			myitem_tradeList.clear();
			mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
			mygoldtrade = 0;

			IS_TRADING = true;
			MenuToggleFlag = JOY_CTRL_T;
			pc.trade_confirm = 1;
		}
#ifdef _COMFIRM_TRADE_REQUEST
		else if (trade_command.startsWith("2"))
		{
			tradeStatus = 1;
			MenuToggleFlag = JOY_CTRL_T;
			pc.trade_confirm = 1;
			tradeWndNo = 1;
		}
#endif

	}
	//处理物品交易资讯传递
	else if (Head.startsWith("T"))
	{

		if (!IS_TRADING)
			return;

		QString buf_showindex;

		//andy_add mttrade
		getStringToken(data, "|", 4, trade_kind);
		if (trade_kind.startsWith("S"))
		{
			QString buf1;
			int objno = -1;//, showno = -1;

			getStringToken(data, "|", 6, buf1);
			objno = buf1.toInt();
			getStringToken(data, "|", 7, buf1);
			//showno = buf1.toInt();
			getStringToken(data, "|", 5, buf1);

			if (buf1.startsWith("I"))
			{	//I
			}
			else
			{	//P
				tradePetIndex = objno;
				tradePet[0].index = objno;

				if (pet[objno].useFlag && pc.ridePetNo != objno)
				{
					if (pet[objno].freeName[0] != NULL)
						tradePet[0].name = pet[objno].freeName;
					else
						tradePet[0].name = pet[objno].name;
					tradePet[0].level = pet[objno].level;
					tradePet[0].atk = pet[objno].atk;
					tradePet[0].def = pet[objno].def;
					tradePet[0].quick = pet[objno].quick;
					tradePet[0].graNo = pet[objno].graNo;

					showindex[3] = 3;
					//DeathAction(pActPet4);
					//pActPet4 = NULL;
				}
			}

			//mouse.itemNo = -1;

			return;
		}

		getStringToken(data, "|", 2, buf_sockfd);
		getStringToken(data, "|", 3, buf_name);
		getStringToken(data, "|", 4, trade_kind);
		getStringToken(data, "|", 5, buf_showindex);
		opp_showindex = buf_showindex.toInt();

		if (!buf_sockfd.contains(opp_sockfd) || !buf_name.contains(opp_name))
			return;

		if (trade_kind.startsWith("G"))
		{

			getStringToken(data, "|", 6, opp_goldmount);
			int mount = opp_goldmount.toInt();


			if (opp_showindex == 1)
			{
				if (mount != -1)
				{
					showindex[4] = 2;
					tradeWndDropGoldGet = mount;
				}
				else
				{
					showindex[4] = 0;
					tradeWndDropGoldGet = 0;
				}
			}
			else if (opp_showindex == 2)
			{
				if (mount != -1)
				{
					showindex[5] = 2;
					tradeWndDropGoldGet = mount;
				}
				else
				{
					showindex[5] = 0;
					tradeWndDropGoldGet = 0;
				}
			}
			else return;


		}

		if (trade_kind.startsWith("I"))
		{
			QString pilenum, item_freename;

			getStringToken(data, "|", 6, opp_itemgraph);

			getStringToken(data, "|", 7, opp_itemname);
			getStringToken(data, "|", 8, item_freename);

			getStringToken(data, "|", 9, opp_itemeffect);
			getStringToken(data, "|", 10, opp_itemindex);
			getStringToken(data, "|", 11, opp_itemdamage);// 显示物品耐久度

#ifdef _ITEM_PILENUMS
			getStringToken(data, "|", 12, pilenum);//pilenum
#endif

		}
	}


	if (trade_kind.startsWith("P"))
	{
#ifdef _PET_ITEM
		int		iItemNo;
		QString	szData;
#endif
		int index = -1;

#ifdef _PET_ITEM
		for (int i = 0;; ++i)
		{
			if (getStringToken(data, "|", 26 + i * 6, szData))
				break;
			iItemNo = szData.toInt();
			getStringToken(data, "|", 27 + i * 6, opp_pet[index].oPetItemInfo[iItemNo].name);
			getStringToken(data, "|", 28 + i * 6, opp_pet[index].oPetItemInfo[iItemNo].memo);
			getStringToken(data, "|", 29 + i * 6, opp_pet[index].oPetItemInfo[iItemNo].damage);
			getStringToken(data, "|", 30 + i * 6, szData);
			opp_pet[index].oPetItemInfo[iItemNo].color = szData.toInt();
			getStringToken(data, "|", 31 + i * 6, szData);
			opp_pet[index].oPetItemInfo[iItemNo].bmpNo = szData.toInt();
		}
#endif

	}


	if (trade_kind.startsWith("C"))
	{
		if (pc.trade_confirm == 1)
			pc.trade_confirm = 3;
		if (pc.trade_confirm == 2)
			pc.trade_confirm = 4;
		if (pc.trade_confirm == 3)
		{
			//我方已點確認後，收到對方點確認
		}
		setPC(pc);
	}

	// end
	if (trade_kind.startsWith("A"))
	{
		tradeStatus = 2;
		IS_TRADING = false;
		myitem_tradeList.clear();
		mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
		mygoldtrade = 0;
	}

	else if (Head.startsWith("W"))
	{//取消交易
		IS_TRADING = false;
		myitem_tradeList.clear();
		mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
		mygoldtrade = 0;
	}
}

void Server::lssproto_CHAREFFECT_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

//自訂對話框收到按鈕消息
void Server::lssproto_CustomWN_recv(const QString& data)
{
	QStringList dataList = data.split(util::rexOR, Qt::SkipEmptyParts);
	if (dataList.size() != 4)
		return;

	int x = dataList.at(0).toInt();
	int y = dataList.at(1).toInt();
	BUTTON_TYPE button = static_cast<BUTTON_TYPE>(dataList[2].toInt());
	QString dataStr = dataList.at(3);
	int row = -1;
	bool ok = false;
	int tmp = dataStr.toInt(&ok);
	if (ok && tmp > 0)
	{
		row = dataStr.toInt();
	}
	//qDebug() << x << y << button << (row != -1 ? QString::number(row) : dataStr);
	customDialog = customdialog_t{ x, y, button, row };
	IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;
}

//自訂對話
void Server::lssproto_CustomTK_recv(const QString& data)
{
	QStringList dataList = data.split(util::rexOR, Qt::SkipEmptyParts);
	if (dataList.size() != 5)
		return;

	int x = dataList.at(0).toInt();
	int y = dataList.at(1).toInt();
	int color = dataList.at(2).toInt();
	int area = dataList.at(3).toInt();
	QString dataStr = dataList.at(4).simplified();
	QStringList args = dataStr.split(" ", Qt::SkipEmptyParts);
	if (args.isEmpty())
		return;
	int size = args.size();
	if (args.at(0).startsWith("//skup") && size == 5)
	{
		bool ok;
		int vit = args.at(1).toInt(&ok);
		if (!ok)
			return;
		int str = args.at(2).toInt(&ok);
		if (!ok)
			return;
		int tgh = args.at(3).toInt(&ok);
		if (!ok)
			return;
		int dex = args.at(4).toInt(&ok);
		if (!ok)
			return;

		if (skupFuture.isRunning())
			return;

		skupFuture = QtConcurrent::run([this, vit, str, tgh, dex]()
			{
				const QVector<int> vec = { vit, str, tgh, dex };
				int j = 0;
				for (int i = 0; i < 4; ++i)
				{
					j = vec.at(i);
					if (j <= 0)
						continue;

					if (!addPoint(i, j))
						return;
				}
			});
	}

	qDebug() << dataList;
}
#pragma endregion

#ifdef OCR_ENABLE
#include "webauthenticator.h"
bool Server::captchaOCR(QString* pmsg)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return false;

	QScreen* screen = QGuiApplication::primaryScreen();
	if (nullptr == screen)
	{
		announce("<ocr>screen pointer is nullptr");
		return false;
	}

	LPARAM data = MAKELPARAM(0, 0);
	injector.sendMessage(WM_MOUSEMOVE, NULL, data);
	QPixmap pixmap = screen->grabWindow(reinterpret_cast<WId>(injector.getProcessWindow()));
	QImage image = pixmap.toImage();

	//only take middle part of the image
	image = image.copy(269, 226, 102, 29);//368,253

#if 0
	QString randomHash = generateRandomHash();
	image.save(QString("D:/py/dddd_trainer/projects/antocode/image_set/%1_%2.png").arg(image_count++).arg(randomHash), "PNG");
	QString tempPath = QString("%1/%2.png").arg(QDir::tempPath()).arg(randomHash);
	QFile file(tempPath);
	if (file.exists())
		file.remove();
	image.save(tempPath, "PNG");
#endif

	QString errorMsg;
	QString gifCode;
	if (AntiCaptcha::httpPostCodeImage(image, &errorMsg, gifCode))
	{
		*pmsg = gifCode;
		announce("<ocr>success! result is:" + gifCode);
		return true;
	}
	else
		announce("<ocr>failed! error:" + errorMsg);

	return false;
}
#endif