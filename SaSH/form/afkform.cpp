#include "stdafx.h"
#include "afkform.h"
#include <util.h>
#include <injector.h>

#include "signaldispatcher.h"
#include "selecttargetform.h"
#include "selectobjectform.h"

AfkForm::AfkForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	//util::setTab(ui.tabWidget_afk);

	connect(this, &AfkForm::resetControlTextLanguage, this, &AfkForm::onResetControlTextLanguage, Qt::UniqueConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &AfkForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateComboBoxItemText, this, &AfkForm::onUpdateComboBoxItemText, Qt::UniqueConnection);

	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &QPushButton::clicked, this, &AfkForm::onButtonClicked, Qt::UniqueConnection);
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox)
			connect(checkBox, &QCheckBox::stateChanged, this, &AfkForm::onCheckBoxStateChanged, Qt::UniqueConnection);
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox)
			connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::UniqueConnection);
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
	}

	QList <ComboBox*> ccomboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : ccomboBoxList)
	{
		if (comboBox)
		{
			connect(comboBox, &ComboBox::clicked, this, &AfkForm::onComboBoxClicked, Qt::UniqueConnection);
			connect(comboBox, &ComboBox::currentTextChanged, this, &AfkForm::onComboBoxTextChanged, Qt::UniqueConnection);
		}
	}
	onResetControlTextLanguage();


	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	emit signalDispatcher.applyHashSettingsToUI();
}

AfkForm::~AfkForm()
{

}

void AfkForm::closeEvent(QCloseEvent* event)
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();

	QWidget::closeEvent(event);
}

void AfkForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	QString temp;
	QStringList srcList;
	QStringList dstList;
	QStringList srcSelectList;

	Injector& injector = Injector::getInstance();

	//battle
	if (name == "pushButton_roundaction_char")
	{
		if (createSelectTargetForm(util::kBattleCharRoundActionTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_crossaction_char")
	{
		if (createSelectTargetForm(util::kBattleCharCrossActionTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_normalaction_char")
	{
		if (createSelectTargetForm(util::kBattleCharNormalActionTargetValue, &temp, this))
			pPushButton->setText(temp);
	}

	if (name == "pushButton_roundaction_pet")
	{
		if (createSelectTargetForm(util::kBattlePetRoundActionTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_crossaction_pet")
	{
		if (createSelectTargetForm(util::kBattlePetCrossActionTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_normalaction_pet")
	{
		if (createSelectTargetForm(util::kBattlePetNormalActionTargetValue, &temp, this))
			pPushButton->setText(temp);
	}

	//heal
	else if (name == "pushButton_magicheal")
	{
		if (createSelectTargetForm(util::kBattleMagicHealTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_itemheal")
	{
		if (createSelectTargetForm(util::kBattleItemHealTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_magicrevive")
	{
		if (createSelectTargetForm(util::kBattleMagicReviveTargetValue, &temp, this))
			pPushButton->setText(temp);
	}
	else if (name == "pushButton_itemrevive")
	{
		if (createSelectTargetForm(util::kBattleItemReviveTargetValue, &temp, this))
			pPushButton->setText(temp);
	}

	//catch
	else if (name == "pushButton_autocatchpet")
	{
		QVariant dat = injector.getUserData(util::kUserEnemyNames);
		if (dat.isValid())
		{
			srcSelectList = dat.toStringList();
		}
		srcSelectList.removeDuplicates();

		QString src = ui.lineEdit_autocatchpet->text();
		if (src.isEmpty())
		{
			src = injector.getStringHash(util::kBattleCatchPetNameString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoCatch, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(util::kBattleCatchPetNameString, dst);
		ui.lineEdit_autocatchpet->setText(dst);
	}
	else if (name == "pushButton_autodroppet")
	{
		QVariant dat = injector.getUserData(util::kUserEnemyNames);
		if (dat.isValid())
		{
			srcSelectList = dat.toStringList();
		}

		dat = injector.getUserData(util::kUserPetNames);
		if (dat.isValid())
		{
			srcSelectList.append(dat.toStringList());
		}
		srcSelectList.removeDuplicates();

		QString src = ui.lineEdit_autodroppet_name->text();
		if (src.isEmpty())
		{
			src = injector.getStringHash(util::kDropPetNameString);
		}

		if (!src.isEmpty())
		{
			srcList = src.split(util::rexOR, Qt::SkipEmptyParts);
		}

		if (!createSelectObjectForm(SelectObjectForm::kAutoDropPet, srcSelectList, srcList, &dstList, this))
			return;

		QString dst = dstList.join("|");
		injector.setStringHash(util::kDropPetNameString, dst);
		ui.lineEdit_autodroppet_name->setText(dst);
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

	Injector& injector = Injector::getInstance();

	//battle
	if (name == "checkBox_crossaction_char")
	{
		injector.setEnableHash(util::kCrossActionCharEnable, isChecked);
	}
	else if (name == "checkBox_crossaction_pet")
	{
		injector.setEnableHash(util::kCrossActionPetEnable, isChecked);
	}

	//battle heal
	else if (name == "checkBox_magicheal")
	{
		injector.setEnableHash(util::kBattleMagicHealEnable, isChecked);
	}
	else if (name == "checkBox_itemheal")
	{
		injector.setEnableHash(util::kBattleItemHealEnable, isChecked);
	}
	else if (name == "checkBox_itemheal_meatpriority")
	{
		injector.setEnableHash(util::kBattleItemHealMeatPriorityEnable, isChecked);
	}
	else if (name == "checkBox_itemhealmp")
	{
		injector.setEnableHash(util::kBattleItemHealMpEnable, isChecked);
	}
	else if (name == "checkBox_magicrevive")
	{
		injector.setEnableHash(util::kBattleMagicReviveEnable, isChecked);
	}
	else if (name == "checkBox_itemrevive")
	{
		injector.setEnableHash(util::kBattleItemReviveEnable, isChecked);
	}

	//normal heal
	else if (name == "checkBox_magicheal_normal")
	{
		injector.setEnableHash(util::kNormalMagicHealEnable, isChecked);
	}
	else if (name == "checkBox_itemheal_normal")
	{
		injector.setEnableHash(util::kNormalItemHealEnable, isChecked);
	}
	else if (name == "checkBox_itemheal_normal_meatpriority")
	{
		injector.setEnableHash(util::kNormalItemHealMeatPriorityEnable, isChecked);
	}
	else if (name == "checkBox_itemhealmp_normal")
	{
		injector.setEnableHash(util::kNormalItemHealMpEnable, isChecked);
	}

	//catch
	else if (name == "checkBox_autocatchpet_level")
	{
		injector.setEnableHash(util::kBattleCatchTargetLevelEnable, isChecked);
	}
	else if (name == "checkBox_autocatchpet_hp")
	{
		injector.setEnableHash(util::kBattleCatchTargetMaxHpEnable, isChecked);
	}
	else if (name == "checkBox_autocatchpet_magic")
	{
		injector.setEnableHash(util::kBattleCatchPlayerMagicEnable, isChecked);
	}
	else if (name == "checkBox_autocatchpet_item")
	{
		injector.setEnableHash(util::kBattleCatchPlayerItemEnable, isChecked);
	}
	else if (name == "checkBox_autocatchpet_petskill")
	{
		injector.setEnableHash(util::kBattleCatchPetSkillEnable, isChecked);
	}

	//catch->drop
	else if (name == "checkBox_autodroppet")
	{
		injector.setEnableHash(util::kDropPetEnable, isChecked);
	}
	else if (name == "checkBox_autodroppet_str")
	{
		injector.setEnableHash(util::kDropPetStrEnable, isChecked);
	}
	else if (name == "spinBox_autodroppet_def")
	{
		injector.setEnableHash(util::kDropPetDefEnable, isChecked);
	}
	else if (name == "spinBox_autodroppet_agi")
	{
		injector.setEnableHash(util::kDropPetAgiEnable, isChecked);
	}
	else if (name == "checkBox_autodroppet_hp")
	{
		injector.setEnableHash(util::kDropPetHpEnable, isChecked);
	}
	else if (name == "checkBox_autodroppet_hp")
	{
		injector.setEnableHash(util::kDropPetHpEnable, isChecked);
	}
	else if (name == "checkBox_autodroppet_aggregate")
	{
		injector.setEnableHash(util::kDropPetAggregateEnable, isChecked);
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

	Injector& injector = Injector::getInstance();

	//battle heal
	if (name == "spinBox_magicheal_char")
	{
		injector.setValueHash(util::kBattleMagicHealCharValue, value);
	}
	else if (name == "spinBox_magicheal_pet")
	{
		injector.setValueHash(util::kBattleMagicHealPetValue, value);
	}
	else if (name == "spinBox_magicheal_allie")
	{
		injector.setValueHash(util::kBattleMagicHealAllieValue, value);
	}
	//
	else if (name == "spinBox_itemheal_char")
	{
		injector.setValueHash(util::kBattleItemHealCharValue, value);
	}
	else if (name == "spinBox_itemheal_pet")
	{
		injector.setValueHash(util::kBattleItemHealPetValue, value);
	}
	else if (name == "spinBox_itemheal_allie")
	{
		injector.setValueHash(util::kBattleItemHealAllieValue, value);
	}
	else if (name == "spinBox_itemhealmp")
	{
		injector.setValueHash(util::kBattleItemHealMpValue, value);
	}

	//normal heal
	else if (name == "spinBox_magicheal_normal_char")
	{
		injector.setValueHash(util::kNormalMagicHealCharValue, value);
	}
	else if (name == "spinBox_magicheal_normal_pet")
	{
		injector.setValueHash(util::kNormalMagicHealPetValue, value);
	}
	else if (name == "spinBox_magicheal_normal_allie")
	{
		injector.setValueHash(util::kNormalMagicHealAllieValue, value);
	}
	//
	else if (name == "spinBox_itemheal_normal_char")
	{
		injector.setValueHash(util::kNormalItemHealCharValue, value);
	}
	else if (name == "spinBox_itemheal_normal_pet")
	{
		injector.setValueHash(util::kNormalItemHealPetValue, value);
	}
	else if (name == "spinBox_itemheal_normal_allie")
	{
		injector.setValueHash(util::kNormalItemHealAllieValue, value);
	}
	else if (name == "spinBox_itemhealmp_normal")
	{
		injector.setValueHash(util::kNormalItemHealMpValue, value);
	}

	//autowalk
	else if (name == "spinBox_autowalkdelay")
	{
		injector.setValueHash(util::kAutoWalkDelayValue, value);
	}

	else if (name == "spinBox_autowalklen")
	{
		injector.setValueHash(util::kAutoWalkDistanceValue, value);
	}

	//catch
	else if (name == "spinBox_autocatchpet_level")
	{
		injector.setValueHash(util::kBattleCatchTargetLevelValue, value);
	}
	else if (name == "spinBox_autocatchpet_hp")
	{
		injector.setValueHash(util::kBattleCatchTargetMaxHpValue, value);
	}
	else if (name == "spinBox_autocatchpet_magic")
	{
		injector.setValueHash(util::kBattleCatchTargetMagicHpValue, value);
	}
	else if (name == "spinBox_autocatchpet_item")
	{
		injector.setValueHash(util::kBattleCatchTargetItemHpValue, value);
	}

	//catch->drop
	else if (name == "spinBox_autodroppet_str")
	{
		injector.setValueHash(util::kDropPetStrValue, value);
	}
	else if (name == "spinBox_autodroppet_def")
	{
		injector.setValueHash(util::kDropPetDefValue, value);
	}
	else if (name == "spinBox_autodroppet_agi")
	{
		injector.setValueHash(util::kDropPetAgiValue, value);
	}
	else if (name == "spinBox_autodroppet_hp")
	{
		injector.setValueHash(util::kDropPetHpValue, value);
	}
	else if (name == "spinBox_autodroppet_aggregate")
	{
		injector.setValueHash(util::kDropPetAggregateValue, value);
	}

	else if (name == "spinBox_rounddelay")
	{
		injector.setValueHash(util::kBattleActionDelayValue, value);
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

	Injector& injector = Injector::getInstance();

	//battle char
	if (name == "comboBox_roundaction_char_round")
	{
		injector.setValueHash(util::kBattleCharRoundActionRoundValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_roundaction_char_action")
	{
		injector.setValueHash(util::kBattleCharRoundActionTypeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_roundaction_char_enemy")
	{
		injector.setValueHash(util::kBattleCharRoundActionEnemyValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_roundaction_char_level")
	{
		injector.setValueHash(util::kBattleCharRoundActionLevelValue, value != -1 ? value : 0);
	}

	else if (name == "comboBox_crossaction_char_action")
	{
		injector.setValueHash(util::kBattleCharCrossActionTypeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_crossaction_char_round")
	{
		injector.setValueHash(util::kBattleCharCrossActionRoundValue, value != -1 ? value : 0);
	}

	else if (name == "comboBox_normalaction_char_action")
	{
		injector.setValueHash(util::kBattleCharNormalActionTypeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_normalaction_char_enemy")
	{
		injector.setValueHash(util::kBattleCharNormalActionEnemyValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_normalaction_char_level")
	{
		injector.setValueHash(util::kBattleCharNormalActionLevelValue, value != -1 ? value : 0);
	}


	//battle pet
	else if (name == "comboBox_roundaction_pet_round")
	{
		injector.setValueHash(util::kBattlePetRoundActionRoundValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_roundaction_pet_action")
	{
		injector.setValueHash(util::kBattlePetRoundActionTypeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_roundaction_pet_enemy")
	{
		injector.setValueHash(util::kBattlePetRoundActionEnemyValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_roundaction_pet_level")
	{
		injector.setValueHash(util::kBattlePetRoundActionLevelValue, value != -1 ? value : 0);
	}

	else if (name == "comboBox_crossaction_pet_action")
	{
		injector.setValueHash(util::kBattlePetCrossActionTypeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_crossaction_pet_round")
	{
		injector.setValueHash(util::kBattlePetCrossActionRoundValue, value != -1 ? value : 0);
	}

	else if (name == "comboBox_normalaction_pet_action")
	{
		injector.setValueHash(util::kBattlePetNormalActionTypeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_normalaction_pet_enemy")
	{
		injector.setValueHash(util::kBattlePetNormalActionEnemyValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_normalaction_pet_level")
	{
		injector.setValueHash(util::kBattlePetNormalActionLevelValue, value != -1 ? value : 0);
	}

	//magic heal
	else if (name == "comboBox_magicheal")
	{
		injector.setValueHash(util::kBattleMagicHealMagicValue, value != -1 ? value : 0);
	}

	else if (name == "comboBox_magicrevive")
	{
		injector.setValueHash(util::kBattleMagicReviveMagicValue, value != -1 ? value : 0);
	}

	//normal
	else if (name == "comboBox_magicheal_normal")
	{
		injector.setValueHash(util::kNormalMagicHealMagicValue, value != -1 ? value : 0);
	}

	//walk
	else if (name == "comboBox_autowalkdir")
	{
		injector.setValueHash(util::kAutoWalkDirectionValue, value != -1 ? value : 0);
	}

	//catch
	else if (name == "comboBox_autocatchpet_mode")
	{
		injector.setValueHash(util::kBattleCatchModeValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_autocatchpet_magic")
	{
		injector.setValueHash(util::kBattleCatchPlayerMagicValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_autocatchpet_petskill")
	{
		injector.setValueHash(util::kBattleCatchPetSkillValue, value != -1 ? value : 0);
	}
}

void AfkForm::onComboBoxClicked()
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();


	util::UserSetting settingType = util::kSettingNotUsed;

	//battle
	if (name == "comboBox_itemheal")
	{
		settingType = util::kBattleItemHealItemString;
	}
	else if (name == "comboBox_itemhealmp")
	{
		settingType = util::kBattleItemHealMpItemString;
	}
	else if (name == "comboBox_itemrevive")
	{
		settingType = util::kBattleItemReviveItemString;
	}
	//normal
	else if (name == "comboBox_itemheal_normal")
	{
		settingType = util::kNormalItemHealItemString;
	}
	else if (name == "comboBox_itemhealmp_normal")
	{
		settingType = util::kNormalItemHealMpItemString;
	}

	//catch
	else if (name == "comboBox_autocatchpet_item")
	{
		settingType = util::kBattleCatchPlayerItemString;
	}

	if (settingType == util::kSettingNotUsed)
		return;

	QString currentText = pComboBox->currentText();
	pComboBox->clear();
	QStringList itemList = injector.getUserData(util::kUserItemNames).toStringList();
	itemList.prepend(currentText);
	//清除重複
	itemList.removeDuplicates();
	pComboBox->addItems(itemList);
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

	Injector& injector = Injector::getInstance();

	//battle
	if (name == "comboBox_itemheal")
	{
		injector.setStringHash(util::kBattleItemHealItemString, newText);
	}
	else if (name == "comboBox_itemhealmp")
	{
		injector.setStringHash(util::kBattleItemHealMpItemString, newText);
	}
	else if (name == "comboBox_itemrevive")
	{
		injector.setStringHash(util::kBattleItemReviveItemString, newText);
	}
	//normal
	else if (name == "comboBox_itemheal_normal")
	{
		injector.setStringHash(util::kNormalItemHealItemString, newText);
	}
	else if (name == "comboBox_itemhealmp_normal")
	{
		injector.setStringHash(util::kNormalItemHealMpItemString, newText);
	}
}

void AfkForm::onResetControlTextLanguage()
{

	auto appendRound = [](QComboBox* combo)->void
	{

		combo->clear();
		combo->addItem(tr("not use"));
		for (int i = 1; i <= 20; ++i)
		{
			QString text = tr("at round %1").arg(i);
			combo->addItem(text);
			int index = combo->count() - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendEnemyAmount = [](QComboBox* combo)->void
	{

		combo->clear();
		combo->addItem(tr("not use"));
		for (int i = 1; i <= 9; ++i)
		{
			QString text = tr("enemy amount > %1").arg(i);
			combo->addItem(text);
			int index = combo->count() - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendCrossRound = [](QComboBox* combo)->void
	{

		combo->clear();
		for (int i = 1; i <= 20; ++i)
		{
			QString text = tr("every %1 round").arg(i);
			combo->addItem(text);
			int index = combo->count() - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendEnemyLevel = [](QComboBox* combo)->void
	{

		combo->clear();
		combo->addItem(tr("not use"));
		for (int i = 10; i <= 250; i += 10)
		{
			QString text = tr("enemy level > %1").arg(i);
			combo->addItem(text);
			int index = combo->count() - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
	};

	auto appendCharAction = [](QComboBox* combo, bool notBattle = false)->void
	{

		combo->clear();
		QStringList actionList = {
			tr("attack"), tr("defense"), tr("escape"),
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
			tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
			tr("gloves")
		};

		for (int i = 0; i < actionList.size(); ++i)
		{
			if (notBattle && i < 3)
				continue;

			combo->addItem(actionList[i] + (i >= 3 ? ":" : ""));
			int index = combo->count() - 1;
			combo->setItemData(index, QString("%1").arg(actionList[i]), Qt::ToolTipRole);
		}
	};

	auto appendNumbers = [](QComboBox* combo, int max)->void
	{

		combo->clear();
		for (int i = 1; i <= max; ++i)
		{
			combo->addItem(QString("%1:").arg(i));
			int index = combo->count() - 1;
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
	appendCharAction(ui.comboBox_magicheal, true);
	appendCharAction(ui.comboBox_magicrevive, true);
	appendCharAction(ui.comboBox_magicheal_normal, true);


	//battle
	appendNumbers(ui.comboBox_roundaction_pet_action, MAX_SKILL);
	appendNumbers(ui.comboBox_crossaction_pet_action, MAX_SKILL);
	appendNumbers(ui.comboBox_normalaction_pet_action, MAX_SKILL);

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
	Injector& injector = Injector::getInstance();

	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		QString title = tr("AfkForm");
		QString newTitle = QString("[%1] %2").arg(injector.server->pc.name).arg(title);
		setWindowTitle(newTitle);
	}


	//battle
	ui.checkBox_magicheal->setChecked(injector.getEnableHash(util::kBattleMagicHealEnable));
	ui.checkBox_itemheal->setChecked(injector.getEnableHash(util::kBattleItemHealEnable));
	ui.checkBox_itemheal_meatpriority->setChecked(injector.getEnableHash(util::kBattleItemHealMeatPriorityEnable));
	ui.checkBox_itemhealmp->setChecked(injector.getEnableHash(util::kBattleItemHealMpEnable));
	ui.checkBox_magicrevive->setChecked(injector.getEnableHash(util::kBattleMagicReviveEnable));
	ui.checkBox_itemrevive->setChecked(injector.getEnableHash(util::kBattleItemReviveEnable));

	ui.checkBox_magicheal_normal->setChecked(injector.getEnableHash(util::kNormalMagicHealEnable));
	ui.checkBox_itemheal_normal->setChecked(injector.getEnableHash(util::kNormalItemHealEnable));
	ui.checkBox_itemheal_normal_meatpriority->setChecked(injector.getEnableHash(util::kNormalItemHealMeatPriorityEnable));
	ui.checkBox_itemhealmp_normal->setChecked(injector.getEnableHash(util::kNormalItemHealMpEnable));

	ui.comboBox_roundaction_char_round->setCurrentIndex(injector.getValueHash(util::kBattleCharRoundActionRoundValue));
	ui.comboBox_roundaction_char_action->setCurrentIndex(injector.getValueHash(util::kBattleCharRoundActionTypeValue));
	ui.comboBox_roundaction_char_enemy->setCurrentIndex(injector.getValueHash(util::kBattleCharRoundActionEnemyValue));
	ui.comboBox_roundaction_char_level->setCurrentIndex(injector.getValueHash(util::kBattleCharRoundActionLevelValue));

	ui.comboBox_crossaction_char_action->setCurrentIndex(injector.getValueHash(util::kBattleCharCrossActionTypeValue));
	ui.comboBox_crossaction_char_round->setCurrentIndex(injector.getValueHash(util::kBattleCharCrossActionRoundValue));

	ui.comboBox_normalaction_char_action->setCurrentIndex(injector.getValueHash(util::kBattleCharNormalActionTypeValue));
	ui.comboBox_normalaction_char_enemy->setCurrentIndex(injector.getValueHash(util::kBattleCharNormalActionEnemyValue));
	ui.comboBox_normalaction_char_level->setCurrentIndex(injector.getValueHash(util::kBattleCharNormalActionLevelValue));

	ui.comboBox_roundaction_pet_round->setCurrentIndex(injector.getValueHash(util::kBattlePetRoundActionRoundValue));
	ui.comboBox_roundaction_pet_action->setCurrentIndex(injector.getValueHash(util::kBattlePetRoundActionTypeValue));
	ui.comboBox_roundaction_pet_enemy->setCurrentIndex(injector.getValueHash(util::kBattlePetRoundActionEnemyValue));
	ui.comboBox_roundaction_pet_level->setCurrentIndex(injector.getValueHash(util::kBattlePetRoundActionLevelValue));

	ui.comboBox_crossaction_pet_action->setCurrentIndex(injector.getValueHash(util::kBattlePetCrossActionTypeValue));
	ui.comboBox_crossaction_pet_round->setCurrentIndex(injector.getValueHash(util::kBattlePetCrossActionRoundValue));

	ui.comboBox_normalaction_pet_action->setCurrentIndex(injector.getValueHash(util::kBattlePetNormalActionTypeValue));
	ui.comboBox_normalaction_pet_enemy->setCurrentIndex(injector.getValueHash(util::kBattlePetNormalActionEnemyValue));
	ui.comboBox_normalaction_pet_level->setCurrentIndex(injector.getValueHash(util::kBattlePetNormalActionLevelValue));

	ui.comboBox_magicheal->setCurrentIndex(injector.getValueHash(util::kBattleMagicHealMagicValue));

	ui.comboBox_itemheal->setCurrentText(injector.getStringHash(util::kBattleItemHealItemString));
	ui.comboBox_itemhealmp->setCurrentText(injector.getStringHash(util::kBattleItemHealMpItemString));
	ui.comboBox_itemrevive->setCurrentText(injector.getStringHash(util::kBattleItemReviveItemString));
	ui.comboBox_itemheal_normal->setCurrentText(injector.getStringHash(util::kNormalItemHealItemString));
	ui.comboBox_itemhealmp_normal->setCurrentText(injector.getStringHash(util::kNormalItemHealMpItemString));

	//heal
	ui.spinBox_magicheal_char->setValue(injector.getValueHash(util::kBattleMagicHealCharValue));
	ui.spinBox_magicheal_pet->setValue(injector.getValueHash(util::kBattleMagicHealPetValue));
	ui.spinBox_magicheal_allie->setValue(injector.getValueHash(util::kBattleMagicHealAllieValue));
	ui.spinBox_itemheal_char->setValue(injector.getValueHash(util::kBattleItemHealCharValue));
	ui.spinBox_itemheal_pet->setValue(injector.getValueHash(util::kBattleItemHealPetValue));
	ui.spinBox_itemheal_allie->setValue(injector.getValueHash(util::kBattleItemHealAllieValue));

	ui.spinBox_magicheal_normal_char->setValue(injector.getValueHash(util::kNormalMagicHealCharValue));
	ui.spinBox_magicheal_normal_pet->setValue(injector.getValueHash(util::kNormalMagicHealPetValue));
	ui.spinBox_magicheal_normal_allie->setValue(injector.getValueHash(util::kNormalMagicHealAllieValue));
	ui.spinBox_itemheal_normal_char->setValue(injector.getValueHash(util::kNormalItemHealCharValue));
	ui.spinBox_itemheal_normal_pet->setValue(injector.getValueHash(util::kNormalItemHealPetValue));
	ui.spinBox_itemhealmp_normal->setValue(injector.getValueHash(util::kNormalItemHealMpValue));

	//walk
	ui.comboBox_autowalkdir->setCurrentIndex(injector.getValueHash(util::kAutoWalkDirectionValue));
	ui.spinBox_autowalkdelay->setValue(injector.getValueHash(util::kAutoWalkDelayValue));
	ui.spinBox_autowalklen->setValue(injector.getValueHash(util::kAutoWalkDistanceValue));
	ui.checkBox_crossaction_char->setChecked(injector.getEnableHash(util::kCrossActionCharEnable));
	ui.checkBox_crossaction_pet->setChecked(injector.getEnableHash(util::kCrossActionPetEnable));

	//catch
	ui.comboBox_autocatchpet_mode->setCurrentIndex(injector.getValueHash(util::kBattleCatchModeValue));
	ui.comboBox_autocatchpet_magic->setCurrentIndex(injector.getValueHash(util::kBattleCatchPlayerMagicValue));
	ui.comboBox_autocatchpet_petskill->setCurrentIndex(injector.getValueHash(util::kBattleCatchPetSkillValue));
	ui.checkBox_autocatchpet_level->setChecked(injector.getEnableHash(util::kBattleCatchTargetLevelEnable));
	ui.checkBox_autocatchpet_hp->setChecked(injector.getEnableHash(util::kBattleCatchTargetMaxHpEnable));
	ui.checkBox_autocatchpet_magic->setChecked(injector.getEnableHash(util::kBattleCatchPlayerMagicEnable));
	ui.checkBox_autocatchpet_item->setChecked(injector.getEnableHash(util::kBattleCatchPlayerItemEnable));
	ui.checkBox_autocatchpet_petskill->setChecked(injector.getEnableHash(util::kBattleCatchPetSkillEnable));
	ui.spinBox_autocatchpet_level->setValue(injector.getValueHash(util::kBattleCatchTargetLevelValue));
	ui.spinBox_autocatchpet_hp->setValue(injector.getValueHash(util::kBattleCatchTargetMaxHpValue));
	ui.lineEdit_autocatchpet->setText(injector.getStringHash(util::kBattleCatchPetNameString));
	ui.comboBox_autocatchpet_item->setCurrentText(injector.getStringHash(util::kBattleCatchPlayerItemString));
	ui.spinBox_autocatchpet_magic->setValue(injector.getValueHash(util::kBattleCatchTargetMagicHpValue));
	ui.spinBox_autocatchpet_item->setValue(injector.getValueHash(util::kBattleCatchTargetItemHpValue));

	ui.spinBox_rounddelay->setValue(injector.getValueHash(util::kBattleActionDelayValue));

	//catch->drop
	ui.checkBox_autodroppet->setChecked(injector.getEnableHash(util::kDropPetEnable));
	ui.checkBox_autodroppet_str->setChecked(injector.getEnableHash(util::kDropPetStrEnable));
	ui.checkBox_autodroppet_def->setChecked(injector.getEnableHash(util::kDropPetDefEnable));
	ui.checkBox_autodroppet_agi->setChecked(injector.getEnableHash(util::kDropPetAgiEnable));
	ui.checkBox_autodroppet_hp->setChecked(injector.getEnableHash(util::kDropPetHpEnable));
	ui.checkBox_autodroppet_aggregate->setChecked(injector.getEnableHash(util::kDropPetAggregateEnable));
	ui.spinBox_autodroppet_str->setValue(injector.getValueHash(util::kDropPetStrValue));
	ui.spinBox_autodroppet_def->setValue(injector.getValueHash(util::kDropPetDefValue));
	ui.spinBox_autodroppet_agi->setValue(injector.getValueHash(util::kDropPetAgiValue));
	ui.spinBox_autodroppet_hp->setValue(injector.getValueHash(util::kDropPetHpValue));
	ui.spinBox_autodroppet_aggregate->setValue(injector.getValueHash(util::kDropPetAggregateValue));
	ui.lineEdit_autodroppet_name->setText(injector.getStringHash(util::kDropPetNameString));

	updateTargetButtonText();
}

void AfkForm::onUpdateComboBoxItemText(int type, const QStringList& textList)
{
	auto appendText = [&textList](QComboBox* combo, int max, bool noIndex)->void
	{
		combo->blockSignals(true);
		int nOriginalIndex = combo->currentIndex();
		combo->clear();
		int size = textList.size();
		int n = 0;
		for (int i = 0; i < max; ++i)
		{
			int index = 0;
			QString text;
			if (!noIndex)
			{
				if (i >= size)
				{
					combo->addItem(QString("%1:").arg(i + 1));
					index = combo->count() - 1;
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
			index = combo->count() - 1;
			combo->setItemData(index, text, Qt::ToolTipRole);
		}
		combo->blockSignals(false);
		combo->setCurrentIndex(nOriginalIndex);
	};

	switch (type)
	{
	case util::kComboBoxCharAction:
	{
		QStringList actionList = {
			tr("attack"), tr("defense"), tr("escape"),
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
			tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
			tr("gloves")
		};

		auto appendMagicText = [&actionList, &textList](QComboBox* combo, bool notBattle = false)->void
		{
			combo->blockSignals(true);
			int nOriginalIndex = combo->currentIndex();
			combo->clear();
			int size = actionList.size();
			for (int i = 0; i < size; ++i)
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

				int index = combo->count() - 1;
				combo->setItemData(index, text, Qt::ToolTipRole);
			}

			int textListSize = textList.size();
			for (int i = size - 3; i < textListSize; ++i)
			{
				if (notBattle)
					continue;

				combo->addItem(QString("%1:%2").arg(i - size + 4).arg(textList[i]));
			}

			combo->blockSignals(false);
			combo->setCurrentIndex(nOriginalIndex);
		};

		//battle
		appendMagicText(ui.comboBox_normalaction_char_action);
		appendMagicText(ui.comboBox_crossaction_char_action);
		appendMagicText(ui.comboBox_roundaction_char_action);
		appendMagicText(ui.comboBox_magicheal, true);
		appendMagicText(ui.comboBox_magicrevive, true);
		appendMagicText(ui.comboBox_magicheal_normal, true);

		//catch
		appendMagicText(ui.comboBox_autocatchpet_magic);
		break;
	}
	case util::kComboBoxPetAction:
	{
		//battle
		appendText(ui.comboBox_normalaction_pet_action, MAX_SKILL, false);
		appendText(ui.comboBox_crossaction_pet_action, MAX_SKILL, false);
		appendText(ui.comboBox_roundaction_pet_action, MAX_SKILL, false);

		//catch
		appendText(ui.comboBox_autocatchpet_petskill, MAX_SKILL, false);
		break;
	}
	case util::kComboBoxItem:
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
	Injector& injector = Injector::getInstance();

	auto get = [](unsigned int value)->QString { return SelectTargetForm::generateShortName(value); };

	ui.pushButton_roundaction_char->setText(get(injector.getValueHash(util::kBattleCharRoundActionTargetValue)));
	ui.pushButton_crossaction_char->setText(get(injector.getValueHash(util::kBattleCharCrossActionTargetValue)));
	ui.pushButton_normalaction_char->setText(get(injector.getValueHash(util::kBattleCharNormalActionTargetValue)));

	ui.pushButton_roundaction_pet->setText(get(injector.getValueHash(util::kBattlePetRoundActionTargetValue)));
	ui.pushButton_crossaction_pet->setText(get(injector.getValueHash(util::kBattlePetCrossActionTargetValue)));
	ui.pushButton_normalaction_pet->setText(get(injector.getValueHash(util::kBattlePetNormalActionTargetValue)));

	ui.pushButton_magicheal->setText(get(injector.getValueHash(util::kBattleMagicHealTargetValue)));
	ui.pushButton_magicrevive->setText(get(injector.getValueHash(util::kBattleItemReviveTargetValue)));
	ui.pushButton_itemheal->setText(get(injector.getValueHash(util::kBattleItemHealTargetValue)));
	ui.pushButton_itemrevive->setText(get(injector.getValueHash(util::kBattleItemReviveTargetValue)));
}