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
	class flag
	{
	public:
		flag() = default;
		flag(bool flag) : flag_(flag) {}

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

	class integer
	{
	public:
		integer() = default;
		integer(long long flag) : flag_(flag) {}

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
			flag_.store(0LL, std::memory_order_release);
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
			flag_.fetch_add(1LL, std::memory_order_release);
		}

		void dec()
		{
#ifdef SAFEFLAG_USE_MUTEX
			std::unique_lock<std::shared_mutex> lock(mutex_);
#endif
			flag_.fetch_sub(1LL, std::memory_order_release);
		}

	private:
		std::atomic_llong flag_ = 0LL;
#ifdef SAFEFLAG_USE_MUTEX
		mutable std::shared_mutex mutex_;
#endif
	};

	class auto_flag
	{
	public:
		auto_flag(flag* flag)
			: flag_(flag)
		{
			if (flag_ != nullptr)
				flag_->on();
		}

		~auto_flag()
		{
			if (flag_ != nullptr)
				flag_->off();
		}

	private:
		flag* flag_ = nullptr;

	};

	class auto_integer
	{
	public:
		auto_integer(integer* flag)
			: flag_(flag)
		{
			if (flag_ != nullptr)
				flag_->set(flag_->get() + 1);
		}

		~auto_integer()
		{
			if (flag_ != nullptr)
				flag_->set(flag_->get() - 1);
		}

	private:
		integer* flag_ = nullptr;
	};

#pragma region Data
	template <typename T>
	class data
	{
	public:
		data() = default;
		virtual ~data() = default;

		data(T d) : data_(d) {}

		T get() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return data_;
		}

		void set(const T& d)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = d;
		}

		void reset()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = T();
		}

		//==
		bool operator==(const T& d) const
		{
			return (get() == d);
		}

		//!=
		bool operator!=(const T& d) const
		{
			return (get() != d);
		}

	private:
		T data_;
		mutable std::shared_mutex mutex_;
	};
#pragma endregion

#pragma region Hash
	//基於Qt QHash 的線程安全Hash容器
	template <typename K, typename V>
	class hash
	{
	public:
		hash() = default;

		inline hash(std::initializer_list<std::pair<K, V> > list)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = list;
		}

		bool isEmpty() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.isEmpty();
		}

		//copy
		hash(const QHash<K, V>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = other;
		}

		//copy assign
		hash(const hash<K, V>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = other.hash_;
		}

		//move
		hash(QHash<K, V>&& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = other;
		}

		//move assign
		hash(hash<K, V>&& other) noexcept
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = other.hash_;
		}

		hash operator=(const hash& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = other.hash_;
			return *this;
		}

		//operator=
		hash operator=(const QHash <K, V>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_ = other;
			return *this;
		}

		inline void insert(const K& key, const V& value)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_.insert(key, value);
		}
		inline void remove(const K& key)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_.remove(key);
		}
		inline bool contains(const K& key) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.contains(key);
		}
		inline V value(const K& key) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.value(key);
		}
		inline V value(const K& key, const V& defaultValue) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.value(key, defaultValue);
		}

		inline long long size() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.size();
		}
		inline QList <K> keys() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.keys();
		}

		inline QList <V> values() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.values();
		}
		//take
		inline V take(const K& key)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return hash_.take(key);
		}

		inline void clear()
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			hash_.clear();
		}

		inline K key(const V& value) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.key(value);
		}

		inline K key(const V& value, const K& defaultValue) const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.key(value, defaultValue);
		}

		inline typename QHash<K, V>::iterator begin()
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.begin();
		}

		inline typename QHash<K, V>::const_iterator begin() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.begin();
		}

		//const
		inline typename QHash<K, V>::const_iterator cbegin() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.constBegin();
		}

		inline typename QHash<K, V>::iterator end()
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.end();
		}

		//const
		inline typename QHash<K, V>::const_iterator cend() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.constEnd();
		}

		inline typename QHash<K, V>::const_iterator end() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.end();
		}

		//erase
		inline  typename QHash<K, V>::iterator erase(typename QHash<K, V>::iterator it)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			return hash_.erase(it);
		}

		//find
		inline typename QHash<K, V>::iterator find(const K& key)
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_.find(key);
		}

		QHash <K, V> toHash() const
		{
			std::shared_lock<std::shared_mutex> lock(mutex_);
			return hash_;
		}

	private:
		QHash<K, V> hash_;
		mutable std::shared_mutex mutex_;
	};
#pragma endregion

#pragma region queue
	template <typename V>
	class queue
	{
	public:
		queue() = default;
		explicit queue(long long maxSize)
			: maxSize_(maxSize)
		{
		}
		virtual ~queue() = default;

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
		queue& operator=(const queue& other)
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
	class vector
	{
	public:
		vector() = default;
		virtual ~vector() = default;
		explicit vector(long long size) : data_(size)
		{
		}

		vector(long long size, T fill) : data_(size, fill)
		{
		}

		explicit vector(const QVector<T>& other) : data_(other)
		{
		}

		explicit vector(QVector<T>&& other) : data_(other)
		{
		}

		explicit vector(std::initializer_list<T> args) : data_(args)
		{
		}

		vector<T>& operator=(const QVector<T>& other)
		{
			std::unique_lock<std::shared_mutex> lock(mutex_);
			data_ = other;
			return *this;
		}

		explicit vector(const vector<T>& other) : data_(other.data_)
		{
		}

		vector(vector<T>&& other) noexcept : data_(other.data_)
		{
		}

		vector<T>& operator=(vector<T>&& other) noexcept
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

		explicit vector(const std::vector<T>& other) : data_(other.begin(), other.end())
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

		void append(const vector<T>& other)
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