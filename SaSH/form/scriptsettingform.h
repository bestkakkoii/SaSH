#pragma once

#include <QMainWindow>
#include "ui_scriptsettingform.h"

class ScriptSettingForm : public QMainWindow
{
	Q_OBJECT

public:
	ScriptSettingForm(QWidget *parent = nullptr);
	~ScriptSettingForm();

private:
	Ui::ScriptSettingFormClass ui;
};
