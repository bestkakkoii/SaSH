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
		setFrameShape(QFrame::NoFrame);

		setStyleSheet(R"(
QWidget {
    color: black;
    background: white;
}

QTableWidget {
    color: black;
    background-color: white;
    border: 1px solid gray;
    alternate-background-color: white;
    gridline-color: gray;
	font-size:12px;
	outline:0px; /*虛線框*/
}

QTableWidget QTableCornerButton::section {
    background-color: white;
    border: 1px solid gray;
}

QHeaderView::section {
    color: black;
    background-color: white;
	border: 1px solid gray;
}

QHeaderView::section:horizontal
{
    color: black;
    background-color: white;
}

QHeaderView::section:vertical
{
    color: black;
    background-color: white;
}
/*
QTableWidget::item {
    color: black;
    background-color: white;
	min-height: 11px;
    font-size:12px;
}

QTableWidget::item:selected {
    color: white;
    background-color:black;
}

QTableWidget::item:hover {
    color: white;
    background-color:black;
}
*/
QScrollBar:vertical {
	min-height: 30px;  
    background-color: white; 
}

QScrollBar::handle:vertical {
    background-color: white;
  	border: 3px solid  white;
	min-height:30px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background-color: #3487FF;
}

QScrollBar:horizontal {
    background-color: white; 
}

QScrollBar::handle:horizontal {
    background-color: #3282F6;
  	border: 3px solid white;
	min-width:50px;
}

QScrollBar::handle:hover:horizontal,
QScrollBar::handle:pressed:horizontal {
    background-color: #3487FF;
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

		QAbstractButton* button = findChild<QAbstractButton*>();
		if (button)
		{
			QVBoxLayout* lay = new QVBoxLayout(button);
			lay->setContentsMargins(0, 0, 0, 0);
			pCornerLabel = q_check_ptr(new QLabel());
			assert(pCornerLabel != nullptr);
			if (pCornerLabel)
			{
				pCornerLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
				pCornerLabel->setContentsMargins(0, 0, 0, 0);
				lay->addWidget(pCornerLabel);
			}
		}
	}

	virtual ~TableWidget() = default;

	void setCornerText(const QString& text)
	{
		if (pCornerLabel)
			pCornerLabel->setText(text);
	}

	void setItemForeground(long long row, long long column, const QColor& color)
	{
		QTableWidgetItem* item = QTableWidget::item(row, column);
		if (item == nullptr)
			return;

		if (item->text().simplified().remove("　").isEmpty())
		{
			return;
		}

		item->setForeground(color);
	}

	void setItemBackground(long long row, long long column, const QColor& color)
	{
		QTableWidgetItem* item = QTableWidget::item(row, column);
		if (item == nullptr)
			return;

		if (item->text().simplified().remove("　").isEmpty())
		{
			item->setBackground(Qt::white);
			return;
		}

		if (color != QColor(Qt::white))
			item->setForeground(Qt::white);
		else
			item->setForeground(Qt::black);
		item->setBackground(color);
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

private:
	QLabel* pCornerLabel = nullptr;

};