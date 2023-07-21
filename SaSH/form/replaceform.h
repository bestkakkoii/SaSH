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

#pragma once
#include <QDialog>
class CodeEditor;
class FindDialog : public QDialog
{
	Q_OBJECT

protected:
	QGroupBox m_radioGrpBx;

	QGridLayout m_layout;
	QHBoxLayout m_hbLayout;

	QLabel m_findLbl;
	QLineEdit m_findEdit;
	QPushButton m_findBtn;
	QPushButton m_closeBtn;
	QCheckBox m_matchChkBx;
	QRadioButton m_forwardBtn;
	QRadioButton m_backwardBtn;

	CodeEditor* m_pText; // FindDialog 聚合使用 QPlainTextEdit

	void initControl();
	void connectSlot();
protected slots:
	void onFindClicked();
	void onCloseClicked();
public:
	explicit FindDialog(CodeEditor* textWidget);
	void setPlainTextEdit(CodeEditor* pText);
	CodeEditor* getPlainTextEdit();
	bool event(QEvent* evt);


};

class ReplaceDialog : public FindDialog
{
	Q_OBJECT

protected:
	QLabel m_replaceLbl;
	QLineEdit m_replaceEdit;
	QPushButton m_replaceBtn;
	QPushButton m_replaceAllBtn;

	void initControl();
	void connectSlot();
protected slots:
	void onReplaceClicked();
	void onReplaceAllClicked();
public:
	explicit ReplaceDialog(CodeEditor* pText = 0);
};

