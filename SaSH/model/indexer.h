/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#pragma once

#include <atomic>
#include <QObject>

class Indexer
{
public:
	explicit Indexer(qint64 index) : index_(index) {}

	virtual inline void setIndex(qint64 index)
	{
		index_.store(index, std::memory_order_release);
	}

	virtual inline qint64 getIndex() const { return index_.load(std::memory_order_acquire); }

private:
	std::atomic_int64_t index_ = -1;
};