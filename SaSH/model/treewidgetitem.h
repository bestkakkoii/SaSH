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
#ifndef TREEWIDGETITEM_H
#define TREEWIDGETITEM_H
#include <QTreeWidgetItem>
class TreeWidgetItem : public QTreeWidgetItem
{
public:
	TreeWidgetItem(int type = Type) : QTreeWidgetItem(type) {}
	TreeWidgetItem(QStringList l, int type = Type) :QTreeWidgetItem(l, type) {}
	// 重写 operator<() 函数
	bool operator<(const QTreeWidgetItem& other) const
	{
		int column = treeWidget()->sortColumn();
		// 先比较两个项目的类型
		if (type() != other.type())
		{
			// 如果类型不同，資料夾排在文件前面
			return type() < other.type();
		}
		else
		{
			// 如果类型相同，则按照文本排序
			QString text1 = text(column);
			QString text2 = other.text(column);
			return text1.compare(text2, Qt::CaseInsensitive) < 0;
		}
	}
};
#endif