#pragma once

#include <QWidget>
#include "ui_iteminfoform.h"

class ItemInfoForm : public QWidget
{
	Q_OBJECT

public:
	ItemInfoForm(QWidget* parent = nullptr);
	~ItemInfoForm();

public slots:
	void onResetControlTextLanguage();

private slots:
	void onUpdateItemInfoRowContents(int row, const QVariant& data);
	void onUpdateEquipInfoRowContents(int row, const QVariant& data);

	void on_tableWidget_item_cellDoubleClicked(int row, int column);
	void on_tableWidget_equip_cellDoubleClicked(int row, int column);
private:
	void updateItemInfoRowContents(QTableWidget* tableWidget, int row, const QVariant& data);
private:
	Ui::ItemInfoFormClass ui;
};
