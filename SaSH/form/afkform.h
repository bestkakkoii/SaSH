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
	void onComboBoxClicked();
	void onComboBoxTextChanged(const QString& text);

	void onApplyHashSettingsToUI();

	void onResetControlTextLanguage();

	void onUpdateComboBoxItemText(int type, const QStringList& textList);

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	void updateTargetButtonText();

private:
	Ui::AfkFormClass ui;
};
