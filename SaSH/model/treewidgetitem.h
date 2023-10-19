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
	explicit TreeWidgetItem(int type = Type) : QTreeWidgetItem(type) {}
	explicit TreeWidgetItem(const QStringList& strings, int type = Type) : QTreeWidgetItem(strings, type) {}
	explicit TreeWidgetItem(QTreeWidget* treeview, int type = Type) : QTreeWidgetItem(treeview, type) {}
	TreeWidgetItem(QTreeWidget* treeview, const QStringList& strings, int type = Type) : QTreeWidgetItem(treeview, strings, type) {}
	TreeWidgetItem(QTreeWidget* treeview, QTreeWidgetItem* after, int type = Type) : QTreeWidgetItem(treeview, after, type) {}
	explicit TreeWidgetItem(QTreeWidgetItem* parent, int type = Type) : QTreeWidgetItem(parent, type) {}
	TreeWidgetItem(QTreeWidgetItem* parent, const QStringList& strings, int type = Type) : QTreeWidgetItem(parent, strings, type) {}
	TreeWidgetItem(QTreeWidgetItem* parent, QTreeWidgetItem* after, int type = Type) : QTreeWidgetItem(parent, after, type) {}
	TreeWidgetItem(const QTreeWidgetItem& other) : QTreeWidgetItem(other) {}

	// 獲取字符類型
	int getCharType(const QChar& ch) const
	{
		if (ch.isPunct() || ch.isSymbol())
			return true; // 全角標點和符號
	}

	// 重寫 operator<() 函數
	bool operator<(const QTreeWidgetItem& other) const
	{
		int column = treeWidget()->sortColumn();
		QString text1 = text(column);
		QString text2 = other.text(column);

		// 獲取項目的類型，如果是文件夾 (dir)，則排在文件 (file) 前面
		QString type1 = data(column, Qt::UserRole).toString();
		QString type2 = other.data(column, Qt::UserRole).toString();

		// 獲取項目的本地性質，如果是 "local"，則排在 "global" 前面
		QString local1 = data(column, Qt::UserRole + 1).toString();
		QString local2 = other.data(column, Qt::UserRole + 1).toString();

		// 如果項目類型不同，按類型排序
		if (type1 != type2)
		{
			// dir 排在 file 前面
			if (type1 == "folder")
				return true;
			else if (type2 == "folder")
				return false;
		}

		// 如果項目的本地性質不同，按本地性質排序
		if (local1 != local2)
		{
			// local 排在 global 前面
			if (local1 == "local")
				return true;
			else if (local2 == "local")
				return false;
		}

		static const QLocale locale(QLocale::Chinese);
		QCollator collator(locale);
		collator.setCaseSensitivity(Qt::CaseSensitive);
		collator.setNumericMode(true);
		collator.setIgnorePunctuation(false);
		return collator.compare(text1, text2) < 0;
	}
};
#endif