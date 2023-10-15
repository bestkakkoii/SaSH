#include "stdafx.h"
#include "battlesettingfrom.h"

#include "util.h"
#include "signaldispatcher.h"
#include "injector.h"
#include "script/parser.h"

BattleConditionTextItem::BattleConditionTextItem(const QString& text, QGraphicsItem* parent)
	: QGraphicsTextItem(text, parent)
{
	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	setZValue(1);
	setTextWidth(width_);
	font_ = util::getFont();
}

void BattleConditionTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e)
{
	//left double click
	if (e->button() != Qt::LeftButton)
		return;

	emit clicked(data(Qt::UserRole).toString(), false);
}

void BattleConditionTextItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	isPressed_ = true;
	update();

	if (e->button() != Qt::RightButton)
		return;

	emit clicked(data(Qt::UserRole).toString(), true);
}

void BattleConditionTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
	isHovered_ = true;
	update();
}

void BattleConditionTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
	isHovered_ = false;
	update();
}

void BattleConditionTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent*)
{
	if (isPressed_)
	{
		emit clicked(text_, false);
		isPressed_ = false;
		update();
	}
}

void BattleConditionTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
	QGraphicsItem::mouseMoveEvent(e);
}

void BattleConditionTextItem::dragEnterEvent(QGraphicsSceneDragDropEvent* e)
{
	if (e->mimeData()->hasText())
	{
		e->acceptProposedAction();
	}
}

void BattleConditionTextItem::dropEvent(QGraphicsSceneDragDropEvent* e)
{
	QString droppedText = e->mimeData()->text();
	qDebug() << "Dropped text: " << droppedText;
	qDebug() << "Target text: " << toPlainText();
}

void BattleConditionTextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QColor outlineColor = Qt::black;
	if (isPressed_)
		outlineColor = Qt::darkBlue;
	else if (isHovered_)
		outlineColor = Qt::blue;

	painter->setFont(font_);
	painter->setPen(QPen(outlineColor, 2));
	painter->setBrush(Qt::white);
	painter->drawRect(boundingRect());

	QGraphicsTextItem::paint(painter, option, widget);
}

BattleSettingFrom::BattleSettingFrom(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose);
	QFont font = util::getFont();
	setFont(font);

	// 創建 QGraphicsScene 和 QGraphicsView
	QGraphicsScene* scene = q_check_ptr(new QGraphicsScene(this));
	if (scene != nullptr)
	{
		ui.graphicsView_condition->setScene(scene);
		ui.graphicsView_condition->setRenderHint(QPainter::Antialiasing);
		ui.graphicsView_condition->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		ui.graphicsView_condition->setCacheMode(QGraphicsView::CacheBackground);
	}

	ui.widget_preview->setReadOnly(false);

	ui.listWidget->setResizeMode(QListView::Fixed);
	font.setPointSize(12);
	font.setFamily("YaHei Consolas Hybrid");
	ui.listWidget->setFont(font);
	ui.listWidget->setStyleSheet(R"(
		QListWidget { font-size:12px; font-family:YaHei Consolas Hybrid; }
		QListWidget::item:selected { background-color: black; color: white;
	})");

	// 創建一個 QHash 用於映射中文文本和英文文本
	QList<QPair<QString, QString>> textMapping = {
		{ "等於","==" },
		{ "不等於","!=" },
		{ "大於",">" },
		{ "小於","<" },
		{ "大於等於",">=" },
		{ "小於等於","<=" },
		{ "包含","contains" },
		{ "不包含","not contains" },

		{ "","" },

		{ "如果","if () then\n    \nend" },
		{ "如果否則","if () then\n    \nelse\n    \nend" },
		{ "如果否則如果","if () then\n    \nelseif () then\n    \nelse\n    \nend" },

		{ "","" },

		{ "人物名稱","char.name" },
		{ "人物等級","char.lv" },


		{ "","" },

		{ "一般動作","bt.charUseAttack()" },
		{ "使用精靈","bt.charUseMagic(,)" },
		{ "使用技能","bt.charUseSkill(,)" },
		{ "使用道具","bt.useItem(,)" },
		{ "寵物一般動作","bt.petUseSkill()" },
		{ "交換寵物","bt.switchPet()" },
		{ "逃跑","bt.escape()" },
		{ "防禦","bt.defense()" },
		{ "捕捉","bt.catchPet()" },
		{ "登出","sys.out()" },

		{ "","" },
		{ "加入庫","local bt <const> = BattleClass();\nlocal sys <const> = SystemClass();\n" },

	};

	// 添加更多映射...

	long long x = 0;
	long long y = 0;
	long long itemWidth = 80; // 调整每个项的宽度，根据需要修改
	long long itemHeight = 20; // 调整每个项的高度，根据需要修改
	for (const QPair<QString, QString>& textPair : textMapping)
	{
		if (textPair.first.isEmpty())//換行
		{
			x = 0;
			++y;
			continue;
		}

		BattleConditionTextItem* item = q_check_ptr(new  BattleConditionTextItem(textPair.first));
		if (item == nullptr)
			continue;

		scene->addItem(item);

		// 计算每个项的位置，靠左对齐
		qreal xPos = x * itemWidth;
		qreal yPos = y * itemHeight;

		item->setPos(xPos, yPos);

		// 存储英文文本到图形项的 UserData 中
		item->setData(Qt::UserRole, textPair.second);

		connect(item, &BattleConditionTextItem::clicked, this, &BattleSettingFrom::onTextBrowserAppendText);

		// 更新 x 和 y 的值
		++x;

		if (x >= 10) // 一行最多10列，如果超过10列，换行
		{
			x = 0;
			++y;
		}
	}

	connect(ui.listWidget, &QListWidget::itemDoubleClicked, this, &BattleSettingFrom::onListWidgetDoubleClicked);

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &BattleSettingFrom::onButtonClicked, Qt::QueuedConnection);
	}

}

BattleSettingFrom::~BattleSettingFrom()
{}

void BattleSettingFrom::onListWidgetAddItem(const QString& text)
{


}

void BattleSettingFrom::onTextBrowserAppendText(const QString& text, bool cacheOnly)
{
	if (text.isEmpty())
		return;

	if (!cacheOnly)
	{
		ui.label_combinepreview->setText(text);
		ui.widget_preview->append(text);
		cacheText.clear();
		return;
	}

	static const QStringList ops = {
		">", "<", "==", "!=", ">=", "<=", "contains", "not contains"
	};

	static const QStringList canrecord = {
	 "contains", "not contains"
	};

	if (cacheText.isEmpty())
	{
		if (!canrecord.contains(text))
			return;

		cacheText = text;
		ui.label_combinepreview->setText(cacheText);
		return;
	}

	if (cacheText.contains("contains"))
	{
		if (!cacheText.contains("("))
		{
			if (ops.contains(text))
				return;

			// "contains(xxxxx" or "not contains(xxxxx"
			QString newText = QString("%1(%2, '')").arg(cacheText).arg(text);
			cacheText = newText;
			ui.label_combinepreview->setText(newText);
			if (!ui.widget_preview->text().isEmpty())
				ui.widget_preview->append(newText);
			else
				ui.widget_preview->setText(newText);
		}
	}

	cacheText.clear();
}

void BattleSettingFrom::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();

	Injector& injector = Injector::getInstance(currentIndex);

	if (name == "pushButton_save")
	{
		QStringList logicList;
		for (long long i = 0; i < ui.listWidget->count(); ++i)
		{
			QListWidgetItem* item = ui.listWidget->item(i);
			if (item == nullptr)
				continue;

			logicList.append(item->text());
		}

		injector.setStringHash(util::kBattleLogicsString, logicList.join("\n"));
		return;
	}

	if (name == "pushButton_testresult")
	{
		Parser parser(getIndex());
		QVariant var = parser.luaDoString(ui.widget_preview->text().simplified());
		if (var.isValid())
		{
			QMessageBox::information(this, tr("Test Result"), tr("Test Result: %1").arg(var.toString()));
		}
		return;
	}

	if (name == "pushButton_addlogic")
	{
		QString text = ui.widget_preview->text().simplified();
		if (text.isEmpty())
			return;

		QListWidgetItem* item = q_check_ptr(new QListWidgetItem(text));
		if (item == nullptr)
			return;

		item->setSizeHint(QSize(0, 18));
		item->setData(Qt::UserRole, QVariant::fromValue(ui.widget_preview->text()));
		ui.listWidget->addItem(item);
		return;
	}

	if (name == "pushButton_logicup")
	{
		util::SwapRowUp(ui.listWidget);
		return;
	}

	if (name == "pushButton_logicdown")
	{
		util::SwapRowDown(ui.listWidget);
		return;
	}
}

void BattleSettingFrom::onListWidgetDoubleClicked(QListWidgetItem* item)
{
	if (item == nullptr)
		return;

	bool isLeftButton = (QApplication::mouseButtons() & Qt::LeftButton);
	if (isLeftButton)
	{
		//remove item
		ui.listWidget->takeItem(ui.listWidget->row(item));
		return;
	}

	QString text = item->data(Qt::UserRole).toString();
	if (text.isEmpty())
		return;

	ui.widget_preview->setText(text);
}