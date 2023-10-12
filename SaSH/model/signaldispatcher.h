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
#include <QObject>
#include <indexer.h>

class SignalDispatcher : public QObject, public Indexer
{
	Q_OBJECT;
private:
	explicit SignalDispatcher(__int64 index);

public:
	virtual ~SignalDispatcher();

	static SignalDispatcher& getInstance(__int64 index);

	static bool contains(__int64 index);

public:
	void setParent(QObject* parent);

	static void remove(__int64 index);

signals:
	//global
	void nodifyAllStop();
	void nodifyAllScriptStop();
	void messageBoxShow(const QString& text, __int64 type = 0, QString title = "", __int64* pnret = nullptr, QString topText = "", QString detail = "", void* p = nullptr);
	void inputBoxShow(const QString& text, __int64 type, QVariant* retvalue, void* p);
	void fileDialogShow(const QString& name, __int64 acceptType, QString* retstring, void* p);

	void applyHashSettingsToUI();
	void saveHashSettings(const QString& name = "default", bool isFullPath = false);
	void loadHashSettings(const QString& name = "default", bool isFullPath = false);

	void gameStarted();

	//mainform
	void updateMainFormTitle(const QString& text);
	void updateCharHpProgressValue(__int64 level, __int64 value, __int64 maxvalue);
	void updateCharMpProgressValue(__int64 level, __int64 value, __int64 maxvalue);
	void updatePetHpProgressValue(__int64 level, __int64 value, __int64 maxvalue);
	void updateRideHpProgressValue(__int64 level, __int64 value, __int64 maxvalue);

	void updateStatusLabelTextChanged(__int64 status);
	void updateMapLabelTextChanged(const QString& text);
	void updateCursorLabelTextChanged(const QString& text);
	void updateCoordsPosLabelTextChanged(const QString& text);
	void updateLoginTimeLabelTextChanged(const QString& text);

	void appendScriptLog(const QString& text, __int64 color = 0);
	void appendChatLog(const QString& text, __int64 color = 0);

	//infoform
	void updateCharInfoColContents(__int64 col, const QVariant& data);
	void updateCharInfoStone(__int64 stone);
	void updateCharInfoPetState(__int64 petIndex, __int64 state);

	void updateItemInfoRowContents(__int64 row, const QVariant& data);
	void updateEquipInfoRowContents(__int64 row, const QVariant& data);


	//battleForm
	void updateTopInfoContents(const QVariant& data);
	void updateBottomInfoContents(const QVariant& data);
	void updateBattleTimeLabelTextChanged(const QString& text);
	void updateLabelCharAction(const QString& text);
	void updateLabelPetAction(const QString& text);
	void notifyBattleActionState(__int64 index, bool left);
	void battleTableItemForegroundColorChanged(__int64 index, const QColor& color);
	void battleTableAllItemResetColor();

	void setStartButtonEnabled(bool enable);

	//mapform
	void updateNpcList(__int64 floor);

	//afkform
	void updateComboBoxItemText(__int64 type, const QStringList& textList);

	//afkinfo
	void updateAfkInfoTable(__int64 row, const QString& text);

	//otherform->group
	void updateTeamInfo(const QStringList& text);


	//script
	void scriptLabelRowTextChanged(__int64 row, __int64 max, bool noSelect);
	void scriptPaused();
	void scriptResumed();
	void scriptBreaked();
	void scriptStarted();
	void scriptStoped();
	void scriptFinished();
	void scriptContentChanged(const QString& fileName, const QVariant& tokens);
	void loadFileToTable(const QString& fileName, bool start = false);
	void reloadScriptList();
	void varInfoImported(void* p, const QVariantHash& d, const QStringList& globalNames);
	void breakMarkInfoImport();

	void scriptSpeedChanged(__int64 speed);

	void addForwardMarker(__int64 liner, bool b);
	void addErrorMarker(__int64 liner, bool b);
	void addBreakMarker(__int64 liner, bool b);
	void addStepMarker(__int64 liner, bool b);
	//void loadHashSettings(const QString& name = "default");
};