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
#include "playerinfoform.h"
#include "abilityform.h"

#include "signaldispatcher.h"
#include "injector.h"

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
		tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

		//tableWidget->setStyleSheet(R"(
		//QTableWidget { font-size:11px; } 
		//	QTableView::item:selected { background-color: black; color: white;
		//})");
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

	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
	{
		util::SafeHash<int, QVariant> playerInfoColContents = injector.server->playerInfoColContents;
		for (auto it = playerInfoColContents.begin(); it != playerInfoColContents.end(); ++it)
		{
			onUpdatePlayerInfoColContents(it.key(), it.value());
		}

		int stone = injector.server->getPC().gold;
		onUpdatePlayerInfoStone(stone);

		for (int i = 0; i < MAX_PET; ++i)
		{
			PET pet = injector.server->getPet(i);
			onUpdatePlayerInfoPetState(i, pet.state);
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::updatePlayerInfoColContents, this, &PlayerInfoForm::onUpdatePlayerInfoColContents);
	connect(&signalDispatcher, &SignalDispatcher::updatePlayerInfoStone, this, &PlayerInfoForm::onUpdatePlayerInfoStone);
	connect(&signalDispatcher, &SignalDispatcher::updatePlayerInfoPetState, this, &PlayerInfoForm::onUpdatePlayerInfoPetState);
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
		tr("atk"), tr("def"), tr("agi"), tr("luck"), tr("growth"), tr("power"),
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

void PlayerInfoForm::onUpdatePlayerInfoPetState(int petIndex, int state)
{
	int col = petIndex + 1;
	if (col < 0 || col >= ui.tableWidget->columnCount())
		return;

	const QStringList stateStrList = {
		tr("battle"), tr("standby"), tr("mail"), tr("rest"), tr("ride")
	};

	--state;
	if (state < 0 || state >= stateStrList.size())
		return;

	//設置指定 col = petIndex + 1的 header
	QString petName = QString(tr("pet%1 (%2)")).arg(petIndex + 1).arg(stateStrList.at(state));

	//get item 
	QTableWidgetItem* item = ui.tableWidget->horizontalHeaderItem(col + 1);
	if (item)
	{
		item->setText(petName);
	}
	else
	{
		item = new QTableWidgetItem(petName);
		if (item)
		{
			ui.tableWidget->setHorizontalHeaderItem(col, item);
		}
	}
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
	{
		int petIndex = logicalIndex - 2;
		if (petIndex < 0 || petIndex >= MAX_PET)
			break;
		Injector& injector = Injector::getInstance();
		if (injector.server.isNull())
			break;

		PET pet = injector.server->getPet(petIndex);
		switch (pet.state)
		{
		case PetState::kBattle:
			injector.server->setPetState(petIndex, PetState::kStandby);
			break;
		case PetState::kStandby:
			injector.server->setPetState(petIndex, PetState::kMail);
			break;
		case PetState::kMail:
			injector.server->setPetState(petIndex, PetState::kRest);
			break;
		case PetState::kRest:
		{
			if (injector.server->getPet(petIndex).loyal == 100)
				injector.server->setPetState(petIndex, PetState::kRide);
			else
				injector.server->setPetState(petIndex, PetState::kBattle);
			break;
		}
		case PetState::kRide:
			injector.server->setPetState(petIndex, PetState::kRest);
			injector.server->setPetState(petIndex, PetState::kBattle);
			break;
		}
		break;
	}
	}
}

