#pragma once
#include <QScopedPointer>
#include <QRegularExpression>
#include <QObject>
#include <QWidget>
#include <QList>
#include <QVariant>
#include <QVariantMap>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QTabBar>
#include "3rdparty/simplecrypt.h"

constexpr int SASH_VERSION_MAJOR = 1;
constexpr int SASH_VERSION_MINOR = 0;
constexpr int SASH_VERSION_PATCH = 0;


namespace mem
{
	bool read(HANDLE hProcess, DWORD desiredAccess, SIZE_T size, PVOID buffer);
	Q_REQUIRED_RESULT int readInt(HANDLE hProcess, DWORD desiredAccess, SIZE_T size);
	Q_REQUIRED_RESULT float readFloat(HANDLE hProcess, DWORD desiredAccess);
	Q_REQUIRED_RESULT qreal readDouble(HANDLE hProcess, DWORD desiredAccess);
	Q_REQUIRED_RESULT QString readString(HANDLE hProcess, DWORD desiredAccess, int size, bool enableTrim);
	bool write(HANDLE hProcess, DWORD baseAddress, PVOID buffer, SIZE_T dwSize);
	bool writeInt(HANDLE hProcess, DWORD baseAddress, int data, int mode);
	bool writeString(HANDLE hProcess, DWORD baseAddress, const QString& str);
	bool virtualFree(HANDLE hProcess, int baseAddress);
	Q_REQUIRED_RESULT int virtualAlloc(HANDLE hProcess, int size);
	Q_REQUIRED_RESULT int virtualAllocW(HANDLE hProcess, const QString& str);
	Q_REQUIRED_RESULT int virtualAllocA(HANDLE hProcess, const QString& str);
	DWORD GetRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName);
}

namespace util
{
	template <typename K, typename V>
	class SafeHash
	{
	public:
		SafeHash()
		{
		}

		inline SafeHash(std::initializer_list<std::pair<K, V> > list)
		{
			QWriteLocker locker(&lock);
			hash = list;
		}

		//copy
		SafeHash(const QHash<K, V>& other)
		{
			QWriteLocker locker(&lock);
			hash = other;
		}

		//copy assign
		SafeHash(const SafeHash<K, V>& other)
		{
			QWriteLocker locker(&lock);
			hash = other.hash;
		}

		//move
		SafeHash(QHash<K, V>&& other)
		{
			QWriteLocker locker(&lock);
			hash = std::move(other);
		}

		//move assign
		SafeHash(SafeHash<K, V>&& other) noexcept
		{
			QWriteLocker locker(&lock);
			hash = std::move(other.hash);
		}

		SafeHash& operator=(const SafeHash& other)
		{
			QWriteLocker locker(&lock);
			if (this != &other)
			{
				hash = other.hash;
			}
			return *this;
		}

		//operator=
		SafeHash& operator=(const QHash <K, V>& other)
		{
			QWriteLocker locker(&lock);
			if (this != &other)
			{
				hash = other;
			}
			return *this;
		}

		inline void insert(const K& key, const V& value)
		{
			QWriteLocker locker(&lock);
			hash.insert(key, value);
		}
		inline void remove(const K& key)
		{
			QWriteLocker locker(&lock);
			hash.remove(key);
		}
		inline bool contains(const K& key) const
		{
			QReadLocker locker(&lock);
			return hash.contains(key);
		}
		inline V value(const K& key) const
		{
			QReadLocker locker(&lock);
			return hash.value(key);
		}
		inline V value(const K& key, const V& defaultValue) const
		{
			QReadLocker locker(&lock);
			return hash.value(key, defaultValue);
		}

		inline int size() const
		{
			QReadLocker locker(&lock);
			return hash.size();
		}
		inline QList <K> keys() const
		{
			QReadLocker locker(&lock);
			return hash.keys();
		}

		inline QList <V> values() const
		{
			QReadLocker locker(&lock);
			return hash.values();
		}
		//take
		inline V take(const K& key)
		{
			QWriteLocker locker(&lock);
			return hash.take(key);
		}

		inline void clear()
		{
			QWriteLocker locker(&lock);
			hash.clear();
		}

		//=
		inline V& operator[](const K& key)
		{
			QWriteLocker locker(&lock);
			return hash[key];
		}


		inline K key(const V& value) const
		{
			QReadLocker locker(&lock);
			return hash.key(value);
		}

		inline K key(const V& value, const K& defaultValue) const
		{
			QReadLocker locker(&lock);
			return hash.key(value, defaultValue);
		}

		inline typename QHash<K, V>::iterator begin()
		{
			QReadLocker locker(&lock);
			return hash.begin();
		}

		inline typename QHash<K, V>::const_iterator begin() const
		{
			QReadLocker locker(&lock);
			return hash.begin();
		}

		inline typename QHash<K, V>::iterator end()
		{
			QReadLocker locker(&lock);
			return hash.end();
		}

		inline typename QHash<K, V>::const_iterator end() const
		{
			QReadLocker locker(&lock);
			return hash.end();
		}

	private:
		QHash<K, V> hash;
		mutable QReadWriteLock lock;
	};

	static Q_REQUIRED_RESULT QString toUnicode(const char* str, bool ext = true)
	{
		static QTextCodec* codec = QTextCodec::codecForName("GB2312");

		QString qstr = codec->toUnicode(str);
		std::wstring wstr = qstr.toStdWString();
		UINT ACP = ::GetACP();
		if (950 == ACP && ext)
		{
			// 繁體系統地圖名要轉繁體否則遊戲視窗標題會亂碼
			int size = lstrlenW(wstr.c_str());
			QScopedArrayPointer <wchar_t> wbuf(new wchar_t[size + 1]());
			LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_TRADITIONAL_CHINESE, wstr.c_str(), size, wbuf.data(), size, NULL, NULL, NULL);
			qstr = QString::fromWCharArray(wbuf.data());
		}

		return qstr;
	}

	static Q_REQUIRED_RESULT std::string fromUnicode(const QString& str)
	{
		static QTextCodec* codec = QTextCodec::codecForName("GB2312");
		QString qstr = str;
		std::wstring wstr = qstr.toStdWString();
		UINT ACP = ::GetACP();
		if (950 == ACP)
		{
			// 繁體系統要轉回簡體體否則遊戲視窗會亂碼
			int size = lstrlenW(wstr.c_str());
			QScopedArrayPointer <wchar_t> wbuf(new wchar_t[size + 1]());
			LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, LCMAP_SIMPLIFIED_CHINESE, wstr.c_str(), size, wbuf.data(), size, NULL, NULL, NULL);
			wstr = wbuf.data();
			qstr = QString::fromWCharArray(wstr.c_str());
		}

		QByteArray bytes = codec->fromUnicode(qstr);

		std::string s = bytes.data();

		return s;
	}

	template<typename T>
	QList<T*> findWidgets(QWidget* widget)
	{
		QList<T*> widgets;

		if (widget)
		{
			// 檢查 widget 是否為指定類型的控件，如果是，則添加到結果列表中
			T* typedWidget = qobject_cast<T*>(widget);
			if (typedWidget)
			{
				widgets.append(typedWidget);
			}

			// 遍歷 widget 的子控件並遞歸查找
			QList<QWidget*> childWidgets = widget->findChildren<QWidget*>();
			for (QWidget* childWidget : childWidgets)
			{
				// 遞歸調用 findWidgets 函數並將結果合並到 widgets 列表中
				QList<T*> childResult = findWidgets<T>(childWidget);
				widgets += childResult;
			}
		}

		return widgets;
	}

	class Config
	{
	public:
		Config(const QString& fileName);
		~Config();

		bool open(const QString& fileName);
		void sync();

		void removeSec(const QString sec);

		void write(const QString& key, const QVariant& value);
		void write(const QString& sec, const QString& key, const QVariant& value);
		void write(const QString& sec, const QString& key, const QString& sub, const QVariant& value);

		void WriteHash(const QString& sec, const QString& key, QMap<QString, QPair<bool, QString>>& hash);
		QMap<QString, QPair<bool, QString>> EnumString(const QString& sec, const QString& key) const;

		void writeStringList(const QString& key, const QStringList& value);
		void writeStringList(const QString& sec, const QString& key, const QStringList& value);
		void writeStringList(const QString& sec, const QString& key, const QString& sub, const QStringList& value);

		QString readString(const QString& sec, const QString& key, const QString& sub) const;
		QString readString(const QString& sec, const QString& key) const;
		QString readString(const QString& key) const;
		bool readBool(const QString& key) const;
		bool readBool(const QString& sec, const QString& key) const;
		bool readBool(const QString& sec, const QString& key, const QString& sub) const;
		int readInt(const QString& key) const;
		int readInt(const QString& sec, const QString& key) const;
		int readInt(const QString& sec, const QString& key, const QString& sub) const;
		qreal readDouble(const QString& sec, const QString& key, qreal retnot) const;
		int readInt(const QString& sec, const QString& key, int retnot) const;
		QStringList readStringList(const QString& sec, const QString& key, const QString& sub) const;
		QStringList readStringList(const QString& sec, const QString& key) const;

	private:
		QLockFile lock_;

		QJsonDocument document_;

		QString fileName_ = "\0";

		QVariantMap cache_ = {};

		bool hasChanged_ = false;
	};

	class Crypt
	{
	public:
		Crypt()
			:crypto_(new SimpleCrypt(0x7f5f6c3ee5bai64))
		{

		}

		virtual ~Crypt() = default;

		bool isBase64(const QString data) const
		{
			//正則式 + 長度判斷 + 字符集判斷 + 字符串判斷 是否為 Base64
			QRegularExpression rx("^[A-Za-z0-9+/]+={0,2}$");
			return rx.match(data).hasMatch() && data.length() % 4 == 0 && data.contains(QRegularExpression("[^A-Za-z0-9+/=]")) == false;
		}

		/* 加密數據 */
		QString encryptToString(const QString& plaintext)
		{
			QString  ciphertext = crypto_->encryptToString(plaintext);
			//QString str = QEncode(ciphertext);
			return ciphertext;

		}

		QString encryptFromByteArray(QByteArray plaintext)
		{
			QString  ciphertext = crypto_->encryptToString(plaintext);
			//QString str = QEncode(ciphertext);
			return ciphertext;
		}

		/* 解密數據 */
		QString decryptToString(const QString& plaintext)
		{
			//QString str = QDecode(plaintext);
			QString ciphertext = crypto_->decryptToString(plaintext);
			return ciphertext;
		}

		QString decryptFromByteArray(QByteArray plaintext)
		{
			//QString str = QDecode(plaintext);
			QString ciphertext = crypto_->decryptToString(plaintext);
			return ciphertext;
		}

		QByteArray decryptToByteArray(const QString& plaintext)
		{
			//QString str = QDecode(plaintext);
			QByteArray ciphertext = crypto_->decryptToByteArray(plaintext);
			return ciphertext;
		}

	private:
		QScopedPointer<SimpleCrypt> crypto_;

	};

	class FormSettingManager
	{
	public:
		explicit FormSettingManager(QWidget* widget) { widget_ = widget; }
		explicit FormSettingManager(QMainWindow* widget) { mainwindow_ = widget; }
		void loadSettings();
		void saveSettings();

	private:
		QWidget* widget_ = nullptr;
		QMainWindow* mainwindow_ = nullptr;
	};

	static void createMenu(QMenuBar* pMenuBar, QHash<QString, QAction*>* phash = nullptr)
	{
		if (!pMenuBar)
			return;

		auto setMenuStyleSheet = [](QMenu* menu)->void
		{
			menu->setStyleSheet(R"(
				QMenu {
					background-color: rgb(249, 249, 249); /*整個背景*/
					border: 0px;
					/*item寬度*/
					width: 150px;
				
				}
				QMenu::item {
					font-size: 9pt;
					/*color: rgb(225, 225, 225); 字體顏色*/
					border: 2px; solid rgb(249, 249, 249); /*item選框*/
					background-color: rgb(249, 249, 249);
					padding: 10px 10px; /*設置菜單項文字上下和左右的內邊距，效果就是菜單中的條目左右上下有了間隔*/
					margin: 2px 2px; /*設置菜單項的外邊距*/
					/*item高度*/	
					height: 10px;
				}
				QMenu::item:selected {
					background-color: rgb(240, 240, 240); /*選中的樣式*/
					border: 2px solid rgb(249, 249, 249); /*选中状态下的边框*/
				}
				QMenu::item:pressed {
					/*菜單項按下效果
					border: 0px; /*solid rgb(60, 60, 61);*/
					background-color: rgb(50, 130, 246);
				}
			)");
		};

		pMenuBar->setAttribute(Qt::WA_StyledBackground, true);

		pMenuBar->clear();

		QHash<QString, QAction*> hash;
		QMenu* pMenuSystem = nullptr;
		QAction* pActionHide = nullptr;
		QAction* pSeparator = nullptr;
		QAction* pActionInfo = nullptr;
		QAction* pActionWibsite = nullptr;
		QAction* pSeparator2 = nullptr;
		QAction* pActionClose = nullptr;

		QMenu* pMenuOther = nullptr;
		QAction* pActionOtherInfo = nullptr;
		QAction* pSeparator3 = nullptr;
		QAction* pActionSettings = nullptr;

		do
		{
			pMenuSystem = new QMenu(QObject::tr("system"));
			if (!pMenuSystem)
				break;
			setMenuStyleSheet(pMenuSystem);

			pActionHide = new QAction(QObject::tr("hide"), pMenuSystem);
			if (!pActionHide)
				break;
			pActionHide->setObjectName("actionHide");
			hash.insert("actionHide", pActionHide);

			pSeparator = new QAction(pMenuSystem);
			if (!pSeparator)
				break;
			pSeparator->setSeparator(true);

			pActionInfo = new QAction(QObject::tr("info"), pMenuSystem);
			if (!pActionInfo)
				break;
			pActionInfo->setObjectName("actionInfo");
			hash.insert("actionInfo", pActionInfo);

			pActionWibsite = new QAction(QObject::tr("website"), pMenuSystem);
			if (!pActionWibsite)
				break;
			pActionWibsite->setObjectName("actionWibsite");
			hash.insert("actionWibsite", pActionWibsite);

			pSeparator2 = new QAction(pMenuSystem);
			if (!pSeparator2)
				break;
			pSeparator2->setSeparator(true);

			pActionClose = new QAction(QObject::tr("close"), pMenuSystem);
			if (!pActionClose)
				break;
			pActionClose->setObjectName("actionClose");
			hash.insert("actionClose", pActionClose);

			pMenuSystem->addAction(pActionHide);
			pMenuSystem->addAction(pSeparator);
			pMenuSystem->addAction(pActionInfo);
			pMenuSystem->addAction(pActionWibsite);
			pMenuSystem->addAction(pSeparator2);
			pMenuSystem->addAction(pActionClose);

			pMenuBar->addMenu(pMenuSystem);

			pMenuOther = new QMenu(QObject::tr("other"));
			if (!pMenuOther)
				break;
			setMenuStyleSheet(pMenuOther);

			pActionOtherInfo = new QAction(QObject::tr("otherinfo"), pMenuOther);
			if (!pActionOtherInfo)
				break;
			pActionOtherInfo->setObjectName("actionOtherInfo");
			hash.insert("actionOtherInfo", pActionOtherInfo);

			pSeparator3 = new QAction(pMenuOther);
			if (!pSeparator3)
				break;
			pSeparator3->setSeparator(true);

			pActionSettings = new QAction(QObject::tr("script settings"), pMenuOther);
			if (!pActionSettings)
				break;
			pActionSettings->setObjectName("actionScriptSettings");
			hash.insert("actionScriptSettings", pActionSettings);

			pMenuOther->addAction(pActionOtherInfo);
			pMenuOther->addAction(pSeparator3);
			pMenuOther->addAction(pActionSettings);

			pMenuBar->addMenu(pMenuOther);

			if (phash)
				*phash = hash;
			return;
		} while (false);

		if (pMenuSystem)
			delete pMenuSystem;
		if (pActionHide)
			delete pActionHide;
		if (pSeparator)
			delete pSeparator;
		if (pActionInfo)
			delete pActionInfo;
		if (pActionWibsite)
			delete pActionWibsite;
		if (pSeparator2)
			delete pSeparator2;
		if (pActionClose)
			delete pActionClose;
		if (pMenuOther)
			delete pMenuOther;
		if (pActionOtherInfo)
			delete pActionOtherInfo;
		if (pSeparator3)
			delete pSeparator3;
		if (pActionSettings)
			delete pActionSettings;
	}

	static void setTab(QTabWidget* pTab)
	{
		QString styleSheet = R"(
			QTabWidget{
				background-color:rgb(249, 249, 249);
				background-clip:rgb(31, 31, 31);
				background-origin:rgb(31, 31, 31);
			}

			QTabWidget::pane{
				top:0px;
				border-top:2px solid #7160E8;
				border-bottom:1px solid rgb(61,61,61);
				border-right:1px solid rgb(61,61,61);
				border-left:1px solid rgb(61,61,61);
			}

			QTabWidget::tab-bar
			{
				alignment: left;
				top:0px;
				left:5px;
				border:none;
			}

			QTabBar::tab:first{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);

				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);
			}

			QTabBar::tab:middle{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);
				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);

			}

			QTabBar::tab:last{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);

				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);
			}

			QTabBar::tab:selected{
				background:rgb(50, 130, 246);
				border-top:2px solid rgb(50, 130, 246);

			}

			QTabBar::tab:hover{
				background:rgb(50, 130, 246);
				border-top:2px solid rgb(50, 130, 246);
				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;
			}
		)";
		pTab->setStyleSheet(styleSheet);

		QTabBar* pTabBar = pTab->tabBar();

		pTabBar->setDocumentMode(true);
		pTabBar->setExpanding(true);
	}

	typedef enum
	{
		REASON_NO_ERROR,
		//Thread
		REASON_CLOSE_BEFORE_START,
		REASON_PID_INVALID,
		REASON_FAILED_TO_INIT,
		REASON_MEMBERSHIP_INVALID,
		REASON_REQUEST_STOP,
		REASON_TARGET_WINDOW_DISAPPEAR,
		REASON_FAILED_READ_DATA,

		//TCP
		REASON_TCP_PRECONNECTION_TIMEOUT,
		REASON_TCP_CONNECTION_TIMEOUT,
		REASON_TCP_CREATE_SOCKET_FAIL,
		REASON_TCP_IPV6_CONNECT_FAIL,
		REASON_TCP_IPV4_CONNECT_FAIL,
		REASON_TCP_EVENT_SELECT_FAIL,
		REASON_TCP_KEEPALIVE_FAIL,

		//CreateProcess
		REASON_DIRECTORY_EMPTY,
		REASON_ARGUMENT_EMPTY,
		REASON_IMAGE_PATH_NOFOUND,
		REASON_EXECUTABLE_FILE_PATH_EMPTY,
		REASON_FAILED_TO_START,

		//Injection
		REASON_ENUM_WINDOWS_FAIL,
		REASON_HOOK_DLL_SHA512_FAIL,
		REASON_GET_KERNEL32_FAIL,
		REASON_GET_KERNEL32_UNDOCUMENT_API_FAIL,
		REASON_OPEN_PROCESS_FAIL,
		REASON_VIRTUAL_ALLOC_FAIL,
		REASON_INJECT_LIBRARY_FAIL,
		REASON_CLOSE_MUTEX_FAIL,
		REASON_REMOTE_LIBRARY_INIT_FAIL,
	}REMOVE_THREAD_REASON, * LPREMOVE_THREAD_REASON;

	class VMemory
	{
	public:
		typedef enum
		{
			V_ANSI,
			V_UNICODE,
		}VENCODE;
		VMemory() = default;

		explicit VMemory(HANDLE h, int size, bool autoclear)
			: autoclear(autoclear)
			, hProcess(h)
		{

			lpAddress = mem::virtualAlloc(h, size);
		}

		explicit VMemory(HANDLE h, const QString& str, VENCODE use_unicode, bool autoclear)
			: autoclear(autoclear)
		{

			lpAddress = (use_unicode) == VMemory::V_UNICODE ? (mem::virtualAllocW(h, str)) : (mem::virtualAllocA(h, str));
			hProcess = h;
		}

		operator int() const
		{
			return this->lpAddress;
		}

		VMemory& operator=(int other)
		{
			this->lpAddress = other;
			return *this;
		}

		VMemory* operator&()
		{
			return this;
		}

		// copy constructor
		VMemory(const VMemory& other)
			: autoclear(other.autoclear)
			, lpAddress(other.lpAddress)
		{
		}
		//copy assignment
		VMemory& operator=(const VMemory& other)
		{
			this->lpAddress = other.lpAddress;
			this->autoclear = other.autoclear;
			this->hProcess = other.hProcess;
			return *this;
		}
		// move constructor
		VMemory(VMemory&& other) noexcept
			: autoclear(other.autoclear)
			, lpAddress(other.lpAddress)
			, hProcess(other.hProcess)

		{
			other.lpAddress = 0;
		}
		// move assignment
		VMemory& operator=(VMemory&& other) noexcept
		{
			this->lpAddress = other.lpAddress;
			this->autoclear = other.autoclear;
			this->hProcess = other.hProcess;
			other.lpAddress = 0;
			return *this;
		}

		friend constexpr inline bool operator==(const VMemory& p1, const VMemory& p2)
		{
			return p1.lpAddress == p2.lpAddress;
		}
		friend constexpr inline bool operator!=(const VMemory& p1, const VMemory& p2)
		{
			return p1.lpAddress != p2.lpAddress;
		}

		virtual ~VMemory()
		{
			do
			{
				if ((autoclear) && (this->lpAddress) && (hProcess))
				{

					mem::writeInt(hProcess, this->lpAddress, 0, 2);
					mem::virtualFree(hProcess, this->lpAddress);
					this->lpAddress = NULL;
				}
			} while (false);
		}

		Q_REQUIRED_RESULT inline bool __vectorcall isNull() const
		{
			return !lpAddress;
		}
		inline void __vectorcall clear()
		{

			if ((this->lpAddress))
			{
				mem::virtualFree(hProcess, (this->lpAddress));
				this->lpAddress = NULL;
			}
		}

		Q_REQUIRED_RESULT inline bool __vectorcall isValid()const
		{
			return (this->lpAddress) != NULL;
		}

	private:
		bool autoclear = false;
		int lpAddress = NULL;
		HANDLE hProcess = NULL;
	};

#pragma warning(push)
#pragma warning(disable:304)
	Q_DECLARE_METATYPE(VMemory);
	Q_DECLARE_TYPEINFO(VMemory, Q_MOVABLE_TYPE);
#pragma warning(pop)

	enum UnLoginStatus
	{
		kStatusUnknown,
		kStatusInputUser,
		kStatusSelectServer,
		kStatusSelectSubServer,
		kStatusSelectCharacter,
		kStatusLogined,
		kStatusDisconnect,
		kStatusTimeout,
		kNoUserNameOrPassword,
		kStatusBusy,
	};

	enum UserSetting
	{
		kSettingNotUsed = 0,

		///////////////////

		kSettingMinValue,

		kServerValue,
		kSubServerValue,
		kPositionValue,
		kLockTimeValue,

		//afk->walk
		kAutoWalkDelayValue,
		kAutoWalkDistanceValue,
		kAutoWalkDirectionValue,

		kSpeedBoostValue,

		kSettingMaxValue,

		///////////////////

		kSettingMinEnable,

		kAutoLoginEnable,
		kAutoReconnectEnable,
		kLogOutEnable,
		kLogBackEnable,
		kEchoEnable,
		kHideCharacterEnable,
		kCloseEffectEnable,
		kOptimizeEnable,
		kHideWindowEnable,
		kMuteEnable,
		kAutoJoinEnable,
		kLockTimeEnable,
		kAutoFreeMemoryEnable,
		kFastWalkEnable,
		kPassWallEnable,
		kLockMoveEnable,
		kLockImageEnable,
		kAutoDropMeatEnable,
		kAutoDropEnable,
		kAutoStackEnable,
		kKNPCEnable,
		kAutoAnswerEnable,
		kForceLeaveBattleEnable,
		kAutoWalkEnable,
		kFastAutoWalkEnable,
		kFastBattleEnable,
		kAutoBattleEnable,
		kAutoCatchEnable,
		kLockAttackEnable,
		kAutoEscapeEnable,
		kLockEscapeEnable,
		kBattleTimeExtendEnable,
		kFallDownEscapeEnable,

		//switcher
		kSwitcherTeamEnable,
		kSwitcherPKEnable,
		kSwitcherCardEnable,
		kSwitcherTradeEnable,
		kSwitcherFamilyEnable,
		kSwitcherJobEnable,
		kSwitcherWorldEnable,


		kSettingMaxEnable,

		//////////////////

		kSettingMinString,

		kAutoDropItemString,
		kLockAttackString,
		kLockEscapeString,

		kGameAccountString,
		kGamePasswordString,
		kGameSecurityCodeString,


		kSettingMaxString,
	};


	//用於將枚舉直轉換為字符串，提供給json當作key
	static const QHash<UserSetting, QString> user_setting_string_hash = {
		{ kSettingNotUsed, "SettingNotUsed" },

		{ kSettingMinValue, "SettingMinValue" },

		{ kServerValue, "ServerValue" },
		{ kSubServerValue, "SubServerValue" },
		{ kPositionValue, "PositionValue" },
		{ kLockTimeValue, "LockTimeValue" },
		{ kSpeedBoostValue, "SpeedBoostValue" },

		//afk->walk
		{ kAutoWalkDelayValue, "AutoWalkDelayValue" },
		{ kAutoWalkDistanceValue, "AutoWalkDistanceValue" },
		{ kAutoWalkDirectionValue, "AutoWalkDirectionValue" },

		{ kSettingMaxValue, "SettingMaxValue" },

		{ kSettingMinEnable, "SettingMinEnable" },
		{ kAutoLoginEnable, "AutoLoginEnable" },
		{ kAutoReconnectEnable, "AutoReconnectEnable" },
		{ kLogOutEnable, "LogOutEnable" },
		{ kLogBackEnable, "LogBackEnable" },
		{ kEchoEnable, "EchoEnable" },
		{ kHideCharacterEnable, "HideCharacterEnable" },
		{ kCloseEffectEnable, "CloseEffectEnable" },
		{ kOptimizeEnable, "OptimizeEnable" },
		{ kHideWindowEnable, "HideWindowEnable" },
		{ kMuteEnable, "MuteEnable" },
		{ kAutoJoinEnable, "AutoJoinEnable" },
		{ kLockTimeEnable, "LockTimeEnable" },
		{ kAutoFreeMemoryEnable, "AutoFreeMemoryEnable" },
		{ kFastWalkEnable, "FastWalkEnable" },
		{ kPassWallEnable, "PassWallEnable" },
		{ kLockMoveEnable, "LockMoveEnable" },
		{ kLockImageEnable, "LockImageEnable" },
		{ kAutoDropMeatEnable, "AutoDropMeatEnable" },
		{ kAutoDropEnable, "AutoDropEnable" },
		{ kAutoStackEnable, "AutoStackEnable" },
		{ kKNPCEnable, "KNPCEnable" },
		{ kAutoAnswerEnable, "AutoAnswerEnable" },
		{ kForceLeaveBattleEnable, "ForceLeaveBattleEnable" },
		{ kAutoWalkEnable, "AutoWalkEnable" },
		{ kFastAutoWalkEnable, "FastAutoWalkEnable" },
		{ kFastBattleEnable, "FastBattleEnable" },
		{ kAutoBattleEnable, "AutoBattleEnable" },
		{ kAutoCatchEnable, "AutoCatchEnable" },
		{ kLockAttackEnable, "LockAttackEnable" },
		{ kAutoEscapeEnable, "AutoEscapeEnable" },
		{ kLockEscapeEnable, "LockEscapeEnable" },
		{ kBattleTimeExtendEnable, "BattleTimeExtendEnable" },
		{ kFallDownEscapeEnable, "FallDownEscapeEnable" },
		{ kSettingMaxEnable, "SettingMaxEnable" },

		//switcher
		{ kSwitcherTeamEnable, "SwitcherTeamEnable" },
		{ kSwitcherPKEnable, "SwitcherPKEnable" },
		{ kSwitcherCardEnable, "SwitcherCardEnable" },
		{ kSwitcherTradeEnable, "SwitcherTradeEnable" },
		{ kSwitcherFamilyEnable, "SwitcherFamilyEnable" },
		{ kSwitcherJobEnable, "SwitcherJobEnable" },
		{ kSwitcherWorldEnable, "SwitcherWorldEnable" },

		{ kSettingMinString, "SettingMinString" },

		{ kAutoDropItemString, "AutoDropItemString" },
		{ kLockAttackString, "LockAttackString" },
		{ kLockEscapeString, "LockEscapeString" },

		{ kGameAccountString, "GameAccountString" },
		{ kGamePasswordString, "GamePasswordString" },
		{ kGameSecurityCodeString, "GameSecurityCodeString" },

		{ kSettingMaxString, "SettingMaxString" }
	};

	enum UserStatus
	{
		kLabelStatusNotUsed = 0,//未知
		kLabelStatusNotOpen,//未開啟石器
		kLabelStatusOpening,//開啟石器中
		kLabelStatusOpened,//已開啟石器
		kLabelStatusLogining,//登入
		kLabelStatusSignning,//簽入中
		kLabelStatusSelectServer,//選擇伺服器
		kLabelStatusSelectSubServer,//選擇分伺服器
		kLabelStatusGettingPlayerList,//取得人物中
		kLabelStatusSelectPosition,//選擇人物中
		kLabelStatusLoginSuccess,//登入成功
		kLabelStatusInNormal,//平時
		kLabelStatusInBattle,//戰鬥中
		kLabelStatusBusy,//忙碌中
		kLabelStatusTimeout,//連線逾時
		kLabelNoUserNameOrPassword,//無帳號密碼
		kLabelStatusDisconnected,//斷線
	};


	enum UserData
	{
		kUserDataNotUsed = 0,
		kUserItemNames,
		kUserEnemyNames,
	};

	static bool createFileDialog(const QString& name, QString* retstring, QWidget* parent)
	{
		QScopedPointer<QFileDialog> dialog(new QFileDialog(parent));
		if (dialog.isNull())
		{
			return false;
		}

		dialog->setModal(true);
		dialog->setAttribute(Qt::WA_QuitOnClose);
		dialog->setFileMode(QFileDialog::ExistingFile);
		dialog->setViewMode(QFileDialog::Detail);
		dialog->setOption(QFileDialog::ReadOnly, true);
		dialog->setOption(QFileDialog::DontResolveSymlinks, true);
		dialog->setOption(QFileDialog::DontConfirmOverwrite, true);
		dialog->setAcceptMode(QFileDialog::AcceptOpen);


		//check suffix
		if (!name.isEmpty())
		{
			QStringList filters;
			filters << name;
			dialog->setNameFilters(filters);
		}

		//directory
		//自身目錄往上一層
		QString directory = QCoreApplication::applicationDirPath();
		directory = QDir::toNativeSeparators(directory);
		directory = QDir::cleanPath(directory + QDir::separator() + "..");
		dialog->setDirectory(directory);

		if (dialog->exec() == QDialog::Accepted)
		{
			QStringList fileNames = dialog->selectedFiles();
			if (fileNames.size() > 0)
			{
				QString fileName = fileNames.at(0);
				if (retstring)
					*retstring = fileName;

				return true;
			}
		}
		return false;
	}

	static int safe_memcmp(const void* s1, size_t len1, const void* s2, size_t len2)
	{
		if (!s1 || !s2)
		{
			return INT_MIN;
		}

		size_t min_len = (len1 < len2) ? len1 : len2;

		const unsigned char* p1 = (const unsigned char*)s1;
		const unsigned char* p2 = (const unsigned char*)s2;

		int result = 0;
		try
		{
			for (size_t i = 0; i < min_len && result == 0; i++)
			{
				result = p1[i] - p2[i];
			}

			if (result == 0)
			{
				if (len1 < len2)
				{
					return -1;
				}
				else if (len1 > len2)
				{
					return 1;
				}
				else
				{
					return 0;
				}
			}
			else
			{
				return result;
			}
		}
		catch (...)
		{
			return INT_MIN;
		}
	}

	static char* safe_strstr(char* src, size_t srclen, const char* target, size_t targetlen)
	{
		if (srclen < targetlen || !src || !target)
		{
			return nullptr;
		}

		try
		{
			for (size_t i = 0; i <= srclen - targetlen; ++i)
			{
				if (safe_memcmp(src + i, targetlen, target, targetlen) == 0)
				{
					return src + i;
				}
			}
		}
		catch (...)
		{
			return nullptr;
		}

		return nullptr;
	}

	static int safe_atoi(const char* str)
	{
		if (!str)
		{
			return 0;
		}
		long result = 0ul;
		try
		{
			char* end;
			result = strtol(str, &end, 10);

			if (end == str || *end != '\0')
			{
				// invalid conversion
				return 0;
			}
		}
		catch (...)
		{
			return 0;
		}

		if (result > INT_MAX || result < INT_MIN)
		{
			// overflow or underflow
			return 0;
		}

		return static_cast<int>(result);
	}
}

