/*
                GNU GENERAL PUBLIC LICENSE
                   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include <astardevice.h>

#include <algorithm>
#if _MSVC_LANG > 201703L
#include <ranges>
#endif

//#define USE_BSTAR
//#define Octile_distance


AStarDevice::AStarDevice()
    : rng_(std::random_device{}())
    , dist_(0.0, 1.0)
    , path_variation_factor_(0.2)
    , steps_before_variation_(15)
    , current_step_(0)
    , max_alternative_goals_(5)
    , max_deviation_factor_(0.3)
{
}

AStarDevice::~AStarDevice()
{
    clear();
}

void AStarDevice::initialize(qint64 width, qint64 height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (height_ == height && width_ == width)
        return;

    height_ = height;
    width_ = width;
    resource_.reset(q_check_ptr(new std::pmr::monotonic_buffer_resource(width_ * height_ * sizeof(Node) * 2)));
    Q_ASSERT(resource_ != nullptr);
    allocator_.reset(q_check_ptr(new std::pmr::polymorphic_allocator<Node>(resource_.get())));
    Q_ASSERT(allocator_ != nullptr);
    open_list_.clear();
    mapping_.clear();
    mapping_.resize(static_cast<size_t>(width_) * height_);
}

void AStarDevice::set_canpass(const AStarCallback& callback)
{
    can_pass_ = callback;
}

void AStarDevice::set_corner(bool corner)
{
    corner_ = corner;
}

// 清理參數
void AStarDevice::clear()
{
    // 释放 mapping_ 中的节点内存
    for (Node*& node : record_)
    {
        if (node)
        {
            std::allocator_traits<std::pmr::polymorphic_allocator<Node>>::destroy(*allocator_, node);
            allocator_->deallocate(node, 1);
        }
    }
    open_list_.clear();
    record_.clear();
    RtlSecureZeroMemory(mapping_.data(), mapping_.size() * sizeof(Node*));
}

// 參數是否有效
bool AStarDevice::is_valid_params(const QPoint& start, const QPoint& end) const
{

    return ((can_pass_ != nullptr)
        && ((start.x() >= 0) && (start.x() < width_))
        && ((start.y() >= 0) && (start.y() < height_))
        && ((end.x() >= 0) && (end.x() < width_))
        && ((end.y() >= 0) && (end.y() < height_))
        && width_ > 0 && height_ > 0);
}

// 獲取節點索引
bool AStarDevice::get_node_index(Node*& node, qint64* index)
{
    *index = 0;
    qint64 size = static_cast<qint64>(open_list_.size());
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
void AStarDevice::percolate_up(qint64& hole)
{
    qint64 parent = 0;
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

#if defined(Chebyshev_distance)
static __forceinline qint64 __fastcall Chebyshev_Distance(const QPoint& current, const QPoint& end)
{
    return std::max(std::abs(current.x() - end.x()), std::abs(current.y() - end.y()));
}
#elif defined(Euclidean_distance)
static __forceinline qint64 __fastcall Euclidean_Distance(const QPoint& current, const QPoint& end)
{
    return std::sqrt(std::pow(current.x() - end.x(), 2) + std::pow(current.y() - end.y(), 2));
}
#elif defined(Octile_distance)
static __forceinline qint64 __fastcall Octile_Distance(const QPoint& current, const QPoint& end)
{
    quint64 dx = std::abs(current.x() - end.x());
    quint64 dy = std::abs(current.y() - end.y());

    if (dx > dy)
    {
        return kStepValue * dx + (kObliqueValue - kStepValue) * dy;
    }

    return kStepValue * dy + (kObliqueValue - kStepValue) * dx;
}
#else 
static __forceinline qint64 __fastcall Manhattan_Distance(const QPoint& current, const QPoint& end)
{
    return (current - end).manhattanLength();
}
#endif
// 計算G值

// 節點是否存在於開啟列表
__forceinline bool AStarDevice::in_open_list(const QPoint& pos, Node*& out_node)
{
    out_node = mapping_[static_cast<size_t>(pos.y()) * width_ + pos.x()];
    return out_node ? out_node->state == NodeState::IN_OPENLIST : false;
}

// 節點是否存在於關閉列表
__forceinline bool AStarDevice::in_closed_list(const QPoint& pos)
{
    Node* node_ptr = mapping_[static_cast<size_t>(pos.y()) * width_ + pos.x()];
    return node_ptr ? node_ptr->state == NodeState::IN_CLOSEDLIST : false;
}

qreal AStarDevice::get_actual_cost(const QPoint& pos)
{
    if (pos.x() < 0 || pos.x() >= width_ || pos.y() < 0 || pos.y() >= height_)
    {
        return -1.0;
    }

    return can_pass_(pos);
}

// 是否可到達
bool AStarDevice::can_pass(const QPoint& pos)
{
    if (pos.x() < 0 || pos.x() >= width_ || pos.y() < 0 || pos.y() >= height_)
    {
        return false;
    }

    float cost = can_pass_(pos);
    return cost >= 0;
}


// 當前點是否可到達目標點
bool AStarDevice::can_pass(const QPoint& current, const QPoint& destination, const bool& allow_corner, qreal& cost)
{
    if (destination.x() < 0 || destination.x() >= width_ || destination.y() < 0 || destination.y() >= height_)
    {
        return false;
    }

    if (in_closed_list(destination))
    {
        return false;
    }

    qreal tile_cost = get_actual_cost(destination);
    if (tile_cost <= -1.0f)
    {
        return false;  // 不可通過的地塊
    }

    qint64 dx = static_cast<qint64>(destination.x() - current.x());
    qint64 dy = static_cast<qint64>(destination.y() - current.y());

    if (std::abs(dx) + std::abs(dy) == 1)
    {
        cost = ORTHOGONAL_COST * (tile_cost < 0 ? AVOIDANCE_PENALTY : tile_cost);
        return true;
    }

    if (!allow_corner || std::abs(dx) != 1 || std::abs(dy) != 1)
    {
        return false;
    }

    // 檢查對角移動的兩個相鄰地塊
    qreal cost1 = get_actual_cost(QPoint(current.x() + dx, current.y()));
    qreal cost2 = get_actual_cost(QPoint(current.x(), current.y() + dy));

    if (cost1 <= -1.0f || cost2 <= -1.0f)
    {
        return false;
    }

    cost = DIAGONAL_COST * (tile_cost < 0 ? AVOIDANCE_PENALTY : tile_cost);
    return true;
}

__forceinline qint64 AStarDevice::calcul_g_value(Node*& parent, const QPoint& current)
{
    qreal cost = 0.0;
    if (can_pass(parent->pos, current, corner_, cost))
    {
        return parent->g + static_cast<qint64>(cost);
    }

    return std::numeric_limits<qint64>::max();
}

// 計算F值
__forceinline qint64 AStarDevice::calcul_h_value(const QPoint& current, const QPoint& end)
{
    qint64 dx = std::abs(static_cast<qint64>(current.x() - end.x()));
    qint64 dy = std::abs(static_cast<qint64>(current.y() - end.y()));

    qint64 straight = std::abs(dx - dy);
    qint64 diagonal = std::min(dx, dy);

    qint64 base_h_value = straight * ORTHOGONAL_COST + diagonal * DIAGONAL_COST;

    // 增加對需要避開區域的懲罰
    qint64 avoidance_penalty = 0;
    for (qint64 y = std::max(0LL, current.y() - OBSTACLE_CHECK_RADIUS); y <= std::min(height_ - 1, current.y() + OBSTACLE_CHECK_RADIUS); ++y)
    {
        for (qint64 x = std::max(0LL, current.x() - OBSTACLE_CHECK_RADIUS); x <= std::min(width_ - 1, current.x() + OBSTACLE_CHECK_RADIUS); ++x)
        {
            qreal cost = get_actual_cost(QPoint(x, y));
            if (cost < 0 && cost > -1.0f)  // 檢查是否是需要避開的區域
            {
                qint64 distance = std::max(std::abs(x - current.x()), std::abs(y - current.y()));
                avoidance_penalty += static_cast<qint64>(AVOIDANCE_PENALTY * OBSTACLE_PENALTY / (distance + 1));
            }
        }
    }

    base_h_value += avoidance_penalty;

    if (++current_step_ % steps_before_variation_ == 0)
    {
        qreal variation = dist_(rng_) * path_variation_factor_ * 2.0 - path_variation_factor_;
        base_h_value = static_cast<qint64>(base_h_value * (1.0 + variation));
    }

    return base_h_value;
}

// 處理找到節點的情況
void AStarDevice::handle_found_node(Node*& current, Node*& destination)
{
    qint64 g_value = calcul_g_value(current, destination->pos);
    if (g_value < destination->g)
    {
        destination->g = g_value;
        destination->parent = current;

        qint64 index = 0;
        if (get_node_index(destination, &index))
        {
            percolate_up(index);
        }
        else
        {
            Q_ASSERT(false);
        }
    }
}

// 處理未找到節點的情況
void AStarDevice::handle_not_found_node(Node*& current, Node*& destination, const QPoint& end)
{
    qreal cost = 0.0;
    if (!can_pass(current->pos, destination->pos, corner_, cost))
    {
        return;
    }

    destination->parent = current;
    destination->h = calcul_h_value(destination->pos, end);
    destination->g = calcul_g_value(current, destination->pos);

    // Add obstacle avoidance cost
    qint64 obstacle_cost = 0;
    for (qint64 y = std::max(0LL, destination->pos.y() - OBSTACLE_CHECK_RADIUS); y <= std::min(height_ - 1, destination->pos.y() + OBSTACLE_CHECK_RADIUS); ++y)
    {
        for (qint64 x = std::max(0LL, destination->pos.x() - OBSTACLE_CHECK_RADIUS); x <= std::min(width_ - 1, destination->pos.x() + OBSTACLE_CHECK_RADIUS); ++x)
        {
            if (get_actual_cost(QPoint(x, y)) < 0)
            {
                qint64 distance = std::max(std::abs(x - destination->pos.x()), std::abs(y - destination->pos.y()));
                obstacle_cost += OBSTACLE_PENALTY / (distance + 1);
            }
        }
    }
    destination->g += obstacle_cost;

    destination->state = NodeState::IN_OPENLIST;
    mapping_[static_cast<size_t>(destination->pos.y()) * width_ + destination->pos.x()] = destination;

    open_list_.emplace_back(destination);
    std::push_heap(open_list_.begin(), open_list_.end(), [](const Node* a, const Node* b)->bool
        { return a->f() > b->f(); });
}

// 查找附近可通過的節點
void AStarDevice::find_can_pass_nodes(const QPoint& current, const bool& corner, std::vector<QPoint>* out_lists)
{
    QPoint destination;
    qint64 row_index = std::max(0, current.y() - 1);
    const qint64 max_row = std::min(height_ - 1LL, current.y() + 1LL);
    const qint64 max_col = std::min(width_ - 1LL, current.x() + 1LL);

    std::vector<std::pair<QPoint, qint64>> potential_nodes;

    for (; row_index <= max_row; ++row_index)
    {
        for (qint64 col_index = std::max(0, current.x() - 1); col_index <= max_col; ++col_index)
        {
            destination.setX(col_index);
            destination.setY(row_index);
            qreal cost = 0.0;
            if (can_pass(current, destination, corner, cost))
            {
                qint64 obstacle_distance = std::numeric_limits<qint64>::max();
                for (qint64 y = std::max(0LL, destination.y() - OBSTACLE_CHECK_RADIUS); y <= std::min(height_ - 1, destination.y() + OBSTACLE_CHECK_RADIUS); ++y)
                {
                    for (qint64 x = std::max(0LL, destination.x() - OBSTACLE_CHECK_RADIUS); x <= std::min(width_ - 1, destination.x() + OBSTACLE_CHECK_RADIUS); ++x)
                    {
                        if (get_actual_cost(QPoint(x, y)) < 0)
                        {
                            qint64 distance = std::max(std::abs(x - destination.x()), std::abs(y - destination.y()));
                            obstacle_distance = std::min(obstacle_distance, distance);
                        }
                    }
                }

                potential_nodes.emplace_back(destination, obstacle_distance);
            }
        }
    }

    // Sort potential nodes by their distance from obstacles (descending order)
    std::sort(potential_nodes.begin(), potential_nodes.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Add sorted nodes to the output list
    for (const auto& node : potential_nodes)
    {
        out_lists->push_back(node.first);
    }

    // Shuffle the nodes that are equally far from obstacles
    auto it = std::stable_partition(out_lists->begin(), out_lists->end(),
        [&](const QPoint& a) { return std::find_if(potential_nodes.cbegin(), potential_nodes.cend(),
            [&](const auto& p)->bool { return p.first == a && p.second == potential_nodes.front().second; }) != potential_nodes.cend(); });

    std::shuffle(out_lists->begin(), it, rng_);
}


std::vector<QPoint> AStarDevice::generate_alternative_goals(const QPoint& start, const QPoint& end)
{
    std::vector<QPoint> goals;
    QPoint direction = end - start;
    qreal distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());

    for (qint64 i = 0; i < max_alternative_goals_; ++i)
    {
        qreal deviation = dist_(rng_) * max_deviation_factor_ * distance;
        qreal angle = dist_(rng_) * 2 * M_PI;

        qint64 dx = static_cast<qint64>(deviation * std::cos(angle));
        qint64 dy = static_cast<qint64>(deviation * std::sin(angle));

        QPoint alt_goal = start + direction / 2 + QPoint(dx, dy);

        // 确保替代目标在网格范围内
        alt_goal.setX(std::clamp(static_cast<qint64>(alt_goal.x()), 0LL, static_cast<qint64>(width_) - 1LL));
        alt_goal.setY(std::clamp(static_cast<qint64>(alt_goal.y()), 0LL, static_cast<qint64>(height_) - 1LL));

        // 只有当目标可通过时才添加
        if (can_pass(alt_goal))
        {
            goals.push_back(alt_goal);
        }
    }

    // 如果没有找到任何替代目标，至少添加终点
    if (goals.empty())
    {
        goals.push_back(end);
    }

    return goals;
}

// 執行尋路操作
bool AStarDevice::find(const QPoint& start, const QPoint& end, std::vector<QPoint>* pPaths)
{
    std::lock_guard<std::mutex> lock(mutex_);
    current_step_ = 0;  // 重置步数计数器

    if (pPaths)
    {
        pPaths->clear();
    }

    if (!is_valid_params(start, end))
    {
        clear();
        return false;
    }

    start_ = start;
    end_ = end;

    std::vector<QPoint> nearby_nodes;
    nearby_nodes.reserve(corner_ ? 8 : 4);
    constexpr size_t alloc_size(1u);

    // 将起点放入开启列表
    Node* start_node = allocator_->allocate(alloc_size);  // 分配内存
    record_.emplace_back(start_node);
    std::allocator_traits<std::pmr::polymorphic_allocator<Node>>::construct(*allocator_, start_node, start_);// 构造对象
    open_list_.emplace_back(start_node);
    start_node->state = NodeState::IN_OPENLIST;
    mapping_[static_cast<size_t>(start_node->pos.y()) * width_ + start_node->pos.x()] = start_node;

    QPoint current_goal = end;

    // 寻路操作
    while (!open_list_.empty())
    {
        // 找出f值最小节点
        Node* current = nullptr;
        if (pPaths != nullptr && current_step_ % steps_before_variation_ == 0 && dist_(rng_) < path_variation_factor_)
        {
            // 在特定步数时有机会选择次优路径
            size_t random_index = std::min<size_t>(static_cast<size_t>(dist_(rng_) * 3), open_list_.size() - 1);
            std::nth_element(open_list_.begin(), open_list_.begin() + random_index, open_list_.end(),
                [](const Node* a, const Node* b) { return a->f() < b->f(); });
            current = open_list_[random_index];
            open_list_.erase(open_list_.begin() + random_index);
        }
        else
        {
            // 大部分时间选择最优路径
            current = open_list_.front();
            std::pop_heap(open_list_.begin(), open_list_.end(),
                [](const Node* a, const Node* b) { return a->f() > b->f(); });
            open_list_.pop_back();
        }

        mapping_[static_cast<size_t>(current->pos.y()) * width_ + current->pos.x()]->state = NodeState::IN_CLOSEDLIST;

        // 是否找到当前目标
        if (current->pos == current_goal)
        {
            // 找到最终目标
            if (current_goal == end)
            {
                // 僅用於測試不實際返回路徑
                if (nullptr == pPaths)
                {
                    clear();
                    return true;
                }

                // 生成路徑 (此處是終點到起點)
                while (current->parent)
                {
                    pPaths->emplace_back(current->pos);
                    current = current->parent;
                }

                // 反轉路徑 (轉換成起點到終點)
                std::reverse(pPaths->begin(), pPaths->end());
                break;
            }
            else
            {
                // 找到中间目标，现在瞄准最终目标
                current_goal = end;
            }
        }

        // 查找周围可通过节点
        nearby_nodes.clear();
        find_can_pass_nodes(current->pos, corner_, &nearby_nodes);

        // 计算周围节点的估值
        for (const auto& nearby_pos : nearby_nodes)
        {
            Node* next_node = nullptr;
            if (in_open_list(nearby_pos, next_node))
            {
                handle_found_node(current, next_node);
            }
            else
            {
                next_node = allocator_->allocate(alloc_size);  // 分配内存
                record_.push_back(next_node);
                std::allocator_traits<std::pmr::polymorphic_allocator<Node>>::construct(*allocator_, next_node, nearby_pos);// 构造对象
                handle_not_found_node(current, next_node, current_goal);
            }

            // 更新h值基于当前目标
            next_node->h = calcul_h_value(next_node->pos, current_goal);
        }

        ++current_step_;
    }

    clear();

    if (pPaths != nullptr)
    {
        return !pPaths->empty();
    }
    return false;
}