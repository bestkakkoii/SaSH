#pragma once

#include <QWidget>
#include "ui_battlesettingfrom.h"
#include "indexer.h"

class BattleSettingFrom : public QWidget, public Indexer
{
	Q_OBJECT

public:
	BattleSettingFrom(long long index, QWidget* parent);
	virtual ~BattleSettingFrom();

private slots:
	void onButtonClicked();
	void onUpdateComboBoxItemText(long long type, const QStringList& textList);
private:
	Ui::BattleSettingFromClass ui;


};