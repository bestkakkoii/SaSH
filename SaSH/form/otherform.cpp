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
#include "otherform.h"
#include <util.h>
#include <gamedevice.h>
#include "signaldispatcher.h"
#include "selectobjectform.h"

OtherForm::OtherForm(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setFont(util::getFont());

	util::setTab(ui.tabWidge_other);

	QStringList nameCheckList;
	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button && !nameCheckList.contains(button->objectName()))
		{
			util::setPushButton(button);
			nameCheckList.append(button->objectName());
			connect(button, &PushButton::clicked, this, &OtherForm::onButtonClicked, Qt::UniqueConnection);
		}
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox && !nameCheckList.contains(checkBox->objectName()))
		{
			util::setCheckBox(checkBox);
			nameCheckList.append(checkBox->objectName());
			connect(checkBox, &QCheckBox::stateChanged, this, &OtherForm::onCheckBoxStateChanged, Qt::UniqueConnection);
		}
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox && !nameCheckList.contains(spinBox->objectName()))
		{
			util::setSpinBox(spinBox);
			nameCheckList.append(spinBox->objectName());
			connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::UniqueConnection);
		}
	}

	QList <QListWidget*> listWidgetList = util::findWidgets<QListWidget>(this);
	for (auto& listWidget : listWidgetList)
	{
		if (listWidget && !nameCheckList.contains(listWidget->objectName()))
		{
			nameCheckList.append(listWidget->objectName());
			connect(listWidget, &QListWidget::itemDoubleClicked, this, &OtherForm::onListWidgetDoubleClicked, Qt::UniqueConnection);
		}
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox && !nameCheckList.contains(comboBox->objectName()))
		{
			util::setComboBox(comboBox);
			nameCheckList.append(comboBox->objectName());
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
			connect(comboBox, &ComboBox::clicked, this, &OtherForm::onComboBoxClicked, Qt::UniqueConnection);
			connect(comboBox, &ComboBox::currentTextChanged, this, &OtherForm::onComboBoxTextChanged, Qt::UniqueConnection);
		}
	}

	QList <QLineEdit*> lineEditList = util::findWidgets<QLineEdit>(this);
	for (auto& lineEdit : lineEditList)
	{
		if (lineEdit && !nameCheckList.contains(lineEdit->objectName()))
		{
			util::setLineEdit(lineEdit);
			nameCheckList.append(lineEdit->objectName());
			connect(lineEdit, &QLineEdit::textChanged, this, &OtherForm::onLineEditTextChanged, Qt::UniqueConnection);
		}
	}

	QList <QGroupBox*> groupBoxList = util::findWidgets<QGroupBox>(this);
	for (auto& groupBox : groupBoxList)
	{
		if (groupBox && !nameCheckList.contains(groupBox->objectName()))
		{
			nameCheckList.append(groupBox->objectName());
			connect(groupBox, &QGroupBox::clicked, this, &OtherForm::groupBoxClicked, Qt::UniqueConnection);
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &OtherForm::onApplyHashSettingsToUI, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateTeamInfo, this, &OtherForm::onUpdateTeamInfo, Qt::QueuedConnection);

	ui.spinBox_lockpetslevel->setEnabled(true);

	emit signalDispatcher.applyHashSettingsToUI();
}

OtherForm::~OtherForm()
{
}

void OtherForm::showEvent(QShowEvent* e)
{
	onResetControlTextLanguage();
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);
}

void OtherForm::groupBoxClicked(bool checked)
{
	QGroupBox* pGroupBox = qobject_cast<QGroupBox*>(sender());
	if (!pGroupBox)
		return;

	QString name = pGroupBox->objectName();

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "groupBox_lockpets")
	{
		gamedevice.setEnableHash(util::kLockPetScheduleEnable, checked);
		if (checked)
		{
			gamedevice.setEnableHash(util::kLockPetEnable, !checked);
			ui.checkBox_lockpet->setChecked(!checked);
			gamedevice.setEnableHash(util::kLockRideEnable, !checked);
			ui.checkBox_lockride->setChecked(!checked);
		}
	}
}

void OtherForm::onListWidgetDoubleClicked(QListWidgetItem* item)
{
	QListWidget* pListWidget = qobject_cast<QListWidget*>(sender());
	if (!pListWidget)
		return;

	QString name = pListWidget->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "listWidget_prelockpets")
	{
		long long row = pListWidget->row(item);
		pListWidget->takeItem(row);
	}
	else if (name == "listWidget_lockpets")
	{
		//delete item
		long long row = pListWidget->row(item);
		pListWidget->takeItem(row);
		long long size = ui.listWidget_lockpets->count();
		QStringList list;
		for (long long i = 0; i < size; ++i)
		{
			QListWidgetItem* pitem = ui.listWidget_lockpets->item(i);
			if (pitem)
			{
				QString str = pitem->text().simplified();
				if (str.isEmpty())
					continue;
				list.append(str);
			}
		}
		gamedevice.setStringHash(util::kLockPetScheduleString, list.join("|"));
	}

}

void OtherForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "pushButton_lockpetsadd")
	{
		QString text = ui.comboBox_lockpets->currentText().simplified();
		if (text.isEmpty())
			return;
		QString typeStr = ui.comboBox_locktype->currentText().simplified();
		if (typeStr.isEmpty())
			return;
		long long level = ui.spinBox_lockpetslevel->value();
		QString str = QString("%1, %2, %3").arg(text).arg(level).arg(typeStr);
		if (!str.isEmpty())
			ui.listWidget_prelockpets->addItem(str);

	}
	else if (name == "pushButton_addlockpets")
	{
		long long size = ui.listWidget_prelockpets->count();
		QStringList list;
		for (long long i = 0; i < size; ++i)
		{
			QListWidgetItem* item = ui.listWidget_prelockpets->item(i);
			if (item)
			{
				QString str = item->text().simplified();
				if (str.isEmpty())
					continue;
				list.append(str);
			}
		}

		if (list.isEmpty())
			return;

		ui.listWidget_lockpets->addItem(list.join(";"));

		size = ui.listWidget_lockpets->count();
		list.clear();
		for (long long i = 0; i < size; ++i)
		{
			QListWidgetItem* item = ui.listWidget_lockpets->item(i);
			if (item)
			{
				QString str = item->text().simplified();
				if (str.isEmpty())
					continue;
				list.append(str);
			}
		}

		gamedevice.setStringHash(util::kLockPetScheduleString, list.join("|"));
		ui.listWidget_prelockpets->clear();
		return;
	}
	else if (name == "pushButton_clearlockpets")
	{
		ui.listWidget_lockpets->clear();
		gamedevice.setStringHash(util::kLockPetScheduleString, "");
	}
	else if (name == "pushButton_lockpetsall")
	{
		if (gamedevice.worker.isNull() || !gamedevice.worker->getOnlineFlag())
			return;

		QString typeStr = ui.comboBox_locktype->currentText().simplified();
		long long level = ui.spinBox_lockpetslevel->value();
		QStringList list;
		for (long long i = 0; i < sa::MAX_PET; ++i)
		{
			QString text;
			sa::pet_t pet = gamedevice.worker->getPet(i);
			if (pet.valid && !pet.name.isEmpty())
				text = QString("%1:%2").arg(i + 1).arg(pet.name.simplified());
			else
				text = QString("%1:%2").arg(i + 1).arg(tr("unknown"));

			QString str = QString("%1, %2, %3").arg(text).arg(level).arg(typeStr);
			list.append(str);
		}
		ui.listWidget_prelockpets->addItem(list.join(";"));

	}

	else if (name == "pushButton_lockpetsup")
	{
		util::SwapRowUp(ui.listWidget_lockpets);
	}
	else if (name == "pushButton_lockpetsdown")
	{
		util::SwapRowDown(ui.listWidget_lockpets);
	}


	else if (name == "pushButton_leaderleave")
	{
		if (gamedevice.worker.isNull() || !gamedevice.worker->getOnlineFlag())
			return;

		gamedevice.worker->setTeamState(false);
	}

	for (long long i = 1; i < sa::MAX_TEAM; ++i)
	{
		if (name == QString("pushButton_teammate%1kick").arg(i))
		{
			if (gamedevice.worker.isNull() || !gamedevice.worker->getOnlineFlag())
				return;

			gamedevice.worker->kickteam(i);
			break;
		}
	}

	QStringList srcSelectList;
	QStringList srcList;
	QStringList dstList;
	if (name == "pushButton_groupwhitelist")
	{
		if (!gamedevice.worker.isNull())
		{
			srcSelectList = gamedevice.worker->getJoinableUnitList();
		}

		QString src = ui.lineEdit_groupwhitelist->text();
		if (src.isEmpty())
		{
			src = gamedevice.getStringHash(util::kGroupWhiteListString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kWhiteList, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kGroupWhiteListString, dst);
		ui.lineEdit_groupwhitelist->setText(dst);
		return;
	}

	if (name == "pushButton_groupblacklist")
	{
		if (!gamedevice.worker.isNull())
		{
			//下拉框可選列表
			srcSelectList = gamedevice.worker->getJoinableUnitList();
		}

		QString src = ui.lineEdit_groupblacklist->text();
		if (src.isEmpty())
		{
			src = gamedevice.getStringHash(util::kGroupBlackListString);
		}

		if (!src.isEmpty())
		{
			//用戶保存的已選列表
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kBlackList, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kGroupBlackListString, dst);
		ui.lineEdit_groupblacklist->setText(dst);
		return;
	}
}

void OtherForm::onCheckBoxStateChanged(int state)
{
	QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender());
	if (!pCheckBox)
		return;

	bool isChecked = (state == Qt::Checked);

	QString name = pCheckBox->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//lockpet
	if (name == "checkBox_lockpet")
	{
		gamedevice.setEnableHash(util::kLockPetEnable, isChecked);
		if (isChecked)
		{
			gamedevice.setEnableHash(util::kLockPetScheduleEnable, !isChecked);
			ui.groupBox_lockpets->setChecked(!isChecked);
		}
	}
	else if (name == "checkBox_lockride")
	{
		gamedevice.setEnableHash(util::kLockRideEnable, isChecked);
		if (isChecked)
		{
			gamedevice.setEnableHash(util::kLockPetScheduleEnable, !isChecked);
			ui.groupBox_lockpets->setChecked(!isChecked);
		}
	}
}

void OtherForm::onSpinBoxValueChanged(int value)
{
	QSpinBox* pSpinBox = qobject_cast<QSpinBox*>(sender());
	if (!pSpinBox)
		return;

	QString name = pSpinBox->objectName();
	if (name.isEmpty())
		return;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	if (name == "spinBox_tcpdelay")
	{
		gamedevice.setValueHash(util::UserSetting::kTcpDelayValue, value);
		return;
	}

}

void OtherForm::onComboBoxCurrentIndexChanged(int value)
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//group
	if (name == "comboBox_autofuntype")
	{
		gamedevice.setValueHash(util::kAutoFunTypeValue, value != -1 ? value : 0);
	}

	//lockpet
	else if (name == "comboBox_lockpet")
	{
		gamedevice.setValueHash(util::kLockPetValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_lockride")
	{
		gamedevice.setValueHash(util::kLockRideValue, value != -1 ? value : 0);
	}

}

void OtherForm::onComboBoxTextChanged(const QString& text)
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	QString newText = text.simplified();

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "comboBox_autofunname")
	{
		gamedevice.setStringHash(util::kAutoFunNameString, newText);
	}
}

void OtherForm::onComboBoxClicked()
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
	{
		pComboBox->setDisableFocusCheck(false);
		return;
	}

	QString name = pComboBox->objectName();
	if (name.isEmpty())
	{
		pComboBox->setDisableFocusCheck(false);
		return;
	}

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "comboBox_autofunname")
	{
		if (gamedevice.worker.isNull())
		{
			pComboBox->setDisableFocusCheck(false);
			return;
		}

		QStringList unitList = gamedevice.worker->getJoinableUnitList();
		updateComboboxAutoFunNameList(unitList);
	}

	else if (name == "comboBox_lockpets" || name == "comboBox_lockrides" || name == "comboBox_lockpet" || name == "comboBox_lockride")
	{
		long long oldIndex = pComboBox->currentIndex();
		QStringList list;
		if (!gamedevice.worker.isNull() && gamedevice.worker->getOnlineFlag())
		{
			for (long long i = 0; i < sa::MAX_PET; ++i)
			{
				sa::pet_t pet = gamedevice.worker->getPet(i);
				if (pet.name.isEmpty() || !pet.valid)
				{
					list.append(QString("%1:").arg(i + 1));
					continue;
				}
				list.append(QString("%1:%2").arg(i + 1).arg(pet.name));
			}
		}
		pComboBox->clear();
		pComboBox->addItems(list);
		if (oldIndex < list.size())
			pComboBox->setCurrentIndex(oldIndex);
	}


	pComboBox->setDisableFocusCheck(false);
}

void OtherForm::onLineEditTextChanged(const QString& text)
{
	QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(sender());
	if (!pLineEdit)
		return;

	QString name = pLineEdit->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//other2
	if (name == "lineEdit_gameaccount")
	{
		gamedevice.setStringHash(util::UserSetting::kGameAccountString, text);
		return;
	}

	if (name == "lineEdit_gamepassword")
	{
		gamedevice.setStringHash(util::UserSetting::kGamePasswordString, text);
		return;
	}

	if (name == "lineEdit_gamesecuritycode")
	{
		gamedevice.setStringHash(util::UserSetting::kGameSecurityCodeString, text);
		return;
	}

	if (name == "lineEdit_remotewhitelist")
	{
		gamedevice.setStringHash(util::UserSetting::kMailWhiteListString, text);
		return;
	}

	if (name == "lineEdit_eocmd")
	{
		gamedevice.setStringHash(util::UserSetting::kEOCommandString, text);
		return;
	}

	if (name == "lineEdit_title")
	{
		gamedevice.setStringHash(util::UserSetting::kTitleFormatString, text);
		return;
	}

	if (name == "lineEdit_battleinfo_allie")
	{
		gamedevice.setStringHash(util::kBattleAllieFormatString, text);
		return;
	}

	if (name == "lineEdit_battleinfo_enemy")
	{
		gamedevice.setStringHash(util::kBattleEnemyFormatString, text);
		return;
	}

	if (name == "lineEdit_battleinfo_selfmark")
	{
		gamedevice.setStringHash(util::kBattleSelfMarkString, text);
		return;
	}

	if (name == "lineEdit_battleinfo_actmark")
	{
		gamedevice.setStringHash(util::kBattleActMarkString, text);
		return;
	}

	if (name == "lineEdit_battleinfo_space")
	{
		gamedevice.setStringHash(util::kBattleSpaceMarkString, text);
		return;
	}

	if (name == "lineEdit_groupwhitelist")
	{
		gamedevice.setStringHash(util::kGroupWhiteListString, text);
		return;
	}

	if (name == "lineEdit_groupblacklist")
	{
		gamedevice.setStringHash(util::kGroupBlackListString, text);
		return;
	}
}

void OtherForm::updateComboboxAutoFunNameList(const QStringList& autoFunNameList)
{
	QString currentText = ui.comboBox_autofunname->currentText();
	ui.comboBox_autofunname->clear();
	ui.comboBox_autofunname->addItems(autoFunNameList);
	ui.comboBox_autofunname->setCurrentText(currentText);
}

void OtherForm::onResetControlTextLanguage()
{
	ui.comboBox_autofuntype->clear();
	const QStringList autoFunList = { tr("auto join"), tr("auto follow"), tr("auto pk"), tr("auto watch") };
	ui.comboBox_autofuntype->addItems(autoFunList);

	ui.comboBox_locktype->clear();
	ui.comboBox_locktype->addItems(QStringList{ tr("B"),tr("R") });
}

void OtherForm::onApplyHashSettingsToUI()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash = gamedevice.getEnablesHash();
	QHash<util::UserSetting, long long> valueHash = gamedevice.getValuesHash();
	QHash<util::UserSetting, QString> stringHash = gamedevice.getStringsHash();

	if (ui.comboBox_lockride->count() == 0 || ui.comboBox_lockpet->count() == 0)
	{
		QStringList list;
		for (long long i = 0; i < sa::MAX_PET; ++i)
		{
			list.append(QString("%1:").arg(i + 1));
		}

		if (ui.comboBox_lockride->count() == 0)
			ui.comboBox_lockride->addItems(list);

		if (ui.comboBox_lockpet->count() == 0)
			ui.comboBox_lockpet->addItems(list);
	}

	QStringList list;
	if (!gamedevice.worker.isNull() && gamedevice.worker->getOnlineFlag())
	{
		for (long long i = 0; i < sa::MAX_PET; ++i)
		{
			sa::pet_t pet = gamedevice.worker->getPet(i);
			if (pet.name.isEmpty() || !pet.valid)
			{
				list.append(QString("%1:").arg(i + 1));
				continue;
			}
			list.append(QString("%1:%2").arg(i + 1).arg(pet.name));
		}
	}

	if (!list.isEmpty())
	{
		ui.comboBox_lockride->clear();
		ui.comboBox_lockride->addItems(list);
		ui.comboBox_lockpet->clear();
		ui.comboBox_lockpet->addItems(list);
	}

	//group
	ui.comboBox_autofuntype->setCurrentIndex(valueHash.value(util::kAutoFunTypeValue));
	ui.comboBox_autofunname->setCurrentText(stringHash.value(util::kAutoFunNameString));

	//other2
	ui.lineEdit_gameaccount->setText(stringHash.value(util::UserSetting::kGameAccountString));
	ui.lineEdit_gamepassword->setText(stringHash.value(util::UserSetting::kGamePasswordString));
	ui.lineEdit_gamesecuritycode->setText(stringHash.value(util::UserSetting::kGameSecurityCodeString));

	//lockpet
	ui.comboBox_lockpet->setCurrentIndex(valueHash.value(util::kLockPetValue));
	ui.comboBox_lockride->setCurrentIndex(valueHash.value(util::kLockRideValue));
	ui.checkBox_lockpet->setChecked(enableHash.value(util::kLockPetEnable));
	ui.checkBox_lockride->setChecked(enableHash.value(util::kLockRideEnable));

	ui.lineEdit_remotewhitelist->setText(stringHash.value(util::kMailWhiteListString));
	ui.lineEdit_eocmd->setText(stringHash.value(util::kEOCommandString));
	ui.lineEdit_title->setText(stringHash.value(util::kTitleFormatString));
	ui.lineEdit_battleinfo_allie->setText(stringHash.value(util::kBattleAllieFormatString));
	ui.lineEdit_battleinfo_enemy->setText(stringHash.value(util::kBattleEnemyFormatString));
	ui.lineEdit_battleinfo_selfmark->setText(stringHash.value(util::kBattleSelfMarkString));
	ui.lineEdit_battleinfo_actmark->setText(stringHash.value(util::kBattleActMarkString));
	ui.lineEdit_battleinfo_space->setText(stringHash.value(util::kBattleSpaceMarkString));

	ui.groupBox_lockpets->setChecked(enableHash.value(util::kLockPetScheduleEnable));

	ui.spinBox_tcpdelay->setValue(valueHash.value(util::UserSetting::kTcpDelayValue));

	QString schedule = stringHash.value(util::kLockPetScheduleString).simplified();
	QStringList scheduleList = schedule.split(util::rexOR, Qt::SkipEmptyParts);
	ui.listWidget_lockpets->clear();
	ui.listWidget_lockpets->addItems(scheduleList);

}

void OtherForm::onUpdateTeamInfo(const QStringList& strList)
{
	for (long long i = 0; i <= sa::MAX_TEAM; ++i)
	{
		QString objName = QString("label_teammate%1").arg(i);
		QLabel* label = ui.groupBox_team->findChild<QLabel*>(objName);
		if (!label)
			continue;
		if (i >= strList.size())
			break;

		label->setText(strList[i]);
	}
}