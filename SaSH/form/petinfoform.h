#pragma once

#include <QWidget>
#include "ui_petinfoform.h"

class PetInfoForm : public QWidget
{
	Q_OBJECT

public:
	PetInfoForm(QWidget *parent = nullptr);
	~PetInfoForm();

private:
	Ui::PetInfoFormClass ui;
};
