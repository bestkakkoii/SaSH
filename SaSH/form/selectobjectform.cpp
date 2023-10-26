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
#include "selectobjectform.h"
#include <util.h>

SelectObjectForm::SelectObjectForm(TitleType type, QWidget* parent)
	: QDialog(parent), type_(type)
{
	ui.setupUi(this);
	setFont(util::getFont());
	util::setWidget(this);
	setAttribute(Qt::WA_QuitOnClose);
	setWindowFlags(Qt::Tool | Qt::Dialog | Qt::WindowCloseButtonHint);
	setModal(true);

	QString listWidgetStyle = /*select item black*/ "QListWidget::item:selected{background-color: rgb(0, 0, 0);color: rgb(255, 255, 255);}";
	ui.listWidget->setAttribute(Qt::WA_StyledBackground);
	ui.listWidget->setStyleSheet(listWidgetStyle);

	ui.listWidget->setUniformItemSizes(false);
	ui.comboBox->setEditable(true);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &SelectObjectForm::onAccept);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("ok"));
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("cancel"));

	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);

	for (auto& button : buttonList)
	{
		util::setPushButton(button);
		connect(button, &QPushButton::clicked, this, &SelectObjectForm::onButtonClicked, Qt::QueuedConnection);
	}

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	static const QHash<TitleType, QString> title_hash = {
	{ kAutoDropItem, tr("auto drop item") },//自動丟棄
	{ kLockAttack, tr("lock attack") },//鎖定攻擊
	{ kLockEscape, tr("lock escape") },//鎖定逃跑
	{ kAutoCatch, tr("auto catch") },//自動捉寵
	{ kAutoDropPet, tr("auto drop pet") },//自動丟寵
	{ kAutoLogOut, tr("auto logout") },//自動登出名單
	{ kWhiteList, tr("white list") },//允許加入名單
	{ kBlackList, tr("black list") },//自動踢除名單
	{ kItem, tr("select item") },//道具選擇
	};
	setWindowTitle(title_hash.value(type_, tr("unknown type")));

	util::setPushButton(ui.buttonBox->button(QDialogButtonBox::Ok));
	util::setPushButton(ui.buttonBox->button(QDialogButtonBox::Cancel));

	util::setComboBox(ui.comboBox);
}

SelectObjectForm::~SelectObjectForm()
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
}

void SelectObjectForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QDialog::showEvent(e);
}

void SelectObjectForm::setList(QStringList objectList)
{
	objectList.removeDuplicates();
	for (auto& str : objectList)
	{
		str = str.simplified();
	}
	ui.listWidget->clear();
	ui.listWidget->addItems(objectList);

}

void SelectObjectForm::setSelectList(QStringList objectList)
{
	objectList.removeDuplicates();
	for (auto& str : objectList)
	{
		str = str.simplified();
	}
	ui.comboBox->clear();
	ui.comboBox->addItems(objectList);
}

void SelectObjectForm::onButtonClicked()
{
	QPushButton* pButton = qobject_cast<QPushButton*>(sender());
	if (!pButton)
		return;

	QString name = pButton->objectName();
	if (name.isEmpty())
		return;

	do
	{
		if (name == "pushButton_add")
		{
			appendItem();
			break;
		}

		if (name == "pushButton_clear")
		{
			ui.listWidget->clear();
			break;
		}

		if (name == "pushButton_up")
		{
			util::SwapRowUp(ui.listWidget);

			break;
		}

		if (name == "pushButton_down")
		{
			util::SwapRowDown(ui.listWidget);
			break;
		}
	} while (false);
}


void SelectObjectForm::onAccept()
{
	if (pRecviveList_ != nullptr)
	{
		pRecviveList_->clear();
		long long size = ui.listWidget->count();
		for (long long i = 0; i < size; ++i)
		{
			pRecviveList_->append(ui.listWidget->item(i)->text());
		}
	}
}

void SelectObjectForm::deleteItem()
{
	QList<QListWidgetItem*> selectedItems = ui.listWidget->selectedItems();
	for (QListWidgetItem* item : selectedItems)
	{
		ui.listWidget->takeItem(ui.listWidget->row(item));
	}
}

void SelectObjectForm::appendItem()
{
	QString currentText = ui.comboBox->currentText().simplified();
	currentText.replace(" ", "");
	if (currentText.isEmpty())
		return;

	currentText.replace(" ", "");

	std::unique_ptr<QListWidgetItem> newItem(q_check_ptr(new QListWidgetItem(currentText)));
	if (newItem == nullptr)
		return;

	QList<QListWidgetItem*> existingItems = ui.listWidget->findItems(currentText, Qt::MatchExactly);
	if (!existingItems.isEmpty())
		return;

	ui.comboBox->setCurrentText(QString());
	ui.listWidget->addItem(newItem.release());
	ui.listWidget->scrollToBottom();
}

void SelectObjectForm::setRecviveList(QStringList* pList)
{
	if (pList == nullptr)
	{
		return;
	}

	pRecviveList_ = pList;
}

//remove
void SelectObjectForm::on_listWidget_itemDoubleClicked(QListWidgetItem* item)
{
	if (item == nullptr)
		return;

	deleteItem();
}

void SelectObjectForm::on_listWidget_itemSelectionChanged()
{
	QListWidgetItem* item = ui.listWidget->currentItem();
	if (item == nullptr)
		return;

	QString text = item->text();
	if (text.isEmpty())
		return;

	ui.comboBox->setCurrentText(text);
}