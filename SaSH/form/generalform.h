#pragma once

#include <QWidget>
#include "ui_generalform.h"

class AfkForm;
class GeneralForm : public QWidget
{
	Q_OBJECT

public:
	explicit GeneralForm(QWidget* parent = nullptr);
	virtual ~GeneralForm();

signals:
	void resetControlTextLanguage();

public slots:
	void onApplyHashSettingsToUI();

private slots:
	void onButtonClicked();

	void onComboBoxClicked();

	void onCheckBoxStateChanged(int state);

	void onSpinBoxValueChanged(int value);

	void onComboBoxCurrentIndexChanged(int value);

	void onResetControlTextLanguage();

	void onGameStart();

protected:
	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}

private:
	void createServerList();

private:
	Ui::GeneralFormClass ui;
	AfkForm* pAfkForm_ = nullptr;
	QHash<int, QHash<QString, QStringList>> serverList;
};
