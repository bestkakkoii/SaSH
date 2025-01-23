#include "stdafx.h"
#include <tool.h>
#include <timer.h>

QString tool::applicationFilePath()
{
	static bool init = false;
	if (!init)
	{
		WCHAR buffer[MAX_PATH] = {};
		DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		if (length == 0)
			return "";

		QString path(QString::fromWCharArray(buffer, length));
		if (path.size() <= 0)
			return "";

		path.replace("\\", "/");

		qputenv("_CURRENT_PATH", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("_CURRENT_PATH"));
}

QString tool::applicationDirPath()
{
	static bool init = false;
	if (!init)
	{
		QString path(applicationFilePath());
		if (path.size() <= 0)
			return "";

		QFileInfo fileInfo(path);
		path = fileInfo.absolutePath();
		path.replace("\\", "/");
		qputenv("_CURRENT_DIR", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("_CURRENT_DIR"));
}

QString tool::applicationName()
{
	static bool init = false;
	if (!init)
	{
		QString path(applicationFilePath());
		if (path.size() <= 0)
			return "";

		path.replace("\\", "/");

		QFileInfo fileInfo(path);
		path = fileInfo.fileName();

		qputenv("_CURRENT_NAME", path.toUtf8());
		init = true;
	}
	return QString::fromUtf8(qgetenv("_CURRENT_NAME"));
}

QString tool::applicationNameNoSuffix()
{
	static bool init = false;
	if (!init)
	{
		QString path(applicationFilePath());
		if (path.size() <= 0)
			return "";

		path.replace("\\", "/");

		QFileInfo fileInfo(path);
		QString suffix = fileInfo.suffix();
		if (suffix.size() > 0)
			suffix = "." + suffix;

		path = fileInfo.fileName();
		path.remove(suffix);

		qputenv("_CURRENT_NAME_NO_SUFFIX", path.toUtf8());
		init = true;
	}

	return QString::fromUtf8(qgetenv("_CURRENT_NAME_NO_SUFFIX"));
}

QString tool::generateDateTimeToken()
{
	QDateTime currentDateTime = QDateTime::currentDateTime();
	qint64 year = currentDateTime.date().year();
	qint64 month = currentDateTime.date().month();
	qint64 day = currentDateTime.date().day();
	qint64 hour = currentDateTime.time().hour();
	qint64 minute = currentDateTime.time().minute();
	qint64 second = currentDateTime.time().second();

	// 進行位運算疊加，分成兩個32位整數
	quint32 part1 = ((year & 0xFFFF) << 16) | ((month & 0xFF) << 8) | (day & 0xFF);
	quint32 part2 = ((hour & 0xFF) << 16) | ((minute & 0xFF) << 8) | (second & 0xFF);

	// 轉換為小寫十六進制字符串，並將兩部分連接起來
	QString token = QString("%1%2").arg(QString::number(part1, 16).toLower()).arg(QString::number(part2, 16).toLower());
	return token;
}


qint64 tool::getPercentage(qint64 currentValue, qint64 maxValue)
{
	if ((0LL == currentValue) || (0LL == maxValue))
	{
		return 0LL;
	}

	const qreal percentage = (static_cast<qreal>(currentValue) / (maxValue)) * 100.0;
	const qint64 roundedPercentage = static_cast<qint64>(std::floor(percentage));

	if ((0LL == (roundedPercentage)) && ((percentage) > 0.0))
	{
		return 1LL;
	}

	return roundedPercentage;
}


namespace
{
	struct WindowData
	{
		DWORD processId;
		HWND windowHandle;
	};

	static bool isCurrentWindow(const HWND& handle)
	{
		return (GetWindow(handle, GW_OWNER) == nullptr) && (IsWindowVisible(handle) == TRUE);
	}

	static BOOL CALLBACK EnumWindowsCallback(HWND windowHandle, LPARAM lParam)
	{
		WindowData* data = reinterpret_cast<WindowData*>(lParam);

		DWORD processId = 0;
		GetWindowThreadProcessId(windowHandle, &processId);
		if (data->processId != processId || !isCurrentWindow(windowHandle))
			return TRUE;

		data->windowHandle = windowHandle;
		return FALSE;
	}

	static HWND findWindow(const DWORD& processId)
	{
		WindowData data = {};
		data.processId = processId;
		data.windowHandle = 0;

		EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));

		return data.windowHandle;
	}
}

void tool::terminateCurrentProcess()
{
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
}

HWND tool::getCurrentWindowHandle()
{
	const DWORD processId = _getpid();

	HWND hwnd = findWindow(processId);

	return hwnd;
}

HWND tool::getWindowHandle(DWORD processId)
{
	Timer timer;
	HWND windowHandle = nullptr;
	for (;;)
	{
		if (timer.hasExpired(30000))
		{
			break;
		}

		windowHandle = findWindow(processId);
		if (windowHandle != nullptr)
		{
			break;
		}

		QThread::msleep(100);
	}

	return windowHandle;
}

QString tool::formatTime(qint64 milliseconds, const QString& format)
{
	QTime time(0, 0);
	time = time.addMSecs(milliseconds);
	return  time.toString(format);
}

//#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
//DWORD tool::getParentProcessId(DWORD processId)
//{
//	ScopedProcessHandle processHandle(processId);
//
//	MINT::PROCESS_BASIC_INFORMATION pbi = {};
//	MINT::NTSTATUS status = NtQueryInformationProcess(processHandle, MINT::ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);
//	if (!NT_SUCCESS(status))
//	{
//		return 0;
//
//	}
//
//	return static_cast<DWORD>(reinterpret_cast<quint64>(pbi.InheritedFromUniqueProcessId));
//}
//#endif

qint64 string::base62ToDecimal(const QString& a)
{
	qint64 ret = 0;
	qint64 sign = 1;
	qint64 size = a.length();
	for (qint64 i = 0; i < size; ++i)
	{
		try
		{
			ret *= 62;
			if ('0' <= a.at(i) && a.at(i) <= '9')
				ret += static_cast<qint64>(a.at(i).unicode()) - '0';
			else if ('a' <= a.at(i) && a.at(i) <= 'z')
				ret += static_cast<qint64>(a.at(i).unicode()) - 'a' + 10;
			else if ('A' <= a.at(i) && a.at(i) <= 'Z')
				ret += static_cast<qint64>(a.at(i).unicode()) - 'A' + 36;
			else if (a.at(i) == '-')
				sign = -1;
			else
				return 0;
		}
		catch (...)
		{
			return 0;
		}
	}

	return ret * sign;
}

QString string::decimalToBase62(qint64 value)
{
	if (value == 0)
	{
		return "0";
	}

	constexpr const char base62Chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	QString result;
	bool isNegative = value < 0;

	if (isNegative)
	{
		// 处理最小负数的特殊情况
#ifdef min
#undef min
#endif
		if (value == std::numeric_limits<qint64>::min())
		{
			value = -(value + 1);
			qint64 remainder = value % 62 + 1;
			result.prepend(base62Chars[remainder]);
			value /= 62;
		}
		else
		{
			value = -value;
		}
	}

	while (value > 0)
	{
		qint64 remainder = value % 62;
		result.prepend(base62Chars[remainder]);
		value /= 62;
	}

	if (isNegative)
	{
		result.prepend('-');
	}

	return result;
}

qreal tool::fpsToMs(qreal fps)
{
	return 1000.0 / fps;
}

void tool::clrscr()
{
	COORD coordScreen = { 0, 0 };
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hConsole, &csbi);
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacterA(hConsole, ' ', dwConSize, coordScreen, &cCharsWritten);
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	SetConsoleCursorPosition(hConsole, coordScreen);
}

void tool::clearScreenButKeepFirstLine()
{
	// 獲取控制台句柄
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// 獲取控制台屏幕緩衝區信息
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);

	// 獲取控制台大小（總字符數）
	DWORD dwConSize = csbi.dwSize.X * (csbi.dwSize.Y - 1); // 減去第一行

	// 定義從第二行開始的位置
	COORD coordSecondLine = { 0, 1 };
	DWORD cCharsWritten;

	// 用空格填充第二行及之後的所有行
	FillConsoleOutputCharacterA(hConsole, ' ', dwConSize, coordSecondLine, &cCharsWritten);

	// 再次獲取控制台屏幕緩衝區信息
	GetConsoleScreenBufferInfo(hConsole, &csbi);

	// 填充屬性
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordSecondLine, &cCharsWritten);

	// 將光標位置設置回第二行
	SetConsoleCursorPosition(hConsole, coordSecondLine);
}

#if 0
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#pragma comment(lib, "wbemuuid.lib")
//@备注 获取硬件序列号
//@参数 硬件类型
//@返回 序列号列表
//@别名 取硬件序列号(硬件类型)
QStringList tool::getHardwareSerialNumber(HardwareType type)
{
	QStringList serialNumber;
	HRESULT hres;

	hres = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hres) && hres != 0x80010106)
	{
		return serialNumber;
	}

	hres = CoInitializeSecurity(
		nullptr,
		-1,                          // COM 身份验证
		nullptr,                   // 身份验证服务
		nullptr,                   // 保留
		RPC_C_AUTHN_LEVEL_DEFAULT,   // 默认身份验证 
		RPC_C_IMP_LEVEL_IMPERSONATE, // 默认模拟 
		nullptr,                   // 身份验证信息
		EOAC_NONE,                   // 附加功能
		nullptr                    // 保留
	);

	IWbemLocator* pLoc = nullptr;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres))
	{
		CoUninitialize();
		return serialNumber;
	}

	IWbemServices* pSvc = nullptr;

	// connect to WMI
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // WMI 命名空间
		nullptr,               // 用户名
		nullptr,               // 密码
		0,                       // 语言区域设置
		NULL,                    // 安全标志
		0,                       // 权限
		0,                       // 上下文对象 
		&pSvc                    // IWbemServices 代理
	);

	if (FAILED(hres))
	{
		pLoc->Release();
		CoUninitialize();
		return serialNumber;
	}

	hres = CoSetProxyBlanket(
		pSvc,                        // 代理
		RPC_C_AUTHN_WINNT,           // 身份验证服务
		RPC_C_AUTHZ_NONE,            // 授权服务
		nullptr,                   // 服务器主体名称 
		RPC_C_AUTHN_LEVEL_CALL,      // 身份验证级别
		RPC_C_IMP_LEVEL_IMPERSONATE, // 模拟级别
		nullptr,                   // 身份验证信息
		EOAC_NONE                    // 代理功能
	);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return serialNumber;
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	std::wstring queryLanguage = L"WQL";
	std::wstring queryString;
	std::vector<std::wstring> key = {};

	switch (type)
	{
	case HardwareType::BIOS:
		queryString = L"SELECT SerialNumber FROM Win32_BIOS";
		key.push_back(L"SerialNumber");
		break;
	case HardwareType::Motherboard:
		queryString = L"SELECT SerialNumber FROM Win32_BaseBoard";
		key.push_back(L"SerialNumber");
		break;
	case HardwareType::HardDrive:
		//名称 和 编号
		queryString = L"SELECT SerialNumber FROM Win32_DiskDrive";
		key.push_back(L"SerialNumber");
		break;
	case HardwareType::CPU:
		queryString = L"SELECT ProcessorId FROM Win32_Processor";
		key.push_back(L"ProcessorId");
		break;
	case HardwareType::GraphicsCard:
		queryString = L"SELECT DeviceID, Name FROM Win32_VideoController";
		key.push_back(L"DeviceID");
		key.push_back(L"Name");
		break;
	case HardwareType::NetworkAdapter:
		queryString = L"SELECT Caption, MACAddress FROM Win32_NetworkAdapter";
		key.push_back(L"Caption");
		key.push_back(L"MACAddress");
		break;
	case HardwareType::PhysicalMemory:
		queryString = L"SELECT Capacity, Speed, Manufacturer FROM Win32_PhysicalMemory";
		key.push_back(L"Capacity");
		key.push_back(L"Speed");
		key.push_back(L"Manufacturer");
		break;
	case HardwareType::SoundDevice:
		queryString = L"SELECT Name, Manufacturer FROM Win32_SoundDevice";
		key.push_back(L"Name");
		key.push_back(L"Manufacturer");
		break;
	case HardwareType::OperatingSystem:
		queryString = L"SELECT Caption, Version, Manufacturer FROM Win32_OperatingSystem";
		key.push_back(L"Caption");
		key.push_back(L"Version");
		key.push_back(L"Manufacturer");
		break;
	default:
		break;
	}

	hres = pSvc->ExecQuery(
		bstr_t(queryLanguage.c_str()),
		bstr_t(queryString.c_str()),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		nullptr,
		&pEnumerator);

	if (FAILED(hres))
	{
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return serialNumber;
	}

	IWbemClassObject* pclsObj = nullptr;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		for (const auto& k : key)
		{
			std::wstring value;
			VARIANT vtProp;
			VariantInit(&vtProp);
			hr = pclsObj->Get(k.c_str(), 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr))
			{
				if (vtProp.vt == VT_BSTR)
				{
					value = vtProp.bstrVal;
				}
				else if (vtProp.vt == VT_I4)
				{
					value = std::to_wstring(vtProp.lVal);
				}
				else if (vtProp.vt == VT_UI4)
				{
					value = std::to_wstring(vtProp.ulVal);
				}
				else if (vtProp.vt == VT_I8)
				{
					value = std::to_wstring(vtProp.llVal);
				}
				else if (vtProp.vt == VT_UI8)
				{
					value = std::to_wstring(vtProp.ullVal);
				}
				else if (vtProp.vt == VT_R8)
				{
					value = std::to_wstring(vtProp.dblVal);
				}
				else if (vtProp.vt == VT_R4)
				{
					value = std::to_wstring(vtProp.fltVal);
				}
				else if (vtProp.vt == VT_BOOL)
				{
					value = vtProp.boolVal ? L"true" : L"false";
				}
				else if (vtProp.vt == VT_DATE)
				{
					value = std::to_wstring(vtProp.date);
				}
				else if (vtProp.vt == VT_NULL)
				{
					value = L"";
				}
				else
				{
					value = L"";
				}
			}
			VariantClear(&vtProp);

			if (static_cast<qint64>(value.size()) > 0)
			{
				serialNumber.append(QString::fromStdWString(value));
			}
		}

		pclsObj->Release();
	}

	pEnumerator->Release();
	pSvc->Release();
	pLoc->Release();
	CoUninitialize();

	return serialNumber;
}

QString tool::getMachineCode()
{
	QString machineCode;
	QStringList serialNumber;
	serialNumber.append(getHardwareSerialNumber(HardwareType::BIOS));
	serialNumber.append(getHardwareSerialNumber(HardwareType::Motherboard));
	//serialNumber.append(getHardwareSerialNumber(HardwareType::HardDrive));
	serialNumber.append(getHardwareSerialNumber(HardwareType::CPU));
	//serialNumber.append(getHardwareSerialNumber(HardwareType::GraphicsCard));
	//serialNumber.append(getHardwareSerialNumber(HardwareType::NetworkAdapter));
	//serialNumber.append(getHardwareSerialNumber(HardwareType::PhysicalMemory));
	//serialNumber.append(getHardwareSerialNumber(HardwareType::SoundDevice));
	//serialNumber.append(getHardwareSerialNumber(HardwareType::OperatingSystem));
	serialNumber.removeDuplicates();
	serialNumber.removeAll("");

	for (const QString& s : serialNumber)
	{
		machineCode.append(s.simplified().remove(" "));
	}

	//計算機器碼哈希值
	QByteArray byteArray = machineCode.toUtf8();

	QCryptographicHash hashDevice(QCryptographicHash::Blake2b_512);
	hashDevice.addData(byteArray);			  // 添加數據到加密哈希值

	QByteArray result = hashDevice.result();  // 返回最終的哈希值
	machineCode = result.toHex().toLower();

	return machineCode;
}
#endif
#endif

QByteArray tool::toHash(const QByteArray& data, QCryptographicHash::Algorithm algorithm)
{
	QCryptographicHash hash(algorithm);
	hash.addData(data);
	return hash.result().toHex().toLower();
}

QString tool::getWindowsSDKVersion()
{
	QProcess process;
	process.start("powershell", QStringList() << "-Command" << "[System.Environment]::GetEnvironmentVariable('WindowsSdkVerBinPath')");
	process.waitForFinished();
	QString sdkPath = process.readAllStandardOutput().trimmed();

	if (sdkPath.size() <= 0)
	{
		return QString();
	}

	// 使用正則表達式提取版本號
	static const QRegularExpression regex(R"(\\(\d+\.\d+\.\d+)\b)");
	QRegularExpressionMatch match = regex.match(sdkPath);

	if (match.hasMatch())
	{
		return match.captured(1);
	}

	return QString();
}


QString string::toPercentEncoding(const QString& src)
{
	QByteArray utf8Src = src.toUtf8();
	QByteArray encoded = utf8Src.toPercentEncoding();
	QString percentEncoded = QString::fromUtf8(encoded);
	percentEncoded.replace(".", "%2E");

	return percentEncoded;
}

QString string::toPercentEncoding(const QByteArray& src)
{
	QByteArray encoded = src.toPercentEncoding();
	QString percentEncoded = QString::fromUtf8(encoded);
	percentEncoded.replace(".", "%2E");


	return percentEncoded;

}

QString string::encryptUrl(const QString& url)
{
	QString encryptedUrl;
	for (QChar ch : url) {
		if (ch == ':') {
			encryptedUrl.append("%3A");
		}
		else if (ch == '/') {
			encryptedUrl.append("%2F");
		}
		else {
			encryptedUrl.append(ch);
		}
	}
	return encryptedUrl;
}

QString string::toIndentedJson(const QString& objectString)
{
	QJsonParseError error;
	QJsonDocument jsonDocument = QJsonDocument::fromJson(objectString.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError)
	{
		return objectString;
	}

	return QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Indented));
}

bool string::tryFuzzyMatch(const QString& source, const QString& target, const QString& extraTarget)
{
	// 檢查是否有一個字符串為空
	if (source.size() <= 0 || target.size() <= 0)
	{
		return false;
	}

	// 如果兩個字符串相等,返回true
	if (source == target)
	{
		return true;
	}

	// 如果目標字符串以問號開頭,進行包含式比較
	if (target.startsWith(u'?'))
	{
		return source.contains(target.mid(1), Qt::CaseInsensitive);
	}

	if (target.startsWith(u'+') && extraTarget.size() > 0)
	{
		return source.contains(extraTarget, Qt::CaseInsensitive);
	}

	// 如果以上條件都不滿足,返回false
	return false;
}

quint16 string::generate32BitMD5Hash(const QString& input)
{
	// 將輸入字串轉換為 QByteArray
	QByteArray byteArray = input.toUtf8();

	// 使用 MD5 哈希函數生成哈希
	QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Md5);

	// 壓縮計算: 將所有字節進行 XOR 運算，生成一個 16 位的哈希值
	quint16 result = 0;
	for (qint64 i = 0; i < hash.size(); ++i)
	{
		result ^= static_cast<quint8>(hash.at(i));
	}

	return result;
}

#pragma region JsonReply
tool::JsonReply::JsonReply(const QString& action, const QString& message)
	: action_(action)
	, message_(message)
{
	jsonObject_.insert("action", action_);
	if (message.size() > 0)
	{
		dataObject_.insert("message", message_);
	}

	if (!jsonObject_.contains("conversationId"))
	{
		jsonObject_.insert("conversationId", "server");
	}

	jsonDocument_.setObject(jsonObject_);
}

tool::JsonReply::JsonReply(const QString& action)
	: action_(action)
{
	jsonObject_.insert("action", action_);

	if (!jsonObject_.contains("conversationId"))
	{
		jsonObject_.insert("conversationId", "server");
	}

	jsonDocument_.setObject(jsonObject_);
}

tool::JsonReply::JsonReply(const QJsonDocument& jsonDocument)
	: jsonDocument_(jsonDocument)
	, jsonObject_(jsonDocument_.object())
	, message_(jsonObject_.value("message").toString() != "" ? jsonObject_.value("message").toString() : dataObject_.value("message").toString())
	, action_(jsonObject_.value("action").toString())
	, dataObject_(jsonObject_.value("data").toObject())
	, parametersObject_(dataObject_.value("parameters").toObject())
{
	if (!jsonObject_.contains("conversationId"))
	{
		jsonObject_.insert("conversationId", "server");
	}

	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::setStatus(bool status)
{
	insert("status", status ? "success" : "failure");
}

void tool::JsonReply::setConversationId(const QString& conversationId)
{
	insert("conversationId", conversationId);
}

void tool::JsonReply::setAction(const QString& action)
{
	action_ = action;
	jsonObject_.insert("action", action_);
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::setCommand(const QString& command)
{
	insertData("command", command);
}

void tool::JsonReply::setMessage(const QString& message)
{
	message_ = message;
	insertData("message", message);
}

void tool::JsonReply::setField(const QString& field)
{
	insertParameter("field", field);
}

void tool::JsonReply::setProcessId(qint64 processId)
{
	insertParameter("processId", processId);
}

void tool::JsonReply::setUsername(const QString& username)
{
	insertParameter("username", username);
}

void tool::JsonReply::insert(const QString& key, const QVariant& value)
{
	jsonObject_.insert(key, QJsonValue::fromVariant(value));
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::insertData(const QString& key, const  QVariant& value)
{
	dataObject_.insert(key, QJsonValue::fromVariant(value));
	jsonObject_.insert("data", dataObject_);
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::setData(const QJsonObject& jsonData)
{
	dataObject_ = jsonData;
	jsonObject_.insert("data", dataObject_);
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::insertParameter(const QString& key, const QVariant& value)
{
	parametersObject_.insert(key, QJsonValue::fromVariant(value));
	dataObject_.insert("parameters", parametersObject_);
	jsonObject_.insert("data", dataObject_);
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::setParameters(const QJsonObject& jsonParameters)
{
	parametersObject_ = jsonParameters;
	dataObject_.insert("parameters", parametersObject_);
	jsonObject_.insert("data", dataObject_);
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::appendParameters(const QJsonObject& jsonParameters)
{
	for (auto it = jsonParameters.begin(); it != jsonParameters.end(); ++it)
	{
		parametersObject_.insert(it.key(), it.value());
	}

	dataObject_.insert("parameters", parametersObject_);
	jsonObject_.insert("data", dataObject_);
	jsonDocument_.setObject(jsonObject_);
}

void tool::JsonReply::setTimestamp(qint64 timestamp)
{
	insert("timestamp", timestamp);
}

QString tool::JsonReply::toString() const
{
	return QString::fromUtf8(jsonDocument_.toJson(QJsonDocument::Compact));
}

QByteArray tool::JsonReply::toUtf8() const
{
	return jsonDocument_.toJson(QJsonDocument::Compact);
}
#pragma endregion