import Utility;
#include "tablewidget.h"

TableWidget::TableWidget(QWidget* parent)
	: QTableWidget(parent)
{
	setFont(util::getFont());
	setAttribute(Qt::WA_StyledBackground);
	setStyleSheet(R"(
			QTableWidget {
				font-size:12px;
			} 

			QTableView::item:selected {
				background-color: black;
				color: white;
			}
		)");

	__int64 rowCount = this->rowCount();
	__int64 columnCount = this->columnCount();
	for (__int64 row = 0; row < rowCount; ++row)
	{
		for (__int64 column = 0; column < columnCount; ++column)
		{
			QTableWidgetItem* item = new QTableWidgetItem("");
			if (item == nullptr)
				continue;

			setItem(row, column, item);
		}
	}

	//tablewidget set single selection
	setSelectionMode(QAbstractItemView::SingleSelection);
	//tablewidget set selection behavior
	setSelectionBehavior(QAbstractItemView::SelectRows);
	//set auto resize to form size
	horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

	//scroll bar ok
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	verticalHeader()->setDefaultSectionSize(12);
	verticalHeader()->setMaximumSectionSize(12);
	verticalHeader()->setStretchLastSection(false);
	verticalHeader()->setHighlightSections(false);
	verticalHeader()->setDefaultAlignment(Qt::AlignLeft);

	horizontalHeader()->setStretchLastSection(false);
	horizontalHeader()->setHighlightSections(false);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

void TableWidget::setItemForeground(__int64 row, __int64 column, const QColor& color)
{
	QTableWidgetItem* item = QTableWidget::item(row, column);
	if (item == nullptr)
		return;

	item->setForeground(color);
}

void TableWidget::setText(__int64 row, __int64 column, const QString& text, const QString& toolTip, QVariant data)
{
	__int64 max = QTableWidget::rowCount();
	if (row >= max)
	{
		for (__int64 i = max; i <= row; ++i)
			QTableWidget::insertRow(i);
	}

	QTableWidgetItem* item = QTableWidget::item(row, column);
	if (item != nullptr)
	{
		item->setText(text);
		if (!toolTip.isEmpty())
			item->setToolTip(toolTip);
		else
			item->setToolTip(text);

		item->setData(Qt::UserRole, data);
	}
	else
	{
		item = new QTableWidgetItem(text);
		if (item == nullptr)
			return;

		if (!toolTip.isEmpty())
			item->setToolTip(toolTip);
		else
			item->setToolTip(text);

		item->setData(Qt::UserRole, data);

		setItem(row, column, item);
	}
}

void TableWidget::setHorizontalHeaderText(__int64 col, const QString& text)
{
	QTableWidgetItem* item = QTableWidget::horizontalHeaderItem(col);
	if (item == nullptr)
	{
		item = new QTableWidgetItem(text);
		if (item == nullptr)
			return;

		item->setToolTip(text);
		QTableWidget::setHorizontalHeaderItem(col, item);
	}
	else
	{
		item->setText(text);
		item->setToolTip(text);
	}
}

//private
void TableWidget::setItem(__int64 row, __int64 column, QTableWidgetItem* item)
{
	if (item == nullptr)
		return;

	//height
	QSize size = item->sizeHint();
	size.setHeight(10);
	item->setSizeHint(size);

	QTableWidget::setItem(row, column, item);
}