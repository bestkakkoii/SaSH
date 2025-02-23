﻿/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

constexpr long long NewlineArrow = 32;

constexpr const char* g_sash_keywords[9] = {
	"",
	/*pink*/
	"call pause exit label jmp back continue "

	/*lua original*/
	"function end goto break for while if repeat until do in then else elseif return ",

	/*yellow*/
	"dlg rnd setglobal getglobal clearglobal capture "
	"print printf sleep msg logout logback eo button say input menu "
	"talk cls set saveset loadset "
	"chpet chname chpetname chmap "
	"usemagic doffpet buy sell useitem doffitem swapitem pickup putitem "
	"getitem putpet getpet make cook uequip requip "
	"wequip pequip puequip skup join leave kick move "
	"walkpos w dir findpath findnpc lclick rclick ldbclick dragto warp "
	"learn trade run dostr sellpet mail reg "
	"regex rex rexg trim upper lower half toint tostr todb replace find full "
	"bh bj bp bs be bd bi bn bw bwn bwait bend "
	"dofile createch delch doffstone send format "
	"tsort trsort split mktable trotate tunique tshuffle tsleft tsright tmerge tjoin "
	"tswap tadd tpadd tpopback tpopfront tfront tback mkpath findfiles contains openlog checkdaily "

	"rungame closegame openwindow setlogin runex stoprunex dostrex loadsetex getgamestate "

	"waitdlg waitsay waititem waitmap waitteam waitpet waitpos "

	/* . */
	"item:count item:indexof item:find "
	"unit.find dialog.contains petskill.find skill.find magic.find "
	"map.isxy map.isrect map.ismap chat.contains "

	/*lua original*/
	"assert collectgarbage dostring "
	"coroutine.close coroutine.create coroutine.isyieldable coroutine.resume coroutine.running coroutine.status coroutine.wrap coroutine.yield "
	"debug.debug debug.gethook debug.getinfo debug.getlocal debug.getmetatable debug.getregistry debug.getupvalue debug.getuservalue debug.setcstacklimit debug.sethook "
	"debug.setlocal debug.setmetatable debug.setupvalue debug.setuservalue debug.traceback debug.upvalueid debug.upvaluejoin "
	"dofile error "
	"file:close file:flush file:lines file:read file:seek file:setvbuf file:write "
	"getmetatable io.close io.flush io.input io.lines io.open io.output io.popen io.read io.tmpfile io.type io.write "
	"ipairs load loadfile "
	"math.abs math.acos math.asin math.atan math.ceil math.cos math.deg math.exp math.floor math.fmod math.log "
	"math.max math.min math.modf math.rad math.random math.randomseed math.sin math.sqrt math.tan math.tointeger math.type math.ult "
	"next "
	"os.clock os.date os.difftime os.execute os.exit os.getenv os.remove os.rename os.setlocale os.time os.tmpname "
	"package.loadlib package.searchpath "
	"pairs pcall print rawequal rawget rawlen rawset require select setmetatable "
	"string.byte string.char string.dump string.find string.format string.gmatch string.gsub string.len string.lower "
	"string.match string.pack string.packsize string.rep string.reverse string.sub string.unpack string.upper "
	"table.concat table.insert table.move table.pack table.remove table.sort table.unpack "
	"tonumber tostring type "
	"utf8.char utf8.codepoint utf8.codes utf8.len utf8.offset warn xpcall ",
	/*light green*/
	"Timer cout "
	"string table "
	"TARGET ",
	/*blue green*/
	"TARGET.SELF TARGET.PET TARGET.ALLIE_ANY TARGET.ALLIE_ALL TARGET.ENEMY_ANY "
	"TARGET.ENEMY_ALL TARGET.ENEMY_FRONT TARGET.ENEMY_BACK TARGET.LEADER TARGET.LEADER_PET "
	"TARGET.TEAM TARGET.TEAM1_PET TARGET.TEAM2 TARGET.TEAM2_PET TARGET.TEAM3 "
	"TARGET.TEAM3_PET TARGET.TEAM4 TARGET.TEAM4_PET ",
	/*dark blue*/
	"local true false any "
	"int double bool not and or nil ",
	/*light blue*/
	"pet magic skill petskill equip petequip map dialog chat point battle char mails "
	/*lua original*/
	"math os file debug coroutine utf8 package io ",
	/*dark orange*/
	"ifdef els elif endif ifndef define undef pragma err lua endlua ",
	/*purple*/
	"vret"
	"LINE FILE FUNCTION ROWCOUNT "
	"PID HWND GAMEPID GAMEHWND GAMEHANDLE THREADID GAME WORLD INDEX "
	"INFINITE MAXPET MAXITEM MAXCHAR MAXSKILL MAXPETSKILL MAXEQUIP MAXCHAT MAXDLG MAXENEMY MAXCARD MAXDIR MAXMAGIC "
	"CURRENTDIR CURRENTSCRIPTDIR SETTINGDIR SCRIPTDIR "
	"isonline isbattle isnormal isdialog item.space item.isfull pet.count team.count "
	"map.name map.floor map.ground map.x map.y unit.count dialog.buttontext dialog.id dialog.unitid dialog.type dialog.button "
	"battle.field battle.round "
	"char.hash char.name char.fname char.modelid char.faceid char.lv char.hp char.maxhp char.hpp char.mp char.maxmp char.mpp char.exp "
	"char.maxexp char.stone char.point char.vit char.str char.tgh char.dex char.atk char.def char.agi char.chasma char.turn "
	"char.earth char.water char.fire char.wind char.battlepet char.ridepet char.mailpet"
	/*lua original*/
	"_G _VERSION "
	"utf8.charpattern package.path package.preload package.searchers package.config package.cpath package.loaded math.huge math.maxinteger math.mininteger math.pi "

};

Highlighter::Highlighter(const QHash<long long, QByteArray>& keywords, QObject* parent)
	: QsciLexerLua(parent)
	, keywords_(keywords)
{
	font_ = util::getFont();

	{
		constexpr const char* kDefaultFont = "YaHei Consolas Hybrid";

		util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		QString fontName = config.read<QString>("ScriptEditorClass", "Highlighter", "Font");
		if (fontName.isEmpty())
		{
			font_.setFamily(kDefaultFont);
			fontName = font_.family();
			config.write("ScriptEditorClass", "Highlighter", "Font", fontName);
		}

		font_.setFamily(fontName.isEmpty() ? kDefaultFont : fontName);

		long long fontSize = config.read<long long>("ScriptEditorClass", "Highlighter", "FontSize");
		if (fontSize <= 0)
		{
			fontSize = 11;
			config.write("ScriptEditorClass", "Highlighter", "FontSize", fontSize);
		}

		font_.setPointSize(fontSize);
	}


}

const char* Highlighter::keywords(int set) const
{
	switch (set)
	{
	case kPink://粉色
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kYellow://QsciLexerLua::BasicFunctions//黃色
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kLightGreen://QsciLexerLua::StringTableMathsFunction//草綠
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kBlueGreen://QsciLexerLua::CoroutinesIOSystemFacilities//青綠
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kDarkBlue://KeywordSet5//深藍色
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kLightBlue://KeywordSet6//淺藍色
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kDarkOrange://KeywordSet7//土橘色
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case kPurple://KeywordSet8//紫色
	{
		return !keywords_.value(set).isEmpty() ? keywords_[set].constData() : g_sash_keywords[set];
	}
	case 9:
		return "";

	default:
		break;
	}

	return QsciLexerLua::keywords(set);
}

// Returns the foreground colour of the text for a style.
QColor Highlighter::defaultColor(int style) const
{
	switch (style)
	{
		//綠色
	case Comment:
	case LineComment:
		return QColor(106, 153, 85);

		//草綠
	case Number:
		return QColor(181, 206, 168);

		//粉色
	case Keyword:
		return QColor(197, 134, 192);

		//土黃色
	case BasicFunctions:
		return QColor(220, 220, 170);

		//草綠
	case CoroutinesIOSystemFacilities:
		return QColor(181, 206, 168);

		//土橘色
	case Label:
	case String:
	case Character:
	case LiteralString:
	case Preprocessor:
		return QColor(206, 145, 120);

	case KeywordSet7:
	case Operator:
		return QColor(206, 145, 0);

		//灰白色
	case Default:
	case Identifier:
		return QColor(212, 212, 212);

		//乳藍色

		//return QColor(77, 177, 252);

		//青綠
	case StringTableMathsFunctions:
	case UnclosedString:
		return QColor(78, 201, 176);

		//深藍色
	case KeywordSet5:
		return QColor(86, 156, 214);

		//淺藍色
	case KeywordSet6:
		return QColor(156, 220, 254);

		//紫色
	case KeywordSet8:
		return QColor(190, 183, 255);

	case NewlineArrow:
		return QColor(80, 80, 255);

	default:
		break;
	}

	return QsciLexerLua::defaultColor(style);
}

// Returns the font of the text for a style.
QFont Highlighter::defaultFont(int) const
{
	return font_;
}

// Return the set of character sequences that can separate auto-completion
// words.
QStringList Highlighter::autoCompletionWordSeparators() const
{
	return QsciLexerLua::autoCompletionWordSeparators();
}

// Returns the background colour of the text for a style.
QColor Highlighter::defaultPaper(int) const
{
	static QColor color(30, 30, 30);
	return color;
}

// Default implementation to return the set of fill up characters that can end
// auto-completion.
const char* Highlighter::autoCompletionFillups() const
{
	return QsciLexerLua::autoCompletionFillups();
}

// Return the list of characters that can end a block.
const char* Highlighter::blockEnd(int* style) const
{
	return QsciLexerLua::blockEnd(style);
}

const char* Highlighter::blockStartKeyword(int* style) const
{
	return QsciLexerLua::blockStartKeyword(style);
}

// Return the list of characters that can start a block.
const char* Highlighter::blockStart(int* style) const
{
	return QsciLexerLua::blockStart(style);
}

// Return the string of characters that comprise a word.
const char* Highlighter::wordCharacters() const
{
	return QsciLexerLua::wordCharacters();
}

// Returns the end-of-line fill for a style.
bool Highlighter::defaultEolFill(int style) const
{
	return QsciLexerLua::defaultEolFill(style);
}