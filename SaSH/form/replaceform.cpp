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
	setAttribute(Qt::WA_DeleteOnClose);
	m_findLbl.setText("Find What:");
	m_findBtn.setText("Find Next");
	m_closeBtn.setText("Close");
	m_matchChkBx.setText("Match Case");
	m_backwardBtn.setText("Backward");
	m_forwardBtn.setText("Forward");
	m_forwardBtn.setChecked(true);
	m_radioGrpBx.setTitle("Direction");

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
	QString cmpText = m_findEdit.text();

	if ((m_pText != nullptr) && (!cmpText.isEmpty()))
	{
		QString text = m_pText->text();
		if (text.isEmpty())
		{
			return;
		}

		int fromLineIndex = 0;
		int fromIndex = 0;
		m_pText->getCursorPosition(&fromLineIndex, &fromIndex);

		int index = -1;
		int lineIndex = fromLineIndex;
		int currentIndex = fromIndex;

		if (m_forwardBtn.isChecked())
		{
			index = text.indexOf(cmpText, currentIndex, m_matchChkBx.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

			if (index >= 0)
			{
				lineIndex = text.left(index).count("\n");
				currentIndex = index;
			}
		}
		else if (m_backwardBtn.isChecked())
		{
			index = text.lastIndexOf(cmpText, currentIndex, m_matchChkBx.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

			if (index >= 0)
			{
				lineIndex = text.left(index).count("\n");
				currentIndex = index;
			}
		}

		if (index >= 0)
		{
			m_pText->setCursorPosition(lineIndex, currentIndex);
			m_pText->setSelection(lineIndex, currentIndex, lineIndex, currentIndex + cmpText.length());
		}
		else
		{
			QMessageBox msg(this);
			msg.setWindowTitle("Find");
			msg.setText("Can not find \"" + cmpText + "\" anymore...");
			msg.setIcon(QMessageBox::Information);
			msg.setStandardButtons(QMessageBox::Ok);
			msg.exec();
		}
	}
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
	setAttribute(Qt::WA_DeleteOnClose);
	m_replaceLbl.setText("Replace To:");
	m_replaceBtn.setText("Replace");
	m_replaceAllBtn.setText("Replace All");

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
	QString target = m_findEdit.text();
	QString to = m_replaceEdit.text();

	if ((m_pText != NULL) && (target != "") && (to != ""))
	{
		QString text = m_pText->text();

		text.replace(target, to, m_matchChkBx.isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

		m_pText->clear();

		m_pText->insert(text);
	}
}

