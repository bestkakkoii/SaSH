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
#include "battleinfoform.h"

#include "signaldispatcher.h"
#include "injector.h"

//constexpr qint64 max_col = 2;
constexpr qint64 max_row = 10;

BattleInfoForm::BattleInfoForm(qint64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	ui.label_time->setFlag(Qt::AlignCenter | Qt::AlignVCenter);

	ui.tableWidget_top->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget_bottom->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget_top->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget_bottom->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	ui.tableWidget_top->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget_bottom->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget_bottom->verticalHeader()->setStretchLastSection(true);
	ui.tableWidget_top->verticalHeader()->setStretchLastSection(true);

	Injector& injector = Injector::getInstance(index);
	if (!injector.server.isNull())
	{
		QVariant topInfoContents = injector.server->topInfoContents.get();
		QVariant bottomInfoContents = injector.server->bottomInfoContents.get();
		QString timeLabelContents = injector.server->timeLabelContents.get();
		QString labelCharAction = injector.server->labelCharAction.get();
		QString labelPetAction = injector.server->labelPetAction.get();

		onUpdateTopInfoContents(topInfoContents);
		onUpdateBottomInfoContents(bottomInfoContents);
		onUpdateTimeLabelContents(timeLabelContents);
		onUpdateLabelCharAction(labelCharAction);
		onUpdateLabelPetAction(labelPetAction);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateTopInfoContents, this, &BattleInfoForm::onUpdateTopInfoContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateBottomInfoContents, this, &BattleInfoForm::onUpdateBottomInfoContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateTimeLabelContents, this, &BattleInfoForm::onUpdateTimeLabelContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateLabelCharAction, this, &BattleInfoForm::onUpdateLabelCharAction, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateLabelPetAction, this, &BattleInfoForm::onUpdateLabelPetAction, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::notifyBattleActionState, this, &BattleInfoForm::onNotifyBattleActionState, Qt::UniqueConnection);
}

BattleInfoForm::~BattleInfoForm()
{
}

void BattleInfoForm::onUpdateTopInfoContents(const QVariant& dat)
{
	updateItemInfoRowContents(ui.tableWidget_top, dat);
}

void BattleInfoForm::onUpdateBottomInfoContents(const QVariant& dat)
{
	updateItemInfoRowContents(ui.tableWidget_bottom, dat);
}

void BattleInfoForm::onUpdateTimeLabelContents(const QString& text)
{
	ui.label_time->setText(text);
}

void BattleInfoForm::onUpdateLabelCharAction(const QString& text)
{
	ui.label_charaction->setText(text);
}

void BattleInfoForm::onUpdateLabelPetAction(const QString& text)
{
	ui.label_petaction->setText(text);
}

//13 11 10 12 14 back
//18 16 15 17 19 front     
//8 6 5 7 9      front
//3 1 0 2 4      back

//top form 1357行 為人物+騎寵(back) 2468為戰寵(front)
//固定2列10行，戰寵的字符串前方需要補上4格空格
/*
col 列1                    列2
	14(人物)           14(騎寵)
	   19(戰寵)
	12(人物)           12(騎寵)
	   17(戰寵)
	10(人物)           10(騎寵)
	   15(戰寵)
	11(人物)           11(騎寵)
	   16(戰寵)
	13(人物)           13(騎寵)
	   18(戰寵)
row
*/

//bottom form 1357行 為人物+騎寵(back) 2468為戰寵(front)
//固定2列10行，戰寵的字符串前方需要補上4格空格
/*
col 列1                     列2
	4(人物)              4(騎寵)
	   9(戰寵)
	2(人物)              2(騎寵)
	   7(戰寵)
	0(人物)              0(騎寵)
	   5(戰寵)
	1(人物)              1(騎寵)
	   6(戰寵)
	3(人物)              3(騎寵)
	   8(戰寵)
row
*/

static const QHash<qint64, QPair<qint64, qint64>> fill_hash = {
	{ 4, QPair<qint64, qint64>{ 0, 0 } },
	{ 9, QPair<qint64, qint64>{ 1, 0 } },
	{ 2, QPair<qint64, qint64>{ 2, 0 } },
	{ 7, QPair<qint64, qint64>{ 3, 0 } },
	{ 0, QPair<qint64, qint64>{ 4, 0 } },
	{ 5, QPair<qint64, qint64>{ 5, 0 } },
	{ 1, QPair<qint64, qint64>{ 6, 0 } },
	{ 6, QPair<qint64, qint64>{ 7, 0 } },
	{ 3, QPair<qint64, qint64>{ 8, 0 } },
	{ 8, QPair<qint64, qint64>{ 9, 0 } },

	{ 14, QPair<qint64, qint64>{ 0, 0 } },
	{ 19, QPair<qint64, qint64>{ 1, 0 } },
	{ 12, QPair<qint64, qint64>{ 2, 0 } },
	{ 17, QPair<qint64, qint64>{ 3, 0 } },
	{ 10, QPair<qint64, qint64>{ 4, 0 } },
	{ 15, QPair<qint64, qint64>{ 5, 0 } },
	{ 11, QPair<qint64, qint64>{ 6, 0 } },
	{ 16, QPair<qint64, qint64>{ 7, 0 } },
	{ 13, QPair<qint64, qint64>{ 8, 0 } },
	{ 18, QPair<qint64, qint64>{ 9, 0 } },
};

void BattleInfoForm::onNotifyBattleActionState(qint64 index, bool left)
{
	if (!fill_hash.contains(index))
		return;

	const QPair<qint64, qint64> pair = fill_hash.value(index);

	QTableWidgetItem* item = nullptr;
	if (left)
	{
		item = ui.tableWidget_top->item(pair.first, pair.second);
	}
	else
	{
		item = ui.tableWidget_bottom->item(pair.first, pair.second);
	}

	if (item == nullptr)
		return;

	QString text = item->text().mid(1);

	//找到非空格的的第一個字符 前方插入 "*"
	for (qint64 i = 0; i < text.size(); ++i)
	{
		if (text.at(i) != ' ')
		{
			text.insert(i, "*");
			break;
		}
	}

	item->setText(text);
	item->setToolTip(text);
}

void BattleInfoForm::updateItemInfoRowContents(QTableWidget* tableWidget, const QVariant& dat)
{
	if (tableWidget == nullptr)
		return;

	// 檢查是否為 QVector<QStringList>
	if (dat.type() != QVariant::Type::UserType)
		return;

	QVector<QStringList> list = dat.value<QVector<QStringList>>();

	if (list.isEmpty())
		return;

	tableWidget->clearContents();

	auto setText = [tableWidget](qint64 row, qint64 col, const QString& text)->void
	{
		// 檢查指針
		QTableWidgetItem* item = tableWidget->item(row, col);
		if (row >= tableWidget->rowCount())
		{
			for (qint64 i = tableWidget->rowCount(); i <= row; ++i)
				tableWidget->insertRow(i);
		}

		if (item != nullptr)
		{
			// 如果指針不為空，則使用原有的 QTableWidgetItem
			item->setText(text);
			item->setToolTip(text);
		}
		else
		{
			// 如果指針為空，創建新的 QTableWidgetItem
			item = new QTableWidgetItem(text);
			if (item == nullptr)
				return;

			item->setToolTip(text);
			tableWidget->setItem(row, col, item);
		}
	};

	const QString objectName = tableWidget->objectName();
	//const bool isTop = objectName.contains("top", Qt::CaseInsensitive);

	bool ok = false;
	for (const QStringList& l : list)
	{
		if (l.size() != 3)
		{
			continue;
		}

		qint64 pos = l.value(0).toLongLong(&ok);
		if (!ok)
		{
			continue;
		}

		QString text = l.value(1);
		const QString ride = l.value(2);

		const QPair<qint64, qint64> fill = fill_hash.value(pos, qMakePair(-1, -1));
		if (fill.first == -1)
		{
			continue;
		}


		if (fill.first % 2)
		{
			setText(fill.first, fill.second, "    " + text);
		}
		else
			setText(fill.first, fill.second, text);

		setText(fill.first, fill.second + 1, ride);
	}
}