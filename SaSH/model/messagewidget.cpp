#include "stdafx.h"
#include <QLabel>
#include <QStyleOption>
#include <QPainter>
#include <QPushButton>
#include <QHBoxLayout>
#include "messagewidget.h"

QMessageWidget::QMessageWidget(const QString& text, QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_DeleteOnClose);

	//top most
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

	setFixedHeight(20);
	setFixedWidth(parent->width());

	setAutoFillBackground(true);
	setObjectName("messageWidget");

	setFont(util::getFont());

	QLabel* pIconLabel = new QLabel(this);
	pMessageLabel_ = new QPushButton(text, this);
	QPushButton* pCloseButton = new QPushButton(this);

	pCloseButton->setFixedSize(8, 8);
	pIconLabel->setFixedSize(16, 16);

	pIconLabel->setScaledContents(true);
	pIconLabel->setObjectName("informationLabel");
	pIconLabel->setPixmap(QPixmap(":/image/icon_statusinfo.svg"));

	//pMessageLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	pMessageLabel_->setObjectName("highlightLabel");
	pMessageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	pCloseButton->setObjectName("closeTipButton");

	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->addWidget(pIconLabel);
	pLayout->addWidget(pMessageLabel_);
	pLayout->addWidget(pCloseButton);
	pLayout->setSpacing(1);
	pLayout->setContentsMargins(1, 1, 5, 1);

	setLayout(pLayout);

	QString style = R"(
/* 界面样式 */
QWidget#messageWidget {
	background: #8AE0FF;
	border: 1px solid #4096FF;
}

/* 提示信息样式 */
QPushButton#highlightLabel {
	border: 0px solid #4096FF;
	background: transparent;
	padding-left: 5px;
	color: #35353D;
}

/* hover */
QPushButton#highlightLabel:hover {
	color: #3AB0FF;
}

/* press */
QPushButton#highlightLabel:pressed {
	color: #31B6FF;
}

/* 图标样式 */
QPushButton#closeTipButton {
	border-radius: none;
	border-image: url(:/image/icon_tipclose.svg);
	background: transparent;
}

/* 图标悬停样式 */
QPushButton#closeTipButton:hover {
	border-image: url(:/image/icon_tipclose_hover.svg);
}

/* 图标按下样式 */
QPushButton#closeTipButton:pressed {
	border-image: url(:/image/icon_tipclose_pressed.svg);
}
)";

	setStyleSheet(style);

	connect(pCloseButton, SIGNAL(clicked(bool)), this, SLOT(close()));
}

QMessageWidget::~QMessageWidget()
{

}

// 设置显示文本
void QMessageWidget::setText(const QString& text)
{
	pMessageLabel_->setText(text);
}

// 设置样式需要重写
void QMessageWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
