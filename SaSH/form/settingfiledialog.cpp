#include "stdafx.h"
#include "settingfiledialog.h"

#include "util.h"

settingfiledialog::settingfiledialog(const QString& defaultName, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setFont(util::getFont());
	util::setWidget(this);
	setWindowFlags(Qt::Tool | Qt::Dialog | Qt::WindowCloseButtonHint);
	setModal(true);
	setAttribute(Qt::WA_QuitOnClose);
	setWindowTitle("");
	setAttribute(Qt::WA_StyledBackground);


	QStringList list;
	util::searchFiles(util::applicationDirPath() + "/settings", "", ".json", &list, false);

	if (list.isEmpty())
		return;

	list.removeDuplicates();

	QCollator collator = util::getCollator();
	std::sort(list.begin(), list.end(), collator);

	auto createItem = [&](const QString& text, const QString& colorStr = "", const QString& userText = "", QListWidgetItem** pitem = nullptr)->bool
		{
			QFileInfo info(text);
			QListWidgetItem* item = q_check_ptr(new QListWidgetItem(info.fileName()));
			sash_assume(item != nullptr);
			if (item == nullptr)
				return false;

			if (userText.isEmpty())
				item->setData(Qt::UserRole, text);
			else
				item->setData(Qt::UserRole, userText);

			if (!colorStr.isEmpty())
				item->setForeground(QColor(colorStr));

			ui.listWidget->addItem(item);

			if (pitem != nullptr)
				*pitem = item;

			return true;
		};

	do
	{
		if (defaultName.isEmpty())
			break;

		if (!createItem(defaultName, "#BD5F5F", "custom", &firstItem_))
			break;

		if (!defaultName.contains("default.json"))
		{
			if (!createItem("default.json", "#5F5FBD", "default.json", &defaultItem_))
				break;
		}


		lineEdit_ = q_check_ptr(new QLineEdit(firstItem_->text()));
		sash_assume(lineEdit_ != nullptr);
		if (lineEdit_ == nullptr)
		{
			delete firstItem_;
			firstItem_ = nullptr;
			break;
		}

		util::setLineEdit(lineEdit_);
		connect(lineEdit_, &QLineEdit::textChanged, this, &settingfiledialog::onLineEditTextChanged);
		this->layout()->addWidget(lineEdit_);

	} while (false);

	for (const QString& path : list)
	{
		if (path.contains("system.json"))
			continue;

		if (defaultName == path)
			continue;
		if (path.contains("default.json"))
			continue;

		QFileInfo info(path);

		QListWidgetItem* item = nullptr;
		if (!createItem(path, "", "", &item))
			continue;

		ui.listWidget->addItem(item);
	}

	QRect parentRect = parent->geometry();
	QPoint centerPos = parentRect.center();
	QSize thisSize = size();
	QPoint newPos = centerPos - QPoint(thisSize.width() / 2, thisSize.height() / 2);

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();
	move(newPos);
}

settingfiledialog::~settingfiledialog()
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
}

void settingfiledialog::onLineEditTextChanged(const QString& text)
{
	if (firstItem_ == nullptr)
		return;

	firstItem_->setText(text);
}

void settingfiledialog::on_listWidget_itemDoubleClicked(QListWidgetItem* item)
{
	if (item == nullptr)
		return;

	QString text = item->data(Qt::UserRole).toString();
	if (text.isEmpty())
		return;

	if (text == "custom")
	{
		text = lineEdit_->text();
		if (text.isEmpty())
			return;

		text = util::applicationDirPath() + "/settings/" + text;
		if (!text.endsWith(".json"))
			text += ".json";
	}

	returnValue = text;

	accept();
}
