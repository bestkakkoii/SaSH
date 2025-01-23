#pragma once

#include <QWidget>
#include "ui_generalform.h"
#include <indexer.h>
#include "form/afkform.h"

#include "SaSH/process/processmanager.h"

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

	void onGameStart(long long delay = 5000);

protected:
	virtual void showEvent(QShowEvent* e) override;

private:
	void createServerList();
	void serverListReLoad();
	void reloadPaths();

private slots:
	void onMainThradDestroyed();

public:
	AfkForm pAfkForm_;
	QHash<qint64, ProcessManager*> processManagers_; // 進程管理器
	QHash<QString, ProcessManager*> tempProcessManagers_; // 臨時進程管理器 用於防止重複開啟

private:
	Ui::GeneralFormClass ui;

	QHash<long long, QHash<QString, QStringList>> serverList;
};
