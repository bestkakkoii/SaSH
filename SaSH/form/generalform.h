#pragma once

#include <QWidget>
#include "ui_generalform.h"

class GeneralForm : public QWidget
{
	Q_OBJECT

public:
	GeneralForm(QWidget* parent = nullptr);
	~GeneralForm();

signals:
	void resetControlTextLanguage();

public slots:
	void onSaveHashSettings(const QString& name = "default");
	void onLoadHashSettings(const QString& name = "default");
	void onApplyHashSettingsToUI();
private slots:
	void onButtonClicked();
	void onCheckBoxStateChanged(int state);
	void onSpinBoxValueChanged(int value);
	void onComboBoxCurrentIndexChanged(int value);

	void onResetControlTextLanguage();

protected:

private:

private:
	Ui::GeneralFormClass ui;
};
