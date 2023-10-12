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

import Config;
import Global;
import Utility;
#include "stdafx.h"
#include "abilityform.h"
#include <injector.h>

AbilityForm::AbilityForm(__int64 index, QWidget* parent)
	: QDialog(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setWindowFlags(Qt::Dialog | Qt::Tool);
	setFixedSize(this->width(), this->height());
	setAttribute(Qt::WA_DeleteOnClose);

	ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	setStyleSheet("QDialog{border: 1px solid black;}");

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &AbilityForm::onButtonClicked, Qt::QueuedConnection);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);

	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &AbilityForm::onApplyHashSettingsToUI, Qt::QueuedConnection);

	emit signalDispatcher.applyHashSettingsToUI();

	FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();
}

AbilityForm::~AbilityForm()
{
	FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
}

void AbilityForm::onApplyHashSettingsToUI()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QHash<UserSetting, bool> enableHash = injector.getEnablesHash();
	QHash<UserSetting, __int64> valueHash = injector.getValuesHash();
	QHash<UserSetting, QString> stringHash = injector.getStringsHash();

	ui.checkBox->setChecked(enableHash.value(kAutoAbilityEnable));

	QString str = stringHash.value(kAutoAbilityString);
	if (str.isEmpty())
		return;

	QStringList list = str.split(rexOR);
	if (list.isEmpty())
		return;

	ui.tableWidget->clearContents();
	//resize row
	ui.tableWidget->setRowCount(list.size());

	__int64 size = list.size();
	__int64 count = 0;
	for (__int64 i = 0; i < size; ++i)
	{
		QString s = list.at(i);
		QStringList strList = s.split(rexComma);
		if (strList.size() != 2)
			continue;

		QString type = strList.at(0);
		QString value = strList.at(1);
		if (type.isEmpty() || value.isEmpty())
			continue;

		bool ok = false;
		if (value.toLongLong(&ok) <= 0 && !ok)
			continue;

		ui.tableWidget->setText(i, 0, type);
		ui.tableWidget->setText(i, 1, value);
		++count;
	}

	if (size > count)
	{
		for (__int64 i = count; i < size; ++i)
			ui.tableWidget->removeRow(i);
	}
}

void AbilityForm::on_tableWidget_cellDoubleClicked(int row, int column)
{
	TableWidget* pTableWidget = qobject_cast<TableWidget*>(sender());
	if (nullptr == pTableWidget)
		return;

	pTableWidget->removeRow(row);
}

void AbilityForm::on_checkBox_stateChanged(int state)
{
	Injector& injector = Injector::getInstance(getIndex());
	injector.setEnableHash(kAutoAbilityEnable, state == Qt::Checked);
}

void AbilityForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	if (name == "pushButton_add")
	{
		__int64 point = ui.spinBox->value();
		if (point <= 0 || point > 1000)
			return;

		QString text = ui.comboBox->currentText();

		__int64 size = ui.tableWidget->rowCount();
		ui.tableWidget->setText(size, 0, text);
		ui.tableWidget->setText(size, 1, QString::number(point));

		size = ui.tableWidget->rowCount();
		QStringList list;
		for (__int64 i = 0; i < size; ++i)
		{
			QTableWidgetItem* itemType = ui.tableWidget->item(i, 0);
			if (nullptr == itemType)
				continue;
			QTableWidgetItem* itemValue = ui.tableWidget->item(i, 1);
			if (nullptr == itemValue)
				continue;

			QString type = itemType->text();
			if (type.isEmpty())
				continue;

			QString value = itemValue->text();
			bool ok = false;
			if (value.isEmpty() || (value.toLongLong(&ok) <= 0 && !ok))
				continue;

			QString str = QString("%1,%2").arg(type).arg(value);
			list.append(str);
		}

		if (list.isEmpty())
			return;

		QString str = list.join("|");

		Injector& injector = Injector::getInstance(getIndex());

		injector.setStringHash(kAutoAbilityString, str);
		return;
	}

	if (name == "pushButton_clear")
	{
		__int64 size = ui.tableWidget->rowCount();
		for (__int64 i = size - 1, j = 0; i >= j; --i)
			ui.tableWidget->removeRow(i);
		Injector& injector = Injector::getInstance(getIndex());
		injector.setStringHash(kAutoAbilityString, "");
		return;
	}

	if (name == "pushButton_up")
	{
		util::RowSwap::up(ui.tableWidget);
		return;
	}

	if (name == "pushButton_down")
	{
		util::RowSwap::down(ui.tableWidget);
		return;
	}

}