#pragma once

#include <QMainWindow>
#include "ui_scriptsettingform.h"

class ScriptSettingForm : public QMainWindow
{
	Q_OBJECT

public:
	ScriptSettingForm(QWidget* parent = nullptr);
	~ScriptSettingForm();
protected:
	void showEvent(QShowEvent* e) override;
	void closeEvent(QCloseEvent* e) override;

private:
	void ScriptSettingForm::fileSave(const QString& d, DWORD flag);

private slots:
	void onApplyHashSettingsToUI();

private:
	Ui::ScriptSettingFormClass ui;
	QLabel m_staticLabel;
};
