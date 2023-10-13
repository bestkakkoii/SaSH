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

#include "stdafx.h"
#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <QFile>

#include "util.h"

#ifdef _DEBUG
#pragma comment(lib, "libcrypto32MDd.lib")
#pragma comment(lib, "libssl32MDd.lib")
#else
#pragma comment(lib, "libcrypto32MD.lib")
#pragma comment(lib, "libssl32MD.lib")
#endif
#pragma comment(lib, "crypt32.lib")

// 4096 bits IV
constexpr char IV[] = "cWx2tHE.qRy7x2#";

constexpr char RSA_PUBLIC_KEY[] = R"(-----BEGIN RSA PUBLIC KEY-----
MIICCgKCAgEAt4+s70iSydi908ys0JA6ySBHZtI04VjIzB5MxaOyaqzbEyJjOwTe
OyIsDrB2ZopSroJh9FG4wSJlXObJVVOyRjD0hjpb1ykFxGn3AOOoj03Qv6hvWCyo
vT3AxVi2EfwwlqpbTGawMgDavtfDu5at9YAbLxd60LZFZwMwcjywEVYSJ3DmPDLv
kA2wO6IHIMIRjlA2p5NMUG5YuM1auHK4weCLlx/z9aQKW14yjm3yrDawTYMX0r3h
h9drVOg2GkdGiAO6YgB9qlMxDkZxHrQBkkiNGg/hzMna/a9/2zWYhHRYaFIjp13z
wJ9f95S7HklOmHw8U6psklyoYV7yCllckER5OG15L1lHCs7yyIph0oW+p+w8Oslk
KYGpa1LmKz7sfN6MfhOyvryBZEGrBZdvqewbq9PvxKb7x+kDqClJvgn0Ddb1v5zd
SmNv1oEd7wWJqPdHmp9neuxGG7dTUb4Bsd5yxypces8ajo/m1ns4+tTOrkL4oUK0
8UdoaCTUJQXFhk4lvYF6Ae+G9lqbIRi9ql9+2m5xcZNcbUcnJwjO1+NRLfkzeCQz
NCdWdoVroD+l+9GR3j56MrCp2uH7AAd0CK5NYeCLhDQ9RLPHGCu3vr+CLNyDVhhG
z7Dp513fsnQSLP5CpzYE/bElD++GsB8Uh6fXurqpOvhyzMda5cahhXkCAwEAAQ==
-----END RSA PUBLIC KEY-----)";

constexpr char RSA_PRIVATE_KEY[] = R"(-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
)";

Crypto::Crypto() {}

//void Crypto::generateAndSaveRSAKeys()
//{
//	int bits = 4096;
//	unsigned long e = RSA_F4;
//
//	BIGNUM* bne = BN_new();
//	BN_set_word(bne, e);
//
//	RSA* rsa = RSA_new();
//	RSA_generate_key_ex(rsa, bits, bne, NULL);
//
//	BIO* bioPublic = BIO_new(BIO_s_mem());
//	PEM_write_bio_RSAPublicKey(bioPublic, rsa);
//
//	BUF_MEM* publicKeyBuffer;
//	BIO_get_mem_ptr(bioPublic, &publicKeyBuffer);
//	QByteArray publicKey((char*)publicKeyBuffer->data, publicKeyBuffer->length);
//
//	QFile filePublic("d:/publicKey.pem");
//	filePublic.open(QIODevice::WriteOnly);
//	filePublic.write(publicKey);
//	filePublic.close();
//
//	BIO* bioPrivate = BIO_new(BIO_s_mem());
//	PEM_write_bio_RSAPrivateKey(bioPrivate, rsa, NULL, NULL, 0, NULL, NULL);
//
//	BUF_MEM* privateKeyBuffer;
//	BIO_get_mem_ptr(bioPrivate, &privateKeyBuffer);
//	QByteArray privateKey((char*)privateKeyBuffer->data, privateKeyBuffer->length);
//
//	QFile filePrivate("d:/privateKey.pem");
//	filePrivate.open(QIODevice::WriteOnly);
//	filePrivate.write(privateKey);
//	filePrivate.close();
//
//	// Don't forget to free resources
//	BIO_free_all(bioPublic);
//	BIO_free_all(bioPrivate);
//	RSA_free(rsa);
//	BN_free(bne);
//}

QByteArray Crypto::aesEncrypt(const QByteArray& data, const QByteArray& key)
{
	// Generate IV
	const unsigned char* iv = reinterpret_cast<const unsigned char*>(IV);

	// Create AES context
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (nullptr == ctx)
	{
		qWarning() << "EVP_CIPHER_CTX_new failed";
		return QByteArray();
	}

	int ret = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key.constData()), iv);
	if (ret != 1)
	{
		qWarning() << "EVP_EncryptInit_ex failed";
		return QByteArray();
	}

	// Calculate the size of the encrypted data
	int encryptedSize = data.size() + EVP_CIPHER_CTX_block_size(ctx);
	QByteArray encryptedData(encryptedSize, 0);

	// Perform encryption
	int len = 0;
	ret = EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(encryptedData.data()), &len, reinterpret_cast<const unsigned char*>(data.constData()), data.size());
	if (ret != 1)
	{
		qWarning() << "EVP_EncryptUpdate failed";
		return QByteArray();
	}

	int finalLen = 0;
	ret = EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(encryptedData.data()) + len, &finalLen);
	if (ret != 1)
	{
		qWarning() << "EVP_EncryptFinal_ex failed";
		return QByteArray();
	}

	// Cleanup
	if (ctx != nullptr)
		EVP_CIPHER_CTX_free(ctx);

	// Resize the encrypted data to the actual size
	encryptedData.resize(len + finalLen);
	return encryptedData;
}

QByteArray Crypto::aesDecrypt(const QByteArray& encryptedData, const QByteArray& key)
{
	// Generate IV
	const unsigned char* iv = reinterpret_cast<const unsigned char*>(IV);

	// Create AES context
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (nullptr == ctx)
	{
		qWarning() << "EVP_CIPHER_CTX_new failed";
		return QByteArray();
	}

	int ret = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, reinterpret_cast<const unsigned char*>(key.constData()), iv);
	if (ret != 1)
	{
		qDebug() << "Failed to initialize AES decryption";
		return QByteArray();
	}

	// Calculate the size of the decrypted data
	int decryptedSize = encryptedData.size() + EVP_CIPHER_CTX_block_size(ctx);
	QByteArray decryptedData(decryptedSize, 0);

	// Perform decryption
	int len = 0;
	ret = EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(decryptedData.data()), &len, reinterpret_cast<const unsigned char*>(encryptedData.constData()), encryptedData.size());
	if (ret != 1)
	{
		qDebug() << "Failed to decrypt data";
		return QByteArray();
	}

	int finalLen = 0;
	ret = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(decryptedData.data()) + len, &finalLen);
	if (ret != 1)
	{
		qDebug() << "Failed to finalize decryption";
		return QByteArray();
	}

	// Cleanup
	if (ctx != nullptr)
		EVP_CIPHER_CTX_free(ctx);

	// Resize the decrypted data to the actual size
	decryptedData.resize(len + finalLen);
	return decryptedData;
}

QByteArray Crypto::rsaEncrypt(const QByteArray& data, const QByteArray& publicKey)
{
	BIO* bio = BIO_new_mem_buf(publicKey.data(), publicKey.length());
	EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
	BIO_free(bio);

	if (pkey == nullptr)
	{
		std::cerr << "Failed to read public key" << std::endl;
		return QByteArray();
	}

	EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
	EVP_PKEY_free(pkey);

	if (ctx == nullptr)
	{
		std::cerr << "Failed to create EVP_PKEY_CTX" << std::endl;
		return QByteArray();
	}

	if (EVP_PKEY_encrypt_init(ctx) <= 0)
	{
		std::cerr << "Failed to initialize encryption" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
	{
		std::cerr << "Failed to set RSA padding" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	size_t encryptedLen;
	if (EVP_PKEY_encrypt(ctx, nullptr, &encryptedLen, reinterpret_cast<const unsigned char*>(data.data()), data.length()) <= 0)
	{
		std::cerr << "Failed to get encrypted data length" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	QByteArray encryptedData(encryptedLen, '\0');
	if (EVP_PKEY_encrypt(ctx, reinterpret_cast<unsigned char*>(encryptedData.data()), &encryptedLen, reinterpret_cast<const unsigned char*>(data.data()), data.length()) <= 0)
	{
		std::cerr << "Failed to encrypt data" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	EVP_PKEY_CTX_free(ctx);

	return encryptedData;
}

QByteArray Crypto::rsaDecrypt(const QByteArray& encryptedData, const QByteArray& privateKey)
{
	BIO* bio = BIO_new_mem_buf(privateKey.data(), privateKey.length());
	EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
	BIO_free(bio);

	if (pkey == nullptr)
	{
		std::cerr << "Failed to read private key" << std::endl;
		return QByteArray();
	}

	EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
	EVP_PKEY_free(pkey);

	if (ctx == nullptr)
	{
		std::cerr << "Failed to create EVP_PKEY_CTX" << std::endl;
		return QByteArray();
	}

	if (EVP_PKEY_decrypt_init(ctx) <= 0)
	{
		std::cerr << "Failed to initialize decryption" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
	{
		std::cerr << "Failed to set RSA padding" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	size_t decryptedLen;
	if (EVP_PKEY_decrypt(ctx, nullptr, &decryptedLen, reinterpret_cast<const unsigned char*>(encryptedData.data()), encryptedData.length()) <= 0)
	{
		std::cerr << "Failed to get decrypted data length" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	QByteArray decryptedData(decryptedLen, '\0');
	if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char*>(decryptedData.data()), &decryptedLen, reinterpret_cast<const unsigned char*>(encryptedData.data()), encryptedData.length()) <= 0)
	{
		std::cerr << "Failed to decrypt data" << std::endl;
		EVP_PKEY_CTX_free(ctx);
		return QByteArray();
	}

	EVP_PKEY_CTX_free(ctx);

	return decryptedData;

}

bool Crypto::encodeScript(const QString& scriptFileName, const QString& userAesKey)
{
	QFile file(scriptFileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		// Handle error
		return false;
	}
	
	do
	{
		QTextStream in(&file);
		in.setCodec("UTF-8");
		QString text = in.readAll();
		QByteArray scriptData = text.toUtf8();
		file.close();

		if (scriptData.isEmpty())
		{
			return false;
		}

		QByteArray encryptedScript = Crypto::aesEncrypt(scriptData, userAesKey.toUtf8());
		if (encryptedScript.isEmpty())
		{
			return false;
		}

		QByteArray encryptedKey = Crypto::rsaEncrypt(userAesKey.toUtf8(), RSA_PUBLIC_KEY);
		if (encryptedKey.isEmpty())
		{
			return false;
		}

		QFileInfo fileInfo(scriptFileName);
		QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.baseName() + util::SCRIPT_PRIVATE_SUFFIX_DEFAULT;
		QFile encryptedFile(newFileName);
		if (!encryptedFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			// Handle error
			return false;
		}

		//to viewable hex text
		QTextStream out(&encryptedFile);
		out.setCodec("UTF-8");
		out << encryptedKey.toHex() << '\n' << encryptedScript.toHex() << Qt::endl;

		encryptedFile.flush();
		encryptedFile.close();
	} while (false);

	return false;
}

bool Crypto::decodeScript(const QString& scriptFileName, QString& scriptContent)
{
	QFile encryptedFile(scriptFileName);
	if (!encryptedFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return false;
	}

	QTextStream in(&encryptedFile);
	in.setCodec("UTF-8");
	QString encryptedKey = in.readLine();
	if (encryptedKey.isEmpty())
	{
		encryptedFile.close();
		return false;
	}

	QString encryptedScript = in.readLine();
	if (encryptedScript.isEmpty())
	{
		encryptedFile.close();
		return false;
	}

	encryptedFile.close();

	QByteArray rawKey = QByteArray::fromHex(encryptedKey.toUtf8());
	QByteArray rawScript = QByteArray::fromHex(encryptedScript.toUtf8());

	QByteArray decryptedKey = Crypto::rsaDecrypt(rawKey, RSA_PRIVATE_KEY);
	if (decryptedKey.isEmpty())
	{
		return false;
	}

	QByteArray decryptedScript = Crypto::aesDecrypt(rawScript, decryptedKey);
	if (decryptedScript.isEmpty())
	{
		return false;
	}

	scriptContent = QString::fromUtf8(decryptedScript);

	return true;
}

bool Crypto::decodeScript(const QString& scriptFileName, const QString& userAesKey, QString& scriptContent)
{
	QFile encryptedFile(scriptFileName);
	if (!encryptedFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		// Handle error
		return false;
	}

	QTextStream in(&encryptedFile);
	in.setCodec("UTF-8");
	
	QString encryptedKey = in.readLine();
	if (encryptedKey.isEmpty())
	{
		encryptedFile.close();
		return false;
	}

	QString encryptedScript = in.readLine();
	if (encryptedScript.isEmpty())
	{
		encryptedFile.close();
		return false;
	}

	encryptedFile.close();

	QByteArray rawKey = QByteArray::fromHex(encryptedKey.toUtf8());

	QByteArray rawScript = QByteArray::fromHex(encryptedScript.toUtf8());

	QByteArray decryptedKey = Crypto::rsaDecrypt(rawKey, RSA_PRIVATE_KEY);
	if (decryptedKey.isEmpty())
	{
		return false;
	}

	QByteArray decryptedScript = Crypto::aesDecrypt(rawScript, userAesKey.toUtf8());
	if (decryptedScript.isEmpty())
	{
		return false;
	}

	scriptContent = QString::fromUtf8(decryptedScript);

	QFileInfo fileInfo(scriptFileName);
	QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.baseName() + util::SCRIPT_SUFFIX_DEFAULT;
	QFile decryptedFile(newFileName);
	if (!decryptedFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		// Handle error
		return false;
	}

	QTextStream out(&decryptedFile);
	out.setCodec("UTF-8");

	out << scriptContent;

	decryptedFile.flush();
	decryptedFile.close();
	return true;
}