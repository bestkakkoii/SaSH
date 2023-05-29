#pragma once

#include <QWidget>
#include "ui_mapform.h"

class MapForm : public QWidget
{
	Q_OBJECT

public:
	MapForm(QWidget *parent = nullptr);
	~MapForm();

private:
	Ui::MapFormClass ui;
};
