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
#include "afkinfoform.h"

#include "signaldispatcher.h"
#include "injector.h"

AfkInfoForm::AfkInfoForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	auto setTableWidget = [](QTableWidget* tableWidget)->void
	{
		//tablewidget set single selection
		tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
		//tablewidget set selection behavior
		tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
		//set auto resize to form size
		tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
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
		constexpr int max_row = 31;
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

	setTableWidget(ui.tableWidget);

	connect(ui.pushButton, &QPushButton::clicked, this, &AfkInfoForm::onButtonClicked);

	onResetControlTextLanguage();

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::updateAfkInfoTable, this, &AfkInfoForm::onUpdateAfkInfoTable);
}

AfkInfoForm::~AfkInfoForm()
{
}

void AfkInfoForm::onButtonClicked()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	injector.server->loginTimer.restart();
	PC pc = injector.server->getPC();
	util::AfkRecorder recorder;
	recorder.levelrecord = pc.level;
	recorder.exprecord = pc.exp;
	recorder.goldearn = 0;
	recorder.deadthcount = 0;
	injector.server->recorder[0] = recorder;

	for (int i = 0; i < MAX_PET; ++i)
	{
		PET pet = injector.server->pet[i];
		recorder = {};
		recorder.levelrecord = pet.level;
		recorder.exprecord = pet.exp;
		recorder.deadthcount = 0;
		injector.server->recorder[i + 1] = recorder;
	}
}

void AfkInfoForm::updateTableText(int row, int col, const QString& text)
{
	QTableWidgetItem* item = ui.tableWidget->item(row, col);
	if (item)
	{
		item->setText(text);
		item->setToolTip(text);
	}
	else
	{
		item = new QTableWidgetItem(text);
		if (item)
		{
			item->setToolTip(text);
			ui.tableWidget->setItem(row, col, item);
		}
	}
}

void AfkInfoForm::onUpdateAfkInfoTable(int row, const QString& text)
{
	updateTableText(row, 1, text);
}

void AfkInfoForm::onResetControlTextLanguage()
{
	QStringList sectionList = {
		tr("duration"),
		"",
		tr("level difference"),
		tr("exp difference"),
		tr("deadth count"),
		tr("gold difference"),
		"",
	};

	for (int i = 0; i < MAX_PET; ++i)
	{
		sectionList.append(tr("pet %1 level difference").arg(i + 1));
		sectionList.append(tr("pet %1 exp difference").arg(i + 1));
		sectionList.append(tr("pet %1 deadth count").arg(i + 1));
		if (i <= MAX_PET - 1)
			sectionList.append("");
	}


	int rowCount = ui.tableWidget->rowCount();
	for (int row = 0; row < rowCount; ++row)
	{
		if (row >= sectionList.size())
			break;
		updateTableText(row, 0, sectionList.at(row));
		ui.tableWidget->horizontalHeader()->resizeSection(row, 100);
	}
}