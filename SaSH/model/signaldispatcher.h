#pragma once
#include <QObject>

class SignalDispatcher : public QObject
{
	Q_OBJECT;
	Q_DISABLE_COPY_MOVE(SignalDispatcher);
public:
	static SignalDispatcher& getInstance()
	{
		static SignalDispatcher* instance = new SignalDispatcher();
		return *instance;
	}
	virtual ~SignalDispatcher() = default;
private:
	SignalDispatcher() = default;

signals:
	void updateCharHpProgressValue(int level, int value, int maxvalue);
	void updateCharMpProgressValue(int level, int value, int maxvalue);
	void updatePetHpProgressValue(int level, int value, int maxvalue);
	void updateRideHpProgressValue(int level, int value, int maxvalue);

	void updateStatusLabelTextChanged(int status);
	void updateMapLabelTextChanged(const QString& text);
	void updateCursorLabelTextChanged(const QString& text);
	void updateCoordsPosLabelTextChanged(const QString& text);

	void updatePlayerInfoColContents(int col, const QVariant& data);
	void updatePlayerInfoStone(int stone);

	void updateItemInfoRowContents(int row, const QVariant& data);
	void updateEquipInfoRowContents(int row, const QVariant& data);

	//battleForm
	void updateTopInfoContents(const QVariant& data);
	void updateBottomInfoContents(const QVariant& data);
	void updateTimeLabelContents(const QString& text);
	void updateLabelPlayerAction(const QString& text);
	void updateLabelPetAction(const QString& text);

	void setStartButtonEnabled(bool enable);

	void applyHashSettingsToUI();
};