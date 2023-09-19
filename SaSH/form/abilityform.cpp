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

	setStyleSheet("QDialog{border: 1px solid black;}");

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &AbilityForm::onButtonClicked, Qt::UniqueConnection);
	}

	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	PC pc = injector.server->getPC();

	ui.label_vit->setText(QString::number(pc.vit));
	ui.label_str->setText(QString::number(pc.str));
	ui.label_tgh->setText(QString::number(pc.tgh));
	ui.label_dex->setText(QString::number(pc.dex));
	ui.label_left->setText(QString::number(pc.point));
}

AbilityForm::~AbilityForm()
{
}


void AbilityForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
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

	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (injector.server->getPC().point <= 0)
		return;

	if (name == "pushButton_vit")
	{
		int amt = ui.label_vit->text().toInt();
		injector.server->addPoint(0, amt > 0 ? amt : 1);
		return;
	}

	if (name == "pushButton_str")
	{
		int amt = ui.label_str->text().toInt();
		injector.server->addPoint(1, amt > 0 ? amt : 1);
		return;
	}

	if (name == "pushButton_tgh")
	{
		int amt = ui.label_tgh->text().toInt();
		injector.server->addPoint(2, amt > 0 ? amt : 1);
		return;
	}

	if (name == "pushButton_dex")
	{
		int amt = ui.label_dex->text().toInt();
		injector.server->addPoint(3, amt > 0 ? amt : 1);
		return;
	}
}