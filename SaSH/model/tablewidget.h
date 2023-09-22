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

		//tablewidget set single selection
		setSelectionMode(QAbstractItemView::SingleSelection);
		//tablewidget set selection behavior
		setSelectionBehavior(QAbstractItemView::SelectRows);
		//set auto resize to form size
		horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

		verticalHeader()->setDefaultSectionSize(12);
		horizontalHeader()->setStretchLastSection(true);
		horizontalHeader()->setHighlightSections(false);
		verticalHeader()->setHighlightSections(false);
		horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
		verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
	}
	virtual ~TableWidget() = default;
};