
/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include "afkform.h"
#include <util.h>
#include <gamedevice.h>

#include "signaldispatcher.h"
#include "selecttargetform.h"
#include "selectobjectform.h"
#include "battlesettingfrom.h"

AfkForm::AfkForm(long long index, QWidget* parent)
	: QWidget(nullptr)
	, Indexer(index)
	, pparent(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground);
	setFont(util::getFont());

	Qt::WindowFlags windowflag = this->windowFlags();
	windowflag |= Qt::WindowType::Tool;
	setWindowFlag(Qt::WindowType::Tool);

	util::setWidget(this);
	util::setTab(ui.tabWidget);


	connect(ui.tableWidget, &DragDropWidget::orderChanged, this, &AfkForm::onDragDropWidgetItemChanged, Qt::QueuedConnection);

	connect(this, &AfkForm::resetControlTextLanguage, this, &AfkForm::onResetControlTextLanguage, Qt::QueuedConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &AfkForm::onApplyHashSettingsToUI, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateComboBoxItemText, this, &AfkForm::onUpdateComboBoxItemText, Qt::QueuedConnection);

	QStringList nameCheckList;
	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button && !nameCheckList.contains(button->objectName()))
		{
			util::setPushButton(button);
			nameCheckList.append(button->objectName());
			connect(button, &PushButton::clicked, this, &AfkForm::onButtonClicked, Qt::UniqueConnection);
		}
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox && !nameCheckList.contains(checkBox->objectName()))
		{
			util::setCheckBox(checkBox);
			nameCheckList.append(checkBox->objectName());
			connect(checkBox, &QCheckBox::stateChanged, this, &AfkForm::onCheckBoxStateChanged, Qt::UniqueConnection);
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

	QList <QLineEdit*> lineEditList = util::findWidgets<QLineEdit>(this);
	for (auto& lineEdit : lineEditList)
	{
		if (lineEdit && !nameCheckList.contains(lineEdit->objectName()))
		{
			util::setLineEdit(lineEdit);
			nameCheckList.append(lineEdit->objectName());
			connect(lineEdit, &QLineEdit::textChanged, this, &AfkForm::onLineEditTextChanged, Qt::UniqueConnection);
		}
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox && !nameCheckList.contains(comboBox->objectName()))
		{
			util::setComboBox(comboBox);
			nameCheckList.append(comboBox->objectName());
			connect(comboBox, &ComboBox::clicked, this, &AfkForm::onComboBoxClicked, Qt::UniqueConnection);
			connect(comboBox, &ComboBox::currentTextChanged, this, &AfkForm::onComboBoxTextChanged, Qt::UniqueConnection);
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
		}
	}

	QList <QLabel*> labelList = util::findWidgets<QLabel>(this);
	for (auto& label : labelList)
	{
		if (label && !nameCheckList.contains(label->objectName()))
		{
			util::setLabel(label);
			nameCheckList.append(label->objectName());
		}
	}

	QList <TableWidget*> tableList = util::findWidgets<TableWidget>(this);
	for (auto& table : tableList)
	{
		if (table && !nameCheckList.contains(table->objectName()))
		{
			util::setTableWidget(table);
			nameCheckList.append(table->objectName());
		}
	}

	onResetControlTextLanguage();

	GameDevice& gamedevice = GameDevice::getInstance(index);
	if (!gamedevice.worker.isNull())
		gamedevice.worker->updateComboBoxList();

	BattleSettingFrom* pBattleSettingFrom = new BattleSettingFrom(index, this);
	ui.tabWidget->addTab(pBattleSettingFrom, tr("demo"));
}

AfkForm::~AfkForm()
{

}

void AfkForm::hideEvent(QHideEvent* e)
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
	QWidget::hideEvent(e);
}

void AfkForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (!gamedevice.worker.isNull())
		gamedevice.worker->updateComboBoxList();

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	signalDispatcher.applyHashSettingsToUI();
}

void AfkForm::closeEvent(QCloseEvent* event)
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();

	QWidget::closeEvent(event);
}

bool AfkForm::eventFilter(QObject* obj, QEvent* eve)
{
	return QWidget::eventFilter(obj, eve);
}

void AfkForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	QString temp;
	QStringList srcList;
	QStringList dstList;
	QStringList srcSelectList;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//battle
	if (name == "pushButton_roundaction_char")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleCharRoundActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_crossaction_char")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleCharCrossActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_normalaction_char")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleCharNormalActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}

	if (name == "pushButton_roundaction_pet")
	{
		if (createSelectTargetForm(currentIndex, util::kBattlePetRoundActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_crossaction_pet")
	{
		if (createSelectTargetForm(currentIndex, util::kBattlePetCrossActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_normalaction_pet")
	{
		if (createSelectTargetForm(currentIndex, util::kBattlePetNormalActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}

	//heal
	if (name == "pushButton_magicheal")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleMagicHealTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_itemheal")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleItemHealTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_magicrevive")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleMagicReviveTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_itemrevive")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleItemReviveTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_petheal")
	{
		if (createSelectTargetForm(currentIndex, util::kBattlePetHealTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_petpurg")
	{
		if (createSelectTargetForm(currentIndex, util::kBattlePetPurgTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_charpurg")
	{
		if (createSelectTargetForm(currentIndex, util::kBattleCharPurgTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}

	auto fillItem = [this, &gamedevice](QLineEdit* pLineEdit, util::UserSetting set)->void
		{
			QStringList srcSelectList;
			QStringList srcList;
			QStringList dstList;

			QVariant dat = gamedevice.getUserData(util::kUserItemNames);
			if (dat.isValid())
				srcSelectList = dat.toStringList();

			QString src = pLineEdit->text();
			if (src.isEmpty())
				src = gamedevice.getStringHash(set);

			if (!src.isEmpty())
				srcList = src.split(util::rexOR, Qt::SkipEmptyParts);

			if (!createSelectObjectForm(SelectObjectForm::kItem, srcSelectList, srcList, &dstList, this))
				return;

			QString dst = dstList.join("|");
			gamedevice.setStringHash(set, dst);
			pLineEdit->setText(dst);
		};

	if (name == "pushButton_magicheal_normal_select")
	{
		fillItem(ui.lineEdit_magicheal_normal, util::kNormalMagicHealItemString);
		return;
	}

	if (name == "pushButton_itemheal_normal_select")
	{
		fillItem(ui.lineEdit_itemheal_normal, util::kNormalItemHealItemString);
		return;
	}

	if (name == "pushButton_itemhealmp_normal_select")
	{
		fillItem(ui.lineEdit_itemhealmp_normal, util::kNormalItemHealMpItemString);
		return;
	}

	if (name == "pushButton_itemheal_select")
	{
		fillItem(ui.lineEdit_itemheal, util::kBattleItemHealItemString);
		return;
	}

	if (name == "pushButton_itemhealmp_select")
	{
		fillItem(ui.lineEdit_itemhealmp, util::kBattleItemHealMpItemString);
		return;
	}

	if (name == "pushButton_itemrevive_select")
	{
		fillItem(ui.lineEdit_itemrevive, util::kBattleItemReviveItemString);
		return;
	}

	if (name == "pushButton_autocatchpet_item_select")
	{
		fillItem(ui.lineEdit_autocatchpet_item, util::kBattleCatchCharItemString);
		return;
	}


	//catch
	if (name == "pushButton_autocatchpet")
	{
		QVariant dat = gamedevice.getUserData(util::kUserEnemyNames);
		if (dat.isValid())
		{
			srcSelectList = dat.toStringList();
		}
		srcSelectList.removeDuplicates();

		QString src = ui.lineEdit_autocatchpet->text();
		if (src.isEmpty())
		{
			src = gamedevice.getStringHash(util::kBattleCatchPetNameString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoCatch, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kBattleCatchPetNameString, dst);
		ui.lineEdit_autocatchpet->setText(dst);
		return;
	}
	if (name == "pushButton_autodroppet")
	{
		QVariant dat = gamedevice.getUserData(util::kUserEnemyNames);
		if (dat.isValid())
		{
			srcSelectList = dat.toStringList();
		}

		dat = gamedevice.getUserData(util::kUserPetNames);
		if (dat.isValid())
		{
			srcSelectList.append(dat.toStringList());
		}
		srcSelectList.removeDuplicates();

		QString src = ui.lineEdit_autodroppet_name->text();
		if (src.isEmpty())
		{
			src = gamedevice.getStringHash(util::kDropPetNameString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoDropPet, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		gamedevice.setStringHash(util::kDropPetNameString, dst);
		ui.lineEdit_autodroppet_name->setText(dst);
		return;
	}
}

void AfkForm::onCheckBoxStateChanged(int state)
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

	//battle
	if (name == "checkBox_crossaction_char")
	{
		gamedevice.setEnableHash(util::kBattleCrossActionCharEnable, isChecked);
		return;
	}
	if (name == "checkBox_crossaction_pet")
	{
		gamedevice.setEnableHash(util::kBattleCrossActionPetEnable, isChecked);
		return;
	}

	if (name == "checkBox_noscapewhilelockpet")
	{
		gamedevice.setEnableHash(util::kBattleNoEscapeWhileLockPetEnable, isChecked);
		return;
	}

	//battle heal
	if (name == "checkBox_magicheal")
	{
		gamedevice.setEnableHash(util::kBattleMagicHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal")
	{
		gamedevice.setEnableHash(util::kBattleItemHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal_meatpriority")
	{
		gamedevice.setEnableHash(util::kBattleItemHealMeatPriorityEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemhealmp")
	{
		gamedevice.setEnableHash(util::kBattleItemHealMpEnable, isChecked);
		return;
	}

	if (name == "checkBox_magicrevive")
	{
		gamedevice.setEnableHash(util::kBattleMagicReviveEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemrevive")
	{
		gamedevice.setEnableHash(util::kBattleItemReviveEnable, isChecked);
		return;
	}

	if (name == "checkBox_skillMp")
	{
		gamedevice.setEnableHash(util::kBattleSkillMpEnable, isChecked);
		return;
	}

	if (name == "checkBox_petheal")
	{
		gamedevice.setEnableHash(util::kBattlePetHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_petpurg")
	{
		gamedevice.setEnableHash(util::kBattlePetPurgEnable, isChecked);
		return;
	}

	if (name == "checkBox_charpurg")
	{
		gamedevice.setEnableHash(util::kBattleCharPurgEnable, isChecked);
		return;
	}

	//normal heal
	if (name == "checkBox_magicheal_normal")
	{
		gamedevice.setEnableHash(util::kNormalMagicHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal_normal")
	{
		gamedevice.setEnableHash(util::kNormalItemHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal_normal_meatpriority")
	{
		gamedevice.setEnableHash(util::kNormalItemHealMeatPriorityEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemhealmp_normal")
	{
		gamedevice.setEnableHash(util::kNormalItemHealMpEnable, isChecked);
		return;
	}

	//catch
	if (name == "checkBox_autocatchpet_level")
	{
		gamedevice.setEnableHash(util::kBattleCatchTargetLevelEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_hp")
	{
		gamedevice.setEnableHash(util::kBattleCatchTargetMaxHpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_magic")
	{
		gamedevice.setEnableHash(util::kBattleCatchCharMagicEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_item")
	{
		gamedevice.setEnableHash(util::kBattleCatchCharItemEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_petskill")
	{
		gamedevice.setEnableHash(util::kBattleCatchPetSkillEnable, isChecked);
		return;
	}

	//catch->drop
	if (name == "checkBox_autodroppet")
	{
		gamedevice.setEnableHash(util::kDropPetEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_str")
	{
		gamedevice.setEnableHash(util::kDropPetStrEnable, isChecked);
		return;
	}

	if (name == "spinBox_autodroppet_def")
	{
		gamedevice.setEnableHash(util::kDropPetDefEnable, isChecked);
		return;
	}

	if (name == "spinBox_autodroppet_agi")
	{
		gamedevice.setEnableHash(util::kDropPetAgiEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_hp")
	{
		gamedevice.setEnableHash(util::kDropPetHpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_hp")
	{
		gamedevice.setEnableHash(util::kDropPetHpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_aggregate")
	{
		gamedevice.setEnableHash(util::kDropPetAggregateEnable, isChecked);
		return;
	}
}

void AfkForm::onSpinBoxValueChanged(int value)
{
	QSpinBox* pSpinBox = qobject_cast<QSpinBox*>(sender());
	if (!pSpinBox)
		return;

	QString name = pSpinBox->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//battle heal
	if (name == "spinBox_magicheal_char")
	{
		gamedevice.setValueHash(util::kBattleMagicHealCharValue, value);
		return;
	}

	if (name == "spinBox_magicheal_pet")
	{
		gamedevice.setValueHash(util::kBattleMagicHealPetValue, value);
		return;
	}

	if (name == "spinBox_magicheal_allie")
	{
		gamedevice.setValueHash(util::kBattleMagicHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemheal_char")
	{
		gamedevice.setValueHash(util::kBattleItemHealCharValue, value);
		return;
	}

	if (name == "spinBox_itemheal_pet")
	{
		gamedevice.setValueHash(util::kBattleItemHealPetValue, value);
		return;
	}

	if (name == "spinBox_itemheal_allie")
	{
		gamedevice.setValueHash(util::kBattleItemHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemhealmp")
	{
		gamedevice.setValueHash(util::kBattleItemHealMpValue, value);
		return;
	}

	if (name == "spinBox_skillMp")
	{
		gamedevice.setValueHash(util::kBattleSkillMpValue, value);
		return;
	}

	if (name == "spinBox_petheal_char")
	{
		gamedevice.setValueHash(util::kBattlePetHealCharValue, value);
		return;
	}

	if (name == "spinBox_petheal_pet")
	{
		gamedevice.setValueHash(util::kBattlePetHealPetValue, value);
		return;
	}

	if (name == "spinBox_petheal_allie")
	{
		gamedevice.setValueHash(util::kBattlePetHealAllieValue, value);
		return;
	}
	//normal heal
	if (name == "spinBox_magicheal_normal_char")
	{
		gamedevice.setValueHash(util::kNormalMagicHealCharValue, value);
		return;
	}

	if (name == "spinBox_magicheal_normal_pet")
	{
		gamedevice.setValueHash(util::kNormalMagicHealPetValue, value);
		return;
	}

	if (name == "spinBox_magicheal_normal_allie")
	{
		gamedevice.setValueHash(util::kNormalMagicHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemheal_normal_char")
	{
		gamedevice.setValueHash(util::kNormalItemHealCharValue, value);
		return;
	}

	if (name == "spinBox_itemheal_normal_pet")
	{
		gamedevice.setValueHash(util::kNormalItemHealPetValue, value);
		return;
	}

	if (name == "spinBox_itemheal_normal_allie")
	{
		gamedevice.setValueHash(util::kNormalItemHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemhealmp_normal")
	{
		gamedevice.setValueHash(util::kNormalItemHealMpValue, value);
		return;
	}

	//autowalk
	if (name == "spinBox_autowalkdelay")
	{
		gamedevice.setValueHash(util::kAutoWalkDelayValue, value);
		return;
	}

	if (name == "spinBox_autowalklen")
	{
		gamedevice.setValueHash(util::kAutoWalkDistanceValue, value);
		return;
	}

	//catch
	if (name == "spinBox_autocatchpet_level")
	{
		gamedevice.setValueHash(util::kBattleCatchTargetLevelValue, value);
		return;
	}

	if (name == "spinBox_autocatchpet_hp")
	{
		gamedevice.setValueHash(util::kBattleCatchTargetMaxHpValue, value);
		return;
	}

	if (name == "spinBox_autocatchpet_magic")
	{
		gamedevice.setValueHash(util::kBattleCatchTargetMagicHpValue, value);
		return;
	}

	if (name == "spinBox_autocatchpet_item")
	{
		gamedevice.setValueHash(util::kBattleCatchTargetItemHpValue, value);
		return;
	}

	//catch->drop
	if (name == "spinBox_autodroppet_str")
	{
		gamedevice.setValueHash(util::kDropPetStrValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_def")
	{
		gamedevice.setValueHash(util::kDropPetDefValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_agi")
	{
		gamedevice.setValueHash(util::kDropPetAgiValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_hp")
	{
		gamedevice.setValueHash(util::kDropPetHpValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_aggregate")
	{
		gamedevice.setValueHash(util::kDropPetAggregateValue, value);
		return;
	}

	if (name == "spinBox_rounddelay")
	{
		gamedevice.setValueHash(util::kBattleActionDelayValue, value);
		return;
	}

	if (name == "spinBox_resend_delay")
	{
		gamedevice.setValueHash(util::kBattleResendDelayValue, value);
		return;
	}
}

void AfkForm::onLineEditTextChanged(const QString& text)
{
	QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(sender());
	if (!pLineEdit)
		return;

	QString name = pLineEdit->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();

	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (name == "lineEdit_magicheal_normal")
	{
		gamedevice.setStringHash(util::kNormalMagicHealItemString, text);
		return;
	}

	if (name == "lineEdit_itemheal_normal")
	{
		gamedevice.setStringHash(util::kNormalItemHealItemString, text);
		return;
	}

	if (name == "lineEdit_itemhealmp_normal")
	{
		gamedevice.setStringHash(util::kNormalItemHealMpItemString, text);
		return;
	}

	if (name == "lineEdit_itemheal")
	{
		gamedevice.setStringHash(util::kBattleItemHealItemString, text);
		return;
	}

	if (name == "lineEdit_itemhealmp")
	{
		gamedevice.setStringHash(util::kBattleItemHealMpItemString, text);
		return;
	}

	if (name == "lineEdit_itemrevive")
	{
		gamedevice.setStringHash(util::kBattleItemReviveItemString, text);
		return;
	}

	if (name == "lineEdit_autocatchpet_item")
	{
		gamedevice.setStringHash(util::kBattleCatchCharItemString, text);
		return;
	}


}

void AfkForm::onComboBoxCurrentIndexChanged(int value)
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	//battle char
	if (name == "comboBox_roundaction_char_round")
	{
		gamedevice.setValueHash(util::kBattleCharRoundActionRoundValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_char_action")
	{
		gamedevice.setValueHash(util::kBattleCharRoundActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_char_enemy")
	{
		gamedevice.setValueHash(util::kBattleCharRoundActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_char_level")
	{
		gamedevice.setValueHash(util::kBattleCharRoundActionLevelValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_crossaction_char_action")
	{
		gamedevice.setValueHash(util::kBattleCharCrossActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_crossaction_char_round")
	{
		gamedevice.setValueHash(util::kBattleCharCrossActionRoundValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_normalaction_char_action")
	{
		gamedevice.setValueHash(util::kBattleCharNormalActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_char_enemy")
	{
		gamedevice.setValueHash(util::kBattleCharNormalActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_char_level")
	{
		gamedevice.setValueHash(util::kBattleCharNormalActionLevelValue, value != -1 ? value : 0);
		return;
	}


	//battle pet
	if (name == "comboBox_roundaction_pet_round")
	{
		gamedevice.setValueHash(util::kBattlePetRoundActionRoundValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_pet_action")
	{
		gamedevice.setValueHash(util::kBattlePetRoundActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_pet_enemy")
	{
		gamedevice.setValueHash(util::kBattlePetRoundActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_pet_level")
	{
		gamedevice.setValueHash(util::kBattlePetRoundActionLevelValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_crossaction_pet_action")
	{
		gamedevice.setValueHash(util::kBattlePetCrossActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_crossaction_pet_round")
	{
		gamedevice.setValueHash(util::kBattlePetCrossActionRoundValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_normalaction_pet_action")
	{
		gamedevice.setValueHash(util::kBattlePetNormalActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_pet_enemy")
	{
		gamedevice.setValueHash(util::kBattlePetNormalActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_pet_level")
	{
		gamedevice.setValueHash(util::kBattlePetNormalActionLevelValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_healaction_pet_action")
	{
		gamedevice.setValueHash(util::kBattlePetHealActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_purgaction_pet_action")
	{
		gamedevice.setValueHash(util::kBattlePetPurgActionTypeValue, value != -1 ? value : 0);
		return;
	}
	//magic purg
	if (name == "comboBox_purgaction_char_action")
	{
		gamedevice.setValueHash(util::kBattleCharPurgActionTypeValue, value != -1 ? value : 0);
		return;
	}
	//magic heal
	if (name == "comboBox_magicheal")
	{
		gamedevice.setValueHash(util::kBattleMagicHealMagicValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_magicrevive")
	{
		gamedevice.setValueHash(util::kBattleMagicReviveMagicValue, value != -1 ? value : 0);
		return;
	}

	//normal
	if (name == "comboBox_magicheal_normal")
	{
		if (value <= 2)
		{
			value = 3;
			ui.comboBox_magicheal_normal->blockSignals(true);
			ui.comboBox_magicheal_normal->setCurrentIndex(value);
			ui.comboBox_magicheal_normal->blockSignals(false);
		}

		gamedevice.setValueHash(util::kNormalMagicHealMagicValue, value != -1 ? value : 0);
		return;
	}

	//walk
	if (name == "comboBox_autowalkdir")
	{
		gamedevice.setValueHash(util::kAutoWalkDirectionValue, value != -1 ? value : 0);
		return;
	}

	//catch
	if (name == "comboBox_autocatchpet_mode")
	{
		gamedevice.setValueHash(util::kBattleCatchModeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_autocatchpet_magic")
	{
		gamedevice.setValueHash(util::kBattleCatchCharMagicValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_autocatchpet_petskill")
	{
		gamedevice.setValueHash(util::kBattleCatchPetSkillValue, value != -1 ? value : 0);
		return;
	}
}

void AfkForm::onComboBoxClicked()
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
	{
		return;
	}

	QString name = pComboBox->objectName();
	if (name.isEmpty())
	{
		return;
	}

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.worker.isNull())
		return;

	if (!name.contains("item"))
	{
		gamedevice.worker->updateComboBoxList();
	}
}

void AfkForm::onComboBoxTextChanged(const QString& text)
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

	//battle
	if (name == "comboBox_itemheal")
	{
		gamedevice.setStringHash(util::kBattleItemHealItemString, newText);
		return;
	}
	if (name == "comboBox_itemhealmp")
	{
		gamedevice.setStringHash(util::kBattleItemHealMpItemString, newText);
		return;
	}
	if (name == "comboBox_itemrevive")
	{
		gamedevice.setStringHash(util::kBattleItemReviveItemString, newText);
		return;
	}
	//normal
	if (name == "comboBox_itemheal_normal")
	{
		gamedevice.setStringHash(util::kNormalItemHealItemString, newText);
		return;
	}
	if (name == "comboBox_itemhealmp_normal")
	{
		gamedevice.setStringHash(util::kNormalItemHealMpItemString, newText);
		return;
	}
}

void AfkForm::onResetControlTextLanguage()
{
	auto appendRound = [](QComboBox* combo)->void
		{
			if (combo == nullptr)
				return;

			combo->clear();
			combo->addItem(tr("not use"));
			for (long long i = 1; i <= 100; ++i)
			{
				QString text = tr("at round %1").arg(i);
				combo->addItem(text);
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
			}
		};

	auto appendEnemyAmount = [](QComboBox* combo)->void
		{
			if (combo == nullptr)
				return;

			combo->clear();
			combo->addItem(tr("not use"));
			for (long long i = 1; i <= 9; ++i)
			{
				QString text = tr("enemy amount > %1").arg(i);
				combo->addItem(text);
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
			}
		};

	auto appendCrossRound = [](QComboBox* combo)->void
		{
			if (combo == nullptr)
				return;

			if (combo->hasFocus())
				return;

			combo->clear();
			for (long long i = 1; i <= 100; ++i)
			{
				QString text = tr("every %1 round").arg(i);
				combo->addItem(text);
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
			}
		};

	auto appendEnemyLevel = [](QComboBox* combo)->void
		{
			if (combo == nullptr)
				return;

			combo->clear();
			combo->addItem(tr("not use"));
			for (long long i = 10; i <= 250; i += 10)
			{
				QString text = tr("enemy level > %1").arg(i);
				combo->addItem(text);
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
			}
		};

	auto appendCharAction = [](QComboBox* combo, bool notBattle = false)->void
		{
			if (combo == nullptr)
				return;

			combo->clear();
			QStringList actionList = {
				tr("attack"), tr("defense"), tr("escape"),
				tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
				tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
				tr("gloves")
			};

			long long size = actionList.size();
			for (long long i = 0; i < actionList.size(); ++i)
			{
				if (notBattle && i < 3)
					continue;

				combo->addItem(QString("%1:%2:%3").arg(i + 1).arg(actionList[i]).arg(i >= 3 ? ":" : ""));
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, QString("%1").arg(actionList[i]), Qt::ToolTipRole);
			}

			if (notBattle)
				return;

			for (long long i = 0; i < sa::MAX_PROFESSION_SKILL; ++i)
			{
				QString text = QString("%1:").arg(i + 1 + size);
				combo->addItem(text);
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
			}
		};

	auto appendNumbers = [](QComboBox* combo, long long max)->void
		{
			if (combo == nullptr)
				return;

			combo->clear();
			for (long long i = 1; i <= max; ++i)
			{
				combo->addItem(QString("%1:").arg(i));
				long long index = static_cast<long long>(combo->count()) - 1;
				combo->setItemData(index, QString("%1").arg(i), Qt::ToolTipRole);
			}
		};

	//battle
	appendRound(ui.comboBox_roundaction_char_round);
	appendRound(ui.comboBox_roundaction_pet_round);

	appendEnemyAmount(ui.comboBox_roundaction_char_enemy);
	appendEnemyAmount(ui.comboBox_normalaction_char_enemy);
	appendEnemyAmount(ui.comboBox_roundaction_pet_enemy);
	appendEnemyAmount(ui.comboBox_normalaction_pet_enemy);

	appendCrossRound(ui.comboBox_crossaction_char_round);
	appendCrossRound(ui.comboBox_crossaction_pet_round);

	appendEnemyLevel(ui.comboBox_roundaction_char_level);
	appendEnemyLevel(ui.comboBox_normalaction_char_level);
	appendEnemyLevel(ui.comboBox_roundaction_pet_level);
	appendEnemyLevel(ui.comboBox_normalaction_pet_level);

	appendCharAction(ui.comboBox_roundaction_char_action);
	appendCharAction(ui.comboBox_crossaction_char_action);
	appendCharAction(ui.comboBox_normalaction_char_action);
	appendCharAction(ui.comboBox_magicheal, false);
	appendCharAction(ui.comboBox_magicrevive, false);
	appendCharAction(ui.comboBox_purgaction_char_action, false);

	appendCharAction(ui.comboBox_magicheal_normal, false);

	//battle
	appendNumbers(ui.comboBox_roundaction_pet_action, sa::MAX_PET_SKILL);
	appendNumbers(ui.comboBox_crossaction_pet_action, sa::MAX_PET_SKILL);
	appendNumbers(ui.comboBox_normalaction_pet_action, sa::MAX_PET_SKILL);

	appendNumbers(ui.comboBox_healaction_pet_action, sa::MAX_PET_SKILL);
	appendNumbers(ui.comboBox_purgaction_pet_action, sa::MAX_PET_SKILL);
	//catch
	appendCharAction(ui.comboBox_autocatchpet_magic);

	appendNumbers(ui.comboBox_autocatchpet_petskill, sa::MAX_PET_SKILL);

	ui.comboBox_autocatchpet_mode->clear();
	ui.comboBox_autocatchpet_mode->addItems(QStringList{ tr("escape from encounter") , tr("engage in encounter") });

	ui.comboBox_autowalkdir->clear();
	ui.comboBox_autowalkdir->addItems(QStringList{ tr("↖↘"), tr("↗↙"), tr("random") });

	updateTargetButtonText();

	ui.tableWidget->onResetControlTextLanguage();
}

void AfkForm::onApplyHashSettingsToUI()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash = gamedevice.getEnablesHash();
	QHash<util::UserSetting, long long> valueHash = gamedevice.getValuesHash();
	QHash<util::UserSetting, QString> stringHash = gamedevice.getStringsHash();

	if (!gamedevice.worker.isNull() && gamedevice.worker->getOnlineFlag())
	{
		QString title = tr("AfkForm");
		QString newTitle = QString("[%1]%2-%3").arg(getIndex()).arg(gamedevice.worker->getCharacter().name).arg(title);
		setWindowTitle(newTitle);
	}

	//battle
	ui.checkBox_magicheal->setChecked(enableHash.value(util::kBattleMagicHealEnable));
	ui.checkBox_itemheal->setChecked(enableHash.value(util::kBattleItemHealEnable));
	ui.checkBox_itemheal_meatpriority->setChecked(enableHash.value(util::kBattleItemHealMeatPriorityEnable));
	ui.checkBox_itemhealmp->setChecked(enableHash.value(util::kBattleItemHealMpEnable));
	ui.checkBox_magicrevive->setChecked(enableHash.value(util::kBattleMagicReviveEnable));
	ui.checkBox_itemrevive->setChecked(enableHash.value(util::kBattleItemReviveEnable));

	ui.checkBox_magicheal_normal->setChecked(enableHash.value(util::kNormalMagicHealEnable));
	ui.checkBox_itemheal_normal->setChecked(enableHash.value(util::kNormalItemHealEnable));
	ui.checkBox_itemheal_normal_meatpriority->setChecked(enableHash.value(util::kNormalItemHealMeatPriorityEnable));
	ui.checkBox_itemhealmp_normal->setChecked(enableHash.value(util::kNormalItemHealMpEnable));
	ui.checkBox_skillMp->setChecked(enableHash.value(util::kBattleSkillMpEnable));

	ui.comboBox_roundaction_char_round->setCurrentIndex(valueHash.value(util::kBattleCharRoundActionRoundValue));
	ui.comboBox_roundaction_char_action->setCurrentIndex(valueHash.value(util::kBattleCharRoundActionTypeValue));
	ui.comboBox_roundaction_char_enemy->setCurrentIndex(valueHash.value(util::kBattleCharRoundActionEnemyValue));
	ui.comboBox_roundaction_char_level->setCurrentIndex(valueHash.value(util::kBattleCharRoundActionLevelValue));

	ui.comboBox_crossaction_char_action->setCurrentIndex(valueHash.value(util::kBattleCharCrossActionTypeValue));
	ui.comboBox_crossaction_char_round->setCurrentIndex(valueHash.value(util::kBattleCharCrossActionRoundValue));

	ui.comboBox_normalaction_char_action->setCurrentIndex(valueHash.value(util::kBattleCharNormalActionTypeValue));
	ui.comboBox_normalaction_char_enemy->setCurrentIndex(valueHash.value(util::kBattleCharNormalActionEnemyValue));
	ui.comboBox_normalaction_char_level->setCurrentIndex(valueHash.value(util::kBattleCharNormalActionLevelValue));

	ui.comboBox_roundaction_pet_round->setCurrentIndex(valueHash.value(util::kBattlePetRoundActionRoundValue));
	ui.comboBox_roundaction_pet_action->setCurrentIndex(valueHash.value(util::kBattlePetRoundActionTypeValue));
	ui.comboBox_roundaction_pet_enemy->setCurrentIndex(valueHash.value(util::kBattlePetRoundActionEnemyValue));
	ui.comboBox_roundaction_pet_level->setCurrentIndex(valueHash.value(util::kBattlePetRoundActionLevelValue));

	ui.comboBox_crossaction_pet_action->setCurrentIndex(valueHash.value(util::kBattlePetCrossActionTypeValue));
	ui.comboBox_crossaction_pet_round->setCurrentIndex(valueHash.value(util::kBattlePetCrossActionRoundValue));

	ui.comboBox_normalaction_pet_action->setCurrentIndex(valueHash.value(util::kBattlePetNormalActionTypeValue));
	ui.comboBox_normalaction_pet_enemy->setCurrentIndex(valueHash.value(util::kBattlePetNormalActionEnemyValue));
	ui.comboBox_normalaction_pet_level->setCurrentIndex(valueHash.value(util::kBattlePetNormalActionLevelValue));

	ui.comboBox_magicheal->setCurrentIndex(valueHash.value(util::kBattleMagicHealMagicValue));
	ui.comboBox_magicrevive->setCurrentIndex(valueHash.value(util::kBattleMagicReviveMagicValue));

	ui.lineEdit_itemheal->setText(stringHash.value(util::kBattleItemHealItemString));
	ui.lineEdit_itemhealmp->setText(stringHash.value(util::kBattleItemHealMpItemString));
	ui.lineEdit_itemrevive->setText(stringHash.value(util::kBattleItemReviveItemString));

	ui.lineEdit_magicheal_normal->setText(stringHash.value(util::kNormalMagicHealItemString));
	ui.comboBox_magicheal_normal->setCurrentIndex(valueHash.value(util::kNormalMagicHealMagicValue));
	ui.lineEdit_itemheal_normal->setText(stringHash.value(util::kNormalItemHealItemString));
	ui.lineEdit_itemhealmp_normal->setText(stringHash.value(util::kNormalItemHealMpItemString));

	ui.checkBox_noscapewhilelockpet->setChecked(enableHash.value(util::kBattleNoEscapeWhileLockPetEnable));

	ui.checkBox_petheal->setChecked(enableHash.value(util::kBattlePetHealEnable));
	ui.comboBox_healaction_pet_action->setCurrentIndex(valueHash.value(util::kBattlePetHealActionTypeValue));
	ui.checkBox_petpurg->setChecked(enableHash.value(util::kBattlePetPurgEnable));
	ui.comboBox_purgaction_pet_action->setCurrentIndex(valueHash.value(util::kBattlePetPurgActionTypeValue));
	ui.checkBox_charpurg->setChecked(enableHash.value(util::kBattleCharPurgEnable));
	ui.comboBox_purgaction_char_action->setCurrentIndex(valueHash.value(util::kBattleCharPurgActionTypeValue));
	//heal
	ui.spinBox_magicheal_char->setValue(valueHash.value(util::kBattleMagicHealCharValue));
	ui.spinBox_magicheal_pet->setValue(valueHash.value(util::kBattleMagicHealPetValue));
	ui.spinBox_magicheal_allie->setValue(valueHash.value(util::kBattleMagicHealAllieValue));
	ui.spinBox_itemheal_char->setValue(valueHash.value(util::kBattleItemHealCharValue));
	ui.spinBox_itemheal_pet->setValue(valueHash.value(util::kBattleItemHealPetValue));
	ui.spinBox_itemheal_allie->setValue(valueHash.value(util::kBattleItemHealAllieValue));
	ui.spinBox_itemhealmp->setValue(valueHash.value(util::kBattleItemHealMpValue));
	ui.spinBox_skillMp->setValue(valueHash.value(util::kBattleSkillMpValue));

	ui.spinBox_magicheal_normal_char->setValue(valueHash.value(util::kNormalMagicHealCharValue));
	ui.spinBox_magicheal_normal_pet->setValue(valueHash.value(util::kNormalMagicHealPetValue));
	ui.spinBox_magicheal_normal_allie->setValue(valueHash.value(util::kNormalMagicHealAllieValue));
	ui.spinBox_itemheal_normal_char->setValue(valueHash.value(util::kNormalItemHealCharValue));
	ui.spinBox_itemheal_normal_pet->setValue(valueHash.value(util::kNormalItemHealPetValue));
	ui.spinBox_itemhealmp_normal->setValue(valueHash.value(util::kNormalItemHealMpValue));

	ui.spinBox_petheal_char->setValue(valueHash.value(util::kBattlePetHealCharValue));
	ui.spinBox_petheal_pet->setValue(valueHash.value(util::kBattlePetHealPetValue));
	ui.spinBox_petheal_allie->setValue(valueHash.value(util::kBattlePetHealAllieValue));
	//walk
	ui.comboBox_autowalkdir->setCurrentIndex(valueHash.value(util::kAutoWalkDirectionValue));
	ui.spinBox_autowalkdelay->setValue(valueHash.value(util::kAutoWalkDelayValue));
	ui.spinBox_autowalklen->setValue(valueHash.value(util::kAutoWalkDistanceValue));
	ui.checkBox_crossaction_char->setChecked(enableHash.value(util::kBattleCrossActionCharEnable));
	ui.checkBox_crossaction_pet->setChecked(enableHash.value(util::kBattleCrossActionPetEnable));

	//catch
	ui.comboBox_autocatchpet_mode->setCurrentIndex(valueHash.value(util::kBattleCatchModeValue));
	ui.comboBox_autocatchpet_magic->setCurrentIndex(valueHash.value(util::kBattleCatchCharMagicValue));
	ui.comboBox_autocatchpet_petskill->setCurrentIndex(valueHash.value(util::kBattleCatchPetSkillValue));
	ui.checkBox_autocatchpet_level->setChecked(enableHash.value(util::kBattleCatchTargetLevelEnable));
	ui.checkBox_autocatchpet_hp->setChecked(enableHash.value(util::kBattleCatchTargetMaxHpEnable));
	ui.checkBox_autocatchpet_magic->setChecked(enableHash.value(util::kBattleCatchCharMagicEnable));
	ui.checkBox_autocatchpet_item->setChecked(enableHash.value(util::kBattleCatchCharItemEnable));
	ui.checkBox_autocatchpet_petskill->setChecked(enableHash.value(util::kBattleCatchPetSkillEnable));
	ui.spinBox_autocatchpet_level->setValue(valueHash.value(util::kBattleCatchTargetLevelValue));
	ui.spinBox_autocatchpet_hp->setValue(valueHash.value(util::kBattleCatchTargetMaxHpValue));
	ui.lineEdit_autocatchpet->setText(stringHash.value(util::kBattleCatchPetNameString));
	ui.lineEdit_autocatchpet_item->setText(stringHash.value(util::kBattleCatchCharItemString));
	ui.spinBox_autocatchpet_magic->setValue(valueHash.value(util::kBattleCatchTargetMagicHpValue));
	ui.spinBox_autocatchpet_item->setValue(valueHash.value(util::kBattleCatchTargetItemHpValue));

	ui.spinBox_rounddelay->setValue(valueHash.value(util::kBattleActionDelayValue));

	//catch->drop
	ui.checkBox_autodroppet->setChecked(enableHash.value(util::kDropPetEnable));
	ui.checkBox_autodroppet_str->setChecked(enableHash.value(util::kDropPetStrEnable));
	ui.checkBox_autodroppet_def->setChecked(enableHash.value(util::kDropPetDefEnable));
	ui.checkBox_autodroppet_agi->setChecked(enableHash.value(util::kDropPetAgiEnable));
	ui.checkBox_autodroppet_hp->setChecked(enableHash.value(util::kDropPetHpEnable));
	ui.checkBox_autodroppet_aggregate->setChecked(enableHash.value(util::kDropPetAggregateEnable));
	ui.spinBox_autodroppet_str->setValue(valueHash.value(util::kDropPetStrValue));
	ui.spinBox_autodroppet_def->setValue(valueHash.value(util::kDropPetDefValue));
	ui.spinBox_autodroppet_agi->setValue(valueHash.value(util::kDropPetAgiValue));
	ui.spinBox_autodroppet_hp->setValue(valueHash.value(util::kDropPetHpValue));
	ui.spinBox_autodroppet_aggregate->setValue(valueHash.value(util::kDropPetAggregateValue));
	ui.lineEdit_autodroppet_name->setText(stringHash.value(util::kDropPetNameString));


	ui.spinBox_resend_delay->setValue(valueHash.value(util::kBattleResendDelayValue));

	QString orderString = stringHash.value(util::kBattleActionOrderString);
	qDebug() << "read order:" << orderString;
	if (!orderString.isEmpty())
	{
		ui.tableWidget->reorderRows(orderString);
	}

	updateTargetButtonText();
}

void AfkForm::onUpdateComboBoxItemText(long long type, const QStringList& textList)
{
	if (isHidden())
		return;

	long long currentIndex = getIndex();

	switch (type)
	{
	case util::kComboBoxCharAction:
	{
		const QStringList normalActionList = {
			tr("attack"), tr("defense"), tr("escape")
		};

		const QStringList equipActionList = {
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
			tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
			tr("gloves")
		};

		auto appendMagicText = [this, &normalActionList, &equipActionList, &textList, currentIndex](QComboBox* combo)->void
			{
				if (combo == nullptr)
					return;

				if (combo->hasFocus())
					return;

				if (combo->view()->hasFocus())
					return;

				combo->blockSignals(true);
				combo->setUpdatesEnabled(false);
				long long nOriginalIndex = combo->currentIndex();

				constexpr long long base = 3; //基礎3個動作
				long long size = sa::MAX_MAGIC + base + sa::MAX_PROFESSION_SKILL;
				long long n = 0;

				//去除多餘的
				if (combo->count() > size)
				{
					for (long long i = static_cast<long long>(combo->count()) - 1; i >= size; --i)
						combo->removeItem(i);
				}

				//更新精靈
				for (long long i = 0; i < size; ++i)
				{
					//缺則補
					if (i >= combo->count())
						combo->addItem("");

					//重設基礎動作
					QString text = "";
					if (i < base)
					{
						text = QString("%1:%2").arg(i + 1).arg(normalActionList.value(i));
					}
					else if (i >= base && i < sa::MAX_MAGIC + 3)
					{
						text = QString("%1:%2:%3").arg(i + 1).arg(equipActionList.value(i - base)).arg(textList.value(i - base));
					}
					else
					{
						text = QString("%1:%3").arg(i + 1).arg(textList.value(i - base));
					}

					combo->setItemText(i, text);
					combo->setItemData(i, text, Qt::ToolTipRole);
					++n;
				}

				combo->setCurrentIndex(nOriginalIndex);
				combo->setUpdatesEnabled(true);
				combo->blockSignals(false);
			};

		//battle
		appendMagicText(ui.comboBox_normalaction_char_action);
		appendMagicText(ui.comboBox_crossaction_char_action);
		appendMagicText(ui.comboBox_roundaction_char_action);
		appendMagicText(ui.comboBox_magicheal);
		appendMagicText(ui.comboBox_magicrevive);
		appendMagicText(ui.comboBox_magicheal_normal);

		//appendProfText(ui.comboBox_skillMp);
		appendMagicText(ui.comboBox_purgaction_char_action);

		//catch
		appendMagicText(ui.comboBox_autocatchpet_magic);
		break;
	}
	case util::kComboBoxPetAction:
	{
		auto appendText = [&textList](QComboBox* combo, long long max, bool noIndex)->void
			{
				if (combo == nullptr)
					return;

				if (combo->hasFocus())
					return;

				if (combo->view()->hasFocus())
					return;

				combo->blockSignals(true);
				combo->setUpdatesEnabled(false);
				long long nOriginalIndex = combo->currentIndex();

				long long size = textList.size();
				long long n = 0;

				if (combo->count() > max)
				{
					for (long long i = static_cast<long long>(combo->count()) - 1; i >= max; --i)
						combo->removeItem(i);
				}

				for (long long i = 0; i < max; ++i)
				{
					if (i >= combo->count())
						combo->addItem("");

					long long index = 0;
					QString text;
					if (!noIndex)
					{
						if (i >= size)
						{
							combo->setItemText(i, QString("%1:").arg(i + 1));
							//index = static_cast<long long>(combo->count()) - 1;
							combo->setItemData(i, QString("%1:").arg(i + 1), Qt::ToolTipRole);
							continue;
						}
						text = QString("%1:%2").arg(i + 1).arg(textList[i]);
					}
					else
					{
						if (i >= size)
							break;
						text = textList[n];
						++n;
					}

					combo->setItemText(i, text);
					index = static_cast<long long>(combo->count()) - 1;
					combo->setItemData(index, text, Qt::ToolTipRole);
				}

				combo->setCurrentIndex(nOriginalIndex);
				combo->setUpdatesEnabled(true);
				combo->blockSignals(false);
			};

		//battle
		appendText(ui.comboBox_normalaction_pet_action, sa::MAX_PET_SKILL, false);
		appendText(ui.comboBox_crossaction_pet_action, sa::MAX_PET_SKILL, false);
		appendText(ui.comboBox_roundaction_pet_action, sa::MAX_PET_SKILL, false);

		appendText(ui.comboBox_healaction_pet_action, sa::MAX_PET_SKILL, false);
		appendText(ui.comboBox_purgaction_pet_action, sa::MAX_PET_SKILL, false);
		//catch
		appendText(ui.comboBox_autocatchpet_petskill, sa::MAX_PET_SKILL, false);
		break;
	}
	case util::kComboBoxItem:
	{

		break;
	}
	}
}

void AfkForm::updateTargetButtonText()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	QHash<util::UserSetting, long long> valueHash = gamedevice.getValuesHash();

	auto get = [](unsigned long long value)->QString { return SelectTargetForm::generateShortName(value); };

	ui.pushButton_roundaction_char->setText(get(valueHash.value(util::kBattleCharRoundActionTargetValue)));
	ui.pushButton_crossaction_char->setText(get(valueHash.value(util::kBattleCharCrossActionTargetValue)));
	ui.pushButton_normalaction_char->setText(get(valueHash.value(util::kBattleCharNormalActionTargetValue)));

	ui.pushButton_roundaction_pet->setText(get(valueHash.value(util::kBattlePetRoundActionTargetValue)));
	ui.pushButton_crossaction_pet->setText(get(valueHash.value(util::kBattlePetCrossActionTargetValue)));
	ui.pushButton_normalaction_pet->setText(get(valueHash.value(util::kBattlePetNormalActionTargetValue)));

	ui.pushButton_magicheal->setText(get(valueHash.value(util::kBattleMagicHealTargetValue)));
	ui.pushButton_magicrevive->setText(get(valueHash.value(util::kBattleItemReviveTargetValue)));
	ui.pushButton_itemheal->setText(get(valueHash.value(util::kBattleItemHealTargetValue)));
	ui.pushButton_itemrevive->setText(get(valueHash.value(util::kBattleItemReviveTargetValue)));

	ui.pushButton_petheal->setText(get(valueHash.value(util::kBattlePetHealTargetValue)));
	ui.pushButton_petpurg->setText(get(valueHash.value(util::kBattlePetPurgTargetValue)));
	ui.pushButton_charpurg->setText(get(valueHash.value(util::kBattleCharPurgTargetValue)));
}

void AfkForm::onDragDropWidgetItemChanged(const QStringList& order)
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.setStringHash(util::kBattleActionOrderString, order.join("|"));
}
