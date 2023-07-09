#include "stdafx.h"
#include "replaceform.h"
#include "codeeditor.h"

FindDialog::FindDialog(CodeEditor* textWidget)
	: QDialog(textWidget, Qt::WindowCloseButtonHint | Qt::Drawer), m_pText(textWidget)
{
	initControl();
	connectSlot();
	setLayout(&m_layout);
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
	m_findLbl.setText(tr("Find What:"));
	m_findBtn.setText(tr("Find Next"));
	m_findBtn.setStyleSheet(buttonStyle);
	m_closeBtn.setText(tr("Close"));
	m_closeBtn.setStyleSheet(buttonStyle);
	m_matchChkBx.setText(tr("Match Case"));
	m_backwardBtn.setText(tr("Backward"));
	m_forwardBtn.setText(tr("Forward"));
	m_forwardBtn.setChecked(true);
	m_radioGrpBx.setTitle(tr("Direction"));

	m_findEdit.setStyleSheet(lineEditStyle);

	m_hbLayout.addWidget(&m_forwardBtn);
	m_hbLayout.addWidget(&m_backwardBtn);

	m_radioGrpBx.setLayout(&m_hbLayout);

	m_layout.setSpacing(10);
	m_layout.addWidget(&m_findLbl, 0, 0);
	m_layout.addWidget(&m_findEdit, 0, 1);
	m_layout.addWidget(&m_findBtn, 0, 2);
	m_layout.addWidget(&m_matchChkBx, 1, 0);
	m_layout.addWidget(&m_radioGrpBx, 1, 1);
	m_layout.addWidget(&m_closeBtn, 1, 2);

}

void FindDialog::connectSlot()
{
	connect(&m_findBtn, SIGNAL(clicked()), this, SLOT(onFindClicked()));
	connect(&m_closeBtn, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
}

void FindDialog::setPlainTextEdit(CodeEditor* pText)
{
	m_pText = pText;
}

CodeEditor* FindDialog::getPlainTextEdit()
{
	return m_pText;
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
	QString cmpText = m_findEdit.text();

	if ((m_pText == nullptr) || (cmpText.isEmpty()))
	{
		return;
	}

	//取搜索來源全內容
	QString text = m_pText->text();
	if (text.isEmpty())
	{
		return;
	}

	text.replace("\r\n", "\n");

	QStringList strList = text.split("\n");

	//取當前光標位置
	int fromLineIndex = 0;
	int fromIndex = 0;
	m_pText->getCursorPosition(&fromLineIndex, &fromIndex);

	//取當前選中的內容
	int lineFrom = 0;
	int indexFrom = 0;
	int lineTo = 0;
	int indexTo = 0;
	m_pText->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
	QString selectText = m_pText->selectedText();

	// 确定起始行和起始索引
	int startLine = fromLineIndex;
	int startIndex = fromIndex;

	// 是否顺向搜索
	bool isForward = m_forwardBtn.isChecked();

	if (!selectText.isEmpty())
	{
		// 如果有选中内容，则根据选中内容的末尾位置作为起始位置
		startLine = lineTo;
		if (isForward)
			startIndex = indexTo;
		else
			startIndex = indexFrom;
	}

	// 是否区分大小写
	Qt::CaseSensitivity caseSensitivity = m_matchChkBx.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

	int nowStartIndex = startIndex;
	int nowEndIndex = -1;
	if (isForward)
	{
		for (int i = startLine; i < strList.size(); ++i)
		{
			QString lineText = strList.at(i);

			nowStartIndex = lineText.indexOf(cmpText, nowStartIndex, caseSensitivity);
			if (nowStartIndex == -1)
			{
				// 从下一行开始搜索
				nowStartIndex = 0;
				continue;
			}
			nowEndIndex = nowStartIndex + cmpText.length();


			if (nowStartIndex != -1)
			{
				m_pText->setSelection(i, nowStartIndex, i, nowEndIndex);
				m_pText->ensureLineVisible(i);
				return;
			}
		}
	}
	else
	{
		for (int i = startLine; i >= 0; --i)
		{
			QString lineText = strList.at(i);

			int lineLength = lineText.length();
			nowStartIndex = lineText.lastIndexOf(cmpText, nowStartIndex, caseSensitivity);
			if (nowStartIndex == -1)
			{
				// 从上一行末尾开始搜索
				nowStartIndex = lineLength;
				continue;
			}
			nowEndIndex = nowStartIndex + cmpText.length();

			if (nowStartIndex != -1)
			{
				// 如果找到匹配的字符串，但该字符串是当前选中的内容，则继续搜索上一个匹配
				if (i == lineTo && nowStartIndex <= indexTo && nowEndIndex >= indexFrom)
				{
					nowStartIndex = nowStartIndex - cmpText.length();
					continue;
				}

				m_pText->setCursorPosition(i, nowStartIndex);
				m_pText->setSelection(i, nowStartIndex, i, nowEndIndex);
				m_pText->ensureLineVisible(i);
				return;
			}
		}
	}

	// 没有找到
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
	m_replaceLbl.setText(tr("Replace To:"));
	m_replaceBtn.setText(tr("Replace"));
	m_replaceBtn.setStyleSheet(buttonStyle);
	m_replaceAllBtn.setText(tr("Replace All"));
	m_replaceAllBtn.setStyleSheet(buttonStyle);

	m_replaceEdit.setStyleSheet(lineEditStyle);

	m_layout.removeWidget(&m_matchChkBx); // 父类的构造函数已经初始化，所以需要移除
	m_layout.removeWidget(&m_radioGrpBx);
	m_layout.removeWidget(&m_closeBtn);

	m_layout.addWidget(&m_replaceLbl, 1, 0);
	m_layout.addWidget(&m_replaceEdit, 1, 1);
	m_layout.addWidget(&m_replaceBtn, 1, 2);
	m_layout.addWidget(&m_matchChkBx, 2, 0);
	m_layout.addWidget(&m_radioGrpBx, 2, 1);
	m_layout.addWidget(&m_replaceAllBtn, 2, 2);
	m_layout.addWidget(&m_closeBtn, 3, 2);
}

void ReplaceDialog::connectSlot()
{
	connect(&m_replaceBtn, SIGNAL(clicked()), this, SLOT(onReplaceClicked()));
	connect(&m_replaceAllBtn, SIGNAL(clicked()), this, SLOT(onReplaceAllClicked()));
}

void ReplaceDialog::onReplaceClicked()
{
	QString target = m_findEdit.text();
	QString to = m_replaceEdit.text();

	if ((m_pText != NULL) && (target != "") && (to != ""))
	{
		//QString selText = m_pText->textCursor().selectedText();//第一次没有选择任何文本
		QString selText = m_pText->selectedText();
		if (selText == target)
		{
			m_pText->insert(to);
		}

		onFindClicked();//第一次不会替换，会查找，符合Windows上的记事本替换准则
	}
}

void ReplaceDialog::onReplaceAllClicked()
{
	//select all
	m_pText->selectAll();

	QString target = m_findEdit.text();
	QString to = m_replaceEdit.text();

	if (target.isEmpty() || to.isEmpty())
	{
		return;
	}

	if (target == to)
	{
		return;
	}

	QString str = m_pText->selectedText();

	str.replace(target, to, m_matchChkBx.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

	m_pText->replaceSelectedText(str);
}

