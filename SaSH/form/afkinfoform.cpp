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

AfkInfoForm::AfkInfoForm(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	connect(ui.pushButton, &PushButton::clicked, this, &AfkInfoForm::onButtonClicked);

	onResetControlTextLanguage();

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateAfkInfoTable, this, &AfkInfoForm::onUpdateAfkInfoTable);

	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	ui.tableWidget->verticalHeader()->setDefaultSectionSize(11);

	util::setPushButton(ui.pushButton);

}

AfkInfoForm::~AfkInfoForm()
{
}

void AfkInfoForm::onButtonClicked()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.worker.isNull())
		return;

	injector.worker->loginTimer.restart();
	sa::PC pc = injector.worker->getPC();
	util::AfkRecorder recorder;
	recorder.levelrecord = pc.level;
	recorder.exprecord = pc.exp;
	recorder.goldearn = 0;
	recorder.deadthcount = 0;
	injector.worker->recorder[0] = recorder;

	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		sa::PET pet = injector.worker->getPet(i);
		recorder = {};
		recorder.levelrecord = pet.level;
		recorder.exprecord = pet.exp;
		recorder.deadthcount = 0;
		injector.worker->recorder[i + 1] = recorder;
	}
}

void AfkInfoForm::updateTableText(long long row, long long col, const QString& text)
{
	ui.tableWidget->setText(row, col, text);
}

void AfkInfoForm::onUpdateAfkInfoTable(long long row, const QString& text)
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

	for (long long i = 0; i < sa::MAX_PET; ++i)
	{
		sectionList.append(tr("pet %1 level difference").arg(i + 1));
		sectionList.append(tr("pet %1 exp difference").arg(i + 1));
		sectionList.append(tr("pet %1 deadth count").arg(i + 1));
		if (i <= sa::MAX_PET - 1)
			sectionList.append("");
	}


	long long rowCount = ui.tableWidget->rowCount();
	for (long long row = 0; row < rowCount; ++row)
	{
		if (row >= sectionList.size())
			break;
		updateTableText(row, 0, sectionList.value(row));
		ui.tableWidget->horizontalHeader()->resizeSection(row, 100);
	}
}