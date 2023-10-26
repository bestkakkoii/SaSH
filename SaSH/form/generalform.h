#pragma once

#include <QWidget>
#include "ui_generalform.h"
#include <indexer.h>
#include "form/afkform.h"

class AfkForm;
class GeneralForm : public QWidget, public Indexer
{
	Q_OBJECT

public:
	GeneralForm(long long index, QWidget* parent);
	virtual ~GeneralForm();

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
	virtual void showEvent(QShowEvent* e) override;

private:
	void createServerList();
	void serverListReLoad();
	void reloadPaths();

	Q_INVOKABLE void startGameAsync();

private:
	Ui::GeneralFormClass ui;
	AfkForm pAfkForm_;
	QHash<long long, QHash<QString, QStringList>> serverList;
};
