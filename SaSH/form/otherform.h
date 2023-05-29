#pragma once

#include <QWidget>
#include "ui_otherform.h"

class OtherForm : public QWidget
{
	Q_OBJECT
public:
	OtherForm(QWidget* parent = nullptr);
	~OtherForm();

public slots:
	void onApplyHashSettingsToUI();


private slots:
	void onButtonClicked();
	void onCheckBoxStateChanged(int state);
	void onSpinBoxValueChanged(int value);
	void onComboBoxCurrentIndexChanged(int value);
	void onLineEditTextChanged(const QString& text);
private:
	Ui::OtherFormClass ui;
};
