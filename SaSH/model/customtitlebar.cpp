#include "stdafx.h"
#include "customtitlebar.h"

CustomTitleBar::CustomTitleBar(QWidget* parent) : QWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground);
	setFixedHeight(35);

	QString style = R"(

QWidget { background-color: rgb(31, 31, 31); }

QLabel {
	color: rgb(214, 214, 214);
	font: 12pt "Consolas";
}

QPushButton {
	background-color: rgb(31, 31, 31);
	border: 0px solid #dcdfe6;
	color: rgb(214, 214, 214);
	padding: 3px;
}

QPushButton:hover {
	background-color: rgb(61, 61, 61);
}

QPushButton:pressed, QPushButton:checked {
	background-color: rgb(56, 56, 56);
}
		)";

	setStyleSheet(style);

	QLabel* icon = new QLabel("");
	icon->setFixedSize(35, 35);
	QPixmap originalPixmap(":/image/ico.png");
	QPixmap scaledPixmap = originalPixmap.scaled(QSize(25, 25), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	icon->setPixmap(scaledPixmap);

	titleLabel_ = new QLabel("sash");
	titleLabel_->setFixedHeight(35);
	titleLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QPushButton* minimizeButton = new QPushButton("");
	minimizeButton->setFixedSize(35, 35);
	minimizeButton->setIcon(QIcon(":/image/icon_min"));
	QPushButton* maximizeButton = new QPushButton("");
	maximizeButton->setFixedSize(35, 35);
	maximizeButton->setIcon(QIcon(":/image/icon_max"));
	QPushButton* closeButton = new QPushButton("");
	closeButton->setFixedSize(35, 35);
	closeButton->setIcon(QIcon(":/image/icon_close"));

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(5, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(icon);
	layout->addWidget(titleLabel_);
	layout->addWidget(minimizeButton);
	layout->addWidget(maximizeButton);
	layout->addWidget(closeButton);

	connect(minimizeButton, &QPushButton::clicked, parent, &QMainWindow::showMinimized);
	connect(maximizeButton, &QPushButton::clicked, this, &CustomTitleBar::toggleMaximize);
	connect(closeButton, &QPushButton::clicked, parent, &QMainWindow::close);
	connect(parent, &QMainWindow::windowTitleChanged, this, &CustomTitleBar::onTitleChanged);
}

CustomTitleBar::~CustomTitleBar()
{

}

void CustomTitleBar::onTitleChanged(const QString& title)
{
	if (titleLabel_ != nullptr)
	{
		titleLabel_->setText(title);
	}
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	emit maximizeClicked();
}

void CustomTitleBar::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		dragStartPosition_ = event->pos();
	}
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		if ((event->pos() - dragStartPosition_).manhattanLength() >= QApplication::startDragDistance())
		{
			QWidget* parentWindow = qobject_cast<QWidget*>(parent());
			if (parentWindow && parentWindow->isMaximized())
			{
				parentWindow->showNormal();
			}

			QPoint diff = event->pos() - dragStartPosition_;
			QPoint newpos = parentWidget()->pos() + diff;
			parentWidget()->move(newpos);
		}
	}
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	dragStartPosition_ = QPoint();
}

void CustomTitleBar::toggleMaximize()
{
	emit maximizeClicked();
}