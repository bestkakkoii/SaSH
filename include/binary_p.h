#ifndef BINARY_P_H_
#define BINARY_P_H_

#include "binary.h"

#include <windows.h>
#include <vector>

/**
 * @brief A helper class for managing memory protection changes.
 */
class BinaryPrivate
{
public:
	/**
	 * @brief Constructor that changes the memory protection to PAGE_EXECUTE_READWRITE.
	 *
	 * @param address The base address of the memory region.
	 * @param size The size of the memory region.
	 */
	explicit BinaryPrivate(Binary* binary);

	/**
	 * @brief Destructor that restores the original memory protection.
	 */
	virtual ~BinaryPrivate();

	void begin();

	void end();

private:
	const HANDLE hProcess_;
	void* dest_ = nullptr;
	size_t size_ = 0;
	std::vector<uint8_t> originalData_;
	std::vector<uint8_t> writtenData_;
	ULONG oldProtect_ = 0;
	bool autoRestore_ = true;
	bool saveOnWrite_ = true;

private:
	Q_DECLARE_PUBLIC(Binary);

	Binary* q_ptr = nullptr;
};

#endif // BINARY_P_H_