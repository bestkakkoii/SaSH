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
			QTableWidget {
				font-size:12px;
			} 

			QTableView::item:selected {
				background-color: black;
				color: white;
			}
		)");

		qint64 rowCount = this->rowCount();
		qint64 columnCount = this->columnCount();
		for (qint64 row = 0; row < rowCount; ++row)
		{
			for (qint64 column = 0; column < columnCount; ++column)
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
	virtual ~TableWidget() = default;

	void setItem(qint64 row, qint64 column, QTableWidgetItem* item)
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