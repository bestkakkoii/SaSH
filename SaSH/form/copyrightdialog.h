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
	bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
	void pushButton_copyinfo_clicked();
	void pushButton_sysinfo_clicked();
	void pushButton_dxdiag_clicked();

private:
	Ui::CopyRightDialogClass ui;
};
