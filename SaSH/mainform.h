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
#include <indexer.h>

class QTranslator;
class QMenuBar;
class QShowEvent;
class QCloseEvent;
class QSystemTrayIcon;

class GeneralForm;
class MapForm;
class OtherForm;
class ScriptForm;
class LuaScriptForm;

class InfoForm;
class MapWidget;
class ScriptEditor;

class QThumbnailForm;

class Interpreter;

class MainForm : public QMainWindow, public Indexer
{
	Q_OBJECT

public:
	explicit MainForm(qint64 index, QWidget* parent);
	virtual ~MainForm();

	static MainForm* createNewWindow(qint64 idToAllocate, qint64* pId = nullptr);

protected:
	void showEvent(QShowEvent* e) override;
	void closeEvent(QCloseEvent* e) override;

	//接收原生的窗口消息
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#else
	virtual bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

	//paint
	//void paintEvent(QPaintEvent* e) override;

	//window move
	void moveEvent(QMoveEvent* e) override;

private:
	void resetControlTextLanguage();
	void updateStatusText(const QString text = "");

	void createMenu(QMenuBar* pMenuBar);

private slots:
	void onMenuActionTriggered();

	void onSaveHashSettings(const QString& name = "default", bool isFullPath = false);
	void onLoadHashSettings(const QString& name = "default", bool isFullPath = false);

	void onUpdateStatusLabelTextChanged(qint64 status);
	void onUpdateMapLabelTextChanged(const QString& text);
	void onUpdateCursorLabelTextChanged(const QString& text);
	void onUpdateCoordsPosLabelTextChanged(const QString& text);
	void onUpdateTimeLabelTextChanged(const QString& text);
	void onUpdateStonePosLabelTextChanged(qint64 ntext);
	void onUpdateMainFormTitle(const QString& text);

	void onMessageBoxShow(const QString& text, qint64 type = 0, QString title = "", qint64* pnret = nullptr, QString topText = "", QString detail = "", void* p = nullptr);
	void onInputBoxShow(const QString& text, qint64 type, QVariant* retvalue, void* p);
	void onFileDialogShow(const QString& name, qint64 acceptType, QString* retstring, void* p);

	void onAppendScriptLog(const QString& text, qint64 color = 0);
	void onAppendChatLog(const QString& text, qint64 color = 0);

private:
	bool markAsClose_ = false;
	Ui::MainFormClass ui;
	QMenuBar* pMenuBar_ = nullptr;
	QTranslator translator_;
	QHash<QString, QAction*> menu_action_hash_;

	GeneralForm* pGeneralForm_ = nullptr;
	MapForm* pMapForm_ = nullptr;

	OtherForm* pOtherForm_ = nullptr;
	ScriptForm* pScriptForm_ = nullptr;
	LuaScriptForm* pLuaScriptForm_ = nullptr;

	qint64 interfaceCount_ = 0;

	InfoForm* pInfoForm_ = nullptr;
	MapWidget* mapWidget_ = nullptr;
	ScriptEditor* pScriptEditor_ = nullptr;

	QThumbnailForm* pThumbnailForm_ = nullptr;

	QSystemTrayIcon* trayIcon = nullptr;

	QHash<qint64, QSharedPointer<Interpreter>> interpreter_hash_;

	QAction* hideTrayAction_ = nullptr;
};
