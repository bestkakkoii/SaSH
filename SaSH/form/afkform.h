#pragma once

#include <QWidget>
#include "ui_afkform.h"

class AfkForm : public QWidget
{
	Q_OBJECT

public:
	AfkForm(QWidget* parent = nullptr);
	~AfkForm();

signals:
	void resetControlTextLanguage();

private slots:
	void onButtonClicked();
	void onCheckBoxStateChanged(int state);
	void onSpinBoxValueChanged(int value);
	void onComboBoxCurrentIndexChanged(int value);

	void onApplyHashSettingsToUI();

	void onResetControlTextLanguage();
private:
	Ui::AfkFormClass ui;
};
