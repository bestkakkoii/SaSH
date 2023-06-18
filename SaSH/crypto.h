#pragma once
#include <QByteArray>

class Crypto
{
public:
	Crypto();
	virtual ~Crypto() = default;

public:
	bool encodeScript(const QString& scriptFileName, const QString& userAesKey);
	bool decodeScript(const QString& scriptFileName, const QString& userAesKey, QString& scriptContent);
	bool decodeScript(const QString& scriptFileName, QString& scriptContent);

private:
	QByteArray aesEncrypt(const QByteArray& data, const QByteArray& key);
	QByteArray aesDecrypt(const QByteArray& encryptedData, const QByteArray& key);
	QByteArray rsaEncrypt(const QByteArray& data, const QByteArray& publicKey);
	QByteArray rsaDecrypt(const QByteArray& encryptedData, const QByteArray& privateKey);
	//void generateAndSaveRSAKeys();
};