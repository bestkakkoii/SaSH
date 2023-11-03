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

#ifndef CRYPTO_H
#define CRYPTO_H
#include <QByteArray>

class Crypto
{
public:
	Crypto();
	virtual ~Crypto() = default;

public:
	bool encodeScript(const QString& scriptFileName, const QString& userAesKey);
	bool decodeScript(QString& scriptFileName, const QString& userAesKey, QString& scriptContent);
	bool decodeScript(const QString& scriptFileName, QString& scriptContent);

private:
	QByteArray aesEncrypt(const QByteArray& data, const QByteArray& key);
	QByteArray aesDecrypt(const QByteArray& encryptedData, const QByteArray& key);
	QByteArray rsaEncrypt(const QByteArray& data, const QByteArray& publicKey);
	QByteArray rsaDecrypt(const QByteArray& encryptedData, const QByteArray& privateKey);
	void generateAndSaveRSAKeys();
};

#endif // CRYPTO_H