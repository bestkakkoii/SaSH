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

	QList <QListWidget*> listWidgetList = util::findWidgets<QListWidget>(this);
	for (auto& listWidget : listWidgetList)
	{
		if (listWidget)
			connect(listWidget, &QListWidget::itemDoubleClicked, this, &OtherForm::onListWidgetDoubleClicked, Qt::UniqueConnection);
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

	QList <QGroupBox*> groupBoxList = util::findWidgets<QGroupBox>(this);
	for (auto& groupBox : groupBoxList)
	{
		if (groupBox)
			connect(groupBox, &QGroupBox::clicked, this, &OtherForm::groupBoxClicked, Qt::UniqueConnection);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &OtherForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::updateTeamInfo, this, &OtherForm::onUpdateTeamInfo, Qt::UniqueConnection);

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

	Injector& injector = Injector::getInstance();

	if (name == "groupBox_lockpets")
	{

	}
	else if (name == "groupBox_lockrides")
	{

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

	Injector& injector = Injector::getInstance();

	if (name == "listWidget_lockpets")
	{
		//delete item
		int row = pListWidget->row(item);
		pListWidget->takeItem(row);
	}
	else if (name == "listWidget_lockrides")
	{
		int row = pListWidget->row(item);
		pListWidget->takeItem(row);
	}
}

void OtherForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "pushButton_lockpetsadd")
	{
		if (injector.server.isNull() || !injector.server->IS_ONLINE_FLAG)
			return;

		QString text = ui.comboBox_lockpets->currentText();
		int level = ui.spinBox_lockpetslevel->value();
		QString str = QString("%1, %2").arg(text).arg(level);
		ui.listWidget_lockpets->addItem(str);

		int size = ui.listWidget_lockpets->count();
		QStringList list;
		for (int i = 0; i < size; ++i)
		{
			QListWidgetItem* item = ui.listWidget_lockpets->item(i);
			if (item)
			{
				QString str = item->text();
				list.append(str);
			}
		}
		injector.setStringHash(util::kLockPetScheduleString, list.join("|"));
		return;
	}
	else if (name == "pushButton_lockridesadd")
	{
		if (injector.server.isNull() || !injector.server->IS_ONLINE_FLAG)
			return;

		QString text = ui.comboBox_lockrides->currentText();
		int level = ui.spinBox_lockrideslevel->value();
		QString str = QString("%1, %2").arg(text).arg(level);
		ui.listWidget_lockrides->addItem(str);

		int size = ui.listWidget_lockrides->count();
		QStringList list;
		for (int i = 0; i < size; ++i)
		{
			QListWidgetItem* item = ui.listWidget_lockrides->item(i);
			if (item)
			{
				QString str = item->text();
				list.append(str);
			}
		}
		injector.setStringHash(util::kLockRideScheduleString, list.join("|"));
		return;
	}
	else if (name == "pushButton_lockpetsup")
	{
		util::SwapRowUp(ui.listWidget_lockpets);
	}
	else if (name == "pushButton_lockpetsdown")
	{
		util::SwapRowDown(ui.listWidget_lockpets);
	}
	else if (name == "pushButton_lockridesup")
	{
		util::SwapRowUp(ui.listWidget_lockrides);
	}
	else if (name == "pushButton_lockridesdown")
	{
		util::SwapRowDown(ui.listWidget_lockrides);
	}

	else if (name == "pushButton_leaderleave")
	{
		if (injector.server.isNull() || !injector.server->IS_ONLINE_FLAG)
			return;

		injector.server->setTeamState(false);
	}

	for (int i = 1; i < MAX_PARTY; ++i)
	{
		if (name == QString("pushButton_teammate%1kick").arg(i))
		{
			if (injector.server.isNull() || !injector.server->IS_ONLINE_FLAG)
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

	else if (name == "comboBox_lockpets" || name == "comboBox_lockrides" || name == "comboBox_lockpet" || name == "comboBox_lockride")
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
		pComboBox->clear();
		pComboBox->addItems(list);
		if (oldIndex < list.size())
			pComboBox->setCurrentIndex(oldIndex);
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


	ui.groupBox_lockpets->setChecked(enableHash.value(util::kLockPetScheduleEnable));
	ui.groupBox_lockrides->setChecked(enableHash.value(util::kLockRideScheduleEnable));

	QString schedule = stringHash.value(util::kLockPetScheduleString);
	QStringList scheduleList = schedule.split(util::rexComma);
	ui.listWidget_lockpets->addItems(scheduleList);

	schedule = stringHash.value(util::kLockRideScheduleString);
	scheduleList = schedule.split(util::rexComma);
	ui.listWidget_lockrides->addItems(scheduleList);
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