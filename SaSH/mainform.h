#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainform.h"

class QTranslator;
class QMenuBar;
class QShowEvent;
class QCloseEvent;
class GeneralForm;
class MapForm;
class AfkForm;
class OtherForm;
class ScriptForm;

class InfoForm;

class MainForm : public QMainWindow
{
	Q_OBJECT

public:
	MainForm(QWidget* parent = nullptr);
	~MainForm();

protected:
	void showEvent(QShowEvent* e) override;
	void closeEvent(QCloseEvent* e) override;

	//接收原生的窗口消息
	bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
private:
	void resetControlTextLanguage();

private slots:
	void onMenuActionTriggered();

	void onUpdateStatusLabelTextChanged(int status);
	void onUpdateMapLabelTextChanged(const QString& text);
	void onUpdateCursorLabelTextChanged(const QString& text);
	void onUpdateCoordsPosLabelTextChanged(const QString& text);

private:
	Ui::MainFormClass ui;
	QMenuBar* pMenuBar_ = nullptr;
	QTranslator translator_;
	QHash<QString, QAction*> menu_action_hash_;

	GeneralForm* pGeneralForm_ = nullptr;
	MapForm* pMapForm_ = nullptr;
	AfkForm* pAfkForm_ = nullptr;
	OtherForm* pOtherForm_ = nullptr;
	ScriptForm* pScriptForm_ = nullptr;

	InfoForm* pInfoForm_ = nullptr;
};
