#pragma once

#include <QWidget>
#include "ui_afkinfoform.h"

class AfkInfoForm : public QWidget
{
	Q_OBJECT

public:
	AfkInfoForm(QWidget* parent = nullptr);
	~AfkInfoForm();

private slots:
	void onResetControlTextLanguage();
	void onUpdateAfkInfoTable(int row, const QString& text);
	void onButtonClicked();
private:
	void updateTableText(int row, int col, const QString& text);

private:
	Ui::AfkInfoFormClass ui;
};
