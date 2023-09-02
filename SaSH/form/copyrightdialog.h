#pragma once

#include <QDialog>
#include "ui_copyrightdialog.h"

class CopyRightDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CopyRightDialog(QWidget* parent = nullptr);
	virtual ~CopyRightDialog();

protected:
	virtual bool eventFilter(QObject* watched, QEvent* event) override;

	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QDialog::showEvent(e);
	}

private slots:
	void pushButton_copyinfo_clicked();

	void pushButton_sysinfo_clicked();

	void pushButton_dxdiag_clicked();

private:
	Ui::CopyRightDialogClass ui;
};
