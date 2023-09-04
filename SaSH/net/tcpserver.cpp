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

#include <spdlogger.hpp>
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

QString Server::makeStringFromEscaped(QString& src) const
{
	src.replace("\\n", "\n");
	src.replace("\\c", ",");
	src.replace("\\z", "|");
	src.replace("\\y", "\\");
	src = src.simplified();
	return src;

	//struct EscapeChar
	//{
	//	QChar escapechar;
	//	QChar escapedchar;
	//};

	//static const EscapeChar escapeChar[] = {
	//	{ QChar('n'), QChar('\n') },
	//	{ QChar('c'), QChar(',')},
	//	{ QChar('z'), QChar('|')},
	//	{ QChar('y'), QChar('\\') },
	//};

	//QString result;
	//int srclen = src.length();

	//for (int i = 0; i < srclen; ++i)
	//{
	//	if (src[i] == '\\' && i + 1 < srclen)
	//	{
	//		QChar nextChar = src[i + 1];

	//		bool isDBCSLeadByte = false;
	//		if (nextChar.isHighSurrogate() && i + 2 < srclen && src[i + 2].isLowSurrogate())
	//		{
	//			result.append(src.midRef(i, 2));
	//			i += 2;
	//			isDBCSLeadByte = true;
	//		}

	//		if (!isDBCSLeadByte)
	//		{
	//			for (int j = 0; j < sizeof(escapeChar) / sizeof(escapeChar[0]); ++j)
	//			{
	//				if (escapeChar[j].escapedchar == nextChar)
	//				{
	//					result.append(escapeChar[j].escapechar);
	//					i++;
	//					break;
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		result.append(src[i]);
	//	}
	//}

	//return result;
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
	oneRoundDurationTimer.start();
	normalDurationTimer.start();
	eottlTimer.start();
	connectingTimer.start();
	repTimer.start();

	Autil::util_Init();
	clearNetBuffer();

	sync_.setCancelOnWait(true);
	ayncBattleCommandSync.setCancelOnWait(true);

	mapAnalyzer.reset(new MapAnalyzer);

	Autil::PersonalKey.set("upupupupp");
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
	for (int i = 0; i < MAX_CHARACTER; ++i)
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
	for (int i = 0; i < MAX_MISSION; ++i)
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

	connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onClientReadyRead, Qt::QueuedConnection);
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
	appendReadBuf(badata);

	if (net_readbuf.isEmpty())
		return;

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

		// get line from read buffer
		if (!ba.isEmpty())
		{
			int ret = saDispatchMessage(ba.data());

			if (ret < 0)
			{
				//qDebug() << "************************ LSSPROTO_END ************************";
				//代表此段數據已到結尾
				Autil::util_Clear();
				break;
			}
			else if (ret == BC_NEED_TO_CLEAN || ret == BC_INVALID)
			{
				//qDebug() << "************************ CLEAR_BUFFER ************************";
				//錯誤的數據 或 需要清除緩存
				clearNetBuffer();
				break;
			}
		}
		else
		{
			//qDebug() << "************************ DONE_BUFFER ************************";
			//數據讀完了
			Autil::util_Clear();
		}
	}
}

//經由 handleData 調用同步解析數據
int Server::saDispatchMessage(char* encoded)
{
	int	func = 0, fieldcount = 0;
	int	iChecksum = 0, iChecksumrecv = 0;

	QByteArray raw(Autil::NETBUFSIZ, '\0');
	QByteArray data(Autil::NETDATASIZE, '\0');
	QByteArray result(Autil::NETDATASIZE, '\0');

	Autil::util_DecodeMessage(raw.data(), Autil::NETBUFSIZ, encoded);
	Autil::util_SplitMessage(raw.data(), Autil::NETBUFSIZ, const_cast<char*>(Autil::SEPARATOR));
	if (Autil::util_GetFunctionFromSlice(&func, &fieldcount) != 1)
		return 0;

	//qDebug() << "fun" << func << "fieldcount" << fieldcount;

	if (func == LSSPROTO_ERROR_RECV)
		return -1;

	SPD_LOG(g_logger_name, QString("[proto] lssproto func: %1").arg(func));

	switch (func)
	{
	case LSSPROTO_XYD_RECV: /*戰後刷新人物座標、方向2*/
	{
		int x = 0;
		int y = 0;
		int dir = 0;

		if (!Autil::util_Receive(&x, &y, &dir))
			return 0;

		qDebug() << "LSSPROTO_XYD_RECV" << "x" << x << "y" << y << "dir" << dir;
		lssproto_XYD_recv(QPoint(x, y), dir);
		break;
	}
	case LSSPROTO_EV_RECV: /*WRAP 4*/
	{
		int dialogid = 0;
		int result = 0;

		if (!Autil::util_Receive(&dialogid, &result))
			return 0;

		//qDebug() << "LSSPROTO_EV_RECV" << "dialogid" << dialogid << "result" << result;
		lssproto_EV_recv(dialogid, result);
		break;
	}
	case LSSPROTO_EN_RECV: /*Battle EncountFlag //開始戰鬥 7*/
	{
		int result = 0;
		int field = 0;

		if (!Autil::util_Receive(&result, &field))
			return 0;

		//qDebug() << "LSSPROTO_EN_RECV" << "result" << result << "field" << field;
		lssproto_EN_recv(result, field);
		break;
	}
	case LSSPROTO_RS_RECV: /*戰後獎勵 12*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_RS_RECV" << util::toUnicode(data.data());
		lssproto_RS_recv(data.data());
		break;
	}
	case LSSPROTO_RD_RECV:/*戰後經驗 13*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_RD_RECV" << util::toUnicode(data.data());
		lssproto_RD_recv(data.data());
		break;
	}
	case LSSPROTO_B_RECV: /*每回合開始的戰場資訊 15*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_B_RECV" << util::toUnicode(data.data());
		lssproto_B_recv(data.data());
		break;
	}
	case LSSPROTO_I_RECV: /*物品變動 22*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_I_RECV" << util::toUnicode(data.data());
		lssproto_I_recv(data.data());
		break;
	}
	case LSSPROTO_SI_RECV:/* 道具位置交換24*/
	{
		int fromindex;
		int toindex;

		if (!Autil::util_Receive(&fromindex, &toindex))
			return 0;

		//qDebug() << "LSSPROTO_SI_RECV" << "fromindex" << fromindex << "toindex" << toindex;
		lssproto_SI_recv(fromindex, toindex);
		break;
	}
	case LSSPROTO_MSG_RECV:/*收到郵件26*/
	{
		int aindex;
		int color;

		if (!Autil::util_Receive(&aindex, data.data(), &color))
			return 0;

		//qDebug() << "LSSPROTO_MSG_RECV" << util::toUnicode(data.data());
		lssproto_MSG_recv(aindex, data.data(), color);
		break;
	}
	case LSSPROTO_PME_RECV:/*28*/
	{
		int unitid;
		int graphicsno;
		int x;
		int y;
		int dir;
		int flg;
		int no;

		if (!Autil::util_Receive(&unitid, &graphicsno, &x, &y, &dir, &flg, &no, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_PME_RECV" << "unitid" << unitid << "graphicsno" << graphicsno <<
			//"x" << x << "y" << y << "dir" << dir << "flg" << flg << "no" << no << "cdata" << util::toUnicode(data.data());
		lssproto_PME_recv(unitid, graphicsno, QPoint(x, y), dir, flg, no, data.data());
		break;
	}
	case LSSPROTO_AB_RECV:/* 30*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_AB_RECV" << util::toUnicode(data.data());
		lssproto_AB_recv(data.data());
		break;
	}
	case LSSPROTO_ABI_RECV:/*名片數據31*/
	{
		int num;

		if (!Autil::util_Receive(&num, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_ABI_RECV" << "num" << num << "data" << util::toUnicode(data.data());
		lssproto_ABI_recv(num, data.data());
		break;
	}
	case LSSPROTO_TK_RECV: /*收到對話36*/
	{
		int index;
		int color;

		if (!Autil::util_Receive(&index, data.data(), &color))
			return 0;

		//qDebug() << "LSSPROTO_TK_RECV" << "index" << index << "message" << util::toUnicode(data.data()) << "color" << color;
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

		if (!Autil::util_Receive(&fl, &x1, &y1, &x2, &y2, &tilesum, &objsum, &eventsum, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_MC_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 <<
			//"tilesum" << tilesum << "objsum" << objsum << "eventsum" << eventsum << "data" << util::toUnicode(data.data());
		lssproto_MC_recv(fl, x1, y1, x2, y2, tilesum, objsum, eventsum, data.data());
		break;
	}
	case LSSPROTO_M_RECV: /*地圖數據更新，重新寫入地圖2 39*/
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;

		if (!Autil::util_Receive(&fl, &x1, &y1, &x2, &y2, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_M_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 << "data" << util::toUnicode(data.data());
		lssproto_M_recv(fl, x1, y1, x2, y2, data.data());
		break;
	}
	case LSSPROTO_C_RECV: /*服務端發送的靜態信息，可用於顯示玩家，其它玩家，公交，寵物等信息 41*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_C_RECV" << util::toUnicode(data.data());
		lssproto_C_recv(data.data());
		break;
	}
	case LSSPROTO_CA_RECV: /*//周圍人、NPC..等等狀態改變必定是 _C_recv已經新增過的單位 42*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CA_RECV" << util::toUnicode(data.data());
		lssproto_CA_recv(data.data());
		break;
	}
	case LSSPROTO_CD_RECV: /*刪除指定一個或多個周圍人、NPC單位 43*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CD_RECV" << util::toUnicode(data.data());
		lssproto_CD_recv(data.data());
		break;
	}
	case LSSPROTO_R_RECV:
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_R_RECV" << util::toUnicode(data.data());
		lssproto_R_recv(data.data());
		break;
	}
	case LSSPROTO_S_RECV: /*更新所有基礎資訊 46*/
	{
		if (!Autil::util_Receive(data.data()))
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

		if (!Autil::util_Receive(&category, &dx, &dy, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_D_RECV" << "category" << category << "dx" << dx << "dy" << dy << "data" << util::toUnicode(data.data());
		lssproto_D_recv(category, dx, dy, data.data());
		break;
	}
	case LSSPROTO_FS_RECV:/*開關切換 49*/
	{
		int flg;

		if (!Autil::util_Receive(&flg))
			return 0;

		//qDebug() << "LSSPROTO_FS_RECV" << "flg" << flg;
		lssproto_FS_recv(flg);
		break;
	}
	case LSSPROTO_HL_RECV:/*51*/
	{
		int flg;

		if (!Autil::util_Receive(&flg))
			return 0;

		//qDebug() << "LSSPROTO_HL_RECV" << "flg" << flg;
		lssproto_HL_recv(flg);
		break;
	}
	case LSSPROTO_PR_RECV:/*組隊變化 53*/
	{
		int request;
		int result;

		if (!Autil::util_Receive(&request, &result))
			return 0;

		//qDebug() << "LSSPROTO_PR_RECV" << "request" << request << "result" << result;
		lssproto_PR_recv(request, result);
		break;
	}
	case LSSPROTO_KS_RECV: /*寵物更換狀態55*/
	{
		int petarray;
		int result;

		if (!Autil::util_Receive(&petarray, &result))
			return 0;

		//qDebug() << "LSSPROTO_KS_RECV" << "petarray" << petarray << "result" << result;
		lssproto_KS_recv(petarray, result);
		break;
	}
	case LSSPROTO_PS_RECV:
	{
		int result;
		int havepetindex;
		int havepetskill;
		int toindex;

		if (!Autil::util_Receive(&result, &havepetindex, &havepetskill, &toindex))
			return 0;

		//qDebug() << "LSSPROTO_PS_RECV" << "result" << result << "havepetindex" << havepetindex << "havepetskill" << havepetskill << "toindex" << toindex;
		lssproto_PS_recv(result, havepetindex, havepetskill, toindex);
		break;
	}
	case LSSPROTO_SKUP_RECV: /*更新點數 63*/
	{
		int point;

		if (!Autil::util_Receive(&point))
			return 0;

		//qDebug() << "LSSPROTO_SKUP_RECV" << "point" << point;
		lssproto_SKUP_recv(point);
		break;
	}
	case LSSPROTO_WN_RECV:/*NPC對話框 66*/
	{
		int windowtype;
		int buttontype;
		int dialogid;
		int unitid;

		if (!Autil::util_Receive(&windowtype, &buttontype, &dialogid, &unitid, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_WN_RECV" << "windowtype" << windowtype << "buttontype" << buttontype << "dialogid" << dialogid << "unitid" << unitid << "data" << util::toUnicode(data.data());
		lssproto_WN_recv(windowtype, buttontype, dialogid, unitid, data.data());
		break;
	}
	case LSSPROTO_EF_RECV: /*天氣68*/
	{
		int effect;
		int level;

		if (!Autil::util_Receive(&effect, &level, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_EF_RECV" << "effect" << effect << "level" << level << "option" << util::toUnicode(data.data());
		lssproto_EF_recv(effect, level, data.data());
		break;
	}
	case LSSPROTO_SE_RECV:/*69*/
	{
		int x;
		int y;
		int senumber;
		int sw;

		if (!Autil::util_Receive(&x, &y, &senumber, &sw))
			return 0;

		//qDebug() << "LSSPROTO_SE_RECV" << "x" << x << "y" << y << "senumber" << senumber << "sw" << sw;
		lssproto_SE_recv(QPoint(x, y), senumber, sw);
		break;
	}
	case LSSPROTO_CLIENTLOGIN_RECV:/*選人畫面 72*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CLIENTLOGIN_RECV" << util::toUnicode(data.data());
		lssproto_ClientLogin_recv(data.data());

		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CREATENEWCHAR_RECV:/*人物新增74*/
	{
		if (!Autil::util_Receive(result.data(), data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CREATENEWCHAR_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CreateNewChar_recv(result.data(), data.data());
		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARDELETE_RECV:/*人物刪除 76*/
	{
		if (!Autil::util_Receive(result.data(), data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CHARDELETE_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharDelete_recv(result.data(), data.data());
		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARLOGIN_RECV: /*成功登入 78*/
	{
		if (!Autil::util_Receive(result.data(), data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CHARLOGIN_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharLogin_recv(result.data(), data.data());
		break;
	}
	case LSSPROTO_CHARLIST_RECV:/*選人頁面資訊 80*/
	{
		if (!Autil::util_Receive(result.data(), data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CHARLIST_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharList_recv(result.data(), data.data());

		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARLOGOUT_RECV:/*登出 82*/
	{
		if (!Autil::util_Receive(result.data(), data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CHARLOGOUT_RECV" << util::toUnicode(result) << util::toUnicode(data.data());
		lssproto_CharLogout_recv(result.data(), data.data());
		break;
	}
	case LSSPROTO_PROCGET_RECV:/*84*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//ebug() << "LSSPROTO_PROCGET_RECV" << util::toUnicode(data.data());
		lssproto_ProcGet_recv(data.data());
		break;
	}
	case LSSPROTO_PLAYERNUMGET_RECV:/*86*/
	{
		int logincount;
		int player;

		if (!Autil::util_Receive(&logincount, &player))
			return 0;

		//qDebug() << "LSSPROTO_PLAYERNUMGET_RECV" << "logincount:" << logincount << "player:" << player; //"logincount:%d player:%d\n
		lssproto_PlayerNumGet_recv(logincount, player);
		break;
	}
	case LSSPROTO_ECHO_RECV: /*伺服器定時ECHO "hoge" 88*/
	{

		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_ECHO_RECV" << util::toUnicode(data.data());
		lssproto_Echo_recv(data.data());
		break;
	}
	case LSSPROTO_NU_RECV: /*不知道幹嘛的 90*/
	{
		int AddCount;

		if (!Autil::util_Receive(&AddCount))
			return 0;

		//qDebug() << "LSSPROTO_NU_RECV" << "AddCount:" << AddCount;
		lssproto_NU_recv(AddCount);
		break;
	}
	case LSSPROTO_TD_RECV:/*92*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_TD_RECV" << util::toUnicode(data.data());
		lssproto_TD_recv(data.data());
		break;
	}
	case LSSPROTO_FM_RECV:/*家族頻道93*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;


		//qDebug() << "LSSPROTO_FM_RECV" << util::toUnicode(data.data());
		lssproto_FM_recv(data.data());
		break;
	}
	case LSSPROTO_WO_RECV:/*95*/
	{
		int effect;

		if (!Autil::util_Receive(&effect))
			return 0;

		//qDebug() << "LSSPROTO_WO_RECV" << "effect:" << effect;
		lssproto_WO_recv(effect);
		break;
	}
	case LSSPROTO_NC_RECV: /*沈默? 101* 戰鬥結束*/
	{
		int flg = 0;

		if (!Autil::util_Receive(&flg))
			return 0;

		//qDebug() << "LSSPROTO_NC_RECV" << "flg:" << flg;
		lssproto_NC_recv(flg);
		break;
	}
	case LSSPROTO_CS_RECV:/*固定客戶端的速度104*/
	{
		int deltimes = 0;

		if (!Autil::util_Receive(&deltimes))
			return 0;

		//qDebug() << "LSSPROTO_CS_RECV" << "deltimes:" << deltimes;
		lssproto_CS_recv(deltimes);
		break;
	}
	case LSSPROTO_PETST_RECV: /*寵物狀態改變 107*/
	{
		int petarray;
		int nresult;

		if (!Autil::util_Receive(&petarray, &nresult))
			return 0;

		//qDebug() << "LSSPROTO_PETST_RECV" << "petarray:" << petarray << "result:" << result;
		lssproto_PETST_recv(petarray, nresult);
		break;
	}
	case LSSPROTO_SPET_RECV: /*寵物更換狀態115*/
	{
		int standbypet;
		int nresult;

		if (!Autil::util_Receive(&standbypet, &nresult))
			return 0;

		//qDebug() << "LSSPROTO_SPET_RECV" << "standbypet:" << standbypet << "result:" << result;
		lssproto_SPET_recv(standbypet, nresult);
		break;
	}
	case LSSPROTO_JOBDAILY_RECV:/*任務日誌120*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;


		//qDebug() << "LSSPROTO_JOBDAILY_RECV" << util::toUnicode(data.data());
		lssproto_JOBDAILY_recv(data.data());
		break;
	}
	case LSSPROTO_TEACHER_SYSTEM_RECV:/*導師系統123*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_TEACHER_SYSTEM_RECV" << util::toUnicode(data.data());
		lssproto_TEACHER_SYSTEM_recv(data.data());
		break;
	}
	case LSSPROTO_FIREWORK_RECV:/*煙火?126*/
	{
		int iCharaindex, iType, iActionNum;

		if (!Autil::util_Receive(&iCharaindex, &iType, &iActionNum))
			return 0;

		//qDebug() << "LSSPROTO_FIREWORK_RECV" << "iCharaindex:" << iCharaindex << "iType:" << iType << "iActionNum:" << iActionNum;
		lssproto_Firework_recv(iCharaindex, iType, iActionNum);
		break;
	}
	case LSSPROTO_CHAREFFECT_RECV:/*146*/
	{
		if (!Autil::util_Receive(data.data()))
			return 0;

		//qDebug() << "LSSPROTO_CHAREFFECT_RECV" << util::toUnicode(data.data());
		lssproto_CHAREFFECT_recv(data.data());
		break;
	}
	case LSSPROTO_IMAGE_RECV:/*151 SE SO驗證圖*/
	{
		int x = 0;
		int y = 0;
		int z = 0;

		if (!Autil::util_Receive(data.data(), &x, &y, &z))
			return 0;

		break;
	}
	case LSSPROTO_DENGON_RECV:/*200*/
	{
		int coloer;
		int num;

		if (!Autil::util_Receive(data.data(), &coloer, &num))
			return 0;

		//qDebug() << "LSSPROTO_DENGON_RECV" << util::toUnicode(data.data()) << "coloer:" << coloer << "num:" << num;
		lssproto_DENGON_recv(data.data(), coloer, num);
		break;
	}
	case LSSPROTO_SAMENU_RECV:/*201*/
	{
		int count;

		if (!Autil::util_Receive(&count, data.data()))
			return 0;

		//qDebug() << "LSSPROTO_SAMENU_RECV" << "count:" << count << util::toUnicode(data.data());
		break;
	}
	case 220:
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
	else if (6 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLoginFailed);
		return util::kStatusLoginFailed;//簽入失敗
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
		return util::kStatusLogined;//已豋入(平時且無其他對話框或特殊場景)

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
	if (checkAND(pc.status, CHR_STATUS_LEADER) || checkAND(pc.status, CHR_STATUS_PARTY))
	{
		for (int i = 0; i < MAX_PARTY; ++i)
		{
			PARTY party = getParty(i);
			if (!party.valid)
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

	return mem::readString(hProcess, ptr, MAX_CHAT_BUFFER, false);
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
	QString newMemo = memo.simplified();

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
		QString itemMemo = pc.item[i].memo.simplified();

		if (itemName.isEmpty() || !pc.item[i].valid)
			continue;

		//說明為空
		if (memo.isEmpty())
		{
			if (isExact && (newName == itemName))
				v.append(i);
			else if (!isExact && (itemName.contains(newName)))
				v.append(i);
		}
		//道具名稱為空
		else if (name.isEmpty())
		{
			if (itemMemo.contains(newMemo))
				v.append(i);
		}
		//兩者都不為空
		else if (!itemName.isEmpty() && !itemMemo.isEmpty())
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
		if (itemName.isEmpty() || !pc.item[i].valid)
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
				if (petSkillName.isEmpty() || !petSkill[j][i].valid)
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
			if (petSkillName.isEmpty() || !petSkill[petIndex][i].valid)
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
		if (newPetName.isEmpty() || !_pet.valid)
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
		if (name.isEmpty() || !pc.item[i].valid)
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
		if (name.isEmpty() || !pc.item[i].valid)
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
		do
		{
			bool ok = false;
			QPoint point;
			point.setX(nameSrcList.at(0).simplified().toInt(&ok));
			if (!ok)
				break;

			point.setY(nameSrcList.at(1).simplified().toInt(&ok));
			if (!ok)
				break;

			for (const mapunit_t& it : units)
			{
				if (it.modelid == 0 || it.modelid == 9999)
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
		} while (false);
	}

	if (modelid != -1)
	{
		for (const mapunit_t& it : units)
		{
			if (it.modelid == 0 || it.modelid == 9999)
				continue;

			if (it.modelid == modelid)
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
				if (it.modelid == 0)
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
	QString freeName = pet[index].freeName.simplified();

	if (!name.isEmpty() && newCmpName == name)
		return true;
	if (!freeName.isEmpty() && newCmpName == freeName)
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

		if (!pc.item[i].name.isEmpty() && (pc.item[i].valid))
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
		if (!pc.item[i].name.isEmpty() && (pc.item[i].valid))
		{
			if (pc.item[i].name2.isEmpty())
				varList = { i - CHAR_EQUIPPLACENUM + 1, pc.item[i].name, pc.item[i].stack, pc.item[i].damage, pc.item[i].level, pc.item[i].memo };
			else
				varList = { i - CHAR_EQUIPPLACENUM + 1, QString("%1(%2)").arg(pc.item[i].name).arg(pc.item[i].name2), pc.item[i].stack, pc.item[i].damage, pc.item[i].level, pc.item[i].memo };
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
		pc.item[i].valid = mem::read<short>(hProcess, hModule + 0x422C028 + i * item_offest) > 0;
		if (pc.item[i].valid != 1)
		{
			pc.item[i] = {};
			continue;
		}

		pc.item[i].name = mem::readString(hProcess, hModule + 0x422C032 + i * item_offest, ITEM_NAME_LEN, true, false);
		pc.item[i].memo = mem::readString(hProcess, hModule + 0x422C060 + i * item_offest, ITEM_MEMO_LEN, true, false);
		if (i >= CHAR_EQUIPPLACENUM)
			pc.item[i].stack = mem::read<short>(hProcess, hModule + 0x422BF58 + i * item_offest);
		else
			pc.item[i].stack = 1;

		if (pc.item[i].stack == 0)
			pc.item[i].stack = 1;
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
	if (isInTeam == 1 && !checkAND(pc.status, CHR_STATUS_PARTY))
		pc.status |= CHR_STATUS_PARTY;
	else if (isInTeam == 0 && checkAND(pc.status, CHR_STATUS_PARTY))
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
				{ "fname", battle.freeName },
				{ "modelid", battle.modelid },
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

//刷新要顯示的戰鬥時間和相關數據
void Server::updateBattleTimeInfo()
{
	double time = battleDurationTimer.elapsed() / 1000.0;
	double cost = battle_one_round_time.load(std::memory_order_acquire) / 1000.0;
	double total_time = battle_total_time.load(std::memory_order_acquire) / 1000.0 / 60.0;

	QString battle_time_text = QString(QObject::tr("%1 count    no %2 round    duration: %3 sec    cost: %4 sec    total time: %5 minues"))
		.arg(battle_total.load(std::memory_order_acquire))
		.arg(battleCurrentRound.load(std::memory_order_acquire) + 1)
		.arg(QString::number(time, 'f', 3))
		.arg(QString::number(cost, 'f', 3))
		.arg(QString::number(total_time, 'f', 5));

	if (battle_time_text.isEmpty() || timeLabelContents != battle_time_text)
	{
		timeLabelContents = battle_time_text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateTimeLabelContents(battle_time_text);
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
	QWriteLocker lock(&worldStateLocker);
	Injector& injector = Injector::getInstance();
	mem::write<int>(injector.getProcess(), injector.getProcessModule() + kOffestWorldStatus, w);
}

void Server::setGameStatus(int g)
{
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

void Server::updateComboBoxList()
{
	PC pc = getPC();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QStringList itemList;
	for (const ITEM& it : pc.item)
	{
		if (it.name.isEmpty())
			continue;
		itemList.append(it.name);
	}

	emit signalDispatcher.updateComboBoxItemText(util::kComboBoxItem, itemList);

	QStringList magicNameList;
	for (int i = 0; i < MAX_MAGIC; ++i)
	{
		MAGIC magic = getMagic(i);
		magicNameList.append(magic.name);
	}
	for (int i = 0; i < MAX_PROFESSION_SKILL; ++i)
	{
		PROFESSION_SKILL profession_skill = getSkill(i);
		magicNameList.append(profession_skill.name);
	}
	emit signalDispatcher.updateComboBoxItemText(util::kComboBoxCharAction, magicNameList);

	int battlePetIndex = pc.battlePetNo;
	if (battlePetIndex >= 0)
	{
		QStringList skillNameList;
		for (int i = 0; i < MAX_SKILL; ++i)
		{
			PET_SKILL petSkill = getPetSkill(battlePetIndex, i);
			skillNameList.append(petSkill.name);
		}
		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxPetAction, skillNameList);
	}
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

	PC pc = getPC();
	int flg = pc.etcFlag;
	QString msg = "P|";
	if (mode == kTalkGlobal)
		msg += ("/XJ ");
	else if (mode == kTalkFamily)
		msg += ("/FM ");
	else if (mode == kTalkWorld)
		msg += ("/WD ");
	else if (mode == kTalkTeam)
	{
		int newflg = flg;
		if (!checkAND(newflg, PC_ETCFLAG_PARTY_CHAT))
		{
			newflg |= PC_ETCFLAG_PARTY_CHAT;
			setSwitcher(newflg);
		}
	}

	msg += text;
	std::string str = util::fromUnicode(msg);
	lssproto_TK_send(getPoint(), const_cast<char*>(str.c_str()), color, 3);
}

//創建人物
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
	//hometown: 薩姆0 漁村1 加加2 卡魯3
	if (dataplacenum != 0 && dataplacenum != 1)
		return;

	if (chartable[dataplacenum].valid)
		return;

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
	if (index < 0 || index > MAX_CHARACTER)
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

#include "macchanger.h"
#include "descrypt.h"
void Server::clientLogin(const QString& userName, const QString& password)
{
	std::string sname = util::fromUnicode(userName);
	std::string spassword = util::fromUnicode(password);
	MyMACAddr m;
	std::string mac = m.GenRandMAC();
	std::string ip = "192.168.1.1";
	int serverIndex = 0;

	char userId[32] = {};
	char userPassword[32] = {};

	_snprintf_s(userId, sizeof(userId), _TRUNCATE, "%s", sname.c_str());
	_snprintf_s(userPassword, sizeof(userPassword), _TRUNCATE, "%s", spassword.c_str());

	lssproto_ClientLogin_send(userId
		, userPassword
		, const_cast<char*>(mac.c_str())
		, serverIndex
		, const_cast<char*>(ip.c_str())
		, WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS);
}

void Server::playerLogin(int index)
{
	if (index < 0 || index >= MAX_CHARACTER)
		return;

	if (!chartable[index].valid)
		return;

	std::string name = util::fromUnicode(chartable[index].name);

	lssproto_CharLogin_send(const_cast<char*>(name.c_str()));
}

//登入
bool Server::login(int s)
{
	util::UnLoginStatus status = static_cast<util::UnLoginStatus>(s);
	Injector& injector = Injector::getInstance();
	HANDLE hProcess = injector.getProcess();
	DWORD hModule = injector.getProcessModule();

	int server = injector.getValueHash(util::kServerValue);
	int subserver = injector.getValueHash(util::kSubServerValue);
	int position = injector.getValueHash(util::kPositionValue);
	QString account = injector.getStringHash(util::kGameAccountString);
	QString password = injector.getStringHash(util::kGamePasswordString);

	const QString fileName(qgetenv("JSON_PATH"));
	util::Config config(fileName);
	QElapsedTimer timer; timer.start();

	switch (status)
	{
	case util::kStatusDisconnect:
	{
		IS_DISCONNECTED = true;
		QList<int> list = config.readArray<int>("System", "Login", "Disconnect");
		if (list.size() == 2)
			injector.leftDoubleClick(list.at(0), list.at(1));
		else
		{
			injector.leftDoubleClick(315, 270);
			config.writeArray<int>("System", "Login", "Disconnect", { 315, 270 });
		}
		break;
	}
	case util::kStatusLoginFailed:
	{
		QList<int> list = config.readArray<int>("System", "Login", "LoginFailed");
		if (list.size() == 2)
			injector.leftDoubleClick(list.at(0), list.at(1));
		else
		{
			injector.leftDoubleClick(315, 255);
			config.writeArray<int>("System", "Login", "LoginFailed", { 315, 255 });
		}
		break;
	}
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
		injector.mouseMove(0, 0);

		if (account.isEmpty())
		{
			QString acct = mem::readString(hProcess, hModule + kOffestAccount, 32);
			if (!acct.isEmpty())
				account = acct;
		}

		if (password.isEmpty())
		{
			QString pwd = mem::readString(hProcess, hModule + kOffestPassword, 32);
			if (!pwd.isEmpty())
				password = pwd;
		}

		mem::writeString(hProcess, hModule + kOffestAccount, account);
		mem::writeString(hProcess, hModule + kOffestPassword, password);

#ifndef USE_MOUSE
		std::string saccount = util::fromUnicode(account);
		std::string spassword = util::fromUnicode(password);

		//sa_8001.exe+2086A - 09 09                 - or [ecx],ecx
		char userAccount[32] = {};
		char userPassword[32] = {};
		_snprintf_s(userAccount, sizeof(userAccount), _TRUNCATE, "%s", saccount.c_str());
		sacrypt::ecb_crypt("f;encor1c", userAccount, sizeof(userAccount), sacrypt::DES_DECRYPT);
		_snprintf_s(userPassword, sizeof(userPassword), _TRUNCATE, "%s", spassword.c_str());
		sacrypt::ecb_crypt("f;encor1c", userPassword, sizeof(userPassword), sacrypt::DES_DECRYPT);
		mem::write(hProcess, hModule + kOffestAccountECB, userAccount, sizeof(userAccount));
		mem::write(hProcess, hModule + kOffestPasswordECB, userPassword, sizeof(userPassword));

		/*
		sa_8001.exe+206F1 - EB 04                 - jmp sa_8001.exe+206F7
		sa_8001.exe+206F3 - 90                    - nop
		sa_8001.exe+206F4 - 90                    - nop
		sa_8001.exe+206F5 - 90                    - nop
		sa_8001.exe+206F6 - 90                    - nop
		*/
		mem::write(hProcess, hModule + 0x206F1, const_cast<char*>("\xEB\x04\x90\x90\x90\x90"), 6);//進入OK點擊事件

		timer.restart();
		//bool ok = true;
		for (;;)
		{
			if (getWorldStatus() == 2)
				break;

			if (timer.hasExpired(1000))
			{
				//ok = false;
				break;
			}

			if (isInterruptionRequested())
				return false;

			QThread::msleep(100);
		}

		//sa_8001.exe+206F1 - 0F85 1A020000         - jne sa_8001.exe+20911
		mem::write(hProcess, hModule + 0x206F1, const_cast<char*>("\x0F\x85\x1A\x02\x00\x00"), 6);//還原OK點擊事件

#else
		if (!ok)
		{
			QList<int> list = config.readArray<int>("System", "Login", "OK");
			if (list.size() == 2)
				injector.leftDoubleClick(list.at(0), list.at(1));
			else
			{
				injector.leftDoubleClick(380, 310);
				config.writeArray<int>("System", "Login", "OK", { 380, 310 });
			}
		}
#endif

		break;
	}
	case util::kStatusSelectServer:
	{
		if (server < 0 && server >= 15)
			break;

		injector.mouseMove(0, 0);

#ifndef USE_MOUSE
		/*
		sa_8001.exe+21536 - B8 00000000           - mov eax,00000000 { 0 }
		sa_8001.exe+2153B - EB 07                 - jmp sa_8001.exe+21544
		sa_8001.exe+2153D - 90                    - nop
		*/
		BYTE code[8] = { 0xB8, static_cast<BYTE>(server), 0x00, 0x00, 0x00, 0xEB, 0x07, 0x90 };
		mem::write(hProcess, hModule + 0x21536, code, sizeof(code));//進入伺服器點擊事件

		timer.restart();
		for (;;)
		{
			if (getGameStatus() == 3)
				break;

			if (timer.hasExpired(1000))
				break;

			if (isInterruptionRequested())
				return false;

			QThread::msleep(100);
		}

		/*
		sa_8001.exe+21536 - 0F8C 91000000         - jl sa_8001.exe+215CD
		sa_8001.exe+2153C - 3B C1                 - cmp eax,ecx
		*/
		mem::write(hProcess, hModule + 0x21536, const_cast<char*>("\x0F\x8C\x91\x00\x00\x00\x3B\xC1"), 8);//還原伺服器點擊事件
#else
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
#endif
		break;
	}
	case util::kStatusSelectSubServer:
	{
		if (subserver < 0 || subserver >= 15)
			break;

		injector.mouseMove(0, 0);

		int serverIndex = mem::read<int>(hProcess, hModule + kOffestServerIndex);

#ifndef USE_MOUSE
		/*
		sa_8001.exe+2186C - 8D 0C D2              - lea ecx,[edx+edx*8]
		sa_8001.exe+2186F - C1 E1 03              - shl ecx,03 { 3 }

		sa_8001.exe+21881 - 66 8B 89 2CEDEB04     - mov cx,[ecx+sa_8001.exe+4ABED2C]
		sa_8001.exe+21888 - 66 03 C8              - add cx,ax

		sa_8001.exe+2189B - 66 89 0D 88424C00     - mov [sa_8001.exe+C4288],cx { (23) }

		*/

		int ecxValue = serverIndex + (serverIndex * 8);
		ecxValue <<= 3;
		short cxValue = mem::read<short>(hProcess, ecxValue + hModule + 0x4ABED2C);
		cxValue += static_cast<short>(subserver);

		mem::write<short>(hProcess, hModule + 0xC4288, cxValue);//選擇伺服器+分流

		/*
		sa_8001.exe+2189B - 90                    - nop
		sa_8001.exe+2189C - 90                    - nop
		sa_8001.exe+2189D - 90                    - nop
		sa_8001.exe+2189E - 90                    - nop
		sa_8001.exe+2189F - 90                    - nop
		sa_8001.exe+218A0 - 90                    - nop
		sa_8001.exe+218A1 - 90                    - nop
		*/
		mem::write(hProcess, hModule + 0x2189B, const_cast<char*>("\x90\x90\x90\x90\x90\x90\x90"), 7);//防止被複寫

		//sa_8001.exe+21A53 - 0FBF 05 88424C00      - movsx eax,word ptr [sa_8001.exe+C4288] { (0) } << 這裡決定最後登入的伺服器

		/*
		sa_8001.exe+21864 - BA 00000000           - mov edx,00000007 { 0 }
		sa_8001.exe+21869 - 90                    - nop
		sa_8001.exe+2186A - 90                    - nop
		sa_8001.exe+2186B - 90                    - nop
		*/
		BYTE bser = static_cast<BYTE>(serverIndex) + 1ui8;
		BYTE code[8] = { 0xBA, bser, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90 };
		mem::write(hProcess, hModule + 0x21864, code, sizeof(code));//進入分流點擊事件

		timer.restart();
		connectingTimer.restart();
		for (;;)
		{
			if (getGameStatus() > 3)
				break;

			if (timer.hasExpired(1000))
				break;

			if (isInterruptionRequested())
				return false;

			QThread::msleep(100);
		}

		/*
		sa_8001.exe+21864 - 3B C6                 - cmp eax,esi
		sa_8001.exe+21866 - 0F8C 97000000         - jl sa_8001.exe+21903
		*/
		mem::write(hProcess, hModule + 0x21864, const_cast<char*>("\x3B\xC6\x0F\x8C\x97\x00\x00\x00"), 8);//還原分流點擊事件

		//sa_8001.exe+2189B - 66 89 0D 88424C00     - mov [sa_8001.exe+C4288],cx { (23) }
		mem::write(hProcess, hModule + 0x2189B, const_cast<char*>("\x66\x89\x0D\x88\x42\x4C\x00"), 7);//還原複寫伺服器+分流
#else
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
		}
#endif
		break;
	}
	case util::kStatusSelectCharacter:
	{
		if (position < 0 || position >= MAX_CHARACTER)
			break;

		if (!chartable[position].valid)
			break;

		//#ifndef USE_MOUSE
				//playerLogin(position);
				//setGameStatus(1);
				//setWorldStatus(5);
				//QThread::msleep(1000);
		//#else
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

		injector.leftDoubleClick(x, y);
		//#endif
		break;
	}
	case util::kStatusConnecting:
	{
		if (connectingTimer.hasExpired(3000))
		{
			setWorldStatus(7);
			setGameStatus(0);
			connectingTimer.restart();
		}
		break;
	}
	case util::kStatusLogined:
	{
		IS_DISCONNECTED = false;
		return true;
	}
	default:
		break;
	}
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

void Server::press(BUTTON_TYPE select, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

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
		if (checkAND(dialog.buttontype, BUTTON_OK))
			select = BUTTON_OK;
		else if (checkAND(dialog.buttontype, BUTTON_YES))
			select = BUTTON_YES;
		else if (checkAND(dialog.buttontype, BUTTON_NEXT))
			select = BUTTON_NEXT;
		else if (checkAND(dialog.buttontype, BUTTON_PREVIOUS))
			select = BUTTON_PREVIOUS;
		else if (checkAND(dialog.buttontype, BUTTON_NO))
			select = BUTTON_NO;
		else if (checkAND(dialog.buttontype, BUTTON_CANCEL))
			select = BUTTON_CANCEL;
	}

	lssproto_WN_send(getPoint(), dialogid, unitid, select, const_cast<char*>(data));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

void Server::press(int row, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;
	QString qrow = QString::number(row);
	std::string srow = util::fromUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//買東西
void Server::buy(int index, int amt, int dialogid, int unitid)
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
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qrow = QString("%1\\z%2").arg(index + 1).arg(amt);
	std::string srow = util::fromUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//賣東西
void Server::sell(const QString& name, const QString& memo, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (name.isEmpty())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QVector<int> indexs;
	if (!getItemIndexsByName(name, memo, &indexs, CHAR_EQUIPPLACENUM))
		return;

	sell(indexs, dialogid, unitid);
}

//賣東西
void Server::sell(int index, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (index < 0 || index >= MAX_ITEM)
		return;

	PC pc = getPC();
	if (pc.item[index].name.isEmpty() || !pc.item[index].valid)
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qrow = QString("%1\\z%2").arg(index).arg(pc.item[index].stack);
	std::string srow = util::fromUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

//賣東西
void Server::sell(const QVector<int>& indexs, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (indexs.isEmpty())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QStringList list;
	for (const int it : indexs)
	{
		if (it < 0 || it >= MAX_ITEM)
			continue;

		sell(it, dialogid, unitid);
	}
}

//寵物學技能
void Server::learn(int skillIndex, int petIndex, int spot, int dialogid, int unitid)
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
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;
	//8\z3\z3\z1000 技能
	QString qrow = QString("%1\\z%3\\z%3\\z%4").arg(skillIndex + 1).arg(petIndex + 1).arg(spot + 1).arg(0);
	std::string srow = util::fromUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	Injector& injector = Injector::getInstance();
	injector.sendMessage(Injector::kDistoryDialog, NULL, NULL);
}

void Server::depositItem(int itemIndex, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = QString::number(itemIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::withdrawItem(int itemIndex, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = QString::number(itemIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::depositPet(int petIndex, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = QString::number(petIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::withdrawPet(int petIndex, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = QString::number(petIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

//遊戲對話框輸入文字送出
void Server::inputtext(const QString& text, int dialogid, int unitid)
{
	if (!getOnlineFlag())
		return;

	if (getBattleFlag())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;
	if (unitid == -1)
		unitid = dialog.unitid;
	std::string s = util::fromUnicode(text);

	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_OK, const_cast<char*>(s.c_str()));
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

	std::string scode = util::fromUnicode(code);
	lssproto_WN_send(getPoint(), kDialogSecurityCode, -1, NULL, const_cast<char*>(scode.c_str()));
}

void Server::windowPacket(const QString& command, int dialogid, int unitid)
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
	lssproto_WN_send(getPoint(), dialogid, unitid, NULL, const_cast<char*>(s.c_str()));
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
	else if (!getPC().item[magicIndex].valid)
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

	if (party[n].valid)
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
			if (!addressBook[i].valid)
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

	if (!addressBook[index].valid)
		return;

	std::string sstr = util::fromUnicode(text);
	if (itemName.isEmpty() && itemMemo.isEmpty() && (petIndex < 0 || petIndex > MAX_PET))
	{
		lssproto_MSG_send(index, const_cast<char*>(sstr.c_str()), NULL);
	}
	else
	{
		if (!addressBook[index].onlineFlag)
			return;

		int itemIndex = getItemIndexByName(itemName, true, itemMemo, CHAR_EQUIPPLACENUM);
		if (itemIndex < 0 || itemIndex >= MAX_ITEM)
			return;

		PET pet = getPet(petIndex);
		if (!pet.valid)
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
	std::string sstr = util::fromUnicode(str);
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
		short dialogid = 0;
	}ev;

	if (mem::read(injector.getProcess(), injector.getProcessModule() + kOffestEV, sizeof(ev), &ev))
		lssproto_EV_send(ev.ev, ev.dialogid, getPoint(), -1);
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

	std::string sdir = util::fromUnicode(dir);
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
	std::string sdirStr = util::fromUnicode(dirStr.toUpper());
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
	std::string sdirStr = util::fromUnicode(qdirStr.toUpper());
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

	if (swapitemModeFlag == 0 || !deepSort)
	{
		QMutexLocker lock(&swapItemMutex_);
		if (deepSort)
			swapitemModeFlag = 1;

		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (!pc.item[i].valid)
					continue;

				//if (!isItemStackable(pc.item[i].sendFlag))
				//	continue;

				if (pc.item[i].name.isEmpty())
					continue;

				if (pc.item[i].name != pc.item[j].name)
					continue;

				if (pc.item[i].memo != pc.item[j].memo)
					continue;

				if (pc.item[i].modelid != pc.item[j].modelid)
					continue;

				if (pc.maxload <= 0)
					continue;

				QString key = QString("%1|%2|%3").arg(pc.item[j].name).arg(pc.item[j].memo).arg(pc.item[j].modelid);
				if (pc.item[j].stack > 1 && !itemStackFlagHash.contains(key))
					itemStackFlagHash.insert(key, true);
				else
				{
					swapItem(i, j);
					itemStackFlagHash.insert(key, false);
					continue;
				}

				if (itemStackFlagHash.contains(key) && !itemStackFlagHash.value(key) && pc.item[j].stack == 1)
					continue;

				if (pc.item[j].stack >= pc.maxload)
				{
					pc.maxload = pc.item[j].stack;
					if (pc.item[j].stack != pc.item[j].maxStack)
					{
						pc.item[j].maxStack = pc.item[j].stack;
						swapItem(i, j);
					}
					continue;
				}

				if (pc.item[i].stack > pc.item[j].stack)
					continue;

				pc.item[j].maxStack = pc.item[j].stack;

				swapItem(i, j);
			}
		}
	}
	else if (swapitemModeFlag == 1 && deepSort)
	{
		QMutexLocker lock(&swapItemMutex_);
		swapitemModeFlag = 2;

		//補齊  item[i].valid == false 的空格
		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (!pc.item[i].valid)
					continue;

				if (!pc.item[j].valid)
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
				if (!pc.item[i].valid)
					continue;

				if (!pc.item[j].valid)
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
				else if (pc.item[i].modelid != pc.item[j].modelid)
				{
					if (pc.item[i].modelid < pc.item[j].modelid)
					{
						swapItem(i, j);
					}
				}
				//數量少的放前面
				else if (pc.item[i].stack < pc.item[j].stack)
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
	if (pc.item[index].name.isEmpty() || !pc.item[index].valid)
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
	std::string str = util::fromUnicode(qstr);
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
	std::string str = util::fromUnicode(qstr);
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
	std::string str = util::fromUnicode(qstr);
	lssproto_FM_send(const_cast<char*>(str.c_str()));
}

void Server::tradeComfirm(const QString& name)
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
		int stack = pc.item[i].stack;
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
			if (!it.valid)
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
			opptradelist.append(QString("I|%1").arg(it.itemIndex));
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
	if (!battleState)
		return;

	if (battleDurationTimer.elapsed() > 0ll)
		battle_total_time.fetch_add(battleDurationTimer.elapsed(), std::memory_order_release);

	updateBattleTimeInfo();

	normalDurationTimer.restart();
	ayncBattleCommandFlag.store(true, std::memory_order_release);
	ayncBattleCommandSync.clearFutures();
	ayncBattleCommandFlag.store(false, std::memory_order_release);

	lssproto_EO_send(0);

	setBattleFlag(false);

	if (getWorldStatus() == 10)
		setGameStatus(7);
}

inline bool Server::checkFlagState(int pos)
{
	if (pos < 0 || pos >= MAX_ENEMY)
		return false;
	return checkAND(battleCurrentAnimeFlag, 1 << pos);
}

//異步處理自動/快速戰鬥邏輯和發送封包
void Server::doBattleWork(bool async)
{
	Injector& injector = Injector::getInstance();
	bool fastChecked = injector.getEnableHash(util::kFastBattleEnable);
	bool normalChecked = injector.getEnableHash(util::kAutoBattleEnable);
	fastChecked = fastChecked || (normalChecked && getWorldStatus() == 9);
	normalChecked = normalChecked || (fastChecked && getWorldStatus() == 10);
	if (!fastChecked && !normalChecked)
		return;

	//if (async || (normalChecked && checkWG(10, 4)))
	//	QtConcurrent::run(this, &Server::asyncBattleAction);
	//else if (!async && getWorldStatus() == 9)
	//	syncBattleAction();

	QtConcurrent::run(this, &Server::asyncBattleAction);
}

void Server::syncBattleAction()
{
	Injector& injector = Injector::getInstance();
	battledata_t bt = getBattleData();
	if (!bt.charAlreadyAction)
	{
		bt.charAlreadyAction = true;

		//戰鬥延時
		int delay = injector.getValueHash(util::kBattleActionDelayValue);
		if (delay > 0)
		{
			if (delay > 1000)
			{
				int maxDelaySize = delay / 1000;
				for (int i = 0; i < maxDelaySize; ++i)
				{
					QThread::msleep(1000);
					if (isInterruptionRequested())
						return;
				}

				if (delay % 1000 > 0)
					QThread::msleep(delay % 1000);
			}
			else
				QThread::msleep(delay);
		}

		playerDoBattleWork(bt);
	}

	if (!bt.petAlreadyAction)
	{
		bt.petAlreadyAction = true;
		petDoBattleWork(bt);
	}

	if (injector.getEnableHash(util::kBattleAutoEOEnable))
		lssproto_EO_send(0);
	isEnemyAllReady.store(false, std::memory_order_release);
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
			//announce("[async battle] 从外部中断的战斗等待", 7);
			return false;
		}

		if (!getOnlineFlag())
		{
			//announce("[async battle] 人物不在线上，忽略动作", 7);
			return false;
		}

		if (!getBattleFlag())
		{
			//announce("[async battle] 人物不在战斗中，忽略动作", 7);
			return false;
		}

		//if (!isEnemyAllReady.load(std::memory_order_acquire))
		//{
		//	//announce("[async battle] 敌方尚未准备完成，忽略动作", 7);
		//	return false;
		//}

		if (!injector.getEnableHash(util::kAutoBattleEnable) && !injector.getEnableHash(util::kFastBattleEnable))
		{
			//announce("[async battle] 快战或自动战斗没有开启，忽略动作", 7);
			return false;
		}

		return true;
	};

	if (!checkAllFlags())
		return;

	//自動戰鬥打開 或 快速戰鬥打開且處於戰鬥場景
	bool fastChecked = injector.getEnableHash(util::kFastBattleEnable);
	bool normalChecked = injector.getEnableHash(util::kAutoBattleEnable);
	fastChecked = fastChecked || (normalChecked && getWorldStatus() == 9);
	normalChecked = normalChecked || (fastChecked && getWorldStatus() == 10);
	if (normalChecked && !checkWG(10, 4))
	{
		//announce(QString("[async battle] 画面不对,当前游戏状态[%1]，画面状态[%2]").arg(getWorldStatus()).arg(getGameStatus()), 7);
		return;
	}

	auto delay = [&checkAllFlags, &injector]()
	{
		//戰鬥延時
		int delay = injector.getValueHash(util::kBattleActionDelayValue);
		if (delay <= 0)
			return;

		//announce(QString("[async battle] 战斗 %1 开始延时 %2 毫秒").arg(name).arg(delay), 6);

		if (delay > 1000)
		{
			int maxDelaySize = delay / 1000;
			for (int i = 0; i < maxDelaySize; ++i)
			{
				QThread::msleep(1000);
				if (!checkAllFlags())
					return;
			}

			if (delay % 1000 > 0)
				QThread::msleep(delay % 1000);
		}
		else
			QThread::msleep(delay);
	};

	auto setCurrentRoundEnd = [this, &injector, normalChecked]()
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

		//這里不發的話一般戰鬥、和快戰都不會再收到後續的封包 (應該?)
		if (injector.getEnableHash(util::kBattleAutoEOEnable))
			lssproto_EO_send(0);
		//lssproto_Echo_send(const_cast<char*>("hoge"));
		isEnemyAllReady.store(false, std::memory_order_release);
	};

	battledata_t bt = getBattleData();
	//人物和宠物分开发 TODO 修正多个BA人物多次发出战斗指令的问题
	if (!bt.charAlreadyAction)//!checkFlagState(BattleMyNo) &&
	{
		bt.charAlreadyAction = true;
		//announce(QString("[async battle] 准备发出人物战斗指令"));
		delay();
		//解析人物战斗逻辑并发送指令
		playerDoBattleWork(bt);
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
	if (!bt.petAlreadyAction)//!checkFlagState(BattleMyNo + 5) &&
	{
		bt.petAlreadyAction = true;
		//announce(QString("[async battle] 准备发出宠物战斗指令"));
		petDoBattleWork(bt);
		//announce("[async battle] 宠物战斗指令发送完毕");
		setCurrentRoundEnd();
	}
}

//人物戰鬥
int Server::playerDoBattleWork(const battledata_t& bt)
{
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

		handlePlayerBattleLogics(bt);

	} while (false);

	//if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
	//	mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//else
	//

	//
	return 1;
}

//寵物戰鬥
int Server::petDoBattleWork(const battledata_t& bt)
{
	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return 0;

	Injector& injector = Injector::getInstance();
	do
	{
		//自動逃跑
		if (hasUnMoveableStatue(bt.pet.status) || injector.getEnableHash(util::kAutoEscapeEnable) || petEscapeEnableTempFlag)
		{
			sendBattlePetDoNothing();
			break;
		}

		handlePetBattleLogics(bt);

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
	if (!checkAND(pc.status, CHR_STATUS_PARTY) && !checkAND(pc.status, CHR_STATUS_LEADER))
		return false;

	for (int i = 0; i < MAX_PARTY; ++i)
	{
		if (party[i].hpPercent < cmpvalue && party[i].level > 0 && party[i].maxHp > 0 && party[i].valid)
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
		if ((pet[i].level <= 0) || (pet[i].maxHp <= 0) || (!pet[i].valid))
			return false;
	}

	return true;
}

//人物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
void Server::handlePlayerBattleLogics(const battledata_t& bt)
{
	using namespace util;
	Injector& injector = Injector::getInstance();

	tempCatchPetTargetIndex = -1;
	petEscapeEnableTempFlag = false; //用於在必要的時候切換戰寵動作為逃跑模式

	QStringList items;

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

	//自動換寵
	do
	{
		bool autoSwitch = injector.getEnableHash(util::kBattleAutoSwitchEnable);
		if (!autoSwitch)
			break;

		PC pc = getPC();

		if (bt.objects[BattleMyNo + 5].modelid > 0 || !bt.objects[BattleMyNo + 5].name.isEmpty())
			break;

		int petIndex = -1;
		for (int i = 0; i < MAX_PET; ++i)
		{
			PET _pet = pet[i];
			if (_pet.level <= 0 || _pet.maxHp <= 0 || _pet.hp < 1 || _pet.modelid == 0 || _pet.name.isEmpty())
				continue;

			if (i == pc.battlePetNo)
				continue;

			petIndex = i;
		}

		if (petIndex == -1)
			break;

		sendBattlePlayerSwitchPetAct(petIndex);
		return;

	} while (false);

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
		if (checkAND(targetFlags, kSelectPet))
		{
			if (checkPetHp(NULL, &tempTarget, true))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
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
		if (checkAND(targetFlags, kSelectPet))
		{
			if (checkPetHp(NULL, &tempTarget, true))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
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

		if (atRoundIndex != battleCurrentRound.load(std::memory_order_acquire) + 1)
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
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target + 20;

		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target + 20;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = BattleMyNo;
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = BattleMyNo + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = 20;
		}
		else if (checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

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
		if ((battleCurrentRound.load(std::memory_order_acquire) + 1) % round)
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
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target + 20;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target + 20;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = BattleMyNo;
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = BattleMyNo + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = 20;
		}
		else if (checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

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

		if (checkAND(targetFlags, kSelectSelf))
		{
			if (checkPlayerHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectPet))
			{
				if (checkPetHp(petPercent, &tempTarget))
				{
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
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
		if (checkAND(targetFlags, kSelectSelf))
		{
			if (checkPlayerHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectPet))
			{
				if (checkPetHp(petPercent, &tempTarget))
				{
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
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
			itemIndex = getItemIndexByName(u8"?肉", false, u8"耐久力", CHAR_EQUIPPLACENUM);
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
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target + 20;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target + 20;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = BattleMyNo;
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = BattleMyNo + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = 20;
		}
		else if (checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

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
void Server::handlePetBattleLogics(const battledata_t& bt)
{
	using namespace util;
	Injector& injector = Injector::getInstance();

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
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = BattleMyNo;
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = BattleMyNo + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = 20;
		}
		else if (checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

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
		if ((battleCurrentRound.load(std::memory_order_acquire) + 1) % round)
		{
			break;
		}

		unsigned int targetFlags = injector.getValueHash(util::kBattlePetCrossActionTargetValue);
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = BattleMyNo;
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = BattleMyNo + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = 20;
		}
		else if (checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

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
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + 20;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + 20;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = BattleMyNo;
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = BattleMyNo + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = 20;
		}
		else if (checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

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
	if (magic[magicIndex].costmp > pc.mp)
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
			if ((obj.hp > 0) && (obj.level > 0) && (obj.maxHp > 0) && (obj.modelid > 0) && (obj.pos == it)
				&& !checkAND(obj.status, BC_FLG_DEAD) && !checkAND(obj.status, BC_FLG_HIDE))
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
bool Server::matchBattleEnemyByName(const QString& name, bool isExact, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const
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
bool Server::matchBattleEnemyByLevel(int level, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const
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
bool Server::matchBattleEnemyByMaxHp(int maxHp, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const
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
		int max = MAX_ENEMY;
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

	const QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
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
		QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
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
		QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
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
		QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
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
	QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
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

	if (!pet[petIndex].valid)
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
			name = !obj.name.isEmpty() ? obj.name : obj.freeName;
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
			if (party[i].name.isEmpty() || (!party[i].valid) || (party[i].maxHp <= 0))
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
				if (party[i].valid)
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
void Server::lssproto_EV_recv(int dialogid, int result)
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

	injector.setEnableHash(util::kSwitcherTeamEnable, checkAND(flg, PC_ETCFLAG_GROUP));//組隊開關
	injector.setEnableHash(util::kSwitcherPKEnable, checkAND(flg, PC_ETCFLAG_PK));//決鬥開關
	injector.setEnableHash(util::kSwitcherCardEnable, checkAND(flg, PC_ETCFLAG_CARD));//名片開關
	injector.setEnableHash(util::kSwitcherTradeEnable, checkAND(flg, PC_ETCFLAG_TRADE));//交易開關
	injector.setEnableHash(util::kSwitcherWorldEnable, checkAND(flg, PC_ETCFLAG_WORLD));//世界頻道開關
	injector.setEnableHash(util::kSwitcherGroupEnable, checkAND(flg, PC_ETCFLAG_PARTY_CHAT));//組隊頻道開關
	injector.setEnableHash(util::kSwitcherFamilyEnable, checkAND(flg, PC_ETCFLAG_FM));//家族頻道開關
	injector.setEnableHash(util::kSwitcherJobEnable, checkAND(flg, PC_ETCFLAG_JOB));//職業頻道開關

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
	bool valid;
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
		valid = getIntegerToken(data, "|", no + 1) > 0;
		if (!valid)
		{
#if 0
			if (addressBook[i].valid)
#else
			if (!MailHistory[i].dateStr[MAIL_MAX_HISTORY - 1].isEmpty())
#endif
			{
				MailHistory[i] = MailHistory[i];

			}
			addressBook[i].valid = false;
			addressBook[i].name.clear();
			addressBook[i] = {};
			continue;
		}

#ifdef _EXTEND_AB
		if (i == MAX_ADR_BOOK - 1)
			addressBook[i].valid = valid;
		else
			addressBook[i].valid = true;
#else
		addressBook[i].valid = true;
#endif

		flag = getStringToken(data, "|", no + 2, name);

		if (flag == 1)
			break;

		makeStringFromEscaped(name);
		addressBook[i].name = name;
		addressBook[i].level = getIntegerToken(data, "|", no + 3);
		addressBook[i].dp = getIntegerToken(data, "|", no + 4);
		addressBook[i].onlineFlag = (short)getIntegerToken(data, "|", no + 5);
		addressBook[i].modelid = getIntegerToken(data, "|", no + 6);
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
	bool valid;
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	QString planetid[8];
	int j;
#endif

	if (num >= MAX_ADR_BOOK)
		return;

	valid = getIntegerToken(data, "|", 1) > 0;

	if (!valid)
	{
		if (MailHistory[num].dateStr[MAIL_MAX_HISTORY - 1][0] != 0)
		{
			MailHistory[num].clear();
		}
		addressBook[num].valid = valid;
		addressBook[num].name[0] = '\0';
		return;
	}

#ifdef _EXTEND_AB
	if (num == MAX_ADR_BOOK - 1)
		addressBook[num].valid = valid;
	else
		addressBook[num].valid = true;
#else
	addressBook[num].valid = valid;
#endif

	getStringToken(data, "|", 2, name);
	makeStringFromEscaped(name);

	addressBook[num].name = name;
	addressBook[num].level = getIntegerToken(data, "|", 3);
	addressBook[num].dp = getIntegerToken(data, "|", 4);
	addressBook[num].onlineFlag = (short)getIntegerToken(data, "|", 5);
	addressBook[num].modelid = getIntegerToken(data, "|", 6);
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

	appendList(item);
	getStringToken(token, "|", 2, item);
	makeStringFromEscaped(item);

	appendList(item);
	getStringToken(token, "|", 3, item);
	makeStringFromEscaped(item);

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
	updateComboBoxList();
}

//道具數據改變
void Server::lssproto_I_recv(char* cdata)
{
	PC pc = getPC();

	{
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
			if (name.isEmpty())
			{
				pc.item[i].valid = false;
				pc.item[i].name.clear();
				pc.item[i] = {};
				refreshItemInfo(i);
				continue;
			}
			pc.item[i].valid = true;
			pc.item[i].name = name;
			getStringToken(data, "|", no + 3, name2);//第二個道具名
			makeStringFromEscaped(name2);

			pc.item[i].name2 = name2;
			pc.item[i].color = getIntegerToken(data, "|", no + 4);//顏色
			if (pc.item[i].color < 0)
				pc.item[i].color = 0;
			getStringToken(data, "|", no + 5, memo);//道具介紹
			makeStringFromEscaped(memo);

			pc.item[i].memo = memo;
			pc.item[i].modelid = getIntegerToken(data, "|", no + 6);//道具形像
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

				pc.item[i].stack = pile.toInt();
				if (pc.item[i].valid && pc.item[i].stack == 0)
					pc.item[i].stack = 1;
			}
#endif

#ifdef _ALCHEMIST //_ITEMSET7_TXT
			{
				QString alch;
				getStringToken(data, "|", no + 13, alch);
				makeStringFromEscaped(alch);

				pc.item[i].alch = alch;
			}
#endif
#ifdef _PET_ITEM
			{
				QString type;
				getStringToken(data, "|", no + 14, type);
				makeStringFromEscaped(type);

				pc.item[i].type = type.toUShort();
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
					SetJigsaw( pc.item[i].modelid, pc.item[i].jigsaw );
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
	}

	setPC(pc);

	updateComboBoxList();
}

//對話框
void Server::lssproto_WN_recv(int windowtype, int buttontype, int dialogid, int unitid, char* cdata)
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
			strList[i].remove(u8"【");
			strList[i].remove(u8"】");
			strList[i].remove(u8"『");
			strList[i].remove(u8"』");
			strList[i].remove(u8"〖");
			strList[i].remove(u8"〗");
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

	currentDialog.set(dialog_t{ windowtype, buttontype, dialogid, unitid, data, linedatas, strList });

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
			item.valid = true;
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
			press(BUTTON_AUTO, dialogid, unitid);
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
				press(BUTTON_AUTO, dialogid, unitid);
				break;
			}
		}
	}


}

void Server::lssproto_PME_recv(int unitid,
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
		int modelid;
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
		modelid = smalltoken.toInt();
		getStringToken(data, "|", ps++, smalltoken);
		level = smalltoken.toInt();
		nameColor = getIntegerToken(data, "|", ps++);
		getStringToken(data, "|", ps++, name);
		makeStringFromEscaped(name);

		getStringToken(data, "|", ps++, freeName);
		makeStringFromEscaped(freeName);

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

//求救
void Server::lssproto_HL_recv(int flg)
{

}

//開始戰鬥
void Server::lssproto_EN_recv(int result, int field)
{
	//開始戰鬥為1，未開始戰鬥為0
	if (result > 0)
	{
		setBattleFlag(true);
		IS_LOCKATTACK_ESCAPE_DISABLE = false;
		battle_total.fetch_add(1, std::memory_order_release);
		normalDurationTimer.restart();
		battleDurationTimer.restart();
		oneRoundDurationTimer.restart();
	}
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

		bt.allies.clear();
		bt.enemies.clear();
		bt.fieldAttr = 0;
		bt.pet = {};
		bt.player = {};
		bt.objects.clear();
		bt.objects.resize(MAX_ENEMY);
		bt.charAlreadyAction = false;
		bt.charAlreadyAction = false;

		int i = 0, j = 0;
		int n = 0;

		QString temp;
		QStringList tempList = {};
		//檢查敵我其中一方是否全部陣亡
		bool isEnemyAllDead = true;
		bool isAllieAllDead = true;
		battleobject_t obj = {};
		int pos = 0;
		bool ok = false;
		bool valid = false;

		bt.fieldAttr = getIntegerToken(data, "|", 1);

		QElapsedTimer timer; timer.start();
		for (;;)
		{
			//16進制使用 a62toi
			//string 使用 getStringToken(data, "|", n, var); 而後使用 makeStringFromEscaped(var) 處理轉譯
			//int 使用 getIntegerToken(data, "|", n);

			getStringToken(data, "|", i * 13 + 2, temp);
			pos = temp.toInt(&ok, 16);
			if (!ok)
				break;

			if (pos < 0 || pos >= MAX_ENEMY)
				break;

			obj.pos = pos;

			getStringToken(data, "|", i * 13 + 3, temp);
			makeStringFromEscaped(temp);

			obj.name = temp;

			getStringToken(data, "|", i * 13 + 4, temp);
			makeStringFromEscaped(temp);

			obj.freeName = temp;

			getStringToken(data, "|", i * 13 + 5, temp);
			obj.modelid = temp.toInt(nullptr, 16);

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

			obj.rideName = temp;

			getStringToken(data, "|", i * 13 + 12, temp);
			obj.rideLevel = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 13, temp);
			obj.rideHp = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 14, temp);
			obj.rideMaxHp = temp.toInt(nullptr, 16);

			obj.rideHpPercent = util::percent(obj.rideHp, obj.rideMaxHp);

			valid = obj.modelid > 0 && obj.maxHp > 0 && obj.level > 0 && !checkAND(obj.status, BC_FLG_HIDE) && !checkAND(obj.status, BC_FLG_DEAD);

			if ((pos >= bt.enemymin) && (pos <= bt.enemymax) && obj.rideFlag == 0 && obj.modelid > 0 && !obj.name.isEmpty())
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
				bt.player.freeName = obj.freeName;
				bt.player.modelid = obj.modelid;
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
					for (j = 0; j < MAX_PET; ++j)
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

				setPC(pc);
			}

			//分開記錄敵我數據
			if (pos >= bt.alliemin && pos <= bt.alliemax)
			{
				if (valid)
					bt.allies.append(obj);
			}

			if (pos >= bt.enemymin && pos <= bt.enemymax)
			{
				if (valid)
					bt.enemies.append(obj);
			}

			if (pos < bt.objects.size())
				bt.objects.insert(pos, obj);

			if (valid || checkAND(obj.status, BC_FLG_HIDE))
			{
				if (obj.pos >= bt.alliemin && obj.pos <= bt.alliemax)
					isAllieAllDead = false;
				else
					isEnemyAllDead = false;
			}

			tempList.clear();
			temp.clear();
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

			if ((obj.pos >= bt.alliemin) && (obj.pos <= bt.alliemax))
			{
				qDebug() << "allie pos:" << obj.pos << "value:" << tempList;
				bottomList.append(tempList);
			}
			else if ((obj.pos >= bt.enemymin) && (obj.pos < bt.enemymax))
			{
				qDebug() << "enemy pos:" << obj.pos << "value:" << tempList;
				topList.append(tempList);
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
			obj = bt.objects.value(BattleMyNo + 5, battleobject_t{});
			if (obj.level <= 0 || obj.maxHp <= 0 || obj.modelid <= 0)
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
				for (j = 0; j < MAX_PET; ++j)
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
					bt.pet.freeName = obj.freeName;
					bt.pet.modelid = obj.modelid;
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

		//我方全部陣亡或敵方全部陣亡至戰鬥標誌為false
		if (!isAllieAllDead && !isEnemyAllDead)
			doBattleWork(false);//sync
		else
		{
			if (isAllieAllDead)
				announce("[async battle] 我方全部阵亡，结束战斗");
			if (isEnemyAllDead)
				announce("[async battle] 敌方全部阵亡，结束战斗");
		}

		qDebug() << "-------------------- cost:" << timer.elapsed() << "ms --------------------";
		qDebug() << "";
	}
	else if (first == "P")
	{
		QStringList list = data.split(util::rexOR);
		if (list.size() < 3)
			return;

		battle_one_round_time.store(oneRoundDurationTimer.elapsed(), std::memory_order_release);
		oneRoundDurationTimer.restart();

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
		setPC(pc);
		setBattleData(bt);
		updateCurrentSideRange(bt);
		isEnemyAllReady.store(false, std::memory_order_release);
	}
	else if (first == "A")
	{
		QStringList list = data.split(util::rexOR);
		if (list.size() < 2)
			return;

		battleCurrentAnimeFlag = list.at(0).toInt(nullptr, 16);
		battleCurrentRound.store(list.at(1).toInt(nullptr, 16), std::memory_order_release);

		if (battleCurrentAnimeFlag <= 0)
			return;

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
						announce(QString("[async battle] 自己 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName));
					if (i == BattleMyNo + 5)
						announce(QString("[async battle] 戰寵 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName));
					else
						announce(QString("[async battle] 隊友 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName));
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
				isEnemyAllReady.store(true, std::memory_order_release);
			}
		}

		setBattleData(bt);
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

	updateComboBoxList();
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

	QStringList list = { addressBook[aindex].name };

	getStringToken(text, "|", 1, MailHistory[aindex].dateStr[MailHistory[aindex].newHistoryNo]);

	getStringToken(text, "|", 2, MailHistory[aindex].str[MailHistory[aindex].newHistoryNo]);

	QString temp = MailHistory[aindex].str[MailHistory[aindex].newHistoryNo];
	makeStringFromEscaped(temp);

	MailHistory[aindex].str[MailHistory[aindex].newHistoryNo] = temp;
	list.append(temp);


	noReadFlag = getIntegerToken(text, "|", 3);

	if (noReadFlag != -1)
	{
		MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo] = noReadFlag;
		list.append(QString::number(noReadFlag));

		MailHistory[aindex].petLevel[MailHistory[aindex].newHistoryNo] = getIntegerToken(text, "|", 4);
		list.append(QString::number(MailHistory[aindex].petLevel[MailHistory[aindex].newHistoryNo]));

		getStringToken(text, "|", 5, MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo]);

		temp = MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo];
		makeStringFromEscaped(temp);

		MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo] = temp;
		list.append(temp);

		MailHistory[aindex].itemGraNo[MailHistory[aindex].newHistoryNo] = getIntegerToken(text, "|", 6);
		list.append(QString::number(MailHistory[aindex].itemGraNo[MailHistory[aindex].newHistoryNo]));

		//sprintf_s(moji, "收到%s送來的寵物郵件！", addressBook[aindex].name);
		announce(list.join("|"), color);
	}

	else
	{
		MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo] = TRUE;
		list.append(QString::number(MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo]));

		announce(list.join("|"), color);

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
	//dir = (dir + 3) % 8;

	//pc.dir = dir;

	setBattleEnd();
	updateMapArea();
	setPcWarpPoint(pos);
	setPoint(pos);

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
	//qDebug() << "-- NU --";
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
			if (i - 1 >= MAX_MISSION)
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
	makeStringFromEscaped(message);

	static const QRegularExpression rexGetGold(u8R"(到\s*(\d+)\s*石)");
	static const QRegularExpression rexPickGold(u8R"([獲|获]\s*(\d+)\s*Stone)");

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

	int i, j, id, x, y, dir, modelid, level, nameColor, walkable, height, classNo, money, charType, charNameColor;
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
			modelid = smalltoken.toInt();
			if (modelid == 9999) continue;
			getStringToken(bigtoken, "|", 8, smalltoken);
			level = smalltoken.toInt();
			nameColor = getIntegerToken(bigtoken, "|", 9);
			getStringToken(bigtoken, "|", 10, name);
			makeStringFromEscaped(name);

			getStringToken(bigtoken, "|", 11, freeName);
			makeStringFromEscaped(freeName);

			getStringToken(bigtoken, "|", 12, smalltoken);
			walkable = smalltoken.toInt();
			getStringToken(bigtoken, "|", 13, smalltoken);
			height = smalltoken.toInt();
			charNameColor = getIntegerToken(bigtoken, "|", 14);
			getStringToken(bigtoken, "|", 15, fmname);
			makeStringFromEscaped(fmname);

			getStringToken(bigtoken, "|", 16, petname);
			makeStringFromEscaped(petname);

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
				if (checkAND(pc.status, CHR_STATUS_LEADER) != 0 && party[0].valid)
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
					if (party[j].valid && party[j].id == id)
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
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, gm_name);
#else
#ifdef _NPC_PICTURE
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, picture);
#else
				//setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
				//	level, petname, petlevel, nameColor, walkable, height, charType, profession_class);
#endif
#endif
#else
#ifdef _NPC_EVENT_NOTICE
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, noticeNo
#ifdef _CHARTITLE_STR_
					, titlestr
#endif

				);
#else
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
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
					//	if (party[j].valid && party[j].id == id)
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
			unit.modelid = modelid;
			unit.level = level;
			unit.nameColor = nameColor;
			unit.name = name;
			unit.freeName = freeName;
			unit.walkable = walkable;
			unit.height = height;
			unit.charNameColor = charNameColor;
			unit.family = fmname;
			unit.petname = petname;
			unit.petlevel = petlevel;
			unit.profession_class = profession_class;
			unit.profession_level = profession_level;
			unit.profession_skill_point = profession_skill_point;
			unit.isVisible = modelid > 0 && modelid != 9999;
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
			modelid = smalltoken.toInt();
			classNo = getIntegerToken(bigtoken, "|", 6);
			getStringToken(bigtoken, "|", 7, info);
			makeStringFromEscaped(info);

			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.modelid = modelid;
			unit.classNo = classNo;
			unit.item_name = info;
			unit.isVisible = modelid > 0 && modelid != 9999;
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
			unit.isVisible = true;
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

			getStringToken(bigtoken, "|", 4, smalltoken);
			dir = (smalltoken.toInt() + 3) % 8;
			getStringToken(bigtoken, "|", 5, smalltoken);
			modelid = smalltoken.toInt();
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
			unit.modelid = modelid;
			unit.isVisible = modelid > 0 && modelid != 9999;
			unit.objType = util::OBJ_HUMAN;
			mapUnitHash.insert(id, unit);
		}

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
		setNpcCharObj(id, modelid, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0, "");
#else
#ifdef _NPC_PICTURE
		setNpcCharObj(id, modelid, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0, 0);
#else
		/*setNpcCharObj(id, modelid, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0);*/
#endif
#endif
#else
#ifdef _NPC_EVENT_NOTICE
		setNpcCharObj(id, modelid, x, y, dir, "", name, "",
			level, petname, petlevel, nameColor, 0, height, 2, 0
#ifdef _CHARTITLE_STR_
			, titlestr
#endif
		);
#else
		setNpcCharObj(id, modelid, x, y, dir, "", name, "",
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
			modelid = smalltoken.toInt();
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
			unit.modelid = modelid;
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
			unit.isvisible = modelid > 0;
			mapUnitHash.insert(id, unit);

			if (pc.id == id)
			{
				//if (pc.ptAct == NULL)
				//{
				//	//createPc(modelid, x, y, dir);
				//	//updataPcAct();
				//}
				//else
				{
					pc.modelid = modelid;
					pc.dir = dir;
				}
				updateMapArea();
				setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point);
				//setPcNameColor(charNameColor);
				if (checkAND(pc.status, CHR_STATUS_LEADER) != 0
					&& party[0].valid)
				{
					party[0].level = pc.level;
					party[0].name = pc.name;
				}
				for (j = 0; j < MAX_PARTY; ++j)
				{
					if (party[j].valid && party[j].id == id)
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
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, 0);
#else
				//setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName, level, petname, petlevel, nameColor, walkable, height, charType);
#endif
				//ptAct = getCharObjAct(id);
				//if (ptAct != NULL)
				//{
				//	for (j = 0; j < MAX_PARTY; ++j)
				//	{
				//		if (party[j].valid && party[j].id == id)
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
				modelid = smalltoken.toInt();
				classNo = getIntegerToken(bigtoken, "|", 5);
				getStringToken(bigtoken, "|", 6, info);
				makeStringFromEscaped(info);

				mapunit_t unit = mapUnitHash.value(id);
				unit.id = id;
				unit.x = x;
				unit.y = y;
				unit.p = QPoint(x, y);
				unit.modelid = modelid;
				unit.classNo = classNo;
				unit.info = info;
				mapUnitHash.insert(id, unit);

				//setItemCharObj(id, modelid, x, y, 0, classNo, info);
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
				if (checkAND(MenuToggleFlag, JOY_CTRL_M))
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

			pc.vit = getIntegerToken(data, "|", 6);		// 0x00000020
			pc.str = getIntegerToken(data, "|", 7);		// 0x00000040
			pc.tgh = getIntegerToken(data, "|", 8);		// 0x00000080
			pc.dex = getIntegerToken(data, "|", 9);		// 0x00000100
			pc.exp = getIntegerToken(data, "|", 10);		// 0x00000200
			pc.maxExp = getIntegerToken(data, "|", 11);		// 0x00000400
			pc.level = getIntegerToken(data, "|", 12);		// 0x00000800
			pc.atk = getIntegerToken(data, "|", 13);		// 0x00001000
			pc.def = getIntegerToken(data, "|", 14);		// 0x00002000
			pc.agi = getIntegerToken(data, "|", 15);		// 0x00004000
			pc.chasma = getIntegerToken(data, "|", 16);		// 0x00008000
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

			pc.name = name;
			getStringToken(data, "|", 31, freeName);
			makeStringFromEscaped(freeName);

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
				if (checkAND(kubun, mask))
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
						pc.vit = getIntegerToken(data, "|", i);// 0x00000020
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
						pc.agi = getIntegerToken(data, "|", i);// 0x00004000
						i++;
					}
					else if (mask == 0x00008000)
					{
						pc.chasma = getIntegerToken(data, "|", i);// 0x00008000
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

						pc.name = name;
						i++;
					}
					else if (mask == 0x04000000)
					{
						getStringToken(data, "|", i, freeName);// 0x02000000
						makeStringFromEscaped(freeName);

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

		if (checkAND(pc.status, CHR_STATUS_LEADER) && party[0].valid)
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
			pc.chasma, pc.atk, pc.def, pc.agi, pc.luck
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
		QString family;

		getStringToken(data, "|", 1, family);
		makeStringFromEscaped(family);

		pc.family = family;

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
		if (checkAND(pc.status, CHR_STATUS_LEADER) && party[0].valid)
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
			if (pet[no].valid)
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
			pet[no].valid = true;
			if (kubun == 1)
			{
				pet[no].modelid = getIntegerToken(data, "|", 2);		// 0x00000002
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
				pet[no].agi = getIntegerToken(data, "|", 12);		// 0x00000800
				pet[no].loyal = getIntegerToken(data, "|", 13);		// 0x00001000
				pet[no].earth = getIntegerToken(data, "|", 14);		// 0x00002000
				pet[no].water = getIntegerToken(data, "|", 15);		// 0x00004000
				pet[no].fire = getIntegerToken(data, "|", 16);		// 0x00008000
				pet[no].wind = getIntegerToken(data, "|", 17);		// 0x00010000
				pet[no].maxSkill = getIntegerToken(data, "|", 18);		// 0x00020000
				pet[no].changeNameFlag = getIntegerToken(data, "|", 19);// 0x00040000
				pet[no].transmigration = getIntegerToken(data, "|", 20);
#ifdef _SHOW_FUSION
				pet[no].fusion = getIntegerToken(data, "|", 21);
				getStringToken(data, "|", 22, name);// 0x00080000
				makeStringFromEscaped(name);

				pet[no].name = name;
				getStringToken(data, "|", 23, freeName);// 0x00100000
				makeStringFromEscaped(freeName);

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
					if (checkAND(kubun, mask))
					{
						if (mask == 0x00000002)
						{
							pet[no].modelid = getIntegerToken(data, "|", i);// 0x00000002
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
							pet[no].agi = getIntegerToken(data, "|", i);// 0x00000800
							i++;
						}
						else if (mask == 0x00001000)
						{
							pet[no].loyal = getIntegerToken(data, "|", i);// 0x00001000
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

							pet[no].name = name;
							i++;
						}
						else if (mask == 0x00100000)
						{
							getStringToken(data, "|", i, freeName);// 0x00100000
							makeStringFromEscaped(freeName);

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
			if (_pet.valid)
			{
				pet[i].power = (((static_cast<double>(_pet.atk + _pet.def + _pet.agi) + (static_cast<double>(_pet.maxHp) / 4.0)) / static_cast<double>(_pet.level)) * 100.0);
				varList = QVariantList{
					_pet.name, _pet.freeName, "",
					QObject::tr("%1(%2tr)").arg(_pet.level).arg(_pet.transmigration), _pet.exp, _pet.maxExp, _pet.maxExp - _pet.exp, "",
					QString("%1/%2").arg(_pet.hp).arg(_pet.maxHp), "",
					_pet.loyal, _pet.atk, _pet.def, _pet.agi, "", pet[i].power

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

		magic[no].valid = getIntegerToken(data, "|", 1) > 0;
		if (magic[no].valid)
		{
			magic[no].costmp = getIntegerToken(data, "|", 2);
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

			magic[no].name = name;
			getStringToken(data, "|", 6, memo);
			makeStringFromEscaped(memo);

			magic[no].memo = memo;
		}
		else
		{
			magic[no] = {};
		}


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
				if (party[i].name.isEmpty() || (!party[i].valid) || (party[i].maxHp <= 0))
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
			if (party[no].valid && party[no].id != pc.id)
			{

			}

			party[no].valid = false;
			party[no] = {};
			checkPartyCount = 0;
			no2 = -1;
#ifdef MAX_AIRPLANENUM
			for (i = 0; i < MAX_AIRPLANENUM; ++i)
#else
			for (i = 0; i < MAX_PARTY; ++i)
#endif
			{
				if (party[i].valid)
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
		party[no].valid = true;

		if (kubun == 1)
		{
			party[no].id = getIntegerToken(data, "|", 2);	// 0x00000002
			party[no].level = getIntegerToken(data, "|", 3);	// 0x00000004
			party[no].maxHp = getIntegerToken(data, "|", 4);	// 0x00000008
			party[no].hp = getIntegerToken(data, "|", 5);	// 0x00000010
			party[no].mp = getIntegerToken(data, "|", 6);	// 0x00000020
			getStringToken(data, "|", 7, name);	// 0x00000040
			makeStringFromEscaped(name);

			party[no].name = name;
		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (checkAND(kubun, mask))
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

			if (temp.isEmpty())
			{
				pc.item[i].valid = false;
				pc.item[i].name.clear();
				pc.item[i] = {};
				refreshItemInfo(i);
				continue;
			}
			pc.item[i].valid = true;
			pc.item[i].name = temp.simplified();
			getStringToken(data, "|", no + 2, temp);
			makeStringFromEscaped(temp);

			pc.item[i].name2 = temp;
			pc.item[i].color = getIntegerToken(data, "|", no + 3);
			if (pc.item[i].color < 0)
				pc.item[i].color = 0;
			getStringToken(data, "|", no + 4, temp);
			makeStringFromEscaped(temp);

			pc.item[i].memo = temp;
			pc.item[i].modelid = getIntegerToken(data, "|", no + 5);
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

			pc.item[i].damage = temp;
#ifdef _ITEM_PILENUMS
			getStringToken(data, "|", no + 11, temp);
			makeStringFromEscaped(temp);

			pc.item[i].stack = temp.toInt();
#endif
#ifdef _ALCHEMIST //_ITEMSET7_TXT
			getStringToken(data, "|", no + 12, temp);
			makeStringFromEscaped(temp);
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
			makeStringFromEscaped(temp);
			pc.item[i].jigsaw = temp;

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

		QStringList magicNameList;
		for (int i = 0; i < MAX_MAGIC; ++i)
		{
			magicNameList.append(magic[i].name);
		}

		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxCharAction, magicNameList);
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
			petSkill[no][i].valid = false;
		}

		QStringList skillNameList;
		for (i = 0; i < MAX_SKILL; ++i)
		{
			no2 = i * 5;
			getStringToken(data, "|", no2 + 4, temp);
			makeStringFromEscaped(temp);

			if (temp.isEmpty())
				continue;
			petSkill[no][i].valid = true;
			petSkill[no][i].name = temp;
			petSkill[no][i].skillId = getIntegerToken(data, "|", no2 + 1);
			petSkill[no][i].field = getIntegerToken(data, "|", no2 + 2);
			petSkill[no][i].target = getIntegerToken(data, "|", no2 + 3);
			getStringToken(data, "|", no2 + 5, temp);
			makeStringFromEscaped(temp);

			petSkill[no][i].memo = temp;

			if ((pc.battlePetNo >= 0) && pc.battlePetNo < MAX_PET)
				skillNameList.append(petSkill[no][i].name);
		}
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
			profession_skill[i].valid = false;
			profession_skill[i].kind = 0;
		}
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			count = i * 9;
			profession_skill[i].valid = getIntegerToken(data, "|", 1 + count) > 0;
			profession_skill[i].skillId = getIntegerToken(data, "|", 2 + count);
			profession_skill[i].target = getIntegerToken(data, "|", 3 + count);
			profession_skill[i].kind = getIntegerToken(data, "|", 4 + count);
			profession_skill[i].icon = getIntegerToken(data, "|", 5 + count);
			profession_skill[i].costmp = getIntegerToken(data, "|", 6 + count);
			profession_skill[i].skill_level = getIntegerToken(data, "|", 7 + count);

			getStringToken(data, "|", 8 + count, name);
			makeStringFromEscaped(name);

			profession_skill[i].name = name;

			getStringToken(data, "|", 9 + count, memo);
			makeStringFromEscaped(memo);

			profession_skill[i].memo = memo;
		}

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

			if (szData.isEmpty())	// 沒道具
			{
				pet[nPetIndex].item[i] = {};
				continue;
			}
			pet[nPetIndex].item[i].valid = true;
			pet[nPetIndex].item[i].name = szData;
			getStringToken(data, "|", no + 2, szData);
			makeStringFromEscaped(szData);

			pet[nPetIndex].item[i].name2 = szData;
			pet[nPetIndex].item[i].color = getIntegerToken(data, "|", no + 3);
			if (pet[nPetIndex].item[i].color < 0)
				pet[nPetIndex].item[i].color = 0;
			getStringToken(data, "|", no + 4, szData);
			makeStringFromEscaped(szData);

			pet[nPetIndex].item[i].memo = szData.simplified();
			pet[nPetIndex].item[i].modelid = getIntegerToken(data, "|", no + 5);
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

			pet[nPetIndex].item[i].damage = szData;
			pet[nPetIndex].item[i].stack = getIntegerToken(data, "|", no + 11);
#ifdef _ALCHEMIST //_ITEMSET7_TXT
			getStringToken(data, "|", no + 12, szData);
			makeStringFromEscaped(szData);

			pet[nPetIndex].item[i].alch = szData;
#endif
			pet[nPetIndex].item[i].type = getIntegerToken(data, "|", no + 13);
#ifdef _ITEM_JIGSAW
			getStringToken(data, "|", no + 14, szData);
			makeStringFromEscaped(szData);

			pet[nPetIndex].item[i].jigsaw = szData;
			//可拿給寵物裝備的道具,就不會是拼圖了,以下就免了
			//if( i == JigsawIdx )
			//	SetJigsaw( pc.item[i].modelid, pc.item[i].jigsaw );
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

	updateComboBoxList();
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

	for (i = 0; i < MAX_CHARACTER; ++i)
		chartable[i] = {};

	QVector<CHARLISTTABLE> vec;
	for (i = 0; i < MAX_CHARACTER; ++i)
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
			if (index >= 0 && index < MAX_CHARACTER)
			{
				table.valid = true;
				table.name = nm;
				table.faceid = args.at(1).toInt();
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
		if (i < 0 || i >= MAX_CHARACTER)
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

				if (pet[objno].valid && pc.ridePetNo != objno)
				{
					if (pet[objno].freeName[0] != NULL)
						tradePet[0].name = pet[objno].freeName;
					else
						tradePet[0].name = pet[objno].name;
					tradePet[0].level = pet[objno].level;
					tradePet[0].atk = pet[objno].atk;
					tradePet[0].def = pet[objno].def;
					tradePet[0].agi = pet[objno].agi;
					tradePet[0].modelid = pet[objno].modelid;

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