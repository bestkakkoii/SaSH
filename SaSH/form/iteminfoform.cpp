﻿/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

#include <gamedevice.h>
#include "signaldispatcher.h"

ItemInfoForm::ItemInfoForm(long long index, QWidget* parent)
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
	ui.tableWidget_equip->horizontalHeader()->setStretchLastSection(false);
	ui.tableWidget_item->horizontalHeader()->setStretchLastSection(false);
	ui.tableWidget_equip->verticalHeader()->setStretchLastSection(false);
	ui.tableWidget_item->verticalHeader()->setStretchLastSection(false);

	util::setPushButton(ui.pushButton_refresh);

	onResetControlTextLanguage();

	GameDevice& gamedevice = GameDevice::getInstance(index);
	if (!gamedevice.worker.isNull())
	{
		QHash<long long, QVariant> hashItem = gamedevice.worker->itemInfoRowContents.toHash();
		for (auto it = hashItem.begin(); it != hashItem.end(); ++it)
		{
			onUpdateItemInfoRowContents(it.key(), it.value());
		}

		QHash<long long, QVariant> hashEquip = gamedevice.worker->equipInfoRowContents.toHash();
		for (auto it = hashEquip.begin(); it != hashEquip.end(); ++it)
		{
			onUpdateEquipInfoRowContents(it.key(), it.value());
		}
	}

	connect(ui.pushButton_refresh, &PushButton::clicked, this, [index]()
		{
			GameDevice& gamedevice = GameDevice::getInstance(index);
			if (!gamedevice.worker.isNull())
			{
				for (long long i = 0; i < 4; ++i)
					gamedevice.worker->sortItem();
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
	long long size = equipVHeaderList.size();

	for (long long row = 0; row < size; ++row)
	{
		if (row >= size)
			break;
		ui.tableWidget_equip->setText(row, 0, equipVHeaderList.value(row), equipVHeaderList.value(row));
	}

	size = sa::MAX_ITEM - sa::CHAR_EQUIPSLOT_COUNT;
	for (long long row = 0; row < size; ++row)
	{
		ui.tableWidget_item->setText(row, 0, util::toQString(row + 1), util::toQString(row + 1));
	}
}

void ItemInfoForm::updateItemInfoRowContents(TableWidget* tableWidget, long long row, const QVariant& vdata)
{

	// 檢查是否為 QVariantList
	if (vdata.type() != QVariant::List)
		return;

	// 取得 QVariantList
	QVariantList list = vdata.toList();

	// 取得 QVariantList 的大小
	const long long size = list.size();

	// 獲取當前表格的總行數
	const long long colCount = tableWidget->rowCount();
	if (row < 0 || row >= colCount)
		return;

	// 開始填入內容
	for (long long col = 0; col < colCount; ++col)
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

void ItemInfoForm::onUpdateEquipInfoRowContents(long long row, const QVariant& vdata)
{
	updateItemInfoRowContents(ui.tableWidget_equip, row, vdata);
}

void ItemInfoForm::onUpdateItemInfoRowContents(long long row, const QVariant& vdata)
{
	updateItemInfoRowContents(ui.tableWidget_item, row - 9, vdata);
}

void ItemInfoForm::on_tableWidget_item_cellDoubleClicked(int row, int column)
{
	std::ignore = column;
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return;

	gamedevice.worker->useItem(static_cast<long long>(row) + sa::CHAR_EQUIPSLOT_COUNT, 0);
}

void ItemInfoForm::on_tableWidget_equip_cellDoubleClicked(int row, int column)
{
	std::ignore = column;
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return;

	long long spotIndex = gamedevice.worker->getItemEmptySpotIndex();
	if (spotIndex == -1)
		return;

	gamedevice.worker->swapItem(row, spotIndex);
}