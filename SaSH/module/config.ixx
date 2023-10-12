module;

#include <QWidget>
#include <QMainWindow>

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <QVariant>
#include <QVariantMap>
#include <QPair>

export module Config;
import Scoped;
import Safe;
import Global;
import Utility;
import String;

QMutex g_fileLock;

//Json配置讀寫
export class Config
{
public:
	Config()
		: fileName_(QString::fromUtf8(qgetenv("JSON_PATH")))
	{
		g_fileLock.lock();//附加文件寫鎖
		file_.setFileName(fileName_);
		isVaild = open();
	}

	explicit Config(const QString& fileName)
		: fileName_(fileName)
	{
		g_fileLock.lock();//附加文件寫鎖
		file_.setFileName(fileName_);
		isVaild = open();
	}

	explicit Config(const QByteArray& fileName)
		: fileName_(QString::fromUtf8(fileName))
	{
		g_fileLock.lock();//附加文件寫鎖
		file_.setFileName(fileName_);
		isVaild = open();
	}

	virtual ~Config()
	{
		sync();//同步數據
		g_fileLock.unlock();//解鎖
	}

	bool open()
	{
		//緩存存在則直接從緩存讀取
		if (cacheHash_.contains(fileName_))
		{
			cache_ = cacheHash_.value(fileName_);
			return true;
		}

		if (!file_.exists())
		{
			//文件不存在，以全新文件複寫方式打開
			if (file_.openWriteNew())
			{
				//成功直接返回因為是全新的不需要往下讀
				return true;
			}
			else
				return false;

		}
		else
		{
			//文件存在，以普通讀寫方式打開
			if (!file_.openReadWrite())
			{
				return false;
			}
		}

		QByteArray allData = file_.readAll();

		//第一次使用所以是空的不需要解析成json
		if (allData.simplified().isEmpty())
		{
			return true;
		}

		//解析json
		QJsonParseError jsonError;
		document_ = QJsonDocument::fromJson(allData, &jsonError);
		if (jsonError.error != QJsonParseError::NoError)
		{
			//解析錯誤則直接複寫新的
			if (file_.isOpen())
				file_.close();

			if (file_.openWriteNew())
				return true;//成功直接返回因為是全新的不需要往下讀
			else
				return false;
		}

		//把json轉換成qvariantmap
		QJsonObject root = document_.object();
		cache_ = root.toVariantMap();

		//緩存起來
		cacheHash_.insert(fileName_, cache_);

		return true;
	}

	void sync()
	{
		if (!isVaild && !hasChanged_)
		{
			return;
		}

		cacheHash_.insert(fileName_, cache_);

		QJsonObject root = QJsonObject::fromVariantMap(cache_);
		document_.setObject(root);
		QByteArray data = document_.toJson(QJsonDocument::Indented);

		if (!file_.openWriteNew())
		{
			return;
		}

		file_.write(data);
		file_.flush();
	}

	inline bool isValid() const { return isVaild; }

	void removeSec(const QString sec)
	{
		if (cache_.contains(sec))
		{
			cache_.remove(sec);
			if (!hasChanged_)
				hasChanged_ = true;
		}
	}

	void write(const QString& key, const QVariant& value)
	{
		cache_.insert(key, value);
		if (!hasChanged_)
			hasChanged_ = true;
	}

	void write(const QString& sec, const QString& key, const QVariant& value)
	{
		if (!cache_.contains(sec))
		{
			QJsonObject json;
			json.insert(key, QJsonValue::fromVariant(value));
			cache_.insert(sec, QJsonValue(json));
		}
		else
		{
			QJsonObject json = cache_.value(sec).toJsonObject();
			json.insert(key, QJsonValue::fromVariant(value));
			cache_.insert(sec, QJsonValue(json));
		}
		if (!hasChanged_)
			hasChanged_ = true;
	}

	void write(const QString& sec, const QString& key, const QString& sub, const QVariant& value)
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
			QJsonObject json = cache_.value(sec).toJsonObject();

			if (!json.contains(key))
			{
				QJsonObject subjson;
				subjson.insert(sub, QJsonValue::fromVariant(value));
				json.insert(key, QJsonValue(subjson));
				cache_.insert(sec, QJsonValue(json));
			}
			else
			{
				QJsonObject subjson = json.value(key).toObject();
				subjson.insert(sub, QJsonValue::fromVariant(value));
				json.insert(key, QJsonValue(subjson));
				cache_.insert(sec, QJsonValue(json));
			}
		}
		if (!hasChanged_)
			hasChanged_ = true;
	}

	void WriteHash(const QString& sec, const QString& key, QMap<QString, QPair<bool, QString>>& hash)
	{
		//將將多餘的都刪除
		QJsonObject json = cache_.value(sec).toJsonObject();
		if (json.contains(key))
		{
			QJsonObject subjson = json.value(key).toObject();
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
			QJsonObject j = cache_.value(sec).toJsonObject();
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
				QJsonObject subjson = json.value(key).toObject();
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

	QMap<QString, QPair<bool, QString>> EnumString(const QString& sec, const QString& key) const
	{
		QMap<QString, QPair<bool, QString>> ret;
		if (!cache_.contains(sec))
		{
			return ret;
		}

		QJsonObject json = cache_.value(sec).toJsonObject();
		if (!json.contains(key))
		{
			return ret;
		}

		QJsonObject subjson = json.value(key).toObject();
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

	template <typename T>
	T read(const QString& sec, const QString& key, const QString& sub) const
	{
		if (!cache_.contains(sec))
		{
			return T();
		}

		QJsonObject json = cache_.value(sec).toJsonObject();
		if (!json.contains(key))
		{
			return T();
		}

		QJsonObject subjson = json.value(key).toObject();
		if (!subjson.contains(sub))
		{
			return T();
		}
		return subjson[sub].toVariant().value<T>();
	}

	template <typename T>
	T read(const QString& sec, const QString& key) const
	{
		//read json value from cache_
		if (cache_.contains(sec))
		{
			QJsonObject json = cache_.value(sec).toJsonObject();
			if (json.contains(key))
			{
				return json.value(key).toVariant().value<T>();
			}
		}
		return T();
	}

	template <typename T>
	T read(const QString& key) const
	{
		if (cache_.contains(key))
		{
			return cache_.value(key).value<T>();
		}

		return T();
	}

	template <typename T>
	QList<T> readArray(const QString& sec, const QString& key, const QString& sub) const
	{
		QList<T> result;
		QVariant variant;

		if (cache_.contains(sec))
		{
			QJsonObject json = cache_.value(sec).toJsonObject();

			if (json.contains(key))
			{
				QJsonObject subJson = json.value(key).toObject();

				if (subJson.contains(sub))
				{
					QJsonArray jsonArray = subJson.value(sub).toArray();

					for (const QJsonValue& value : jsonArray)
					{
						variant = value.toVariant();
						result.append(variant.value<T>());
					}
				}
			}
		}

		return result;
	}

	template <typename T>
	void writeArray(const QString& sec, const QString& key, const QList<T>& values)
	{
		if (!cache_.contains(sec))
		{
			cache_.insert(sec, QJsonObject());
		}

		QVariantList variantList;
		for (const T& value : values)
		{
			variantList.append(value);
		}

		QJsonObject json = cache_.value(sec).toJsonObject();
		json.insert(key, QJsonArray::fromVariantList(variantList));
		cache_.insert(sec, json);

		if (!hasChanged_)
			hasChanged_ = true;
	}

	template <typename T>
	void writeArray(const QString& sec, const QString& key, const QString& sub, const QList<T>& values)
	{
		QJsonObject json;

		if (cache_.contains(sec))
		{
			json = cache_.value(sec).toJsonObject();
		}

		QJsonArray jsonArray;

		for (const T& value : values)
		{
			jsonArray.append(value);
		}

		QJsonObject subJson;

		if (json.contains(key))
		{
			subJson = json.value(key).toObject();
		}

		subJson.insert(sub, jsonArray);
		json.insert(key, subJson);
		cache_.insert(sec, json);

		if (!hasChanged_)
			hasChanged_ = true;
	}

	void writeMapData(const QString& sec, const MapData& data)
	{
		QString key = toQString(data.floor);
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

		cache_.insert(key, jarray);

		if (!hasChanged_)
			hasChanged_ = true;
	}

	QList<MapData> readMapData(const QString& key) const
	{
		QList<MapData> result;

		if (!cache_.contains(key))
			return result;

		QJsonArray jarray = cache_.value(key).toJsonArray();
		if (jarray.isEmpty())
			return result;

		for (const QJsonValue& value : jarray)
		{
			QStringList list = value.toString().split(rexOR);
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

			data.x = posList[0].simplified().toLongLong();
			data.y = posList[1].simplified().toLongLong();
			if (data.x == 0 && data.y == 0)
				continue;

			result.append(data);
		}

		return result;
	}

private:
	QString fileName_ = "\0";

	QVariantMap cache_ = {};

	bool isVaild = false;

	bool hasChanged_ = false;

	QJsonDocument document_;

	ScopedFile file_;

	inline static SafeHash<QString, QVariantMap> cacheHash_ = {};
};


//介面排版或配置管理 主用於保存窗口狀態
#pragma region FormSettingManager
export class FormSettingManager
{
public:
	explicit FormSettingManager(QWidget* widget) { widget_ = widget; }
	explicit FormSettingManager(QMainWindow* widget) { mainwindow_ = widget; }

	void loadSettings()
	{
		Config config;

		QString ObjectName;
		if (mainwindow_ != nullptr)
		{
			ObjectName = mainwindow_->objectName();
		}
		else
		{
			ObjectName = widget_->objectName();
		}


		QString strSize = config.read<QString>(ObjectName, "Size");
		QSize size;
		if (!strSize.isEmpty())
		{
			QStringList list = strSize.split(rexComma, Qt::SkipEmptyParts);
			if (list.size() == 2)
			{
				size.setWidth(list.value(0).toLongLong());
				size.setHeight(list.value(1).toLongLong());
			}
		}

		QByteArray geometry;
		QString qstrGeometry = config.read<QString>("Form", ObjectName, "Geometry");
		if (!qstrGeometry.isEmpty())
			geometry = hexStringToByteArray(qstrGeometry);//DECODE

		if (mainwindow_ != nullptr)
		{
			QByteArray state;
			QString qstrState = config.read<QString>("Form", ObjectName, "State");
			if (!qstrState.isEmpty())
				state = hexStringToByteArray(qstrState);//DECODE
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

	void saveSettings()
	{
		Config config;
		QString ObjectName;
		QString qstrGeometry;
		QString qstrState;
		QSize size;
		if (mainwindow_ != nullptr)
		{
			ObjectName = mainwindow_->objectName();
			qstrGeometry = byteArrayToHexString(mainwindow_->saveGeometry());//ENCODE
			qstrState = byteArrayToHexString(mainwindow_->saveState(3));//ENCODE
			size = mainwindow_->size();
			config.write("Form", ObjectName, "State", qstrState);
		}
		else
		{
			ObjectName = widget_->objectName();
			qstrGeometry = byteArrayToHexString(widget_->saveGeometry());//ENCODE
			size = widget_->size();
		}

		config.write("Form", ObjectName, "Geometry", qstrGeometry);
		config.write("Form", ObjectName, "Size", QString("%1,%2").arg(size.width()).arg(size.height()));
	}

private:
	QWidget* widget_ = nullptr;
	QMainWindow* mainwindow_ = nullptr;
};


#pragma endregion