#include "stdafx.h"
#include "playerinfoform.h"
#include "abilityform.h"

#include "signaldispatcher.h"

PlayerInfoForm::PlayerInfoForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);


	//register meta type of QVariant
	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	auto setTableWidget = [](QTableWidget* tableWidget)->void
	{
		//tablewidget set single selection
		tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
		//tablewidget set selection behavior
		tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
		//set auto resize to form size
		tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

		tableWidget->setStyleSheet(R"(
		QTableWidget { font-size:11px; } 
			QTableView::item:selected { background-color: black; color: white;
		})");
		tableWidget->verticalHeader()->setDefaultSectionSize(11);
		tableWidget->horizontalHeader()->setStretchLastSection(true);
		tableWidget->horizontalHeader()->setHighlightSections(false);
		tableWidget->verticalHeader()->setHighlightSections(false);
		tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
		tableWidget->verticalHeader()->setDefaultAlignment(Qt::AlignLeft);

		int rowCount = tableWidget->rowCount();
		int columnCount = tableWidget->columnCount();
		for (int row = 0; row < rowCount; ++row)
		{
			for (int column = 0; column < columnCount; ++column)
			{
				QTableWidgetItem* item = new QTableWidgetItem("");
				if (item)
					tableWidget->setItem(row, column, item);
			}
		}
	};

	setTableWidget(ui.tableWidget);

	onResetControlTextLanguage();

	connect(ui.tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &PlayerInfoForm::onHeaderClicked);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	connect(&signalDispatcher, &SignalDispatcher::updatePlayerInfoColContents, this, &PlayerInfoForm::onUpdatePlayerInfoColContents);
	connect(&signalDispatcher, &SignalDispatcher::updatePlayerInfoStone, this, &PlayerInfoForm::onUpdatePlayerInfoStone);
}

PlayerInfoForm::~PlayerInfoForm()
{
}

void PlayerInfoForm::onResetControlTextLanguage()
{
	QStringList equipVHeaderList = {
		tr("name"), tr("freename"), "",
		tr("level"), tr("exp"), tr("nextexp"), tr("leftexp"), "",
		tr("hp"), tr("mp"), tr("chasma/loyal"),
		tr("atk"), tr("def"), tr("agi"), tr("luck")
	};

	//put on first col
	int rowCount = ui.tableWidget->rowCount();
	int size = equipVHeaderList.size();
	while (rowCount < size)
	{
		ui.tableWidget->insertRow(rowCount);
		++rowCount;
	}
	for (int row = 0; row < rowCount; ++row)
	{
		if (row >= size)
			break;
		QTableWidgetItem* item = ui.tableWidget->item(row, 0);
		if (item)
			item->setText(equipVHeaderList.at(row));
		else
		{
			item = new QTableWidgetItem(equipVHeaderList.at(row));
			if (item)
				ui.tableWidget->setItem(row, 0, item);
		}
	}
}

void PlayerInfoForm::onUpdatePlayerInfoColContents(int col, const QVariant& data)
{
	// 檢查是否為 QVariantList
	if (data.type() != QVariant::List)
		return;

	col += 1;

	// 取得 QVariantList
	QVariantList list = data.toList();

	// 取得 QVariantList 的大小
	const int size = list.size();

	// 獲取當前表格的總行數
	const int rowCount = ui.tableWidget->rowCount();
	if (col < 0 || col >= rowCount)
		return;

	// 開始填入內容
	for (int row = 0; row < rowCount; ++row)
	{
		// 設置內容
		if (row >= size)
		{
			break;
		}

		// 使用 QVariantList 中的數據
		const QVariant rowData = list.at(row);
		QString fillText = rowData.toString();


		// 檢查指針
		QTableWidgetItem* item = ui.tableWidget->item(row, col);

		if (!item)
		{
			// 如果指針為空，創建新的 QTableWidgetItem
			item = new QTableWidgetItem(fillText);
			item->setToolTip(fillText);
			ui.tableWidget->setItem(row, col, item);
		}
		else
		{
			// 如果指針不為空，則使用原有的 QTableWidgetItem
			item->setText(fillText);
			item->setToolTip(fillText);
		}
	}
}


void PlayerInfoForm::onUpdatePlayerInfoStone(int stone)
{
	ui.label->setText(tr("stone:%1").arg(stone));
}

void PlayerInfoForm::onHeaderClicked(int logicalIndex)
{
	qDebug() << "onHeaderClicked:" << logicalIndex;
	switch (logicalIndex)
	{
	case 1:
	{
		AbilityForm* abilityForm = new AbilityForm(this);
		abilityForm->setModal(true);
		QPoint mousePos = QCursor::pos();
		int x = mousePos.x() - 10;
		int y = mousePos.y() + 50;
		abilityForm->move(x, y);
		if (abilityForm->exec() == QDialog::Accepted)
		{
			//do nothing
		}
		break;
	}
	default:
		break;
	}
}

