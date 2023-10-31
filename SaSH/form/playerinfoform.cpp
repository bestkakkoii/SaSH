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


#include "signaldispatcher.h"
#include "injector.h"

CharInfoForm::CharInfoForm(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
	, abilityForm_(index, nullptr)
{
	ui.setupUi(this);

	//register meta type of QVariant
	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	abilityForm_.hide();

	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	onResetControlTextLanguage();

	connect(ui.tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &CharInfoForm::onHeaderClicked);

	Injector& injector = Injector::getInstance(index);
	if (!injector.worker.isNull())
	{
		QHash<long long, QVariant> playerInfoColContents = injector.worker->playerInfoColContents.toHash();
		for (auto it = playerInfoColContents.begin(); it != playerInfoColContents.end(); ++it)
		{
			onUpdateCharInfoColContents(it.key(), it.value());
		}

		long long stone = injector.worker->getPC().gold;
		onUpdateCharInfoStone(stone);

		for (long long i = 0; i < sa::MAX_PET; ++i)
		{
			sa::PET pet = injector.worker->getPet(i);
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

	long long size = equipVHeaderList.size();
	for (long long row = 0; row < size; ++row)
	{
		ui.tableWidget->setText(row, 0, equipVHeaderList.value(row));
	}
}

void CharInfoForm::onUpdateCharInfoColContents(long long col, const QVariant& vdata)
{
	// 檢查是否為 QVariantList
	if (vdata.type() != QVariant::List)
		return;

	col += 1;

	// 取得 QVariantList
	QVariantList list = vdata.toList();

	// 取得 QVariantList 的大小
	const long long size = list.size();

	// 獲取當前表格的總行數
	const long long rowCount = ui.tableWidget->rowCount();
	if (col < 0 || col >= rowCount)
		return;

	// 開始填入內容
	for (long long row = 0; row < rowCount; ++row)
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


void CharInfoForm::onUpdateCharInfoStone(long long stone)
{
	ui.label->setText(tr("stone:%1").arg(stone));
}

void CharInfoForm::onUpdateCharInfoPetState(long long petIndex, long long state)
{
	long long col = petIndex + 1;
	if (col < 0 || col >= ui.tableWidget->columnCount())
		return;

	const QStringList stateStrList = {
		tr("battle"), tr("standby"), tr("mail"), tr("rest"), tr("ride")
	};

	--state;
	if (state < 0 || state >= stateStrList.size())
		return;

	//設置指定 col = petIndex + 1的 header
	QString petName = QString(tr("pet%1(%2)")).arg(petIndex + 1).arg(stateStrList.value(state));

	//get item 
	ui.tableWidget->setHorizontalHeaderText(col + 1, petName);
}

void CharInfoForm::onHeaderClicked(long long logicalIndex)
{
	long long currentIndex = getIndex();
	switch (logicalIndex)
	{
	case 1:
	{
		QPoint mousePos = QCursor::pos();
		long long x = static_cast<long long>(mousePos.x()) - 10;
		long long y = static_cast<long long>(mousePos.y()) + 50;
		abilityForm_.move(x, y);
		abilityForm_.show();
		break;
	}
	default:
	{
		long long petIndex = logicalIndex - 2;
		if (petIndex < 0 || petIndex >= sa::MAX_PET)
			break;
		Injector& injector = Injector::getInstance(currentIndex);
		if (injector.worker.isNull())
			break;

		sa::PET pet = injector.worker->getPet(petIndex);
		switch (pet.state)
		{
		case sa::PetState::kRest:
		{

			injector.worker->setPetState(petIndex, sa::PetState::kStandby);
			break;
		}
		case sa::PetState::kStandby:
		{
			injector.worker->setPetState(petIndex, sa::PetState::kBattle);
			break;
		}
		case sa::PetState::kBattle:
		{
			injector.worker->setPetState(petIndex, sa::PetState::kMail);
			break;
		}
		case sa::PetState::kMail:
		{

			if (injector.worker->getPet(petIndex).loyal == 100)
				injector.worker->setPetState(petIndex, sa::PetState::kRide);
			else
				injector.worker->setPetState(petIndex, sa::PetState::kRest);
			break;
		}
		case sa::PetState::kRide:
		{
			injector.worker->setPetState(petIndex, sa::PetState::kRest);
			break;
		}
		}
	}
	}
}

