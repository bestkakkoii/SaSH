#pragma once
#include <QObject>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>

class TreeWidget : public QTreeWidget
{
	Q_OBJECT
public:
	explicit TreeWidget(QWidget* parent = nullptr)
		: QTreeWidget(parent)
	{
		setAttribute(Qt::WA_StyledBackground);
		//setStyleSheet(R"(
		//	QTreeWidget {
		//		font-size:12px;
		//	} 
		//	QTreeView::item:selected {
		//		background-color: black; 
		//		color: white; 
		//	} 
		//)");

		header()->setSectionsClickable(true);
		header()->setSectionResizeMode(QHeaderView::Interactive);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		resizeColumnToContents(1);
		sortItems(0, Qt::AscendingOrder);
		setIconSize(QSize(20, 20));

	}
	virtual ~TreeWidget() = default;
};