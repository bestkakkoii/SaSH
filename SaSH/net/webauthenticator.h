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

#ifndef WEBAUTHENTICATOR_H
#define WEBAUTHENTICATOR_H

#include <curl/curl.h>

namespace Net
{

	constexpr int VIP_GROUP_ID = 20;
	class Authenticator : public QObject
	{
		Q_OBJECT;
		Q_DISABLE_COPY_MOVE(Authenticator)
	public:
		typedef enum
		{
			LocalRecordGameData = 1,
			LocalRecordGameAccount = 3,
			LocalMachineUniqueId = 4,
			LocalVerison = 5,
			LocalRecordLoginedAccount = 6,
		}XinLocal;

		typedef struct userinfo_s
		{
			int adminid;
			int credits;
			QString email;
			QString dateline;
			int groupexpiry;
			int groupid;
			QString grouptitle;
			int jiaoyi;
			QString phone;
			QString qq;
			QString sgin;
			bool status;
			QString username;
			QString weixin;
		} userinfo_t;

		enum XinGlobal
		{
			GlobalAccountBlackList = 9,
			GlobalMachineUniqueIdBlackList = 10,
		};

		static Authenticator* getInstance()
		{
			static Authenticator* instance = new Authenticator();
			return instance;
		}
	private:
		mutable QReadWriteLock m_lock;

		class KeyManager
		{
		public:
			inline explicit KeyManager(const QString& src) { encrypt(src, &m_private_key); }

			Q_REQUIRED_RESULT inline QString GetKey() const { return decrypt(m_private_key); }
			inline void SetKey(const QString& key) { encrypt(key, &m_private_key); }
			inline void SetCardToken(const QString& token) { encrypt(token, &m_private_card_token); }
			inline void SetUser(const QString& username, const QString& password)
			{
				encrypt(username, &m_private_username);
				encrypt(password, &m_private_password);
			}
			inline void SetUid(unsigned int uid) { encrypt(QString::number(uid), &m_private_uid); }
			Q_REQUIRED_RESULT inline QString GetUsername() const { return decrypt(m_private_username); }
			Q_REQUIRED_RESULT inline QString GetPassword() const { return decrypt(m_private_password); }
			Q_REQUIRED_RESULT inline QString GetCardToken() const { return decrypt(m_private_card_token); }
			Q_REQUIRED_RESULT inline QString GetToken() const { return decrypt(m_private_token); }
			Q_REQUIRED_RESULT inline unsigned int GetUid() const { return decrypt(m_private_uid).toUInt(); }
			inline void SetToken(const QString& token) { encrypt(token, &m_private_token); }
			inline void Clear()
			{
				m_private_card_token.clear();
				m_private_token.clear();
				m_private_key.clear();
			}

		private:
			QByteArray m_private_card_token;
			QByteArray m_private_username;
			QByteArray m_private_password;
			QByteArray m_private_token;
			QByteArray m_private_key;
			QByteArray m_private_uid;

			inline void encrypt(const QString& src, QByteArray* dst)
			{
				QByteArray data;
				data.append(src.toUtf8());

				QByteArray encryptedData;
				int size = data.size();
				for (int i = 0; i < size; ++i)
				{
					encryptedData.append(data[i] ^ 0xff);
					encryptedData[i] = (encryptedData[i] & 0x03) << 6 |
						(encryptedData[i] & 0x0c) << 2 |
						(encryptedData[i] & 0x30) >> 2 |
						(encryptedData[i] & 0xc0) >> 6;
				}

				if (dst)
					*dst = encryptedData.toHex();
			}

			QString decrypt(const QByteArray& src) const
			{
				QByteArray data = QByteArray::fromHex(src);

				QByteArray decryptedData;
				int size = data.size();
				for (int i = 0; i < size; ++i)
				{
					unsigned char c = (data[i] & 0x03) << 6 |
						(data[i] & 0x0c) << 2 |
						(data[i] & 0x30) >> 2 |
						(data[i] & 0xc0) >> 6;
					decryptedData.append(c ^ 0xff);
				}

				return QString::fromUtf8(decryptedData);
			}
		};

	public:
		virtual ~Authenticator();

		Q_REQUIRED_RESULT inline unsigned int GetUid() const { return keyManager_.GetUid(); }
		bool GetSync(const std::string& message, QString* retstring, long timeout = 30);
		//static bool WINAPI PostSync(const std::string& message, QString fileName, QString* retstring, long timeout = 30);
		bool Login(const QString& username, const QString& password);
		bool Login();
		bool Logout();

		void SetLocal(int col, const QString& qstr);
		QString GetLocal(int col, bool decodeenable = true);
		QString GetGlobal(int row, int col, bool decodeenable = true);

		inline userinfo_t GetUser()
		{
			return user_;
		}

		inline void SetUser(const userinfo_t& user)
		{
			user_ = user;
		}

		inline void SetUserClear()
		{
			user_ = {};
		}

		bool SwitchUserGroup(int group);

		bool WINAPI LineNotifySync(const QString& qtoken, const QString& name, const QString& msg, bool hasimage, const QString& fileName = "");

		inline bool isMaster() const
		{
			if (user_.groupid == 1 || user_.groupid == 2 || user_.groupid == 3 || user_.dateline == "1970-01-01 08:00:00")
			{
				return true;
			}
			else
				return false;
		}

		inline bool isVIP() const
		{

			if (user_.groupid == VIP_GROUP_ID)
			{
				return true;
			}
			else
				return false;
		}

		inline bool isValidGroup() const
		{
			QReadLocker locker(&m_lock);
			if (isMaster() || isVIP())
				return true;
			else
				return false;
		}

		inline bool isValidGroupEx() const
		{
			if (isMaster())
				return true;
			else
				return false;
		}

		QString ReadAll(const QUrl& url);
		std::string urlencode(const QString qsrc);

	signals:
		void msgImport(QString msg);

	private:
		explicit Authenticator();

		bool CHECKJSON(const QString& str, QJsonDocument* doc);
		bool CHECKJSON2(const QString& str, QJsonDocument* doc);
		void LoadKey();

		std::string MakeUrl(const QStringList& contents);

		void ClearUser() { user_ = {}; }


		QString GetErrorString(const QString& errorCode);

		int GetUserCredit(const QString& username, const char* type, int* uid);
		bool ExchangeCardTime(int n, int uid, const QString& password);
		void SetStatusSync();
		bool SendEcho(int max);

		QString GetAdminKey();
		bool SetPoint(int point, int ptype = 4);
		void CheckUserGroup();
		bool CheckUserState();
		bool CheckCurrentGroupHas(int g);
		bool SetBuyUserGroup(int point, int group);


		userinfo_t user_ = {};

		KeyManager keyManager_;

		mutable QMutex mutex_;
	};

}


#pragma region AntiCodeImage

namespace AntiCaptcha
{
#include <vector>
	enum RequestType
	{
		GET,
		POST,
	};

	enum AcceptType
	{
		ACCEPT_TEXT,
		ACCEPT_IMAGE,
	};

	enum ContentType
	{
		CONTENT_JSON,
		CONTENT_MIME,
		CONTENT_TEXT,
	};

	enum SiteKeyType
	{
		kSiteCloudFlare,
		kSiteGoogle,
	};

	enum Encode
	{
		kEncodeBig5,
		kEncodeUTF8,
	};

	enum SessionType
	{
		kSessionBlue,
		kSessionAnti,
	};

	size_t httpWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

	void httpInit(CURL* curl, RequestType type, ContentType format, AcceptType acceptType, long timeout, std::vector<struct curl_slist*>& headers_);

	bool httpPostCodeImage(const QImage& img, QString* pmsg, QString& gifCode_);

#if 0
	QString generateRandomHash();
#endif
}
#pragma endregion

#endif // WEBAUTHENTICATOR_H