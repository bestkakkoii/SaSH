#pragma once

#include <QWidget>
#include "ui_playerinfoform.h"

class PlayerInfoForm : public QWidget
{
	Q_OBJECT

public:
	PlayerInfoForm(QWidget* parent = nullptr);
	~PlayerInfoForm();

public slots:
	void onResetControlTextLanguage();
private slots:

	void onUpdatePlayerInfoColContents(int col, const QVariant& data);
	void onUpdatePlayerInfoStone(int stone);
	void onHeaderClicked(int logicalIndex);

private:
	Ui::PlayerInfoFormClass ui;
};
