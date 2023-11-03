#include "stdafx.h"
#include "findandreplaceform.h"
#include "util.h"

FindAndReplaceForm::FindAndReplaceForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose);
	setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

	QStringList nameCheckList;
	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button && !nameCheckList.contains(button->objectName()))
		{
			nameCheckList.append(button->objectName());
			connect(button, &PushButton::clicked, this, &FindAndReplaceForm::onButtonClicked, Qt::UniqueConnection);
		}
	}

	QList<QComboBox*> comboBoxList = util::findWidgets<QComboBox>(this);
	for (auto& combobox : comboBoxList)
	{
		if (combobox && !nameCheckList.contains(combobox->objectName()))
		{
			nameCheckList.append(combobox->objectName());
			connect(combobox, &QComboBox::currentTextChanged, this, &FindAndReplaceForm::onComboBoxTextChanged, Qt::UniqueConnection);
		}
	}
}

FindAndReplaceForm::~FindAndReplaceForm()
{}

void FindAndReplaceForm::showEvent(QShowEvent* event)
{
	isFirstFindPrev_ = true;
	isFirstFindNext_ = true;
	QWidget::showEvent(event);
	ui.comboBox_keyword->setFocus();
}
void FindAndReplaceForm::closeEvent(QCloseEvent* e)
{
	isFirstFindPrev_ = true;
	isFirstFindNext_ = true;
	QWidget::closeEvent(e);
}

void FindAndReplaceForm::setSearchText(const QString& text)
{
	ui.comboBox_keyword->setCurrentText(text);
	long long check = ui.comboBox_keyword->findText(text);
	if (-1 == check)
		ui.comboBox_keyword->insertItem(0, text);
}

void FindAndReplaceForm::onComboBoxTextChanged(const QString& text)
{
	QComboBox* pComboBox = qobject_cast<QComboBox*>(sender());
	if (nullptr == pComboBox)
		return;

	QString name = pComboBox->objectName();

	if (name == "comboBox_keyword")
	{
		//搜索表達式改變重製標誌
		isFirstFindPrev_ = true;
		isFirstFindNext_ = true;
		return;
	}

	if (name == "comboBox_replacekeyword")
	{
		return;
	}
}

void FindAndReplaceForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (nullptr == pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	QString expr = ui.comboBox_keyword->currentText();
	bool isCaseSensitive = ui.checkBox_casesensitive->isChecked();
	bool isWholeWordMatch = ui.checkBox_wholewordmatch->isChecked();
	bool isRegularExpression = ui.checkBox_regularexpression->isChecked();

	if (name == "pushButton_findprevious")
	{

		if (isFirstFindPrev_)
		{
			isFirstFindPrev_ = false;//關閉首次向前搜索標誌
			isFirstFindNext_ = true;//開啟首次向後搜索標誌
			emit findFirsted(
				expr, isRegularExpression, isCaseSensitive, isWholeWordMatch,
				false /* continue when EOF */,
				false /* true: forward, false: backward */,
				-1    /* begin line */,
				-1    /* begin index */,
				true  /* show when found */,
				true  /* expr match posix */);
			long long check = ui.comboBox_keyword->findText(expr);
			if (-1 == check)
				ui.comboBox_keyword->insertItem(0, expr);
		}
		else
		{
			emit findNexted();
		}

		return;
	}

	if (name == "pushButton_findnext")
	{
		if (isFirstFindNext_)
		{
			isFirstFindNext_ = false; //關閉首次向後搜索標誌
			isFirstFindPrev_ = true;  //開啟首次向前搜索標誌
			emit findFirsted(
				expr, isRegularExpression, isCaseSensitive, isWholeWordMatch,
				false /* continue when EOF */,
				true  /* true: forward, false: backward */,
				-1    /* begin line */,
				-1    /* begin index */,
				true  /* show when found */,
				true  /* expr match posix */);

			//if the first one is not found, add it to the combobox
			long long check = ui.comboBox_keyword->findText(expr);
			if (-1 == check)
				ui.comboBox_keyword->insertItem(0, expr);
		}
		else
		{
			emit findNexted();
		}

		return;
	}

	if (name == "pushButton_findall")
	{
		//重製標誌
		isFirstFindPrev_ = true;
		isFirstFindNext_ = true;

		emit findAlled(
			expr, isRegularExpression, isCaseSensitive, isWholeWordMatch,
			false /* continue when EOF */,
			true  /* true: forward, false: backward */,
			-1    /* begin line */,
			-1    /* begin index */,
			true  /* show when found */,
			true  /* expr match posix */);

		return;
	}

	if (name == "pushButton_replaceall")
	{
		//重製標誌
		isFirstFindPrev_ = true;
		isFirstFindNext_ = true;

		QString replaceText = ui.comboBox_replacekeyword->currentText();
		long long check = ui.comboBox_replacekeyword->findText(replaceText);
		if (-1 == check)
			ui.comboBox_replacekeyword->insertItem(0, replaceText);

		emit replaceAlled(replaceText,
			expr, isRegularExpression, isCaseSensitive, isWholeWordMatch,
			false /* continue when EOF */,
			true  /* true: forward, false: backward */,
			-1    /* begin line */,
			-1    /* begin index */,
			true  /* show when found */,
			true  /* expr match posix */);
		return;
	}
}
