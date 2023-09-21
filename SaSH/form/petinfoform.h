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

#pragma once

#include <QWidget>
#include "ui_petinfoform.h"
#include <indexer.h>
class PetInfoForm : public QWidget, public Indexer
{
	Q_OBJECT

public:
	explicit PetInfoForm(qint64 index, QWidget* parent = nullptr);

	virtual ~PetInfoForm();

private slots:
	void on_comboBox_currentIndexChanged(int index);

	void on_comboBox_clicked();

	void on_pushButton_calc_clicked();

	void on_pushButton_clear_clicked();

protected:
	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}

private:
	Ui::PetInfoFormClass ui;

	qint64 base_level_ = 0;
	qint64 base_maxHp_ = 0;
	qint64 base_atk_ = 0;
	qint64 base_def_ = 0;
	qint64 base_agi_ = 0;

	qint64 cur_level_ = 0;
	qint64 cur_maxHp_ = 0;
	qint64 cur_atk_ = 0;
	qint64 cur_def_ = 0;
	qint64 cur_agi_ = 0;
};
