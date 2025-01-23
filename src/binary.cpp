#include "stdafx.h"
#include "binary_p.h"

/**
 * @brief Changes the memory protection of the specified memory region to PAGE_EXECUTE_READWRITE.
 *
 * @param address The base address of the memory region.
 * @param size The size of the memory region.
 */
BinaryPrivate::BinaryPrivate(Binary* binary)
    : q_ptr(binary)
    , hProcess_(GetCurrentProcess())
{
}

/**
 * @brief Restores the original memory protection of the specified memory region.
 */
BinaryPrivate::~BinaryPrivate()
{
}

void BinaryPrivate::begin()
{
    if (nullptr == dest_)
    {
        qCritical().noquote() << "No data to save.";
        return;
    }

    PVOID baseAddress = dest_;
    SIZE_T regionSize = size_;

    // Call MINT::NtProtectVirtualMemory to change the memory protection to PAGE_EXECUTE_READWRITE
    BOOL status = VirtualProtectEx(hProcess_, baseAddress, regionSize, PAGE_EXECUTE_READWRITE, &oldProtect_);
    if (status == 0)
    {
        qCritical().noquote() << "Failed to change memory protection with status" << QString::number(GetLastError(), 16);
    }
}

void BinaryPrivate::end()
{
    if (nullptr == dest_)
    {
        qCritical().noquote() << "No data to restore.";
        return;
    }

    PVOID baseAddress = dest_;
    SIZE_T regionSize = size_;

    // Call MINT::NtProtectVirtualMemory to restore the original memory protection
    BOOL status = VirtualProtectEx(hProcess_, baseAddress, regionSize, oldProtect_, &oldProtect_);
    if (status == 0)
    {
        qCritical().noquote() << "Failed to restore memory protection with status" << QString::number(GetLastError(), 16);
    }
}

/**
 * @brief Constructor that initializes the binary manager.
 */
Binary::Binary()
    : d_ptr(new BinaryPrivate(this))
{
}

/**
 * @brief Destructor that automatically restores the original data.
 */
Binary::~Binary()
{
    Q_D(Binary);
    if (!d->autoRestore_)
    {
        qCritical().noquote() << "Auto restore is disabled.";
        return;
    }

    restore();

    delete d;
}

/**
 * @brief Internal method to save original data.
 *
 * @param dest The destination address.
 * @param size The size of the memory region.
 */
void Binary::save(void* originalData, void* newData, size_t size, bool autoRestore)
{
    Q_D(Binary);
    if (nullptr == originalData || size == 0)
    {
        qCritical().noquote() << "Invalid memory region." << originalData << newData << size;
        return;
    }

    d->autoRestore_ = autoRestore;
    if (!d->saveOnWrite_)
    {
        return;
    }

    d->dest_ = originalData;
    d->size_ = size;
    d->originalData_.resize(size);
    d->writtenData_.resize(size);
    std::memmove(d->originalData_.data(), originalData, size);

    if (newData != nullptr)
    {
        std::memmove(d->writtenData_.data(), newData, size);
    }
}

void Binary::setSaveOnWrite(bool saveOnWrite)
{
    Q_D(Binary);
    d->saveOnWrite_ = saveOnWrite;
}

/**
 * @brief Restores the original data saved in the constructor.
 *
 * This function restores the original data that was saved during the first write
 * operation.
 */
void Binary::restore()
{
    Q_D(Binary);
    if (nullptr == d->dest_ || d->originalData_.empty() && std::memcmp(d->dest_, d->originalData_.data(), d->size_) == 0)
    {
        qCritical().noquote() << "No data to restore.";
        return;
    }

    commit(d->originalData_.data(), d->dest_);
}

/**
 * @brief Rewrites the written data back to the memory region.
 *
 * This function writes the written data back to the memory region.
 */
void Binary::rewrite()
{
    Q_D(Binary);
    if (nullptr == d->dest_ || d->writtenData_.empty() && std::memcmp(d->dest_, d->writtenData_.data(), d->writtenData_.size()) == 0)
    {
        qCritical().noquote() << "No data to rewrite.";
        return;
    }

    commit(d->writtenData_.data(), d->dest_);
}

void Binary::commit(void* src, void* dest)
{
    Q_D(Binary);
    d->begin();
    std::memmove(dest, src, d->size_);
    d->end();
}

void  Binary::write(unsigned char data)
{
    Q_D(Binary);

    write(d->dest_, std::vector<uint8_t>(d->size_, data));
}

void  Binary::write(const std::vector<uint8_t>& data)
{
    Q_D(Binary);
    write(d->dest_, data);
}

void Binary::write(const char* data, size_t size)
{
    Q_D(Binary);
    if (-1 == size)
    {
        size = d->size_;
    }

    write(d->dest_, data, size);
}