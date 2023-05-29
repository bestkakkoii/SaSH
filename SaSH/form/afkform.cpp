#include "stdafx.h"
#include "afkform.h"
#include "util.h"
#include "injector.h"

#include "signaldispatcher.h"

AfkForm::AfkForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	util::setTab(ui.tabWidget_afk);

	connect(this, &AfkForm::resetControlTextLanguage, this, &AfkForm::onResetControlTextLanguage, Qt::UniqueConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &AfkForm::onApplyHashSettingsToUI, Qt::UniqueConnection);

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

	QList <QComboBox*> comboBoxList = util::findWidgets<QComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox)
			connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
	}

	onResetControlTextLanguage();
}

AfkForm::~AfkForm()
{
}

void AfkForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();
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

	if (name == "spinBox_autowalkdelay")
	{
		injector.setValueHash(util::kAutoWalkDelayValue, value);
		return;
	}

	if (name == "spinBox_autowalklen")
	{
		injector.setValueHash(util::kAutoWalkDistanceValue, value);
		return;
	}
}
void AfkForm::onComboBoxCurrentIndexChanged(int value)
{
	QComboBox* pComboBox = qobject_cast<QComboBox*>(sender());
	if (!pComboBox)
		return;

	QString name = pComboBox->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "comboBox_autowalkdir")
	{
		injector.setValueHash(util::kAutoWalkDirectionValue, value != -1 ? value : 0);
		return;
	}
}


void AfkForm::onResetControlTextLanguage()
{
	ui.comboBox_autowalkdir->clear();
	ui.comboBox_autowalkdir->addItems(QStringList{ tr("↖↘"), tr("↗↙"), tr("random") });
}

void AfkForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	ui.comboBox_autowalkdir->setCurrentIndex(injector.getValueHash(util::kAutoWalkDirectionValue));
	ui.spinBox_autowalkdelay->setValue(injector.getValueHash(util::kAutoWalkDelayValue));
	ui.spinBox_autowalklen->setValue(injector.getValueHash(util::kAutoWalkDistanceValue));
}