#include "stdafx.h"
#include "chatinfoform.h"

#include "util.h"
#include "injector.h"
#include "signaldispatcher.h"

static const QHash<int, QColor> combo_colorhash = {
	{ 0, QColor(255, 255, 255) },
	{ 1, QColor(0, 255, 255) },
	{ 2, QColor(255, 0, 255) },
	{ 3, QColor(0, 0, 255) },
	{ 4, QColor(255, 255, 0) },
	{ 5, QColor(0, 255, 0) },
	{ 6, QColor(255, 0, 0) },
	{ 7, QColor(160, 160, 164) },
	{ 8, QColor(166, 202, 240) },
	{ 9, QColor(192, 220, 192) },
	{ 10, QColor(218, 175, 66) },
};

class ColorDelegate : public QStyledItemDelegate
{
public:
	explicit ColorDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QStyledItemDelegate::paint(painter, option, index);

		if (index.data(Qt::UserRole).isValid())
		{
			QColor color = qvariant_cast<QColor>(index.data(Qt::UserRole));
			painter->save();
			painter->setRenderHint(QPainter::Antialiasing, true);
			painter->setPen(Qt::NoPen);
			painter->setBrush(color);
			painter->drawRect(option.rect.adjusted(2, 2, -2, -2));
			painter->restore();
		}
	}
};

ChatInfoForm::ChatInfoForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(this, &ChatInfoForm::resetControlTextLanguage, this, &ChatInfoForm::onResetControlTextLanguage, Qt::UniqueConnection);

	ui.listView_log->setWordWrap(false);
	ui.listView_log->setTextElideMode(Qt::ElideNone);
	ui.listView_log->setResizeMode(QListView::Adjust);
	ui.listView_log->installEventFilter(this);

	ui.comboBox_send->installEventFilter(this);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ChatInfoForm::onApplyHashSettingsToUI, Qt::UniqueConnection);

	Injector& injector = Injector::getInstance();

	if (!injector.chatLogModel.isNull())
		ui.listView_log->setModel(injector.chatLogModel.get());

	delegate_ = new ColorDelegate(this);

	QComboBox comboBox;
	ui.comboBox_channel->clear();
	ui.comboBox_color->setItemDelegate(delegate_);

	for (auto it = combo_colorhash.begin(); it != combo_colorhash.end(); ++it)
	{
		ui.comboBox_color->addItem(QString::number(it.key()), QVariant(it.value()));
	}

	onResetControlTextLanguage();
}

ChatInfoForm::~ChatInfoForm()
{
}

bool ChatInfoForm::eventFilter(QObject* watched, QEvent* e)
{
	if (watched == ui.listView_log && e->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(e);
		if (keyEvent->key() == Qt::Key_Delete)
		{
			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->cleanChatHistory();
			return true;
		}
	}
	else if (watched == ui.comboBox_send && e->type() == QEvent::KeyPress)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
		if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
		{
			QComboBox* comboBox = qobject_cast<QComboBox*>(watched);
			if (comboBox)
			{
				QString text = comboBox->currentText();
				Injector& injector = Injector::getInstance();
				if (!injector.server.isNull())
				{
					int nMode = ui.comboBox_channel->currentIndex();
					TalkMode mode = static_cast<TalkMode>(nMode != -1 ? nMode : kTalkNormal);
					if (nMode != (channelList_.size() - 1))
						injector.server->talk(text, ui.comboBox_color->currentIndex(), mode);
					else
						injector.server->inputtext(text);

					comboBox->insertItem(0, text);

				}
				comboBox->lineEdit()->clear();
				return true;
			}
		}
	}
	return QWidget::eventFilter(watched, e);
}

void ChatInfoForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	if (!injector.chatLogModel.isNull())
		ui.listView_log->setModel(injector.chatLogModel.get());
}

void ChatInfoForm::onResetControlTextLanguage()
{
	//ui.comboBox_color->clear();
	////白色 淺藍 紫色 藍色 黃色 綠色 紅色 灰色 灰藍 灰綠 土黃
	//QStringList colorList = {
	//	tr("white"), tr("light blue") , tr("purple") , tr("blue"), tr("yellow"),
	//	tr("green") , tr("red") , tr("gray") , tr("light blue-gray") , tr("light green"),
	//	tr("ochre")
	//};

	//ui.comboBox_color->addItems(colorList);

	//



	channelList_ = QStringList{
		tr("normal"), tr("team"), tr("family"), tr("world"), tr("global"), tr("dialog")
	};
	int index = ui.comboBox_channel->currentIndex();
	ui.comboBox_channel->clear();
	ui.comboBox_channel->addItems(channelList_);
	ui.comboBox_channel->setCurrentIndex(index);
}