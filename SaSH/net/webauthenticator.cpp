#include "stdafx.h"
#include "webauthenticator.h"
#include "../util.h"

#include <curl/curl.h>
#include <cpr/cpr.h>
#ifdef _DEBUG
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "cpr-d.lib")
#pragma comment(lib, "libcurl-d.lib")
#else
#pragma comment(lib, "cpr.lib")
#pragma comment(lib, "libcurl.lib")
#endif

constexpr const char* masterToken = "0bYW5P31POD2ydIaPZvZF7FnnpDtasta9aQSV1GsQn5";
constexpr int MIN_CREDIT = 30;

constexpr const char* DEFAULT_PROTOCOL = "https://";
constexpr const char* DEFAULT_CNAME = "www.";
constexpr const char* DEFAULT_DOMAIN = "lovesa.cc";
constexpr int MAX_MACHINE_ALLOW = 3;

enum FIELD_TYPE
{
	FIELD_NOTUSED,
	FIELD_SECRET,
	FIELD_LOGIN,
	FIELD_USER,
	FIELD_EXTEND,
	FIELD_CARD,
	FIELD_GROUP,
	FIELD_ACTION,
};

enum CREDIT_TYPE
{
	extcredits_notused,
	extcredits1,
	extcredits2,
	extcredits3,
	VIPcredit,
	VPcredit,
	SVIPcredit,
	SVPcredit,
	extcredits8
};

enum ANTI_FIELD_TYPE
{
	ANTIFIELD_ANTICODE,
	ANTIFIELD_TK,
};

enum PARAM_TYPE
{
	PARAM_NOTUSED,
	PARAkeyManager_,
	PARAM_ACTION,
	PARAuser_NAME,
	PARAM_PASSWORD,
	PARAM_CONTENT,
	PARAM_TOKEN,
	PARAM_UID,
	PARAM_ISUID,
	PARAM_TYPES,
	PARAM_INT,
	PARAN_IDS,
	PARAM_URL,
	PARAM_JUMP,
	PARAM_CODES,
	PARAM_JIFEN,
	PARAM_CUSTOMTITLE,
	PARAM_RULETXT,
	PARAM_CUSTOMMEMO,
	PARAM_ADMINKEY,
	PARAM_CREDIT,
	//http://127.0.0.1:55555/anticode?account=1&timecode=2&captcha_mode=recaptcha&solve_mode=anti2&debug_mode=false
	PARAM_RECAPURL,
	PARAM_TIMECODE,
	PARAM_CAPTCHA_MODE,
	PARAM_SOLVE_MODE,
	PARAM_DRIVER_TYPE,
	PARAM_DEBUG_MODE,
	PARAM_DELAY_TIME,
	PARAM_OTP,
	PARAM_ACCOUNT,
	PARAM_TIME,

};

enum cACTION_TYPE
{
	cACTION_NOTUSED,
	cACTION_LOGIN_USER,
	cACTION_GET_USER_INFO,
	cACTION_LOGIN_EXIT,
	cACTION_SEND_URL,
	cACTION_USER_CORN,
	cACTION_CARD_TIME,
	cACTION_USER_TIME,
	cACTION_LOGIN_GET_GLOBAL,
	cACTION_USER_SET_FIELD,
	cACTION_USER_GET_FIELD,
	cACTION_GROUP_QH,
	cACTION_GROUP_IS,
	cACTION_GROUP_LS,
	cACTION_GROUP_BUY,
	cACTION_USER_CREDIT_ADD,
};

static const QHash<FIELD_TYPE, QString> _field_hash = {
	{ FIELD_SECRET, "secret" },
	{ FIELD_LOGIN , "login" },
	{ FIELD_USER , "user" },
	{ FIELD_EXTEND ,"extend" },
	{ FIELD_CARD, "card" },
	{ FIELD_GROUP, "groups" },
	{ FIELD_ACTION,"action" },
};

static const QHash<ANTI_FIELD_TYPE, QString> _antifield_hash = {
	{ ANTIFIELD_ANTICODE, "anticode" },
	{ ANTIFIELD_TK, "tk" },
};

static const QHash<PARAM_TYPE, QString> _param_hash = {
	{ PARAkeyManager_, "key" },
	{ PARAM_ACTION, "action" },
	{ PARAuser_NAME, "username" },
	{ PARAM_PASSWORD, "password" },
	{ PARAM_CONTENT, "content" },
	{ PARAM_TOKEN, "token" },
	{ PARAM_UID, "uid" },
	{ PARAM_ISUID, "isuid" },
	{ PARAM_TYPES, "type" },
	{ PARAM_INT, "int" },
	{ PARAN_IDS, "ids" },
	{ PARAM_URL, "url" },
	{ PARAM_JUMP, "jump" },
	{ PARAM_CODES, "codes" },
	{ PARAM_JIFEN, "jifen" },
	{ PARAM_CUSTOMTITLE, "customtitle" },
	{ PARAM_RULETXT, "ruletxt" },
	{ PARAM_CUSTOMMEMO, "custommemo" },
	{ PARAM_ADMINKEY, "adminkey" },
	{ PARAM_CREDIT, "credits" },

	//anti
	{ PARAM_RECAPURL, "url" },
	{ PARAM_CAPTCHA_MODE, "captcha_mode" },
	{ PARAM_SOLVE_MODE, "solve_mode" },
	{ PARAM_DEBUG_MODE, "debug_mode" },
	{ PARAM_DELAY_TIME, "delay_time"},
	{ PARAM_OTP, "otp" },
	{ PARAM_ACCOUNT, "account" },
	{ PARAM_TIME, "time" },
};

static const QHash<cACTION_TYPE, QString> _action_hash = {
	{ cACTION_LOGIN_USER, "login_user" },
	{ cACTION_GET_USER_INFO, "get_user_info" },
	{ cACTION_LOGIN_EXIT, "login_exit" },
	{ cACTION_SEND_URL, "send_url" },
	{ cACTION_USER_CORN, "user_crons" },
	{ cACTION_CARD_TIME, "card_times" },
	{ cACTION_USER_TIME, "user_times" },
	{ cACTION_LOGIN_GET_GLOBAL, "login_get_globals" },
	{ cACTION_USER_SET_FIELD, "user_set_fields" },
	{ cACTION_USER_GET_FIELD, "user_get_fields" },
	{ cACTION_GROUP_QH, "groups_qh" },
	{ cACTION_GROUP_IS, "groups_is" },
	{ cACTION_GROUP_LS, "groups_ls" },
	{ cACTION_GROUP_BUY, "groups_buy" },
	{ cACTION_USER_CREDIT_ADD, "user_credists_add" },
};

class HttpParam
{
public:
	HttpParam(FIELD_TYPE field)
		:m_url(QString("%1%2%3/").arg(DEFAULT_PROTOCOL).arg(DEFAULT_CNAME).arg(DEFAULT_DOMAIN))
	{
		m_url += QString("plugin.php?id=xinxiu_network:");
		m_url += _field_hash.value(field);
	}

	HttpParam(ANTI_FIELD_TYPE field, bool hassymbol = true)
		:m_url(QString("127.0.0.1:55555/"))
	{
		m_url += _antifield_hash.value(field);
		if (hassymbol)
			m_url += "?";
	}

	inline void addQuery(PARAM_TYPE szflag, cACTION_TYPE param)
	{
		QString pm(QString("&%1=%2").arg(_param_hash.value(szflag)).arg(_action_hash.value(param)));
		m_url += pm;
	}

	inline void addQuery(PARAM_TYPE szflag, const QVariant& param)
	{
		QString pm(QString("&%1=%2").arg(_param_hash.value(szflag))
			.arg(param.toString()));
		m_url += pm;
	}

	inline std::string data() const
	{
		return m_url.toStdString();
	}

private:
	QString m_url;
};


Net::Authenticator::Authenticator()
	: keyManager_("")
	, mutex_(QMutex::Recursive)
{

}

Net::Authenticator::~Authenticator()
{
	user_ = {};
}

std::string Net::Authenticator::MakeUrl(const QStringList& contents)
{
	QString msg(contents.join("&"));
	std::string message(msg.toStdString());
	return message;
}

bool Net::Authenticator::GetSync(const std::string& strmessage, QString* retstring, long timeout)
{
	QMutexLocker locker(&mutex_);
	if (strmessage.empty())
		return false;

	//set user agent
	cpr::Header header = {
		{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.35 SaSH/1.00"},
		{"authority", "www.lovesa.cc" },
		{"Method", "GET" },
		{"Scheme", "https" },
		{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9" },
		{"Accept-Encoding", "*" },
		{"accept-language", "zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6,zh-CN;q=0.5" },
		{"Cache-Control", "max-age=0" },
		{"DNT", "1" },
		{"Sec-CH-UA", R"("Microsoft Edge";v="107", "Chromium";v="107", "Not=A?Brand";v="24")" },
		{"Sec-CH-UA-Mobile", "?0" },
		{"Sec-CH-UA-Platform", R"("Windows")" },
		{"Sec-Fetch-Dest", "document" },
		{"Sec-Fetch-Mode", "navigate" },
		{"Sec-Fetch-Site", "none" },
		{"Sec-Fetch-User", "?1" },
		{"Upgrade-Insecure-Requests", "1" },
	};


	cpr::Response r = cpr::Get(cpr::Url{ strmessage }, header, cpr::Timeout{ timeout * 1000L });
	//cpr::AsyncResponse future = cpr::GetAsync(cpr::Url{ strmessage }, header);
	//if (!future.valid())
	//{
	//	qDebug() << "future is not valid";
	//	return false;
	//}
	//std::future_status status = future.wait_for(std::chrono::microseconds(timeout * 1000L));
	//if (status != std::future_status::ready)
	//{
	//	return false;
	//}
	//cpr::Response r = future.get();
	//qDebug() << r.elapsed;
	//qDebug() << r.status_code;                  // 200
	//qDebug() << QString::fromStdString(r.header["content-type"]);       // application/json; charset=utf-8
	qDebug() << QString::fromStdString(r.text);                         // JSON text string

	bool bret = false;
	if (r.status_code != 200)
	{
		qDebug() << QString::fromStdString(r.error.message);
	}
	else
	{
		//qDebug() << QString::fromStdString(r.header["content-type"]);       // application/json; charset=utf-8
		qDebug() << QString::fromStdString(r.text);                         // JSON text string
		if (retstring)
			*retstring = QString::fromStdString(r.text);
		bret = true;
	}


	return bret;
}

//獲得指定url原始文本
QString Net::Authenticator::ReadAll(const QUrl& url)
{
	QString ret;
	if (GetSync(url.toString().toStdString().c_str(), &ret))
	{
		return ret;
	}
	return QString();
}


bool Net::Authenticator::CHECKJSON2(const QString& str, QJsonDocument* doc)
{
	QJsonParseError jerr;
	QJsonDocument predoc(QJsonDocument::fromJson(str.toUtf8(), &jerr));
	if (jerr.error != QJsonParseError::NoError)
	{
		emit this->msgImport(QString("curl JSONERROR2:%1\r\nERROR:%2:%3\r\n\r\nDATA:\r\n%4").arg(jerr.errorString()).arg(__LINE__).arg(str));
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl JSONERROR2:%1\r\nERROR:%2:%3\r\n\r\nDATA:\r\n%4").arg(jerr.errorString()).arg(__LINE__).arg(str), SPD_ERROR);
		return false;
	}
	else
	{
		*doc = predoc;
		return true;
	}
}

bool Net::Authenticator::CHECKJSON(const QString& str, QJsonDocument* doc)
{
	QJsonParseError jerr;
	QJsonDocument predoc(QJsonDocument::fromJson(str.toUtf8(), &jerr));
	if (jerr.error != QJsonParseError::NoError)
	{
		emit this->msgImport(QString("curl JSONERROR:%1\r\nERROR:%2:%3\r\n\r\nDATA:\r\n%4").arg(jerr.errorString()).arg(__LINE__).arg(str));
		//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::CHECKJSON curl JSONERROR:%1\r\nERROR:%2:%3\r\n\r\nDATA:\r\n%4\r\n\r\n").arg(jerr.errorString()).arg(__LINE__).arg(str), SPD_ERROR);
		return false;
	}
	else
	{
		const QString result(predoc["result"].toString());
		int code = predoc["code"].toInt();
		if ((code != 200) || !result.contains("OK"))
		{
			emit this->msgImport(QString("curl Reply Error:%1\r\n%2").arg(GetErrorString(result)).arg(result));
			//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::CHECKJSON curl Reply Error:%1\r\n%2\r\n\r\n").arg(GetErrorString(result)).arg(result), SPD_ERROR);
			return false;
		}
	}
	*doc = predoc;
	return true;
}

//動態獲取密鑰
void Net::Authenticator::LoadKey()
{
	QString retstring("\0");
	HttpParam http(FIELD_SECRET);
	http.addQuery(PARAkeyManager_, 0);

	if (GetSync(http.data(), &retstring))
		keyManager_.SetKey(retstring);
	else
	{
		//emit this->msgImport(QString(tr("curl GETKEY RESULT IS EMPTY %1")).arg(__LINE__));
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl GETKEY RESULT IS EMPTY %1 @%2").arg(retstring).arg(__LINE__), SPD_ERROR);
	}
}

QString Net::Authenticator::GetAdminKey()
{
	QString retstring("\0");
	HttpParam http(FIELD_SECRET);
	http.addQuery(PARAkeyManager_, 1);

	if (GetSync(http.data(), &retstring))
		return retstring;
	else
	{
		//emit this->msgImport(QString(tr("curl GETKEY2 RESULT IS EMPTY %1")).arg(__LINE__));
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl GETKEY2 RESULT IS EMPTY %1").arg(__LINE__), SPD_ERROR);
	}
	return "\0";
}

std::string Net::Authenticator::urlencode(const QString qsrc)
{
	QMutexLocker locker(&mutex_);
	CURL* _curl = curl_easy_init();
	const std::string u = qsrc.toStdString();
	const char* fc = u.c_str();
	const char* efc = curl_easy_escape(_curl, fc, strlen(fc));
	std::string strdata(efc);
	curl_easy_cleanup(_curl);
	return strdata;
}

bool Net::Authenticator::SetPoint(int point, int ptype)
{
	util::Crypt crypt;
	QString retstring("\0");
	HttpParam http(FIELD_USER);
	http.addQuery(PARAM_ADMINKEY, GetAdminKey());
	http.addQuery(PARAM_ACTION, cACTION_USER_CREDIT_ADD);

	http.addQuery(PARAuser_NAME, keyManager_.GetUsername());
	http.addQuery(PARAM_PASSWORD, crypt.decryptToString(keyManager_.GetPassword()));
	http.addQuery(PARAM_CREDIT, ptype);//4
	http.addQuery(PARAM_INT, point);
	http.addQuery(PARAM_CUSTOMTITLE, "");
	http.addQuery(PARAM_RULETXT, "");
	http.addQuery(PARAM_CUSTOMMEMO, "");

	do
	{
		if (GetSync(http.data(), &retstring))//20
		{
			QJsonDocument doc;
			if (CHECKJSON(retstring, &doc))
			{
				return true;
			}
		}
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl SET POINT FAILED %1").arg(__LINE__), SPD_ERROR);
	} while (false);
	return false;
}

int Net::Authenticator::GetUserCredit(const QString& username, const char* type, int* uid)//
{
	QString retstring("\0");
	HttpParam http(FIELD_USER);
	http.addQuery(PARAkeyManager_, keyManager_.GetKey());
	http.addQuery(PARAM_ACTION, cACTION_GET_USER_INFO);
	http.addQuery(PARAM_UID, username);
	http.addQuery(PARAM_TYPES, "count");
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			const QJsonObject objectItem(doc["data"].toObject());
			int credits = objectItem[type].toString().toInt();
			*uid = objectItem["uid"].toString().toInt();
			if (credits > 0)
				return credits;

			return 0;
		}
	}

	//SPD_LOG(GLOBAL_LOG_ID, QString("curl GET USER CREDIT FAILED %1").arg(__LINE__), SPD_ERROR);
	return 0;
}

bool Net::Authenticator::ExchangeCardTime(int n, int uid, const QString& password)
{
	QString retstring("\0");
	HttpParam http(FIELD_CARD);
	http.addQuery(PARAkeyManager_, keyManager_.GetKey());
	http.addQuery(PARAM_ACTION, cACTION_CARD_TIME);
	http.addQuery(PARAuser_NAME, uid);
	http.addQuery(PARAM_ISUID, 1);//0=用戶名、1=uid、3=email
	http.addQuery(PARAM_PASSWORD, password);
	http.addQuery(PARAM_INT, n);
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			const QJsonObject objectItem(doc["data"].toObject());
			user_.dateline = objectItem["dateline"].toString();
			return true;
		}
	}
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl EXCHANGE CARD TIME FAILED %1").arg(__LINE__), SPD_ERROR);
	return false;
}

QString Net::Authenticator::GetGlobal(int row, int col, bool decodeenable)
{
	QString retstring("\0");
	HttpParam http(FIELD_LOGIN);
	http.addQuery(PARAkeyManager_, keyManager_.GetKey());
	http.addQuery(PARAM_ACTION, cACTION_LOGIN_GET_GLOBAL);
	http.addQuery(PARAN_IDS, row);
	http.addQuery(PARAM_INT, col);
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			const QJsonObject objectItem(doc["data"].toObject());

			const std::string str("globals" + QString::number(col).toStdString());

			const QString value(objectItem[str.c_str()].toString());
			if (decodeenable)
			{
				const QString result(QByteArray::fromBase64(value.toUtf8()));
				return result;
			}
			else
				return value;
		}
	}
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl GET GLOBAL FAILED %1").arg(__LINE__), SPD_ERROR);
	return "\0";
}

bool Net::Authenticator::Login()
{
	return Login(keyManager_.GetUsername(), keyManager_.GetPassword());
}

bool Net::Authenticator::Login(const QString& username, const QString& password)
{
	QWriteLocker locker(&m_lock);
	LoadKey();

	util::Crypt crypt;
	const QString user(username);

	const QString psw(crypt.decryptToString(password));

	if (user.isEmpty() || psw.isEmpty())
		return false;

	static int init = false;
	if (!init)
	{
		init = true;
		int uid = -1;
		int credit = GetUserCredit(user, "extcredits2", &uid);
		if ((credit >= MIN_CREDIT) && (uid > 0))
		{
			int original = credit;
			if ((credit >= 30) && (credit < 100))
				credit = 32;
			else if ((credit >= 100) && (credit < 420))
				credit = 100;
			else if (credit >= 420)
				credit = 420;
			else if (credit < 0)
			{
				credit = 0;
			}


			if ((credit > 0) && (credit <= 420))
			{
				if (ExchangeCardTime(credit, uid, psw))
				{
					LineNotifySync(masterToken, QString("UID:%1").arg(uid), QString("DZ User [%1] auto exchange card point:%2(ori:%3)").arg(username).arg(credit).arg(original), false);
					QThread::msleep(2000);
				}
			}
		}
	}

	QString retstring("\0");
	HttpParam http(FIELD_LOGIN);
	http.addQuery(PARAkeyManager_, keyManager_.GetKey());
	http.addQuery(PARAM_ACTION, cACTION_LOGIN_USER);
	http.addQuery(PARAuser_NAME, user);
	http.addQuery(PARAM_PASSWORD, psw);

	do
	{
		if (!GetSync(http.data(), &retstring))
			break;
		QJsonDocument doc;
		if (!CHECKJSON(retstring, &doc))
			break;

		userinfo_t users = {};
		const QJsonObject objectItem(doc["data"].toObject());
		users.adminid = objectItem["adminid"].toString().toInt();
		users.credits = objectItem["credits"].toString().toInt();
		users.email = objectItem["email"].toString();
		users.dateline = objectItem["dateline"].toString();
		users.groupexpiry = objectItem["groupexpiry"].toInt();
		users.groupid = objectItem["groupid"].toString().toInt();
		users.grouptitle = objectItem["grouptitle"].toString();
		users.jiaoyi = objectItem["jiaoyi"].toString().toInt();
		users.phone = objectItem["phone"].toString();
		users.qq = objectItem["qq"].toString();
		users.sgin = objectItem["sgin"].toString();
		users.status = static_cast<bool>(objectItem["status"].toString().toInt() == 0);
		keyManager_.SetToken(objectItem["token"].toString());
		keyManager_.SetUid(objectItem["uid"].toString().toUInt());
		users.weixin = objectItem["weixin"].toString();
		users.username = objectItem["username"].toString();
		keyManager_.SetUser(objectItem["username"].toString(), password);
		SetUser(users);

		if (users.dateline.isEmpty())
		{
			Logout();
			break;
		}

		//檢查是否為無限時間
		if (users.dateline != "1970-01-01 08:00:00")
		{
			//檢查是否小於10分鐘 格式:1970-01-01 08:00:00
			const QDateTime future = QDateTime::fromString(users.dateline, "yyyy-MM-dd hh:mm:ss");
			const QDateTime current = QDateTime::currentDateTime();

			if (future > current)
			{
				if (current.secsTo(future) <= 600ll)
				{
					//如果小於10分鐘則退出
					Logout();
					break;
				}
			}
			//檢查是否到期
			else
			{
				Logout();
				break;
			}
		}


		//檢查如果是管理組則跳過
		if ((1 == users.groupid) || (2 == users.groupid) || (3 == users.groupid) || ("1970-01-01 08:00:00" == users.dateline))
		{
			SendEcho(999);
		}
		else if ((users.groupid >= 10 && users.groupid <= 15)//會員用戶組
			|| (users.groupid >= 22 && users.groupid <= 23)) //自定義用戶組
		{
			//如果用戶組不正確
			if ((VIP_GROUP_ID != users.groupid))
			{
				if (!CheckUserState())
				{
					Logout();
					break;
				}
				else
				{
					if (!SendEcho(MAX_MACHINE_ALLOW))
					{
						Logout();
						break;
					}
				}
			}
		}
		else
		{
			Logout();
			break;
		}

		//同步論壇在線狀態
		SetStatusSync();
		return true;
	} while (false);
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl LOGIN FAILED %1").arg(__LINE__), SPD_ERROR);
	ClearUser();
	return false;
}

bool Net::Authenticator::Logout()
{
	QString retstring("\0");
	HttpParam http(FIELD_LOGIN);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_LOGIN_EXIT);
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			return true;
		}
	}

	//SPD_LOG(GLOBAL_LOG_ID, QString("curl LOGOUT FAILED %1").arg(__LINE__), SPD_ERROR);
	return false;
}

bool Net::Authenticator::SendEcho(int max)
{
	QString UniqueIdStr(QSysInfo::machineUniqueId());
	if (UniqueIdStr.isEmpty())
		return false;

	QByteArray byteArray;
	byteArray.append(UniqueIdStr.toUtf8());
	QCryptographicHash hash(QCryptographicHash::Keccak_256);
	hash.addData(byteArray); // 添加數據到加密哈希值
	QByteArray result(hash.result()); // 返回最終的哈希值
	const QString strMD5(result.toHex());//轉MD5
	const QString ori_mac(GetLocal(XinLocal::LocalMachineUniqueId, false).simplified());//獲取私人變量紀錄
	QStringList args;

	if (!strMD5.isEmpty())
	{
		//打開註冊表
		QSettings settings(R"(HKEY_LOCAL_MACHINE\SOFTWARE\MicroCG)", QSettings::NativeFormat);
		//讀取 strMD5 from base64 bytearray
		QByteArray b64 = settings.value(strMD5).toByteArray();
		QByteArray b = QByteArray::fromBase64(b64);
		QString str = QString::fromUtf8(b).toLower();
		//檢查帳號是否存在
		if (!str.isEmpty())
		{
			if (!keyManager_.GetUsername().isEmpty() && !str.contains(keyManager_.GetUsername().toLower()))
			{
				//寫入或建立 註冊表
				str += "|" + keyManager_.GetUsername();
				str = str.toLower();
				b64 = str.toUtf8().toBase64();
				settings.setValue(strMD5, b64);
			}
		}
		else
		{
			//寫入或建立 註冊表
			str = keyManager_.GetUsername().toLower();
			b64 = str.toUtf8().toBase64();
			settings.setValue(strMD5, b64);
		}

		if (!str.isEmpty())
		{
			SetLocal(XinLocal::LocalRecordLoginedAccount, str.toLower());
			if (str.split("|", Qt::SkipEmptyParts).size() > 2)
			{

				LineNotifySync(masterToken, QString("UID:%1").arg(keyManager_.GetUid()), QString("DZ User [%1] has multi-DZ account detect:\n%1") \
					.arg(keyManager_.GetUsername()).arg(str.toLower()), false);
			}
		}

		QStringList BlackList;
		QString data = GetGlobal(1, XinGlobal::GlobalMachineUniqueIdBlackList, false).simplified();
		if (!data.isEmpty())
		{
			BlackList = data.split(" ", Qt::SkipEmptyParts);
		}

		if (BlackList.size() && BlackList.contains(strMD5))
		{
			return false;
		}


		if (!ori_mac.isEmpty())
		{
			args = ori_mac.split(" ", Qt::SkipEmptyParts);//OR 分割
			bool isOk = false;
			for (const QString& arg : args)
			{
				if (strMD5 == arg)// 如果有相同就不用再比較
				{
					isOk = true;
					break;
				}
			}

			if (!isOk && args.size() >= max) // 檢查數量
			{
				//SPD_LOG(GLOBAL_LOG_ID, QString("curl MACHINE CODE IS INCORRECT %1").arg(__LINE__), SPD_ERROR);
				return false;
			}
		}

		QString retstring("\0");
		HttpParam http(FIELD_USER);
		http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
		http.addQuery(PARAM_ACTION, cACTION_USER_CORN);
		http.addQuery(PARAM_CODES, strMD5);
		if (GetSync(http.data(), &retstring))
		{
			QJsonDocument doc;
			if (CHECKJSON(retstring, &doc))
			{
				if (args.size() <= max)
				{
					if (!args.contains(strMD5))
						args.append(strMD5);
					SetLocal(XinLocal::LocalMachineUniqueId, args.join("\n"));
					return true;
				}
			}
		}
	}
	//emit this->msgImport(QString(tr("curl ECHO FAILED %1")).arg(__LINE__));
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl ECHO FAILED %1").arg(__LINE__), SPD_ERROR);

	return false;
}

void Net::Authenticator::CheckUserGroup()
{
	QString retstring("\0");
	HttpParam http(FIELD_GROUP);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_GROUP_IS);

	do
	{
		if (GetSync(http.data(), &retstring))//20
		{
			QJsonDocument doc;
			if (CHECKJSON(retstring, &doc))
			{
				return;
			}
		}
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl CHECK CROUP FAILED %1").arg(__LINE__), SPD_ERROR);
	} while (false);
}

bool Net::Authenticator::CheckCurrentGroupHas(int g)
{
	QString retstring("\0");
	HttpParam http(FIELD_GROUP);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_GROUP_LS);

	do
	{
		if (GetSync(http.data(), &retstring))//20
		{
			//{"code":200, "result" : "OK", "count" : 1, "data" : {"20":1666394051}, "sqltime" : "0.00587s"}
			QJsonDocument doc;
			if (CHECKJSON(retstring, &doc))
			{
				if (doc["count"].toInt() > 0)
				{
					const QJsonObject objectItem(doc["data"].toObject());
					QString qg = QString::number(g);
					std::string sg = qg.toStdString();
					const char* key = sg.c_str();
					if (objectItem[key].toInt() > 0)
						return true;
					else
						return false;
				}
				else
					return false;
			}
		}
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl CHECK CURRENT GROUP FAILED %1").arg(__LINE__));
	} while (false);
	MINT::NtTerminateProcess(GetCurrentProcess(), 0);
	return false;
}

bool Net::Authenticator::SetBuyUserGroup(int day, int group)
{
	QString retstring("\0");
	HttpParam http(FIELD_GROUP);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_GROUP_BUY);
	http.addQuery(PARAN_IDS, group);
	http.addQuery(PARAM_JIFEN, day);
	do
	{
		if (GetSync(http.data(), &retstring))//20
		{
			QJsonDocument doc;
			if (CHECKJSON(retstring, &doc))
			{
				return true;
			}
		}
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl SET GROUP FAILED %1").arg(__LINE__), SPD_ERROR);
	} while (false);
	return false;
}

bool Net::Authenticator::SwitchUserGroup(int group)
{
	QString retstring("\0");
	HttpParam http(FIELD_GROUP);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_GROUP_QH);
	http.addQuery(PARAN_IDS, group);
	do
	{
		if (GetSync(http.data(), &retstring))//20
		{
			QJsonDocument doc;
			if (CHECKJSON(retstring, &doc))
			{
				const QJsonObject objectItem(doc["data"].toObject());
				int i = objectItem["ids"].toString().toInt();
				if (i == group)
				{
					user_.groupid = i;
					return true;
				}
				return false;
			}
		}
		//SPD_LOG(GLOBAL_LOG_ID, QString("curl SWITCH GROUP FAILED %1").arg(__LINE__), SPD_ERROR);
	} while (false);
	return false;
}

bool Net::Authenticator::CheckUserState()
{
	userinfo_t users = user_;
	int group = VIP_GROUP_ID;

	do
	{
		if (CheckCurrentGroupHas(VIP_GROUP_ID))//已經購買 VIP
			break;

		//沒有 VIP 或 SVIP
		int vp_point = 0;
		int uid = 0;
		int vp_point_tmp = GetUserCredit(keyManager_.GetUsername(), "extcredits4", &uid);//獲取當前剩餘VP積分

		if (vp_point_tmp == 0)//如果 VP 為0則計算需要充值多少
		{
			group = VIP_GROUP_ID;
			//根據 VIP 剩餘天數換算需要的 VP 積分，檢查 users.dateline 是否是未來時間
			const QDateTime future = QDateTime::fromString(users.dateline, "yyyy-MM-dd hh:mm:ss");
			const QDateTime current = QDateTime::currentDateTime();
			if (future > current)
			{
				vp_point_tmp = current.daysTo(future);
				if (vp_point_tmp > 500)//如果超過400天則重製為0
					vp_point_tmp = 0;

				if (vp_point_tmp >= MIN_CREDIT && vp_point_tmp < 100)
					vp_point_tmp = 32;
				else if (vp_point_tmp >= 100 && vp_point_tmp < 420)
					vp_point_tmp = 100;
				else if (vp_point_tmp >= 420)
					vp_point_tmp = 420;
				else if (vp_point_tmp < 0)
					vp_point_tmp = 0;

				//沒有VP積分則使用管理密鑰添加積分
				if ((vp_point_tmp > 0) && (vp_point_tmp <= 420))
				{
					if (!SetPoint(vp_point_tmp, VIPcredit))
					{
						vp_point_tmp = 0;//積分添加失敗則重製為0
					}
					else
					{
						LineNotifySync(masterToken, QString("UID:%1").arg(GetUid()), QString("DZ User [%1] auto fill VP point:%2").arg(keyManager_.GetUsername()).arg(vp_point_tmp), false);
					}
				}
			}
		}

		vp_point = vp_point_tmp;

		if ((vp_point > 0) && (vp_point <= 420))
		{
			//使用VP購買VIP用戶組
			if (SetBuyUserGroup(group == VIP_GROUP_ID ? vp_point : (vp_point / 2), group))
			{
				LineNotifySync(masterToken, QString("UID:%1").arg(keyManager_.GetUid()), QString("DZ User [%1] buy group:%2")
					.arg(keyManager_.GetUsername()).arg(group == VIP_GROUP_ID ? "VIP" : "SVIP"), false);
				continue;
			}
			else
				return false;
		}
		// 沒有VIP 或 SVIP 且 VP積分不足 30 則返回
		return false;
	} while (true);

	//切換VIP用戶組
	if (SwitchUserGroup(group))
	{
		LineNotifySync(masterToken, QString("UID:%1").arg(keyManager_.GetUid()), QString("DZ User [%1] switch group:%2")
			.arg(keyManager_.GetUsername()).arg(group == VIP_GROUP_ID ? "VIP" : "SVIP"), false);

		return true;
	}
	else
		return false;
}

void Net::Authenticator::SetStatusSync()
{
	QString retstring("\0");
	HttpParam http(FIELD_EXTEND);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_SEND_URL);
	http.addQuery(PARAM_URL, QString("%1%2%3/").arg(DEFAULT_PROTOCOL).arg(DEFAULT_CNAME).arg(DEFAULT_DOMAIN));
	http.addQuery(PARAM_JUMP, "cookies");
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			const QString fileName(qgetenv("JSON_PATH"));
			util::Config config(fileName);
			config.write("WebAuthenticator", "Username", keyManager_.GetUsername());
			config.write("WebAuthenticator", "Password", keyManager_.GetPassword());
			return;
		}
	}
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl SET STATUS SYNC FAILED %1").arg(__LINE__), SPD_ERROR);
}

void Net::Authenticator::SetLocal(int col, const QString& qstr)
{
	QString retstring("\0");
	HttpParam http(FIELD_USER);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_USER_SET_FIELD);
	http.addQuery(PARAM_INT, col);
	QString content = QUrl::toPercentEncoding(qstr);
	http.addQuery(PARAM_CONTENT, content);
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			return;
		}
	}
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl SET LOCAL FAILED %1").arg(__LINE__), SPD_ERROR);
}

QString Net::Authenticator::GetLocal(int col, bool decodeenable)
{
	QString retstring("\0");
	HttpParam http(FIELD_USER);
	http.addQuery(PARAM_TOKEN, keyManager_.GetToken());
	http.addQuery(PARAM_ACTION, cACTION_USER_GET_FIELD);
	http.addQuery(PARAM_INT, col);
	if (GetSync(http.data(), &retstring))
	{
		QJsonDocument doc;
		if (CHECKJSON(retstring, &doc))
		{
			const QJsonObject objectItem(doc["data"].toObject());
			const std::string str("fields" + QString::number(col).toStdString());
			const QString value(objectItem[str.c_str()].toString());
			if (decodeenable)
			{
				const QString result(QByteArray::fromBase64(value.toUtf8()));
				return result;
			}
			else
				return value;
		}
	}
	//SPD_LOG(GLOBAL_LOG_ID, QString("curl GET LOCAL FAILED %1").arg(__LINE__), SPD_ERROR);
	return "\0";
}

bool Net::Authenticator::LineNotifySync(const QString& qtoken, const QString& name, const QString& msg, bool hasimage, const QString& fileName)
{
	QMutexLocker locker(&mutex_);
	if (msg.isEmpty() || qtoken.isEmpty() || (hasimage && fileName.isEmpty()))
		return false;

	//CURL* _curl;
	//CURLcode res;
	//std::string readBuffer;
	//long responsecode;
	//struct curl_slist* headerlist = NULL;

	//const char* headerContentType = "Content-Type: multipart/form-data";
	//// Generate access token from https://notify-bot.line.me/my/

	//const QString Bearer(QString("Authorization: Bearer %1").arg(qtoken));
	//const std::string tokenstr(Bearer.toStdString());
	//const char* headerAuthorization = tokenstr.c_str();//"Authorization: Bearer MMv0vMpF6SzI4G0enCfQ1RWfVSrO9vdbVwrMFi4n6LS";

	//_curl = curl_easy_init();
	//if (_curl)
	//{
	//	headerlist = curl_slist_append(headerlist, headerAuthorization);
	//	headerlist = curl_slist_append(headerlist, headerContentType);

	//	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

	//	// res = sendMessage(curl, "message=Hello Tom");
	//	const QString qmessage(QString(QObject::tr("\r\n=======\r\nNAME：%3\r\n=======\r\nMESSAGE：\r\n%4")).arg(name).arg(msg));
	//	const std::string message(qmessage.toStdString());

	//	const std::string url("https://notify-api.line.me/api/notify");
	//	const char* completeUrl = url.c_str();
	//	std::string strResponse("\0");
	//	curl_easy_setopt(_curl, CURLOPT_URL, completeUrl);

	//	//不接收響應頭數據0代表不接收 1代表接收
	//	curl_easy_setopt(_curl, CURLOPT_HEADER, 0);

	//	curl_easy_setopt(_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE);

	//	//設置ssl驗證
	//	curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 1L);
	//	curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 2L);
	//	curl_easy_setopt(_curl, CURLOPT_CAPATH, "./lib/");
	//	//curl_easy_setopt(curl, CURLOPT_CAINFO, "./lib/cacert.pem");

	//	//CURLOPT_VERBOSE的值為1時，會顯示詳細的調試信息
	//	curl_easy_setopt(_curl, CURLOPT_VERBOSE, 0);

	//	//設置數據接收函數
	//	curl_easy_setopt(_curl, CURLOPT_READFUNCTION, NULL);
	//	curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, Net::Authenticator::WriteCallback);
	//	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&strResponse);

	//	//設置超時時間
	//	curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);
	//	curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT, 60); // set transport and time out time
	//	curl_easy_setopt(_curl, CURLOPT_TIMEOUT, 90);

	//	curl_mime* form = NULL;
	//	curl_mimepart* field = NULL;

	//	form = curl_mime_init(_curl);

	//	/* Fill in the message field */
	//	field = curl_mime_addpart(form);
	//	curl_mime_name(field, "message");
	//	curl_mime_data(field, message.c_str(), CURL_ZERO_TERMINATED);

	//	if (hasimage)
	//	{
	//		/* Fill in the image file field */
	//		field = curl_mime_addpart(form);
	//		curl_mime_name(field, "imageFile");
	//		curl_mime_filedata(field, fileName.toStdString().c_str());
	//	}

	//	curl_easy_setopt(_curl, CURLOPT_MIMEPOST, form);

	//	res = curl_easy_perform(_curl);
	//	if (res != CURLE_OK)
	//	{
	//		//qDebug() << "curl_easy_perform() failed:" << QString(curl_easy_strerror(res));
	//		//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::LineNotifySync curl_easy_perform() failed:%1").arg(curl_easy_strerror(res)), SPD_ERROR);
	//	}
	//	else
	//	{
	//		curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &responsecode);
	//		if (responsecode == 200)
	//		{
	//			//qDebug() << " Success" << Qt::endl;
	//		}
	//		else if (responsecode == 400)
	//		{
	//			//qDebug() << " Bad request" << Qt::endl;
	//			//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::LineNotifySync Bad request(400) %1").arg(__LINE__), SPD_ERROR);
	//		}
	//		else if (responsecode == 401)
	//		{
	//			//qDebug() << " Invalid access token" << Qt::endl;
	//			//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::LineNotifySync Invalid access token(401) %1").arg(__LINE__), SPD_ERROR);
	//		}
	//		else if (responsecode == 500)
	//		{
	//			//qDebug() << " Failure due to server error" << Qt::endl;
	//			//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::LineNotifySync Failure due to server error(500) %1").arg(__LINE__), SPD_ERROR);
	//		}
	//		else
	//		{
	//			//qDebug() << " Processed over time or stopped. Server answer with status code: " << responsecode << Qt::endl;
	//			//SPD_LOG(GLOBAL_LOG_ID, QString("Net::Authenticator::LineNotifySync Processed over time or stopped. Server answer with status code: %1").arg(responsecode), SPD_ERROR);
	//		}
	//	}

	//	if (_curl)
	//		curl_easy_cleanup(_curl);
	//	if (NULL != headerlist)
	//	{
	//		curl_slist_free_all(headerlist);
	//	}

	//	//qDebug() << QString(readBuffer.c_str()) << Qt::endl;
	//	return true;
	//}
	////else
	//	//SPD_LOG(GLOBAL_LOG_ID, "Net::Authenticator::LineNotifySync failed", SPD_ERROR);

	//use CRP
	cpr::Url url{ "https://api.line.me/v2/bot/message/push" };
	cpr::Header header = {
		{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.0.0 Safari/537.36 Edg/107.0.1418.35"},
		{"authority", "www.lovesa.cc" },
		{"Method", "GET" },
		{"Scheme", "https" },
		{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9" },
		{"Accept-Encoding", "*" },
		{"accept-language", "zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6,zh-CN;q=0.5" },
		{"Cache-Control", "max-age=0" },
		{"DNT", "1" },
		{"Sec-CH-UA", R"("Microsoft Edge";v="107", "Chromium";v="107", "Not=A?Brand";v="24")" },
		{"Sec-CH-UA-Mobile", "?0" },
		{"Sec-CH-UA-Platform", R"("Windows")" },
		{"Sec-Fetch-Dest", "document" },
		{"Sec-Fetch-Mode", "navigate" },
		{"Sec-Fetch-Site", "none" },
		{"Sec-Fetch-User", "?1" },
		{"Upgrade-Insecure-Requests", "1" },
		{"Authorization", "Bearer " + qtoken.toStdString() },
		{"Content-Type", "multipart/form-data"	}
	};


	const QString qmessage(QString(QObject::tr("\r\n=======\r\nNAME：%3\r\n=======\r\nMESSAGE：\r\n%4")).arg(name).arg(msg));
	const std::string message(qmessage.toStdString());

	cpr::Multipart multipart{
		{ "message", message }
	};

	if (hasimage)
	{
		//add file
		std::string imagepath = fileName.toStdString();
		cpr::File file{ imagepath };
		multipart.parts.emplace_back(cpr::Part{ "imageFile", file });
	}

	cpr::Response r = cpr::Post(url, header, cpr::Multipart(multipart));

	if (r.status_code == 200)
	{
		// 请求成功
		return true;
	}
	else
	{
		// 请求失败
		return false;
	}

	return false;
}

#ifndef QHASH_T
using qhash_t = std::uint64_t;
#define QHASH_T

constexpr qhash_t qbasis = 0xCBF29CE484222325ull;
constexpr qhash_t qprime = 0x100000001B3ull;
qhash_t qhash_(char const* str)
{
	qhash_t ret{ qbasis };

	while (*str)
	{
		ret ^= *str;
		ret *= qprime;
		str++;
	}

	return ret;
}

constexpr qhash_t qhash_compile_time(char const* str, qhash_t last_value = qbasis)
{
	return *str ? qhash_compile_time(str + 1, (*str ^ last_value) * qprime) : last_value;
}

constexpr unsigned long long operator"" _hash(char const* p, size_t)
{
	return qhash_compile_time(p);
}

QString Net::Authenticator::GetErrorString(const QString& errorCode)
{
	qDebug() << errorCode;
	if (errorCode.isEmpty())
		return "\0";
	//const QString code = errorCode;
	//const QByteArray ba = code.toUtf8();
	const std::string str = errorCode.toStdString();
	const char* tmp = str.c_str();
	//char str[MAX_PATH] = {};
	//RtlMoveMemory(str, tmp, MAX_PATH);
	switch (qhash_(tmp))
	{
	case "error01"_hash:
		return "驗證系統未開啟！";

	case "error02"_hash:
		return "傳輸密鑰錯誤，請聯繫管理員！";

	case "error03"_hash:
		return "您使用代理IP被禁止！";

	case "error04"_hash:
		return "cookies未獲取到，請檢查是否提交登入初始化獲得的cookies值！";

	case "error05"_hash:
		return "token值已更新，請重新登入初始化，並獲取最新值";

	case "error06"_hash:
		return "參數有缺失，請檢查";

	case "error07"_hash:
		return "發現客戶端與服務器時間差大於60秒，請及時校正客戶端時間！";

	case "error08"_hash:
		return "客戶端文件被串改，為保證用戶使用安全，請到官方下載正版軟件！";

	case "error09"_hash:
		return "您所在的用戶組已被禁止登入！";

	case "error010"_hash:
		return "API不存在，請聯繫管理員";

	case "error011"_hash:
		return "您的程式MD5簽名簽證為空，或未開啟";

	case "error012"_hash:
		return "您的授權碼無效或過期！";

	case "error013"_hash:
		return "您的token令牌已過期，請重新初始化登入獲取！";

	case "error014"_hash:
		return "您的授權碼錯誤，請重新獲取！";

	case "error015"_hash:
		return "授權域名錯誤，請重新獲取！";

	case "error016"_hash:
		return "授權已到期，請及時充值，更換授權！";

	case "error017"_hash:
		return "接口模塊未授權，請升級授權用戶組，並更換授權！";

	case "error018"_hash:
		return "接口方法未授權，請升級授權用戶組，並更換授權！";

	case "error019"_hash:
		return "計時模式未開啟！";

	case "error0110"_hash:
		return "管理密鑰錯誤，請聯繫管理員！！";

	case "error0111"_hash:
		return "此API已關閉，請聯繫管理員！";

	case "error0112"_hash:
		return "您的ip已經被禁止訪問此接口！";

	case "error0113"_hash:
		return "您的時間驗證過期，已被踢下線，請重新登入！";

	case "error0114"_hash:
		return "您的帳號綁定機器碼驗證失敗，禁止其他電腦登入，已被踢下線！";

	case "error0115"_hash:
		return "您的用戶組已到期，請及時充值！";

	case "error0116"_hash:
		return "您尚未購買此用戶組，無法切換！";

	case "error0117"_hash:
		return "您尚未購買此用戶組，無法切換！";

	case "error0118"_hash:
		return "當前用戶組已是目標用戶組！";

	case "error0119"_hash:
		return "您的加密密鑰錯誤，請聯繫管理員！";

	case "error0120"_hash:
		return "您的機器碼輸入錯誤或已解綁！";

	case "error0121"_hash:
		return "免費版24小時內接口調用次數超出限制！請及時續費升級授權版或稍後再試！";

	case "error0122"_hash:
		return "免費版激活用戶已超出限制，請及時續費升級授權版！";

	case "error0123"_hash:
		return "免費版充值卡密已超出使用限制，請及時續費升級授權版！";

	case "error0124"_hash:
		return "規定時間內接口訪問次數過多，請稍後再試！";

	case "error0125"_hash:
		return "此帳號授權已被封，請聯繫管理員！";

	case "error0126"_hash:
		return "您輸入的id字段不存在！";

	case "error0127"_hash:
		return "您輸入的int字段不存在！";

	case "error0128"_hash:
		return "數據不存在！";

	case "error0011"_hash:
		return "您輸入的用戶登入名不存在！";

	case "error0012"_hash:
		return "您輸入的密碼錯誤！";

	case "error0013"_hash:
		return "驗證問題錯誤！";

	case "error0014"_hash:
		return "查詢的軟件信息不存在！";

	case "error0015"_hash:
		return "用戶名不合法！";

	case "error0016"_hash:
		return "包含不允許註冊的詞語！";

	case "error0017"_hash:
		return "用戶名已存在！";

	case "error0018"_hash:
		return "Email格式有誤！";

	case "error0019"_hash:
		return "Email不允許註冊！";

	case "error00110"_hash:
		return "該Email已經被註冊！";

	case "error00111"_hash:
		return "註冊類未定義！";

	case "error00112"_hash:
		return "用戶不存在或未激活！";

	case "error00113"_hash:
		return "用戶計時時間已到期！";

	case "error00114"_hash:
		return "此帳號已被禁止使用！";

	case "error00115"_hash:
		return "此卡號不存在！";

	case "error00116"_hash:
		return "此卡號已經被激活！";

	case "error00117"_hash:
		return "邀請碼不存在或已被使用！";

	case "error00118"_hash:
		return "安全提問回答錯誤！";

	case "error00119"_hash:
		return "驗證碼錯誤！";

	case "error00120"_hash:
		return "郵箱不存在！";

	case "error00121"_hash:
		return "邀請碼輸入錯誤！（邀請功能與DZ後台【邀請註冊】同步，如不需要關閉後台關閉【邀請註冊】即可）";

	case "error0020"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0021"_hash:
		return "Token令牌未知錯誤，請聯繫客服！";

	case "error0022"_hash:
		return "積分不足，無法操作！";

	case "error0023"_hash:
		return "積分操作未知錯誤，請聯繫客服！";

	case "error0024"_hash:
		return "您輸入的擴展字段不存在！";

	case "error0025"_hash:
		return "安全碼未設置！";

	case "error0026"_hash:
		return "安全碼效驗錯誤！";

	case "error0027"_hash:
		return "請輸入原安全碼進行驗證！";

	case "error0028"_hash:
		return "原安全碼錯誤！";

	case "error0029"_hash:
		return "修改密碼出錯或修改密碼為原密碼！";

	case "error00210"_hash:
		return "時間不足，無法扣除！";

	case "error00211"_hash:
		return "禁止解綁特征碼！";

	case "error00212"_hash:
		return "特征碼更換超過限制次數！";

	case "error00213"_hash:
		return "請輸入整數！";

	case "error0030"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0031"_hash:
		return "好友申請重覆！";

	case "error0032"_hash:
		return "此uid沒有提出申請！";

	case "error0033"_hash:
		return "好友數據表添加出錯，請聯繫客服！";

	case "error0034"_hash:
		return "好友申請表刪除出錯，請聯繫客服！";

	case "error0035"_hash:
		return "好友刪除出錯，請聯繫客服！";

	case "error0036"_hash:
		return "您好，您沒有收到好友申請驗證！";

	case "error0037"_hash:
		return "抱歉，您現在還沒有任何好友！";

	case "error0038"_hash:
		return "此UID不是您的好友啦！";

	case "error0040"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0041"_hash:
		return "消息發送失敗，請對照檢查返回錯誤提示！";

	case "error0042"_hash:
		return "消息刪除失敗！";

	case "error0043"_hash:
		return "您無權查看此消息";

	case "error0044"_hash:
		return "此消息不存在";

	case "error0045"_hash:
		return "查找失敗，1、不是群內成員，2、此群不存在";

	case "error0046"_hash:
		return "踢出群成員失敗，1、不是群主，2、此群或群成員不存在";

	case "error0047"_hash:
		return "添加群成員失敗，1、不是群主，2、此群不存在";

	case "error0048"_hash:
		return "添加黑名單失敗";

	case "error0049"_hash:
		return "刪除黑名單失敗";

	case "error00410"_hash:
		return "與此用戶無短信息記錄";

	case "error00411"_hash:
		return "此消息已讀，或此用戶無未讀消息";

	case "error00412"_hash:
		return "群聊刪除或退出失敗，請檢查自己是否是群聊發起者";

	case "error0050"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0051"_hash:
		return "充值卡不存在！";

	case "error0052"_hash:
		return "充值卡已經被使用！";

	case "error0053"_hash:
		return "充值卡狀態更新出錯！";

	case "error0054"_hash:
		return "充值卡積分操作錯誤！";

	case "error0055"_hash:
		return "時間充值失敗，此帳號為永久免費用戶！";

	case "error0056"_hash:
		return "此充值卡密不存在！";

	case "error0060"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0061"_hash:
		return "請設置後台交易積分種類！";

	case "error0062"_hash:
		return "抱歉您查詢的面值種類沒有卡密了！";

	case "error0063"_hash:
		return "抱歉您提取的卡密數量，庫存不足，請聯繫管理員上貨！";

	case "error0064"_hash:
		return "抱歉您購買卡密的積分不足，請及時充值！";

	case "error0065"_hash:
		return "對不起，您所在會員組不允許使用卡密操作！";

	case "error0071"_hash:
		return "郵件發送失敗，請檢查後台SMTP郵件參數！";

	case "error0072"_hash:
		return "手機未綁定帳號，不能發送消息！";

	case "error0073"_hash:
		return "手機未綁定帳號，請綁定帳號後登入！";

	case "error0074"_hash:
		return "阿里雲短信接口已關閉！";

	case "error0075"_hash:
		return "阿里雲短信發送失敗！";

	case "error0076"_hash:
		return "阿里雲短信發送時，必須輸入參數值code或content！";

	case "error0077"_hash:
		return "規定時間內禁止多次發送！";

	case "error0078"_hash:
		return "手機登入功能已關閉！";

	case "error0079"_hash:
		return "手機驗證碼輸入錯誤！";

	case "error00710"_hash:
		return "此手機號已經綁定帳號，請使用手機登入功能！";

	case "error00711"_hash:
		return "此用戶已經綁定帳號！";

	case "error00712"_hash:
		return "充值金額要大於0！";

	case "error00713"_hash:
		return "碼支付系統未開啟！";

	case "error00714"_hash:
		return "遠程文件獲取失敗，請更換其他地址！";

	case "error00715"_hash:
		return "遠程下載功能關閉，請到後台設置遠程下載擴展名！";

	case "error00716"_hash:
		return "此文件擴展名禁止下載！";

	case "error00717"_hash:
		return "遠程文件大小超過最大允許值，請在後台設置中調整！";

	case "error00718"_hash:
		return "兩個原因：1、cookies提交錯誤；2、用戶未登入過驗證系統，請使用登入接口激活一次！";

	case "error0080"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0081"_hash:
		return "抱歉，您不是管理員無法操作！";

	case "error0082"_hash:
		return "請輸入支付寶或微信打款帳號！";

	case "error0090"_hash:
		return "未知錯誤，請聯繫客服！";

	case "error0091"_hash:
		return "這里是免費用戶組，不支持購買！";

	case "error0092"_hash:
		return "未設置用戶組購買積分！";

	case "error00101"_hash:
		return "未查詢到附件";

	case "error00102"_hash:
		return "此附件無人購買";

	case "error00103"_hash:
		return "此附件未購買";

	case "error00104"_hash:
		return "您未有出售附件";
	}

	return "\0";
}
#endif

namespace AntiCaptcha
{
	size_t httpWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}

	void httpInit(CURL* curl, RequestType type, ContentType format, AcceptType acceptType, long timeout, std::vector<struct curl_slist*>& headers_)
	{
		curl_easy_reset(curl);
		if (GET == type)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, 0L);
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
		}



		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 跳過 SSL 驗證
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L); // 關閉 response header
		curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L); // 自動設置 referer
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 自動跟隨 302 重定向
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // 不使用 signal
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout); // 設置連接超時時間
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout + 10L); // 設置總超時時間
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpWriteCallback);


		struct curl_slist* headers = nullptr;

		if (headers)
		{
			curl_slist_free_all(headers);
			headers = nullptr;
		}

		headers = curl_slist_append(headers, "Authority: www.bluecg.net");
		if (ACCEPT_IMAGE == acceptType)
			headers = curl_slist_append(headers, "Accept: image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8");
		else
			headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
		headers = curl_slist_append(headers, "Accept-Language: zh-TW,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6");
		headers = curl_slist_append(headers, "Accept-Encoding: *");
		headers = curl_slist_append(headers, "Cache-Control: max-age=0");
		if (CONTENT_JSON == format)
			headers = curl_slist_append(headers, "Content-Type: application/json");
		else if (CONTENT_MIME == format)
			headers = curl_slist_append(headers, "Content-Type: multipart/form-data");
		else
			headers = curl_slist_append(headers, "Content-Type: text/plain");
		headers = curl_slist_append(headers, "Origin: https://www.bluecg.net");
		//QString qRefer(QString("Referer: %1").arg(url_));
		//std::string sRefer(qRefer.toStdString());
		//headers = curl_slist_append(headers, sRefer.c_str());
		headers = curl_slist_append(headers, "sec-ch-ua: \"Microsoft Edge\";v=\"111\", \"Not(A:Brand\";v=\"8\", \"Chromium\";v=\"111\"");
		headers = curl_slist_append(headers, "sec-ch-ua-mobile: ?0");
		headers = curl_slist_append(headers, "sec-ch-ua-platform: \"Windows\"");
		headers = curl_slist_append(headers, "sec-fetch-dest: document");
		headers = curl_slist_append(headers, "sec-fetch-mode: navigate");
		headers = curl_slist_append(headers, "sec-fetch-site: same-origin");
		headers = curl_slist_append(headers, "sec-fetch-user: ?1");
		headers = curl_slist_append(headers, "upgrade-insecure-requests: 1");

		headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/111.0.0.0 Safari/537.36 Edg/111.0.1661.62");


		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		if (headers)
			headers_.push_back(headers);
	}

	bool httpPostCodeImage(const QImage& img, QString* pmsg, QString& gifCode_)
	{
		curl_mime* mime = nullptr;

		bool bret = false;

		CURL* curl = curl_easy_init();

		do
		{

			if (!pmsg)
				break;

			if (!curl)
			{
				*pmsg = "Invalid curl handle.";
				break;
			}

			std::vector<struct curl_slist*> header;
			httpInit(curl, POST, CONTENT_MIME, ACCEPT_TEXT, 5000, header);

			mime = curl_mime_init(curl);
			curl_mimepart* part = curl_mime_addpart(mime);
			curl_mime_name(part, "file");
			curl_mime_type(part, "image/png");
			curl_mime_filename(part, "image.png");
			//std::string sfilename = imgPath.toStdString();
			//curl_mime_filedata(part, sfilename.c_str());
			QBuffer buffer;
			buffer.open(QIODevice::WriteOnly);
			img.save(&buffer, "PNG"); // 將 QImage 存為 PNG 格式到 buffer
			QByteArray byteData = buffer.buffer(); // 獲取 QByteArray
			std::string data(byteData.constData(), byteData.size());
			curl_mime_data(part, data.c_str(), data.size());
			curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

			std::string responseBuffer = "\0";
			curl_easy_setopt(curl, CURLOPT_URL, "http://198.13.52.137:58889/verify_code/");
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&responseBuffer);

			CURLcode res = curl_easy_perform(curl);
			if (res != CURLE_OK)
			{
				*pmsg = QString("curl_easy_perform failed: %1 [%2]").arg(curl_easy_strerror(res)).arg(__LINE__);
				break;
			}

			// Process responseBuffer as needed
			QJsonParseError jerr;
			const QString retstring(QString::fromStdString(responseBuffer));
			QJsonDocument responseDoc = QJsonDocument::fromJson(retstring.toUtf8(), &jerr);
			if (jerr.error != QJsonParseError::NoError)
			{
				*pmsg = QString("json parse error: %1").arg(jerr.errorString());
				break;
			}

			QJsonObject responseObject = responseDoc.object();
			if (!responseObject.contains("status"))
			{
				*pmsg = "Response does not contain 'status' field";
				break;
			}
			QString status = responseObject["status"].toString();

			if (status != "success")
			{
				if (responseObject.contains("msg"))
					*pmsg = responseObject["msg"].toString();
				else
					*pmsg = "Response does not contain 'msg' field";

				break;
			}


			if (responseObject.contains("msg"))
			{

				const QString gifCode = responseObject["msg"].toString();
				//檢查是否為4位數純英文大小寫數字
				static QRegularExpression re("[a-zA-Z0-9]{3,4}");
				const QRegularExpressionMatch match = re.match(gifCode);
				if (!match.hasMatch())
				{
					*pmsg = "Response 'msg' field is not 3 or 4 digits";
					break;
				}

				bret = true;
				gifCode_ = gifCode;
				pmsg->clear();
				qDebug() << "gif code: " << gifCode_;

			}
			else
				gifCode_ = "Response does not contain 'msg' field";


		} while (false);

		if (mime)
		{
			curl_mime_free(mime);
		}

		if (curl)
		{
			curl_easy_cleanup(curl);
		}

		return bret;
	}
}