module;

#include <QReadWriteLock>
#include <QHash>
#include <QQueue>
#include <QVector>

export module Safe;

//基於Qt QHash 的線程安全Hash容器
export template <typename K, typename V>
class SafeHash
{
public:
	SafeHash() = default;

	inline SafeHash(std::initializer_list<std::pair<K, V> > list)
	{
		QWriteLocker locker(&lock);
		hash = list;
	}

	bool isEmpty() const
	{
		QReadLocker locker(&lock);
		return hash.isEmpty();
	}

	//copy
	SafeHash(const QHash<K, V>& other)
	{
		QWriteLocker locker(&lock);
		hash = other;
	}

	//copy assign
	SafeHash(const SafeHash<K, V>& other)
	{
		QWriteLocker locker(&lock);
		hash = other.hash;
	}

	//move
	SafeHash(QHash<K, V>&& other)
	{
		QWriteLocker locker(&lock);
		hash = other;
	}

	//move assign
	SafeHash(SafeHash<K, V>&& other) noexcept
	{
		QWriteLocker locker(&lock);
		hash = other.hash;
	}

	SafeHash operator=(const SafeHash& other)
	{
		QWriteLocker locker(&lock);
		hash = other.hash;
		return *this;
	}

	//operator=
	SafeHash operator=(const QHash <K, V>& other)
	{
		QWriteLocker locker(&lock);
		hash = other;
		return *this;
	}

	inline void insert(const K& key, const V& value)
	{
		QWriteLocker locker(&lock);
		hash.insert(key, value);
	}
	inline void remove(const K& key)
	{
		QWriteLocker locker(&lock);
		hash.remove(key);
	}
	inline bool contains(const K& key) const
	{
		QReadLocker locker(&lock);
		return hash.contains(key);
	}
	inline V value(const K& key) const
	{
		QReadLocker locker(&lock);
		return hash.value(key);
	}
	inline V value(const K& key, const V& defaultValue) const
	{
		QReadLocker locker(&lock);
		return hash.value(key, defaultValue);
	}

	inline __int64 size() const
	{
		QReadLocker locker(&lock);
		return hash.size();
	}
	inline QList <K> keys() const
	{
		QReadLocker locker(&lock);
		return hash.keys();
	}

	inline QList <V> values() const
	{
		QReadLocker locker(&lock);
		return hash.values();
	}
	//take
	inline V take(const K& key)
	{
		QWriteLocker locker(&lock);
		return hash.take(key);
	}

	inline void clear()
	{
		QWriteLocker locker(&lock);
		hash.clear();
	}

	inline K key(const V& value) const
	{
		QReadLocker locker(&lock);
		return hash.key(value);
	}

	inline K key(const V& value, const K& defaultValue) const
	{
		QReadLocker locker(&lock);
		return hash.key(value, defaultValue);
	}

	inline typename QHash<K, V>::iterator begin()
	{
		QReadLocker locker(&lock);
		return hash.begin();
	}

	inline typename QHash<K, V>::const_iterator begin() const
	{
		QReadLocker locker(&lock);
		return hash.begin();
	}

	//const
	inline typename QHash<K, V>::const_iterator cbegin() const
	{
		QReadLocker locker(&lock);
		return hash.constBegin();
	}

	inline typename QHash<K, V>::iterator end()
	{
		QReadLocker locker(&lock);
		return hash.end();
	}

	//const
	inline typename QHash<K, V>::const_iterator cend() const
	{
		QReadLocker locker(&lock);
		return hash.constEnd();
	}

	inline typename QHash<K, V>::const_iterator end() const
	{
		QReadLocker locker(&lock);
		return hash.end();
	}

	//erase
	inline  typename QHash<K, V>::iterator erase(typename QHash<K, V>::iterator it)
	{
		QWriteLocker locker(&lock);
		return hash.erase(it);
	}

	//find
	inline typename QHash<K, V>::iterator find(const K& key)
	{
		QReadLocker locker(&lock);
		return hash.find(key);
	}

	QHash <K, V> toHash() const
	{
		QReadLocker locker(&lock);
		return hash;
	}

	void lockForRead() const
	{
		lock.lockForRead();
	}

	void lockForWrite() const
	{
		lock.lockForWrite();
	}

	void unlock() const
	{
		lock.unlock();
	}

private:
	QHash<K, V> hash;
	mutable QReadWriteLock lock;
};

export template <typename V>
class SafeQueue
{
public:
	explicit SafeQueue(__int64 maxSize)
		: maxSize_(maxSize)
	{
	}
	virtual ~SafeQueue() = default;

	void clear()
	{
		QWriteLocker locker(&lock_);
		queue_.clear();
	}

	QVector<V> values() const
	{
		QReadLocker locker(&lock_);
		return queue_.toVector();

	}

	__int64 size() const
	{
		QReadLocker locker(&lock_);
		return queue_.size();
	}

	bool isEmpty() const
	{
		QReadLocker locker(&lock_);
		return queue_.isEmpty();
	}

	void enqueue(const V& value)
	{
		QWriteLocker locker(&lock_);
		if (queue_.size() >= maxSize_)
		{
			queue_.dequeue();
		}
		queue_.enqueue(value);
	}

	bool dequeue(V* pvalue)
	{
		QWriteLocker locker(&lock_);
		if (queue_.isEmpty())
		{
			return false;
		}
		if (pvalue)
			*pvalue = queue_.dequeue();
		return true;
	}

	V front() const
	{
		QReadLocker locker(&lock_);
		return queue_.head();
	}

	void setMaxSize(__int64 maxSize)
	{
		QWriteLocker locker(&lock_);
		queue_.setMaxSize(maxSize);
	}

	//=
	SafeQueue& operator=(const SafeQueue& other)
	{
		QWriteLocker locker(&lock_);
		if (this != &other)
		{
			queue_ = other.queue_;
		}
		return *this;
	}

private:
	QQueue<V> queue_;
	__int64 maxSize_;
	mutable QReadWriteLock lock_;
};;

export template <typename T>
class SafeData
{
public:
	SafeData() = default;
	virtual ~SafeData() = default;

	T get() const
	{
		QReadLocker locker(&lock_);
		return data_;
	}

	T data() const
	{
		QReadLocker locker(&lock_);
		return data_;
	}

	void set(const T& data)
	{
		QWriteLocker locker(&lock_);
		data_ = data;
	}

	SafeData<T>& operator=(const T& data)
	{
		set(data);
		return *this;
	}

	operator T() const
	{
		return get();
	}

	//==
	bool operator==(const T& data) const
	{
		return (get() == data);
	}

	//!=
	bool operator!=(const T& data) const
	{
		return (get() != data);
	}

private:
	T data_;
	mutable QReadWriteLock lock_;
};;

export template <typename T>
class SafeVector
{
public:
	SafeVector() = default;
	explicit SafeVector(__int64 size) : data_(size)
	{
	}

	explicit SafeVector(const QVector<T>& other) : data_(other)
	{
	}

	explicit SafeVector(QVector<T>&& other) : data_(other)
	{
	}

	explicit SafeVector(std::initializer_list<T> args) : data_(args)
	{
	}

	SafeVector<T>& operator=(const QVector<T>& other)
	{
		QWriteLocker locker(&lock_);
		data_ = other;
		return *this;
	}

	explicit SafeVector(const SafeVector<T>& other) : data_(other.data_)
	{
	}

	SafeVector(SafeVector<T>&& other) noexcept : data_(other.data_)
	{
	}

	SafeVector<T>& operator=(SafeVector<T>&& other) noexcept
	{
		QWriteLocker locker(&lock_);
		data_ = other.data_;
		return *this;
	}

	T* operator->()
	{
		QWriteLocker locker(&lock_);
		return data_.data();
	}

	explicit SafeVector(const std::vector<T>& other) : data_(other.begin(), other.end())
	{
	}

	T& operator[](__int64 i)
	{
		QWriteLocker locker(&lock_);
		if (i < 0 || i >= data_.size())
		{
			return defaultValue_;
		}
		return data_[i];
	}

	const T& operator[](__int64 i) const
	{
		QReadLocker locker(&lock_);
		if (i < 0 || i >= data_.size())
		{
			return defaultValue_;
		}
		return data_[i];
	}

	const T at(__int64 i) const
	{
		QReadLocker locker(&lock_);
		if (i < 0 || i >= data_.size())
		{
			return defaultValue_;
		}
		return data_.at(i);
	}

	bool contains(const T& value) const
	{
		QReadLocker locker(&lock_);
		return data_.contains(value);
	}

	bool operator==(const QVector<T>& other) const
	{
		QReadLocker locker(&lock_);
		return (data_ == other);
	}

	void clear()
	{
		QWriteLocker locker(&lock_);
		data_.clear();
	}

	void append(const T& value)
	{
		QWriteLocker locker(&lock_);
		data_.append(value);
	}

	void append(const SafeVector<T>& other)
	{
		QWriteLocker locker(&lock_);
		data_.append(other.data_);
	}

	void append(const QVector<T>& other)
	{
		QWriteLocker locker(&lock_);
		data_.append(other);
	}

	void append(const std::vector<T>& other)
	{
		QWriteLocker locker(&lock_);
		data_.append(other.begin(), other.end());
	}

	void append(const std::initializer_list<T>& args)
	{
		QWriteLocker locker(&lock_);
		data_.append(args);
	}

	void push_back(const T& value)
	{
		QWriteLocker locker(&lock_);
		data_.push_back(value);
	}

	typename QVector<T>::iterator begin()
	{
		QWriteLocker locker(&lock_);
		return data_.begin();
	}

	typename QVector<T>::iterator end()
	{
		QWriteLocker locker(&lock_);
		return data_.end();
	}

	typename QVector<T>::const_iterator cbegin() const
	{
		QReadLocker locker(&lock_);
		return data_.cbegin();
	}

	typename QVector<T>::const_iterator cend() const
	{
		QReadLocker locker(&lock_);
		return data_.cend();
	}

	void resize(__int64 size)
	{
		QWriteLocker locker(&lock_);
		data_.resize(size);
	}

	__int64 size() const
	{
		QReadLocker locker(&lock_);
		return data_.size();
	}

	bool isEmpty() const
	{
		QReadLocker locker(&lock_);
		return data_.isEmpty();
	}

	QVector<T> toVector() const
	{
		QReadLocker locker(&lock_);
		return data_;
	}

	T takeFirst()
	{
		QWriteLocker locker(&lock_);
		return data_.takeFirst();
	}

	T first() const
	{
		QReadLocker locker(&lock_);
		return data_.first();
	}

	T front() const
	{
		QReadLocker locker(&lock_);
		return data_.front();
	}

	void pop_front()
	{
		QWriteLocker locker(&lock_);
		data_.pop_front();
	}

	void erase(typename QVector<T>::iterator position)
	{
		QWriteLocker locker(&lock_);
		data_.erase(position);
	}

	void erase(typename QVector<T>::iterator first, typename QVector<T>::iterator last)
	{
		QWriteLocker locker(&lock_);
		data_.erase(first, last);
	}

	T value(__int64 i) const
	{
		QReadLocker locker(&lock_);
		return data_.value(i);
	}

	virtual ~SafeVector() = default;

private:
	QVector<T> data_;
	T defaultValue_;
	mutable QReadWriteLock lock_;
};