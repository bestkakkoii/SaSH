#include "stdafx.h"
#include "abilityform.h"
#include <util.h>
#include <injector.h>

AbilityForm::AbilityForm(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);
	setFixedSize(this->width(), this->height());
	setAttribute(Qt::WA_DeleteOnClose);


	//boarder black put on the frameless window with style sheet
	setStyleSheet("QDialog{border: 1px solid black;}");


	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &QPushButton::clicked, this, &AbilityForm::onButtonClicked, Qt::UniqueConnection);
	}
}

AbilityForm::~AbilityForm()
{
}


void AbilityForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	//Injector& injector = Injector::getInstance();

	if (name == "pushButton_close")
	{
		accept();
		return;
	}

}