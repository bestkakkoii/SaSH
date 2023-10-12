
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
import Config;
#include "stdafx.h"
#include "afkform.h"
#include <injector.h>
#include "signaldispatcher.h"
#include "selecttargetform.h"
#include "selectobjectform.h"

AfkForm::AfkForm(__int64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	Qt::WindowFlags windowflag = this->windowFlags();
	windowflag |= Qt::WindowType::Tool;
	setWindowFlag(Qt::WindowType::Tool);

	connect(this, &AfkForm::resetControlTextLanguage, this, &AfkForm::onResetControlTextLanguage, Qt::QueuedConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &AfkForm::onApplyHashSettingsToUI, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateComboBoxItemText, this, &AfkForm::onUpdateComboBoxItemText, Qt::QueuedConnection);

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &AfkForm::onButtonClicked, Qt::QueuedConnection);
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox)
			connect(checkBox, &QCheckBox::stateChanged, this, &AfkForm::onCheckBoxStateChanged, Qt::QueuedConnection);
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox)
			connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::QueuedConnection);
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::QueuedConnection);
	}

	QList <ComboBox*> ccomboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : ccomboBoxList)
	{
		if (comboBox)
		{
			connect(comboBox, &ComboBox::clicked, this, &AfkForm::onComboBoxClicked, Qt::QueuedConnection);
			connect(comboBox, &ComboBox::currentTextChanged, this, &AfkForm::onComboBoxTextChanged, Qt::QueuedConnection);
		}
	}

	onResetControlTextLanguage();

	Injector& injector = Injector::getInstance(index);
	if (!injector.server.isNull())
		injector.server->updateComboBoxList();

	FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	emit signalDispatcher.applyHashSettingsToUI();
}

AfkForm::~AfkForm()
{

}

void AfkForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);

	Injector& injector = Injector::getInstance(getIndex());
	if (!injector.server.isNull())
		injector.server->updateComboBoxList();
}

void AfkForm::closeEvent(QCloseEvent* event)
{
	FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();

	QWidget::closeEvent(event);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//battle
	if (name == "pushButton_roundaction_char")
	{
		if (createSelectTargetForm(currentIndex, kBattleCharRoundActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_crossaction_char")
	{
		if (createSelectTargetForm(currentIndex, kBattleCharCrossActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_normalaction_char")
	{
		if (createSelectTargetForm(currentIndex, kBattleCharNormalActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}

	if (name == "pushButton_roundaction_pet")
	{
		if (createSelectTargetForm(currentIndex, kBattlePetRoundActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_crossaction_pet")
	{
		if (createSelectTargetForm(currentIndex, kBattlePetCrossActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_normalaction_pet")
	{
		if (createSelectTargetForm(currentIndex, kBattlePetNormalActionTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}

	//heal
	if (name == "pushButton_magicheal")
	{
		if (createSelectTargetForm(currentIndex, kBattleMagicHealTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_itemheal")
	{
		if (createSelectTargetForm(currentIndex, kBattleItemHealTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_magicrevive")
	{
		if (createSelectTargetForm(currentIndex, kBattleMagicReviveTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_itemrevive")
	{
		if (createSelectTargetForm(currentIndex, kBattleItemReviveTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_petheal")
	{
		if (createSelectTargetForm(currentIndex, kBattlePetHealTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_petpurg")
	{
		if (createSelectTargetForm(currentIndex, kBattlePetPurgTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	if (name == "pushButton_charpurg")
	{
		if (createSelectTargetForm(currentIndex, kBattleCharPurgTargetValue, &temp, this))
			pPushButton->setText(temp);
		return;
	}
	//catch
	if (name == "pushButton_autocatchpet")
	{
		QVariant dat = injector.getUserData(kUserEnemyNames);
		if (dat.isValid())
		{
			srcSelectList = dat.toStringList();
		}
		srcSelectList.removeDuplicates();

		QString src = ui.lineEdit_autocatchpet->text();
		if (src.isEmpty())
		{
			src = injector.getStringHash(kBattleCatchPetNameString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoCatch, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(kBattleCatchPetNameString, dst);
		ui.lineEdit_autocatchpet->setText(dst);
		return;
	}
	if (name == "pushButton_autodroppet")
	{
		QVariant dat = injector.getUserData(kUserEnemyNames);
		if (dat.isValid())
		{
			srcSelectList = dat.toStringList();
		}

		dat = injector.getUserData(kUserPetNames);
		if (dat.isValid())
		{
			srcSelectList.append(dat.toStringList());
		}
		srcSelectList.removeDuplicates();

		QString src = ui.lineEdit_autodroppet_name->text();
		if (src.isEmpty())
		{
			src = injector.getStringHash(kDropPetNameString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoDropPet, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(kDropPetNameString, dst);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//battle
	if (name == "checkBox_crossaction_char")
	{
		injector.setEnableHash(kBattleCrossActionCharEnable, isChecked);
		return;
	}
	if (name == "checkBox_crossaction_pet")
	{
		injector.setEnableHash(kBattleCrossActionPetEnable, isChecked);
		return;
	}

	if (name == "checkBox_noscapewhilelockpet")
	{
		injector.setEnableHash(kBattleNoEscapeWhileLockPetEnable, isChecked);
		return;
	}

	//battle heal
	if (name == "checkBox_magicheal")
	{
		injector.setEnableHash(kBattleMagicHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal")
	{
		injector.setEnableHash(kBattleItemHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal_meatpriority")
	{
		injector.setEnableHash(kBattleItemHealMeatPriorityEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemhealmp")
	{
		injector.setEnableHash(kBattleItemHealMpEnable, isChecked);
		return;
	}

	if (name == "checkBox_magicrevive")
	{
		injector.setEnableHash(kBattleMagicReviveEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemrevive")
	{
		injector.setEnableHash(kBattleItemReviveEnable, isChecked);
		return;
	}

	if (name == "checkBox_skillMp")
	{
		injector.setEnableHash(kBattleSkillMpEnable, isChecked);
		return;
	}

	if (name == "checkBox_petheal")
	{
		injector.setEnableHash(kBattlePetHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_petpurg")
	{
		injector.setEnableHash(kBattlePetPurgEnable, isChecked);
		return;
	}

	if (name == "checkBox_charpurg")
	{
		injector.setEnableHash(kBattleCharPurgEnable, isChecked);
		return;
	}

	//normal heal
	if (name == "checkBox_magicheal_normal")
	{
		injector.setEnableHash(kNormalMagicHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal_normal")
	{
		injector.setEnableHash(kNormalItemHealEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemheal_normal_meatpriority")
	{
		injector.setEnableHash(kNormalItemHealMeatPriorityEnable, isChecked);
		return;
	}

	if (name == "checkBox_itemhealmp_normal")
	{
		injector.setEnableHash(kNormalItemHealMpEnable, isChecked);
		return;
	}

	//catch
	if (name == "checkBox_autocatchpet_level")
	{
		injector.setEnableHash(kBattleCatchTargetLevelEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_hp")
	{
		injector.setEnableHash(kBattleCatchTargetMaxHpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_magic")
	{
		injector.setEnableHash(kBattleCatchCharMagicEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_item")
	{
		injector.setEnableHash(kBattleCatchCharItemEnable, isChecked);
		return;
	}

	if (name == "checkBox_autocatchpet_petskill")
	{
		injector.setEnableHash(kBattleCatchPetSkillEnable, isChecked);
		return;
	}

	//catch->drop
	if (name == "checkBox_autodroppet")
	{
		injector.setEnableHash(kDropPetEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_str")
	{
		injector.setEnableHash(kDropPetStrEnable, isChecked);
		return;
	}

	if (name == "spinBox_autodroppet_def")
	{
		injector.setEnableHash(kDropPetDefEnable, isChecked);
		return;
	}

	if (name == "spinBox_autodroppet_agi")
	{
		injector.setEnableHash(kDropPetAgiEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_hp")
	{
		injector.setEnableHash(kDropPetHpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_hp")
	{
		injector.setEnableHash(kDropPetHpEnable, isChecked);
		return;
	}

	if (name == "checkBox_autodroppet_aggregate")
	{
		injector.setEnableHash(kDropPetAggregateEnable, isChecked);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//battle heal
	if (name == "spinBox_magicheal_char")
	{
		injector.setValueHash(kBattleMagicHealCharValue, value);
		return;
	}

	if (name == "spinBox_magicheal_pet")
	{
		injector.setValueHash(kBattleMagicHealPetValue, value);
		return;
	}

	if (name == "spinBox_magicheal_allie")
	{
		injector.setValueHash(kBattleMagicHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemheal_char")
	{
		injector.setValueHash(kBattleItemHealCharValue, value);
		return;
	}

	if (name == "spinBox_itemheal_pet")
	{
		injector.setValueHash(kBattleItemHealPetValue, value);
		return;
	}

	if (name == "spinBox_itemheal_allie")
	{
		injector.setValueHash(kBattleItemHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemhealmp")
	{
		injector.setValueHash(kBattleItemHealMpValue, value);
		return;
	}

	if (name == "spinBox_skillMp")
	{
		injector.setValueHash(kBattleSkillMpValue, value);
		return;
	}

	if (name == "spinBox_petheal_char")
	{
		injector.setValueHash(kBattlePetHealCharValue, value);
		return;
	}

	if (name == "spinBox_petheal_pet")
	{
		injector.setValueHash(kBattlePetHealPetValue, value);
		return;
	}

	if (name == "spinBox_petheal_allie")
	{
		injector.setValueHash(kBattlePetHealAllieValue, value);
		return;
	}
	//normal heal
	if (name == "spinBox_magicheal_normal_char")
	{
		injector.setValueHash(kNormalMagicHealCharValue, value);
		return;
	}

	if (name == "spinBox_magicheal_normal_pet")
	{
		injector.setValueHash(kNormalMagicHealPetValue, value);
		return;
	}

	if (name == "spinBox_magicheal_normal_allie")
	{
		injector.setValueHash(kNormalMagicHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemheal_normal_char")
	{
		injector.setValueHash(kNormalItemHealCharValue, value);
		return;
	}

	if (name == "spinBox_itemheal_normal_pet")
	{
		injector.setValueHash(kNormalItemHealPetValue, value);
		return;
	}

	if (name == "spinBox_itemheal_normal_allie")
	{
		injector.setValueHash(kNormalItemHealAllieValue, value);
		return;
	}

	if (name == "spinBox_itemhealmp_normal")
	{
		injector.setValueHash(kNormalItemHealMpValue, value);
		return;
	}

	//autowalk
	if (name == "spinBox_autowalkdelay")
	{
		injector.setValueHash(kAutoWalkDelayValue, value);
		return;
	}

	if (name == "spinBox_autowalklen")
	{
		injector.setValueHash(kAutoWalkDistanceValue, value);
		return;
	}

	//catch
	if (name == "spinBox_autocatchpet_level")
	{
		injector.setValueHash(kBattleCatchTargetLevelValue, value);
		return;
	}

	if (name == "spinBox_autocatchpet_hp")
	{
		injector.setValueHash(kBattleCatchTargetMaxHpValue, value);
		return;
	}

	if (name == "spinBox_autocatchpet_magic")
	{
		injector.setValueHash(kBattleCatchTargetMagicHpValue, value);
		return;
	}

	if (name == "spinBox_autocatchpet_item")
	{
		injector.setValueHash(kBattleCatchTargetItemHpValue, value);
		return;
	}

	//catch->drop
	if (name == "spinBox_autodroppet_str")
	{
		injector.setValueHash(kDropPetStrValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_def")
	{
		injector.setValueHash(kDropPetDefValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_agi")
	{
		injector.setValueHash(kDropPetAgiValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_hp")
	{
		injector.setValueHash(kDropPetHpValue, value);
		return;
	}

	if (name == "spinBox_autodroppet_aggregate")
	{
		injector.setValueHash(kDropPetAggregateValue, value);
		return;
	}

	if (name == "spinBox_rounddelay")
	{
		injector.setValueHash(kBattleActionDelayValue, value);
		return;
	}

	if (name == "spinBox_resend_delay")
	{
		injector.setValueHash(kBattleResendDelayValue, value);
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

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//battle char
	if (name == "comboBox_roundaction_char_round")
	{
		injector.setValueHash(kBattleCharRoundActionRoundValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_char_action")
	{
		injector.setValueHash(kBattleCharRoundActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_char_enemy")
	{
		injector.setValueHash(kBattleCharRoundActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_char_level")
	{
		injector.setValueHash(kBattleCharRoundActionLevelValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_crossaction_char_action")
	{
		injector.setValueHash(kBattleCharCrossActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_crossaction_char_round")
	{
		injector.setValueHash(kBattleCharCrossActionRoundValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_normalaction_char_action")
	{
		injector.setValueHash(kBattleCharNormalActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_char_enemy")
	{
		injector.setValueHash(kBattleCharNormalActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_char_level")
	{
		injector.setValueHash(kBattleCharNormalActionLevelValue, value != -1 ? value : 0);
		return;
	}


	//battle pet
	if (name == "comboBox_roundaction_pet_round")
	{
		injector.setValueHash(kBattlePetRoundActionRoundValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_pet_action")
	{
		injector.setValueHash(kBattlePetRoundActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_pet_enemy")
	{
		injector.setValueHash(kBattlePetRoundActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_roundaction_pet_level")
	{
		injector.setValueHash(kBattlePetRoundActionLevelValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_crossaction_pet_action")
	{
		injector.setValueHash(kBattlePetCrossActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_crossaction_pet_round")
	{
		injector.setValueHash(kBattlePetCrossActionRoundValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_normalaction_pet_action")
	{
		injector.setValueHash(kBattlePetNormalActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_pet_enemy")
	{
		injector.setValueHash(kBattlePetNormalActionEnemyValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_normalaction_pet_level")
	{
		injector.setValueHash(kBattlePetNormalActionLevelValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_healaction_pet_action")
	{
		injector.setValueHash(kBattlePetHealActionTypeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_purgaction_pet_action")
	{
		injector.setValueHash(kBattlePetPurgActionTypeValue, value != -1 ? value : 0);
		return;
	}
	//magic purg
	if (name == "comboBox_purgaction_char_action")
	{
		injector.setValueHash(kBattleCharPurgActionTypeValue, value != -1 ? value : 0);
		return;
	}
	//magic heal
	if (name == "comboBox_magicheal")
	{
		injector.setValueHash(kBattleMagicHealMagicValue, value != -1 ? value : 0);
		return;
	}

	if (name == "comboBox_magicrevive")
	{
		injector.setValueHash(kBattleMagicReviveMagicValue, value != -1 ? value : 0);
		return;
	}

	//normal
	if (name == "comboBox_magicheal_normal")
	{
		injector.setValueHash(kNormalMagicHealMagicValue, value != -1 ? value : 0);
		return;
	}

	//walk
	if (name == "comboBox_autowalkdir")
	{
		injector.setValueHash(kAutoWalkDirectionValue, value != -1 ? value : 0);
		return;
	}

	//catch
	if (name == "comboBox_autocatchpet_mode")
	{
		injector.setValueHash(kBattleCatchModeValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_autocatchpet_magic")
	{
		injector.setValueHash(kBattleCatchCharMagicValue, value != -1 ? value : 0);
		return;
	}
	if (name == "comboBox_autocatchpet_petskill")
	{
		injector.setValueHash(kBattleCatchPetSkillValue, value != -1 ? value : 0);
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

	UserSetting settingType = kSettingNotUsed;

	//battle
	do
	{
		if (name == "comboBox_itemheal")
		{
			settingType = kBattleItemHealItemString;
			break;
		}
		if (name == "comboBox_itemhealmp")
		{
			settingType = kBattleItemHealMpItemString;
			break;
		}
		if (name == "comboBox_itemrevive")
		{
			settingType = kBattleItemReviveItemString;
			break;
		}
		//normal
		if (name == "comboBox_itemheal_normal")
		{
			settingType = kNormalItemHealItemString;
			break;
		}
		if (name == "comboBox_itemhealmp_normal")
		{
			settingType = kNormalItemHealMpItemString;
			break;
		}

		//catch
		if (name == "comboBox_autocatchpet_item")
		{
			settingType = kBattleCatchCharItemString;
			break;
		}
	} while (false);

	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return;

	if (name.contains("item"))
	{
		QString currentText = pComboBox->currentText();
		pComboBox->clear();
		QStringList itemList = injector.getUserData(kUserItemNames).toStringList();
		itemList.prepend(currentText);
		itemList.removeDuplicates();
		pComboBox->addItems(itemList);
	}
	else
	{
		injector.server->updateComboBoxList();
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
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	//battle
	if (name == "comboBox_itemheal")
	{
		injector.setStringHash(kBattleItemHealItemString, newText);
		return;
	}
	if (name == "comboBox_itemhealmp")
	{
		injector.setStringHash(kBattleItemHealMpItemString, newText);
		return;
	}
	if (name == "comboBox_itemrevive")
	{
		injector.setStringHash(kBattleItemReviveItemString, newText);
		return;
	}
	//normal
	if (name == "comboBox_itemheal_normal")
	{
		injector.setStringHash(kNormalItemHealItemString, newText);
		return;
	}
	if (name == "comboBox_itemhealmp_normal")
	{
		injector.setStringHash(kNormalItemHealMpItemString, newText);
		return;
	}
}

void AfkForm::onResetControlTextLanguage()
{
	auto appendRound = [](QComboBox* combo)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->clear();
		combo->addItem(tr("not use"));
		for (__int64 i = 1; i <= 100; ++i)
		{
			QString text = tr("at round %1").arg(i);
			combo->addItem(text);
			__int64 index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendEnemyAmount = [](QComboBox* combo)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->clear();
		combo->addItem(tr("not use"));
		for (__int64 i = 1; i <= 9; ++i)
		{
			QString text = tr("enemy amount > %1").arg(i);
			combo->addItem(text);
			__int64 index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendCrossRound = [](QComboBox* combo)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->clear();
		for (__int64 i = 1; i <= 100; ++i)
		{
			QString text = tr("every %1 round").arg(i);
			combo->addItem(text);
			__int64 index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendEnemyLevel = [](QComboBox* combo)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->clear();
		combo->addItem(tr("not use"));
		for (__int64 i = 10; i <= 250; i += 10)
		{
			QString text = tr("enemy level > %1").arg(i);
			combo->addItem(text);
			__int64 index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendCharAction = [](QComboBox* combo, bool notBattle = false)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->clear();
		QStringList actionList = {
			tr("attack"), tr("defense"), tr("escape"),
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
			tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
			tr("gloves")
		};

		for (__int64 i = 0; i < actionList.size(); ++i)
		{
			if (notBattle && i < 3)
				continue;

			combo->addItem(actionList[i] + (i >= 3 ? ":" : ""));
			__int64 index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, QString("%1").arg(actionList[i]), Qt::ToolTipRole);
		}

		if (notBattle)
			return;

		for (__int64 i = 0; i < MAX_PROFESSION_SKILL; ++i)
		{
			QString text = QString("%1:").arg(i + 1);
			combo->addItem(text);
			__int64 index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendNumbers = [](QComboBox* combo, __int64 max)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->clear();
		for (__int64 i = 1; i <= max; ++i)
		{
			combo->addItem(QString("%1:").arg(i));
			__int64 index = static_cast<__int64>(combo->count()) - 1;
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

	appendCharAction(ui.comboBox_magicheal_normal, true);
	for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
	{
		QString text = QString("%1:").arg(i + 1 - CHAR_EQUIPPLACENUM);
		ui.comboBox_magicheal_normal->addItem(text);
		__int64 index = static_cast<__int64>(ui.comboBox_magicheal_normal->count()) - 1;
		ui.comboBox_magicheal_normal->setItemData(index, text, Qt::ToolTipRole);
	}

	//battle
	appendNumbers(ui.comboBox_roundaction_pet_action, MAX_SKILL);
	appendNumbers(ui.comboBox_crossaction_pet_action, MAX_SKILL);
	appendNumbers(ui.comboBox_normalaction_pet_action, MAX_SKILL);

	appendNumbers(ui.comboBox_healaction_pet_action, MAX_SKILL);
	appendNumbers(ui.comboBox_purgaction_pet_action, MAX_SKILL);
	//catch
	appendCharAction(ui.comboBox_autocatchpet_magic);

	appendNumbers(ui.comboBox_autocatchpet_petskill, MAX_SKILL);

	ui.comboBox_autocatchpet_mode->clear();
	ui.comboBox_autocatchpet_mode->addItems(QStringList{ tr("escape from encounter") , tr("engage in encounter") });

	ui.comboBox_autowalkdir->clear();
	ui.comboBox_autowalkdir->addItems(QStringList{ tr("↖↘"), tr("↗↙"), tr("random") });

	updateTargetButtonText();
}

void AfkForm::onApplyHashSettingsToUI()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QHash<UserSetting, bool> enableHash = injector.getEnablesHash();
	QHash<UserSetting, __int64> valueHash = injector.getValuesHash();
	QHash<UserSetting, QString> stringHash = injector.getStringsHash();

	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		QString title = tr("AfkForm");
		QString newTitle = QString("[%1] %2").arg(injector.server->getPC().name).arg(title);
		setWindowTitle(newTitle);
	}

	//battle
	ui.checkBox_magicheal->setChecked(enableHash.value(kBattleMagicHealEnable));
	ui.checkBox_itemheal->setChecked(enableHash.value(kBattleItemHealEnable));
	ui.checkBox_itemheal_meatpriority->setChecked(enableHash.value(kBattleItemHealMeatPriorityEnable));
	ui.checkBox_itemhealmp->setChecked(enableHash.value(kBattleItemHealMpEnable));
	ui.checkBox_magicrevive->setChecked(enableHash.value(kBattleMagicReviveEnable));
	ui.checkBox_itemrevive->setChecked(enableHash.value(kBattleItemReviveEnable));

	ui.checkBox_magicheal_normal->setChecked(enableHash.value(kNormalMagicHealEnable));
	ui.checkBox_itemheal_normal->setChecked(enableHash.value(kNormalItemHealEnable));
	ui.checkBox_itemheal_normal_meatpriority->setChecked(enableHash.value(kNormalItemHealMeatPriorityEnable));
	ui.checkBox_itemhealmp_normal->setChecked(enableHash.value(kNormalItemHealMpEnable));
	ui.checkBox_skillMp->setChecked(enableHash.value(kBattleSkillMpEnable));

	ui.comboBox_roundaction_char_round->setCurrentIndex(valueHash.value(kBattleCharRoundActionRoundValue));
	ui.comboBox_roundaction_char_action->setCurrentIndex(valueHash.value(kBattleCharRoundActionTypeValue));
	ui.comboBox_roundaction_char_enemy->setCurrentIndex(valueHash.value(kBattleCharRoundActionEnemyValue));
	ui.comboBox_roundaction_char_level->setCurrentIndex(valueHash.value(kBattleCharRoundActionLevelValue));

	ui.comboBox_crossaction_char_action->setCurrentIndex(valueHash.value(kBattleCharCrossActionTypeValue));
	ui.comboBox_crossaction_char_round->setCurrentIndex(valueHash.value(kBattleCharCrossActionRoundValue));

	ui.comboBox_normalaction_char_action->setCurrentIndex(valueHash.value(kBattleCharNormalActionTypeValue));
	ui.comboBox_normalaction_char_enemy->setCurrentIndex(valueHash.value(kBattleCharNormalActionEnemyValue));
	ui.comboBox_normalaction_char_level->setCurrentIndex(valueHash.value(kBattleCharNormalActionLevelValue));

	ui.comboBox_roundaction_pet_round->setCurrentIndex(valueHash.value(kBattlePetRoundActionRoundValue));
	ui.comboBox_roundaction_pet_action->setCurrentIndex(valueHash.value(kBattlePetRoundActionTypeValue));
	ui.comboBox_roundaction_pet_enemy->setCurrentIndex(valueHash.value(kBattlePetRoundActionEnemyValue));
	ui.comboBox_roundaction_pet_level->setCurrentIndex(valueHash.value(kBattlePetRoundActionLevelValue));

	ui.comboBox_crossaction_pet_action->setCurrentIndex(valueHash.value(kBattlePetCrossActionTypeValue));
	ui.comboBox_crossaction_pet_round->setCurrentIndex(valueHash.value(kBattlePetCrossActionRoundValue));

	ui.comboBox_normalaction_pet_action->setCurrentIndex(valueHash.value(kBattlePetNormalActionTypeValue));
	ui.comboBox_normalaction_pet_enemy->setCurrentIndex(valueHash.value(kBattlePetNormalActionEnemyValue));
	ui.comboBox_normalaction_pet_level->setCurrentIndex(valueHash.value(kBattlePetNormalActionLevelValue));

	ui.comboBox_magicheal->setCurrentIndex(valueHash.value(kBattleMagicHealMagicValue));
	ui.comboBox_magicrevive->setCurrentIndex(valueHash.value(kBattleMagicReviveMagicValue));

	ui.comboBox_itemheal->setCurrentText(stringHash.value(kBattleItemHealItemString));
	ui.comboBox_itemhealmp->setCurrentText(stringHash.value(kBattleItemHealMpItemString));
	ui.comboBox_itemrevive->setCurrentText(stringHash.value(kBattleItemReviveItemString));
	ui.comboBox_itemheal_normal->setCurrentText(stringHash.value(kNormalItemHealItemString));
	ui.comboBox_itemhealmp_normal->setCurrentText(stringHash.value(kNormalItemHealMpItemString));
	ui.comboBox_magicheal_normal->setCurrentIndex(valueHash.value(kNormalMagicHealMagicValue));

	ui.checkBox_noscapewhilelockpet->setChecked(enableHash.value(kBattleNoEscapeWhileLockPetEnable));

	ui.checkBox_petheal->setChecked(enableHash.value(kBattlePetHealEnable));
	ui.comboBox_healaction_pet_action->setCurrentIndex(valueHash.value(kBattlePetHealActionTypeValue));
	ui.checkBox_petpurg->setChecked(enableHash.value(kBattlePetPurgEnable));
	ui.comboBox_purgaction_pet_action->setCurrentIndex(valueHash.value(kBattlePetPurgActionTypeValue));
	ui.checkBox_charpurg->setChecked(enableHash.value(kBattleCharPurgEnable));
	ui.comboBox_purgaction_char_action->setCurrentIndex(valueHash.value(kBattleCharPurgActionTypeValue));
	//heal
	ui.spinBox_magicheal_char->setValue(valueHash.value(kBattleMagicHealCharValue));
	ui.spinBox_magicheal_pet->setValue(valueHash.value(kBattleMagicHealPetValue));
	ui.spinBox_magicheal_allie->setValue(valueHash.value(kBattleMagicHealAllieValue));
	ui.spinBox_itemheal_char->setValue(valueHash.value(kBattleItemHealCharValue));
	ui.spinBox_itemheal_pet->setValue(valueHash.value(kBattleItemHealPetValue));
	ui.spinBox_itemheal_allie->setValue(valueHash.value(kBattleItemHealAllieValue));
	ui.spinBox_itemhealmp->setValue(valueHash.value(kBattleItemHealMpValue));
	ui.spinBox_skillMp->setValue(valueHash.value(kBattleSkillMpValue));

	ui.spinBox_magicheal_normal_char->setValue(valueHash.value(kNormalMagicHealCharValue));
	ui.spinBox_magicheal_normal_pet->setValue(valueHash.value(kNormalMagicHealPetValue));
	ui.spinBox_magicheal_normal_allie->setValue(valueHash.value(kNormalMagicHealAllieValue));
	ui.spinBox_itemheal_normal_char->setValue(valueHash.value(kNormalItemHealCharValue));
	ui.spinBox_itemheal_normal_pet->setValue(valueHash.value(kNormalItemHealPetValue));
	ui.spinBox_itemhealmp_normal->setValue(valueHash.value(kNormalItemHealMpValue));

	ui.spinBox_petheal_char->setValue(valueHash.value(kBattlePetHealCharValue));
	ui.spinBox_petheal_pet->setValue(valueHash.value(kBattlePetHealPetValue));
	ui.spinBox_petheal_allie->setValue(valueHash.value(kBattlePetHealAllieValue));
	//walk
	ui.comboBox_autowalkdir->setCurrentIndex(valueHash.value(kAutoWalkDirectionValue));
	ui.spinBox_autowalkdelay->setValue(valueHash.value(kAutoWalkDelayValue));
	ui.spinBox_autowalklen->setValue(valueHash.value(kAutoWalkDistanceValue));
	ui.checkBox_crossaction_char->setChecked(enableHash.value(kBattleCrossActionCharEnable));
	ui.checkBox_crossaction_pet->setChecked(enableHash.value(kBattleCrossActionPetEnable));

	//catch
	ui.comboBox_autocatchpet_mode->setCurrentIndex(valueHash.value(kBattleCatchModeValue));
	ui.comboBox_autocatchpet_magic->setCurrentIndex(valueHash.value(kBattleCatchCharMagicValue));
	ui.comboBox_autocatchpet_petskill->setCurrentIndex(valueHash.value(kBattleCatchPetSkillValue));
	ui.checkBox_autocatchpet_level->setChecked(enableHash.value(kBattleCatchTargetLevelEnable));
	ui.checkBox_autocatchpet_hp->setChecked(enableHash.value(kBattleCatchTargetMaxHpEnable));
	ui.checkBox_autocatchpet_magic->setChecked(enableHash.value(kBattleCatchCharMagicEnable));
	ui.checkBox_autocatchpet_item->setChecked(enableHash.value(kBattleCatchCharItemEnable));
	ui.checkBox_autocatchpet_petskill->setChecked(enableHash.value(kBattleCatchPetSkillEnable));
	ui.spinBox_autocatchpet_level->setValue(valueHash.value(kBattleCatchTargetLevelValue));
	ui.spinBox_autocatchpet_hp->setValue(valueHash.value(kBattleCatchTargetMaxHpValue));
	ui.lineEdit_autocatchpet->setText(stringHash.value(kBattleCatchPetNameString));
	ui.comboBox_autocatchpet_item->setCurrentText(stringHash.value(kBattleCatchCharItemString));
	ui.spinBox_autocatchpet_magic->setValue(valueHash.value(kBattleCatchTargetMagicHpValue));
	ui.spinBox_autocatchpet_item->setValue(valueHash.value(kBattleCatchTargetItemHpValue));

	ui.spinBox_rounddelay->setValue(valueHash.value(kBattleActionDelayValue));

	//catch->drop
	ui.checkBox_autodroppet->setChecked(enableHash.value(kDropPetEnable));
	ui.checkBox_autodroppet_str->setChecked(enableHash.value(kDropPetStrEnable));
	ui.checkBox_autodroppet_def->setChecked(enableHash.value(kDropPetDefEnable));
	ui.checkBox_autodroppet_agi->setChecked(enableHash.value(kDropPetAgiEnable));
	ui.checkBox_autodroppet_hp->setChecked(enableHash.value(kDropPetHpEnable));
	ui.checkBox_autodroppet_aggregate->setChecked(enableHash.value(kDropPetAggregateEnable));
	ui.spinBox_autodroppet_str->setValue(valueHash.value(kDropPetStrValue));
	ui.spinBox_autodroppet_def->setValue(valueHash.value(kDropPetDefValue));
	ui.spinBox_autodroppet_agi->setValue(valueHash.value(kDropPetAgiValue));
	ui.spinBox_autodroppet_hp->setValue(valueHash.value(kDropPetHpValue));
	ui.spinBox_autodroppet_aggregate->setValue(valueHash.value(kDropPetAggregateValue));
	ui.lineEdit_autodroppet_name->setText(stringHash.value(kDropPetNameString));


	ui.spinBox_resend_delay->setValue(valueHash.value(kBattleResendDelayValue));

	updateTargetButtonText();
}

void AfkForm::onUpdateComboBoxItemText(__int64 type, const QStringList& textList)
{
	if (isHidden())
		return;

	__int64 currentIndex = getIndex();

	auto appendText = [&textList](QComboBox* combo, __int64 max, bool noIndex)->void
	{
		if (combo == nullptr)
			return;

		if (combo->hasFocus())
			return;

		if (combo->view()->hasFocus())
			return;

		combo->blockSignals(true);
		combo->setUpdatesEnabled(false);
		__int64 nOriginalIndex = combo->currentIndex();
		combo->clear();
		__int64 size = textList.size();
		__int64 n = 0;
		for (__int64 i = 0; i < max; ++i)
		{
			__int64 index = 0;
			QString text;
			if (!noIndex)
			{
				if (i >= size)
				{
					combo->addItem(QString("%1:").arg(i + 1));
					index = static_cast<__int64>(combo->count()) - 1;
					combo->setItemData(index, QString("%1:").arg(i + 1), Qt::ToolTipRole);
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

			combo->addItem(text);
			index = static_cast<__int64>(combo->count()) - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}


		combo->setCurrentIndex(nOriginalIndex);
		combo->setUpdatesEnabled(true);
		combo->blockSignals(false);
	};

	switch (type)
	{
	case kComboBoxCharAction:
	{
		QStringList actionList = {
			tr("attack"), tr("defense"), tr("escape"),
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
			tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
			tr("gloves")
		};

		auto appendMagicText = [this, &actionList, &textList, currentIndex](QComboBox* combo, bool notBattle = false)->void
		{
			if (combo == nullptr)
				return;

			if (combo->hasFocus())
				return;

			if (combo->view()->hasFocus())
				return;

			combo->blockSignals(true);
			combo->setUpdatesEnabled(false);
			__int64 nOriginalIndex = combo->currentIndex();
			combo->clear();
			__int64 size = actionList.size();
			__int64 n = 0;
			for (__int64 i = 0; i < size; ++i)
			{

				QString text;
				if (i < 3)
				{
					if (notBattle)
						continue;
					text = actionList[i];
					combo->addItem(text);
				}
				else
				{
					text = QString("%1:%2").arg(actionList[i]).arg(textList[i - 3]);
					combo->addItem(text);
				}

				__int64 index = static_cast<__int64>(combo->count()) - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
				++n;
			}

			__int64 textListSize = textList.size();
			for (__int64 i = size - 3; i < textListSize; ++i)
			{
				if (notBattle)
					continue;

				combo->addItem(QString("%1:%2").arg(i - size + 4).arg(textList[i]));
				++n;
			}


			Injector& injector = Injector::getInstance(currentIndex);
			if (!injector.server.isNull() && injector.server->getOnlineFlag() && combo == ui.comboBox_magicheal_normal)
			{

				QHash<__int64, ITEM> items = injector.server->getItems();
				for (__int64 i = CHAR_EQUIPPLACENUM; i < MAX_ITEM; ++i)
				{
					ITEM item = items.value(i);
					QString text = QString("%1:%2").arg(i - CHAR_EQUIPPLACENUM + 1).arg(item.name);
					combo->addItem(text);
					__int64 index = combo->count() - 1;
					combo->setItemData(index, text, Qt::ToolTipRole);
				}

			}

			combo->setCurrentIndex(nOriginalIndex);
			combo->setUpdatesEnabled(true);
			combo->blockSignals(false);
		};

		//battle
		appendMagicText(ui.comboBox_normalaction_char_action);
		appendMagicText(ui.comboBox_crossaction_char_action);
		appendMagicText(ui.comboBox_roundaction_char_action);
		appendMagicText(ui.comboBox_magicheal, true);
		appendMagicText(ui.comboBox_magicrevive, true);
		appendMagicText(ui.comboBox_magicheal_normal, true);

		//appendProfText(ui.comboBox_skillMp);
		appendMagicText(ui.comboBox_purgaction_char_action, true);

		//catch
		appendMagicText(ui.comboBox_autocatchpet_magic);
		break;
	}
	case kComboBoxPetAction:
	{
		//battle
		appendText(ui.comboBox_normalaction_pet_action, MAX_SKILL, false);
		appendText(ui.comboBox_crossaction_pet_action, MAX_SKILL, false);
		appendText(ui.comboBox_roundaction_pet_action, MAX_SKILL, false);

		appendText(ui.comboBox_healaction_pet_action, MAX_SKILL, false);
		appendText(ui.comboBox_purgaction_pet_action, MAX_SKILL, false);
		//catch
		appendText(ui.comboBox_autocatchpet_petskill, MAX_SKILL, false);
		break;
	}
	case kComboBoxItem:
	{
		//appendText(ui.comboBox_itemheal, MAX_ITEM, true);
		//appendText(ui.comboBox_itemhealmp, MAX_ITEM, true);
		//appendText(ui.comboBox_itemheal_normal, MAX_ITEM, true);
		//appendText(ui.comboBox_itemhealmp_normal, MAX_ITEM, true);
		break;
	}
	}
}

void AfkForm::updateTargetButtonText()
{
	__int64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QHash<UserSetting, __int64> valueHash = injector.getValuesHash();

	auto get = [](quint64 value)->QString { return SelectTargetForm::generateShortName(value); };

	ui.pushButton_roundaction_char->setText(get(valueHash.value(kBattleCharRoundActionTargetValue)));
	ui.pushButton_crossaction_char->setText(get(valueHash.value(kBattleCharCrossActionTargetValue)));
	ui.pushButton_normalaction_char->setText(get(valueHash.value(kBattleCharNormalActionTargetValue)));

	ui.pushButton_roundaction_pet->setText(get(valueHash.value(kBattlePetRoundActionTargetValue)));
	ui.pushButton_crossaction_pet->setText(get(valueHash.value(kBattlePetCrossActionTargetValue)));
	ui.pushButton_normalaction_pet->setText(get(valueHash.value(kBattlePetNormalActionTargetValue)));

	ui.pushButton_magicheal->setText(get(valueHash.value(kBattleMagicHealTargetValue)));
	ui.pushButton_magicrevive->setText(get(valueHash.value(kBattleItemReviveTargetValue)));
	ui.pushButton_itemheal->setText(get(valueHash.value(kBattleItemHealTargetValue)));
	ui.pushButton_itemrevive->setText(get(valueHash.value(kBattleItemReviveTargetValue)));

	ui.pushButton_petheal->setText(get(valueHash.value(kBattlePetHealTargetValue)));
	ui.pushButton_petpurg->setText(get(valueHash.value(kBattlePetPurgTargetValue)));
	ui.pushButton_charpurg->setText(get(valueHash.value(kBattleCharPurgTargetValue)));
}
