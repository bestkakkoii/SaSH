#pragma once
#include <QObject>
#include <QTableWidget>
#include <QHeaderView>

class TableWidget : public QTableWidget
{
	Q_OBJECT
public:
	explicit TableWidget(QWidget* parent = nullptr)
		: QTableWidget(parent)
	{
		setFont(util::getFont());
		setAttribute(Qt::WA_StyledBackground);
		setStyleSheet(R"(
QTableWidget::item {
    color: black;
    background: #FFFFFF;
	min-height: 11px;
    font-size:12px;
}

QTableWidget::item:selected {
    color: white;
    background:black;
}

QTableWidget::item:hover {
    color: white;
    background:black;
}

QTableWidget {
    color: black;
    background: #FFFFFF;
    border: 1px solid black;
    alternate-background-color: rgb(214,214,214);
    gridline-color: gray;
	font-size:12px;
	outline:0px; /*虛線框*/
}

QScrollBar:vertical {
	min-height: 30px;  
    background: #F1F1F1; 
}

QScrollBar::handle:vertical {
    background: #2D74DB;
  	border: 3px solid  #F1F1F1;
	min-height:50px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background: #3487FF;
}

QScrollBar:horizontal {
    background: #F1F1F1; 
}

QScrollBar::handle:horizontal {
    background: #2D74DB;
  	border: 3px solid  #F1F1F1;
	min-width:50px;
}

QScrollBar::handle:hover:horizontal,
QScrollBar::handle:pressed:horizontal {
    background: #3487FF;
}



		)");

		long long rowCount = this->rowCount();
		long long columnCount = this->columnCount();
		for (long long row = 0; row < rowCount; ++row)
		{
			for (long long column = 0; column < columnCount; ++column)
			{
				QTableWidgetItem* item = q_check_ptr(new QTableWidgetItem(""));
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

		verticalHeader()->setDefaultSectionSize(11);
		verticalHeader()->setMaximumSectionSize(11);
		verticalHeader()->setStretchLastSection(false);
		verticalHeader()->setHighlightSections(false);
		verticalHeader()->setDefaultAlignment(Qt::AlignLeft);

		horizontalHeader()->setStretchLastSection(false);
		horizontalHeader()->setHighlightSections(false);
		horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	}

	virtual ~TableWidget() = default;

	void setItemForeground(long long row, long long column, const QColor& color)
	{
		QTableWidgetItem* item = QTableWidget::item(row, column);
		if (item == nullptr)
			return;

		item->setForeground(color);
	}

	void setText(long long row, long long column, const QString& text, const QString& toolTip = "", QVariant data = QVariant())
	{
		long long max = QTableWidget::rowCount();
		if (row >= max)
		{
			for (long long i = max; i <= row; ++i)
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
			item = q_check_ptr(new QTableWidgetItem(text));
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

	void setHorizontalHeaderText(long long col, const QString& text)
	{
		QTableWidgetItem* item = QTableWidget::horizontalHeaderItem(col);
		if (item == nullptr)
		{
			item = q_check_ptr(new QTableWidgetItem(text));
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

private:
	void setItem(long long row, long long column, QTableWidgetItem* item)
	{
		if (item == nullptr)
			return;

		//height
		QSize size = item->sizeHint();
		size.setHeight(10);
		item->setSizeHint(size);

		QTableWidget::setItem(row, column, item);
	}
};