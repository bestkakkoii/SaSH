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

	// 獲取字符類型
	int getCharType(const QChar& ch) const
	{
		if (ch.isPunct() || ch.isSymbol())
			return 0; // 全角標點和符號
		else if (ch.isDigit())
			return 1; // 數字
		else if (ch.isLower())
			return 2; // 英文小寫
		else if (ch.isUpper())
			return 3; // 英文大寫
		else
			return 4; // 中文筆畫數
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

		// 按字符類型和優先級進行排序
		for (int i = 0; i < qMin(text1.length(), text2.length()); ++i)
		{
			QChar char1 = text1.at(i);
			QChar char2 = text2.at(i);

			int charType1 = getCharType(char1);
			int charType2 = getCharType(char2);

			if (charType1 != charType2)
				return charType1 < charType2;
			else if (char1 != char2)
				return char1 < char2;
		}

		// 如果前面的字符都相同，則按長度排序
		return text1.length() < text2.length();
	}

};
#endif