#pragma once
#include <atomic>
#include <shared_mutex>
class SafeFlag
{
public:
	SafeFlag() = delete;
	SafeFlag(bool flag) : flag_(flag) {}

	//copy
	SafeFlag(const SafeFlag& other)
	{
		std::unique_lock<std::shared_mutex> lock(mutex_);
		flag_.store(other.flag_, std::memory_order_release);
	}

	//move
	SafeFlag(SafeFlag&& other) noexcept
	{
		std::unique_lock<std::shared_mutex> lock(mutex_);
		flag_.store(other.flag_, std::memory_order_release);
	}

	//copy assign
	SafeFlag& operator=(const SafeFlag& other)
	{
		std::unique_lock<std::shared_mutex> lock(mutex_);
		flag_.store(other.flag_, std::memory_order_release);
		return *this;
	}

	//move assign
	SafeFlag& operator=(SafeFlag&& other) noexcept
	{
		std::unique_lock<std::shared_mutex> lock(mutex_);
		flag_.store(other.flag_, std::memory_order_release);
		return *this;
	}

	operator bool() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex_);
		return flag_.load(std::memory_order_acquire);
	}

	void set(bool flag)
	{
		std::unique_lock<std::shared_mutex> lock(mutex_);
		flag_.store(flag, std::memory_order_release);
	}

	bool get() const
	{
		std::shared_lock<std::shared_mutex> lock(mutex_);
		return flag_.load(std::memory_order_acquire);
	}

	void reset()
	{
		set(false);
	}

	//=
	SafeFlag& operator=(bool flag) noexcept
	{
		set(flag);
		return *this;
	}

	//==
	bool operator==(bool flag) const
	{
		return get() == flag;
	}

	//!=
	bool operator!=(bool flag) const
	{
		return get() != flag;
	}

	//!
	bool operator!() const
	{
		return !get();
	}

	//~
	bool operator~() const
	{
		return !get();
	}

private:
	std::atomic_bool flag_ = false;
	mutable std::shared_mutex mutex_;
};