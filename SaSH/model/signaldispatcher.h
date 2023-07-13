#pragma once
#include <QObject>

class SignalDispatcher : public QObject
{
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(SignalDispatcher);
public:
	static SignalDispatcher& getInstance()
	{
		static SignalDispatcher instance;
		return instance;
	}
	virtual ~SignalDispatcher() = default;

	void setParent(QObject* parent) { QObject::setParent(parent); }
private:
	SignalDispatcher() = default;

signals:
	//global
	void nodifyAllStop();
	void messageBoxShow(const QString& text, int type = 0, int* pnret = nullptr);
	void inputBoxShow(const QString& text, int type, QVariant* retvalue);

	void applyHashSettingsToUI();
	void saveHashSettings(const QString& name = "default", bool isFullPath = false);
	void loadHashSettings(const QString& name = "default", bool isFullPath = false);

	void gameStarted();

	//mainform
	void updateMainFormTitle(const QString& text);
	void updateCharHpProgressValue(int level, int value, int maxvalue);
	void updateCharMpProgressValue(int level, int value, int maxvalue);
	void updatePetHpProgressValue(int level, int value, int maxvalue);
	void updateRideHpProgressValue(int level, int value, int maxvalue);

	void updateStatusLabelTextChanged(int status);
	void updateMapLabelTextChanged(const QString& text);
	void updateCursorLabelTextChanged(const QString& text);
	void updateCoordsPosLabelTextChanged(const QString& text);

	void appendScriptLog(const QString& text, int color = 0);
	void appendChatLog(const QString& text, int color = 0);

	//infoform
	void updatePlayerInfoColContents(int col, const QVariant& data);
	void updatePlayerInfoStone(int stone);
	void updatePlayerInfoPetState(int petIndex, int state);

	void updateItemInfoRowContents(int row, const QVariant& data);
	void updateEquipInfoRowContents(int row, const QVariant& data);


	//battleForm
	void updateTopInfoContents(const QVariant& data);
	void updateBottomInfoContents(const QVariant& data);
	void updateTimeLabelContents(const QString& text);
	void updateLabelPlayerAction(const QString& text);
	void updateLabelPetAction(const QString& text);

	void setStartButtonEnabled(bool enable);

	//mapform
	void updateNpcList(int floor);

	//afkform
	void updateComboBoxItemText(int type, const QStringList& textList);

	//afkinfo
	void updateAfkInfoTable(int row, const QString& text);

	//otherform->group
	void updateTeamInfo(const QStringList& text);


	//script
	void scriptLabelRowTextChanged(int row, int max, bool noSelect);
	void scriptPaused();
	void scriptResumed();
	void scriptBreaked();
	void scriptStarted();
	void scriptStoped();
	void scriptFinished();
	void scriptContentChanged(const QString& fileName, const QVariant& tokens);
	void loadFileToTable(const QString& fileName);
	void reloadScriptList();
	void varInfoImported(const QHash<QString, QVariant>& d);


	void callStackInfoChanged(const QVariant& var);
	void jumpStackInfoChanged(const QVariant& var);

	void addForwardMarker(int liner, bool b);
	void addErrorMarker(int liner, bool b);
	void addBreakMarker(int liner, bool b);
	void addStepMarker(int liner, bool b);
	//void loadHashSettings(const QString& name = "default");
};

