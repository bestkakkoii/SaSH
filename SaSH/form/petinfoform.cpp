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
	cur_level_ = v[0];
	cur_maxHp_ = v[1];
	cur_atk_ = v[2];
	cur_def_ = v[3];
	cur_agi_ = v[4];

	ui.spinBox_current_level->setValue(cur_level_);
	ui.spinBox_current_hp->setValue(cur_maxHp_);
	ui.spinBox_current_atk->setValue(cur_atk_);
	ui.spinBox_current_def->setValue(cur_def_);
	ui.spinBox_current_agi->setValue(cur_agi_);

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
		base_level_ = basev[0];
		base_maxHp_ = basev[1];
		base_atk_ = basev[2];
		base_def_ = basev[3];
		base_agi_ = basev[4];

	}
	else if (size == 4)
	{
		base_level_ = 1;
		base_maxHp_ = basev[0];
		base_atk_ = basev[1];
		base_def_ = basev[2];
		base_agi_ = basev[3];
	}
	else
	{
		base_level_ = 1;
		base_maxHp_ = 1;
		base_atk_ = 1;
		base_def_ = 1;
		base_agi_ = 1;
	}

	emit ui.pushButton_calc->clicked();

	ui.spinBox_base_level->setValue(base_level_);
	ui.spinBox_base_hp->setValue(base_maxHp_);
	ui.spinBox_base_atk->setValue(base_atk_);
	ui.spinBox_base_def->setValue(base_def_);
	ui.spinBox_base_agi->setValue(base_agi_);
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

constexpr inline double __vectorcall calcRate(int diff, int diff_level)
{
	return diff / static_cast<double>(diff_level);
}

constexpr inline double __vectorcall calcExpectValue(double base, double growthRate, int targetLevel)
{
	return (base + (growthRate * static_cast<double>(targetLevel - 1)));
}

constexpr inline double __vectorcall calcPower(double hp, double atk, double def, double agi)
{
	return (hp / 4.0) + atk + def + agi;
}

void PetInfoForm::on_pushButton_calc_clicked()
{
	base_level_ = ui.spinBox_base_level->value();
	base_maxHp_ = ui.spinBox_base_hp->value();
	base_atk_ = ui.spinBox_base_atk->value();
	base_def_ = ui.spinBox_base_def->value();
	base_agi_ = ui.spinBox_base_agi->value();

	cur_level_ = ui.spinBox_current_level->value();
	cur_maxHp_ = ui.spinBox_current_hp->value();
	cur_atk_ = ui.spinBox_current_atk->value();
	cur_def_ = ui.spinBox_current_def->value();
	cur_agi_ = ui.spinBox_current_agi->value();

	int diff_level = cur_level_ - base_level_;
	int diff_maxHp = cur_maxHp_ - base_maxHp_;
	int diff_atk = cur_atk_ - base_atk_;
	int diff_def = cur_def_ - base_def_;
	int diff_agi = cur_agi_ - base_agi_;

	constexpr double rate_level = 1.0;
	double rate_maxHp = calcRate(diff_maxHp, diff_level);
	double rate_atk = calcRate(diff_atk, diff_level);
	double rate_def = calcRate(diff_def, diff_level);
	double rate_agi = calcRate(diff_agi, diff_level);

	int expect_level = ui.spinBox_expect_level->value();

	if (base_level_ >= expect_level)
		return;

	ui.doubleSpinBox->setValue(rate_atk + rate_def + rate_agi);
	ui.doubleSpinBox_hp->setValue(rate_maxHp);
	ui.doubleSpinBox_atk->setValue(rate_atk);
	ui.doubleSpinBox_def->setValue(rate_def);
	ui.doubleSpinBox_agi->setValue(rate_agi);

	ui.spinBox_expect_level->setValue(expect_level);
	ui.spinBox_expect_hp->setValue(calcExpectValue(base_maxHp_, rate_maxHp, expect_level));
	ui.spinBox_expect_atk->setValue(calcExpectValue(base_atk_, rate_atk, expect_level));
	ui.spinBox_expect_def->setValue(calcExpectValue(base_def_, rate_def, expect_level));
	ui.spinBox_expect_agi->setValue(calcExpectValue(base_agi_, rate_agi, expect_level));
	ui.doubleSpinBox_power->setValue(calcPower(
		ui.spinBox_expect_hp->value(),
		ui.spinBox_expect_atk->value(),
		ui.spinBox_expect_def->value(),
		ui.spinBox_expect_agi->value()));
}

void PetInfoForm::on_pushButton_clear_clicked()
{
	cur_level_ = 1;
	cur_maxHp_ = 1;
	cur_atk_ = 1;
	cur_def_ = 1;
	cur_agi_ = 1;
	base_level_ = 1;
	base_maxHp_ = 1;
	base_atk_ = 1;
	base_def_ = 1;
	base_agi_ = 1;

	ui.spinBox_base_level->setValue(base_level_);
	ui.spinBox_base_hp->setValue(base_maxHp_);
	ui.spinBox_base_atk->setValue(base_atk_);
	ui.spinBox_base_def->setValue(base_def_);
	ui.spinBox_base_agi->setValue(base_agi_);

	ui.spinBox_current_level->setValue(cur_level_);
	ui.spinBox_current_hp->setValue(cur_maxHp_);
	ui.spinBox_current_atk->setValue(cur_atk_);
	ui.spinBox_current_def->setValue(cur_def_);
	ui.spinBox_current_agi->setValue(cur_agi_);

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
	ui.doubleSpinBox_power->setValue(0.0);
}