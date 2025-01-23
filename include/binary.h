#ifndef BINARY_H_
#define BINARY_H_

#include <vector>

class BinaryPrivate;

/**
 * @brief A class for managing and protecting binary data in memory.
 */
class Binary
{
public:
	/**
	 * @brief Constructor that initializes the binary manager.
	 */
	Binary();

	template<typename T>
	void initialize(T dest, size_t size)
	{
		void* destPtr = ((void*)(uintptr_t)(dest));
		save(destPtr, nullptr, size, false);
		setSaveOnWrite(false);
	}

	/**
	 * @brief Destructor that automatically restores the original data.
	 */
	virtual ~Binary();

	/**
	 * @brief Writes data to the specified memory region.
	 *
	 * @tparam T The type of the destination address (can be an integer or a pointer).
	 * @param dest The destination address.
	 * @param data The data to write (std::vector<uint8_t>).
	 */
	template<typename T>
	void write(T dest, const std::vector<uint8_t>& data)
	{
		void* srcPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data.data()));
		void* destPtr = ((void*)(uintptr_t)(dest));
		size_t dataSize = data.size();
		save(destPtr, srcPtr, dataSize);
		commit(srcPtr, destPtr);
	}

	/**
	 * @brief Writes data to the specified memory region.
	 *
	 * @tparam T The type of the destination address (can be an integer or a pointer).
	 * @param dest The destination address.
	 * @param data The data to write (const char*).
	 * @param size The size of the data to write.
	 */
	template<typename T>
	void write(T dest, const char* data, size_t size)
	{
		std::vector<uint8_t> byteData(data, data + size);
		write(dest, byteData);
	}

	void write(unsigned char data);

	void write(const std::vector<uint8_t>& data);

	void write(const char* data, size_t size = -1);

	/**
	 * @brief Restores the original data saved in the constructor.
	 */
	void restore();

	/**
	 * @brief Rewrites the written data back to the memory region.
	 */
	void rewrite();

private:


	/**
	 * @brief Internal method to save original data.
	 *
	 * @param dest The destination address.
	 * @param size The size of the memory region.
	 */
	void save(void* originalData, void* newData, size_t size, bool autoRestore = true);

	void setSaveOnWrite(bool enable);

	void commit(void* src, void* dest);

private:
	Q_DECLARE_PRIVATE(Binary);

	BinaryPrivate* d_ptr = nullptr;
};

#endif // BINARY_H_
