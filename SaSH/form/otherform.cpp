#include "stdafx.h"
#include "otherform.h"
#include <util.h>
#include <injector.h>
#include "signaldispatcher.h"

OtherForm::OtherForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	util::setTab(ui.tabWidge_other);
	util::setTab(ui.tabWidget_lock);

	connect(this, &OtherForm::resetControlTextLanguage, this, &OtherForm::onResetControlTextLanguage, Qt::UniqueConnection);


	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &QPushButton::clicked, this, &OtherForm::onButtonClicked, Qt::UniqueConnection);
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox)
			connect(checkBox, &QCheckBox::stateChanged, this, &OtherForm::onCheckBoxStateChanged, Qt::UniqueConnection);
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
		{
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
			connect(comboBox, &ComboBox::clicked, this, &OtherForm::onComboBoxClicked, Qt::UniqueConnection);
			connect(comboBox, &ComboBox::currentTextChanged, this, &OtherForm::onComboBoxTextChanged, Qt::UniqueConnection);
		}
	}

	QList <QLineEdit*> lineEditList = util::findWidgets<QLineEdit>(this);
	for (auto& lineEdit : lineEditList)
	{
		if (lineEdit)
			connect(lineEdit, &QLineEdit::textChanged, this, &OtherForm::onLineEditTextChanged, Qt::UniqueConnection);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &OtherForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateTeamInfo, this, &OtherForm::onUpdateTeamInfo, Qt::UniqueConnection);

	emit signalDispatcher.applyHashSettingsToUI();
}

OtherForm::~OtherForm()
{
}

void OtherForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;
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

	Injector& injector = Injector::getInstance();

	//lockpet
	if (name == "checkBox_lockpet")
	{
		injector.setEnableHash(util::kLockPetEnable, isChecked);
	}
	else if (name == "checkBox_lockride")
	{
		injector.setEnableHash(util::kLockRideEnable, isChecked);
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

	Injector& injector = Injector::getInstance();
}

void OtherForm::onComboBoxCurrentIndexChanged(int value)
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	//group
	if (name == "comboBox_autofuntype")
	{
		injector.setValueHash(util::kAutoFunTypeValue, value != -1 ? value : 0);
	}

	//lockpet
	else if (name == "comboBox_lockpet")
	{
		injector.setValueHash(util::kLockPetValue, value != -1 ? value : 0);
	}
	else if (name == "comboBox_lockride")
	{
		injector.setValueHash(util::kLockRideValue, value != -1 ? value : 0);
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

	Injector& injector = Injector::getInstance();

	if (name == "comboBox_autofunname")
	{
		injector.setStringHash(util::kAutoFunNameString, newText);
	}
}

void OtherForm::onComboBoxClicked()
{
	ComboBox* pComboBox = qobject_cast<ComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "comboBox_autofunname")
	{
		if (injector.server.isNull())
			return;

		QStringList unitList = injector.server->getJoinableUnitList();
		updateComboboxAutoFunNameList(unitList);
	}
	//lockpet
	else if (name == "comboBox_lockpet")
	{
		int oldIndex = pComboBox->currentIndex();
		QStringList list;
		if (!injector.server.isNull() && injector.server->IS_ONLINE_FLAG)
		{
			for (int i = 0; i < MAX_PET; ++i)
			{
				PET pet = injector.server->pet[i];
				if (pet.name.isEmpty() || pet.useFlag == 0)
				{
					list.append(QString("%1:").arg(i + 1));
					continue;
				}
				list.append(QString("%1:%2").arg(i + 1).arg(pet.name));
			}
		}
		ui.comboBox_lockpet->clear();
		ui.comboBox_lockpet->addItems(list);
		if (oldIndex < list.size())
			ui.comboBox_lockpet->setCurrentIndex(oldIndex);
	}
	else if (name == "comboBox_lockride")
	{
		int oldIndex = pComboBox->currentIndex();
		QStringList list;
		if (!injector.server.isNull() && injector.server->IS_ONLINE_FLAG)
		{
			for (int i = 0; i < MAX_PET; ++i)
			{
				PET pet = injector.server->pet[i];
				if (pet.name.isEmpty() || pet.useFlag == 0)
				{
					list.append(QString("%1:").arg(i + 1));
					continue;
				}
				list.append(QString("%1:%2").arg(i + 1).arg(pet.name));
			}
		}
		ui.comboBox_lockride->clear();
		ui.comboBox_lockride->addItems(list);
		if (oldIndex < list.size())
			ui.comboBox_lockride->setCurrentIndex(oldIndex);
	}
}

void OtherForm::onLineEditTextChanged(const QString& text)
{
	QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(sender());
	if (!pLineEdit)
		return;

	QString name = pLineEdit->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	//other2
	if (name == "lineEdit_gameaccount")
	{
		injector.setStringHash(util::UserSetting::kGameAccountString, text);
	}

	if (name == "lineEdit_gamepassword")
	{
		injector.setStringHash(util::UserSetting::kGamePasswordString, text);
	}

	if (name == "lineEdit_gamesecuritycode")
	{
		injector.setStringHash(util::UserSetting::kGameSecurityCodeString, text);
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
}

void OtherForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash = injector.getEnableHash();
	util::SafeHash<util::UserSetting, int> valueHash = injector.getValueHash();
	util::SafeHash<util::UserSetting, QString> stringHash = injector.getStringHash();

	if (ui.comboBox_lockride->count() == 0 || ui.comboBox_lockpet->count() == 0)
	{
		QStringList list;
		for (int i = 0; i < MAX_PET; ++i)
		{
			list.append(QString("%1:").arg(i + 1));
		}

		if (ui.comboBox_lockride->count() == 0)
			ui.comboBox_lockride->addItems(list);

		if (ui.comboBox_lockpet->count() == 0)
			ui.comboBox_lockpet->addItems(list);
	}

	QStringList list;
	if (!injector.server.isNull() && injector.server->IS_ONLINE_FLAG)
	{
		for (int i = 0; i < MAX_PET; ++i)
		{
			PET pet = injector.server->pet[i];
			if (pet.name.isEmpty() || pet.useFlag == 0)
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
}

void OtherForm::onUpdateTeamInfo(const QStringList& strList)
{
	for (int i = 0; i <= MAX_PARTY; ++i)
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