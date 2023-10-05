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

CharInfoForm::CharInfoForm(qint64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	//register meta type of QVariant
	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	onResetControlTextLanguage();

	connect(ui.tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &CharInfoForm::onHeaderClicked);

	Injector& injector = Injector::getInstance(index);
	if (!injector.server.isNull())
	{
		QHash<qint64, QVariant> playerInfoColContents = injector.server->playerInfoColContents.toHash();
		for (auto it = playerInfoColContents.begin(); it != playerInfoColContents.end(); ++it)
		{
			onUpdateCharInfoColContents(it.key(), it.value());
		}

		qint64 stone = injector.server->getPC().gold;
		onUpdateCharInfoStone(stone);

		for (qint64 i = 0; i < MAX_PET; ++i)
		{
			PET pet = injector.server->getPet(i);
			onUpdateCharInfoPetState(i, pet.state);
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateCharInfoColContents, this, &CharInfoForm::onUpdateCharInfoColContents);
	connect(&signalDispatcher, &SignalDispatcher::updateCharInfoStone, this, &CharInfoForm::onUpdateCharInfoStone);
	connect(&signalDispatcher, &SignalDispatcher::updateCharInfoPetState, this, &CharInfoForm::onUpdateCharInfoPetState);
}

CharInfoForm::~CharInfoForm()
{
}

void CharInfoForm::onResetControlTextLanguage()
{
	QStringList equipVHeaderList = {
		tr("name"), tr("freename"),
		tr("level"), tr("exp"), tr("nextexp"), tr("leftexp"),
		tr("hp") , tr("mp"), tr("chasma/loyal"),
		tr("atk") + "/" + tr("def") + "/" + tr("agi"), tr("growth"), tr("power"),
	};

	qint64 size = equipVHeaderList.size();
	for (qint64 row = 0; row < size; ++row)
	{
		ui.tableWidget->setText(row, 0, equipVHeaderList.value(row));
	}
}

void CharInfoForm::onUpdateCharInfoColContents(qint64 col, const QVariant& data)
{
	// 檢查是否為 QVariantList
	if (data.type() != QVariant::List)
		return;

	col += 1;

	// 取得 QVariantList
	QVariantList list = data.toList();

	// 取得 QVariantList 的大小
	const qint64 size = list.size();

	// 獲取當前表格的總行數
	const qint64 rowCount = ui.tableWidget->rowCount();
	if (col < 0 || col >= rowCount)
		return;

	// 開始填入內容
	for (qint64 row = 0; row < rowCount; ++row)
	{
		// 設置內容
		if (row >= size)
		{
			break;
		}

		// 使用 QVariantList 中的數據
		const QVariant rowData = list.value(row);
		QString fillText = rowData.toString();

		ui.tableWidget->setText(row, col, fillText);
	}
}


void CharInfoForm::onUpdateCharInfoStone(qint64 stone)
{
	ui.label->setText(tr("stone:%1").arg(stone));
}

void CharInfoForm::onUpdateCharInfoPetState(qint64 petIndex, qint64 state)
{
	qint64 col = petIndex + 1;
	if (col < 0 || col >= ui.tableWidget->columnCount())
		return;

	const QStringList stateStrList = {
		tr("battle"), tr("standby"), tr("mail"), tr("rest"), tr("ride")
	};

	--state;
	if (state < 0 || state >= stateStrList.size())
		return;

	//設置指定 col = petIndex + 1的 header
	QString petName = QString(tr("pet%1 (%2)")).arg(petIndex + 1).arg(stateStrList.value(state));

	//get item 
	ui.tableWidget->setHorizontalHeaderText(col + 1, petName);
}

void CharInfoForm::onHeaderClicked(qint64 logicalIndex)
{
	qint64 currentIndex = getIndex();
	switch (logicalIndex)
	{
	case 1:
	{
		AbilityForm* abilityForm = new AbilityForm(currentIndex, this);
		abilityForm->setModal(true);
		QPoint mousePos = QCursor::pos();
		qint64 x = static_cast<qint64>(mousePos.x()) - 10;
		qint64 y = static_cast<qint64>(mousePos.y()) + 50;
		abilityForm->move(x, y);
		if (abilityForm->exec() == QDialog::Accepted)
		{
			//do nothing
		}
		break;
	}
	default:
	{
		qint64 petIndex = logicalIndex - 2;
		if (petIndex < 0 || petIndex >= MAX_PET)
			break;
		Injector& injector = Injector::getInstance(currentIndex);
		if (injector.server.isNull())
			break;

		PET pet = injector.server->getPet(petIndex);
		switch (pet.state)
		{
		case PetState::kRest:
		{

			injector.server->setPetState(petIndex, PetState::kStandby);
			break;
		}
		case PetState::kStandby:
		{
			injector.server->setPetState(petIndex, PetState::kBattle);
			break;
		}
		case PetState::kBattle:
		{
			injector.server->setPetState(petIndex, PetState::kMail);
			break;
		}
		case PetState::kMail:
		{

			if (injector.server->getPet(petIndex).loyal == 100)
				injector.server->setPetState(petIndex, PetState::kRide);
			else
				injector.server->setPetState(petIndex, PetState::kRest);
			break;
		}
		case PetState::kRide:
		{
			injector.server->setPetState(petIndex, PetState::kRest);
			break;
		}
		}
	}
	}
}

