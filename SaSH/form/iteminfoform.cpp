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

#include "stdafx.h"
#include "iteminfoform.h"

#include "injector.h"
#include "signaldispatcher.h"

ItemInfoForm::ItemInfoForm(QWidget* parent)
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
		tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

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

	setTableWidget(ui.tableWidget_equip);
	setTableWidget(ui.tableWidget_item);

	onResetControlTextLanguage();

	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		util::SafeHash<int, QVariant> hashItem = injector.server->itemInfoRowContents;
		for (auto it = hashItem.begin(); it != hashItem.end(); ++it)
		{
			onUpdateItemInfoRowContents(it.key(), it.value());
		}

		util::SafeHash<int, QVariant> hashEquip = injector.server->equipInfoRowContents;
		for (auto it = hashEquip.begin(); it != hashEquip.end(); ++it)
		{
			onUpdateEquipInfoRowContents(it.key(), it.value());
		}
	}

	connect(ui.pushButton_refresh, &QPushButton::clicked, this, []()
		{
			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
			{
				for (int i = 0; i < 9; ++i)
					injector.server->sortItem(true);
			}
		});

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::updateEquipInfoRowContents, this, &ItemInfoForm::onUpdateEquipInfoRowContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateItemInfoRowContents, this, &ItemInfoForm::onUpdateItemInfoRowContents, Qt::UniqueConnection);


}

ItemInfoForm::~ItemInfoForm()
{

}

void ItemInfoForm::onResetControlTextLanguage()
{
	QStringList equipVHeaderList = {
		tr("head"), tr("body"), tr("righthand"), tr("leftacc"), tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"), tr("gloves")
	};

	//put on first col
	int rowCount = ui.tableWidget_equip->rowCount();
	int size = equipVHeaderList.size();
	while (rowCount < size)
	{
		ui.tableWidget_equip->insertRow(rowCount);
		++rowCount;
	}
	for (int row = 0; row < rowCount; ++row)
	{
		if (row >= size)
			break;
		QTableWidgetItem* item = ui.tableWidget_equip->item(row, 0);
		if (item)
			item->setText(equipVHeaderList.at(row));
		else
		{
			item = new QTableWidgetItem(equipVHeaderList.at(row));
			if (item)
				ui.tableWidget_equip->setItem(row, 0, item);
		}
	}

	rowCount = ui.tableWidget_item->rowCount();
	size = 15;
	while (rowCount < size)
	{
		ui.tableWidget_item->insertRow(rowCount);
		++rowCount;
	}
	for (int row = 0; row < rowCount; ++row)
	{
		QTableWidgetItem* item = ui.tableWidget_item->item(row, 0);
		if (item)
			item->setText(QString::number(row + 1));
		else
		{
			item = new QTableWidgetItem(QString::number(row + 1));
			if (item)
				ui.tableWidget_item->setItem(row, 0, item);
		}
	}
}

void ItemInfoForm::updateItemInfoRowContents(QTableWidget* tableWidget, int row, const QVariant& data)
{

	// 檢查是否為 QVariantList
	if (data.type() != QVariant::List)
		return;

	// 取得 QVariantList
	QVariantList list = data.toList();

	// 取得 QVariantList 的大小
	const int size = list.size();

	// 獲取當前表格的總行數
	const int colCount = tableWidget->rowCount();
	if (row < 0 || row >= colCount)
		return;

	// 開始填入內容
	for (int col = 0; col < colCount; ++col)
	{
		// 設置內容
		if (col >= size)
		{
			break;
		}

		// 使用 QVariantList 中的數據
		const QVariant rowData = list.at(col);
		QString fillText = rowData.toString();

		// 檢查指針
		QTableWidgetItem* item = tableWidget->item(row, col);

		if (!item)
		{
			// 如果指針為空，創建新的 QTableWidgetItem
			item = new QTableWidgetItem(fillText);
			item->setToolTip(fillText);
			tableWidget->setItem(row, col, item);
		}
		else
		{
			// 如果指針不為空，則使用原有的 QTableWidgetItem
			item->setText(fillText);
			item->setToolTip(fillText);
		}
	}
}

void ItemInfoForm::onUpdateEquipInfoRowContents(int row, const QVariant& data)
{
	updateItemInfoRowContents(ui.tableWidget_equip, row, data);
}

void ItemInfoForm::onUpdateItemInfoRowContents(int row, const QVariant& data)
{
	updateItemInfoRowContents(ui.tableWidget_item, row - 9, data);
}

void ItemInfoForm::on_tableWidget_item_cellDoubleClicked(int row, int column)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	injector.server->useItem(row + CHAR_EQUIPPLACENUM, 0);
}

void ItemInfoForm::on_tableWidget_equip_cellDoubleClicked(int row, int column)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	int spotIndex = injector.server->getItemEmptySpotIndex();
	if (spotIndex == -1)
		return;

	injector.server->swapItem(row, spotIndex);
}