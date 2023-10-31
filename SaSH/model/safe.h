#pragma once
#include <atomic>
#include <QHash>
#include <QVector>
#include <QQueue>
#define SAFEFLAG_USE_MUTEX
#ifdef SAFEFLAG_USE_MUTEX
#include <shared_mutex>
#endif

namespace safe
{
	class Flag
	{
	public:
		Flag() = default;
		Flag(bool flag) : flag_(flag) {}

		void on()
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.store(true, std::memory_order_release);
		}

		void set(bool flag)
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.store(flag, std::memory_order_release);
		}

		bool get() const
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::shared_lock<std::shared_mutex> lock(mutex_);
#endif
			return flag_.load(std::memory_order_acquire);
		}

		void off()
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.store(false, std::memory_order_release);
		}
	private:
		std::atomic_bool flag_ = false;
#ifdef SAFEFLAG_USE_MUTEX
		mutable std::shared_mutex mutex_;
#endif
	};

	class Integer
	{
	public:
		Integer() = default;
		Integer(long long flag) : flag_(flag) {}

		void set(long long flag)
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.store(flag, std::memory_order_release);
		}

		long long get() const
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::shared_lock<std::shared_mutex> lock(mutex_);
#endif
			return flag_.load(std::memory_order_acquire);
		}

		void reset()
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.store(0, std::memory_order_release);
		}

		void add(long long flag)
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.fetch_add(flag, std::memory_order_release);
		}

		void sub(long long flag)
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.fetch_sub(flag, std::memory_order_release);
		}

		void inc()
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.fetch_add(1, std::memory_order_release);
		}

		void dec()
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.fetch_sub(1, std::memory_order_release);
		}

	private:
		std::atomic_llong flag_ = 0;
#ifdef SAFEFLAG_USE_MUTEX
		mutable std::shared_mutex mutex_;
#endif
	};

	class AutoFlag
	{
	public:
		AutoFlag(Flag* flag)
			: flag_(flag)
		{
			if (flag_ != nullptr)
				flag_->on();
		}

		~AutoFlag()
		{
			if (flag_ != nullptr)
				flag_->off();
		}

	private:
		Flag* flag_ = nullptr;

	};

	class AutoInteger
	{
	public:
		AutoInteger(Integer* flag)
			: flag_(flag)
		{
			if (flag_ != nullptr)
				flag_->set(flag_->get() + 1);
		}

		~AutoInteger()
		{
			if (flag_ != nullptr)
				flag_->set(flag_->get() - 1);
		}

	private:
		Integer* flag_ = nullptr;
	};

#pragma region Data
	template <typename T>
	class Data
	{
	public:
		Data() = default;
		virtual ~Data() = default;

		Data(T data) : data_(data) {}

		T get() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_;
		}

		void set(const T& data)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = data;
		}

		void reset()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = T();
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
		mutable std::shared_mutex mutex_;
	};
#pragma endregion

#pragma region Hash
	//基於Qt QHash 的線程安全Hash容器
	template <typename K, typename V>
	class Hash
	{
	public:
		Hash() = default;

		inline Hash(std::initializer_list<std::pair<K, V> > list)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = list;
		}

		bool isEmpty() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.isEmpty();
		}

		//copy
		Hash(const QHash<K, V>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = other;
		}

		//copy assign
		Hash(const Hash<K, V>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = other.hash;
		}

		//move
		Hash(QHash<K, V>&& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = other;
		}

		//move assign
		Hash(Hash<K, V>&& other) noexcept
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = other.hash;
		}

		Hash operator=(const Hash& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = other.hash;
			return *this;
		}

		//operator=
		Hash operator=(const QHash <K, V>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash = other;
			return *this;
		}

		inline void insert(const K& key, const V& value)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash.insert(key, value);
		}
		inline void remove(const K& key)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash.remove(key);
		}
		inline bool contains(const K& key) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.contains(key);
		}
		inline V value(const K& key) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.value(key);
		}
		inline V value(const K& key, const V& defaultValue) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.value(key, defaultValue);
		}

		inline long long size() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.size();
		}
		inline QList <K> keys() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.keys();
		}

		inline QList <V> values() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.values();
		}
		//take
		inline V take(const K& key)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return hash.take(key);
		}

		inline void clear()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash.clear();
		}

		inline K key(const V& value) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.key(value);
		}

		inline K key(const V& value, const K& defaultValue) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.key(value, defaultValue);
		}

		inline typename QHash<K, V>::iterator begin()
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.begin();
		}

		inline typename QHash<K, V>::const_iterator begin() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.begin();
		}

		//const
		inline typename QHash<K, V>::const_iterator cbegin() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.constBegin();
		}

		inline typename QHash<K, V>::iterator end()
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.end();
		}

		//const
		inline typename QHash<K, V>::const_iterator cend() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.constEnd();
		}

		inline typename QHash<K, V>::const_iterator end() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.end();
		}

		//erase
		inline  typename QHash<K, V>::iterator erase(typename QHash<K, V>::iterator it)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return hash.erase(it);
		}

		//find
		inline typename QHash<K, V>::iterator find(const K& key)
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash.find(key);
		}

		QHash <K, V> toHash() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash;
		}

	private:
		QHash<K, V> hash;
		mutable std::shared_mutex mutex_;
	};
#pragma endregion

#pragma region Queue
	template <typename V>
	class Queue
	{
	public:
		Queue() = default;
		explicit Queue(long long maxSize)
			: maxSize_(maxSize)
		{
		}
		virtual ~Queue() = default;

		void clear()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			queue_.clear();
		}

		QVector<V> values() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return queue_.toVector();
		}

		long long size() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return queue_.size();
		}

		bool isEmpty() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return queue_.isEmpty();
		}

		void enqueue(const V& value)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			if (queue_.size() >= maxSize_ && maxSize_ != 0)
			{
				queue_.dequeue();
			}
			queue_.enqueue(value);
		}

		bool dequeue(V* pvalue)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			if (queue_.isEmpty())
			{
				return false;
			}
			if (pvalue)
				*pvalue = queue_.dequeue();
			return true;
		}

		V dequeue()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return std::move(queue_.dequeue());
		}

		V front() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return queue_.head();
		}

		void setMaxSize(long long maxSize)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			queue_.setMaxSize(maxSize);
		}

		//=
		Queue& operator=(const Queue& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			if (this != &other)
			{
				queue_ = other.queue_;
			}
			return *this;
		}

	private:
		QQueue<V> queue_;
		long long maxSize_ = 0;
		mutable std::shared_mutex mutex_;
	};;
#pragma endregion

#pragma region Vector
	template <typename T>
	class Vector
	{
	public:
		Vector() = default;
		virtual ~Vector() = default;
		explicit Vector(long long size) : data_(size)
		{
		}

		explicit Vector(const QVector<T>& other) : data_(other)
		{
		}

		explicit Vector(QVector<T>&& other) : data_(other)
		{
		}

		explicit Vector(std::initializer_list<T> args) : data_(args)
		{
		}

		Vector<T>& operator=(const QVector<T>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = other;
			return *this;
		}

		explicit Vector(const Vector<T>& other) : data_(other.data_)
		{
		}

		Vector(Vector<T>&& other) noexcept : data_(other.data_)
		{
		}

		Vector<T>& operator=(Vector<T>&& other) noexcept
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = other.data_;
			return *this;
		}

		T* operator->()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return data_.data();
		}

		explicit Vector(const std::vector<T>& other) : data_(other.begin(), other.end())
		{
		}

		T& operator[](long long i)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			if (i < 0 || i >= data_.size())
			{
				return defaultValue_;
			}
			return data_[i];
		}

		const T& operator[](long long i) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			if (i < 0 || i >= data_.size())
			{
				return defaultValue_;
			}
			return data_[i];
		}

		const T at(long long i) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.value(i);
		}

		T value(long long i) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.value(i);
		}

		bool contains(const T& value) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.contains(value);
		}

		bool operator==(const QVector<T>& other) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return (data_ == other);
		}

		void clear()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.clear();
		}

		void append(const T& value)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.append(value);
		}

		void append(const Vector<T>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.append(other.data_);
		}

		void append(const QVector<T>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.append(other);
		}

		void append(const std::vector<T>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.append(other.begin(), other.end());
		}

		void append(const std::initializer_list<T>& args)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.append(args);
		}

		void push_back(const T& value)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.push_back(value);
		}

		typename QVector<T>::iterator begin()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return data_.begin();
		}

		typename QVector<T>::iterator end()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return data_.end();
		}

		typename QVector<T>::const_iterator cbegin() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.cbegin();
		}

		typename QVector<T>::const_iterator cend() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.cend();
		}

		void resize(long long size)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.resize(size);
		}

		long long size() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.size();
		}

		bool isEmpty() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.isEmpty();
		}

		QVector<T> toVector() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_;
		}

		T takeFirst()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return data_.takeFirst();
		}

		T first() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.first();
		}

		T front() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_.front();
		}

		void pop_front()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.pop_front();
		}

		void erase(typename QVector<T>::iterator position)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.erase(position);
		}

		void erase(typename QVector<T>::iterator first, typename QVector<T>::iterator last)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_.erase(first, last);
		}

	private:
		QVector<T> data_ = QVector<T>();
		T defaultValue_ = T();
		mutable std::shared_mutex mutex_;
	};
#pragma endregion
}