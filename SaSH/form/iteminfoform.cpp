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

ItemInfoForm::ItemInfoForm(qint64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	//register meta type of QVariant
	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	auto setTableWidget = [](QTableWidget* tableWidget)->void
	{
		qint64 rowCount = tableWidget->rowCount();
		qint64 columnCount = tableWidget->columnCount();
		for (qint64 row = 0; row < rowCount; ++row)
		{
			for (qint64 column = 0; column < columnCount; ++column)
			{
				QTableWidgetItem* item = new QTableWidgetItem("");
				if (item == nullptr)
					continue;

				tableWidget->setItem(row, column, item);
			}
		}
	};

	setTableWidget(ui.tableWidget_equip);
	setTableWidget(ui.tableWidget_item);

	onResetControlTextLanguage();

	Injector& injector = Injector::getInstance(index);
	if (!injector.server.isNull())
	{
		QHash<qint64, QVariant> hashItem = injector.server->itemInfoRowContents.toHash();
		for (auto it = hashItem.begin(); it != hashItem.end(); ++it)
		{
			onUpdateItemInfoRowContents(it.key(), it.value());
		}

		QHash<qint64, QVariant> hashEquip = injector.server->equipInfoRowContents.toHash();
		for (auto it = hashEquip.begin(); it != hashEquip.end(); ++it)
		{
			onUpdateEquipInfoRowContents(it.key(), it.value());
		}
	}

	connect(ui.pushButton_refresh, &PushButton::clicked, this, [index]()
		{
			Injector& injector = Injector::getInstance(index);
			if (!injector.server.isNull())
			{
				for (qint64 i = 0; i < 4; ++i)
					injector.server->sortItem(true);
			}
		});

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
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
	qint64 rowCount = ui.tableWidget_equip->rowCount();
	qint64 size = equipVHeaderList.size();
	while (rowCount < size)
	{
		ui.tableWidget_equip->insertRow(rowCount);
		++rowCount;
	}
	for (qint64 row = 0; row < rowCount; ++row)
	{
		if (row >= size)
			break;
		QTableWidgetItem* item = ui.tableWidget_equip->item(row, 0);
		if (item != nullptr)
			item->setText(equipVHeaderList.at(row));
		else
		{
			item = new QTableWidgetItem(equipVHeaderList.at(row));
			if (item == nullptr)
				continue;

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
	for (qint64 row = 0; row < rowCount; ++row)
	{
		QTableWidgetItem* item = ui.tableWidget_item->item(row, 0);
		if (item != nullptr)
			item->setText(util::toQString(row + 1));
		else
		{
			item = new QTableWidgetItem(util::toQString(row + 1));
			if (item == nullptr)
				continue;

			ui.tableWidget_item->setItem(row, 0, item);
		}
	}
}

void ItemInfoForm::updateItemInfoRowContents(QTableWidget* tableWidget, qint64 row, const QVariant& data)
{

	// 檢查是否為 QVariantList
	if (data.type() != QVariant::List)
		return;

	// 取得 QVariantList
	QVariantList list = data.toList();

	// 取得 QVariantList 的大小
	const qint64 size = list.size();

	// 獲取當前表格的總行數
	const qint64 colCount = tableWidget->rowCount();
	if (row < 0 || row >= colCount)
		return;

	// 開始填入內容
	for (qint64 col = 0; col < colCount; ++col)
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

		if (item != nullptr)
		{
			// 如果指針不為空，則使用原有的 QTableWidgetItem
			item->setText(fillText);
			item->setToolTip(fillText);
		}
		else
		{
			// 如果指針為空，創建新的 QTableWidgetItem
			item = new QTableWidgetItem(fillText);
			if (item == nullptr)
				continue;

			item->setToolTip(fillText);
			tableWidget->setItem(row, col, item);
		}
	}
}

void ItemInfoForm::onUpdateEquipInfoRowContents(qint64 row, const QVariant& data)
{
	updateItemInfoRowContents(ui.tableWidget_equip, row, data);
}

void ItemInfoForm::onUpdateItemInfoRowContents(qint64 row, const QVariant& data)
{
	updateItemInfoRowContents(ui.tableWidget_item, row - 9, data);
}

void ItemInfoForm::on_tableWidget_item_cellDoubleClicked(int row, int column)
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return;

	injector.server->useItem(row + CHAR_EQUIPPLACENUM, 0);
}

void ItemInfoForm::on_tableWidget_equip_cellDoubleClicked(int row, int column)
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return;

	qint64 spotIndex = injector.server->getItemEmptySpotIndex();
	if (spotIndex == -1)
		return;

	injector.server->swapItem(row, spotIndex);
}