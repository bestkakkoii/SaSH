#ifndef TOOL_H_
#define TOOL_H_

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QWebSocket>
#include <QCryptographicHash>
#include <QPointer>
#include <QVariant>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

class Screen : public QObject
{
	Q_OBJECT
public:
	enum State
		: qint64
	{
		error = -3,
		unexpectedDisconnect = -2,
		uninitialize = -1,
		unknown = 0,
		waitForSelectAccount,
		selectAccount,
		selectServer,
		selectSubServer,
		waitForConnection,
		waitForSelectCharacter,
		selectCharacter,
		waitForOnlineNormal,
		onlineNormal,

		onlineBattle,
		waitForOffline,
		disconnected,

		warpping,
	};

	Q_ENUM(State);
};

class User : public QObject
{
	Q_OBJECT

public:
	enum Settings
	{
		// automationSettings
		autoLoginEnable,
		autoBattleEnable,
		autoItemHealEnable,
		autoItemHealSettingString,

		rewardDialogEnable,
		dialogEnable,
		warpAnimeEnable,

		autoDiscardItemEnable,
		autoDiscardItemListString,

		// performanceSettings
		walkSpeedValue,
		battleFPSValue,

		// securitySettings
		nicMacAddressString,

		// battleSettings
		characterBattleActionDelayValue,
		petBattleActionDelayValue,
		autoBattleEscapeEnable,
	};
	Q_ENUM(Settings);

	enum Category
	{
		displaySettings = 1,
		securitySettings,
		performanceSettings,
		automationSettings,
		battleSettings,
		scriptSettings,
	};
	Q_ENUM(Category);
};

namespace tool
{
	static constexpr bool isBitSet(quint64 value, quint64 bit)
	{
		return (value & (1ULL << bit)) == (1ULL << bit);
	}

	static constexpr bool isFlagSet(quint64 flags, quint64 flag)
	{
		return  (flags & flag) == flag;
	}

	qint64 getPercentage(qint64 currentValue, qint64 maxValue);

	[[nodiscard]] QString applicationFilePath();

	[[nodiscard]] QString applicationDirPath();

	[[nodiscard]] QString applicationName();

	[[nodiscard]] QString applicationNameNoSuffix();

	QString generateDateTimeToken();

	void terminateCurrentProcess();

	HWND getCurrentWindowHandle();

	HWND getWindowHandle(DWORD processId);

	//#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	//	DWORD getParentProcessId(DWORD processId);
	//#endif

	qreal fpsToMs(qreal fps);

	void clrscr();

	void clearScreenButKeepFirstLine();

	QByteArray toHash(const QByteArray& data, QCryptographicHash::Algorithm algorithm);

	QString getWindowsSDKVersion();

#if 0
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	enum HardwareType
		: qint64
	{
		BIOS, //@别名 BIOS
		Motherboard, //@别名 主板
		HardDrive, //@别名 硬盘
		CPU, //@别名 处理器
		GraphicsCard, //@别名 显卡
		NetworkAdapter, //@别名 网卡
		PhysicalMemory, //@别名 内存
		SoundDevice, //@别名 声卡
		OperatingSystem, //@别名 操作系统
	};

	QStringList getHardwareSerialNumber(HardwareType type);
#endif
#endif

	template<class T, class T2>
	inline void memoryMove(T dis, T2* src, size_t size)
	{
		DWORD dwOldProtect = 0UL;
		VirtualProtect((void*)dis, size, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		memmove_s((void*)dis, size, (void*)src, size);
		VirtualProtect((void*)dis, size, dwOldProtect, &dwOldProtect);
	}

	/// <summary>
	/// 
	/// </summary>
	/// <typeparam name="ToType"></typeparam>
	/// <typeparam name="FromType"></typeparam>
	/// <param name="addr"></param>
	/// <param name="f"></param>
	/// <returns></returns>
	template <class ToType, class FromType>
	void __stdcall getFuncAddr(ToType* addr, FromType f)
	{
		union
		{
			FromType _f;
			ToType   _t;
		}ut{};

		ut._f = f;

		*addr = ut._t;
	}

	/// <summary>
	/// HOOK Function
	/// </summary>
	/// <typeparam name="T">A global or class function or a specific address segment to jump to</typeparam>
	/// <typeparam name="T2">BYTE Array</typeparam>
	/// <param name="pfnHookFunc">The function or address to CALL or JMP to</param>
	/// <param name="bOri">The address where the HOOK will be written</param>
	/// <param name="bOld">A BYTE array used to save the original data, depending on the size of the original assembly at the write address</param>
	/// <param name="bNew">A BYTE array for writing the jump or CALL, pre-filled with necessary 0x90 or 0xE8 0xE9</param>
	/// <param name="nByteSize">The size of the bOld and bNew arrays</param>
	/// <param name="offset">Sometimes there might be other things before the jump target address that need to be skipped, hence an offset is needed, which most of the time is 0</param>
	/// <returns>void</returns>
	template<class T, class T2>
	void __stdcall detour(T pfnHookFunc, DWORD bOri, T2* bOld, T2* bNew, const size_t nByteSize, const DWORD offest)
	{
		DWORD hookfunAddr = 0UL;
		getFuncAddr(&hookfunAddr, pfnHookFunc);//獲取函數HOOK目標函數指針(地址)
		DWORD dwOffset = (hookfunAddr + offest) - (DWORD)bOri - nByteSize;//計算偏移

		memoryMove((DWORD)&bNew[nByteSize - 4u], &dwOffset, sizeof(dwOffset));//將計算出的結果寫到緩存 CALL XXXXXXX 或 JMP XXXXXXX

		memoryMove((DWORD)bOld, (void*)bOri, nByteSize);//將原始內容保存到bOld (之後需要還原時可用)

		memoryMove((DWORD)bOri, bNew, nByteSize);//將緩存內的東西寫到要HOOK的地方(跳轉到hook函數 或調用hook函數)
	}

	/// <summary>
	/// Cancel HOOK
	/// </summary>
	/// <typeparam name="T">BYTE Array</typeparam>
	/// <param name="ori">The address segment to be restored</param>
	/// <param name="oldBytes">A pointer to the BYTE array used for backup</param>
	/// <param name="size">The size of the BYTE array</param>
	/// <returns>void</returns>
	template<class T>
	void  __stdcall undetour(T ori, BYTE* oldBytes, SIZE_T size)
	{
		memoryMove(ori, oldBytes, size);
	}

	QString getMachineCode();

	static inline qint32 equal(float p1, float p2) { return static_cast<int>((qFuzzyCompare(p1, p2) ? p2 : std::floor(qMin(p1, p2)))); }

	QString formatTime(qint64 milliseconds, const QString& format);

	class JsonReply
	{
	public:
		JsonReply(const QString& action, const QString& message);

		JsonReply(const QString& action);

		JsonReply(const QJsonDocument& jsonDocument);

		void setStatus(bool status);

		void setConversationId(const QString& conversationId);

		void setAction(const QString& action);

		void setCommand(const QString& command);

		void setMessage(const QString& message);

		void setField(const QString& field);

		void setProcessId(qint64 processId);

		void setUsername(const QString& username);

		void insert(const QString& key, const QVariant& value);

		void insertData(const QString& key, const QVariant& value);

		void setData(const QJsonObject& jsonData);

		void insertParameter(const QString& key, const QVariant& value);

		void setParameters(const QJsonObject& jsonParameters);

		void appendParameters(const QJsonObject& jsonParameters);

		void setTimestamp(qint64 timestamp);

		QString toString() const;

		QByteArray toUtf8() const;

		QJsonObject parameters() const { return parametersObject_; }

		QJsonValue parameter(const QString& key) const { return parametersObject_.value(key); }

		QJsonObject datas() const { return dataObject_; }

		QJsonValue data(const QString& key) const { return dataObject_.value(key); }

		QJsonValue json(const QString& key) const { return jsonObject_.value(key); }

		QJsonObject json() const { return jsonObject_; }

		QJsonDocument jsonDocument() const { return jsonDocument_; }

		QString action() const { return action_; }

		QString message() const { return message_; }

		QString field() const { return parameter("field").toString(); }

		QString username() const { return parameter("username").toString(); }

		QString conversationId() const
		{
			QString str = json("conversationId").toString();
			if (str.size() <= 0)
			{
				str = "empty";
			}

			return str;
		}

		QString command() const { return data("command").toString(); }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		qint64 processId() const { return parameter("processId").toInteger(); }
#else
		int processId() const { return parameter("processId").toInt(); }
#endif

		void setClient(void* client) { client_ = client; }

		void* client() const { return client_; }

	private:
		void* client_ = nullptr;

		QJsonDocument jsonDocument_;
		QJsonObject jsonObject_;
		QJsonObject dataObject_;
		QJsonObject parametersObject_;
		QString action_;
		QString message_;
	};
}


namespace string
{
	qint64 base62ToDecimal(const QString& data);

	QString decimalToBase62(qint64 data);


	QString toPercentEncoding(const QString& src);

	QString toPercentEncoding(const QByteArray& src);

	QString encryptUrl(const QString& url);

	QString toIndentedJson(const QString& objectString);

	bool tryFuzzyMatch(const QString& source, const QString& target, const QString& extraTarget = "");

	quint16 generate32BitMD5Hash(const QString& input);
}

#endif // TOOL_H_