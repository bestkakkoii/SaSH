#include "stdafx.h"
#include "settingfiledialog.h"

#include "util.h"

settingfiledialog::settingfiledialog(const QString& defaultName, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setFont(util::getFont());
	setWindowFlags(Qt::Tool | Qt::Dialog | Qt::WindowCloseButtonHint);
	setModal(true);
	setAttribute(Qt::WA_QuitOnClose);
	setWindowTitle("");
	setAttribute(Qt::WA_StyledBackground);


	QStringList list;
	util::searchFiles(util::applicationDirPath() + "/settings", "", ".json", &list, false);

	if (list.isEmpty())
		return;

	if (!defaultName.isEmpty())
	{
		QFileInfo info(defaultName);
		firstItem_ = q_check_ptr(new QListWidgetItem(info.fileName()));
		if (firstItem_ != nullptr)
		{
			firstItem_->setData(Qt::UserRole, QString("custom"));
			firstItem_->setForeground(QColor("#BD5F5F"));
			ui.listWidget->addItem(firstItem_);
			lineEdit_ = q_check_ptr(new QLineEdit(info.fileName()));
			if (lineEdit_ != nullptr)
			{
				util::setLineEdit(lineEdit_);
				connect(lineEdit_, &QLineEdit::textChanged, this, &settingfiledialog::onLineEditTextChanged);
				this->layout()->addWidget(lineEdit_);
			}

		}
	}

	for (const QString& str : list)
	{
		if (str.contains("system.json"))
			continue;

		QFileInfo info(str);

		QListWidgetItem* item = q_check_ptr(new QListWidgetItem(info.fileName()));
		if (item == nullptr)
			continue;

		item->setData(Qt::UserRole, str);
		ui.listWidget->addItem(item);
	}


}

settingfiledialog::~settingfiledialog()
{
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
