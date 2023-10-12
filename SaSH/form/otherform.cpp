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

import Utility;

#include "stdafx.h"
#include "otherform.h"
#include <injector.h>
#include "signaldispatcher.h"

OtherForm::OtherForm(__int64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	connect(this, &OtherForm::resetControlTextLanguage, this, &OtherForm::onResetControlTextLanguage, Qt::QueuedConnection);


	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &OtherForm::onButtonClicked, Qt::QueuedConnection);
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox)
			connect(checkBox, &QCheckBox::stateChanged, this, &OtherForm::onCheckBoxStateChanged, Qt::QueuedConnection);
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox)
			connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::QueuedConnection);
	}

	QList <QListWidget*> listWidgetList = util::findWidgets<QListWidget>(this);
	for (auto& listWidget : listWidgetList)
	{
		if (listWidget)
			connect(listWidget, &QListWidget::itemDoubleClicked, this, &OtherForm::onListWidgetDoubleClicked, Qt::QueuedConnection);
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
		{
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::QueuedConnection);
			connect(comboBox, &ComboBox::clicked, this, &OtherForm::onComboBoxClicked, Qt::QueuedConnection);
			connect(comboBox, &ComboBox::currentTextChanged, this, &OtherForm::onComboBoxTextChanged, Qt::QueuedConnection);
		}
	}

	QList <QLineEdit*> lineEditList = util::findWidgets<QLineEdit>(this);
	for (auto& lineEdit : lineEditList)
	{
		if (lineEdit)
			connect(lineEdit, &QLineEdit::textChanged, this, &OtherForm::onLineEditTextChanged, Qt::QueuedConnection);
	}

	QList <QGroupBox*> groupBoxList = util::findWidgets<QGroupBox>(this);
	for (auto& groupBox : groupBoxList)
	{
		if (groupBox)
			connect(groupBox, &QGroupBox::clicked, this, &OtherForm::groupBoxClicked, Qt::QueuedConnection);
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

void OtherForm::groupBoxClicked(bool checked)
{
	QGroupBox* pGroupBox = qobject_cast<QGroupBox*>(sender());
	if (!pGroupBox)
		return;

	QString name = pGroupBox->objectName();

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (name == "groupBox_lockpets")
	{
		injector.setEnableHash(kLockPetScheduleEnable, checked);
		if (checked)
		{
			injector.setEnableHash(kLockPetEnable, !checked);
			ui.checkBox_lockpet->setChecked(!checked);
			injector.setEnableHash(kLockRideEnable, !checked);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (name == "listWidget_prelockpets")
	{
		__int64 row = pListWidget->row(item);
		pListWidget->takeItem(row);
	}
	else if (name == "listWidget_lockpets")
	{
		//delete item
		__int64 row = pListWidget->row(item);
		pListWidget->takeItem(row);
		__int64 size = ui.listWidget_lockpets->count();
		QStringList list;
		for (__int64 i = 0; i < size; ++i)
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
		injector.setStringHash(kLockPetScheduleString, list.join("|"));
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (name == "pushButton_lockpetsadd")
	{
		QString text = ui.comboBox_lockpets->currentText().simplified();
		if (text.isEmpty())
			return;
		QString typeStr = ui.comboBox_locktype->currentText().simplified();
		if (typeStr.isEmpty())
			return;
		__int64 level = ui.spinBox_lockpetslevel->value();
		QString str = QString("%1, %2, %3").arg(text).arg(level).arg(typeStr);
		if (!str.isEmpty())
			ui.listWidget_prelockpets->addItem(str);

	}
	else if (name == "pushButton_addlockpets")
	{
		__int64 size = ui.listWidget_prelockpets->count();
		QStringList list;
		for (__int64 i = 0; i < size; ++i)
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
		for (__int64 i = 0; i < size; ++i)
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

		injector.setStringHash(kLockPetScheduleString, list.join("|"));
		ui.listWidget_prelockpets->clear();
		return;
	}
	else if (name == "pushButton_clearlockpets")
	{
		ui.listWidget_lockpets->clear();
		injector.setStringHash(kLockPetScheduleString, "");
	}
	else if (name == "pushButton_lockpetsall")
	{
		if (injector.server.isNull() || !injector.server->getOnlineFlag())
			return;

		QString typeStr = ui.comboBox_locktype->currentText().simplified();
		__int64 level = ui.spinBox_lockpetslevel->value();
		QStringList list;
		for (__int64 i = 0; i < MAX_PET; ++i)
		{
			QString text;
			PET pet = injector.server->getPet(i);
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
		util::RowSwap::up(ui.listWidget_lockpets);
	}
	else if (name == "pushButton_lockpetsdown")
	{
		util::RowSwap::down(ui.listWidget_lockpets);
	}


	else if (name == "pushButton_leaderleave")
	{
		if (injector.server.isNull() || !injector.server->getOnlineFlag())
			return;

		injector.server->setTeamState(false);
	}

	for (__int64 i = 1; i < MAX_PARTY; ++i)
	{
		if (name == QString("pushButton_teammate%1kick").arg(i))
		{
			if (injector.server.isNull() || !injector.server->getOnlineFlag())
				return;

			injector.server->kickteam(i);
			break;
		}
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//lockpet
	if (name == "checkBox_lockpet")
	{
		injector.setEnableHash(kLockPetEnable, isChecked);
		if (isChecked)
		{
			injector.setEnableHash(kLockPetScheduleEnable, !isChecked);
			ui.groupBox_lockpets->setChecked(!isChecked);
		}
	}
	else if (name == "checkBox_lockride")
	{
		injector.setEnableHash(kLockRideEnable, isChecked);
		if (isChecked)
		{
			injector.setEnableHash(kLockPetScheduleEnable, !isChecked);
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

}

void OtherForm::onComboBoxCurrentIndexChanged(int value)
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//group
	if (name == "comboBox_autofuntype")
	{
		injector.setValueHash(kAutoFunTypeValue, value != -1 ? value : 0);
	}

	//lockpet
	else if (name == "comboBox_lockpet")
	{
		injector.setValueHash(kLockPetValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_lockride")
	{
		injector.setValueHash(kLockRideValue, value != -1 ? value : 0);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (name == "comboBox_autofunname")
	{
		injector.setStringHash(kAutoFunNameString, newText);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (name == "comboBox_autofunname")
	{
		if (injector.server.isNull())
		{
			pComboBox->setDisableFocusCheck(false);
			return;
		}

		QStringList unitList = injector.server->getJoinableUnitList();
		updateComboboxAutoFunNameList(unitList);
	}

	else if (name == "comboBox_lockpets" || name == "comboBox_lockrides" || name == "comboBox_lockpet" || name == "comboBox_lockride")
	{
		__int64 oldIndex = pComboBox->currentIndex();
		QStringList list;
		if (!injector.server.isNull() && injector.server->getOnlineFlag())
		{
			for (__int64 i = 0; i < MAX_PET; ++i)
			{
				PET pet = injector.server->getPet(i);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//other2
	if (name == "lineEdit_gameaccount")
	{
		injector.setStringHash(UserSetting::kGameAccountString, text);
	}

	if (name == "lineEdit_gamepassword")
	{
		injector.setStringHash(UserSetting::kGamePasswordString, text);
	}

	if (name == "lineEdit_gamesecuritycode")
	{
		injector.setStringHash(UserSetting::kGameSecurityCodeString, text);
	}

	if (name == "lineEdit_remotewhitelist")
	{
		injector.setStringHash(UserSetting::kMailWhiteListString, text);
	}

	if (name == "lineEdit_eocmd")
	{
		injector.setStringHash(UserSetting::kEOCommandString, text);
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
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QHash<UserSetting, bool> enableHash = injector.getEnablesHash();
	QHash<UserSetting, __int64> valueHash = injector.getValuesHash();
	QHash<UserSetting, QString> stringHash = injector.getStringsHash();

	if (ui.comboBox_lockride->count() == 0 || ui.comboBox_lockpet->count() == 0)
	{
		QStringList list;
		for (__int64 i = 0; i < MAX_PET; ++i)
		{
			list.append(QString("%1:").arg(i + 1));
		}

		if (ui.comboBox_lockride->count() == 0)
			ui.comboBox_lockride->addItems(list);

		if (ui.comboBox_lockpet->count() == 0)
			ui.comboBox_lockpet->addItems(list);
	}

	QStringList list;
	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		for (__int64 i = 0; i < MAX_PET; ++i)
		{
			PET pet = injector.server->getPet(i);
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
	ui.comboBox_autofuntype->setCurrentIndex(valueHash.value(kAutoFunTypeValue));
	ui.comboBox_autofunname->setCurrentText(stringHash.value(kAutoFunNameString));

	//other2
	ui.lineEdit_gameaccount->setText(stringHash.value(UserSetting::kGameAccountString));
	ui.lineEdit_gamepassword->setText(stringHash.value(UserSetting::kGamePasswordString));
	ui.lineEdit_gamesecuritycode->setText(stringHash.value(UserSetting::kGameSecurityCodeString));

	//lockpet
	ui.comboBox_lockpet->setCurrentIndex(valueHash.value(kLockPetValue));
	ui.comboBox_lockride->setCurrentIndex(valueHash.value(kLockRideValue));
	ui.checkBox_lockpet->setChecked(enableHash.value(kLockPetEnable));
	ui.checkBox_lockride->setChecked(enableHash.value(kLockRideEnable));

	ui.lineEdit_remotewhitelist->setText(stringHash.value(kMailWhiteListString));
	ui.lineEdit_eocmd->setText(stringHash.value(kEOCommandString));


	ui.groupBox_lockpets->setChecked(enableHash.value(kLockPetScheduleEnable));

	QString schedule = stringHash.value(kLockPetScheduleString).simplified();
	QStringList scheduleList = schedule.split(rexOR, Qt::SkipEmptyParts);
	ui.listWidget_lockpets->clear();
	ui.listWidget_lockpets->addItems(scheduleList);

}

void OtherForm::onUpdateTeamInfo(const QStringList& strList)
{
	for (__int64 i = 0; i <= MAX_PARTY; ++i)
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