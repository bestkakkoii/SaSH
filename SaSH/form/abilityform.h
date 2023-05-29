#pragma once

#include <QDialog>
#include "ui_abilityform.h"

class AbilityForm : public QDialog
{
	Q_OBJECT

public:
	AbilityForm(QWidget* parent = nullptr);
	~AbilityForm();

private slots:
	void onButtonClicked();

private:
	Ui::AbilityFormClass ui;
};
