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

AfkInfoForm::AfkInfoForm(qint64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	connect(ui.pushButton, &PushButton::clicked, this, &AfkInfoForm::onButtonClicked);

	onResetControlTextLanguage();

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateAfkInfoTable, this, &AfkInfoForm::onUpdateAfkInfoTable);
}

AfkInfoForm::~AfkInfoForm()
{
}

void AfkInfoForm::onButtonClicked()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
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

	for (qint64 i = 0; i < MAX_PET; ++i)
	{
		PET pet = injector.server->getPet(i);
		recorder = {};
		recorder.levelrecord = pet.level;
		recorder.exprecord = pet.exp;
		recorder.deadthcount = 0;
		injector.server->recorder[i + 1] = recorder;
	}
}

void AfkInfoForm::updateTableText(qint64 row, qint64 col, const QString& text)
{
	ui.tableWidget->setText(row, col, text);
}

void AfkInfoForm::onUpdateAfkInfoTable(qint64 row, const QString& text)
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

	for (qint64 i = 0; i < MAX_PET; ++i)
	{
		sectionList.append(tr("pet %1 level difference").arg(i + 1));
		sectionList.append(tr("pet %1 exp difference").arg(i + 1));
		sectionList.append(tr("pet %1 deadth count").arg(i + 1));
		if (i <= MAX_PET - 1)
			sectionList.append("");
	}


	qint64 rowCount = ui.tableWidget->rowCount();
	for (qint64 row = 0; row < rowCount; ++row)
	{
		if (row >= sectionList.size())
			break;
		updateTableText(row, 0, sectionList.value(row));
		ui.tableWidget->horizontalHeader()->resizeSection(row, 100);
	}
}