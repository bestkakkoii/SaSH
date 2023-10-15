/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "stdafx.h"
#include "replaceform.h"
#include "codeeditor.h"

FindDialog::FindDialog(CodeEditor* textWidget)
	: QDialog(textWidget, Qt::WindowCloseButtonHint | Qt::Drawer), pText_(textWidget)
{
	initControl();
	connectSlot();
	setLayout(&layout_);
	setWindowTitle("Find");
}

void FindDialog::initControl()
{
	QString buttonStyle = R"(
		QPushButton {
		background-color: rgb(66, 66, 66);
		border: 0px solid #dcdfe6;
		padding: 3px;
		color:rgb(214, 214, 214);
		}

		QPushButton:hover {
		background-color: rgb(64, 53, 130);
		color:rgb(250, 250, 250);
		}

		QPushButton:pressed, QPushButton:checked {
		background-color: rgb(145, 131, 238);
		color:rgb(250, 250, 250);
		}
	)";

	QString lineEditStyle = R"(
		QLineEdit{
		  color:rgb(250,250,250);
		  font-size:12px;
		  padding: 1px 15px 1px 3px;
		  border:1px solid rgb(66,66,66);
		  background: rgb(56,56,56);
		}

		QLineEdit::hover{
		  color:rgb(250,250,250);
		  font-size:12px;
		  padding: 1px 15px 1px 3px;
		  border:1px solid rgb(153,153,153);
		  background: rgb(31,31,31);
		}
	)";

	setAttribute(Qt::WA_DeleteOnClose);
	findLbl_.setText(tr("Find What:"));
	findBtn_.setText(tr("Find Next"));
	findBtn_.setStyleSheet(buttonStyle);
	closeBtn_.setText(tr("Close"));
	closeBtn_.setStyleSheet(buttonStyle);
	matchChkBx_.setText(tr("Match Case"));
	backwardBtn_.setText(tr("Backward"));
	forwardBtn_.setText(tr("Forward"));
	forwardBtn_.setChecked(true);
	radioGrpBx_.setTitle(tr("Direction"));

	findEdit_.setStyleSheet(lineEditStyle);

	hbLayout_.addWidget(&forwardBtn_);
	hbLayout_.addWidget(&backwardBtn_);

	radioGrpBx_.setLayout(&hbLayout_);

	layout_.setSpacing(10);
	layout_.addWidget(&findLbl_, 0, 0);
	layout_.addWidget(&findEdit_, 0, 1);
	layout_.addWidget(&findBtn_, 0, 2);
	layout_.addWidget(&matchChkBx_, 1, 0);
	layout_.addWidget(&radioGrpBx_, 1, 1);
	layout_.addWidget(&closeBtn_, 1, 2);

}

void FindDialog::connectSlot()
{
	connect(&findBtn_, SIGNAL(clicked()), this, SLOT(onFindClicked()));
	connect(&closeBtn_, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
}

void FindDialog::setPlainTextEdit(CodeEditor* pText)
{
	pText_ = pText;
}

CodeEditor* FindDialog::getPlainTextEdit()
{
	return pText_;
}

bool FindDialog::event(QEvent* evt)
{
	if (evt->type() == QEvent::Close)
	{
		hide();

		return true;
	}

	return QDialog::event(evt);
}

void FindDialog::onFindClicked()
{
	//取當前要查找的字符串
	QString cmpText = findEdit_.text();

	if ((pText_ == nullptr) || (cmpText.isEmpty()))
		return;

	//取搜索來源全內容
	QString text = pText_->text();
	if (text.isEmpty())
	{
		return;
	}

	text.replace("\r\n", "\n");

	QStringList strList = text.split("\n");

	//取當前光標位置
	int fromLineIndex = 0;
	int fromIndex = 0;
	pText_->getCursorPosition(&fromLineIndex, &fromIndex);

	//取當前選中的內容
	int lineFrom = 0;
	int indexFrom = 0;
	int lineTo = 0;
	int indexTo = 0;
	pText_->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
	QString selectText = pText_->selectedText();

	// 確定起始行和起始索引
	long long startLine = fromLineIndex;
	long long startIndex = fromIndex;

	// 是否順向搜索
	bool isForward = forwardBtn_.isChecked();

	if (!selectText.isEmpty())
	{
		// 如果有選中內容，則根據選中內容的末尾位置作為起始位置
		startLine = lineTo;
		if (isForward)
			startIndex = indexTo;
		else
			startIndex = indexFrom;
	}

	// 是否區分大小寫
	Qt::CaseSensitivity caseSensitivity = matchChkBx_.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

	long long nowStartIndex = startIndex;
	long long nowEndIndex = -1;
	if (isForward)
	{
		for (long long i = startLine; i < strList.size(); ++i)
		{
			QString lineText = strList.value(i);

			nowStartIndex = lineText.indexOf(cmpText, nowStartIndex, caseSensitivity);
			if (nowStartIndex == -1)
			{
				// 從下一行開始搜索
				nowStartIndex = 0;
				continue;
			}
			nowEndIndex = nowStartIndex + cmpText.length();


			if (nowStartIndex != -1)
			{
				pText_->setSelection(i, nowStartIndex, i, nowEndIndex);
				pText_->ensureLineVisible(i);
				return;
			}
		}
	}
	else
	{
		for (long long i = startLine; i >= 0; --i)
		{
			QString lineText = strList.value(i);

			long long lineLength = lineText.length();
			nowStartIndex = lineText.lastIndexOf(cmpText, nowStartIndex, caseSensitivity);
			if (nowStartIndex == -1)
			{
				// 從上一行末尾開始搜索
				nowStartIndex = lineLength;
				continue;
			}
			nowEndIndex = nowStartIndex + cmpText.length();

			if (nowStartIndex != -1)
			{
				// 如果找到匹配的字符串，但該字符串是當前選中的內容，則繼續搜索上一個匹配
				if (i == lineTo && nowStartIndex <= indexTo && nowEndIndex >= indexFrom)
				{
					nowStartIndex = nowStartIndex - cmpText.length();
					continue;
				}

				pText_->setCursorPosition(i, nowStartIndex);
				pText_->setSelection(i, nowStartIndex, i, nowEndIndex);
				pText_->ensureLineVisible(i);
				return;
			}
		}
	}

	// 沒有找到
	QMessageBox::information(this, tr("Find"), tr("Can not find \"%1\".").arg(cmpText));

}


void FindDialog::onCloseClicked()
{
	close();
}

ReplaceDialog::ReplaceDialog(CodeEditor* pText) :
	FindDialog(pText)
{

	initControl();
	connectSlot();
	setWindowTitle("Replace");
}

void ReplaceDialog::initControl()
{
	QString buttonStyle = R"(
		QPushButton {
		background-color: rgb(66, 66, 66);
		border: 0px solid #dcdfe6;
		padding: 3px;
		color:rgb(214, 214, 214);
		}

		QPushButton:hover {
		background-color: rgb(64, 53, 130);
		color:rgb(250, 250, 250);
		}

		QPushButton:pressed, QPushButton:checked {
		background-color: rgb(145, 131, 238);
		color:rgb(250, 250, 250);
		}
	)";

	QString lineEditStyle = R"(
		QLineEdit{
		  color:rgb(250,250,250);
		  font-size:12px;
		  padding: 1px 15px 1px 3px;
		  border:1px solid rgb(66,66,66);
		  background: rgb(56,56,56);
		}

		QLineEdit::hover{
		  color:rgb(250,250,250);
		  font-size:12px;
		  padding: 1px 15px 1px 3px;
		  border:1px solid rgb(153,153,153);
		  background: rgb(31,31,31);
		}
	)";

	setAttribute(Qt::WA_DeleteOnClose);
	replaceLbl_.setText(tr("Replace To:"));
	replaceBtn_.setText(tr("Replace"));
	replaceBtn_.setStyleSheet(buttonStyle);
	replaceAllBtn_.setText(tr("Replace All"));
	replaceAllBtn_.setStyleSheet(buttonStyle);

	replaceEdit_.setStyleSheet(lineEditStyle);

	layout_.removeWidget(&matchChkBx_); // 父類的構造函數已經初始化，所以需要移除
	layout_.removeWidget(&radioGrpBx_);
	layout_.removeWidget(&closeBtn_);

	layout_.addWidget(&replaceLbl_, 1, 0);
	layout_.addWidget(&replaceEdit_, 1, 1);
	layout_.addWidget(&replaceBtn_, 1, 2);
	layout_.addWidget(&matchChkBx_, 2, 0);
	layout_.addWidget(&radioGrpBx_, 2, 1);
	layout_.addWidget(&replaceAllBtn_, 2, 2);
	layout_.addWidget(&closeBtn_, 3, 2);
}

void ReplaceDialog::connectSlot()
{
	connect(&replaceBtn_, SIGNAL(clicked()), this, SLOT(onReplaceClicked()));
	connect(&replaceAllBtn_, SIGNAL(clicked()), this, SLOT(onReplaceAllClicked()));
}

void ReplaceDialog::onReplaceClicked()
{
	QString target = findEdit_.text();
	QString to = replaceEdit_.text();

	if ((pText_ != NULL) && (target != "") && (to != ""))
	{
		//QString selText = m_pText->textCursor().selectedText();//第一次沒有選擇任何文本
		QString selText = pText_->selectedText();
		if (selText == target)
		{
			pText_->insert(to);
		}

		onFindClicked();//第一次不會替換，會查找，符合Windows上的記事本替換準則
	}
}

void ReplaceDialog::onReplaceAllClicked()
{
	//select all
	pText_->selectAll();

	QString target = findEdit_.text();
	QString to = replaceEdit_.text();

	if (target.isEmpty() || to.isEmpty())
	{
		return;
	}

	if (target == to)
	{
		return;
	}

	QString str = pText_->selectedText();

	str.replace(target, to, matchChkBx_.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

	pText_->replaceSelectedText(str);
}

