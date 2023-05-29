#include "stdafx.h"
#include "otherform.h"
#include "util.h"
#include "injector.h"
#include "signaldispatcher.h"

OtherForm::OtherForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	util::setTab(ui.tabWidge_other);
	util::setTab(ui.tabWidget_lock);

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

	QList <QComboBox*> comboBoxList = util::findWidgets<QComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
	}

	QList <QLineEdit*> lineEditList = util::findWidgets<QLineEdit>(this);
	for (auto& lineEdit : lineEditList)
	{
		if (lineEdit)
			connect(lineEdit, &QLineEdit::textChanged, this, &OtherForm::onLineEditTextChanged, Qt::UniqueConnection);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &OtherForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
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
	QComboBox* pComboBox = qobject_cast<QComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();
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

void OtherForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	util::SafeHash<util::UserSetting, bool> enableHash = injector.getEnableHash();
	util::SafeHash<util::UserSetting, int> valueHash = injector.getValueHash();
	util::SafeHash<util::UserSetting, QString> stringHash = injector.getStringHash();

	//other2
	ui.lineEdit_gameaccount->setText(stringHash.value(util::UserSetting::kGameAccountString));
	ui.lineEdit_gamepassword->setText(stringHash.value(util::UserSetting::kGamePasswordString));
	ui.lineEdit_gamesecuritycode->setText(stringHash.value(util::UserSetting::kGameSecurityCodeString));
}