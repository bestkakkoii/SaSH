#pragma once

#include <QWidget>
#include "ui_scriptform.h"

class ScriptForm : public QWidget
{
	Q_OBJECT

public:
	ScriptForm(QWidget *parent = nullptr);
	~ScriptForm();

private:
	Ui::ScriptFormClass ui;
};
