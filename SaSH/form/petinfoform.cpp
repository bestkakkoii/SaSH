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
#include "petinfoform.h"

#include "util.h"
#include "injector.h"

PetInfoForm::PetInfoForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

PetInfoForm::~PetInfoForm()
{
}

void PetInfoForm::on_comboBox_currentIndexChanged(int index)
{
	QString str = ui.comboBox->currentText();
	if (str.isEmpty())
		return;

	QString cmpstr = str.mid(2);
	if (cmpstr.isEmpty())
		return;



	QVector<int> v = ui.comboBox->currentData().value<QVector<int>>();
	cur_level = v[0];
	cur_maxHp = v[1];
	cur_atk = v[2];
	cur_def = v[3];
	cur_agi = v[4];

	ui.spinBox_current_level->setValue(cur_level);
	ui.spinBox_current_hp->setValue(cur_maxHp);
	ui.spinBox_current_atk->setValue(cur_atk);
	ui.spinBox_current_def->setValue(cur_def);
	ui.spinBox_current_agi->setValue(cur_agi);

	static const QRegularExpression re("([0-9]+)");
	//找能匹配 4 組 則默認level為1 5組則第一個為level

	QRegularExpressionMatchIterator i = re.globalMatch(cmpstr);
	//int count = 0;
	QVector <int> basev;
	while (i.hasNext())
	{
		QRegularExpressionMatch match = i.next();
		QString word = match.captured(1);
		basev.append(word.toInt());
	}

	int size = basev.size();
	if (size >= 5)
	{
		base_level = basev[0];
		base_maxHp = basev[1];
		base_atk = basev[2];
		base_def = basev[3];
		base_agi = basev[4];

	}
	else if (size == 4)
	{
		base_level = 1;
		base_maxHp = basev[0];
		base_atk = basev[1];
		base_def = basev[2];
		base_agi = basev[3];
	}
	else
	{
		base_level = 1;
		base_maxHp = 1;
		base_atk = 1;
		base_def = 1;
		base_agi = 1;
	}

	ui.spinBox_base_level->setValue(base_level);
	ui.spinBox_base_hp->setValue(base_maxHp);
	ui.spinBox_base_atk->setValue(base_atk);
	ui.spinBox_base_def->setValue(base_def);
	ui.spinBox_base_agi->setValue(base_agi);
}

void PetInfoForm::on_comboBox_clicked()
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	int current = ui.comboBox->currentIndex();
	ui.comboBox->clear();

	for (int i = 0; i < MAX_PET; ++i)
	{
		QVector<int> v;
		PET pet = injector.server->getPet(i);
		QString name = QString::number(i + 1) + ":";
		if (!pet.name.isEmpty() && pet.useFlag == 1)
		{
			if (!pet.freeName.isEmpty())
				name += pet.freeName;
			else
				name += pet.name;
			v.append(pet.level);
			v.append(pet.maxHp);
			v.append(pet.atk);
			v.append(pet.def);
			v.append(pet.quick);
		}
		else
			v.resize(5);
		ui.comboBox->addItem(name, QVariant::fromValue(v));
	}


	if (current >= 0 && current < MAX_PET)
		ui.comboBox->setCurrentIndex(current);
	else
		ui.comboBox->setCurrentIndex(0);
}

void PetInfoForm::on_pushButton_calc_clicked()
{
	int diff_level = cur_level - base_level + 1;
	int diff_maxHp = cur_maxHp - base_maxHp;
	int diff_atk = cur_atk - base_atk;
	int diff_def = cur_def - base_def;
	int diff_agi = cur_agi - base_agi;

	double rate_level = 1.0;
	double rate_maxHp = diff_maxHp * rate_level / diff_level;
	double rate_atk = diff_atk * rate_level / diff_level;
	double rate_def = diff_def * rate_level / diff_level;
	double rate_agi = diff_agi * rate_level / diff_level;

	int expect_level = ui.spinBox_expect_level->value();

	ui.doubleSpinBox->setValue(rate_atk + rate_def + rate_agi);
	ui.doubleSpinBox_hp->setValue(rate_maxHp);
	ui.doubleSpinBox_atk->setValue(rate_atk);
	ui.doubleSpinBox_def->setValue(rate_def);
	ui.doubleSpinBox_agi->setValue(rate_agi);

	ui.spinBox_expect_level->setValue(expect_level);
	ui.spinBox_expect_hp->setValue((rate_maxHp * static_cast<double>(expect_level - base_level + 1)) + static_cast<double>(base_maxHp));
	ui.spinBox_expect_atk->setValue((rate_atk * static_cast<double>(expect_level - base_level + 1)) + static_cast<double>(base_atk));
	ui.spinBox_expect_def->setValue((rate_def * static_cast<double>(expect_level - base_level + 1)) + static_cast<double>(base_def));
	ui.spinBox_expect_agi->setValue((rate_agi * static_cast<double>(expect_level - base_level + 1)) + static_cast<double>(base_agi));
}

void PetInfoForm::on_pushButton_clear_clicked()
{
	cur_level = 1;
	cur_maxHp = 1;
	cur_atk = 1;
	cur_def = 1;
	cur_agi = 1;
	base_level = 1;
	base_maxHp = 1;
	base_atk = 1;
	base_def = 1;
	base_agi = 1;

	ui.spinBox_base_level->setValue(base_level);
	ui.spinBox_base_hp->setValue(base_maxHp);
	ui.spinBox_base_atk->setValue(base_atk);
	ui.spinBox_base_def->setValue(base_def);
	ui.spinBox_base_agi->setValue(base_agi);

	ui.spinBox_current_level->setValue(cur_level);
	ui.spinBox_current_hp->setValue(cur_maxHp);
	ui.spinBox_current_atk->setValue(cur_atk);
	ui.spinBox_current_def->setValue(cur_def);
	ui.spinBox_current_agi->setValue(cur_agi);

	ui.doubleSpinBox->setValue(0.0);
	ui.doubleSpinBox_atk->setValue(0.0);
	ui.doubleSpinBox_def->setValue(0.0);
	ui.doubleSpinBox_hp->setValue(0.0);
	ui.doubleSpinBox_agi->setValue(0.0);

	ui.spinBox_expect_level->setValue(160);
	ui.spinBox_expect_hp->setValue(1);
	ui.spinBox_expect_atk->setValue(1);
	ui.spinBox_expect_def->setValue(1);
	ui.spinBox_expect_agi->setValue(1);
}