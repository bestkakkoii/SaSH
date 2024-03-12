#pragma once

#include <QDialog>
#include "ui_settingfiledialog.h"

class settingfiledialog : public QDialog
{
	Q_OBJECT

public:
	explicit settingfiledialog(const QString& defaultName, QWidget* parent = nullptr);
	virtual ~settingfiledialog();

	QString returnValue;

private slots:
	void on_listWidget_itemDoubleClicked(QListWidgetItem* item);
	void onLineEditTextChanged(const QString& text);

private:
	Ui::settingfiledialogClass ui;
	QListWidgetItem* firstItem_ = nullptr;
	QListWidgetItem* defaultItem_ = nullptr;
	QLineEdit* lineEdit_ = nullptr;
};
