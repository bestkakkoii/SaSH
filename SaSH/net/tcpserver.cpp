#include "stdafx.h"
#include "tcpserver.h"
#include "autil.h"
#include "util.h"
#include "injector.h"
#include "signaldispatcher.h"

constexpr size_t LINEBUFSIZ = 8192;
int net_readbuflen = 0;
char net_readbuf[LINEBUFSIZ] = {};
char rpc_linebuffer[Autil::NETBUFSIZ] = {};

void Server::clearNetBuffer()
{
	net_readbuflen = 0;
	memset(net_readbuf, 0, sizeof(net_readbuf));
	memset(rpc_linebuffer, 0, sizeof(rpc_linebuffer));
	Autil::util_Release();
}

#pragma region StringControl
int a62toi(const QString& a)
{
	int ret = 0;
	int sign = 1;
	try
	{
		for (int i = 0; i < a.length(); i++)
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
	}
	catch (...)
	{
		return 0;
	}
	return ret * sign;
}

int getStringToken(const QString& src, const QString& delim, int count, QString& out)
{
	int c = 1;
	int i = 0;

	try
	{
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
	}
	catch (...)
	{
		return 1;
	}
	return 0;
}

int getIntegerToken(const QString& src, const QString& delim, int count)
{
	QString s;
	if (getStringToken(src, delim, count, s) == 1)
		return -1;

	return s.toInt();

}

int getInteger62Token(const QString& src, const QString& delim, int count)
{
	QString s;
	getStringToken(src, delim, count, s);
	if (s.isEmpty())
		return -1;
	return a62toi(s);
}

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

QString makeStringFromEscaped(const QString& src)
{
	QString result;
	int srclen = src.length();

	for (int i = 0; i < srclen; i++)
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
				for (int j = 0; j < sizeof(escapeChar) / sizeof(escapeChar[0]); j++)
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

// 0-9,a-z(10-35),A-Z(36-61)
int a62toi(char* a)
{
	int ret = 0;
	int fugo = 1;

	while (*a != NULL)
	{
		ret *= 62;
		if ('0' <= (*a) && (*a) <= '9')
			ret += (*a) - '0';
		else
			if ('a' <= (*a) && (*a) <= 'z')
				ret += (*a) - 'a' + 10;
			else
				if ('A' <= (*a) && (*a) <= 'Z')
					ret += (*a) - 'A' + 36;
				else
					if (*a == '-')
						fugo = -1;
					else
						return 0;
		a++;
	}
	return ret * fugo;
}


int appendReadBuf(char* buf, int size)
{
	if ((net_readbuflen + size) > Autil::NETBUFSIZ)
		return -1;

	memcpy_s(net_readbuf + net_readbuflen, Autil::NETBUFSIZ, buf, size);
	net_readbuflen += size;
	return 0;
}

int shiftReadBuf(int size)
{
	int i;

	if (size > net_readbuflen)
		return -1;
	for (i = size; i < net_readbuflen; i++)
	{
		net_readbuf[i - size] = net_readbuf[i];
	}
	net_readbuflen -= size;
	return 0;
}

int getLineFromReadBuf(char* output, int maxlen)
{
	int i, j;
	if (net_readbuflen >= Autil::NETBUFSIZ)
		return -1;

	for (i = 0; i < net_readbuflen && i < (maxlen - 1); i++)
	{
		if (net_readbuf[i] == '\n')
		{
			memcpy_s(output, Autil::NETBUFSIZ, net_readbuf, i);
			output[i] = '\0';

			for (j = i + 1; j >= 0; j--)
			{
				if (j >= Autil::NETBUFSIZ)
					continue;
				if (output[j] == '\r')
				{
					output[j] = '\0';
					break;
				}
			}

			shiftReadBuf(i + 1);

			net_readbuf[net_readbuflen] = '\0';
			return 0;
		}
	}
	return -1;
}


#pragma endregion

Server::Server(QObject* parent)
	: QObject(parent)
{
	battleDurationTimer.start();
	normalDurationTimer.start();
	eottlTimer.start();

	sync_.setCancelOnWait(true);

	server_.reset(new QTcpServer(parent));
	connect(server_.data(), &QTcpServer::newConnection, this, &Server::onNewConnection);

	if (!server_->listen(QHostAddress::AnyIPv6, 0))
	{
		qDebug() << "Failed to listen on socket";
		return;
	}

	port_ = server_->serverPort();
	Autil::util_Init();
	clearNetBuffer();
}

Server::~Server()
{
	clearNetBuffer();
	requestInterruption();
	server_->close();
	for (QTcpSocket* clientSocket : clientSockets_)
	{
		clientSocket->close();
	}

	for (QTcpSocket* clientSocket : clientSockets_)
	{
		clientSocket->waitForDisconnected();
		delete clientSocket;
	}

	clientSockets_.clear();
	sync_.waitForFinished();
	qDebug() << "Server is distroyed";
}

void Server::run()
{
	//WSADATA data;
	//if (::WSAStartup(MAKEWORD(2, 2), &data) != 0)
	//{
	//	qDebug() << "Failed to initialize Winsock";
	//	return;
	//}

	//SOCKET listenSocket = ::socket(AF_INET6, SOCK_STREAM, 0);
	//if (listenSocket == INVALID_SOCKET)
	//{
	//	qDebug() << "Failed to create socket";
	//	::WSACleanup();
	//	return;
	//}

	//auto setsocket = [](SOCKET fd)
	//{
	//	int bufferSize = 8192;
	//	int minBufferSize = 1;
	//	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(bufferSize));
	//	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(bufferSize));
	//	setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, (char*)&minBufferSize, sizeof(minBufferSize));
	//	setsockopt(fd, SOL_SOCKET, SO_SNDLOWAT, (char*)&minBufferSize, sizeof(minBufferSize));
	//	LINGER lingerOption = {};
	//	lingerOption.l_onoff = 0i16;        // 立即中止
	//	lingerOption.l_linger = 0i16;      // 延遲時間不適用
	//	setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(lingerOption));
	//	int keepAliveOption = 1;  // 開啟保持活動機制
	//	setsockopt(fd, SOL_SOCKET, TCP_KEEPALIVE, (char*)&keepAliveOption, sizeof(keepAliveOption));

	//	//DWORD timeout = 5000; // 超時時間為 5000 毫秒
	//	//setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
	//	//setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	//};

	//setsocket(listenSocket);

	//sockaddr_in6 serverAddr{};
	//serverAddr.sin6_family = AF_INET6;
	//serverAddr.sin6_port = htons(0);
	//serverAddr.sin6_addr = in6addr_any;

	//if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	//{
	//	qDebug() << "Failed to bind socket";
	//	closesocket(listenSocket);
	//	WSACleanup();
	//	return;
	//}

	//int addrLen = sizeof(serverAddr);
	//if (getsockname(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), &addrLen) == SOCKET_ERROR)
	//{
	//	qDebug() << "Failed to get socket address";
	//	closesocket(listenSocket);
	//	WSACleanup();
	//	return;
	//}

	//// 保存綁定的端口號供其他地方使用
	//port_ = ntohs(serverAddr.sin6_port);

	//if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	//{
	//	qDebug() << "Failed to listen on socket";
	//	closesocket(listenSocket);
	//	WSACleanup();
	//	return;
	//}

	//qDebug() << "Server listening on port " << port_;

	//SOCKET sockfd = accept(listenSocket, nullptr, nullptr);
	//if (sockfd == INVALID_SOCKET)
	//{
	//	qDebug() << "Failed to accept client connection";
	//	closesocket(listenSocket);
	//	WSACleanup();
	//	return;
	//}
	//sockfd_ = sockfd;
	//setsocket(sockfd);

	//qDebug() << "Accepted client connection";

	//Autil::util_Init();
	//clearNetBuffer();

	//for (;;)
	//{
	//	if (isInterruptionRequested())
	//		break;

	//	if (!handleClient(sockfd))
	//		break;
	//}

	//clearNetBuffer();
	//if (sockfd != INVALID_SOCKET)
	//	closesocket(sockfd);
	//if (listenSocket != INVALID_SOCKET)
	//	closesocket(listenSocket);
	//sockfd_ = INVALID_SOCKET;
	//WSACleanup();


}

void Server::onNewConnection()
{
	QTcpSocket* clientSocket = server_->nextPendingConnection();
	if (!clientSocket)
		return;

	clientSockets_.append(clientSocket);
	clientSocket->setReadBufferSize(8192);

	clientSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
	clientSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
	clientSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 8192);
	clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 8192);

	connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onClientReadyRead);
	connect(this, &Server::write, this, &Server::onWrite, Qt::QueuedConnection);
}

void Server::onClientReadyRead()
{
	QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
	if (!clientSocket)
		return;
	QByteArray badata = clientSocket->readAll();

	sync_.addFuture(QtConcurrent::run(this, &Server::handleData, clientSocket, badata));

}

void Server::onWrite(QTcpSocket* clientSocket, QByteArray ba, int size)
{
	if (clientSocket && clientSocket->state() != QAbstractSocket::UnconnectedState)
	{
		clientSocket->write(ba.data(), size);
		clientSocket->flush();
	}
}

void Server::handleData(QTcpSocket* clientSocket, QByteArray badata)
{
	QMutexLocker locker(&mutex_);
	int len = badata.size();
	QScopedArrayPointer <char> sendBuf(new char[len]());
	ZeroMemory(rpc_linebuffer, LINEBUFSIZ);
	memcpy_s(rpc_linebuffer, LINEBUFSIZ, badata.data(), len);
	memcpy_s(sendBuf.data(), len, rpc_linebuffer, len);

	appendReadBuf(rpc_linebuffer, len);

	if (net_readbuflen <= 0)
	{
		emit write(clientSocket, sendBuf.data(), len);
		return;
	}

	qDebug() << "Received " << len << " bytes from client but actual len is:" << strlen(rpc_linebuffer);


	Injector& injector = Injector::getInstance();
	QString key = mem::readString(injector.getProcess(), injector.getProcessModule() + 0x4AC0898, 32, true);
	std::string skey = key.toStdString();
	_snprintf_s(Autil::PersonalKey, 32, _TRUNCATE, "%s", skey.c_str());

	bool block = false;
	for (;;)
	{
		if (isInterruptionRequested())
			break;

		RtlZeroMemory(&rpc_linebuffer, sizeof(rpc_linebuffer));
		// get line from read buffer
		if (!getLineFromReadBuf(rpc_linebuffer, sizeof(rpc_linebuffer)))
		{
			int ret = SaDispatchMessage(sockfd_, rpc_linebuffer);
			if (ret < 0)
			{
				qDebug() << "************************ LSSPROTO_END ************************";
				//代表此段數據已到結尾
				//if (isPacketAutoClear.load(std::memory_order_acquire))
				clearNetBuffer();
				break;
			}
			else if (ret == 0)
			{
				//qDebug() << "************************ CLEAR_BUFFER ************************";
				//錯誤的數據 或 需要清除緩存
				clearNetBuffer();
				break;
			}
		}
		else
		{
			//數據讀完了
			Autil::util_Release();
			break;
		}
	}

	emit write(clientSocket, sendBuf.data(), len);
}

int Server::SaDispatchMessage(int fd, char* encoded)
{
	using namespace Autil;

	int		func = 0, fieldcount = 0;
	int		iChecksum = 0, iChecksumrecv = 0;
	QScopedPointer<char> raw(new char[Autil::NETBUFSIZ]());

	util_DecodeMessage(raw.get(), Autil::NETBUFSIZ, encoded);
	util_SplitMessage(raw.get(), Autil::NETBUFSIZ, const_cast<char*>(Autil::SEPARATOR));
	if (util_GetFunctionFromSlice(&func, &fieldcount) != 1)
	{
		return 0;
	}

	//qDebug() << "fun" << func << "fieldcount" << fieldcount;

	auto CHECKFUN = [&func](int cmpfunc) -> bool { return (func == cmpfunc); };

	if (CHECKFUN(LSSPROTO_ERROR_RECV))
	{
		return -1;
	}

	if (CHECKFUN(LSSPROTO_XYD_RECV) /*戰後刷新人物座標、方向2*/)
	{
		int x = 0;
		int y = 0;
		int dir = 0;
		iChecksum += util_deint(2, &x);
		iChecksum += util_deint(3, &y);
		iChecksum += util_deint(4, &dir);

		util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_XYD_RECV" << "x" << x << "y" << y << "dir" << dir;
		lssproto_XYD_recv(fd, x, y, dir);
	}
	else if (CHECKFUN(LSSPROTO_EV_RECV) /*WRAP 4*/)
	{
		int seqno = 0;
		int result = 0;
		iChecksum += util_deint(2, &seqno);
		iChecksum += util_deint(3, &result);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_EV_RECV" << "seqno" << seqno << "result" << result;
		lssproto_EV_recv(fd, seqno, result);
	}
	else if (CHECKFUN(LSSPROTO_EN_RECV) /*Battle EncountFlag //開始戰鬥 7*/)
	{
		int result = 0;
		int field = 0;
		iChecksum += util_deint(2, &result);
		iChecksum += util_deint(3, &field);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_EN_RECV" << "result" << result << "field" << field;
		lssproto_EN_recv(fd, result, field);

		Injector& injector = Injector::getInstance();
		bool isFastBattle = injector.getEnableHash(util::kFastBattleEnable);
		if (isFastBattle)
		{

		}
	}
	else if (CHECKFUN(LSSPROTO_RS_RECV) /*戰後獎勵 12*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_RS_RECV" << util::toUnicode(data);
		lssproto_RS_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_RD_RECV)/*13*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_RD_RECV" << util::toUnicode(data);
		lssproto_RD_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_B_RECV) /*每回合開始的戰場資訊 15*/)
	{
		char command[16384];
		iChecksum += util_destring(2, command);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_B_RECV" << util::toUnicode(command);
		lssproto_B_recv(fd, command);
	}
	else if (CHECKFUN(LSSPROTO_I_RECV) /*物品變動 22*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_I_RECV" << util::toUnicode(data);
		lssproto_I_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_SI_RECV)/* 24*/)
	{
		int fromindex;
		int toindex;

		iChecksum += util_deint(2, &fromindex);
		iChecksum += util_deint(3, &toindex);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_SI_RECV" << "fromindex" << fromindex << "toindex" << toindex;
		lssproto_SI_recv(fd, fromindex, toindex);
	}
	else if (CHECKFUN(LSSPROTO_MSG_RECV)/*26*/)
	{
		int aindex;
		char text[16384];
		int color;

		iChecksum += util_deint(2, &aindex);
		iChecksum += util_destring(3, text);
		iChecksum += util_deint(4, &color);
		util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_MSG_RECV" << util::toUnicode(text);
		lssproto_MSG_recv(fd, aindex, text, color);
	}
	else if (CHECKFUN(LSSPROTO_PME_RECV)/*28*/)
	{
		int objindex;
		int graphicsno;
		int x;
		int y;
		int dir;
		int flg;
		int no;
		char cdata[16384];

		iChecksum += util_deint(2, &objindex);
		iChecksum += util_deint(3, &graphicsno);
		iChecksum += util_deint(4, &x);
		iChecksum += util_deint(5, &y);
		iChecksum += util_deint(6, &dir);
		iChecksum += util_deint(7, &flg);
		iChecksum += util_deint(8, &no);
		iChecksum += util_destring(9, cdata);
		util_deint(10, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_PME_RECV" << "objindex" << objindex << "graphicsno" << graphicsno <<
			"x" << x << "y" << y << "dir" << dir << "flg" << flg << "no" << no << "cdata" << util::toUnicode(cdata);
		lssproto_PME_recv(fd, objindex, graphicsno, x, y, dir, flg, no, cdata);
	}
	else if (CHECKFUN(LSSPROTO_AB_RECV)/* 30*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_AB_RECV" << util::toUnicode(data);
		lssproto_AB_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_ABI_RECV)/*31*/)
	{
		int num;
		char data[16384];

		iChecksum += util_deint(2, &num);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_ABI_RECV" << "num" << num << "data" << util::toUnicode(data);
		lssproto_ABI_recv(fd, num, data);
	}
	else if (CHECKFUN(LSSPROTO_TK_RECV) /*收到對話36*/)
	{
		int index;
		char message[16384];
		int color;

		iChecksum += util_deint(2, &index);
		iChecksum += util_destring(3, message);
		iChecksum += util_deint(4, &color);
		util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_TK_RECV" << "index" << index << "message" << util::toUnicode(message) << "color" << color;
		lssproto_TK_recv(fd, index, message, color);
	}
	else if (CHECKFUN(LSSPROTO_MC_RECV) /*地圖數據更新37*/)
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		int tilesum;
		int objsum;
		int eventsum;
		char data[16384];

		iChecksum += util_deint(2, &fl);
		iChecksum += util_deint(3, &x1);
		iChecksum += util_deint(4, &y1);
		iChecksum += util_deint(5, &x2);
		iChecksum += util_deint(6, &y2);
		iChecksum += util_deint(7, &tilesum);
		iChecksum += util_deint(8, &objsum);
		iChecksum += util_deint(9, &eventsum);
		iChecksum += util_destring(10, data);
		util_deint(11, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_MC_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 <<
			"tilesum" << tilesum << "objsum" << objsum << "eventsum" << eventsum << "data" << util::toUnicode(data);
		lssproto_MC_recv(fd, fl, x1, y1, x2, y2, tilesum, objsum, eventsum, data);
		//m_map.name = qmap;
	}
	else if (CHECKFUN(LSSPROTO_M_RECV) /*地圖數據更新2 39*/)
	{
		int fl;
		int x1;
		int y1;
		int x2;
		int y2;
		char data[16384];

		iChecksum += util_deint(2, &fl);
		iChecksum += util_deint(3, &x1);
		iChecksum += util_deint(4, &y1);
		iChecksum += util_deint(5, &x2);
		iChecksum += util_deint(6, &y2);
		iChecksum += util_destring(7, data);
		util_deint(8, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_M_RECV" << "fl" << fl << "x1" << x1 << "y1" << y1 << "x2" << x2 << "y2" << y2 << "data" << util::toUnicode(data);
		lssproto_M_recv(fd, fl, x1, y1, x2, y2, data);
		//m_map.floor = fl;
	}
	else if (CHECKFUN(LSSPROTO_C_RECV) /*服務端發送的靜態信息，可用於顯示玩家，其它玩家，公交，寵物等信息 41*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_C_RECV" << util::toUnicode(data);
		lssproto_C_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_CA_RECV) /*42*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CA_RECV" << util::toUnicode(data);
		lssproto_CA_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_CD_RECV) /*遊戲物件更新? 43*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CD_RECV" << util::toUnicode(data);
		lssproto_CD_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_R_RECV))
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_R_RECV" << util::toUnicode(data);
		lssproto_R_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_S_RECV) /*更新所有基礎資訊 46*/)
	{
		char data[16384];
		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_S_RECV" << util::toUnicode(data);
		lssproto_S_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_D_RECV)/*47*/)
	{
		int category;
		int dx;
		int dy;
		char data[16384];

		iChecksum += util_deint(2, &category);
		iChecksum += util_deint(3, &dx);
		iChecksum += util_deint(4, &dy);
		iChecksum += util_destring(5, data);
		util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_D_RECV" << "category" << category << "dx" << dx << "dy" << dy << "data" << util::toUnicode(data);
		lssproto_D_recv(fd, category, dx, dy, data);
	}
	else if (CHECKFUN(LSSPROTO_FS_RECV)/* 49*/)
	{
		int flg;

		iChecksum += util_deint(2, &flg);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_FS_RECV" << "flg" << flg;
		lssproto_FS_recv(fd, flg);
	}
	else if (CHECKFUN(LSSPROTO_HL_RECV)/*51*/)
	{
		int flg;

		iChecksum += util_deint(2, &flg);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_HL_RECV" << "flg" << flg;
		lssproto_HL_recv(fd, flg);
	}
	else if (CHECKFUN(LSSPROTO_PR_RECV)/*53*/)
	{
		int request;
		int result;

		iChecksum += util_deint(2, &request);
		iChecksum += util_deint(3, &result);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_PR_RECV" << "request" << request << "result" << result;
		lssproto_PR_recv(fd, request, result);
	}
	else if (CHECKFUN(LSSPROTO_KS_RECV) /*寵物更換狀態55*/)
	{
		int petarray;
		int result;

		iChecksum += util_deint(2, &petarray);
		iChecksum += util_deint(3, &result);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_KS_RECV" << "petarray" << petarray << "result" << result;
		lssproto_KS_recv(fd, petarray, result);
	}
	else if (CHECKFUN(LSSPROTO_PS_RECV))
	{
		int result;
		int havepetindex;
		int havepetskill;
		int toindex;

		iChecksum += util_deint(2, &result);
		iChecksum += util_deint(3, &havepetindex);
		iChecksum += util_deint(4, &havepetskill);
		iChecksum += util_deint(5, &toindex);
		util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_PS_RECV" << "result" << result << "havepetindex" << havepetindex << "havepetskill" << havepetskill << "toindex" << toindex;
		lssproto_PS_recv(fd, result, havepetindex, havepetskill, toindex);
	}
	else if (CHECKFUN(LSSPROTO_SKUP_RECV) /*更新點數 63*/)
	{
		int point;

		iChecksum += util_deint(2, &point);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_SKUP_RECV" << "point" << point;
		lssproto_SKUP_recv(fd, point);
	}
	else if (CHECKFUN(LSSPROTO_WN_RECV)/*NPC對話框 66*/)
	{
		int windowtype;
		int buttontype;
		int seqno;
		int objindex;
		char data[16384];

		iChecksum += util_deint(2, &windowtype);
		iChecksum += util_deint(3, &buttontype);
		iChecksum += util_deint(4, &seqno);
		iChecksum += util_deint(5, &objindex);
		iChecksum += util_destring(6, data);
		util_deint(7, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_WN_RECV" << "windowtype" << windowtype << "buttontype" << buttontype << "seqno" << seqno << "objindex" << objindex << "data" << util::toUnicode(data);
		lssproto_WN_recv(fd, windowtype, buttontype, seqno, objindex, data);
	}
	else if (CHECKFUN(LSSPROTO_EF_RECV) /*天氣68*/)
	{
		int effect;
		int level;
		char option[16384];

		iChecksum += util_deint(2, &effect);
		iChecksum += util_deint(3, &level);
		iChecksum += util_destring(4, option);
		util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_EF_RECV" << "effect" << effect << "level" << level << "option" << util::toUnicode(option);
		lssproto_EF_recv(fd, effect, level, option);
	}
	else if (CHECKFUN(LSSPROTO_SE_RECV)/*69*/)
	{
		int x;
		int y;
		int senumber;
		int sw;

		iChecksum += util_deint(2, &x);
		iChecksum += util_deint(3, &y);
		iChecksum += util_deint(4, &senumber);
		iChecksum += util_deint(5, &sw);
		util_deint(6, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_SE_RECV" << "x" << x << "y" << y << "senumber" << senumber << "sw" << sw;
		lssproto_SE_recv(fd, x, y, senumber, sw);
	}
	else if (CHECKFUN(LSSPROTO_CLIENTLOGIN_RECV)/*選人畫面 72*/)
	{
		char result[16384];

		iChecksum += util_destring(2, result);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CLIENTLOGIN_RECV" << util::toUnicode(result);
		lssproto_ClientLogin_recv(fd, result);

		return 0;
	}
	else if (CHECKFUN(LSSPROTO_CREATENEWCHAR_RECV)/*人物新增74*/)
	{
		char result[16384];
		char data[16384];

		iChecksum += util_destring(2, result);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CREATENEWCHAR_RECV" << util::toUnicode(result) << util::toUnicode(data);
		lssproto_CreateNewChar_recv(fd, result, data);
	}
	else if (CHECKFUN(LSSPROTO_CHARDELETE_RECV)/*人物刪除 76*/)
	{
		char result[16384];
		char data[16384];

		iChecksum += util_destring(2, result);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CHARDELETE_RECV" << util::toUnicode(result) << util::toUnicode(data);
		lssproto_CharDelete_recv(fd, result, data);

	}
	else if (CHECKFUN(LSSPROTO_CHARLOGIN_RECV) /*成功登入 78*/)
	{
		char result[16384];
		char data[16384];

		iChecksum += util_destring(2, result);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CHARLOGIN_RECV" << util::toUnicode(result) << util::toUnicode(data);
		lssproto_CharLogin_recv(fd, result, data);
	}
	else if (CHECKFUN(LSSPROTO_CHARLIST_RECV)/*選人頁面資訊 80*/)
	{
		char result[16384];
		char data[16384];

		iChecksum += util_destring(2, result);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_CHARLIST_RECV" << util::toUnicode(result) << util::toUnicode(data);
		lssproto_CharList_recv(fd, result, data);

		return 0;
	}
	else if (CHECKFUN(LSSPROTO_CHARLOGOUT_RECV)/*登出 82*/)
	{
		char result[16384];
		char data[16384];

		iChecksum += util_destring(2, result);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_CHARLOGOUT_RECV" << util::toUnicode(result) << util::toUnicode(data);
		lssproto_CharLogout_recv(fd, result, data);
	}
	else if (CHECKFUN(LSSPROTO_PROCGET_RECV)/*84*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_PROCGET_RECV" << util::toUnicode(data);
		lssproto_ProcGet_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_PLAYERNUMGET_RECV)/*86*/)
	{
		int logincount;
		int player;

		iChecksum += util_deint(2, &logincount);
		iChecksum += util_deint(3, &player);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_PLAYERNUMGET_RECV" << "logincount:" << logincount << "player:" << player; //"logincount:%d player:%d\n
		lssproto_PlayerNumGet_recv(fd, logincount, player);
	}
	else if (CHECKFUN(LSSPROTO_ECHO_RECV) /*伺服器定時ECHO "hoge" 88*/)
	{
		char test[16384];

		iChecksum += util_destring(2, test);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_ECHO_RECV" << util::toUnicode(test);
		lssproto_Echo_recv(fd, test);
	}
	else if (CHECKFUN(LSSPROTO_NU_RECV) /*不知道幹嘛的 90*/)
	{
		int AddCount;

		iChecksum += util_deint(2, &AddCount);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_NU_RECV" << "AddCount:" << AddCount;
		lssproto_NU_recv(fd, AddCount);

	}
	else if (CHECKFUN(LSSPROTO_TD_RECV)/*92*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_TD_RECV" << util::toUnicode(data);
		lssproto_TD_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_FM_RECV)/*93*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_FM_RECV" << util::toUnicode(data);
		lssproto_FM_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_WO_RECV)/*95*/)
	{
		int effect;

		iChecksum += util_deint(2, &effect);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_WO_RECV" << "effect:" << effect;
		lssproto_WO_recv(fd, effect);
	}
	else if (CHECKFUN(LSSPROTO_NC_RECV) /*沈默? 101* 戰鬥結束*/)
	{
		int flg = 0;
		iChecksum += util_deint(2, &flg);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_NC_RECV" << "flg:" << flg;
		lssproto_NC_recv(fd, flg);
	}
	else if (CHECKFUN(LSSPROTO_CS_RECV)/*104*/)
	{
		int deltimes = 0;
		iChecksum += util_deint(2, &deltimes);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_CS_RECV" << "deltimes:" << deltimes;
		lssproto_CS_recv(fd, deltimes);
	}
	else if (CHECKFUN(LSSPROTO_PETST_RECV) /*寵物狀態改變 107*/)
	{
		int petarray;
		int result;

		iChecksum += util_deint(2, &petarray);
		iChecksum += util_deint(3, &result);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_PETST_RECV" << "petarray:" << petarray << "result:" << result;
		lssproto_PETST_recv(fd, petarray, result);
	}
	else if (CHECKFUN(LSSPROTO_SPET_RECV) /*寵物更換狀態115*/)
	{
		int standbypet;
		int result;

		iChecksum += util_deint(2, &standbypet);
		iChecksum += util_deint(3, &result);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_SPET_RECV" << "standbypet:" << standbypet << "result:" << result;
		lssproto_SPET_recv(fd, standbypet, result);
	}
	else if (CHECKFUN(LSSPROTO_JOBDAILY_RECV)/*120*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_JOBDAILY_RECV" << util::toUnicode(data);
		lssproto_JOBDAILY_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_TEACHER_SYSTEM_RECV)/*123*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_TEACHER_SYSTEM_RECV" << util::toUnicode(data);
		lssproto_TEACHER_SYSTEM_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_FIREWORK_RECV)/*126*/)
	{
		int	iChecksum = 0, iChecksumrecv, iCharaindex, iType, iActionNum;

		iChecksum += util_deint(2, &iCharaindex);
		iChecksum += util_deint(3, &iType);
		iChecksum += util_deint(4, &iActionNum);
		util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}
		qDebug() << "LSSPROTO_FIREWORK_RECV" << "iCharaindex:" << iCharaindex << "iType:" << iType << "iActionNum:" << iActionNum;
		lssproto_Firework_recv(fd, iCharaindex, iType, iActionNum);
	}
	else if (CHECKFUN(LSSPROTO_CHAREFFECT_RECV)/*146*/)
	{
		char data[16384];

		iChecksum += util_destring(2, data);
		util_deint(3, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_CHAREFFECT_RECV" << util::toUnicode(data);
		lssproto_CHAREFFECT_recv(fd, data);
	}
	else if (CHECKFUN(LSSPROTO_DENGON_RECV)/*200*/)
	{
		char data[512];
		int coloer;
		int num;
		int iChecksumrecv;

		iChecksum += util_destring(2, data);
		iChecksum += util_deint(3, &coloer);
		iChecksum += util_deint(4, &num);
		util_deint(5, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_DENGON_RECV" << util::toUnicode(data) << "coloer:" << coloer << "num:" << num;
		lssproto_DENGON_recv(data, coloer, num);
	}
	else if (CHECKFUN(LSSPROTO_SAMENU_RECV)/*201*/)
	{
		int count;
		char data[16384];
		iChecksum += util_deint(2, &count);
		iChecksum += util_destring(3, data);
		util_deint(4, &iChecksumrecv);
		if (iChecksum != iChecksumrecv)
		{
			Autil::SliceCount = 0;
			return 0;
		}

		qDebug() << "LSSPROTO_SAMENU_RECV" << "count:" << count << util::toUnicode(data);
	}
	else if (CHECKFUN(220))
	{
	}
	else
	{
		qDebug() << "-------------------fun" << func << "fieldcount" << fieldcount;
	}

	Autil::SliceCount = 0;
	return 1;

}

void Server::lssproto_CharDelete_recv(int fd, char* cresult, char* cdata)
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

void Server::clearPartyParam()
{
	int i;
#ifdef	MAX_AIRPLANENUM
	for (i = 0; i < MAX_AIRPLANENUM; i++)
#else
	for (i = 0; i < MAX_PARTY; i++)
#endif
	{
		if (party[i].useFlag != 0)
		{
			if (party[i].id == pc.id)
			{
				//if (party[i].ptAct != NULL)
				//{
				//	party[i].ptAct->bufCount = 0;
				//}
				pc.status &= (~CHR_STATUS_PARTY);
			}
			else
			{
				//if ((pc.status & CHR_STATUS_LEADER) != 0
				//	&& party[i].ptAct != NULL)
				//{
				//	party[i].ptAct->bufCount = 0;
				//}
				//delCharParty(party[i].ptAct);
			}
		}
		party[i].useFlag = 0;
		party[i].id = 0;
		//party[i].ptAct = NULL;

	}
	pc.status &= (~CHR_STATUS_LEADER);
}

void Server::lssproto_PR_recv(int fd, int request, int result)
{

	if (request == 1 && result == 1)
	{
		pc.status |= CHR_STATUS_PARTY;
	}
	else
	{
		if (request == 0 && result == 1)
		{
			partyModeFlag = 0;
			clearPartyParam();
#ifdef _CHANNEL_MODIFY
			//pc.etcFlag &= ~PC_ETCFLAG_CHAT_MODE;
			if (TalkMode == 2) TalkMode = 0;
#endif

			char dir = (pc.dir + 5) % 8;

			//lssproto_SP_send(fd, nextGx, nextGy, dir);

		}
	}
	prSendFlag = 0;
}

void Server::lssproto_EV_recv(int fd, int seqno, int result)
{
	if (eventWarpSendId == seqno)
	{
		eventWarpSendFlag = 0;
		if (result == 0)
		{
			redrawMap();
			floorChangeFlag = FALSE;

			warpEffectStart = true;
			warpEffectOk = true;
		}
	}
	else
	{
		if (eventEnemySendId == seqno)
		{
			if (result == 0)
			{
				eventEnemySendFlag = 0;
			}
		}
	}
}

void Server::lssproto_FS_recv(int fd, int flg)
{
	pc.etcFlag = static_cast<unsigned short>(flg);

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

bool Server::CheckMailNoReadFlag()
{
	int i, j;

	for (i = 0; i < MAX_ADR_BOOK; i++)
	{
		for (j = 0; j < MAIL_MAX_HISTORY; j++)
		{
			if (MailHistory[i].noReadFlag[j] >= TRUE) return true;
		}
	}
	return false;
}

void Server::lssproto_AB_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//	int i;
	//	int no;
	//	int nameLen;
	//	QString name;
	//	int flag;
	//	int useFlag;
	//#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	//	QString planetid;
	//	int j;
	//#endif
	//
	//	if (logOutFlag)
	//		return;
	//
	//	for (i = 0; i < MAX_ADR_BOOK; i++)
	//	{
	//		//no = i * 6; //the second
	//		no = i * 8;
	//		useFlag = getIntegerToken(data, "|", no + 1);
	//		if (useFlag == -1)
	//		{
	//			useFlag = 0;
	//		}
	//		if (useFlag <= 0)
	//		{
	//#if 0
	//			if (addressBook[i].useFlag == 1)
	//#else
	//			if (MailHistory[i].dateStr[MAIL_MAX_HISTORY - 1][0] != '\0')
	//#endif
	//			{
	//				MailHistory[i] = MailHistory[i];
	//				//SaveMailHistory(i);
	//				mailLamp = CheckMailNoReadFlag();
	//				//DeathLetterAction();
	//			}
	//			addressBook[i].useFlag = 0;
	//			addressBook[i].name[0] = '\0';
	//			continue;
	//	}
	//
	//#ifdef _EXTEND_AB
	//		if (i == MAX_ADR_BOOK - 1)
	//			addressBook[i].useFlag = useFlag;
	//		else
	//			addressBook[i].useFlag = 1;
	//#else
	//		addressBook[i].useFlag = 1;
	//#endif
	//
	//		flag = getStringToken(data, "|", no + 2, name);
	//
	//		if (flag == 1)
	//			break;
	//
	//		makeStringFromEscaped(name);
	//		nameLen = name.size();
	//		if (0 < nameLen && nameLen <= CHAR_NAME_LEN)
	//		{
	//			addressBook[i].name = name;
	//		}
	//		addressBook[i].level = getIntegerToken(data, "|", no + 3);
	//		addressBook[i].dp = getIntegerToken(data, "|", no + 4);
	//		addressBook[i].onlineFlag = (short)getIntegerToken(data, "|", no + 5);
	//		addressBook[i].graNo = getIntegerToken(data, "|", no + 6);
	//		addressBook[i].transmigration = getIntegerToken(data, "|", no + 7);
	//#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	//		for (j = 0; j < MAX_GMSV; j++)
	//		{
	//			if (gmsv[j].used == '1')
	//			{
	//				getStringToken(gmsv[j].ipaddr, '.', 4, sizeof(planetid) - 1, planetid);
	//				if (addressBook[i].onlineFlag == atoi(planetid))
	//				{
	//					sprintf_s(addressBook[i].planetname, "%s", gmsv[j].name);
	//					break;
	//				}
	//			}
	//}
	//#endif
	//	}
}

////////////////////////////////////////
void Server::lssproto_ABI_recv(int fd, int num, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString name;
	int nameLen;
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
#if 0
		if (addressBook[num].useFlag == 1)
#else
		if (MailHistory[num].dateStr[MAIL_MAX_HISTORY - 1][0] != '\0')
#endif
		{
			MailHistory[num].clear();
			//SaveMailHistory(num);
			mailLamp = CheckMailNoReadFlag();
			//DeathLetterAction();
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
	nameLen = name.size();
	addressBook[num].name = name;
	addressBook[num].level = getIntegerToken(data, "|", 3);
	addressBook[num].dp = getIntegerToken(data, "|", 4);
	addressBook[num].onlineFlag = (short)getIntegerToken(data, "|", 5);
	addressBook[num].graNo = getIntegerToken(data, "|", 6);
	addressBook[num].transmigration = getIntegerToken(data, "|", 7);
#ifdef _MAILSHOWPLANET				// (可開放) Syu ADD 顯示名片星球
	if (addressBook[num].onlineFlag == 0)
		sprintf_s(addressBook[num].planetname, " ");
	for (j = 0; j < MAX_GMSV; j++)
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

void Server::lssproto_RS_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i;
	QString token;
	QString item;

	battleResultMsg.useFlag = 1;
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

	for (i = 0; i < cols; i++)
	{
		if (i >= 5)
			break;
		getStringToken(data, ",", i + 1, token);

		battleResultMsg.resChr[i].petNo = getIntegerToken(token, "|", 1);
		battleResultMsg.resChr[i].levelUp = getIntegerToken(token, "|", 2);
		battleResultMsg.resChr[i].exp = getInteger62Token(token, "|", 3);
	}

	getStringToken(data, ",", i + 1, token);
	getStringToken(token, "|", 1, item);
	makeStringFromEscaped(item);
	battleResultMsg.item[0] = item;
	getStringToken(token, "|", 2, item);
	makeStringFromEscaped(item);
	battleResultMsg.item[1] = item;
	getStringToken(token, "|", 3, item);
	makeStringFromEscaped(item);
	battleResultMsg.item[2] = item;

	setBattleEnd();
}

/////////////////////////////////////////////////
void Server::lssproto_RD_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	battleResultMsg.useFlag = 2;

	battleResultMsg.resChr[0].exp = getInteger62Token(data, "|", 1);
	battleResultMsg.resChr[1].exp = getInteger62Token(data, "|", 2);
}

void Server::swapItem(int from, int to)
{
	if (from < 0 || to < 0)
		return;
	std::swap(pc.item[from], pc.item[to]);
}

///////////////////////////////////////////////////
void Server::lssproto_SI_recv(int fd, int from, int to)
{
	swapItem(from, to);
	refreshItemInfo(from);
	refreshItemInfo(to);
}

///////////////////////////////////////////////////////
void Server::lssproto_I_recv(int fd, char* cdata)
{
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

	for (j = 0; ; j++)
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

		if (i >= 24)
			break;

		makeStringFromEscaped(name);
		if (name.isEmpty())
		{
			pc.item[i].useFlag = 0;
			pc.item[i].name = "";
			refreshItemInfo(i);
			continue;
		}
		pc.item[i].useFlag = 1;
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
			pc.item[i].pile = pile.toInt();
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
}

void Server::lssproto_WN_recv(int fd, int windowtype, int buttontype, int seqno, int objindex, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//if (strstr(data, "否則家族在七天之後會消失唷！"))
	//{
	//	if (TimeGetTime() - MsgCooltime > 300000)
	//		MsgCooltime = TimeGetTime();
	//	else
	//		return;
	//}
	//openServerWindow(windowtype, buttontype, seqno, objindex, data);

}

///////////////////////////////////////////////////////
void Server::lssproto_PME_recv(int fd, int objindex,
	int graphicsno, int x, int y, int dir, int flg, int no, char* cdata)
{
	QString data = util::toUnicode(cdata);

	if (IS_BATTLE_FLAG)
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
		getStringToken(data, "|", ps++, freeName);
		makeStringFromEscaped(freeName);
		getStringToken(data, "|", ps++, smalltoken);
		walkable = smalltoken.toInt();
		getStringToken(data, "|", ps++, smalltoken);
		height = smalltoken.toInt();

		//if (setReturnPetObj(id, graNo, x, y, dir, name, freeName,
		//	level, nameColor, walkable, height, charType))
		//{
		//	switch (no)
		//	{
		//	case 0:
		//		//createPetAction(graphicsno, x, y, dir, 1, 0, objindex);
		//		break;

		//	case 1:
		//		//createPetAction(graphicsno, x, y, dir, 3, 0, objindex);
		//		break;
		//	}
		//}
	}
}

/////////////////////////////////////////////
void Server::lssproto_EF_recv(int fd, int effect, int level, char* coption)
{
	//char* pCommand = NULL;
	//DWORD dwDiceTimer;
	QString option = util::toUnicode(coption);

	if (effect == 0)
	{
		mapEffectRainLevel = 0;
		mapEffectSnowLevel = 0;
		mapEffectKamiFubukiLevel = 0;
#ifdef _HALLOWEEN_EFFECT
		mapEffectHalloween = 0;
		initMapEffect(FALSE);
#endif
		return;
	}

	if (effect & 1)
	{
		mapEffectRainLevel = level;
	}

	if (effect & 2)
	{
		mapEffectSnowLevel = level;
	}

	if (effect & 4)
	{
		mapEffectKamiFubukiLevel = level;
	}
#ifdef _HALLOWEEN_EFFECT
	if (effect & 8) mapEffectHalloween = level;
#endif
	// Terry add 2002/01/14
#ifdef __EDEN_DICE
	// 骰子
	if (effect == 10)
	{
		pCommand = (char*)MALLOC(strlen(option) + 1);
#ifdef _STONDEBUG_
		g_iMallocCount++;
#endif
		if (pCommand != NULL)
		{
			strcpy(pCommand, strlen(option) + 1, option);
			bMapEffectDice = TRUE;
			dwDiceTimer = TimeGetTime();
		}
	}
#endif
	// Terry end
}

///////////////////////////////////////////////////////////////////
//void Server::lssproto_IS_recv(int fd, char* cdata)
//{
//}

void Server::lssproto_EN_recv(int fd, int result, int field)
{
	//開始戰鬥為1，未開始戰鬥為0
	if (result > 0)
	{
		if (result == 4)
			vsLookFlag = 1;
		else
			vsLookFlag = 0;
		if (result == 6 || result == 2)
			eventEnemyFlag = 1;
		else
			eventEnemyFlag = 0;

		if (field < 0 || BATTLE_MAP_FILES <= field)
			BattleMapNo = 0;
		else
			BattleMapNo = field;
		if (result == 2)
			DuelFlag = true;
		else
			DuelFlag = false;

		if (result == 2 || result == 5)
			NoHelpFlag = true;
		else
			NoHelpFlag = false;

		BattleStatusReadPointer = BattleStatusWritePointer = 0;
		BattleCmdReadPointer = BattleCmdWritePointer = 0;
#ifdef PK_SYSTEM_TIMER_BY_ZHU
		BattleCliTurnNo = -1;
#endif
		enablePlayerWork = false;
		enablePetWork = false;
		IS_BATTLE_FLAG = true;
		normalDurationTimer.restart();
		battleDurationTimer.restart();
	}
	else
	{
		sendEnFlag = 0;
		duelSendFlag = 0;
		jbSendFlag = 0;
	}
}

void Server::lssproto_HL_recv(int fd, int flg)
{
	helpFlag = flg;
}

void Server::lssproto_B_recv(int fd, char* ccommand)
{
	Injector& injector = Injector::getInstance();
	bool isFastBattle = injector.getEnableHash(util::kFastBattleEnable);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString command = util::toUnicode(ccommand);
	QString first = command.left(2);
	first.remove("B");
	QString data = command.mid(3);
	if (data.isEmpty())
		return;

	if (first == "C")
	{
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
		if (BattleStatusWritePointer < 4)
		{
			BattleTurnReceiveFlag = true;
			BattleStatusBak[BattleStatusWritePointer] = command;
			BattleStatusWritePointer = (BattleStatusWritePointer + 1) & (BATTLE_BUF_SIZE - 1);
		}
		QString temp;
		//fl = getIntegerToken(data, "|", 1);

		battleData.allies.clear();
		battleData.enemies.clear();
		battleData.fieldAttr = 0;
		battleData.pet = {};
		battleData.player = {};
		battleData.objects.clear();
		battleData.objects.resize(20);

		int i = 0;
		battleData.fieldAttr = getIntegerToken(data, "|", 1);

		//16進制使用 a62toi
		//string 使用 getStringToken(data, "|", n, var); 而後使用 makeStringFromEscaped(var); 轉換
		//int 使用 getIntegerToken(data, "|", n);

		QVector<QStringList> topList;
		QVector<QStringList> bottomList;

		bool bret = false;
		bool ok = false;
		for (;;)
		{
			getStringToken(data, "|", i * 13 + 2, temp);
			int pos = temp.toInt(&ok, 16);
			if (!ok)
				break;

			battleobject_t obj = {};

			obj.pos = pos;

			getStringToken(data, "|", i * 13 + 3, temp);
			makeStringFromEscaped(temp);
			obj.name = temp;

			getStringToken(data, "|", i * 13 + 4, temp);
			makeStringFromEscaped(temp);
			obj.freename = temp;

			getStringToken(data, "|", i * 13 + 5, temp);
			obj.faceid = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 6, temp);
			obj.level = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 7, temp);
			obj.hp = temp.toInt(nullptr, 16);

			getStringToken(data, "|", i * 13 + 8, temp);
			obj.maxHp = temp.toInt(nullptr, 16);

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

			if (pc.name == obj.name)
			{
				battleData.player.pos = pos;
				battleData.player.name = obj.name;
				battleData.player.freename = obj.freename;
				battleData.player.faceid = obj.faceid;
				battleData.player.level = obj.level;
				battleData.player.hp = obj.hp;
				battleData.player.maxHp = obj.maxHp;
				battleData.player.status = obj.status;
				battleData.player.rideFlag = obj.rideFlag;
				battleData.player.rideName = obj.rideName;
				battleData.player.rideLevel = obj.rideLevel;
				battleData.player.rideHp = obj.rideHp;
				battleData.player.rideMaxHp = obj.rideMaxHp;

				pc.hp = obj.hp;
				pc.maxHp = obj.maxHp;
				emit signalDispatcher.updateCharHpProgressValue(obj.level, obj.hp, obj.maxHp);
				if (obj.rideFlag == 1)
				{
					for (int i = 0; i < MAX_PET; ++i)
					{
						if (pet[i].name.isEmpty() && pet[i].freeName.isEmpty())
							continue;
						if ((pet[i].maxHp == obj.maxHp) &&
							(pet[i].level == obj.level) &&
							(pet[i].name == obj.name || pet[i].freeName == obj.name))
						{
							pc.ridePetNo = i;
							break;
						}
					}

					if (pc.ridePetNo > 0 && pc.ridePetNo < MAX_PET)
					{
						pet[pc.ridePetNo].hp = obj.rideHp;
						pet[pc.ridePetNo].maxHp = obj.rideMaxHp;
						emit signalDispatcher.updateRideHpProgressValue(obj.rideLevel, obj.rideHp, obj.rideMaxHp);
					}
				}
				else
				{
					pc.ridePetNo = -1;
				}

				if (pc.ridePetNo < 0)
				{
					emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
				}
			}

			for (int i = 0; i < MAX_PET; ++i)
			{
				if (pet[i].name.isEmpty() && pet[i].freeName.isEmpty())
					continue;
				if (pet[i].maxHp == obj.maxHp &&
					(pet[i].level == obj.level) &&
					(pet[i].name == obj.name || pet[i].freeName == obj.name))
				{
					pc.battlePetNo = i;
					break;
				}
			}

			if (pc.battlePetNo < 0)
			{
				emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
			}

			if (pc.battlePetNo >= 0 && pc.battlePetNo < MAX_PET)
			{

				if ((pet[pc.battlePetNo].maxHp == obj.maxHp) &&
					(pet[pc.battlePetNo].level == obj.level) &&
					(pet[pc.battlePetNo].name == obj.name || pet[pc.battlePetNo].freeName == obj.name))
				{
					battleData.pet.pos = pos;
					battleData.pet.name = obj.name;
					battleData.pet.freename = obj.freename;
					battleData.pet.faceid = obj.faceid;
					battleData.pet.level = obj.level;
					battleData.pet.hp = obj.hp;
					battleData.pet.maxHp = obj.maxHp;
					battleData.pet.status = obj.status;
					battleData.pet.rideFlag = obj.rideFlag;
					battleData.pet.rideName = obj.rideName;
					battleData.pet.rideLevel = obj.rideLevel;
					battleData.pet.rideHp = obj.rideHp;
					battleData.pet.rideMaxHp = obj.rideMaxHp;

					pet[pc.battlePetNo].hp = obj.hp;
					pet[pc.battlePetNo].maxHp = obj.maxHp;
					emit signalDispatcher.updatePetHpProgressValue(obj.level, obj.hp, obj.maxHp);
				}
			}

			if (pos < battleData.objects.size())
				battleData.objects[pos] = obj;

			QStringList tempList = {};

			tempList.append(QString::number(pos));

			temp = QString("%1 LV:%2 HP:%3/%4").arg(obj.name).arg(obj.level)
				.arg(obj.hp).arg(obj.maxHp);

			tempList.append(temp);

			if (obj.rideFlag == 1)
			{
				temp = QString("%1 LV:%2 HP:%3/%4").arg(obj.rideName).arg(obj.rideLevel)
					.arg(obj.rideHp).arg(obj.rideMaxHp);
			}
			else
				temp.clear();

			tempList.append(temp);

			if ((pos >= 0) && (pos <= 9))
			{
				qDebug() << "allie pos:" << pos << "value:" << tempList;
				bottomList.append(tempList);
			}
			else if ((pos >= 10) && (pos <= 19))
			{
				qDebug() << "enemy pos:" << pos << "value:" << tempList;
				topList.append(tempList);
			}

			++i;
		}

		bool isEnemyAllDead = true;
		bool isAllieAllDead = true;
		for (const battleobject_t& it : battleData.objects)
		{
			if ((it.level > 0 && it.maxHp > 0 && it.hp > 0) || it.status == BC_FLG_HIDE)
			{
				if (it.pos >= 0 && it.pos <= 9)
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

		if (!topList.isEmpty())
		{
			qDebug() << "top(enemy) = " << topList.size();
			QVariant top = QVariant::fromValue(topList);
			emit signalDispatcher.updateTopInfoContents(top);
		}

		if (!bottomList.isEmpty())
		{
			qDebug() << "bottom(allie) = " << bottomList.size();
			QVariant bottom = QVariant::fromValue(bottomList);
			emit signalDispatcher.updateBottomInfoContents(bottom);
		}

		if (isAllieAllDead || isEnemyAllDead)
		{
			setBattleEnd();
		}
		else
		{
			IS_BATTLE_FLAG = true;

			//if (!battleData.objects.isEmpty())
			//{
			//	asyncBattleWork();
			//}
		}
	}

	else if (first == "P")
	{
		QStringList list = data.split("|");
		//sscanf_s(command + 3, "%X|%X|%X", &BattleMyNo, &BattleBpFlag, &BattleMyMp);
		if (list.size() < 3)
			return;
		BattleMyNo = list[0].toInt(nullptr, 16);
		BattleBpFlag = list[1].toInt(nullptr, 16);
		BattleMyMp = list[2].toInt(nullptr, 16);
		battleData.player.pos = BattleMyNo;
		pc.mp = BattleMyMp;
	}

	else if (first == "A")
	{
		//sscanf_s(command + 3, "%X|%X", &BattleAnimFlag, &BattleSvTurnNo);
		QStringList list = data.split("|");
		if (list.size() < 2)
			return;

		BattleAnimFlag = list[0].toInt(nullptr, 16);
		BattleSvTurnNo = list[1].toInt(nullptr, 16);
		if (BattleTurnReceiveFlag)
		{
			BattleCliTurnNo = BattleSvTurnNo;
			BattleTurnReceiveFlag = false;
		}

		if (BattleAnimFlag <= 0)
			return;

		int bit = 0;
		int value = 0;
		int n = 0;
		enablePlayerWork = false;
		enablePetWork = false;
		enemyAllReady = false;

		battleData.allies.clear();
		if (!battleData.objects.isEmpty())
		{
			for (int i = 0; i < 10; ++i)
			{
				if (i >= battleData.objects.size())
					break;

				bit = 1 << i;
				value = BattleAnimFlag & bit;
				if (value == bit)
				{
					battleobject_t bt = battleData.objects.at(i);
					battleData.allies.append(bt);
				}
				else if (i == battleData.player.pos)
				{
					enablePlayerWork = true;
				}
				else if (i == battleData.pet.pos)
				{
					enablePetWork = true;
				}
			}

			battleData.enemies.clear();

			for (int i = 10; i < 20; ++i)
			{
				if (i >= battleData.objects.size())
					break;
				battleobject_t bt = battleData.objects.at(i);
				bit = 1 << i;
				value = BattleAnimFlag & bit;
				if (value == bit)
				{

					battleData.enemies.append(bt);
				}

				if (bt.maxHp > 0 && bt.level > 0)
					++n;
			}

			if (battleData.enemies.size() == n)
			{
				enemyAllReady = true;
			}
		}

		qDebug() << "Enemy Count:" << battleData.enemies.size() << "Allie Count:" << battleData.allies.size();
		qDebug() << "Player Work:" << enablePlayerWork << "Pet Work:" << enablePetWork << "Enemy All Ready:" << enemyAllReady << n;

		if (!battleData.enemies.isEmpty())
		{
			asyncBattleWork();
		}
		else
		{
			setBattleEnd();
		}
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
#ifdef PK_SYSTEM_TIMER_BY_ZHU
	else if (first == "Z")
	{
		int TurnNo = -1;
		sscanf(command + 3, "%X", &TurnNo);
		if (TurnNo >= 0)
		{
			if (TurnNo > 0)
			{
				BattleCntDownRest = TRUE;
			}
			else
			{
				BattleCntDown = TimeGetTime() + BATTLE_CNT_DOWN_TIME;
			}
			BattleCliTurnNo = TurnNo;
		}
	}
	else if (first == "F")
	{
		int TurnNo = -1;
		sscanf(command + 3, "%X", &TurnNo);
		if (TurnNo >= 0)
		{
			BattleCliTurnNo = TurnNo;
		}
		//if ( TurnNo >= 0 )
		//{
		//	if ( TurnNo > 0 )
		//	{
		//		BattleCntDownRest = TRUE;
		//	} else {
		//		BattleCntDown = TimeGetTime() + BATTLE_CNT_DOWN_TIME;
		//	}
		//	BattleCliTurnNo = TurnNo;
		//}
		BattleDown();
		//BattleSetWazaHitBox( 0, 0 );
	}
	else if (first == "O")
	{
		int TurnNo = -1;
		sscanf(command + 3, "%X", &TurnNo);
		if (TurnNo >= 0)
		{
			BattleCliTurnNo = TurnNo;
			BattleCntDown = TimeGetTime() + BATTLE_CNT_DOWN_TIME;
		}
	}
#endif
	else
	{
		qDebug() << "lssproto_B_recv: unknown command" << command;
		if (BattleCmdWritePointer < 4)
		{
			BattleCmdBak[BattleCmdWritePointer] = command;
			BattleCmdWritePointer = (BattleCmdWritePointer + 1) & (BATTLE_BUF_SIZE - 1);
		}
	}

#ifdef _STONDEBUG__MSG
	//StockChatBufferLine( command, FONT_PAL_RED );
#endif

}

void Server::asyncBattleWork()
{

	if (!enemyAllReady)
		return;

	Injector& injector = Injector::getInstance();
	if (ayncBattleCommand.isRunning())
	{
		ayncBattleCommandFlag.store(true, std::memory_order_release);
		ayncBattleCommand.cancel();
		ayncBattleCommand.waitForFinished();
	}

	ayncBattleCommandFlag.store(false, std::memory_order_release);

	//bool FastCheck = injector.getEnableHash(util::kFastBattleEnable) && getWorldStatus() == 9;
	//if (FastCheck)
	//{
	//	qDebug() << "Async Fast Battle Work";
	//	if (enablePlayerWork)
	//		playerDoBattleWork();
	//	if (enablePetWork)
	//		petDoBattleWork();
	//}

	ayncBattleCommand = QtConcurrent::run([this]()
		{
			Injector& injector = Injector::getInstance();
			bool FastCheck = injector.getEnableHash(util::kFastBattleEnable);
			bool Checked = injector.getEnableHash(util::kAutoBattleEnable) || (injector.getEnableHash(util::kFastBattleEnable) && getWorldStatus() == 10);
			if (Checked)
			{
				qDebug() << "Async Normal Battle Work";
				QElapsedTimer timer; timer.start();
				for (;;)
				{
					if (!this->isRunning())
						return;
					if (timer.hasExpired(5000))
						break;
					if (ayncBattleCommandFlag.load(std::memory_order_acquire))
						return;

					int G = getGameStatus();
					int W = getWorldStatus();
					if ((4 == G) && (10 == W))
					{
						if (!IS_BATTLE_FLAG)
							IS_BATTLE_FLAG = true;
						break;
					}
					else if ((3 == G) && (9 == W))
					{
						if (IS_BATTLE_FLAG)
							IS_BATTLE_FLAG = false;
						return;
					}
					else if (!IS_BATTLE_FLAG)
						break;
					else if (!IS_ONLINE_FLAG)
						return;
				}


			}

			auto setEnd = [this, Checked]()->void
			{
				if (Checked)
				{
					QThread::msleep(100);

					int G = getGameStatus();
					if (G == 4)
					{
						++G;
						setGameStatus(G);
					}
				}
				lssproto_EO_send(sockfd_, 0);
				lssproto_Echo_send(sockfd_, const_cast<char*>("????"));

			};

			if (enablePlayerWork)
			{
				playerDoBattleWork();
				if (pc.battlePetNo < 0 && pc.battlePetNo >= MAX_PET)
				{
					setEnd();
				}
				return;
			}

			if (enablePetWork)
			{
				petDoBattleWork();
				setEnd();
			}

			ayncBattleCommandFlag.store(false, std::memory_order_release);
		});
}

#ifdef _PETS_SELECTCON
void Server::lssproto_PETST_recv(int fd, int petarray, int result)
{
	if (petarray < 0 || petarray >= 5) return;
	pc.selectPetNo[petarray] = result;
	BattlePetStMenCnt--;
	if (BattlePetStMenCnt < 0) BattlePetStMenCnt = 0;
	if (BattlePetStMenCnt > 4) BattlePetStMenCnt = 4;
	if (pc.battlePetNo == petarray)
		pc.battlePetNo = -1;

}

#endif

void Server::lssproto_KS_recv(int fd, int petarray, int result)
{
	if (petarray < 0 || petarray >= 5) return;
	int cnt = 0;
	int i;

	BattlePetReceiveFlag = false;
	BattlePetReceivePetNo = -1;
	if (result == TRUE)
	{
		battlePetNoBak = -2;
		if (petarray != -1)
		{
			pc.selectPetNo[petarray] = TRUE;
			if (pc.mailPetNo == petarray)
				pc.mailPetNo = -1;
			for (i = 0; i < 5; i++)
			{
				if (pc.selectPetNo[i] == TRUE && i != petarray) cnt++;
				if (cnt >= 4)
				{
					pc.selectPetNo[i] = FALSE;
					cnt--;
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
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (pc.battlePetNo < 0)
		emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
	else
	{
		PET _pet = pet[pc.battlePetNo];
		emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
	}
}

#ifdef _STANDBYPET
void Server::lssproto_SPET_recv(int fd, int standbypet, int result)
{
	int cnt = 0;
	int i;

	StandbyPetSendFlag = false;

	if (result == TRUE)
	{
		pc.standbyPet = standbypet;
		for (i = 0; i < MAX_PET; i++)
		{
			if (standbypet & (1 << i))
				pc.selectPetNo[i] = TRUE;
			else
				pc.selectPetNo[i] = FALSE;
		}
	}

}
#endif

void Server::lssproto_SKUP_recv(int fd, int point)
{
	StatusUpPoint = point;
}

void Server::lssproto_MSG_recv(int fd, int aindex, char* ctext, int color)
{
	QString text = util::toUnicode(ctext);
	if (text.isEmpty())
		return;
	//char moji[256];
	int noReadFlag;

	mailLamp = TRUE;

	if (aindex < 0 || aindex >= MAIL_MAX_HISTORY)
		return;

	MailHistory[aindex].newHistoryNo--;

	if (MailHistory[aindex].newHistoryNo <= -1)
		MailHistory[aindex].newHistoryNo = MAIL_MAX_HISTORY - 1;

	getStringToken(text, "|", 1, MailHistory[aindex].dateStr[MailHistory[aindex].newHistoryNo]);

	getStringToken(text, "|", 2, MailHistory[aindex].str[MailHistory[aindex].newHistoryNo]);

	makeStringFromEscaped(MailHistory[aindex].str[MailHistory[aindex].newHistoryNo]);


	noReadFlag = getIntegerToken(text, "|", 3);

	if (noReadFlag != -1)
	{
		MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo] = noReadFlag;

		MailHistory[aindex].petLevel[MailHistory[aindex].newHistoryNo] = getIntegerToken(text, "|", 4);

		getStringToken(text, "|", 5, MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo]);

		makeStringFromEscaped(MailHistory[aindex].petName[MailHistory[aindex].newHistoryNo]);

		MailHistory[aindex].itemGraNo[MailHistory[aindex].newHistoryNo] = getIntegerToken(text, "|", 6);

		//sprintf_s(moji, "收到%s送來的寵物郵件！", addressBook[aindex].name);
	}

	else
	{
		MailHistory[aindex].noReadFlag[MailHistory[aindex].newHistoryNo] = TRUE;

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

void Server::lssproto_PS_recv(int fd, int result, int havepetindex, int havepetskill, int toindex)
{
	QString moji;

	ItemMixRecvFlag = false;

	if (result == 0)
	{

		moji = "fail";

		//StockChatBufferLine(moji, FONT_PAL_WHITE);
	}

}

void Server::lssproto_SE_recv(int fd, int x, int y, int senumber, int sw)
{

	if (sw)
	{
		//play_se(senumber, x, y);
	}
	else
	{

	}
}

void Server::lssproto_XYD_recv(int fd, int x, int y, int dir)
{
	updateMapArea();
	setPcWarpPoint(x, y);

	//setPcPoint();
	dir = (dir + 3) % 8;
	pc.dir = dir;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2").arg(nowGx).arg(nowGy));
}

void Server::lssproto_WO_recv(int fd, int effect)
{
	return;

	if (effect == 0)
	{
		transmigrationEffectFlag = 1;
		transEffectPaletteStatus = 1;
		palNo = 15;
		palTime = 300;
	}
}

/////////////////////////////////////////////////////////
void Server::lssproto_Echo_recv(int fd, char* test)
{
	IS_ONLINE_FLAG = true;
	if (isEOTTLSend)
	{
		isEOTTLSend = false;
		announce(tr("server response time:%1ms").arg(eottlTimer.elapsed()));//伺服器響應時間:xxxms
	}
#if 1
#ifdef _STONDEBUG__MSG

	time(&serverAliveLongTime);
	localtime_s(&serverAliveTime, &serverAliveLongTime);

#endif
#endif
}

// Robin 2001/04/06
void Server::lssproto_NU_recv(int fd, int AddCount)
{
	qDebug() << "-- NU --";
}

void Server::lssproto_PlayerNumGet_recv(int fd, int logincount, int player)
{
}

void Server::lssproto_ProcGet_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

void Server::lssproto_R_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

void Server::lssproto_D_recv(int fd, int category, int dx, int dy, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

//void Server::lssproto_ACI_recv(int fd, char* data)
//{
//
//}

void Server::lssproto_FM_recv(int fd, char* cdata)
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
}

//DWORD dwPingTime;
//DWORD dwPingState;
#ifdef __NEW_CLIENT

#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0
#define ICMP_MIN 8 // minimum 8 byte icmp packet (just header)

/* The IP header */
typedef struct iphdr
{
	unsigned int h_len : 4;          // length of the header
	unsigned int version : 4;        // Version of IP
	unsigned char tos;             // Type of service
	unsigned short total_len;      // total length of the packet
	unsigned short ident;          // unique identifier
	unsigned short frag_and_flags; // flags
	unsigned char  ttl;
	unsigned char proto;           // protocol (TCP, UDP etc)
	unsigned short checksum;       // IP checksum

	unsigned int sourceIP;
	unsigned int destIP;

}IpHeader;

typedef struct _ihdr
{
	BYTE i_type;
	BYTE i_code; /* type sub code */
	USHORT i_cksum;
	USHORT i_id;
	USHORT i_seq;
	/* This is not the std header, but we reserve space for time */
	ULONG timestamp;
}IcmpHeader;

#define STATUS_FAILED 0xFFFF
#define DEF_PACKET_SIZE 32
#define MAX_PACKET 1024

USHORT checksum(USHORT* buffer, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	if (size)
		cksum += *(UCHAR*)buffer;
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

int iWrote, iRead;
SOCKET sockRaw = INVALID_SOCKET;
DWORD WINAPI PingFunc(LPVOID param)
{
	struct sockaddr_in from;
	struct sockaddr_in dest;
	int datasize;
	int fromlen = sizeof(from);
	int timeout = 1000;
	IcmpHeader* icmp_hdr;

	char icmp_data[MAX_PACKET];
	char recvbuf[MAX_PACKET];
	USHORT seq_no = 0;

	ZeroMemory(&dest, sizeof(dest));
	memcpy(&(dest.sin_addr), (void*)param, 4);
	dest.sin_family = AF_INET;
	if (sockRaw != INVALID_SOCKET)
	{
		closesocket(sockRaw);
		sockRaw = INVALID_SOCKET;
	}
	sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockRaw != INVALID_SOCKET)
	{
		iRead = setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
		if (iRead != SOCKET_ERROR)
		{
			timeout = 1000;
			iRead = setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
			if (iRead != SOCKET_ERROR)
			{
				datasize = DEF_PACKET_SIZE + sizeof(IcmpHeader);

				icmp_hdr = (IcmpHeader*)icmp_data;
				icmp_hdr->i_type = ICMP_ECHO;
				icmp_hdr->i_code = 0;
				icmp_hdr->i_id = (USHORT)GetCurrentProcessId();
				icmp_hdr->i_cksum = 0;
				icmp_hdr->i_seq = 0;
				memset(icmp_data + sizeof(IcmpHeader), 'E', datasize - sizeof(IcmpHeader));
				while (1)
				{
					((IcmpHeader*)icmp_data)->i_cksum = 0;
					((IcmpHeader*)icmp_data)->timestamp = GetTickCount();

					((IcmpHeader*)icmp_data)->i_seq = seq_no;
					((IcmpHeader*)icmp_data)->i_cksum = checksum((USHORT*)icmp_data, datasize);

					iWrote = sendto(sockRaw, icmp_data, datasize, 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));
					if (iWrote == SOCKET_ERROR)
					{
						if (WSAGetLastError() == WSAETIMEDOUT)
						{
							dwPingTime = -1;
							continue;
						}
						dwPingState = 0x40000000 | WSAGetLastError();
						continue;
					}
					if (iWrote < datasize)
						dwPingState = 0x04000000;//len error
					while (1)
					{
						iRead = recvfrom(sockRaw, recvbuf, MAX_PACKET, 0, (struct sockaddr*)&from, &fromlen);
						if (iRead == SOCKET_ERROR)
						{
							if (WSAGetLastError() == WSAETIMEDOUT)
							{
								dwPingTime = -1;
								break;
							}
							dwPingState = 0x40000000 | WSAGetLastError();
							continue;
						}
						IpHeader* iphdr;
						unsigned short iphdrlen;
						iphdr = (IpHeader*)recvbuf;
						iphdrlen = iphdr->h_len * 4; // number of 32-bit words *4 = bytes
						if (iRead < iphdrlen + ICMP_MIN)
							dwPingState = 0x08000000;//len error
						icmp_hdr = (IcmpHeader*)(recvbuf + iphdrlen);
						if (icmp_hdr->i_type != ICMP_ECHOREPLY)
						{
							dwPingState = 0x10000000;//non-echo type recvd
							continue;
						}
						if (icmp_hdr->i_seq != seq_no)
							continue;
						if (icmp_hdr->i_id != (USHORT)GetCurrentProcessId())
						{
							dwPingState = 0x20000000;//someone else's packet!
							continue;
						}
						dwPingTime = GetTickCount() - icmp_hdr->timestamp;
						break;
					}
					seq_no++;
					dwPingState = 0x80000000;
					Sleep(3000);
				}
			}
		}
	}
	return 0;
}

#endif
#ifdef _ITEM_CRACKER 
extern bool m_bt;
void Server::lssproto_IC_recv(int fd, int x, int y)
{
	m_bt = true; setCharMind(pc.ptAct, SPR_cracker);
	m_bt = false;
}
#endif

#ifdef _CHECK_GAMESPEED
static int delaytimes = 0;
void Server::lssproto_CS_recv(int fd, int deltimes)
{
	delaytimes = deltimes;
}

int Server::lssproto_getdelaytimes()
{
	if (delaytimes < 0) delaytimes = 0;
	return delaytimes;
}

void Server::lssproto_setdelaytimes(int delays)
{
	delaytimes = delays;
}
#endif

#ifdef _MAGIC_NOCAST//沈默
void Server::lssproto_NC_recv(int fd, int flg)
{
	if (flg == 1)
		NoCastFlag = true;
	else
	{
		setBattleEnd();
		NoCastFlag = false;
	}
}
#endif
#ifdef _CHANNEL_MODIFY
//// 儲存對話內容
//FILE* pSaveChatDataFile[6] = { NULL,NULL,NULL,NULL,NULL,NULL };
//void Server::SaveChatData(char* msg, char KindOfChannel, bool bCloseFile)
//{
//	static char szFileName[256];
//	static struct tm nowTime;
//	static time_t longTime;
//	static unsigned short Channel[] = {
//		PC_ETCFLAG_CHAT_MODE	//隊伍頻道開關
//		,PC_ETCFLAG_CHAT_TELL	//密語頻道開關
//		,PC_ETCFLAG_CHAT_FM		//家族頻道開關
//#ifdef _CHAR_PROFESSION
//		,PC_ETCFLAG_CHAT_OCC	//職業頻道開關
//#endif
//#ifdef _CHATROOMPROTOCOL
//		,PC_ETCFLAG_CHAT_CHAT	//聊天室開關
//#endif
//#ifdef _CHANNEL_WORLD
//		,PC_ETCFLAG_CHAT_WORLD	//世界頻道開關
//#endif
//#ifdef _CHANNEL_ALL_SERV
//		,PC_ETCFLAG_ALL_SERV	//星球頻道開關
//#endif
//	};
//	char ChannelType[] = { 'T','M','F',
//#ifdef _CHAR_PROFESSION
//		'O',
//#endif
//		'R',
//#ifdef _CHANNEL_WORLD
//		'W',
//#endif
//#ifdef _CHANNEL_ALL_SERV
//		'S',
//#endif
//	};
//
//	// 儲存對話內容選項開啟
//	if ((pc.etcFlag & PC_ETCFLAG_CHAT_SAVE) && !bCloseFile)
//	{
//		time(&longTime);
//		localtime_s(&nowTime, &longTime);
//		for (int i = 0; i < 6; i++)
//		{
//			if (pc.etcFlag & Channel[i])
//			{
//				if (pSaveChatDataFile[i] == NULL)
//				{
//					sprintf_s(szFileName, "chat\\%c%02d%02d%02d.TXT", ChannelType[i], (nowTime.tm_year % 100), nowTime.tm_mon + 1, nowTime.tm_mday);
//					if ((pSaveChatDataFile[i] = fopen(szFileName, "a")) == NULL) continue;
//				}
//			}
//			else
//			{
//				if (pSaveChatDataFile[i] != NULL)
//				{
//					fclose(pSaveChatDataFile[i]);
//					pSaveChatDataFile[i] = NULL;
//				}
//			}
//		}
//		for (int i = 0; i < 6; i++)
//		{
//			if (KindOfChannel == ChannelType[i])
//			{
//				if (pSaveChatDataFile[i] != NULL) fprintf(pSaveChatDataFile[i], "[%02d:%02d:%02d]%s\n", nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec, msg);
//			}
//		}
//	}
//	else bCloseFile = 1;
//
//	if (bCloseFile)
//	{
//		for (int i = 0; i < 6; i++)
//			if (pSaveChatDataFile[i] != NULL)
//			{
//				fclose(pSaveChatDataFile[i]);
//				pSaveChatDataFile[i] = NULL;
//			}
//	}
//}
#endif

#ifdef _STREET_VENDOR
extern short sStreetVendorBtn;
extern short sStreetVendorBuyBtn;
extern void Server::StreetVendorWndfunc(bool bReset, char* data);
extern void Server::StreetVendorBuyWndfunc(char* data);

void Server::lssproto_STREET_VENDOR_recv(int fd, char* data)
{
	char szMessage[32];

	getStringToken(data, "|", 1, sizeof(szMessage) - 1, szMessage);
	switch (szMessage[0])
	{
		// 開新擺攤介面
	case 'O':
		sStreetVendorBtn = 1;
		pc.iOnStreetVendor = 1;
		break;
		// 設定擺攤內容
	case 'S':
		sStreetVendorBtn = 3;
		StreetVendorWndfunc(false, data);
		break;
		// server送來的賣方販賣內容
	case 'B':
		sStreetVendorBuyBtn = 1;
		StreetVendorBuyWndfunc(data);
		break;
		// server 送來關閉視窗
	case 'C':
		sStreetVendorBuyBtn = 0;
		break;
		// server 送來的單筆販賣物詳細資料
	case 'D':
		StreetVendorBuyWndfunc(data);
		break;
	}
}
#endif

#ifdef _STONDEBUG_ // 手動送出封包功能 Robin
/*
	(封包編號)`d`(數值資料)`s`(字串資料)`......
	例: 35`d`100`d`100`s`P|Hellp~~`d`1`d`1
*/
void Server::sendDataToServer(char* data)
{
	char token[1024];
	char token2[1024];
	char token3[1024];
	char sendbuf[16384] = "";
	char showbuf[16384] = "";
	char showsubbuf[1024];
	int checksum = 0;
	int datakind;
	int i = 1;

	strcat_s(showbuf, "手動送出 ");

	getStringToken(data, '`', i++, sizeof(token), token);
	if (token[0] == NULL)	return;
	datakind = atoi(token);
	sprintf_s(showsubbuf, "封包=%d ", datakind);
	strcat_s(showbuf, showsubbuf);

	while (1)
	{

		getStringToken(data, '`', i++, sizeof(token2), token2);
		if (token2[0] == NULL)	break;
		getStringToken(data, '`', i++, sizeof(token3), token3);
		if (token3[0] == NULL)	break;

		if (!strcmp(token2, "d"))
		{
			checksum += util_mkint(sendbuf, atoi(token3));
			sprintf_s(showsubbuf, "數=%d ", atoi(token3));
			strcat_s(showbuf, showsubbuf);
		}
		else if (!strcmp(token2, "s"))
		{
			checksum += util_mkstring(sendbuf, token3);
			sprintf_s(showsubbuf, "字=%s ", token3);
			strcat_s(showbuf, showsubbuf);
		}
		else
		{
			break;
		}

	}

	util_mkint(sendbuf, checksum);
	util_SendMesg(sockfd, datakind, sendbuf);

	StockChatBufferLine(showbuf, FONT_PAL_RED);

}
#endif
#ifdef _FAMILYBADGE_
extern int 徽章數據[];
extern int 徽章個數;
int 徽章價格;
void Server::lssproto_FamilyBadge_recv(char* data)
{
	徽章個數 = 0;
	int  i = 2;
	徽章價格 = getIntegerToken(data, "|", 1);
	for (i; i < 201; i++)
	{
		徽章數據[i - 2] = getIntegerToken(data, "|", i);
		if (徽章數據[i - 2] == -1) break;
		徽章個數++;
	}
}
#endif

#ifdef _JOBDAILY
void Server::lssproto_JOBDAILY_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//解讀資料
	int  i = 1, j = 1;
	QString getdata;
	QString perdata;

	//StockChatBufferLine(data,FONT_PAL_RED); 

	memset(jobdaily, 0, sizeof(jobdaily));

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
				jobdaily[i - 1].explain = perdata;
				break;
			case 3:
				jobdaily[i - 1].state = perdata;
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
}
#endif

#ifdef _TEACHER_SYSTEM
void Server::lssproto_TEACHER_SYSTEM_recv(int fd, char* cdata)
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

#ifdef _ADD_STATUS_2
void Server::lssproto_S2_recv(int fd, char* data)
{
	char szMessage[16];
#ifdef _EVIL_KILL
	int ftype = 0, newfame = 0;
#endif

	getStringToken(data, "|", 1, sizeof(szMessage) - 1, szMessage);

#ifdef _NEW_MANOR_LAW
	if (strcmp(szMessage, "FAME") == 0)
	{
		getStringToken(data, "|", 2, sizeof(szMessage) - 1, szMessage);
		pc.fame = atoi(szMessage);
	}
#endif

#ifdef _EVIL_KILL	
	pc.ftype = 0;
	pc.newfame = 0;

	if (getStringToken(data, "|", 3, sizeof(szMessage) - 1, szMessage) == 1) return;
	ftype = atoi(szMessage);
	getStringToken(data, "|", 4, sizeof(szMessage) - 1, szMessage);
	newfame = atoi(szMessage);
	pc.ftype = ftype;
	pc.newfame = newfame;
#endif
}
#endif

#ifdef _ITEM_FIREWORK
void Server::lssproto_Firework_recv(int fd, int nCharaindex, int nType, int nActionNum)
{
	//ACTION* pAct;

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

#ifdef _MOVE_SCREEN
// client 移動熒幕
void Server::lssproto_MoveScreen_recv(int fd, BOOL bMoveScreenMode, int iXY)
{
	pc.bMoveScreenMode = bMoveScreenMode;
	pc.bCanUseMouse = bMoveScreenMode;
	if (bMoveScreenMode)
	{
		pc.iDestX = HIWORD(iXY);
		pc.iDestY = LOWORD(iXY);
	}
	else
		iScreenMoveX = iScreenMoveY = 0;
}
#endif

#ifdef _THEATER
void Server::lssproto_TheaterData_recv(int fd, char* pData)
{
	int				iType, iData;
	char			szMessage[16];
	float			fX, fY;

	getStringToken(pData, "|", 1, sizeof(szMessage) - 1, szMessage);
	iType = atoi(szMessage);
	getStringToken(pData, "|", 2, sizeof(szMessage) - 1, szMessage);
	iData = atoi(szMessage);
	switch (iType)
	{
	case E_THEATER_SEND_DATA_THEATER_MODE:
		pc.iTheaterMode = iData;
		if (iData == 0)
		{
			pc.bCanUseMouse = FALSE;		// 表演完畢,可以正常使用滑鼠移動
			pc.iSceneryNumber = -1;
		}
		else
		{
			pc.bCanUseMouse = TRUE;	// 表演中
			pc.iSceneryNumber = 26558;
		}
		break;
	case E_DATA_TYPE_MOVE:		// 移動
		camMapToGamen((float)(HIWORD(iData) * GRID_SIZE), float(LOWORD(iData) * GRID_SIZE), &fX, &fY);
		MouseNowPoint((int)(fX + 0.5f), (int)(fY + 0.5f));
		MouseCrickLeftDownPoint((int)(fX + 0.5f), (int)(fY + 0.5f));
		MouseCrickLeftUpPoint((int)(fX + 0.5f), (int)(fY + 0.5f));
		pc.bCanUseMouse = FALSE;			// 設為 FALSE,不然人物不能移動
		mouse.level = DISP_PRIO_TILE;
		closeCharActionAnimeChange();
		break;
	case E_DATA_TYPE_DIR:		// 方向
		setPcDir(iData);
		szMessage[0] = cnvServDir(iData, 1);
		szMessage[1] = '\0';
		walkSendForServer(nowGx, nowGy, szMessage);
		break;
	case E_DATA_TYPE_SCENERY:	// 布景
		pc.iSceneryNumber = iData;
		break;
	case E_DATA_TYPE_BGM:		// 背景音樂
		play_bgm(iData);
		break;
	case E_THEATER_SEND_DATA_DISPLAY_SCORE:		// 顯示分數
		pc.iTheaterMode |= 0x00000004;
		pc.iTheaterMode |= iData << 16;			// iData 是分數值
		break;
	case E_DATA_TYPE_NPC:						// 產生或是消失或更改臨時NPC
		// 當 iType 為 E_DATA_TYPE_NPC 時取出來的 iData 是 NPC 編號
		if (iData >= 0 && iData < 5)
		{
			int iSprNum, iGX, iGY, iAction, iDir;

			getStringToken(pData, "|", 3, sizeof(szMessage) - 1, szMessage);	// 取出指令
			if (atoi(szMessage) == 1)
			{
				getStringToken(pData, "|", 4, sizeof(szMessage) - 1, szMessage);	// 取出圖號
				iSprNum = atoi(szMessage);
				getStringToken(pData, "|", 5, sizeof(szMessage) - 1, szMessage);	// 取出座標
				iGX = atoi(szMessage);
				getStringToken(pData, "|", 6, sizeof(szMessage) - 1, szMessage);	// 取出座標
				iGY = atoi(szMessage);
				getStringToken(pData, "|", 7, sizeof(szMessage) - 1, szMessage);	// 取出動作
				iAction = atoi(szMessage);
				getStringToken(pData, "|", 8, sizeof(szMessage) - 1, szMessage);	// 取出方向
				iDir = atoi(szMessage);
				camMapToGamen((float)iGX * GRID_SIZE, (float)iGY * GRID_SIZE, &fX, &fY);
				if (pc.pActNPC[iData] == NULL)
				{
					pc.pActNPC[iData] = MakeAnimDisp((int)fX, (int)fY, iSprNum, ANIM_DISP_THEATER_NPC);
					ATR_DISP_PRIO(pc.pActNPC[iData]) = DISP_PRIO_CHAR - 1;
				}
				if (pc.iTheaterMode & 0x00000001)
				{
					iGX -= iScreenMoveX;
					iGY -= iScreenMoveY;
				}
				pc.pActNPC[iData]->gx = iGX;
				pc.pActNPC[iData]->gy = iGY;
				pc.pActNPC[iData]->mx = (float)(iGX * GRID_SIZE);
				pc.pActNPC[iData]->my = (float)(iGY * GRID_SIZE);
				ATR_CHR_NO(pc.pActNPC[iData]) = iSprNum;
				ATR_CHR_ACT(pc.pActNPC[iData]) = iAction;
				ATR_CHR_ANG(pc.pActNPC[iData]) = iDir;
			}
			else
			{
				if (pc.pActNPC[iData])
				{
					DeathAction(pc.pActNPC[iData]);
					pc.pActNPC[iData] = NULL;
				}
			}
		}
		break;
	}
}
#endif

#ifdef _NPC_MAGICCARD

bool bShowflag[20];
int iShowdamage[20];
int iPosition[20];
int iOffsetY[20];
int ioffsetsx = 0;
int ioffsetsy = 0;
unsigned int inextexet = 0;

void Server::lssproto_MagiccardDamage_recv(int fd, int position, int damage, int offsetx, int offsety)
{
	int i;

	if (position == 10)
	{
		wnCloseFlag = 1;
		return;
	}

	for (i = 0; i < 20; i++)
	{
		if (bShowflag[i] == TRUE) continue;
		iPosition[i] = position;
		iShowdamage[i] = damage;
		bShowflag[i] = TRUE;
		break;
	}
	ioffsetsx = offsetx;
	ioffsetsy = offsety;
}

void Server::lssproto_MagiccardAction_recv(int fd, char* data)
{
	ACTION* pAct;
	int  charaindex, player, card, dir, actionNum, action, offsetx, offsety;
	char token[2048];

	getStringToken(data, "|", 1, token);
	charaindex = atoi(token);
	getStringToken(data, "|", 2, token);
	player = atoi(token);
	getStringToken(data, "|", 3, token);
	card = atoi(token);
	getStringToken(data, "|", 4, token);
	dir = (atoi(token) + 3) % 8;
	getStringToken(data, "|", 5, token);
	actionNum = atoi(token);	//圖號
	getStringToken(data, "|", 6, token);
	action = atoi(token);
	getStringToken(data, "|", 7, token);
	offsetx = atoi(token);
	getStringToken(data, "|", 8, token);
	offsety = atoi(token);

	//if ( actionNum == 101652 ){
	//	dir = 1;	
	//	dir = 2;
	//}
	if (pc.id == charaindex)
		//changePcAct(0, 0, 0, 51, nType, nActionNum, 0);
		changePcAct(offsetx, offsety, dir, 60, player, actionNum, action);
	else
	{
		pAct = getCharObjAct(charaindex);
		//changeCharAct( ptAct, x, y, dir, act, effectno, effectparam1, effectparam2 );	
		changeCharAct(pAct, offsetx, offsety, dir, 60, player, actionNum, action);
	}
}
#endif

#ifdef _NPC_DANCE
void Server::lssproto_DancemanOption_recv(int fd, int option)
{
	switch (option)
	{
	case 0:	//關閉視窗
		wnCloseFlag = 1;
		break;
	case 1: //開啟動一動模式
		pc.iDanceMode = 1;
		break;
	case 2: //關閉動一動模式
		pc.iDanceMode = 0;
		break;
	}
}
#endif

#ifdef _HUNDRED_KILL
void Server::lssproto_hundredkill_recv(int fd, int flag)
{

	if (flag == 1)
		BattleHundredFlag = TRUE;
	else
		BattleHundredFlag = FALSE;
}
#endif

#ifdef _ANNOUNCEMENT_
void Server::lssproto_DENGON_recv(char* cdata, int colors, int nums)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	//extern int 公告數量;
	//extern char 公告內容[512];
	//extern int 公告顏色;
	//extern int 公告時間;
	//公告時間 = 0;
	//sprintf(公告內容, "%s", data);
	//公告顏色 = colors;
	//公告數量 = nums;
}
#endif

#ifdef _PET_SKINS
void Server::lssproto_PetSkins_recv(char* data)
{
	char* str = "寵物欄位置|當前使用皮膚圖號|總皮膚數|皮膚圖號|說明|皮膚圖號|說明|...";
}
#endif

void Server::lssproto_TK_recv(int fd, int index, char* cmessage, int color)
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
	int fontsize = 0;
#ifdef _MESSAGE_FRONT_
	msg1[0] = 0xA1;
	msg1[1] = 0xF4;
	msg1[2] = 0;
	msg = msg1 + 2;
#endif
	QString message = util::toUnicode(cmessage);
	if (message.isEmpty())
		return;
	getStringToken(message, "|", 1, id);

	if (id.startsWith("P"))
	{
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
					QString temp = tr("Tell you:");
					int found;

					if (found = msg.indexOf(temp))
					{
						tellName = msg.left(found);
						color = 5;
						szMsgBuf = msg.mid(found + temp.length());
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
				pc.status |= CHR_STATUS_FUKIDASHI;
			}
			else
			{
				//ptAct = getCharObjAct(index);
				//if (ptAct != NULL)
				//{
				//	// 1000
				//	setCharFukidashi(ptAct, 1000);
				//}
			}
		}
	}
	return;
}

static unsigned char BitTable[] = {
		0x00 , 0x80 , 0x40 , 0xC0 , 0x20 , 0xA0 , 0x60 , 0xE0 ,
		0x10 , 0x90 , 0x50 , 0xD0 , 0x30 , 0xB0 , 0x70 , 0xF0 ,
		0x08 , 0x88 , 0x48 , 0xC8 , 0x28 , 0xA8 , 0x68 , 0xE8 ,
		0x18 , 0x98 , 0x58 , 0xD8 , 0x38 , 0xB8 , 0x78 , 0xF8 ,
		0x04 , 0x84 , 0x44 , 0xC4 , 0x24 , 0xA4 , 0x64 , 0xE4 ,
		0x14 , 0x94 , 0x54 , 0xD4 , 0x34 , 0xB4 , 0x74 , 0xF4 ,
		0x0C , 0x8C , 0x4C , 0xCC , 0x2C , 0xAC , 0x6C , 0xEC ,
		0x1C , 0x9C , 0x5C , 0xDC , 0x3C , 0xBC , 0x7C , 0xFC ,
		0x02 , 0x82 , 0x42 , 0xC2 , 0x22 , 0xA2 , 0x62 , 0xE2 ,
		0x12 , 0x92 , 0x52 , 0xD2 , 0x32 , 0xB2 , 0x72 , 0xF2 ,
		0x0A , 0x8A , 0x4A , 0xCA , 0x2A , 0xAA , 0x6A , 0xEA ,
		0x1A , 0x9A , 0x5A , 0xDA , 0x3A , 0xBA , 0x7A , 0xFA ,
		0x06 , 0x86 , 0x46 , 0xC6 , 0x26 , 0xA6 , 0x66 , 0xE6 ,
		0x16 , 0x96 , 0x56 , 0xD6 , 0x36 , 0xB6 , 0x76 , 0xF6 ,
		0x0E , 0x8E , 0x4E , 0xCE , 0x2E , 0xAE , 0x6E , 0xEE ,
		0x1E , 0x9E , 0x5E , 0xDE , 0x3E , 0xBE , 0x7E , 0xFE ,
		0x01 , 0x81 , 0x41 , 0xC1 , 0x21 , 0xA1 , 0x61 , 0xE1 ,
		0x11 , 0x91 , 0x51 , 0xD1 , 0x31 , 0xB1 , 0x71 , 0xF1 ,
		0x09 , 0x89 , 0x49 , 0xC9 , 0x29 , 0xA9 , 0x69 , 0xE9 ,
		0x19 , 0x99 , 0x59 , 0xD9 , 0x39 , 0xB9 , 0x79 , 0xF9 ,
		0x05 , 0x85 , 0x45 , 0xC5 , 0x25 , 0xA5 , 0x65 , 0xE5 ,
		0x15 , 0x95 , 0x55 , 0xD5 , 0x35 , 0xB5 , 0x75 , 0xF5 ,
		0x0D , 0x8D , 0x4D , 0xCD , 0x2D , 0xAD , 0x6D , 0xED ,
		0x1D , 0x9D , 0x5D , 0xDD , 0x3D , 0xBD , 0x7D , 0xFD ,
		0x03 , 0x83 , 0x43 , 0xC3 , 0x23 , 0xA3 , 0x63 , 0xE3 ,
		0x13 , 0x93 , 0x53 , 0xD3 , 0x33 , 0xB3 , 0x73 , 0xF3 ,
		0x0B , 0x8B , 0x4B , 0xCB , 0x2B , 0xAB , 0x6B , 0xEB ,
		0x1B , 0x9B , 0x5B , 0xDB , 0x3B , 0xBB , 0x7B , 0xFB ,
		0x07 , 0x87 , 0x47 , 0xC7 , 0x27 , 0xA7 , 0x67 , 0xE7 ,
		0x17 , 0x97 , 0x57 , 0xD7 , 0x37 , 0xB7 , 0x77 , 0xF7 ,
		0x0F , 0x8F , 0x4F , 0xCF , 0x2F , 0xAF , 0x6F , 0xEF ,
		0x1F , 0x9F , 0x5F , 0xDF , 0x3F , 0xBF , 0x7F , 0xFF
};
static unsigned short crctab16[] = {
		0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
		0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
		0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
		0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
		0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
		0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
		0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
		0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
		0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
		0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
		0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
		0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
		0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
		0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
		0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
		0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
		0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
		0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
		0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
		0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
		0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
		0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
		0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
		0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
		0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
		0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
		0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
		0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
		0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
		0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
		0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
		0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0,
};
unsigned short CheckCRC(unsigned char* p, int size)
{
	unsigned short	crc = 0;
	int		i;

	for (i = 0; i < size; i++)
	{
		if (((crc >> 8) & 0xFF) < 256)
			crc = (crctab16[(crc >> 8) & 0xFF] ^ (crc << 8) ^ BitTable[p[i]]);
	}
	return crc;
}

bool readMap(int floor, int x1, int y1, int x2, int y2, unsigned short* tile, unsigned short* parts, unsigned short* event)
{
	//	FILE* fp;
	//	char filename[255];
	//#ifdef _FIX_DEL_MAP           // WON 修正玩家抽地圖
	//	char list[10];
	//#endif
	//	int fWidth, fHeight, fOffset, mWidth, width, height, fx, fy, mx, my, len, len2, i;
	//	//printf("floor=%d x1=%d y1=%d x2=%d y2=%d\n",floor,x1,y1,x2,y2);
	//
	//	sprintf_s(filename, "map\\%d.dat", floor);
	//
	//	if ((fopen_s(&fp, filename, "rb")) != NULL)
	//	{
	//#ifdef _FIX_DEL_MAP	//andy_add
	//		memset(tile, 193, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//		//memset(parts, 193, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//#else
	//		std::ignore = _mkdir("map");
	//
	//		if (fopen_s(&fp, filename, "wb") == NULL)
	//		{
	//			if (fp)
	//				fclose(fp);
	//		}
	//#endif
	//
	//		if ((fopen_s(&fp, filename, "rb")) != NULL)
	//			return FALSE;
	//	}
	//
	//#ifdef _FIX_DEL_MAP           // WON 修正玩家抽地圖    
	//	fseek(fp, 0, SEEK_SET);
	//	if (!fread(list, sizeof(char), 1, fp))
	//	{   // 玩家抽掉地圖送監獄
	////andy_add
	//		memset(tile, 193, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//		//memset(parts, 193, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//		fclose(fp);
	//		//lssproto_DM_send(sockfd);
	//		return TRUE;
	//	}
	//#endif
	//	//printf("MAP_X_SIZE=%d MAP_Y_SIZE=%d\n",MAP_X_SIZE,MAP_Y_SIZE);
	//	memset(tile, 0, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//	memset(parts, 0, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//	memset(event, 0, MAP_X_SIZE * MAP_Y_SIZE * sizeof(short));
	//	fseek(fp, 0, SEEK_SET);
	//	fread(&fWidth, sizeof(int), 1, fp);
	//	fread(&fHeight, sizeof(int), 1, fp);
	//	//printf("fWidth=%d fHeight=%d\n",fWidth,fHeight);
	//	mWidth = x2 - x1;
	//	width = mWidth;
	//	height = y2 - y1;
	//	mx = 0;
	//	fx = x1;
	//
	//	//printf("mWidth=%d width=%d  height=%d  fx=%d\n",mWidth,width,height,fx);
	//	if (x1 < 0)
	//	{
	//		width += x1;
	//		fx = 0;
	//		mx -= x1;
	//	}
	//	if (x2 > fWidth)
	//		width -= (x2 - fWidth);
	//	my = 0;
	//	fy = y1;
	//	if (y1 < 0)
	//	{
	//		height += y1;
	//		fy = 0;
	//		my -= y1;
	//	}
	//	if (y2 > fHeight)
	//		height -= (y2 - fHeight);
	//
	//	fOffset = sizeof(int) * 2;
	//	len = fy * fWidth + fx;
	//	len2 = my * mWidth + mx;
	//	for (i = 0; i < height; i++)
	//	{
	//		fseek(fp, sizeof(short) * len + fOffset, SEEK_SET);
	//		fread(&tile[len2], sizeof(short) * width, 1, fp);
	//		len += fWidth;
	//		len2 += mWidth;
	//	}
	//	fOffset += sizeof(short) * (fWidth * fHeight);
	//	len = fy * fWidth + fx;
	//	len2 = my * mWidth + mx;
	//	for (i = 0; i < height; i++)
	//	{
	//		fseek(fp, sizeof(short) * len + fOffset, SEEK_SET);
	//		fread(&parts[len2], sizeof(short) * width, 1, fp);//aaaaaaaaaaaaa
	//		len += fWidth;
	//		len2 += mWidth;
	//	}
	//	fOffset += sizeof(short) * (fWidth * fHeight);
	//	len = fy * fWidth + fx;
	//	len2 = my * mWidth + mx;
	//	for (i = 0; i < height; i++)
	//	{
	//		fseek(fp, sizeof(short) * len + fOffset, SEEK_SET);
	//		fread(&event[len2], sizeof(short) * width, 1, fp);
	//		len += fWidth;
	//		len2 += mWidth;
	//	}
	//	fclose(fp);

	return true;
}

bool Server::mapCheckSum(int floor, int x1, int y1, int x2, int y2, int tileSum, int partsSum, int eventSum)
{
	int tilesum = 0, objsum = 0, eventsum = 0, databufferindex = 0, width = x2 - x1, height = y2 - y1, i, j;
	unsigned short tile[MAP_X_SIZE * MAP_Y_SIZE];
	unsigned short parts[MAP_X_SIZE * MAP_Y_SIZE];
	unsigned short event[MAP_X_SIZE * MAP_Y_SIZE];


	readMap(floor, x1, y1, x2, y2, tile, parts, event);
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
			event[i * width + j] &= 0x0fff;
	}

	tilesum = CheckCRC((unsigned char*)tile, 27 * 27 * sizeof(short));
	objsum = CheckCRC((unsigned char*)parts, 27 * 27 * sizeof(short));
	eventsum = CheckCRC((unsigned char*)event, 27 * 27 * sizeof(short));

	if (tileSum == tilesum && partsSum == objsum && eventSum == eventsum)
	{
		redrawMap();
		return true;
	}
	else
	{

		//lssproto_M_send(sockfd, floor, x1, y1, x2, y2);

		return false;
	}
}

void Server::lssproto_MC_recv(int fd, int fl, int x1, int y1, int x2, int y2, int tileSum, int partsSum, int eventSum, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString showString, floorName;

#ifdef DEBUGPUSH
	QString msg;
	sprintf_s(msg, "FL%d %d,%d-%d,%d (%ud/%ud)", fl, x1, y1, x2, y2, tileSum, partsSum);
	PUSH(msg);
#endif
	getStringToken(data, "|", 1, showString);
	makeStringFromEscaped(showString);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	Injector& injector = Injector::getInstance();
	//BB414 418118C 4181D3C

	if (nowFloor == fl)
	{
		QString strPal;

		getStringToken(showString, "|", 1, floorName);
		if (floorName.contains("\\z1"))
			floorName.remove("\\z1");
		nowFloorName = floorName;
		palNo = -2;
		getStringToken(showString, "|", 2, strPal);
		if (strPal.isEmpty())
		{
			if (!TimeZonePalChangeFlag || IS_ONLINE_FLAG)
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
				if (TimeZonePalChangeFlag || IS_ONLINE_FLAG)
				{
					palNo = pal;
					palTime = 0;
					drawTimeAnimeFlag = 0;
				}
			}
			else
			{
				if (!TimeZonePalChangeFlag || IS_ONLINE_FLAG)
				{
					palNo = -1;
					palTime = 0;
					drawTimeAnimeFlag = 1;
				}
			}
		}
	}

	if (mapCheckSum(fl, x1, y1, x2, y2, tileSum, partsSum, eventSum))
	{
		if (nowFloor == fl)
		{
			floorChangeFlag = false;
			if (warpEffectStart)
				warpEffectOk = true;
		}
	}

	emit signalDispatcher.updateMapLabelTextChanged(QString("%1(%2)").arg(nowFloorName).arg(nowFloor));
}

void Server::setEventMemory(int x, int y, unsigned short ev)
{
	if (((y - mapAreaY1) * mapAreaWidth + (x - mapAreaX1)) < 1369)
		event_[(y - mapAreaY1) * mapAreaWidth + (x - mapAreaX1)] = ev;
}

bool Server::writeMap(int floor, int x1, int y1, int x2, int y2, unsigned short* tile, unsigned short* parts, unsigned short* event)
{
	//FILE* fp;
	//char filename[255];
	//int fWidth, fHeight, fOffset, mWidth, width, height, fx, fy, mx, my, len, len2, i, j;

	//sprintf_s(filename, "map\\%d.dat", floor);

	//if ((fopen_s(&fp, filename, "rb+")) != NULL)
	//{
	//	std::ignore = _mkdir("map");
	//	if ((fopen_s(&fp, filename, "rb+")) != NULL)
	//		return FALSE;
	//}
	//fseek(fp, 0, SEEK_SET);
	//fread(&fWidth, sizeof(int), 1, fp);
	//fread(&fHeight, sizeof(int), 1, fp);
	//mWidth = x2 - x1;
	//width = mWidth;
	//height = y2 - y1;
	//mx = 0;
	//fx = x1;
	//if (x1 < 0)
	//{
	//	width += x1;
	//	fx = 0;
	//	mx -= x1;
	//}
	//if (x2 > fWidth)
	//	width -= (x2 - fWidth);
	//my = 0;
	//fy = y1;
	//if (y1 < 0)
	//{
	//	height += y1;
	//	fy = 0;
	//	my -= y1;
	//}
	//if (y2 > fHeight)
	//	height -= (y2 - fHeight);
	//fOffset = sizeof(int) * 2;
	//len = fy * fWidth + fx;
	//len2 = my * mWidth + mx;
	//for (i = 0; i < height; i++)
	//{
	//	fseek(fp, sizeof(short) * len + fOffset, SEEK_SET);
	//	fwrite(&tile[len2], sizeof(short) * width, 1, fp);
	//	len += fWidth;
	//	len2 += mWidth;
	//}
	//fOffset += sizeof(short) * (fWidth * fHeight);
	//len = fy * fWidth + fx;
	//len2 = my * mWidth + mx;
	//for (i = 0; i < height; i++)
	//{
	//	fseek(fp, sizeof(short) * len + fOffset, SEEK_SET);
	//	fwrite(&parts[len2], sizeof(short) * width, 1, fp);
	//	len += fWidth;
	//	len2 += mWidth;
	//}
	//fOffset += sizeof(short) * (fWidth * fHeight);
	//len = fy * fWidth + fx;
	//len2 = my * mWidth + mx;
	//for (i = 0; i < height; i++)
	//{

	//	for (j = 0; j < width; j++)
	//	{
	//		event[len2 + j] |= (MAP_SEE_FLAG | MAP_READ_FLAG);
	//		if (nowFloor == floor && (mapAreaX1 <= x1 + j && x1 + j < mapAreaX2 && mapAreaY1 <= y1 + i && y1 + i < mapAreaY2))

	//			setEventMemory(x1 + j, y1 + i, event[len2 + j]);
	//	}
	//	fseek(fp, sizeof(short) * len + fOffset, SEEK_SET);
	//	fwrite(&event[len2], sizeof(short) * width, 1, fp);
	//	len += fWidth;
	//	len2 += mWidth;
	//}
	//fclose(fp);

	return true;
}

void Server::lssproto_M_recv(int fd, int fl, int x1, int y1, int x2, int y2, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString showString, floorName, tilestring, partsstring, eventstring, tmp;
	unsigned short tile[2048] = {}, parts[2048] = {}, event[2048] = {};
	int i, flag;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

#ifdef DEBUGPUSH
	char msg[800];
	sprintf_s(msg, "FL%d %d,%d-%d,%d", fl, x1, y1, x2, y2);
	PUSH(msg);
#endif
	getStringToken(data, "|", 1, showString);
	makeStringFromEscaped(showString);
	nowFloor = fl;
	if (nowFloor == fl)
	{
		QString strPal;

		getStringToken(showString, "|", 1, floorName);
		if (floorName.contains("\\z1"))
			floorName.remove("\\z1");
		nowFloorName = floorName;
		palNo = -2;
		getStringToken(showString, "|", 2, strPal);
		if (strPal.isEmpty())
		{
			if (!TimeZonePalChangeFlag || IS_ONLINE_FLAG)
			{
				palNo = -1;
				palTime = 0;
				drawTimeAnimeFlag = 1;
			}
		}
		else
		{
			int pal;

			pal = strPal.toInt();
			if (pal >= 0)
			{
				if (TimeZonePalChangeFlag || IS_ONLINE_FLAG)
				{
					palNo = pal;
					palTime = 0;
					drawTimeAnimeFlag = 0;
				}
			}
			else
			{
				if (!TimeZonePalChangeFlag || IS_ONLINE_FLAG)
				{
					palNo = -1;
					palTime = 0;
					drawTimeAnimeFlag = 1;
				}
			}
		}
	}
	getStringToken(data, "|", 2, tilestring);
	getStringToken(data, "|", 3, partsstring);
	getStringToken(data, "|", 4, eventstring);
	for (i = 0; ; i++)
	{
		flag = getStringToken(tilestring, ",", i + 1, tmp);
		tile[i] = a62toi(tmp);
		getStringToken(partsstring, ",", i + 1, tmp);
		parts[i] = a62toi(tmp);
		getStringToken(eventstring, ",", i + 1, tmp);
		event[i] = a62toi(tmp);
		if (flag == 1)
			break;
	}
	writeMap(fl, x1, y1, x2, y2, tile, parts, event);
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

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
//    #ifdef _GM_IDENTIFY		// Rog ADD GM識別
//  void setPcParam(char *name, char *freeName, int level, char *petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_exp, int profession_skill_point , char *gm_name)
//    void setPcParam(char *name, char *freeName, int level, char *petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_skill_point , char *gm_name)    
//	#else
//	void setPcParam(char *name, char *freeName, int level, char *petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_exp, int profession_skill_point)
#ifdef _ALLDOMAN // (不可開) Syu ADD 排行榜NPC
void Server::setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_skill_point, int herofloor)
#else
void Server::setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height, int profession_class, int profession_level, int profession_skill_point)
#endif
// 	#endif
#else
void Server::setPcParam(const QString& name, const QString& freeName, int level, const QString& petname, int petlevel, int nameColor, int walk, int height)
#endif
{
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
	int gmnameLen;
#endif
	pc.name = name;

	pc.freeName = freeName;

	pc.level = level;

	pc.ridePetName = petname;

	pc.ridePetLevel = petlevel;

	pc.nameColor = nameColor;
	if (walk != 0)
	{
		pc.status |= CHR_STATUS_W;
	}
	if (height != 0)
	{
		pc.status |= CHR_STATUS_H;
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
}

/*===========================
1 OBJTYPE_CHARA
2 OBJTYPE_ITEM
3 OBJTYPE_GOLD
4 NPC&other player
===========================*/
void Server::lssproto_C_recv(int fd, char* cdata)
{
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

	IS_ONLINE_FLAG = true;
	for (i = 0; ; i++)
	{
		getStringToken(data, ",", i + 1, bigtoken);
		if (bigtoken.isEmpty())
			break;
#ifdef _OBJSEND_C
		getStringToken(bigtoken, "|", 1, smalltoken);
		if (strlen(smalltoken) == 0)
			return;
		switch (smalltoken.toInt())
		{
		case 1://OBJTYPE_CHARA
			charType = getIntegerToken(bigtoken, "|", 2);
			getStringToken(bigtoken, "|", 3, smalltoken);
			id = a62toi(smalltoken);

			extern BOOL 人物屏蔽開關;
			if (人物屏蔽開關)
			{
				if (id != pc.id)
				{
					if (charType < 4)
						continue;
				}
			}


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
			getStringToken(bigtoken, "|", 11, sizeof(freeName) - 1, freeName);
			makeStringFromEscaped(freeName);
			getStringToken(bigtoken, "|", 12, smalltoken);
			walkable = smalltoken.toInt();
			getStringToken(bigtoken, "|", 13, smalltoken);
			height = smalltoken.toInt();
			charNameColor = getIntegerToken(bigtoken, "|", 14);
			getStringToken(bigtoken, "|", 15, sizeof(fmname) - 1, fmname);
			makeStringFromEscaped(fmname);
			getStringToken(bigtoken, "|", 16, sizeof(petname) - 1, petname);
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
				if (pc.ptAct == NULL)
				{
					createPc(graNo, x, y, dir);
					updataPcAct();
				}
				else
					setPcGraNo(graNo, pc.dir);

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
				setPcNameColor(charNameColor);
				if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
				{
					party[0].level = pc.level;
					strcpy(party[0].name, pc.name);
				}
#ifdef MAX_AIRPLANENUM
				for (j = 0; j < MAX_AIRPLANENUM; j++)
#else
				for (j = 0; j < MAX_PARTY; j++)
#endif
				{
					if (party[j].useFlag != 0 && party[j].id == id)
					{
						party[j].ptAct = pc.ptAct;
						setPcParty();
						if (j == 0)
							setPcLeader();
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
				setNpcCharObj(id, graNo, x, y, dir, fmname, name, freeName,
					level, petname, petlevel, nameColor, walkable, height, charType, profession_class);
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
				ptAct = getCharObjAct(id);
#ifdef _NPC_EVENT_NOTICE
				//	noticeNo=120137;
				if (charType == 13 && noticeNo > 0)
				{
					setNpcNotice(ptAct, noticeNo);
				}
#endif
				if (ptAct != NULL)
				{
#ifdef MAX_AIRPLANENUM
					for (j = 0; j < MAX_AIRPLANENUM; j++)
#else
					for (j = 0; j < MAX_PARTY; j++)
#endif
					{
						if (party[j].useFlag != 0 && party[j].id == id)
						{
							party[j].ptAct = ptAct;
							setCharParty(ptAct);
							if (j == 0)
								setCharLeader(ptAct);
							break;
						}
					}
					setCharNameColor(ptAct, charNameColor);
				}
			}
			break;
		case 2://OBJTYPE_ITEM
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toInt();
			getStringToken(bigtoken, "|", 5, smalltoken);
			graNo = smalltoken.toInt();
			classNo = getIntegerToken(bigtoken, "|", 6);
			getStringToken(bigtoken, "|", 7, sizeof(info) - 1, info);
			makeStringFromEscaped(info);
			setItemCharObj(id, graNo, x, y, 0, classNo, info);
			break;
		case 3://OBJTYPE_GOLD
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 4, smalltoken);
			y = smalltoken.toInt();
			getStringToken(bigtoken, "|", 5, smalltoken);
			money = smalltoken.toInt();
			sprintf_s(info, "%d Stone", money);
			if (money > 10000)
				setMoneyCharObj(id, 24050, x, y, 0, money, info);
			else if (money > 1000)
				setMoneyCharObj(id, 24051, x, y, 0, money, info);
			else
				setMoneyCharObj(id, 24052, x, y, 0, money, info);
			break;
		case 4:
			getStringToken(bigtoken, "|", 2, smalltoken);
			id = a62toi(smalltoken);
			getStringToken(bigtoken, "|", 3, name);
			makeStringFromEscaped(name);
			getStringToken(bigtoken, "|", 4, smalltoken);
			dir = (smalltoken.toInt() + 3) % 8;
			getStringToken(bigtoken, "|", 5, smalltoken);
			graNo = smalltoken.toInt();
			getStringToken(bigtoken, "|", 6, smalltoken);
			x = smalltoken.toInt();
			getStringToken(bigtoken, "|", 7, smalltoken);
			y = smalltoken.toInt();

#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
#ifdef _GM_IDENTIFY		// Rog ADD GM識別
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0, "");
#else
#ifdef _NPC_PICTURE
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0, 0);
#else
			setNpcCharObj(id, graNo, x, y, dir, "", name, "",
				level, petname, petlevel, nameColor, 0, height, 2, 0);
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
			ptAct = getCharObjAct(id);
			break;
		}
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
				for (j = 0; j < MAX_PARTY; j++)
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
				//	for (j = 0; j < MAX_PARTY; j++)
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
	}

}

void Server::lssproto_CA_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	QString bigtoken;
	QString smalltoken;
	//int alreadytellC[1024];
	int tellCindex = 0;
	//int tellflag;
	int i;//, j;
	int charindex;
	int x;
	int y;
	int act;
	int dir;
	int effectno = 0, effectparam1 = 0, effectparam2 = 0;
#ifdef _STREET_VENDOR
	char szStreetVendorTitle[32];
#endif
	//ACTION* ptAct;


	for (i = 0; ; i++)
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
#ifdef _STREET_VENDOR
		if (act == 41) strncpy_s(szStreetVendorTitle, sizeof(szStreetVendorTitle), smalltoken, sizeof(szStreetVendorTitle));
		else
#endif
		{
			effectno = smalltoken.toInt();
			effectparam1 = getIntegerToken(bigtoken, "|", 7);
			effectparam2 = getIntegerToken(bigtoken, "|", 8);
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
		//	for (j = 0; j < tellCindex; j++)
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

void Server::lssproto_CD_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	int i, j;
	int id;

	for (i = 1; ; i++)
	{
		//		id = getInteger62Token(data, "|", i );
		id = getInteger62Token(data, ",", i);
		if (id == -1)
			break;

		//delCharObj(id);

#ifdef MAX_AIRPLANENUM
		for (j = 0; j < MAX_AIRPLANENUM; j++)
#else
		for (j = 0; j < MAX_PARTY; j++)
#endif
		{
			if (party[j].useFlag != 0 && party[j].id == id)
			{
				//party[j].ptAct = NULL;
				break;
			}
		}
	}
}

static long era = 912766409L + 5400L;
void Server::RealTimeToSATime(LSTIME* lstime)
{

	long lsseconds;
	long lsdays;

	//cary 十五
	lsseconds = (TimeGetTime() - FirstTime) / 1000L + serverTime - era;

	lstime->year = (int)(lsseconds / (LSTIME_SECONDS_PER_DAY * LSTIME_DAYS_PER_YEAR));

	lsdays = lsseconds / LSTIME_SECONDS_PER_DAY;
	lstime->day = lsdays % LSTIME_DAYS_PER_YEAR;


	/*(750*12)*/
	lstime->hour = (int)(lsseconds % LSTIME_SECONDS_PER_DAY)

		* LSTIME_HOURS_PER_DAY / LSTIME_SECONDS_PER_DAY;

	return;
}

LSTIME_SECTION getLSTime(LSTIME* lstime)
{
	if (NIGHT_TO_MORNING < lstime->hour
		&& lstime->hour <= MORNING_TO_NOON)
		return LS_MORNING;
	else if (NOON_TO_EVENING < lstime->hour
		&& lstime->hour <= EVENING_TO_NIGHT)
		return LS_EVENING;
	else if (EVENING_TO_NIGHT < lstime->hour
		&& lstime->hour <= NIGHT_TO_MORNING)
		return LS_NIGHT;
	else
		return LS_NOON;
}

void Server::PaletteChange(int palNo, int time)
{
	PalState.palNo = palNo;

	PalState.time = time;

	if (PalState.time <= 0)
		PalState.time = 1;
}

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

void Server::lssproto_S_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString first = data.left(1);
	data = data.mid(1);
	if (first.isEmpty())
		return;

	IS_ONLINE_FLAG = true;

	if (first == "C")//C warp 用
	{
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
		setMap(fl, gx, gy);
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
	else if (first == "D")// D 修正時間
	{
		pc.id = getIntegerToken(data, "|", 1);
		serverTime = getIntegerToken(data, "|", 2);
		FirstTime = TimeGetTime();
		RealTimeToSATime(&SaTime);
		SaTimeZoneNo = getLSTime(&SaTime);
		PaletteChange(SaTimeZoneNo, 0);
		//andy_add
	}
	else if (first == "X")// X 騎寵
	{
		pc.lowsride = getIntegerToken(data, "|", 2);
	}
	else if (first == "P")// P 人物狀態
	{
		QString name, freeName;
		int i, kubun;
		unsigned int mask;

		kubun = getInteger62Token(data, "|", 1);
		//if (!bNewServer)
		//	pc.ridePetNo = -1;
		if (kubun == 1)
		{
			pc.hp = getIntegerToken(data, "|", 2);		// 0x00000002
			pc.maxHp = getIntegerToken(data, "|", 3);		// 0x00000004
			pc.mp = getIntegerToken(data, "|", 4);		// 0x00000008
			pc.maxMp = getIntegerToken(data, "|", 5);		// 0x00000010
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
				if (kubun & mask)
				{
					if (mask == 0x00000002) // ( 1 << 1 )
					{
						pc.hp = getIntegerToken(data, "|", i);// 0x00000002
						i++;
					}
					else if (mask == 0x00000004) // ( 1 << 2 )
					{
						pc.maxHp = getIntegerToken(data, "|", i);// 0x00000004
						i++;
					}
					else if (mask == 0x00000008)
					{
						pc.mp = getIntegerToken(data, "|", i);// 0x00000008
						i++;
					}
					else if (mask == 0x00000010)
					{
						pc.maxMp = getIntegerToken(data, "|", i);// 0x00000010
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
		//updataPcAct();
		if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
		{
			party[0].level = pc.level;
			party[0].maxHp = pc.maxHp;
			party[0].hp = pc.hp;
			party[0].name = pc.name;
		}

		emit signalDispatcher.updateCharHpProgressValue(pc.level, pc.hp, pc.maxHp);
		emit signalDispatcher.updateCharMpProgressValue(pc.level, pc.mp, pc.maxMp);

		if (pc.ridePetNo < 0)
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		else
		{
			PET _pet = pet[pc.ridePetNo];
			emit signalDispatcher.updateRideHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}

		if (pc.battlePetNo < 0)
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		else
		{
			PET _pet = pet[pc.battlePetNo];
			emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}

		emit signalDispatcher.updatePlayerInfoStone(pc.gold);
		const QVariantList varList = {
			pc.name, pc.freeName, "",
			tr("%1(%2tr)").arg(pc.level).arg(pc.transmigration), pc.exp, pc.maxExp, pc.exp > pc.maxExp ? 0 : pc.maxExp - pc.exp, "",
			QString("%1/%2").arg(pc.hp).arg(pc.maxHp), QString("%1/%2").arg(pc.mp).arg(pc.maxMp),
			pc.charm, pc.atk, pc.def, pc.quick, pc.luck
		};

		const QVariant var = QVariant::fromValue(varList);

		emit signalDispatcher.updatePlayerInfoColContents(0, var);

		//#ifdef _STONDEBUG_
		//		char title[128];
		//		sprintf_s(title, "%s %s [%s  %s:%s]", DEF_APPNAME, "調試版本",
		//			gmsv[selectServerIndex].name,
		//			gmsv[selectServerIndex].ipaddr, gmsv[selectServerIndex].port);
		//#else
		//		char title[128];
		//		extern int nServerGroup;
		//		sprintf_s(title, "%s %s [%s] %s", DEF_APPNAME, gmgroup[nServerGroup].name, gmsv[selectServerIndex].name, pc.name);
		//
		//		extern int 繁體開關;
		//		if (繁體開關)
		//		{
		//			char 繁體[1024] = { 0 };
		//			LCMapString(0x804, 0x4000000, title, strlen(title), 繁體, 1024);
		//			sprintf(title, "%s", 繁體);
		//		}
		//
		//#endif
		//		extern int 編碼;
		//		extern char* GB2312ToBIG5(const char* szGBString);
		Injector& injector = Injector::getInstance();
		int server = injector.getValueHash(util::kServerValue);
		int subserver = injector.getValueHash(util::kSubServerValue);
		int position = injector.getValueHash(util::kPositionValue);

		QString title = QString("SaSH [%1:%2:%3] - %4 LV:%5(%6/%7) MP:%8/%9")
			.arg(server).arg(subserver).arg(position).arg(pc.name).arg(pc.level).arg(pc.hp).arg(pc.maxHp).arg(pc.mp).arg(pc.maxMp);
		std::wstring wtitle = title.toStdWString();
		//SetWindowTextW(injector.getProcessWindow(), wtitle.c_str());
		//if (!bNewServer)
		//	pc.ridePetNo = -1;
		//if ((bNewServer & 0xf000000) == 0xf000000 && sPetStatFlag == 1)
		//	saveUserSetting();
	}
	else if (first == "F") // F 家族狀態
	{
		QString familyName;

		getStringToken(data, "|", 1, familyName);
		makeStringFromEscaped(familyName);
		pc.familyName = familyName;

		pc.familyleader = getIntegerToken(data, "|", 2);
		pc.channel = getIntegerToken(data, "|", 3);
		pc.familySprite = getIntegerToken(data, "|", 4);
		pc.big4fm = getIntegerToken(data, "|", 5);
#ifdef _CHANNEL_MODIFY
		if (pc.familyleader == FMMEMBER_NONE)
		{
			//pc.etcFlag &= ~PC_ETCFLAG_CHAT_FM;
			TalkMode = 0;
		}
#endif

		// HP,MP,EXP
	}
	else if (first == "M") // M HP,MP,EXP
	{
		pc.hp = getIntegerToken(data, "|", 1);
		pc.mp = getIntegerToken(data, "|", 2);
		pc.exp = getIntegerToken(data, "|", 3);
		//updataPcAct();
		if ((pc.status & CHR_STATUS_LEADER) != 0 && party[0].useFlag != 0)
			party[0].hp = pc.hp;

		emit signalDispatcher.updateCharHpProgressValue(pc.level, pc.hp, pc.maxHp);
		emit signalDispatcher.updateCharMpProgressValue(pc.level, pc.mp, pc.maxMp);
	}
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
			pet[no].useFlag = 0;
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
							i++;
						}
						else if (mask == 0x00000008)
						{
							pet[no].maxHp = getIntegerToken(data, "|", i);// 0x00000008
							i++;
						}
						else if (mask == 0x00000010)
						{
							pet[no].mp = getIntegerToken(data, "|", i);// 0x00000010
							i++;
						}
						else if (mask == 0x00000020)
						{
							pet[no].maxMp = getIntegerToken(data, "|", i);// 0x00000020
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
			emit signalDispatcher.updateRideHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updateRideHpProgressValue(0, 0, 100);
		}

		if (pc.battlePetNo >= 0 && pc.battlePetNo < MAX_PET)
		{
			PET _pet = pet[pc.battlePetNo];
			emit signalDispatcher.updatePetHpProgressValue(_pet.level, _pet.hp, _pet.maxHp);
		}
		else
		{
			emit signalDispatcher.updatePetHpProgressValue(0, 0, 100);
		}

		PET _pet = pet[no];
		const QVariantList varList = {
			_pet.name, _pet.freeName, "",
			tr("%1(%2tr)").arg(_pet.level).arg(_pet.trn), _pet.exp, _pet.maxExp, _pet.maxExp - _pet.exp, "",
			QString("%1/%2").arg(_pet.hp).arg(_pet.maxHp), "",
			_pet.ai, _pet.atk, _pet.def, _pet.quick, ""
		};

		const QVariant var = QVariant::fromValue(varList);

		emit signalDispatcher.updatePlayerInfoColContents(no + 1, var);
	}
	else if (first == "E") // E nowEncountPercentage
	{
		minEncountPercentage = getIntegerToken(data, "|", 1);
		maxEncountPercentage = getIntegerToken(data, "|", 2);
		nowEncountPercentage = minEncountPercentage;
	}
	else if (first == "J") //J 魔法
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
			magic[no].name = name;
			getStringToken(data, "|", 6, memo);
			makeStringFromEscaped(memo);
			magic[no].memo = memo;
		}
	}
	else if (first == "N")  // N 隊伍資訊
	{
		//ACTION* ptAct;
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
				//ptAct = getCharObjAct(party[no].id);
				//if (ptAct != NULL)
				//	delCharParty(ptAct);
			}
			//gx = -1;
			//gy = -1;
			//if (party[no].ptAct != NULL)
			//{
			//	//gx = party[no].ptAct->nextGx;
			//	//gy = party[no].ptAct->nextGy;
			//}
			party[no].useFlag = 0;
			//party[no].ptAct = NULL;
			checkPartyCount = 0;
			no2 = -1;
#ifdef MAX_AIRPLANENUM
			for (i = 0; i < MAX_AIRPLANENUM; i++)
#else
			for (i = 0; i < MAX_PARTY; i++)
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
				clearPartyParam();
#ifdef _CHANNEL_MODIFY
				//pc.etcFlag &= ~PC_ETCFLAG_CHAT_MODE;
				if (TalkMode == 2)
					TalkMode = 0;
#endif
			}
			else
			{
				//if (no2 >= 0 || gx >= 0 || gy >= 0)
					//goFrontPartyCharacter(no2, gx, gy);
			}
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
	}

	else if (first == "I") //I 道具
	{
		int i, no;
		QString temp;

		for (i = 0; i < MAX_ITEM; i++)
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
				pc.item[i].useFlag = 0;
				pc.item[i].name = "";
				refreshItemInfo(i);
				continue;
			}
			pc.item[i].useFlag = 1;
			pc.item[i].name = temp;
			getStringToken(data, "|", no + 2, temp);
			makeStringFromEscaped(temp);
			pc.item[i].name2 = temp;
			pc.item[i].color = getIntegerToken(data, "|", no + 3);
			if (pc.item[i].color < 0)
				pc.item[i].color = 0;
			getStringToken(data, "|", no + 4, temp);
			makeStringFromEscaped(temp);
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
			pc.item[i].damage = temp;
#ifdef _ITEM_PILENUMS
			getStringToken(data, "|", no + 11, temp);
			makeStringFromEscaped(temp);
			pc.item[i].pile = temp.toInt();
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

	}
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


		for (i = 0; i < MAX_SKILL; i++)
			petSkill[no][i].useFlag = 0;
		for (i = 0; i < MAX_SKILL; i++)
		{
			no2 = i * 5;
			getStringToken(data, "|", no2 + 4, temp);
			makeStringFromEscaped(temp);
			if (temp.isEmpty())
				continue;
			petSkill[no][i].useFlag = 1;
			petSkill[no][i].name = temp;
			petSkill[no][i].skillId = getIntegerToken(data, "|", no2 + 1);
			petSkill[no][i].field = getIntegerToken(data, "|", no2 + 2);
			petSkill[no][i].target = getIntegerToken(data, "|", no2 + 3);
			getStringToken(data, "|", no2 + 5, temp);
			makeStringFromEscaped(temp);
			petSkill[no][i].memo = temp;
		}
	}
#ifdef _CHAR_PROFESSION			// WON ADD 人物職業
	else if (first == "S") // S 職業技能
	{
		QString name;
		QString memo;
		int i, count = 0;

		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
		{
			profession_skill[i].useFlag = 0;
			profession_skill[i].kind = 0;
		}
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
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
#ifdef _PRO3_ADDSKILL
	case 'G':
	{
		int i, count = 0;
		data++;
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
			profession_skill[i].cooltime = 0;
		for (i = 0; i < MAX_PROFESSION_SKILL; i++)
		{
			count = i * 1;
			profession_skill[i].cooltime = getIntegerToken(data, "|", 1 + count);
		}
		break;
	}
#endif
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

		for (i = 0; i < MAX_PET_ITEM; i++)
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
			pet[nPetIndex].item[i].useFlag = 1;
			pet[nPetIndex].item[i].name = szData;
			getStringToken(data, "|", no + 2, szData);
			makeStringFromEscaped(szData);
			pet[nPetIndex].item[i].name2 = szData;
			pet[nPetIndex].item[i].color = getIntegerToken(data, "|", no + 3);
			if (pet[nPetIndex].item[i].color < 0)
				pet[nPetIndex].item[i].color = 0;
			getStringToken(data, "|", no + 4, szData);
			makeStringFromEscaped(szData);
			pet[nPetIndex].item[i].memo = szData;
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
			pet[nPetIndex].item[i].damage = szData;
			pet[nPetIndex].item[i].pile = getIntegerToken(data, "|", no + 11);
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
	else
	{
		qDebug() << "[" << first << "]:" << data;
	}
}

void Server::lssproto_ClientLogin_recv(int fd, char* cresult)
{
	QString result = util::toUnicode(cresult);
	if (result.isEmpty())
		return;
	//netproc_sending = NETPROC_SENDING;
	//if (netproc_sending == NETPROC_SENDING)
	//{
		//netproc_sending = NETPROC_RECEIVED;
	if (result.contains(OKSTR))
	{
		clientLoginStatus = 1;
		time(&serverAliveLongTime);
		localtime_s(&serverAliveTime, &serverAliveLongTime);
	}
	else if (result.contains(CANCLE))
	{
		//ChangeProc(PROC_TITLE_MENU , 6 );

		//cleanupNetwork();

		PaletteChange(DEF_PAL, 0);

		//cary
		ProcNo = PROC_ID_PASSWORD;
		SubProcNo = 5;

		//DeathAllAction();

	}
	//}
}

void Server::lssproto_CreateNewChar_recv(int fd, char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;

	if (result.contains(SUCCESSFULSTR) || data.contains(SUCCESSFULSTR))
	{
		newCharStatus = 1;
	}
	else
	{
		//創建人物內容提示
	}
}

void Server::lssproto_CharList_recv(int fd, char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);

	if (result.isEmpty() && data.isEmpty())
		return;
	//char LoginErrorMessage[1024];
	//memset(LoginErrorMessage, 0, 1024);
	if (result.contains("failed"))
	{
		//_snprintf_s(LoginErrorMessage, sizeof(LoginErrorMessage), _TRUNCATE, "%s", data);
#ifdef _AIDENGLU_
		PcLanded.登陸延時時間 = TimeGetTime() + 2000;
#endif
	}

	//if (netproc_sending == NETPROC_SENDING)
	//{
	QString nm, opt;
	int i;

	//netproc_sending = NETPROC_RECEIVED;
	if (result.contains(SUCCESSFULSTR) || data.contains(SUCCESSFULSTR))
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusGettingPlayerList);


		if (result.contains("OUTOFSERVICE"))
			charListStatus = 2;
#ifdef _CHANGEGALAXY
		if (result.contains("CHANGE_GALAXY")
			charListStatus = 3;
#endif
#ifdef _ERROR301
			if (result.contains("301")
				charListStatus = 4;
#endif
				return;
	}
	charListStatus = 1;
	for (i = 0; i < MAXCHARACTER; i++)
	{
		nm.clear();
		opt.clear();
		getStringToken(data, "|", i * 2 + 1, nm);
		getStringToken(data, "|", i * 2 + 2, opt);
		//setCharacterList(nm, opt);
	}
	//}
}

void Server::lssproto_CharLogout_recv(int fd, char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;
	//if (netproc_sending == NETPROC_SENDING)
	//{
	//netproc_sending = NETPROC_RECEIVED;
	if (result.contains(SUCCESSFULSTR) || data.contains(SUCCESSFULSTR))
	{
		IS_ONLINE_FLAG = false;
		IS_BATTLE_FLAG = false;
	}
	//}
}

void Server::lssproto_CharLogin_recv(int fd, char* cresult, char* cdata)
{
	QString data = util::toUnicode(cdata);
	QString result = util::toUnicode(cresult);
	if (result.isEmpty() && data.isEmpty())
		return;
	//if (netproc_sending == NETPROC_SENDING)
	//{
		//netproc_sending = NETPROC_RECEIVED;
#ifdef __NEW_CLIENT
	if (strcmp(result, SUCCESSFULSTR) == 0 && !hPing)
#else
	if (result.contains(SUCCESSFULSTR) || data.contains(SUCCESSFULSTR))
#endif
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSignning);

		IS_ONLINE_FLAG = true;
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
	//}
}

void Server::lssproto_TD_recv(int fd, char* cdata)//交易
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
}

void Server::lssproto_CHAREFFECT_recv(int fd, char* cdata)
{
	QString data = util::toUnicode(cdata);
	if (data.isEmpty())
		return;
	//	if (data[0] == '1')
	//		setCharFamily(pc.ptAct, atoi(data + 2));
	//	else if (data[0] == '2')
	//		setCharMind(pc.ptAct, atoi(data + 2));
	//	else if (data[0] == '3')
	//		setCharmFamily(pc.ptAct, atoi(data + 2));
	//#ifdef _CHARTITLE_
	//	else if (data[0] == '4')
	//	{
	//		setCharmTitle(pc.ptAct, atoi(data + 2));
	//	}
	//#endif
	//#ifdef _CHAR_MANOR_
	//	else if (data[0] == '5')
	//		setCharmManor(pc.ptAct, atoi(data + 2));
	//#endif
}

int Server::getUnloginStatus()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	int W = getWorldStatus();
	int G = getGameStatus();

	if (11 == W && 2 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusDisconnected);
		return util::kStatusDisconnect;
	}
	if (3 == W && 101 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusBusy);
		return util::kStatusBusy;
	}
	if (2 == W && 101 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusTimeout);
		return util::kStatusTimeout;
	}
	if (1 == W && 101 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelNoUserNameOrPassword);
		return util::kNoUserNameOrPassword;
	}
	if (1 == W && 2 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusLogining);
		return util::kStatusInputUser;
	}
	if (2 == W && 2 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSelectServer);
		return util::kStatusSelectServer;
	}
	if (2 == W && 3 == G)
	{
		IS_ONLINE_FLAG = false;
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSelectSubServer);
		return util::kStatusSelectSubServer;
	}
	if (3 == W && 11 == G)
	{
		emit signalDispatcher.updateStatusLabelTextChanged(util::kLabelStatusSelectPosition);
		return util::kStatusSelectCharacter;
	}
	if (9 == W && 3 == G)
		return util::kStatusLogined;
	return util::kStatusUnknown;
}

int Server::getWorldStatus()
{

	Injector& injector = Injector::getInstance();
	return mem::readInt(injector.getProcess(), injector.getProcessModule() + 0x4230DD8, sizeof(int));
}

int Server::getGameStatus()
{

	Injector& injector = Injector::getInstance();
	return mem::readInt(injector.getProcess(), injector.getProcessModule() + 0x4230DF0, sizeof(int));
}

void Server::setWorldStatus(int w)
{

	Injector& injector = Injector::getInstance();
	mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0x4230DD8, w, 0);
}
void Server::setGameStatus(int g)
{
	Injector& injector = Injector::getInstance();
	injector.postMessage(Injector::kSetGameStatus, g, NULL);
}

void Server::logOut()
{
	if (!IS_ONLINE_FLAG)
		return;

	lssproto_CharLogout_send(sockfd_, 0);
}

void Server::logBack()
{
	if (!IS_ONLINE_FLAG)
		return;

	lssproto_CharLogout_send(sockfd_, 1);
}

// 0斷線1回點
void Server::lssproto_CharLogout_send(int fd, int Flg)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (Flg == 0)
	{
		setWorldStatus(7);
		setGameStatus(0);
	}

	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
#ifdef _CHAR_NEWLOGOUT
	iChecksum += Autil::util_mkint(buffer, Flg);
#endif
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_CHARLOGOUT_SEND, buffer);
}

void Server::setSwitcher(int flg, bool enable)
{
	if (enable)
		pc.etcFlag |= flg;
	else
		pc.etcFlag &= ~flg;

	setSwitcher(pc.etcFlag);
}
void Server::setSwitcher(int flg)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (IS_BATTLE_FLAG)
		return;

	lssproto_FS_send(sockfd_, flg);
}

//Switcher
void Server::lssproto_FS_send(int fd, int flg)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkint(buffer, flg);
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_FS_SEND, buffer);
}

void Server::unlockSecurityCode(const QString& code)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (IS_BATTLE_FLAG)
		return;

	if (code.isEmpty())
		return;

	//6-15个字符（必须大小写字母加数字)
	static const QRegularExpression regex("^(?=.*[A-Z])(?=.*\\d)[A-Za-z\\d]{6,15}$");

	if (!regex.match(code).hasMatch())
	{
		return;
	}

	std::string scode = code.toStdString();
	lssproto_WN_send(sockfd_, nowGx, nowGy, 522, -1, NULL, const_cast<char*>(scode.c_str()));
}

//對話框封包
void Server::lssproto_WN_send(int fd, int x, int y, int seqno, int objindex, int select, char* data)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkint(buffer, x);
	iChecksum += Autil::util_mkint(buffer, y);
	iChecksum += Autil::util_mkint(buffer, seqno);
	iChecksum += Autil::util_mkint(buffer, objindex);
	iChecksum += Autil::util_mkint(buffer, select);
	iChecksum += Autil::util_mkstring(buffer, data);
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_WN_SEND, buffer);
}

void Server::move(const QPoint& p, const QString& dir)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (IS_BATTLE_FLAG)
		return;

	if (p.x() < 0 || p.x() > 1500 || p.y() < 0 || p.y() > 1500)
		return;

	std::string sdir = dir.toStdString();
	lssproto_W2_send(sockfd_, p.x(), p.y(), const_cast<char*>(sdir.c_str()));
}

void Server::move(const QPoint& p)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (IS_BATTLE_FLAG)
		return;

	if (p.x() < 0 || p.x() > 1500 || p.y() < 0 || p.y() > 1500)
		return;
	Injector& injector = Injector::getInstance();
	injector.postMessage(Injector::kSetMove, p.x(), p.y());
}

//移動轉向封包
void Server::lssproto_W2_send(int fd, int x, int y, char* direction)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkint(buffer, x);
	iChecksum += Autil::util_mkint(buffer, y);
	iChecksum += Autil::util_mkstring(buffer, direction);
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_W2_SEND, buffer);
}

void Server::announce(const QString& msg, int color)
{
	if (!IS_ONLINE_FLAG)
		return;

	Injector& injector = Injector::getInstance();

	std::string str = util::fromUnicode(msg);
	HANDLE hProcess = injector.getProcess();
	util::VMemory ptr(hProcess, str.size(), true);
	mem::write(hProcess, ptr, const_cast<char*>(str.c_str()), str.size());
	injector.sendMessage(Injector::kSendAnnounce, ptr, color);
}

void Server::setBattleEnd()
{
	if (!IS_ONLINE_FLAG)
		return;

	Injector& injector = Injector::getInstance();

	if (getWorldStatus() != 10)
		IS_BATTLE_FLAG = false;


	battle_total_time += battleDurationTimer.elapsed();
	battleDurationTimer.restart();

	lssproto_EO_send(sockfd_, 0);
	lssproto_Echo_send(sockfd_, const_cast<char*>("????"));
	lssproto_EO_send(sockfd_, 0);
	lssproto_Echo_send(sockfd_, const_cast<char*>("!!!!"));
}

void Server::EO()
{
	if (!IS_ONLINE_FLAG)
		return;

	eottlTimer.restart();
	isEOTTLSend = true;
	lssproto_EO_send(sockfd_, 0);
	lssproto_Echo_send(sockfd_, const_cast<char*>("!!!!"));
	move(QPoint(nowGx, nowGy), "C");
}

void Server::lssproto_EO_send(int fd, int dummy)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkint(buffer, dummy);
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_EO_SEND, buffer);
}

void Server::lssproto_Echo_send(int fd, char* test)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkstring(buffer, test);
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_ECHO_SEND, buffer);
}

void Server::dropItem(int index)
{
	if (index < 0 || index> MAX_ITEM)
		return;

	if (!IS_ONLINE_FLAG)
		return;

	if (IS_BATTLE_FLAG)
		return;

	lssproto_DI_send(sockfd_, nowGx, nowGy, index);
}

void Server::lssproto_DI_send(int fd, int x, int y, int itemindex)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkint(buffer, x);
	iChecksum += Autil::util_mkint(buffer, y);
	iChecksum += Autil::util_mkint(buffer, itemindex);
	Autil::util_mkint(buffer, iChecksum);
	Autil::util_SendMesg(fd, LSSPROTO_DI_SEND, buffer);
}

void Server::leftCLick(int x, int y)
{
	Injector& injector = Injector::getInstance();
	HWND hWnd = injector.getProcessWindow();
	LPARAM data = MAKELPARAM(x, y);
	SendMessageW(hWnd, WM_MOUSEMOVE, NULL, data);
	PostMessageW(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, data);
	PostMessageW(hWnd, WM_LBUTTONUP, MK_LBUTTON, data);
}

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
			leftCLick(315, 270);
			disconnectflag = true;
		}
		return false;
	}

	if (!enableAutoLogin && enableReconnect && !disconnectflag)
		return false;

	switch (status)
	{
	case util::kStatusBusy:
	{
		leftCLick(315, 255);
		break;
	}
	case util::kStatusTimeout:
	{
		leftCLick(315, 253);
		break;
	}
	case util::kNoUserNameOrPassword:
	{
		leftCLick(315, 253);
		break;
	}
	case util::kStatusInputUser:
	{
		Injector& injector = Injector::getInstance();
		static const QRegularExpression accountRegex("^[a-z\\d]{4,15}$");// 4-15个字符（可用小写字母和数字）
		static const QRegularExpression passwordRegex("^(?=.*[A-Z])(?=.*\\d)[A-Za-z\\d]{6,15}$");// 6-15个字符（必须大小写字母加数字）

		if (!account.isEmpty() && accountRegex.match(account).hasMatch())
		{
			mem::writeString(injector.getProcess(), injector.getProcessModule() + 0x414F278, account);
		}
		if (!password.isEmpty() && passwordRegex.match(password).hasMatch())
		{
			mem::writeString(injector.getProcess(), injector.getProcessModule() + 0x415AA58, password);
		}
		leftCLick(380, 310);
		break;
	}
	case util::kStatusSelectServer:
	{
		constexpr int table[36] = {
			0, 0, 0,
			1, 0, 1,
			2, 0, 2,
			3, 0, 3,
			4, 1, 0,
			5, 1, 1,
			6, 1, 2,
			7, 1, 3,
			8, 2, 0,
			9, 2, 1,
			10, 2, 2,
			11, 2, 3,
		};
		if (server >= 0 && server < 12)
		{
			const int a = table[server * 3 + 1];
			const int b = table[server * 3 + 2];
			leftCLick(170 + (a * 125), 165 + (b * 25));
		}
		break;
	}
	case util::kStatusSelectSubServer:
	{
		if (subserver >= 0 && subserver <= 4)
		{
			leftCLick(250, 265 + (subserver * 25));
		}
		break;
	}
	case util::kStatusSelectCharacter:
	{
		if (position >= 0 && position <= 1)
			leftCLick(100 + (position * 300), 340);
		break;
	}
	case util::kStatusLogined:
	{
		disconnectflag = false;
		return true;
	}
	default:
		break;
	}
	disconnectflag = false;
	return false;
}

void Server::refreshItemInfo(int i)
{
	QVariant var;
	QVariantList varList;

	if (i < 0 || i >= MAX_ITEM)
		return;

	if (i < CHAR_EQUIPPLACENUM)
	{
		QStringList equipVHeaderList = {
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"), tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"), tr("gloves")
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
		emit signalDispatcher.updateEquipInfoRowContents(i, var);
	else
		emit signalDispatcher.updateItemInfoRowContents(i, var);
}

void Server::refreshItemInfo()
{
	for (int i = 0; i < MAX_ITEM; i++)
	{
		refreshItemInfo(i);
	}
}

void Server::updateDatasFromMemory()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	Injector& injector = Injector::getInstance();
	int hModule = injector.getProcessModule();
	HANDLE hProcess = injector.getProcess();

	if (!IS_ONLINE_FLAG)
		return;

	nowGx = mem::readInt(hProcess, hModule + 0x4181D3C, sizeof(int));
	nowGy = mem::readInt(hProcess, hModule + 0x4181D40, sizeof(int));

	mem::read(hProcess, hModule + 0x422BF34, sizeof(pc.selectPetNo), pc.selectPetNo);

	pc.mailPetNo = static_cast<short>(mem::readInt(hProcess, hModule + 0x422BF3E, sizeof(short)));
	pc.ridePetNo = static_cast<short>(mem::readInt(hProcess, hModule + 0x422E3D8, sizeof(short)));

	emit signalDispatcher.updateCoordsPosLabelTextChanged(QString("%1,%2").arg(nowGx).arg(nowGy));

	if (!injector.getEnableHash(util::kFastBattleEnable))
	{
		int W = getWorldStatus();
		if (!IS_BATTLE_FLAG && (10 == W))
		{
			IS_BATTLE_FLAG = true;
		}
		else if (IS_BATTLE_FLAG && (9 == W))
		{
			IS_BATTLE_FLAG = false;
		}
	}
}

#pragma region BattleAction

int Server::playerDoBattleWork()
{
	Injector& injector = Injector::getInstance();

	do
	{
		if (injector.getEnableHash(util::kAutoEscapeEnable))
		{
			sendBattlePlayerEscapeAct();
			break;
		}

		int target = getBattleSelectableEnemyTarget();

		sendBattlePlayerAttckAct(target);
	} while (false);

	//if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
	//	mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//else
	//	mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 2, sizeof(short));

	//mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1, sizeof(short));
	return 1;
}

int Server::petDoBattleWork()
{
	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return 0;

	Injector& injector = Injector::getInstance();

	do
	{
		if (injector.getEnableHash(util::kAutoEscapeEnable))
		{
			sendBattlePetDoNothing();
			break;
		}

		int target = getBattleSelectableEnemyTarget();

		int skillIndex = getGetPetSkillIndexByName(pc.battlePetNo, GetACP() == 950 ? QString("攻擊") : QString("攻击"));
		if (skillIndex < 0)
		{
			sendBattlePetDoNothing();
			break;
		}

		sendBattlePetSkillAct(skillIndex, target);
	} while (false);
	//mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E4, 0, sizeof(short));
	//mem::writeInt(injector.getProcess(), injector.getProcessModule() + 0xE21E8, 1, sizeof(short));
	return 1;
}

int Server::getGetPetSkillIndexByName(int petIndex, const QString& name)
{
	if (!IS_ONLINE_FLAG)
		return -1;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return -1;

	if (name.isEmpty())
		return -1;

	int petSkillIndex = -1;
	for (int i = 0; i < MAX_SKILL; i++)
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

int Server::getBattleSelectableEnemyTarget()
{
	int defaultTarget = 15;
	if (BattleMyNo >= 10)
		defaultTarget = 5;

	if (!IS_ONLINE_FLAG)
		return defaultTarget;

	if (!IS_BATTLE_FLAG)
		return defaultTarget;

	QVector<battleobject_t> enemies = battleData.enemies;
	if (enemies.isEmpty())
		return defaultTarget;

	int size = enemies.size();

	auto compareByOrder = [](const battleobject_t& enemy1, const battleobject_t& enemy2)->bool
	{
		constexpr int maxorder = 20;
		constexpr int order[maxorder] = { 8, 6, 5, 7, 9, 3, 1, 0, 2, 4, 18, 16, 15, 17, 19, 13, 11, 10, 12, 14 };

		int index1 = std::distance(std::begin(order), std::find(std::begin(order), std::end(order), enemy1.pos));
		int index2 = std::distance(std::begin(order), std::find(std::begin(order), std::end(order), enemy2.pos));

		return index1 < index2;
	};

	std::sort(enemies.begin(), enemies.end(), compareByOrder);

	return enemies[0].pos;
}

bool Server::fixPlayerTargetByMagicIndex(int magicIndex, int oldtarget, int* target)
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
		int max = 9;
		int min = 0;
		if (BattleMyNo >= 10)
		{
			max = 19;
			min = 10;
		}

		if (oldtarget == BattleMyNo)
		{
			oldtarget = -1;
		}
		else if (oldtarget < min || oldtarget > max)
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
		else if (oldtarget == battleData.pet.pos)
		{
			oldtarget = -1;
		}
		break;
	}
	case MAGIC_TARGET_WHOLEOTHERSIDE://敵方全體 或 我方全體
	{
		if (oldtarget != 20 && oldtarget != 21)
		{
			if (BattleMyNo < 10)
				oldtarget = 21;
			else
				oldtarget = 20;
		}

		break;
	}
	case MAGIC_TARGET_SINGLE:				// 針對敵方某一方
	{
		if (oldtarget < 10 || oldtarget > 19)
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

bool Server::fixPetTargetBySkillIndex(int skillIndex, int oldtarget, int* target)
{
	if (!target)
		return false;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return false;

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
		oldtarget = -1;
		break;
	}
	case PETSKILL_TARGET_OTHERWITHOUTMYSELF:
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
		else if (oldtarget == battleData.pet.pos)
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
		else if (oldtarget == battleData.pet.pos)
		{
			oldtarget = -1;
		}
		break;
	}
	}

	*target = oldtarget;

	return true;
}

void Server::sendBattlePlayerAttckAct(int target)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (target < 0 || target > 21)
		return;

	QString qcmd = QString("H|%1").arg(target);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString name = !battleData.objects.at(target).freename.isEmpty() ? battleData.objects.at(target).freename : battleData.objects.at(target).name;
	emit signalDispatcher.updateLabelPlayerAction(tr("use attack %1").arg(name));
}

void Server::sendBattlePlayerMagicAct(int magicIndex, int target)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (target < 0 || target > 21)
		return;

	if (magicIndex < 0 || magicIndex >= MAX_MAGIC)
		return;

	QString qcmd = QString("J|%1|%2").arg(magicIndex).arg(target);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString magicName = magic[magicIndex].name;
	QString name = !battleData.objects.at(target).freename.isEmpty() ? battleData.objects.at(target).freename : battleData.objects.at(target).name;
	emit signalDispatcher.updateLabelPlayerAction(tr("use magic %1 to %2").arg(magicName).arg(name));
}

void Server::sendBattlePlayerJobSkillAct(int skillIndex, int target)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (target < 0 || target > 21)
		return;

	if (skillIndex < 0 || skillIndex >= MAX_PROFESSION_SKILL)
		return;

	QString qcmd = QString("P|%1|%2").arg(skillIndex).arg(target);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString skillName = profession_skill[skillIndex].name;
	QString name = !battleData.objects.at(target).freename.isEmpty() ? battleData.objects.at(target).freename : battleData.objects.at(target).name;
	emit signalDispatcher.updateLabelPlayerAction(tr("use skill %1 to %2").arg(skillName).arg(name));
}

void Server::sendBattlePlayerItemAct(int itemIndex, int target)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (target < 0 || target > 21)
		return;

	if (itemIndex < 0 || itemIndex >= MAX_ITEM)
		return;

	QString qcmd = QString("I|%1|%2").arg(itemIndex).arg(target);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString itemName = pc.item[itemIndex].name;
	QString name = !battleData.objects.at(target).freename.isEmpty() ? battleData.objects.at(target).freename : battleData.objects.at(target).name;
	emit signalDispatcher.updateLabelPlayerAction(tr("use item %1 to %2").arg(itemName).arg(name));
}

void Server::sendBattlePlayerDefenseAct()
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	QString qcmd("G");
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateLabelPlayerAction(tr("defense"));
}

void Server::sendBattlePlayerEscapeAct()
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	QString qcmd("E");
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateLabelPlayerAction(tr("escape"));
}

void Server::sendBattlePlayerCatchPetAct(int target)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (target < 0 || target > 21)
		return;

	QString qcmd = QString("T|%1").arg(target);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString name = !battleData.objects.at(target).freename.isEmpty() ? battleData.objects.at(target).freename : battleData.objects.at(target).name;
	emit signalDispatcher.updateLabelPlayerAction(tr("catch %1").arg(name));
}

void Server::sendBattlePlayerSwitchPetAct(int petIndex)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (petIndex < 0 || petIndex >= MAX_PET)
		return;

	QString qcmd = QString("S|%1").arg(petIndex);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateLabelPlayerAction(QString(tr("switch pet to %1"))
		.arg(!pet[petIndex].freeName.isEmpty() ? pet[petIndex].freeName : pet[petIndex].name));
}

void Server::sendBattlePlayerDoNothing()
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	QString qcmd("N");
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateLabelPlayerAction(tr("do nothing"));
}

void Server::sendBattlePetSkillAct(int skillIndex, int target)
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return;

	if (target < 0 || target > 21)
		return;

	if (skillIndex < 0 || skillIndex >= MAX_SKILL)
		return;

	if (pc.battlePetNo < 0 || pc.battlePetNo> MAX_PET)
		return;

	QString qcmd = QString("W|%1|%2").arg(skillIndex).arg(target);
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString name = !battleData.objects.at(target).freename.isEmpty() ? battleData.objects.at(target).freename : battleData.objects.at(target).name;
	emit signalDispatcher.updateLabelPetAction(tr("use %1 to %2").arg(petSkill[pc.battlePetNo][skillIndex].name).arg(name));
}

void Server::sendBattlePetDoNothing()
{
	if (!IS_ONLINE_FLAG)
		return;

	if (!IS_BATTLE_FLAG)
		return;

	if (pc.battlePetNo < 0 || pc.battlePetNo >= MAX_PET)
		return;

	QString qcmd("W|FF|FF");
	std::string str = qcmd.toStdString();

	lssproto_B_send(sockfd_, const_cast<char*>(str.c_str()));
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.updateLabelPetAction(tr("do nothing"));
}

void Server::lssproto_B_send(int fd, char* command)
{
	char buffer[16384] = { 0 };
	int iChecksum = 0;

	buffer[0] = '\0';
	iChecksum += Autil::util_mkstring(buffer, command);
	Autil::util_mkint(buffer, iChecksum);
	qDebug() << "BSend" << QString(command);
	Autil::util_SendMesg(fd, LSSPROTO_B_SEND, buffer);
	qDebug() << "BSend done";
}

#pragma endregion


