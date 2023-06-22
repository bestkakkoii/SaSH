#pragma once

#include <QWidget>
#include "ui_otherform.h"

class OtherForm : public QWidget
{
	Q_OBJECT
public:
	OtherForm(QWidget* parent = nullptr);
	~OtherForm();

signals:
	void resetControlTextLanguage();

public slots:
	void onApplyHashSettingsToUI();


private slots:
	void onResetControlTextLanguage();
	void onButtonClicked();
	void groupBoxClicked(bool checked);
	void onCheckBoxStateChanged(int state);
	void onSpinBoxValueChanged(int value);
	void onComboBoxCurrentIndexChanged(int value);
	void onComboBoxClicked();
	void onComboBoxTextChanged(const QString& text);
	void onLineEditTextChanged(const QString& text);
	void onUpdateTeamInfo(const QStringList& text);
	void onListWidgetDoubleClicked(QListWidgetItem* item);
private:
	void updateComboboxAutoFunNameList(const QStringList& textList);
private:
	Ui::OtherFormClass ui;
};
