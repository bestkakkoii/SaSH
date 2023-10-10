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
#include "util.h"

class SignalDispatcher : public QObject, public Indexer
{
	Q_OBJECT;
private:
	inline static util::SafeHash<qint64, SignalDispatcher*> instances;

	explicit SignalDispatcher(qint64 index)
		: Indexer(index)
	{
	}

public:
	virtual ~SignalDispatcher()
	{
		qDebug() << "SignalDispatcher is distoryed!!";
	}

	static SignalDispatcher& getInstance(qint64 index)
	{
		if (!instances.contains(index))
		{
			SignalDispatcher* instance = new SignalDispatcher(index);
			instances.insert(index, instance);
		}
		return *instances.value(index);
	}

	static bool contains(qint64 index)
	{
		return instances.contains(index);
	}

public:
	inline void setParent(QObject* parent) { QObject::setParent(parent); }

	static inline void remove(qint64 index)
	{
		if (!instances.contains(index))
			return;

		SignalDispatcher* instance = instances.take(index);
		if (instance != nullptr)
		{
			instance->deleteLater();
		}
	}

signals:
	//global
	void nodifyAllStop();
	void nodifyAllScriptStop();
	void messageBoxShow(const QString& text, qint64 type = 0, QString title = "", qint64* pnret = nullptr, QString topText = "", QString detail = "", void* p = nullptr);
	void inputBoxShow(const QString& text, qint64 type, QVariant* retvalue, void* p);
	void fileDialogShow(const QString& name, qint64 acceptType, QString* retstring, void* p);

	void applyHashSettingsToUI();
	void saveHashSettings(const QString& name = "default", bool isFullPath = false);
	void loadHashSettings(const QString& name = "default", bool isFullPath = false);

	void gameStarted();

	//mainform
	void updateMainFormTitle(const QString& text);
	void updateCharHpProgressValue(qint64 level, qint64 value, qint64 maxvalue);
	void updateCharMpProgressValue(qint64 level, qint64 value, qint64 maxvalue);
	void updatePetHpProgressValue(qint64 level, qint64 value, qint64 maxvalue);
	void updateRideHpProgressValue(qint64 level, qint64 value, qint64 maxvalue);

	void updateStatusLabelTextChanged(qint64 status);
	void updateMapLabelTextChanged(const QString& text);
	void updateCursorLabelTextChanged(const QString& text);
	void updateCoordsPosLabelTextChanged(const QString& text);
	void updateLoginTimeLabelTextChanged(const QString& text);

	void appendScriptLog(const QString& text, qint64 color = 0);
	void appendChatLog(const QString& text, qint64 color = 0);

	//infoform
	void updateCharInfoColContents(qint64 col, const QVariant& data);
	void updateCharInfoStone(qint64 stone);
	void updateCharInfoPetState(qint64 petIndex, qint64 state);

	void updateItemInfoRowContents(qint64 row, const QVariant& data);
	void updateEquipInfoRowContents(qint64 row, const QVariant& data);


	//battleForm
	void updateTopInfoContents(const QVariant& data);
	void updateBottomInfoContents(const QVariant& data);
	void updateBattleTimeLabelTextChanged(const QString& text);
	void updateLabelCharAction(const QString& text);
	void updateLabelPetAction(const QString& text);
	void notifyBattleActionState(qint64 index, bool left);
	void battleTableItemForegroundColorChanged(qint64 index, const QColor& color);
	void battleTableAllItemResetColor();

	void setStartButtonEnabled(bool enable);

	//mapform
	void updateNpcList(qint64 floor);

	//afkform
	void updateComboBoxItemText(qint64 type, const QStringList& textList);

	//afkinfo
	void updateAfkInfoTable(qint64 row, const QString& text);

	//otherform->group
	void updateTeamInfo(const QStringList& text);


	//script
	void scriptLabelRowTextChanged(qint64 row, qint64 max, bool noSelect);
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

	void scriptSpeedChanged(qint64 speed);

	void addForwardMarker(qint64 liner, bool b);
	void addErrorMarker(qint64 liner, bool b);
	void addBreakMarker(qint64 liner, bool b);
	void addStepMarker(qint64 liner, bool b);
	//void loadHashSettings(const QString& name = "default");
};

