#include "stdafx.h"
#include "fastlabel.h"
#include <QPainter>

FastLabel::FastLabel(QWidget* parent)
	: QWidget(parent)
{
	content_msg_ = "";
	text_cache_ = "";
	text_color_ = Qt::black;
	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_StaticContents, true);
	//setAttribute(Qt::WA_OpaquePaintEvent, true);
}

FastLabel::FastLabel()
{
	content_msg_ = "";
	text_cache_ = "";
	text_color_ = Qt::black;
	//setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_StaticContents, true);
	//setAttribute(Qt::WA_OpaquePaintEvent, true);
}

FastLabel::~FastLabel()
{
}

void FastLabel::setTextColor(const QColor& color)
{
	text_color_ = color;
}

QColor FastLabel::getTextColor()
{
	return text_color_;
}

void FastLabel::resizeEvent(QResizeEvent*)
{
	update();
}

void FastLabel::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);
	painter.setPen(QPen(text_color_));
	painter.drawText(this->rect(), flag_, content_msg_);
}

void FastLabel::setFlag(int flag)
{
	flag_ = flag;
}
int FastLabel::getFlag()
{
	return flag_;
}

void FastLabel::setText(const QString& text)
{
	content_msg_ = text;
	update();
}

