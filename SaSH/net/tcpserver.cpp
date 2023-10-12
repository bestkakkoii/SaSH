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
import Mem;
import Config;
import String;
import Global;
#include "stdafx.h"
#include "tcpserver.h"
#include "autil.h"
#include <injector.h>
#include "signaldispatcher.h"
#include "map/mapanalyzer.h"
#include "script/interpreter.h"

#pragma comment(lib, "winmm.lib")

#pragma region StringControl
// 0-9,a-z(10-35),A-Z(36-61)
__int64 Server::a62toi(const QString& a) const
{
	__int64 ret = 0;
	__int64 sign = 1;
	__int64 size = a.length();
	for (__int64 i = 0; i < size; ++i)
	{
		ret *= 62;
		if ('0' <= a.at(i) && a.at(i) <= '9')
			ret += a.at(i).unicode() - '0';
		else if ('a' <= a.at(i) && a.at(i) <= 'z')
			ret += a.at(i).unicode() - 'a' + 10;
		else if ('A' <= a.at(i) && a.at(i) <= 'Z')
			ret += a.at(i).unicode() - 'A' + 36;
		else if (a.at(i) == '-')
			sign = -1;
		else
			return 0;
	}
	return ret * sign;
}

__int64 Server::getStringToken(const QString& src, const QString& delim, __int64 count, QString& out) const
{
	if (src.isEmpty() || delim.isEmpty() || count < 0)
	{
		out.clear();
		return 1;
	}

	__int64 c = 1;
	__int64 i = 0;

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

	__int64 j = src.indexOf(delim, i);
	if (j == -1)
	{
		out = src.mid(i);
		return 0;
	}

	out = src.mid(i, j - i);
	return 0;
}

__int64 Server::getIntegerToken(const QString& src, const QString& delim, __int64 count) const
{
	if (src.isEmpty() || delim.isEmpty() || count < 0)
	{
		return -1;
	}

	QString s;
	if (getStringToken(src, delim, count, s) == 1)
		return -1;

	bool ok = false;
	__int64 value = s.toLongLong(&ok);
	if (ok)
		return value;
	return -1;
}

__int64 Server::getInteger62Token(const QString& src, const QString& delim, __int64 count) const
{
	QString s;
	getStringToken(src, delim, count, s);
	if (s.isEmpty())
		return -1;
	return a62toi(s);
}

void Server::makeStringFromEscaped(QString& src) const
{
	src.replace("\\n", "\n");
	src.replace("\\c", ",");
	src.replace("\\z", "|");
	src.replace("\\y", "\\");
	src.replace("　", " ");
	src = src.simplified();
}

#if 0
// 0-9,a-z(10-35),A-Z(36-61)
__int64 Server::a62toi(char* a) const
{
	__int64 ret = 0;
	__int64 fugo = 1;

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
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	net_readbuf_.clear();
	injector.autil.util_Clear();
}

__int64 Server::appendReadBuf(const QByteArray& data)
{
	net_readbuf_.append(data);
	return 0;
}

QByteArrayList Server::splitLinesFromReadBuf()
{
	QByteArrayList lines = net_readbuf_.split('\n'); // Split net_readbuf into lines

	if (!net_readbuf_.endsWith('\n'))
	{
		// The last line is incomplete, remove it from the list and keep it in net_readbuf
		__int64 lastIndex = static_cast<__int64>(lines.size()) - 1;
		net_readbuf_ = lines[lastIndex];
		lines.removeAt(lastIndex);
	}
	else
	{
		// net_readbuf does not contain any incomplete line
		net_readbuf_.clear();
	}

	for (__int64 i = 0; i < lines.size(); ++i)
	{
		// Remove '\r' from each line
		lines[i] = lines[i].replace('\r', "");
	}

	return lines;
}

#pragma endregion

inline constexpr bool checkAND(quint64 a, quint64 b)
{
	return (a & b) == b;
}

#pragma region Net
Server::Server(__int64 index, QObject* parent)
	: ThreadPlugin(index, parent)
	, Lssproto(&Injector::getInstance(index).autil)
	, chatQueue(MAX_CHAT_HISTORY)
	, petInfoLock_(QReadWriteLock::Recursive)
	, petSkillInfoLock_(QReadWriteLock::Recursive)
	, charInfoLock_(QReadWriteLock::Recursive)
	, charSkillInfoLock_(QReadWriteLock::Recursive)
	, charMagicInfoLock_(QReadWriteLock::Recursive)
	, itemInfoLock_(QReadWriteLock::Recursive)
	, petEquipInfoLock_(QReadWriteLock::Recursive)
	, teamInfoLock_(QReadWriteLock::Recursive)
{
	loginTimer.start();
	battleDurationTimer.start();
	oneRoundDurationTimer.start();
	normalDurationTimer.start();
	eottlTimer.start();
	connectingTimer.start();
	repTimer.start();

	Injector& injector = Injector::getInstance(index);
	injector.autil.util_Init();
	clearNetBuffer();

	injector.autil.PersonalKey.set("upupupupp");

	std::unique_ptr<MapAnalyzer> _mapAnalyzer(new MapAnalyzer(index));
	if (_mapAnalyzer == nullptr)
		return;

	mapAnalyzer.reset(_mapAnalyzer.release());
}

Server::~Server()
{
	requestInterruption();

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
		clientSocket->deleteLater();
	}

	clientSockets_.clear();
	mapAnalyzer.reset(nullptr);

	qDebug() << "Server is distroyed!!";
}

//用於清空部分數據 主要用於登出後清理數據避免數據混亂，每次登出後都應該清除大部分的基礎訊息
void Server::clear()
{
	Injector& injector = Injector::getInstance(getIndex());
	injector.autil.PersonalKey.set("upupupupp");

	enemyNameListCache = QStringList();
	customDialog = customdialog_t{};
	currentDialog = dialog_t{};

	playerInfoColContents.clear();
	itemInfoRowContents.clear();
	equipInfoRowContents.clear();
	enemyNameListCache = QStringList();
	topInfoContents = QVariant();
	bottomInfoContents = QVariant();
	timeLabelContents = QString();
	labelCharAction = QString();
	labelPetAction = QString();

	__int64 i = 0;
	for (i = 0; i < MAX_PET + 1; ++i)
		recorder[i] = {};

	nowFloor_ = 0;
	nowFloorName_ = QString();
	nowPoint_ = QPoint();

	currentBankPetList = QPair<__int64, QVector<bankpet_t>>{};
	currentBankItemList.clear();

	pc_ = PC{};

	party_.clear();
	item_.clear();
	petItem_.clear();
	pet_.clear();
	magic_.clear();
	addressBook_.clear();
	jobdaily_.clear();
	chartable_.clear();
	mailHistory_.clear();
	profession_skill_.clear();
	petSkill_.clear();
	battlePetDisableList_.clear();
	battleCharCurrentPos = 0;
	battleBpFlag = 0;
	battleCharEscapeFlag = 0;
	battleCharCurrentMp = 0;
	battleCurrentAnimeFlag = 0;
	lastSecretChatName.clear();

	mapUnitHash.clear();
	chatQueue.clear();

	IS_LOCKATTACK_ESCAPE_DISABLE = false;
	IS_WAITFOT_SKUP_RECV = false;

	petEscapeEnableTempFlag = false;
	tempCatchPetTargetIndex = -1;

	IS_WAITFOR_JOBDAILY_FLAG = false;
	IS_WAITFOR_BANK_FLAG = false;
	IS_WAITFOR_DIALOG_FLAG = false;
	IS_WAITFOR_CUSTOM_DIALOG_FLAG = false;
	IS_WAITOFR_ITEM_CHANGE_PACKET = false;
	isBattleDialogReady = false;
	isEOTTLSend = false;
	lastEOTime = 0;

	battleCurrentRound = 0;
	battle_total_time = 0;
	battle_total = 0;
	battle_one_round_time = 0;

	clearNetBuffer();

	skupFuture.cancel();
	skupFuture.waitForFinished();
}

//啟動TCP服務端，並監聽系統自動配發的端口
bool Server::start(QObject* parent)
{
	server_.reset(new QTcpServer(parent));
	if (server_.isNull())
		return false;

	connect(server_.data(), &QTcpServer::newConnection, this, &Server::onNewConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	QOperatingSystemVersion version = QOperatingSystemVersion::current();
	if (version > QOperatingSystemVersion::Windows7)
	{
		if (!server_->listen(QHostAddress::AnyIPv6))
		{
			qDebug() << "ipv6 Failed to listen on socket";
			emit signalDispatcher.messageBoxShow(tr("Failed to listen on IPV6 socket"), QMessageBox::Icon::Critical);
			return false;
		}
	}
	else
	{
		if (!server_->listen(QHostAddress::AnyIPv4))
		{
			qDebug() << "ipv4 Failed to listen on socket";
			emit signalDispatcher.messageBoxShow(tr("Failed to listen on IPV4 socket"), QMessageBox::Icon::Critical);
			return false;
		}
	}

	port_.store(server_->serverPort(), std::memory_order_release);
	return true;
}

//異步接收客戶端連入通知
void Server::onNewConnection()
{
	QTcpSocket* clientSocket = server_->nextPendingConnection();
	if (clientSocket == nullptr)
		return;

	if (clientSockets_.contains(clientSocket))
		return;

	clientSockets_.append(clientSocket);
	clientSocket->setReadBufferSize(8191);

	//clientSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
	clientSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	clientSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 8191);//8191
	clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 8191);

	connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onClientReadyRead, Qt::QueuedConnection);
	connect(this, &Server::write, this, &Server::onWrite, Qt::QueuedConnection);
}

//異步接收客戶端數據
void Server::onClientReadyRead()
{
	QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
	if (clientSocket == nullptr)
		return;

	QByteArray badata = clientSocket->readAll();
	if (badata.isEmpty())
		return;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	std::ignore = QtConcurrent::run(this, &Server::handleData, badata);
#else
	std::ignore = QtConcurrent::run(&Server::handleData, this, badata);
#endif
	//QMetaObject::invokeMethod(this, "handleData", Qt::QueuedConnection, Q_ARG(QByteArray, badata));
	//handleData(badata);
}

//異步發送數據
void Server::onWrite(QTcpSocket* clientSocket, QByteArray ba, __int64 size)
{
	if (clientSocket && clientSocket->state() != QAbstractSocket::UnconnectedState)
	{
		clientSocket->write(ba, size);
		clientSocket->flush();
	}
}

bool Server::handleCustomMessage(const QByteArray& badata)
{
	QString preStr = toQString(badata);
	__int64 indexEof = preStr.indexOf("\n");
	//\n之後的移除
	preStr = preStr.left(indexEof);

	if (preStr.startsWith("dc|"))
	{
		setBattleFlag(false);
		setOnlineFlag(false);
		return true;
	}

	if (preStr.startsWith("bpk|"))
	{
		__int64 currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		//__int64 value = mem::read<short>(injector.getProcess(), injector.getProcessModule() + 0xE21E4);
		isBattleDialogReady.store(true, std::memory_order_release);
		doBattleWork(true);
		return true;
	}

	if (preStr.startsWith("dk|"))
	{
		//remove dk|s
		preStr = preStr.mid(3);
		lssproto_CustomWN_recv(preStr);
		return true;
	}

	if (preStr.startsWith("tk|"))
	{
		//remove tk|s
		preStr = preStr.mid(3);
		lssproto_CustomTK_recv(preStr);
		return true;
	}

	return false;
}

//異步處理數據
void Server::handleData(QByteArray badata)
{
	QMutexLocker lock(&net_mutex);
	if (handleCustomMessage(badata))
		return;

	appendReadBuf(badata);

	if (net_readbuf_.isEmpty())
		return;

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QString key = mem::readString(injector.getProcess(), injector.getProcessModule() + kOffsetPersonalKey, PERSONALKEYSIZE, true, true);
	if (key != injector.autil.PersonalKey)
		injector.autil.PersonalKey = key;

	QByteArrayList dataList = splitLinesFromReadBuf();
	if (dataList.isEmpty())
		return;

	for (QByteArray& ba : dataList)
	{
		if (isInterruptionRequested())
			break;

		// get line from read buffer
		if (ba.isEmpty())
		{
			//qDebug() << "************************ EMPTY_BUFFER ************************";
			//數據讀完了
			injector.autil.util_Clear();
			continue;
		}

		__int64 ret = dispatchMessage(ba);

		if (ret < 0)
		{
			qDebug() << "************************ LSSPROTO_END ************************";
			//代表此段數據已到結尾
			injector.autil.util_Clear();
			break;
		}
		else if (ret == BC_INVALID)
		{
			qDebug() << "************************ INVALID_DATA ************************";
			continue;
		}
		else if (ret == BC_NEED_TO_CLEAN)
		{
			qDebug() << "************************ CLEAR_BUFFER ************************";
			//錯誤的數據 或 需要清除緩存
			clearNetBuffer();
			break;
		}
		else if (ret == BC_ABOUT_TO_END)
		{
			//qDebug() << "************************ ABOUT_TO_END ************************";
			continue;
		}
		else if (ret == BC_HAS_NEXT)
		{
			qDebug() << "************************ HAS_NEXT ************************";
			continue;
		}


	}
}

//經由 handleData 調用同步解析數據
__int64 Server::dispatchMessage(const QByteArray& encoded)
{
	__int64 func = 0, fieldcount = 0;
	__int64 iChecksum = 0, iChecksumrecv = 0;

	Injector& injector = Injector::getInstance(getIndex());
	net_raw_.clear();
	injector.autil.util_DecodeMessage(net_raw_, encoded);
	injector.autil.util_SplitMessage(net_raw_, SEPARATOR);
	if (injector.autil.util_GetFunctionFromSlice(&func, &fieldcount) != 1)
		return BC_HAS_NEXT;

	if (func == LSSPROTO_ERROR_RECV)
		return BC_INVALID;

	qDebug() << "fun" << func << "fieldcount" << fieldcount;

	switch (func)
	{
	case LSSPROTO_XYD_RECV: /*戰後刷新人物座標、方向2*/
	{
		int x = 0;
		int y = 0;
		int dir = 0;

		if (!injector.autil.util_Receive(&x, &y, &dir))
			return BC_INVALID;

		qDebug() << "LSSPROTO_XYD_RECV 戰後導正" << "x" << x << "y" << y << "dir" << dir;
		lssproto_XYD_recv(QPoint(x, y), dir);
		break;
	}
	case LSSPROTO_EV_RECV: /*WRAP 4*/
	{
		int dialogid = 0;
		int result = 0;

		if (!injector.autil.util_Receive(&dialogid, &result))
			return BC_INVALID;

		qDebug() << "LSSPROTO_EV_RECV WARP" << "dialogid" << dialogid << "result" << result;
		lssproto_EV_recv(dialogid, result);
		break;
	}
	case LSSPROTO_EN_RECV: /*Battle EncountFlag //開始戰鬥 7*/
	{
		int result = 0;
		int field = 0;

		if (!injector.autil.util_Receive(&result, &field))
			return BC_INVALID;

		qDebug() << "LSSPROTO_EN_RECV 開始戰鬥" << "result" << result << "field" << field;
		lssproto_EN_recv(result, field);
		break;
	}
	case LSSPROTO_RS_RECV: /*戰後獎勵 12*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_RS_RECV" << toQUnicode(data.data());
		lssproto_RS_recv(net_data.data());
		break;
	}
	case LSSPROTO_RD_RECV:/*戰後經驗 13*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;


		//qDebug() << "LSSPROTO_RD_RECV" << toQUnicode(data.data());
		lssproto_RD_recv(net_data.data());
		break;
	}
	case LSSPROTO_B_RECV: /*每回合開始的戰場資訊 15*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		qDebug() << "LSSPROTO_B_RECV" << toQUnicode(net_data.data());
		lssproto_B_recv(net_data.data());
		break;
	}
	case LSSPROTO_I_RECV: /*物品變動 22*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_I_RECV" << toQUnicode(data.data());
		lssproto_I_recv(net_data.data());
		break;
	}
	case LSSPROTO_SI_RECV:/* 道具位置交換24*/
	{
		int fromindex;
		int toindex;

		if (!injector.autil.util_Receive(&fromindex, &toindex))
			return BC_INVALID;

		qDebug() << "LSSPROTO_SI_RECV" << "fromindex" << fromindex << "toindex" << toindex;
		lssproto_SI_recv(fromindex, toindex);
		break;
	}
	case LSSPROTO_MSG_RECV:/*收到郵件26*/
	{
		int aindex;
		int color;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&aindex, net_data.data(), &color))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_MSG_RECV" << toQUnicode(data.data());
		lssproto_MSG_recv(aindex, net_data.data(), color);
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
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&unitid, &graphicsno, &x, &y, &dir, &flg, &no, net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_PME_RECV" << "unitid" << unitid << "graphicsno" << graphicsno <<
			//"x" << x << "y" << y << "dir" << dir << "flg" << flg << "no" << no << "cdata" << toQUnicode(data.data());
		lssproto_PME_recv(unitid, graphicsno, QPoint(x, y), dir, flg, no, net_data.data());
		break;
	}
	case LSSPROTO_AB_RECV:/* 30*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_AB_RECV" << toQUnicode(data.data());
		lssproto_AB_recv(net_data.data());
		break;
	}
	case LSSPROTO_ABI_RECV:/*名片數據31*/
	{
		int num;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&num, net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_ABI_RECV" << "num" << num << "data" << toQUnicode(data.data());
		lssproto_ABI_recv(num, net_data.data());
		break;
	}
	case LSSPROTO_TK_RECV: /*收到對話36*/
	{
		int index;
		int color;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&index, net_data.data(), &color))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_TK_RECV" << "index" << index << "message" << toQUnicode(data.data()) << "color" << color;
		lssproto_TK_recv(index, net_data.data(), color);
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
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&fl, &x1, &y1, &x2, &y2, &tilesum, &objsum, &eventsum, net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_MC_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 <<
			//"tilesum" << tilesum << "objsum" << objsum << "eventsum" << eventsum << "data" << toQUnicode(data.data());
		lssproto_MC_recv(fl, x1, y1, x2, y2, tilesum, objsum, eventsum, net_data.data());
		break;
	}
	case LSSPROTO_M_RECV: /*地圖數據更新，重新寫入地圖2 39*/
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&fl, &x1, &y1, &x2, &y2, net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_M_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 << "data" << toQUnicode(data.data());
		lssproto_M_recv(fl, x1, y1, x2, y2, net_data.data());
		break;
	}
	case LSSPROTO_C_RECV: /*服務端發送的靜態信息，可用於顯示玩家，其它玩家，公交，寵物等信息 41*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		qDebug() << "LSSPROTO_C_RECV" << toQUnicode(net_data.data());
		lssproto_C_recv(net_data.data());
		break;
	}
	case LSSPROTO_CA_RECV: /*//周圍人、NPC..等等狀態改變必定是 _C_recv已經新增過的單位 42*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CA_RECV" << toQUnicode(data.data());
		lssproto_CA_recv(net_data.data());
		break;
	}
	case LSSPROTO_CD_RECV: /*刪除指定一個或多個周圍人、NPC單位 43*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CD_RECV" << toQUnicode(data.data());
		lssproto_CD_recv(net_data.data());
		break;
	}
	case LSSPROTO_R_RECV:
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_R_RECV" << toQUnicode(data.data());
		lssproto_R_recv(net_data.data());
		break;
	}
	case LSSPROTO_S_RECV: /*更新所有基礎資訊 46*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		qDebug() << "LSSPROTO_S_RECV" << toQUnicode(net_data.data());
		lssproto_S_recv(net_data.data());
		break;
	}
	case LSSPROTO_D_RECV:/*47*/
	{
		int category;
		int dx;
		int dy;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&category, &dx, &dy, net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_D_RECV" << "category" << category << "dx" << dx << "dy" << dy << "data" << toQUnicode(data.data());
		lssproto_D_recv(category, dx, dy, net_data.data());
		break;
	}
	case LSSPROTO_FS_RECV:/*開關切換 49*/
	{
		int flg;

		if (!injector.autil.util_Receive(&flg))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_FS_RECV" << "flg" << flg;
		lssproto_FS_recv(flg);
		break;
	}
	case LSSPROTO_HL_RECV:/*51*/
	{
		int flg;

		if (!injector.autil.util_Receive(&flg))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_HL_RECV" << "flg" << flg;
		lssproto_HL_recv(flg);
		break;
	}
	case LSSPROTO_PR_RECV:/*組隊變化 53*/
	{
		int request;
		int result;

		if (!injector.autil.util_Receive(&request, &result))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_PR_RECV" << "request" << request << "result" << result;
		lssproto_PR_recv(request, result);
		break;
	}
	case LSSPROTO_KS_RECV: /*寵物更換狀態55*/
	{
		int petarray;
		int result;

		if (!injector.autil.util_Receive(&petarray, &result))
			return BC_INVALID;

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

		if (!injector.autil.util_Receive(&result, &havepetindex, &havepetskill, &toindex))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_PS_RECV" << "result" << result << "havepetindex" << havepetindex << "havepetskill" << havepetskill << "toindex" << toindex;
		lssproto_PS_recv(result, havepetindex, havepetskill, toindex);
		break;
	}
	case LSSPROTO_SKUP_RECV: /*更新點數 63*/
	{
		int point;

		if (!injector.autil.util_Receive(&point))
			return BC_INVALID;

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
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&windowtype, &buttontype, &dialogid, &unitid, net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_WN_RECV" << "windowtype" << windowtype << "buttontype" << buttontype << "dialogid" << dialogid << "unitid" << unitid << "data" << toQUnicode(data.data());
		lssproto_WN_recv(windowtype, buttontype, dialogid, unitid, net_data.data());
		break;
	}
	case LSSPROTO_EF_RECV: /*天氣68*/
	{
		int effect;
		int level;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(&effect, &level, net_data.data()))
			return BC_INVALID;

		qDebug() << "LSSPROTO_EF_RECV WEATHER" << "effect" << effect << "level" << level << "option" << toQUnicode(net_data.data());
		lssproto_EF_recv(effect, level, net_data.data());
		break;
	}
	case LSSPROTO_SE_RECV:/*69*/
	{
		int x;
		int y;
		int senumber;
		int sw;

		if (!injector.autil.util_Receive(&x, &y, &senumber, &sw))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_SE_RECV" << "x" << x << "y" << y << "senumber" << senumber << "sw" << sw;
		lssproto_SE_recv(QPoint(x, y), senumber, sw);
		break;
	}
	case LSSPROTO_CLIENTLOGIN_RECV:/*選人畫面 72*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CLIENTLOGIN_RECV" << toQUnicode(data.data());
		lssproto_ClientLogin_recv(net_data.data());

		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CREATENEWCHAR_RECV:/*人物新增74*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		QByteArray result(SBUFSIZE, '\0');
		if (!injector.autil.util_Receive(result.data(), net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CREATENEWCHAR_RECV" << toQUnicode(result) << toQUnicode(data.data());
		lssproto_CreateNewChar_recv(result.data(), net_data.data());
		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARDELETE_RECV:/*人物刪除 76*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		QByteArray result(SBUFSIZE, '\0');
		if (!injector.autil.util_Receive(result.data(), net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CHARDELETE_RECV" << toQUnicode(result) << toQUnicode(data.data());
		lssproto_CharDelete_recv(result.data(), net_data.data());
		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARLOGIN_RECV: /*成功登入 78*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		QByteArray result(SBUFSIZE, '\0');
		if (!injector.autil.util_Receive(result.data(), net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CHARLOGIN_RECV" << toQUnicode(result) << toQUnicode(data.data());
		lssproto_CharLogin_recv(result.data(), net_data.data());
		break;
	}
	case LSSPROTO_CHARLIST_RECV:/*選人頁面資訊 80*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		QByteArray result(SBUFSIZE, '\0');
		if (!injector.autil.util_Receive(result.data(), net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CHARLIST_RECV" << toQUnicode(result) << toQUnicode(data.data());
		lssproto_CharList_recv(result.data(), net_data.data());

		return BC_NEED_TO_CLEAN;
	}
	case LSSPROTO_CHARLOGOUT_RECV:/*登出 82*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		QByteArray result(SBUFSIZE, '\0');
		if (!injector.autil.util_Receive(result.data(), net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CHARLOGOUT_RECV" << toQUnicode(result) << toQUnicode(data.data());
		lssproto_CharLogout_recv(result.data(), net_data.data());
		break;
	}
	case LSSPROTO_PROCGET_RECV:/*84*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//ebug() << "LSSPROTO_PROCGET_RECV" << toQUnicode(data.data());
		lssproto_ProcGet_recv(net_data.data());
		break;
	}
	case LSSPROTO_PLAYERNUMGET_RECV:/*86*/
	{
		int logincount;
		int player;

		if (!injector.autil.util_Receive(&logincount, &player))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_PLAYERNUMGET_RECV" << "logincount:" << logincount << "player:" << player; //"logincount:%d player:%d\n
		lssproto_CharNumGet_recv(logincount, player);
		break;
	}
	case LSSPROTO_ECHO_RECV: /*伺服器定時ECHO "hoge" 88*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_ECHO_RECV" << toQUnicode(data.data());
		lssproto_Echo_recv(net_data.data());
		break;
	}
	case LSSPROTO_NU_RECV: /*不知道幹嘛的 90*/
	{
		int AddCount;

		if (!injector.autil.util_Receive(&AddCount))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_NU_RECV" << "AddCount:" << AddCount;
		lssproto_NU_recv(AddCount);
		break;
	}
	case LSSPROTO_TD_RECV:/*92*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_TD_RECV" << toQUnicode(data.data());
		lssproto_TD_recv(net_data.data());
		break;
	}
	case LSSPROTO_FM_RECV:/*家族頻道93*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_FM_RECV" << toQUnicode(data.data());
		lssproto_FM_recv(net_data.data());
		break;
	}
	case LSSPROTO_WO_RECV:/*95*/
	{
		int effect;

		if (!injector.autil.util_Receive(&effect))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_WO_RECV" << "effect:" << effect;
		lssproto_WO_recv(effect);
		break;
	}
	case LSSPROTO_NC_RECV: /*沈默? 101* 戰鬥結束*/
	{
		int flg = 0;

		if (!injector.autil.util_Receive(&flg))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_NC_RECV" << "flg:" << flg;
		lssproto_NC_recv(flg);
		break;
	}
	case LSSPROTO_CS_RECV:/*固定客戶端的速度104*/
	{
		int deltimes = 0;

		if (!injector.autil.util_Receive(&deltimes))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CS_RECV" << "deltimes:" << deltimes;
		lssproto_CS_recv(deltimes);
		break;
	}
	case LSSPROTO_PETST_RECV: /*寵物狀態改變 107*/
	{
		int petarray;
		int nresult;

		if (!injector.autil.util_Receive(&petarray, &nresult))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_PETST_RECV" << "petarray:" << petarray << "result:" << result;
		lssproto_PETST_recv(petarray, nresult);
		break;
	}
	case LSSPROTO_SPET_RECV: /*寵物更換狀態115*/
	{
		int standbypet;
		int nresult;

		if (!injector.autil.util_Receive(&standbypet, &nresult))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_SPET_RECV" << "standbypet:" << standbypet << "result:" << result;
		lssproto_SPET_recv(standbypet, nresult);
		break;
	}
	case LSSPROTO_JOBDAILY_RECV:/*任務日誌120*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;


		//qDebug() << "LSSPROTO_JOBDAILY_RECV" << toQUnicode(data.data());
		lssproto_JOBDAILY_recv(net_data.data());
		break;
	}
	case LSSPROTO_TEACHER_SYSTEM_RECV:/*導師系統123*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_TEACHER_SYSTEM_RECV" << toQUnicode(data.data());
		lssproto_TEACHER_SYSTEM_recv(net_data.data());
		break;
	}
	case LSSPROTO_S2_RECV:
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		lssproto_S2_recv(net_data.data());
		break;
	}
	case LSSPROTO_FIREWORK_RECV:/*煙火?126*/
	{
		int iCharaindex, iType, iActionNum;

		if (!injector.autil.util_Receive(&iCharaindex, &iType, &iActionNum))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_FIREWORK_RECV" << "iCharaindex:" << iCharaindex << "iType:" << iType << "iActionNum:" << iActionNum;
		lssproto_Firework_recv(iCharaindex, iType, iActionNum);
		break;
	}
	case LSSPROTO_CHAREFFECT_RECV:/*146*/
	{
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data()))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_CHAREFFECT_RECV" << toQUnicode(data.data());
		lssproto_CHAREFFECT_recv(net_data.data());
		break;
	}
	case LSSPROTO_IMAGE_RECV:/*151 SE SO驗證圖*/
	{
		//int x = 0;
		//int y = 0;
		//int z = 0;

		//if (!injector.autil.util_Receive(net_data.data(), &x, &y, &z))
		//	return BC_INVALID;

		break;
	}
	case LSSPROTO_DENGON_RECV:/*200*/
	{
		int coloer;
		int num;
		QByteArray net_data(NETDATASIZE, '\0');
		if (!injector.autil.util_Receive(net_data.data(), &coloer, &num))
			return BC_INVALID;

		//qDebug() << "LSSPROTO_DENGON_RECV" << toQUnicode(data.data()) << "coloer:" << coloer << "num:" << num;
		lssproto_DENGON_recv(net_data.data(), coloer, num);
		break;
	}
	case LSSPROTO_SAMENU_RECV:/*201*/
	{
		//int count;

		//if (!injector.autil.util_Receive(&count, net_data.data()))
		//	return BC_INVALID;

		//qDebug() << "LSSPROTO_SAMENU_RECV" << "count:" << count << toQUnicode(data.data());
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

	injector.autil.util_DiscardMessage();
	return BC_ABOUT_TO_END;
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region GET
bool Server::getBattleFlag()
{
	if (isInterruptionRequested())
		return false;

	return IS_BATTLE_FLAG.load(std::memory_order_acquire) || getWorldStatus() == 10;
}

bool Server::getOnlineFlag() const
{
	if (isInterruptionRequested())
		return false;
	return IS_ONLINE_FLAG.load(std::memory_order_acquire);
}

//用於判斷畫面的狀態的數值 (9平時 10戰鬥 <8非登入)
__int64 Server::getWorldStatus()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	return static_cast<__int64>(mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetWorldStatus));
}

//用於判斷畫面或動畫狀態的數值 (平時一般是3 戰鬥中選擇面板是4 戰鬥動作中是5或6，平時還有很多其他狀態值)
__int64 Server::getGameStatus()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	return static_cast<__int64>(mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetGameStatus));
}

bool Server::checkWG(__int64 w, __int64 g)
{
	return getWorldStatus() == w && getGameStatus() == g;
}

//檢查非登入時所在頁面
__int64 Server::getUnloginStatus()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	__int64 W = getWorldStatus();
	__int64 G = getGameStatus();

	if (0 == W && 0 == G)
	{
		return kStatusDisappear;//窗口不存在
	}
	if (11 == W && 2 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusDisconnected);
		return kStatusDisconnect;//斷線
	}
	else if ((2 == W && 5 == G) || (6 == W && 1 == G))//|| (9 == W && 102 == G) || (9 == W && 0 == G) || (9 == W && 103 == G)
	{
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusConnecting);
		return kStatusConnecting;//連線中
	}
	else if (3 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusBusy);
		return kStatusBusy;//忙碌
	}
	else if (2 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusTimeout);
		return kStatusTimeout;//逾時
	}
	else if (6 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusLoginFailed);
		return kStatusLoginFailed;//簽入失敗
	}
	else if (1 == W && 101 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelNoUserNameOrPassword);
		return kNoUserNameOrPassword;//無賬號密碼
	}
	else if ((1 == W && 2 == G) || (1 == W && 3 == G))
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusLogining);
		return kStatusInputUser;//輸入賬號密碼
	}
	else if (2 == W && 2 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusSelectServer);
		return kStatusSelectServer;//選擇伺服器
	}
	else if (2 == W && 3 == G)
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusSelectSubServer);
		return kStatusSelectSubServer;//選擇子伺服器(分流)
	}
	else if ((3 == W && 11 == G) || (3 == W && 1 == G))
	{
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusSelectPosition);
		return kStatusSelectCharacter;//選擇人物
	}
	else if ((9 == W && 200 == G) || (9 == W && 201 == G) || (9 == W && 202 == G) || (9 == W && 203 == G) || (9 == W && 204 == G))//刷新
	{
		return kStatusUnknown;
	}
	else if (9 == W && 1 == G)//切回平時
	{
		return kStatusUnknown;
	}
	else if (9 == W && 2 == G)//切回平時2
	{
		return kStatusUnknown;
	}
	else if (9 == W && 20 == G)//切換場景
	{
		return kStatusUnknown;
	}
	else if ((3 == W && 0 == G) || (3 == W && 10 == G))//????選人畫面->登入
	{
		return kStatusUnknown;
	}
	else if (5 == W && 0 == G)//????登入後->平時
	{
		return kStatusUnknown;
	}
	else if (9 == W && 100 == G)//????登入後->平時2
	{
		return kStatusUnknown;
	}
	else if (9 == W && 101 == G)//????登入後->平時3
	{
		return kStatusUnknown;
	}
	else if (9 == W && 103 == G)//????登入後->平時3
	{
		return kStatusUnknown;
	}
	else if (9 == W && 4 == G)//轉移入戰鬥
	{
		return kStatusUnknown;
	}
	else if (10 == W && 0 == G)//轉移入戰鬥
	{
		return kStatusUnknown;
	}
	else if (10 == W && 1 == G)//轉移入戰鬥2
	{
		return kStatusUnknown;
	}
	else if (10 == W && 2 == G)//戰鬥中動作之後?
	{
		return kStatusUnknown;
	}
	else if (10 == W && 3 == G)//戰鬥前置
	{
		return kStatusUnknown;
	}
	else if (10 == W && 4 == G)//戰鬥切出面板
	{
		return kStatusUnknown;
	}
	else if (10 == W && 5 == G)//戰鬥面板結束
	{
		return kStatusUnknown;
	}
	else if (10 == W && 6 == G)//戰鬥動作中
	{
		return kStatusUnknown;
	}
	else if (10 == W && 7 == G)//????戰鬥->平時
	{
		return kStatusUnknown;
	}
	else if (10 == W && 8 == G)//????戰鬥->平時2
	{
		return kStatusUnknown;
	}
	else if (9 == W && 3 == G)
		return kStatusLogined;//已豋入(平時且無其他對話框或特殊場景)

	qDebug() << "getUnloginStatus: " << W << " " << G;
	return kStatusUnknown;
}

//計算人物最單物品大堆疊數(負重量)
void Server::getCharMaxCarryingCapacity()
{
	QWriteLocker locker(&charInfoLock_);
	int nowMaxload = pc_.maxload;
	switch (pc_.transmigration)
	{
	case 0:
		pc_.maxload = 3;
		break;
	case 1:
	case 2:
	case 3:
	case 4:
		pc_.maxload = 3 + pc_.transmigration;
		break;
	case 5:
		pc_.maxload = 10;
		break;
	case 6:
		pc_.maxload = 11;
		break;
	}

	//取腰带的负重
	ITEM item = item_.value(CHAR_EQBELT);
	if (!item.name.isEmpty())
	{
		//負重|负重
		static const QRegularExpression re("負重|负重");
		__int64 index = item.memo.indexOf(re);
		if (index != -1)
		{

			QString buf = item.memo.mid(index + 3);
			bool ok = false;
			__int64 value = buf.toLongLong(&ok);
			if (ok && value > 0)
				pc_.maxload += value;
		}
	}

	if (pc_.maxload < nowMaxload)
		pc_.maxload = nowMaxload;
}

__int64 Server::getPartySize() const
{
	PC pc = getPC();
	__int64 count = 0;

	if (checkAND(pc.status, CHR_STATUS_LEADER) || checkAND(pc.status, CHR_STATUS_PARTY))
	{
		QReadLocker locker(&teamInfoLock_);
		for (__int64 i = 0; i < MAX_PARTY; ++i)
		{
			PARTY party = party_.value(i);
			if (!party.valid)
				continue;
			if (party.level <= 0)
				continue;

			++count;
		}
	}
	return count;
}

QString Server::getChatHistory(__int64 index)
{
	if (index < 0 || index >= MAX_CHAT_HISTORY)
		return "\0";

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();

	__int64 total = static_cast<__int64>(mem::read<int>(hProcess, hModule + kOffsetChatBufferMaxCount));
	if (index > total)
		return "\0";

	//int maxptr = mem::read<int>(hProcess, hModule + 0x146278);

	constexpr __int64 MAX_CHAT_BUFFER = 0x10C;
	__int64 ptr = hModule + kOffsetChatBuffer + ((total - index) * MAX_CHAT_BUFFER);

	return mem::readString(hProcess, ptr, MAX_CHAT_BUFFER, true);
}

//獲取周圍玩家名稱列表
QStringList Server::getJoinableUnitList() const
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QString leader = injector.getStringHash(kAutoFunNameString).simplified();
	QStringList unitNameList;
	if (!leader.isEmpty())
		unitNameList.append(leader);

	PC pc = getPC();
	for (const mapunit_t& unit : mapUnitHash)
	{
		QString newNpcName = unit.name.simplified();
		if (newNpcName.isEmpty() || (unit.objType != OBJ_HUMAN))
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
bool Server::getItemIndexsByName(const QString& name, const QString& memo, QVector<__int64>* pv, __int64 from, __int64 to)
{
	updateItemByMemory();

	bool isExact = true;
	QString newName = name.simplified();
	QString newMemo = memo.simplified();

	if (name.startsWith("?"))
	{
		isExact = false;
		newName = name.mid(1).simplified();
	}

	QVector<__int64> v;
	QHash<__int64, ITEM> items = getItems();
	for (__int64 i = 0; i < MAX_ITEM; ++i)
	{
		QString itemName = items.value(i).name.simplified();
		QString itemMemo = items.value(i).memo.simplified();

		if (itemName.isEmpty() || !items.value(i).valid)
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
__int64 Server::getItemIndexByName(const QString& name, bool isExact, const QString& memo, __int64 from, __int64 to)
{
	updateItemByMemory();

	if (name.isEmpty())
		return -1;

	QString newStr = name.simplified();
	QString newMemo = memo.simplified();

	if (newStr.startsWith("?"))
	{
		newStr.remove(0, 1);
		isExact = false;
	}

	QHash<__int64, ITEM> items = getItems();
	for (__int64 i = from; i < to; ++i)
	{
		QString itemName = items.value(i).name.simplified();
		if (itemName.isEmpty() || !items.value(i).valid)
			continue;

		QString itemMemo = items.value(i).memo.simplified();

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
__int64 Server::getPetSkillIndexByName(__int64& petIndex, const QString& name) const
{
	QString newName = name.simplified();
	__int64 i = 0;
	if (petIndex == -1)
	{

		for (__int64 j = 0; j < MAX_PET; ++j)
		{
			QHash<__int64, PET_SKILL> petSkill = getPetSkills(j);
			for (i = 0; i < MAX_SKILL; ++i)
			{
				QString petSkillName = petSkill.value(i).name.simplified();
				if (petSkillName.isEmpty() || !petSkill.value(i).valid)
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
		QHash<__int64, PET_SKILL> petSkill = getPetSkills(petIndex);
		for (i = 0; i < MAX_SKILL; ++i)
		{
			QString petSkillName = petSkill.value(i).name.simplified();
			if (petSkillName.isEmpty() || !petSkill.value(i).valid)
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
bool Server::getPetIndexsByName(const QString& name, QVector<__int64>* pv) const
{
	QVector<__int64> v;
	QStringList nameList = name.simplified().split(rexOR, Qt::SkipEmptyParts);

	for (__int64 i = 0; i < MAX_PET; ++i)
	{
		if (v.contains(i))
			continue;

		PET pet = getPet(i);
		QString newPetName = pet.name.simplified();
		if (newPetName.isEmpty() || !pet.valid)
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
__int64 Server::getItemEmptySpotIndex()
{
	updateItemByMemory();

	QHash<__int64, ITEM> items = getItems();
	for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		QString name = items.value(i).name.simplified();
		if (name.isEmpty() || !items.value(i).valid)
			return i;
	}

	return -1;
}

bool Server::getItemEmptySpotIndexs(QVector<__int64>* pv)
{
	updateItemByMemory();

	QVector<__int64> v;
	QHash<__int64, ITEM> items = getItems();
	for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		QString name = items.value(i).name.simplified();
		if (name.isEmpty() || !items.value(i).valid)
			v.append(i);
	}
	if (pv)
		*pv = v;

	return !v.isEmpty();
}

QString Server::getBadStatusString(__int64 status)
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
		temp.append(QObject::tr("drunk")); // 酒醉
	if (checkAND(status, BC_FLG_CONFUSION))
		temp.append(QObject::tr("confused")); // 混亂
	if (checkAND(status, BC_FLG_HIDE))
		temp.append(QObject::tr("hidden")); // 是否隱藏，地球一周
	if (checkAND(status, BC_FLG_REVERSE))
		temp.append(QObject::tr("reverse")); // 反轉
	if (checkAND(status, BC_FLG_WEAKEN))
		temp.append(QObject::tr("weaken")); // 虛弱
	if (checkAND(status, BC_FLG_DEEPPOISON))

		temp.append(QObject::tr("deep poison")); // 劇毒
	if (checkAND(status, BC_FLG_BARRIER))
		temp.append(QObject::tr("barrier")); // 魔障
	if (checkAND(status, BC_FLG_NOCAST))
		temp.append(QObject::tr("no cast")); // 沈默
	if (checkAND(status, BC_FLG_SARS))
		temp.append(QObject::tr("sars")); // 毒煞蔓延
	if (checkAND(status, BC_FLG_DIZZY))
		temp.append(QObject::tr("dizzy")); // 暈眩
	if (checkAND(status, BC_FLG_ENTWINE))
		temp.append(QObject::tr("entwine")); // 樹根纏繞
	if (checkAND(status, BC_FLG_DRAGNET))
		temp.append(QObject::tr("dragnet")); // 天羅地網
	if (checkAND(status, BC_FLG_ICECRACK))
		temp.append(QObject::tr("ice crack")); // 冰爆術
	if (checkAND(status, BC_FLG_OBLIVION))
		temp.append(QObject::tr("oblivion")); // 遺忘
	if (checkAND(status, BC_FLG_ICEARROW))
		temp.append(QObject::tr("ice arrow")); // 冰箭
	if (checkAND(status, BC_FLG_BLOODWORMS))
		temp.append(QObject::tr("blood worms")); // 嗜血蠱
	if (checkAND(status, BC_FLG_SIGN))
		temp.append(QObject::tr("sign")); // 一針見血
	if (checkAND(status, BC_FLG_CARY))
		temp.append(QObject::tr("cary")); // 挑撥
	if (checkAND(status, BC_FLG_F_ENCLOSE))
		temp.append(QObject::tr("fire enclose")); // 火附體
	if (checkAND(status, BC_FLG_I_ENCLOSE))
		temp.append(QObject::tr("ice enclose")); // 冰附體
	if (checkAND(status, BC_FLG_T_ENCLOSE))
		temp.append(QObject::tr("thunder enclose")); // 雷附體
	if (checkAND(status, BC_FLG_WATER))
		temp.append(QObject::tr("water enclose")); // 水附體
	if (checkAND(status, BC_FLG_FEAR))
		temp.append(QObject::tr("fear")); // 恐懼
	if (checkAND(status, BC_FLG_CHANGE))
		temp.append(QObject::tr("change")); // 雷爾變身

	return temp.join(" ");
}

QString Server::getFieldString(__int64 field)
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

__int64 Server::getDir()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();
	__int64 dir = static_cast<__int64>((mem::read<int>(hProcess, hModule + kOffsetDir) + 5) % 8);
	if (pc_.dir != dir)
	{
		pc_.dir = dir;
		QPoint point = nowPoint_;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2(%3)").arg(point.x()).arg(point.y()).arg(g_dirStrHash.value(getDir())));
	}

	return dir;
}

QPoint Server::getPoint()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	if (hModule == 0)
		return QPoint{};

	HANDLE hProcess = injector.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return QPoint{};

	int x = mem::read<int>(hProcess, hModule + kOffsetNowX);
	int y = mem::read<int>(hProcess, hModule + kOffsetNowY);

	QPoint point(x, y);

	if (nowPoint_ != point)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2(%3)").arg(point.x()).arg(point.y()).arg(g_dirStrHash.value(getDir())));
		nowPoint_ = point;
	}

	return point;
}

__int64 Server::getFloor()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	if (hModule == 0)
		return 0;

	HANDLE hProcess = injector.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return 0;

	__int64 floor = static_cast<__int64>(mem::read<int>(hProcess, hModule + kOffsetNowFloor));
	if (floor != nowFloor_.load(std::memory_order_acquire))
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		QString mapname = nowFloorName_.get();
		emit signalDispatcher.updateNpcList(floor);
		emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(mapname).arg(floor));
		nowFloor_.store(floor, std::memory_order_release);
	}
	return floor;
}

QString Server::getFloorName()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	if (hModule == 0)
		return "";

	HANDLE hProcess = injector.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return "";

	QString mapname = mem::readString(hProcess, hModule + kOffsetNowFloorName, FLOOR_NAME_LEN, true);
	makeStringFromEscaped(mapname);
	if (mapname != nowFloorName_)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		__int64 floor = nowFloor_.load(std::memory_order_acquire);
		emit signalDispatcher.updateNpcList(floor);
		emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(mapname).arg(floor));
		nowFloorName_ = mapname;
	}

	return mapname;
}

//檢查指定任務狀態，並同步等待封包返回
__int64 Server::checkJobDailyState(const QString& missionName)
{
	QString newMissionName = missionName.simplified();
	if (newMissionName.isEmpty())
		return false;

	IS_WAITFOR_JOBDAILY_FLAG.store(true, std::memory_order_release);
	lssproto_JOBDAILY_send(const_cast<char*>("dyedye"));
	QElapsedTimer timer; timer.start();

	for (;;)
	{
		if (timer.hasExpired(2000))
			break;

		if (!IS_WAITFOR_JOBDAILY_FLAG.load(std::memory_order_acquire))
			break;

		QThread::msleep(100);
	}

	IS_WAITFOR_JOBDAILY_FLAG.store(false, std::memory_order_release);

	bool isExact = true;

	if (newMissionName.startsWith("?"))
	{
		isExact = false;
		newMissionName = newMissionName.mid(1);
	}

	QHash<__int64, JOBDAILY> jobdaily = getJobDailys();
	for (const JOBDAILY& it : jobdaily)
	{
		if (!isExact && (it.explain == newMissionName))
			return it.state == QString("已完成") ? 1 : 2;
		else if (!isExact && it.explain.contains(newMissionName))
			return it.state == QString("已完成") ? 1 : 2;
	}

	return 0;
}

//查找指定類型和名稱的單位
bool Server::findUnit(const QString& nameSrc, __int64 type, mapunit_t* punit, const QString& freenameSrc, __int64 modelid)
{
	QList<mapunit_t> units = mapUnitHash.values();

	QString newSrcName = nameSrc.simplified();
	QStringList nameSrcList = newSrcName.split(rexOR, Qt::SkipEmptyParts);

	QString newSrcFreeName = freenameSrc.simplified();
	QStringList freeNameSrcList = newSrcFreeName.split(rexOR);

	//coord
	if (nameSrcList.size() == 2)
	{
		do
		{
			bool ok = false;
			QPoint point;
			point.setX(nameSrcList.value(0).simplified().toLongLong(&ok));
			if (!ok)
				break;

			point.setY(nameSrcList.value(1).simplified().toLongLong(&ok));
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
					setCharFaceToPoint(point);
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

QString Server::getGround()
{
	return mapAnalyzer->getGround(getFloor(), getFloorName(), nowPoint_);
}

//查找非滿血自己寵物或隊友的索引 (主要用於自動吃肉)
__int64 Server::findInjuriedAllie()
{
	PC pc = getPC();
	if (pc.hp < pc.maxHp)
		return 0;

	__int64 i = 0;
	QHash<__int64, PET> pet = getPets();
	for (i = 0; i < MAX_PET; ++i)
	{
		if ((pet.value(i).hp > 0) && (pet.value(i).hp < pet.value(i).maxHp))
			return i + 1;
	}

	QHash<__int64, PARTY> party = getParties();
	for (i = 0; i < MAX_PARTY; ++i)
	{
		if ((party.value(i).hp > 0) && (party.value(i).hp < party.value(i).maxHp))
			return i + 1 + MAX_PET;
	}

	return 0;
}

//根據名稱和索引查找寵物是否存在
bool Server::matchPetNameByIndex(__int64 index, const QString& cmpname)
{
	if (index < 0 || index >= MAX_PET)
		return false;

	QString newCmpName = cmpname.simplified();
	if (newCmpName.isEmpty())
		return false;

	PET pet = pet_.value(index);
	if (!pet.valid)
		return false;

	QString name = pet.name.simplified();
	QString freeName = pet.freeName.simplified();

	if (!name.isEmpty() && newCmpName == name)
		return true;
	if (!freeName.isEmpty() && newCmpName == freeName)
		return true;

	return false;
}

__int64 Server::getProfessionSkillIndexByName(const QString& names) const
{
	__int64 i = 0;
	bool isExact = true;
	QStringList list = names.split(rexOR, Qt::SkipEmptyParts);
	QHash <__int64, PROFESSION_SKILL> profession_skill = getSkills();

	for (QString name : list)
	{
		if (name.isEmpty())
			continue;

		if (name.startsWith("?"))
		{
			name = name.mid(1);
			isExact = false;
		}


		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			if (!profession_skill.value(i).valid)
				continue;

			if (isExact && profession_skill.value(i).name == name)
				return i;
			else if (!isExact && profession_skill.value(i).name.contains(name))
				return i;
		}
	}
	return -1;
}
#pragma endregion

#pragma region SET
//更新敵我索引範圍
void Server::updateCurrentSideRange(battledata_t& bt)
{
	if (battleCharCurrentPos < 10)
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
}

//根據索引刷新道具資訊
void Server::refreshItemInfo(__int64 i)
{
	QVariant var;
	QVariantList varList;

	if (i < 0 || i >= MAX_ITEM)
		return;

	ITEM item = getItem(i);

	if (i < CHAR_EQUIPPLACENUM)
	{
		QStringList equipVHeaderList = {
			QObject::tr("head"), QObject::tr("body"), QObject::tr("righthand"), QObject::tr("leftacc"),
			QObject::tr("rightacc"), QObject::tr("belt"), QObject::tr("lefthand"), QObject::tr("shoes"),
			QObject::tr("gloves")
		};

		if (!item.name.isEmpty() && (item.valid))
		{
			if (item.name2.isEmpty())
				varList = { equipVHeaderList.value(i), item.name, item.damage,	item.memo };
			else
				varList = { equipVHeaderList.value(i), QString("%1(%2)").arg(item.name).arg(item.name2), item.damage,	item.memo };
		}
		else
		{
			varList = { equipVHeaderList.value(i), "", "",	"" };
		}


		var = QVariant::fromValue(varList);

	}
	else
	{
		if (!item.name.isEmpty() && (item.valid))
		{
			if (item.name2.isEmpty())
				varList = { i - CHAR_EQUIPPLACENUM + 1, item.name, item.stack, item.damage, item.level, item.memo };
			else
				varList = { i - CHAR_EQUIPPLACENUM + 1, QString("%1(%2)").arg(item.name).arg(item.name2), item.stack, item.damage, item.level, item.memo };
		}
		else
		{
			varList = { i - CHAR_EQUIPPLACENUM + 1, "", "", "", "", "" };
		}
		var = QVariant::fromValue(varList);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

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
	for (__int64 i = 0; i < MAX_ITEM; ++i)
	{
		refreshItemInfo(i);
	}
}

//本來應該一次性讀取整個結構體的，但我們不需要這麼多訊息
void Server::updateItemByMemory()
{
	QWriteLocker locker(&itemInfoLock_);

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	constexpr __int64 item_offest = 0x184;
	QHash<__int64, ITEM> items = item_.toHash();
	for (__int64 i = 0; i < MAX_ITEM; ++i)
	{
		items[i].valid = mem::read<short>(hProcess, hModule + 0x422C028 + i * item_offest) > 0;
		if (!items[i].valid)
		{
			items.remove(i);
			continue;
		}

		items[i].name = mem::readString(hProcess, hModule + 0x422C032 + i * item_offest, ITEM_NAME_LEN, true, false);
		makeStringFromEscaped(items[i].name);
		items[i].memo = mem::readString(hProcess, hModule + 0x422C060 + i * item_offest, ITEM_MEMO_LEN, true, false);
		makeStringFromEscaped(items[i].memo);
		if (i >= CHAR_EQUIPPLACENUM)
			items[i].stack = mem::read<short>(hProcess, hModule + 0x422BF58 + i * item_offest);

		if (items.value(i).stack == 0)
			items[i].stack = 1;
	}

	item_ = items;
}

//讀取內存刷新各種基礎數據，有些封包數據不明確、或不確定，用來補充不足的部分
void Server::updateDatasFromMemory()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	__int64 i = 0;
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	QWriteLocker lockerPet(&petInfoLock_);
	QWriteLocker lockerChar(&charInfoLock_);
	QHash<__int64, PET> pets = pet_.toHash();

	std::ignore = getDir();

	//每隻寵物如果處於等待或戰鬥則為1
	short selectPetNo[MAX_PET] = { 0i16, 0i16 ,0i16 ,0i16 ,0i16 };
	mem::read(hProcess, hModule + kOffsetSelectPetArray, sizeof(selectPetNo), selectPetNo);
	for (i = 0; i < MAX_PET; ++i)
		pc_.selectPetNo[i] = static_cast<__int64>(selectPetNo[i]);

	//郵件寵物索引
	__int64 mailPetIndex = static_cast<__int64>(mem::read<short>(hProcess, hModule + kOffsetMailPetIndex));
	if (mailPetIndex < 0 || mailPetIndex >= MAX_PET)
		mailPetIndex = -1;

	pc_.mailPetNo = mailPetIndex;

	//騎乘寵物索引
	__int64 ridePetIndex = static_cast<__int64>(mem::read<short>(hProcess, hModule + kOffsetRidePetIndex));
	if (ridePetIndex < 0 || ridePetIndex >= MAX_PET)
		ridePetIndex = -1;

	if (ridePetIndex >= 0 && ridePetIndex < MAX_PET)
		emit signalDispatcher.updateRideHpProgressValue(pets.value(ridePetIndex).level, pets.value(ridePetIndex).hp, pets.value(ridePetIndex).maxHp);
	else
		emit signalDispatcher.updateRideHpProgressValue(0, 0, 0);
	pc_.ridePetNo = ridePetIndex;


	__int64 battlePetIndex = static_cast<__int64>(mem::read<short>(hProcess, hModule + kOffsetBattlePetIndex));
	if (battlePetIndex < 0 || battlePetIndex >= MAX_PET)
		battlePetIndex = -1;

	if (battlePetIndex >= 0 && battlePetIndex < MAX_PET)
		emit signalDispatcher.updatePetHpProgressValue(pets.value(battlePetIndex).level, pets.value(battlePetIndex).hp, pets.value(battlePetIndex).maxHp);
	else
		emit signalDispatcher.updatePetHpProgressValue(0, 0, 0);
	pc_.battlePetNo = battlePetIndex;

	__int64 standyPetCount = static_cast<__int64>(mem::read<short>(hProcess, hModule + kOffsetStandbyPetCount));
	pc_.standbyPet = standyPetCount;

	//人物狀態 (是否組隊或其他..)
	pc_.status = mem::read<short>(hProcess, hModule + kOffsetCharStatus);

	bool isInTeam = mem::read<short>(hProcess, hModule + kOffsetTeamState) > 0;
	if (isInTeam && !checkAND(pc_.status, CHR_STATUS_PARTY))
		pc_.status |= CHR_STATUS_PARTY;
	else if (!isInTeam && checkAND(pc_.status, CHR_STATUS_PARTY))
		pc_.status &= (~CHR_STATUS_PARTY);

	for (__int64 i = 0; i < MAX_PET; ++i)
	{
		if (i == pc_.mailPetNo)
		{
			pets[i].state = kMail;
		}
		else if (i == pc_.ridePetNo)
		{
			pets[i].state = kRide;
		}
		else if (i == pc_.battlePetNo)
		{
			pets[i].state = kBattle;
		}
		else if (pc_.selectPetNo[i] > 0)
		{
			pets[i].state = kStandby;
		}
		else
		{
			pets[i].state = kRest;
		}
		emit signalDispatcher.updateCharInfoPetState(i, pets[i].state);
	}

	pet_ = pets;
}

//刷新要顯示的戰鬥時間和相關數據
void Server::updateBattleTimeInfo()
{
	double time = battleDurationTimer.elapsed() / 1000.0;
	double cost = battle_one_round_time.load(std::memory_order_acquire) / 1000.0;
	double total_time = battle_total_time.load(std::memory_order_acquire) / 1000.0 / 60.0;

	QString battle_time_text = QString(QObject::tr("%1 count no %2 round duration: %3 sec cost: %4 sec total time: %5 minues"))
		.arg(battle_total.load(std::memory_order_acquire))
		.arg(battleCurrentRound.load(std::memory_order_acquire) + 1)
		.arg(toQString(time))
		.arg(toQString(cost))
		.arg(toQString(total_time));

	__int64 bp = battleBpFlag.load(std::memory_order_acquire);
	if (bp <= 0)
		battle_time_text += " " + QObject::tr("(normal)");
	else if (checkAND(battleBpFlag.load(std::memory_order_acquire), BATTLE_BP_PLAYER_SURPRISAL))
		battle_time_text += " " + QObject::tr("(surprise)");
	else if (checkAND(battleBpFlag.load(std::memory_order_acquire), BATTLE_BP_ENEMY_SURPRISAL))
		battle_time_text += " " + QObject::tr("(be surprised)");

	if (battle_time_text.isEmpty() || timeLabelContents != battle_time_text)
	{
		timeLabelContents = battle_time_text;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		emit signalDispatcher.updateBattleTimeLabelTextChanged(battle_time_text.simplified());
	}
}

void Server::swapItemLocal(__int64 from, __int64 to)
{
	if (from < 0 || to < 0)
		return;

	QWriteLocker locker(&itemInfoLock_);
	QHash<__int64, ITEM> items = item_.toHash();
	ITEM tmp = items.take(from);
	items.insert(from, items.value(to));
	items.insert(to, tmp);
	item_ = items;
}

void Server::setWorldStatus(__int64 w)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	mem::write<int>(injector.getProcess(), injector.getProcessModule() + kOffsetWorldStatus, w);
}

void Server::setGameStatus(__int64 g)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	mem::write<int>(injector.getProcess(), injector.getProcessModule() + kOffsetGameStatus, g);
}

//切換是否在戰鬥中的標誌
void Server::setBattleFlag(bool enable)
{
	IS_BATTLE_FLAG.store(enable, std::memory_order_release);
	isBattleDialogReady.store(false, std::memory_order_release);
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();

	//這裡關乎頭上是否會出現V.S.圖標
	__int64 status = mem::read<short>(hProcess, hModule + kOffsetCharStatus);
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

	mem::write<int>(hProcess, hModule + kOffsetCharStatus, status);
	mem::write<int>(hProcess, hModule + kOffsetBattleStatus, enable ? 1 : 0);

	mem::write<int>(hProcess, hModule + 0x4169B6C, 0);

	mem::write<int>(hProcess, hModule + 0x4181198, 0);
	mem::write<int>(hProcess, hModule + 0x41829FC, 0);

	mem::write<int>(hProcess, hModule + 0x415EF9C, 0);
	mem::write<int>(hProcess, hModule + 0x4181D44, 0);
	mem::write<int>(hProcess, hModule + 0x41829F8, 0);

	mem::write<int>(hProcess, hModule + 0x415F4EC, 30);
}

void Server::setOnlineFlag(bool enable)
{
	IS_ONLINE_FLAG.store(enable, std::memory_order_release);


	if (!enable)
	{
		setBattleEnd();
	}
}

void Server::setWindowTitle()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 subServer = static_cast<__int64>(mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetSubServerIndex));//injector.getValueHash(kServerValue);
	//__int64 subserver = 0;//injector.getValueHash(kSubServerValue);
	__int64 position = static_cast<__int64>(mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetPositionIndex));//injector.getValueHash(kPositionValue);

	QString subServerName;
	__int64 size = injector.subServerNameList.get().size();
	if (subServer >= 0 && subServer < size)
		subServerName = injector.subServerNameList.get().value(subServer);
	else
		subServerName = "0";

	QString positionName;
	if (position >= 0 && position < MAX_CHARACTER - 1)
		positionName = position == 0 ? QObject::tr("left") : QObject::tr("right");
	else
		positionName = toQString(position);

	PC pc = pc_;
	QString title = QString("[%1] SaSH [%2:%3] - %4 Lv:%5 HP:%6/%7 MP:%8/%9 $:%10") \
		.arg(currentIndex).arg(subServerName).arg(positionName).arg(pc.name).arg(pc.level).arg(pc.hp).arg(pc.maxHp).arg(pc.mp).arg(pc.maxMp).arg(pc.gold);
	std::wstring wtitle = title.toStdWString();
	SetWindowTextW(injector.getProcessWindow(), wtitle.c_str());
}

void Server::setPoint(const QPoint& point)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 hModule = injector.getProcessModule();
	if (hModule == 0)
		return;

	HANDLE hProcess = injector.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return;

	if (nowPoint_ != point)
	{
		nowPoint_ = point;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2(%3)").arg(point.x()).arg(point.y()).arg(g_dirStrHash.value(getDir())));
	}

	if (mem::read<int>(hProcess, hModule + kOffsetNowX) != point.x())
		mem::write<int>(hProcess, hModule + kOffsetNowX, point.x());
	if (mem::read<int>(hProcess, hModule + kOffsetNowY) != point.y())
		mem::write<int>(hProcess, hModule + kOffsetNowY, point.y());
}

//清屏 (實際上就是 char數組置0)
void Server::cleanChatHistory()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kCleanChatHistory, NULL, NULL);
	chatQueue.clear();
	if (!injector.chatLogModel.isNull())
		injector.chatLogModel->clear();
}

void Server::updateComboBoxList()
{
	__int64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	__int64 battlePetIndex = -1;

	QStringList itemList;
	QHash <__int64, ITEM> items = getItems();
	for (const ITEM& it : items)
	{
		if (it.name.isEmpty())
			continue;
		itemList.append(it.name);
	}

	emit signalDispatcher.updateComboBoxItemText(kComboBoxItem, itemList);

	QFontMetrics fontMetrics(QApplication::font());

	QStringList magicNameList;
	for (__int64 i = 0; i < MAX_MAGIC; ++i)
	{
		MAGIC magic = getMagic(i);
		__int64 textWidth = fontMetrics.horizontalAdvance(magic.name);
		QString shortText = QString(QObject::tr("(cost:%1)")).arg(magic.costmp);
		__int64 shortcutWidth = fontMetrics.horizontalAdvance(shortText);
		constexpr __int64 totalWidth = 120;
		__int64 spaceCount = (totalWidth - textWidth - shortcutWidth) / fontMetrics.horizontalAdvance(' ');

		QString alignedText = magic.name + QString(spaceCount, ' ') + shortText;

		magicNameList.append(alignedText);
	}

	for (__int64 i = 0; i < MAX_PROFESSION_SKILL; ++i)
	{
		PROFESSION_SKILL profession_skill = getSkill(i);
		if (profession_skill.valid)
		{
			if (profession_skill.name.size() == 3)
				profession_skill.name += "　";

			__int64 textWidth = fontMetrics.horizontalAdvance(profession_skill.name);
			QString shortText = QString("(%1%)").arg(profession_skill.skill_level);
			__int64 shortcutWidth = fontMetrics.horizontalAdvance(shortText);
			constexpr __int64 totalWidth = 140;
			__int64 spaceCount = (totalWidth - textWidth - shortcutWidth) / fontMetrics.horizontalAdvance(' ');

			if (i < 9)
				profession_skill.name += "  ";

			QString alignedText = profession_skill.name + QString(spaceCount, ' ') + shortText;

			magicNameList.append(alignedText);
		}
		else
			magicNameList.append("");

	}
	emit signalDispatcher.updateComboBoxItemText(kComboBoxCharAction, magicNameList);
	battlePetIndex = getPC().battlePetNo;


	if (battlePetIndex >= 0)
	{
		QStringList skillNameList;
		for (__int64 i = 0; i < MAX_SKILL; ++i)
		{
			PET_SKILL petSkill = getPetSkill(battlePetIndex, i);
			skillNameList.append(petSkill.name + ":" + petSkill.memo);
		}
		emit signalDispatcher.updateComboBoxItemText(kComboBoxPetAction, skillNameList);
	}
}
#pragma endregion

#pragma region System
//公告
void Server::announce(const QString& msg, __int64 color)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	HANDLE hProcess = injector.getProcess();
	if (!msg.isEmpty())
	{
		std::string str = fromQUnicode(msg);
		mem::VirtualMemory ptr(hProcess, str.size(), true);
		mem::write(hProcess, ptr, const_cast<char*>(str.c_str()), str.size());
		injector.sendMessage(kSendAnnounce, ptr, color);
	}
	else
	{
		mem::VirtualMemory ptr(hProcess, "", mem::VirtualMemory::kAnsi, true);
		injector.sendMessage(kSendAnnounce, ptr, color);
	}
	chatQueue.enqueue(qMakePair(color, msg));
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendChatLog(msg, color);
}

//喊話
void Server::talk(const QString& text, __int64 color, TalkMode mode)
{
	if (text.startsWith("//skup"))
	{
		lssproto_CustomTK_recv(text);
		return;
	}


	if (color < 0 || color > 10)
		color = 0;

	__int64 flg = getPC().etcFlag;
	QString msg = "P|";
	if (mode == kTalkGlobal)
		msg += ("/XJ ");
	else if (mode == kTalkFamily)
		msg += ("/FM ");
	else if (mode == kTalkWorld)
		msg += ("/WD ");
	else if (mode == kTalkTeam)
	{
		__int64 newflg = flg;
		if (!checkAND(newflg, PC_ETCFLAG_PARTY_CHAT))
		{
			newflg |= PC_ETCFLAG_PARTY_CHAT;
			setSwitcher(newflg);
			QThread::msleep(500);
		}
	}

	msg += text;
	std::string str = fromQUnicode(msg);
	lssproto_TK_send(nowPoint_, const_cast<char*>(str.c_str()), color, 3);
}

//創建人物
void Server::createCharacter(__int64 dataplacenum
	, const QString& charname
	, __int64 imgno
	, __int64 faceimgno
	, __int64 vit
	, __int64 str
	, __int64 tgh
	, __int64 dex
	, __int64 earth
	, __int64 water
	, __int64 fire
	, __int64 wind
	, __int64 hometown
	, bool forcecover)
{

	if (!forcecover)
	{
		//防複寫保護
		if (getCharListTable(dataplacenum).valid)
			return;
	}

	std::string sname = fromQUnicode(charname);
	lssproto_CreateNewChar_send(dataplacenum, const_cast<char*>(sname.c_str()), imgno, faceimgno, vit, str, tgh, dex, earth, water, fire, wind, hometown);

	if (!checkWG(3, 11))
		return;

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();
	mem::write<int>(hProcess, hModule + 0x421C000, 1);
	__int64 time = timeGetTime();
	mem::write<int>(hProcess, hModule + 0x421C004, time);
	mem::write<int>(hProcess, hModule + 0x4152B44, 2);

	setWorldStatus(2);
	setGameStatus(2);
}

void Server::deleteCharacter(__int64 index, const QString password, bool backtofirst)
{
	if (index < 0 || index > MAX_CHARACTER)
		return;

	if (!checkWG(3, 11))
		return;

	CHARLISTTABLE table = getCharListTable(index);

	if (!table.valid)
		return;
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();

	mem::write<int>(hProcess, hModule + 0x4230A88, index);
	mem::writeString(hProcess, hModule + 0x421BF74, table.name);

	std::string sname = fromQUnicode(table.name);
	std::string spassword = fromQUnicode(password);
	lssproto_CharDelete_send(const_cast<char*>(sname.c_str()), const_cast<char*>(spassword.c_str()));

	mem::write<int>(hProcess, hModule + 0x421C000, 1);
	mem::write<int>(hProcess, hModule + 0x415EF6C, 2);

	__int64 time = timeGetTime();
	mem::write<int>(hProcess, hModule + 0x421C004, time);

	setGameStatus(21);

	if (backtofirst)
	{
		setWorldStatus(1);
		setGameStatus(2);
	}
}

//老菜單
void Server::shopOk(__int64 n)
{
	//SE 1隨身倉庫 2查看聲望氣勢
	lssproto_ShopOk_send(n);
}

//新菜單
void Server::saMenu(__int64 n)
{
	lssproto_SaMenu_send(n);
}

//切換單一開關
void Server::setSwitcher(__int64 flg, bool enable)
{
	PC pc = getPC();
	if (enable)
		pc.etcFlag |= flg;
	else
		pc.etcFlag &= ~flg;

	setSwitcher(pc.etcFlag);
}

//切換全部開關
void Server::setSwitcher(__int64 flg)
{
	lssproto_FS_send(flg);
}

bool Server::isDialogVisible()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();

	bool bret = mem::read<int>(hProcess, hModule + 0xB83EC) != -1;
	bool custombret = mem::read<int>(hProcess, hModule + 0x4200000) > 0;
	return bret && custombret;
}
#pragma endregion

#pragma region Connection
//元神歸位
void Server::EO()
{
	lssproto_EO_send(0);
	lastEOTime.store(-1, std::memory_order_release);
	isEOTTLSend.store(true, std::memory_order_release);
	eottlTimer.restart();
	lssproto_Echo_send(const_cast<char*>("hoge"));
	__int64 currentIndex = getIndex();
	//石器私服SE SO專用
	Injector& injector = Injector::getInstance(currentIndex);
	QString cmd = injector.getStringHash(kEOCommandString);
	if (!cmd.isEmpty())
		talk(cmd);
}
//登出
void Server::logOut()
{
	lssproto_CharLogout_send(0);
	setWorldStatus(7);
	setGameStatus(0);
}

//回點
void Server::logBack()
{
	lssproto_CharLogout_send(1);
}

void Server::clientLogin(const QString& userName, const QString& password)
{
	//std::string sname = fromQUnicode(userName);
	//std::string spassword = fromQUnicode(password);
	//MyMACAddr m;
	//std::string mac = m.GenRandMAC();
	//std::string ip = "192.168.1.1";
	//__int64 serverIndex = 0;

	//char userId[32] = {};
	//char userPassword[32] = {};

	//_snprintf_s(userId, sizeof(userId), _TRUNCATE, "%s", sname.c_str());
	//_snprintf_s(userPassword, sizeof(userPassword), _TRUNCATE, "%s", spassword.c_str());

	//lssproto_ClientLogin_send(userId
	//	, userPassword
	//	, const_cast<char*>(mac.c_str())
	//	, serverIndex
	//	, const_cast<char*>(ip.c_str())
	//	, WITH_CDKEY | WITH_PASSWORD | WITH_MACADDRESS);
}

void Server::playerLogin(__int64 index)
{
	if (index < 0 || index >= MAX_CHARACTER)
		return;
	CHARLISTTABLE chartable = getCharListTable(index);
	if (!chartable.valid)
		return;

	std::string name = fromQUnicode(chartable.name);

	lssproto_CharLogin_send(const_cast<char*>(name.c_str()));
}

//登入
bool Server::login(__int64 s)
{
	UnLoginStatus status = static_cast<UnLoginStatus>(s);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();

	__int64 server = injector.getValueHash(kServerValue);
	__int64 subserver = injector.getValueHash(kSubServerValue);
	__int64 position = injector.getValueHash(kPositionValue);
	QString account = injector.getStringHash(kGameAccountString);
	QString password = injector.getStringHash(kGamePasswordString);

	Config config;
	QElapsedTimer timer; timer.start();

	auto backToFirstPage = [this, &signalDispatcher, &injector, s]()
	{
		if (s == kStatusInputUser)
			return;

		injector.setEnableHash(kAutoLoginEnable, false);
		emit signalDispatcher.applyHashSettingsToUI();

		setWorldStatus(7);
		setGameStatus(0);
	};

	auto input = [this, &signalDispatcher, &injector, hProcess, hModule, &backToFirstPage, s, &account, &password]()->bool
	{

		QString acct = mem::readString(hProcess, hModule + kOffsetAccount, 32);
		QString pwd = mem::readString(hProcess, hModule + kOffsetPassword, 32);
		QString acctECB = mem::readString(hProcess, hModule + kOffsetAccountECB, 32, false, true);
		QString pwdECB = mem::readString(hProcess, hModule + kOffsetPasswordECB, 32, false, true);

		if (account.isEmpty())
		{
			//檢查是否已手動輸入帳號
			if (!acct.isEmpty())
			{
				account = acct;
				injector.setStringHash(kGameAccountString, account);
				emit signalDispatcher.applyHashSettingsToUI();
			}
		}
		else //寫入配置中的帳號
			mem::writeString(hProcess, hModule + kOffsetAccount, account);

		if (password.isEmpty())
		{
			//檢查是否已手動輸入密碼
			if (!pwd.isEmpty())
			{
				password = pwd;
				injector.setStringHash(kGamePasswordString, password);
				emit signalDispatcher.applyHashSettingsToUI();
			}
		}
		else //寫入配置中的密碼
			mem::writeString(hProcess, hModule + kOffsetPassword, password);

		if (account.isEmpty() && password.isEmpty() && acctECB.isEmpty() && pwdECB.isEmpty())
		{
			emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusNoUsernameAndPassword);
			backToFirstPage();
			return false;
		}
		else if (account.isEmpty() && acctECB.isEmpty())
		{
			emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusNoUsername);
			backToFirstPage();
			return false;
		}
		else if (password.isEmpty() && pwdECB.isEmpty())
		{
			emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusNoPassword);
			backToFirstPage();
			return false;
		}

		//如果用戶手動輸入帳號與配置不同
		if (account != acct && s == kStatusInputUser || acctECB.isEmpty() && s != kStatusInputUser)
		{
			backToFirstPage();
			return false;
		}

		//如果用戶手動輸入密碼與配置不同
		if (password != pwd && s == kStatusInputUser || pwdECB.isEmpty() && s != kStatusInputUser)
		{
			backToFirstPage();
			return false;
		}

		return true;
#ifndef USE_MOUSE
		//std::string saccount = fromQUnicode(account);
		//std::string spassword = fromQUnicode(password);

		////sa_8001.exe+2086A - 09 09                 - or [ecx],ecx
		//char userAccount[32] = {};
		//char userPassword[32] = {};
		//_snprintf_s(userAccount, sizeof(userAccount), _TRUNCATE, "%s", saccount.c_str());
		//sacrypt::ecb_crypt("f;encor1c", userAccount, sizeof(userAccount), sacrypt::DES_ENCRYPT);
		//mem::write(hProcess, hModule + kOffsetAccountECB, userAccount, sizeof(userAccount));
		//qDebug() << "before encode" << account << "after encode" << QString(userAccount);

		//_snprintf_s(userPassword, sizeof(userPassword), _TRUNCATE, "%s", spassword.c_str());
		//sacrypt::ecb_crypt("f;encor1c", userPassword, sizeof(userPassword), sacrypt::DES_ENCRYPT);
		//mem::write(hProcess, hModule + kOffsetPasswordECB, userPassword, sizeof(userPassword));
		//qDebug() << "before encode" << password << "after encode" << QString(userPassword);
	};

	switch (status)
	{
	case kStatusDisconnect:
	{
		IS_DISCONNECTED.store(true, std::memory_order_release);
		QList<int> list = config.readArray<int>("System", "Login", "Disconnect");
		if (list.size() == 2)
			injector.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			injector.leftDoubleClick(315, 270);
			config.writeArray<int>("System", "Login", "Disconnect", { 315, 270 });
		}
		break;
	}
	case kStatusLoginFailed:
	{
		QList<int> list = config.readArray<int>("System", "Login", "LoginFailed");
		if (list.size() == 2)
			injector.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			injector.leftDoubleClick(315, 255);
			config.writeArray<int>("System", "Login", "LoginFailed", { 315, 255 });
		}
		break;
	}
	case kStatusBusy:
	{
		QList<int> list = config.readArray<int>("System", "Login", "Busy");
		if (list.size() == 2)
			injector.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			injector.leftDoubleClick(315, 255);
			config.writeArray<int>("System", "Login", "Busy", { 315, 255 });
		}
		break;
	}
	case kStatusTimeout:
	{
		QList<int> list = config.readArray<int>("System", "Login", "Timeout");
		if (list.size() == 2)
			injector.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			injector.leftDoubleClick(315, 253);
			config.writeArray<int>("System", "Login", "Timeout", { 315, 253 });
		}
		break;
	}
	case kNoUserNameOrPassword:
	{
		backToFirstPage();
#ifdef USE_MOUSE
		QList<int> list = config.readArray<int>("System", "Login", "NoUserNameOrPassword");
		if (list.size() == 2)
			injector.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			injector.leftDoubleClick(315, 253);
			config.writeArray<int>("System", "Login", "NoUserNameOrPassword", { 315, 253 });
		}
#endif
		break;
	}
	case kStatusInputUser:
	{
		if (!input())
			break;

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

			if (timer.hasExpired(1500))
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
				injector.leftDoubleClick(list.value(0), list.value(1));
			else
			{
				injector.leftDoubleClick(380, 310);
				config.writeArray<int>("System", "Login", "OK", { 380, 310 });
			}
		}
	}

#endif

	break;
	}
	case kStatusSelectServer:
	{
		if (!input())
			break;

		if (server < 0 && server >= 15)
			break;

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

			if (timer.hasExpired(1500))
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
		constexpr __int64 table[48] = {
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

		const __int64 a = table[server * 3 + 1];
		const __int64 b = table[server * 3 + 2];

		__int64 x = 160 + (a * 125);
		__int64 y = 165 + (b * 25);

		QList<int> list = config.readArray<int>("System", "Login", "SelectServer");
		if (list.size() == 4)
		{
			x = list.value(0) + (a * list.value(1));
			y = list.value(2) + (b * list.value(3));
		}
		else
		{
			config.writeArray<int>("System", "Login", "SelectServer", { 160, 125, 165, 25 });
		}

		for (;;)
		{
			injector.mouseMove(x, y);
			__int64 value = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetMousePointedIndex);
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
	case kStatusSelectSubServer:
	{
		if (!input())
			break;

		if (subserver < 0 || subserver >= 15)
			break;

		__int64 serverIndex = static_cast<__int64>(mem::read<int>(hProcess, hModule + kOffsetServerIndex));

#ifndef USE_MOUSE
		/*
		sa_8001.exe+2186C - 8D 0C D2              - lea ecx,[edx+edx*8]
		sa_8001.exe+2186F - C1 E1 03              - shl ecx,03 { 3 }

		sa_8001.exe+21881 - 66 8B 89 2CEDEB04     - mov cx,[ecx+sa_8001.exe+4ABED2C]
		sa_8001.exe+21888 - 66 03 C8              - add cx,ax

		sa_8001.exe+2189B - 66 89 0D 88424C00     - mov [sa_8001.exe+C4288],cx { (23) }

		*/

		__int64 ecxValue = serverIndex + (serverIndex * 8);
		ecxValue <<= 3;
		__int64 cxValue = mem::read<short>(hProcess, ecxValue + hModule + 0x4ABED2C);
		cxValue += subserver;

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

			if (timer.hasExpired(1500))
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
			__int64 x = 500;
			__int64 y = 340;

			QList<int> list = config.readArray<int>("System", "Login", "SelectSubServerGoBack");
			if (list.size() == 2)
			{
				x = list.value(0);
				y = list.value(1);
			}
			else
			{
				config.writeArray<int>("System", "Login", "SelectSubServer", QList<int>{ 500, 340 });
			}

			for (;;)
			{
				injector.mouseMove(x, y);
				__int64 value = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetMousePointedIndex);
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
			__int64 x = 250;
			__int64 y = 265 + (subserver * 20);

			QList<int> list = config.readArray<int>("System", "Login", "SelectSubServer");
			if (list.size() == 3)
			{
				x = list.value(0);
				y = list.value(1) + (subserver * list.value(2));
			}
			else
			{
				config.writeArray<int>("System", "Login", "SelectSubServer", { 250, 265, 20 });
			}

			for (;;)
			{
				injector.mouseMove(x, y);
				__int64 value = mem::read<int>(injector.getProcess(), injector.getProcessModule() + kOffsetMousePointedIndex);
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
	case kStatusSelectCharacter:
	{
		if (position < 0 || position > MAX_CHARACTER)
			break;

		if (!getCharListTable(position).valid)
			break;

		//#ifndef USE_MOUSE
				//playerLogin(position);
				//setGameStatus(1);
				//setWorldStatus(5);
				//QThread::msleep(1000);
		//#else
		__int64 x = 100 + (position * 300);
		__int64 y = 340;

		QList<int> list = config.readArray<int>("System", "Login", "SelectCharacter");
		if (list.size() == 3)
		{
			x = list.value(0) + (position * list.value(1));
			y = list.value(2);
		}
		else
		{
			config.writeArray<int>("System", "Login", "SelectCharacter", { 100, 300, 340 });
		}

		injector.leftDoubleClick(x, y);
		//#endif
		break;
	}
	case kStatusConnecting:
	{
		if (connectingTimer.hasExpired(10000))
		{
			setWorldStatus(7);
			setGameStatus(0);
			connectingTimer.restart();
		}
		break;
	}
	case kStatusLogined:
	{
		IS_DISCONNECTED.store(false, std::memory_order_release);
		return true;
	}
	default:
	{
		break;
	}
}
return false;
}

#pragma endregion

#pragma region WindowPacket
//創建對話框
void Server::createRemoteDialog(quint64 type, quint64 button, const QString& text)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	mem::VirtualMemory ptr(injector.getProcess(), text, mem::VirtualMemory::kAnsi, true);

	injector.sendMessage(kCreateDialog, MAKEWPARAM(type, button), ptr);
}

void Server::press(BUTTON_TYPE select, __int64 dialogid, __int64 unitid)
{
	if (select == BUTTON_CLOSE)
	{
		__int64 currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		injector.sendMessage(kDistoryDialog, NULL, NULL);
		return;
	}

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
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

void Server::press(__int64 row, __int64 dialogid, __int64 unitid)
{
	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;
	QString qrow = toQString(row);
	std::string srow = fromQUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

//買東西
void Server::buy(__int64 index, __int64 amt, __int64 dialogid, __int64 unitid)
{
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
	std::string srow = fromQUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

//賣東西
void Server::sell(const QString& name, const QString& memo, __int64 dialogid, __int64 unitid)
{
	if (name.isEmpty())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QVector<__int64> indexs;
	if (!getItemIndexsByName(name, memo, &indexs, CHAR_EQUIPPLACENUM))
		return;

	sell(indexs, dialogid, unitid);
}

//賣東西
void Server::sell(__int64 index, __int64 dialogid, __int64 unitid)
{
	if (index < 0 || index >= MAX_ITEM)
		return;

	ITEM item = getItem(index);
	if (item.name.isEmpty() || !item.valid)
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qrow = QString("%1\\z%2").arg(index).arg(item.stack);
	std::string srow = fromQUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

//賣東西
void Server::sell(const QVector<__int64>& indexs, __int64 dialogid, __int64 unitid)
{
	if (indexs.isEmpty())
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QStringList list;
	for (const __int64 it : indexs)
	{
		if (it < 0 || it >= MAX_ITEM)
			continue;

		sell(it, dialogid, unitid);
	}
}

//寵物學技能
void Server::learn(__int64 skillIndex, __int64 petIndex, __int64 spot, __int64 dialogid, __int64 unitid)
{
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
	std::string srow = fromQUnicode(qrow);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

void Server::depositItem(__int64 itemIndex, __int64 dialogid, __int64 unitid)
{
	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = toQString(itemIndex + 1);
	std::string srow = fromQUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

void Server::withdrawItem(__int64 itemIndex, __int64 dialogid, __int64 unitid)
{
	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = toQString(itemIndex + 1);
	std::string srow = fromQUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);

	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

void Server::depositPet(__int64 petIndex, __int64 dialogid, __int64 unitid)
{
	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = toQString(petIndex + 1);
	std::string srow = fromQUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

void Server::withdrawPet(__int64 petIndex, __int64 dialogid, __int64 unitid)
{
	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = toQString(petIndex + 1);
	std::string srow = fromQUnicode(qstr);
	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_NOTUSED, const_cast<char*>(srow.c_str()));
}

//遊戲對話框輸入文字送出
void Server::inputtext(const QString& text, __int64 dialogid, __int64 unitid)
{
	dialog_t dialog = currentDialog;
	if (dialogid == -1)
		dialogid = dialog.dialogid;
	if (unitid == -1)
		unitid = dialog.unitid;
	std::string s = fromQUnicode(text);

	lssproto_WN_send(getPoint(), dialogid, unitid, BUTTON_OK, const_cast<char*>(s.c_str()));
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.sendMessage(kDistoryDialog, NULL, NULL);
}

//解除安全瑪
void Server::unlockSecurityCode(const QString& code)
{
	if (code.isEmpty())
		return;

	std::string scode = fromQUnicode(code);
	lssproto_WN_send(getPoint(), kDialogSecurityCode, -1, NULL, const_cast<char*>(scode.c_str()));
}

void Server::windowPacket(const QString& command, __int64 dialogid, __int64 unitid)
{
	//SI|itemIndex(0-15)|Stack(-1)
	//TI|itemIndex(0-59)|Stack(-1)
	//SP|petIndex(0-4)|
	//TP|petIndex(0-?)|
	//SG|gold|
	//TG|gold|
	std::string s = fromQUnicode(command);
	lssproto_WN_send(getPoint(), dialogid, unitid, NULL, const_cast<char*>(s.c_str()));
}
#pragma endregion

#pragma region CHAR
//使用精靈
void Server::useMagic(__int64 magicIndex, __int64 target)
{
	if (target < 0 || target >= (MAX_PET + MAX_PARTY))
		return;

	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return;

	if (magicIndex < MAX_MAGIC && !isCharMpEnoughForMagic(magicIndex))
		return;

	if (magicIndex >= 0 && magicIndex < MAX_MAGIC && !getMagic(magicIndex).valid)
		return;

	lssproto_MU_send(nowPoint_, magicIndex, target);
}

//組隊或離隊 true 組隊 false 離隊
void Server::setTeamState(bool join)
{
	lssproto_PR_send(nowPoint_, join ? 1 : 0);
}

void Server::kickteam(__int64 n)
{
	if (n >= MAX_PARTY)
		return;

	if (n <= 0)
	{
		setTeamState(false);
		return;
	}

	if (getParty(n).valid)
		lssproto_KTEAM_send(n);
}

void Server::mail(const QVariant& card, const QString& text, __int64 petIndex, const QString& itemName, const QString& itemMemo)
{
	__int64 index = -1;
	if (card.type() == QVariant::Type::Int || card.type() == QVariant::Type::LongLong)
		index = card.toLongLong();
	else if (card.type() == QVariant::Type::String)
	{
		bool isExact = false;
		QString name = card.toString();
		if (name.startsWith("?"))
		{
			name = name.mid(1);
			isExact = true;
		}

		QHash<__int64, ADDRESS_BOOK> addressBooks = getAddressBooks();
		for (auto it = addressBooks.constBegin(); it != addressBooks.constEnd(); ++it)
		{
			if (!it.value().valid)
				continue;

			if (isExact)
			{
				if (name == it.value().name)
				{
					index = it.key();
					break;
				}
			}
			else
			{
				if (it.value().name.contains(name))
				{
					index = it.key();
					break;
				}
			}
		}
	}

	if (index < 0 || index > MAX_ADDRESS_BOOK)
		return;

	ADDRESS_BOOK addressBook = getAddressBook(index);
	if (!addressBook.valid)
		return;

	std::string sstr = fromQUnicode(text);
	if (itemName.isEmpty() && itemMemo.isEmpty() && (petIndex < 0 || petIndex > MAX_PET))
	{
		lssproto_MSG_send(index, const_cast<char*>(sstr.c_str()), NULL);
	}
	else
	{
		if (!addressBook.onlineFlag)
			return;

		__int64 itemIndex = getItemIndexByName(itemName, true, itemMemo, CHAR_EQUIPPLACENUM);

		PET pet = getPet(petIndex);
		if (!pet.valid)
			return;

		if (pet.state != kMail)
			setPetState(petIndex, kMail);

		lssproto_PMSG_send(index, petIndex, itemIndex, const_cast<char*>(sstr.c_str()), NULL);
		if (itemIndex >= 0 && itemIndex < MAX_ITEM)
		{
			IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
		}
	}
}

//加點
bool Server::addPoint(__int64 skillid, __int64 amt)
{
	if (skillid < 0 || skillid > 4)
		return false;

	if (amt < 1)
		return false;

	PC pc = getPC();

	if (amt > pc.point)
		amt = pc.point;

	QElapsedTimer timer; timer.start();
	for (__int64 i = 0; i < amt; ++i)
	{
		IS_WAITFOT_SKUP_RECV.store(true, std::memory_order_release);
		lssproto_SKUP_send(skillid);
		for (;;)
		{
			if (timer.hasExpired(1000))
				break;

			if (getBattleFlag())
				return false;

			if (!getOnlineFlag())
				return false;

			if (!IS_WAITFOT_SKUP_RECV.load(std::memory_order_acquire))
				break;
		}
		timer.restart();
	}
	return true;
}

//人物改名
void Server::setCharFreeName(const QString& name)
{
	std::string sname = fromQUnicode(name);
	lssproto_FT_send(const_cast<char*> (sname.c_str()));
}

//寵物改名
void Server::setPetFreeName(__int64 petIndex, const QString& name)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	std::string sname = fromQUnicode(name);
	lssproto_KN_send(petIndex, const_cast<char*> (sname.c_str()));
}
#pragma endregion

#pragma region PET
//設置寵物狀態 (戰鬥 | 等待 | 休息 | 郵件 | 騎乘)
void Server::setPetState(__int64 petIndex, PetState state)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (getBattleFlag())
		return;

	if (!getOnlineFlag())
		return;

	updateDatasFromMemory();

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	{
		QWriteLocker locker(&petInfoLock_);
		QWriteLocker locker2(&charInfoLock_);

		PET pet = pet_.value(petIndex);
		if (!pet.valid)
			return;

		switch (state)
		{
		case kBattle:
		{
			if (pc_.battlePetNo != petIndex)
			{
				setFightPet(-1);
			}

			if (pc_.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc_.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			setPetStandby(petIndex, state);

			setFightPet(petIndex);

			//mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, petIndex);
			//pc.selectPetNo[petIndex] = 1;
			//mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 1);

			break;
		}
		case kStandby:
		{
			if (pc_.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc_.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, -1);
				pc_.selectPetNo[petIndex] = 0;
				mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc_.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc_.selectPetNo[petIndex] = 1;
			mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 1);
			setPetStateSub(petIndex, 1);
			setPetStandby(petIndex, state);
			break;
		}
		case kMail:
		{
			if (pc_.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc_.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, -1);
				pc_.selectPetNo[petIndex] = 0;
				mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc_.mailPetNo >= 0 && pc_.mailPetNo != petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetMailPetIndex, -1);
				setPetStateSub(pc_.mailPetNo, 0);
			}

			mem::write<short>(hProcess, hModule + kOffsetMailPetIndex, petIndex);
			pc_.selectPetNo[petIndex] = 0;
			mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setPetStateSub(petIndex, 4);
			setPetStandby(petIndex, state);
			break;
		}
		case kRest:
		{
			if (pc_.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc_.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, -1);
				mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc_.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc_.selectPetNo[petIndex] = 0;
			mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setPetStateSub(petIndex, 0);
			setPetStandby(petIndex, state);
			break;
		}
		case kRide:
		{
			if (pet.loyal != 100)
				break;

			if (pc_.ridePetNo != -1)
			{
				mem::write<short>(hProcess, hModule + kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc_.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, -1);
				pc_.selectPetNo[petIndex] = 0;
				mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc_.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc_.selectPetNo[petIndex] = 0;
			mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setRidePet(petIndex);
			setPetStandby(petIndex, state);
			break;
		}
		default:
			break;
		}
	}

	updateDatasFromMemory();
}

void Server::setAllPetState()
{
	QHash <__int64, PET> pet = getPets();
	for (__int64 i = 0; i < MAX_PET; ++i)
	{
		__int64 state = 0;
		switch (pet.value(i).state)
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
void Server::setFightPet(__int64 petIndex)
{
	lssproto_KS_send(petIndex);
}

//設置騎乘寵物
void Server::setRidePet(__int64 petIndex)
{
	QString str = QString("R|P|%1").arg(petIndex);
	std::string sstr = fromQUnicode(str);
	lssproto_FM_send(const_cast<char*>(sstr.c_str()));
}

//設置寵物狀態封包  0:休息 1:戰鬥或等待 4:郵件
void Server::setPetStateSub(__int64 petIndex, __int64 state)
{
	lssproto_PETST_send(petIndex, state);
}

//設置寵物等待狀態
void Server::setPetStandby(__int64 petIndex, __int64 state)
{
	quint64 standby = 0;
	__int64 count = 0;
	PC pc = pc_;
	for (__int64 i = 0; i < MAX_PET; ++i)
	{
		if ((state == 0 || state == 4) && petIndex == i)
			continue;

		if (pc.selectPetNo[i] > 0)
		{
			standby += 1ll << i;
			++count;
		}
	}

	lssproto_SPET_send(standby);
	pc_.standbyPet = count;
	Injector& injector = Injector::getInstance(getIndex());
	mem::write<short>(injector.getProcess(), injector.getProcessModule() + kOffsetStandbyPetCount, count);
}

//丟棄寵物
void Server::dropPet(__int64 petIndex)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	PET pet = getPet(petIndex);
	if (!pet.valid)
		return;

	lssproto_DP_send(nowPoint_, petIndex);

	updateDatasFromMemory();
}

//自動鎖寵
void Server::checkAutoLockPet()
{
	Injector& injector = Injector::getInstance(getIndex());

	__int64 lockedIndex = -1;
	bool enableLockRide = injector.getEnableHash(kLockRideEnable) && !injector.getEnableHash(kLockPetScheduleEnable);
	if (enableLockRide)
	{
		__int64 lockRideIndex = injector.getValueHash(kLockRideValue);
		if (lockRideIndex >= 0 && lockRideIndex < MAX_PET)
		{
			PET pet = getPet(lockRideIndex);
			if (pet.state != PetState::kRide)
			{
				lockedIndex = lockRideIndex;
				setPetState(lockRideIndex, kRide);
			}
		}
	}

	bool enableLockPet = injector.getEnableHash(kLockPetEnable) && !injector.getEnableHash(kLockPetScheduleEnable);
	if (enableLockPet)
	{
		__int64 lockPetIndex = injector.getValueHash(kLockPetValue);
		if (lockPetIndex >= 0 && lockPetIndex < MAX_PET)
		{
			PET pet = getPet(lockPetIndex);
			if (pet.state != PetState::kBattle)
			{
				if (lockedIndex == lockPetIndex)
					return;

				setPetState(lockPetIndex, kBattle);
			}
		}
	}
}

//自動加點
void Server::checkAutoAbility()
{
	Injector& injector = Injector::getInstance(getIndex());
	auto checkEnable = [this, &injector]()->bool
	{
		if (isInterruptionRequested())
			return false;

		if (!injector.getEnableHash(kAutoAbilityEnable))
			return false;

		if (!getOnlineFlag())
			return false;

		if (getBattleFlag())
			return false;

		return true;
	};

	if (!checkEnable())
		return;

	QString strAbility = injector.getStringHash(kAutoAbilityString);
	if (strAbility.isEmpty())
		return;

	QStringList abilityList = strAbility.split(rexOR, Qt::SkipEmptyParts);
	if (abilityList.isEmpty())
		return;

	static const QHash<QString, __int64> abilityNameHash = {
		{ "vit", 0 },
		{ "str", 1 },
		{ "tgh", 2 },
		{ "dex", 3 },

		{ "體", 0 },
		{ "腕", 1 },
		{ "耐", 2 },
		{ "速", 3 },

		{ "体", 0 },
		{ "腕", 1 },
		{ "耐", 2 },
		{ "速", 3 },
	};

	for (const QString& ability : abilityList)
	{
		if (!checkEnable())
			return;

		if (ability.isEmpty())
			continue;

		QStringList abilityInfo = ability.split(rexComma, Qt::SkipEmptyParts);
		if (abilityInfo.isEmpty())
			continue;

		if (abilityInfo.size() != 2)
			continue;

		QString abilityName = abilityInfo.at(0);
		QString abilityValue = abilityInfo.at(1);

		if (abilityName.isEmpty() || abilityValue.isEmpty())
			continue;

		if (!abilityNameHash.contains(abilityName))
			continue;

		__int64 value = abilityValue.toInt();
		if (value <= 0)
			continue;

		__int64 abilityIndex = abilityNameHash.value(abilityName, -1);
		if (abilityIndex == -1)
			continue;

		PC pc = getPC();
		QVector<__int64> ability = { pc.vit, pc.str, pc.tgh, pc.dex };
		__int64 abilityPoint = ability.value(abilityIndex, -1);
		if (abilityPoint == -1)
			continue;

		if (abilityPoint >= value)
			continue;

		__int64 abilityPointLeft = pc.point;
		if (abilityPointLeft <= 0)
			continue;

		__int64 abilityPointNeed = value - abilityPoint;
		if (abilityPointNeed > abilityPointLeft)
			abilityPointNeed = abilityPointLeft;

		addPoint(abilityIndex, abilityPointNeed);
	}
}

//檢查並自動吃肉、或丟肉
void Server::checkAutoDropMeat()
{
	Injector& injector = Injector::getInstance(getIndex());
	auto checkEnable = [this, &injector]()->bool
	{
		if (isInterruptionRequested())
			return false;

		if (!injector.getEnableHash(kAutoDropMeatEnable))
			return false;

		if (!getOnlineFlag())
			return false;

		if (getBattleFlag())
			return false;

		return true;
	};

	if (!checkEnable())
		return;

	bool bret = false;
	constexpr const char* meat = "?肉";
	constexpr const char* memo = "耐久力";


	QVector<__int64> items;
	if (!getItemIndexsByName(meat, "", &items, CHAR_EQUIPPLACENUM, MAX_ITEM))
	{
		return;
	}

	for (const __int64 index : items)
	{
		if (!checkEnable())
			return;

		ITEM item = item_.value(index);

		if (!item.valid)
			continue;

		for (__int64 i = 0; i < item.stack; ++i)
		{
			if (!checkEnable())
				return;

			QString newItemNmae = item.name.simplified();
			QString newItemMemo = item.memo.simplified();
			if ((newItemNmae != QString("味道爽口的肉湯")) && (newItemNmae != QString("味道爽口的肉汤")))
			{
				if (!newItemMemo.contains(memo))
					dropItem(index);//不可補且非肉湯肉丟棄
				else
					useItem(index, findInjuriedAllie());//優先餵給非滿血
			}
		}
	}

	refreshItemInfo();
}
#pragma endregion

#pragma region MAP
//下載指定坐標 24 * 24 大小的地圖塊
void Server::downloadMap(__int64 x, __int64 y, __int64 floor)
{
	QMutexLocker locker(&net_mutex);
	lssproto_M_send(floor == -1 ? nowFloor_.load(std::memory_order_acquire) : floor, x, y, x + 24, y + 24);
}

//下載全部地圖塊
void Server::downloadMap(__int64 floor)
{
	bool IsDownloadingMap = true;

	__int64 original = floor;

	if (floor == -1)
		floor = getFloor();

	map_t map;
	mapAnalyzer->readFromBinary(floor, getFloorName());
	mapAnalyzer->getMapDataByFloor(floor, &map);

	__int64 downloadMapXSize_ = map.width;
	__int64 downloadMapYSize_ = map.height;

	if (!downloadMapXSize_)
	{
		downloadMapXSize_ = 240;
	}

	if (!downloadMapYSize_)
	{
		downloadMapYSize_ = 240;
	}
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	constexpr __int64 MAX_BLOCK_SIZE = 24;

	__int64 downloadMapX_ = 0;
	__int64 downloadMapY_ = 0;
	__int64 downloadCount_ = 0;

	const __int64 numBlocksX = (downloadMapXSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	const __int64 numBlocksY = (downloadMapYSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	const __int64 totalBlocks = numBlocksX * numBlocksY;
	qreal totalMapBlocks_ = static_cast<qreal>(totalBlocks);

	qreal downloadMapProgress_ = 0.0;

	QString title;
	std::wstring wtitle;

	announce(QString("floor %1 downloadMapXSize: %2 downloadMapYSize: %3 totalBlocks: %4").arg(floor).arg(downloadMapXSize_).arg(downloadMapYSize_).arg(totalBlocks));
	QElapsedTimer timer; timer.start();

	do
	{
		downloadMap(downloadMapX_, downloadMapY_, floor);

		__int64 blockWidth = qMin(MAX_BLOCK_SIZE, downloadMapXSize_ - downloadMapX_);
		__int64 blockHeight = qMin(MAX_BLOCK_SIZE, downloadMapYSize_ - downloadMapY_);

		// 移除一個小區塊
		downloadMapX_ += blockWidth;
		if (downloadMapX_ >= downloadMapXSize_)
		{
			downloadMapX_ = 0;
			downloadMapY_ += blockHeight;
		}

		// 更新下載進度
		downloadCount_++;
		downloadMapProgress_ = static_cast<qreal>(downloadCount_) / totalMapBlocks_ * 100.0;

		// 更新下載進度
		title = QString("downloading floor %1 - %2%").arg(floor).arg(toQString(downloadMapProgress_));
		wtitle = title.toStdWString();
		SetWindowTextW(injector.getProcessWindow(), wtitle.c_str());

		if (downloadMapProgress_ >= 100.0)
		{
			break;
		}

	} while (IsDownloadingMap);

	//清空尋路地圖數據、數據重讀、圖像重繪
	mapAnalyzer->clear(floor);
	announce(QString("floor %1 complete cost: %2 ms").arg(floor).arg(timer.elapsed()));
	announce(QString("floor %1 reload now").arg(floor));
	timer.restart();
	mapAnalyzer->readFromBinary(floor, getFloorName(), false, true);
	announce(QString("floor %1 reload complete cost: %2 ms").arg(floor).arg(timer.elapsed()));
}

//轉移
void Server::warp()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	struct
	{
		short ev = 0;
		short dialogid = 0;
	}ev;

	if (mem::read(injector.getProcess(), injector.getProcessModule() + kOffsetEV, sizeof(ev), &ev))
		lssproto_EV_send(ev.ev, ev.dialogid, nowPoint_, -1);
}

//計算方向
QString calculateDirection(__int64 currentX, __int64 currentY, __int64 targetX, __int64 targetY)
{
	QString table = "abcdefgh";
	QPoint src(currentX, currentY);
	for (const QPoint& it : fix_point)
	{
		if (it + src == QPoint(targetX, targetY))
		{
			__int64 index = fix_point.indexOf(it);
			return table.mid(index, 1);
		}
	}

	return "";
}

//移動(封包) [a-h]
void Server::move(const QPoint& p, const QString& dir)
{
	if (p.x() < 0 || p.x() > 1500 || p.y() < 0 || p.y() > 1500)
		return;

	std::string sdir = fromQUnicode(dir);
	lssproto_W2_send(p, const_cast<char*>(sdir.c_str()));
	std::ignore = getPoint();
}

//移動(記憶體)
void Server::move(const QPoint& p)
{
	QMutexLocker locker(&net_mutex);
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.isValid())
		injector.sendMessage(kSetMove, p.x(), p.y());
}

//轉向指定坐標
__int64 Server::setCharFaceToPoint(const QPoint& pos)
{
	__int64 dir = -1;
	QPoint current = nowPoint_;
	for (const QPoint& it : fix_point)
	{
		if (it + current == pos)
		{
			dir = fix_point.indexOf(it);
			setCharFaceDirection(dir);
			return dir;
		}
	}
	return -1;
}

//轉向 (根據方向索引自動轉換成A-H)
void Server::setCharFaceDirection(__int64 dir)
{
	if (dir < 0 || dir >= MAX_DIR)
		return;

	const QString dirchr = "ABCDEFGH";
	if (dir >= dirchr.size())
		return;

	QString dirStr = dirchr.at(dir);
	std::string sdirStr = fromQUnicode(dirStr.toUpper());
	lssproto_W2_send(getPoint(), const_cast<char*>(sdirStr.c_str()));

	if (getBattleFlag())
		return;

	//這裡是用來使遊戲動畫跟著轉向
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();
	__int64 newdir = (dir + 3) % 8;
	__int64 p = static_cast<__int64>(mem::read<int>(hProcess, hModule + 0x422E3AC));
	if (p > 0)
	{
		mem::write<int>(hProcess, hModule + 0x422BE94, newdir);
		mem::write<int>(hProcess, p + 0x150, newdir);
	}
}

//轉向 使用方位字符串
void Server::setCharFaceDirection(const QString& dirStr)
{
	static const QHash<QString, QString> dirhash = {
		{ "北", "A" }, { "東北", "B" }, { "東", "C" }, { "東南", "D" },
		{ "南", "E" }, { "西南", "F" }, { "西", "G" }, { "西北", "H" },
		{ "北", "A" }, { "东北", "B" }, { "东", "C" }, { "东南", "D" },
		{ "南", "E" }, { "西南", "F" }, { "西", "G" }, { "西北", "H" }
	};

	__int64 dir = -1;
	QString qdirStr;
	const QString dirchr = "ABCDEFGH";
	if (!dirhash.contains(dirStr.toUpper()))
	{
		QRegularExpression re("[A-H]");
		QRegularExpressionMatch match = re.match(dirStr.toUpper());
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

	std::string sdirStr = fromQUnicode(qdirStr.toUpper());
	lssproto_W2_send(nowPoint_, const_cast<char*>(sdirStr.c_str()));

	if (getBattleFlag())
		return;

	//這裡是用來使遊戲動畫跟著轉向
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();
	__int64 newdir = (dir + 3) % 8;
	__int64 p = static_cast<__int64>(mem::read<int>(hProcess, hModule + 0x422E3AC));
	if (p > 0)
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
	updateItemByMemory();
	getCharMaxCarryingCapacity();

	__int64 j = 0;
	__int64 i = 0;
	QHash<__int64, ITEM> items = getItems();
	PC pc = getPC();

	if (swapitemModeFlag == 0 || !deepSort)
	{
		if (deepSort)
			swapitemModeFlag = 1;

		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (!items.value(i).valid)
					continue;

				//if (!isItemStackable(items.value(i).sendFlag))
				//	continue;

				if (items.value(i).name.isEmpty())
					continue;

				if (items.value(i).name != items.value(j).name)
					continue;

				if (items.value(i).memo != items.value(j).memo)
					continue;

				if (items.value(i).modelid != items.value(j).modelid)
					continue;

				if (pc.maxload <= 0)
					continue;

				QString key = QString("%1|%2|%3").arg(items.value(j).name).arg(items.value(j).memo).arg(items.value(j).modelid);
				if (items.value(j).stack > 1 && !itemStackFlagHash.contains(key))
					itemStackFlagHash.insert(key, true);
				else
				{
					swapItem(i, j);
					itemStackFlagHash.insert(key, false);
					continue;
				}

				if (itemStackFlagHash.contains(key) && !itemStackFlagHash.value(key) && items.value(j).stack == 1)
					continue;

				if (items.value(j).stack >= pc.maxload)
				{
					pc.maxload = items.value(j).stack;
					if (items.value(j).stack != items.value(j).maxStack)
					{
						items[j].maxStack = items.value(j).stack;
						swapItem(i, j);
					}
					continue;
				}

				if (items.value(i).stack > items.value(j).stack)
					continue;

				items[j].maxStack = items.value(j).stack;

				swapItem(i, j);
			}
		}
	}
	else if (swapitemModeFlag == 1 && deepSort)
	{
		swapitemModeFlag = 2;

		//補齊  item[i].valid == false 的空格
		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (!items.value(i).valid)
					continue;

				if (!items.value(j).valid)
				{
					swapItem(i, j);
				}
			}
		}
	}
	else if (deepSort)
	{
		swapitemModeFlag = 0;

		static const QLocale locale;
		static const QCollator collator(locale);

		//按 items.value(i).name 名稱排序
		for (i = MAX_ITEM - 1; i > CHAR_EQUIPPLACENUM; --i)
		{
			for (j = CHAR_EQUIPPLACENUM; j < i; ++j)
			{
				if (!items.value(i).valid)
					continue;

				if (!items.value(j).valid)
					continue;

				if (items.value(i).name.isEmpty())
					continue;

				if (items.value(j).name.isEmpty())
					continue;

				if (items.value(i).name != items.value(j).name)
				{
					if (collator.compare(items.value(i).name, items.value(j).name) < 0)
					{
						swapItem(i, j);
					}
				}
				else if (items.value(i).memo != items.value(j).memo)
				{
					if (collator.compare(items.value(i).memo, items.value(j).memo) < 0)
					{
						swapItem(i, j);
					}
				}
				else if (items.value(i).modelid != items.value(j).modelid)
				{
					if (items.value(i).modelid < items.value(j).modelid)
					{
						swapItem(i, j);
					}
				}
				//數量少的放前面
				else if (items.value(i).stack < items.value(j).stack)
				{
					swapItem(i, j);
				}
			}
		}
	}

	updateItemByMemory();
	refreshItemInfo();
}

//丟棄道具
void Server::dropItem(__int64 index)
{
	QPoint pos = nowPoint_;
	QHash<__int64, ITEM> items = getItems();
	if (index == -1)
	{
		for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
		{
			if (items.value(i).name.isEmpty() || !items.value(i).valid)
				continue;

			__int64 stack = items.value(i).stack;
			for (__int64 j = 0; j < stack; ++j)
				lssproto_DI_send(pos, i);
			IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
		}
	}

	if (index < 0 || index >= MAX_ITEM)
		return;

	if (items.value(index).name.isEmpty() || !items.value(index).valid)
		return;

	for (__int64 j = 0; j < items.value(index).stack; ++j)
		lssproto_DI_send(pos, index);
	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

void Server::dropItem(QVector<__int64> indexs)
{
	for (const __int64 it : indexs)
		dropItem(it);
}

void Server::dropGold(__int64 gold)
{
	PC pc = getPC();
	if (gold > pc.gold)
		gold = pc.gold;

	lssproto_DG_send(nowPoint_, gold);
}

//使用道具
void Server::useItem(__int64 itemIndex, __int64 target)
{
	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	if (target < 0 || target > 9)
		return;

	if (!getItem(itemIndex).valid)
		return;

	lssproto_ID_send(nowPoint_, itemIndex, target);
	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

//交換道具
void Server::swapItem(__int64 from, __int64 to)
{
	if (from < 0 || from >= MAX_ITEM)
		return;

	if (to < 0 || to >= MAX_ITEM)
		return;

	lssproto_MI_send(from, to);

	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

//撿道具
void Server::pickItem(__int64 dir)
{
	lssproto_PI_send(nowPoint_, (dir + 3) % 8);
}

//穿裝 to = -1 丟裝 to = -2 脫裝 to = itemspotindex
void Server::petitemswap(__int64 petIndex, __int64 from, __int64 to)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	if (to != -1)
	{
		if ((from >= CHAR_EQUIPPLACENUM) || (to >= CHAR_EQUIPPLACENUM))
			return;
	}

	lssproto_PetItemEquip_send(nowPoint_, petIndex, from, to);
	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

//料理/加工
void Server::craft(CraftType type, const QStringList& ingres)
{
	if (ingres.size() < 2 || ingres.size() > 5)
		return;

	QStringList itemIndexs;
	__int64 petIndex = -1;

	QString skillName;
	if (type == CraftType::kCraftFood)
		skillName = QString("料理");
	else
		skillName = QString("加工");

	__int64 skillIndex = getPetSkillIndexByName(petIndex, skillName);
	if (petIndex == -1 || skillIndex == -1)
		return;

	for (const QString& it : ingres)
	{
		__int64 index = getItemIndexByName(it, true, "", CHAR_EQUIPPLACENUM);
		if (index == -1)
			return;

		itemIndexs.append(toQString(index));
	}

	QString qstr = itemIndexs.join("|");
	std::string str = fromQUnicode(qstr);
	lssproto_PS_send(petIndex, skillIndex, NULL, const_cast<char*>(str.c_str()));
	IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_add(1, std::memory_order_release);
}

void Server::depositGold(__int64 gold, bool isPublic)
{
	if (gold <= 0)
		return;

	QString qstr = QString("B|%1|%2").arg(!isPublic ? "G" : "T").arg(gold);
	std::string str = fromQUnicode(qstr);
	lssproto_FM_send(const_cast<char*>(str.c_str()));
}

void Server::withdrawGold(__int64 gold, bool isPublic)
{
	if (gold <= 0)
		return;

	QString qstr = QString("B|%1|%2").arg(!isPublic ? "G" : "T").arg(-gold);
	std::string str = fromQUnicode(qstr);
	lssproto_FM_send(const_cast<char*>(str.c_str()));
}

void Server::tradeComfirm(const QString& name)
{
	if (!IS_TRADING.load(std::memory_order_acquire))
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	QString cmd = QString("T|%1|%2|C|confirm").arg(opp_sockfd).arg(opp_name);
	std::string scmd = fromQUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));
}

void Server::tradeCancel()
{
	if (!IS_TRADING.load(std::memory_order_acquire))
		return;

	QString cmd = QString("W|%1|%2").arg(opp_sockfd).arg(opp_name);
	std::string scmd = fromQUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));

	PC pc = getPC();
	pc.trade_confirm = 1;
	setPC(pc);
	tradeStatus = 0;
}

bool Server::tradeStart(const QString& name, __int64 timeout)
{
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

void Server::tradeAppendItems(const QString& name, const QVector<__int64>& itemIndexs)
{
	if (!IS_TRADING.load(std::memory_order_acquire))
		return;

	if (itemIndexs.isEmpty())
		return;

	if (name != opp_name)
	{
		tradeCancel();
		return;
	}

	QHash< __int64, ITEM > items = getItems();
	for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		bool bret = false;
		__int64 stack = items.value(i).stack;
		for (__int64 j = 0; j < stack; ++j)
		{
			QString cmd = QString("T|%1|%2|I|1|%3").arg(opp_sockfd).arg(opp_name).arg(i);
			std::string scmd = fromQUnicode(cmd);
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

void Server::tradeAppendGold(const QString& name, __int64 gold)
{
	if (!IS_TRADING.load(std::memory_order_acquire))
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
	std::string scmd = fromQUnicode(cmd);
	lssproto_TD_send(const_cast<char*>(scmd.c_str()));
	mygoldtrade = gold;
}

void Server::tradeAppendPets(const QString& name, const QVector<__int64>& petIndexs)
{
	if (!IS_TRADING.load(std::memory_order_acquire))
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
	for (const __int64 index : petIndexs)
	{
		if (index < 0 || index >= MAX_PET)
			continue;

		PET pet = getPet(index);
		if (!pet.valid)
			continue;

		QStringList list;
		QHash<__int64, PET_SKILL> petSkill = getPetSkills(index);
		for (const PET_SKILL& it : petSkill)
		{
			if (!it.valid)
				list.append("");
			else
				list.append(it.name);
		}
		list.append(pet.name);
		list.append(pet.freeName);

		QString cmd = QString("T|%1|%2|P|3|%3|%4").arg(opp_sockfd).arg(opp_name).arg(index).arg(list.join("|"));
		std::string scmd = fromQUnicode(cmd);
		lssproto_TD_send(const_cast<char*>(scmd.c_str()));
		list[index] = QString("P|%1").arg(index);
	}
	mypet_tradeList = list;
}

void Server::tradeComplete(const QString& name)
{
	if (!IS_TRADING.load(std::memory_order_acquire))
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
	std::string scmd = fromQUnicode(cmd);
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
void Server::realTimeToSATime(LSTIME* lstime)
{
	constexpr __int64 era = 912766409LL + 5400LL;
	//cary 十五
	__int64 lsseconds = (QDateTime::currentMSecsSinceEpoch() - FirstTime) / 1000LL + serverTime - era;

	lstime->year = lsseconds / (LSTIME_SECONDS_PER_DAY * LSTIME_DAYS_PER_YEAR);

	__int64 lsdays = lsseconds / LSTIME_SECONDS_PER_DAY;
	lstime->day = lsdays % LSTIME_DAYS_PER_YEAR;


	/*(750*12)*/
	lstime->hour = (lsseconds % LSTIME_SECONDS_PER_DAY) * LSTIME_HOURS_PER_DAY / LSTIME_SECONDS_PER_DAY;

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
	if (!getBattleFlag())
		return;

	if (battleDurationTimer.elapsed() > 0ll)
		battle_total_time.fetch_add(battleDurationTimer.elapsed(), std::memory_order_release);

	updateBattleTimeInfo();

	normalDurationTimer.restart();
	battlePetDisableList_.clear();

	lssproto_EO_send(0);

	setBattleFlag(false);

	if (getWorldStatus() == 10)
		setGameStatus(7);

	QString temp;
	QStringList tempList = {};
	QVector<QStringList> topList;
	QVector<QStringList> bottomList;
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	emit signalDispatcher.battleTableAllItemResetColor();
	emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusInNormal);

	battledata_t bt = getBattleData();
	QSet <__int64> tempSet;
	for (battleobject_t& obj : bt.objects)
	{
		tempList.clear();
		temp.clear();
		tempList.append(toQString(obj.pos));
		tempSet.insert(obj.pos);
		QString statusStr = getBadStatusString(obj.status);
		if (!statusStr.isEmpty())
			statusStr = QString("(%1)").arg(statusStr);

		if (obj.pos == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			temp = QString("　[%1]★%2%3 LV:%4(%5|%6)%7")
				.arg(obj.pos + 1)
				.arg("")
				.arg(obj.name)
				.arg(obj.level)
				.arg(obj.hp)
				.arg(obj.hpPercent)
				.arg(statusStr);
		}
		else
		{
			if (obj.pos >= bt.enemymin && obj.pos <= bt.enemymax)
				temp = QString("　[%1]%2%3 LV:%4(%5|%6)%7")
				.arg(obj.pos + 1)
				.arg(QString::number(obj.modelid) + ":")
				.arg(obj.name)
				.arg(obj.level)
				.arg(obj.hp)
				.arg(obj.hpPercent)
				.arg(statusStr);
			else
				temp = QString("　[%1]%2%3 LV:%4(%5|%6)%7")
				.arg(obj.pos + 1)
				.arg("")
				.arg(obj.name)
				.arg(obj.level)
				.arg(obj.hp)
				.arg(obj.hpPercent)
				.arg(statusStr);
		}

		tempList.append(temp);
		temp.clear();

		if (obj.rideFlag == 1)
		{
			temp = QString(",%1LV:%2(%3)")
				.arg(QObject::tr("R"))
				.arg(obj.rideLevel)
				.arg(obj.rideHp);
		}

		tempList.append(temp);

		if ((obj.pos >= bt.alliemin) && (obj.pos <= bt.alliemax))
		{
			bottomList.append(tempList);
		}
		else if ((obj.pos >= bt.enemymin) && (obj.pos < bt.enemymax))
		{
			topList.append(tempList);
		}
	}

	for (__int64 i = 0; i < MAX_ENEMY; ++i)
	{
		if (tempSet.contains(i))
			continue;

		tempList.clear();
		tempList.append(toQString(i));
		//               *[00]000000 Lv:000 HP:0000/0000 (000)(999990)
		tempList.append("                                     ");
		tempList.append("                                     ");

		if ((i >= bt.alliemin) && (i <= bt.alliemax))
		{
			bottomList.append(tempList);
		}
		else if ((i >= bt.enemymin) && (i < bt.enemymax))
		{
			topList.append(tempList);
		}
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	std::ignore = QtConcurrent::run(this, &Server::checkAutoLockPet);
#else
	std::ignore = QtConcurrent::run(&Server::checkAutoLockPet, this);
#endif
}

inline bool Server::checkFlagState(__int64 pos)
{
	if (pos < 0 || pos >= MAX_ENEMY)
		return false;
	return checkAND(battleCurrentAnimeFlag.load(std::memory_order_acquire), 1ll << pos);
}

//異步處理自動/快速戰鬥邏輯和發送封包
void Server::doBattleWork(bool waitforBA)
{
	if (waitforBA)
	{
		//asyncBattleAction(waitforBA);
		__int64 recordedRound = battleCurrentRound.load(std::memory_order_acquire);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		std::ignore = QtConcurrent::run(this, &Server::asyncBattleAction, waitforBA);
#else
		std::ignore = QtConcurrent::run(&Server::asyncBattleAction, this, waitforBA);
#endif
		std::ignore = QtConcurrent::run([this, recordedRound, waitforBA]()
			{
				//備用
				Injector& injector = Injector::getInstance(getIndex());
				__int64 delay = injector.getValueHash(kBattleActionDelayValue);
				__int64 resendDelay = injector.getValueHash(kBattleResendDelayValue);
				if (resendDelay <= 0)
					return;

				bool fastChecked = injector.getEnableHash(kFastBattleEnable);
				bool normalChecked = injector.getEnableHash(kAutoBattleEnable);
				QElapsedTimer timer; timer.start();
				for (;;)
				{
					if (isInterruptionRequested())
						return;

					if (!fastChecked && !normalChecked)
						return;

					if (recordedRound != battleCurrentRound.load(std::memory_order_acquire))
						return;

					if (!getOnlineFlag())
						return;

					if (!getBattleFlag())
						return;

					if (timer.hasExpired(resendDelay + delay))
					{
						announce(QObject::tr("[warn]Battle command transmission timeout, initiating backup instructions."));
						break;
					}
				}

				battledata_t bt = getBattleData();
				bt.charAlreadyAction = false;
				bt.petAlreadyAction = false;
				setBattleData(bt);
				asyncBattleAction(waitforBA);
			});
	}
	else
	{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		std::ignore = QtConcurrent::run(this, &Server::asyncBattleAction, waitforBA);
#else
		std::ignore = QtConcurrent::run(&Server::asyncBattleAction, this, waitforBA);
#endif
	}
}

void Server::asyncBattleAction(bool waitforBA)
{
	if (!getOnlineFlag())
		return;

	if (isInterruptionRequested())
		return;

	constexpr __int64 MAX_DELAY = 100;

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//自動戰鬥打開 或 快速戰鬥打開且處於戰鬥場景
	bool fastChecked = injector.getEnableHash(kFastBattleEnable);
	bool normalChecked = injector.getEnableHash(kAutoBattleEnable);
	fastChecked = fastChecked || (normalChecked && getWorldStatus() == 9);
	normalChecked = normalChecked || (fastChecked && getWorldStatus() == 10);
	if (normalChecked && !checkWG(10, 4) || (!fastChecked && !normalChecked))
	{
		return;
	}

	auto delay = [&injector, this]()
	{
		//戰鬥延時
		__int64 delay = injector.getValueHash(kBattleActionDelayValue);
		if (delay <= 0)
			return;

		if (delay > 1000)
		{
			__int64 maxDelaySize = delay / 1000;
			for (__int64 i = 0; i < maxDelaySize; ++i)
			{
				QThread::msleep(1000);
				if (isInterruptionRequested())
					return;

				if (!getOnlineFlag())
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
			//mem::write<short>(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1);
			__int64 G = getGameStatus();
			if (G == 4)
			{
				setGameStatus(5);
				isBattleDialogReady.store(false, std::memory_order_release);
			}
		}

		//這里不發的話一般戰鬥、和快戰都不會再收到後續的封包 (應該?)
		if (injector.getEnableHash(kBattleAutoEOEnable))
			lssproto_EO_send(0);
	};

	battledata_t bt = getBattleData();
	//人物和宠物分开发 TODO 修正多个BA人物多次发出战斗指令的问题
	if (!bt.charAlreadyAction)//!checkFlagState(battleCharCurrentPos) &&
	{
		bt.charAlreadyAction = true;

		delay();
		//解析人物战斗逻辑并发送指令
		playerDoBattleWork(bt);
	}

	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
	{
		bt.petAlreadyAction = true;

		setCurrentRoundEnd();
		return;
	}

	//TODO 修正宠物指令在多个BA时候重复发送的问题
	if (!bt.petAlreadyAction)
	{
		bt.petAlreadyAction = true;

		petDoBattleWork(bt);

		setCurrentRoundEnd();
	}
}

//人物戰鬥
__int64 Server::playerDoBattleWork(const battledata_t& bt)
{
	if (hasUnMoveableStatue(bt.player.status))
	{
		sendBattleCharDoNothing();
		return 1;
	}

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	do
	{
		if (bt.objects.isEmpty() || bt.enemies.isEmpty() || bt.allies.isEmpty() || bt.objects.value(battleCharCurrentPos).hp <= 0)
			break;

		if (battleCharCurrentPos >= MAX_ENEMY
			|| checkAND(battleBpFlag.load(std::memory_order_acquire), BATTLE_BP_PLAYER_MENU_NON)/*觀戰*/)
		{
			sendBattleCharDoNothing();
			break;
		}

		//自動逃跑
		if (injector.getEnableHash(kAutoEscapeEnable))
		{
			sendBattleCharEscapeAct();
			break;
		}

		handleCharBattleLogics(bt);

	} while (false);

	//if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
	//	mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//else
	//

	//
	return 1;
}

//寵物戰鬥
__int64 Server::petDoBattleWork(const battledata_t& bt)
{
	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return 0;

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	do
	{
		if (bt.objects.isEmpty() || bt.enemies.isEmpty() || bt.allies.isEmpty() || bt.objects.value(battleCharCurrentPos + 5).hp <= 0)
			break;

		//自動逃跑
		if (hasUnMoveableStatue(bt.pet.status)
			|| injector.getEnableHash(kAutoEscapeEnable)
			|| petEscapeEnableTempFlag.load(std::memory_order_acquire)
			|| checkAND(battleBpFlag.load(std::memory_order_acquire), BATTLE_BP_PET_MENU_NON))
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
bool Server::checkCharHp(__int64 cmpvalue, __int64* target, bool useequal)
{
	PC pc = getPC();
	if (useequal && (pc.hpPercent <= cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire);
		return true;
	}
	else if (!useequal && (pc.hpPercent < cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire);
		return true;
	}

	return false;
};

//檢查人物氣力
bool Server::checkCharMp(__int64 cmpvalue, __int64* target, bool useequal)
{
	PC pc = getPC();
	if (useequal && (pc.mpPercent <= cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire);
		return true;
	}
	else if (!useequal && (pc.mpPercent < cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire);
		return true;
	}

	return  false;
};

//檢測戰寵血量
bool Server::checkPetHp(__int64 cmpvalue, __int64* target, bool useequal)
{
	PC pc = getPC();

	__int64 i = pc.battlePetNo;
	if (i < 0 || i >= MAX_PET)
		return false;

	PET pet = getPet(i);

	if (useequal && (pet.hpPercent <= cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		return true;
	}
	else if (!useequal && (pet.hpPercent < cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		return true;
	}

	return false;
};

//檢測騎寵血量
bool Server::checkRideHp(__int64 cmpvalue, __int64* target, bool useequal)
{
	PC pc = getPC();

	__int64 i = pc.ridePetNo;
	if (i < 0 || i >= MAX_PET)
		return false;

	PET pet = getPet(i);

	if (useequal && (pet.hpPercent <= cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		return true;
	}
	else if (!useequal && (pet.hpPercent < cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		return true;
	}

	return false;
};

//檢測隊友血量
bool Server::checkPartyHp(__int64 cmpvalue, __int64* target)
{
	if (!target)
		return false;

	PC pc = getPC();
	if (!checkAND(pc.status, CHR_STATUS_PARTY) && !checkAND(pc.status, CHR_STATUS_LEADER))
		return false;

	QHash<__int64, PARTY> party = getParties();
	for (__int64 i = 0; i < MAX_PARTY; ++i)
	{
		if (party.value(i).hpPercent < cmpvalue && party.value(i).level > 0 && party.value(i).maxHp > 0 && party.value(i).valid)
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
	QHash <__int64, PET> pet = getPets();
	for (__int64 i = 0; i < MAX_PET; ++i)
	{
		if ((pet.value(i).level <= 0) || (pet.value(i).maxHp <= 0) || (!pet.value(i).valid))
			return false;
	}

	return true;
}

bool Server::matchBattleTarget(const QVector<battleobject_t>& btobjs, BattleMatchType matchtype, __int64 firstMatchPos, QString op, QVariant cmpvar, __int64* ppos)
{
	auto cmp = [op](QVariant a, QVariant b)
	{
		QString aStr = a.toString();
		QString bStr = b.toString();

		bool aNumOk = false, bNumOk = false;
		__int64 bNum = b.toLongLong(&aNumOk);
		__int64 aNum = a.toLongLong(&bNumOk);

		if (aNumOk && bNumOk)
		{
			if (op == ">")
				return aNum > bNum;
			else if (op == "<")
				return aNum < bNum;
			else if (op == ">=")
				return aNum >= bNum;
			else if (op == "<=")
				return aNum <= bNum;
			else if (op == "==")
				return aNum == bNum;
			else if (op == "!=")
				return aNum != bNum;
			else
				return aNum == bNum;
		}

		static const QLocale locale;
		static const QCollator collator(locale);

		if (op == ">")
			return collator.compare(aStr, bStr) > 0;
		else if (op == "<")
			return collator.compare(aStr, bStr) < 0;
		else if (op == ">=")
			return collator.compare(aStr, bStr) >= 0;
		else if (op == "<=")
			return collator.compare(aStr, bStr) <= 0;
		else if (op == "==")
			return aStr == bStr;
		else if (op == "!=")
			return aStr != bStr;
		else if (op == "!?")
			return !aStr.contains(bStr);
		else if (op == "?")
			return aStr.contains(bStr);
		else
		{
			return aStr == bStr;
		}
	};

	auto it = std::find_if(btobjs.begin(), btobjs.end(), [this, &cmp, firstMatchPos, matchtype, cmpvar, op](const battleobject_t& obj)
		{
			bool isValidEnemy = obj.hp > 0 && obj.maxHp > 0 && !checkAND(obj.status, BC_FLG_HIDE) && !checkAND(obj.status, BC_FLG_DEAD);
			bool bret = false;
			switch (matchtype)
			{
			case MatchPos:
				bret = cmp(obj.pos, cmpvar);
				break;
			case MatchModel:
				bret = cmp(obj.modelid, cmpvar);
				break;
			case MatchLevel:
				bret = cmp(obj.level, cmpvar);
				break;
			case MatchName:
				bret = cmp(obj.name, cmpvar);
				break;
			case MatchHp:
				bret = cmp(obj.hp, cmpvar);
				break;
			case MatchMaxHp:
				bret = cmp(obj.maxHp, cmpvar);
				break;
			case MatchHPP:
				bret = cmp(obj.hpPercent, cmpvar);
				break;
			case MatchStatus:
				bret = cmp(getBadStatusString(obj.status), cmpvar);
				break;
			default:
				break;
			}

			bret = isValidEnemy && bret;

			//指匹配與第一次相同的 obj避免同時匹配到不同隻導致失敗
			if (firstMatchPos != -1)
				return bret && firstMatchPos == obj.pos;
			return bret;
		});

	if (it != btobjs.end())
	{
		if (ppos != nullptr)
			*ppos = it->pos;
		return true;
	}

	return false;
}

//單/多條件匹配敵人
bool Server::conditionMatchTarget(QVector<battleobject_t> btobjs, const QString& conditionStr, __int64* ppos)
{
	__int64 target = -1;
	QStringList targetList = conditionStr.split(rexOR, Qt::SkipEmptyParts);
	if (targetList.isEmpty())
		return false;

	QVector<battleobject_t> tempbattleObjects;

	static const QHash <QString, BattleMatchType> hash = {
		{ "%(EPOS)", MatchPos },
		{ "%(ELV)", MatchLevel },
		{ "%(EHP)", MatchHp },
		{ "%(EMAXHP)", MatchMaxHp },
		{ "%(EHPP)", MatchHPP },
		{ "%(EMOD)", MatchModel },
		{ "%(ENAME)", MatchName },
		{ "%(ESTATUS)", MatchStatus },

		{ "%(APOS)", MatchPos },
		{ "%(ALV)", MatchLevel },
		{ "%(AHP)", MatchHp },
		{ "%(AMAXHP)", MatchMaxHp },
		{ "%(AHPP)", MatchHPP },
		{ "%(AMOD)", MatchModel },
		{ "%(ANAME)", MatchName },
		{ "%(ASTATUS)", MatchStatus }
	};

	auto matchCondition = [this, &btobjs](QString src, __int64 firstMatchPos, __int64* ppos)->bool
	{
		src = src.toUpper();
		BattleMatchType matchType = BattleMatchType::MatchNotUsed;
		//匹配環境變量
		for (const QString& it : hash.keys())
		{
			if (src.contains(it))
			{
				//開頭為A切換成友方數據
				if (src.startsWith("A"))
					btobjs = getBattleData().allies;

				matchType = hash.value(it);
				src.remove(it);
				break;
			}
		}

		if (matchType == BattleMatchType::MatchNotUsed)
			return false;

		//正則拆解邏輯符號和比較數值
		static const QRegularExpression rexOP(R"(\s*([<>\=\!\?]{1,2})\s*([\w\p{Han}]+)\s*)");
		QRegularExpressionMatch match = rexOP.match(src);
		if (!match.hasMatch() && match.capturedTexts().size() != 3)
			return false;

		QString op = match.captured(1);
		QString str = match.captured(2);

		bool ok = false;
		__int64 num = str.toLongLong(&ok);
		if (ok && matchType == MatchPos)
			--num; //提供給用戶使用的索引多1要扣回

		QVariant cmpVar = ok ? QVariant(num) : QVariant(str);
		bool bret = matchBattleTarget(btobjs, matchType, firstMatchPos, op, cmpVar, ppos);
		return bret;
	};

	bool bret = false;
	for (const QString& it : targetList)
	{
		//多條條件(包含&&): name && modilid && pos 或 name && pos 或 pos && modelid...
		QString newIt = it.simplified();
		newIt.replace(" ", "");
		QStringList andList = newIt.split("&&", Qt::SkipEmptyParts);
		if (andList.isEmpty())
			continue;

		//依照包含的順序排序 > %(EPOS) > %(ELV) > %(EHP) > %(EMAXHP) > %(EHPP) > %(EMOD) > %(ENAME) > %(ESTATUS) 
		std::sort(andList.begin(), andList.end(), [](const QString& a, const QString& b)
			{
				__int64 aIndex = -1, bIndex = -1;
				for (const QString& it : hash.keys())
				{
					if (a.contains(it))
					{
						aIndex = hash.value(it);
						break;
					}
				}

				for (const QString& it : hash.keys())
				{
					if (b.contains(it))
					{
						bIndex = hash.value(it);
						break;
					}
				}

				return aIndex < bIndex;
			});

		qDebug() << "prepraser params" << andList;

		__int64 passCount = 0;
		//當passCount == andList.size()時，表示所有條件都通過
		__int64 firstMatchPos = -1; //後續每個符合條件的都要是相同的pos

		for (const QString& andIt : andList)
		{
			__int64 tempTarget = -1;
			//這裡很重要必須傳入第一次匹配索引參與匹配
			if (!matchCondition(andIt, firstMatchPos, &tempTarget))
				continue;

			//紀錄第一次匹配到的索引
			if (firstMatchPos == -1)
			{
				firstMatchPos = tempTarget;
				++passCount;
				continue;
			}

			//只有索引相同才列入計算
			if (firstMatchPos == tempTarget)
			{
				++passCount;
				continue;
			}
		}

		//匹配成功次數等同條件數量才算成功
		if (passCount == andList.size() && firstMatchPos != -1)
		{
			bret = true;
			target = firstMatchPos;
			break;
		}
	}

	if (ppos != nullptr)
		*ppos = target;

	return bret;
}

//人物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
void Server::handleCharBattleLogics(const battledata_t& bt)
{
	using namespace util;
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	tempCatchPetTargetIndex = -1;
	petEscapeEnableTempFlag.store(false, std::memory_order_release); //用於在必要的時候切換戰寵動作為逃跑模式

	QStringList items;

	QVector<battleobject_t> battleObjects = bt.enemies;
	QVector<battleobject_t> tempbattleObjects;

	sortBattleUnit(battleObjects);

	__int64 target = -1;

	//檢測隊友血量
#pragma region CharBattleTools
	auto checkAllieHp = [this, &bt](__int64 cmpvalue, __int64* target, bool useequal)->bool
	{
		if (!target)
			return false;

		__int64 min = 0;
		__int64 max = (MAX_ENEMY / 2) - 1;
		if (battleCharCurrentPos.load(std::memory_order_acquire) >= (MAX_ENEMY / 2))
		{
			min = MAX_ENEMY / 2;
			max = MAX_ENEMY - 1;
		}

		QVector<battleobject_t> battleObjects = bt.objects;
		for (const battleobject_t& obj : battleObjects)
		{
			if (obj.pos < min || obj.pos > max)
				continue;

			if (obj.hp == 0)
				continue;

			if (obj.maxHp == 0)
				continue;

			if (checkAND(obj.status, BC_FLG_HIDE) || checkAND(obj.status, BC_FLG_DEAD))
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

	auto checkDeadAllie = [this, &bt](__int64* target)->bool
	{
		if (!target)
			return false;

		__int64 min = 0;
		__int64 max = (MAX_ENEMY / 2) - 1;
		if (battleCharCurrentPos.load(std::memory_order_acquire) >= (MAX_ENEMY / 2))
		{
			min = MAX_ENEMY / 2;
			max = MAX_ENEMY - 1;
		}

		QVector<battleobject_t> battleObjects = bt.objects;
		for (const battleobject_t& obj : battleObjects)
		{
			if (obj.pos < min || obj.pos > max)
				continue;

			if ((obj.maxHp > 0) && ((obj.hp == 0) || checkAND(obj.status, BC_FLG_DEAD)))
			{
				*target = obj.pos;
				return true;
			}
		}

		return false;
	};

	//檢測隊友狀態
	auto checkAllieStatus = [this, &bt](__int64* target, bool useequal)->bool
	{
		if (!target)
			return false;

		__int64 min = 0;
		__int64 max = (MAX_ENEMY / 2) - 1;
		if (battleCharCurrentPos >= (MAX_ENEMY / 2))
		{
			min = MAX_ENEMY / 2;
			max = MAX_ENEMY - 1;
		}


		QVector<battleobject_t> battleObjects = bt.objects;
		for (const battleobject_t& obj : battleObjects)
		{
			if (obj.pos < min || obj.pos > max)
				continue;

			if (obj.hp == 0)
				continue;

			if (obj.maxHp == 0)
				continue;

			if (checkAND(obj.status, BC_FLG_HIDE) || checkAND(obj.status, BC_FLG_DEAD))
				continue;

			if (!useequal && hasBadStatus(obj.status))
			{
				*target = obj.pos;
				return true;
			}
			else if (useequal && hasBadStatus(obj.status))
			{
				*target = obj.pos;
				return true;
			}
		}

		return false;
	};
#pragma endregion

	//自動換寵
#pragma region AutoSwitchPet
	do
	{
		PC pc = getPC();

		if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).modelid > 0
			|| !bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).name.isEmpty()
			|| bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).level > 0
			|| bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).maxHp > 0)
			break;

		battleobject_t obj = bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5);

		__int64 petIndex = -1;
		for (__int64 i = 0; i < MAX_PET; ++i)
		{
			if (battlePetDisableList_.value(i))
				continue;

			if (pc.battlePetNo == i)
			{
				battlePetDisableList_[i] = true;
				continue;
			}

			PET _pet = getPet(i);
			if (_pet.level <= 0 || _pet.maxHp <= 0 || _pet.hp < 1 || _pet.modelid == 0 || _pet.name.isEmpty())
			{
				battlePetDisableList_[i] = true;
				continue;
			}

			if (_pet.state == kRide || _pet.state != kStandby)
			{
				battlePetDisableList_[i] = true;
				continue;
			}

			petIndex = i;
			break;
		}

		if (petIndex == -1)
			break;

		bool autoSwitch = injector.getEnableHash(kBattleAutoSwitchEnable);
		if (!autoSwitch)
			break;

		sendBattleCharSwitchPetAct(petIndex);
		return;

	} while (false);
#pragma endregion

	//自動捉寵
#pragma region CatchPet
	do
	{
		bool autoCatch = injector.getEnableHash(kAutoCatchEnable);
		if (!autoCatch)
			break;

		if (isPetSpotEmpty())
		{
			petEscapeEnableTempFlag.store(true, std::memory_order_release);
			sendBattleCharEscapeAct();
			return;
		}

		__int64 tempTarget = -1;

		//檢查等級條件
		bool levelLimitEnable = injector.getEnableHash(kBattleCatchTargetLevelEnable);
		if (levelLimitEnable)
		{
			__int64 levelLimit = injector.getValueHash(kBattleCatchTargetLevelValue);
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
		bool maxHpLimitEnable = injector.getEnableHash(kBattleCatchTargetMaxHpEnable);
		if (maxHpLimitEnable && !battleObjects.isEmpty())
		{
			__int64 maxHpLimit = injector.getValueHash(kBattleCatchTargetMaxHpValue);
			if (matchBattleEnemyByMaxHp(maxHpLimit, battleObjects, &tempbattleObjects))
			{
				battleObjects = tempbattleObjects;
			}
			else
				battleObjects.clear();
		}

		//檢查名稱條件
		QStringList targetList = injector.getStringHash(kBattleCatchPetNameString).split(rexOR, Qt::SkipEmptyParts);
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
			__int64 catchMode = injector.getValueHash(kBattleCatchModeValue);
			if (0 == catchMode)
			{
				//遇敵逃跑
				petEscapeEnableTempFlag.store(true, std::memory_order_release);
				sendBattleCharEscapeAct();
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
		bool allowCharAction = injector.getEnableHash(kBattleCatchCharMagicEnable);
		__int64 hpLimit = injector.getValueHash(kBattleCatchTargetMagicHpValue);
		if (allowCharAction && (obj.hpPercent >= hpLimit))
		{
			__int64 actionType = injector.getValueHash(kBattleCatchCharMagicValue);
			if (actionType == 1)
			{
				sendBattleCharDefenseAct();
				return;
			}
			else if (actionType == 2)
			{
				sendBattleCharEscapeAct();
				return;
			}

			if (actionType == 0)
			{
				if (tempTarget >= 0 && tempTarget < MAX_ENEMY)
				{
					sendBattleCharAttackAct(tempTarget);
					return;
				}
			}
			else
			{
				__int64 magicIndex = actionType - 3;
				bool isProfession = magicIndex > (MAX_MAGIC - 1);
				if (isProfession) //0 ~ MAX_PROFESSION_SKILL
				{
					magicIndex -= MAX_MAGIC;
					target = -1;
					if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
					{
						if (isCharMpEnoughForSkill(magicIndex))
						{
							sendBattleCharJobSkillAct(magicIndex, target);
							return;
						}
						else
						{
							tempTarget = getBattleSelectableEnemyTarget(bt);
							sendBattleCharAttackAct(tempTarget);
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
								sendBattleCharDefenseAct();
								return;
							}
						}
						else
							break;
					}

					target = -1;
					if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
					{
						if (isCharMpEnoughForMagic(magicIndex))
						{
							sendBattleCharMagicAct(magicIndex, target);
							return;
						}
						else
							break;
					}
				}
			}
		}

		//允許人物道具降低血量
		bool allowCharItem = injector.getEnableHash(kBattleCatchCharItemEnable);
		hpLimit = injector.getValueHash(kBattleCatchTargetItemHpValue);
		if (allowCharItem && (obj.hpPercent >= hpLimit))
		{
			__int64 itemIndex = -1;
			QString text = injector.getStringHash(kBattleCatchCharItemString).simplified();
			items = text.split(rexOR, Qt::SkipEmptyParts);
			for (const QString& str : items)
			{
				itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
				if (itemIndex != -1)
					break;
			}

			target = -1;
			if (itemIndex != -1)
			{
				if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 1)))
				{
					sendBattleCharItemAct(itemIndex, target);
					return;
				}
			}
		}

		sendBattleCharCatchPetAct(tempTarget);
		return;
	} while (false);
#pragma endregion

	//鎖定逃跑
#pragma region LockEscape
	do
	{
		bool lockEscapeEnable = injector.getEnableHash(kLockEscapeEnable);
		if (!lockEscapeEnable)
			break;

		QString text = injector.getStringHash(kLockEscapeString);
		QStringList targetList = text.split(rexOR, Qt::SkipEmptyParts);
		if (targetList.isEmpty())
			break;

		for (const QString& it : targetList)
		{
			if (matchBattleEnemyByName(it, true, battleObjects, &tempbattleObjects))
			{
				sendBattleCharEscapeAct();
				petEscapeEnableTempFlag.store(true, std::memory_order_release);
				return;
			}
		}
	} while (false);
#pragma endregion

	//鎖定攻擊
#pragma region LockAttack
	do
	{
		bool lockAttackEnable = injector.getEnableHash(kLockAttackEnable);
		if (!lockAttackEnable)
			break;

		QString text = injector.getStringHash(kLockAttackString);
		if (text.isEmpty())
			break;

		if (conditionMatchTarget(battleObjects, text, &target))
		{
			//這個標誌是為了確保目標死亡之後還會繼續戰鬥，而不是打完後就逃跑
			IS_LOCKATTACK_ESCAPE_DISABLE.store(true, std::memory_order_release);
			break;
		}

		//鎖定攻擊條件不滿足時，是否不藥逃跑
		bool doNotEscape = injector.getEnableHash(kBattleNoEscapeWhileLockPetEnable);

		if (!doNotEscape && IS_LOCKATTACK_ESCAPE_DISABLE.load(std::memory_order_acquire))
			break;

		if (doNotEscape)
			break;

		sendBattleCharEscapeAct();
		petEscapeEnableTempFlag.store(true, std::memory_order_release);
		return;
	} while (false);
#pragma endregion

	//落馬逃跑
#pragma region FallDownEscape
	do
	{
		bool fallEscapeEnable = injector.getEnableHash(kFallDownEscapeEnable);
		if (!fallEscapeEnable)
			break;

		if (bt.player.rideFlag != 0)
			break;

		sendBattleCharEscapeAct();
		petEscapeEnableTempFlag.store(true, std::memory_order_release);
		return;
	} while (false);
#pragma endregion

	//道具復活
#pragma region ItemRevive
	do
	{
		bool itemRevive = injector.getEnableHash(kBattleItemReviveEnable);
		if (!itemRevive)
			break;

		__int64 tempTarget = -1;
		bool ok = false;
		quint64 targetFlags = injector.getValueHash(kBattleItemReviveTargetValue);
		if (checkAND(targetFlags, kSelectPet))
		{
			if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).hp == 0
				|| checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).status, BC_FLG_DEAD))
			{
				tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
			{
				if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).maxHp > 0 && checkDeadAllie(&tempTarget))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		QString text = injector.getStringHash(kBattleItemReviveItemString).simplified();
		if (text.isEmpty())
			break;

		items = text.split(rexOR, Qt::SkipEmptyParts);
		if (items.isEmpty())
			break;

		__int64 itemIndex = -1;
		for (const QString& str : items)
		{
			itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
			if (itemIndex != -1)
				break;
		}

		if (itemIndex == -1)
			break;

		target = -1;
		if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0 && target < MAX_ENEMY))
		{
			sendBattleCharItemAct(itemIndex, target);
			return;
		}
	} while (false);
#pragma endregion

	//精靈復活
#pragma region MagicRevive
	do
	{
		bool magicRevive = injector.getEnableHash(kBattleMagicReviveEnable);
		if (!magicRevive)
			break;

		__int64 tempTarget = -1;
		bool ok = false;
		quint64 targetFlags = injector.getValueHash(kBattleMagicReviveTargetValue);
		if (checkAND(targetFlags, kSelectPet) && bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).maxHp > 0)
		{
			if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).hp == 0
				|| checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).status, BC_FLG_DEAD))
			{
				tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
			{
				if (checkDeadAllie(&tempTarget))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		__int64 magicIndex = injector.getValueHash(kBattleMagicReviveMagicValue);
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
			if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target < MAX_ENEMY))
			{
				if (isCharMpEnoughForMagic(magicIndex))
				{
					sendBattleCharMagicAct(magicIndex, target);
					return;
				}
				else
				{
					break;
				}
			}
		}
	} while (false);
#pragma endregion

	//嗜血補氣
#pragma region SkillMp
	do
	{
		bool skillMp = injector.getEnableHash(kBattleSkillMpEnable);
		if (!skillMp)
			break;

		__int64 charMp = injector.getValueHash(kBattleSkillMpValue);
		if ((battleCharCurrentMp.load(std::memory_order_acquire) > charMp)
			&& (battleCharCurrentMp.load(std::memory_order_acquire) > 0))
			break;

		__int64 skillIndex = getProfessionSkillIndexByName("?嗜血");
		if (skillIndex < 0)
			break;

		if (isCharHpEnoughForSkill(skillIndex))
		{
			sendBattleCharJobSkillAct(skillIndex, battleCharCurrentPos.load(std::memory_order_acquire));
			return;
		}

	} while (false);
#pragma endregion

	//道具補氣
#pragma region ItemMp
	do
	{
		bool itemHealMp = injector.getEnableHash(kBattleItemHealMpEnable);
		if (!itemHealMp)
			break;

		__int64 tempTarget = -1;
		//bool ok = false;
		__int64 charMpPercent = injector.getValueHash(kBattleItemHealMpValue);
		if (!checkCharMp(charMpPercent, &tempTarget, true) && (battleCharCurrentMp.load(std::memory_order_acquire) > 0))
		{
			break;
		}

		QString text = injector.getStringHash(kBattleItemHealMpItemString).simplified();
		if (text.isEmpty())
			break;

		items = text.split(rexOR, Qt::SkipEmptyParts);
		if (items.isEmpty())
			break;

		__int64 itemIndex = -1;
		for (const QString& str : items)
		{
			itemIndex = getItemIndexByName(str, true, "", CHAR_EQUIPPLACENUM);
			if (itemIndex != -1)
				break;
		}

		if (itemIndex == -1)
			break;

		target = 0;
		if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target)
			&& (target == battleCharCurrentPos.load(std::memory_order_acquire)))
		{
			sendBattleCharItemAct(itemIndex, target);
			return;
		}
	} while (false);
#pragma endregion

	//指定回合
#pragma region SelectedRound
	do
	{
		__int64 atRoundIndex = injector.getValueHash(kBattleCharRoundActionRoundValue);
		if (atRoundIndex <= 0)
			break;

		if (atRoundIndex != battleCurrentRound.load(std::memory_order_acquire) + 1)
			break;

		__int64 tempTarget = -1;
		///bool ok = false;

		__int64 enemy = injector.getValueHash(kBattleCharRoundActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		__int64 level = injector.getValueHash(kBattleCharRoundActionLevelValue);
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

		__int64 actionType = injector.getValueHash(kBattleCharRoundActionTypeValue);
		if (actionType == 1)
		{
			sendBattleCharDefenseAct();
			return;
		}
		else if (actionType == 2)
		{
			sendBattleCharEscapeAct();
			return;
		}

		quint64 targetFlags = injector.getValueHash(kBattleCharRoundActionTargetValue);
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
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + MAX_ENEMY;
			else
				tempTarget = target + MAX_ENEMY;

		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + MAX_ENEMY;
			else
				tempTarget = target + MAX_ENEMY;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = MAX_ENEMY;
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
				sendBattleCharAttackAct(tempTarget);
				return;
			}
			else
			{
				tempTarget = getBattleSelectableEnemyTarget(bt);
				if (tempTarget == -1)
					break;

				sendBattleCharAttackAct(tempTarget);
				return;
			}
		}
		else
		{
			__int64 magicIndex = actionType - 3;
			bool isProfession = magicIndex > (MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= MAX_MAGIC;

				if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
				{
					if (isCharMpEnoughForSkill(magicIndex))
					{
						sendBattleCharJobSkillAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattleCharAttackAct(tempTarget);
					}
				}
			}
			else
			{

				if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
				{
					if (isCharMpEnoughForMagic(magicIndex))
					{
						sendBattleCharMagicAct(magicIndex, target);
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
#pragma endregion

	//間隔回合
#pragma region IntervalRound
	do
	{
		bool crossActionEnable = injector.getEnableHash(kBattleCrossActionCharEnable);
		if (!crossActionEnable)
			break;

		__int64 tempTarget = -1;

		__int64 round = injector.getValueHash(kBattleCharCrossActionRoundValue) + 1;
		if ((battleCurrentRound.load(std::memory_order_acquire) + 1) % round)
		{
			break;
		}

		__int64 actionType = injector.getValueHash(kBattleCharCrossActionTypeValue);
		if (actionType == 1)
		{
			sendBattleCharDefenseAct();
			return;
		}
		else if (actionType == 2)
		{
			sendBattleCharEscapeAct();
			return;
		}

		quint64 targetFlags = injector.getValueHash(kBattleCharCrossActionTargetValue);
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = MAX_ENEMY + 1;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + MAX_ENEMY;
			else
				tempTarget = target + MAX_ENEMY;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + MAX_ENEMY;
			else
				tempTarget = target + MAX_ENEMY;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = MAX_ENEMY;
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
				sendBattleCharAttackAct(tempTarget);
				return;
			}
			else
			{
				tempTarget = getBattleSelectableEnemyTarget(bt);
				if (tempTarget == -1)
					break;

				sendBattleCharAttackAct(tempTarget);
				return;
			}
		}
		else
		{
			__int64 magicIndex = actionType - 3;
			bool isProfession = magicIndex > (MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= MAX_MAGIC;

				if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
				{
					if (isCharMpEnoughForSkill(magicIndex))
					{
						sendBattleCharJobSkillAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattleCharAttackAct(tempTarget);
					}
				}
			}
			else
			{

				if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
				{
					if (isCharMpEnoughForMagic(magicIndex))
					{
						sendBattleCharMagicAct(magicIndex, target);
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
#pragma endregion

	//精靈淨化
#pragma region MagicPurify
	do
	{
		bool charPurg = injector.getEnableHash(kBattleCharPurgEnable);
		if (!charPurg)
			break;

		__int64 tempTarget = -1;
		bool ok = false;
		quint64 targetFlags = injector.getValueHash(kBattleCharPurgTargetValue);

		if (checkAND(targetFlags, kSelectSelf))
		{
			if (hasBadStatus(bt.objects.value(battleCharCurrentPos).status))
			{
				ok = true;
			}
		}

		if (checkAND(targetFlags, kSelectPet))
		{
			if (!ok && bt.objects.value(battleCharCurrentPos + 5).maxHp > 0)
			{
				if (hasBadStatus(bt.objects.value(battleCharCurrentPos + 5).status) && bt.objects.value(battleCharCurrentPos + 5).hp > 0 &&
					!checkAND(bt.objects.value(battleCharCurrentPos + 5).status, BC_FLG_DEAD) && !checkAND(bt.objects.value(battleCharCurrentPos + 5).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos + 5;
					ok = true;
				}
			}
			else if (!ok && bt.objects.value(battleCharCurrentPos).maxHp > 0)
			{
				if (hasBadStatus(bt.objects.value(battleCharCurrentPos).status) && bt.objects.value(battleCharCurrentPos).rideHp > 0 &&
					!checkAND(bt.objects.value(battleCharCurrentPos).status, BC_FLG_DEAD) && !checkAND(bt.objects.value(battleCharCurrentPos).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos;
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
			{
				if (checkAllieStatus(&tempTarget, false))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		__int64 magicIndex = injector.getValueHash(kBattleCharPurgActionTypeValue);
		if (magicIndex < 0 || magicIndex > MAX_MAGIC)
			break;

		bool isProfession = magicIndex > (MAX_MAGIC - 1);
		if (!isProfession) // ifMagic
		{
			target = -1;
			if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 1)))
			{
				if (isCharMpEnoughForMagic(magicIndex))
				{
					sendBattleCharMagicAct(magicIndex, target);
					return;
				}
				else
				{
					break;
				}
			}
		}
	} while (false);
#pragma endregion

	//精靈補血
#pragma region MagicHeal
	do
	{
		bool magicHeal = injector.getEnableHash(kBattleMagicHealEnable);
		if (!magicHeal)
			break;

		__int64 tempTarget = -1;
		bool ok = false;
		quint64 targetFlags = injector.getValueHash(kBattleMagicHealTargetValue);
		__int64 charPercent = injector.getValueHash(kBattleMagicHealCharValue);
		__int64 petPercent = injector.getValueHash(kBattleMagicHealPetValue);
		__int64 alliePercent = injector.getValueHash(kBattleMagicHealAllieValue);

		if (checkAND(targetFlags, kSelectSelf))
		{
			if (checkCharHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (checkAND(targetFlags, kSelectPet))
		{
			if (!ok && bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).hpPercent <= petPercent
					&& bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).hp > 0
					&& !checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).status, BC_FLG_DEAD)
					&& !checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
					ok = true;
				}
			}
			else if (!ok && bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire)).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire)).rideHpPercent <= petPercent
					&& bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire)).rideHp > 0
					&& !checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire)).status, BC_FLG_DEAD)
					&& !checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire)).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
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

		__int64 magicIndex = injector.getValueHash(kBattleMagicHealMagicValue);
		if (magicIndex < 0 || magicIndex > MAX_MAGIC)
			break;

		bool isProfession = magicIndex > (MAX_MAGIC - 1);
		if (!isProfession) // ifMagic
		{
			target = -1;
			if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 1)))
			{
				if (isCharMpEnoughForMagic(magicIndex))
				{
					sendBattleCharMagicAct(magicIndex, target);
					return;
				}
				else
				{
					break;
				}
			}
		}
	} while (false);
#pragma endregion

	//道具補血
#pragma region ItemHeal
	do
	{
		bool itemHeal = injector.getEnableHash(kBattleItemHealEnable);
		if (!itemHeal)
			break;

		__int64 tempTarget = -1;
		bool ok = false;

		quint64 targetFlags = injector.getValueHash(kBattleItemHealTargetValue);
		__int64 charPercent = injector.getValueHash(kBattleItemHealCharValue);
		__int64 petPercent = injector.getValueHash(kBattleItemHealPetValue);
		__int64 alliePercent = injector.getValueHash(kBattleItemHealAllieValue);
		if (checkAND(targetFlags, kSelectSelf))
		{
			if (checkCharHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectPet) && bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).hpPercent <= petPercent
					&& bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).hp > 0
					&& !checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).status, BC_FLG_DEAD)
					&& !checkAND(bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
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

		__int64 itemIndex = -1;
		bool meatProiory = injector.getEnableHash(kBattleItemHealMeatPriorityEnable);
		if (meatProiory)
		{
			itemIndex = getItemIndexByName("?肉", false, "耐久力", CHAR_EQUIPPLACENUM);
		}

		if (itemIndex == -1)
		{
			QString text = injector.getStringHash(kBattleItemHealItemString).simplified();
			if (text.isEmpty())
				break;

			items = text.split(rexOR, Qt::SkipEmptyParts);
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
		if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 1)))
		{
			sendBattleCharItemAct(itemIndex, target);
			return;
		}
	} while (false);
#pragma endregion

	//一般動作
#pragma region NormalAction
	do
	{
		__int64 tempTarget = -1;
		//bool ok = false;

		__int64 enemy = injector.getValueHash(kBattleCharNormalActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		__int64 level = injector.getValueHash(kBattleCharNormalActionLevelValue);
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

		__int64 actionType = injector.getValueHash(kBattleCharNormalActionTypeValue);
		if (actionType == 1)
		{
			sendBattleCharDefenseAct();
			return;
		}
		else if (actionType == 2)
		{
			sendBattleCharEscapeAct();
			return;
		}

		quint64 targetFlags = injector.getValueHash(kBattleCharNormalActionTargetValue);
		if (checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = MAX_ENEMY + 1;
		}
		else if (checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + MAX_ENEMY;
			else
				tempTarget = target + MAX_ENEMY;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + MAX_ENEMY;
			else
				tempTarget = target + MAX_ENEMY;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		}
		else if (checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = MAX_ENEMY;
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
				sendBattleCharAttackAct(tempTarget);
				return;
			}
			else
			{
				tempTarget = getBattleSelectableEnemyTarget(bt);
				if (tempTarget == -1)
					break;

				sendBattleCharAttackAct(tempTarget);
				return;
			}
		}
		else
		{
			__int64 magicIndex = actionType - 3;
			bool isProfession = magicIndex > (MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= MAX_MAGIC;

				if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
				{
					if (isCharMpEnoughForSkill(magicIndex))
					{
						sendBattleCharJobSkillAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattleCharAttackAct(tempTarget);
					}
				}
			}
			else
			{

				if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
				{
					if (isCharMpEnoughForMagic(magicIndex))
					{
						sendBattleCharMagicAct(magicIndex, target);
						return;
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						sendBattleCharAttackAct(tempTarget);
					}
				}
			}
		}
	} while (false);
#pragma endregion

	sendBattleCharDoNothing();
}

//寵物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
void Server::handlePetBattleLogics(const battledata_t& bt)
{
	using namespace util;
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	QVector<battleobject_t> battleObjects = bt.enemies;
	QVector<battleobject_t> tempbattleObjects;

	sortBattleUnit(battleObjects);
	__int64 target = -1;

#pragma region PetBattleTools
	//檢測隊友血量
	auto checkAllieHp = [this, &bt](__int64 cmpvalue, __int64* target, bool useequal)->bool
	{
		if (!target)
			return false;

		__int64 min = 0;
		__int64 max = (MAX_ENEMY / 2) - 1;
		if (battleCharCurrentPos >= (MAX_ENEMY / 2))
		{
			min = MAX_ENEMY / 2;
			max = MAX_ENEMY - 1;
		}

		QVector<battleobject_t> battleObjects = bt.objects;
		for (const battleobject_t& obj : battleObjects)
		{
			if (obj.pos < min || obj.pos > max)
				continue;

			if (obj.hp == 0)
				continue;

			if (obj.maxHp == 0)
				continue;

			if (checkAND(obj.status, BC_FLG_HIDE) || checkAND(obj.status, BC_FLG_DEAD))
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

	//檢測隊友狀態
	auto checkAllieStatus = [this, &bt](__int64* target, bool useequal)->bool
	{
		if (!target)
			return false;

		__int64 min = 0;
		__int64 max = (MAX_ENEMY / 2) - 1;
		if (battleCharCurrentPos >= (MAX_ENEMY / 2))
		{
			min = MAX_ENEMY / 2;
			max = MAX_ENEMY - 1;
		}

		QVector<battleobject_t> battleObjects = bt.objects;
		for (const battleobject_t& obj : battleObjects)
		{
			if (obj.pos < min || obj.pos > max)
				continue;

			if (obj.hp == 0)
				continue;

			if (obj.maxHp == 0)
				continue;

			if (checkAND(obj.status, BC_FLG_HIDE) || checkAND(obj.status, BC_FLG_DEAD))
				continue;
			if (!useequal && hasBadStatus(obj.status))
			{
				*target = obj.pos;
				return true;
			}
			else if (useequal && hasBadStatus(obj.status))
			{
				*target = obj.pos;
				return true;
			}
		}

		return false;
	};
#pragma endregion

	//自動捉寵
#pragma region CatchPet
	do
	{
		bool autoCatch = injector.getEnableHash(kAutoCatchEnable);
		if (!autoCatch)
			break;

		//允許寵物動作
		bool allowPetAction = injector.getEnableHash(kBattleCatchPetSkillEnable);
		if (!allowPetAction)
		{
			sendBattlePetDoNothing(); //避免有人會忘記改成防禦，默認只要打開捉寵且沒設置動作就什麼都不做
			return;
		}

		__int64 actionType = injector.getValueHash(kBattleCatchPetSkillValue);

		__int64 skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		__int64 tempTarget = tempCatchPetTargetIndex;
		if ((tempTarget != -1) && fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 2)))
		{
			sendBattlePetSkillAct(skillIndex, target);
			return;
		}
		sendBattlePetDoNothing(); //避免有人會忘記改成防禦，默認只要打開捉寵且動作失敗就什麼都不做
		return;
	} while (false);
#pragma endregion

	//鎖定攻擊
#pragma region LockAttack
	do
	{
		bool lockAttackEnable = injector.getEnableHash(kLockAttackEnable);
		if (!lockAttackEnable)
			break;

		QString text = injector.getStringHash(kLockAttackString);
		QStringList targetList = text.split(rexOR, Qt::SkipEmptyParts);
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
#pragma endregion

	//指定回合
#pragma region SelectedRound
	do
	{
		__int64 atRoundIndex = injector.getValueHash(kBattlePetRoundActionEnemyValue);
		if (atRoundIndex <= 0)
			break;

		__int64 tempTarget = -1;
		//bool ok = false;

		__int64 enemy = injector.getValueHash(kBattlePetRoundActionLevelValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		__int64 level = injector.getValueHash(kBattleCharRoundActionLevelValue);
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

		quint64 targetFlags = injector.getValueHash(kBattlePetRoundActionTargetValue);
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
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true) + MAX_ENEMY;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false) + MAX_ENEMY;
			else
				tempTarget = target;
		}
		else if (checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
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

		__int64 actionType = injector.getValueHash(kBattlePetRoundActionTypeValue);

		__int64 skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > MAX_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0 && target <= 22))
		{
			sendBattlePetSkillAct(skillIndex, target);
			return;
		}
	} while (false);
#pragma endregion

	//間隔回合
#pragma region CrossRound
	do
	{
		bool crossActionEnable = injector.getEnableHash(kBattleCrossActionPetEnable);
		if (!crossActionEnable)
			break;

		__int64 tempTarget = -1;

		__int64 round = injector.getValueHash(kBattlePetCrossActionRoundValue) + 1;
		if ((battleCurrentRound.load(std::memory_order_acquire) + 1) % round)
		{
			break;
		}

		quint64 targetFlags = injector.getValueHash(kBattlePetCrossActionTargetValue);
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
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
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

		__int64 actionType = injector.getValueHash(kBattlePetCrossActionTypeValue);

		__int64 skillIndex = actionType;
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
#pragma endregion

	//寵物淨化
#pragma region PetSkillPurg
	do
	{
		bool petPurg = injector.getEnableHash(kBattlePetPurgEnable);
		if (!petPurg)
			break;

		__int64 tempTarget = -1;
		bool ok = false;
		quint64 targetFlags = injector.getValueHash(kBattlePetPurgTargetValue);

		if (checkAND(targetFlags, kSelectSelf))
		{
			if (hasBadStatus(bt.objects.value(battleCharCurrentPos).status))
			{
				ok = true;
			}
		}

		if (checkAND(targetFlags, kSelectPet))
		{
			if (!ok && bt.objects.value(battleCharCurrentPos + 5).maxHp > 0)
			{
				if (hasBadStatus(bt.objects.value(battleCharCurrentPos + 5).status) && bt.objects.value(battleCharCurrentPos + 5).hp > 0 &&
					!checkAND(bt.objects.value(battleCharCurrentPos + 5).status, BC_FLG_DEAD) && !checkAND(bt.objects.value(battleCharCurrentPos + 5).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos + 5;
					ok = true;
				}
			}
			else if (!ok && bt.objects.value(battleCharCurrentPos).maxHp > 0)
			{
				if (hasBadStatus(bt.objects.value(battleCharCurrentPos).status) && bt.objects.value(battleCharCurrentPos).rideHp > 0 &&
					!checkAND(bt.objects.value(battleCharCurrentPos).status, BC_FLG_DEAD) && !checkAND(bt.objects.value(battleCharCurrentPos).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos;
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if (checkAND(targetFlags, kSelectAllieAny) || checkAND(targetFlags, kSelectAllieAll))
			{
				if (checkAllieStatus(&tempTarget, false))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		__int64 petActionIndex = injector.getValueHash(kBattlePetPurgActionTypeValue);
		if (petActionIndex < 0 || petActionIndex > MAX_PETSKILL)
			break;

		bool isProfession = petActionIndex > (MAX_PETSKILL - 1);
		if (!isProfession) // ifpetAction
		{
			target = -1;
			if (fixPetTargetBySkillIndex(petActionIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 1)))
			{
				sendBattlePetSkillAct(petActionIndex, target);
				return;

			}
		}
	} while (false);
#pragma endregion

	//寵物補血
#pragma region PetSkillHeal
	do
	{
		bool petHeal = injector.getEnableHash(kBattlePetHealEnable);
		if (!petHeal)
			break;

		__int64 tempTarget = -1;
		bool ok = false;
		quint64 targetFlags = injector.getValueHash(kBattlePetHealTargetValue);
		__int64 charPercent = injector.getValueHash(kBattlePetHealCharValue);
		__int64 petPercent = injector.getValueHash(kBattlePetHealPetValue);
		__int64 alliePercent = injector.getValueHash(kBattlePetHealAllieValue);

		if (checkAND(targetFlags, kSelectSelf))
		{
			if (checkCharHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (checkAND(targetFlags, kSelectPet))
		{
			if (!ok && bt.objects.value(battleCharCurrentPos + 5).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos + 5).hpPercent <= petPercent && bt.objects.value(battleCharCurrentPos + 5).hp > 0 &&
					!checkAND(bt.objects.value(battleCharCurrentPos + 5).status, BC_FLG_DEAD) && !checkAND(bt.objects.value(battleCharCurrentPos + 5).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos + 5;
					ok = true;
				}
			}
			else if (!ok && bt.objects.value(battleCharCurrentPos).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos).rideHpPercent <= petPercent && bt.objects.value(battleCharCurrentPos).rideHp > 0 &&
					!checkAND(bt.objects.value(battleCharCurrentPos).status, BC_FLG_DEAD) && !checkAND(bt.objects.value(battleCharCurrentPos).status, BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos;
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

		__int64 petActionIndex = injector.getValueHash(kBattlePetHealActionTypeValue);
		if (petActionIndex < 0 || petActionIndex > MAX_PETSKILL)
			break;

		bool isProfession = petActionIndex > (MAX_PETSKILL - 1);
		if (!isProfession) // ifpetAction
		{
			target = -1;
			if (fixPetTargetBySkillIndex(petActionIndex, tempTarget, &target) && (target >= 0 && target <= (MAX_ENEMY + 1)))
			{
				sendBattlePetSkillAct(petActionIndex, target);
				return;

			}
		}
	} while (false);
#pragma endregion

	//一般動作
#pragma region NormalAction
	do
	{
		__int64 tempTarget = -1;
		//bool ok = false;

		__int64 enemy = injector.getValueHash(kBattlePetNormalActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		__int64 level = injector.getValueHash(kBattlePetNormalActionLevelValue);
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

		quint64 targetFlags = injector.getValueHash(kBattlePetNormalActionTargetValue);
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
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire);
		}
		else if (checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
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

		__int64 actionType = injector.getValueHash(kBattlePetNormalActionTypeValue);

		__int64 skillIndex = actionType;
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
#pragma endregion

	sendBattlePetDoNothing();
}

//精靈名稱匹配精靈索引
__int64 Server::getMagicIndexByName(const QString& name, bool isExact) const
{
	if (name.isEmpty())
		return -1;

	QString newName = name.simplified();
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
		isExact = false;
	}

	for (__int64 i = 0; i < MAX_MAGIC; ++i)
	{
		MAGIC magic = getMagic(i);
		if (!magic.valid)
			continue;

		if (magic.name.isEmpty())
			continue;

		if (isExact && (magic.name == newName))
			return i;
		else if (!isExact && magic.name.contains(newName))
			return i;
	}
	return -1;
}

__int64 Server::getSkillIndexByName(const QString& name) const
{
	if (name.isEmpty())
		return -1;

	QString newName = name.simplified();
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
	}

	QHash <__int64, PROFESSION_SKILL> profession_skill = getSkills();
	for (__int64 i = 0; i < MAX_PROFESSION_SKILL; ++i)
	{
		if (!profession_skill.value(i).valid)
			continue;

		if (profession_skill.value(i).name.isEmpty())
			continue;

		if (profession_skill.value(i).name == newName)
			return i;
	}
	return -1;
}

//根據target判斷文字
QString Server::getAreaString(__int64 target)
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
__int64 Server::getGetPetSkillIndexByName(__int64 petIndex, const QString& name) const
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return -1;

	if (name.isEmpty())
		return -1;

	__int64 petSkillIndex = -1;

	QHash <__int64, PET_SKILL> petSkill = getPetSkills(petIndex);
	for (__int64 i = 0; i < MAX_SKILL; ++i)
	{
		if (!petSkill.value(i).valid)
			continue;

		const QString petSkillName = petSkill.value(i).name;
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
bool Server::isCharMpEnoughForMagic(__int64 magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return false;

	if (getMagic(magicIndex).costmp > getPC().mp)
		return false;

	return true;
}

//戰鬥檢查MP是否足夠施放技能
bool Server::isCharMpEnoughForSkill(__int64  magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= MAX_PROFESSION_SKILL)
		return false;

	if (getSkill(magicIndex).costmp > getPC().mp)
		return false;

	return true;
}

//戰鬥檢查HP是否足夠施放技能
bool Server::isCharHpEnoughForSkill(__int64 magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= MAX_PROFESSION_SKILL)
		return false;

	if (MIN_HP_PERCENT > getPC().hpPercent)
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

	constexpr __int64 maxorder = 20;
	constexpr __int64 order[maxorder] = { 19, 17, 15, 16, 18, 14, 12, 10, 11, 13, 8, 6, 5, 7, 9, 3, 1, 0, 2, 4 };

	for (const __int64 it : order)
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
__int64 Server::getBattleSelectableEnemyTarget(const battledata_t& bt) const
{
	__int64 defaultTarget = MAX_ENEMY - 1;
	if (battleCharCurrentPos.load(std::memory_order_acquire) >= (MAX_ENEMY / 2))
		defaultTarget = MAX_ENEMY / 4;

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
__int64 Server::getBattleSelectableEnemyOneRowTarget(const battledata_t& bt, bool front) const
{
	__int64 defaultTarget = MAX_ENEMY - 5;
	if (battleCharCurrentPos.load(std::memory_order_acquire) >= (MAX_ENEMY / 2))
		defaultTarget = MAX_ENEMY / 4;

	QVector<battleobject_t> enemies(bt.enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	sortBattleUnit(enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	__int64 targetIndex = -1;

	if (front)
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < (MAX_ENEMY / 2))
		{
			// 只取 pos 在 15~19 之间的，取最前面的
			for (__int64 i = 0; i < enemies.size(); ++i)
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
			for (__int64 i = 0; i < enemies.size(); ++i)
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
		if (battleCharCurrentPos.load(std::memory_order_acquire) < (MAX_ENEMY / 2))
		{
			// 只取 pos 在 10~14 之间的，取最前面的
			for (__int64 i = 0; i < enemies.size(); ++i)
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
			for (__int64 i = 0; i < enemies.size(); ++i)
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
__int64 Server::getBattleSelectableAllieTarget(const battledata_t& bt) const
{
	__int64 defaultTarget = 5;
	if (battleCharCurrentPos.load(std::memory_order_acquire) >= 10)
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
		if (enemy.hp == 0)
			continue;

		if (enemy.maxHp == 0)
			continue;

		if (enemy.modelid == 0)
			continue;

		if (checkAND(enemy.status, BC_FLG_HIDE))
			continue;

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
bool Server::matchBattleEnemyByLevel(__int64 level, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const
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
bool Server::matchBattleEnemyByMaxHp(__int64 maxHp, const QVector<battleobject_t>& src, QVector<battleobject_t>* v) const
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
bool Server::fixCharTargetByMagicIndex(__int64 magicIndex, __int64 oldtarget, __int64* target) const
{
	if (!target)
		return false;

	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return false;

	__int64 magicType = getMagic(magicIndex).target;

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
			if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ALLMYSIDE://我方全體
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case MAGIC_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
		if (oldtarget == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{

		if (oldtarget == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.load(std::memory_order_acquire) + 5))
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
				if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
					oldtarget = 20;
				else
					oldtarget = 21;
			}
			else
			{
				if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
			if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ONE_ROW:				// 針對敵方某一列
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
		{
			if (oldtarget < 5)
				oldtarget = MAX_ENEMY + 5;
			else if (oldtarget < 10)
				oldtarget = MAX_ENEMY + 6;
			else if (oldtarget < 15)
				oldtarget = MAX_ENEMY + 3;
			else
				oldtarget = MAX_ENEMY + 4;
		}
		else
		{
			if (oldtarget < 5)
				oldtarget = MAX_ENEMY + 5 - 10;
			else if (oldtarget < 10)
				oldtarget = MAX_ENEMY + 6 - 10;
			else if (oldtarget < 15)
				oldtarget = MAX_ENEMY + 3 - 10;
			else
				oldtarget = MAX_ENEMY + 4 - 10;
		}
		break;
	}
	case MAGIC_TARGET_ALL_ROWS:				// 針對敵方所有人
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
bool Server::fixCharTargetBySkillIndex(__int64 magicIndex, __int64 oldtarget, __int64* target) const
{
	if (!target)
		return false;

	if (magicIndex < 0 || magicIndex >= MAX_PROFESSION_SKILL)
		return false;

	__int64 magicType = getSkill(magicIndex).target;

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
			if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ALLMYSIDE://我方全體
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case MAGIC_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
		if (oldtarget == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{
		if (oldtarget == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.load(std::memory_order_acquire) + 5))
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
				if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
					oldtarget = 20;
				else
					oldtarget = 21;
			}
			else
			{
				if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
			if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case MAGIC_TARGET_ONE_ROW:				// 針對敵方某一列
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
		{
			if (oldtarget < 5)
				oldtarget = MAX_ENEMY + 5;
			else if (oldtarget < 10)
				oldtarget = MAX_ENEMY + 6;
			else if (oldtarget < 15)
				oldtarget = MAX_ENEMY + 3;
			else
				oldtarget = MAX_ENEMY + 4;
		}
		else
		{
			if (oldtarget < 5)
				oldtarget = MAX_ENEMY + 5 - 10;
			else if (oldtarget < 10)
				oldtarget = MAX_ENEMY + 6 - 10;
			else if (oldtarget < 15)
				oldtarget = MAX_ENEMY + 3 - 10;
			else
				oldtarget = MAX_ENEMY + 4 - 10;
		}
		break;
	}
	case MAGIC_TARGET_ALL_ROWS:				// 針對敵方所有人
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
bool Server::fixCharTargetByItemIndex(__int64 itemIndex, __int64 oldtarget, __int64* target) const
{
	if (!target)
		return false;

	if (itemIndex < CHAR_EQUIPPLACENUM || itemIndex >= MAX_ITEM)
		return false;

	__int64 itemType = getItem(itemIndex).target;

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
			if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case ITEM_TARGET_ALLMYSIDE://我方全體
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case ITEM_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
		if (oldtarget == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			oldtarget = -1;
		}
		break;
	}
	case ITEM_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{
		if (oldtarget == battleCharCurrentPos.load(std::memory_order_acquire))
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.load(std::memory_order_acquire) + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case ITEM_TARGET_PET://戰寵
	{
		oldtarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		break;
	}

	default:
		break;
	}

	*target = oldtarget;
	return true;
}

//戰鬥修正寵物技能目標範圍
bool Server::fixPetTargetBySkillIndex(__int64 skillIndex, __int64 oldtarget, __int64* target) const
{
	if (!target)
		return false;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return false;

	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return false;

	__int64 skillType = getPetSkill(pc.battlePetNo, skillIndex).target;

	switch (skillType)
	{
	case PETSKILL_TARGET_MYSELF:
	{
		if (oldtarget != (battleCharCurrentPos.load(std::memory_order_acquire) + 5))
			oldtarget = battleCharCurrentPos.load(std::memory_order_acquire) + 5;
		break;
	}
	case PETSKILL_TARGET_OTHER:
	{
		break;
	}
	case PETSKILL_TARGET_ALLMYSIDE:
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case PETSKILL_TARGET_ALLOTHERSIDE:
	{
		if (battleCharCurrentPos.load(std::memory_order_acquire) < 10)
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
		if (oldtarget == (battleCharCurrentPos.load(std::memory_order_acquire) + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case PETSKILL_TARGET_WITHOUTMYSELFANDPET:
	{
		__int64 max = MAX_ENEMY;
		__int64 min = 0;
		if (battleCharCurrentPos.load(std::memory_order_acquire) >= 10)
		{
			max = 19;
			min = 10;
		}

		if (oldtarget < min || oldtarget > max)
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.load(std::memory_order_acquire) + 5))
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
void Server::sendBattleCharAttackAct(__int64 target)
{
	if (target < 0 || target >= MAX_ENEMY)
		return;

	const QString qcmd = QString("H|%1").arg(toQString(target, 16));
	lssproto_B_send(qcmd);


	battledata_t bt = getBattleData();
	battleobject_t obj = bt.objects.value(target, battleobject_t{});

	const QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
	const QString text(QObject::tr("use attack [%1]%2").arg(target + 1).arg(name));
	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#FF5050"));
	}
}

//戰鬥人物使用精靈
void Server::sendBattleCharMagicAct(__int64 magicIndex, __int64  target)
{
	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return;



	const QString qcmd = QString("J|%1|%2").arg(toQString(magicIndex, 16)).arg(toQString(target, 16));
	lssproto_B_send(qcmd);

	MAGIC magic = getMagic(magicIndex);
	const QString magicName = magic.name;

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

	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);

		if (magic.memo.contains("力回"))
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#49BF45"));
		else
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#FF5050"));
	}
}

//戰鬥人物使用職業技能
void Server::sendBattleCharJobSkillAct(__int64 skillIndex, __int64 target)
{
	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (skillIndex < 0 || skillIndex >= MAX_PROFESSION_SKILL)
		return;

	const QString qcmd = QString("P|%1|%2").arg(toQString(skillIndex, 16)).arg(toQString(target, 16));
	lssproto_B_send(qcmd);

	PROFESSION_SKILL skill = getSkill(skillIndex);
	const QString skillName = skill.name.simplified();
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

	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
		if (skill.memo.contains("力回"))
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#49BF45"));
		else
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#FF5050"));
	}
}

//戰鬥人物使用道具
void Server::sendBattleCharItemAct(__int64 itemIndex, __int64 target)
{
	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	const QString qcmd = QString("I|%1|%2").arg(toQString(itemIndex, 16)).arg(toQString(target, 16));
	lssproto_B_send(qcmd);

	ITEM item = getItem(itemIndex);
	const QString itemName = item.name.simplified();

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

	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
		if (item.memo.contains("力回"))
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#49BF45"));
		else
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#FF5050"));
	}
}

//戰鬥人物防禦
void Server::sendBattleCharDefenseAct()
{
	const QString qcmd("G");
	lssproto_B_send(qcmd);

	const QString text = QObject::tr("defense");
	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos, QColor("#CCB157"));
	}
}

//戰鬥人物逃跑
void Server::sendBattleCharEscapeAct()
{
	const QString qcmd("E");
	lssproto_B_send(qcmd);

	const QString text(QObject::tr("escape"));
	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
	}
}

//戰鬥人物捉寵
void Server::sendBattleCharCatchPetAct(__int64 target)
{
	if (target < 0 || target >= MAX_ENEMY)
		return;

	const QString qcmd = QString("T|%1").arg(toQString(target, 16));
	lssproto_B_send(qcmd);

	battledata_t bt = getBattleData();
	battleobject_t obj = bt.objects.value(target, battleobject_t{});
	QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
	const QString text(QObject::tr("catch [%1]%2").arg(target).arg(name));

	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#49BF45"));
	}
}

//戰鬥人物切換戰寵
void Server::sendBattleCharSwitchPetAct(__int64 petIndex)
{
	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	PET pet = getPet(petIndex);

	if (!pet.valid)
		return;

	if (pet.hp <= 0)
		return;

	const QString qcmd = QString("S|%1").arg(toQString(petIndex, 16));
	lssproto_B_send(qcmd);

	QString text(QObject::tr("switch pet to %1") \
		.arg(!pet.freeName.isEmpty() ? pet.freeName : pet.name));

	if (labelCharAction != text)
	{
		labelCharAction = text;

		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelCharAction(text);
	}
}

//戰鬥人物什麼都不做
void Server::sendBattleCharDoNothing()
{
	const QString qcmd("N");
	lssproto_B_send(qcmd);

	QString text(QObject::tr("do nothing"));
	if (labelCharAction != text)
	{
		labelCharAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos, QColor("#696969"));
	}
}

//戰鬥戰寵技能
void Server::sendBattlePetSkillAct(__int64 skillIndex, __int64 target)
{
	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return;

	if (target < 0 || (target > (MAX_ENEMY + 2)))
		return;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return;

	const QString qcmd = QString("W|%1|%2").arg(toQString(skillIndex, 16)).arg(toQString(target, 16));
	lssproto_B_send(qcmd);

	QString text("");
	PET_SKILL petSkill = getPetSkill(pc.battlePetNo, skillIndex);
	if (target < MAX_ENEMY)
	{
		QString name;
		if (petSkill.name != "防禦" && petSkill.name != "防御")
		{
			battledata_t bt = getBattleData();
			battleobject_t obj = bt.objects.value(target, battleobject_t{});
			name = !obj.name.isEmpty() ? obj.name : obj.freeName;
		}
		else
			name = QObject::tr("self");

		text = QObject::tr("use %1 to [%2]%3").arg(petSkill.name).arg(target + 1).arg(name);
	}
	else
	{
		text = QObject::tr("use %1 to %2").arg(petSkill.name).arg(getAreaString(target));
	}

	if (labelPetAction != text)
	{
		labelPetAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelPetAction(text);
		if (petSkill.name.contains("防"))
			emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos, QColor("#CCB157"));
		else if (petSkill.memo.contains("力回"))
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#49BF45"));
		else
			emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#FF5050"));
	}
}

//戰鬥戰寵什麼都不做
void Server::sendBattlePetDoNothing()
{
	PC pc = getPC();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return;

	const QString qcmd("W|FF|FF");

	lssproto_B_send(qcmd);

	QString text(QObject::tr("do nothing"));
	if (labelPetAction != text)
	{
		labelPetAction = text;
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateLabelPetAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos + 5, QColor("#696969"));
	}
}

#pragma endregion

#pragma region Lssproto_Recv
//人物刪除
void Server::lssproto_CharDelete_recv(char* cresult, char* cdata)
{
	QString data = toQUnicode(cdata);
	QString result = toQUnicode(cresult);
	if (data.isEmpty() && result.isEmpty())
		return;

	if (result == SUCCESSFULSTR || data == SUCCESSFULSTR)
	{

	}
}

//組隊變化
void Server::lssproto_PR_recv(int request, int result)
{
	QStringList teamInfoList;

	if (request == 1 && result == 1)
	{
		//pc.status |= CHR_STATUS_PARTY;
		QReadLocker locker(&teamInfoLock_);
		QHash<__int64, PARTY> party = party_.toHash();
		for (__int64 i = 0; i < MAX_PARTY; ++i)
		{
			if (party.value(i).name.isEmpty() || (!party.value(i).valid) || (party.value(i).maxHp <= 0))
			{
				teamInfoList.append("");
				continue;
			}
			QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(party.value(i).name).arg(party.value(i).level)
				.arg(party.value(i).hp).arg(party.value(i).maxHp).arg(party.value(i).hpPercent);
			teamInfoList.append(text);
		}
	}
	else
	{
		if (request == 0 && result == 1)
		{
			QWriteLocker locker(&teamInfoLock_);
			__int64 i;
			QHash<__int64, PARTY> party = party_.toHash();
			for (i = 0; i < MAX_PARTY; ++i)
			{
				party.remove(i);
				teamInfoList.append("");
			}
			party_ = party;

			//pc.status &= (~CHR_STATUS_LEADER);
		}
	}

	__int64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateTeamInfo(teamInfoList);
}

//地圖轉移
void Server::lssproto_EV_recv(int dialogid, int result)
{
	std::ignore = getFloorName();
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	__int64 floor = getFloor();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateNpcList(floor);
}

//開關切換
void Server::lssproto_FS_recv(int flg)
{
	{
		QWriteLocker locker(&charInfoLock_);
		pc_.etcFlag = flg;
	}

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	injector.setEnableHash(kSwitcherTeamEnable, checkAND(flg, PC_ETCFLAG_GROUP));//組隊開關
	injector.setEnableHash(kSwitcherPKEnable, checkAND(flg, PC_ETCFLAG_PK));//決鬥開關
	injector.setEnableHash(kSwitcherCardEnable, checkAND(flg, PC_ETCFLAG_CARD));//名片開關
	injector.setEnableHash(kSwitcherTradeEnable, checkAND(flg, PC_ETCFLAG_TRADE));//交易開關
	injector.setEnableHash(kSwitcherWorldEnable, checkAND(flg, PC_ETCFLAG_WORLD));//世界頻道開關
	injector.setEnableHash(kSwitcherGroupEnable, checkAND(flg, PC_ETCFLAG_PARTY_CHAT));//組隊頻道開關
	injector.setEnableHash(kSwitcherFamilyEnable, checkAND(flg, PC_ETCFLAG_FM));//家族頻道開關
	injector.setEnableHash(kSwitcherJobEnable, checkAND(flg, PC_ETCFLAG_JOB));//職業頻道開關

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.applyHashSettingsToUI();
}

void Server::lssproto_AB_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	__int64 i;
	__int64 no;
	QString name;
	__int64 flag;
	bool valid;
#ifdef _MAILSHOWPLANET				// (可開放) 顯示名片星球
	QString planetid;
	__int64 j;
#endif

	for (i = 0; i < MAX_ADDRESS_BOOK; ++i)
	{
		//no = i * 6; //the second
		no = i * 8;
		valid = getIntegerToken(data, "|", no + 1) > 0;
		ADDRESS_BOOK addressBook = getAddressBook(i);
		if (!valid)
		{
			addressBook_.remove(i);
			continue;
		}

		addressBook.valid = true;

		flag = getStringToken(data, "|", no + 2, name);

		if (flag == 1)
			break;

		makeStringFromEscaped(name);
		addressBook.name = name;
		addressBook.level = getIntegerToken(data, "|", no + 3);
		addressBook.dp = getIntegerToken(data, "|", no + 4);
		addressBook.onlineFlag = getIntegerToken(data, "|", no + 5) > 0;
		addressBook.modelid = getIntegerToken(data, "|", no + 6);
		addressBook.transmigration = getIntegerToken(data, "|", no + 7);

		addressBook_.insert(i, addressBook);
#ifdef _MAILSHOWPLANET				// (可開放) 顯示名片星球
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
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	QString name;
	//__int64 nameLen;
	bool valid;
#ifdef _MAILSHOWPLANET				// (可開放) 顯示名片星球
	QString planetid[8];
	__int64 j;
#endif

	if (num >= MAX_ADDRESS_BOOK)
		return;

	valid = getIntegerToken(data, "|", 1) > 0;

	ADDRESS_BOOK addressBook = getAddressBook(num);
	if (!valid)
	{
		addressBook.valid = valid;
		addressBook.name.clear();
		return;
	}

	addressBook.valid = valid;

	getStringToken(data, "|", 2, name);
	makeStringFromEscaped(name);

	addressBook.name = name;
	addressBook.level = getIntegerToken(data, "|", 3);
	addressBook.dp = getIntegerToken(data, "|", 4);
	addressBook.onlineFlag = getIntegerToken(data, "|", 5) > 0;
	addressBook.modelid = getIntegerToken(data, "|", 6);
	addressBook.transmigration = getIntegerToken(data, "|", 7);

	addressBook_.insert(num, addressBook);

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
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	__int64 i;
	QString token;
	QString item;
	QStringList texts;

	//cary 確定 欄位 數
	__int64 cols = RESULT_CHR_EXP;
	getStringToken(data, ",", RESULT_CHR_EXP + 1, token);
	if (token[0] == 0)
	{
		cols = RESULT_CHR_EXP - 1;
		//battleResultMsg.resChr[RESULT_CHR_EXP - 1].petNo = -1;
		//battleResultMsg.resChr[RESULT_CHR_EXP - 1].levelUp = -1;
		//battleResultMsg.resChr[RESULT_CHR_EXP - 1].exp = -1;
	}
	//end cary
	QString playerExp = QObject::tr("player exp:");
	QString rideExp = QObject::tr("ride exp:");
	QString petExp = QObject::tr("pet exp:");
	PC pc = getPC();
	bool petOk = false;
	bool rideOk = false;
	bool charOk = false;

	for (i = 0; i < cols; ++i)
	{
		if (i >= 5)
			break;
		getStringToken(data, ",", i + 1, token);

		__int64 index = getIntegerToken(token, "|", 1);

		__int64 isLevelUp = getIntegerToken(token, "|", 2);

		QString temp;
		getStringToken(token, "|", 3, temp);
		__int64 exp = a62toi(temp);

		if (index == -2 && !charOk)
		{
			charOk = true;
			if (isLevelUp)
				++recorder[0].leveldifference;

			recorder[0].expdifference += exp;
			texts.append(playerExp + toQString(exp));
		}
		else if (pc.ridePetNo != -1 && pc.ridePetNo == index && !petOk)
		{
			petOk = true;
			if (isLevelUp)
				++recorder[index].leveldifference;

			recorder[index].expdifference += exp;
			texts.append(rideExp + toQString(exp));
		}
		else if (pc.battlePetNo != -1 && pc.battlePetNo == index && !rideOk)
		{
			rideOk = true;
			if (isLevelUp)
				++recorder[index].leveldifference;

			if (index >= 0 && index < (MAX_PET + 1))
				recorder[index].expdifference += exp;
			texts.append(petExp + toQString(exp));
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (texts.size() > 1 && injector.getEnableHash(kShowExpEnable))
		announce(texts.join(" "));

	checkAutoDropMeat();
	checkAutoAbility();
}

//戰後經驗 (逃跑或被打死不會有)
void Server::lssproto_RD_recv(char*)
{
	//QString data = toQUnicode(cdata);
	//if (data.isEmpty())
	//	return;

	//battleResultMsg.resChr[0].exp = getInteger62Token(data, "|", 1);
	//battleResultMsg.resChr[1].exp = getInteger62Token(data, "|", 2);
}

//道具位置交換
void Server::lssproto_SI_recv(int from, int to)
{
	swapItemLocal(from, to);
	updateItemByMemory();
	refreshItemInfo();
	updateComboBoxList();
}

//道具數據改變
void Server::lssproto_I_recv(char* cdata)
{
	{
		QWriteLocker locker(&itemInfoLock_);

		QString data = toQUnicode(cdata);
		if (data.isEmpty())
			return;

		__int64 i, j;
		__int64 no;
		QString name;
		QString name2;
		QString memo;
		//char *data = "9|烏力斯坦的肉||0|耐久力10前後回覆|24002|0|1|0|7|不會損壞|1|肉|20||10|烏力斯坦的肉||0|耐久力10前後回覆|24002|0|1|0|7|不會損壞|1|肉|20|";
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

		QHash <__int64, ITEM> items = item_.toHash();
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
				items.remove(i);
				continue;
			}
			items[i].valid = true;
			items[i].name = name;
			getStringToken(data, "|", no + 3, name2);//第二個道具名
			makeStringFromEscaped(name2);

			items[i].name2 = name2;
			items[i].color = getIntegerToken(data, "|", no + 4);//顏色
			if (items.value(i).color < 0)
				items[i].color = 0;
			getStringToken(data, "|", no + 5, memo);//道具介紹
			makeStringFromEscaped(memo);

			items[i].memo = memo;
			items[i].modelid = getIntegerToken(data, "|", no + 6);//道具形像
			items[i].field = getIntegerToken(data, "|", no + 7);//
			items[i].target = getIntegerToken(data, "|", no + 8);
			if (items.value(i).target >= 100)
			{
				items[i].target %= 100;
				items[i].deadTargetFlag = 1;
			}
			else
			{
				items[i].deadTargetFlag = 0;
			}
			items[i].level = getIntegerToken(data, "|", no + 9);//等級
			items[i].sendFlag = getIntegerToken(data, "|", no + 10);

			// 顯示物品耐久度
			QString damage;
			getStringToken(data, "|", no + 11, damage);
			makeStringFromEscaped(damage);

			if (damage.size() <= 16)
			{
				items[i].damage = damage;
			}

			QString pile;
			getStringToken(data, "|", no + 12, pile);
			makeStringFromEscaped(pile);

			items[i].stack = pile.toLongLong();
			if (items.value(i).valid && items.value(i).stack == 0)
				items[i].stack = 1;

			QString alch;
			getStringToken(data, "|", no + 13, alch);
			makeStringFromEscaped(alch);

			items[i].alch = alch;

			QString type;
			getStringToken(data, "|", no + 14, type);
			makeStringFromEscaped(type);

			items[i].type = type.toUShort();

#if 0
			items[i].道具類型 = getIntegerToken(data, "|", no + 14);
#endif

			QString jigsaw;
			getStringToken(data, "|", no + 15, jigsaw);
			makeStringFromEscaped(jigsaw);
			items[i].jigsaw = jigsaw;


			items[i].itemup = getIntegerToken(data, "|", no + 16);

			items[i].counttime = getIntegerToken(data, "|", no + 17);
		}

		item_ = items;
	}

	refreshItemInfo();
	updateComboBoxList();
	if (IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) > 0)
		IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_sub(1, std::memory_order_release);
}

//對話框
void Server::lssproto_WN_recv(int windowtype, int buttontype, int dialogid, int unitid, char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty() && buttontype == 0)
		return;

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
			"：", "？", "『", "～", "☆", "、", "，", "。", "歡迎",
			"欢迎",  "選", "选", "請問", "请问"
		};

		for (__int64 i = 0; i < strList.size(); ++i)
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

		for (__int64 i = 0; i < strList.size(); ++i)
		{
			strList[i] = strList[i].simplified();
			strList[i].remove("　");
			strList[i].remove("》");
			strList[i].remove("《");
			strList[i].remove("【");
			strList[i].remove("】");
			strList[i].remove("『");
			strList[i].remove("』");
			strList[i].remove("〖");
			strList[i].remove("〗");
		}
	}

	//下面是開始檢查寄放處, 寵店
	data.replace("\\n", "\n");
	data.replace("\\c", ",");
	data = data.trimmed();

	static const QStringList BankPetList = {
		"2\n請選擇寵物", "2\n请选择宠物",
		"2\n請選擇要從倉庫取出的寵物", "2\n请选择要从仓库取出的宠物"
	};
	static const QStringList BankItemList = {
		 "1|寄放店|要領取什么呢？|項目有很多|這樣子就可以了嗎？|", "1|寄放店|要领取什么呢？|项目有很多|这样子就可以了吗？|",
		 "1|寄放店|要賣什么呢？|項目有很多|這樣子就可以了嗎？|", "1|寄放店|要卖什么呢？|项目有很多|这样子就可以了吗？|"
	};
	static const QStringList FirstWarningList = {
		"上一次離線時間", "上一次离线时间"
	};
	static const QStringList SecurityCodeList = {
		 "安全密碼進行解鎖", "安全密码进行解锁"
	};
	static const QStringList KNPCList = {
		//zh_TW
		"如果能贏過我的"/*院藏*/, "如果想通過"/*近藏*/, "吼"/*紅暴*/, "你想找麻煩"/*七兄弟*/, "多謝～。",
		"轉以上確定要出售？", "再度光臨", "已經太多",

		//zh_CN
		"如果能赢过我的"/*院藏*/, "如果想通过"/*近藏*/, "吼"/*红暴*/, "你想找麻烦"/*七兄弟*/, "多謝～。",
		"转以上确定要出售？", "再度光临", "已经太多",
	};
	static const QRegularExpression rexBankPet(R"(LV\.\s*(\d+)\s*MaxHP\s*(\d+)\s*(\S+))");

	//這個是特殊訊息
	static const QRegularExpression rexExtraInfoBig5(R"((?:.*?豆子倍率：\d*(?:\.\d+)?剩余(\d+(?:\.\d+)?)分鐘)?\s+\S+\s+聲望：(\d+)\s+氣勢：(\d+)\s+貝殼：(\d+)\s+活力：(\d+)\s+積分：(\d+)\s+會員點：(\d+))");
	static const QRegularExpression rexExtraInfoGb2312(R"((?:.*?豆子倍率：\d*(?:\.\d+)?剩余(\d+(?:\.\d+)?)分钟)?\s+\S+\s+声望：(\d+)\s+气势：(\d+)\s+贝壳：(\d+)\s+活力：(\d+)\s+积分：(\d+)\s+会员点：(\d+))");

	QStringList linedatas;
	if (data.count("|") > 1)
	{
		linedatas = data.split(rexOR);
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
		QString tmp = data;
		tmp.remove("　");
		tmp = tmp.trimmed();
		if (!tmp.contains(it, Qt::CaseInsensitive))
			continue;

		tmp.remove(it);
		tmp = tmp.trimmed();
		data = tmp;

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
				bankPet.level = itMatch.captured(1).toLongLong();
				bankPet.maxHp = itMatch.captured(2).toLongLong();
				bankPet.name = itMatch.captured(3);
				currentBankPetList.second.append(bankPet);
			}
		}
		IS_WAITFOR_BANK_FLAG.store(false, std::memory_order_release);
		return;
	}

	for (const QString& it : BankItemList)
	{
		if (!data.contains("寄放店", Qt::CaseInsensitive))
			continue;

		currentBankItemList.clear();
		__int64 index = 0;
		for (;;)
		{
			ITEM item = {};
			QString temp;
			getStringToken(data, "|", 6 + index * 7, temp);
			makeStringFromEscaped(temp);
			if (temp.isEmpty())
				break;
			item.valid = true;
			item.name = temp;

			getStringToken(data, "|", 11 + index * 7, temp);
			makeStringFromEscaped(temp);
			item.memo = temp;
			currentBankItemList.append(item);
			++index;
		}
		IS_WAITFOR_BANK_FLAG.store(false, std::memory_order_release);
		return;

	}

	//匹配額外訊息
	QRegularExpressionMatch extraInfoMatch = rexExtraInfoBig5.match(data);
	if (extraInfoMatch.hasMatch())
	{
		currencydata_t currency;
		__int64 n = 1;
		if (extraInfoMatch.lastCapturedIndex() == 7)
			currency.expbufftime = static_cast<__int64>(qFloor(extraInfoMatch.captured(n++).toDouble() * 60.0));
		else
			currency.expbufftime = 0;

		currency.prestige = extraInfoMatch.captured(n++).toLongLong();
		currency.energy = extraInfoMatch.captured(n++).toLongLong();
		currency.shell = extraInfoMatch.captured(n++).toLongLong();
		currency.vitality = extraInfoMatch.captured(n++).toLongLong();
		currency.points = extraInfoMatch.captured(n++).toLongLong();
		currency.VIPPoints = extraInfoMatch.captured(n++).toLongLong();

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
	}
	else
	{
		extraInfoMatch = rexExtraInfoGb2312.match(data);
		if (extraInfoMatch.hasMatch())
		{
			currencydata_t currency;
			__int64 n = 1;
			if (extraInfoMatch.lastCapturedIndex() == 7)
				currency.expbufftime = static_cast<__int64>(qFloor(extraInfoMatch.captured(n++).toDouble() * 60.0));
			else
				currency.expbufftime = 0;
			currency.prestige = extraInfoMatch.captured(n++).toLongLong();
			currency.energy = extraInfoMatch.captured(n++).toLongLong();
			currency.shell = extraInfoMatch.captured(n++).toLongLong();
			currency.vitality = extraInfoMatch.captured(n++).toLongLong();
			currency.points = extraInfoMatch.captured(n++).toLongLong();
			currency.VIPPoints = extraInfoMatch.captured(n++).toLongLong();

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
		}
	}

	//這裡開始是 KNPC
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	data = data.simplified();
	for (const QString& it : FirstWarningList)
	{
		if (data.contains(it, Qt::CaseInsensitive))
		{
			__int64 currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			injector.sendMessage(kDistoryDialog, NULL, NULL);
			return;
		}
	}

	for (const QString& it : SecurityCodeList)
	{
		if (!data.contains(it, Qt::CaseInsensitive))
			continue;

		QString securityCode = injector.getStringHash(kGameSecurityCodeString);
		if (!securityCode.isEmpty())
		{
			unlockSecurityCode(securityCode);
			__int64 currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			injector.sendMessage(kDistoryDialog, NULL, NULL);
			return;
		}
	}


	if (injector.getEnableHash(kKNPCEnable))
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

void Server::lssproto_PME_recv(int unitid, int graphicsno, const QPoint& pos, int dir, int flg, int no, char* cdata)
{
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
		QString data = toQUnicode(cdata);
		if (data.isEmpty())
			return;

		QString smalltoken;
		__int64 id;
		__int64 x;
		__int64 y;
		__int64 dir;
		__int64 modelid;
		__int64 level;
		__int64 nameColor;
		QString name;
		QString freeName;
		__int64 walkable;
		__int64 height;
		__int64 charType;
		__int64 ps = 2;

		charType = getIntegerToken(data, "|", ps++);
		getStringToken(data, "|", ps++, smalltoken);
		id = a62toi(smalltoken);
		getStringToken(data, "|", ps++, smalltoken);
		x = smalltoken.toLongLong();
		getStringToken(data, "|", ps++, smalltoken);
		y = smalltoken.toLongLong();
		getStringToken(data, "|", ps++, smalltoken);
		dir = (smalltoken.toLongLong() + 3) % 8;
		getStringToken(data, "|", ps++, smalltoken);
		modelid = smalltoken.toLongLong();
		getStringToken(data, "|", ps++, smalltoken);
		level = smalltoken.toLongLong();
		nameColor = getIntegerToken(data, "|", ps++);
		getStringToken(data, "|", ps++, name);
		makeStringFromEscaped(name);

		getStringToken(data, "|", ps++, freeName);
		makeStringFromEscaped(freeName);

		getStringToken(data, "|", ps++, smalltoken);
		walkable = smalltoken.toLongLong();
		getStringToken(data, "|", ps++, smalltoken);
		height = smalltoken.toLongLong();
	}
}

//天氣
void Server::lssproto_EF_recv(int effect, int level, char* coption)
{
	//__int64 currentIndex = getIndex();
	//Injector& injector = Injector::getInstance(currentIndex);
	//if (!getOnlineFlag())
	//	return;

	//SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	//__int64 floor = getFloor();
	//emit signalDispatcher.updateNpcList(floor);
}

//求救
void Server::lssproto_HL_recv(int)
{

}

//開始戰鬥
void Server::lssproto_EN_recv(int result, int field)
{
	//開始戰鬥為1，未開始戰鬥為0
	if (result > 0)
	{
		setBattleFlag(true);
		IS_LOCKATTACK_ESCAPE_DISABLE.store(false, std::memory_order_release);
		battleCharEscapeFlag.store(false, std::memory_order_release);
		battle_total.fetch_add(1, std::memory_order_release);
		battlePetDisableList_.clear();
		battlePetDisableList_.resize(MAX_PET);
		normalDurationTimer.restart();
		battleDurationTimer.restart();
		oneRoundDurationTimer.restart();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusInBattle);
	}
}

//戰鬥每回合資訊
void Server::lssproto_B_recv(char* ccommand)
{
	QString command = toQUnicode(ccommand);
	if (command.isEmpty())
		return;

	QString first = command.left(2);
	first.remove("B");

	QString data = command.mid(3);
	if (data.isEmpty())
		return;

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	HANDLE hProcess = injector.getProcess();
	__int64 hModule = injector.getProcessModule();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	switch (first.at(0).unicode())
	{
	case 'P':
	{
		QStringList list = data.split(rexOR);
		if (list.size() < 3)
			break;

		emit signalDispatcher.battleTableAllItemResetColor();

		battledata_t bt = getBattleData();

		battle_one_round_time.store(oneRoundDurationTimer.elapsed(), std::memory_order_release);
		oneRoundDurationTimer.restart();

		battleCharCurrentPos.store(list.value(0).toLongLong(nullptr, 16), std::memory_order_release);
		battleBpFlag.store(list.value(1).toLongLong(nullptr, 16), std::memory_order_release);
		battleCharCurrentMp.store(list.value(2).toLongLong(nullptr, 16), std::memory_order_release);

		bt.player.pos = battleCharCurrentPos;
		bt.charAlreadyAction = false;
		bt.charAlreadyAction = false;

		{
			pc_.mp = battleCharCurrentMp.load(std::memory_order_acquire);
			pc_.mpPercent = util::percent(pc_.mp, pc_.maxMp);
		}

		updateCurrentSideRange(bt);
		setBattleData(bt);
		break;
	}
	case 'A':
	{
		QStringList list = data.split(rexOR);
		if (list.size() < 2)
			break;

		battledata_t bt = getBattleData();

		battleCurrentAnimeFlag.store(list.value(0).toLongLong(nullptr, 16), std::memory_order_release);
		battleCurrentRound.store(list.value(1).toLongLong(nullptr, 16), std::memory_order_release);

		if (battleCurrentAnimeFlag.load(std::memory_order_acquire) <= 0)
			break;

		battleobject_t empty = {};
		if (!bt.objects.isEmpty())
		{
			QVector<battleobject_t> objs = bt.objects;
			for (__int64 i = bt.alliemin; i <= bt.alliemax; ++i)
			{
				if (i >= bt.objects.size())
					break;
				if (checkFlagState(i) && !bt.objects.value(i, empty).ready)
				{
#if 0
					if (i == battleCharCurrentPos.load(std::memory_order_acquire))
					{
						qDebug() << QString("自己 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName);
					}
					if (i == battleCharCurrentPos.load(std::memory_order_acquire) + 5)
					{
						qDebug() << QString("戰寵 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName);
					}
					else
					{
						qDebug() << QString("隊友 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName);
					}
#endif
					emit signalDispatcher.notifyBattleActionState(i, battleCharCurrentPos.load(std::memory_order_acquire) >= (MAX_ENEMY / 2));
					objs[i].ready = true;
				}
			}

			for (__int64 i = bt.enemymin; i <= bt.enemymax; ++i)
			{
				if (i >= bt.objects.size())
					break;
				if (checkFlagState(i) && !bt.objects.value(i, empty).ready)
				{
					emit signalDispatcher.notifyBattleActionState(i, battleCharCurrentPos.load(std::memory_order_acquire) < (MAX_ENEMY / 2));
					objs[i].ready = true;
				}
			}

			bt.objects = objs;
		}

		setBattleData(bt);
		break;
	}
	case 'C':
	{
		battledata_t bt = getBattleData();

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

		__int64 i = 0, j = 0;
		__int64 n = 0;

		QString temp;
		QStringList tempList = {};
		//檢查敵我其中一方是否全部陣亡
		bool isEnemyAllDead = true;
		bool isAllieAllDead = true;
		battleobject_t obj = {};
		__int64 pos = 0;
		bool ok = false;
		bool valid = false;

		bt.fieldAttr = getIntegerToken(data, "|", 1);

		{
			QHash<__int64, PET> pets = pet_.toHash();

			for (;;)
			{
				/*
				16進制使用 a62toi
				string 使用 getStringToken(data, "|", n, var);
				而後使用 makeStringFromEscaped(var) 處理轉譯

				__int64 使用 getIntegerToken(data, "|", n);
				*/

				getStringToken(data, "|", i * 13 + 2, temp);
				pos = temp.toLongLong(&ok, 16);
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
				obj.modelid = temp.toLongLong(nullptr, 16);

				getStringToken(data, "|", i * 13 + 6, temp);
				obj.level = temp.toLongLong(nullptr, 16);

				getStringToken(data, "|", i * 13 + 7, temp);
				obj.hp = temp.toLongLong(nullptr, 16);

				getStringToken(data, "|", i * 13 + 8, temp);
				obj.maxHp = temp.toLongLong(nullptr, 16);

				getStringToken(data, "|", i * 13 + 9, temp);
				obj.status = temp.toLongLong(nullptr, 16);
				if (checkAND(obj.status, BC_FLG_DEAD))
				{
					obj.hp = 0;
					obj.hpPercent = 0;
				}
				else
					obj.hpPercent = util::percent(obj.hp, obj.maxHp);

				obj.rideFlag = getIntegerToken(data, "|", i * 13 + 10);

				getStringToken(data, "|", i * 13 + 11, temp);
				makeStringFromEscaped(temp);

				obj.rideName = temp;

				getStringToken(data, "|", i * 13 + 12, temp);
				obj.rideLevel = temp.toLongLong(nullptr, 16);

				getStringToken(data, "|", i * 13 + 13, temp);
				obj.rideHp = temp.toLongLong(nullptr, 16);

				getStringToken(data, "|", i * 13 + 14, temp);
				obj.rideMaxHp = temp.toLongLong(nullptr, 16);

				obj.rideHpPercent = util::percent(obj.rideHp, obj.rideMaxHp);

				valid = obj.modelid > 0 && obj.maxHp > 0 && obj.level > 0 && !checkAND(obj.status, BC_FLG_HIDE) && !checkAND(obj.status, BC_FLG_DEAD);

				if ((pos >= bt.enemymin) && (pos <= bt.enemymax) && obj.rideFlag == 0 && obj.modelid > 0 && !obj.name.isEmpty())
				{
					QStringList _enemyNameListCache = enemyNameListCache.get();
					if (!_enemyNameListCache.contains(obj.name))
					{
						_enemyNameListCache.append(obj.name);

						if (_enemyNameListCache.size() > 1)
						{
							_enemyNameListCache.removeDuplicates();
							std::sort(_enemyNameListCache.begin(), _enemyNameListCache.end(), util::customStringCompare);
						}

						enemyNameListCache.set(_enemyNameListCache);
					}
				}

				if (battleCharCurrentPos.load(std::memory_order_acquire) == pos)
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

					pc_.hp = obj.hp;
					pc_.maxHp = obj.maxHp;
					pc_.hpPercent = util::percent(obj.hp, obj.maxHp);

					if (obj.hp == 0 || checkAND(obj.status, BC_FLG_DEAD))
					{
						if (!recorder[0].deadthcountflag)
						{
							recorder[0].deadthcountflag = true;
							++recorder[0].deadthcount;
						}
					}
					else
					{
						recorder[0].deadthcountflag = false;
					}

					emit signalDispatcher.updateCharHpProgressValue(obj.level, obj.hp, obj.maxHp);

					//騎寵存在

					if (obj.rideFlag == 1)
					{
						n = -1;
						for (j = 0; j < MAX_PET; ++j)
						{
							if ((pets.value(j).maxHp == obj.rideMaxHp) &&
								(pets.value(j).level == obj.rideLevel) &&
								matchPetNameByIndex(j, obj.rideName))
							{
								n = j;
								break;
							}
						}

						if (pc_.ridePetNo != n)
							pc_.ridePetNo = n;
					}
					//騎寵不存在
					else
					{
						if (pc_.ridePetNo != -1)
						{
							if (pc_.ridePetNo >= 0 && pc_.ridePetNo < MAX_PET)
							{
								pets[pc_.ridePetNo].hp = 0;
								if (!recorder[pc_.ridePetNo + 1].deadthcountflag)
								{
									recorder[pc_.ridePetNo + 1].deadthcountflag = true;
									++recorder[pc_.ridePetNo + 1].deadthcount;
								}
							}
							pc_.ridePetNo = -1;
							mem::write <short>(hProcess, hModule + kOffsetRidePetIndex, -1);
						}
					}
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
				tempList.append(toQString(obj.pos));

				QString statusStr = getBadStatusString(obj.status);
				if (!statusStr.isEmpty())
					statusStr = QString("[%1]").arg(statusStr);

				if (obj.pos == battleCharCurrentPos.load(std::memory_order_acquire))
				{
					temp = QString("　[%1]★%2%3 LV:%4(%5|%6)%7")
						.arg(obj.pos + 1)
						.arg("")
						.arg(obj.name)
						.arg(obj.level)
						.arg(obj.hp)
						.arg(obj.hpPercent)
						.arg(statusStr);
				}
				else
				{
					if (obj.pos >= bt.enemymin && obj.pos <= bt.enemymax)
						temp = QString("　[%1]%2%3 LV:%4(%5|%6)%7")
						.arg(obj.pos + 1)
						.arg(QString::number(obj.modelid) + ":")
						.arg(obj.name)
						.arg(obj.level)
						.arg(obj.hp)
						.arg(obj.hpPercent)
						.arg(statusStr);
					else
						temp = QString("　[%1]%2%3 LV:%4(%5|%6)%7")
						.arg(obj.pos + 1)
						.arg("")
						.arg(obj.name)
						.arg(obj.level)
						.arg(obj.hp)
						.arg(obj.hpPercent)
						.arg(statusStr);
				}

				tempList.append(temp);
				temp.clear();

				if (obj.rideFlag == 1)
				{
					temp = QString(",%1LV:%2(%3)")
						.arg(QObject::tr("R"))
						.arg(obj.rideLevel)
						.arg(obj.rideHp);
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

			for (__int64 i = bt.enemymin; i <= bt.enemymax; ++i)
			{
				battleobject_t obj = bt.objects.value(i, battleobject_t{});
				if (obj.level == 1)
					emit signalDispatcher.battleTableItemForegroundColorChanged(i, QColor("#32A3FF"));
			}

			//更新戰寵數據
			if (battleCharCurrentPos.load(std::memory_order_acquire) >= 0 && battleCharCurrentPos.load(std::memory_order_acquire) < bt.objects.size())
			{
				obj = bt.objects.value(battleCharCurrentPos.load(std::memory_order_acquire) + 5, battleobject_t{});

				//戰寵不存在
				if (!checkAND(obj.status, BC_FLG_HIDE/*排除地球一周*/))
				{
					if ((obj.level <= 0 || obj.maxHp <= 0 || obj.modelid <= 0))
					{
						if (pc_.battlePetNo >= 0)
						{
							//被打飛(也可能是跑走)
							if (!recorder[pc_.battlePetNo + 1].deadthcount)
							{
								recorder[pc_.battlePetNo + 1].deadthcountflag = true;
								++recorder[pc_.battlePetNo + 1].deadthcount;
							}

							mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, -1);
							mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + pc_.battlePetNo * sizeof(short), 0);
							pets[pc_.battlePetNo].hp = 0;
							pc_.selectPetNo[pc_.battlePetNo] = 0;
							pc_.battlePetNo = -1;
						}

						emit signalDispatcher.updateLabelPetAction("");
						emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
					}
					//戰寵存在
					else
					{
						emit signalDispatcher.updatePetHpProgressValue(obj.level, obj.hp, obj.maxHp);
						n = -1;
						for (j = 0; j < MAX_PET; ++j)
						{
							if ((pets.value(j).maxHp == obj.maxHp) && (pets.value(j).level == obj.level)
								&& (pets.value(j).modelid == obj.modelid)
								&& (matchPetNameByIndex(j, obj.name)))
							{
								n = j;
								break;
							}
						}

						if (pc_.battlePetNo != n)
							pc_.battlePetNo = n;

						//戰寵死亡
						if (obj.hp == 0 || checkAND(obj.status, BC_FLG_DEAD))
						{
							if (!recorder[pc_.battlePetNo + 1].deadthcountflag)
							{
								recorder[pc_.battlePetNo + 1].deadthcountflag = true;
								++recorder[pc_.battlePetNo + 1].deadthcount;
							}
						}
						else
							recorder[pc_.battlePetNo + 1].deadthcountflag = false;

						if (pc_.battlePetNo >= 0 && pc_.battlePetNo < MAX_PET)
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

							pets[pc_.battlePetNo].hp = obj.hp;
							pets[pc_.battlePetNo].maxHp = obj.maxHp;
							pets[pc_.battlePetNo].hpPercent = obj.hpPercent;
						}
					}
				}
			}
		}

		setBattleData(bt);
		setWindowTitle();

		if (isAllieAllDead || isEnemyAllDead)
		{
			setBattleEnd();
			return;
		}

		//正常的動作發包
		doBattleWork(true);
		break;
	}
	case 'U':
	{
		battleCharEscapeFlag.store(true, std::memory_order_release);
		break;
	}
	case 'D':
	case 'H':
	case 'b':
		break;
	default:
		qDebug() << "lssproto_B_recv: unknown command" << command;
		break;
	}
}

//寵物取消戰鬥狀態 (不是每個私服都有)
void Server::lssproto_PETST_recv(int petarray, int result)
{
	updateDatasFromMemory();
	updateComboBoxList();
}

//戰寵狀態改變
void Server::lssproto_KS_recv(int petarray, int result)
{
	updateDatasFromMemory();
	updateComboBoxList();
}

//寵物等待狀態改變 (不是每個私服都有)
void Server::lssproto_SPET_recv(int standbypet, int result)
{
	updateDatasFromMemory();
	updateComboBoxList();
}

//可用點數改變
void Server::lssproto_SKUP_recv(int point)
{
	PC pc = getPC();
	pc.point = point;
	setPC(pc);
	IS_WAITFOT_SKUP_RECV.store(false, std::memory_order_release);
}

//收到郵件
void Server::lssproto_MSG_recv(int aindex, char* ctext, int color)
{
	QString text = toQUnicode(ctext);
	if (text.isEmpty())
		return;
	//char moji[256];
	__int64 noReadFlag;

	if (aindex < 0 || aindex >= MAIL_MAX_HISTORY)
		return;

	MAIL_HISTORY mailHistory = getMailHistory(aindex);

	mailHistory.newHistoryNo--;

	if (mailHistory.newHistoryNo <= -1)
		mailHistory.newHistoryNo = MAIL_MAX_HISTORY - 1;

	QStringList list = { getAddressBook(aindex).name };

	getStringToken(text, "|", 1, mailHistory.dateStr[mailHistory.newHistoryNo]);

	getStringToken(text, "|", 2, mailHistory.str[mailHistory.newHistoryNo]);

	QString temp = mailHistory.str[mailHistory.newHistoryNo];
	makeStringFromEscaped(temp);

	mailHistory.str[mailHistory.newHistoryNo] = temp;
	list.append(temp);

	noReadFlag = getIntegerToken(text, "|", 3);

	if (noReadFlag != -1)
	{
		mailHistory.noReadFlag[mailHistory.newHistoryNo] = noReadFlag;
		list.append(toQString(noReadFlag));

		mailHistory.petLevel[mailHistory.newHistoryNo] = getIntegerToken(text, "|", 4);
		list.append(toQString(mailHistory.petLevel[mailHistory.newHistoryNo]));

		getStringToken(text, "|", 5, mailHistory.petName[mailHistory.newHistoryNo]);

		temp = mailHistory.petName[mailHistory.newHistoryNo];
		makeStringFromEscaped(temp);

		mailHistory.petName[mailHistory.newHistoryNo] = temp;
		list.append(temp);

		mailHistory.itemGraNo[mailHistory.newHistoryNo] = getIntegerToken(text, "|", 6);
		list.append(toQString(mailHistory.itemGraNo[mailHistory.newHistoryNo]));

		//sprintf_s(moji, "收到%s送來的寵物郵件！", addressBook.name);
		announce(list.join("|"), color);
	}
	else
	{
		mailHistory.noReadFlag[mailHistory.newHistoryNo] = TRUE;
		list.append(toQString(mailHistory.noReadFlag[mailHistory.newHistoryNo]));

		announce(list.join("|"), color);

		QString msg = mailHistory.str[mailHistory.newHistoryNo];
		makeStringFromEscaped(msg);

		__int64 currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		QString whiteList = injector.getStringHash(kMailWhiteListString);
		if (msg.startsWith("dostr") && whiteList.contains(getAddressBook(aindex).name))
		{
			std::ignore = QtConcurrent::run([this, msg, currentIndex]()
				{
					Interpreter interpreter(currentIndex);
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

	mailHistory_.insert(aindex, mailHistory);

	if (mailHistoryWndSelectNo == aindex)
	{
		mailHistoryWndPageNo++;

		if (mailHistoryWndPageNo >= MAIL_MAX_HISTORY)
			mailHistoryWndPageNo = 0;
	}
}

//收到寵郵
void Server::lssproto_PS_recv(int result, int havepetindex, int havepetskill, int toindex)
{
}

void Server::lssproto_SE_recv(const QPoint&, int, int)
{

}

//戰後坐標更新
void Server::lssproto_XYD_recv(const QPoint& pos, int dir)
{
	//dir = (dir + 3) % 8;
	pc_.dir = dir;
	setPoint(pos);
	setBattleEnd();
}

void Server::lssproto_WO_recv(int)
{

}

/////////////////////////////////////////////////////////

//服務端發來的ECHO 一般是30秒
void Server::lssproto_Echo_recv(char* test)
{
	if (isEOTTLSend.load(std::memory_order_acquire))
	{
		__int64 time = eottlTimer.elapsed();
		lastEOTime.store(time, std::memory_order_release);
		isEOTTLSend.store(false, std::memory_order_release);
		announce(QObject::tr("server response time:%1ms").arg(time));//伺服器響應時間:xxxms
	}
}

void Server::lssproto_NU_recv(int)
{
}

void Server::lssproto_CharNumGet_recv(int, int)
{
}

void Server::lssproto_ProcGet_recv(char*)
{

}

void Server::lssproto_R_recv(char*)
{

}

void Server::lssproto_D_recv(int, int, int, char*)
{
}

//家族頻道
void Server::lssproto_FM_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	QString FMType1;
	QString FMType2;
	QString FMType3;

	getStringToken(data, "|", 1, FMType1);
	//makeStringFromEscaped( FMType1 );
	getStringToken(data, "|", 2, FMType2);
	//makeStringFromEscaped( FMType2 );

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

			PC pc = getPC();
			pc.channel = FMType3.toLongLong();
			if (pc.channel != -1)
				pc.quickChannel = pc.channel;
			setPC(pc);
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
			//pc.ridePetNo = FMType3.toLongLong();
		}
	}
	else if (FMType1 == "L")	// 族長功能
	{
		if (FMType2 == "CHANGE")
		{
			//initFamilyLeaderChange(data);
		}
	}
}

//服務端發來的用於固定客戶端的速度
void Server::lssproto_CS_recv(int)
{
}

//戰鬥結束
void Server::lssproto_NC_recv(int)
{

}

//任務日誌
void Server::lssproto_JOBDAILY_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	//解讀資料
	__int64 i = 1, j = 1;
	QString getdata;
	QString perdata;

	jobdaily_.clear();

	while (getStringToken(data, "#", i, getdata) != 1)
	{
		while (getStringToken(getdata, "|", j, perdata) != 1)
		{
			if (i - 1 >= MAX_MISSION)
				continue;
			JOBDAILY jobdaily = {};
			switch (j)
			{
			case 1:
				jobdaily.JobId = perdata.toLongLong();
				break;
			case 2:
				jobdaily.explain = perdata.simplified();
				break;
			case 3:
				jobdaily.state = perdata.simplified();
				break;
			default: /*StockChatBufferLine("每筆資料內參數有錯誤", FONT_PAL_RED);*/
				break;
			}

			jobdaily_.insert(i - 1, jobdaily);
			perdata.clear();
			++j;
		}
		getdata.clear();
		j = 1;
		++i;
	}

	//是否有接收到資料
	//if (i > 1)
	//	JobdailyGetMax = i - 2;
	//else
	//	JobdailyGetMax = -1;

	if (IS_WAITFOR_JOBDAILY_FLAG.load(std::memory_order_acquire))
		IS_WAITFOR_JOBDAILY_FLAG.store(false, std::memory_order_release);
}

//導師系統
void Server::lssproto_TEACHER_SYSTEM_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
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

	}
	else if (szMessage.startsWith("C"))// 詢問是否要對方當你的導師
	{

	}
	else if (szMessage.startsWith("A"))// 超過一人,詢問要找誰當導師
	{

	}
	else if (szMessage.startsWith("V"))// 顯示導師資料
	{

	}
}

void Server::lssproto_S2_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	QString szMessage;

	__int64 ftype = 0, newfame = 0;

	PC pc = getPC();

	getStringToken(data, "|", 1, szMessage);

	if (szMessage == "FAME")
	{
		getStringToken(data, "|", 2, szMessage);
		pc.fame = szMessage.toLongLong();
	}

	pc.ftype = 0;
	pc.newfame = 0;

	if (getStringToken(data, "|", 3, szMessage) == 1)
		return;
	ftype = szMessage.toLongLong();
	getStringToken(data, "|", 4, szMessage);
	newfame = szMessage.toLongLong();
	pc.ftype = ftype;
	pc.newfame = newfame;

	setPC(pc);
}

//煙火
void Server::lssproto_Firework_recv(int, int, int)
{

}

void Server::lssproto_DENGON_recv(char*, int, int)
{
}

//收到玩家對話或公告
void Server::lssproto_TK_recv(int index, char* cmessage, int color)
{
	__int64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	QString id;
#ifdef _MESSAGE_FRONT_
	QString msg1;
	char* msg;
#else
	QString msg;
#endif

#ifdef _MESSAGE_FRONT_
	msg1[0] = 0xA1;
	msg1[1] = 0xF4;
	msg1[2] = 0;
	msg = msg1 + 2;
#endif
	QString message = toQUnicode(cmessage);
	makeStringFromEscaped(message);
	if (message.isEmpty())
		return;

	static const QRegularExpression rexGetGold(R"(到\s*(\d+)\s*石)");
	static const QRegularExpression rexPickGold(R"([獲|获]\s*(\d+)\s*Stone)");
	static const QRegularExpression rexRep(R"(声望点数：\s*(\d+))");
	static const QRegularExpression rexVit(R"(气势点数：\s*(\d+))");
	static const QRegularExpression rexVip(R"(金币点数：\s*(\d+))");

	getStringToken(message, "|", 1, id);

	if (id.startsWith("P"))
	{
		if (message.contains(rexGetGold))
		{
			//取出中間的整數
			QRegularExpressionMatch match = rexGetGold.match(message);
			if (match.hasMatch())
			{
				QString strGold = match.captured(1);
				__int64 nGold = strGold.toLongLong();
				if (nGold > 0)
				{
					recorder[0].goldearn += nGold;
				}
			}
		}

		if (message.contains(rexRep))
		{
			QRegularExpressionMatch match = rexRep.match(message);
			if (match.hasMatch())
			{
				QString strRep = match.captured(1);
				__int64 nRep = strRep.toLongLong();
				if (nRep > 0)
				{
					currencydata_t currency = currencyData;
					currency.prestige = nRep;
					currencyData = currency;
				}
			}
		}

		if (message.contains(rexVit))
		{
			QRegularExpressionMatch match = rexVit.match(message);
			if (match.hasMatch())
			{
				QString strVit = match.captured(1);
				__int64 nVit = strVit.toLongLong();
				if (nVit > 0)
				{
					currencydata_t currency = currencyData;
					currency.vitality = nVit;
					currencyData = currency;
				}
			}
		}

		if (message.contains(rexVip))
		{
			QRegularExpressionMatch match = rexVip.match(message);
			if (match.hasMatch())
			{
				QString strVip = match.captured(1);
				__int64 nVip = strVip.toLongLong();
				if (nVip > 0)
				{
					currencydata_t currency = currencyData;
					currency.VIPPoints = nVip;
					currencyData = currency;
				}
			}
		}
		//"P|P|拾獲 337181 Stone"

		if (message.contains(rexPickGold))
		{
			//取出中間的整數
			QRegularExpressionMatch match = rexPickGold.match(message);
			if (match.hasMatch())
			{
				QString strGold = match.captured(1);
				__int64 nGold = strGold.toLongLong();
				if (nGold > 0)
				{
					recorder[0].goldearn += nGold;
				}
			}
		}

#ifndef _CHANNEL_MODIFY
		getStringToken(message, "|", 2, msg);
		makeStringFromEscaped(msg);
#ifdef _TRADETALKWND				//交易新增對話框架
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

				}
				else if (szToken == "TE")
				{

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
						lastSecretChatName = tellName;
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
#ifdef _TELLCHANNEL		// (不可開)密語頻道
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
	}

	chatQueue.enqueue(qMakePair(color, msg));
	emit signalDispatcher.appendChatLog(msg, color);
}

//地圖數據更新，重新繪製地圖
void Server::lssproto_MC_recv(int fl, int x1, int y1, int x2, int y2, int tileSum, int partsSum, int eventSum, char* cdata)
{
	//QString data = toQUnicode(cdata);
	//if (data.isEmpty())
	//	return;

	//QString showString, floorName;

	//getStringToken(data, "|", 1, showString);
	//makeStringFromEscaped(showString);

	//QString strPal;

	//getStringToken(showString, "|", 1, floorName);
	//makeStringFromEscaped(floorName);

	//getStringToken(showString, "|", 2, strPal);
	std::ignore = getFloor();
	std::ignore = getFloorName();
	std::ignore = getDir();
	std::ignore = getPoint();
}

//地圖數據更新，重新寫入地圖
void Server::lssproto_M_recv(int fl, int x1, int y1, int x2, int y2, char* cdata)
{
	//QString data = toQUnicode(cdata);
	//if (data.isEmpty())
	//	return;

	//QString showString, floorName, tilestring, partsstring, eventstring, tmp;

	//getStringToken(data, "|", 1, showString);
	//makeStringFromEscaped(showString);

	//QString strPal;

	//getStringToken(showString, "|", 1, floorName);
	//makeStringFromEscaped(floorName);
	std::ignore = getFloor();
	std::ignore = getFloorName();
	std::ignore = getDir();
	std::ignore = getPoint();
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
	const QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	setOnlineFlag(true);

	__int64 i = 0, j = 0, id = 0, x = 0, y = 0, dir = 0;
	__int64 modelid = 0, level = 0, nameColor = 0, walkable = 0, height = 0, classNo = 0, money = 0, charType = 0, charNameColor = 0;
	QString bigtoken, smalltoken, name, freeName, info, fmname, petname;

	QString titlestr;
	__int64 titleindex = 0;
	__int64 petlevel = 0;
	// 人物職業
	__int64 profession_class = 0, profession_level = 0, profession_skill_point = 0, profession_exp = 0;
	// 排行榜NPC
	__int64 herofloor = 0;
	__int64 picture = 0;
	QString gm_name;

	__int64 pcid = getPC().id;

	for (i = 0; ; ++i)
	{
		getStringToken(data, ",", i + 1, bigtoken);
		if (bigtoken.isEmpty())
			break;
#ifdef _OBJSEND_C
		getStringToken(bigtoken, "|", 1, smalltoken);
		if (smalltoken.isEmpty())
			break;
		switch (smalltoken.toLongLong())
		{
		case 1://OBJTYPE_CHARA
		{
			charType = getIntegerToken(bigtoken, "|", 2);
			getStringToken(bigtoken, "|", 3, smalltoken);
			id = a62toi(smalltoken);

			getStringToken(bigtoken, "|", 4, smalltoken);
			x = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 5, smalltoken);
			y = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 6, smalltoken);
			dir = (smalltoken.toLongLong() + 3) % 8;
			getStringToken(bigtoken, "|", 7, smalltoken);
			modelid = smalltoken.toLongLong();

			if (modelid == 9999)
				continue;

			getStringToken(bigtoken, "|", 8, smalltoken);
			level = smalltoken.toLongLong();
			nameColor = getIntegerToken(bigtoken, "|", 9);
			getStringToken(bigtoken, "|", 10, name);
			makeStringFromEscaped(name);

			getStringToken(bigtoken, "|", 11, freeName);
			makeStringFromEscaped(freeName);

			getStringToken(bigtoken, "|", 12, smalltoken);
			walkable = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 13, smalltoken);
			height = smalltoken.toLongLong();
			charNameColor = getIntegerToken(bigtoken, "|", 14);
			getStringToken(bigtoken, "|", 15, fmname);
			makeStringFromEscaped(fmname);

			getStringToken(bigtoken, "|", 16, petname);
			makeStringFromEscaped(petname);

			getStringToken(bigtoken, "|", 17, smalltoken);
			petlevel = smalltoken.toLongLong();

			//getStringToken(bigtoken, "|", 18, smalltoken);
			//noticeNo = smalltoken.toLongLong();

			//_CHARTITLE_STR_
			//getStringToken(bigtoken, "|", 23, sizeof(titlestr) - 1, titlestr);
			//titleindex = atoi(titlestr);
			//memset(titlestr, 0, 128);
			//if (titleindex > 0)
			//{
			//	extern char* FreeGetTitleStr(int id);
			//	sprintf(titlestr, "%s", FreeGetTitleStr(titleindex));
			//}

			//人物職業
			getStringToken(bigtoken, "|", 18, smalltoken);
			profession_class = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 19, smalltoken);
			profession_level = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 20, smalltoken);
			profession_exp = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 20, smalltoken);
			profession_skill_point = smalltoken.toLongLong();

			getStringToken(bigtoken, "|", 21, smalltoken);
			herofloor = smalltoken.toLongLong();

			getStringToken(bigtoken, "|", 22, smalltoken);
			picture = smalltoken.toLongLong();

			getStringToken(bigtoken, "|", 23, gm_name);
			makeStringFromEscaped(gm_name);

			if (charNameColor < 0)
				charNameColor = 0;


			if (pcid == id)
			{
				QWriteLocker locker(&charInfoLock_);
				PC pc = pc_;
				//_CHARTITLE_STR_
				//getCharTitleSplit(titlestr, &pc.ptAct->TitleText);

				// 人物職業
				//排行榜NPC
				//setPcParam(name, freeName, level, petname, petlevel, nameColor, walkable, height, profession_class, profession_level, profession_skill_point, herofloor);
				pc.name = name;

				pc.freeName = freeName;

				pc.level = level;

				pc.ridePetName = petname;

				pc.ridePetLevel = petlevel;

				pc.nameColor = nameColor;
				if (walkable != 0)
				{
					//pc.status |= CHR_STATUS_W;
				}
				if (height != 0)
				{
					//pc.status |= CHR_STATUS_H;
				}


				// 人物職業
				pc.profession_class = profession_class;
				pc.profession_level = profession_level;
				pc.profession_skill_point = profession_skill_point;
				pc.profession_exp = profession_exp;

				// 排行榜NPC
				pc.herofloor = herofloor;

				pc.nameColor = charNameColor;

				pc_ = pc;
			}
			else
			{
#ifdef _CHAR_PROFESSION			//人物職業
#ifdef _GM_IDENTIFY		//GM識別
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, gm_name);
#else
#ifdef _NPC_PICTURE
				setNpcCharObj(id, modelid, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class, picture);
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

#ifdef _NPC_EVENT_NOTICE
				if (charType == 13 && noticeNo > 0)
				{
					setNpcNotice(ptAct, noticeNo);
				}
#endif
			}

			if (name == "を�そó")//排除亂碼
				break;

			mapunit_t unit = mapUnitHash.value(id);
			unit.type = static_cast<CHAR_TYPE>(charType);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.dir = dir;//(dir + 5) % 8;;
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
			unit.objType = unit.type == CHAR_TYPEPLAYER ? OBJ_HUMAN : OBJ_NPC;
			mapUnitHash.insert(id, unit);

			break;
		}
		case 2://OBJTYPE_ITEM
		{
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 5, smalltoken);
			modelid = smalltoken.toLongLong();
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
			unit.objType = OBJ_ITEM;
			mapUnitHash.insert(id, unit);

			break;
		}
		case 3://OBJTYPE_GOLD
		{
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 5, smalltoken);
			money = smalltoken.toLongLong();
			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.gold = money;
			unit.isVisible = true;
			unit.objType = OBJ_GOLD;
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
			dir = (smalltoken.toLongLong() + 3) % 8;
			getStringToken(bigtoken, "|", 5, smalltoken);
			modelid = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 6, smalltoken);
			x = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 7, smalltoken);
			y = smalltoken.toLongLong();

			mapunit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.name = name;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.dir = dir;
			unit.modelid = modelid;
			unit.isVisible = modelid > 0 && modelid != 9999;
			unit.objType = OBJ_HUMAN;
			mapUnitHash.insert(id, unit);
		}

#ifdef _CHAR_PROFESSION			//人物職業
#ifdef _GM_IDENTIFY		//GM識別
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
			x = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 5, smalltoken);
			dir = (smalltoken.toLongLong() + 3) % 8;
			getStringToken(bigtoken, "|", 6, smalltoken);
			modelid = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 7, smalltoken);
			level = smalltoken.toLongLong();
			nameColor = getIntegerToken(bigtoken, "|", 8);
			getStringToken(bigtoken, "|", 9, name);
			makeStringFromEscaped(name);
			getStringToken(bigtoken, "|", 10, freeName);
			makeStringFromEscaped(freeName);
			getStringToken(bigtoken, "|", 11, smalltoken);
			walkable = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 12, smalltoken);
			height = smalltoken.toLongLong();
			charNameColor = getIntegerToken(bigtoken, "|", 13);
			getStringToken(bigtoken, "|", 14, fmname);
			makeStringFromEscaped(fmname);
			getStringToken(bigtoken, "|", 15, petname);
			makeStringFromEscaped(petname);
			getStringToken(bigtoken, "|", 16, smalltoken);
			petlevel = smalltoken.toLongLong();
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

				pc.modelid = modelid;
				pc.dir = dir;

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
						pc.status |= CHR_STATUS_PARTY;
						if (j == 0)
						{
							pc.status &= (~CHR_STATUS_LEADER);
						}
						break;
					}
				}
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
				x = smalltoken.toLongLong();
				getStringToken(bigtoken, "|", 3, smalltoken);
				y = smalltoken.toLongLong();
				getStringToken(bigtoken, "|", 4, smalltoken);
				modelid = smalltoken.toLongLong();
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
					x = smalltoken.toLongLong();
					getStringToken(bigtoken, "|", 3, smalltoken);
					y = smalltoken.toLongLong();
					getStringToken(bigtoken, "|", 4, smalltoken);
					money = smalltoken.toLongLong();

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
}

//周圍人、NPC..等等狀態改變必定是 _C_recv已經新增過的單位
void Server::lssproto_CA_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	QString bigtoken;
	QString smalltoken;
	__int64 i = 0;
	__int64 charindex = 0;
	__int64 x = 0;
	__int64 y = 0;
	__int64 act = 0;
	__int64 dir = 0;

	for (i = 0; ; ++i)
	{
		getStringToken(data, ",", i + 1, bigtoken);
		if (bigtoken.isEmpty())
			break;

		getStringToken(bigtoken, "|", 1, smalltoken);
		charindex = a62toi(smalltoken);
		getStringToken(bigtoken, "|", 2, smalltoken);
		x = smalltoken.toLongLong();
		getStringToken(bigtoken, "|", 3, smalltoken);
		y = smalltoken.toLongLong();
		getStringToken(bigtoken, "|", 4, smalltoken);
		act = smalltoken.toLongLong();
		getStringToken(bigtoken, "|", 5, smalltoken);
		dir = (smalltoken.toLongLong() + 3) % 8;
		getStringToken(bigtoken, "|", 6, smalltoken);

		mapunit_t unit = mapUnitHash.value(charindex);
		unit.id = charindex;
		unit.x = x;
		unit.y = y;
		unit.p = QPoint(x, y);
		unit.status = static_cast<CHR_STATUS> (act);
		unit.dir = dir;
		mapUnitHash.insert(charindex, unit);
	}
}

//刪除指定一個或多個周圍人、NPC單位
void Server::lssproto_CD_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	__int64 i;
	__int64 id;

	for (i = 1; ; ++i)
	{
		id = getInteger62Token(data, ",", i);
		if (id == -1)
			break;

		mapUnitHash.remove(id);
	}
}

//更新所有基礎資訊
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
void Server::lssproto_S_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	__int64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	QString first = data.left(1);
	data = data.mid(1);
	if (first.isEmpty())
		return;

	setOnlineFlag(true);

#pragma region Warp
	if (first == "C")//人物轉移
	{
		std::ignore = getFloorName();
		std::ignore = getFloor();
		std::ignore = getPoint();

		mapUnitHash.clear();
		__int64 fl, maxx, maxy, gx, gy;

		fl = getIntegerToken(data, "|", 1);
		maxx = getIntegerToken(data, "|", 2);
		maxy = getIntegerToken(data, "|", 3);
		gx = getIntegerToken(data, "|", 4);
		gy = getIntegerToken(data, "|", 5);
	}
#pragma endregion
#pragma region TimeModify
	else if (first == "D")// D 修正時間
	{
		PC pc = getPC();
		pc.id = getIntegerToken(data, "|", 1);
		setPC(pc);

		serverTime = getIntegerToken(data, "|", 2);
		FirstTime = QDateTime::currentMSecsSinceEpoch();
		realTimeToSATime(&saTimeStruct);
		saCurrentGameTime.store(getLSTime(&saTimeStruct), std::memory_order_release);
	}
#pragma endregion
#pragma region RideInfo
	else if (first == "X")// X 騎寵
	{
		QWriteLocker locker(&charInfoLock_);
		pc_.lowsride = getIntegerToken(data, "|", 2);
	}
#pragma endregion
#pragma region CharInfo
	else if (first == "P")// P 人物狀態
	{
		{
			QWriteLocker locker(&charInfoLock_);

			QString name, freeName;
			__int64 i, kubun;
			quint64 mask;

			kubun = getInteger62Token(data, "|", 1);

			if (kubun == 1)
			{
				pc_.hp = getIntegerToken(data, "|", 2);		// 0x00000002
				pc_.maxHp = getIntegerToken(data, "|", 3);		// 0x00000004
				pc_.mp = getIntegerToken(data, "|", 4);		// 0x00000008
				pc_.maxMp = getIntegerToken(data, "|", 5);		// 0x00000010

				//custom
				pc_.hpPercent = util::percent(pc_.hp, pc_.maxHp);
				pc_.mpPercent = util::percent(pc_.mp, pc_.maxMp);
				//

				pc_.vit = getIntegerToken(data, "|", 6);		// 0x00000020
				pc_.str = getIntegerToken(data, "|", 7);		// 0x00000040
				pc_.tgh = getIntegerToken(data, "|", 8);		// 0x00000080
				pc_.dex = getIntegerToken(data, "|", 9);		// 0x00000100
				pc_.exp = getIntegerToken(data, "|", 10);		// 0x00000200
				pc_.maxExp = getIntegerToken(data, "|", 11);		// 0x00000400
				pc_.level = getIntegerToken(data, "|", 12);		// 0x00000800
				pc_.atk = getIntegerToken(data, "|", 13);		// 0x00001000
				pc_.def = getIntegerToken(data, "|", 14);		// 0x00002000
				pc_.agi = getIntegerToken(data, "|", 15);		// 0x00004000
				pc_.chasma = getIntegerToken(data, "|", 16);		// 0x00008000
				pc_.luck = getIntegerToken(data, "|", 17);		// 0x00010000
				pc_.earth = getIntegerToken(data, "|", 18);		// 0x00020000
				pc_.water = getIntegerToken(data, "|", 19);		// 0x00040000
				pc_.fire = getIntegerToken(data, "|", 20);		// 0x00080000
				pc_.wind = getIntegerToken(data, "|", 21);		// 0x00100000
				pc_.gold = getIntegerToken(data, "|", 22);		// 0x00200000
				pc_.titleNo = getIntegerToken(data, "|", 23);		// 0x00400000
				pc_.dp = getIntegerToken(data, "|", 24);		// 0x00800000
				pc_.transmigration = getIntegerToken(data, "|", 25);// 0x01000000
				//pc_.ridePetNo = getIntegerToken(data, "|", 26);	// 0x02000000

				pc_.learnride = getIntegerToken(data, "|", 27);	// 0x04000000
				pc_.baseGraNo = getIntegerToken(data, "|", 28);	// 0x08000000
				pc_.lowsride = getIntegerToken(data, "|", 29);		// 0x08000000

				//pc_.sfumato = 0xff0000;

				getStringToken(data, "|", 30, name);
				makeStringFromEscaped(name);

				pc_.name = name;
				getStringToken(data, "|", 31, freeName);
				makeStringFromEscaped(freeName);

				pc_.freeName = freeName;

				//pc_.道具欄狀態 = getIntegerToken(data, "|", 32);

				__int64 pointindex = getIntegerToken(data, "|", 33);
				QStringList pontname = {
					"萨姆吉尔村",
					"玛丽娜丝村",
					"加加村",
					"卡鲁它那村",
				};
				pc_.chusheng = pontname.value(pointindex);

				//pc_.法寶道具狀態 = getIntegerToken(data, "|", 34);
				//pc_.道具光環效果 = getIntegerToken(data, "|", 35);

			}
			else
			{
				mask = 2;
				i = 2;
				for (; mask > 0; mask <<= 1)
				{
					if (!checkAND(kubun, mask))
						continue;
					switch (mask)
					{
					case 0x00000002: // ( 1 << 1 )
					{
						pc_.hp = getIntegerToken(data, "|", i);// 0x00000002
						//custom
						pc_.hpPercent = util::percent(pc_.hp, pc_.maxHp);
						++i;
						break;
					}
					case 0x00000004:// ( 1 << 2 )
					{
						pc_.maxHp = getIntegerToken(data, "|", i);// 0x00000004
						//custom
						pc_.hpPercent = util::percent(pc_.hp, pc_.maxHp);
						++i;
						break;
					}
					case 0x00000008:
					{
						pc_.mp = getIntegerToken(data, "|", i);// 0x00000008
						//custom
						pc_.mpPercent = util::percent(pc_.mp, pc_.maxMp);
						++i;
						break;
					}
					case 0x00000010:
					{
						pc_.maxMp = getIntegerToken(data, "|", i);// 0x00000010
						//custom
						pc_.mpPercent = util::percent(pc_.mp, pc_.maxMp);
						++i;
						break;
					}
					case 0x00000020:
					{
						pc_.vit = getIntegerToken(data, "|", i);// 0x00000020
						++i;
						break;
					}
					case 0x00000040:
					{
						pc_.str = getIntegerToken(data, "|", i);// 0x00000040
						++i;
						break;
					}
					case 0x00000080:
					{
						pc_.tgh = getIntegerToken(data, "|", i);// 0x00000080
						++i;
						break;
					}
					case 0x00000100:
					{
						pc_.dex = getIntegerToken(data, "|", i);// 0x00000100
						++i;
						break;
					}
					case 0x00000200:
					{
						pc_.exp = getIntegerToken(data, "|", i);// 0x00000200
						++i;
						break;
					}
					case 0x00000400:
					{
						pc_.maxExp = getIntegerToken(data, "|", i);// 0x00000400
						++i;
						break;
					}
					case 0x00000800:
					{
						pc_.level = getIntegerToken(data, "|", i);// 0x00000800
						++i;
						break;
					}
					case 0x00001000:
					{
						pc_.atk = getIntegerToken(data, "|", i);// 0x00001000
						++i;
						break;
					}
					case 0x00002000:
					{
						pc_.def = getIntegerToken(data, "|", i);// 0x00002000
						++i;
						break;
					}
					case 0x00004000:
					{
						pc_.agi = getIntegerToken(data, "|", i);// 0x00004000
						++i;
						break;
					}
					case 0x00008000:
					{
						pc_.chasma = getIntegerToken(data, "|", i);// 0x00008000
						++i;
						break;
					}
					case 0x00010000:
					{
						pc_.luck = getIntegerToken(data, "|", i);// 0x00010000
						++i;
						break;
					}
					case 0x00020000:
					{
						pc_.earth = getIntegerToken(data, "|", i);// 0x00020000
						++i;
						break;
					}
					case 0x00040000:
					{
						pc_.water = getIntegerToken(data, "|", i);// 0x00040000
						++i;
						break;
					}
					case 0x00080000:
					{
						pc_.fire = getIntegerToken(data, "|", i);// 0x00080000
						++i;
						break;
					}
					case 0x00100000:
					{
						pc_.wind = getIntegerToken(data, "|", i);// 0x00100000
						++i;
						break;
					}
					case 0x00200000:
					{
						pc_.gold = getIntegerToken(data, "|", i);// 0x00200000
						++i;
						break;
					}
					case 0x00400000:
					{
						pc_.titleNo = getIntegerToken(data, "|", i);// 0x00400000
						++i;
						break;
					}
					case 0x00800000:
					{
						pc_.dp = getIntegerToken(data, "|", i);// 0x00800000
						++i;
						break;
					}
					case 0x01000000:
					{
						pc_.transmigration = getIntegerToken(data, "|", i);// 0x01000000
						++i;
						break;
					}
					case 0x02000000:
					{
						getStringToken(data, "|", i, name);// 0x01000000
						makeStringFromEscaped(name);

						pc_.name = name;
						++i;
						break;
					}
					case 0x04000000:
					{
						getStringToken(data, "|", i, freeName);// 0x02000000
						makeStringFromEscaped(freeName);

						pc_.freeName = freeName;
						++i;
						break;
					}
					case 0x08000000: // ( 1 << 27 )
					{
						//pc_.ridePetNo = getIntegerToken(data, "|", i);// 0x08000000
						++i;
						break;
					}
					case 0x10000000: // ( 1 << 28 )
					{
						pc_.learnride = getIntegerToken(data, "|", i);// 0x10000000
						++i;
						break;
					}
					case 0x20000000: // ( 1 << 29 )
					{
						pc_.baseGraNo = getIntegerToken(data, "|", i);// 0x20000000
						++i;
						break;
					}
					case 0x40000000: // ( 1 << 30 )
					{
						pc_.skywalker = getIntegerToken(data, "|", i);// 0x40000000
						++i;
						break;
					}
					case 0x80000000: // ( 1 << 31 )
					{
						//pc_.簽到標記 = getIntegerToken(data, "|", i);// 0x80000000
						++i;
						break;
					}
					default:
						break;
					}
				}
			}

			emit signalDispatcher.updateCharHpProgressValue(pc_.level, pc_.hp, pc_.maxHp);
			emit signalDispatcher.updateCharMpProgressValue(pc_.level, pc_.mp, pc_.maxMp);

			QHash<__int64, PET> pets = pet_.toHash();
			if (pc_.ridePetNo != -1)
				emit signalDispatcher.updateRideHpProgressValue(pets.value(pc_.ridePetNo).level, pets.value(pc_.ridePetNo).hp, pets.value(pc_.ridePetNo).maxHp);
			if (pc_.battlePetNo != -1)
				emit signalDispatcher.updatePetHpProgressValue(pets.value(pc_.battlePetNo).level, pets.value(pc_.battlePetNo).hp, pets.value(pc_.battlePetNo).maxHp);

			emit signalDispatcher.updateCharInfoStone(pc_.gold);
			qreal power = (((static_cast<double>(pc_.atk + pc_.def + pc_.agi) + (static_cast<double>(pc_.maxHp) / 4.0)) / static_cast<double>(pc_.level)) * 100.0);
			qreal growth = (static_cast<double>(pc_.atk + pc_.def + pc_.agi - 20) / static_cast<double>(pc_.level - 1));
			const QVariantList varList = {
				pc_.name, pc_.freeName,
				QString(QObject::tr("%1(%2tr)").arg(pc_.level).arg(pc_.transmigration) + QObject::tr("L:%1").arg(pc_.luck)),
				pc_.exp, pc_.maxExp,
				pc_.exp > pc_.maxExp ? 0 : pc_.maxExp - pc_.exp,
				QString("%1/%2").arg(pc_.hp).arg(pc_.maxHp) ,
				QString("%1/%2").arg(pc_.mp).arg(pc_.maxMp),
				pc_.chasma, QString("%1,%2,%3").arg(pc_.atk).arg(pc_.def).arg(pc_.agi),
				toQString(growth),
				toQString(power)
			};

			QVariant var = QVariant::fromValue(varList);

			playerInfoColContents.insert(0, var);
			emit signalDispatcher.updateCharInfoColContents(0, var);
		}

		getCharMaxCarryingCapacity();
		setWindowTitle();
	}
#pragma endregion
#pragma region FamilyInfo
	else if (first == "F") // F 家族狀態
	{
		QWriteLocker locker(&charInfoLock_);
		QString family;
		getStringToken(data, "|", 1, family);
		makeStringFromEscaped(family);

		pc_.family = family;

		pc_.familyleader = getIntegerToken(data, "|", 2);
		pc_.channel = getIntegerToken(data, "|", 3);
		pc_.familySprite = getIntegerToken(data, "|", 4);
		pc_.big4fm = getIntegerToken(data, "|", 5);
	}
#pragma endregion
#pragma region CharModify
	else if (first == "M") // M HP,MP,EXP
	{
		QWriteLocker locker(&charInfoLock_);
		pc_.hp = getIntegerToken(data, "|", 1);
		pc_.mp = getIntegerToken(data, "|", 2);
		pc_.exp = getIntegerToken(data, "|", 3);

		//custom
		pc_.hpPercent = util::percent(pc_.hp, pc_.maxHp);
		pc_.mpPercent = util::percent(pc_.mp, pc_.maxMp);

		emit signalDispatcher.updateCharHpProgressValue(pc_.level, pc_.hp, pc_.maxHp);
		emit signalDispatcher.updateCharMpProgressValue(pc_.level, pc_.mp, pc_.maxMp);
	}
#pragma endregion
#pragma region PetInfo
	else if (first == "K") // K 寵物狀態
	{
		QWriteLocker locker(&petInfoLock_);

		QString name, freeName;
		__int64 no, kubun, i;
		quint64 mask;

		no = data.left(1).toUInt();
		data = data.mid(2);
		if (data.isEmpty())
			return;

		if (no < 0 || no >= MAX_PET)
			return;

		kubun = getInteger62Token(data, "|", 1);
		if (kubun == 0)
		{
			pet_.remove(no);
		}
		else
		{
			PET pet = pet_.value(no);
			if (kubun == 1)
			{
				pet.valid = true;
				pet.modelid = getIntegerToken(data, "|", 2);		// 0x00000002
				pet.hp = getIntegerToken(data, "|", 3);		// 0x00000004
				pet.maxHp = getIntegerToken(data, "|", 4);		// 0x00000008
				pet.mp = getIntegerToken(data, "|", 5);		// 0x00000010
				pet.maxMp = getIntegerToken(data, "|", 6);		// 0x00000020

				//custom
				pet.hpPercent = util::percent(pet.hp, pet.maxHp);
				pet.mpPercent = util::percent(pet.mp, pet.maxMp);

				pet.exp = getIntegerToken(data, "|", 7);		// 0x00000040
				pet.maxExp = getIntegerToken(data, "|", 8);		// 0x00000080
				pet.level = getIntegerToken(data, "|", 9);		// 0x00000100
				pet.atk = getIntegerToken(data, "|", 10);		// 0x00000200
				pet.def = getIntegerToken(data, "|", 11);		// 0x00000400
				pet.agi = getIntegerToken(data, "|", 12);		// 0x00000800
				pet.loyal = getIntegerToken(data, "|", 13);		// 0x00001000
				pet.earth = getIntegerToken(data, "|", 14);		// 0x00002000
				pet.water = getIntegerToken(data, "|", 15);		// 0x00004000
				pet.fire = getIntegerToken(data, "|", 16);		// 0x00008000
				pet.wind = getIntegerToken(data, "|", 17);		// 0x00010000
				pet.maxSkill = getIntegerToken(data, "|", 18);		// 0x00020000
				pet.changeNameFlag = getIntegerToken(data, "|", 19);// 0x00040000
				pet.transmigration = getIntegerToken(data, "|", 20);
#ifdef _SHOW_FUSION
				pet.fusion = getIntegerToken(data, "|", 21);
				getStringToken(data, "|", 22, name);// 0x00080000
				makeStringFromEscaped(name);

				pet.name = name;
				getStringToken(data, "|", 23, freeName);// 0x00100000
				makeStringFromEscaped(freeName);

				pet.freeName = freeName;
#else
				getStringToken(data, "|", 21, name);// 0x00080000
				makeStringFromEscaped(name);
				pet.name = name;

				getStringToken(data, "|", 22, freeName);// 0x00100000
				makeStringFromEscaped(freeName);
				pet.freeName = freeName;
#endif

				pet.oldhp = getIntegerToken(data, "|", 25);
				if (pet.oldhp == -1)
					pet.oldhp = 0;
				pet.oldatk = getIntegerToken(data, "|", 26);
				if (pet.oldatk == -1)
					pet.oldatk = 0;
				pet.olddef = getIntegerToken(data, "|", 27);
				if (pet.olddef == -1)
					pet.olddef = 0;
				pet.oldagi = getIntegerToken(data, "|", 28);
				if (pet.oldagi == -1)
					pet.oldagi = 0;
				pet.oldlevel = getIntegerToken(data, "|", 29);
				if (pet.oldlevel == -1)
					pet.oldlevel = 1;

				pet.rideflg = getIntegerToken(data, "|", 30);

				pet.blessflg = getIntegerToken(data, "|", 31);
				pet.blesshp = getIntegerToken(data, "|", 32);
				pet.blessatk = getIntegerToken(data, "|", 33);
				pet.blessdef = getIntegerToken(data, "|", 34);
				pet.blessquick = getIntegerToken(data, "|", 35);
			}
			else
			{
				mask = 2;
				i = 2;
				for (; mask > 0; mask <<= 1)
				{
					if (!checkAND(kubun, mask))
						continue;

					if (mask == 0x00000002)
					{
						pet.modelid = getIntegerToken(data, "|", i);// 0x00000002
						++i;
					}
					else if (mask == 0x00000004)
					{
						pet.hp = getIntegerToken(data, "|", i);// 0x00000004
						pet.hpPercent = util::percent(pet.hp, pet.maxHp);
						++i;
					}
					else if (mask == 0x00000008)
					{
						pet.maxHp = getIntegerToken(data, "|", i);// 0x00000008
						pet.hpPercent = util::percent(pet.hp, pet.maxHp);
						++i;
					}
					else if (mask == 0x00000010)
					{
						pet.mp = getIntegerToken(data, "|", i);// 0x00000010
						pet.mpPercent = util::percent(pet.mp, pet.maxMp);
						++i;
					}
					else if (mask == 0x00000020)
					{
						pet.maxMp = getIntegerToken(data, "|", i);// 0x00000020
						pet.mpPercent = util::percent(pet.mp, pet.maxMp);
						++i;
					}
					else if (mask == 0x00000040)
					{
						pet.exp = getIntegerToken(data, "|", i);// 0x00000040
						++i;
					}
					else if (mask == 0x00000080)
					{
						pet.maxExp = getIntegerToken(data, "|", i);// 0x00000080
						++i;
					}
					else if (mask == 0x00000100)
					{
						pet.level = getIntegerToken(data, "|", i);// 0x00000100
						++i;
					}
					else if (mask == 0x00000200)
					{
						pet.atk = getIntegerToken(data, "|", i);// 0x00000200
						++i;
					}
					else if (mask == 0x00000400)
					{
						pet.def = getIntegerToken(data, "|", i);// 0x00000400
						++i;
					}
					else if (mask == 0x00000800)
					{
						pet.agi = getIntegerToken(data, "|", i);// 0x00000800
						++i;
					}
					else if (mask == 0x00001000)
					{
						pet.loyal = getIntegerToken(data, "|", i);// 0x00001000
						++i;
					}
					else if (mask == 0x00002000)
					{
						pet.earth = getIntegerToken(data, "|", i);// 0x00002000
						++i;
					}
					else if (mask == 0x00004000)
					{
						pet.water = getIntegerToken(data, "|", i);// 0x00004000
						++i;
					}
					else if (mask == 0x00008000)
					{
						pet.fire = getIntegerToken(data, "|", i);// 0x00008000
						++i;
					}
					else if (mask == 0x00010000)
					{
						pet.wind = getIntegerToken(data, "|", i);// 0x00010000
						++i;
					}
					else if (mask == 0x00020000)
					{
						pet.maxSkill = getIntegerToken(data, "|", i);// 0x00020000
						++i;
					}
					else if (mask == 0x00040000)
					{
						pet.changeNameFlag = getIntegerToken(data, "|", i);// 0x00040000
						++i;
					}
					else if (mask == 0x00080000)
					{
						getStringToken(data, "|", i, name);// 0x00080000
						makeStringFromEscaped(name);

						pet.name = name;
						++i;
					}
					else if (mask == 0x00100000)
					{
						getStringToken(data, "|", i, freeName);// 0x00100000
						makeStringFromEscaped(freeName);

						pet.freeName = freeName;
						++i;
					}
#if 0
					else if (mask == 0x00200000)
					{
						pet.oldhp = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x00400000)
					{
						pet.oldatk = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x00800000)
					{
						pet.olddef = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x01000000)
					{
						pet.oldagi = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x02000000)
					{
						pet.oldlevel = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x04000000)
					{
						pet.blessflg = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x08000000)
					{
						pet.blesshp = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x10000000)
					{
						pet.blessatk = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x20000000)
					{
						pet.blessquick = getIntegerToken(data, "|", i);
						++i;
					}
					else if (mask == 0x40000000)
					{
						pet.blessdef = getIntegerToken(data, "|", i);
						++i;
					}
#endif
				}
			}


			pet.power = (((static_cast<double>(pet.atk + pet.def + pet.agi) + (static_cast<double>(pet.maxHp) / 4.0)) / static_cast<double>(pet.level)) * 100.0);
			pet.growth = (static_cast<double>(pet.atk + pet.def + pet.agi) - static_cast<double>(pet.oldatk + pet.olddef + pet.oldagi))
				/ static_cast<double>(pet.level - pet.oldlevel);

			pet_.insert(no, pet);
		}

		PC pc = pc_;
		if (pc.ridePetNo >= 0 && pc.ridePetNo < MAX_PET)
		{
			PET pet = pet_.value(pc.ridePetNo);
			emit signalDispatcher.updateRideHpProgressValue(pet.level, pet.hp, pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 0);
		}

		if (pc.battlePetNo >= 0 && pc.battlePetNo < MAX_PET)
		{
			PET pet = pet_.value(pc.battlePetNo);
			emit signalDispatcher.updatePetHpProgressValue(pet.level, pet.hp, pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 0);
		}

		for (__int64 j = 0; j < MAX_PET; ++j)
		{
			PET pet = pet_.value(j);
			QVariantList varList;
			if (pet.valid)
			{
				varList = QVariantList{
					pet.name,
					pet.freeName,
					QObject::tr("%1(%2tr)").arg(pet.level).arg(pet.transmigration),
					pet.exp, pet.maxExp, pet.maxExp - pet.exp,
					QString("%1/%2").arg(pet.hp).arg(pet.maxHp),
					"",
					pet.loyal,
					QString("%1,%2,%3").arg(pet.atk).arg(pet.def).arg(pet.agi),
					toQString(pet.growth),
					toQString(pet.power)
				};
			}
			else
			{
				for (__int64 k = 0; k < 12; ++k)
					varList.append("");
			}

			QVariant var = QVariant::fromValue(varList);
			playerInfoColContents.insert(j + 1, var);
			emit signalDispatcher.updateCharInfoColContents(j + 1, var);
		}
	}
#pragma endregion
#pragma region EncountPercentage
	else if (first == "E") // E nowEncountPercentage 不知道幹嘛的
	{
		//minEncountPercentage = getIntegerToken(data, "|", 1);
		//maxEncountPercentage = getIntegerToken(data, "|", 2);
		//nowEncountPercentage = minEncountPercentage;
	}
#pragma endregion
#pragma region MagicInfo
	else if (first == "J") //J 精靈
	{
		QWriteLocker locker(&charMagicInfoLock_);
		QString name, memo;
		__int64 no;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= MAX_MAGIC)
			return;

		if (data.isEmpty())
			return;

		MAGIC magic = magic_.value(no);
		magic.valid = getIntegerToken(data, "|", 1) > 0;
		if (magic.valid)
		{
			magic.costmp = getIntegerToken(data, "|", 2);
			magic.field = getIntegerToken(data, "|", 3);
			magic.target = getIntegerToken(data, "|", 4);
			if (magic.target >= 100)
			{
				magic.target %= 100;
				magic.deadTargetFlag = 1;
			}
			else
				magic.deadTargetFlag = 0;
			getStringToken(data, "|", 5, name);
			makeStringFromEscaped(name);

			magic.name = name;
			getStringToken(data, "|", 6, memo);
			makeStringFromEscaped(memo);

			magic.memo = memo;
		}

		magic_.insert(no, magic);
	}
#pragma endregion
#pragma region TeamInfo
	else if (first == "N")  // N 隊伍資訊
	{
		QWriteLocker locker(&teamInfoLock_);

		auto updateTeamInfo = [this, &signalDispatcher]()
		{
			QStringList teamInfoList;
			for (__int64 i = 0; i < MAX_PARTY; ++i)
			{
				PARTY party = party_.value(i);
				if (party.name.isEmpty() || (!party.valid) || (party.maxHp <= 0))
				{
					if (party_.contains(i))
						party_.remove(i);
					teamInfoList.append("");
					continue;
				}
				QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(party.name).arg(party.level)
					.arg(party.hp).arg(party.maxHp).arg(party.hpPercent);
				teamInfoList.append(text);
			}

			emit signalDispatcher.updateTeamInfo(teamInfoList);
		};

		QString name;
		__int64 no, kubun, i;
		quint64 mask;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= MAX_PARTY)
			return;

		if (data.isEmpty())
			return;

		kubun = getInteger62Token(data, "|", 1);

		if (kubun == 0)
		{
			party_.remove(no);

			if (party_.size() == 1)
				party_.clear();
			updateTeamInfo();
			return;
		}

		PARTY party = party_.value(no);

		if (kubun == 1)
		{
			party.valid = true;
			party.id = getIntegerToken(data, "|", 2);	// 0x00000002
			party.level = getIntegerToken(data, "|", 3);	// 0x00000004
			party.maxHp = getIntegerToken(data, "|", 4);	// 0x00000008
			party.hp = getIntegerToken(data, "|", 5);	// 0x00000010
			party.mp = getIntegerToken(data, "|", 6);	// 0x00000020
			getStringToken(data, "|", 7, name);	// 0x00000040
			makeStringFromEscaped(name);
			party.name = name;

			party.hpPercent = util::percent(party.hp, party.maxHp);
			party_.insert(no, party);
		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (!checkAND(kubun, mask))
					continue;

				if (mask == 0x00000002)
				{
					party.id = getIntegerToken(data, "|", i);// 0x00000002
					++i;
				}
				else if (mask == 0x00000004)
				{
					party.level = getIntegerToken(data, "|", i);// 0x00000004
					++i;
				}
				else if (mask == 0x00000008)
				{
					party.maxHp = getIntegerToken(data, "|", i);// 0x00000008
					++i;
				}
				else if (mask == 0x00000010)
				{
					party.hp = getIntegerToken(data, "|", i);// 0x00000010
					++i;
				}
				else if (mask == 0x00000020)
				{
					party.mp = getIntegerToken(data, "|", i);// 0x00000020
					++i;
				}
				else if (mask == 0x00000040)
				{
					getStringToken(data, "|", i, name);// 0x00000040
					makeStringFromEscaped(name);

					party.name = name;
					++i;
				}
			}

			party.hpPercent = util::percent(party.hp, party.maxHp);
			party_.insert(no, party);
		}

		updateTeamInfo();
	}
#pragma endregion
#pragma region ItemInfo
	else if (first == "I") //I 道具
	{
		{
			QWriteLocker locker(&itemInfoLock_);

			__int64 i, no;
			QString temp;

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
					item_.remove(i);
					continue;
				}

				ITEM item = item_.value(i);

				item.valid = true;
				item.name = temp.simplified();
				getStringToken(data, "|", no + 2, temp);
				makeStringFromEscaped(temp);

				item.name2 = temp;
				item.color = getIntegerToken(data, "|", no + 3);
				if (item.color < 0)
					item.color = 0;
				getStringToken(data, "|", no + 4, temp);
				makeStringFromEscaped(temp);

				item.memo = temp;
				item.modelid = getIntegerToken(data, "|", no + 5);
				item.field = getIntegerToken(data, "|", no + 6);
				item.target = getIntegerToken(data, "|", no + 7);
				if (item.target >= 100)
				{
					item.target %= 100;
					item.deadTargetFlag = 1;
				}
				else
					item.deadTargetFlag = 0;
				item.level = getIntegerToken(data, "|", no + 8);
				item.sendFlag = getIntegerToken(data, "|", no + 9);

				// 顯示物品耐久度
				getStringToken(data, "|", no + 10, temp);
				makeStringFromEscaped(temp);

				item.damage = temp;

				getStringToken(data, "|", no + 11, temp);
				makeStringFromEscaped(temp);

				item.stack = temp.toLongLong();
				if (item.stack == 0)
					item.stack = 1;

				getStringToken(data, "|", no + 12, temp);
				makeStringFromEscaped(temp);
				item.alch = temp;

				item.type = getIntegerToken(data, "|", no + 13);

				getStringToken(data, "|", no + 14, temp);
				makeStringFromEscaped(temp);
				item.jigsaw = temp;

				item.itemup = getIntegerToken(data, "|", no + 15);
				item.counttime = getIntegerToken(data, "|", no + 16);

				item_.insert(i, item);
			}
		}

		updateItemByMemory();
		refreshItemInfo();

		QStringList itemList;
		QHash<__int64, ITEM> items = getItems();
		for (const ITEM& it : items)
		{
			if (it.name.isEmpty())
				continue;
			itemList.append(it.name);
		}
		emit signalDispatcher.updateComboBoxItemText(kComboBoxItem, itemList);

		QStringList magicNameList;
		for (__int64 i = 0; i < MAX_MAGIC; ++i)
		{
			magicNameList.append(getMagic(i).name);
		}

		emit signalDispatcher.updateComboBoxItemText(kComboBoxCharAction, magicNameList);
		if (IS_WAITOFR_ITEM_CHANGE_PACKET.load(std::memory_order_acquire) > 0)
			IS_WAITOFR_ITEM_CHANGE_PACKET.fetch_sub(1, std::memory_order_release);
	}
#pragma endregion
#pragma region PetSkill
	else if (first == "W")//接收到的寵物技能
	{
		QWriteLocker locker(&petSkillInfoLock_);
		__int64 i, no, no2;
		QString temp;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= MAX_SKILL)
			return;

		if (data.isEmpty())
			return;


		QHash<__int64, PET_SKILL> petSkills = petSkill_.value(no);
		for (i = 0; i < MAX_SKILL; ++i)
		{
			petSkills.remove(i);
		}

		QStringList skillNameList;
		for (i = 0; i < MAX_SKILL; ++i)
		{
			no2 = i * 5;
			getStringToken(data, "|", no2 + 4, temp);
			makeStringFromEscaped(temp);

			if (temp.isEmpty())
				continue;
			petSkills[i].valid = true;
			petSkills[i].name = temp;
			petSkills[i].skillId = getIntegerToken(data, "|", no2 + 1);
			petSkills[i].field = getIntegerToken(data, "|", no2 + 2);
			petSkills[i].target = getIntegerToken(data, "|", no2 + 3);
			getStringToken(data, "|", no2 + 5, temp);
			makeStringFromEscaped(temp);

			petSkills[i].memo = temp;

			PC pc = getPC();
			if ((pc.battlePetNo >= 0) && pc.battlePetNo < MAX_PET)
				skillNameList.append(petSkills.value(i).name);
		}

		petSkill_.insert(no, petSkills);
	}
#pragma endregion
#pragma region CharSkill
	// 人物職業
	else if (first == "S") // S 職業技能
	{
		QWriteLocker locker(&charSkillInfoLock_);

		QString name;
		QString memo;
		__int64 i, count = 0;

		QHash <__int64, PROFESSION_SKILL> profession_skill = profession_skill_.toHash();

		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			profession_skill.remove(i);
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

		profession_skill_ = profession_skill;
	}
	else if (first == "G")
	{
		QWriteLocker locker(&charSkillInfoLock_);
		__int64 i, count = 0;
		QHash <__int64, PROFESSION_SKILL> profession_skill = profession_skill_.toHash();
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
			profession_skill[i].cooltime = 0;
		for (i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			count = i * 1;
			profession_skill[i].cooltime = getIntegerToken(data, "|", 1 + count);
		}

		profession_skill_ = profession_skill;
	}
#pragma endregion
#pragma region PetEquip
	else if (first == "B") // B 寵物道具
	{
		QWriteLocker locker(&petEquipInfoLock_);

		__int64 i, no, nPetIndex;
		QString szData;

		nPetIndex = data.left(1).toUInt();
		data = data.mid(2);

		if (nPetIndex < 0 || nPetIndex >= MAX_PET)
			return;

		if (data.isEmpty())
			return;

		QHash<__int64, ITEM> petItems = petItem_.toHash().value(nPetIndex);
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
				petItems.remove(nPetIndex);
				continue;
			}

			petItems[i].valid = true;
			petItems[i].name = szData;
			getStringToken(data, "|", no + 2, szData);
			makeStringFromEscaped(szData);

			petItems[i].name2 = szData;
			petItems[i].color = getIntegerToken(data, "|", no + 3);
			if (petItems[i].color < 0)
				petItems[i].color = 0;
			getStringToken(data, "|", no + 4, szData);
			makeStringFromEscaped(szData);

			petItems[i].memo = szData;
			petItems[i].modelid = getIntegerToken(data, "|", no + 5);
			petItems[i].field = getIntegerToken(data, "|", no + 6);
			petItems[i].target = getIntegerToken(data, "|", no + 7);
			if (petItems[i].target >= 100)
			{
				petItems[i].target %= 100;
				petItems[i].deadTargetFlag = 1;
			}
			else
				petItems[i].deadTargetFlag = 0;
			petItems[i].level = getIntegerToken(data, "|", no + 8);
			petItems[i].sendFlag = getIntegerToken(data, "|", no + 9);

			// 顯示物品耐久度
			getStringToken(data, "|", no + 10, szData);
			makeStringFromEscaped(szData);

			petItems[i].damage = szData;
			petItems[i].stack = getIntegerToken(data, "|", no + 11);
			if (petItems[i].stack == 0)
				petItems[i].stack = 1;

			getStringToken(data, "|", no + 12, szData);
			makeStringFromEscaped(szData);

			petItems[i].alch = szData;

			petItems[i].type = getIntegerToken(data, "|", no + 13);

			getStringToken(data, "|", no + 14, szData);
			makeStringFromEscaped(szData);

			petItems[i].jigsaw = szData;

			//可拿給寵物裝備的道具,就不會是拼圖了,以下就免了
			//if( i == JigsawIdx )
			//	SetJigsaw( pc.item[i].modelid, pc.item[i].jigsaw );

			petItems[i].itemup = getIntegerToken(data, "|", no + 15);

			petItems[i].counttime = getIntegerToken(data, "|", no + 16);
		}

		petItem_.insert(nPetIndex, petItems);
	}
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
		qDebug() << "unknown _S_recv type [" << first << "]:" << data;
	}

	updateComboBoxList();
}

//客戶端登入(進去選人畫面)
void Server::lssproto_ClientLogin_recv(char* cresult)
{
	QString result = toQUnicode(cresult);
	if (result.isEmpty())
		return;

	if (result.contains(OKSTR, Qt::CaseInsensitive))
	{
		//更新UI顯示
		__int64 currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusLoginSuccess);
	}
	else if (result.contains(CANCLE, Qt::CaseInsensitive))
	{

	}
}

//新增人物
void Server::lssproto_CreateNewChar_recv(char* cresult, char* cdata)
{
	QString data = toQUnicode(cdata);
	QString result = toQUnicode(cresult);

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
	QString data = toQUnicode(cdata);
	QString result = toQUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains("failed", Qt::CaseInsensitive))
	{
		//_snprintf_s(LoginErrorMessage, sizeof(LoginErrorMessage), _TRUNCATE, "%s", data);
		//PcLanded.登陸延時時間 = TimeGetTime() + 2000;
		return;
	}

	QString nm, opt;
	__int64 i;

	//netproc_sending = NETPROC_RECEIVED;
	if (!result.contains(SUCCESSFULSTR, Qt::CaseInsensitive) && !data.contains(SUCCESSFULSTR, Qt::CaseInsensitive))
	{
		//if (result.contains("OUTOFSERVICE", Qt::CaseInsensitive))
		return;
	}

	__int64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusGettingCharList);

	chartable_.clear();

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
			QStringList args = opt.split(rexOR);
			if (args.size() < 13)
				continue;
			__int64 index = args.value(0).toLongLong();
			if (index >= 0 && index < MAX_CHARACTER)
			{
				table.valid = true;
				table.name = nm;
				table.faceid = args.value(1).toLongLong();
				table.level = args.value(2).toLongLong();
				table.hp = args.value(3).toLongLong();
				table.str = args.value(4).toLongLong();
				table.def = args.value(5).toLongLong();
				table.agi = args.value(6).toLongLong();
				table.chasma = args.value(7).toLongLong();
				table.dp = args.value(8).toLongLong();
				table.attr[0] = args.value(9).toLongLong();
				if (table.attr[0] < 0 || table.attr[0] > 100)
					table.attr[0] = 0;
				table.attr[1] = args.value(10).toLongLong();
				if (table.attr[1] < 0 || table.attr[1] > 100)
					table.attr[1] = 0;
				table.attr[2] = args.value(11).toLongLong();
				if (table.attr[2] < 0 || table.attr[2] > 100)
					table.attr[2] = 0;
				table.attr[3] = args.value(12).toLongLong();
				if (table.attr[3] < 0 || table.attr[3] > 100)
					table.attr[3] = 0;

				table.pos = index;
				vec.append(table);
			}
		}
	}

	__int64 size = vec.size();
	for (i = 0; i < size; ++i)
	{
		if (i < 0 || i >= MAX_CHARACTER)
			continue;

		__int64 index = vec.value(i).pos;
		chartable_.insert(index, vec.value(i));
	}
}

//人物登出(不是每個私服都有，有些是直接切斷後跳回賬號密碼頁)
void Server::lssproto_CharLogout_recv(char* cresult, char* cdata)
{
	QString data = toQUnicode(cdata);
	QString result = toQUnicode(cresult);
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
	QString data = toQUnicode(cdata);
	QString result = toQUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;

	if (!result.contains(SUCCESSFULSTR, Qt::CaseInsensitive) && !data.contains(SUCCESSFULSTR, Qt::CaseInsensitive))
		return;

	std::ignore = QtConcurrent::run([this]()
		{
			QElapsedTimer timer; timer.start();
			for (;;)
			{
				if (checkWG(9, 3))
					break;

				if (timer.hasExpired(10000))
				{
					return;
				}

				QThread::msleep(100);
			}

			setOnlineFlag(true);

			__int64 currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
			emit signalDispatcher.updateStatusLabelTextChanged(kLabelStatusSignning);
			//重置登入計時
			loginTimer.restart();

			//重置戰鬥總局數
			battle_total_time.store(0, std::memory_order_release);

			//重置掛機數據
			PC pc = getPC();
			recorder[0].levelrecord = pc.level;
			recorder[0].exprecord = pc.exp;
			recorder[0].goldearn = 0;
			recorder[0].deadthcount = 0;

			for (__int64 i = 1; i <= MAX_PET; ++i)
			{
				PET pet = pet_.value(i + 1);
				recorder[i] = {};
				recorder[i].levelrecord = pet.level;
				recorder[i].exprecord = pet.exp;
				recorder[i].deadthcount = 0;
			}

			updateComboBoxList();

			//重置對話框開啟標誌(客製)
			mem::write<int>(injector.getProcess(), injector.getProcessModule() + 0x4200000, 0);

			//標題設置為人物名稱
			emit signalDispatcher.updateMainFormTitle(injector.server->getPC().name);
			//顯示NPC列表
			emit signalDispatcher.updateNpcList(injector.server->getFloor());

			emit signalDispatcher.applyHashSettingsToUI();


			//讀取伺服器列表
			QStringList list;
			{
				Config config;
				list = config.readArray<QString>("System", "Server", QString("List_%1").arg(injector.currentServerListIndex));
			}

			QStringList serverNameList;
			QStringList subServerNameList;
			for (const QString& it : list)
			{
				QStringList subList = it.split(rexOR, Qt::SkipEmptyParts);
				if (subList.isEmpty())
					continue;

				if (subList.size() != 2)
					continue;

				QString server = subList.takeFirst();

				subList = subList.first().split(rexComma, Qt::SkipEmptyParts);
				if (subList.isEmpty())
					continue;

				serverNameList.append(server);
				subServerNameList.append(subList);
			}

			injector.serverNameList = serverNameList;
			injector.subServerNameList = subServerNameList;

			setWindowTitle();

			mem::freeUnuseMemory(injector.getProcess());
		});
}

void Server::lssproto_TD_recv(char* cdata)//交易
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;

	QString Head = "";
	QString buf_sockfd = "";
	QString buf_name = "";
	QString buf = "";

	getStringToken(data, "|", 1, Head);

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

			IS_TRADING.store(true, std::memory_order_release);
			PC pc = getPC();
			pc.trade_confirm = 1;
			setPC(pc);
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
		if (!IS_TRADING.load(std::memory_order_acquire))
			return;

		QString buf_showindex;

		getStringToken(data, "|", 4, trade_kind);
		if (trade_kind.startsWith("S"))
		{
			QString buf1;
			__int64 objno = -1;//, showno = -1;

			getStringToken(data, "|", 6, buf1);
			objno = buf1.toLongLong();
			getStringToken(data, "|", 7, buf1);
			//showno = buf1.toLongLong();
			getStringToken(data, "|", 5, buf1);

			if (buf1.startsWith("I"))
			{
				//I
			}
			else
			{
				//P
				tradePetIndex = objno;
				tradePet[0].index = objno;
				PC pc = getPC();
				PET pet = getPet(objno);
				if (pet.valid && pc.ridePetNo != objno)
				{
					if (pet.freeName[0] != NULL)
						tradePet[0].name = pet.freeName;
					else
						tradePet[0].name = pet.name;
					tradePet[0].level = pet.level;
					tradePet[0].atk = pet.atk;
					tradePet[0].def = pet.def;
					tradePet[0].agi = pet.agi;
					tradePet[0].modelid = pet.modelid;

					showindex[3] = 3;
				}
			}
			return;
		}

		getStringToken(data, "|", 2, buf_sockfd);
		getStringToken(data, "|", 3, buf_name);
		getStringToken(data, "|", 4, trade_kind);
		getStringToken(data, "|", 5, buf_showindex);
		opp_showindex = buf_showindex.toLongLong();

		if (!buf_sockfd.contains(opp_sockfd) || !buf_name.contains(opp_name))
			return;

		if (trade_kind.startsWith("G"))
		{

			getStringToken(data, "|", 6, opp_goldmount);
			__int64 mount = opp_goldmount.toLongLong();


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
			getStringToken(data, "|", 12, pilenum);//pilenum
		}
	}

	if (trade_kind.startsWith("P"))
	{
		__int64 iItemNo = 0;
		QString	szData;
		__int64 index = -1;

		for (__int64 i = 0;; ++i)
		{
			if (getStringToken(data, "|", 26 + i * 6, szData))
				break;
			iItemNo = szData.toLongLong();
			if (index < 7)
			{
				getStringToken(data, "|", 27 + i * 6, opp_pet[index].oPetItemInfo[iItemNo].name);
				getStringToken(data, "|", 28 + i * 6, opp_pet[index].oPetItemInfo[iItemNo].memo);
				getStringToken(data, "|", 29 + i * 6, opp_pet[index].oPetItemInfo[iItemNo].damage);
				getStringToken(data, "|", 30 + i * 6, szData);
				opp_pet[index].oPetItemInfo[iItemNo].color = szData.toLongLong();
				getStringToken(data, "|", 31 + i * 6, szData);
				opp_pet[index].oPetItemInfo[iItemNo].bmpNo = szData.toLongLong();
			}
		}
	}

	if (trade_kind.startsWith("C"))
	{
		PC pc = getPC();
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
		IS_TRADING.store(false, std::memory_order_release);
		myitem_tradeList.clear();
		mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
		mygoldtrade = 0;
	}

	else if (Head.startsWith("W"))
	{//取消交易
		IS_TRADING.store(false, std::memory_order_release);
		myitem_tradeList.clear();
		mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };
		mygoldtrade = 0;
	}
}

void Server::lssproto_CHAREFFECT_recv(char* cdata)
{
	QString data = toQUnicode(cdata);
	if (data.isEmpty())
		return;
}

//自訂對話框收到按鈕消息
void Server::lssproto_CustomWN_recv(const QString& data)
{
	QStringList dataList = data.split(rexOR, Qt::SkipEmptyParts);
	if (dataList.size() != 4 && dataList.size() != 3)
		return;

	__int64 x = dataList.value(0).toLongLong();
	__int64 y = dataList.value(1).toLongLong();
	BUTTON_TYPE button = static_cast<BUTTON_TYPE>(dataList.value(2).toLongLong());
	QString dataStr = "";
	if (dataList.size() == 4)
		dataStr = dataList.value(3);
	__int64 row = -1;
	bool ok = false;
	__int64 tmp = dataStr.toLongLong(&ok);
	if (ok && tmp > 0)
	{
		row = dataStr.toLongLong();
	}

	customdialog_t _customDialog;
	_customDialog.x = x;
	_customDialog.y = y;
	_customDialog.button = button;
	_customDialog.row = row;
	_customDialog.rawbutton = dataList.value(2).toLongLong();
	customDialog = _customDialog;
	IS_WAITFOR_CUSTOM_DIALOG_FLAG.store(false, std::memory_order_release);
}

//自訂對話
void Server::lssproto_CustomTK_recv(const QString& data)
{
	QStringList dataList = data.split(rexOR, Qt::SkipEmptyParts);
	if (dataList.size() != 5)
		return;

	//__int64 x = dataList.value(0).toLongLong();
	//__int64 y = dataList.value(1).toLongLong();
	//__int64 color = dataList.value(2).toLongLong();
	//__int64 area = dataList.value(3).toLongLong();
	QString dataStr = dataList.value(4).simplified();
	QStringList args = dataStr.split(" ", Qt::SkipEmptyParts);
	if (args.isEmpty())
		return;
	__int64 size = args.size();
	if (args.value(0).startsWith("//skup") && size == 5)
	{
		bool ok;
		__int64 vit = args.value(1).toLongLong(&ok);
		if (!ok)
			return;
		__int64 str = args.value(2).toLongLong(&ok);
		if (!ok)
			return;
		__int64 tgh = args.value(3).toLongLong(&ok);
		if (!ok)
			return;
		__int64 dex = args.value(4).toLongLong(&ok);
		if (!ok)
			return;

		if (skupFuture.isRunning())
			return;

		skupFuture = QtConcurrent::run([this, vit, str, tgh, dex]()
			{
				const QVector<__int64> vec = { vit, str, tgh, dex };
				__int64 j = 0;
				for (__int64 i = 0; i < 4; ++i)
				{
					j = vec.value(i);
					if (j <= 0)
						continue;

					if (!addPoint(i, j))
						return;
				}
			});
	}
}
#pragma endregion

#ifdef OCR_ENABLE
#include "webauthenticator.h"
bool Server::captchaOCR(QString* pmsg)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
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
	image.save(QString("D:/py/dddd_trainer/projects/antocode/image_set/%1_%2icon_pausedisable.svg").arg(image_count++).arg(randomHash), "PNG");
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