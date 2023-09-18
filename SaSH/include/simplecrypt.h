/*
Copyright (c) 2011, Andre Somers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Rathenau Instituut, Andre Somers nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANDRE SOMERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR #######; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SIMPLECRYPT_H
#define SIMPLECRYPT_H
#include <QString>
#include <QVector>
#include <QFlags>
#include <QByteArray>
#include <QtDebug>
#include <QtGlobal>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDataStream>
#include <QRandomGenerator>

/**
  @short Simple encryption and decryption of strings and byte arrays

  This class provides a simple implementation of encryption and decryption
  of strings and byte arrays.

  @warning The encryption provided by this class is NOT strong encryption. It may
  help to shield things from curious eyes, but it will NOT stand up to someone
  determined to break the encryption. Don't say you were not warned.

  The class uses a 64 bit key. Simply create an instance of the class, set the key,
  and use the encryptToString() method to calculate an encrypted version of the input string.
  To decrypt that string again, use an instance of SimpleCrypt initialized with
  the same key, and call the decryptToString() method with the encrypted string. If the key
  matches, the decrypted version of the string will be returned again.

  If you do not provide a key, or if something else is wrong, the encryption and
  decryption function will return an empty string or will return a string containing nonsense.
  lastError() will return a value indicating if the method was succesful, and if not, why not.

  SimpleCrypt is prepared for the case that the encryption and decryption
  algorithm is changed in a later version, by prepending a version identifier to the cypertext.
  */
class SimpleCrypt
{
public:
	/**
	  CompressionMode describes if compression will be applied to the data to be
	  encrypted.
	  */
	enum CompressionMode
	{
		CompressionAuto,    /*!< Only apply compression if that results in a shorter plaintext. */
		CompressionAlways,  /*!< Always apply compression. Note that for short inputs, a compression may result in longer data */
		CompressionNever    /*!< Never apply compression. */
	};
	/**
	  IntegrityProtectionMode describes measures taken to make it possible to detect problems with the data
	  or wrong decryption keys.

	  Measures involve adding a checksum or a cryptograhpic hash to the data to be encrypted. This
	  increases the length of the resulting cypertext, but makes it possible to check if the plaintext
	  appears to be valid after decryption.
	*/
	enum IntegrityProtectionMode
	{
		ProtectionNone,    /*!< The integerity of the encrypted data is not protected. It is not really possible to detect a wrong key, for instance. */
		ProtectionChecksum,/*!< A simple checksum is used to verify that the data is in order. If not, an empty string is returned. */
		ProtectionHash     /*!< A cryptographic hash is used to verify the integrity of the data. This method produces a much stronger, but longer check */
	};
	/**
	  Error describes the type of error that occured.
	  */
	enum Error
	{
		ErrorNoError,         /*!< No error occurred. */
		ErrorNoKeySet,        /*!< No key was set. You can not encrypt or decrypt without a valid key. */
		ErrorUnknownVersion,  /*!< The version of this data is unknown, or the data is otherwise not valid. */
		ErrorIntegrityFailed, /*!< The integrity check of the data failed. Perhaps the wrong key was used. */
	};

	//enum to describe options that have been used for the encryption. Currently only one, but
	//that only leaves room for future extensions like adding a cryptographic hash...
	enum CryptoFlags
	{
		CryptoFlagNone = 0,
		CryptoFlagCompression = 0x01,
		CryptoFlagChecksum = 0x02,
		CryptoFlagHash = 0x04
	};

	/**
	  Constructor.

	  Constructs a SimpleCrypt instance without a valid key set on it.
	 */
	SimpleCrypt() :
		m_key(0),
		m_compressionMode(CompressionAuto),
		m_protectionMode(ProtectionChecksum),
		m_lastError(ErrorNoError)
	{}
	/**
	  Constructor.

	  Constructs a SimpleCrypt instance and initializes it with the given @arg key.
	 */
	explicit SimpleCrypt(quint64 key) :
		m_key(key),
		m_compressionMode(CompressionAuto),
		m_protectionMode(ProtectionChecksum),
		m_lastError(ErrorNoError)
	{
		splitKey();
	}

	/**
	  (Re-) initializes the key with the given @arg key.
	  */
	void setKey(quint64 key)
	{
		m_key = key;
		splitKey();
	}
	/**
	  Returns true if SimpleCrypt has been initialized with a key.
	  */
	bool hasKey() const { return !m_keyParts.isEmpty(); }

	/**
	  Sets the compression mode to use when encrypting data. The default mode is Auto.

	  Note that decryption is not influenced by this mode, as the decryption recognizes
	  what mode was used when encrypting.
	  */
	void setCompressionMode(CompressionMode mode) { m_compressionMode = mode; }
	/**
	  Returns the CompressionMode that is currently in use.
	  */
	CompressionMode compressionMode() const { return m_compressionMode; }

	/**
	  Sets the integrity mode to use when encrypting data. The default mode is Checksum.

	  Note that decryption is not influenced by this mode, as the decryption recognizes
	  what mode was used when encrypting.
	  */
	void setIntegrityProtectionMode(IntegrityProtectionMode mode) { m_protectionMode = mode; }
	/**
	  Returns the IntegrityProtectionMode that is currently in use.
	  */
	IntegrityProtectionMode integrityProtectionMode() const { return m_protectionMode; }

	/**
	  Returns the last error that occurred.
	  */
	Error lastError() const { return m_lastError; }

	/**
	  Encrypts the @arg plaintext string with the key the class was initialized with, and returns
	  a cyphertext the result. The result is a base64 encoded version of the binary array that is the
	  actual result of the string, so it can be stored easily in a text format.
	  */
	QString encryptToString(const QString& plaintext)
	{
		QByteArray plaintextArray = plaintext.toUtf8();
		QByteArray cypher = encryptToByteArray(plaintextArray);
		QString cypherString = QString::fromLatin1(cypher.toBase64());
		return cypherString;
	}
	/**
	  Encrypts the @arg plaintext QByteArray with the key the class was initialized with, and returns
	  a cyphertext the result. The result is a base64 encoded version of the binary array that is the
	  actual result of the encryption, so it can be stored easily in a text format.
	  */
	QString encryptToString(QByteArray plaintext)
	{
		QByteArray cypher = encryptToByteArray(plaintext);
		QString cypherString = QString::fromLatin1(cypher.toBase64());
		return cypherString;
	}
	/**
	  Encrypts the @arg plaintext string with the key the class was initialized with, and returns
	  a binary cyphertext in a QByteArray the result.

	  This method returns a byte array, that is useable for storing a binary format. If you need
	  a string you can store in a text file, use encryptToString() instead.
	  */
	QByteArray encryptToByteArray(const QString& plaintext)
	{
		QByteArray plaintextArray = plaintext.toUtf8();
		return encryptToByteArray(plaintextArray);
	}
	/**
	  Encrypts the @arg plaintext QByteArray with the key the class was initialized with, and returns
	  a binary cyphertext in a QByteArray the result.

	  This method returns a byte array, that is useable for storing a binary format. If you need
	  a string you can store in a text file, use encryptToString() instead.
	  */
	QByteArray encryptToByteArray(QByteArray plaintext)
	{
		if (m_keyParts.isEmpty())
		{
			qWarning() << "No key set.";
			m_lastError = ErrorNoKeySet;
			return QByteArray();
		}


		QByteArray ba = plaintext;

		quint64 flags = CryptoFlagNone;
		if (m_compressionMode == CompressionAlways)
		{
			ba = qCompress(ba, 9); //maximum compression
			flags |= CryptoFlagCompression;
		}
		else if (m_compressionMode == CompressionAuto)
		{
			QByteArray compressed = qCompress(ba, 9);
			if (compressed.size() < ba.size())
			{
				ba = compressed;
				flags |= CryptoFlagCompression;
			}
		}

		QByteArray integrityProtection;
		if (m_protectionMode == ProtectionChecksum)
		{
			flags |= CryptoFlagChecksum;
			QDataStream s(&integrityProtection, QIODevice::WriteOnly);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			s << qChecksum(ba.constData(), ba.length());
#else
			//use QByteArrayView overload
			s << qChecksum(QByteArrayView(ba));
#endif
		}
		else if (m_protectionMode == ProtectionHash)
		{
			flags |= CryptoFlagHash;
			QCryptographicHash hash(QCryptographicHash::Sha1);
			hash.addData(ba);

			integrityProtection += hash.result();
		}

		//prepend a random char to the string
		char randomChar = char(QRandomGenerator::global()->generate() & 0xFF);
		ba = randomChar + integrityProtection + ba;

		int pos(0);
		char lastChar(0);

		int cnt = ba.size();

		while (pos < cnt)
		{
			if (pos >= 0 && pos < ba.size() && ((pos % 8) < m_keyParts.size()))
			{
				ba[pos] = ba.at(pos) ^ m_keyParts.at(pos % 8) ^ lastChar;
				lastChar = ba.at(pos);
				++pos;
			}
		}

		QByteArray resultArray;
		resultArray.append(char(0x03));  //version for future updates to algorithm
		resultArray.append(char(flags)); //encryption flags
		resultArray.append(ba);

		m_lastError = ErrorNoError;
		return resultArray;
	}

	/**
	  Decrypts a cyphertext string encrypted with this class with the set key back to the
	  plain text version.

	  If an error occured, such as non-matching keys between encryption and decryption,
	  an empty string or a string containing nonsense may be returned.
	  */
	QString decryptToString(const QString& cyphertext)
	{
		QByteArray cyphertextArray = QByteArray::fromBase64(cyphertext.toLatin1());
		QByteArray plaintextArray = decryptToByteArray(cyphertextArray);
		QString plaintext = QString::fromUtf8(plaintextArray, plaintextArray.size());

		return plaintext;
	}
	/**
	  Decrypts a cyphertext string encrypted with this class with the set key back to the
	  plain text version.

	  If an error occured, such as non-matching keys between encryption and decryption,
	  an empty string or a string containing nonsense may be returned.
	  */
	QByteArray decryptToByteArray(const QString& cyphertext)
	{
		QByteArray cyphertextArray = QByteArray::fromBase64(cyphertext.toLatin1());
		QByteArray ba = decryptToByteArray(cyphertextArray);

		return ba;
	}
	/**
	  Decrypts a cyphertext binary encrypted with this class with the set key back to the
	  plain text version.

	  If an error occured, such as non-matching keys between encryption and decryption,
	  an empty string or a string containing nonsense may be returned.
	  */
	QString decryptToString(QByteArray cypher)
	{
		QByteArray ba = decryptToByteArray(cypher);
		QString plaintext = QString::fromUtf8(ba, ba.size());

		return plaintext;
	}
	/**
	  Decrypts a cyphertext binary encrypted with this class with the set key back to the
	  plain text version.

	  If an error occured, such as non-matching keys between encryption and decryption,
	  an empty string or a string containing nonsense may be returned.
	  */
	QByteArray decryptToByteArray(QByteArray cypher)
	{
		if (m_keyParts.isEmpty())
		{
			qWarning() << "No key set.";
			m_lastError = ErrorNoKeySet;
			return QByteArray();
		}

		QByteArray ba = cypher;

		if (cypher.size() < 3)
			return QByteArray();

		char version = ba.at(0);

		if (version != 3)
		{  //we only work with version 3
			m_lastError = ErrorUnknownVersion;
			qWarning() << "Invalid version or not a cyphertext.";
			return QByteArray();
		}

		quint64 flags = CryptoFlags(ba.at(1));

		ba = ba.mid(2);
		int pos(0);
		int cnt(ba.size());
		char lastChar = 0;

		while (pos < cnt)
		{
			char currentChar = ba[pos];
			ba[pos] = ba.at(pos) ^ lastChar ^ m_keyParts.at(pos % 8);
			lastChar = currentChar;
			++pos;
		}

		ba = ba.mid(1); //chop off the random number at the start

		bool integrityOk(true);
		if (flags & CryptoFlagChecksum)
		{
			if (ba.length() < 2)
			{
				m_lastError = ErrorIntegrityFailed;
				return QByteArray();
			}
			quint16 storedChecksum;
			{
				QDataStream s(&ba, QIODevice::ReadOnly);
				s >> storedChecksum;
			}
			ba = ba.mid(2);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			quint16 checksum = qChecksum(ba.constData(), ba.length());
#else
			//use QByteArrayView overload
			quint16 checksum = qChecksum(QByteArrayView(ba));
#endif
			integrityOk = (checksum == storedChecksum);
		}
		else if (flags & CryptoFlagHash)
		{
			if (ba.length() < 20)
			{
				m_lastError = ErrorIntegrityFailed;
				return QByteArray();
			}
			QByteArray storedHash = ba.left(20);
			ba = ba.mid(20);
			QCryptographicHash hash(QCryptographicHash::Sha1);
			hash.addData(ba);
			integrityOk = (hash.result() == storedHash);
		}

		if (!integrityOk)
		{
			m_lastError = ErrorIntegrityFailed;
			return QByteArray();
		}

		if (flags & CryptoFlagCompression)
			ba = qUncompress(ba);

		m_lastError = ErrorNoError;
		return ba;
	}



private:

	void splitKey()
	{
		m_keyParts.clear();
		m_keyParts.resize(8);
		for (int i = 0; i < 8; ++i)
		{
			quint64 part = m_key;
			for (int j = i; j > 0; j--)
				part = part >> 8;
			part = part & 0xff;
			m_keyParts[i] = static_cast<char>(part);
		}
	}

	quint64 m_key;
	QVector<char> m_keyParts;
	CompressionMode m_compressionMode;
	IntegrityProtectionMode m_protectionMode;
	Error m_lastError;
};

#endif // SimpleCrypt_H