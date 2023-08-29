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
#include <highlighter.h>

Highlighter::Highlighter(QObject* parent)
	: QsciLexerLua(parent)
	, m_font("YaHei Consolas Hybrid", 10, 570/*QFont::DemiBold*/, false)
{
}

const char* Highlighter::keywords(int set) const
{
	//	qDebug() << set << QsciLexerLua::keywords(set);
	switch (set)
	{
	case 1://粉色
	{
		//lua key word
		return "goto call function end pause exit label jmp return back break for endfor continue "
			"if ifmap ifplayer ifpet ifpetex ifitem ifteam ifitemfull ifdaily ifbattle ifpos ifonline ifnormal ";
	}
	case 2://QsciLexerLua::BasicFunctions//黃色
	{
		return "ocr dlg "
			"print sleep timer msg logout logback eo button say input menu "
			"talk cls set saveset loadset "
			"chpet chplayername chpetname chmap "
			"usemagic doffpet buy sell useitem doffitem swapitem pickup put "
			"get putpet getpet make cook uequip requip "
			"wequip pequip puequip skup join leave kick move "
			"walkpos w dir findpath movetonpc lclick rclick ldbclick dragto warp "
			"learn trade run dostring sellpet mail reg "
			"regex rex rexg upper lower half toint tostr todb replace find full "
			"bh bj bp bs be bd bi bn bw bwf bwait bend "
			"dofile createch delch doffstone ";
	}
	case 3://QsciLexerLua::StringTableMathsFunction//草綠
	{
		return "";
	}
	case 4://QsciLexerLua::CoroutinesIOSystemFacilities//青綠
	{
		return "";
	}
	case 5://KeywordSet5//深藍色
	{
		return "local var delete releaseall format rnd true false";
	}
	case 6://KeywordSet6//淺藍色
	{
		return "out player pet magic skill petskill equip petequip map dialog chat point battle ";
	}
	case 7://KeywordSet7//土橘色
	{
		return "";
	}
	case 8://KeywordSet8//紫色
	{
		return "waitdlg waitsay waititem waitmap waitteam ";
	}
	case 9:
		break;
	}

	return QsciLexerLua::keywords(set);
}

// Returns the foreground colour of the text for a style.
QColor Highlighter::defaultColor(int style) const
{
	switch (style)
	{
		//case Default:
			//return QColor(85, 85, 85);

	case Comment://綠色
	case LineComment:
		return QColor(106, 153, 85);

	case Number://草綠
		return QColor(181, 206, 168);

	case Keyword://粉色
		return QColor(197, 134, 192);

	case BasicFunctions://土黃色
		return QColor(220, 220, 170);

	case CoroutinesIOSystemFacilities://草綠
		return QColor(181, 206, 168);

	case KeywordSet7://土橘色
	case String:
	case Character:
	case LiteralString:
	case Preprocessor:
		return QColor(206, 145, 120);
	case Label:
		return QColor(206, 145, 0);

	case Default:
	case Operator://灰白色
	case Identifier:
		return QColor(212, 212, 212);

	case StringTableMathsFunctions://青綠
	case UnclosedString:
		return QColor(78, 201, 176);

	case KeywordSet5:
		return QColor(86, 156, 214);//深藍色

	case KeywordSet6:
		return QColor(156, 220, 254);//淺藍色

	case KeywordSet8:
		return QColor(190, 183, 255);//紫色
	}

	return QsciLexer::defaultColor(style);
}

// Returns the font of the text for a style.
QFont Highlighter::defaultFont(int) const
{
	return m_font;
}

// Return the set of character sequences that can separate auto-completion
// words.
QStringList Highlighter::autoCompletionWordSeparators() const
{
	QStringList wl;

	wl << ":" << ".";

	return wl;
}

// Returns the background colour of the text for a style.
QColor Highlighter::defaultPaper(int style) const
{
	Q_UNUSED(style);

	return QColor(30, 30, 30);//QsciLexer::defaultPaper(style);
}

// Default implementation to return the set of fill up characters that can end
// auto-completion.
const char* Highlighter::autoCompletionFillups() const
{
	return "] ) } \" '";
}

// Return the list of characters that can end a block.
const char* Highlighter::blockEnd(int* style) const
{
	if (style)
		*style = Operator;

	return "end endfor";
}

const char* Highlighter::blockStartKeyword(int* style) const
{
	if (style)
		*style = Keyword;

	return "function for";
}

// Return the list of characters that can start a block.
const char* Highlighter::blockStart(int* style) const
{
	if (style)
		*style = Operator;

	return "function for";
}

// Return the string of characters that comprise a word.
const char* Highlighter::wordCharacters() const
{
	return "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_#!";
}

// Returns the end-of-line fill for a style.
bool Highlighter::defaultEolFill(int style) const
{
	switch (style)
	{
	case InactiveUnclosedString:
	case InactiveVerbatimString:
	case InactiveRegex:
	case TripleQuotedVerbatimString:
	case InactiveTripleQuotedVerbatimString:
	case HashQuotedString:
	case InactiveHashQuotedString:
	case Comment:
	case UnclosedString:
		return true;
	}

	return QsciLexer::defaultEolFill(style);
}