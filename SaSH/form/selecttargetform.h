#pragma once

#include <QDialog>
#include "ui_selecttargetform.h"

class QShowEvent;

class SelectTargetForm : public QDialog
{
	Q_OBJECT

public:
	SelectTargetForm(QWidget* parent = nullptr);
	~SelectTargetForm();

protected:
	void showEvent(QShowEvent* e);



private:
	Ui::SelectTargetFormClass ui;
};
