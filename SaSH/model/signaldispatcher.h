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
#include "model/safe.h"

class SignalDispatcher : public QObject, public Indexer
{
	Q_OBJECT;
private:
	inline static safe::Hash<long long, SignalDispatcher*> instances;

	explicit SignalDispatcher(long long index)
		: Indexer(index)
	{
	}

public:
	virtual ~SignalDispatcher()
	{
		qDebug() << "SignalDispatcher is distoryed!!";
	}

	static SignalDispatcher& getInstance(long long index)
	{
		if (!instances.contains(index))
		{
			SignalDispatcher* instance = q_check_ptr(new SignalDispatcher(index));
			instances.insert(index, instance);
		}
		return *instances.value(index);
	}

	static bool contains(long long index)
	{
		return instances.contains(index);
	}

public:
	inline void setParent(QObject* parent) { QObject::setParent(parent); }

	static inline void remove(long long index)
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
	void messageBoxShow(const QString& text, long long type = 0, QString title = "", long long* pnret = nullptr, QString topText = "", QString detail = "");
	void inputBoxShow(const QString& text, long long type, QVariant* retvalue);

	void applyHashSettingsToUI();
	void saveHashSettings(const QString& name = "default", bool isFullPath = false);
	void loadHashSettings(const QString& name = "default", bool isFullPath = false);

	void gameStarted();

	//mainform
	void updateMainFormTitle(const QString& text);
	void updateCharHpProgressValue(long long level, long long value, long long maxvalue);
	void updateCharMpProgressValue(long long level, long long value, long long maxvalue);
	void updatePetHpProgressValue(long long level, long long value, long long maxvalue);
	void updateRideHpProgressValue(long long level, long long value, long long maxvalue);

	void updateStatusLabelTextChanged(long long status);
	void updateMapLabelTextChanged(const QString& text);
	void updateCursorLabelTextChanged(const QString& text);
	void updateCoordsPosLabelTextChanged(const QString& text);
	void updateLoginTimeLabelTextChanged(const QString& text);

	void appendScriptLog(const QString& text, long long color = 0);
	void appendChatLog(const QString& text, long long color = 0);

	//infoform
	void updateCharInfoColContents(long long col, const QVariant& data);
	void updateCharInfoStone(long long stone);
	void updateCharInfoPetState(long long petIndex, long long state);

	void updateItemInfoRowContents(long long row, const QVariant& data);
	void updateEquipInfoRowContents(long long row, const QVariant& data);


	//battleForm
	void updateBattleItemRowContents(long long index, const QString& text, const QColor& color = Qt::white);
	void updateBattleTimeLabelTextChanged(const QString& text);
	void updateLabelCharAction(const QString& text);
	void updateLabelPetAction(const QString& text);
	void notifyBattleActionState(long long index);
	void battleTableItemForegroundColorChanged(long long index, const QColor& color);
	void battleTableAllItemResetColor();

	void setStartButtonEnabled(bool enable);

	//mapform
	void updateNpcList(long long floor);

	//afkform
	void updateComboBoxItemText(long long type, const QStringList& textList);

	//afkinfo
	void updateAfkInfoTable(long long row, const QString& text);

	//otherform->group
	void updateTeamInfo(const QStringList& text);


	//script
	void scriptLabelRowTextChanged(long long row, long long max, bool noSelect);
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

	void scriptSpeedChanged(long long speed);

	void addForwardMarker(long long liner, bool b);
	void addErrorMarker(long long liner, bool b);
	void addBreakMarker(long long liner, bool b);
	void addStepMarker(long long liner, bool b);
	//void loadHashSettings(const QString& name = "default");
};

