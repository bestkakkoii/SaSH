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

#include "stdafx.h"
#include "astar.h"
#include <algorithm>

#if _MSVC_LANG > 201703L
#include <ranges>
#endif
#include <cassert>

#pragma region ASTAR

constexpr int kStepValue = 24;//10;
constexpr int kObliqueValue = 32;//14;

CAStar::CAStar()
	: step_val_(kStepValue)
	, oblique_val_(kObliqueValue)
	, height_(0)
	, width_(0)
{

}

CAStar::~CAStar()
{
	clear();
}

// 獲取直行估值
constexpr int CAStar::get_step_value() const
{
	return step_val_;
}

// 獲取拐角估值
constexpr int CAStar::get_oblique_value() const
{
	return oblique_val_;
}

// 設置直行估值
constexpr void CAStar::set_step_value(int value)
{
	step_val_ = value;
}

// 設置拐角估值
constexpr void CAStar::set_oblique_value(int value)
{
	oblique_val_ = value;
}

// 清理參數
void CAStar::clear()
{
	//size_t index = 0;
	//const size_t max_size = width_ * height_;
	//while (index < max_size)
	//{
	//	allocator_->free(mapping_[index++], sizeof(Node));
	//}

	open_list_.clear();
	can_pass_ = nullptr;
	width_ = height_ = 0;
}

// 初始化操作
void CAStar::init(const CAStarParam& param)
{
	width_ = param.width;
	height_ = param.height;
	can_pass_ = param.can_pass;
	mapping_.clear();
	mapping_.resize(width_ * height_);
	//mapping_.fill(0);
	memset(&mapping_[0], 0, sizeof(Node*) * mapping_.size());

	resource_.reset(new std::pmr::monotonic_buffer_resource(width_ * height_ * sizeof(Node)));
	allocator_.reset(new std::pmr::polymorphic_allocator<Node>(resource_.get()));
}

// 參數是否有效
bool CAStar::is_vlid_params(const CAStarParam& param) const
{
	return ((param.can_pass != nullptr)
		&& ((param.width > 0) && (param.height > 0))
		&& ((param.width < 1500) && (param.height < 1500))
		&& ((param.end.x() >= 0) && (param.end.x() < param.width))
		&& ((param.end.y() >= 0) && (param.end.y() < param.height))
		&& ((param.start.x() >= 0) && (param.start.x() < param.width))
		&& ((param.start.y() >= 0) && (param.start.y() < param.height))
		);
}

// 獲取節點索引
bool CAStar::get_node_index(Node*& node, int* index)
{
	*index = 0;
	int size = open_list_.size();
	while (*index < size)
	{
		if (open_list_[*index]->pos == node->pos)
		{
			return true;
	}
		++(*index);
}
	return false;
}

// 二叉堆上濾
void CAStar::percolate_up(int& hole)
{
	int parent = 0;
	while (hole > 0)
	{
		parent = (hole - 1) / 2;
		if (open_list_[hole]->f() < open_list_[parent]->f())
		{
#if _MSVC_LANG > 201703L
			std::ranges::swap(open_list_[hole], open_list_[parent]);
#else
			std::swap(open_list_[hole], open_list_[parent]);
#endif
			hole = parent;
		}
		else
		{
			return;
		}
	}
}

//#define Euclidean_distance
#if defined(Chebyshev_distance)
__forceinline int Chebyshev_Distance(const QPoint& current, const QPoint& end)
{
	return std::max(std::abs(current.x() - end.x()), std::abs(current.y() - end.y()));
}
#elif defined(Euclidean_distance)
__forceinline int Euclidean_Distance(const QPoint& current, const QPoint& end)
{
	return std::sqrt(std::pow(current.x() - end.x(), 2) + std::pow(current.y() - end.y(), 2));
}
#endif
// 計算G值

__forceinline int CAStar::calcul_g_value(Node*& parent, const QPoint& current)
{
#if defined(Chebyshev_distance)
	int g_value = qFloor(Chebyshev_Distance(current, parent->pos)) == 2 ? kObliqueValue : kStepValue;
	return g_value += parent->g;
#elif defined(Euclidean_distance)
	int g_value = qFloor(Euclidean_Distance(current, parent->pos)) == 2 ? kObliqueValue : kStepValue;
	return g_value += parent->g;
#else
	int g_value = (current - parent->pos).manhattanLength() == 2 ? kObliqueValue : kStepValue;
	g_value += parent->g;
	return g_value;
#endif
}

// 計算F值
__forceinline int CAStar::calcul_h_value(const QPoint& current, const QPoint& end)
{
#if defined(Chebyshev_distance)
	int h_value = Chebyshev_Distance(current, end);
#elif defined(Euclidean_distance)
	int h_value = Euclidean_Distance(current, end);
#else
	int h_value = (end - current).manhattanLength();

#endif
	return h_value * kStepValue;
}

// 節點是否存在於開啟列表
__forceinline bool CAStar::in_open_list(const QPoint& pos, Node*& out_node)
{
	out_node = mapping_[(int)pos.y() * width_ + (int)pos.x()];
	return out_node ? out_node->state == NodeState::IN_OPENLIST : false;
}

// 節點是否存在於關閉列表
__forceinline bool CAStar::in_closed_list(const QPoint& pos)
{
	Node* node_ptr = mapping_[(int)pos.y() * width_ + (int)pos.x()];
	return node_ptr ? node_ptr->state == NodeState::IN_CLOSEDLIST : false;
}

// 是否可到達
bool CAStar::can_pass(const QPoint& pos)
{
	return ((int)pos.x() >= 0 && (int)pos.x() < width_ && (int)pos.y() >= 0 && (int)pos.y() < height_) ? can_pass_(pos) : false;
}

// 當前點是否可到達目標點
bool CAStar::can_pass(const QPoint& current, const QPoint& destination, const bool& allow_corner)
{
	if ((int)destination.x() >= 0 && (int)destination.x() < width_ && (int)destination.y() >= 0 && (int)destination.y() < height_)
	{
		if (in_closed_list(destination))
			return false;
#if defined(Chebyshev_distance)
		if (qFloor(Chebyshev_Distance(destination, current)) == 1)
#elif defined(Euclidean_distance)
		if (qFloor(Euclidean_Distance(destination, current)) == 1)
#else
		if ((destination - current).manhattanLength() == 1)
#endif
		{
			return can_pass_(destination);
		}
		else if (allow_corner)
		{
			return (can_pass_(destination)) &&
				(can_pass(QPoint(current.x() + destination.x() - current.x(), current.y()))) &&
				(can_pass(QPoint(current.x(), current.y() + destination.y() - current.y())));
		}
	}
	return false;
}

// 查找附近可通過的節點
void CAStar::find_can_pass_nodes(const QPoint& current, const bool& corner, QVector<QPoint>* out_lists)
{
	QPoint destination;
	int row_index = current.y() - 1;
	int max_row = current.y() + 1;
	int max_col = current.x() + 1;

	if (row_index < 0)
	{
		row_index = 0;
	}

	while (row_index <= max_row)
	{
		int col_index = current.x() - 1;

		if (col_index < 0)
		{
			col_index = 0;
		}

		while (col_index <= max_col)
		{
			destination.setX(col_index);
			destination.setY(row_index);
			if (can_pass(current, destination, corner))
			{
				out_lists->push_back(destination);
			}
			++col_index;
		}
		++row_index;
	}
}

// 處理找到節點的情況
void CAStar::handle_found_node(Node*& current, Node*& destination)
{
	int g_value = calcul_g_value(current, destination->pos);
	if (g_value < destination->g)
	{
		destination->g = g_value;
		destination->parent = current;

		int index = 0;
		if (get_node_index(destination, &index))
		{
			percolate_up(index);
		}
		else
		{
			assert(false);
	}
}
}

// 處理未找到節點的情況
void CAStar::handle_not_found_node(Node*& current, Node*& destination, const QPoint& end)
{
	destination->parent = current;
	destination->h = calcul_h_value(destination->pos, end);
	destination->g = calcul_g_value(current, destination->pos);

	Node*& reference_node = mapping_[(int)destination->pos.y() * width_ + (int)destination->pos.x()];
	reference_node = destination;
	reference_node->state = NodeState::IN_OPENLIST;

	open_list_.push_back(destination);
#if _MSVC_LANG > 201703L
	std::ranges::push_heap(open_list_, [](const Node* a, const Node* b)->bool
#else
	std::push_heap(open_list_.begin(), open_list_.end(), [](const Node* a, const Node* b)->bool
#endif
		{
			return a->f() > b->f();
		});
}

// 執行尋路操作
QVector<QPoint> CAStar::find(const CAStarParam& param)
{
	QVector<QPoint> paths;
	if (!is_vlid_params(param))
	{
		return paths;
	}

	// 初始化
	init(param);
	QVector<QPoint> nearby_nodes;
	nearby_nodes.reserve(param.corner ? 8 : 4);
	constexpr size_t alloc_size(1u);
	// 將起點放入開啟列表
	//Node* start_node = new(allocator_->allocate(sizeof(Node))) Node(param.start);
	Node* start_node = allocator_->allocate(alloc_size);  // 分配内存
	std::allocator_traits<std::pmr::polymorphic_allocator<Node>>::construct(*allocator_, start_node, param.start);// 构造对象
	open_list_.push_back(start_node);
	Node*& reference_node = mapping_[start_node->pos.y() * width_ + start_node->pos.x()];
	reference_node = start_node;
	reference_node->state = NodeState::IN_OPENLIST;

	// 尋路操作
	while (!open_list_.empty())
	{
		// 找出f值最小節點
		Node* current = open_list_.front();
#if _MSVC_LANG > 201703L
		std::ranges::pop_heap(open_list_, [](const Node* a, const Node* b)->bool
#else
		std::pop_heap(open_list_.begin(), open_list_.end(), [](const Node* a, const Node* b)->bool
#endif
			{
				return a->f() > b->f();
			});
		open_list_.pop_back();
		mapping_[(int)current->pos.y() * width_ + (int)current->pos.x()]->state = NodeState::IN_CLOSEDLIST;

		// 是否找到終點
		if (current->pos == param.end)
		{
			while (current->parent)
			{
				paths.push_back(current->pos);
				current = current->parent;
			}
#if _MSVC_LANG > 201703L
			std::ranges::reverse(paths);
#else
			std::reverse(paths.begin(), paths.end());
#endif
			break;
		}

		// 查找周圍可通過節點
		nearby_nodes.clear();
		find_can_pass_nodes(current->pos, param.corner, &nearby_nodes);

		// 計算周圍節點的估值
		size_t index = 0;
		const size_t size = nearby_nodes.size();
		while (index < size)
		{
			Node* next_node = nullptr;
			if (in_open_list(nearby_nodes[index], next_node))
			{
				handle_found_node(current, next_node);
			}
			else
			{
				//next_node = new(allocator_->allocate(sizeof(Node))) Node(nearby_nodes[index]);
				next_node = allocator_->allocate(alloc_size);  // 分配内存
				std::allocator_traits<std::pmr::polymorphic_allocator<Node>>::construct(*allocator_, next_node, nearby_nodes[index]);// 构造对象
				handle_not_found_node(current, next_node, param.end);
			}
			++index;
		}
	}

	clear();
	return paths;
}
#pragma endregion