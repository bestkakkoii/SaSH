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
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
	}

	QList <ComboBox*> ccomboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : ccomboBoxList)
	{
		if (comboBox)
		{
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


	//group
	ui.comboBox_autofuntype->setCurrentIndex(valueHash.value(util::kAutoFunTypeValue));
	ui.comboBox_autofunname->setCurrentText(stringHash.value(util::kAutoFunNameString));

	//other2
	ui.lineEdit_gameaccount->setText(stringHash.value(util::UserSetting::kGameAccountString));
	ui.lineEdit_gamepassword->setText(stringHash.value(util::UserSetting::kGamePasswordString));
	ui.lineEdit_gamesecuritycode->setText(stringHash.value(util::UserSetting::kGameSecurityCodeString));
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