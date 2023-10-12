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
import String;
#include "stdafx.h"
#include "iteminfoform.h"

#include "injector.h"
#include "signaldispatcher.h"

ItemInfoForm::ItemInfoForm(__int64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	//register meta type of QVariant
	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	ui.tableWidget_equip->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.tableWidget_item->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.tableWidget_equip->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget_item->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget_equip->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget_item->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget_equip->verticalHeader()->setStretchLastSection(true);
	ui.tableWidget_item->verticalHeader()->setStretchLastSection(true);

	onResetControlTextLanguage();

	Injector& injector = Injector::getInstance(index);
	if (!injector.server.isNull())
	{
		QHash<__int64, QVariant> hashItem = injector.server->itemInfoRowContents.toHash();
		for (auto it = hashItem.begin(); it != hashItem.end(); ++it)
		{
			onUpdateItemInfoRowContents(it.key(), it.value());
		}

		QHash<__int64, QVariant> hashEquip = injector.server->equipInfoRowContents.toHash();
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
				for (__int64 i = 0; i < 4; ++i)
					injector.server->sortItem(true);
			}
		});

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateEquipInfoRowContents, this, &ItemInfoForm::onUpdateEquipInfoRowContents, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateItemInfoRowContents, this, &ItemInfoForm::onUpdateItemInfoRowContents, Qt::QueuedConnection);
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
	__int64 size = equipVHeaderList.size();

	for (__int64 row = 0; row < size; ++row)
	{
		if (row >= size)
			break;
		ui.tableWidget_equip->setText(row, 0, equipVHeaderList.value(row), equipVHeaderList.value(row));
	}

	size = MAX_ITEM - CHAR_EQUIPPLACENUM;
	for (__int64 row = 0; row < size; ++row)
	{
		ui.tableWidget_item->setText(row, 0, toQString(row + 1), toQString(row + 1));
	}
}

void ItemInfoForm::updateItemInfoRowContents(TableWidget* tableWidget, __int64 row, const QVariant& data)
{

	// 檢查是否為 QVariantList
	if (data.type() != QVariant::List)
		return;

	// 取得 QVariantList
	QVariantList list = data.toList();

	// 取得 QVariantList 的大小
	const __int64 size = list.size();

	// 獲取當前表格的總行數
	const __int64 colCount = tableWidget->rowCount();
	if (row < 0 || row >= colCount)
		return;

	// 開始填入內容
	for (__int64 col = 0; col < colCount; ++col)
	{
		// 設置內容
		if (col >= size)
		{
			break;
		}

		// 使用 QVariantList 中的數據
		const QVariant rowData = list.value(col);
		QString fillText = rowData.toString();

		tableWidget->setText(row, col, fillText, fillText);
	}
}

void ItemInfoForm::onUpdateEquipInfoRowContents(__int64 row, const QVariant& data)
{
	updateItemInfoRowContents(ui.tableWidget_equip, row, data);
}

void ItemInfoForm::onUpdateItemInfoRowContents(__int64 row, const QVariant& data)
{
	updateItemInfoRowContents(ui.tableWidget_item, row - 9, data);
}

void ItemInfoForm::on_tableWidget_item_cellDoubleClicked(int row, int column)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return;

	injector.server->useItem(static_cast<__int64>(row) + CHAR_EQUIPPLACENUM, 0);
}

void ItemInfoForm::on_tableWidget_equip_cellDoubleClicked(int row, int column)
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return;

	__int64 spotIndex = injector.server->getItemEmptySpotIndex();
	if (spotIndex == -1)
		return;

	injector.server->swapItem(row, spotIndex);
}