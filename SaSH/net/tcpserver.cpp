/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include <gamedevice.h>
#include "signaldispatcher.h"
#include "script/interpreter.h"

#pragma comment(lib, "winmm.lib")

#pragma region StringControl
// 0-9,a-z(10-35),A-Z(36-61)
long long Worker::a62toi(const QString& a) const
{
	long long ret = 0;
	long long sign = 1;
	long long size = a.length();
	for (long long i = 0; i < size; ++i)
	{
		ret *= 62;
		if ('0' <= a.at(i) && a.at(i) <= '9')
			ret += static_cast<long long>(a.at(i).unicode()) - '0';
		else if ('a' <= a.at(i) && a.at(i) <= 'z')
			ret += static_cast<long long>(a.at(i).unicode()) - 'a' + 10;
		else if ('A' <= a.at(i) && a.at(i) <= 'Z')
			ret += static_cast<long long>(a.at(i).unicode()) - 'A' + 36;
		else if (a.at(i) == '-')
			sign = -1;
		else
			return 0;
	}
	return ret * sign;
}

long long Worker::getStringToken(const QString& src, const QString& delim, long long count, QString& out) const
{
	out.clear();
	if (src.isEmpty() || delim.isEmpty() || count < 0)
	{
		return 1;
	}

	long long c = 1;
	long long i = 0;

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

	long long j = src.indexOf(delim, i);
	if (j == -1)
	{
		out = src.mid(i);
		return 0;
	}

	out = src.mid(i, j - i);
	return 0;
}

long long Worker::getIntegerToken(const QString& src, const QString& delim, long long count) const
{
	if (src.isEmpty() || delim.isEmpty() || count < 0)
	{
		return -1;
	}

	QString s;
	if (getStringToken(src, delim, count, s) == 1)
		return -1;

	bool ok = false;
	long long value = s.toLongLong(&ok);
	if (ok)
		return value;
	return -1;
}

long long Worker::getInteger62Token(const QString& src, const QString& delim, long long count) const
{
	QString s;
	getStringToken(src, delim, count, s);
	if (s.isEmpty())
		return -1;
	return a62toi(s);
}

void Worker::makeStringFromEscaped(QString& src) const
{
	src.replace("\\r\\n", "\n");
	src.replace("\\n", "\n");
	src.replace("\\c", ",");
	src.replace("\\z", "|");
	src.replace("\\y", "\\");
	src.replace("　", " ");
	src = src.simplified();
}

#if 0
// 0-9,a-z(10-35),A-Z(36-61)
long long Worker::a62toi(char* a) const
{
	long long ret = 0;
	long long fugo = 1;

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

void Worker::clearNetBuffer()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	netReadBufferArray_.clear();
	gamedevice.autil.util_Clear();
}

long long Worker::appendReadBuf(const QByteArray& data)
{
	netReadBufferArray_.append(data);
	return 0;
}

bool Worker::splitLinesFromReadBuf(QByteArrayList& lines)
{
	lines = std::move(netReadBufferArray_.split('\n')); // Split net_readbuf into lines

	if (!netReadBufferArray_.endsWith('\n'))
	{
		// The last line is incomplete, remove it from the list and keep it in net_readbuf
		long long lastIndex = static_cast<long long>(lines.size()) - 1;
		netReadBufferArray_ = std::move(lines[lastIndex]);
		lines.removeAt(lastIndex);
	}
	else
	{
		// net_readbuf does not contain any incomplete line
		netReadBufferArray_.clear();
	}

	for (QByteArray& it : lines)
	{
		// Remove '\r' from each line
		it.replace('\r', "");
	}

	return !lines.isEmpty();
}

#pragma endregion

#pragma region Net
Server::Server(QObject* parent)
	: QTcpServer(parent)
{

}

Server::~Server()
{
	qDebug() << "Server is distroyed!!";
}

//啟動TCP服務端，並監聽系統自動配發的端口
bool Server::start(QObject* parent)
{
	std::ignore = parent;
	QOperatingSystemVersion version = QOperatingSystemVersion::current();

	if (version > QOperatingSystemVersion::Windows7)
	{
		if (!this->listen(QHostAddress::AnyIPv6))
		{
			qDebug() << "ipv6 Failed to listen on socket";
			QString msg = tr("Failed to listen on IPV6 socket");
			std::wstring wstr = msg.toStdWString();
			MessageBoxW(NULL, wstr.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}
	else
	{
		if (!this->listen(QHostAddress::AnyIPv6))
		{
			qDebug() << "ipv4 Failed to listen on socket";
			QString msg = tr("Failed to listen on IPV4 socket");
			std::wstring wstr = msg.toStdWString();
			MessageBoxW(NULL, wstr.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return true;
}

void Server::clear()
{
	for (Socket* clientSocket : clientSockets_)
	{
		if (clientSocket == nullptr)
			continue;

		clientSocket->close();

		if (clientSocket->state() == QAbstractSocket::ConnectedState)
			clientSocket->waitForDisconnected();
		clientSocket->deleteLater();
	}

	clientSockets_.clear();
}

//異步接收客戶端連入通知
void Server::incomingConnection(qintptr socketDescriptor)
{
	Socket* clientSocket = q_check_ptr(new Socket(socketDescriptor, this));
	sash_assume(clientSocket != nullptr);
	if (clientSocket == nullptr)
		return;

	addPendingConnection(clientSocket);
	clientSockets_.append(clientSocket);
	connect(clientSocket, &Socket::disconnected, this, [this, clientSocket]()
		{
			clientSockets_.removeOne(clientSocket);
			//clientSocket->thread.quit();
			//clientSocket->thread.wait();
			clientSocket->deleteLater();
		});
}

Socket::Socket(qintptr socketDescriptor, QObject* parent)
	: QTcpSocket(nullptr)
{
	std::ignore = parent;
	setSocketDescriptor(socketDescriptor);
	setReadBufferSize(8191);

	setSocketOption(QAbstractSocket::LowDelayOption, 1);
	setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 0);
	setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 0);
	setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);
	connect(this, &QIODevice::readyRead, this, &Socket::onReadyRead, Qt::DirectConnection);
	//moveToThread(&thread_);
	//thread_.start();
}

void Socket::onReadyRead()
{
	QByteArray badata = readAll();
	if (badata.isEmpty())
		return;

	if (init)
	{
		GameDevice& gamedevice = GameDevice::getInstance(index_);
		if (!gamedevice.worker.isNull())
		{
			GameDevice& gamedevice = GameDevice::getInstance(index_);

			if (!gamedevice.isGameInterruptionRequested())
				std::ignore = QtConcurrent::run([this, badata]()
					{
						QMutexLocker locker(&socketLock_);
						GameDevice& gamedevice = GameDevice::getInstance(index_);
						gamedevice.worker->handleData(std::move(badata));
						QThread::yieldCurrentThread();
					});
			//gamedevice.worker->handleData(std::move(badata));
		}

		QThread::yieldCurrentThread();
		return;
	}

	QString preStr = util::toQString(badata);
	long long indexEof = preStr.indexOf("\n");
	//\n之後的移除
	preStr = preStr.left(indexEof);

	//握手
	if (preStr.startsWith("hs|"))
	{
		long long i = preStr.indexOf("|");
		QString key = preStr.mid(i + 1);
		long long index = key.toLongLong();
		if (index < 0 || index >= SASH_MAX_THREAD)
			return;

		init = true;
		index_ = index;
		GameDevice& gamedevice = GameDevice::getInstance(index);
		gamedevice.worker.reset(q_check_ptr(new Worker(index, nullptr)));
		sash_assume(gamedevice.worker != nullptr);
		connect(gamedevice.worker.get(), &Worker::write, this, &Socket::onWrite, Qt::QueuedConnection);
		qDebug() << "tcp ok";
		gamedevice.IS_TCP_CONNECTION_OK_TO_USE.on();
	}

	QThread::yieldCurrentThread();
}

//異步發送數據
void Socket::onWrite(QByteArray ba, long long size)
{
	if (state() != QAbstractSocket::UnconnectedState)
	{
		if (size > 0)
			write(ba.constData(), size);
		else
			write(ba);
		flush();
		waitForBytesWritten();
	}
}

Socket::~Socket()
{
	qDebug() << "Socket is distroyed!!";
}

Worker::Worker(long long index, QObject* parent)
	: Indexer(index)
	, Lssproto(&GameDevice::getInstance(index).autil)
	, chatQueue(sa::MAX_CHAT_HISTORY)
	, readQueue_(24)
	, mapDevice(index)
{
	std::ignore = parent;

	GameDevice& gamedevice = GameDevice::getInstance(index);
	gamedevice.autil.util_Init();
	clearNetBuffer();

	gamedevice.autil.setKey("upupupupp");
}

Worker::~Worker()
{
	qDebug() << "Worker is distroyed!!";
}

//用於清空部分數據 主要用於登出後清理數據避免數據混亂，每次登出後都應該清除大部分的基礎訊息
void Worker::clear()
{
	if (battleLua != nullptr)
	{
		sol::state& lua = battleLua->getLua();
		if (lua.lua_state() != nullptr && battleLua->isRunning())
		{
			lua_State* L = lua.lua_state();
			luaL_error(L, "");
		}
		battleLua.reset();
	}

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.autil.setKey("upupupupp");

	enemyNameListCache.reset();
	customDialog.reset();
	currentDialog.reset();

	playerInfoColContents.clear();
	itemInfoRowContents.clear();
	equipInfoRowContents.clear();
	enemyNameListCache.reset();
	timeLabelContents.reset();
	labelCharAction.reset();
	labelPetAction.reset();

	long long i = 0;
	for (i = 0; i < sa::MAX_PET + 1; ++i)
		afkRecords[i] = {};

	nowFloor_.reset();
	nowFloorName_.reset();
	nowPoint_.reset();

	currentBankPetList = QPair<long long, QVector<sa::bank_pet_t>>{};
	currentBankItemList.clear();

	character_.reset();

	team_.clear();
	item_.clear();
	petItem_.clear();
	pet_.clear();
	magic_.clear();
	addressBook_.clear();
	missionInfo_.clear();
	charListData_.clear();
	mailHistory_.clear();
	professionSkill_.clear();
	petSkill_.clear();
	battlePetDisableList_.clear();
	battleCharCurrentPos.reset();
	battleBpFlag.reset();
	battleCharEscapeFlag.off();
	battleCharCurrentMp.reset();
	battleCurrentAnimeFlag.reset();
	lastSecretChatName_.reset();

	battleCharLuaScript_.clear();
	battlePetLuaScript_.clear();
	battleCharLuaScriptPath_.clear();
	battlePetLuaScriptPath_.clear();

	mapUnitHash.clear();
	chatQueue.clear();

	serverTime_.reset();
	firstServerTime_.reset();

	IS_LOCKATTACK_ESCAPE_DISABLE_.off();
	IS_WAITFOT_SKUP_RECV_.off();

	petEnableEscapeForTemp_.off();
	tempCatchPetTargetIndex_.set(-1);

	IS_WAITFOR_missionInfo_FLAG.off();
	IS_WAITFOR_BANK_FLAG.off();
	IS_WAITFOR_DIALOG_FLAG.off();
	IS_WAITFOR_CUSTOM_DIALOG_FLAG.off();
	IS_WAITOFR_ITEM_CHANGE_PACKET.reset();
	isBattleDialogReady.off();
	isEOTTLSend.off();
	doNotChangeTitle_.off();
	lastEOTime.reset();

	battleCurrentRound.reset();
	battle_total_time.reset();
	battle_total.reset();
	battle_one_round_time.reset();

	clearNetBuffer();

	skupFuture_.cancel();
	skupFuture_.waitForFinished();
}

bool Worker::handleCustomMessage(const QByteArray& badata)
{
	QString preStr = util::toQString(badata);
	long long indexEof = preStr.indexOf("\n");
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
		isBattleDialogReady.on();
		asyncBattleAction(true);
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

	//if (preStr.startsWith("SEND|"))
	//{
	//	dispatchSendMessage(badata.mid(5));

	//	return true;
	//}

	return false;
}

void Worker::dispatchSendMessage(const QByteArray& encoded) const
{
	QHash<long long, QByteArray> slices;
	QByteArray buffer;
	long long func = 0;
	long long fieldcount = 0;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.autil.util_DecodeMessage(buffer, encoded);
	gamedevice.autil.util_SplitMessage(buffer, SEPARATOR, slices);
	if (gamedevice.autil.util_GetFunctionFromSlice(slices, &func, &fieldcount, 13) != 1)
		return;

	if (func <= 0)
		return;

	qDebug() << "SEND" << func << fieldcount;
	switch (func)
	{
	case sa::LSSPROTO_WN_SEND:
	{
		long long x = 0;
		long long y = 0;
		long long dialogid = 0;
		long long unitid = 0;
		long long select = 0;
		char data[NETDATASIZE] = {};

		if (!gamedevice.autil.util_Receive(slices, &x, &y, &dialogid, &unitid, &select, data))
			return;

		qDebug() << "LSSPROTO_WN_SEND" << "x" << x << "y" << y << "dialogid" << dialogid << "unitid" << unitid << "select" << select << "data" << util::toUnicode(data);

		break;
	}
	}
}

//異步處理數據
void Worker::handleData(QByteArray badata)
{
	if (handleCustomMessage(badata))
	{
		return;
	}

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long delay = gamedevice.getValueHash(util::UserSetting::kTcpDelayValue);
	if (delay > 0)
		QThread::msleep(delay);//avoid too fast

	appendReadBuf(badata);

	if (netReadBufferArray_.isEmpty())
	{
		return;
	}


	QString key = mem::readString(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetPersonalKey, PERSONALKEYSIZE, true, true);
	gamedevice.autil.setKey(util::toConstData(key));

	if (!splitLinesFromReadBuf(netDataArrayList_))
	{
		return;
	}

	for (QByteArray& ba : netDataArrayList_)
	{
		if (gamedevice.isGameInterruptionRequested())
			break;

		// get line from read buffer
		if (ba.isEmpty())
		{
			//qDebug() << "************************ EMPTY_BUFFER ************************";
			//數據讀完了
			gamedevice.autil.util_Clear();
			continue;
		}

		long long ret = dispatchMessage(std::move(ba));

		if (ret < 0)
		{
			qDebug() << "************************ LSSPROTO_END ************************";
			//代表此段數據已到結尾
			gamedevice.autil.util_Clear();
			break;
		}
		else if (ret == kInvalidBuffer)
		{
			qDebug() << "************************ INVALID_DATA ************************";
			continue;
		}
		else if (ret == kBufferNeedToClean)
		{
			qDebug() << "************************ CLEAR_BUFFER ************************";
			//錯誤的數據 或 需要清除緩存
			clearNetBuffer();
			break;
		}
		else if (ret == kBufferAboutToEnd)
		{
			//qDebug() << "************************ ABOUT_TO_END ************************";
			continue;
		}
		else if (ret == kContinueAppendBuffer)
		{
			qDebug() << "************************ HAS_NEXT ************************";
			continue;
		}
	}
}

//經由 handleData 調用同步解析數據
long long Worker::dispatchMessage(const QByteArray& encoded)
{
	long long func = 0;
	long long fieldcount = 0;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	netRawBufferArray_.clear();
	gamedevice.autil.util_DecodeMessage(netRawBufferArray_, encoded);
	gamedevice.autil.util_SplitMessage(netRawBufferArray_, SEPARATOR);
	if (gamedevice.autil.util_GetFunctionFromSlice(&func, &fieldcount) != 1)
		return kContinueAppendBuffer;

	if (func == sa::LSSPROTO_ERROR_RECV)
		return kInvalidBuffer;

	qDebug() << "fun" << func << "fieldcount" << fieldcount;

	switch (func)
	{
	case sa::LSSPROTO_XYD_RECV: /* 戰後刷新人物座標、方向 2 */
	{
		long long x = 0;
		long long y = 0;
		long long dir = 0;

		if (!gamedevice.autil.util_Receive(&x, &y, &dir))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_XYD_RECV 戰後導正" << "x" << x << "y" << y << "dir" << dir;
		lssproto_XYD_recv(QPoint(x, y), dir);
		break;
	}
	case sa::LSSPROTO_EV_RECV: /* 環境改變WRAP 4 */
	{
		long long dialogid = 0;//新地圖名稱顯示對話框ID
		long long result = 0;

		if (!gamedevice.autil.util_Receive(&dialogid, &result))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_EV_RECV WARP" << "dialogid" << dialogid << "result" << result;
		lssproto_EV_recv(dialogid, result);
		break;
	}
	case sa::LSSPROTO_EN_RECV: /* 開始戰鬥 7 */
	{
		long long result = 0;
		long long field = 0;

		if (!gamedevice.autil.util_Receive(&result, &field))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_EN_RECV 開始戰鬥" << "result" << result << "field" << field;
		lssproto_EN_recv(result, field);
		break;
	}
	case sa::LSSPROTO_RS_RECV: /* 戰後獎勵 12 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_RS_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_RS_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_RD_RECV: /* PK獎勵 13 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;


		qDebug() << "LSSPROTO_RD_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_RD_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_B_RECV: /* 每回合開始的戰場資訊 15 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_B_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_B_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_I_RECV: /*物品變動 22*/
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_I_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_I_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_SI_RECV: /* 道具位置交換 24 */
	{
		long long fromindex;
		long long toindex;

		if (!gamedevice.autil.util_Receive(&fromindex, &toindex))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_SI_RECV" << "fromindex" << fromindex << "toindex" << toindex;
		lssproto_SI_recv(fromindex, toindex);
		break;
	}
	case sa::LSSPROTO_MSG_RECV: /* 收到郵件 26 */
	{
		long long aindex;
		long long color;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&aindex, netDataBuffer_, &color))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_MSG_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_MSG_recv(aindex, netDataBuffer_, color);
		break;
	}
	case sa::LSSPROTO_PME_RECV: /* 寵郵飛進來28 */
	{
		long long unitid;
		long long graphicsno;
		long long x;
		long long y;
		long long dir;
		long long flg;
		long long no;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&unitid, &graphicsno, &x, &y, &dir, &flg, &no, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PME_RECV" << "unitid" << unitid << "graphicsno" << graphicsno <<
			"x" << x << "y" << y << "dir" << dir << "flg" << flg << "no" << no << "cdata" << util::toUnicode(netDataBuffer_);
		lssproto_PME_recv(unitid, graphicsno, QPoint(x, y), dir, flg, no, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_AB_RECV: /* 名片 30 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_AB_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_AB_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_ABI_RECV: /* 名片數據 31 */
	{
		long long num;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&num, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_ABI_RECV" << "num" << num << "data" << util::toUnicode(netDataBuffer_);
		lssproto_ABI_recv(num, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_TK_RECV: /* 收到對話 36 */
	{
		long long index;
		long long color;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&index, netDataBuffer_, &color))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_TK_RECV" << "index" << index << "message" << util::toUnicode(netDataBuffer_) << "color" << color;
		lssproto_TK_recv(index, netDataBuffer_, color);
		break;
	}
	case sa::LSSPROTO_MC_RECV: /* 重新繪製地圖 37 */
	{
		long long fl;
		long long x1;
		long long y1;
		long long x2;
		long long y2;
		long long tilesum;
		long long objsum;
		long long eventsum;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&fl, &x1, &y1, &x2, &y2, &tilesum, &objsum, &eventsum, netDataBuffer_))
			return kInvalidBuffer;

		//qDebug() << "LSSPROTO_MC_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 <<
			//"tilesum" << tilesum << "objsum" << objsum << "eventsum" << eventsum << "data" << util::toUnicode(netDataBuffer_);
		lssproto_MC_recv(fl, x1, y1, x2, y2, tilesum, objsum, eventsum, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_M_RECV: /* 地圖數據更新，重新寫入地圖 39 */
	{
		long long fl;
		long long x1;
		long long y1;
		long long x2;
		long long y2;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&fl, &x1, &y1, &x2, &y2, netDataBuffer_))
			return kInvalidBuffer;

		//qDebug() << "LSSPROTO_M_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 << "data" << util::toUnicode(netDataBuffer_);
		lssproto_M_recv(fl, x1, y1, x2, y2, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_C_RECV: /* 服務端發送的靜態信息，可用於顯示玩家，其它玩家，公交，寵物等信息 41 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_C_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_C_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_CA_RECV: /* 周圍人、NPC..等等狀態改變必定是 _C_recv已經新增過的單位 42 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CA_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_CA_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_CD_RECV: /* 刪除指定一個或多個周圍人、NPC單位 43 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CD_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_CD_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_R_RECV: /* 未知 44 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_R_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_R_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_S_RECV: /* 更新所有基礎資訊 46 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_S_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_S_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_D_RECV: /* 未知 47 */
	{
		long long category;
		long long dx;
		long long dy;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&category, &dx, &dy, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_D_RECV" << "category" << category << "dx" << dx << "dy" << dy << "data" << util::toUnicode(netDataBuffer_);
		lssproto_D_recv(category, dx, dy, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_FS_RECV: /* 開關切換 49 */
	{
		long long flg;

		if (!gamedevice.autil.util_Receive(&flg))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_FS_RECV" << "flg" << flg;
		lssproto_FS_recv(flg);
		break;
	}
	case sa::LSSPROTO_HL_RECV: /* 戰鬥求救 51 */
	{
		long long flg;

		if (!gamedevice.autil.util_Receive(&flg))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_HL_RECV" << "flg" << flg;
		lssproto_HL_recv(flg);
		break;
	}
	case sa::LSSPROTO_PR_RECV: /* 組隊變化 53 */
	{
		long long request;
		long long result;

		if (!gamedevice.autil.util_Receive(&request, &result))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PR_RECV" << "request" << request << "result" << result;
		lssproto_PR_recv(request, result);
		break;
	}
	case sa::LSSPROTO_KS_RECV: /* 寵物更換狀態 55 */
	{
		long long petarray;
		long long result;

		if (!gamedevice.autil.util_Receive(&petarray, &result))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_KS_RECV" << "petarray" << petarray << "result" << result;
		lssproto_KS_recv(petarray, result);
		break;
	}
	case sa::LSSPROTO_PS_RECV: /* 收到寵郵 59 */
	{
		long long result;
		long long havepetindex;
		long long havepetskill;
		long long toindex;

		if (!gamedevice.autil.util_Receive(&result, &havepetindex, &havepetskill, &toindex))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PS_RECV" << "result" << result << "havepetindex" << havepetindex << "havepetskill" << havepetskill << "toindex" << toindex;
		lssproto_PS_recv(result, havepetindex, havepetskill, toindex);
		break;
	}
	case sa::LSSPROTO_SKUP_RECV: /* 更新點數 63 */
	{
		long long point;

		if (!gamedevice.autil.util_Receive(&point))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_SKUP_RECV" << "point" << point;
		lssproto_SKUP_recv(point);
		break;
	}
	case sa::LSSPROTO_WN_RECV: /* NPC對話框 66 */
	{
		long long windowtype;
		long long buttontype;
		long long dialogid;
		long long unitid;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&windowtype, &buttontype, &dialogid, &unitid, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_WN_RECV" << "windowtype" << windowtype << "buttontype" << buttontype
			<< "dialogid" << dialogid << "unitid" << unitid << "data" << util::toUnicode(netDataBuffer_);
		lssproto_WN_recv(windowtype, buttontype, dialogid, unitid, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_EF_RECV: /* 天氣 68 */
	{
		long long effect;
		long long level;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&effect, &level, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_EF_RECV WEATHER" << "effect" << effect << "level" << level << "option" << util::toUnicode(netDataBuffer_);
		lssproto_EF_recv(effect, level, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_SE_RECV: /* 未知 69 */
	{
		long long x;
		long long y;
		long long senumber;
		long long sw;

		if (!gamedevice.autil.util_Receive(&x, &y, &senumber, &sw))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_SE_RECV" << "x" << x << "y" << y << "senumber" << senumber << "sw" << sw;
		lssproto_SE_recv(QPoint(x, y), senumber, sw);
		break;
	}
	case sa::LSSPROTO_CLIENTLOGIN_RECV: /* 選人畫面 72 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CLIENTLOGIN_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_ClientLogin_recv(netDataBuffer_);

		return kBufferNeedToClean;
	}
	case sa::LSSPROTO_CREATENEWCHAR_RECV: /* 人物新增 74 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		memset(netResultDataBuffer_, 0, SBUFSIZE);
		if (!gamedevice.autil.util_Receive(netResultDataBuffer_, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CREATENEWCHAR_RECV" << util::toUnicode(netResultDataBuffer_) << util::toUnicode(netDataBuffer_);
		lssproto_CreateNewChar_recv(netResultDataBuffer_, netDataBuffer_);
		return kBufferNeedToClean;
	}
	case sa::LSSPROTO_CHARDELETE_RECV: /* 人物刪除 76 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		memset(netResultDataBuffer_, 0, SBUFSIZE);
		if (!gamedevice.autil.util_Receive(netResultDataBuffer_, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CHARDELETE_RECV" << util::toUnicode(netResultDataBuffer_) << util::toUnicode(netDataBuffer_);
		lssproto_CharDelete_recv(netResultDataBuffer_, netDataBuffer_);
		return kBufferNeedToClean;
	}
	case sa::LSSPROTO_CHARLOGIN_RECV: /* 成功登入 78 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		memset(netResultDataBuffer_, 0, SBUFSIZE);
		if (!gamedevice.autil.util_Receive(netResultDataBuffer_, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CHARLOGIN_RECV" << util::toUnicode(netResultDataBuffer_) << util::toUnicode(netDataBuffer_);
		lssproto_CharLogin_recv(netResultDataBuffer_, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_CHARLIST_RECV: /* 選人頁面資訊 80 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		memset(netResultDataBuffer_, 0, SBUFSIZE);
		if (!gamedevice.autil.util_Receive(netResultDataBuffer_, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CHARLIST_RECV" << util::toUnicode(netResultDataBuffer_) << util::toUnicode(netDataBuffer_);
		lssproto_CharList_recv(netResultDataBuffer_, netDataBuffer_);

		return kBufferNeedToClean;
	}
	case sa::LSSPROTO_CHARLOGOUT_RECV: /* 登出 82 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		memset(netResultDataBuffer_, 0, SBUFSIZE);
		if (!gamedevice.autil.util_Receive(netResultDataBuffer_, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CHARLOGOUT_RECV" << util::toUnicode(netResultDataBuffer_) << util::toUnicode(netDataBuffer_);
		lssproto_CharLogout_recv(netResultDataBuffer_, netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_PROCGET_RECV: /* 未知 84 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PROCGET_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_ProcGet_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_PLAYERNUMGET_RECV: /* 未知 86 */
	{
		long long logincount;
		long long player;

		if (!gamedevice.autil.util_Receive(&logincount, &player))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PLAYERNUMGET_RECV" << "logincount:" << logincount << "player:" << player; //"logincount:%d player:%d\n
		lssproto_CharNumGet_recv(logincount, player);
		break;
	}
	case sa::LSSPROTO_ECHO_RECV: /* 伺服器定時ECHO "hoge" 88 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_ECHO_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_Echo_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_NU_RECV: /* 未知 90 */
	{
		long long AddCount;

		if (!gamedevice.autil.util_Receive(&AddCount))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_NU_RECV" << "AddCount:" << AddCount;
		lssproto_NU_recv(AddCount);
		break;
	}
	case sa::LSSPROTO_TD_RECV: /* 交易 92 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_TD_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_TD_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_FM_RECV: /* 家族頻道 93 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_FM_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_FM_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_WO_RECV: /* 未知 95 */
	{
		long long effect;

		if (!gamedevice.autil.util_Receive(&effect))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_WO_RECV" << "effect:" << effect;
		lssproto_WO_recv(effect);
		break;
	}
	case sa::LSSPROTO_IC_RECV:/* 未知 100 */
	{
		long long x, y;
		if (!gamedevice.autil.util_Receive(&x, &y))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_IC_RECV" << "x:" << x << "y:" << y;
		break;
	}
	case sa::LSSPROTO_NC_RECV: /*未知(大多是戰鬥結束才有) 101 */
	{
		long long flg = 0;

		if (!gamedevice.autil.util_Receive(&flg))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_NC_RECV" << "flg:" << flg;
		lssproto_NC_recv(flg);
		break;
	}
	case sa::LSSPROTO_CS_RECV: /* 固定客戶端的速度 104 */
	{
		long long deltimes = 0;

		if (!gamedevice.autil.util_Receive(&deltimes))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CS_RECV" << "deltimes:" << deltimes;
		lssproto_CS_recv(deltimes);
		break;
	}
	case sa::LSSPROTO_PETST_RECV: /* 寵物狀態改變 107 */
	{
		long long petarray;
		long long nresult;

		if (!gamedevice.autil.util_Receive(&petarray, &nresult))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PETST_RECV" << "petarray:" << petarray << "result:" << nresult;
		lssproto_PETST_recv(petarray, nresult);
		break;
	}
	case sa::LSSPROTO_SPET_RECV: /* 寵物更換狀態 115 */
	{
		long long standbypet;
		long long nresult;

		if (!gamedevice.autil.util_Receive(&standbypet, &nresult))
			return kInvalidBuffer;

		//qDebug() << "LSSPROTO_SPET_RECV" << "standbypet:" << standbypet << "result:" << result;
		lssproto_SPET_recv(standbypet, nresult);
		break;
	}
	case sa::LSSPROTO_STREET_VENDOR_RECV: /* 擺攤 117 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_STREET_VENDOR_RECV" << "data:" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_missionInfo_RECV: /* 任務日誌 120 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;


		qDebug() << "LSSPROTO_missionInfo_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_missionInfo_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_TEACHER_SYSTEM_RECV: /* 導師系統 123 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_TEACHER_SYSTEM_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_TEACHER_SYSTEM_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_S2_RECV: /* 額外基礎訊息 125 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		lssproto_S2_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_FIREWORK_RECV: /* 煙火? 126 */
	{
		long long iCharaindex, iType, iActionNum;

		if (!gamedevice.autil.util_Receive(&iCharaindex, &iType, &iActionNum))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_FIREWORK_RECV" << "iCharaindex:" << iCharaindex << "iType:" << iType << "iActionNum:" << iActionNum;
		lssproto_Firework_recv(iCharaindex, iType, iActionNum);
		break;
	}
	case sa::LSSPROTO_MOVE_SCREEN_RECV: /* 未知 128 */
	{
		int	iXY;
		int	bMoveScreenMode;
		if (!gamedevice.autil.util_Receive(&bMoveScreenMode, &iXY))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_MOVE_SCREEN_RECV" << "bMoveScreenMode:" << bMoveScreenMode << "dstX" << HIWORD(iXY) << "dstY" << LOWORD(iXY);

		break;
	}
	case sa::LSSPROTO_HOSTNAME_RECV: /* 未知 130 */
	{
		int	hostnametamp;
		if (!gamedevice.autil.util_Receive(&hostnametamp))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_HOSTNAME_RECV" << "hostnametamp:" << hostnametamp;
		break;
	}
	case sa::LSSPROTO_MAGICCARD_ACTION_RECV: /* 未知 133 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_MAGICCARD_ACTION_RECV" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_MAGICCARD_DAMAGE_RECV: /* 未知 134 */
	{
		long long position;
		long long damage;
		long long offsetx;
		long long offsety;
		if (!gamedevice.autil.util_Receive(&position, &damage, &offsetx, &offsety))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_MAGICCARD_DAMAGE_RECV" << "position:" << position << "damage:" << damage << "offsetx:" << offsetx << "offsety:" << offsety;
		break;
	}
	case sa::LSSPROTO_ALCHEPLUS_RECV: /* 未知 136 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_ALCHEPLUS_RECV" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_SECONDARY_WINDOW_RECV: /* 二级窗口内容 137 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_SECONDARY_WINDOW_RECV" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_TRUNTABLE_RECV: /* 轉盤 簽到 138 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_TRUNTABLE_RECV" << util::toUnicode(netDataBuffer_);

		break;
	}
	case sa::LSSPROTO_PKLIST_RECV: /* 未知 140 */
	{
		long long count;
		memset(netDataBuffer_, 0, NETDATASIZE);

		if (!gamedevice.autil.util_Receive(&count, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PKLIST_RECV" << "count:" << count << util::toUnicode(netDataBuffer_);

		break;
	}
	case sa::LSSPROTO_CHAREFFECT_RECV: /* 未知 146 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CHAREFFECT_RECV" << util::toUnicode(netDataBuffer_);
		lssproto_CHAREFFECT_recv(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_REDMEMOY_RECV: /* 未知 148 */
	{
		long long type;
		long long time;
		long long vip;
		long long index;

		memset(netDataBuffer_, 0, NETDATASIZE);

		if (!gamedevice.autil.util_Receive(&type, &time, &vip, netDataBuffer_, &index))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_REDMEMOY_RECV" << "type:" << type << "time:" << time << "vip:" << vip << "index:" << index << util::toUnicode(netDataBuffer_);

		break;
	}
	case sa::LSSPROTO_IMAGE_RECV: /* 驗證圖 151 */
	{
		long long x = 0;
		long long y = 0;
		long long z = 0;
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_, &x, &y, &z))
			return kInvalidBuffer;

		QString text = util::toUnicode(netDataBuffer_);

		util::ScopedFile file("d:/data.txt");
		if (file.openWriteAppend())
		{
			file.write(text.toUtf8().data());
			file.write("\n");
		}

		break;
	}
	case sa::LSSPROTO_DENGON_RECV:/* 特殊公告 200*/
	{
		long long coloer;
		long long num;

		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_, &coloer, &num))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_DENGON_RECV" << util::toUnicode(netDataBuffer_) << "coloer:" << coloer << "num:" << num;
		lssproto_DENGON_recv(netDataBuffer_, coloer, num);
		break;
	}
	case sa::LSSPROTO_SAMENU_RECV: /* ShellExecute 'open' 201*/
	{
		long long count;
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&count, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_SAMENU_RECV" << "count:" << count << util::toUnicode(netDataBuffer_);
		break;
	}
	case 206:
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_206_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_SHOPOK_RECV: /* 伺服器控制菜單 209 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_SHOPOK_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_FAMILYBADGE_RECV: /* 家族徽章相關數據 211 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_FAMILYBADGE_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);

		break;
	}
	case sa::LSSPROTO_CHARTITLE_RECV: /* 人物稱號 213 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_CHARTITLE_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);

		break;
	}
	case 216:
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_216_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_VB_RECV: /* 祝福窗口 219 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_VB_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);
		break;
	}
	case 220:
	{
		long long unknown;
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(&unknown, netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_220_RECV" << "unknown:" << unknown << util::toUnicode(netDataBuffer_);
		break;
	}
	case sa::LSSPROTO_PETSKINS_RECV: /* 寵物皮膚 222 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_PETSKINS_RECV" << "unknown:" << util::toUnicode(netDataBuffer_);

		break;
	}
	case 300: /* 丟棄道具後才會出現 */
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_300_RECV" << util::toUnicode(netDataBuffer_);

		//char int
		break;
	}
	case 301:
	{
		memset(netDataBuffer_, 0, NETDATASIZE);
		if (!gamedevice.autil.util_Receive(netDataBuffer_))
			return kInvalidBuffer;

		qDebug() << "LSSPROTO_301_RECV" << util::toUnicode(netDataBuffer_);

		break;
	}
	default:
	{
		qDebug() << "-------------------UNKNOWN fun" << func << "fieldcount" << fieldcount;
		break;
	}
	}

	gamedevice.autil.util_DiscardMessage();
	QThread::yieldCurrentThread();
	return kBufferAboutToEnd;
}
#pragma endregion

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region GET
//檢查是否在戰鬥中
bool Worker::getBattleFlag()
{
	return !(!isBattle_.get() && getWorldStatus() != 10);
}

//檢查是否在線
bool Worker::getOnlineFlag()
{
	long long W = getWorldStatus();
	long long G = getGameStatus();

	if (11 == W && 2 == G)
	{
		setOnlineFlag(false);
		return false;
	}
	return isOnline_.get();
}

//用於判斷畫面的狀態的數值 (9平時 10戰鬥 <8非登入)
long long Worker::getWorldStatus() const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return static_cast<long long>(mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetWorldStatus));
}

//用於判斷畫面或動畫狀態的數值 (平時一般是3 戰鬥中選擇面板是4 戰鬥動作中是5或6，平時還有很多其他狀態值)
long long Worker::getGameStatus() const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return static_cast<long long>(mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetGameStatus));
}

//檢查W G 值是否匹配指定值
bool Worker::checkWG(long long w, long long g) const
{
	return getWorldStatus() == w && getGameStatus() == g;
}

//檢查非登入時所在頁面
long long Worker::getUnloginStatus()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	long long W = getWorldStatus();
	long long G = getGameStatus();

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
		return util::kNoUserNameOrPassword;//無賬號密碼
	}
	else if ((1 == W && 2 == G) || (1 == W && 3 == G))
	{
		setOnlineFlag(false);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLogining);
		return util::kStatusInputUser;//輸入賬號密碼
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
	{
		if (!getOnlineFlag())
		{
			setOnlineFlag(true);
		}
		return util::kStatusLogined;//已豋入(平時且無其他對話框或特殊場景)
	}

	if (10 == W)
	{
		if (!getOnlineFlag())
		{
			setOnlineFlag(true);
		}
	}

	qDebug() << "getUnloginStatus: " << W << " " << G;
	return util::kStatusUnknown;
}

//計算人物最單物品大堆疊數(負重量)
void Worker::getCharMaxCarryingCapacity()
{
	sa::character_t pc = getCharacter();
	long long nowMaxload = pc.maxload;
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
	sa::item_t item = item_.value(sa::CHAR_EQBELT);
	if (!item.name.isEmpty())
	{
		//負重|负重
		static const QRegularExpression re("負重|负重");
		long long index = item.memo.indexOf(re);
		if (index != -1)
		{

			QString buf = item.memo.mid(index + 3);
			bool ok = false;
			long long value = buf.toLongLong(&ok);
			if (ok && value > 0)
				pc.maxload += value;
		}
	}

	if (pc.maxload < nowMaxload)
		pc.maxload = nowMaxload;

	setCharacter(pc);
}

//取當前隊伍人數
long long Worker::getTeamSize()
{
	updateDatasFromMemory();

	sa::character_t pc = getCharacter();
	long long count = 0;

	if (util::checkAND(pc.status, sa::kCharacterStatus_IsLeader) || util::checkAND(pc.status, sa::kCharacterStatus_HasTeam))
	{
		for (long long i = 0; i < sa::MAX_TEAM; ++i)
		{
			sa::team_t team = team_.value(i);
			if (!team.valid)
				continue;
			if (team.level <= 0)
				continue;

			++count;
		}
	}
	return count;
}

//取對話紀錄
QString Worker::getChatHistory(long long index) const
{
	if (index < 0 || index >= sa::MAX_CHAT_HISTORY)
		return "\0";

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();

	long long total = static_cast<long long>(mem::read<int>(hProcess, hModule + sa::kOffsetChatBufferMaxCount));
	if (index > total)
		return "\0";

	long long ptr = hModule + sa::kOffsetChatBuffer + ((total - index) * sa::kChatBufferSize);
	//long long maxptr = static_cast<long long>(mem::read<int>(hProcess, hModule + kOffsetChatBufferMaxPointer));
	//if (ptr > maxptr)
		//return "\0";

	QString str = mem::readString(hProcess, ptr, sa::kChatBufferSize, true);
	if (str.isEmpty())
	{
		return chatQueue.values().value(index).second;
	}

	return str;
}

//獲取周圍玩家名稱列表
QStringList Worker::getJoinableUnitList() const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QString leader = gamedevice.getStringHash(util::kAutoFunNameString).simplified();
	QStringList unitNameList;
	if (!leader.isEmpty())
		unitNameList.append(leader);

	sa::character_t pc = getCharacter();
	for (const sa::map_unit_t& unit : mapUnitHash)
	{
		QString newNpcName = unit.name.simplified();
		if (newNpcName.isEmpty() || (unit.objType != sa::kObjectHuman))
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
bool Worker::getItemIndexsByName(const QString& name, const QString& memo, QVector<long long>* pv, long long from, long long to, QVector<long long>* pindexs)
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

	QVector<long long> v;
	QHash<long long, sa::item_t> items = getItems();
	for (long long i = from; i < to; ++i)
	{
		if (pindexs != nullptr && !pindexs->isEmpty())
		{
			if (!pindexs->contains(i))
				continue;
		}

		QString itemName = items.value(i).name.simplified();
		QString itemMemo = items.value(i).memo.simplified();

		if (itemName.isEmpty() || !items.value(i).valid)
			continue;

		//說明為空
		if (memo.isEmpty())
		{
			if (isExact && (!newName.isEmpty() && (newName == itemName)))
				v.append(i);
			else if (!isExact && (!newName.isEmpty() && itemName.contains(newName)))
				v.append(i);
		}
		//道具名稱為空
		else if (name.isEmpty())
		{
			if (itemMemo.contains(newMemo) && !newMemo.isEmpty())
				v.append(i);
		}
		//兩者都不為空
		else if (!itemName.isEmpty() && !itemMemo.isEmpty())
		{
			if (isExact && (!newName.isEmpty() && newName == itemName) && (!newMemo.isEmpty() && itemMemo.contains(newMemo)))
				v.append(i);
			else if (!isExact && (!newName.isEmpty() && itemName.contains(newName)) && (!newMemo.isEmpty() && itemMemo.contains(newMemo)))
				v.append(i);
		}
	}

	if (pv)
		*pv = v;

	return !v.isEmpty();
}

//根據道具名稱(或包含說明文)獲取模糊或精確匹配道具索引
long long Worker::getItemIndexByName(const QString& name, bool isExact, const QString& memo, long long from, long long to)
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

	QHash<long long, sa::item_t> items = getItems();
	for (long long i = from; i < to; ++i)
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
long long Worker::getPetSkillIndexByName(long long& petIndex, const QString& name) const
{
	QString newName = name.simplified();
	long long i = 0;
	if (petIndex == -1)
	{

		for (long long j = 0; j < sa::MAX_PET; ++j)
		{
			QHash<long long, sa::pet_skill_t> petSkill = getPetSkills(j);
			for (i = 0; i < sa::MAX_PET_SKILL; ++i)
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
		QHash<long long, sa::pet_skill_t> petSkill = getPetSkills(petIndex);
		for (i = 0; i < sa::MAX_PET_SKILL; ++i)
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
bool Worker::getPetIndexsByName(const QString& cmpname, QVector<long long>* pv) const
{
	QVector<long long> v;
	QStringList nameList = cmpname.simplified().split(util::rexOR, Qt::SkipEmptyParts);

	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		if (v.contains(i))
			continue;

		sa::pet_t pet = getPet(i);
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
long long Worker::getItemEmptySpotIndex()
{
	updateItemByMemory();

	QHash<long long, sa::item_t> items = getItems();
	for (long long i = sa::CHAR_EQUIPSLOT_COUNT; i < sa::MAX_ITEM; ++i)
	{
		QString name = items.value(i).name.simplified();
		if (name.isEmpty() || !items.value(i).valid)
			return i;
	}

	return -1;
}

//取背包空格索引列表
bool Worker::getItemEmptySpotIndexs(QVector<long long>* pv)
{
	updateItemByMemory();

	QVector<long long> v;
	QHash<long long, sa::item_t> items = getItems();
	for (long long i = sa::CHAR_EQUIPSLOT_COUNT; i < sa::MAX_ITEM; ++i)
	{
		QString name = items.value(i).name.simplified();
		if (name.isEmpty() || !items.value(i).valid)
			v.append(i);
	}
	if (pv)
		*pv = v;

	return !v.isEmpty();
}

//戰鬥狀態標誌位轉狀態字符串
QString Worker::getBadStatusString(long long status)
{
	QStringList temp;
	if (util::checkAND(status, sa::BC_FLG_DEAD))
		temp.append(QObject::tr("dead")); // 死亡
	if (util::checkAND(status, sa::BC_FLG_POISON))
		temp.append(QObject::tr("poisoned")); // 中毒
	if (util::checkAND(status, sa::BC_FLG_PARALYSIS))
		temp.append(QObject::tr("paralyzed")); // 麻痹
	if (util::checkAND(status, sa::BC_FLG_SLEEP))
		temp.append(QObject::tr("sleep")); // 昏睡
	if (util::checkAND(status, sa::BC_FLG_STONE))
		temp.append(QObject::tr("petrified")); // 石化
	if (util::checkAND(status, sa::BC_FLG_DRUNK))
		temp.append(QObject::tr("drunk")); // 酒醉
	if (util::checkAND(status, sa::BC_FLG_CONFUSION))
		temp.append(QObject::tr("confused")); // 混亂
	if (util::checkAND(status, sa::BC_FLG_HIDE))
		temp.append(QObject::tr("hidden")); // 是否隱藏，地球一周
	if (util::checkAND(status, sa::BC_FLG_REVERSE))
		temp.append(QObject::tr("reverse")); // 反轉
	if (util::checkAND(status, sa::BC_FLG_WEAKEN))
		temp.append(QObject::tr("weaken")); // 虛弱
	if (util::checkAND(status, sa::BC_FLG_DEEPPOISON))

		temp.append(QObject::tr("deep poison")); // 劇毒
	if (util::checkAND(status, sa::BC_FLG_BARRIER))
		temp.append(QObject::tr("barrier")); // 魔障
	if (util::checkAND(status, sa::BC_FLG_NOCAST))
		temp.append(QObject::tr("no cast")); // 沈默
	if (util::checkAND(status, sa::BC_FLG_SARS))
		temp.append(QObject::tr("sars")); // 毒煞蔓延
	if (util::checkAND(status, sa::BC_FLG_DIZZY))
		temp.append(QObject::tr("dizzy")); // 暈眩
	if (util::checkAND(status, sa::BC_FLG_ENTWINE))
		temp.append(QObject::tr("entwine")); // 樹根纏繞
	if (util::checkAND(status, sa::BC_FLG_DRAGNET))
		temp.append(QObject::tr("dragnet")); // 天羅地網
	if (util::checkAND(status, sa::BC_FLG_ICECRACK))
		temp.append(QObject::tr("ice crack")); // 冰爆術
	if (util::checkAND(status, sa::BC_FLG_OBLIVION))
		temp.append(QObject::tr("oblivion")); // 遺忘
	if (util::checkAND(status, sa::BC_FLG_ICEARROW))
		temp.append(QObject::tr("ice arrow")); // 冰箭
	if (util::checkAND(status, sa::BC_FLG_BLOODWORMS))
		temp.append(QObject::tr("blood worms")); // 嗜血蠱
	if (util::checkAND(status, sa::BC_FLG_SIGN))
		temp.append(QObject::tr("sign")); // 一針見血
	if (util::checkAND(status, sa::BC_FLG_CARY))
		temp.append(QObject::tr("cary")); // 挑撥
	if (util::checkAND(status, sa::BC_FLG_F_ENCLOSE))
		temp.append(QObject::tr("fire enclose")); // 火附體
	if (util::checkAND(status, sa::BC_FLG_I_ENCLOSE))
		temp.append(QObject::tr("ice enclose")); // 冰附體
	if (util::checkAND(status, sa::BC_FLG_T_ENCLOSE))
		temp.append(QObject::tr("thunder enclose")); // 雷附體
	if (util::checkAND(status, sa::BC_FLG_WATER))
		temp.append(QObject::tr("water enclose")); // 水附體
	if (util::checkAND(status, sa::BC_FLG_FEAR))
		temp.append(QObject::tr("fear")); // 恐懼
	if (util::checkAND(status, sa::BC_FLG_CHANGE))
		temp.append(QObject::tr("change")); // 雷爾變身

	return temp.join(" ");
}

std::string sa::battle_object_t::getStatus() const { return util::toConstData(Worker::getBadStatusString(status)); }

//戰鬥場地屬性標誌位轉場地屬性字符串
QString Worker::getFieldString(long long field)
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
		return QObject::tr("none");
	}
}

//取人物方位
long long Worker::getDir()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	HANDLE hProcess = gamedevice.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return -1;

	long long dir = (static_cast<long long>(mem::read<int>(hProcess, hModule + sa::kOffsetDir)) + 5) % sa::MAX_DIR;
	sa::character_t pc = getCharacter();
	pc.dir = dir;
	setCharacter(pc);

	QPoint point = nowPoint_.get();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2(%3)").arg(point.x()).arg(point.y()).arg(g_dirStrHash.value(dir)));

	return dir;
}

//取人物座標
QPoint Worker::getPoint()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	if (hModule == 0)
		return QPoint{};

	HANDLE hProcess = gamedevice.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return QPoint{};

	const QPoint point(mem::read<int>(hProcess, hModule + sa::kOffsetNowX), mem::read<int>(hProcess, hModule + sa::kOffsetNowY));
	nowPoint_.set(point);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2(%3)").arg(point.x()).arg(point.y()).arg(g_dirStrHash.value(getDir())));

	return point;
}

//取人物當前地圖編號
long long Worker::getFloor()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	if (hModule == 0)
		return 0;

	HANDLE hProcess = gamedevice.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return 0;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	long long floor = static_cast<long long>(mem::read<int>(hProcess, hModule + sa::kOffsetNowFloor));
	if (nowFloor_.get() != floor)
	{
		emit signalDispatcher.updateNpcList(floor);
	}
	nowFloor_.set(floor);

	QString mapname = nowFloorName_.get();
	emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(mapname).arg(floor));

	return floor;
}

//取人物當前地圖名稱
QString Worker::getFloorName()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	if (hModule == 0)
		return "";

	HANDLE hProcess = gamedevice.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return "";

	QString mapname = mem::readString(hProcess, hModule + sa::kOffsetNowFloorName, sa::FLOOR_NAME_LEN, true);
	makeStringFromEscaped(mapname);
	nowFloorName_.set(mapname);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	long long floor = nowFloor_.get();
	emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(mapname).arg(floor));


	return mapname;
}

//檢查指定任務狀態，並同步等待封包返回
long long Worker::checkJobDailyState(const QString& missionName, long long timeout)
{
	QString newMissionName = missionName.simplified();
	if (newMissionName.isEmpty())
		return 0;

	if (timeout <= 0)
		timeout = 5000;

	IS_WAITFOR_missionInfo_FLAG.on();

	if (!lssproto_missionInfo_send(const_cast<char*>("dyedye")))
	{
		IS_WAITFOR_missionInfo_FLAG.off();
		return -1;
	}

	util::timer timer;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	for (;;)
	{
		if (gamedevice.isGameInterruptionRequested())
		{
			IS_WAITFOR_missionInfo_FLAG.off();
			return -1;
		}

		if (timer.hasExpired(timeout))
		{
			IS_WAITFOR_missionInfo_FLAG.off();
			return -1;
		}

		if (!IS_WAITFOR_missionInfo_FLAG.get())
			break;

		QThread::msleep(200);
	}

	if (newMissionName.startsWith("?"))
	{
		newMissionName = newMissionName.mid(1);
	}

	QHash<long long, sa::mission_data_t> jobdaily = getMissionInfos();
	long long state = 0;
	for (const sa::mission_data_t& it : jobdaily)
	{
		if (it.name.contains(newMissionName))
		{
			state = it.state;
			//如果是完成則直接退出，如果是進行中則繼續搜索以防出現同時有兩種進度的情況
			if (state == 2)
				break;
		}
	}

	return state;
}

//查找指定類型和名稱的單位
bool Worker::findUnit(const QString& nameSrc, long long type, sa::map_unit_t* punit, const QString& freenameSrc, long long modelid)
{
	QList<sa::map_unit_t> units = mapUnitHash.values();

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
			point.setX(nameSrcList.value(0).simplified().toLongLong(&ok));
			if (!ok)
				break;

			point.setY(nameSrcList.value(1).simplified().toLongLong(&ok));
			if (!ok)
				break;

			for (const sa::map_unit_t& it : units)
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

	if (modelid != -1 && modelid != 0 && modelid != 9999)
	{
		for (const sa::map_unit_t& it : units)
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

				for (const sa::map_unit_t& it : units)
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

//取人物當前位子地圖數據塊的數據
QString Worker::getGround()
{
	return mapDevice.getGround(getIndex(), getFloor(), getFloorName(), getPoint());
}

//查找非滿血自己寵物或隊友的索引 (主要用於自動吃肉)
long long Worker::findInjuriedAllie() const
{
	sa::character_t pc = getCharacter();
	if (pc.hp < pc.maxHp)
		return 0;

	long long i = 0;
	QHash<long long, sa::pet_t> pet = getPets();
	for (i = 0; i < sa::MAX_PET; ++i)
	{
		if ((pet.value(i).hp > 0) && (pet.value(i).hp < pet.value(i).maxHp))
			return i + 1;
	}

	QHash<long long, sa::team_t> team = getTeams();
	for (i = 0; i < sa::MAX_TEAM; ++i)
	{
		if ((team.value(i).hp > 0) && (team.value(i).hp < team.value(i).maxHp))
			return i + 1 + sa::MAX_PET;
	}

	return 0;
}

//根據名稱和索引查找寵物是否存在
bool Worker::matchPetNameByIndex(long long index, const QString& cmpname)
{
	if (index < 0 || index >= sa::MAX_PET)
		return false;

	QString newCmpName = cmpname.simplified();
	if (newCmpName.isEmpty())
		return false;

	sa::pet_t pet = pet_.value(index);
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

//名稱取職業技能索引
long long Worker::getProfessionSkillIndexByName(const QString& names) const
{
	long long i = 0;
	bool isExact = true;
	QStringList list = names.split(util::rexOR, Qt::SkipEmptyParts);
	QHash <long long, sa::profession_skill_t> profession_skill = getSkills();

	for (QString name : list)
	{
		if (name.isEmpty())
			continue;

		if (name.startsWith("?"))
		{
			name = name.mid(1);
			isExact = false;
		}


		for (i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
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
void Worker::updateCurrentSideRange(sa::battle_data_t* bt) const
{
	if (nullptr == bt)
		return;

	if (battleCharCurrentPos.get() < 10)
	{
		bt->alliemin = 0;
		bt->alliemax = 9;
		bt->enemymin = 10;
		bt->enemymax = 19;
	}
	else
	{
		bt->alliemin = 10;
		bt->alliemax = 19;
		bt->enemymin = 0;
		bt->enemymax = 9;
	}
}

//根據索引刷新道具資訊
void Worker::refreshItemInfo(long long i)
{
	QVariant var;
	QVariantList varList;

	if (i < 0 || i >= sa::MAX_ITEM)
		return;

	sa::item_t item = getItem(i);

	if (i < sa::CHAR_EQUIPSLOT_COUNT)
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
				varList = { i - sa::CHAR_EQUIPSLOT_COUNT + 1, item.name, item.stack, item.damage, item.level, item.memo };
			else
				varList = { i - sa::CHAR_EQUIPSLOT_COUNT + 1, QString("%1(%2)").arg(item.name).arg(item.name2), item.stack, item.damage, item.level, item.memo };
		}
		else
		{
			varList = { i - sa::CHAR_EQUIPSLOT_COUNT + 1, "", "", "", "", "" };
		}
		var = QVariant::fromValue(varList);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());

	if (i < sa::CHAR_EQUIPSLOT_COUNT)
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
void Worker::refreshItemInfo()
{
	for (long long i = 0; i < sa::MAX_ITEM; ++i)
	{
		refreshItemInfo(i);
	}
}

//本來應該一次性讀取整個結構體的，但我們不需要這麼多訊息
void Worker::updateItemByMemory()
{
	QWriteLocker locker(&itemInfoLock_);

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	HANDLE hProcess = gamedevice.getProcess();

	QHash<long long, sa::item_t> items = item_.toHash();
	QStringList itemNames;

	for (long long i = 0; i < sa::MAX_ITEM; ++i)
	{
		items[i].valid = mem::read<short>(hProcess, hModule + sa::kOffestItemValid + i * sa::kItemStructSize) > 0;
		if (!items[i].valid)
		{
			items.remove(i);
			continue;
		}

		items[i].name = mem::readString(hProcess, hModule + sa::kOffsetItemName + i * sa::kItemStructSize, sa::ITEM_NAME_LEN, true, false);
		makeStringFromEscaped(items[i].name);
		if (items[i].name.isEmpty())
		{
			items.remove(i);
			continue;
		}

		items[i].memo = mem::readString(hProcess, hModule + sa::kOffsetItemMemo + i * sa::kItemStructSize, sa::ITEM_MEMO_LEN, true, false);
		makeStringFromEscaped(items[i].memo);

		if (items[i].name == "惡魔寶石" || items[i].name == "恶魔宝石")
		{
			static QRegularExpression rex("(\\d+)");
			QRegularExpressionMatch match = rex.match(items[i].memo);
			if (match.hasMatch())
			{
				QString str = match.captured(1);
				bool ok = false;
				long long dura = str.toLongLong(&ok);
				if (ok)
					items[i].count = dura;
			}
		}

		if (i >= sa::CHAR_EQUIPSLOT_COUNT)
			items[i].stack = mem::read<short>(hProcess, hModule + sa::kOffsetItemStack + i * sa::kItemStructSize);

		if (items.value(i).stack == 0)
			items[i].stack = 1;

		QString damage = mem::readString(hProcess, hModule + sa::kOffsetItemDurability + i * sa::kItemStructSize, 12, true, false);
		makeStringFromEscaped(damage);
		damage.remove("%");
		damage.remove("％");
		bool ok = false;
		items[i].damage = damage.toLongLong(&ok);
		if (!ok)
			items[i].damage = 100;

		itemNames.append(items[i].name);
	}
	gamedevice.setUserData(util::kUserItemNames, itemNames);

	item_ = items;
}

//讀取內存刷新各種基礎數據，有些封包數據不明確、或不確定，用來補充不足的部分
void Worker::updateDatasFromMemory()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	long long i = 0;
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	HANDLE hProcess = gamedevice.getProcess();

	QWriteLocker lockerPet(&petInfoLock_);
	//QWriteLocker lockerChar(&charInfoLock_);
	QHash<long long, sa::pet_t> pets = pet_.toHash();
	sa::pet_t pet = {};
	sa::character_t pc = getCharacter();
	std::ignore = getDir();

	//每隻寵物如果處於等待或戰鬥則為1
	short selectPetNo[sa::MAX_PET] = { 0i16, 0i16 ,0i16 ,0i16 ,0i16 };
	mem::read(hProcess, hModule + sa::kOffsetSelectPetArray, sizeof(selectPetNo), selectPetNo);
	for (i = 0; i < sa::MAX_PET; ++i)
	{
		pc.selectPetNo[i] = static_cast<long long>(selectPetNo[i]);
		if (pc.selectPetNo[i] > 0 && !pets.value(i).valid)
			pets[i].valid = true;
	}

	//郵件寵物索引
	long long mailPetIndex = static_cast<long long>(mem::read<short>(hProcess, hModule + sa::kOffsetMailPetIndex));
	if (mailPetIndex < 0 || mailPetIndex >= sa::MAX_PET)
		mailPetIndex = -1;

	pc.mailPetNo = mailPetIndex;
	if (mailPetIndex != -1 && !pets.value(mailPetIndex).valid)
		pets[mailPetIndex].valid = true;

	//騎乘寵物索引
	long long ridePetIndex = static_cast<long long>(mem::read<short>(hProcess, hModule + sa::kOffsetRidePetIndex));
	if (ridePetIndex < 0 || ridePetIndex >= sa::MAX_PET)
		ridePetIndex = -1;
	if (ridePetIndex != -1 && !pets.value(ridePetIndex).valid)
		pets[ridePetIndex].valid = true;

	if (ridePetIndex >= 0 && ridePetIndex < sa::MAX_PET)
	{
		pet = pets.value(ridePetIndex);
		emit signalDispatcher.updateRideHpProgressValue(pet.level, pet.hp, pet.maxHp);
	}
	else
		emit signalDispatcher.updateRideHpProgressValue(0, 0, 0);
	pc.ridePetNo = ridePetIndex;


	long long battlePetIndex = static_cast<long long>(mem::read<short>(hProcess, hModule + sa::kOffsetBattlePetIndex));
	if (battlePetIndex < 0 || battlePetIndex >= sa::MAX_PET)
		battlePetIndex = -1;
	if (battlePetIndex != -1 && !pets.value(battlePetIndex).valid)
		pets[battlePetIndex].valid = true;

	if (battlePetIndex >= 0 && battlePetIndex < sa::MAX_PET)
	{
		pet = pets.value(battlePetIndex);
		emit signalDispatcher.updatePetHpProgressValue(pet.level, pet.hp, pet.maxHp);
	}
	else
		emit signalDispatcher.updatePetHpProgressValue(0, 0, 0);
	pc.battlePetNo = battlePetIndex;

	long long standyPetCount = static_cast<long long>(mem::read<short>(hProcess, hModule + sa::kOffsetStandbyPetCount));
	pc.standbyPet = standyPetCount;

	//人物狀態 (是否組隊或其他..)
	pc.status = mem::read<short>(hProcess, hModule + sa::kOffsetCharStatus);

	bool isInTeam = mem::read<short>(hProcess, hModule + sa::kOffsetTeamState) > 0;
	if (isInTeam && !util::checkAND(pc.status, sa::kCharacterStatus_HasTeam))
		pc.status |= sa::kCharacterStatus_HasTeam;
	else if (!isInTeam && util::checkAND(pc.status, sa::kCharacterStatus_HasTeam))
		pc.status &= (~sa::kCharacterStatus_HasTeam);

	for (i = 0; i < sa::MAX_PET; ++i)
	{
		if (i == pc.mailPetNo)
		{
			pets[i].state = sa::kMail;
		}
		else if (i == pc.ridePetNo)
		{
			pets[i].state = sa::kRide;
		}
		else if (i == pc.battlePetNo)
		{
			pets[i].state = sa::kBattle;
		}
		else if (pc.selectPetNo[i] > 0)
		{
			pets[i].state = sa::kStandby;
		}
		else
		{
			pets[i].state = sa::kRest;
		}
		emit signalDispatcher.updateCharInfoPetState(i, pets[i].state);
	}

	pet_ = pets;
	setCharacter(pc);
	std::ignore = getPoint();
	std::ignore = getFloorName();
	std::ignore = getDir();
	std::ignore = getFloor();
}

//刷新要顯示的戰鬥時間和相關數據
void Worker::updateBattleTimeInfo()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.isGameInterruptionRequested())
		return;

	if (!getOnlineFlag())
		return;

	if (!getBattleFlag())
		return;

	QString battle_time_text;
	double time = 0.0;
	double cost = 0.0;
	double total_time = 0.0;
	long long bp = 0;
	long long field = 0;

	time = battleDurationTimer.cost() / 1000.0;
	cost = battle_one_round_time.get() / 1000.0;
	total_time = battle_total_time.get() / 1000.0 / 60.0;

	battle_time_text = QString(QObject::tr("%1 count no %2 round duration: %3 sec cost: %4 sec total time: %5 minues"))
		.arg(battle_total.get())
		.arg(battleCurrentRound.get() + 1)
		.arg(util::toQString(time))
		.arg(util::toQString(cost))
		.arg(util::toQString(total_time));

	bp = battleBpFlag.get();
	if (bp != 0 && util::checkAND(bp, sa::BATTLE_BP_PLAYER_SURPRISAL))
		battle_time_text += " " + QObject::tr("(surprise)");
	else if (bp != 0 && util::checkAND(bp, sa::BATTLE_BP_ENEMY_SURPRISAL))
		battle_time_text += " " + QObject::tr("(be surprised)");
	else
		battle_time_text += " " + QObject::tr("(normal)");

	field = battleField.get();
	battle_time_text += QString(QObject::tr(" field[%1]")).arg(getFieldString(field));

	timeLabelContents.set(battle_time_text);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.updateBattleTimeLabelTextChanged(battle_time_text.simplified());
}

//SASH內物品數據交換位置
void Worker::swapItemLocal(long long from, long long to)
{
	if (from < 0 || to < 0)
		return;

	QWriteLocker locker(&itemInfoLock_);
	QHash<long long, sa::item_t> items = item_.toHash();
	sa::item_t tmp = items.take(from);
	items.insert(from, items.value(to));
	items.insert(to, tmp);
	item_ = items;
}

void Worker::setWorldStatus(long long w) const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.sendMessage(kSetWorldStatus, w, NULL) == FALSE)
		mem::write<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetWorldStatus, w);
}

void Worker::setGameStatus(long long g) const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (gamedevice.sendMessage(kSetGameStatus, g, NULL) == FALSE)
		mem::write<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetGameStatus, g);
}

//切換是否在戰鬥中的標誌
void Worker::setBattleFlag(bool enable)
{
	isBattle_.set(enable);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.updateStatusLabelTextChanged(enable ? util::kLabelStatusInBattle : util::kLabelStatusInNormal);

	isBattleDialogReady.off();
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.worker.isNull())
		return;

	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();

	//這裡關乎頭上是否會出現V.S.圖標
	long long status = mem::read<short>(hProcess, hModule + sa::kOffsetCharStatus);
	bool result = util::checkAND(status, sa::kCharacterStatus_InBattle);
	if (enable)
	{
		if (!result)
			status |= sa::kCharacterStatus_InBattle;
	}
	else
	{
		if (result)
			status &= ~sa::kCharacterStatus_InBattle;
	}

	mem::write<int>(hProcess, hModule + sa::kOffsetCharStatus, status);
	mem::write<int>(hProcess, hModule + sa::kOffsetBattleStatus, enable ? 1 : 0);

	mem::write<int>(hProcess, hModule + 0x4169B6C, 0);

	mem::write<int>(hProcess, hModule + 0x4181198, 0);
	mem::write<int>(hProcess, hModule + 0x41829FC, 0);

	mem::write<int>(hProcess, hModule + 0x415EF9C, 0);
	mem::write<int>(hProcess, hModule + 0x4181D44, 0);
	mem::write<int>(hProcess, hModule + 0x41829F8, 0);

	mem::write<int>(hProcess, hModule + 0x415F4EC, 30);

	if (enable || gamedevice.getEnableHash(util::kLockMoveEnable))
	{
		gamedevice.sendMessage(kEnableMoveLock, true, NULL);
	}
	else if (!enable && !gamedevice.getEnableHash(util::kLockMoveEnable))
	{
		gamedevice.sendMessage(kEnableMoveLock, false, NULL);
	}
}

void Worker::setOnlineFlag(bool enable)
{
	if (!enable)
	{
		setBattleEnd();
		IS_TRADING.off();
	}

	isOnline_.set(enable);
}

bool Worker::setWindowTitle(QString formatStr)
{
	if (doNotChangeTitle_.get())
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.isValid())
		return false;

	long long subServer = static_cast<long long>(mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetSubServerIndex));
	long long position = static_cast<long long>(mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetPositionIndex));

	QString subServerName;
	long long size = gamedevice.subServerNameList.get().size();
	if (subServer >= 0 && subServer < size)
		subServerName = gamedevice.subServerNameList.get().value(subServer);
	else
		subServerName = "????";

	QString positionName;
	if (position >= 0 && position < sa::MAX_CHARACTER - 1)
		positionName = position == 0 ? QObject::tr("left") : QObject::tr("right");
	else
		positionName = util::toQString(position);

	formatStr.replace("%(index)", util::toQString(currentIndex));
	formatStr.replace("%(sser)", subServerName);
	formatStr.replace("%(pos)", positionName);

	sa::character_t pc = getCharacter();
	formatStr.replace("%(hp)", util::toQString(pc.hp), Qt::CaseInsensitive);
	formatStr.replace("%(maxhp)", util::toQString(pc.maxHp), Qt::CaseInsensitive);
	formatStr.replace("%(mp)", util::toQString(pc.mp), Qt::CaseInsensitive);
	formatStr.replace("%(hpp)", util::toQString(pc.hpPercent), Qt::CaseInsensitive);
	formatStr.replace("%(mpp)", util::toQString(pc.mpPercent), Qt::CaseInsensitive);
	formatStr.replace("%(lv)", util::toQString(pc.level), Qt::CaseInsensitive);
	formatStr.replace("%(exp)", util::toQString(pc.exp), Qt::CaseInsensitive);
	formatStr.replace("%(maxexp)", util::toQString(pc.maxExp), Qt::CaseInsensitive);
	formatStr.replace("%(name)", pc.name, Qt::CaseInsensitive);
	formatStr.replace("%(fname)", pc.freeName, Qt::CaseInsensitive);

	sa::pet_t pet = getPet(pc.battlePetNo);
	formatStr.replace("%(php)", util::toQString(pet.hp), Qt::CaseInsensitive);
	formatStr.replace("%(pmaxhp)", util::toQString(pet.maxHp), Qt::CaseInsensitive);
	formatStr.replace("%(phpp)", util::toQString(pet.hpPercent), Qt::CaseInsensitive);

	if (formatStr.contains("%(floor)", Qt::CaseInsensitive))
		formatStr.replace("%(floor)", util::toQString(getFloor()), Qt::CaseInsensitive);
	if (formatStr.contains("%(map)", Qt::CaseInsensitive))
		formatStr.replace("%(map)", getFloorName(), Qt::CaseInsensitive);

	if (formatStr.contains("%(x)", Qt::CaseInsensitive) || formatStr.contains("%(y)", Qt::CaseInsensitive))
	{
		QPoint pos = getPoint();
		formatStr.replace("%(x)", util::toQString(pos.x()), Qt::CaseInsensitive);
		formatStr.replace("%(y)", util::toQString(pos.y()), Qt::CaseInsensitive);
	}

	if (formatStr.contains("%(script)", Qt::CaseInsensitive))
	{
		QString fileName = gamedevice.currentScriptFileName.get();
		fileName.remove(util::applicationDirPath() + "/script/");
		fileName.remove(util::applicationDirPath());
		QFileInfo fileInfo(fileName);
		formatStr.replace("%(script)", fileName, Qt::CaseInsensitive);
	}

	std::wstring wtitle = formatStr.toStdWString();
	return SetWindowTextW(gamedevice.getProcessWindow(), wtitle.c_str()) == TRUE;
}

void Worker::setPoint(const QPoint& point)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long hModule = gamedevice.getProcessModule();
	if (hModule == 0)
		return;

	HANDLE hProcess = gamedevice.getProcess();
	if (hProcess == 0 || hProcess == INVALID_HANDLE_VALUE)
		return;

	nowPoint_.set(point);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2(%3)").arg(point.x()).arg(point.y()).arg(g_dirStrHash.value(getDir())));

	if (mem::read<int>(hProcess, hModule + sa::kOffsetNowX) != point.x())
		mem::write<int>(hProcess, hModule + sa::kOffsetNowX, point.x());
	if (mem::read<int>(hProcess, hModule + sa::kOffsetNowY) != point.y())
		mem::write<int>(hProcess, hModule + sa::kOffsetNowY, point.y());
}

//清屏 (實際上就是 char數組置0)
bool Worker::cleanChatHistory()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.sendMessage(kCleanChatHistory, NULL, NULL))
		return false;

	chatQueue.clear();
	gamedevice.chatLogModel.clear();

	return true;
}

void Worker::updateComboBoxList() const
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	long long battlePetIndex = -1;

	QStringList itemList;
	QHash <long long, sa::item_t> items = getItems();
	for (const sa::item_t& it : items)
	{
		if (it.name.isEmpty())
			continue;
		itemList.append(it.name);
	}

	emit signalDispatcher.updateComboBoxItemText(util::kComboBoxItem, itemList);

	QFontMetrics fontMetrics(QApplication::font());

	QStringList magicNameList;
	for (long long i = 0; i < sa::MAX_MAGIC; ++i)
	{
		sa::magic_t magic = getMagic(i);
		long long textWidth = fontMetrics.horizontalAdvance(magic.name);
		QString shortText = QString(QObject::tr("(cost:%1)")).arg(magic.costmp);
		long long shortcutWidth = fontMetrics.horizontalAdvance(shortText);
		constexpr long long totalWidth = 120;
		const auto size = fontMetrics.horizontalAdvance(' ');
		long long spaceCount = 1;
		if (size > 0)
			spaceCount = (totalWidth - textWidth - shortcutWidth) / fontMetrics.horizontalAdvance(' ');

		QString alignedText = magic.name + QString(spaceCount, ' ') + shortText;

		magicNameList.append(alignedText);
	}

	for (long long i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
	{
		sa::profession_skill_t profession_skill = getSkill(i);
		if (profession_skill.valid)
		{
			if (profession_skill.name.size() == 3)
				profession_skill.name += "　";

			long long textWidth = fontMetrics.horizontalAdvance(profession_skill.name);
			QString shortText = QString("(%1%)").arg(profession_skill.skill_level);
			long long shortcutWidth = fontMetrics.horizontalAdvance(shortText);
			constexpr long long totalWidth = 140;
			long long spaceCount = (totalWidth - textWidth - shortcutWidth) / fontMetrics.horizontalAdvance(' ');

			if (i < 9)
				profession_skill.name += "  ";

			QString alignedText = profession_skill.name + QString(spaceCount, ' ') + shortText;

			magicNameList.append(alignedText);
		}
		else
			magicNameList.append("");

	}
	emit signalDispatcher.updateComboBoxItemText(util::kComboBoxCharAction, magicNameList);
	battlePetIndex = getCharacter().battlePetNo;


	if (battlePetIndex >= 0)
	{
		QStringList skillNameList;
		for (long long i = 0; i < sa::MAX_PET_SKILL; ++i)
		{
			sa::pet_skill_t petSkill = getPetSkill(battlePetIndex, i);
			skillNameList.append(petSkill.name + ":" + petSkill.memo);
		}
		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxPetAction, skillNameList);
	}
}
#pragma endregion

#pragma region System
//公告
bool Worker::announce(const QString& msg, long long color)
{
	if (!getOnlineFlag())
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	HANDLE hProcess = gamedevice.getProcess();
	if (!msg.isEmpty())
	{
		std::string str = util::fromUnicode(msg);
		mem::VirtualMemory ptr(hProcess, str.size(), true);
		if (!mem::write(hProcess, ptr, const_cast<char*>(str.c_str()), str.size()))
			return false;

		if (gamedevice.sendMessage(kSendAnnounce, ptr, color) <= 0)
			return false;
	}
	else
	{
		mem::VirtualMemory ptr(hProcess, "", mem::VirtualMemory::kAnsi, true);
		if (gamedevice.sendMessage(kSendAnnounce, ptr, color) <= 0)
			return false;
	}

	chatQueue.enqueue(qMakePair(color, msg.simplified()));
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendChatLog(msg, color);

	return true;
}

//喊話
bool Worker::talk(const QString& text, long long color, sa::TalkMode mode)
{
	if (!getOnlineFlag())
		return false;

	if (text.startsWith("//skup"))
	{
		return lssproto_CustomTK_recv(text);
	}

	if (color < 0 || color > 10)
		color = 0;

	long long flg = getCharacter().etcFlag;
	QString msg = "P|";
	if (mode == sa::kTalkGlobal)
		msg += ("/XJ ");
	else if (mode == sa::kTalkFamily)
		msg += ("/FM ");
	else if (mode == sa::kTalkWorld)
		msg += ("/WD ");
	else if (mode == sa::kTalkTeam)
	{
		long long newflg = flg;
		if (!util::checkAND(newflg, sa::PC_ETCFLAG_TEAM_CHAT))
		{
			newflg |= sa::PC_ETCFLAG_TEAM_CHAT;
			setSwitchers(newflg);
			QThread::msleep(500);
		}
	}

	msg += text;
	std::string str = util::fromUnicode(msg);
	return lssproto_TK_send(getPoint(), const_cast<char*>(str.c_str()), color, 3);
}

//創建人物
bool Worker::createCharacter(long long dataplacenum
	, const QString& charname
	, long long imgno
	, long long faceimgno
	, long long vit
	, long long str
	, long long tgh
	, long long dex
	, long long earth
	, long long water
	, long long fire
	, long long wind
	, long long hometown
	, bool forcecover)
{

	if (!forcecover)
	{
		//防複寫保護
		if (getCharListTable(dataplacenum).valid)
			return false;
	}

	std::string sname = util::fromUnicode(charname);
	if (!lssproto_CreateNewChar_send(dataplacenum, const_cast<char*>(sname.c_str()), imgno, faceimgno, vit, str, tgh, dex, earth, water, fire, wind, hometown))
		return false;

	if (!checkWG(3, 11))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();
	mem::write<int>(hProcess, hModule + 0x421C000, 1);
	long long time = timeGetTime();
	mem::write<int>(hProcess, hModule + 0x421C004, time);
	mem::write<int>(hProcess, hModule + 0x4152B44, 2);

	setWorldStatus(2);
	setGameStatus(2);

	return true;
}

//刪除人物
bool Worker::deleteCharacter(long long index, const QString password, bool backtofirst)
{
	if (index < 0 || index > sa::MAX_CHARACTER)
		return false;

	if (!checkWG(3, 11))
		return false;

	sa::char_list_data_t table = getCharListTable(index);

	if (!table.valid)
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();

	mem::write<int>(hProcess, hModule + 0x4230A88, index);
	mem::writeString(hProcess, hModule + 0x421BF74, table.name);

	std::string sname = util::fromUnicode(table.name);
	std::string spassword = util::fromUnicode(password);
	if (!lssproto_CharDelete_send(const_cast<char*>(sname.c_str()), const_cast<char*>(spassword.c_str())))
		return false;

	mem::write<int>(hProcess, hModule + 0x421C000, 1);
	mem::write<int>(hProcess, hModule + 0x415EF6C, 2);

	long long time = timeGetTime();
	mem::write<int>(hProcess, hModule + 0x421C004, time);

	setGameStatus(21);

	if (backtofirst)
	{
		setWorldStatus(1);
		setGameStatus(2);
	}

	return true;
}

//老菜單
bool Worker::shopOk(long long n)
{
	//SE 1隨身倉庫 2查看聲望氣勢
	return lssproto_ShopOk_send(n);
}

//新菜單
bool Worker::saMenu(long long n)
{
	return lssproto_SaMenu_send(n);
}

//切換單一開關
bool Worker::setSwitcher(long long flg, bool enable)
{
	sa::character_t pc = getCharacter();
	if (enable)
		pc.etcFlag |= flg;
	else
		pc.etcFlag &= ~flg;

	return setSwitchers(pc.etcFlag);
}

//切換全部開關
bool Worker::setSwitchers(long long flg)
{
	return lssproto_FS_send(flg);
}

//對話框是否存在
bool Worker::isDialogVisible() const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();

	bool bret = mem::read<int>(hProcess, hModule + sa::kOffsetDialogType) != -1;
	bool custombret = mem::read<int>(hProcess, hModule + sa::kOffsetDialogValid) > 0;
	return bret && custombret;
}
#pragma endregion

#pragma region Connection
bool Worker::echo()
{
	lssproto_EO_send(0);
	eoTTLTimer_.restart();
	return lssproto_Echo_send(const_cast<char*>("hoge"));
}

//元神歸位
bool Worker::EO()
{
#ifdef _DEBUG
	//測試用區塊
#endif
	long long currentIndex = getIndex();
	//石器私服SE SO專用
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QString cmd = gamedevice.getStringHash(util::kEOCommandString);
	if (!cmd.isEmpty())
		talk(cmd);

	if (getOnlineFlag())
	{
		lastEOTime.set(-1);
		isEOTTLSend.on();
	}

	return echo();
}
//登出
bool Worker::logOut()
{
	if (!getOnlineFlag())
		return false;

	if (!lssproto_CharLogout_send(0))
		return false;

	lssproto_Echo_send(const_cast<char*>("hoge"));

	setWorldStatus(7);
	setGameStatus(0);

	return true;
}

//回點
bool Worker::logBack()
{
	if (!getOnlineFlag())
		return false;

	if (getBattleFlag())
		lssproto_BU_send(0);//退出觀戰

	return lssproto_CharLogout_send(1);
}

#include "macchanger.h"
#include "descrypt.h"
bool Worker::clientLogin(const QString& userName, const QString& password)
{
	std::string sname = util::fromUnicode(userName);
	std::string spassword = util::fromUnicode(password);
	MyMACAddr m;
	std::string mac = m.GenRandMAC();
	std::string ip = "192.168.1.1";
	long long serverIndex = 0;

	char userId[32] = {};
	char userPassword[32] = {};

	_snprintf_s(userId, sizeof(userId), _TRUNCATE, "%s", sname.c_str());
	_snprintf_s(userPassword, sizeof(userPassword), _TRUNCATE, "%s", spassword.c_str());

	return lssproto_ClientLogin_send(userId
		, userPassword
		, const_cast<char*>(mac.c_str())
		, serverIndex
		, const_cast<char*>(ip.c_str())
		, sa::WITH_CDKEY | sa::WITH_PASSWORD | sa::WITH_MACADDRESS);
}

bool Worker::playerLogin(long long index)
{
	if (index < 0 || index >= sa::MAX_CHARACTER)
		return false;
	sa::char_list_data_t chartable = getCharListTable(index);
	if (!chartable.valid)
		return false;

	std::string name = util::fromUnicode(chartable.name);

	return lssproto_CharLogin_send(const_cast<char*>(name.c_str()));
}

//登入
bool Worker::login(long long s)
{
	util::UnLoginStatus status = static_cast<util::UnLoginStatus>(s);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();

	long long server = gamedevice.getValueHash(util::kServerValue);
	long long subserver = gamedevice.getValueHash(util::kSubServerValue);
	long long position = gamedevice.getValueHash(util::kPositionValue);
	QString account = gamedevice.getStringHash(util::kGameAccountString);
	QString password = gamedevice.getStringHash(util::kGamePasswordString);

	util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
	util::timer timer;

	auto backToFirstPage = [this, &signalDispatcher, &gamedevice, s]()
		{
			if (s == util::kStatusInputUser)
				return;

			gamedevice.setEnableHash(util::kAutoLoginEnable, false);
			emit signalDispatcher.applyHashSettingsToUI();

			setWorldStatus(7);
			setGameStatus(0);
		};

	auto input = [this, &signalDispatcher, &gamedevice, hProcess, hModule, &backToFirstPage, s, &account, &password]()->bool
		{

			QString acct = mem::readString(hProcess, hModule + sa::kOffsetAccount, 32);
			QString pwd = mem::readString(hProcess, hModule + sa::kOffsetPassword, 32);
			QString acctECB = mem::readString(hProcess, hModule + sa::kOffsetAccountECB, 32, false, true);
			QString pwdECB = mem::readString(hProcess, hModule + sa::kOffsetPasswordECB, 32, false, true);

			if (account.isEmpty())
			{
				//檢查是否已手動輸入帳號
				if (!acct.isEmpty())
				{
					account = acct;
					gamedevice.setStringHash(util::kGameAccountString, account);
					emit signalDispatcher.applyHashSettingsToUI();
				}
			}
			else //寫入配置中的帳號
				mem::writeString(hProcess, hModule + sa::kOffsetAccount, account);

			if (password.isEmpty())
			{
				//檢查是否已手動輸入密碼
				if (!pwd.isEmpty())
				{
					password = pwd;
					gamedevice.setStringHash(util::kGamePasswordString, password);
					emit signalDispatcher.applyHashSettingsToUI();
				}
			}
			else //寫入配置中的密碼
				mem::writeString(hProcess, hModule + sa::kOffsetPassword, password);

			if (account.isEmpty() && password.isEmpty() && acctECB.isEmpty() && pwdECB.isEmpty())
			{
				emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNoUsernameAndPassword);
				backToFirstPage();
				return false;
			}
			else if (account.isEmpty() && acctECB.isEmpty())
			{
				emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNoUsername);
				backToFirstPage();
				return false;
			}
			else if (password.isEmpty() && pwdECB.isEmpty())
			{
				emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusNoPassword);
				backToFirstPage();
				return false;
			}

			//如果用戶手動輸入帳號與配置不同
			if (account != acct && s == util::kStatusInputUser || acctECB.isEmpty() && s != util::kStatusInputUser)
			{
				backToFirstPage();
				return false;
			}

			//如果用戶手動輸入密碼與配置不同
			if (password != pwd && s == util::kStatusInputUser || pwdECB.isEmpty() && s != util::kStatusInputUser)
			{
				backToFirstPage();
				return false;
			}

			return true;

			//std::string saccount = util::fromUnicode(account);
			//std::string spassword = util::fromUnicode(password);

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
	case util::kStatusDisconnect:
	{
		if (!gamedevice.getEnableHash(util::kAutoReconnectEnable))
			break;

		QList<long long> list = config.readArray<long long>("System", "Login", "Disconnect");
		if (list.size() == 2)
			gamedevice.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			gamedevice.leftDoubleClick(315, 270);
			config.writeArray<long long>("System", "Login", "Disconnect", { 315, 270 });
		}
		break;
	}
	case util::kStatusLoginFailed:
	{
		QList<long long> list = config.readArray<long long>("System", "Login", "LoginFailed");
		if (list.size() == 2)
			gamedevice.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			gamedevice.leftDoubleClick(315, 255);
			config.writeArray<long long>("System", "Login", "LoginFailed", { 315, 255 });
		}
		break;
	}
	case util::kStatusBusy:
	{
		QList<long long> list = config.readArray<long long>("System", "Login", "Busy");
		if (list.size() == 2)
			gamedevice.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			gamedevice.leftDoubleClick(315, 255);
			config.writeArray<long long>("System", "Login", "Busy", { 315, 255 });
		}
		break;
	}
	case util::kStatusTimeout:
	{
		QList<long long> list = config.readArray<long long>("System", "Login", "Timeout");
		if (list.size() == 2)
			gamedevice.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			gamedevice.leftDoubleClick(315, 253);
			config.writeArray<long long>("System", "Login", "Timeout", { 315, 253 });
		}
		break;
	}
	case util::kNoUserNameOrPassword:
	{
		backToFirstPage();
#ifdef USE_MOUSE
		QList<long long> list = config.readArray<long long>("System", "Login", "NoUserNameOrPassword");
		if (list.size() == 2)
			gamedevice.leftDoubleClick(list.value(0), list.value(1));
		else
		{
			gamedevice.leftDoubleClick(315, 253);
			config.writeArray<long long>("System", "Login", "NoUserNameOrPassword", { 315, 253 });
		}
#endif
		break;
	}
	case util::kStatusInputUser:
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
		if (!mem::write(hProcess, hModule + 0x206F1, const_cast<char*>("\xEB\x04\x90\x90\x90\x90"), 6))//進入OK點擊事件
			break;

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

			if (gamedevice.isGameInterruptionRequested())
				return false;

			QThread::msleep(200);
		}

		//sa_8001.exe+206F1 - 0F85 1A020000         - jne sa_8001.exe+20911
		mem::write(hProcess, hModule + 0x206F1, const_cast<char*>("\x0F\x85\x1A\x02\x00\x00"), 6);//還原OK點擊事件
		break;
	}
	case util::kStatusSelectServer:
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
		if (!mem::write(hProcess, hModule + 0x21536, code, sizeof(code)))//進入伺服器點擊事件
			break;

		timer.restart();
		for (;;)
		{
			if (getGameStatus() == 3)
				break;

			if (timer.hasExpired(1500))
				break;

			if (gamedevice.isGameInterruptionRequested())
				return false;

			QThread::msleep(200);
		}

		/*
		sa_8001.exe+21536 - 0F8C 91000000         - jl sa_8001.exe+215CD
		sa_8001.exe+2153C - 3B C1                 - cmp eax,ecx
		*/
		mem::write(hProcess, hModule + 0x21536, const_cast<char*>("\x0F\x8C\x91\x00\x00\x00\x3B\xC1"), 8);//還原伺服器點擊事件
#else
		constexpr long long table[48] = {
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

		const long long a = table[server * 3 + 1];
		const long long b = table[server * 3 + 2];

		long long x = 160 + (a * 125);
		long long y = 165 + (b * 25);

		QList<long long> list = config.readArray<long long>("System", "Login", "SelectServer");
		if (list.size() == 4)
		{
			x = list.value(0) + (a * list.value(1));
			y = list.value(2) + (b * list.value(3));
		}
		else
		{
			config.writeArray<long long>("System", "Login", "SelectServer", { 160, 125, 165, 25 });
		}

		for (;;)
		{
			gamedevice.mouseMove(x, y);
			long long value = mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + kOffsetMousePointedIndex);
			if (value != -1)
			{
				gamedevice.leftDoubleClick(x, y);
				break;
			}
			x -= 5;
			if (x <= 0)
				break;

			if (timer.hasExpired(1500))
				break;

		}
#endif
		break;
	}
	case util::kStatusSelectSubServer:
	{
		if (!input())
			break;

		if (subserver < 0 || subserver >= 15)
			break;

		long long serverIndex = static_cast<long long>(mem::read<int>(hProcess, hModule + sa::kOffsetServerIndex));

#ifndef USE_MOUSE
		/*
		sa_8001.exe+2186C - 8D 0C D2              - lea ecx,[edx+edx*8]
		sa_8001.exe+2186F - C1 E1 03              - shl ecx,03 { 3 }

		sa_8001.exe+21881 - 66 8B 89 2CEDEB04     - mov cx,[ecx+sa_8001.exe+4ABED2C]
		sa_8001.exe+21888 - 66 03 C8              - add cx,ax

		sa_8001.exe+2189B - 66 89 0D 88424C00     - mov [sa_8001.exe+C4288],cx { (23) }

		*/

		long long ecxValue = serverIndex + (serverIndex * 8);
		ecxValue <<= 3;
		long long cxValue = mem::read<short>(hProcess, ecxValue + hModule + 0x4ABED2C);
		cxValue += subserver;

		if (!mem::write<short>(hProcess, hModule + 0xC4288, cxValue))//選擇伺服器+分流
			break;

		/*
		sa_8001.exe+2189B - 90                    - nop
		sa_8001.exe+2189C - 90                    - nop
		sa_8001.exe+2189D - 90                    - nop
		sa_8001.exe+2189E - 90                    - nop
		sa_8001.exe+2189F - 90                    - nop
		sa_8001.exe+218A0 - 90                    - nop
		sa_8001.exe+218A1 - 90                    - nop
		*/
		if (!mem::write(hProcess, hModule + 0x2189B, const_cast<char*>("\x90\x90\x90\x90\x90\x90\x90"), 7))//防止被複寫
			break;

		//sa_8001.exe+21A53 - 0FBF 05 88424C00      - movsx eax,word ptr [sa_8001.exe+C4288] { (0) } << 這裡決定最後登入的伺服器

		/*
		sa_8001.exe+21864 - BA 00000000           - mov edx,00000007 { 0 }
		sa_8001.exe+21869 - 90                    - nop
		sa_8001.exe+2186A - 90                    - nop
		sa_8001.exe+2186B - 90                    - nop
		*/
		BYTE bser = static_cast<BYTE>(serverIndex) + 1ui8;
		BYTE code[8] = { 0xBA, bser, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90 };
		if (!mem::write(hProcess, hModule + 0x21864, code, sizeof(code)))//進入分流點擊事件
			break;

		timer.restart();
		connectingTimer_.restart();
		for (;;)
		{
			if (getGameStatus() > 3)
				break;

			if (timer.hasExpired(1500))
				break;

			if (gamedevice.isGameInterruptionRequested())
				return false;

			QThread::msleep(200);
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
			long long x = 500;
			long long y = 340;

			QList<
			> list = config.readArray<long long>("System", "Login", "SelectSubServerGoBack");
			if (list.size() == 2)
			{
				x = list.value(0);
				y = list.value(1);
			}
			else
			{
				config.writeArray<long long>("System", "Login", "SelectSubServer", QList<long long>{ 500, 340 });
			}

			for (;;)
			{
				gamedevice.mouseMove(x, y);
				long long value = mem::read<long long>(gamedevice.getProcess(), gamedevice.getProcessModule() + kOffsetMousePointedIndex);
				if (value != -1)
				{
					gamedevice.leftDoubleClick(x, y);
					break;
				}

				x -= 10;
				if (x <= 0)
					break;

				if (timer.hasExpired(1500))
					break;

			}
			break;
		}

		if (subserver >= 0 && subserver < 15)
		{
			long long x = 250;
			long long y = 265 + (subserver * 20);

			QList<long long> list = config.readArray<long long>("System", "Login", "SelectSubServer");
			if (list.size() == 3)
			{
				x = list.value(0);
				y = list.value(1) + (subserver * list.value(2));
			}
			else
			{
				config.writeArray<long long>("System", "Login", "SelectSubServer", { 250, 265, 20 });
			}

			for (;;)
			{
				gamedevice.mouseMove(x, y);
				long long value = mem::read<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + kOffsetMousePointedIndex);
				if (value != -1)
				{
					gamedevice.leftDoubleClick(x, y);
					break;
				}

				x += 5;
				if (x >= 640)
					break;

				if (timer.hasExpired(1500))
					break;

			}
		}
#endif
		break;
	}
	case util::kStatusSelectCharacter:
	{
		if (position < 0 || position > sa::MAX_CHARACTER)
			break;

		//if (!getCharListTable(position).valid)
		//	break;

		//#ifndef USE_MOUSE
				//playerLogin(position);
				//setGameStatus(1);
				//setWorldStatus(5);
				//QThread::msleep(1000);
		//#else
		long long x = 100 + (position * 300);
		long long y = 340;

		QList<long long> list = config.readArray<long long>("System", "Login", "SelectCharacter");
		if (list.size() == 3)
		{
			x = list.value(0) + (position * list.value(1));
			y = list.value(2);
		}
		else
		{
			config.writeArray<long long>("System", "Login", "SelectCharacter", { 100, 300, 340 });
		}

		gamedevice.leftClick(x, y);
		//#endif
		break;
	}
	case util::kStatusConnecting:
	{
		if (connectingTimer_.hasExpired(sa::MAX_TIMEOUT))
		{
			setWorldStatus(7);
			setGameStatus(0);
			connectingTimer_.restart();
		}
		break;
	}
	case util::kStatusLogined:
	{
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
bool Worker::createRemoteDialog(unsigned long long type, unsigned long long button, const QString& text) const
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	mem::VirtualMemory ptr(gamedevice.getProcess(), text, mem::VirtualMemory::kAnsi, true);

	return gamedevice.sendMessage(kCreateDialog, MAKEWPARAM(type, button), ptr);
}

//按下按鈕
bool Worker::press(sa::ButtonType select, long long dialogid, long long unitid)
{
	if (select == sa::kButtonClose)
	{
		long long currentIndex = getIndex();
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
	}

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	char data[2] = { '\0', '\0' };
	if (sa::kButtonBuy == select)
	{
		data[0] = { '1' };
		select = sa::kButtonNone;
	}
	else if (sa::kButtonSell == select)
	{
		data[0] = { '2' };
		select = sa::kButtonNone;
	}
	else if (sa::kButtonLeave == select)
	{
		data[0] = { '3' };
		select = sa::kButtonNone;
	}
	else if (sa::kButtonBack == select)
	{
		select = sa::kButtonNone;
	}
	else if (sa::kButtonAuto == select)
	{
		if (util::checkAND(dialog.buttontype, sa::kButtonNext))
			select = sa::kButtonNext;
		else if (util::checkAND(dialog.buttontype, sa::kButtonPrevious))
			select = sa::kButtonPrevious;
		else if (util::checkAND(dialog.buttontype, sa::kButtonYes))
			select = sa::kButtonYes;
		else if (util::checkAND(dialog.buttontype, sa::kButtonOk))
			select = sa::kButtonOk;
		else if (util::checkAND(dialog.buttontype, sa::kButtonNo))
			select = sa::kButtonNo;
		else if (util::checkAND(dialog.buttontype, sa::kButtonCancel))
			select = sa::kButtonCancel;
	}

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, select, const_cast<char*>(data)))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

//按下行按鈕
bool Worker::press(long long row, long long dialogid, long long unitid)
{
	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;
	QString qrow = util::toQString(row);
	std::string srow = util::fromUnicode(qrow);

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str())))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

//買東西
bool Worker::buy(long long index, long long amt, long long dialogid, long long unitid)
{
	if (index < 0)
		return false;

	if (amt <= 0)
		return false;

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qrow = QString("%1\\z%2").arg(index + 1).arg(amt);
	std::string srow = util::fromUnicode(qrow);

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str())))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

//賣東西
bool Worker::sell(const QString& name, const QString& memo, long long dialogid, long long unitid)
{
	if (name.isEmpty())
		return false;

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QVector<long long> indexs;
	if (!getItemIndexsByName(name, memo, &indexs, sa::CHAR_EQUIPSLOT_COUNT))
		return false;

	return sell(indexs, dialogid, unitid);
}

//賣東西
bool Worker::sell(long long index, long long dialogid, long long unitid)
{
	if (index < 0 || index >= sa::MAX_ITEM)
		return false;

	sa::item_t item = getItem(index);
	if (item.name.isEmpty() || !item.valid)
		return false;

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qrow = QString("%1\\z%2").arg(index).arg(item.stack);
	std::string srow = util::fromUnicode(qrow);

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str())))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

//賣東西
bool Worker::sell(const QVector<long long>& indexs, long long dialogid, long long unitid)
{
	if (indexs.isEmpty())
		return false;

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QStringList list;
	bool bret = false;
	for (const long long it : indexs)
	{
		if (it < 0 || it >= sa::MAX_ITEM)
			continue;

		bret = sell(it, dialogid, unitid);
	}

	return bret;
}

//寵物學技能
bool Worker::learn(long long petIndex, long long shopSkillIndex, long long petSkillSpot, long long dialogid, long long unitid)
{
	if (shopSkillIndex < 0)
		return false;

	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return false;

	if (petSkillSpot < 0 || petSkillSpot >= sa::MAX_PET_SKILL)
		return false;

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;
	//8\z3\z3\z1000 技能 商品編號\z寵索引\z位置\z金額
	QString qrow = QString("%1|%2|%3|0").arg(++shopSkillIndex).arg(++petIndex).arg(++petSkillSpot);
	qrow.replace("|", "\\z");
	std::string srow = util::fromUnicode(qrow);

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str())))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

bool Worker::depositItem(long long itemIndex, long long dialogid, long long unitid)
{
	if (itemIndex < 0 || itemIndex >= sa::MAX_ITEM)
		return false;

	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = util::toQString(itemIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str())))
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

bool Worker::withdrawItem(long long itemIndex, long long dialogid, long long unitid)
{
	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = util::toQString(itemIndex + 1);
	std::string srow = util::fromUnicode(qstr);

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str())))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.sendMessage(kDistoryDialog, NULL, NULL))
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

bool Worker::depositPet(long long petIndex, long long dialogid, long long unitid)
{
	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = util::toQString(petIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	return lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str()));
}

bool Worker::withdrawPet(long long petIndex, long long dialogid, long long unitid)
{
	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;

	if (unitid == -1)
		unitid = dialog.unitid;

	QString qstr = util::toQString(petIndex + 1);
	std::string srow = util::fromUnicode(qstr);
	return lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonNone, const_cast<char*>(srow.c_str()));
}

//遊戲對話框輸入文字送出
bool Worker::inputtext(const QString& text, long long dialogid, long long unitid)
{
	sa::dialog_t dialog = currentDialog.get();
	if (dialogid == -1)
		dialogid = dialog.dialogid;
	if (unitid == -1)
		unitid = dialog.unitid;
	std::string s = util::fromUnicode(text);

	if (!lssproto_WN_send(getPoint(), dialogid, unitid, sa::kButtonOk, const_cast<char*>(s.c_str())))
		return false;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

//解除安全瑪
bool Worker::unlockSecurityCode(const QString& code)
{
	if (code.isEmpty())
		return false;

	std::string scode = util::fromUnicode(code);
	if (!lssproto_WN_send(getPoint(), sa::kDialogSecurityCode, -1, NULL, const_cast<char*>(scode.c_str())))
		return false;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	return gamedevice.sendMessage(kDistoryDialog, NULL, NULL);
}

bool Worker::windowPacket(const QString& command, long long dialogid, long long unitid)
{
	//SI|itemIndex(0-15)|Stack(-1)
	//TI|itemIndex(0-59)|Stack(-1)
	//SP|petIndex(0-4)|
	//TP|petIndex(0-?)|
	//SG|gold|
	//TG|gold|
	std::string s = util::fromUnicode(command);
	return lssproto_WN_send(getPoint(), dialogid, unitid, NULL, const_cast<char*>(s.c_str()));
}
#pragma endregion

#pragma region CHAR
//使用精靈
bool Worker::useMagic(long long magicIndex, long long target)
{
	if (target < 0 || target >= (sa::MAX_PET + sa::MAX_TEAM))
		return false;

	if (magicIndex < 0 || magicIndex >= sa::MAX_ITEM)
		return false;

	if (magicIndex < sa::MAX_MAGIC && !isCharMpEnoughForMagic(magicIndex))
		return false;

	if (magicIndex >= 0 && magicIndex < sa::MAX_MAGIC && !getMagic(magicIndex).valid)
		return false;

	return lssproto_MU_send(getPoint(), magicIndex, target);
}

//組隊或離隊 true 組隊 false 離隊
bool Worker::setTeamState(bool join)
{
	return lssproto_PR_send(getPoint(), join ? 1 : 0);
}

//踢走隊員或隊長解散
bool Worker::kickteam(long long n)
{
	if (n >= sa::MAX_TEAM)
		return false;

	if (n <= 0)
	{
		return setTeamState(false);
	}

	if (!getTeam(n).valid)
		return false;

	return lssproto_KTEAM_send(n);
}

//郵件或寵郵
bool Worker::mail(const QVariant& card, const QString& text, long long petIndex, const QString& itemName, const QString& itemMemo)
{
	long long index = -1;
	if (card.type() == QVariant::Type::Int || card.type() == QVariant::Type::LongLong)
		index = card.toLongLong();
	else if (card.type() == QVariant::Type::String)
	{
		bool isExact = true;
		QString name = card.toString();
		if (name.startsWith("?"))
		{
			name = name.mid(1);
			isExact = false;
		}

		QHash<long long, sa::address_bool_t> addressBooks = getAddressBooks();
		for (auto it = addressBooks.constBegin(); it != addressBooks.constEnd(); ++it)
		{
			if (!it.value().valid)
				continue;

			if (name.isEmpty())
				continue;

			if (it.value().name.isEmpty())
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

	if (index < 0 || index > sa::MAX_ADDRESS_BOOK)
		return false;

	sa::address_bool_t addressBook = getAddressBook(index);
	if (!addressBook.valid)
		return false;

	std::string sstr = util::fromUnicode(text);
	if (itemName.isEmpty() && itemMemo.isEmpty() && (petIndex < 0 || petIndex > sa::MAX_PET))
	{
		return lssproto_MSG_send(index, const_cast<char*>(sstr.c_str()), NULL);
	}

	if (!addressBook.onlineFlag)
		return false;

	long long itemIndex = getItemIndexByName(itemName, true, itemMemo, sa::CHAR_EQUIPSLOT_COUNT);

	sa::pet_t pet = getPet(petIndex);
	if (!pet.valid)
		return false;

	if (pet.state != sa::kMail)
	{
		if (!setPetState(petIndex, sa::kMail))
			return false;
	}

	if (!lssproto_PMSG_send(index, petIndex, itemIndex, const_cast<char*>(sstr.c_str()), NULL))
		return false;

	if (itemIndex < 0 || itemIndex >= sa::MAX_ITEM)
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

//加點
bool Worker::addPoint(long long skillid, long long amt)
{
	if (skillid < 0 || skillid > 4)
		return false;

	if (amt < 1)
		return false;

	sa::character_t pc = getCharacter();

	if (amt > pc.point)
		amt = pc.point;

	util::timer timer;
	for (long long i = 0; i < amt; ++i)
	{
		IS_WAITFOT_SKUP_RECV_.on();

		if (!lssproto_SKUP_send(skillid))
		{
			IS_WAITFOT_SKUP_RECV_.off();
			continue;
		}

		for (;;)
		{
			if (timer.hasExpired(1500))
				break;

			if (getBattleFlag())
				return false;

			if (!getOnlineFlag())
				return false;

			if (!IS_WAITFOT_SKUP_RECV_.get())
				break;
		}
		timer.restart();
	}

	return true;
}

//人物改名
bool Worker::setCharFreeName(const QString& name)
{
	std::string sname = util::fromUnicode(name);
	return lssproto_FT_send(const_cast<char*> (sname.c_str()));
}

//寵物改名
bool Worker::setPetFreeName(long long petIndex, const QString& name)
{
	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return false;

	std::string sname = util::fromUnicode(name);
	return lssproto_KN_send(petIndex, const_cast<char*> (sname.c_str()));
}
#pragma endregion

#pragma region PET
//設置寵物狀態 (戰鬥 | 等待 | 休息 | 郵件 | 騎乘)
bool Worker::setPetState(long long petIndex, sa::PetState state)
{
	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return false;

	if (getBattleFlag())
		return false;

	if (!getOnlineFlag())
		return false;

	updateDatasFromMemory();

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();

	{
		QWriteLocker locker(&petInfoLock_);
		//QWriteLocker locker2(&charInfoLock_);

		sa::pet_t pet = pet_.value(petIndex);
		if (!pet.valid)
			return false;

		sa::character_t pc = getCharacter();

		switch (state)
		{
		case sa::kBattle:
		{
			if (pc.battlePetNo != petIndex)
			{
				setFightPet(-1);
			}

			if (pc.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			setPetStandby(petIndex, state);

			setFightPet(petIndex);

			//mem::write<short>(hProcess, hModule + kOffsetBattlePetIndex, petIndex);
			//pc.selectPetNo[petIndex] = 1;
			//mem::write<short>(hProcess, hModule + kOffsetSelectPetArray + (petIndex * sizeof(short)), 1);

			break;
		}
		case sa::kStandby:
		{
			if (pc.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetBattlePetIndex, -1);
				pc.selectPetNo[petIndex] = 0;
				setCharacter(pc);
				mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc.selectPetNo[petIndex] = 1;
			setCharacter(pc);
			mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 1);
			setPetStateSub(petIndex, 1);
			setPetStandby(petIndex, state);
			break;
		}
		case sa::kMail:
		{
			if (pc.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetBattlePetIndex, -1);
				pc.selectPetNo[petIndex] = 0;
				setCharacter(pc);
				mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc.mailPetNo >= 0 && pc.mailPetNo != petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, -1);
				setPetStateSub(pc.mailPetNo, 0);
			}

			mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, petIndex);
			pc.selectPetNo[petIndex] = 0;
			setCharacter(pc);
			mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setPetStateSub(petIndex, 4);
			setPetStandby(petIndex, state);
			break;
		}
		case sa::kRest:
		{
			if (pc.ridePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetBattlePetIndex, -1);
				mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc.selectPetNo[petIndex] = 0;
			setCharacter(pc);
			mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setPetStateSub(petIndex, 0);
			setPetStandby(petIndex, state);
			break;
		}
		case sa::kRide:
		{
			if (pet.loyal != 100)
				break;

			if (pc.ridePetNo != -1)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetBattlePetIndex, -1);
				pc.selectPetNo[petIndex] = 0;
				setCharacter(pc);
				mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc.selectPetNo[petIndex] = 0;
			setCharacter(pc);
			mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setRidePet(petIndex);
			setPetStandby(petIndex, state);
			break;
		}
		case sa::kDouble:
		{
			if (pet.loyal != 100)
				break;

			if (pc.ridePetNo != -1)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
				setRidePet(-1);
			}

			if (pc.battlePetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetBattlePetIndex, -1);
				pc.selectPetNo[petIndex] = 0;
				setCharacter(pc);
				mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
				setFightPet(-1);
			}

			if (pc.mailPetNo == petIndex)
			{
				mem::write<short>(hProcess, hModule + sa::kOffsetMailPetIndex, -1);
				setPetStateSub(petIndex, 0);
			}

			pc.selectPetNo[petIndex] = 0;
			setCharacter(pc);
			mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + (petIndex * sizeof(short)), 0);
			setRidePet(petIndex);
			setFightPet(petIndex);
			break;
		}
		default:
			return false;
		}
	}

	updateDatasFromMemory();

	return true;
}

//根據當前數據重設所有寵物狀態
bool Worker::setAllPetState()
{
	QHash <long long, sa::pet_t> pet = getPets();
	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		long long state = 0;
		switch (pet.value(i).state)
		{
		case sa::kBattle:
			state = 1;
			break;
		case sa::kStandby:
			state = 1;
			break;
		case sa::kMail:
			state = 4;
			break;
		case sa::kRest:
			break;
		case sa::kRide:
			return setRidePet(i);
		}

		setPetState(i, pet[i].state);
	}

	return true;
}

//設置戰鬥寵物
bool Worker::setFightPet(long long petIndex)
{
	return lssproto_KS_send(petIndex);
}

//設置騎乘寵物
bool Worker::setRidePet(long long petIndex)
{
	QString str = QString("R|P|%1").arg(petIndex);
	std::string sstr = util::fromUnicode(str);
	return lssproto_FM_send(const_cast<char*>(sstr.c_str()));
}

//設置寵物狀態封包  0:休息 1:戰鬥或等待 4:郵件
bool Worker::setPetStateSub(long long petIndex, long long state)
{
	return lssproto_PETST_send(petIndex, state);
}

//設置寵物等待狀態
bool Worker::setPetStandby(long long petIndex, long long state)
{
	unsigned long long standby = 0;
	long long count = 0;
	sa::character_t pc = getCharacter();
	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		if ((state == 0 || state == 4) && petIndex == i)
			continue;

		if (pc.selectPetNo[i] > 0)
		{
			standby += 1ll << i;
			++count;
		}
	}

	if (!lssproto_SPET_send(standby))
		return false;

	pc.standbyPet = count;
	setCharacter(pc);
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	return mem::write<short>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetStandbyPetCount, count);
}

//丟棄寵物
bool Worker::dropPet(long long petIndex)
{
	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return false;

	sa::pet_t pet = getPet(petIndex);
	if (!pet.valid)
		return false;

	if (!lssproto_DP_send(getPoint(), petIndex))
		return false;

	updateDatasFromMemory();

	return true;
}

//自動鎖寵
void Worker::checkAutoLockPet()
{
	//qDebug() << "checkAutoLockPet";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.isGameInterruptionRequested())
		return;

	if (getBattleFlag())
		return;

	bool enableSwitchPet = GameDevice::getInstance(getIndex()).getEnableHash(util::kBattleAutoSwitchEnable);
	if (enableSwitchPet)
	{
		for (long long i = 0; i < sa::MAX_PET; ++i)
		{
			sa::pet_t pet = getPet(i);
			if (pet.state != sa::PetState::kRest)
				continue;

			setPetState(i, sa::kStandby);
		}
	}

	bool enableLockRide = gamedevice.getEnableHash(util::kLockRideEnable) && !gamedevice.getEnableHash(util::kLockPetScheduleEnable);
	bool enableLockPet = gamedevice.getEnableHash(util::kLockPetEnable) && !gamedevice.getEnableHash(util::kLockPetScheduleEnable);
	if (!enableLockRide && !enableLockPet)
		return;

	long long lockedIndex = -1;

	if (enableLockRide && enableLockPet && gamedevice.getValueHash(util::kLockRideValue) == gamedevice.getValueHash(util::kLockPetValue))
	{
		long long lockRideIndex = gamedevice.getValueHash(util::kLockRideValue);
		if (lockRideIndex >= 0 && lockRideIndex < sa::MAX_PET)
		{
			sa::pet_t pet = getPet(lockRideIndex);
			sa::character_t pc = getCharacter();
			if (pet.state != sa::PetState::kRide || pc.ridePetNo != pc.battlePetNo)
			{
				lockedIndex = lockRideIndex;
				setPetState(lockRideIndex, sa::kDouble);
			}

		}
	}
	else
	{
		if (enableLockRide)
		{
			long long lockRideIndex = gamedevice.getValueHash(util::kLockRideValue);
			if (lockRideIndex >= 0 && lockRideIndex < sa::MAX_PET)
			{
				sa::pet_t pet = getPet(lockRideIndex);
				if (pet.state != sa::PetState::kRide)
				{
					lockedIndex = lockRideIndex;
					setPetState(lockRideIndex, sa::kRide);
				}
			}
		}
		if (enableLockPet)
		{
			long long lockPetIndex = gamedevice.getValueHash(util::kLockPetValue);
			if (lockPetIndex >= 0 && lockPetIndex < sa::MAX_PET)
			{
				sa::pet_t pet = getPet(lockPetIndex);
				if (pet.state != sa::PetState::kBattle)
				{
					if (lockedIndex == lockPetIndex)
						return;

					setPetState(lockPetIndex, sa::kBattle);
				}

			}
		}
	}
}

//自動加點
void Worker::checkAutoAbility()
{
	qDebug() << "checkAutoAbility";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	auto checkEnable = [this, &gamedevice]()->bool
		{
			if (gamedevice.isGameInterruptionRequested())
				return false;

			if (!gamedevice.getEnableHash(util::kAutoAbilityEnable))
				return false;

			if (!getOnlineFlag())
				return false;

			if (getBattleFlag())
				return false;

			return true;
		};

	if (!checkEnable())
		return;

	QString strAbility = gamedevice.getStringHash(util::kAutoAbilityString);
	if (strAbility.isEmpty())
		return;

	QStringList abilityList = strAbility.split(util::rexOR, Qt::SkipEmptyParts);
	if (abilityList.isEmpty())
		return;

	static const QHash<QString, long long> abilityNameHash = {
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

		QStringList abilityInfo = ability.split(util::rexComma, Qt::SkipEmptyParts);
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

		long long value = abilityValue.toInt();
		if (value <= 0)
			continue;

		long long abilityIndex = abilityNameHash.value(abilityName, -1);
		if (abilityIndex == -1)
			continue;

		sa::character_t pc = getCharacter();
		const QVector<long long> abilitys = { pc.vit, pc.str, pc.tgh, pc.dex };
		long long abilityPoint = abilitys.value(abilityIndex, -1);
		if (abilityPoint == -1)
			continue;

		if (abilityPoint >= value)
			continue;

		long long abilityPointLeft = pc.point;
		if (abilityPointLeft <= 0)
			continue;

		long long abilityPointNeed = value - abilityPoint;
		if (abilityPointNeed > abilityPointLeft)
			abilityPointNeed = abilityPointLeft;

		addPoint(abilityIndex, abilityPointNeed);
	}
}

//檢查並自動吃肉、或丟肉
void Worker::checkAutoDropMeat()
{
	qDebug() << "checkAutoDropMeat";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	auto checkEnable = [this, &gamedevice]()->bool
		{
			if (gamedevice.isGameInterruptionRequested())
				return false;

			if (!gamedevice.getEnableHash(util::kAutoDropMeatEnable))
				return false;

			if (!getOnlineFlag())
				return false;

			if (getBattleFlag())
				return false;

			return true;
		};

	if (!checkEnable())
		return;

	constexpr const char* meat = "?肉";
	constexpr const char* memo = "耐久力";

	QVector<long long> items;
	if (!getItemIndexsByName(meat, "", &items, sa::CHAR_EQUIPSLOT_COUNT, sa::MAX_ITEM))
	{
		return;
	}

	for (const long long index : items)
	{
		if (!checkEnable())
			return;

		sa::item_t item = item_.value(index);

		if (!item.valid)
			continue;

		for (long long i = 0; i < item.stack; ++i)
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

//自動吃經驗加乘道具
void Worker::checkAutoEatBoostExpItem()
{
	qDebug() << "checkAutoEatBoostExpItem";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	auto checkEnable = [this, &gamedevice]()->bool
		{
			if (gamedevice.isGameInterruptionRequested())
				return false;

			if (!gamedevice.getEnableHash(util::kAutoEatBeanEnable))
				return false;

			if (!getOnlineFlag())
				return false;

			if (getBattleFlag())
				return false;

			return true;
		};

	if (!checkEnable())
		return;

	updateItemByMemory();
	QHash<long long, sa::item_t> items = getItems();
	for (long long i = 0; i < sa::MAX_ITEM; ++i)
	{
		if (!checkEnable())
			return;

		sa::item_t item = items.value(i);
		if (item.name.isEmpty() || item.memo.isEmpty() || !item.valid)
			continue;

		if (item.memo.contains("經驗值上升") || item.memo.contains("经验值上升"))
		{
			useItem(i, 0);
		}
	}
}

//自動丟棄道具
void Worker::checkAutoDropItems()
{
	qDebug() << "checkAutoDropItems";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	if (!gamedevice.getEnableHash(util::kAutoDropEnable))
		return;

	auto checkEnable = [this, &gamedevice]()->long long
		{
			if (gamedevice.isGameInterruptionRequested())
				return -1;

			if (!gamedevice.getEnableHash(util::kAutoDropEnable))
				return -1;

			if (!getOnlineFlag())
				return 0;

			if (getBattleFlag())
				return 0;

			return 1;
		};

	long long state = checkEnable();
	if (-1 == state)
		return;
	else if (0 == state)
		return;

	QString strDropItems = gamedevice.getStringHash(util::kAutoDropItemString);
	if (strDropItems.isEmpty())
		return;

	QStringList dropItems = strDropItems.split(util::rexOR, Qt::SkipEmptyParts);
	if (dropItems.isEmpty())
		return;

	updateItemByMemory();

	QHash<long long, sa::item_t> items = getItems();
	for (long long i = 0; i < sa::MAX_ITEM; ++i)
	{
		sa::item_t item = items.value(i);
		if (item.name.isEmpty())
			continue;

		for (const QString& cmpItem : dropItems)
		{
			if (checkEnable() != 1)
				break;

			if (cmpItem.isEmpty())
				continue;

			if (cmpItem.startsWith("?"))//如果丟棄列表名稱前面有?則表示要模糊匹配
			{
				QString newCmpItem = cmpItem.mid(1);//去除問號
				if (item.name.contains(newCmpItem))
				{
					dropItem(i);
					continue;
				}
			}
			else if ((item.name == cmpItem))//精確匹配
			{
				dropItem(i);
			}
		}
	}
}

void Worker::checkAutoDropPet()
{
	qDebug() << "checkAutoDropPet";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	auto checkStatus = [this, &gamedevice]()->bool
		{
			if (gamedevice.isGameInterruptionRequested())
				return false;

			if (!gamedevice.getEnableHash(util::kDropPetEnable))
				return false;

			if (!getOnlineFlag())
				return false;

			if (getBattleFlag())
				return false;

			return true;
		};


	if (gamedevice.isGameInterruptionRequested())
		return;

	if (!checkStatus())
		return;

	bool strLowAtEnable = gamedevice.getEnableHash(util::kDropPetStrEnable);
	bool defLowAtEnable = gamedevice.getEnableHash(util::kDropPetDefEnable);
	bool agiLowAtEnable = gamedevice.getEnableHash(util::kDropPetAgiEnable);
	bool aggregateLowAtEnable = gamedevice.getEnableHash(util::kDropPetAggregateEnable);
	double strLowAtValue = gamedevice.getValueHash(util::kDropPetStrValue);
	double defLowAtValue = gamedevice.getValueHash(util::kDropPetDefValue);
	double agiLowAtValue = gamedevice.getValueHash(util::kDropPetAgiValue);
	double aggregateLowAtValue = gamedevice.getValueHash(util::kDropPetAggregateValue);
	QString text = gamedevice.getStringHash(util::kDropPetNameString);
	QStringList nameList;
	if (!text.isEmpty())
		nameList = text.split(util::rexOR, Qt::SkipEmptyParts);

	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		if (checkStatus() != 1)
			break;

		sa::pet_t pet = getPet(i);
		if (!pet.valid || pet.maxHp <= 0 || pet.level <= 0)
			continue;

		double str = pet.atk;
		double def = pet.def;
		double agi = pet.agi;
		double aggregate = ((str + def + agi + (static_cast<double>(pet.maxHp) / 4.0)) / static_cast<double>(pet.level)) * 100.0;

		bool okDrop = false;
		if (strLowAtEnable && (str < strLowAtValue))
		{
			okDrop = true;
		}
		else if (defLowAtEnable && (def < defLowAtValue))
		{
			okDrop = true;
		}
		else if (agiLowAtEnable && (agi < agiLowAtValue))
		{
			okDrop = true;
		}
		else if (aggregateLowAtEnable && (aggregate < aggregateLowAtValue))
		{
			okDrop = true;
		}
		else
		{
			okDrop = false;
		}


		if (okDrop)
		{
			if (!nameList.isEmpty())
			{
				bool isExact = true;
				okDrop = false;
				for (const QString& it : nameList)
				{
					if (checkStatus() != 1)
						break;

					QString newName = it.simplified();
					if (newName.startsWith("?"))
					{
						isExact = false;
						newName = newName.mid(1);
					}

					if (isExact && pet.name == newName)
					{
						okDrop = true;
						break;
					}
					else if (isExact && pet.name.contains(newName))
					{
						okDrop = true;
						break;
					}
				}
			}

			if (okDrop)
				dropPet(i);
		}
	}
}

#pragma endregion

#pragma region MAP
//下載指定坐標 24 * 24 大小的地圖塊
bool Worker::downloadMap(long long x, long long y, long long floor)
{
	return lssproto_M_send(floor == -1 ? nowFloor_.get() : floor, x, y, x + 24, y + 24);
}

//下載全部地圖塊
bool Worker::downloadMap(long long floor)
{
	long long original = floor;

	if (floor == -1)
		floor = getFloor();

	sa::map_t map = {};
	long long currentIndex = getIndex();
	mapDevice.readFromBinary(currentIndex, floor, getFloorName());
	mapDevice.getMapDataByFloor(floor, &map);

	long long downloadMapXSize_ = map.width;
	long long downloadMapYSize_ = map.height;

	if (!downloadMapXSize_)
	{
		downloadMapXSize_ = 240;
	}

	if (!downloadMapYSize_)
	{
		downloadMapYSize_ = 240;
	}

	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	constexpr long long MAX_BLOCK_SIZE = 24;

	long long downloadMapX_ = 0;
	long long downloadMapY_ = 0;
	long long downloadCount_ = 0;

	const long long numBlocksX = (downloadMapXSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	const long long numBlocksY = (downloadMapYSize_ + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;
	const long long totalBlocks = numBlocksX * numBlocksY;
	qreal totalMapBlocks_ = static_cast<qreal>(totalBlocks);

	qreal downloadMapProgress_ = 0.0;

	QString title;
	std::wstring wtitle;

	announce(QString("floor %1 downloadMapXSize: %2 downloadMapYSize: %3 totalBlocks: %4")
		.arg(floor).arg(downloadMapXSize_).arg(downloadMapYSize_).arg(totalBlocks));
	util::timer timer;

	for (;;)
	{
		if (original != getFloor())
			return false;

		downloadMap(downloadMapX_, downloadMapY_, floor);

		long long blockWidth = qMin(MAX_BLOCK_SIZE, downloadMapXSize_ - downloadMapX_);
		long long blockHeight = qMin(MAX_BLOCK_SIZE, downloadMapYSize_ - downloadMapY_);

		// 移除一個小區塊
		downloadMapX_ += blockWidth;
		if (downloadMapX_ >= downloadMapXSize_)
		{
			downloadMapX_ = 0;
			downloadMapY_ += blockHeight;
		}

		// 更新下載進度
		++downloadCount_;
		downloadMapProgress_ = static_cast<qreal>(downloadCount_) / totalMapBlocks_ * 100.0;

		// 更新下載進度
		title = QString("downloading floor %1 - %2%").arg(floor).arg(util::toQString(downloadMapProgress_));

		if (downloadMapProgress_ >= 100.0)
		{
			break;
		}
	};

	//清空尋路地圖數據、數據重讀、圖像重繪
	mapDevice.clear(floor);
	announce(QString("floor %1 complete cost: %2 ms").arg(floor).arg(timer.cost()));
	announce(QString("floor %1 reload now").arg(floor));
	timer.restart();
	mapDevice.readFromBinary(currentIndex, floor, getFloorName(), false, true);
	announce(QString("floor %1 reload complete cost: %2 ms").arg(floor).arg(timer.cost()));
	return true;
}

//轉移
bool Worker::warp()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	struct
	{
		short ev = 0;
		short dialogid = 0;
	}ev;

	if (!mem::read(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetEV, sizeof(ev), &ev))
		return false;

	return lssproto_EV_send(ev.ev, ev.dialogid, getPoint(), -1);
}

//移動(封包) [a-h]
bool Worker::move(const QPoint& p, const QString& dir)
{
	if (p.x() < 0 || p.x() > 1500 || p.y() < 0 || p.y() > 1500)
		return false;

	std::string sdir = util::fromUnicode(dir);
	if (!lssproto_W2_send(p, const_cast<char*>(sdir.c_str())))
		return false;

	std::ignore = getPoint();
	return true;
}

//根據自身座標和目標座標計算出面相方位
static QString getDirByPoint(const QPoint& p, const QPoint& tp)
{
	QPoint c = tp - p;
	double r = atan2(c.x(), c.y()) / M_PI * 180.0;
	//八方位 北方為 a, 東北 b 東 c 東南 d 南 e 西南 f 西 g 西北 h
	QString dirStr;
	if (r <= -135 + 22.5 && r >= -135 - 22.5)
	{
		dirStr = "h";
	}
	else if (r <= -90 + 22.5 && r >= -90 - 22.5)
	{
		dirStr = "g";
	}
	else if (r <= -45 + 22.5 && r >= -45 - 22.5)
	{
		dirStr = "f";
	}
	else if (r <= 0 + 22.5 && r >= 0 - 22.5)
	{
		dirStr = "e";
	}
	else if (r <= 45 + 22.5 && r >= 45 - 22.5)
	{
		dirStr = "d";
	}
	else if (r <= 90 + 22.5 && r >= 90 - 22.5)
	{
		dirStr = "c";
	}
	else if (r <= 135 + 22.5 && r >= 135 - 22.5)
	{
		dirStr = "b";
	}
	else if (r < -135 - 22.5 || (r <= 180 + 22.5 && r >= 180 - 22.5))
	{
		dirStr = "a";
	}

	return dirStr;
}

//移動(記憶體)
bool Worker::move(const QPoint& p) const
{
	QMutexLocker locker(&moveLock_);
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.isValid())
		return false;

	return gamedevice.sendMessage(kSetMove, p.x(), p.y());
}

//單步移動(封包)
bool Worker::stepMove(const QPoint& p)
{
	QMutexLocker locker(&moveLock_);
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.isValid())
		return false;

	QPoint point = getPoint();
	QString dirStr = getDirByPoint(point, p);
	move(point, dirStr);

	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();
	mem::write<int>(hProcess, hModule + sa::kOffsetNowX, p.x());
	mem::write<int>(hProcess, hModule + sa::kOffsetNowY, p.y());
	mem::write<float>(hProcess, hModule + 0x416A644, (float)p.x() * 64.0);
	mem::write<float>(hProcess, hModule + 0x416A648, (float)p.y() * 64.0);
	mem::write<float>(hProcess, hModule + 0x4182998, (float)p.x() * 64.0);
	mem::write<float>(hProcess, hModule + 0x4182994, (float)p.y() * 64.0);
	return true;
}

//轉向指定坐標
long long Worker::setCharFaceToPoint(const QPoint& pos)
{
	QPoint c = pos - getPoint();
	double r = atan2(c.x(), c.y()) / M_PI * 180.0;
	long long dir = -1;
	if (r <= -135 + 22.5 && r >= -135 - 22.5)
	{
		dir = 7;
	}
	if (r <= -90 + 22.5 && r >= -90 - 22.5)
	{
		dir = 6;
	}
	if (r <= -45 + 22.5 && r >= -45 - 22.5)
	{
		dir = 5;
	}
	if (r <= 0 + 22.5 && r >= 0 - 22.5)
	{
		dir = 4;
	}
	if (r <= 45 + 22.5 && r >= 45 - 22.5)
	{
		dir = 3;
	}
	if (r <= 90 + 22.5 && r >= 90 - 22.5)
	{
		dir = 2;
	}
	if (r <= 135 + 22.5 && r >= 135 - 22.5)
	{
		dir = 1;
	}
	if (r < -135 - 22.5 || (r <= 180 + 22.5 && r >= 180 - 22.5))
	{
		dir = 0;
	}

	if (!setCharFaceDirection(dir))
		return -1;

	return dir;
}

//人物模型轉向(本地)
bool Worker::setCharModelDirection(long long dir) const
{
	//這裡是用來使遊戲動畫跟著轉向
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();
	long long newdir = (dir + 3) % sa::MAX_DIR;
	long long p = static_cast<long long>(mem::read<int>(hProcess, hModule + 0x422E3AC));
	if (p <= 0)
		return false;

	if (!mem::write<int>(hProcess, hModule + 0x422BE94, newdir))
		return false;

	return mem::write<int>(hProcess, p + 0x150, newdir);
}

//轉向 (根據方向索引自動轉換成A-H)
bool Worker::setCharFaceDirection(long long dir, bool noWindow)
{
	if (dir < 0 || dir >= sa::MAX_DIR)
		return false;

	const QString dirchr = "ABCDEFGH";
	if (dir >= dirchr.size())
		return false;

	QString dirStr = dirchr.at(dir);
	std::string sdirStr = util::fromUnicode(dirStr.toUpper());

	if (!lssproto_W2_send(getPoint(), const_cast<char*>(sdirStr.c_str())))
		return false;

	if (!noWindow)
	{
		if (!lssproto_L_send(dir))
			return false;
	}

	if (getBattleFlag())
		return false;

	return setCharModelDirection(dir);
}

//轉向 使用方位字符串
bool Worker::setCharFaceDirection(const QString& dirStr, bool noWindow)
{
	static const QHash<QString, QString> dirhash = {
		{ "北", "A" }, { "東北", "B" }, { "東", "C" }, { "東南", "D" },
		{ "南", "E" }, { "西南", "F" }, { "西", "G" }, { "西北", "H" },
		{ "北", "A" }, { "东北", "B" }, { "东", "C" }, { "东南", "D" },
		{ "南", "E" }, { "西南", "F" }, { "西", "G" }, { "西北", "H" }
	};

	long long dir = -1;
	QString qdirStr;
	const QString dirchr = "ABCDEFGH";
	if (!dirhash.contains(dirStr.toUpper()))
	{
		QRegularExpression re("[A-H]");
		QRegularExpressionMatch match = re.match(dirStr.toUpper());
		if (!match.hasMatch())
			return false;

		qdirStr = match.captured(0);
		dir = dirchr.indexOf(qdirStr, 0, Qt::CaseInsensitive);
	}
	else
	{
		dir = dirchr.indexOf(dirhash.value(dirStr), 0, Qt::CaseInsensitive);
		qdirStr = dirhash.value(dirStr);
	}

	std::string sdirStr = util::fromUnicode(qdirStr.toUpper());
	if (!lssproto_W2_send(getPoint(), const_cast<char*>(sdirStr.c_str())))
		return false;

	if (!noWindow)
	{
		if (!lssproto_L_send(dir))
			return false;
	}

	if (getBattleFlag())
		return false;

	return setCharModelDirection(dir);
}
#pragma endregion

#pragma region ITEM
//物品排序
void Worker::sortItem()
{
	updateItemByMemory();
	getCharMaxCarryingCapacity();

	long long j = 0;
	long long i = 0;
	QHash<long long, sa::item_t> items = getItems();
	sa::character_t pc = getCharacter();

	for (i = sa::MAX_ITEM - 1; i > sa::CHAR_EQUIPSLOT_COUNT; --i)
	{
		for (j = sa::CHAR_EQUIPSLOT_COUNT; j < i; ++j)
		{
			if (!items.value(i).valid)
				continue;

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
			if (items.value(j).stack > 1 && !itemStackFlagHash_.contains(key))
				itemStackFlagHash_.insert(key, true);
			else
			{
				swapItem(i, j);
				itemStackFlagHash_.insert(key, false);
				continue;
			}

			if (itemStackFlagHash_.contains(key) && !itemStackFlagHash_.value(key) && items.value(j).stack == 1)
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

//丟棄道具
bool Worker::dropItem(long long index)
{
	QPoint pos = getPoint();
	QHash<long long, sa::item_t> items = getItems();
	if (index == -1)
	{
		for (long long i = sa::CHAR_EQUIPSLOT_COUNT; i < sa::MAX_ITEM; ++i)
		{
			if (items.value(i).name.isEmpty() || !items.value(i).valid)
				continue;

			long long stack = items.value(i).stack;
			for (long long j = 0; j < stack; ++j)
			{
				if (!lssproto_DI_send(pos, i))
					return false;
			}

			IS_WAITOFR_ITEM_CHANGE_PACKET.inc();
		}
	}

	if (index < 0 || index >= sa::MAX_ITEM)
		return false;

	if (items.value(index).name.isEmpty() || !items.value(index).valid)
		return false;

	for (long long j = 0; j < items.value(index).stack; ++j)
	{
		if (!lssproto_DI_send(pos, index))
			return false;
	}

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

//批量丟棄道具
bool Worker::dropItem(QVector<long long> indexs)
{
	bool bret = true;
	for (const long long it : indexs)
		bret = dropItem(it);

	return bret;
}

//丟棄石幣
bool Worker::dropGold(long long gold)
{
	sa::character_t pc = getCharacter();
	if (gold > pc.gold)
		gold = pc.gold;

	return lssproto_DG_send(getPoint(), gold);
}

//使用道具
bool Worker::useItem(long long itemIndex, long long target)
{
	if (itemIndex < 0 || itemIndex >= sa::MAX_ITEM)
		return false;

	if (target < 0 || target > 9)
		return false;

	if (!getItem(itemIndex).valid)
		return false;

	if (!lssproto_ID_send(getPoint(), itemIndex, target))
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

//交換道具
bool Worker::swapItem(long long from, long long to)
{
	if (from < 0 || from >= sa::MAX_ITEM)
		return false;

	if (to < 0 || to >= sa::MAX_ITEM)
		return false;

	if (!lssproto_MI_send(from, to))
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

//撿道具
bool Worker::pickItem(long long dir)
{
	if (!setCharFaceDirection(dir))
		return false;

	return lssproto_PI_send(getPoint(), dir);
}

//穿裝 to = -1 丟裝 to = -2 脫裝 to = itemspotindex
bool Worker::petitemswap(long long petIndex, long long from, long long to)
{
	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return false;

	if (!lssproto_PetItemEquip_send(getPoint(), petIndex, from, to))
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

//料理/加工
bool Worker::craft(sa::CraftType type, const QStringList& ingres)
{
	if (ingres.size() < 2 || ingres.size() > 5)
		return false;

	QStringList itemIndexs;
	long long petIndex = -1;

	QString skillName;
	if (type == sa::CraftType::kCraftFood)
		skillName = QString("料理");
	else
		skillName = QString("加工");

	long long skillIndex = getPetSkillIndexByName(petIndex, skillName);
	if (petIndex == -1 || skillIndex == -1)
		return false;

	for (const QString& it : ingres)
	{
		long long index = getItemIndexByName(it, true, "", sa::CHAR_EQUIPSLOT_COUNT);
		if (index == -1)
			return false;

		itemIndexs.append(util::toQString(index));
	}

	QString qstr = itemIndexs.join("|");
	std::string str = util::fromUnicode(qstr);

	if (!lssproto_PS_send(petIndex, skillIndex, NULL, const_cast<char*>(str.c_str())))
		return false;

	IS_WAITOFR_ITEM_CHANGE_PACKET.inc();

	return true;
}

bool Worker::depositGold(long long gold, bool isPublic)
{
	if (gold <= 0)
		return false;

	QString qstr = QString("B|%1|%2").arg(!isPublic ? "G" : "T").arg(gold);
	std::string str = util::fromUnicode(qstr);
	return lssproto_FM_send(const_cast<char*>(str.c_str()));
}

bool Worker::withdrawGold(long long gold, bool isPublic)
{
	if (gold <= 0)
		return false;

	QString qstr = QString("B|%1|%2").arg(!isPublic ? "G" : "T").arg(-gold);
	std::string str = util::fromUnicode(qstr);
	return lssproto_FM_send(const_cast<char*>(str.c_str()));
}

//交易確認
bool Worker::tradeComfirm(const QString& name)
{
	if (!IS_TRADING.get())
		return false;

	if (name != opp_name)
	{
		std::ignore = tradeCancel();
		return false;
	}

	QString cmd = QString("T|%1|%2|C|confirm").arg(opp_sockfd).arg(opp_name);
	std::string scmd = util::fromUnicode(cmd);
	return lssproto_TD_send(const_cast<char*>(scmd.c_str()));
}

//交易取消
bool Worker::tradeCancel()
{
	if (!IS_TRADING.get())
		return false;

	QString cmd = QString("W|%1|%2").arg(opp_sockfd).arg(opp_name);
	std::string scmd = util::fromUnicode(cmd);

	if (!lssproto_TD_send(const_cast<char*>(scmd.c_str())))
		return false;

	clearTradeData();
	return true;
}

//交易開始
bool Worker::tradeStart(const QString& name, long long timeout)
{
	if (IS_TRADING.get())
	{
		qDebug() << "is not trading";
		return false;
	}

	if (!lssproto_TD_send(const_cast<char*>("D|D")))
	{
		qDebug() << "lssproto_TD_send fail";
		return false;
	}

	util::timer timer;
	for (;;)
	{
		if (!getOnlineFlag())
			return false;

		if (getBattleFlag())
			return false;

		if (timer.hasExpired(timeout))
			return false;

		if (IS_TRADING.get())
			break;

		QThread::msleep(200);
	}

	bool bret = opp_name == name;
	if (!bret)
	{
		qDebug() << "opp_name != name";
	}
	return bret;
}

//批量添加交易物品
long long Worker::tradeAppendItems(const QString& name, const QVector<long long>& itemIndexs)
{
	if (!IS_TRADING.get())
	{
		qDebug() << "is not trading";
		return -1;
	}

	if (itemIndexs.isEmpty())
	{
		qDebug() << "itemIndexs is empty";
		return -1;
	}

	if (name != opp_name)
	{
		qDebug() << "name != opp_name";
		return -1;
	}

	myitem_tradeList.clear();

	long long count = 0;
	QHash< long long, sa::item_t> items = getItems();
	for (long long i = sa::CHAR_EQUIPSLOT_COUNT; i < sa::MAX_ITEM; ++i)
	{
		bool bret = false;
		long long stack = items.value(i).stack;
		if (!itemIndexs.contains(i))
			continue;

		for (long long j = 0; j < stack; ++j)
		{
			QString cmd = QString("T|%1|%2|I|1|%3").arg(opp_sockfd).arg(opp_name).arg(i);
			std::string scmd = util::fromUnicode(cmd);

			if (!lssproto_TD_send(const_cast<char*>(scmd.c_str())))
			{
				qDebug() << "lssproto_TD_send fail";
				return -1;
			}

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

		++count;
	}

	return count;
}

//添加交易石幣
long long Worker::tradeAppendGold(const QString& name, long long gold)
{
	if (!IS_TRADING.get())
	{
		qDebug() << "is not trading";
		return -1;
	}

	sa::character_t pc = getCharacter();
	if (gold < 0 || gold > pc.gold)
	{
		qDebug() << "gold value" << gold << "is out of range:" << pc.gold;
		return -1;
	}

	if (name != opp_name)
	{
		qDebug() << "name != opp_name";
		return -1;
	}

	mygoldtrade = 0;

	QString cmd = QString("T|%1|%2|G|%3|%4").arg(opp_sockfd).arg(opp_name).arg(3).arg(gold);
	std::string scmd = util::fromUnicode(cmd);

	if (!lssproto_TD_send(const_cast<char*>(scmd.c_str())))
	{
		qDebug() << "lssproto_TD_send fail";
		return false;
	}

	mygoldtrade = gold;

	return mygoldtrade;
}

//批量添加交易寵物
long long Worker::tradeAppendPets(const QString& name, const QVector<long long>& petIndexs)
{
	if (!IS_TRADING.get())
	{
		qDebug() << "is not trading";
		return -1;
	}

	if (petIndexs.isEmpty())
	{
		qDebug() << "petIndexs is empty";
		return -1;
	}

	if (name != opp_name)
	{
		qDebug() << "name != opp_name";
		return -1;
	}

	//T|87| 02020202|P|3| 3 |  攻击|忠犬|料理|||||乌力斯坦|嘿嘿嘿嘿
	//T|%s| %s      |P|3| %d | %s
	mypet_tradeList = QStringList{ "P|-1", "P|-1", "P|-1" , "P|-1", "P|-1" };

	long long count = 0;
	for (const long long index : petIndexs)
	{
		if (index < 0 || index >= sa::MAX_PET)
			continue;

		sa::pet_t pet = getPet(index);
		if (!pet.valid)
			continue;

		QStringList list;
		QHash<long long, sa::pet_skill_t> petSkill = getPetSkills(index);
		for (const sa::pet_skill_t& it : petSkill)
		{
			if (!it.valid)
				list.append("");
			else
				list.append(it.name);
		}
		list.append(pet.name);
		list.append(pet.freeName);

		QString cmd = QString("T|%1|%2|P|3|%3|%4").arg(opp_sockfd).arg(opp_name).arg(index).arg(list.join("|"));
		std::string scmd = util::fromUnicode(cmd);

		if (!lssproto_TD_send(const_cast<char*>(scmd.c_str())))
		{
			qDebug() << "lssproto_TD_send fail";
			return false;
		}

		mypet_tradeList[index] = QString("P|%1").arg(index);
		++count;
	}

	return count;
}

//完成交易
bool Worker::tradeComplete(const QString& name)
{
	if (!IS_TRADING.get())
	{
		qDebug() << "is not trading";
		return false;
	}

	if (name != opp_name)
	{
		qDebug() << "name != opp_name";
		return false;
	}

	QStringList mytradeList;
	mytradeList.append(myitem_tradeList.join("|"));
	mytradeList.append(mypet_tradeList.join("|"));
	mytradeList.append(QString("G|%1").arg(mygoldtrade));

	QStringList opptradelist;
	for (const sa::show_item_t& it : opp_item)
	{
		if (it.name.isEmpty())
			opptradelist.append("I|-1");
		else
			opptradelist.append(QString("I|%1").arg(it.itemIndex));
	}

	for (const sa::show_pet_t& it : opp_pet)
	{
		if (it.opp_petname.isEmpty())
			opptradelist.append("P|-1");
		else
			opptradelist.append(QString("P|%1").arg(it.opp_petindex));
	}

	opptradelist.append(QString("G|%1").arg(tradeWndDropGoldGet));

	QString cmd = QString("T|%1|%2|K|%3|%4").arg(opp_sockfd).arg(opp_name).arg(mytradeList.join("|")).arg(opptradelist.join("|"));
	std::string scmd = util::fromUnicode(cmd);
	return lssproto_TD_send(const_cast<char*>(scmd.c_str()));

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
//真實時間轉SA時間
void Worker::realTimeToSATime(sa::ls_time_t* lstime)
{
	constexpr long long era = 912766409LL + 5400LL;
	//cary 十五
	long long lsseconds = (QDateTime::currentMSecsSinceEpoch() - firstServerTime_.get()) / 1000LL + serverTime_.get() - era;

	lstime->year = lsseconds / (sa::LSTIME_SECONDS_PER_DAY * sa::LSTIME_DAYS_PER_YEAR);

	long long lsdays = lsseconds / sa::LSTIME_SECONDS_PER_DAY;
	lstime->day = lsdays % sa::LSTIME_DAYS_PER_YEAR;


	/*(750*12)*/
	lstime->hour = (lsseconds % sa::LSTIME_SECONDS_PER_DAY) * sa::LSTIME_HOURS_PER_DAY / sa::LSTIME_SECONDS_PER_DAY;

	return;
}
//取SA時間區段
static sa::LSTimeSection getLSTime(sa::ls_time_t* lstime)
{
	if (sa::NIGHT_TO_MORNING < lstime->hour && lstime->hour <= sa::MORNING_TO_NOON)
	{
		return sa::kTimeMorning;
	}
	else if (sa::NOON_TO_EVENING < lstime->hour && lstime->hour <= sa::EVENING_TO_NIGHT)
	{
		return sa::kTimeEvening;
	}
	else if (sa::EVENING_TO_NIGHT < lstime->hour && lstime->hour <= sa::NIGHT_TO_MORNING)
	{
		return sa::kTimeNight;
	}
	else
		return sa::kTimeNoon;
}

#pragma endregion

#pragma region BattleAction

//設置戰鬥結束
void Worker::setBattleEnd()
{
	//if (!getBattleFlag())
	//	return;

	battleBackupThreadFlag_.on();

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.getEnableHash(util::kFastBattleEnable))//這裡不加限制的話，非快戰結束後會因為和客戶端EO重複發送導致周圍單位無法顯示
		lssproto_EO_send(0);
	eoTTLTimer_.restart();
	lssproto_Echo_send(const_cast<char*>("hoge"));

	long long battleDuation = battleDurationTimer.cost();
	if (battleDuation > 0ll)
		battle_total_time.add(battleDurationTimer.cost());
	setBattleFlag(false);

	normalDurationTimer.restart();

	//強退戰鬥畫面
	//if (getWorldStatus() == 10)
		//setGameStatus(7);

	bool enableLockRide = gamedevice.getEnableHash(util::kLockRideEnable) && !gamedevice.getEnableHash(util::kLockPetScheduleEnable);
	bool enableLockPet = gamedevice.getEnableHash(util::kLockPetEnable) && !gamedevice.getEnableHash(util::kLockPetScheduleEnable);
	if ((enableLockRide || enableLockPet))
		checkAutoLockPet();

	battlePetDisableList_.clear();

	waitfor_C_recv_.on();

	std::ignore = QtConcurrent::run([this]()
		{
			util::timer timer;
			GameDevice& gamedevice = GameDevice::getInstance(getIndex());
			long long W = 0;

			for (;;)
			{
				if (gamedevice.isGameInterruptionRequested())
					return;

				if (gamedevice.worker.isNull())
					return;

				if (getBattleFlag())
					return;

				if (!getOnlineFlag())
					return;

				W = getWorldStatus();
				if (W != 10)
					return;

				if (timer.hasExpired(5000))
					break;

				QThread::msleep(200);
			}

			if (W == 10)
			{
				setGameStatus(7);
			}

		});
}

//戰鬥BA包標誌位檢查
inline bool Worker::checkFlagState(long long pos) const
{
	if (pos < 0 || pos >= sa::MAX_ENEMY)
		return false;
	return util::checkAND(battleCurrentAnimeFlag.get(), 1ll << pos);
}

//異步處理自動/快速戰鬥邏輯和發送封包
#if 0
void Worker::doBattleWork(bool canDelay)
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (canDelay)
	{
		//紀錄動作前的回合
		long long recordedRound = battleCurrentRound.get();
		long long delay = gamedevice.getValueHash(util::kBattleActionDelayValue);
		long long resendDelay = gamedevice.getValueHash(util::kBattleResendDelayValue);
		if ((delay + resendDelay) >= 1000 && battleCurrentRound.get() > 0 && !battleBackupFuture_.isRunning() && gamedevice.worker->getBattleFlag())
		{
			battleBackupFuture_ = QtConcurrent::run([this, canDelay](long long round)
				{
					GameDevice& gamedevice = GameDevice::getInstance(getIndex());
					for (long long i = 0; i < 50; ++i)
					{
						QThread::msleep(200);
						if (gamedevice.isGameInterruptionRequested())
							return;

						if (gamedevice.worker.isNull())
							return;

						if (!gamedevice.worker->getBattleFlag())
							return;
					}

					//備用
					long long delay = gamedevice.getValueHash(util::kBattleActionDelayValue);
					long long resendDelay = gamedevice.getValueHash(util::kBattleResendDelayValue);
					if (resendDelay <= 0)
						return;

					util::timer timer;
					for (;;)
					{
						if (gamedevice.isGameInterruptionRequested())
							return;

						if (battleBackupThreadFlag_.get())
							return;

						bool fastChecked = gamedevice.getEnableHash(util::kFastBattleEnable);
						bool normalChecked = gamedevice.getEnableHash(util::kAutoBattleEnable);
						if (!fastChecked && !normalChecked)
							return;

						if (round != battleCurrentRound.get())
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

						QThread::msleep(200);
					}

					if (battleCharAlreadyActed.get())
						battleCharAlreadyActed.off();
					if (battlePetAlreadyActed.get())
						battlePetAlreadyActed.off();

					if (gamedevice.battleActionFuture.isRunning())
						return;

					gamedevice.battleActionFuture = QtConcurrent::run([this, canDelay]() { asyncBattleAction(canDelay); });
				}, recordedRound);
		}
	}

	if (gamedevice.battleActionFuture.isRunning())
		return;

	gamedevice.battleActionFuture = QtConcurrent::run([this, canDelay]() { asyncBattleAction(canDelay); });
}
#endif

//異步戰鬥動作處理
bool Worker::asyncBattleAction(bool canDelay)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.isGameInterruptionRequested())
		return false;

	if (!getOnlineFlag())
		return false;

	if (!getBattleFlag())
		return false;

	//自動戰鬥打開 或 快速戰鬥打開且處於戰鬥場景
	bool fastChecked = gamedevice.getEnableHash(util::kFastBattleEnable);//快速戰鬥是否開啟
	bool normalChecked = gamedevice.getEnableHash(util::kAutoBattleEnable);//自動戰鬥是否開啟
	bool fastEnabled = (getWorldStatus() == 9) && ((fastChecked) || (normalChecked));//快速戰鬥開啟且不處於戰鬥畫面
	bool normalEnabled = (getWorldStatus() == 10) && ((normalChecked) || (fastChecked));//自動戰鬥開啟且處於戰鬥畫面
	if (normalEnabled && !checkWG(10, 4) || (!fastEnabled && !normalEnabled))
	{
		return false;
	}

	auto delay = [&gamedevice, this]()
		{
			//戰鬥延時
			long long delay = gamedevice.getValueHash(util::kBattleActionDelayValue);
			if (delay <= 0)
				return;

			if (delay > 1000)
			{
				long long maxDelaySize = delay / 1000;
				for (long long i = 0; i < maxDelaySize; ++i)
				{
					QThread::msleep(1000);
					if (gamedevice.isGameInterruptionRequested())
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

	auto setCurrentRoundEnd = [this, &gamedevice, normalEnabled]()
		{
			//通知結束這一回合
			if (normalEnabled)
			{
				long long G = getGameStatus();
				if (G == 4)
				{
					setGameStatus(5);
					isBattleDialogReady.off();
				}
			}

			//這里不發的話一般戰鬥、和快戰都不會再收到後續的封包 (應該?)
			if (gamedevice.getEnableHash(util::kBattleAutoEOEnable))
				echo();
		};

	sa::battle_data_t bt = getBattleData();
	long long nret = -1;

	//人物和寵物分開發 TODO 修正多個BA人物多次發出戰鬥指令的問題
	if (!battleCharAlreadyActed.get())
	{
		if (canDelay)
			delay();
		//解析人物戰鬥邏輯並發送指令
		nret = playerDoBattleWork(bt);
		if (1 == nret)
			battleCharAlreadyActed.on();
	}

	//TODO 修正寵物指令在多個BA時候重覆發送的問題
	if (!battlePetAlreadyActed.get())
	{
		long long nret = petDoBattleWork(bt);
		if (1 == nret || -1 == nret)
		{
			battlePetAlreadyActed.on();
			setCurrentRoundEnd();
		}

		return nret == 1;
	}

	return false;
}

//人物戰鬥
long long Worker::playerDoBattleWork(const sa::battle_data_t& bt)
{
	if (sa::hasUnMoveableStatus(bt.objects.value(battleCharCurrentPos.get()).status))
	{
		sendBattleCharDoNothing();
		return 1;
	}

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	do
	{
		if (bt.objects.isEmpty() || bt.enemies.isEmpty() || bt.allies.isEmpty() || bt.objects.value(battleCharCurrentPos.get()).hp <= 0)
		{
			sendBattleCharDoNothing();
			break;
		}

		if (battleCharCurrentPos.get() >= sa::MAX_ENEMY
			|| util::checkAND(battleBpFlag.get(), sa::BATTLE_BP_PLAYER_MENU_NON)/*觀戰*/)
		{
			sendBattleCharDoNothing();
			break;
		}

		//自動逃跑
		if (gamedevice.getEnableHash(util::kAutoEscapeEnable))
		{
			sendBattleCharEscapeAct();
			break;
		}

		if (!handleCharBattleLogics(bt))
			return 0;

	} while (false);

	return 1;
}

//寵物戰鬥
long long Worker::petDoBattleWork(const sa::battle_data_t& bt)
{
	sa::character_t pc = getCharacter();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= sa::MAX_PET)
		return -1;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	do
	{
		if (bt.objects.isEmpty() || bt.enemies.isEmpty() || bt.allies.isEmpty() || bt.objects.value(battleCharCurrentPos.get() + 5).hp <= 0)
		{
			sendBattlePetDoNothing();
			break;
		}

		//自動逃跑
		if (sa::hasUnMoveableStatus(bt.objects.value(battleCharCurrentPos.get() + 5).status)
			|| gamedevice.getEnableHash(util::kAutoEscapeEnable)
			|| petEnableEscapeForTemp_.get()
			|| util::checkAND(battleBpFlag.get(), sa::BATTLE_BP_PET_MENU_NON))
		{
			sendBattlePetDoNothing();
			break;
		}

		if (!handlePetBattleLogics(bt))
			return 0;

	} while (false);
	//mem::writeInt(gamedevice.getProcess(), gamedevice.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//mem::writeInt(gamedevice.getProcess(), gamedevice.getProcessModule() + 0xE21E8, 1, sizeof(short));
	return 1;
}

//檢查人物血量
bool Worker::checkCharHp(long long cmpvalue, long long* target, bool useequal) const
{
	sa::character_t pc = getCharacter();
	if (useequal && (pc.hpPercent <= cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.get();
		return true;
	}
	else if (!useequal && (pc.hpPercent < cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.get();
		return true;
	}

	return false;
};

//檢查人物氣力
bool Worker::checkCharMp(long long cmpvalue, long long* target, bool useequal) const
{
	sa::character_t pc = getCharacter();
	if (useequal && (pc.mpPercent <= cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.get();
		return true;
	}
	else if (!useequal && (pc.mpPercent < cmpvalue))
	{
		if (target)
			*target = battleCharCurrentPos.get();
		return true;
	}

	return  false;
};

//檢測戰寵血量
bool Worker::checkPetHp(long long cmpvalue, long long* target, bool useequal) const
{
	sa::character_t pc = getCharacter();

	long long i = pc.battlePetNo;
	if (i < 0 || i >= sa::MAX_PET)
		return false;

	sa::pet_t pet = getPet(i);

	if (useequal && (pet.hpPercent <= cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.get() + 5;
		return true;
	}
	else if (!useequal && (pet.hpPercent < cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.get() + 5;
		return true;
	}

	return false;
};

//檢測騎寵血量
bool Worker::checkRideHp(long long cmpvalue, long long* target, bool useequal) const
{
	sa::character_t pc = getCharacter();

	long long i = pc.ridePetNo;
	if (i < 0 || i >= sa::MAX_PET)
		return false;

	sa::pet_t pet = getPet(i);

	if (useequal && (pet.hpPercent <= cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.get();
		return true;
	}
	else if (!useequal && (pet.hpPercent < cmpvalue) && (pet.level > 0) && (pet.maxHp > 0))
	{
		if (target)
			*target = battleCharCurrentPos.get();
		return true;
	}

	return false;
};

//檢測隊友血量
bool Worker::checkTeammateHp(long long cmpvalue, long long* target) const
{
	if (!target)
		return false;

	sa::character_t pc = getCharacter();
	if (!util::checkAND(pc.status, sa::kCharacterStatus_HasTeam) && !util::checkAND(pc.status, sa::kCharacterStatus_IsLeader))
		return false;

	QHash<long long, sa::team_t> team = getTeams();
	for (long long i = 0; i < sa::MAX_TEAM; ++i)
	{
		if (team.value(i).hpPercent < cmpvalue && team.value(i).level > 0 && team.value(i).maxHp > 0 && team.value(i).valid)
		{
			*target = i;
			return true;
		}
	}

	return false;
}

//檢查是否寵物是否已滿
bool Worker::isPetSpotEmpty() const
{
	QHash <long long, sa::pet_t> pet = getPets();
	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		if ((pet.value(i).level <= 0) || (pet.value(i).maxHp <= 0) || (!pet.value(i).valid))
			return false;
	}

	return true;
}

//根據用戶輸入的條件式匹配戰場上的目標
bool Worker::matchBattleTarget(const QVector<sa::battle_object_t>& btobjs, BattleMatchType matchtype, long long firstMatchPos, QString op, QVariant cmpvar, long long* ppos)
{
	auto cmp = [op](QVariant a, QVariant b)
		{
			QString aStr = a.toString();
			QString bStr = b.toString();

			bool aNumOk = false, bNumOk = false;
			long long bNum = b.toLongLong(&aNumOk);
			long long aNum = a.toLongLong(&bNumOk);

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

			QCollator collator = util::getCollator();

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

	auto it = std::find_if(btobjs.begin(), btobjs.end(), [this, &cmp, firstMatchPos, matchtype, cmpvar, op](const sa::battle_object_t& obj)
		{
			bool isValidEnemy = obj.hp > 0 && obj.maxHp > 0 && !util::checkAND(obj.status, sa::BC_FLG_HIDE) && !util::checkAND(obj.status, sa::BC_FLG_DEAD);
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
bool Worker::conditionMatchTarget(QVector<sa::battle_object_t> btobjs, const QString& conditionStr, long long* ppos)
{
	long long target = -1;
	QStringList targetList = conditionStr.split(util::rexOR, Qt::SkipEmptyParts);
	if (targetList.isEmpty())
		return false;

	QVector<sa::battle_object_t> tempbattleObjects;

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

	auto matchCondition = [this, &btobjs](QString src, long long firstMatchPos, long long* ppos)->bool
		{
			src = src.toUpper();
			BattleMatchType matchType = BattleMatchType::MatchNotUsed;
			//匹配環境變量
			for (const QString& it : hash.keys())
			{
				if (src.contains(it, Qt::CaseInsensitive))
				{
					//開頭為A切換成友方數據
					if (src.startsWith("A"))
						btobjs = getBattleData().allies;

					matchType = hash.value(it.toUpper());
					src.remove(it, Qt::CaseInsensitive);
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
			long long num = str.toLongLong(&ok);
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
		newIt.remove(" ");
		QStringList andList = newIt.split("&&", Qt::SkipEmptyParts);
		if (andList.isEmpty())
			continue;

		//依照包含的順序排序 > %(EPOS) > %(ELV) > %(EHP) > %(EMAXHP) > %(EHPP) > %(EMOD) > %(ENAME) > %(ESTATUS) 
		std::sort(andList.begin(), andList.end(), [](const QString& a, const QString& b)
			{
				long long aIndex = -1, bIndex = -1;
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

		long long passCount = 0;
		//當passCount == andList.size()時，表示所有條件都通過
		long long firstMatchPos = -1; //後續每個符合條件的都要是相同的pos

		for (const QString& andIt : andList)
		{
			long long tempTarget = -1;
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

//lua托管戰鬥
bool Worker::runBattleLua(BattleScriptType type)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	bool bret = false;

	do
	{
		if (!gamedevice.getEnableHash(util::kBattleLuaModeEnable))
			break;

		std::string sscript;
		if (kCharScript == type)
			sscript = util::toConstData(battleCharLuaScript_);
		else
			sscript = util::toConstData(battlePetLuaScript_);

		bret = battleLua->doString(sscript);

	} while (false);
	return bret;
}

//人物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
bool Worker::handleCharBattleLogics(const sa::battle_data_t& bt)
{
	using namespace util;
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	tempCatchPetTargetIndex_.set(-1);
	petEnableEscapeForTemp_.off(); //用於在必要的時候切換戰寵動作為逃跑模式

	if (runBattleLua(kCharScript))
		return true;

	QStringList items;

	QVector<sa::battle_object_t> battleObjects = bt.enemies;
	QVector<sa::battle_object_t> tempbattleObjects;

	sortBattleUnit(battleObjects);

	long long target = -1;

	//檢測隊友血量
#pragma region CharBattleTools
	auto checkAllieHp = [this, &bt](long long cmpvalue, long long* target, bool useequal)->bool
		{
			if (!target)
				return false;

			long long min = 0;
			long long max = (sa::MAX_ENEMY / 2) - 1;
			if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
			{
				min = sa::MAX_ENEMY / 2;
				max = sa::MAX_ENEMY - 1;
			}

			QVector<sa::battle_object_t> battleObjects = bt.objects;
			for (const sa::battle_object_t& obj : battleObjects)
			{
				if (obj.pos < min || obj.pos > max)
					continue;

				if (obj.hp == 0)
					continue;

				if (obj.maxHp == 0)
					continue;

				if (util::checkAND(obj.status, sa::BC_FLG_HIDE) || util::checkAND(obj.status, sa::BC_FLG_DEAD))
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

	auto checkDeadAllie = [this, &bt](long long* target)->bool
		{
			if (nullptr == target)
				return false;

			long long min = 0;
			long long max = (sa::MAX_ENEMY / 2) - 1;
			if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
			{
				min = sa::MAX_ENEMY / 2;
				max = sa::MAX_ENEMY - 1;
			}

			QVector<sa::battle_object_t> battleObjects = bt.objects;
			for (const sa::battle_object_t& obj : battleObjects)
			{
				if (obj.pos < min || obj.pos > max)
					continue;

				if ((obj.level > 0) && (obj.maxHp > 0) && ((obj.hp == 0) || util::checkAND(obj.status, sa::BC_FLG_DEAD)))
				{
					*target = obj.pos;
					return true;
				}
			}

			return false;
		};

	//檢測隊友狀態
	auto checkAllieStatus = [this, &bt](long long* target, bool useequal)->bool
		{
			if (!target)
				return false;

			long long min = 0;
			long long max = (sa::MAX_ENEMY / 2) - 1;
			if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
			{
				min = sa::MAX_ENEMY / 2;
				max = sa::MAX_ENEMY - 1;
			}


			QVector<sa::battle_object_t> battleObjects = bt.objects;
			for (const sa::battle_object_t& obj : battleObjects)
			{
				if (obj.pos < min || obj.pos > max)
					continue;

				if (obj.hp == 0)
					continue;

				if (obj.maxHp == 0)
					continue;

				if (util::checkAND(obj.status, sa::BC_FLG_HIDE) || util::checkAND(obj.status, sa::BC_FLG_DEAD))
					continue;

				if (!useequal && sa::hasBadStatus(obj.status))
				{
					*target = obj.pos;
					return true;
				}
				else if (useequal && sa::hasBadStatus(obj.status))
				{
					*target = obj.pos;
					return true;
				}
			}

			return false;
		};
#pragma endregion

#pragma region Actions
	//落馬逃跑
#pragma region FallDownEscape
	auto fallDownEscapeFun = [this, &gamedevice, &bt]()->bool
		{
			do
			{
				bool fallEscapeEnable = gamedevice.getEnableHash(util::kFallDownEscapeEnable);
				if (!fallEscapeEnable)
					break;

				if (bt.objects.value(battleCharCurrentPos.get()).rideFlag != 0)
					break;

				if (sendBattleCharEscapeAct())
				{
					petEnableEscapeForTemp_.on();
					return true;
				}
			} while (false);
			return false;
		};
#pragma endregion

	//鎖定逃跑
#pragma region LockEscape
	auto lockEscapeFun = [this, &gamedevice, &battleObjects, &tempbattleObjects]()->bool
		{
			do
			{
				bool lockEscapeEnable = gamedevice.getEnableHash(util::kLockEscapeEnable);
				if (!lockEscapeEnable)
					break;

				QString text = gamedevice.getStringHash(util::kLockEscapeString);
				QStringList targetList = text.split(util::rexOR, Qt::SkipEmptyParts);
				if (targetList.isEmpty())
					break;

				for (const QString& it : targetList)
				{
					if (matchBattleEnemyByName(it, true, battleObjects, &tempbattleObjects))
					{
						if (sendBattleCharEscapeAct())
						{
							petEnableEscapeForTemp_.on();
							return true;
						}
					}
				}
			} while (false);

			return false;
		};
#pragma endregion

	//鎖定攻擊
#pragma region LockAttack
	auto lockAttackFun = [this, &gamedevice, &battleObjects, &target]()->bool
		{
			do
			{
				bool lockAttackEnable = gamedevice.getEnableHash(util::kLockAttackEnable);
				if (!lockAttackEnable)
					break;

				QString text = gamedevice.getStringHash(util::kLockAttackString);
				if (text.isEmpty())
					break;

				if (conditionMatchTarget(battleObjects, text, &target))
				{
					//這個標誌是為了確保目標死亡之後還會繼續戰鬥，而不是打完後就逃跑
					IS_LOCKATTACK_ESCAPE_DISABLE_.on();
					break;
				}

				//鎖定攻擊條件不滿足時，是否不藥逃跑
				bool doNotEscape = gamedevice.getEnableHash(util::kBattleNoEscapeWhileLockPetEnable);

				if (!doNotEscape && IS_LOCKATTACK_ESCAPE_DISABLE_.get())
					break;

				if (doNotEscape)
					break;

				if (sendBattleCharEscapeAct())
				{
					petEnableEscapeForTemp_.on();
					return true;
				}
			} while (false);

			return false;
		};
#pragma endregion

	//道具復活
#pragma region ItemRevive
	auto itemReviveFun = [this, &gamedevice, &bt, &checkDeadAllie, &items, &target]()->bool
		{
			do
			{
				bool itemRevive = gamedevice.getEnableHash(util::kBattleItemReviveEnable);
				if (!itemRevive)
					break;

				long long tempTarget = -1;
				bool ok = false;
				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleItemReviveTargetValue);
				if (util::checkAND(targetFlags, kSelectPet))
				{
					sa::battle_object_t obj = bt.objects.value(battleCharCurrentPos.get() + 5);
					if (obj.maxHp > 0 && obj.level > 0 && (obj.hp == 0
						|| util::checkAND(obj.status, sa::BC_FLG_DEAD)))
					{
						tempTarget = battleCharCurrentPos.get() + 5;
						ok = true;
					}
				}

				if (!ok)
				{
					if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
					{
						if (checkDeadAllie(&tempTarget))
						{
							ok = true;
						}
					}
					else
					{
						for (long long i = sa::MAX_ENEMY - 1; i >= 10; --i)
						{
							sa::battle_object_t obj = bt.objects.value(i - 10);
							if (util::checkAND(targetFlags, 1LL << i)
								&& obj.level > 0 && obj.maxHp > 0 && (obj.hp == 0 || util::checkAND(obj.status, sa::BC_FLG_DEAD)))
							{
								tempTarget = i - 10;
								ok = true;
								break;
							}
						}

						if (!ok)
							break;
					}
				}

				if (!ok)
					break;

				QString text = gamedevice.getStringHash(util::kBattleItemReviveItemString).simplified();
				if (text.isEmpty())
					break;

				items = text.split(util::rexOR, Qt::SkipEmptyParts);
				if (items.isEmpty())
					break;

				long long itemIndex = -1;
				for (const QString& str : items)
				{
					itemIndex = getItemIndexByName(str, true, "", sa::CHAR_EQUIPSLOT_COUNT);
					if (itemIndex != -1)
						break;
				}

				if (itemIndex == -1)
					break;

				target = -1;
				if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0))
					return sendBattleCharItemAct(itemIndex, target);
			} while (false);

			return false;
		};
#pragma endregion

	//指定回合
#pragma region SelectedRound
	auto selectRoundFun = [this, &gamedevice, &bt, &target]()->bool
		{
			do
			{
				long long atRoundIndex = gamedevice.getValueHash(util::kBattleCharRoundActionRoundValue);
				if (atRoundIndex <= 0)
					break;

				if (atRoundIndex != battleCurrentRound.get() + 1)
					break;

				long long tempTarget = -1;
				///bool ok = false;

				long long enemy = gamedevice.getValueHash(util::kBattleCharRoundActionEnemyValue);
				if (enemy != 0)
				{
					if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
					{
						break;
					}
				}

				long long level = gamedevice.getValueHash(util::kBattleCharRoundActionLevelValue);
				if (level != 0)
				{
					auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
						[](const sa::battle_object_t& obj1, const sa::battle_object_t& obj2)
						{
							return obj1.level < obj2.level;
						});

					if (minIt->level <= (level * 10))
					{
						break;
					}
				}

				long long actionType = gamedevice.getValueHash(util::kBattleCharRoundActionTypeValue);
				if (actionType == 1)
				{
					return sendBattleCharDefenseAct();
				}
				else if (actionType == 2)
				{
					return sendBattleCharEscapeAct();
				}

				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleCharRoundActionTargetValue);
				QHash<long long, long long> tagetHash = {
					{ kSelectEnemyAny, target == -1 ? getBattleSelectableEnemyTarget(bt) : target },
					{ kSelectEnemyAll, sa::TARGET_SIDE_1 },
					{ kSelectEnemyFront, getBattleSelectableEnemyOneRowTarget(bt, true) },
					{ kSelectEnemyBack, getBattleSelectableEnemyOneRowTarget(bt, false) },
					{ kSelectSelf, battleCharCurrentPos.get() },
					{ kSelectPet, battleCharCurrentPos.get() + 5 },
					{ kSelectAllieAny, getBattleSelectableAllieTarget(bt) },
					{ kSelectAllieAll, sa::TARGET_SIDE_0 },
				};

				tempTarget = tagetHash.value(targetFlags, -1);
				if (-1 == tempTarget)
				{
					for (long long i = 10; i < sa::MAX_ENEMY; ++i)
					{
						if (!util::checkAND(targetFlags, 1LL << i))
							continue;

						tempTarget = i - 10;
						break;
					}

					if (-1 == tempTarget)
						break;
				}

				if (actionType == 0)
				{
					if (tempTarget >= 0 && tempTarget < sa::MAX_ENEMY)
					{
						return sendBattleCharAttackAct(tempTarget);
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						if (tempTarget == -1)
							break;

						return sendBattleCharAttackAct(tempTarget);
					}
				}
				else
				{
					long long magicIndex = actionType - 3;
					bool isProfession = magicIndex > (sa::MAX_MAGIC - 1);
					if (isProfession) //0 ~ MAX_PROFESSION_SKILL
					{
						magicIndex -= sa::MAX_MAGIC;

						if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0))
						{
							if (isCharMpEnoughForSkill(magicIndex))
							{
								return sendBattleCharJobSkillAct(magicIndex, target);
							}
							else
							{
								tempTarget = getBattleSelectableEnemyTarget(bt);
								return sendBattleCharAttackAct(tempTarget);
							}
						}
					}
					else
					{

						if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0))
						{
							if (isCharMpEnoughForMagic(magicIndex))
							{
								return sendBattleCharMagicAct(magicIndex, target);
							}
							else
							{
								break;
							}
						}
					}
				}
			} while (false);

			return false;
		};
#pragma endregion

	//精靈補血
#pragma region MagicHeal
	auto magicHealFun = [this, &gamedevice, &bt, &checkAllieHp, &target]()->bool
		{
			do
			{
				bool magicHeal = gamedevice.getEnableHash(util::kBattleMagicHealEnable);
				if (!magicHeal)
					break;

				long long tempTarget = -1;
				bool ok = false;
				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleMagicHealTargetValue);
				long long charPercent = gamedevice.getValueHash(util::kBattleMagicHealCharValue);
				long long petPercent = gamedevice.getValueHash(util::kBattleMagicHealPetValue);
				long long alliePercent = gamedevice.getValueHash(util::kBattleMagicHealAllieValue);

				if (util::checkAND(targetFlags, kSelectSelf))
				{
					if (checkCharHp(charPercent, &tempTarget))
					{
						ok = true;
					}
					else if (!ok && bt.objects.value(battleCharCurrentPos.get()).rideMaxHp > 0)
					{
						if (bt.objects.value(battleCharCurrentPos.get()).rideHpPercent <= petPercent
							&& bt.objects.value(battleCharCurrentPos.get()).rideHp > 0
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_DEAD)
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_HIDE))
						{
							tempTarget = battleCharCurrentPos.get();
							ok = true;
						}
					}
				}

				if (util::checkAND(targetFlags, kSelectPet))
				{
					if (!ok && bt.objects.value(battleCharCurrentPos.get() + 5).maxHp > 0)
					{
						if (bt.objects.value(battleCharCurrentPos.get() + 5).hpPercent <= petPercent
							&& bt.objects.value(battleCharCurrentPos.get() + 5).hp > 0
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_DEAD)
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_HIDE))
						{
							tempTarget = battleCharCurrentPos.get() + 5;
							ok = true;
						}
					}
					else if (!ok && bt.objects.value(battleCharCurrentPos.get()).rideMaxHp > 0)
					{
						if (bt.objects.value(battleCharCurrentPos.get()).rideHpPercent <= petPercent
							&& bt.objects.value(battleCharCurrentPos.get()).rideHp > 0
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_DEAD)
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_HIDE))
						{
							tempTarget = battleCharCurrentPos.get();
							ok = true;
						}
					}
				}

				if (!ok)
				{
					if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
					{
						if (checkAllieHp(alliePercent, &tempTarget, false))
						{
							ok = true;
						}
					}
				}

				if (!ok)
					break;

				long long magicIndex = gamedevice.getValueHash(util::kBattleMagicHealMagicValue) - 3;
				if (magicIndex < 0 || magicIndex > sa::MAX_MAGIC)
					break;

				bool isProfession = magicIndex > (sa::MAX_MAGIC - 1);
				if (!isProfession) // ifMagic
				{
					target = -1;
					if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0))
					{
						if (isCharMpEnoughForMagic(magicIndex))
						{
							return sendBattleCharMagicAct(magicIndex, target);
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					magicIndex -= sa::MAX_MAGIC;
					if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0))
					{
						if (isCharMpEnoughForSkill(magicIndex))
						{
							return sendBattleCharJobSkillAct(magicIndex, target);
						}
						else
						{
							break;
						}
					}
				}
			} while (false);

			return false;
		};
#pragma endregion

	//道具補血
#pragma region ItemHeal
	auto itemHealFun = [this, &gamedevice, &bt, &checkAllieHp, &items, &target]()->bool
		{
			do
			{
				bool itemHeal = gamedevice.getEnableHash(util::kBattleItemHealEnable);
				if (!itemHeal)
					break;

				long long tempTarget = -1;
				bool ok = false;

				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleItemHealTargetValue);
				long long charPercent = gamedevice.getValueHash(util::kBattleItemHealCharValue);
				long long petPercent = gamedevice.getValueHash(util::kBattleItemHealPetValue);
				long long alliePercent = gamedevice.getValueHash(util::kBattleItemHealAllieValue);
				if (util::checkAND(targetFlags, kSelectSelf))
				{
					if (checkCharHp(charPercent, &tempTarget))
					{
						ok = true;
					}
				}

				if (!ok)
				{
					if (util::checkAND(targetFlags, kSelectPet) && bt.objects.value(battleCharCurrentPos.get() + 5).maxHp > 0)
					{
						if (bt.objects.value(battleCharCurrentPos.get() + 5).hpPercent <= petPercent
							&& bt.objects.value(battleCharCurrentPos.get() + 5).hp > 0
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_DEAD)
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_HIDE))
						{
							tempTarget = battleCharCurrentPos.get() + 5;
							ok = true;
						}
					}
				}

				if (!ok)
				{
					if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
					{
						if (checkAllieHp(alliePercent, &tempTarget, false))
						{
							ok = true;
						}
					}
				}

				if (!ok)
					break;

				long long itemIndex = -1;
				bool meatProiory = gamedevice.getEnableHash(util::kBattleItemHealMeatPriorityEnable);
				if (meatProiory)
				{
					itemIndex = getItemIndexByName("?肉", false, "耐久力", sa::CHAR_EQUIPSLOT_COUNT);
				}

				if (itemIndex == -1)
				{
					QString text = gamedevice.getStringHash(util::kBattleItemHealItemString).simplified();
					if (text.isEmpty())
						break;

					items = text.split(util::rexOR, Qt::SkipEmptyParts);
					if (items.isEmpty())
						break;

					for (const QString& str : items)
					{
						itemIndex = getItemIndexByName(str, true, "", sa::CHAR_EQUIPSLOT_COUNT);
						if (itemIndex != -1)
							break;
					}
				}

				if (itemIndex == -1)
					break;

				target = -1;
				if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0))
				{
					return sendBattleCharItemAct(itemIndex, target);
				}
			} while (false);

			return false;
		};
#pragma endregion

	//嗜血補氣
#pragma region SkillMp
	auto skillMpFun = [this, &gamedevice]()->bool
		{
			do
			{
				bool skillMp = gamedevice.getEnableHash(util::kBattleSkillMpEnable);
				if (!skillMp)
					break;

				long long charMp = gamedevice.getValueHash(util::kBattleSkillMpValue);
				if ((battleCharCurrentMp.get() > charMp)
					&& (battleCharCurrentMp.get() > 0))
					break;

				long long skillIndex = getProfessionSkillIndexByName("?成性");
				if (skillIndex < 0)
					break;

				if (isCharHpEnoughForSkill(skillIndex))
				{
					return sendBattleCharJobSkillAct(skillIndex, battleCharCurrentPos.get());
				}

			} while (false);

			return false;
		};
#pragma endregion

	//道具補氣
#pragma region ItemMp
	auto itemMpFun = [this, &gamedevice, &items, &target]()->bool
		{
			do
			{
				bool itemHealMp = gamedevice.getEnableHash(util::kBattleItemHealMpEnable);
				if (!itemHealMp)
					break;

				long long tempTarget = -1;
				//bool ok = false;
				long long charMpPercent = gamedevice.getValueHash(util::kBattleItemHealMpValue);
				if (!checkCharMp(charMpPercent, &tempTarget, true) && (battleCharCurrentMp.get() > 0))
				{
					break;
				}

				QString text = gamedevice.getStringHash(util::kBattleItemHealMpItemString).simplified();
				if (text.isEmpty())
					break;

				items = text.split(util::rexOR, Qt::SkipEmptyParts);
				if (items.isEmpty())
					break;

				long long itemIndex = -1;
				for (const QString& str : items)
				{
					itemIndex = getItemIndexByName(str, true, "", sa::CHAR_EQUIPSLOT_COUNT);
					if (itemIndex != -1)
						break;
				}

				if (itemIndex == -1)
					break;

				target = 0;
				if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target)
					&& (target == battleCharCurrentPos.get()))
				{
					return sendBattleCharItemAct(itemIndex, target);
				}
			} while (false);

			return false;
		};
#pragma endregion

	//精靈復活
#pragma region MagicRevive
	auto magicReviveFun = [this, &gamedevice, &checkDeadAllie, &bt, &target]()->bool
		{
			do
			{
				bool magicRevive = gamedevice.getEnableHash(util::kBattleMagicReviveEnable);
				if (!magicRevive)
					break;

				long long tempTarget = -1;
				bool ok = false;
				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleMagicReviveTargetValue);
				if (util::checkAND(targetFlags, kSelectPet) && bt.objects.value(battleCharCurrentPos.get() + 5).maxHp > 0)
				{
					sa::battle_object_t obj = bt.objects.value(battleCharCurrentPos.get() + 5);
					if (obj.maxHp > 0 && obj.level > 0 &&
						(obj.hp == 0 || util::checkAND(obj.status, sa::BC_FLG_DEAD)))
					{
						tempTarget = battleCharCurrentPos.get() + 5;
						ok = true;
					}
				}

				if (!ok)
				{
					if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
					{
						if (checkDeadAllie(&tempTarget))
						{
							ok = true;
						}
					}
					else
					{
						for (long long i = sa::MAX_ENEMY - 1; i >= 10; --i)
						{
							sa::battle_object_t obj = bt.objects.value(i - 10);
							if (util::checkAND(targetFlags, 1LL << i)
								&& obj.maxHp > 0 && (obj.hp == 0 || util::checkAND(obj.status, sa::BC_FLG_DEAD)))
							{
								tempTarget = i - 10;
								ok = true;
								break;
							}
						}

						if (!ok)
							break;
					}
				}

				long long magicIndex = gamedevice.getValueHash(util::kBattleMagicReviveMagicValue) - 3;
				if (magicIndex < 0 || magicIndex > sa::MAX_MAGIC)
					break;

				bool isProfession = magicIndex > (3 + sa::MAX_MAGIC - 1);
				if (isProfession) //0 ~ MAX_PROFESSION_SKILL
				{
					magicIndex -= (3 + sa::MAX_MAGIC);

				}
				else
				{
					target = -1;
					if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0))
					{
						if (isCharMpEnoughForMagic(magicIndex))
						{
							return sendBattleCharMagicAct(magicIndex, target);
						}
						else
						{
							break;
						}
					}
				}
			} while (false);

			return false;
		};
#pragma endregion

	//間隔回合
#pragma region IntervalRound
	auto intervalRoundFun = [this, &gamedevice, &bt, &target]()->bool
		{
			do
			{
				bool crossActionEnable = gamedevice.getEnableHash(util::kBattleCrossActionCharEnable);
				if (!crossActionEnable)
					break;

				long long tempTarget = -1;

				long long round = gamedevice.getValueHash(util::kBattleCharCrossActionRoundValue) + 1;
				if ((battleCurrentRound.get() + 1) % round)
				{
					break;
				}

				long long actionType = gamedevice.getValueHash(util::kBattleCharCrossActionTypeValue);
				if (actionType == 1)
				{
					return sendBattleCharDefenseAct();
				}
				else if (actionType == 2)
				{
					return sendBattleCharEscapeAct();
				}

				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleCharCrossActionTargetValue);
				QHash<long long, long long> tagetHash = {
					{ kSelectEnemyAny, target == -1 ? getBattleSelectableEnemyTarget(bt) : target },
					{ kSelectEnemyAll, sa::TARGET_SIDE_1 },
					{ kSelectEnemyFront, getBattleSelectableEnemyOneRowTarget(bt, true) },
					{ kSelectEnemyBack, getBattleSelectableEnemyOneRowTarget(bt, false)},
					{ kSelectSelf, battleCharCurrentPos.get() },
					{ kSelectPet, battleCharCurrentPos.get() + 5 },
					{ kSelectAllieAny, getBattleSelectableAllieTarget(bt) },
					{ kSelectAllieAll, sa::TARGET_SIDE_0 },
				};

				tempTarget = tagetHash.value(targetFlags, -1);
				if (-1 == tempTarget)
				{
					for (long long i = 10; i < sa::MAX_ENEMY; ++i)
					{
						if (!util::checkAND(targetFlags, 1LL << i))
							continue;

						tempTarget = i - 10;
						break;
					}

					if (-1 == tempTarget)
						break;
				}

				if (actionType == 0)
				{
					if (tempTarget >= 0 && tempTarget < sa::MAX_ENEMY)
					{
						return sendBattleCharAttackAct(tempTarget);
					}
					else
					{
						tempTarget = getBattleSelectableEnemyTarget(bt);
						if (tempTarget == -1)
							break;

						return sendBattleCharAttackAct(tempTarget);
					}
				}
				else
				{
					long long magicIndex = actionType - 3;
					bool isProfession = magicIndex > (sa::MAX_MAGIC - 1);
					if (isProfession) //0 ~ MAX_PROFESSION_SKILL
					{
						magicIndex -= sa::MAX_MAGIC;

						if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0))
						{
							if (isCharMpEnoughForSkill(magicIndex))
							{
								return sendBattleCharJobSkillAct(magicIndex, target);
							}
							else
							{
								tempTarget = getBattleSelectableEnemyTarget(bt);
								return sendBattleCharAttackAct(tempTarget);
							}
						}
					}
					else
					{

						if (fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) && (target >= 0))
						{
							if (isCharMpEnoughForMagic(magicIndex))
							{
								return sendBattleCharMagicAct(magicIndex, target);
							}
							else
							{
								break;
							}
						}
					}
				}
			} while (false);

			return false;
		};
#pragma endregion

	//精靈淨化
#pragma region MagicPurify
	auto magicPurifyFun = [this, &gamedevice, &bt, &checkAllieStatus, &target]()->bool
		{
			do
			{
				bool charPurg = gamedevice.getEnableHash(util::kBattleCharPurgEnable);
				if (!charPurg)
					break;

				long long tempTarget = -1;
				bool ok = false;
				unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleCharPurgTargetValue);

				if (util::checkAND(targetFlags, kSelectSelf))
				{
					if (sa::hasBadStatus(bt.objects.value(battleCharCurrentPos.get()).status))
					{
						ok = true;
					}
				}

				if (util::checkAND(targetFlags, kSelectPet))
				{
					if (!ok && bt.objects.value(battleCharCurrentPos.get() + 5).maxHp > 0)
					{
						if (sa::hasBadStatus(bt.objects.value(battleCharCurrentPos.get() + 5).status)
							&& bt.objects.value(battleCharCurrentPos.get() + 5).hp > 0
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_DEAD)
							&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_HIDE))
						{
							tempTarget = battleCharCurrentPos.get() + 5;
							ok = true;
						}
					}
					else if (!ok && bt.objects.value(battleCharCurrentPos.get()).maxHp > 0)
					{
						if (sa::hasBadStatus(bt.objects.value(battleCharCurrentPos.get()).status) && bt.objects.value(battleCharCurrentPos.get()).rideHp > 0 &&
							!util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_DEAD) && !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_HIDE))
						{
							tempTarget = battleCharCurrentPos.get();
							ok = true;
						}
					}
				}

				if (!ok)
				{
					if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
					{
						if (checkAllieStatus(&tempTarget, false))
						{
							ok = true;
						}
					}
				}

				if (!ok)
					break;

				long long magicIndex = gamedevice.getValueHash(util::kBattleCharPurgActionTypeValue);
				if (magicIndex < 0 || magicIndex > sa::MAX_MAGIC)
					break;

				bool isProfession = magicIndex > (sa::MAX_MAGIC - 1);
				if (!isProfession) // ifMagic
				{
					target = -1;
					if (!fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) || (target < 0))
						break;

					if (!isCharMpEnoughForMagic(magicIndex))
						break;

					return sendBattleCharMagicAct(magicIndex, target);
				}
			} while (false);

			return false;
		};
#pragma endregion

	//自動換寵
#pragma region AutoSwitchPet
	auto autoSwitchPetFun = [this, &gamedevice, &bt]()->bool
		{
			do
			{
				sa::character_t pc = getCharacter();
				sa::battle_object_t obj = bt.objects.value(battleCharCurrentPos.get() + 5);

				if (obj.modelid > 0
					|| obj.level > 0
					|| obj.maxHp > 0)
					break;

				long long petIndex = -1;
				for (long long i = 0; i < sa::MAX_PET; ++i)
				{
					if (battlePetDisableList_.value(i))
						continue;

					if (pc.battlePetNo == i)
					{
						battlePetDisableList_[i] = true;
						continue;
					}

					sa::pet_t _pet = getPet(i);
					if (_pet.level <= 0 || _pet.maxHp <= 0 || _pet.hp < 1 || _pet.modelid == 0)
					{
						battlePetDisableList_[i] = true;
						continue;
					}

					if (_pet.state == sa::kRide || _pet.state != sa::kStandby)
					{
						battlePetDisableList_[i] = true;
						continue;
					}

					petIndex = i;
					break;
				}

				if (petIndex == -1)
					break;

				bool autoSwitch = gamedevice.getEnableHash(util::kBattleAutoSwitchEnable);
				if (!autoSwitch)
					break;

				return sendBattleCharSwitchPetAct(petIndex);

			} while (false);

			return false;
		};
#pragma endregion

	//自動捉寵
#pragma region CatchPet
	auto catchPetFun = [this, &gamedevice, &bt, &battleObjects, &tempbattleObjects, &target, &items]()->bool
		{
			do
			{
				bool autoCatch = gamedevice.getEnableHash(util::kAutoCatchEnable);
				if (!autoCatch)
					break;

				if (isPetSpotEmpty())
				{
					if (sendBattleCharEscapeAct())
					{
						petEnableEscapeForTemp_.on();
						return true;
					}
				}

				long long tempTarget = -1;

				//檢查等級條件
				bool levelLimitEnable = gamedevice.getEnableHash(util::kBattleCatchTargetLevelEnable);
				if (levelLimitEnable)
				{
					long long levelLimit = gamedevice.getValueHash(util::kBattleCatchTargetLevelValue);
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
				bool maxHpLimitEnable = gamedevice.getEnableHash(util::kBattleCatchTargetMaxHpEnable);
				if (maxHpLimitEnable && !battleObjects.isEmpty())
				{
					long long maxHpLimit = gamedevice.getValueHash(util::kBattleCatchTargetMaxHpValue);
					if (matchBattleEnemyByMaxHp(maxHpLimit, battleObjects, &tempbattleObjects))
					{
						battleObjects = tempbattleObjects;
					}
					else
						battleObjects.clear();
				}

				//檢查名稱條件
				QStringList targetList = gamedevice.getStringHash(util::kBattleCatchPetNameString).split(util::rexOR, Qt::SkipEmptyParts);
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
					long long catchMode = gamedevice.getValueHash(util::kBattleCatchModeValue);
					if (0 == catchMode)
					{
						//遇敵逃跑
						if (sendBattleCharEscapeAct())
						{
							petEnableEscapeForTemp_.on();
							return true;
						}
					}

					//遇敵攻擊 (跳出while 往下檢查其他常規動作)
					break;
				}

				sa::battle_object_t obj = battleObjects.first();
				battleObjects.pop_front();
				tempTarget = obj.pos;
				tempCatchPetTargetIndex_.set(tempTarget);

				//允許人物動作降低血量
				bool allowCharAction = gamedevice.getEnableHash(util::kBattleCatchCharMagicEnable);
				long long hpLimit = gamedevice.getValueHash(util::kBattleCatchTargetMagicHpValue);
				if (allowCharAction && (obj.hpPercent >= hpLimit))
				{
					long long actionType = gamedevice.getValueHash(util::kBattleCatchCharMagicValue);
					if (actionType == 1)
					{
						return sendBattleCharDefenseAct();
					}
					else if (actionType == 2)
					{
						return sendBattleCharEscapeAct();
					}

					if (actionType == 0)
					{
						if (tempTarget >= 0 && tempTarget < sa::MAX_ENEMY)
						{
							return sendBattleCharAttackAct(tempTarget);
						}
					}
					else
					{
						long long magicIndex = actionType - 3;
						bool isProfession = magicIndex > (sa::MAX_MAGIC - 1);
						if (isProfession) //0 ~ MAX_PROFESSION_SKILL
						{
							magicIndex -= sa::MAX_MAGIC;
							target = -1;
							if (fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) && (target >= 0))
							{
								if (isCharMpEnoughForSkill(magicIndex))
								{

									return sendBattleCharJobSkillAct(magicIndex, target);
								}
								else
								{
									tempTarget = getBattleSelectableEnemyTarget(bt);
									return sendBattleCharAttackAct(tempTarget);
								}
							}
						}
						else
						{
							//如果敵方已經中狀態，則尋找下一個目標
							for (;;)
							{
								if (sa::hasBadStatus(obj.status))
								{
									if (!battleObjects.isEmpty())
									{
										obj = battleObjects.front();
										tempTarget = obj.pos;
										battleObjects.pop_front();
										return sendBattleCharCatchPetAct(tempTarget);
									}
									else
									{
										return sendBattleCharDefenseAct();
									}
								}
								else
									break;
							}

							target = -1;
							if (!fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) || (target < 0))
								break;

							if (!isCharMpEnoughForMagic(magicIndex))
								break;

							return sendBattleCharMagicAct(magicIndex, target);
						}
					}
				}

				//允許人物道具降低血量
				bool allowCharItem = gamedevice.getEnableHash(util::kBattleCatchCharItemEnable);
				hpLimit = gamedevice.getValueHash(util::kBattleCatchTargetItemHpValue);
				if (allowCharItem && (obj.hpPercent >= hpLimit))
				{
					long long itemIndex = -1;
					QString text = gamedevice.getStringHash(util::kBattleCatchCharItemString).simplified();
					items = text.split(util::rexOR, Qt::SkipEmptyParts);
					for (const QString& str : items)
					{
						itemIndex = getItemIndexByName(str, true, "", sa::CHAR_EQUIPSLOT_COUNT);
						if (itemIndex != -1)
							break;
					}

					target = -1;
					if (itemIndex != -1)
					{
						if (fixCharTargetByItemIndex(itemIndex, tempTarget, &target) && (target >= 0))
						{
							return sendBattleCharItemAct(itemIndex, target);
						}
					}
				}

				return sendBattleCharCatchPetAct(tempTarget);
			} while (false);

			return false;
		};
#pragma endregion
#pragma endregion

	QHash<long long, std::function<bool()>> actions;
	actions.insert(0, fallDownEscapeFun);
	actions.insert(1, lockEscapeFun);
	actions.insert(2, lockAttackFun);
	actions.insert(3, itemReviveFun);
	actions.insert(4, selectRoundFun);
	actions.insert(5, magicHealFun);
	actions.insert(6, itemHealFun);
	actions.insert(7, skillMpFun);
	actions.insert(8, itemMpFun);
	actions.insert(9, magicReviveFun);
	actions.insert(10, intervalRoundFun);
	actions.insert(11, magicPurifyFun);
	actions.insert(12, autoSwitchPetFun);
	actions.insert(13, catchPetFun);

	QString orderStr = gamedevice.getStringHash(util::kBattleActionOrderString);
	if (orderStr.isEmpty())
	{
		for (const auto& it : actions)
		{
			if (it())
				return true;
		}
	}
	else
	{
		QStringList orderList = orderStr.split(util::rexOR, Qt::SkipEmptyParts);
		for (const QString& it : orderList)
		{
			bool ok = false;
			long long index = it.toLongLong(&ok) - 1;
			if (!ok || !actions.contains(index))
				continue;

			std::function<bool()> fun = actions.value(index);
			if (fun != nullptr && fun())
				return true;
		}
	}

	//一般動作
#pragma region NormalAction
	do
	{
		long long tempTarget = -1;
		//bool ok = false;

		long long enemy = gamedevice.getValueHash(util::kBattleCharNormalActionEnemyValue);
		if (enemy > 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		long long level = gamedevice.getValueHash(util::kBattleCharNormalActionLevelValue);
		if (level != 0)
		{
			auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const sa::battle_object_t& obj1, const sa::battle_object_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		long long actionType = gamedevice.getValueHash(util::kBattleCharNormalActionTypeValue);
		if (actionType == 1)
		{
			return sendBattleCharDefenseAct();
		}
		else if (actionType == 2)
		{
			return sendBattleCharEscapeAct();
		}

		unsigned long long targetFlags = gamedevice.getValueHash(util::kBattleCharNormalActionTargetValue);

		QHash<long long, long long> tagetHash = {
			{ kSelectEnemyAny, target == -1 ? getBattleSelectableEnemyTarget(bt) : target },
			{ kSelectEnemyAll, sa::TARGET_SIDE_1 },
			{ kSelectEnemyFront, getBattleSelectableEnemyOneRowTarget(bt, true) },
			{ kSelectEnemyBack, getBattleSelectableEnemyOneRowTarget(bt, false) },
			{ kSelectSelf, battleCharCurrentPos.get() },
			{ kSelectPet, battleCharCurrentPos.get() + 5 },
			{ kSelectAllieAny, getBattleSelectableAllieTarget(bt) },
			{ kSelectAllieAll, sa::TARGET_SIDE_0 },
		};

		tempTarget = tagetHash.value(targetFlags, -1);
		if (-1 == tempTarget)
		{
			for (long long i = 10; i < sa::MAX_ENEMY; ++i)
			{
				if (!util::checkAND(targetFlags, 1LL << i))
					continue;

				tempTarget = i - 10;
				break;
			}

			if (-1 == tempTarget)
				break;
		}

		if (actionType == 0)
		{
			if (tempTarget < 0 || tempTarget >= sa::MAX_ENEMY)
				break;

			return sendBattleCharAttackAct(tempTarget);
		}
		else
		{
			long long magicIndex = actionType - 3;
			bool isProfession = magicIndex > (sa::MAX_MAGIC - 1);
			if (isProfession) //0 ~ MAX_PROFESSION_SKILL
			{
				magicIndex -= sa::MAX_MAGIC;

				if (!fixCharTargetBySkillIndex(magicIndex, tempTarget, &target) || (target < 0))
					break;

				if (!isCharMpEnoughForSkill(magicIndex))
					break;

				return sendBattleCharJobSkillAct(magicIndex, target);
			}
			else
			{
				if (!fixCharTargetByMagicIndex(magicIndex, tempTarget, &target) || (target < 0))
					break;

				if (!isCharMpEnoughForMagic(magicIndex))
					break;

				return sendBattleCharMagicAct(magicIndex, target);
			}
		}
	} while (false);
#pragma endregion

	return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));
}

//寵物戰鬥邏輯(這裡因為懶了所以寫了一坨狗屎)
bool Worker::handlePetBattleLogics(const sa::battle_data_t& bt)
{
	if (runBattleLua(kPetScript))
		return true;

	using namespace util;
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	QVector<sa::battle_object_t> battleObjects = bt.enemies;
	QVector<sa::battle_object_t> tempbattleObjects;

	sortBattleUnit(battleObjects);
	long long target = -1;

#pragma region PetBattleTools
	//檢測隊友血量
	auto checkAllieHp = [this, &bt](long long cmpvalue, long long* target, bool useequal)->bool
		{
			if (!target)
				return false;

			long long min = 0;
			long long max = (sa::MAX_ENEMY / 2) - 1;
			if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
			{
				min = sa::MAX_ENEMY / 2;
				max = sa::MAX_ENEMY - 1;
			}

			QVector<sa::battle_object_t> battleObjects = bt.objects;
			for (const sa::battle_object_t& obj : battleObjects)
			{
				if (obj.pos < min || obj.pos > max)
					continue;

				if (obj.hp == 0)
					continue;

				if (obj.maxHp == 0)
					continue;

				if (util::checkAND(obj.status, sa::BC_FLG_HIDE) || util::checkAND(obj.status, sa::BC_FLG_DEAD))
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
	auto checkAllieStatus = [this, &bt](long long* target, bool useequal)->bool
		{
			if (!target)
				return false;

			long long min = 0;
			long long max = (sa::MAX_ENEMY / 2) - 1;
			if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
			{
				min = sa::MAX_ENEMY / 2;
				max = sa::MAX_ENEMY - 1;
			}

			QVector<sa::battle_object_t> battleObjects = bt.objects;
			for (const sa::battle_object_t& obj : battleObjects)
			{
				if (obj.pos < min || obj.pos > max)
					continue;

				if (obj.hp == 0)
					continue;

				if (obj.maxHp == 0)
					continue;

				if (util::checkAND(obj.status, sa::BC_FLG_HIDE) || util::checkAND(obj.status, sa::BC_FLG_DEAD))
					continue;
				if (!useequal && sa::hasBadStatus(obj.status))
				{
					*target = obj.pos;
					return true;
				}
				else if (useequal && sa::hasBadStatus(obj.status))
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
		bool autoCatch = gamedevice.getEnableHash(util::kAutoCatchEnable);
		if (!autoCatch)
			break;

		//允許寵物動作
		bool allowPetAction = gamedevice.getEnableHash(util::kBattleCatchPetSkillEnable);
		if (!allowPetAction)
		{
			return sendBattlePetDoNothing();//避免有人會忘記改成防禦，默認只要打開捉寵且沒設置動作就什麼都不做
		}

		long long actionType = gamedevice.getValueHash(util::kBattleCatchPetSkillValue);

		long long skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > sa::MAX_PET_SKILL)
			break;

		long long tempTarget = tempCatchPetTargetIndex_.get();
		if ((tempTarget != -1) && fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0))
		{
			return sendBattlePetSkillAct(skillIndex, target);
		}

		return sendBattlePetDoNothing(); //避免有人會忘記改成防禦，默認只要打開捉寵且動作失敗就什麼都不做
	} while (false);
#pragma endregion

	//鎖定攻擊
#pragma region LockAttack
	do
	{
		bool lockAttackEnable = gamedevice.getEnableHash(util::kLockAttackEnable);
		if (!lockAttackEnable)
			break;

		QString text = gamedevice.getStringHash(util::kLockAttackString);
		if (text.isEmpty())
			break;

		if (conditionMatchTarget(battleObjects, text, &target))
		{
			break;
		}

		//鎖定攻擊條件不滿足時，是否不逃跑
		bool doNotEscape = gamedevice.getEnableHash(util::kBattleNoEscapeWhileLockPetEnable);

		if (!doNotEscape && IS_LOCKATTACK_ESCAPE_DISABLE_.get())
			break;

		if (doNotEscape)
			break;

		return sendBattlePetDoNothing();
	} while (false);
#pragma endregion

	//指定回合
#pragma region SelectedRound
	do
	{
		long long atRoundIndex = gamedevice.getValueHash(util::kBattlePetRoundActionEnemyValue);
		if (atRoundIndex <= 0)
			break;

		long long tempTarget = -1;
		//bool ok = false;

		long long enemy = gamedevice.getValueHash(util::kBattlePetRoundActionLevelValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		long long level = gamedevice.getValueHash(util::kBattleCharRoundActionLevelValue);
		if (level != 0)
		{
			auto minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const sa::battle_object_t& obj1, const sa::battle_object_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		unsigned long long targetFlags = gamedevice.getValueHash(util::kBattlePetRoundActionTargetValue);
		if (util::checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = 21;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.get();
		}
		else if (util::checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.get() + 5;
		}
		else if (util::checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (util::checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = sa::TARGET_SIDE_0;
		}
		else if (util::checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (util::checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

		long long actionType = gamedevice.getValueHash(util::kBattlePetRoundActionTypeValue);

		long long skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > sa::MAX_PET_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0))
		{
			return sendBattlePetSkillAct(skillIndex, target);
		}
	} while (false);
#pragma endregion

	//間隔回合
#pragma region CrossRound
	do
	{
		bool crossActionEnable = gamedevice.getEnableHash(util::kBattleCrossActionPetEnable);
		if (!crossActionEnable)
			break;

		long long tempTarget = -1;

		long long round = gamedevice.getValueHash(util::kBattlePetCrossActionRoundValue) + 1;
		if ((battleCurrentRound.get() + 1) % round)
		{
			break;
		}

		unsigned long long targetFlags = gamedevice.getValueHash(util::kBattlePetCrossActionTargetValue);
		if (util::checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = sa::TARGET_SIDE_1;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.get();
		}
		else if (util::checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.get() + 5;
		}
		else if (util::checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (util::checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = sa::TARGET_SIDE_0;
		}
		else if (util::checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (util::checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

		long long actionType = gamedevice.getValueHash(util::kBattlePetCrossActionTypeValue);

		long long skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > sa::MAX_PET_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0))
		{
			return sendBattlePetSkillAct(skillIndex, target);
		}
		else
		{
			tempTarget = getBattleSelectableEnemyTarget(bt);
			if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0))
			{
				return sendBattlePetSkillAct(skillIndex, target);
			}
		}
	} while (false);
#pragma endregion

	//寵物淨化
#pragma region PetSkillPurg
	do
	{
		bool petPurg = gamedevice.getEnableHash(util::kBattlePetPurgEnable);
		if (!petPurg)
			break;

		long long tempTarget = -1;
		bool ok = false;
		unsigned long long targetFlags = gamedevice.getValueHash(util::kBattlePetPurgTargetValue);

		if (util::checkAND(targetFlags, kSelectSelf))
		{
			if (sa::hasBadStatus(bt.objects.value(battleCharCurrentPos.get()).status))
			{
				ok = true;
			}
		}

		if (util::checkAND(targetFlags, kSelectPet))
		{
			if (!ok && bt.objects.value(battleCharCurrentPos.get() + 5).maxHp > 0)
			{
				if (sa::hasBadStatus(bt.objects.value(battleCharCurrentPos.get() + 5).status)
					&& bt.objects.value(battleCharCurrentPos.get() + 5).hp > 0
					&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_DEAD)
					&& !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.get() + 5;
					ok = true;
				}
			}
			else if (!ok && bt.objects.value(battleCharCurrentPos.get()).maxHp > 0)
			{
				if (sa::hasBadStatus(bt.objects.value(battleCharCurrentPos.get()).status) && bt.objects.value(battleCharCurrentPos.get()).rideHp > 0 &&
					!util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_DEAD) && !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.get();
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
			{
				if (checkAllieStatus(&tempTarget, false))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		long long petActionIndex = gamedevice.getValueHash(util::kBattlePetPurgActionTypeValue);
		if (petActionIndex < 0 || petActionIndex >sa::MAX_PET_SKILL)
			break;

		bool isProfession = petActionIndex > (sa::MAX_PET_SKILL - 1);
		if (!isProfession) // ifpetAction
		{
			target = -1;
			if (fixPetTargetBySkillIndex(petActionIndex, tempTarget, &target) && (target >= 0))
			{
				return sendBattlePetSkillAct(petActionIndex, target);
			}
		}
	} while (false);
#pragma endregion

	//寵物補血
#pragma region PetSkillHeal
	do
	{
		bool petHeal = gamedevice.getEnableHash(util::kBattlePetHealEnable);
		if (!petHeal)
			break;

		long long tempTarget = -1;
		bool ok = false;
		unsigned long long targetFlags = gamedevice.getValueHash(util::kBattlePetHealTargetValue);
		long long charPercent = gamedevice.getValueHash(util::kBattlePetHealCharValue);
		long long petPercent = gamedevice.getValueHash(util::kBattlePetHealPetValue);
		long long alliePercent = gamedevice.getValueHash(util::kBattlePetHealAllieValue);

		if (util::checkAND(targetFlags, kSelectSelf))
		{
			if (checkCharHp(charPercent, &tempTarget))
			{
				ok = true;
			}
		}

		if (util::checkAND(targetFlags, kSelectPet))
		{
			if (!ok && bt.objects.value(battleCharCurrentPos.get() + 5).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos.get() + 5).hpPercent <= petPercent && bt.objects.value(battleCharCurrentPos.get() + 5).hp > 0 &&
					!util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_DEAD) && !util::checkAND(bt.objects.value(battleCharCurrentPos.get() + 5).status, sa::BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.get() + 5;
					ok = true;
				}
			}
			else if (!ok && bt.objects.value(battleCharCurrentPos.get()).maxHp > 0)
			{
				if (bt.objects.value(battleCharCurrentPos.get()).rideHpPercent <= petPercent && bt.objects.value(battleCharCurrentPos.get()).rideHp > 0 &&
					!util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_DEAD) && !util::checkAND(bt.objects.value(battleCharCurrentPos.get()).status, sa::BC_FLG_HIDE))
				{
					tempTarget = battleCharCurrentPos.get();
					ok = true;
				}
			}
		}

		if (!ok)
		{
			if (util::checkAND(targetFlags, kSelectAllieAny) || util::checkAND(targetFlags, kSelectAllieAll))
			{
				if (checkAllieHp(alliePercent, &tempTarget, false))
				{
					ok = true;
				}
			}
		}

		if (!ok)
			break;

		long long petActionIndex = gamedevice.getValueHash(util::kBattlePetHealActionTypeValue);
		if (petActionIndex < 0 || petActionIndex > sa::MAX_PET_SKILL)
			break;

		bool isProfession = petActionIndex > (sa::MAX_PET_SKILL - 1);
		if (!isProfession) // ifpetAction
		{
			target = -1;
			if (fixPetTargetBySkillIndex(petActionIndex, tempTarget, &target) && (target >= 0))
			{
				return sendBattlePetSkillAct(petActionIndex, target);
			}
		}
	} while (false);
#pragma endregion

	//一般動作
#pragma region NormalAction
	do
	{
		long long tempTarget = -1;
		//bool ok = false;

		long long enemy = gamedevice.getValueHash(util::kBattlePetNormalActionEnemyValue);
		if (enemy != 0)
		{
			if (bt.enemies.size() <= enemy) //敵人 <= 設置數量
			{
				break;
			}
		}

		long long level = gamedevice.getValueHash(util::kBattlePetNormalActionLevelValue);
		if (level != 0)
		{
			const sa::battle_object_t* minIt = std::min_element(bt.enemies.begin(), bt.enemies.end(),
				[](const sa::battle_object_t& obj1, const sa::battle_object_t& obj2)
				{
					return obj1.level < obj2.level;
				});

			if (minIt->level <= (level * 10))
			{
				break;
			}
		}

		unsigned long long targetFlags = gamedevice.getValueHash(util::kBattlePetNormalActionTargetValue);
		if (util::checkAND(targetFlags, kSelectEnemyAny))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyAll))
		{
			tempTarget = sa::TARGET_SIDE_1;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyFront))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, true);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectEnemyBack))
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyOneRowTarget(bt, false);
			else
				tempTarget = target;
		}
		else if (util::checkAND(targetFlags, kSelectSelf))
		{
			tempTarget = battleCharCurrentPos.get();
		}
		else if (util::checkAND(targetFlags, kSelectPet))
		{
			tempTarget = battleCharCurrentPos.get() + 5;
		}
		else if (util::checkAND(targetFlags, kSelectAllieAny))
		{
			tempTarget = getBattleSelectableAllieTarget(bt);
		}
		else if (util::checkAND(targetFlags, kSelectAllieAll))
		{
			tempTarget = sa::TARGET_SIDE_0;
		}
		else if (util::checkAND(targetFlags, kSelectLeader))
		{
			tempTarget = bt.alliemin + 0;
		}
		else if (util::checkAND(targetFlags, kSelectLeaderPet))
		{
			tempTarget = bt.alliemin + 0 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate1))
		{
			tempTarget = bt.alliemin + 1;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate1Pet))
		{
			tempTarget = bt.alliemin + 1 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate2))
		{
			tempTarget = bt.alliemin + 2;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate2Pet))
		{
			tempTarget = bt.alliemin + 2 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate3))
		{
			tempTarget = bt.alliemin + 3;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate3Pet))
		{
			tempTarget = bt.alliemin + 3 + 5;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate4))
		{
			tempTarget = bt.alliemin + 4;
		}
		else if (util::checkAND(targetFlags, kSelectTeammate4Pet))
		{
			tempTarget = bt.alliemin + 4 + 5;
		}

		long long actionType = gamedevice.getValueHash(util::kBattlePetNormalActionTypeValue);

		long long skillIndex = actionType;
		if (skillIndex < 0 || skillIndex > sa::MAX_PET_SKILL)
			break;

		if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0))
		{
			return sendBattlePetSkillAct(skillIndex, target);
		}
		else
		{
			if (target == -1)
				tempTarget = getBattleSelectableEnemyTarget(bt);
			if (fixPetTargetBySkillIndex(skillIndex, tempTarget, &target) && (target >= 0))
			{

				return sendBattlePetSkillAct(skillIndex, target);
			}
		}
	} while (false);
#pragma endregion

	return sendBattlePetDoNothing();
}

//精靈名稱匹配精靈索引
long long Worker::getMagicIndexByName(const QString& name, bool isExact) const
{
	if (name.isEmpty())
		return -1;

	QString newName = name.simplified();
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
		isExact = false;
	}

	for (long long i = 0; i < sa::MAX_MAGIC; ++i)
	{
		sa::magic_t magic = getMagic(i);
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

long long Worker::getSkillIndexByName(const QString& name) const
{
	if (name.isEmpty())
		return -1;

	QString newName = name.simplified();
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
	}

	QHash <long long, sa::profession_skill_t> profession_skill = getSkills();
	for (long long i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
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
QString Worker::getAreaString(long long target)
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
long long Worker::getGetPetSkillIndexByName(long long petIndex, const QString& name) const
{
	if (petIndex < 0 || petIndex >= sa::MAX_PET)
		return -1;

	if (name.isEmpty())
		return -1;

	long long petSkillIndex = -1;

	QHash <long long, sa::pet_skill_t> petSkill = getPetSkills(petIndex);
	for (long long i = 0; i < sa::MAX_PET_SKILL; ++i)
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
bool Worker::isCharMpEnoughForMagic(long long magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= sa::MAX_MAGIC)
		return false;

	if (getMagic(magicIndex).costmp > getCharacter().mp)
		return false;

	return true;
}

//戰鬥檢查MP是否足夠施放技能
bool Worker::isCharMpEnoughForSkill(long long  magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= sa::MAX_PROFESSION_SKILL)
		return false;

	if (getSkill(magicIndex).costmp > getCharacter().mp)
		return false;

	return true;
}

//戰鬥檢查HP是否足夠施放技能
bool Worker::isCharHpEnoughForSkill(long long magicIndex) const
{
	if (magicIndex < 0 || magicIndex >= sa::MAX_PROFESSION_SKILL)
		return false;

	if (sa::MIN_HP_PERCENT > getCharacter().hpPercent)
		return false;

	return true;
}

static bool compareBattleObjects(const sa::battle_object_t& a, const sa::battle_object_t& b)
{
	// 首先按照 hp 升序排序
	if (a.hp != b.hp)
	{
		return a.hp < b.hp;
	}

	// 如果 hp 相同，按照 maxHp 升序排序
	if (a.maxHp != b.maxHp)
	{
		return a.maxHp < b.maxHp;
	}

	// 如果 maxHp 也相同，按照 level 升序排序
	if (a.level != b.level)
	{
		return a.level < b.level;
	}

	// 如果 level 也相同，按照 status 升序排序
	if (sa::hasBadStatus(a.status))
		return true;
	if (sa::hasBadStatus(b.status))
		return false;

	constexpr long long order[sa::MAX_ENEMY] = { 19, 17, 15, 16, 18, 14, 12, 10, 11, 13, 8, 6, 5, 7, 9, 3, 1, 0, 2, 4 };
	const long long* ait = std::find(order, order + sa::MAX_ENEMY, a.pos);
	const long long* bit = std::find(order, order + sa::MAX_ENEMY, b.pos);
	long long aindex = std::distance(order, ait);
	long long bindex = std::distance(order, bit);
	return aindex < bindex;
}

//戰場上單位排序
void Worker::sortBattleUnit(QVector<sa::battle_object_t>& v) const
{
	if (v.isEmpty())
		return;

	QVector<sa::battle_object_t> front;
	QVector<sa::battle_object_t> back;

	//這裡不需要考慮我方是在左邊還右邊 只需要判斷是前排還是後排
	long long efrontmin = 15, efrontmax = 19;
	long long ebackmin = 10, ebackmax = 14;
	long long afrontmin = 5, afrontmax = 9;
	long long abackmin = 0, abackmax = 4;

	for (const sa::battle_object_t& it : v)
	{
		if (it.pos >= efrontmin && it.pos <= efrontmax || it.pos >= afrontmin && it.pos <= afrontmax)
			front.append(it);
		else if (it.pos >= ebackmin && it.pos <= ebackmax || it.pos >= abackmin && it.pos <= abackmax)
			back.append(it);
	}

	//前後排分開排序
	std::sort(front.begin(), front.end(), compareBattleObjects);
	std::sort(back.begin(), back.end(), compareBattleObjects);

	//前排優先
	v = front;
	v.append(back);

	//for (const long long it : order)
	//{
	//	for (const battleobject_t& obj : dstv)
	//	{
	//		if ((obj.hp > 0) && (obj.level > 0) && (obj.maxHp > 0) && (obj.modelid > 0) && (obj.pos == it)
	//			&& !util::checkAND(obj.status, BC_FLG_DEAD) && !util::checkAND(obj.status, BC_FLG_HIDE))
	//		{
	//			newv.append(obj);
	//			break;
	//		}
	//	}
	//}
}

//是否可直接接觸(如果目標為後排但前排還存活視為不可接觸)
static bool isTouchable(const QVector<sa::battle_object_t>& obj, long long index)
{
	const QHash<long long, long long> hash = {
		{ 13, 18 },
		{ 11, 16 },
		{ 10, 15 },
		{ 12, 17 },
		{ 14, 19 },
		{ 3, 8 },
		{ 1, 6 },
		{ 0, 5 },
		{ 2, 7 },
		{ 4, 9 },
	};

	if (hash.keys().contains(index))
	{
		long long frontIndex = hash.value(index);
		sa::battle_object_t frontObj = obj.value(frontIndex);
		//如果嘗試施放於後排但有前排存在則視為不可攻擊目標
		return frontObj.hp <= 0 || util::checkAND(frontObj.status, sa::BC_FLG_HIDE) || util::checkAND(frontObj.status, sa::BC_FLG_DEAD);
	}

	return true;
}

//取戰鬥敵方可選編號
long long Worker::getBattleSelectableEnemyTarget(const sa::battle_data_t& bt) const
{
	long long defaultTarget = sa::MAX_ENEMY - 1;
	if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
		defaultTarget = sa::MAX_ENEMY / 4;

	QVector<sa::battle_object_t> enemies(bt.enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	sortBattleUnit(enemies);
	if (enemies.size())
	{
		for (const sa::battle_object_t& obj : enemies)
		{
			if (isTouchable(enemies, obj.pos))
				return obj.pos;
		}
	}

	return defaultTarget;
}

//取戰鬥一排可選編號
long long Worker::getBattleSelectableEnemyOneRowTarget(const sa::battle_data_t& bt, bool front) const
{
	//逆序
	if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
	{
		if (front)
			return sa::TARGET_SIDE_0_F_ROW;
		else
			return sa::TARGET_SIDE_0_B_ROW;
	}
	else
	{
		if (front)
			return sa::TARGET_SIDE_1_F_ROW;
		else
			return sa::TARGET_SIDE_1_B_ROW;
	}

#if 0
	long long defaultTarget = sa::MAX_ENEMY - 5;
	if (battleCharCurrentPos.get() >= (sa::MAX_ENEMY / 2))
		defaultTarget = sa::MAX_ENEMY / 4;

	QVector<sa::battle_object_t> enemies(bt.enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	sortBattleUnit(enemies);
	if (enemies.isEmpty() || !enemies.size())
		return defaultTarget;

	long long targetIndex = -1;

	if (front)
	{
		if (battleCharCurrentPos.get() < (sa::MAX_ENEMY / 2))
		{
			// 只取 pos 在 15~19 之间的，取最前面的
			for (long long i = 0; i < enemies.size(); ++i)
			{
				if (enemies[i].pos >= 15 && enemies[i].pos < sa::MAX_ENEMY)
				{
					targetIndex = i;
					break;
				}
			}
		}
		else
		{
			// 只取 pos 在 5~9 之间的，取最前面的
			for (long long i = 0; i < enemies.size(); ++i)
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
		if (battleCharCurrentPos.get() < (sa::MAX_ENEMY / 2))
		{
			// 只取 pos 在 10~14 之间的，取最前面的
			for (long long i = 0; i < enemies.size(); ++i)
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
			for (long long i = 0; i < enemies.size(); ++i)
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
#endif
}

//取戰鬥隊友可選目標編號
long long Worker::getBattleSelectableAllieTarget(const sa::battle_data_t& bt) const
{
	long long defaultTarget = 5;
	if (battleCharCurrentPos.get() >= 10)
		defaultTarget = 15;

	QVector<sa::battle_object_t> allies(bt.allies);
	if (allies.isEmpty() || !allies.size())
		return defaultTarget;

	sortBattleUnit(allies);
	if (allies.size())
		return allies[0].pos;
	else
		return defaultTarget;
}

//戰鬥匹配敵方名稱
bool Worker::matchBattleEnemyByName(const QString& name, bool isExact, const QVector<sa::battle_object_t>& src, QVector<sa::battle_object_t>* v) const
{
	QVector<sa::battle_object_t> tempv;
	if (name.isEmpty())
		return false;

	QString newName = name;
	if (newName.startsWith("?"))
	{
		newName = newName.mid(1);
		isExact = false;
	}

	QVector<sa::battle_object_t> enemies = src;

	if (enemies.isEmpty())
		return false;

	sortBattleUnit(enemies);

	for (const sa::battle_object_t& enemy : enemies)
	{
		if (enemy.hp == 0)
			continue;

		if (enemy.maxHp == 0)
			continue;

		if (enemy.modelid == 0)
			continue;

		if (util::checkAND(enemy.status, sa::BC_FLG_HIDE))
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
bool Worker::matchBattleEnemyByLevel(long long level, const QVector<sa::battle_object_t>& src, QVector<sa::battle_object_t>* v) const
{
	QVector<sa::battle_object_t> tempv;
	if (level <= 0 || level > 255)
		return false;

	QVector<sa::battle_object_t> enemies = src;

	if (enemies.isEmpty())
		return false;

	sortBattleUnit(enemies);

	for (const sa::battle_object_t& enemy : enemies)
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
bool Worker::matchBattleEnemyByMaxHp(long long maxHp, const QVector<sa::battle_object_t>& src, QVector<sa::battle_object_t>* v) const
{
	QVector<sa::battle_object_t> tempv;
	if (maxHp <= 0 || maxHp > 100000)
		return false;

	QVector<sa::battle_object_t> enemies = src;

	if (enemies.isEmpty())
		return false;

	sortBattleUnit(enemies);

	for (const sa::battle_object_t& enemy : enemies)
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
bool Worker::fixCharTargetByMagicIndex(long long magicIndex, long long oldtarget, long long* target) const
{
	if (!target)
		return false;

	if (magicIndex < 0 || magicIndex >= sa::MAX_MAGIC)
		return false;

	long long magicType = getMagic(magicIndex).target;

	switch (magicType)
	{
	case sa::MAGIC_TARGET_MYSELF:
	{
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case sa::MAGIC_TARGET_OTHER://雙方任意單體
	{
		if (oldtarget < 0 || oldtarget >= 20)
		{
			if (battleCharCurrentPos.get() < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case sa::MAGIC_TARGET_ALLMYSIDE://我方全體
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = 20;
		else
			oldtarget = 21;
		break;
	}
	case sa::MAGIC_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = 21;
		else
			oldtarget = 20;
		break;
	}
	case sa::MAGIC_TARGET_ALL://雙方全體同時(場地)
	{
		oldtarget = 22;
		break;
	}
	case sa::MAGIC_TARGET_NONE://無
	{
		oldtarget = -1;
		break;
	}
	case sa::MAGIC_TARGET_OTHERWITHOUTMYSELF://我方任意除了自己
	{
		if (oldtarget == battleCharCurrentPos.get())
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::MAGIC_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{

		if (oldtarget == battleCharCurrentPos.get())
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.get() + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::MAGIC_TARGET_WHOLEOTHERSIDE://敵方全體 或 我方全體
	{
		if (oldtarget != sa::TARGET_SIDE_0 && oldtarget != sa::TARGET_SIDE_1)
		{
			if (oldtarget >= 0 && oldtarget <= 9)
			{
				if (battleCharCurrentPos.get() < 10)
					oldtarget = sa::TARGET_SIDE_0;
				else
					oldtarget = sa::TARGET_SIDE_1;
			}
			else
			{
				if (battleCharCurrentPos.get() < 10)
					oldtarget = sa::TARGET_SIDE_1;
				else
					oldtarget = sa::TARGET_SIDE_0;
			}
		}

		break;
	}
	case sa::MAGIC_TARGET_SINGLE:				// 針對敵方某一方
	{
		if (oldtarget < 10 || oldtarget >= sa::MAX_ENEMY)
		{
			if (battleCharCurrentPos.get() < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case sa::MAGIC_TARGET_ONE_ROW:				// 針對敵方某一列
	{
		if (battleCharCurrentPos.get() < 10)
		{
			if (oldtarget < 5)
				oldtarget = sa::MAX_ENEMY + 5;
			else if (oldtarget < 10)
				oldtarget = sa::MAX_ENEMY + 6;
			else if (oldtarget < 15)
				oldtarget = sa::MAX_ENEMY + 3;
			else
				oldtarget = sa::MAX_ENEMY + 4;
		}
		else
		{
			if (oldtarget < 5)
				oldtarget = sa::MAX_ENEMY + 5 - 10;
			else if (oldtarget < 10)
				oldtarget = sa::MAX_ENEMY + 6 - 10;
			else if (oldtarget < 15)
				oldtarget = sa::MAX_ENEMY + 3 - 10;
			else
				oldtarget = sa::MAX_ENEMY + 4 - 10;
		}
		break;
	}
	case sa::MAGIC_TARGET_ALL_ROWS:				// 針對敵方所有人
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_1;
		else
			oldtarget = sa::TARGET_SIDE_0;
		break;
	}
	default:
		break;
	}

	*target = oldtarget;
	return true;
}

//戰鬥人物修正職業技能目標範圍
bool Worker::fixCharTargetBySkillIndex(long long magicIndex, long long oldtarget, long long* target) const
{
	if (!target)
		return false;

	if (magicIndex < 0 || magicIndex >= sa::MAX_PROFESSION_SKILL)
		return false;

	long long magicType = getSkill(magicIndex).target;

	switch (magicType)
	{
	case sa::MAGIC_TARGET_MYSELF:
	{
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case sa::MAGIC_TARGET_OTHER://雙方任意單體
	{
		if (oldtarget < 0 || oldtarget >= sa::MAX_ENEMY)
		{
			if (battleCharCurrentPos.get() < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case sa::MAGIC_TARGET_ALLMYSIDE://我方全體
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_0;
		else
			oldtarget = sa::TARGET_SIDE_1;
		break;
	}
	case sa::MAGIC_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_1;
		else
			oldtarget = sa::TARGET_SIDE_0;
		break;
	}
	case sa::MAGIC_TARGET_ALL://雙方全體同時(場地)
	{
		oldtarget = sa::TARGET_ALL;
		break;
	}
	case sa::MAGIC_TARGET_NONE://無
	{
		//oldtarget = -1;
		//break;
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case sa::MAGIC_TARGET_OTHERWITHOUTMYSELF://我方任意除了自己
	{
		if (oldtarget == battleCharCurrentPos.get())
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::MAGIC_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{
		if (oldtarget == battleCharCurrentPos.get())
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.get() + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::MAGIC_TARGET_WHOLEOTHERSIDE://敵方全體 或 我方全體
	{
		if (oldtarget != 20 && oldtarget != 21)
		{
			if (oldtarget >= 0 && oldtarget <= 9)
			{
				if (battleCharCurrentPos.get() < 10)
					oldtarget = sa::TARGET_SIDE_0;
				else
					oldtarget = sa::TARGET_SIDE_1;
			}
			else
			{
				if (battleCharCurrentPos.get() < 10)
					oldtarget = sa::TARGET_SIDE_1;
				else
					oldtarget = sa::TARGET_SIDE_0;
			}
		}

		break;
	}
	case sa::MAGIC_TARGET_SINGLE:				// 針對敵方某一方
	{
		if (oldtarget < 10 || oldtarget >= sa::MAX_ENEMY)
		{
			if (battleCharCurrentPos.get() < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case sa::MAGIC_TARGET_ONE_ROW:				// 針對敵方某一列
	{
		if (battleCharCurrentPos.get() < 10)
		{
			if (oldtarget < 5)
				oldtarget = sa::MAX_ENEMY + 5;
			else if (oldtarget < 10)
				oldtarget = sa::MAX_ENEMY + 6;
			else if (oldtarget < 15)
				oldtarget = sa::MAX_ENEMY + 3;
			else
				oldtarget = sa::MAX_ENEMY + 4;
		}
		else
		{
			if (oldtarget < 5)
				oldtarget = sa::MAX_ENEMY + 5 - 10;
			else if (oldtarget < 10)
				oldtarget = sa::MAX_ENEMY + 6 - 10;
			else if (oldtarget < 15)
				oldtarget = sa::MAX_ENEMY + 3 - 10;
			else
				oldtarget = sa::MAX_ENEMY + 4 - 10;
		}
		break;
	}
	case sa::MAGIC_TARGET_ALL_ROWS:				// 針對敵方所有人
	{
		if (battleCharCurrentPos.get() < 10)
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
bool Worker::fixCharTargetByItemIndex(long long itemIndex, long long oldtarget, long long* target) const
{
	if (!target)
		return false;

	if (itemIndex < sa::CHAR_EQUIPSLOT_COUNT || itemIndex >= sa::MAX_ITEM)
		return false;

	long long itemType = getItem(itemIndex).target;

	switch (itemType)
	{
	case sa::ITEM_TARGET_MYSELF:
	{
		if (oldtarget != 0)
			oldtarget = 0;
		break;
	}
	case sa::ITEM_TARGET_OTHER://雙方任意單體
	{
		if (oldtarget < 0 || oldtarget >= sa::MAX_ENEMY)
		{
			if (battleCharCurrentPos.get() < 10)
				oldtarget = 15;
			else
				oldtarget = 5;
		}
		break;
	}
	case sa::ITEM_TARGET_ALLMYSIDE://我方全體
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_0;
		else
			oldtarget = sa::TARGET_SIDE_1;
		break;
	}
	case sa::ITEM_TARGET_ALLOTHERSIDE://敵方全體
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_1;
		else
			oldtarget = sa::TARGET_SIDE_0;
		break;
	}
	case sa::ITEM_TARGET_ALL://雙方全體同時(場地)
	{
		oldtarget = sa::TARGET_ALL;
		break;
	}
	case sa::ITEM_TARGET_NONE://無
	{
		oldtarget = -1;
		break;
	}
	case sa::ITEM_TARGET_OTHERWITHOUTMYSELF://我方任意除了自己
	{
		if (oldtarget == battleCharCurrentPos.get())
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::ITEM_TARGET_WITHOUTMYSELFANDPET://我方任意除了自己和寵物
	{
		if (oldtarget == battleCharCurrentPos.get())
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.get() + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::ITEM_TARGET_PET://戰寵
	{
		oldtarget = battleCharCurrentPos.get() + 5;
		break;
	}

	default:
		break;
	}

	*target = oldtarget;
	return true;
}

//戰鬥修正寵物技能目標範圍
bool Worker::fixPetTargetBySkillIndex(long long skillIndex, long long oldtarget, long long* target) const
{
	if (!target)
		return false;

	if (skillIndex < 0 || skillIndex >= sa::MAX_PET_SKILL)
		return false;

	sa::character_t pc = getCharacter();
	if (pc.battlePetNo < 0 || pc.battlePetNo >= sa::MAX_PET)
		return false;

	long long skillType = getPetSkill(pc.battlePetNo, skillIndex).target;

	switch (skillType)
	{
	case sa::PETSKILL_TARGET_MYSELF:
	{
		if (oldtarget != (battleCharCurrentPos.get() + 5))
			oldtarget = battleCharCurrentPos.get() + 5;
		break;
	}
	case sa::PETSKILL_TARGET_OTHER:
	{
		break;
	}
	case sa::PETSKILL_TARGET_ALLMYSIDE:
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_0;
		else
			oldtarget = sa::TARGET_SIDE_1;
		break;
	}
	case sa::PETSKILL_TARGET_ALLOTHERSIDE:
	{
		if (battleCharCurrentPos.get() < 10)
			oldtarget = sa::TARGET_SIDE_1;
		else
			oldtarget = sa::TARGET_SIDE_0;
		break;
	}
	case sa::PETSKILL_TARGET_ALL:
	{
		oldtarget = sa::TARGET_ALL;
		break;
	}
	case sa::PETSKILL_TARGET_NONE:
	{
		oldtarget = 0;
		break;
	}
	case sa::PETSKILL_TARGET_OTHERWITHOUTMYSELF:
	{
		if (oldtarget == (battleCharCurrentPos.get() + 5))
		{
			oldtarget = -1;
		}
		break;
	}
	case sa::PETSKILL_TARGET_WITHOUTMYSELFANDPET:
	{
		long long max = sa::MAX_ENEMY;
		long long min = 0;
		long long row = sa::TARGET_SIDE_1_F_ROW;
		if (battleCharCurrentPos.get() >= 10)
		{
			max = 19;
			min = 10;
			row = sa::TARGET_SIDE_0_F_ROW;
		}

		if (oldtarget < min || oldtarget > max)
		{
			oldtarget = -1;
		}
		else if (oldtarget == (battleCharCurrentPos.get() + 5))
		{
			oldtarget = -1;
		}
		else
		{
			oldtarget = row;
		}
		break;
	}
	}

	*target = oldtarget;

	return true;
}

//戰鬥人物普通攻擊
bool Worker::sendBattleCharAttackAct(long long target)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::battle_data_t bt = getBattleData();
		if (target < 0 || target >= sa::MAX_ENEMY)
			target = getBattleSelectableEnemyTarget(bt);

		sa::battle_object_t obj = bt.objects.value(target);

		const QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
		const QString text(QObject::tr("use attack [%1]%2").arg(target + 1).arg(name));

		const QString qcmd = QString("H|%1").arg(util::toQString(target, 16)).toUpper();

		if (!lssproto_B_send(qcmd))
			break;

		labelCharAction.set(text);

		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#FF5050"));

		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物使用精靈
bool Worker::sendBattleCharMagicAct(long long magicIndex, long long  target)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::battle_data_t bt = getBattleData();
		if (target < 0)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		if (magicIndex < 0 || magicIndex >= sa::MAX_MAGIC)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		sa::magic_t magic = getMagic(magicIndex);
		if (!magic.valid)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		const QString magicName = magic.name;
		QString text("");

		if (target < sa::MAX_ENEMY)
		{
			sa::battle_object_t obj = bt.objects.value(target);
			QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
			text = QObject::tr("use magic %1 to [%2]%3").arg(magicName).arg(target + 1).arg(name);
		}
		else
		{
			text = QObject::tr("use %1 to %2").arg(magicName).arg(getAreaString(target));
		}

		QColor color("#FF5050");

		if (magic.memo.contains("力回"))//補血
		{
			//不可補敵方
			if (target >= bt.enemymin && target <= bt.enemymax
				|| (battleCharCurrentPos.get() < 10 && target == sa::TARGET_SIDE_1)
				|| (battleCharCurrentPos.get() >= 10 && target == sa::TARGET_SIDE_0))
				return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

			color = QColor("#49BF45");
		}

		const QString qcmd = QString("J|%1|%2").arg(util::toQString(magicIndex, 16)).arg(util::toQString(target, 16)).toUpper();
		if (!lssproto_B_send(qcmd))
			break;

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, color);
		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物使用職業技能
bool Worker::sendBattleCharJobSkillAct(long long skillIndex, long long target)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::battle_data_t bt = getBattleData();
		if (target < 0)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		if (skillIndex < 0 || skillIndex >= sa::MAX_PROFESSION_SKILL)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		sa::profession_skill_t skill = getSkill(skillIndex);
		if (!skill.valid)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		const QString skillName = skill.name.simplified();
		QString text("");
		if (target < sa::MAX_ENEMY)
		{
			sa::battle_object_t obj = bt.objects.value(target);
			QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
			text = QObject::tr("use skill %1 to [%2]%3").arg(skillName).arg(target + 1).arg(name);
		}
		else
		{
			text = QObject::tr("use %1 to %2").arg(skillName).arg(getAreaString(target));
		}

		QColor color("#FF5050");
		if (skill.memo.contains("力回"))//補血
		{
			//不可補敵方
			if (target >= bt.enemymin && target <= bt.enemymax
				|| (battleCharCurrentPos.get() < 10 && target == sa::TARGET_SIDE_1)
				|| (battleCharCurrentPos.get() >= 10 && target == sa::TARGET_SIDE_0))
				return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

			color = QColor("#49BF45");
		}

		const QString qcmd = QString("P|%1|%2").arg(util::toQString(skillIndex, 16)).arg(util::toQString(target, 16)).toUpper();
		if (!lssproto_B_send(qcmd))
			break;

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, color);

		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物使用道具
bool Worker::sendBattleCharItemAct(long long itemIndex, long long target)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::battle_data_t bt = getBattleData();
		if (target < 0)
			return 	sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		if (itemIndex < 0 || itemIndex >= sa::MAX_ITEM)
			return 	sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		sa::item_t item = getItem(itemIndex);
		if (!item.valid)
			return 	sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		const QString itemName = item.name.simplified();

		QString text("");
		if (target < sa::MAX_ENEMY)
		{
			sa::battle_object_t obj = bt.objects.value(target);
			QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
			text = QObject::tr("use item %1 to [%2]%3").arg(itemName).arg(target + 1).arg(name);
		}
		else
		{
			text = QObject::tr("use %1 to %2").arg(itemName).arg(getAreaString(target));
		}

		QColor color("#FF5050");
		if (item.memo.contains("力回"))//補血
		{
			//不可補敵方
			if (target >= bt.enemymin && target <= bt.enemymax
				|| (battleCharCurrentPos.get() < 10 && target == sa::TARGET_SIDE_1)
				|| (battleCharCurrentPos.get() >= 10 && target == sa::TARGET_SIDE_0))
				return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

			color = QColor("#49BF45");
		}

		const QString qcmd = QString("I|%1|%2").arg(util::toQString(itemIndex, 16)).arg(util::toQString(target, 16)).toUpper();
		if (!lssproto_B_send(qcmd))
			break;

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, color);
		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物防禦
bool Worker::sendBattleCharDefenseAct()
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		const QString qcmd("G");
		if (!lssproto_B_send(qcmd))
			break;

		const QString text = QObject::tr("defense");

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos.get(), QColor("#CCB157"));
		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物逃跑
bool Worker::sendBattleCharEscapeAct()
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			return false;

		const QString qcmd("E");
		if (!lssproto_B_send(qcmd))
			break;

		const QString text(QObject::tr("escape"));

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);

		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物捉寵
bool Worker::sendBattleCharCatchPetAct(long long target)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::battle_data_t bt = getBattleData();
		if (target < 0 || target >= sa::MAX_ENEMY)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		const QString qcmd = QString("T|%1").arg(util::toQString(target, 16)).toUpper();
		if (!lssproto_B_send(qcmd))
			break;

		sa::battle_object_t obj = bt.objects.value(target);
		QString name = !obj.name.isEmpty() ? obj.name : obj.freeName;
		const QString text(QObject::tr("catch [%1]%2").arg(target + 1).arg(name));

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, QColor("#49BF45"));
		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物切換戰寵
bool Worker::sendBattleCharSwitchPetAct(long long petIndex)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::battle_data_t bt = getBattleData();
		if (petIndex < 0 || petIndex >= sa::MAX_PET)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		sa::pet_t pet = getPet(petIndex);
		if (!pet.valid)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		if (pet.hp <= 0)
			return sendBattleCharAttackAct(getBattleSelectableEnemyTarget(bt));

		const QString qcmd = QString("S|%1").arg(util::toQString(petIndex, 16));
		if (!lssproto_B_send(qcmd))
			break;

		QString text(QObject::tr("switch pet to %1").arg(!pet.freeName.isEmpty() ? pet.freeName : pet.name) + QString("[%1]").arg(petIndex + 1));

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);

		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥人物什麼都不做
bool Worker::sendBattleCharDoNothing()
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		const QString qcmd("N");
		if (!lssproto_B_send(qcmd))
			break;

		QString text(QObject::tr("do nothing"));

		labelCharAction.set(text);
		emit signalDispatcher.updateLabelCharAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos.get(), QColor("#696969"));

		return true;
	} while (false);

	labelCharAction.set("");
	emit signalDispatcher.updateLabelCharAction("");
	return false;
}

//戰鬥戰寵技能
bool Worker::sendBattlePetSkillAct(long long skillIndex, long long target)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::character_t pc = getCharacter();
		if (pc.battlePetNo < 0 || pc.battlePetNo >= sa::MAX_PET)
			break;

		if (target < 0)
			return sendBattlePetDoNothing();

		if (skillIndex < 0 || skillIndex >= sa::MAX_PET_SKILL)
			return sendBattlePetDoNothing();

		QString text("");
		sa::pet_skill_t petSkill = getPetSkill(pc.battlePetNo, skillIndex);
		sa::battle_data_t bt = getBattleData();
		if (target < sa::MAX_ENEMY)
		{
			QString name;
			if (petSkill.name != "防禦" && petSkill.name != "防御")
			{
				sa::battle_object_t obj = bt.objects.value(target);
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

		QColor color("#FF5050");
		if (petSkill.name.contains("防"))//防禦
			color = QColor("#CCB157");
		else if (petSkill.memo.contains("力回"))//補血
		{
			//不可補敵方
			if (target >= bt.enemymin && target <= bt.enemymax
				|| (battleCharCurrentPos.get() < 10 && target == sa::TARGET_SIDE_1)
				|| (battleCharCurrentPos.get() >= 10 && target == sa::TARGET_SIDE_0))
				return sendBattlePetDoNothing();

			color = QColor("#49BF45");
		}

		const QString qcmd = QString("W|%1|%2").arg(util::toQString(skillIndex, 16)).arg(util::toQString(target, 16)).toUpper();
		if (!lssproto_B_send(qcmd))
			break;

		labelPetAction.set(text);
		emit signalDispatcher.updateLabelPetAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(target, color);

		return true;
	} while (false);

	labelPetAction.set("");
	emit signalDispatcher.updateLabelPetAction("");
	return false;
}

//戰鬥戰寵什麼都不做
bool Worker::sendBattlePetDoNothing()
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	do
	{
		if (!getBattleFlag())
			break;

		sa::character_t pc = getCharacter();
		if (pc.battlePetNo < 0 || pc.battlePetNo >= sa::MAX_PET)
			break;

		const QString qcmd("W|FF|FF");
		if (!lssproto_B_send(qcmd))
			break;

		QString text(QObject::tr("do nothing"));

		labelPetAction.set(text);
		emit signalDispatcher.updateLabelPetAction(text);
		emit signalDispatcher.battleTableItemForegroundColorChanged(battleCharCurrentPos.get() + 5, QColor("#696969"));
		return true;
	} while (false);

	labelPetAction.set("");
	emit signalDispatcher.updateLabelPetAction("");
	return false;
}

#pragma endregion

#pragma region Lssproto_Recv
//組隊變化
void Worker::lssproto_PR_recv(long long request, long long result)
{
	QStringList teamInfoList;

	if (request == 1 && result == 1)
	{
		sa::character_t pc = getCharacter();
		pc.status |= sa::kCharacterStatus_HasTeam;
		setCharacter(pc);

		QHash<long long, sa::team_t> team = team_.toHash();
		for (long long i = 0; i < sa::MAX_TEAM; ++i)
		{
			if ((!team.value(i).valid) || (team.value(i).maxHp <= 0))
			{
				teamInfoList.append("");
				continue;
			}

			QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(team.value(i).name).arg(team.value(i).level)
				.arg(team.value(i).hp).arg(team.value(i).maxHp).arg(team.value(i).hpPercent);
			teamInfoList.append(text);
		}
	}
	else
	{
		if (request == 0 && result == 1)
		{
			long long i;
			QHash<long long, sa::team_t> team = team_.toHash();
			for (i = 0; i < sa::MAX_TEAM; ++i)
			{
				team.remove(i);
				teamInfoList.append("");
			}
			team_ = team;
		}
	}

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateTeamInfo(teamInfoList);
}

//地圖轉移
void Worker::lssproto_EV_recv(long long dialogid, long long result)
{
	//对客户端的EV事件进行回应。在收到此回应之前，客户端将无法执行其他动作，如行走等。
	std::ignore = dialogid;
	std::ignore = result;
	std::ignore = getPoint();
	std::ignore = getFloorName();
	std::ignore = getDir();
	long long currentIndex = getIndex();
	std::ignore = getFloor();
}

//天氣
void Worker::lssproto_EF_recv(long long effect, long long level, char* coption)
{
	std::ignore = effect;
	std::ignore = level;
	std::ignore = coption;
	std::ignore = getPoint();
	std::ignore = getFloorName();
	std::ignore = getDir();
	std::ignore = getFloor();
}

//開關切換
void Worker::lssproto_FS_recv(long long flg)
{
	{
		sa::character_t pc = getCharacter();
		pc.etcFlag = flg;
		setCharacter(pc);
	}

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	gamedevice.setEnableHash(util::kSwitcherTeamEnable, util::checkAND(flg, sa::PC_ETCFLAG_GROUP));//組隊開關
	gamedevice.setEnableHash(util::kSwitcherPKEnable, util::checkAND(flg, sa::PC_ETCFLAG_PK));//決鬥開關
	gamedevice.setEnableHash(util::kSwitcherCardEnable, util::checkAND(flg, sa::PC_ETCFLAG_CARD));//名片開關
	gamedevice.setEnableHash(util::kSwitcherTradeEnable, util::checkAND(flg, sa::PC_ETCFLAG_TRADE));//交易開關
	gamedevice.setEnableHash(util::kSwitcherWorldEnable, util::checkAND(flg, sa::PC_ETCFLAG_WORLD));//世界頻道開關
	gamedevice.setEnableHash(util::kSwitcherGroupEnable, util::checkAND(flg, sa::PC_ETCFLAG_TEAM_CHAT));//組隊頻道開關
	gamedevice.setEnableHash(util::kSwitcherFamilyEnable, util::checkAND(flg, sa::PC_ETCFLAG_FM));//家族頻道開關
	gamedevice.setEnableHash(util::kSwitcherJobEnable, util::checkAND(flg, sa::PC_ETCFLAG_JOB));//職業頻道開關

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.applyHashSettingsToUI();
}

//新增名片
void Worker::lssproto_AB_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	long long i = 0;
	long long no = 0;
	QString name;
	long long flag = 0;
	bool valid = false;
#ifdef _MAILSHOWPLANET				//顯示名片星球
	QString planetid;
	long long j = 0;
#endif

	for (i = 0; i < sa::MAX_ADDRESS_BOOK; ++i)
	{
		//no = i * 6; //the second
		no = i * 8;
		valid = getIntegerToken(data, "|", no + 1) > 0;
		sa::address_bool_t addressBook = getAddressBook(i);
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
void Worker::lssproto_ABI_recv(long long num, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString name;
	//long long nameLen;
	bool valid;
#ifdef _MAILSHOWPLANET				// (可開放) 顯示名片星球
	QString planetid[8];
	long long j;
#endif

	if (num >= sa::MAX_ADDRESS_BOOK)
		return;

	valid = getIntegerToken(data, "|", 1) > 0;

	sa::address_bool_t addressBook = getAddressBook(num);
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
void Worker::lssproto_RS_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	setBattleEnd();

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	long long i;
	QString token;
	QString item;

	//cary 確定 欄位 數
	long long cols = sa::RESULT_CHR_EXP;
	getStringToken(data, ",", sa::RESULT_CHR_EXP + 1, token);
	if (token[0] == 0)
	{
		cols = sa::RESULT_CHR_EXP - 1;
		//battleResultMsg.resChr[RESULT_CHR_EXP - 1].petNo = -1;
		//battleResultMsg.resChr[RESULT_CHR_EXP - 1].levelUp = -1;
		//battleResultMsg.resChr[RESULT_CHR_EXP - 1].exp = -1;
	}
	//end cary
	const QString playerExp = QObject::tr("player exp:");
	const QString rideExp = QObject::tr("ride exp:");
	const QString petExp = QObject::tr("pet exp:");
	sa::character_t pc = getCharacter();
	bool petOk = false;
	bool rideOk = false;
	bool charOk = false;
	QStringList texts = { playerExp + "0", rideExp + "0", petExp + "0" };

	for (i = 0; i < cols; ++i)
	{
		if (i >= sa::RESULT_CHR_EXP)
			continue;

		if (charOk && petOk && rideOk)
			continue;

		getStringToken(data, ",", i + 1, token);
		if (token.isEmpty())
			continue;

		long long index = getIntegerToken(token, "|", 1);
		if (index == -1)
			continue;

		bool isLevelUp = getIntegerToken(token, "|", 2) > 0;

		QString temp;
		getStringToken(token, "|", 3, temp);
		long long exp = a62toi(temp);
		if (exp <= 0)
			continue;

		if (index == -2 && !charOk)
		{
			charOk = true;
			if (isLevelUp)
				++afkRecords[0].leveldifference;

			afkRecords[0].expdifference += exp;
			texts[0] = playerExp + util::toQString(exp);
			continue;
		}

		if (index < 0 || index >= (sa::MAX_PET + 1))
			continue;

		if (pc.ridePetNo != -1 && pc.ridePetNo == index && !petOk)
		{
			petOk = true;
			if (isLevelUp)
				++afkRecords[index].leveldifference;

			afkRecords[index].expdifference += exp;
			texts[1] = rideExp + util::toQString(exp);
			continue;
		}

		if (pc.battlePetNo != -1 && pc.battlePetNo == index && !rideOk)
		{
			rideOk = true;
			if (isLevelUp)
				++afkRecords[index].leveldifference;

			afkRecords[index].expdifference += exp;
			texts[2] = petExp + util::toQString(exp);
			continue;
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

	do
	{
		if (token.isEmpty())
			break;

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

		if (!texts.isEmpty() && gamedevice.getEnableHash(util::kShowExpEnable))
			announce(texts.join(" "));
	} while (false);


	if (gamedevice.getEnableHash(util::kAutoDropMeatEnable))
		checkAutoDropMeat();
	if (gamedevice.getEnableHash(util::kAutoAbilityEnable))
	{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		std::ignore = QtConcurrent::run(&Worker::checkAutoAbility, this);
#else
		std::ignore = QtConcurrent::run(this, &Worker::checkAutoAbility);
#endif
	}
	if (gamedevice.getEnableHash(util::kDropPetEnable))
		checkAutoDropPet();
}

//戰後積分改變
void Worker::lssproto_RD_recv(char*)
{
	//用于在DUEL结束后通知客户端获得或失去的DUEL积分
	//RD|得た(失った)DP(62進)|最終的なDP(62進)|
	setBattleEnd();
	//QString data = util::toUnicode(cdata);
	//if (data.isEmpty())
	//	return;

	//battleResultMsg.resChr[0].exp = getInteger62Token(data, "|", 1);
	//battleResultMsg.resChr[1].exp = getInteger62Token(data, "|", 2);
}

//道具位置交換
void Worker::lssproto_SI_recv(long long from, long long to)
{
	swapItemLocal(from, to);
	updateItemByMemory();
	refreshItemInfo();
	updateComboBoxList();
}

//道具數據改變
void Worker::lssproto_I_recv(char* cdata)
{
	{
		QWriteLocker locker(&itemInfoLock_);

		QString data = util::toUnicode(cdata);
		if (data.isEmpty())
			return;

		long long i, j;
		long long no;
		QString name;
		QString name2;
		QString memo;
		//char *data = "9|烏力斯坦的肉||0|耐久力10前後回覆|24002|0|1|0|7|不會損壞|1|肉|20||10|烏力斯坦的肉||0|耐久力10前後回覆|24002|0|1|0|7|不會損壞|1|肉|20|";

		QHash <long long, sa::item_t> items = item_.toHash();
		for (j = 0; ; ++j)
		{
			no = j * 15;

			i = getIntegerToken(data, "|", no + 1);//道具位
			if (getStringToken(data, "|", no + 2, name) == 1)//道具名
				break;

			if (i < 0 || i >= sa::MAX_ITEM)
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
			damage.remove("%");
			damage.remove("％");
			bool ok = false;
			items[i].damage = damage.toLongLong(&ok);
			if (!ok)
				items[i].damage = 100;

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
	if (IS_WAITOFR_ITEM_CHANGE_PACKET.get() > 0)
		IS_WAITOFR_ITEM_CHANGE_PACKET.dec();

	checkAutoEatBoostExpItem();

	checkAutoDropItems();
}

//對話框
void Worker::lssproto_WN_recv(long long windowtype, long long buttontype, long long dialogid, long long unitid, char* cdata)
{
	QString data = util::toUnicode(cdata, false);
	if (data.isEmpty() && buttontype == 0)
		return;

	//第一部分是把按鈕都拆出來
	QString newText = data.trimmed();
	newText.replace("\\r\\n", "\n");
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

		for (long long i = 0; i < strList.size(); ++i)
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

		for (long long i = 0; i < strList.size(); ++i)
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

	static const QStringList SecurityCodeList = {
		 "安全密碼進行解鎖", "安全密码进行解锁"
	};

	//static const QStringList KNPCList = {
	//	//zh_TW
	//	"如果能贏過我的"/*院藏*/, "如果想通過"/*近藏*/, "吼"/*紅暴*/, "你想找麻煩"/*七兄弟*/, "多謝～。",
	//	"轉以上確定要出售？", "再度光臨", "已經太多",

	//	//zh_CN
	//	"如果能赢过我的"/*院藏*/, "如果想通过"/*近藏*/, "吼"/*红暴*/, "你想找麻烦"/*七兄弟*/, "多謝～。",
	//	"转以上确定要出售？", "再度光临", "已经太多",
	//};
	static const QRegularExpression rexBankPet(R"(LV\.\s*(\d+)\s*MaxHP\s*(\d+)\s*(\S+))");

	//這個是特殊訊息
	static const QRegularExpression rexExtraInfoBig5(R"((?:.*?豆子倍率：\d*(?:\.\d+)?剩余(\d+(?:\.\d+)?)分鐘)?\s+\S+\s+聲望：(\d+)\s+氣勢：(\d+)\s+貝殼：(\d+)\s+活力：(\d+)\s+積分：(\d+)\s+會員點：(\d+))");
	static const QRegularExpression rexExtraInfoGb2312(R"((?:.*?豆子倍率：\d*(?:\.\d+)?剩余(\d+(?:\.\d+)?)分钟)?\s+\S+\s+声望：(\d+)\s+气势：(\d+)\s+贝壳：(\d+)\s+活力：(\d+)\s+积分：(\d+)\s+会员点：(\d+))");

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

	currentDialog.set(sa::dialog_t{ windowtype, buttontype, dialogid, unitid, data, linedatas, strList });

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
				sa::bank_pet_t bankPet;
				bankPet.level = itMatch.captured(1).toLongLong();
				bankPet.maxHp = itMatch.captured(2).toLongLong();
				bankPet.name = itMatch.captured(3);
				currentBankPetList.second.append(bankPet);
			}
		}
		IS_WAITFOR_BANK_FLAG.off();
		return;
	}


	if (data.contains("寄放店", Qt::CaseInsensitive))
	{
		currentBankItemList.clear();
		long long index = 0;
		for (;;)
		{
			sa::item_t item = {};
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
		IS_WAITFOR_BANK_FLAG.off();
		return;
	}

	//匹配額外訊息
	QRegularExpressionMatch extraInfoMatch = rexExtraInfoBig5.match(data);
	if (extraInfoMatch.hasMatch())
	{
		sa::currency_data_t currency;
		long long n = 1;
		if (extraInfoMatch.lastCapturedIndex() == 7)
			currency.expbufftime = static_cast<long long>(qFloor(extraInfoMatch.captured(n++).toDouble() * 60.0));
		else
			currency.expbufftime = 0;

		currency.prestige = extraInfoMatch.captured(n++).toLongLong();
		currency.energy = extraInfoMatch.captured(n++).toLongLong();
		currency.shell = extraInfoMatch.captured(n++).toLongLong();
		currency.vitality = extraInfoMatch.captured(n++).toLongLong();
		currency.points = extraInfoMatch.captured(n++).toLongLong();
		currency.VIPPoints = extraInfoMatch.captured(n++).toLongLong();

		if (afkRecords[0].reprecord == 0)
		{
			afkRecords[0].reprecord = currency.prestige;
			repTimer.restart();
		}
		else
		{
			afkRecords[0].repearn = currency.prestige - afkRecords[0].reprecord;
			if (afkRecords[0].repearn <= 0)
			{
				afkRecords[0].repearn = 0;
				afkRecords[0].reprecord = currency.prestige;
				repTimer.restart();
			}
		}

		currencyData.set(currency);
	}
	else
	{
		extraInfoMatch = rexExtraInfoGb2312.match(data);
		if (extraInfoMatch.hasMatch())
		{
			sa::currency_data_t currency;
			long long n = 1;
			if (extraInfoMatch.lastCapturedIndex() == 7)
				currency.expbufftime = static_cast<long long>(qFloor(extraInfoMatch.captured(n++).toDouble() * 60.0));
			else
				currency.expbufftime = 0;
			currency.prestige = extraInfoMatch.captured(n++).toLongLong();
			currency.energy = extraInfoMatch.captured(n++).toLongLong();
			currency.shell = extraInfoMatch.captured(n++).toLongLong();
			currency.vitality = extraInfoMatch.captured(n++).toLongLong();
			currency.points = extraInfoMatch.captured(n++).toLongLong();
			currency.VIPPoints = extraInfoMatch.captured(n++).toLongLong();

			if (afkRecords[0].reprecord == 0)
			{
				afkRecords[0].reprecord = currency.prestige;
				repTimer.restart();
			}
			else
			{
				afkRecords[0].repearn = currency.prestige - afkRecords[0].reprecord;
				if (afkRecords[0].repearn <= 0)
				{
					afkRecords[0].repearn = 0;
					afkRecords[0].reprecord = currency.prestige;
					repTimer.restart();
				}
			}

			currencyData.set(currency);
		}
	}

	//這裡開始是 KNPC
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	data = data.simplified();

	for (const QString& it : SecurityCodeList)
	{
		if (!data.contains(it, Qt::CaseInsensitive))
			continue;

		QString securityCode = gamedevice.getStringHash(util::kGameSecurityCodeString);
		if (!securityCode.isEmpty())
		{
			unlockSecurityCode(securityCode);
			return;
		}
	}

	QString knpcListString = gamedevice.getStringHash(util::kKNPCListString);
	if (gamedevice.getEnableHash(util::kKNPCEnable) && !knpcListString.isEmpty())
	{
		KNPCDevice knpcDevice(knpcListString);

		for (const KNPCDevice::action_t& action : knpcDevice)
		{
			if (data.contains(action.cmpText, Qt::CaseInsensitive))
			{
				switch (action.type)
				{
				case KNPCDevice::Button:
					press(static_cast<sa::ButtonType>(action.action.toLongLong()), dialogid, unitid);
					break;
				case KNPCDevice::RowButton:
					press(action.action.toLongLong(), dialogid, unitid);
					break;
				case KNPCDevice::Inout:
					inputtext(action.action.toString(), dialogid, unitid);
					break;
				}
				break;
			}
		}
	}

}

//寵郵飛進來
void Worker::lssproto_PME_recv(long long unitid, long long graphicsno, const QPoint& pos, long long dir, long long flg, long long no, char* cdata)
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
		QString data = util::toUnicode(cdata);
		if (data.isEmpty())
			return;

		QString smalltoken;
		long long id;
		long long x;
		long long y;
		long long ldir;
		long long modelid;
		long long level;
		long long nameColor;
		QString name;
		QString freeName;
		long long walkable;
		long long height;
		long long charType;
		long long ps = 2;

		charType = getIntegerToken(data, "|", ps++);
		getStringToken(data, "|", ps++, smalltoken);
		id = a62toi(smalltoken);
		getStringToken(data, "|", ps++, smalltoken);
		x = smalltoken.toLongLong();
		getStringToken(data, "|", ps++, smalltoken);
		y = smalltoken.toLongLong();
		getStringToken(data, "|", ps++, smalltoken);
		ldir = smalltoken.toLongLong();
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

//求救
void Worker::lssproto_HL_recv(long long)
{

}

//開始戰鬥
void Worker::lssproto_EN_recv(long long result, long long field)
{
	//開始戰鬥為1，未開始戰鬥為0   0：不可遇到或錯誤。 1：與敵人戰鬥OK。 2：PvP戰鬥OK
	if (result > 0)
	{
		setBattleFlag(true);
		IS_LOCKATTACK_ESCAPE_DISABLE_.off();
		battleCharEscapeFlag.off();
		battleBackupThreadFlag_.off();
		battle_total.inc();
		battlePetDisableList_.clear();
		battlePetDisableList_.resize(sa::MAX_PET);
		normalDurationTimer.restart();
		battleDurationTimer.restart();
		oneRoundDurationTimer.restart();
		battleCurrentRound.set(-1);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		emit signalDispatcher.battleTableAllItemResetColor();

		long long currentIndex = getIndex();
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

		if (!gamedevice.getEnableHash(util::kBattleLuaModeEnable))
			return;

		auto getFilePath = [](const QString& name)->QString
			{
				QStringList battleLuaFiles;
				util::searchFiles(util::applicationDirPath(), name, ".lua", &battleLuaFiles);
				if (battleLuaFiles.isEmpty())
					return "";

				return battleLuaFiles.first();
			};

		if (!QFile::exists(battleCharLuaScriptPath_))
			battleCharLuaScriptPath_ = getFilePath("battle_char");

		if (!battleCharLuaScriptPath_.isEmpty())
			util::readFile(battleCharLuaScriptPath_, &battleCharLuaScript_);

		if (!QFile::exists(battlePetLuaScriptPath_))
			battlePetLuaScriptPath_ = getFilePath("battle_pet");

		if (!battlePetLuaScriptPath_.isEmpty())
			util::readFile(battlePetLuaScriptPath_, &battlePetLuaScript_);

		if (battleCharLuaScriptPath_.isEmpty() && battlePetLuaScriptPath_.isEmpty())
			return;

		if (nullptr == battleLua)
		{
			battleLua.reset(new CLua(getIndex()));
			sash_assume(battleLua != nullptr);
			if (nullptr == battleLua)
			{
				return;
			}

			battleLua->setHookEnabled(true);
			battleLua->setHookForBattle(true);
			battleLua->openlibs();
		}
	}
}

//戰場動態字串變量轉換
QString Worker::battleStringFormat(const sa::battle_object_t& obj, QString formatStr) const
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	//"[%(pos)]%(self)%(name) LV:%(lv)(%(hp)|%(hpp))%(status)"
	formatStr.replace("%(pos)", util::toQString(obj.pos + 1), Qt::CaseInsensitive);

	bool isself = obj.pos == battleCharCurrentPos.get();
	if ((obj.hp > 0) && (isself || (obj.pos == (battleCharCurrentPos.get() + 5))))
	{
		// Divide by 10 to get the real element values
		long long earth = 0;
		long long water = 0;
		long long fire = 0;
		long long wind = 0;

		sa::character_t pc = getCharacter();
		if (isself)
		{
			earth = pc.earth / 10;
			water = pc.water / 10;
			fire = pc.fire / 10;
			wind = pc.wind / 10;
		}
		else
		{
			sa::pet_t pet = getPet(pc.battlePetNo);
			earth = pet.earth / 10;
			water = pet.water / 10;
			fire = pet.fire / 10;
			wind = pet.wind / 10;
		}

		bool isReverse = util::checkAND(obj.status, sa::BC_FLG_REVERSE);

		QString elementStr;

		if (isself && isReverse)
		{
			// Reverse the elements
			std::swap(fire, water);
			std::swap(earth, wind);
		}

		// Append non-zero elements to the string
		if (fire > 0)
		{
			elementStr += QObject::tr("fire") + util::toQString(fire);
		}
		if (water > 0)
		{
			elementStr += QObject::tr("water") + util::toQString(water);
		}
		if (earth > 0)
		{
			elementStr += QObject::tr("earth") + util::toQString(earth);
		}
		if (wind > 0)
		{
			elementStr += QObject::tr("wind") + util::toQString(wind);
		}

		if (isself)
		{
			formatStr.replace("%(pele)", "", Qt::CaseInsensitive);
			formatStr.replace("%(ele)", elementStr, Qt::CaseInsensitive);
		}
		else
		{
			formatStr.replace("%(pele)", elementStr, Qt::CaseInsensitive);
			formatStr.replace("%(ele)", "", Qt::CaseInsensitive);
		}
	}

	formatStr.replace("%(self)", isself ? gamedevice.getStringHash(util::kBattleSelfMarkString) : "", Qt::CaseInsensitive);
	formatStr.replace("%(name)", obj.name, Qt::CaseInsensitive);
	formatStr.replace("%(fname)", obj.freeName, Qt::CaseInsensitive);
	formatStr.replace("%(lv)", util::toQString(obj.level), Qt::CaseInsensitive);
	formatStr.replace("%(hp)", util::toQString(obj.hp), Qt::CaseInsensitive);
	formatStr.replace("%(maxhp)", util::toQString(obj.maxHp), Qt::CaseInsensitive);
	formatStr.replace("%(hpp)", util::toQString(obj.hpPercent), Qt::CaseInsensitive);

	formatStr.replace("%(mp)", isself ? util::toQString(battleCharCurrentMp.get()) : "", Qt::CaseInsensitive);

	sa::character_t pc = getCharacter();
	formatStr.replace("%(maxmp)", isself ? util::toQString(pc.maxMp) : "", Qt::CaseInsensitive);
	formatStr.replace("%(mpp)", isself ? util::toQString(pc.mpPercent) : "", Qt::CaseInsensitive);

	formatStr.replace("%(mod)", util::toQString(obj.modelid), Qt::CaseInsensitive);

	if (formatStr.contains("%(status)", Qt::CaseInsensitive))
	{
		QString statusStr = getBadStatusString(obj.status);
		if (!statusStr.isEmpty())
			statusStr = QString("%1").arg(statusStr);
		formatStr.replace("%(status)", statusStr, Qt::CaseInsensitive);
	}

	if (obj.rideFlag > 0)
	{
		formatStr.replace("%(rlv)", util::toQString(obj.rideLevel), Qt::CaseInsensitive);
		formatStr.replace("%(rhp)", util::toQString(obj.rideHp), Qt::CaseInsensitive);
		formatStr.replace("%(rmaxhp)", util::toQString(obj.rideMaxHp), Qt::CaseInsensitive);
		formatStr.replace("%(rhpp)", util::toQString(obj.rideHpPercent), Qt::CaseInsensitive);
		formatStr.replace("%(rname)", obj.rideName);

		formatStr.remove("%(rb)", Qt::CaseInsensitive);
		formatStr.remove("%(re)", Qt::CaseInsensitive);
	}
	else
	{
		formatStr.remove("%(rlv)", Qt::CaseInsensitive);
		formatStr.remove("%(rhp)", Qt::CaseInsensitive);
		formatStr.remove("%(rmaxhp)", Qt::CaseInsensitive);
		formatStr.remove("%(rhpp)", Qt::CaseInsensitive);
		formatStr.remove("%(rname)", Qt::CaseInsensitive);

		long long i = formatStr.indexOf("%(rb)", 0, Qt::CaseInsensitive);//begin remove from
		long long j = formatStr.indexOf("%(re)", 0, Qt::CaseInsensitive);//end remove to
		if (i != -1)
			formatStr.remove(i, j - i + 5);
	}

	formatStr.remove("()");
	formatStr.remove("[]");
	formatStr.remove("<>");
	formatStr.remove("{}");

	formatStr.prepend(gamedevice.getStringHash(util::kBattleSpaceMarkString));

	return formatStr;
}

//戰鬥每回合資訊
void Worker::lssproto_B_recv(char* ccommand)
{
	QString command = util::toUnicode(ccommand);
	if (command.isEmpty())
		return;

	QString first = command.left(2).toUpper();
	if (first.startsWith("B"))
		first.remove(0, 1);//移除開頭的B

	QString data = command.mid(3);//紀錄 "XX|" 後的數據
	if (data.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	HANDLE hProcess = gamedevice.getProcess();
	long long hModule = gamedevice.getProcessModule();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	switch (first.at(0).unicode())
	{
	case 'P'://BP|自己的编号[0～19](%X)|標誌位(%X)|當前氣(%X)|???
	{
		//標誌位: 遭遇偷襲 出奇不意或擁有迴旋鏢

		QStringList list = data.split(util::rexOR);
		if (list.size() < 3)
			break;

		emit signalDispatcher.battleTableAllItemResetColor();

		battle_one_round_time.set(oneRoundDurationTimer.cost());
		oneRoundDurationTimer.restart();

		bool ok = false;
		long long tmpValue = list.value(0).toLongLong(&ok, 16);
		if (ok)
			battleCharCurrentPos.set(tmpValue);

		tmpValue = list.value(1).toLongLong(&ok, 16);
		if (ok)
			battleBpFlag.set(tmpValue);

		tmpValue = list.value(2).toLongLong(&ok, 16);
		if (ok)
			battleCharCurrentMp.set(tmpValue);

		{
			sa::character_t pc = getCharacter();
			pc.mp = battleCharCurrentMp.get();
			pc.mpPercent = util::percent(pc.mp, pc.maxMp);
			setCharacter(pc);
		}

		sa::battle_data_t bt = getBattleData();
		updateCurrentSideRange(&bt);
		setBattleData(bt);
		break;
	}
	case 'A'://BA|命令完成標誌位(%X)|回合數(%X)|
	{
		QStringList list = data.split(util::rexOR);
		if (list.size() < 2)
			break;

		bool ok = false;
		long long tmpValue = list.value(0).toLongLong(&ok, 16);
		if (ok)
			battleCurrentAnimeFlag.set(tmpValue);

		tmpValue = list.value(1).toLongLong(&ok, 16);
		if (ok)
			battleCurrentRound.set(tmpValue);

		if (battleCurrentAnimeFlag.get() <= 0)
			break;

		sa::battle_data_t bt = getBattleData();
		if (!bt.objects.isEmpty())
		{
			QVector<sa::battle_object_t> objs = bt.objects;
			for (long long i = bt.alliemin; i <= bt.alliemax; ++i)
			{
				if (i >= bt.objects.size())
					break;
				if (checkFlagState(i) && !bt.objects.value(i).ready)
				{
#if 0
					if (i == battleCharCurrentPos.get())
					{
						qDebug() << QString("自己 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName);
					}
					if (i == battleCharCurrentPos.get() + 5)
					{
						qDebug() << QString("戰寵 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName);
					}
					else
					{
						qDebug() << QString("隊友 [%1]%2(%3) 已出手").arg(i + 1).arg(bt.objects.value(i, empty).name).arg(bt.objects.value(i, empty).freeName);
					}
#endif
					emit signalDispatcher.notifyBattleActionState(i);//標上我方已出手
					objs[i].ready = true;
				}
			}

			for (long long i = bt.enemymin; i <= bt.enemymax; ++i)
			{
				if (i >= bt.objects.size())
					break;
				if (checkFlagState(i) && !bt.objects.value(i).ready)
				{
					emit signalDispatcher.notifyBattleActionState(i); //標上敵方已出手
					objs[i].ready = true;
				}
			}

			bt.objects = std::move(objs);
		}

		setBattleData(bt);

		asyncBattleAction(true);
		break;
	}
	case 'C':
	{
		sa::battle_data_t bt = getBattleData();

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
		bt.objects.clear();
		bt.objects.resize(sa::MAX_ENEMY);

		long long i = 0, j = 0;
		long long n = 0;

		QString temp;
		QString preOutputInfo;
		QColor preOutputInfoColor = Qt::white;

		//檢查敵我其中一方是否全部陣亡
		bool isEnemyAllDead = true;
		bool isAllieAllDead = true;
		sa::battle_object_t obj = {};
		long long pos = 0;
		bool ok = false;
		bool valid = false;

		battleField.set(getIntegerToken(data, "|", 1));

		auto matchPetByBattleInfo = [this](const sa::battle_object_t& obj, bool isRide)->long long
			{
				QHash<long long, sa::pet_t> hash = pet_.toHash();
				long long key = -1;
				sa::pet_t pet = {};
				for (auto it = hash.begin(); it != hash.end(); ++it)
				{
					key = it.key();
					pet = it.value();

					if (!pet.valid)
						continue;

					if (isRide)
					{
						if (pet.level == obj.rideLevel
							&& pet.maxHp == obj.rideMaxHp
							&& matchPetNameByIndex(key, obj.rideName))
							return key;
					}
					else
					{
						if (pet.modelid > 0
							&& pet.level == obj.level
							&& pet.maxHp == obj.maxHp
							&& matchPetNameByIndex(key, obj.name))
							return key;
					}
				}
				return -1;
			};

		{
			QHash<long long, sa::pet_t> pets = pet_.toHash();
			QVector<bool> existFlags(sa::MAX_ENEMY, false);

			for (;;)
			{
				/*
				string 使用 getStringToken(data, "|", n, var);
				而後使用 makeStringFromEscaped(var) 處理轉譯

				long long 使用 getIntegerToken(data, "|", n);
				*/

				getStringToken(data, "|", i * 13 + 2, temp);
				pos = temp.toLongLong(&ok, 16);
				if (!ok)
					break;

				if (pos < 0 || pos >= sa::MAX_ENEMY)
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
				if (util::checkAND(obj.status, sa::BC_FLG_DEAD))
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

				valid = obj.modelid > 0 && obj.maxHp > 0 && obj.level > 0 && !util::checkAND(obj.status, sa::BC_FLG_HIDE) && !util::checkAND(obj.status, sa::BC_FLG_DEAD);

				if (obj.pos >= 0 && obj.pos < existFlags.size())
					existFlags[obj.pos] = obj.modelid > 0 && obj.maxHp > 0 && obj.level > 0 && !util::checkAND(obj.status, sa::BC_FLG_HIDE);

				if ((pos >= bt.enemymin) && (pos <= bt.enemymax) && obj.rideFlag == 0 && obj.modelid > 0 && !obj.name.isEmpty())
				{
					QStringList _enemyNameListCache = enemyNameListCache.get();
					if (!_enemyNameListCache.contains(obj.name))
					{
						_enemyNameListCache.append(obj.name);

						if (_enemyNameListCache.size() > 1)
						{
							_enemyNameListCache.removeDuplicates();
							QCollator collator = util::getCollator();
							std::sort(_enemyNameListCache.begin(), _enemyNameListCache.end(), collator);
						}

						enemyNameListCache.set(_enemyNameListCache);
						gamedevice.setUserData(util::kUserEnemyNames, _enemyNameListCache);
					}
				}

				if (battleCharCurrentPos.get() == pos)
				{
					sa::character_t pc = getCharacter();
					pc.hp = obj.hp;
					pc.maxHp = obj.maxHp;
					pc.hpPercent = util::percent(obj.hp, obj.maxHp);
					setCharacter(pc);

					if (obj.hp == 0 || util::checkAND(obj.status, sa::BC_FLG_DEAD))
					{
						if (!afkRecords[0].deadthcountflag)
						{
							afkRecords[0].deadthcountflag = true;
							++afkRecords[0].deadthcount;
						}
					}
					else
					{
						afkRecords[0].deadthcountflag = false;
					}

					emit signalDispatcher.updateCharHpProgressValue(obj.level, obj.hp, obj.maxHp);

					//騎寵存在
					if (obj.rideFlag == 1)
					{
						if (-1 == pc.ridePetNo)
						{
							pc.ridePetNo = matchPetByBattleInfo(obj, true);
							setCharacter(pc);
						}

						if (pc.ridePetNo >= 0 && pc.ridePetNo < sa::MAX_PET)
						{
							pets[pc.ridePetNo].valid = true;
							pets[pc.ridePetNo].hp = obj.rideHp;
							pets[pc.ridePetNo].maxHp = obj.rideMaxHp;
							pets[pc.ridePetNo].level = obj.rideLevel;
							pet_ = pets;
						}
					}
					//騎寵不存在
					else
					{
						if (pc.ridePetNo >= 0 && pc.ridePetNo < sa::MAX_PET)
						{
							//落馬
							pets[pc.ridePetNo].hp = 0;
							if (!afkRecords[pc.ridePetNo + 1].deadthcountflag)
							{
								afkRecords[pc.ridePetNo + 1].deadthcountflag = true;
								++afkRecords[pc.ridePetNo + 1].deadthcount;
							}

							pc.ridePetNo = -1;
							setCharacter(pc);
							mem::write <short>(hProcess, hModule + sa::kOffsetRidePetIndex, -1);
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
					bt.objects[pos] = obj;

				if (valid || util::checkAND(obj.status, sa::BC_FLG_HIDE))
				{
					if (obj.pos >= bt.alliemin && obj.pos <= bt.alliemax)
						isAllieAllDead = false;
					else
						isEnemyAllDead = false;
				}

				bool isEnemy = obj.pos >= bt.enemymin && obj.pos <= bt.enemymax;
				preOutputInfoColor = Qt::white;

				if (isEnemy)
				{
					preOutputInfo = battleStringFormat(obj, gamedevice.getStringHash(util::kBattleEnemyFormatString));
					if (obj.level == 1)//一等敵人高亮
						preOutputInfoColor = QColor("#32A3FF");
				}
				else
					preOutputInfo = battleStringFormat(obj, gamedevice.getStringHash(util::kBattleAllieFormatString));

				emit signalDispatcher.updateBattleItemRowContents(obj.pos, preOutputInfo, preOutputInfoColor);

				++i;
			}

			for (i = 0; i < sa::MAX_ENEMY; ++i)
			{
				if (!existFlags.value(i))
				{
					emit signalDispatcher.updateBattleItemRowContents(i, "", Qt::white);
				}
			}

			//更新戰寵數據
			if (battleCharCurrentPos.get() >= 0 && battleCharCurrentPos.get() < bt.objects.size())
			{
				//取戰鬥戰寵數據
				obj = bt.objects.value(battleCharCurrentPos.get() + 5);

				if (!util::checkAND(obj.status, sa::BC_FLG_HIDE/*排除地球一周*/))
				{
					//戰寵不存在
					if ((obj.level <= 0 || obj.maxHp <= 0 || obj.modelid <= 0))
					{
						sa::character_t pc = getCharacter();
						//如果索引原本是在正確範圍內則代表被打飛或逃跑
						if (pc.battlePetNo >= 0 && pc.battlePetNo < sa::MAX_PET)
						{
							//被打飛(也可能是跑走)
							if (!afkRecords[pc.battlePetNo + 1].deadthcount)
							{
								afkRecords[pc.battlePetNo + 1].deadthcountflag = true;
								++afkRecords[pc.battlePetNo + 1].deadthcount;
							}

							mem::write<short>(hProcess, hModule + sa::kOffsetBattlePetIndex, -1);
							mem::write<short>(hProcess, hModule + sa::kOffsetSelectPetArray + pc.battlePetNo * sizeof(short), 0);
							pets[pc.battlePetNo].hp = 0;
							pc.selectPetNo[pc.battlePetNo] = 0;
							pc.battlePetNo = -1;
							setCharacter(pc);
							pet_ = pets;
						}

						emit signalDispatcher.updateLabelPetAction("");
						emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
					}
					//戰寵存在
					else
					{
						emit signalDispatcher.updatePetHpProgressValue(obj.level, obj.hp, obj.maxHp);
						//重設戰寵索引
						sa::character_t pc = getCharacter();
						if (-1 == pc.battlePetNo)
						{
							pc.battlePetNo = matchPetByBattleInfo(obj, false);
							setCharacter(pc);
						}

						//戰寵存在但死亡
						if (obj.hp == 0 || util::checkAND(obj.status, sa::BC_FLG_DEAD))
						{
							if (pc.battlePetNo >= 0 && pc.battlePetNo < sa::MAX_PET)
							{
								if (!afkRecords[pc.battlePetNo + 1].deadthcountflag)
								{
									afkRecords[pc.battlePetNo + 1].deadthcountflag = true;
									++afkRecords[pc.battlePetNo + 1].deadthcount;
								}
							}
						}
						//戰寵存在且存活
						else
						{
							//索引在正確範圍內
							if (pc.battlePetNo >= 0 && pc.battlePetNo < sa::MAX_PET)
							{
								afkRecords[pc.battlePetNo + 1].deadthcountflag = false;
								pets[pc.battlePetNo].valid = true;
								pets[pc.battlePetNo].hp = obj.hp;
								pets[pc.battlePetNo].maxHp = obj.maxHp;
								pets[pc.battlePetNo].hpPercent = obj.hpPercent;
								pet_ = pets;
							}
						}
					}
				}
			}
		}

		setBattleData(bt);

		if (!isAllieAllDead && bt.allies.isEmpty())
			isAllieAllDead = true;
		if (!isEnemyAllDead && bt.enemies.isEmpty())
			isEnemyAllDead = true;

		if (isAllieAllDead || isEnemyAllDead)
		{
			return;
		}

		//切換標誌為可動作狀態
		battleCharAlreadyActed.off();
		battlePetAlreadyActed.off();
		break;
	}
	case 'U':
	{
		/*
		BU|的時候，離開戰鬥動畫。
		無參數。此命令用於在客戶端處於異常延遲的情況下，服務器將客戶端從戰鬥中強制退出後發送。
		*/
		battleCharEscapeFlag.on();
		setBattleEnd();
		break;
	}
	default:
	{
		break;
#if 0
		QStringList list = command.split(util::rexOR);
		if (list.isEmpty())
			break;

		long long size = list.size();
		long long i = 0;
		long long index = -1;
		long long argsCount = 0;
		QStringList args;

		//取參數
		auto getList = [&list, size](long long start)->QStringList
			{
				QStringList tmpList;
				QString tmp;
				long long i = start + 1;
				for (; i < size; ++i)
				{
					tmp = list.value(i);
					if (tmp.isEmpty())
						continue;

					if (tmp == "FF")
						break;
					else if (tmp.startsWith("B"))
						break;

					tmpList.append(tmp);
				}

				return tmpList;
			};

		index = list.indexOf("BJ");
		if (index != -1)
		{
			args = getList(index);
			argsCount = args.size();
			qDebug() << "BJ" << argsCount << args;
			if (argsCount > 20)//BJ|a%X|i%X|m%X|%X|%X|%X|s%X|t%X|l%X|%X|%X|%X|%X|%X|%X|o%X|o%X|o%X|s%X|%X|%X|r%X...|FF|
			{
				for (i = 0; i < argsCount; ++i)
				{
					QString valueStr = args.value(i);
					if (valueStr.isEmpty() || valueStr == "FF")
						break;

					qDebug() << valueStr.mid(1).toLongLong(nullptr, 16);
				}
			}
			else if (argsCount >= 5)//BJ|a%X|m%X|e%X|e%X|r%X...|FF|
			{
				QStringList arglabels = { "source", "mp", "effect", "effect", "target" };
				for (i = 0; i < argsCount; ++i)
				{
					QString valueStr = args.value(i);
					if (valueStr.isEmpty() || valueStr == "FF")
						break;

					qDebug() << QString(arglabels.value(i, arglabels.last())) << valueStr.mid(1).toLongLong(nullptr, 16);
				}
			}
			else if (argsCount < 5) //BJ|a%X|m%X|e%X|e%X|FF| 無目標
			{
				for (i = 0; i < argsCount; ++i)
				{
					QString valueStr = args.value(i);
					if (valueStr.isEmpty() || valueStr == "FF")
						break;

					qDebug() << valueStr.mid(1).toLongLong(nullptr, 16);
				}
			}
		}

		index = list.indexOf("BH");
		if (index != -1)
		{
			args = getList(index);
			argsCount = args.size();
			qDebug() << "BH" << argsCount << args;
			if (argsCount == 6)//BH|a%X|r%X|f%X|d%X|p%X|g%X|FF| 有技能
			{
				QStringList arglabels = { "attackNo", "defendNo", "flg", "damage", "petdamage", "skill" };
			}
			else if (argsCount == 5)//BH|a%X|r%X|f%X|d%X|p%X|FF|
			{
				//flg: 鏡:1024 光:2048 守:4096
				QStringList arglabels = { "attackNo", "defendNo", "flg", "damage", "petdamage" };
				for (i = 0; i < argsCount; ++i)
				{
					QString valueStr = args.value(i);
					if (valueStr.isEmpty() || valueStr == "FF")
						break;

					qDebug() << QString(arglabels.value(i, arglabels.last())) << valueStr.mid(1).toLongLong(nullptr, 16);
				}
			}
			else if (argsCount >= 4 && args.contains("f0") && args.contains("d0"))//BH|a%X|[r%X|f0|d0]...|FF| 標誌 0 無傷害 
			{
				QStringList arglabels = { "attackNo", "defendNo" };
			}
			else if (argsCount == 4 && args.contains("0"))//BH|a%X|r%X|0|d%X|FF| 無標誌
			{
				QStringList arglabels = { "attackNo", "defendNo", "", "damage" };
			}
			else if (argsCount == 4)//BH|a%X|r%X|f%X|d%X|FF| 無寵
			{
				QStringList arglabels = { "attackNo", "defendNo", "flg", "damage" };
			}
		}
		break;
		long long size = list.size();
		for (i = 0; i < size;)
		{
			temp.clear();
			temp = list.value(i).toUpper().mid(1);
			if (temp.isEmpty())
				break;
			QChar c = temp.at(0);
			ushort f = c.unicode();

			switch (f)
			{
			case 'B':
			{
				/*
				BB|的時候，遠程攻擊動畫。
				BB|攻擊方編號|投射物類型|防禦方編號|標志|傷害|防禦方編號|???|FF|
				除了第二個參數表示投射物類型
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'D':
			{
				/*
				BD|的時候，HP和MP更改動畫。
				BD|將更改的角色編號|更改類型|增加還是減少|增量|
				更改類型為0表示HP，1表示MP。增加還是減少為1表示增加，0表示減少。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'E':
			{
				/*
				BE|的時候，逃跑動畫。
				BE|逃跑者編號|防禦方編號|標志|
				如果標志為1，逃跑成功。如果標志為0，逃跑失敗。在失敗的情況下，客戶端將以隨機方式確定敵人被拖拽到何處。如果玩家角色逃跑，則沒有BE用於寵物，寵物也會逃跑。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'F':
			{
				/*
				BF|的時候，寵物隱藏動畫。
				BF|隱藏的角色編號|
				只有寵物可以使用此命令。客戶端將播放寵物跑到背後的效果，然後寵物消失。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'G':
			{
				/*
				bg|的時候，防禦動畫。
				bg|防禦的角色編號|
				這個命令以小寫字母發送。角色沒有動作，客戶端不需要處理此命令，將其視為注釋處理。
				*/

				//臨時用
				long long pos = list.value(i).toLongLong();
				i += 2;
				break;
			}
			case 'H':
			{
				/*
				BH|的時候，普通攻擊動畫。
				BH|攻擊方編號|防禦方編號|標志|傷害|防禦方編號|標志|傷害|????重覆。如果防禦方編號為FF，則視為此命令結束。

				(普通攻擊示例) B|BH|攻擊_0|防禦_A|標志_2|傷害_32|防禦_B|標志_2|傷害_32|FF|
				這表示0號(攻擊_0)對A號(防禦_A)造成0x32傷害(傷害_32)，然後對B號(防禦_B)也進行了攻擊，造成0x32傷害(傷害_32)。

				(反擊示例) B|BH|攻擊_0|防禦_A|標志_2|傷害_32|反擊_0|標志_10|傷害_16|FF|
				這表示0號(攻擊_0)對A號(防禦_A)造成0x32傷害(傷害_32)，然後0號(反擊_0)在反擊中受到0x16傷害(傷害_16)。在這種情況下，誰發動了反擊將由直前攻擊的對象確定。

				(有守衛時的示例) B|BH|攻擊_0|防禦_A|標志_202|傷害_32|守衛_B|FF|
				這表示0號(攻擊_0)對A號(防禦_A)造成了0x32傷害(傷害_32)，但由於B號成為了守衛者，所以傷害實際上被轉向了守衛者。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'J':
			{
				/*
				gmsv/battle/battle_magic.c

				BJ|的時候，咒術和物品效果動畫。
				BJ|使用咒術的角色編號|使用咒術的效果編號|受到咒術影響的效果編號|受到效果的角色編號|受到效果的角色編號|???|FF|
				用於物品和咒術的使用。受到影響的角色編號可以連續寫入，但最後一個必須以FF結束。

				snprintf( buf1, sizeof(buf1),
					"BJ|a%X|m%X|e%X|e%X|FF|",
					ToList[i],
					CHAR_getInt( toindex, CHAR_MP ),
					RecevEffect, //MyEffect,
					0  //ToEffect
				);

				BJ|a%X|m%X|e%X|e%X|...r%X|FF|
				snprintf( szCommand, sizeof(szCommand),
					"BJ|a%X|m%X|e%X|e%X|",
					attackNo,
					CHAR_getInt( attackindex, CHAR_MP ),
					MyEffect,
					ToEffect
				);
				BATTLESTR_ADD( szCommand );
				for( i = 0; ToList[i] != -1; i ++ ){
					snprintf( szCommand, sizeof(szCommand), "r%X|",ToList[i]);
					BATTLESTR_ADD( szCommand );
				}
				BATTLESTR_ADD( "FF|" );

				BJ|a%X|i%X|m%X|%X|%X|%X|s%X|t%X|l%X|%X|%X|%X|%X|%X|%X|o%X|o%X|o%X|s%X|%X|%X|...r%X|FF|
			   snprintf(
			   szcommand , sizeof( szcommand ) , "BJ|a%X|i%X|m%X|%X|%X|%X|s%X|t%X|l%X|%X|%X|%X|%X|%X|%X|o%X|o%X|o%X|s%X|%X|%X|" ,
			   attackNo , 12345678 , CHAR_getInt( attackindex , CHAR_MP ) ,
			   ATTMAGIC_magic[i].uiPrevMagicNum ,
			   ATTMAGIC_magic[i].uiSpriteNum ,
			   ATTMAGIC_magic[i].uiPostMagicNum ,
			   ATTMAGIC_magic[i].uiAttackType ,
			   ATTMAGIC_magic[i].uiSliceTime ,
			   ATTMAGIC_magic[i].uiShowType ,
			   ATTMAGIC_magic[i].siSx ,
			   ATTMAGIC_magic[i].siSy ,
			   ATTMAGIC_magic[i].siPrevMagicSx ,
			   ATTMAGIC_magic[i].siPrevMagicSy ,
			   ATTMAGIC_magic[i].siPostMagicSx ,
			   ATTMAGIC_magic[i].siPostMagicSy ,
			   ATTMAGIC_magic[i].siPrevMagicOnChar ,
			   ATTMAGIC_magic[i].uiShowBehindChar ,
			   ATTMAGIC_magic[i].siPostMagicOnChar ,
			   ATTMAGIC_magic[i].uiShakeScreen ,
			   ATTMAGIC_magic[i].uiShakeFrom ,
			   ATTMAGIC_magic[i].uiShakeTo
			   );
			   */



			   //使用咒術的角色編號
				temp = list.value(++i);
				long long pos = temp.mid(1).toLongLong(nullptr, 16);
				//使用咒術的效果編號
				temp = list.value(++i);
				long long magicEffectId = temp.mid(1).toLongLong(nullptr, 16);
				//受到咒術影響的效果編號
				temp = list.value(++i);
				long long targetEffectId = temp.mid(1).toLongLong(nullptr, 16);
				//受到咒術影響的效果編號
				QVector<long long> targets;
				for (;;)
				{
					temp = list.value(++i);
					if (temp == "FF")
						break;

					long long target = temp.mid(1).toLongLong();
					targets.append(target);
				}
				break;
			}
			case 'M':
			{
				/*
				BM|的時候，狀態異常變化動畫。
				BM|狀態變化的角色編號|哪種狀態異常|
				狀態異常編號為：
				---0：無狀態異常
				---1：中毒
				---2：麻痹
				---3：睡眠
				---4：石化
				---5：醉酒
				---6：混亂
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'O':
			{
				/*
				BO|的時候，回旋鏢動畫。
				BO|攻擊方編號|防禦方編號|標志|傷害|防禦方編號|???|FF|
				除了第二個參數表示投射物類型外，其他內容與普通攻擊相同
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'S':
			{
				/*
				BS|的時候，寵物出入動畫。
				BS|寵物編號[0～19](%X)|標志|圖像編號|級別|HP|名稱|
				標志0表示寵物返回時，此後的文本將被忽略。
				標志1表示寵物出現時，如果想交換寵物，則返回並再次出現寵物，這樣可以通過發送2次BS命令來交換寵物。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'T':
			{
				/*
				BT|的時候，敵人捕獲動畫。
				BT|攻擊方編號|防禦方編號|標志|
				如果標志為1，捕獲成功。如果標志為0，捕獲失敗。在失敗的情況下，敵人捕獲的程度由客戶端以隨機方式確定。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'V':
			{
				/*
				BV|的時候，領域屬性更改動畫。
				BV|更改屬性的角色編號|更改的屬性編號|
				更改的屬性編號為：
				---0：無屬性
				---1：地屬性
				---2：水屬性
				---3：火屬性
				---4：風屬性
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			case 'Y':
			{
				/*
				BY|的時候，合體攻擊動畫。
				BY|防禦方編號|攻擊方編號|標志|傷害|攻擊方編號|標志|傷害|????重覆。與普通攻擊不同的是，攻守角色發生了交換。
				*/

				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			default:
			{
				while (!list.value(i).isEmpty() && !list.value(i).toUpper().startsWith("B") && list.value(i) != "FF")
					++i;
				++i;
				break;
			}
			}
		}
#endif
		qDebug() << "lssproto_B_recv: unknown command" << command;
		break;
	}
	}
}

//寵物取消戰鬥狀態 (不是每個私服都有)
void Worker::lssproto_PETST_recv(long long petarray, long long result)
{
	std::ignore = petarray;
	std::ignore = result;

	updateDatasFromMemory();
	updateComboBoxList();
}

//戰寵狀態改變
void Worker::lssproto_KS_recv(long long petarray, long long result)
{
	std::ignore = petarray;
	std::ignore = result;

	updateDatasFromMemory();
	updateComboBoxList();
}

//寵物等待狀態改變 (不是每個私服都有)
void Worker::lssproto_SPET_recv(long long standbypet, long long result)
{
	std::ignore = standbypet;
	std::ignore = result;

	updateDatasFromMemory();
	updateComboBoxList();
}

//可用點數改變
void Worker::lssproto_SKUP_recv(long long point)
{
	sa::character_t pc = getCharacter();
	pc.point = point;
	setCharacter(pc);
	IS_WAITFOT_SKUP_RECV_.off();
}

//收到郵件
void Worker::lssproto_MSG_recv(long long aindex, char* ctext, long long color)
{
	QString text = util::toUnicode(ctext);
	if (text.isEmpty())
		return;
	//char moji[256];
	long long noReadFlag;

	if (aindex < 0 || aindex >= sa::MAIL_MAX_HISTORY)
		return;

	sa::mail_history_t mailHistory = getMailHistory(aindex);

	mailHistory.newHistoryNo--;

	if (mailHistory.newHistoryNo <= -1)
		mailHistory.newHistoryNo = sa::MAIL_MAX_HISTORY - 1;

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
		list.append(util::toQString(noReadFlag));

		mailHistory.petLevel[mailHistory.newHistoryNo] = getIntegerToken(text, "|", 4);
		list.append(util::toQString(mailHistory.petLevel[mailHistory.newHistoryNo]));

		getStringToken(text, "|", 5, mailHistory.petName[mailHistory.newHistoryNo]);

		temp = mailHistory.petName[mailHistory.newHistoryNo];
		makeStringFromEscaped(temp);

		mailHistory.petName[mailHistory.newHistoryNo] = temp;
		list.append(temp);

		mailHistory.itemGraNo[mailHistory.newHistoryNo] = getIntegerToken(text, "|", 6);
		list.append(util::toQString(mailHistory.itemGraNo[mailHistory.newHistoryNo]));

		//sprintf_s(moji, "收到%s送來的寵物郵件！", addressBook.name);
		announce(list.join("|"), color);
	}
	else
	{
		mailHistory.noReadFlag[mailHistory.newHistoryNo] = TRUE;
		list.append(util::toQString(mailHistory.noReadFlag[mailHistory.newHistoryNo]));

		announce(list.join("|"), color);

		QString msg = mailHistory.str[mailHistory.newHistoryNo];
		makeStringFromEscaped(msg);

		long long currentIndex = getIndex();
		GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
		QString whiteList = gamedevice.getStringHash(util::kMailWhiteListString);

		//sprintf_s(moji, "收到%s送來的郵件！", addressBook[aindex].name);
	}

	mailHistory_.insert(aindex, mailHistory);
}

//收到寵郵
void Worker::lssproto_PS_recv(long long result, long long havepetindex, long long havepetskill, long long toindex)
{
	std::ignore = result;
	std::ignore = havepetindex;
	std::ignore = havepetskill;
	std::ignore = toindex;
}

//戰後坐標更新
void Worker::lssproto_XYD_recv(const QPoint& pos, long long dir)
{
	sa::character_t pc = getCharacter();
	pc.dir = dir;
	setCharacter(pc);
	setPoint(pos);
	setBattleEnd();
	std::ignore = getFloor();
	std::ignore = getFloorName();
	std::ignore = getDir();
}

//服務端發來的ECHO 一般是30秒
void Worker::lssproto_Echo_recv(char* test)
{
	if (isEOTTLSend.get())
	{
		long long time = eoTTLTimer_.cost();
		lastEOTime.set(time);
		isEOTTLSend.off();
		announce(QObject::tr("server response time:%1ms").arg(time));//伺服器響應時間:xxxms
	}

	if (!getOnlineFlag())
	{
		setOnlineFlag(true);
	}
}

//家族頻道
void Worker::lssproto_FM_recv(char* cdata)
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

			sa::character_t pc = getCharacter();
			pc.channel = FMType3.toLongLong();
			if (pc.channel != -1)
				pc.quickChannel = pc.channel;
			setCharacter(pc);
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

//任務日誌
void Worker::lssproto_missionInfo_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//解讀資料
	long long i = 1, j = 1;
	QString getdata;
	QString perdata;

	missionInfo_.clear();

	while (getStringToken(data, "#", i, getdata) != 1)
	{
		sa::mission_data_t jobdaily = {};
		while (getStringToken(getdata, "|", j, perdata) != 1)
		{
			if (i - 1 >= sa::MAX_MISSION)
				continue;

			switch (j)
			{
			case 1:
			{
				jobdaily.id = perdata.toLongLong();
				break;
			}
			case 2:
			{
				jobdaily.name = perdata.simplified();
				break;
			}
			case 3:
			{
				QString stateStr = perdata.simplified();
				if (stateStr.contains("行"))
					jobdaily.state = 1;
				else if (stateStr.contains("完"))
					jobdaily.state = 2;
				else
					jobdaily.state = 0;
				break;
			}
			default: /*StockChatBufferLine("每筆資料內參數有錯誤", FONT_PAL_RED);*/
				break;
			}

			perdata.clear();
			++j;
		}

		missionInfo_.insert(i - 1, jobdaily);
		getdata.clear();
		j = 1;
		++i;
	}

	//是否有接收到資料
	if (i <= 1)
		missionInfo_.clear();

	if (IS_WAITFOR_missionInfo_FLAG.get())
		IS_WAITFOR_missionInfo_FLAG.off();
}

//導師系統
void Worker::lssproto_TEACHER_SYSTEM_recv(char* cdata)
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

//其他擴增資訊(少數服才有)
void Worker::lssproto_S2_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString szMessage;

	long long ftype = 0, newfame = 0;

	sa::character_t pc = getCharacter();

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

	setCharacter(pc);
}

//收到玩家對話或公告
void Worker::lssproto_TK_recv(long long index, char* cmessage, long long color)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	QString id;
	QString msg;

	QString message = util::toUnicode(cmessage);
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
				long long nGold = strGold.toLongLong();
				if (nGold > 0)
				{
					afkRecords[0].goldearn += nGold;
				}
			}
		}

		if (message.contains(rexRep))
		{
			QRegularExpressionMatch match = rexRep.match(message);
			if (match.hasMatch())
			{
				QString strRep = match.captured(1);
				long long nRep = strRep.toLongLong();
				if (nRep > 0)
				{
					sa::currency_data_t currency = currencyData.get();
					currency.prestige = nRep;
					currencyData.set(currency);
				}
			}
		}

		if (message.contains(rexVit))
		{
			QRegularExpressionMatch match = rexVit.match(message);
			if (match.hasMatch())
			{
				QString strVit = match.captured(1);
				long long nVit = strVit.toLongLong();
				if (nVit > 0)
				{
					sa::currency_data_t currency = currencyData.get();
					currency.vitality = nVit;
					currencyData.set(currency);
				}
			}
		}

		if (message.contains(rexVip))
		{
			QRegularExpressionMatch match = rexVip.match(message);
			if (match.hasMatch())
			{
				QString strVip = match.captured(1);
				long long nVip = strVip.toLongLong();
				if (nVip > 0)
				{
					sa::currency_data_t currency = currencyData.get();
					currency.VIPPoints = nVip;
					currencyData.set(currency);
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
				long long nGold = strGold.toLongLong();
				if (nGold > 0)
				{
					afkRecords[0].goldearn += nGold;
				}
			}
		}

		//getStringToken(message, "|", 2, msg);
		//makeStringFromEscaped(msg);
#ifdef _TRADETALKWND				//交易新增對話框架
		TradeTalk(msg);
#endif

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
					long long found = msg.indexOf(temp);

					if (found != -1)
					{
						tellName = msg.left(found).simplified();
						color = 5;
						szMsgBuf = msg.mid(found + temp.length()).simplified();
						msg.clear();
						msg = QString("[%1]%2").arg(tellName).arg(szMsgBuf);
						lastSecretChatName_.set(tellName);
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

		//TradeTalk(msg);
		if (msg == "成立聊天室扣除２００石幣")
		{
		}

#if 0 //密語頻道
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
#endif

		chatQueue.enqueue(qMakePair(color, msg.simplified()));

		emit signalDispatcher.appendChatLog(msg, color);
	}
	else
	{
		qDebug() << "lssproto_TK_recv: unknown command" << message;
	}
}

//地圖數據更新，重新繪製地圖
void Worker::lssproto_MC_recv(long long fl, long long x1, long long y1, long long x2, long long y2, long long tileSum, long long partsSum, long long eventSum, char* cdata)
{
	std::ignore = fl;
	std::ignore = x1;
	std::ignore = y1;
	std::ignore = x2;
	std::ignore = y2;
	std::ignore = tileSum;
	std::ignore = partsSum;
	std::ignore = eventSum;
	std::ignore = cdata;

	//QString data = util::toUnicode(cdata);
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
void Worker::lssproto_M_recv(long long fl, long long x1, long long y1, long long x2, long long y2, char* cdata)
{
	std::ignore = fl;
	std::ignore = x1;
	std::ignore = y1;
	std::ignore = x2;
	std::ignore = y2;
	std::ignore = cdata;

	QMutexLocker locker(&moveLock_);
	//QString data = util::toUnicode(cdata);
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
void Worker::lssproto_C_recv(char* cdata)
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

	setOnlineFlag(true);
	if (waitfor_C_recv_.get())
	{
		waitfor_C_recv_.off();
	}

	long long i = 0, j = 0, id = 0, x = 0, y = 0, dir = 0;
	long long modelid = 0, level = 0, nameColor = 0, walkable = 0, height = 0, classNo = 0, money = 0, charType = 0, charNameColor = 0;
	QString bigtoken, smalltoken, name, freeName, info, fmname, petname;

	QString titlestr;
	long long titleindex = 0;
	long long petlevel = 0;
	// 人物職業
	long long profession_class = 0, profession_level = 0, profession_skill_point = 0, profession_exp = 0;
	// 排行榜NPC
	long long herofloor = 0;
	long long picture = 0;
	QString gm_name;

	long long pcid = getCharacter().id;

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
			dir = smalltoken.toLongLong();
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
				//QWriteLocker locker(&charInfoLock_);
				sa::character_t pc = getCharacter();
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

				pc.modelid = modelid;

				pc.nameColor = nameColor;
				if (walkable != 0)
				{
					//pc.status |= kCharacterStatus_W;
				}
				if (height != 0)
				{
					//pc.status |= kCharacterStatus_H;
				}


				// 人物職業
				pc.profession_class = profession_class;
				pc.profession_level = profession_level;
				pc.profession_skill_point = profession_skill_point;
				pc.profession_exp = profession_exp;

				// 排行榜NPC
				pc.herofloor = herofloor;

				pc.nameColor = charNameColor;

				setCharacter(pc);
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

			sa::map_unit_t unit = mapUnitHash.value(id);
			unit.type = static_cast<sa::CHAR_TYPE>(charType);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.dir = dir;
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

			if (unit.type == sa::CHAR_TYPEPLAYER)
				unit.objType = sa::kObjectHuman;
			else if (unit.type == sa::CHAR_TYPEPET || unit.type == sa::CHAR_TYPEBUS)
				unit.objType = sa::kObjectPet;
			else
				unit.objType = sa::kObjectNPC;
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

			sa::map_unit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.modelid = modelid;
			unit.classNo = classNo;
			unit.item_name = info;
			unit.isVisible = modelid > 0 && modelid != 9999;
			unit.objType = sa::kObjectItem;
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
			sa::map_unit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.gold = money;
			unit.isVisible = true;
			unit.objType = sa::kObjectGold;
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
			dir = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 5, smalltoken);
			modelid = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 6, smalltoken);
			x = smalltoken.toLongLong();
			getStringToken(bigtoken, "|", 7, smalltoken);
			y = smalltoken.toLongLong();

			sa::map_unit_t unit = mapUnitHash.value(id);
			unit.id = id;
			unit.name = name;
			unit.x = x;
			unit.y = y;
			unit.p = QPoint(x, y);
			unit.dir = dir;
			unit.modelid = modelid;
			unit.isVisible = modelid > 0 && modelid != 9999;
			unit.objType = sa::kObjectHuman;
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
			dir = smalltoken.toLongLong();
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
				if (util::checkAND(pc.status, kCharacterStatus_IsLeader) != 0
					&& team[0].valid)
				{
					team[0].level = pc.level;
					team[0].name = pc.name;
				}
				for (j = 0; j < MAX_TEAM; ++j)
				{
					if (team[j].valid && team[j].id == id)
					{
						pc.status |= kCharacterStatus_HasTeam;
						if (j == 0)
						{
							pc.status &= (~kCharacterStatus_IsLeader);
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
void Worker::lssproto_CA_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString bigtoken;
	QString smalltoken;
	long long i = 0;
	long long charindex = 0;
	long long x = 0;
	long long y = 0;
	long long act = 0;
	long long dir = 0;

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
		dir = smalltoken.toLongLong();
		getStringToken(bigtoken, "|", 6, smalltoken);

		sa::map_unit_t unit = mapUnitHash.value(charindex);
		unit.id = charindex;
		unit.x = x;
		unit.y = y;
		unit.p = QPoint(x, y);
		unit.status = static_cast<sa::CharacterStatusFlag> (act);
		unit.dir = dir;
		mapUnitHash.insert(charindex, unit);
	}
}

//刪除指定一個或多個周圍人、NPC單位
void Worker::lssproto_CD_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	long long i;
	long long id;

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
void Worker::lssproto_S_recv(char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	QString first = data.left(1);
	data = data.mid(1);
	if (first.isEmpty())
		return;

	setOnlineFlag(true);

#pragma region Warp
	if (first == "C")//人物轉移
	{
		std::ignore = getFloor();
		std::ignore = getFloorName();
		std::ignore = getDir();
		std::ignore = getPoint();

		mapUnitHash.clear();
		long long fl, maxx, maxy, gx, gy;

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
		sa::character_t pc = getCharacter();
		pc.id = getIntegerToken(data, "|", 1);
		setCharacter(pc);

		serverTime_.set(getIntegerToken(data, "|", 2));
		firstServerTime_.set(QDateTime::currentMSecsSinceEpoch());
		realTimeToSATime(&saTimeStruct_);
		saCurrentGameTime.set(getLSTime(&saTimeStruct_));
	}
#pragma endregion
#pragma region RideInfo
	else if (first == "X")// X 騎寵
	{
		//QWriteLocker locker(&charInfoLock_);
		sa::character_t pc = getCharacter();
		pc.lowsride = getIntegerToken(data, "|", 2);
		setCharacter(pc);
	}
#pragma endregion
#pragma region CharInfo
	else if (first == "P")// P 人物狀態
	{
		sa::character_t pc = getCharacter();
		{
			//QWriteLocker locker(&charInfoLock_);

			QString name, freeName;
			long long i, kubun;
			unsigned long long mask;

			QString smalltoken;
			getStringToken(data, "|", 1, smalltoken);

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
				//character_.ridePetNo = getIntegerToken(data, "|", 26);	// 0x02000000

				pc.learnride = getIntegerToken(data, "|", 27);	// 0x04000000
				pc.baseGraNo = getIntegerToken(data, "|", 28);	// 0x08000000
				pc.lowsride = getIntegerToken(data, "|", 29);		// 0x08000000

				//character_.sfumato = 0xff0000;

				getStringToken(data, "|", 30, name);
				makeStringFromEscaped(name);

				pc.name = name;
				getStringToken(data, "|", 31, freeName);
				makeStringFromEscaped(freeName);

				pc.freeName = freeName;

				//character_.道具欄狀態 = getIntegerToken(data, "|", 32);

				long long pointindex = getIntegerToken(data, "|", 33);
				QStringList pontname = {
					"萨姆吉尔村",
					"玛丽娜丝村",
					"加加村",
					"卡鲁它那村",
				};
				pc.chusheng = pontname.value(pointindex);

				//character_.法寶道具狀態 = getIntegerToken(data, "|", 34);
				//character_.道具光環效果 = getIntegerToken(data, "|", 35);

			}
			else
			{
				mask = 2;
				i = 2;
				for (; mask > 0; mask <<= 1)
				{
					if (!util::checkAND(kubun, mask))
						continue;
					switch (mask)
					{
					case 0x00000002: // ( 1 << 1 )
					{
						pc.hp = getIntegerToken(data, "|", i);// 0x00000002
						//custom
						pc.hpPercent = util::percent(pc.hp, pc.maxHp);
						++i;
						break;
					}
					case 0x00000004:// ( 1 << 2 )
					{
						pc.maxHp = getIntegerToken(data, "|", i);// 0x00000004
						//custom
						pc.hpPercent = util::percent(pc.hp, pc.maxHp);
						++i;
						break;
					}
					case 0x00000008:
					{
						pc.mp = getIntegerToken(data, "|", i);// 0x00000008
						//custom
						pc.mpPercent = util::percent(pc.mp, pc.maxMp);
						++i;
						break;
					}
					case 0x00000010:
					{
						pc.maxMp = getIntegerToken(data, "|", i);// 0x00000010
						//custom
						pc.mpPercent = util::percent(pc.mp, pc.maxMp);
						++i;
						break;
					}
					case 0x00000020:
					{
						pc.vit = getIntegerToken(data, "|", i);// 0x00000020
						++i;
						break;
					}
					case 0x00000040:
					{
						pc.str = getIntegerToken(data, "|", i);// 0x00000040
						++i;
						break;
					}
					case 0x00000080:
					{
						pc.tgh = getIntegerToken(data, "|", i);// 0x00000080
						++i;
						break;
					}
					case 0x00000100:
					{
						pc.dex = getIntegerToken(data, "|", i);// 0x00000100
						++i;
						break;
					}
					case 0x00000200:
					{
						pc.exp = getIntegerToken(data, "|", i);// 0x00000200
						++i;
						break;
					}
					case 0x00000400:
					{
						pc.maxExp = getIntegerToken(data, "|", i);// 0x00000400
						++i;
						break;
					}
					case 0x00000800:
					{
						pc.level = getIntegerToken(data, "|", i);// 0x00000800
						++i;
						break;
					}
					case 0x00001000:
					{
						pc.atk = getIntegerToken(data, "|", i);// 0x00001000
						++i;
						break;
					}
					case 0x00002000:
					{
						pc.def = getIntegerToken(data, "|", i);// 0x00002000
						++i;
						break;
					}
					case 0x00004000:
					{
						pc.agi = getIntegerToken(data, "|", i);// 0x00004000
						++i;
						break;
					}
					case 0x00008000:
					{
						pc.chasma = getIntegerToken(data, "|", i);// 0x00008000
						++i;
						break;
					}
					case 0x00010000:
					{
						pc.luck = getIntegerToken(data, "|", i);// 0x00010000
						++i;
						break;
					}
					case 0x00020000:
					{
						pc.earth = getIntegerToken(data, "|", i);// 0x00020000
						++i;
						break;
					}
					case 0x00040000:
					{
						pc.water = getIntegerToken(data, "|", i);// 0x00040000
						++i;
						break;
					}
					case 0x00080000:
					{
						pc.fire = getIntegerToken(data, "|", i);// 0x00080000
						++i;
						break;
					}
					case 0x00100000:
					{
						pc.wind = getIntegerToken(data, "|", i);// 0x00100000
						++i;
						break;
					}
					case 0x00200000:
					{
						pc.gold = getIntegerToken(data, "|", i);// 0x00200000
						++i;
						break;
					}
					case 0x00400000:
					{
						pc.titleNo = getIntegerToken(data, "|", i);// 0x00400000
						++i;
						break;
					}
					case 0x00800000:
					{
						pc.dp = getIntegerToken(data, "|", i);// 0x00800000
						++i;
						break;
					}
					case 0x01000000:
					{
						pc.transmigration = getIntegerToken(data, "|", i);// 0x01000000
						++i;
						break;
					}
					case 0x02000000:
					{
						getStringToken(data, "|", i, name);// 0x01000000
						makeStringFromEscaped(name);

						pc.name = name;
						++i;
						break;
					}
					case 0x04000000:
					{
						getStringToken(data, "|", i, freeName);// 0x02000000
						makeStringFromEscaped(freeName);

						pc.freeName = freeName;
						++i;
						break;
					}
					case 0x08000000: // ( 1 << 27 )
					{
						//character_.ridePetNo = getIntegerToken(data, "|", i);// 0x08000000
						++i;
						break;
					}
					case 0x10000000: // ( 1 << 28 )
					{
						pc.learnride = getIntegerToken(data, "|", i);// 0x10000000
						++i;
						break;
					}
					case 0x20000000: // ( 1 << 29 )
					{
						pc.baseGraNo = getIntegerToken(data, "|", i);// 0x20000000
						++i;
						break;
					}
					case 0x40000000: // ( 1 << 30 )
					{
						pc.skywalker = getIntegerToken(data, "|", i);// 0x40000000
						++i;
						break;
					}
					case 0x80000000: // ( 1 << 31 )
					{
						//character_.簽到標記 = getIntegerToken(data, "|", i);// 0x80000000
						++i;
						break;
					}
					default:
						break;
					}
				}
			}
			setCharacter(pc);

			QStringList teamInfoList;
			for (long long i = 0; i < sa::MAX_TEAM; ++i)
			{
				sa::team_t team = getTeam(i);
				if (team.valid && team.id == pc.id)
				{
					if (team.level != pc.level)
						team.level = pc.level;

					if (team.hp != pc.hp)
						team.hp = pc.hp;

					if (team.maxHp != pc.maxHp)
						team.maxHp = pc.maxHp;

					if (team.hpPercent != pc.hpPercent)
						team.hpPercent = pc.hpPercent;

					team_.insert(i, team);
				}

				QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(team.name).arg(team.level)
					.arg(team.hp).arg(team.maxHp).arg(team.hpPercent);
				teamInfoList.append(text);
			}

			emit signalDispatcher.updateTeamInfo(teamInfoList);

			emit signalDispatcher.updateMainFormTitle(pc.name);//標題設置為人物名稱
			emit signalDispatcher.updateCharHpProgressValue(pc.level, pc.hp, pc.maxHp);
			emit signalDispatcher.updateCharMpProgressValue(pc.level, pc.mp, pc.maxMp);

			QHash<long long, sa::pet_t> pets = pet_.toHash();
			if (pc.ridePetNo != -1)
				emit signalDispatcher.updateRideHpProgressValue(pets.value(pc.ridePetNo).level, pets.value(pc.ridePetNo).hp, pets.value(pc.ridePetNo).maxHp);
			if (pc.battlePetNo != -1)
				emit signalDispatcher.updatePetHpProgressValue(pets.value(pc.battlePetNo).level, pets.value(pc.battlePetNo).hp, pets.value(pc.battlePetNo).maxHp);

			emit signalDispatcher.updateCharInfoStone(pc.gold);
			qreal power = (((static_cast<double>(pc.atk + pc.def + pc.agi) + (static_cast<double>(pc.maxHp) / 4.0)) / static_cast<double>(pc.level)) * 100.0);
			qreal growth = (static_cast<double>(pc.atk + pc.def + pc.agi - 20) / static_cast<double>(pc.level - 1));
			const QVariantList varList = {
				pc.name, pc.freeName,
				QString(QObject::tr("%1(%2tr)").arg(pc.level).arg(pc.transmigration) + QObject::tr("L:%1").arg(pc.luck)),
				pc.exp, pc.maxExp,
				pc.exp > pc.maxExp ? 0 : pc.maxExp - pc.exp,
				QString("%1/%2").arg(pc.hp).arg(pc.maxHp) ,
				QString("%1/%2").arg(pc.mp).arg(pc.maxMp),
				pc.chasma, QString("%1,%2,%3").arg(pc.atk).arg(pc.def).arg(pc.agi),
				util::toQString(growth),
				util::toQString(power)
			};

			QVariant var = QVariant::fromValue(varList);

			playerInfoColContents.insert(0, var);
			emit signalDispatcher.updateCharInfoColContents(0, var);
		}

		getCharMaxCarryingCapacity();
	}
#pragma endregion
#pragma region FamilyInfo
	else if (first == "F") // F 家族狀態
	{
		//QWriteLocker locker(&charInfoLock_);
		QString family;
		getStringToken(data, "|", 1, family);
		makeStringFromEscaped(family);

		sa::character_t pc = getCharacter();
		pc.family = family;
		pc.familyleader = getIntegerToken(data, "|", 2);
		pc.channel = getIntegerToken(data, "|", 3);
		pc.familySprite = getIntegerToken(data, "|", 4);
		pc.big4fm = getIntegerToken(data, "|", 5);
		setCharacter(pc);
	}
#pragma endregion
#pragma region CharModify
	else if (first == "M") // M HP,MP,EXP
	{
		sa::character_t pc = getCharacter();
		pc.hp = getIntegerToken(data, "|", 1);
		pc.mp = getIntegerToken(data, "|", 2);
		pc.exp = getIntegerToken(data, "|", 3);

		//custom
		pc.hpPercent = util::percent(pc.hp, pc.maxHp);
		pc.mpPercent = util::percent(pc.mp, pc.maxMp);
		setCharacter(pc);

		emit signalDispatcher.updateCharHpProgressValue(pc.level, pc.hp, pc.maxHp);
		emit signalDispatcher.updateCharMpProgressValue(pc.level, pc.mp, pc.maxMp);
	}
#pragma endregion
#pragma region PetInfo
	else if (first == "K") // K 寵物狀態
	{
		QWriteLocker locker(&petInfoLock_);

		QString name, freeName;
		long long no, kubun, i;
		unsigned long long mask;

		no = data.left(1).toUInt();
		data = data.mid(2);
		if (data.isEmpty())
			return;

		if (no < 0 || no >= sa::MAX_PET)
			return;

		kubun = getInteger62Token(data, "|", 1);
		if (kubun == 0)
		{
			pet_.remove(no);
		}
		else
		{
			sa::pet_t pet = pet_.value(no);
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
					if (!util::checkAND(kubun, mask))
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

		sa::character_t pc = getCharacter();
		if (pc.ridePetNo >= 0 && pc.ridePetNo < sa::MAX_PET)
		{
			sa::pet_t pet = pet_.value(pc.ridePetNo);
			emit signalDispatcher.updateRideHpProgressValue(pet.level, pet.hp, pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 0);
		}

		if (pc.battlePetNo >= 0 && pc.battlePetNo < sa::MAX_PET)
		{
			sa::pet_t pet = pet_.value(pc.battlePetNo);
			emit signalDispatcher.updatePetHpProgressValue(pet.level, pet.hp, pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 0);
		}

		QStringList petNames;
		for (long long j = 0; j < sa::MAX_PET; ++j)
		{
			sa::pet_t pet = pet_.value(j);
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
					util::toQString(pet.growth),
					util::toQString(pet.power)
				};

				petNames.append(pet.name);
			}
			else
			{
				for (long long k = 0; k < 12; ++k)
					varList.append("");
			}

			QVariant var = QVariant::fromValue(varList);
			playerInfoColContents.insert(j + 1, var);
			emit signalDispatcher.updateCharInfoColContents(j + 1, var);
		}

		GameDevice& gamedevice = GameDevice::getInstance(getIndex());
		gamedevice.setUserData(util::kUserPetNames, petNames);
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
		//QWriteLocker locker(&charMagicInfoLock_);
		QString name, memo;
		long long no;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= sa::MAX_MAGIC)
			return;

		if (data.isEmpty())
			return;

		sa::magic_t magic = magic_.value(no);
		magic.valid = getIntegerToken(data, "|", 1) > 0;
		if (!magic.valid)
		{
			magic_.remove(no);
			return;
		}

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


		magic_.insert(no, magic);
	}
#pragma endregion
#pragma region TeamInfo
	else if (first == "N")  // N 隊伍資訊
	{
		//QWriteLocker locker(&teamInfoLock_);

		auto checkList = [this](util::UserSetting enableElement, const QString& cmpName)->long long
			{
				GameDevice& gamedevice = GameDevice::getInstance(getIndex());
				if (!gamedevice.getEnableHash(enableElement))
					return -1;

				QString preListStr;
				if (util::kGroupWhiteListEnable == enableElement)
				{
					preListStr = gamedevice.getStringHash(util::kGroupWhiteListString);
				}
				else
				{
					preListStr = gamedevice.getStringHash(util::kGroupBlackListString);
				}

				if (preListStr.isEmpty())
					return 0;

				QStringList preList = preListStr.split(util::rexOR, Qt::SkipEmptyParts);
				if (preList.isEmpty())
					return 0;

				QString newName;
				for (const QString& pre : preList)
				{
					newName = pre;
					if (newName.isEmpty())
						continue;

					if (newName.startsWith("?"))
					{
						newName = newName.mid(1);
						if (cmpName.contains(newName))
							return 1;
					}
					else if (newName == cmpName)
					{
						return 1;
					}
				}

				return 0;
			};

		auto updateTeamInfo = [this, &signalDispatcher, &checkList]()
			{
				QStringList teamInfoList;
				for (long long i = 0; i < sa::MAX_TEAM; ++i)
				{
					sa::team_t team = team_.value(i);
					long long whiteListState = checkList(util::kGroupWhiteListEnable, team.name);

					//打開白名單且不在白名單內或位於黑名單
					bool kickEnable = (whiteListState != -1 && whiteListState != 1) || (checkList(util::kGroupBlackListEnable, team.name) == 1);

					if ((!team.valid) || (team.maxHp <= 0) || kickEnable)
					{
						if (team_.contains(i))
							team_.remove(i);
						teamInfoList.append("");

						if (kickEnable && i > 0)//非隊長
							kickteam(i);
						continue;
					}

					QString text = QString("%1 LV:%2 HP:%3/%4 MP:%5").arg(team.name).arg(team.level)
						.arg(team.hp).arg(team.maxHp).arg(team.hpPercent);
					teamInfoList.append(text);
				}

				emit signalDispatcher.updateTeamInfo(teamInfoList);
			};

		QString name;
		long long no, kubun, i;
		unsigned long long mask;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= sa::MAX_TEAM)
			return;

		if (data.isEmpty())
			return;

		kubun = getInteger62Token(data, "|", 1);

		if (kubun == 0)
		{
			team_.remove(no);

			if (team_.size() == 1)//只剩自己
				team_.clear();
			updateTeamInfo();
			return;
		}

		sa::team_t team = team_.value(no);

		if (kubun == 1)
		{
			team.valid = true;
			team.id = getIntegerToken(data, "|", 2);	// 0x00000002
			team.level = getIntegerToken(data, "|", 3);	// 0x00000004
			team.maxHp = getIntegerToken(data, "|", 4);	// 0x00000008
			team.hp = getIntegerToken(data, "|", 5);	// 0x00000010
			team.mp = getIntegerToken(data, "|", 6);	// 0x00000020
			getStringToken(data, "|", 7, name);	// 0x00000040
			makeStringFromEscaped(name);
			team.name = name;

			team.hpPercent = util::percent(team.hp, team.maxHp);
			team_.insert(no, team);
		}
		else
		{
			mask = 2;
			i = 2;
			for (; mask > 0; mask <<= 1)
			{
				if (!util::checkAND(kubun, mask))
					continue;

				if (mask == 0x00000002)
				{
					team.id = getIntegerToken(data, "|", i);// 0x00000002
					++i;
				}
				else if (mask == 0x00000004)
				{
					team.level = getIntegerToken(data, "|", i);// 0x00000004
					++i;
				}
				else if (mask == 0x00000008)
				{
					team.maxHp = getIntegerToken(data, "|", i);// 0x00000008
					++i;
				}
				else if (mask == 0x00000010)
				{
					team.hp = getIntegerToken(data, "|", i);// 0x00000010
					++i;
				}
				else if (mask == 0x00000020)
				{
					team.mp = getIntegerToken(data, "|", i);// 0x00000020
					++i;
				}
				else if (mask == 0x00000040)
				{
					getStringToken(data, "|", i, name);// 0x00000040
					makeStringFromEscaped(name);

					team.name = name;
					++i;
				}
			}

			team.hpPercent = util::percent(team.hp, team.maxHp);
			team_.insert(no, team);
		}

		updateTeamInfo();
	}
#pragma endregion
#pragma region ItemInfo
	else if (first == "I") //I 道具
	{
		{
			QWriteLocker locker(&itemInfoLock_);

			long long i, no;
			QString temp;

			for (i = 0; i < sa::MAX_ITEM; ++i)
			{
				no = i * 14;

				getStringToken(data, "|", no + 1, temp);
				makeStringFromEscaped(temp);

				if (temp.isEmpty())
				{
					item_.remove(i);
					continue;
				}

				sa::item_t item = item_.value(i);

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

				if (item.name == "惡魔寶石" || item.name == "恶魔宝石")
				{
					static QRegularExpression rex("(\\d+)");
					QRegularExpressionMatch match = rex.match(item.memo);
					if (match.hasMatch())
					{
						QString str = match.captured(1);
						bool ok = false;
						long long dura = str.toLongLong(&ok);
						if (ok)
							item.count = dura;
					}
				}

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
				temp.remove("%");
				temp.remove("％");
				bool ok = false;
				item.damage = temp.toLongLong(&ok);
				if (!ok)
					item.damage = 100;

				getStringToken(data, "|", no + 11, temp);
				makeStringFromEscaped(temp);

				item.stack = temp.toLongLong();
				if (item.stack == 0)
					item.stack = 1;
				else if (item.stack == -1)
					item.stack = 0;

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
		QHash<long long, sa::item_t> items = getItems();
		for (const sa::item_t& it : items)
		{
			if (it.name.isEmpty())
				continue;
			itemList.append(it.name);
		}
		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxItem, itemList);

		QStringList magicNameList;
		for (long long i = 0; i < sa::MAX_MAGIC; ++i)
		{
			magicNameList.append(getMagic(i).name);
		}

		emit signalDispatcher.updateComboBoxItemText(util::kComboBoxCharAction, magicNameList);
		if (IS_WAITOFR_ITEM_CHANGE_PACKET.get() > 0)
			IS_WAITOFR_ITEM_CHANGE_PACKET.dec();

		checkAutoEatBoostExpItem();

		checkAutoDropItems();
	}
#pragma endregion
#pragma region PetSkill
	else if (first == "W")//接收到的寵物技能
	{
		//QWriteLocker locker(&petSkillInfoLock_);
		long long i, no, no2;
		QString temp;

		no = data.left(1).toUInt();
		data = data.mid(2);

		if (no < 0 || no >= sa::MAX_PET_SKILL)
			return;

		if (data.isEmpty())
			return;


		QHash<long long, sa::pet_skill_t> petSkills = petSkill_.value(no);
		for (i = 0; i < sa::MAX_PET_SKILL; ++i)
		{
			petSkills.remove(i);
		}

		QStringList skillNameList;
		for (i = 0; i < sa::MAX_PET_SKILL; ++i)
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

			sa::character_t pc = getCharacter();
			if ((pc.battlePetNo >= 0) && pc.battlePetNo < sa::MAX_PET)
				skillNameList.append(petSkills.value(i).name);
		}

		petSkill_.insert(no, petSkills);
	}
#pragma endregion
#pragma region CharSkill
	// 人物職業
	else if (first == "S") // S 職業技能
	{
		//QWriteLocker locker(&charSkillInfoLock_);

		QString name;
		QString memo;
		long long i, count = 0;

		QHash <long long, sa::profession_skill_t> profession_skill = professionSkill_.toHash();

		for (i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
		{
			profession_skill.remove(i);
		}
		for (i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
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

		professionSkill_ = profession_skill;
	}
	else if (first == "G")
	{
		//QWriteLocker locker(&charSkillInfoLock_);
		long long i, count = 0;
		QHash <long long, sa::profession_skill_t> profession_skill = professionSkill_.toHash();
		for (i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
			profession_skill[i].cooltime = 0;
		for (i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
		{
			count = i * 1;
			profession_skill[i].cooltime = getIntegerToken(data, "|", 1 + count);
		}

		professionSkill_ = profession_skill;
	}
#pragma endregion
#pragma region PetEquip
	else if (first == "B") // B 寵物道具
	{
		//QWriteLocker locker(&petEquipInfoLock_);

		long long i, no, nPetIndex;
		QString szData;

		nPetIndex = data.left(1).toUInt();
		data = data.mid(2);

		if (nPetIndex < 0 || nPetIndex >= sa::MAX_PET)
			return;

		if (data.isEmpty())
			return;

		QHash<long long, sa::item_t> petItems = petItem_.toHash().value(nPetIndex);
		for (i = 0; i < sa::MAX_PET_ITEM; ++i)
		{
			no = i * 14;

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
			szData.remove("%");
			szData.remove("％");
			bool ok = false;
			petItems[i].damage = szData.toLongLong(&ok);
			if (!ok)
				petItems[i].damage = 100;

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
void Worker::lssproto_ClientLogin_recv(char* cresult)
{
	QString result = util::toUnicode(cresult);
	if (result.isEmpty())
		return;

	if (result.contains(sa::OKSTR, Qt::CaseInsensitive))
	{
		//更新UI顯示
		long long currentIndex = getIndex();
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLoginSuccess);
	}
	else if (result.contains(sa::CANCLE, Qt::CaseInsensitive))
	{

	}
}

//新增人物
void Worker::lssproto_CreateNewChar_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive)
		|| data.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive))
	{

	}
	else
	{
		//創建人物內容提示
	}
}

//人物刪除
void Worker::lssproto_CharDelete_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (data.isEmpty() && result.isEmpty())
		return;

	if (result == sa::SUCCESSFULSTR || data == sa::SUCCESSFULSTR)
	{

	}
}

//更新人物列表
void Worker::lssproto_CharList_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains("failed", Qt::CaseInsensitive))
	{
		//_snprintf_s(LoginErrorMessage, sizeof(LoginErrorMessage), _TRUNCATE, "%s", data);
		//PcLanded.登陸延時時間 = TimeGetTime() + 2000;
		return;
	}

	QString nm, opt;
	long long i;

	//netproc_sending = NETPROC_RECEIVED;
	if (!result.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive) && !data.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive))
	{
		//if (result.contains("OUTOFSERVICE", Qt::CaseInsensitive))
		return;
	}

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusGettingCharList);

	charListData_.clear();

	QVector<sa::char_list_data_t> vec;
	for (i = 0; i < sa::MAX_CHARACTER; ++i)
	{
		sa::char_list_data_t table = {};
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
			long long index = args.value(0).toLongLong();
			if (index >= 0 && index < sa::MAX_CHARACTER)
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

	long long size = vec.size();
	for (i = 0; i < size; ++i)
	{
		if (i < 0 || i >= sa::MAX_CHARACTER)
			continue;

		long long index = vec.value(i).pos;
		charListData_.insert(index, vec.value(i));
	}
}

//人物登出(不是每個私服都有，有些是直接切斷後跳回賬號密碼頁)
void Worker::lssproto_CharLogout_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive) || data.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive))
	{
		setBattleFlag(false);
		setOnlineFlag(false);
	}
}

//人物登入
void Worker::lssproto_CharLogin_recv(char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;

	if (!result.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive) && !data.contains(sa::SUCCESSFULSTR, Qt::CaseInsensitive))
		return;

	setOnlineFlag(true);

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSignning);

	//重置登入計時
	loginTimer.restart();

	//重置戰鬥總局數
	battle_total_time.reset();

	//重置掛機數據
	sa::character_t pc = getCharacter();
	afkRecords[0].levelrecord = pc.level;
	afkRecords[0].exprecord = pc.exp;
	afkRecords[0].goldearn = 0;
	afkRecords[0].deadthcount = 0;

	for (long long i = 1; i <= sa::MAX_PET; ++i)
	{
		sa::pet_t pet = pet_.value(i - 1);
		afkRecords[i] = {};
		afkRecords[i].levelrecord = pet.level;
		afkRecords[i].exprecord = pet.exp;
		afkRecords[i].deadthcount = 0;
	}

	//重置對話框開啟標誌(客製)
	mem::write<int>(gamedevice.getProcess(), gamedevice.getProcessModule() + sa::kOffsetDialogValid, 0);

	//顯示NPC列表
	emit signalDispatcher.updateNpcList(getFloor());
}

//交易
void Worker::lssproto_TD_recv(char* cdata)//交易
{
	QString data = util::toUnicode(cdata);
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
		QString fd;
		QString name;
		QString command;
		getStringToken(data, "|", 2, fd);
		getStringToken(data, "|", 3, name);
		getStringToken(data, "|", 4, command);

		if (command.startsWith("0"))
		{
			sa::character_t pc = getCharacter();
			pc.trade_confirm = 0;
			setCharacter(pc);
			return;
		}
		else if (command.startsWith("1"))
		{
			clearTradeData();
			opp_sockfd = fd;
			opp_name = name;
			trade_command = command;

			sa::character_t pc = getCharacter();
			pc.trade_confirm = 1;
			setCharacter(pc);
			IS_TRADING.on();
		}
		else if (command.startsWith("2"))
		{
			opp_sockfd = fd;
			opp_name = name;
			trade_command = command;

			tradeStatus = 1;
			sa::character_t pc = getCharacter();
			pc.trade_confirm = 2;
			setCharacter(pc);
		}

		return;
	}
	//处理物品交易资讯传递
	else if (Head.startsWith("T"))
	{
		if (!IS_TRADING.get())
			return;

		QString buf_showindex;

		getStringToken(data, "|", 4, trade_kind);
		if (trade_kind.startsWith("S"))
		{
			QString buf1;
			long long objno = -1;//, showno = -1;

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
				sa::character_t pc = getCharacter();
				sa::pet_t pet = getPet(objno);
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
			long long mount = opp_goldmount.toLongLong();


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

			return;
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

			return;
		}

		if (trade_kind.startsWith("P"))
		{
			long long iItemNo = 0;
			QString	szData;
			long long index = -1;

			for (long long i = 0;; ++i)
			{
				if (getStringToken(data, "|", 26 + i * 6, szData))
					break;
				iItemNo = szData.toLongLong();
				if (index >= 0 && index < sa::MAX_PET && iItemNo >= 0 && iItemNo < sa::MAX_PET_ITEM)
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
			return;
		}

		if (trade_kind.startsWith("C"))
		{
			sa::character_t pc = getCharacter();
			if (pc.trade_confirm == 1)
				pc.trade_confirm = 3;
			if (pc.trade_confirm == 2)
				pc.trade_confirm = 4;
			if (pc.trade_confirm == 3)
			{
				//我方已點確認後，收到對方點確認
				pc.trade_confirm = 5;
			}

			setCharacter(pc);
			return;
		}

		// end
		if (trade_kind.startsWith("A"))
		{
			sa::character_t pc = getCharacter();
			pc.trade_confirm = 0;
			setCharacter(pc);
		}

		else if (Head.startsWith("W"))
		{
			//取消交易
			sa::character_t pc = getCharacter();
			pc.trade_confirm = 0;
			setCharacter(pc);
		}
	}

	clearTradeData();
	IS_TRADING.off();
}

void Worker::lssproto_CHAREFFECT_recv(char*)
{
}

//SASH自訂對話框收到按鈕消息
bool Worker::lssproto_CustomWN_recv(const QString& data)
{
	QStringList dataList = data.split(util::rexOR, Qt::SkipEmptyParts);
	if (dataList.size() != 4 && dataList.size() != 3)
		return false;

	long long x = dataList.value(0).toLongLong();
	long long y = dataList.value(1).toLongLong();
	sa::ButtonType button = static_cast<sa::ButtonType>(dataList.value(2).toLongLong());
	QString dataStr = "";

	if (dataList.size() == 4)
		dataStr = dataList.value(3);

	long long row = -1;
	bool ok = false;
	long long tmp = dataStr.toLongLong(&ok);
	if (ok && tmp > 0)
	{
		row = dataStr.toLongLong();
	}

	sa::custom_dialog_t _customDialog;
	_customDialog.x = x;
	_customDialog.y = y;
	_customDialog.button = button;
	_customDialog.row = row;
	_customDialog.rawbutton = dataList.value(2).toLongLong();
	_customDialog.rawdata = dataStr;
	customDialog.set(_customDialog);
	IS_WAITFOR_CUSTOM_DIALOG_FLAG.off();

	return true;
}

//SASH自訂對話
bool Worker::lssproto_CustomTK_recv(const QString& data)
{
	QStringList dataList = data.split(util::rexOR, Qt::SkipEmptyParts);
	if (dataList.size() != 5)
		return false;

	//long long x = dataList.value(0).toLongLong();
	//long long y = dataList.value(1).toLongLong();
	//long long color = dataList.value(2).toLongLong();
	//long long area = dataList.value(3).toLongLong();
	QString dataStr = dataList.value(4).simplified();
	QStringList args = dataStr.split(" ", Qt::SkipEmptyParts);
	if (args.isEmpty())
		return false;

	long long size = args.size();
	if (!args.value(0).startsWith("//skup") || size != 5)
		return false;

	bool ok;
	long long vit = args.value(1).toLongLong(&ok);
	if (!ok)
		return false;

	long long str = args.value(2).toLongLong(&ok);
	if (!ok)
		return false;

	long long tgh = args.value(3).toLongLong(&ok);
	if (!ok)
		return false;

	long long dex = args.value(4).toLongLong(&ok);
	if (!ok)
		return false;

	if (skupFuture_.isRunning())
		return false;

	skupFuture_ = QtConcurrent::run([this, vit, str, tgh, dex]()
		{
			const QVector<long long> vec = { vit, str, tgh, dex };
			long long j = 0;
			for (long long i = 0; i < 4; ++i)
			{
				j = vec.value(i);
				if (j <= 0)
					continue;

				if (!addPoint(i, j))
					return;
			}
		});

	return skupFuture_.isRunning();
}

void Worker::lssproto_SE_recv(const QPoint&, long long, long long)
{

}

void Worker::lssproto_WO_recv(long long)
{

}

void Worker::lssproto_NU_recv(long long)
{

}

void Worker::lssproto_CharNumGet_recv(long long, long long)
{

}

void Worker::lssproto_ProcGet_recv(char*)
{

}

//服務端發送給客戶端用於顯示某些東西的
void Worker::lssproto_D_recv(long long, long long, long long, char*)
{
	/*
servertoclient D 函数用于向客户端发送显示指令，以在游戏画面上显示特定的内容。该函数接受以下参数：

long long category：显示内容的类别或类型。这个参数通常用于区分不同种类的显示内容。

long long dx 和 long long dy：指定显示内容的位置坐标，即在游戏画面上的 x 和 y 坐标。这决定了内容将显示在何处。

string data：包含要显示的内容的数据字符串。这可以是文本、图像或其他任何游戏中需要显示的信息或元素。

通过使用 servertoclient D 函数，服务器可以向客户端发送各种类型的显示指令，以在游戏中呈现丰富的视觉和文本元素，如文本消息、物品图标、NPC 对话等。这有助于改善游戏的交互性和可玩性。
	*/
}

//服務端發來的用於固定客戶端的速度
void Worker::lssproto_CS_recv(long long)
{
}

//戰鬥結束?
void Worker::lssproto_NC_recv(long long)
{

}

void Worker::lssproto_R_recv(char*)
{
	/*
	将雷达的内容发送到客户端。客户端不需要请求此发送。服务器将在适当的时机发送。例如，每走10步或每1分钟一次。

	数据示例：此字符串不会被转义。"12|22|E|13|24|P|14|28|P"
	*/
}

//煙火
void Worker::lssproto_Firework_recv(long long, long long, long long)
{
}

void Worker::lssproto_DENGON_recv(char* data, long long colors, long long nums)
{
	std::ignore = data;
	std::ignore = colors;
	std::ignore = nums;
}
#pragma endregion

//異步專用尋路
void Worker::findPathAsync(const QPoint& dst, const std::function<bool()>& func)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	safe::auto_flag autoflag(&gamedevice.IS_FINDINGPATH);

	long long floor = getFloor();
	QPoint src(getPoint());
	if (src == dst)
	{
		emit findPathFinished();
		return;
	}

	AStarDevice astar;
	std::vector<QPoint> path;
	util::timer timer;
	QSet<QPoint> blockList;

	QPoint point;
	long long steplen_cache = -1;
	long long pathsize = 0;
	long long current_floor = floor;

	timer.restart();

	for (;;)
	{
		if (gamedevice.isGameInterruptionRequested())
			break;

		if (func)
		{
			if (func())
				break;
		}

		if (!getOnlineFlag())
			break;

		if (!gamedevice.IS_FINDINGPATH.get())
			break;

		if (timer.hasExpired(180000))
			break;

		if (getFloor() != current_floor)
			break;

		src = getPoint();
		if (!mapDevice.calcNewRoute(currentIndex, &astar, floor, src, dst, blockList, &path))
			break;
		pathsize = path.size();

		steplen_cache = 3;

		for (;;)
		{
			if (!((steplen_cache) >= (pathsize)))
				break;
			--steplen_cache;
		}

		if (steplen_cache >= 0 && (steplen_cache < pathsize))
		{
			point = path.at(steplen_cache);
			move(point);
		}

		bool bret = false;
		if (getBattleFlag())
		{
			util::timer stimer;
			bret = true;
			for (;;)
			{
				if (gamedevice.isGameInterruptionRequested())
				{
					emit findPathFinished();
					return;
				}

				if (func)
				{
					if (func())
					{
						emit findPathFinished();
						break;
					}
				}

				if (!gamedevice.IS_FINDINGPATH.get())
				{
					emit findPathFinished();
					return;
				}

				if (!getBattleFlag())
					break;

				if (stimer.hasExpired(180000))
					break;

				QThread::msleep(200);
			}

			QThread::msleep(1000);
		}

		src = getPoint();
		if (!src.isNull() && src == dst)
		{
			move(dst);
			emit findPathFinished();
			return;
		}
	}

	emit findPathFinished();
}

#ifdef OCR_ENABLE
#include "webauthenticator.h"
bool Worker::captchaOCR(QString* pmsg)
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return false;

	//取桌面指針
	QScreen* screen = QGuiApplication::primaryScreen();
	if (nullptr == screen)
	{
		announce("<ocr>screen pointer is nullptr");
		return false;
	}

	//遊戲後臺滑鼠一移動到左上
	LPARAM data = MAKELPARAM(0, 0);
	gamedevice.sendMessage(WM_MOUSEMOVE, NULL, data);

	//根據HWND擷取窗口後臺圖像
	QPixmap pixmap = screen->grabWindow(reinterpret_cast<WId>(gamedevice.getProcessWindow()));

	//轉存為QImage
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
