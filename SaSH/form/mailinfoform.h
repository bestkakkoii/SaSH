#pragma once

#include <QWidget>
#include "ui_mailinfoform.h"

class MailInfoForm : public QWidget
{
	Q_OBJECT

public:
	MailInfoForm(QWidget *parent = nullptr);
	~MailInfoForm();

private:
	Ui::MailInfoFormClass ui;
};
