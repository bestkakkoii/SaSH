/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainform.h"

class QTranslator;
class QMenuBar;
class QShowEvent;
class QCloseEvent;
class QSystemTrayIcon;

class GeneralForm;
class MapForm;
class AfkForm;
class OtherForm;
class ScriptForm;
class LuaScriptForm;

class InfoForm;
class MapWidget;
class ScriptSettingForm;

class QThumbnailForm;

class Interpreter;

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

	//window move
	void moveEvent(QMoveEvent* e) override;

private:
	void resetControlTextLanguage();
	void updateStatusText();

private slots:
	void onMenuActionTriggered();

	void onSaveHashSettings(const QString& name = "default", bool isFullPath = false);
	void onLoadHashSettings(const QString& name = "default", bool isFullPath = false);

	void onUpdateStatusLabelTextChanged(int status);
	void onUpdateMapLabelTextChanged(const QString& text);
	void onUpdateCursorLabelTextChanged(const QString& text);
	void onUpdateCoordsPosLabelTextChanged(const QString& text);
	void onUpdateStonePosLabelTextChanged(int ntext);
	void onUpdateMainFormTitle(const QString& text);

	void onMessageBoxShow(const QString& text, int type = 0, int* pnret = nullptr);
	void onInputBoxShow(const QString& text, int type, QVariant* retvalue);

	void onAppendScriptLog(const QString& text, int color = 0);
	void onAppendChatLog(const QString& text, int color = 0);
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
	LuaScriptForm* pLuaScriptForm_ = nullptr;

	int interfaceCount_ = 0;

	InfoForm* pInfoForm_ = nullptr;
	MapWidget* mapWidget_ = nullptr;
	ScriptSettingForm* pScriptSettingForm_ = nullptr;

	QThumbnailForm* pThumbnailForm_ = nullptr;

	QSystemTrayIcon* trayIcon = nullptr;

	QHash<int, QSharedPointer<Interpreter>> interpreter_hash_;
};
