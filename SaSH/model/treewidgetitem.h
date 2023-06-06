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