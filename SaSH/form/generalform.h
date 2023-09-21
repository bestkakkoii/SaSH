#pragma once

#include <QWidget>
#include "ui_generalform.h"
#include <indexer.h>

class AfkForm;
class GeneralForm : public QWidget, public Indexer
{
	Q_OBJECT

public:
	explicit GeneralForm(qint64 index, QWidget* parent = nullptr);
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
	void serverListReLoad();

private:
	Ui::GeneralFormClass ui;
	AfkForm* pAfkForm_ = nullptr;
	QHash<qint64, QHash<QString, QStringList>> serverList;
};
