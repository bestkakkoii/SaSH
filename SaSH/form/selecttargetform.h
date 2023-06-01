#pragma once

#include <QDialog>
#include "ui_selecttargetform.h"

class QShowEvent;

class SelectTargetForm : public QDialog
{
	Q_OBJECT
public:
	SelectTargetForm(int type, QString* dst, QWidget* parent = nullptr);
	virtual ~SelectTargetForm();
	static Q_REQUIRED_RESULT QString generateShortName(unsigned int flg);
protected:
	void showEvent(QShowEvent* e);

private slots:
	void onCheckBoxStateChanged(int state);
	void onAccept();
private:
	void checkControls();

private:
	Ui::SelectTargetFormClass ui;
	int type_ = 0;
	unsigned int selectflag_ = 0u;
	QString* dst_ = nullptr;
};

static bool createSelectTargetForm(int type, QString* dst, QWidget* perent)
{
	SelectTargetForm* pObjForm = new SelectTargetForm(type, dst, perent);
	if (pObjForm)
	{
		if (pObjForm->exec() == QDialog::Accepted)
		{
			return true;
		}
	}
	return false;
}