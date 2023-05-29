#pragma once

#include <QWidget>
#include "ui_battleinfoform.h"

class BattleInfoForm : public QWidget
{
	Q_OBJECT

public:
	BattleInfoForm(QWidget* parent = nullptr);
	~BattleInfoForm();

private slots:
	void onUpdateTopInfoContents(const QVariant& data);
	void onUpdateBottomInfoContents(const QVariant& data);
	void onUpdateTimeLabelContents(const QString& text);
	void onUpdateLabelPlayerAction(const QString& text);
	void onUpdateLabelPetAction(const QString& text);
private:
	void updateItemInfoRowContents(QTableWidget* tableWidget, const QVariant& data);

private:
	Ui::BattleInfoFormClass ui;
};
