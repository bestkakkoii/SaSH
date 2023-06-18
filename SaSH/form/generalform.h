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
	void onApplyHashSettingsToUI();
private slots:
	void onButtonClicked();
	void onComboBoxClicked();
	void onCheckBoxStateChanged(int state);
	void onSpinBoxValueChanged(int value);
	void onComboBoxCurrentIndexChanged(int value);

	void onResetControlTextLanguage();

	void onGameStart();

protected:

private:

private:
	Ui::GeneralFormClass ui;
};
