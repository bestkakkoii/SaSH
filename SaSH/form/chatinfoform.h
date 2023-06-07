#pragma once

#include <QWidget>
#include "ui_chatinfoform.h"

class ColorDelegate;
class ChatInfoForm : public QWidget
{
	Q_OBJECT

public:
	ChatInfoForm(QWidget* parent = nullptr);
	virtual ~ChatInfoForm();

signals:
	void resetControlTextLanguage();
public:
	void onResetControlTextLanguage();
protected:
	bool eventFilter(QObject* watched, QEvent* e) override;
private slots:
	void onApplyHashSettingsToUI();


private:
	Ui::ChatInfoFormClass ui;
	ColorDelegate* delegate_ = nullptr;
};
