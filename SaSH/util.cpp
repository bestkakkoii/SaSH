#include "stdafx.h"
#include <util.h>

QReadWriteLock g_fileLock;

util::Config::Config(const QString& fileName)
	: fileName_(fileName)
{
	g_fileLock.lockForRead();
	isVaild = open(fileName);
	g_fileLock.unlock();
}

util::Config::~Config()
{
	if (isVaild)
		sync();
}

bool util::Config::open(const QString& fileName)
{

	QFile file(fileName);
	if (!file.exists())
	{
		// 文件不存在，创建新文件
		if (!file.open(QIODevice::ReadWrite | QIODevice::Append))
		{
			return false;
		}
	}
	else
	{
		// 文件存在，以只读模式打开
		if (!file.open(QIODevice::ReadOnly))
		{
			return false;
		}

	}

	QByteArray allData = file.readAll();
	file.close();

	QJsonParseError jsonError;
	document_ = QJsonDocument::fromJson(allData, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		// 解析错误
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
		g_fileLock.lockForWrite();
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
		g_fileLock.unlock();
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
		for (auto it = hash.cbegin(); it != hash.cend(); ++it)
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
			for (auto it = hash.cbegin(); it != hash.cend(); ++it)
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
			for (auto it = hash.cbegin(); it != hash.cend(); ++it)
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

QList<int> util::Config::readIntArray(const QString& sec, const QString& key, const QString& sub) const
{
	QList<int> result;

	if (cache_.contains(sec))
	{
		QJsonObject json = cache_[sec].toJsonObject();

		if (json.contains(key))
		{
			QJsonObject subJson = json[key].toObject();

			if (subJson.contains(sub))
			{
				QJsonArray jsonArray = subJson[sub].toArray();

				for (const QJsonValue& value : jsonArray)
				{
					result.append(value.toInt());
				}
			}
		}
	}

	return result;
}

QStringList util::Config::readStringArray(const QString& sec, const QString& key, const QString& sub) const
{
	QStringList result;

	if (cache_.contains(sec))
	{
		QJsonObject json = cache_[sec].toJsonObject();

		if (json.contains(key))
		{
			QJsonObject subJson = json[key].toObject();

			if (subJson.contains(sub))
			{
				QJsonArray jsonArray = subJson[sub].toArray();

				for (const QJsonValue& value : jsonArray)
				{
					result.append(value.toString());
				}
			}
		}
	}

	return result;
}

void util::Config::writeStringArray(const QString& sec, const QString& key, const QString& sub, const QStringList values)
{
	QJsonObject json;

	if (cache_.contains(sec))
	{
		json = cache_[sec].toJsonObject();
	}

	QJsonArray jsonArray;

	for (const QString& value : values)
	{
		jsonArray.append(value);
	}

	QJsonObject subJson;

	if (json.contains(key))
	{
		subJson = json[key].toObject();
	}

	subJson[sub] = jsonArray;
	json[key] = subJson;
	cache_[sec] = json;

	if (!hasChanged_)
	{
		hasChanged_ = true;
	}
}

void util::Config::writeIntArray(const QString& sec, const QString& key, const QList<int>& values)
{
	if (!cache_.contains(sec))
	{
		cache_[sec] = QJsonObject();
	}
	QVariantList variantList;
	for (int value : values)
	{
		variantList.append(value);
	}
	QJsonObject json = cache_[sec].toJsonObject();
	json[key] = QJsonArray::fromVariantList(variantList);
	cache_[sec] = json;
	if (!hasChanged_)
		hasChanged_ = true;
}

void util::Config::writeIntArray(const QString& sec, const QString& key, const QString& sub, const QList<int>& values)
{
	QJsonObject json;

	if (cache_.contains(sec))
	{
		json = cache_[sec].toJsonObject();
	}

	QJsonArray jsonArray;

	for (int value : values)
	{
		jsonArray.append(value);
	}

	QJsonObject subJson;

	if (json.contains(key))
	{
		subJson = json[key].toObject();
	}

	subJson[sub] = jsonArray;
	json[key] = subJson;
	cache_[sec] = json;

	if (!hasChanged_)
	{
		hasChanged_ = true;
	}
}

void util::Config::writeMapData(const QString& sec, const util::MapData& data)
{
	QString key = QString::number(data.floor);
	QJsonArray jarray;
	if (cache_.contains(key))
	{
		jarray = cache_[key].toJsonArray();
	}

	QString text = QString("%1|%2,%3").arg(data.name).arg(data.x).arg(data.y);

	if (!jarray.contains(text))
	{
		jarray.append(text);
	}

	cache_[key] = jarray;

	if (!hasChanged_)
		hasChanged_ = true;
}

// 读取数据
QList<util::MapData> util::Config::readMapData(const QString& key) const
{
	QList<MapData> result;

	if (!cache_.contains(key))
		return result;

	QJsonArray jarray = cache_[key].toJsonArray();
	if (jarray.isEmpty())
		return result;

	for (const QJsonValue& value : jarray)
	{
		QStringList list = value.toString().split(util::rexOR);
		if (list.size() != 2)
			continue;

		MapData data;
		data.name = list[0].simplified();
		if (data.name.isEmpty())
			continue;

		QString pos = list[1].simplified();
		if (pos.isEmpty())
			continue;

		if (pos.count(",") != 1)
			continue;

		QStringList posList = pos.split(",");
		if (posList.size() != 2)
			continue;

		data.x = posList[0].simplified().toInt();
		data.y = posList[1].simplified().toInt();
		if (data.x == 0 && data.y == 0)
			continue;

		result.append(data);
	}

	return result;
}

QFileInfoList util::loadAllFileLists(TreeWidgetItem* root, const QString& path, QStringList* list)
{
	/*添加path路徑文件*/
	QDir dir(path); //遍歷各級子目錄
	QDir dir_file(path); //遍歷子目錄中所有文件
	dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
	dir_file.setSorting(QDir::Size | QDir::Reversed);
	const QStringList filters = { QString("*%1").arg(util::SCRIPT_SUFFIX_DEFAULT), QString("*%1").arg(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT) };
	dir_file.setNameFilters(filters);
	QFileInfoList list_file = dir_file.entryInfoList();
	for (const QFileInfo& item : list_file)
	{ //將當前目錄中所有文件添加到treewidget中
		if (list)
			list->append(item.fileName());
		TreeWidgetItem* child = q_check_ptr(new TreeWidgetItem(QStringList{ item.fileName() }, 1));
		child->setIcon(0, QIcon(QPixmap(":/image/icon_txt.png")));

		root->addChild(child);
	}

	QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
	int count = folder_list.size();
	for (int i = 0; i != count; ++i) //自動遞歸添加各目錄到上一級目錄
	{
		const QString namepath = folder_list.at(i).absoluteFilePath(); //獲取路徑
		const QFileInfo folderinfo = folder_list.at(i);
		const QString name = folderinfo.fileName(); //獲取目錄名
		if (list)
			list->append(name);
		TreeWidgetItem* childroot = q_check_ptr(new TreeWidgetItem(QStringList{ name }, 0));
		childroot->setIcon(0, QIcon(QPixmap(":/image/icon_directory.png")));
		root->addChild(childroot); //將當前目錄添加成path的子項
		const QFileInfoList child_file_list = loadAllFileLists(childroot, namepath); //進行遞歸
		file_list.append(child_file_list);
	}
	return file_list;
}

bool util::enumAllFiles(const QString dir, const QString suffix, QVector<QPair<QString, QString>>* result)
{
	QDir directory(dir);

	if (!directory.exists())
	{
		return false; // 目錄不存在，返回失敗
	}

	QFileInfoList fileList = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	QFileInfoList dirList = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

	// 遍歷並匹配文件
	for (const QFileInfo& fileInfo : fileList)
	{
		QString fileName = fileInfo.fileName();
		QString filePath = fileInfo.filePath();

		// 如果suffix不為空且文件名不以suffix結尾，則跳過
		if (!suffix.isEmpty() && !fileName.endsWith(suffix, Qt::CaseInsensitive))
		{
			continue;
		}

		// 將匹配的文件信息添加到結果中
		result->append(QPair<QString, QString>(fileName, filePath));
	}

	// 遞歸遍歷子目錄
	for (const QFileInfo& dirInfo : dirList)
	{
		QString subDir = dirInfo.filePath();
		bool success = enumAllFiles(subDir, suffix, result);
		if (!success)
		{
			return false; // 遞歸遍歷失敗，返回失敗
		}
	}

	return true; // 遍歷成功，返回成功
}

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


template<typename T, typename>
T mem::read(HANDLE hProcess, DWORD desiredAccess)
{
	if (!hProcess) return T();

	T buffer{};
	SIZE_T size = sizeof(T);
	if (!size) return T();
	BOOL ret = mem::read(hProcess, desiredAccess, size, &buffer);
	return ret ? buffer : T();
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

QString mem::readString(HANDLE hProcess, DWORD desiredAccess, int size, bool enableTrim, bool keepOriginal)
{
	if (!hProcess) return "\0";
	if (!desiredAccess) return "\0";

	QScopedArrayPointer <char> p(q_check_ptr(new char[size + 1]));
	memset(p.get(), 0, size + 1);
	SIZE_T sizet = size;
	BOOL ret = read(hProcess, desiredAccess, sizet, p.get());
	if (!keepOriginal)
	{
		std::string s = p.get();
		QString retstring = (ret == TRUE) ? (util::toUnicode(s.c_str(), true)) : "";
		return retstring;
	}
	else
	{
		QString retstring = (ret == TRUE) ? QString(p.get()) : "";
		return retstring;
	}

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

template<typename T, typename>
bool mem::write(HANDLE hProcess, DWORD baseAddress, T data)
{
	if (!hProcess) return false;
	if (!baseAddress) return false;
	PVOID pBuffer = &data;
	BOOL ret = write(hProcess, baseAddress, pBuffer, sizeof(T));
	return ret == TRUE;
}

bool mem::writeString(HANDLE hProcess, DWORD baseAddress, const QString& str)
{
	if (!hProcess) return FALSE;
	QTextCodec* codec = QTextCodec::codecForMib(2025);//QTextCodec::codecForName("gb2312");
	QByteArray ba = codec->fromUnicode(str);
	ba.append('\0');
	char* pBuffer = ba.data();
	SIZE_T len = ba.size();
	QScopedArrayPointer <char> p(q_check_ptr(new char[len + 1]()));
	memset(p.get(), 0, len + 1);
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

	memset(&moduleEntry, 0, sizeof(MODULEENTRY32W));
	moduleEntry.dwSize = sizeof(MODULEENTRY32W);
	if (!Module32FirstW(hSnapshot, &moduleEntry))
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
	} while (Module32NextW(hSnapshot, &moduleEntry));
	return NULL;
}

void mem::freeUnuseMemory(HANDLE hProcess)
{
	SetProcessWorkingSetSizeEx(hProcess, -1, -1, 0);
	K32EmptyWorkingSet(hProcess);
}

void util::sortWindows(const QVector<HWND>& windowList, bool alignLeft)
{
	if (windowList.isEmpty())
	{
		return;
	}

	// 獲取桌面分辨率
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// 窗口移動的初始位置
	int x = 0;
	int y = 0;

	// 遍歷窗口列表，移動窗口
	int size = windowList.size();
	for (int i = 0; i < size; ++i)
	{
		HWND hwnd = windowList.at(i);

		// 設置窗口位置
		ShowWindow(hwnd, SW_RESTORE);                             // 先恢覆窗口
		SetForegroundWindow(hwnd);                                // 然後將窗口激活

		// 獲取窗口大小
		RECT windowRect;
		GetWindowRect(hwnd, &windowRect);
		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = windowRect.bottom - windowRect.top;

		// 根據對齊方式設置窗口位置
		if (alignLeft)
		{
			SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER); // 左對齊
		}
		else
		{
			// 右對齊
			int xPos = screenWidth - (x + windowWidth);
			SetWindowPos(hwnd, nullptr, xPos, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}

		// 更新下一個窗口的位置
		x += windowWidth - 15;
		if (x + windowWidth > screenWidth)
		{
			x = 0;
			y += windowHeight;
		}
	}
}