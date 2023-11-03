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
constexpr char IV[] = "s-rvfD%mEz*@TF2E";

constexpr char RSA_PUBLIC_KEY[] = R"(-----BEGIN RSA PUBLIC KEY-----
MIICCgKCAgEAnsSaCpOAED2JSNjOCp93b6ZKB1gyn+vGcLJAyTYL0O12jl00lC/S
ZmqaxLdeofICMHAPXP91pPL7EUAY/JnGbgfHDRqzqgOZZw3XkiI0I4HtvaZnYTHc
97+7sqrhKH27ciMyd7JITMEzI03bzrsUiT4BuFsnQ9CUulahUwOXPX/24pS+/fX1
aJzkkBJLTCU1/VpUWpj7/T5Ny3uEaS9i2u7+OkpFPNC/49gZUN9iGlkUhrdJivp9
BGzVPZ6COBG7wsf7xfbvYe4khYkxM5I/GH27q3shZefyQHVfio2kNrezcF9Fuv71
/2LTEscyy3scvCk2dHZVtyJ4jc+s0pJO1d1viWOcyx+EMRXpyTxu19DjMQ0QBkXz
F13XafjNqd7xj/Va7cI7CfkBaURnu2uY01W5PjcczjNaXhgI0EH1TE8KXCTVsBIb
unUzjdFHllTomipjvhn/JrEMbZ9HbxkXW/L2wMBqIIfAZPR60RTIw0ojpfv/mpMd
NOusCySYlXAMjJRiMKf6ETZbM6VWd3EG/d4FPBdYmxAOqm+RQd2PXKiDWNZpzOLy
euIVEE3FkSFwycBqKbl6BdOAlIIdrDpjc3ed6L6eca21H54HziBCyXCn9p9yUQdi
dPQ8b688FSxHDVkemQ3Efw0UKxlF+hn9lPd53cCsu5SyrJS5QN8Zf+MCAwEAAQ==
-----END RSA PUBLIC KEY-----)";

constexpr char RSA_PRIVATE_KEY[] = R"(-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----)";

Crypto::Crypto()
{
}

#if 0
void Crypto::generateAndSaveRSAKeys()
{
	int bits = 4096;
	unsigned long e = RSA_F4;

	BIGNUM* bne = BN_new();
	BN_set_word(bne, e);

	RSA* rsa = RSA_new();
	RSA_generate_key_ex(rsa, bits, bne, NULL);

	BIO* bioPublic = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPublicKey(bioPublic, rsa);

	BUF_MEM* publicKeyBuffer;
	BIO_get_mem_ptr(bioPublic, &publicKeyBuffer);
	QByteArray publicKey((char*)publicKeyBuffer->data, publicKeyBuffer->length);

	QFile filePublic("d:/publicKey.pem");
	filePublic.open(QIODevice::WriteOnly);
	filePublic.write(publicKey);
	filePublic.close();

	BIO* bioPrivate = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPrivateKey(bioPrivate, rsa, NULL, NULL, 0, NULL, NULL);

	BUF_MEM* privateKeyBuffer;
	BIO_get_mem_ptr(bioPrivate, &privateKeyBuffer);
	QByteArray privateKey((char*)privateKeyBuffer->data, privateKeyBuffer->length);

	QFile filePrivate("d:/privateKey.pem");
	filePrivate.open(QIODevice::WriteOnly);
	filePrivate.write(privateKey);
	filePrivate.close();

	// Don't forget to free resources
	BIO_free_all(bioPublic);
	BIO_free_all(bioPrivate);
	RSA_free(rsa);
	BN_free(bne);
}
#endif

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
	QByteArray scriptData;

	do
	{
		{
			util::ScopedFile file(scriptFileName);
			if (!file.openRead())
			{
				// Handle error
				break;
			}

			util::TextStream in(&file);
			QString text = in.readAll();
			scriptData = text.toUtf8();
		}

		if (scriptData.isEmpty())
		{
			break;
		}

		QByteArray encryptedScript = Crypto::aesEncrypt(scriptData, userAesKey.toUtf8());
		if (encryptedScript.isEmpty())
		{
			break;
		}

		QByteArray encryptedKey = Crypto::rsaEncrypt(userAesKey.toUtf8(), RSA_PUBLIC_KEY);
		if (encryptedKey.isEmpty())
		{
			break;
		}

		QFileInfo fileInfo(scriptFileName);
		QString newFileName = fileInfo.absolutePath() + "/" + fileInfo.baseName() + util::SCRIPT_PRIVATE_SUFFIX_DEFAULT;

		{
			util::ScopedFile encryptedFile(newFileName);
			if (!encryptedFile.openWriteNew())
			{
				// Handle error
				break;
			}

			//to viewable hex text
			util::TextStream out(&encryptedFile);
			out << encryptedKey.toHex() << '\n' << encryptedScript.toHex() << Qt::endl;
		}

		return true;
	} while (false);

	return false;
}

bool Crypto::decodeScript(const QString& scriptFileName, QString& scriptContent)
{
	QString encryptedKey = "";
	QString encryptedScript = "";

	do
	{
		{
			util::ScopedFile encryptedFile(scriptFileName);
			if (!encryptedFile.openRead())
			{
				break;
			}

			util::TextStream in(&encryptedFile);
			encryptedKey = in.readLine();
			if (encryptedKey.isEmpty())
			{
				break;
			}

			encryptedScript = in.readLine();
			if (encryptedScript.isEmpty())
			{
				break;
			}
		}

		QByteArray rawKey = QByteArray::fromHex(encryptedKey.toUtf8());
		QByteArray rawScript = QByteArray::fromHex(encryptedScript.toUtf8());

		QByteArray decryptedKey = Crypto::rsaDecrypt(rawKey, RSA_PRIVATE_KEY);
		if (decryptedKey.isEmpty())
		{
			break;
		}

		QByteArray decryptedScript = Crypto::aesDecrypt(rawScript, decryptedKey);
		if (decryptedScript.isEmpty())
		{
			break;
		}

		scriptContent = QString::fromUtf8(decryptedScript);

		return true;
	} while (false);

	return false;
}

bool Crypto::decodeScript(QString& scriptFileName, const QString& userAesKey, QString& scriptContent)
{
	QString encryptedKey = "";
	QString encryptedScript = "";

	do
	{
		{
			util::ScopedFile encryptedFile(scriptFileName);
			if (!encryptedFile.openRead())
			{
				// Handle error
				break;
			}

			util::TextStream in(&encryptedFile);

			encryptedKey = in.readLine();
			if (encryptedKey.isEmpty())
			{
				break;
			}

			encryptedScript = in.readLine();
			if (encryptedScript.isEmpty())
			{
				break;
			}
		}

		QByteArray rawKey = QByteArray::fromHex(encryptedKey.toUtf8());

		QByteArray rawScript = QByteArray::fromHex(encryptedScript.toUtf8());

		QByteArray decryptedKey = Crypto::rsaDecrypt(rawKey, RSA_PRIVATE_KEY);
		if (decryptedKey.isEmpty())
		{
			break;
		}

		QByteArray decryptedScript = Crypto::aesDecrypt(rawScript, userAesKey.toUtf8());
		if (decryptedScript.isEmpty())
		{
			break;
		}

		scriptContent = QString::fromUtf8(decryptedScript);

		QFileInfo fileInfo(scriptFileName);
		QString newFileName = QString("%1/%2_decoded%3").arg(fileInfo.absolutePath()).arg(fileInfo.baseName()).arg(util::SCRIPT_DEFAULT_SUFFIX);

		{
			util::ScopedFile decryptedFile(newFileName);
			if (!decryptedFile.openWriteNew())
			{
				// Handle error
				break;
			}

			util::TextStream out(&decryptedFile);

			out << scriptContent;
		}

		scriptFileName = newFileName;

		return true;
	} while (false);

	return false;
}