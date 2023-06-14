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
class MapWidget;
class ScriptSettingForm;

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

	void onSaveHashSettings(const QString& name = "default", bool isFullPath = false);
	void onLoadHashSettings(const QString& name = "default", bool isFullPath = false);

	void onUpdateStatusLabelTextChanged(int status);
	void onUpdateMapLabelTextChanged(const QString& text);
	void onUpdateCursorLabelTextChanged(const QString& text);
	void onUpdateCoordsPosLabelTextChanged(const QString& text);
	void onUpdateMainFormTitle(const QString& text);

	void onMessageBoxShow(const QString& text, int type = 0);
	void onInputBoxShow(const QString& text, int type, QVariant* retvalue);
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
	MapWidget* mapWidget_ = nullptr;
	ScriptSettingForm* pScriptSettingForm_ = nullptr;
};
