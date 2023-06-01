#include "stdafx.h"
#include <util.h>


util::Config::Config(const QString& fileName)
	: lock_(fileName)
	, fileName_(fileName)
{
	open(fileName);
}

util::Config::~Config()
{
	sync();
}

bool util::Config::open(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}
	QByteArray allData = file.readAll();
	file.close();

	QJsonParseError jsonError;
	document_ = QJsonDocument::fromJson(allData, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		return false;
	}

	QJsonObject root = document_.object();
	cache_ = root.toVariantMap();

	return true;
}

void util::Config::sync()
{
	if (hasChanged_)
	{
		QJsonObject root = QJsonObject::fromVariantMap(cache_);
		document_.setObject(root);
		QByteArray data = document_.toJson(QJsonDocument::Indented);
		QFile file(fileName_);
		if (file.open(QIODevice::WriteOnly))
		{
			file.write(data);
			file.flush();
			file.close();
		}
	}
}

void util::Config::removeSec(const QString sec)
{
	if (cache_.contains(sec))
	{
		cache_.remove(sec);
		if (!hasChanged_)
			hasChanged_ = true;
	}
}

void util::Config::write(const QString& sec, const QString& key, const QString& sub, const QVariant& value)
{
	if (!cache_.contains(sec))
	{
		QJsonObject subjson;
		subjson.insert(sub, QJsonValue::fromVariant(value));
		QJsonObject json;
		json.insert(key, QJsonValue(subjson));
		cache_.insert(sec, QJsonValue(json));
	}
	else
	{
		QJsonObject json = cache_[sec].toJsonObject();

		if (!json.contains(key))
		{
			QJsonObject subjson;
			subjson.insert(sub, QJsonValue::fromVariant(value));
			json.insert(key, QJsonValue(subjson));
			cache_.insert(sec, QJsonValue(json));
		}
		else
		{
			QJsonObject subjson = json[key].toObject();
			subjson.insert(sub, QJsonValue::fromVariant(value));
			json.insert(key, QJsonValue(subjson));
			cache_.insert(sec, QJsonValue(json));
		}
	}
	if (!hasChanged_)
		hasChanged_ = true;
}

void util::Config::write(const QString& sec, const QString& key, const QVariant& value)
{
	if (!cache_.contains(sec))
	{
		QJsonObject json;
		json.insert(key, QJsonValue::fromVariant(value));
		cache_.insert(sec, QJsonValue(json));
	}
	else
	{
		QJsonObject json = cache_[sec].toJsonObject();
		json.insert(key, QJsonValue::fromVariant(value));
		cache_[sec] = QJsonValue(json);
	}
	if (!hasChanged_)
		hasChanged_ = true;
}

void util::Config::write(const QString& key, const QVariant& value)
{
	cache_.insert(key, value);
	if (!hasChanged_)
		hasChanged_ = true;
}

QString util::Config::readString(const QString& sec, const QString& key, const QString& sub) const
{
	if (!cache_.contains(sec))
	{
		return 0;
	}
	QJsonObject json = cache_[sec].toJsonObject();
	if (!json.contains(key))
	{
		return 0;
	}
	QJsonObject subjson = json[key].toObject();
	if (!subjson.contains(sub))
	{
		return 0;
	}
	return subjson[sub].toString();
}

void util::Config::WriteHash(const QString& sec, const QString& key, QMap<QString, QPair<bool, QString>>& hash)
{
	//將將多餘的都刪除
	QJsonObject json = cache_[sec].toJsonObject();
	if (json.contains(key))
	{
		QJsonObject subjson = json[key].toObject();
		for (auto it = subjson.begin(); it != subjson.end();)
		{
			if (!hash.contains(it.key()))
			{
				it = subjson.erase(it);
			}
			else
			{
				++it;
			}
		}
		json.insert(key, QJsonValue(subjson));
		cache_.insert(sec, QJsonValue(json));
	}

	// hash key is json subkey
	if (!cache_.contains(sec))
	{
		QJsonObject j;
		QJsonObject subjson;
		for (auto it = hash.begin(); it != hash.end(); ++it)
		{
			QJsonObject subsubjson;
			subsubjson.insert("enable", it.value().first);
			subsubjson.insert("value", it.value().second);
			subjson.insert(it.key(), subsubjson);
		}
		j.insert(key, subjson);
		cache_.insert(sec, QJsonValue(j));
	}
	else
	{
		QJsonObject j = cache_[sec].toJsonObject();
		if (!j.contains(key))
		{
			QJsonObject subjson;
			for (auto it = hash.begin(); it != hash.end(); ++it)
			{
				QJsonObject subsubjson;
				subsubjson.insert("enable", it.value().first);
				subsubjson.insert("value", it.value().second);
				subjson.insert(it.key(), subsubjson);
			}
			j.insert(key, subjson);
			cache_.insert(sec, QJsonValue(j));
		}
		else
		{
			QJsonObject subjson = json[key].toObject();
			for (auto it = hash.begin(); it != hash.end(); ++it)
			{
				QJsonObject subsubjson;
				subsubjson.insert("enable", it.value().first);
				subsubjson.insert("value", it.value().second);
				subjson.insert(it.key(), subsubjson);
			}
			json.insert(key, subjson);
			cache_.insert(sec, QJsonValue(json));
		}
	}
}

QMap<QString, QPair<bool, QString>> util::Config::EnumString(const QString& sec, const QString& key) const
{
	QMap<QString, QPair<bool, QString>> ret;
	if (!cache_.contains(sec))
	{
		return ret;
	}
	QJsonObject json = cache_[sec].toJsonObject();
	if (!json.contains(key))
	{
		return ret;
	}
	QJsonObject subjson = json[key].toObject();
	for (auto it = subjson.begin(); it != subjson.end(); ++it)
	{
		QPair<bool, QString> pair;
		//key == enable
		QJsonObject  child = it.value().toObject();
		if (child.contains("enable") && child.contains("value") && child["enable"].isBool() && child["value"].isString())
		{
			pair.first = child["enable"].toBool();
			pair.second = child["value"].toString();
			ret.insert(it.key(), pair);
		}
	}
	return ret;
}

QString util::Config::readString(const QString& sec, const QString& key) const
{
	//read json value from cache_
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			return json.value(key).toString();
		}
	}
	return 0;
}

int util::Config::readInt(const QString& sec, const QString& key, int retnot) const
{
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			return json.value(key).toInt();
		}
	}
	return retnot;
}

QStringList util::Config::readStringList(const QString& sec, const QString& key, const QString& sub) const
{
	if (!cache_.contains(sec))
	{
		return QStringList();
	}
	QJsonObject json = cache_[sec].toJsonObject();
	if (!json.contains(key))
	{
		return QStringList();
	}
	QJsonObject subjson = json[key].toObject();
	if (!subjson.contains(sub))
	{
		return QStringList();
	}
	QJsonArray array = subjson[sub].toArray();
	QStringList ret;
	for (int i = 0; i < array.size(); ++i)
	{
		ret << array[i].toString();
	}
	return ret;
}

QStringList util::Config::readStringList(const QString& sec, const QString& key) const
{
	QStringList ret;
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			QJsonArray array = json.value(key).toArray();

			for (int i = 0; i < array.size(); ++i)
			{
				ret << array[i].toString();
			}
			return ret;
		}
	}

	return ret;
}

void util::Config::writeStringList(const QString& key, const QStringList& value)
{
	if (cache_.contains(key))
	{
		QJsonArray array;
		for (int i = 0; i < value.size(); ++i)
		{
			array << value[i];
		}
		cache_.insert(key, QJsonValue(array));
	}
	else
	{
		QJsonArray array;
		for (int i = 0; i < value.size(); ++i)
		{
			array << value[i];
		}
		cache_.insert(key, QJsonValue(array));
	}
}
void util::Config::writeStringList(const QString& sec, const QString& key, const QStringList& value)
{
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			QJsonArray array;
			for (int i = 0; i < value.size(); ++i)
			{
				array << value[i];
			}
			json.insert(key, array);
			cache_.insert(sec, QJsonValue(json));
		}
		else
		{
			QJsonArray array;
			for (int i = 0; i < value.size(); ++i)
			{
				array << value[i];
			}
			json.insert(key, array);
			cache_.insert(sec, QJsonValue(json));
		}
	}
	else
	{
		QJsonObject json;
		QJsonArray array;
		for (int i = 0; i < value.size(); ++i)
		{
			array << value[i];
		}
		json.insert(key, array);
		cache_.insert(sec, QJsonValue(json));
	}
}
void util::Config::writeStringList(const QString& sec, const QString& key, const QString& sub, const QStringList& value)
{
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_[sec].toJsonObject();
		if (json.contains(key))
		{
			QJsonObject subjson = json[key].toObject();
			QJsonArray array;
			for (int i = 0; i < value.size(); ++i)
			{
				array << value[i];
			}
			subjson.insert(sub, array);
			json.insert(key, subjson);
			cache_.insert(sec, QJsonValue(json));
		}
		else
		{
			QJsonObject subjson;
			QJsonArray array;
			for (int i = 0; i < value.size(); ++i)
			{
				array << value[i];
			}
			subjson.insert(sub, array);
			json.insert(key, subjson);
			cache_.insert(sec, QJsonValue(json));
		}
	}
	else
	{
		QJsonObject json;
		QJsonObject subjson;
		QJsonArray array;
		for (int i = 0; i < value.size(); ++i)
		{
			array << value[i];
		}
		subjson.insert(sub, array);
		json.insert(key, subjson);
		cache_.insert(sec, QJsonValue(json));
	}

}


qreal util::Config::readDouble(const QString& sec, const QString& key, qreal retnot) const
{
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			return json.value(key).toDouble();
		}
	}
	return retnot;
}

int util::Config::readInt(const QString& sec, const QString& key) const
{
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			return json.value(key).toInt();
		}
	}
	return 0;
}

int util::Config::readInt(const QString& sec, const QString& key, const QString& sub) const
{
	if (!cache_.contains(sec))
	{
		return 0;
	}
	QJsonObject json = cache_[sec].toJsonObject();
	if (!json.contains(key))
	{
		return 0;
	}
	QJsonObject subjson = json[key].toObject();
	if (!subjson.contains(sub))
	{
		return 0;
	}
	return subjson[sub].toInt();
}

QString util::Config::readString(const QString& key) const
{
	if (cache_.contains(key))
	{
		return cache_.value(key).toString();
	}

	return 0;
}

bool util::Config::readBool(const QString& key) const
{
	if (cache_.contains(key))
	{
		return cache_.value(key).toBool();
	}

	return false;
}

bool util::Config::readBool(const QString& sec, const QString& key) const
{
	if (cache_.contains(sec))
	{
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			return json.value(key).toBool();
		}
	}
	return false;
}

bool util::Config::readBool(const QString& sec, const QString& key, const QString& sub) const
{
	if (!cache_.contains(sec))
	{
		return false;
	}
	QJsonObject json = cache_[sec].toJsonObject();
	if (!json.contains(key))
	{
		return false;
	}
	QJsonObject subjson = json[key].toObject();
	if (!subjson.contains(sub))
	{
		return false;
	}
	return subjson[sub].toBool();
}

int util::Config::readInt(const QString& key) const
{
	if (cache_.contains(key))
	{
		return cache_.value(key).toInt();
	}

	return 0;
}

//

void util::FormSettingManager::loadSettings()
{
	util::Crypt crypt;
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;

	Config config(fileName);

	QString ObjectName;
	if (mainwindow_ != nullptr)
	{
		ObjectName = mainwindow_->objectName();
	}
	else
	{
		ObjectName = widget_->objectName();
	}


	QString strSize = config.readString(ObjectName, "Size");
	QSize size;
	if (!strSize.isEmpty())
	{
		QStringList list = strSize.split(util::rexComma, Qt::SkipEmptyParts);
		if (list.size() == 2)
		{
			size.setWidth(list.at(0).toInt());
			size.setHeight(list.at(1).toInt());
		}
	}

	QByteArray geometry;
	QString qstrGeometry = config.readString("Form", ObjectName, "Geometry");
	if (!qstrGeometry.isEmpty())
		geometry = crypt.decryptToByteArray(qstrGeometry);//DECODE

	if (mainwindow_ != nullptr)
	{
		QByteArray state;
		QString qstrState = config.readString("Form", ObjectName, "State");
		if (!qstrState.isEmpty())
			state = crypt.decryptToByteArray(qstrState);//DECODE
		mainwindow_->resize(size);
		if (!geometry.isEmpty())
			mainwindow_->restoreGeometry(geometry);
		if (!state.isEmpty())
			mainwindow_->restoreState(state, 3);
	}
	else
	{
		widget_->resize(size);
		if (!geometry.isEmpty())
			widget_->restoreGeometry(geometry);
	}

}
void util::FormSettingManager::saveSettings()
{
	util::Crypt crypt;
	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	Config config(fileName);
	QString ObjectName;
	QString qstrGeometry;
	QString qstrState;
	QSize size;
	if (mainwindow_ != nullptr)
	{
		ObjectName = mainwindow_->objectName();
		qstrGeometry = crypt.encryptFromByteArray(mainwindow_->saveGeometry());//ENCODE
		qstrState = crypt.encryptFromByteArray(mainwindow_->saveState(3));//ENCODE
		size = mainwindow_->size();
		config.write("Form", ObjectName, "State", qstrState);
	}
	else
	{
		ObjectName = widget_->objectName();
		qstrGeometry = crypt.encryptFromByteArray(widget_->saveGeometry());//ENCODE
		size = widget_->size();
	}

	config.write("Form", ObjectName, "Geometry", qstrGeometry);
	config.write("Form", ObjectName, "Size", QString("%1,%2").arg(size.width()).arg(size.height()));
}

bool mem::read(HANDLE hProcess, DWORD desiredAccess, SIZE_T size, PVOID buffer)
{
	if (!size)return FALSE;
	if (!buffer) return FALSE;
	if (!hProcess) return FALSE;
	if (!desiredAccess) return FALSE;
	//ULONG oldProtect = NULL;
	//VirtualProtectEx(m_pi.hProcess, buffer, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = NT_SUCCESS(MINT::NtReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(desiredAccess), buffer, size, NULL));
	//VirtualProtectEx(m_pi.hProcess, buffer, size, oldProtect, &oldProtect);
	return ret == TRUE;
}

int mem::readInt(HANDLE hProcess, DWORD desiredAccess, SIZE_T size)
{
	if (!size)return 0;
	if (!hProcess) return 0;
	if (!desiredAccess) return 0;
	int buffer = 0;
	SIZE_T sizet = static_cast<SIZE_T>(size);
	BOOL ret = read(hProcess, desiredAccess, sizet, &buffer);
	return (ret) ? (buffer) : 0;
}

float mem::readFloat(HANDLE hProcess, DWORD desiredAccess)
{
	if (!hProcess) return 0.0f;
	if (!desiredAccess) return 0.0f;
	float buffer = 0;
	SIZE_T size = sizeof(float);
	BOOL ret = read(hProcess, desiredAccess, size, &buffer);
	return (ret) ? (buffer) : 0.0f;
}

qreal mem::readDouble(HANDLE hProcess, DWORD desiredAccess)
{
	if (!hProcess) return 0.0;
	if (!desiredAccess) return 0.0;
	qreal buffer = 0;
	SIZE_T size = sizeof(qreal);
	BOOL ret = read(hProcess, desiredAccess, size, &buffer);
	return (ret == TRUE) ? (buffer) : 0.0;
}

QString mem::readString(HANDLE hProcess, DWORD desiredAccess, int size, bool enableTrim)
{
	if (!hProcess) return "\0";
	if (!desiredAccess) return "\0";

	QScopedArrayPointer <char> p(q_check_ptr(new char[size + 1]));
	ZeroMemory(p.get(), size + 1);
	SIZE_T sizet = size;
	BOOL ret = read(hProcess, desiredAccess, sizet, p.get());
	const QString retstring((ret == TRUE) ? (util::toUnicode(p.get(), false)) : "");
	p.reset(nullptr);
	return retstring;
}

bool mem::write(HANDLE hProcess, DWORD baseAddress, PVOID buffer, SIZE_T dwSize)
{
	if (!hProcess) return FALSE;
	ULONG oldProtect = NULL;

	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	BOOL ret = WriteProcessMemory(hProcess, reinterpret_cast<PVOID>(baseAddress), buffer, dwSize, NULL);
	VirtualProtectEx(hProcess, (LPVOID)baseAddress, dwSize, oldProtect, &oldProtect);
	return ret == TRUE;
}

bool mem::writeInt(HANDLE hProcess, DWORD baseAddress, int data, int mode)
{
	if (!hProcess) return FALSE;
	if (!baseAddress) return FALSE;
	PVOID pBuffer = &data;
	qint8 bdata = static_cast<qint8>(data);
	qint16 sdata = static_cast<qint16>(data);
	SIZE_T len = sizeof(int);
	if (2 == mode)
	{
		len = sizeof(qint8);
		pBuffer = &bdata;
	}
	else if (1 == mode)
	{
		len = sizeof(qint16);
		pBuffer = &sdata;
	}

	BOOL ret = write(hProcess, baseAddress, pBuffer, len);
	return ret == TRUE;
}

bool mem::writeString(HANDLE hProcess, DWORD baseAddress, const QString& str)
{
	if (!hProcess) return FALSE;
	QTextCodec* g_game_codec = QTextCodec::codecForName("GB2312");
	QByteArray ba = g_game_codec->fromUnicode(str);
	ba.append('\0');
	char* pBuffer = ba.data();
	SIZE_T len = ba.size();
	QScopedArrayPointer <char> p(q_check_ptr(new char[len + 1]()));
	ZeroMemory(p.get(), len + 1);
	_snprintf_s(p.get(), len + 1, _TRUNCATE, "%s\0", pBuffer);
	BOOL ret = write(hProcess, baseAddress, p.get(), len + 1);
	p.reset(nullptr);
	return ret == TRUE;
}

int mem::virtualAlloc(HANDLE hProcess, int size)
{
	if (!hProcess) return 0;
	DWORD ptr = NULL;
	SIZE_T sizet = static_cast<SIZE_T>(size);

	BOOL ret = NT_SUCCESS(MINT::NtAllocateVirtualMemory(hProcess, (PVOID*)&ptr, NULL, &sizet, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	if (ret == TRUE)
	{
		return static_cast<int>(ptr);
	}
	else
		return NULL;
	//return static_cast<int>(this->VirtualAllocEx(m_pi.nWnd, NULL, size, 0));
}

int mem::virtualAllocA(HANDLE hProcess, const QString& str)
{
	int ret = NULL;
	do
	{
		if (!hProcess)
			break;

		ret = virtualAlloc(hProcess, str.toLocal8Bit().size() * 2 * sizeof(char) + 1);
		if (ret == FALSE)
			break;
		if (!writeString(hProcess, ret, str))
		{
			virtualFree(hProcess, ret);
			ret = NULL;
			break;
		}
	} while (false);
	return ret;
}

int mem::virtualAllocW(HANDLE hProcess, const QString& str)
{
	int ret = NULL;
	do
	{
		if (!hProcess)
			break;

		std::wstring wstr(str.toStdWString());
		wstr += L'\0';
		ret = virtualAlloc(hProcess, wstr.length() * sizeof(wchar_t) + 1);
		if (!ret)
			break;
		if (!write(hProcess, ret, (PVOID)wstr.c_str(), wstr.length() * sizeof(wchar_t) + 1))
		{
			virtualFree(hProcess, ret);
			ret = NULL;
			break;
		}
	} while (false);
	return ret;
}

bool mem::virtualFree(HANDLE hProcess, int baseAddress)
{
	if (!hProcess) return FALSE;
	if (baseAddress == NULL) return FALSE;
	SIZE_T size = 0;
	BOOL ret = NT_SUCCESS(MINT::NtFreeVirtualMemory(hProcess, (PVOID*)&baseAddress, &size, MEM_RELEASE));
	return ret == TRUE;
	//return static_cast<int>(this->VirtualFreeEx(m_pi.nWnd, static_cast<qlonglong>(baseAddress)));
}

DWORD mem::getRemoteModuleHandle(DWORD dwProcessId, const QString& moduleName)
{
	MODULEENTRY32 moduleEntry;
	//  获取进程快照中包含在th32ProcessID中指定的进程的所有的模块。
	QScopedHandle hSnapshot(QScopedHandle::CREATE_TOOLHELP32_SNAPSHOT, TH32CS_SNAPMODULE, dwProcessId);
	if (!hSnapshot.isValid()) return NULL;

	ZeroMemory(&moduleEntry, sizeof(MODULEENTRY32));
	moduleEntry.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(hSnapshot, &moduleEntry))
		return NULL;
	else
	{
		const QString str(QString::fromWCharArray(moduleEntry.szModule));
		if (str == moduleName)
			return reinterpret_cast<DWORD>(moduleEntry.hModule);
	}

	do
	{
		const QString str(QString::fromWCharArray(moduleEntry.szModule));
		if (str == moduleName)
			return reinterpret_cast<DWORD>(moduleEntry.hModule);
	} while (Module32Next(hSnapshot, &moduleEntry));
	return NULL;
}

void mem::freeUnuseMemory(HANDLE hProcess)
{
	SetProcessWorkingSetSizeEx(hProcess, -1, -1, 0);
	K32EmptyWorkingSet(hProcess);
}