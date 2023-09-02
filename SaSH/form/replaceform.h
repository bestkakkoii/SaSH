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
public:
	explicit FindDialog(CodeEditor* textWidget);
	void setPlainTextEdit(CodeEditor* pText);
	CodeEditor* getPlainTextEdit();

protected:
	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QDialog::showEvent(e);
	}

	virtual bool event(QEvent* evt) override;

protected:
	void onFindClicked();

private slots:
	void onCloseClicked();

private:
	void initControl();

	void connectSlot();

protected:
	QGroupBox radioGrpBx_;

	QGridLayout layout_;
	QHBoxLayout hbLayout_;

	QLabel findLbl_;
	QLineEdit findEdit_;
	QPushButton findBtn_;
	QPushButton closeBtn_;
	QCheckBox matchChkBx_;
	QRadioButton forwardBtn_;
	QRadioButton backwardBtn_;

	CodeEditor* pText_; // FindDialog 聚合使用 QPlainTextEdit
};

class ReplaceDialog : public FindDialog
{
	Q_OBJECT
public:
	explicit ReplaceDialog(CodeEditor* pText = 0);

private slots:
	void onReplaceClicked();
	void onReplaceAllClicked();

private:
	void initControl();

	void connectSlot();

private:
	QLabel replaceLbl_;
	QLineEdit replaceEdit_;
	QPushButton replaceBtn_;
	QPushButton replaceAllBtn_;
};

