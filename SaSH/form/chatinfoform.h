#pragma once

#include <QWidget>
#include "ui_chatinfoform.h"

class ChatInfoForm : public QWidget
{
	Q_OBJECT

public:
	ChatInfoForm(QWidget *parent = nullptr);
	~ChatInfoForm();

private:
	Ui::ChatInfoFormClass ui;
};
