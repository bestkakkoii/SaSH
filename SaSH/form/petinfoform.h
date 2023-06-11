#pragma once

#include <QWidget>
#include "ui_petinfoform.h"

class PetInfoForm : public QWidget
{
	Q_OBJECT

public:
	PetInfoForm(QWidget* parent = nullptr);
	virtual ~PetInfoForm();

private slots:
	void on_comboBox_currentIndexChanged(int index);
	void on_comboBox_clicked();
	void on_pushButton_calc_clicked();
	void on_pushButton_clear_clicked();
private:
	Ui::PetInfoFormClass ui;

	int base_level = 0;
	int base_maxHp = 0;
	int base_atk = 0;
	int base_def = 0;
	int base_agi = 0;

	int cur_level = 0;
	int cur_maxHp = 0;
	int cur_atk = 0;
	int cur_def = 0;
	int cur_agi = 0;
};
