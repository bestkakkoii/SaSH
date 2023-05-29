#pragma once
#include <QObject>
#include <QProgressBar>

class QString;
class QPaintEvent;

class ProgressBar : public QProgressBar
{
	Q_OBJECT
public:
	enum Type
	{
		kHP,
		kMP,
	};

	explicit ProgressBar(QWidget* parent = nullptr);
	void setType(ProgressBar::Type type);
	void setName(const QString& name);

public slots:
	void onCurrentValueChanged(int level, int value, int maxvalue);

protected:
	//void paintEvent(QPaintEvent* event) override;

private:
	int level_ = 0;
	Type type_ = kHP;
	QString name_ = "";
};