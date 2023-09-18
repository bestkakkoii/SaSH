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

//constexpr int max_col = 2;
constexpr int max_row = 10;

BattleInfoForm::BattleInfoForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	ui.label_time->setFlag(Qt::AlignCenter | Qt::AlignVCenter);

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

		for (int row = 0; row < max_row; ++row)
		{
			tableWidget->insertRow(row);
		}

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


	setTableWidget(ui.tableWidget_top);
	setTableWidget(ui.tableWidget_bottom);

	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		QVariant topInfoContents = injector.server->topInfoContents;
		QVariant bottomInfoContents = injector.server->bottomInfoContents;
		QString timeLabelContents = injector.server->timeLabelContents;
		QString labelPlayerAction = injector.server->labelPlayerAction;
		QString labelPetAction = injector.server->labelPetAction;

		onUpdateTopInfoContents(topInfoContents);
		onUpdateBottomInfoContents(bottomInfoContents);
		onUpdateTimeLabelContents(timeLabelContents);
		onUpdateLabelPlayerAction(labelPlayerAction);
		onUpdateLabelPetAction(labelPetAction);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::updateTopInfoContents, this, &BattleInfoForm::onUpdateTopInfoContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateBottomInfoContents, this, &BattleInfoForm::onUpdateBottomInfoContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateTimeLabelContents, this, &BattleInfoForm::onUpdateTimeLabelContents, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateLabelPlayerAction, this, &BattleInfoForm::onUpdateLabelPlayerAction, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateLabelPetAction, this, &BattleInfoForm::onUpdateLabelPetAction, Qt::UniqueConnection);
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

void BattleInfoForm::onUpdateLabelPlayerAction(const QString& text)
{
	ui.label_charaction->setText(text);
}

void BattleInfoForm::onUpdateLabelPetAction(const QString& text)
{
	ui.label_petaction->setText(text);
}

void BattleInfoForm::updateItemInfoRowContents(QTableWidget* tableWidget, const QVariant& dat)
{
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

	// 檢查是否為 QVector<QStringList>
	if (dat.type() != QVariant::Type::UserType)
		return;

	QVector<QStringList> list = dat.value<QVector<QStringList>>();

	if (list.isEmpty())
		return;

	tableWidget->clearContents();

	auto setText = [tableWidget](int row, int col, const QString& text)->void
	{
		// 檢查指針
		QTableWidgetItem* item = tableWidget->item(row, col);

		if (!item)
		{
			// 如果指針為空，創建新的 QTableWidgetItem
			item = new QTableWidgetItem(text);
			item->setToolTip(text);
			tableWidget->setItem(row, col, item);
		}
		else
		{
			// 如果指針不為空，則使用原有的 QTableWidgetItem
			item->setText(text);
			item->setToolTip(text);
		}
	};

	static const QHash<int, QPair<int, int>> fill_hash = {
		{ 4, QPair<int, int>{ 0, 0 } },
		{ 9, QPair<int, int>{ 1, 0 } },
		{ 2, QPair<int, int>{ 2, 0 } },
		{ 7, QPair<int, int>{ 3, 0 } },
		{ 0, QPair<int, int>{ 4, 0 } },
		{ 5, QPair<int, int>{ 5, 0 } },
		{ 1, QPair<int, int>{ 6, 0 } },
		{ 6, QPair<int, int>{ 7, 0 } },
		{ 3, QPair<int, int>{ 8, 0 } },
		{ 8, QPair<int, int>{ 9, 0 } },

		{ 14, QPair<int, int>{ 0, 0 } },
		{ 19, QPair<int, int>{ 1, 0 } },
		{ 12, QPair<int, int>{ 2, 0 } },
		{ 17, QPair<int, int>{ 3, 0 } },
		{ 10, QPair<int, int>{ 4, 0 } },
		{ 15, QPair<int, int>{ 5, 0 } },
		{ 11, QPair<int, int>{ 6, 0 } },
		{ 16, QPair<int, int>{ 7, 0 } },
		{ 13, QPair<int, int>{ 8, 0 } },
		{ 18, QPair<int, int>{ 9, 0 } },
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

		int pos = l.at(0).toInt(&ok);
		if (!ok)
		{
			continue;
		}

		QString text = l.at(1);
		const QString ride = l.at(2);

		const QPair<int, int> fill = fill_hash.value(pos, QPair<int, int>{ -1, -1 });
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