/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

//TabWidget pages
#include "form/selectobjectform.h"
#include "form/generalform.h"
#include "form/mapform.h"
#include "form/otherform.h"
#include "form/scriptform.h"
#include "form/infoform.h"
#include "form/mapwidget.h"

#include "form/copyrightdialog.h"
#include "form/settingfiledialog.h"

//menu action forms
#include "form/scripteditor.h"
#include "model/qthumbnailform.h"
#include "update/downloader.h"

class QTranslator;
class QMenuBar;
class QShowEvent;
class QCloseEvent;
class QSystemTrayIcon;

class QThumbnailForm;

class Interpreter;

class MainForm : public QMainWindow, public Indexer
{
	Q_OBJECT

public:
	explicit MainForm(long long index, QWidget* parent);
	virtual ~MainForm();

	static MainForm* createNewWindow(long long idToAllocate, long long* pId = nullptr);

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
	std::string print(std::string str);

	void updateStatusText(const QString text = "");

	void createMenu(QMenuBar* pMenuBar);
	void createTrayIcon();

signals:
	void resetControlTextLanguage();

private slots:
	bool onResetControlTextLanguage();

	void onMenuActionTriggered();

	void onSaveHashSettings(const QString& name = "default", bool isFullPath = false);
	void onLoadHashSettings(const QString& name = "default", bool isFullPath = false);

	void onUpdateStatusLabelTextChanged(long long status);
	void onUpdateMapLabelTextChanged(const QString& text);
	void onUpdateCursorLabelTextChanged(const QString& text);
	void onUpdateCoordsPosLabelTextChanged(const QString& text);
	void onUpdateTimeLabelTextChanged(const QString& text);
	void onUpdateStonePosLabelTextChanged(long long ntext);
	void onUpdateMainFormTitle(const QString& text);

	void onMessageBoxShow(const QString& text, long long type = 0, QString title = "", long long* pnret = nullptr, QString topText = "", QString detail = "");
	void onInputBoxShow(const QString& text, long long type, QVariant* retvalue);

	void onAppendScriptLog(const QString& text, long long color = 0);
	void onAppendChatLog(const QString& text, long long color = 0);

signals:
	void messageBoxFinished();
	void inputBoxFinished();

private:
	bool markAsClose_ = false;
	Ui::MainFormClass ui;
	QMenuBar* pMenuBar_ = nullptr;

	QHash<QString, QAction*> menu_action_hash_;

	GeneralForm pGeneralForm_;
#ifndef LEAK_TEST
	MapForm pMapForm_;
	OtherForm pOtherForm_;
	ScriptForm pScriptForm_;
	InfoForm pInfoForm_;
	ScriptEditor pScriptEditor_;
	MapWidget mapWidget_;
#endif

	long long interfaceCount_ = 0;

	Downloader downloader_;

	QThumbnailForm* pThumbnailForm_ = nullptr;

	QSystemTrayIcon trayIcon_ = nullptr;

	QHash<long long, std::shared_ptr<Interpreter>> interpreter_hash_;

	QAction* hideTrayAction_ = nullptr;
};
