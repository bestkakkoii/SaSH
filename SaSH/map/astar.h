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
#if _MSC_VER >= 1600 
#pragma execution_character_set("utf-8") 
#endif


#include <memory_resource>
#include <functional>
#include <QPoint>

using AStarCallback = std::function<bool(const QPoint&)>;

class CAStar
{
private:
	/**
	 * 路徑節點狀態
	 */
	typedef enum
	{
		NOTEXIST,               // 不存在
		IN_OPENLIST,            // 在開啟列表
		IN_CLOSEDLIST           // 在關閉列表
	}NodeState;

	/**
	 * 路徑節點
	 */
	struct Node
	{
		qint64      g;          // 與起點距離
		qint64      h;          // 與終點距離
		QPoint      pos;        // 節點位置
		NodeState   state;      // 節點狀態
		Node* parent;     // 父節點

		/**
		 * 計算f值
		 */
		inline qint64 __fastcall f() const { return g + h; }

		inline Node(const QPoint& pos)
			: g(0), h(0), pos(pos), parent(nullptr), state(NodeState::NOTEXIST)
		{
		}
	};

public:
	explicit CAStar();

	virtual ~CAStar();

public:
	/**
	 * 獲取直行估值
	 */
	constexpr qint64 __fastcall get_step_value() const;

	/**
	 * 獲取拐角估值
	 */
	constexpr qint64 __fastcall get_oblique_value() const;

	/**
	 * 設置直行估值
	 */
	constexpr void __fastcall set_step_value(qint64 value);

	/**
	 * 獲取拐角估值
	 */
	constexpr void __fastcall set_oblique_value(qint64 value);

	void __fastcall set_canpass(const AStarCallback& callback);

	void __fastcall set_corner(bool corner);

	/**
	 * 初始化參數
	 */
	void __fastcall init(qint64 width, qint64 height);


	/**
	 * 執行尋路操作
	 */
	bool __fastcall find(const QPoint& start, const QPoint& end, std::vector<QPoint>* pPath);

private:
	/**
	 * 清理參數
	 */
	void clear();

	/**
	 * 參數是否有效
	 */
	bool __fastcall is_vlid_params(const QPoint& start, const QPoint& end) const;

private:
	/**
	 * 二叉堆上濾
	 */
	void __fastcall percolate_up(qint64& hole);

	/**
	 * 獲取節點索引
	 */
	bool __fastcall get_node_index(Node*& node, qint64* index);

	/**
	 * 計算G值
	 */
	__forceinline qint64 __fastcall calcul_g_value(Node*& parent, const QPoint& current);

	/**
	 * 計算F值
	 */
	__forceinline qint64 __fastcall calcul_h_value(const QPoint& current, const QPoint& end);

	/**
	 * 節點是否存在於開啟列表
	 */
	__forceinline bool __fastcall in_open_list(const QPoint& pos, Node*& out_node);

	/**
	 * 節點是否存在於關閉列表
	 */
	__forceinline bool __fastcall in_closed_list(const QPoint& pos);

	/**
	 * 是否可通過
	 */
	bool __fastcall can_pass(const QPoint& pos);

	/**
	 * 當前點是否可到達目標點
	 */
	bool __fastcall can_pass(const QPoint& current, const QPoint& destination, const bool& allow_corner);

	/**
	 * 查找附近可通過的節點
	 */
	void __fastcall find_can_pass_nodes(const QPoint& current, const bool& allow_corner, std::vector<QPoint>* out_lists);

	/**
	 * 處理找到節點的情況
	 */
	void __fastcall handle_found_node(Node*& current, Node*& destination);

	/**
	 * 處理未找到節點的情況
	 */
	void __fastcall handle_not_found_node(Node*& current, Node*& destination, const QPoint& end);

private:
	bool					corner_ = true;
	qint64                  step_val_;
	qint64                  oblique_val_;
	qint64                  height_ = 0;
	qint64                  width_ = 0;
	AStarCallback           can_pass_ = nullptr;
	QPoint					start_;
	QPoint					end_;
	std::mutex			    mutex_;
	std::vector<Node*>      mapping_;
	std::vector<Node*>      open_list_;
	std::vector<Node*>      record_;

#if _MSVC_LANG >= 201703L
	std::unique_ptr<std::pmr::monotonic_buffer_resource> resource_;
	std::unique_ptr<std::pmr::polymorphic_allocator<Node>> allocator_;
#endif
};