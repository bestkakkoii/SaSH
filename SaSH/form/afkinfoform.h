#pragma once

#include <QWidget>
#include "ui_afkinfoform.h"

class AfkInfoForm : public QWidget
{
	Q_OBJECT

public:
	AfkInfoForm(QWidget *parent = nullptr);
	~AfkInfoForm();

private:
	Ui::AfkInfoFormClass ui;
};
