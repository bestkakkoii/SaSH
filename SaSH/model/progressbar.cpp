#include "stdafx.h"
#include "progressbar.h"

void setProgressBarStyle(QProgressBar* pProgress, const QString& qstrcolor)
{
	QString styleSheet = QString(R"(
		QProgressBar {
			border: 1px solid black;
			border-radius: 0px;
			text-align: center;
			background-color: rbg(255, 255, 255);
			/*text size*/
			font-size: 10px;
		}

		QProgressBar::chunk {
			background-color: %1;
			margin: 0px; /* 调整chunk与边框的间距 */
			subcontrol-position: left; /* 调整chunk的位置为左侧 */
		}
	)").arg(qstrcolor);

	pProgress->setStyleSheet(styleSheet);
}

ProgressBar::ProgressBar(QWidget* parent)
	: QProgressBar(parent)
{
	setTextVisible(true);
	setRange(0, 100);
	QFont font = this->font();
	font.setPointSize(10);
	setFont(font);
}

void ProgressBar::setType(ProgressBar::Type type)
{
	type_ = type;
	if (kHP == type_)
		setProgressBarStyle(this, \
			"qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(255, 65, 65, 255), stop:0.556818 rgba(255, 138, 98, 255), stop:0.977273 rgba(255, 163, 107, 255), stop:1 rgba(255, 150, 100, 255))");
	else
		setProgressBarStyle(this, \
			"qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(75, 67, 255, 255), stop:0.556818 rgba(132, 114, 255, 255), stop:0.977273 rgba(148, 143, 255, 255), stop:1 rgba(140, 130, 255, 255))");
}

void ProgressBar::setName(const QString& name)
{
	name_ = name;
}

//slots
void ProgressBar::onCurrentValueChanged(int level, int value, int maxvalue)
{
	level_ = level;
	if (maxvalue == 0)
		maxvalue = 100;

	if (value > maxvalue)
	{
		value = 0;
		maxvalue = 100;
	}

	setRange(0, maxvalue);
	setValue(value);
	QString text;
	if (kHP == type_)
	{
		text = QString("%1 LV:%2, HP:%3/%4(%5)")
			.arg(name_)
			.arg(level_)
			.arg("%v")
			.arg("%m")
			.arg("%p%");
	}
	else
	{
		text = QString("MP:%1/%2(%3)")
			.arg("%v")
			.arg("%m")
			.arg("%p%");
	}
	setFormat(text);
}