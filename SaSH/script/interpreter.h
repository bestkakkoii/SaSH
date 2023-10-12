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

import Utility;

#pragma once
#include <QObject>
#include <atomic>
#include "parser.h"

constexpr __int64 DEFAULT_FUNCTION_TIMEOUT = 5000;

class Interpreter : public ThreadPlugin
{
	Q_OBJECT
public:
	enum VarShareMode
	{
		kNotShare,
		kShare,
	};

public:
	explicit Interpreter(__int64 index);
	virtual ~Interpreter();

	inline bool isRunning() const { return isRunning_.load(std::memory_order_acquire) && !isInterruptionRequested(); }

	void preview(const QString& fileName);

	void doString(const QString& script, Interpreter* pinterpretter, VarShareMode shareMode);

	void doFileWithThread(__int64 beginLine, const QString& fileName);

	bool doFile(__int64 beginLine, const QString& fileName, Interpreter* pinterpretter, Parser* pparser, bool issub, Parser::Mode mode);

	void stop();

signals:
	void finished();

public slots:
	void proc();

private:
	__int64 scriptCallBack(__int64 currentIndex, __int64 currentLine, const TokenMap& token);

private:

	template<typename Func>
	void registerFunction(const QString functionName, Func fun);
	void openLibs();

private:
	bool checkBattleThenWait();
	bool checkOnlineThenWait();

	bool waitfor(__int64 timeout, std::function<bool()> exprfun);
	bool checkString(const TokenMap& TK, __int64 idx, QString* ret);
	bool checkInteger(const TokenMap& TK, __int64 idx, __int64* ret);

	__int64 checkJump(const TokenMap& TK, __int64 idx, bool expr, JumpBehavior behavior);
	bool checkRange(const TokenMap& TK, __int64 idx, __int64* min, __int64* max);
	bool checkRelationalOperator(const TokenMap& TK, __int64 idx, RESERVE* ret) const;

	void logExport(__int64 currentIndex, __int64 currentLine, const QString& text, __int64 color = 0);

	void errorExport(__int64 currentIndex, __int64 currentLine, __int64 level, const QString& text);

private: //註冊給Parser的函數
	//system
	__int64 press(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 run(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 dostr(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 send(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	//check
	__int64 checkdaily(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 waitmap(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 waitdlg(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 waitsay(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 waititem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 waitpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 waitpos(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	//check-group
	__int64 waitteam(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);


	//move
	__int64 setdir(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 walkpos(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	//action
	__int64 useitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 dropitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 swapitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 playerrename(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 petrename(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 setpetstate(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 droppet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 buy(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 sell(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 sellpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 make(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 cook(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 usemagic(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 pickitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 depositgold(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 withdrawgold(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 addpoint(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 learn(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 trade(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 doffstone(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	__int64 recordequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 wearequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 unwearequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 petequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 petunequip(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	__int64 depositpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 deposititem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 withdrawpet(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 withdrawitem(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	__int64 mail(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	//action-group
	__int64 join(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 leave(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);
	__int64 kick(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	//hide
	__int64 ocr(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);

	//battle
	__int64 bh(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//atk
	__int64 bj(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//magic
	__int64 bp(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//skill
	__int64 bs(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//switch
	__int64 be(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//escape
	__int64 bd(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//defense
	__int64 bi(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//item
	__int64 bt(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//catch
	__int64 bn(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//nothing
	__int64 bw(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//petskill
	__int64 bwf(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//pet nothing
	__int64 bwait(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//wait
	__int64 bend(__int64 currentIndex, __int64 currentLine, const TokenMap& TK);//wait

private:
	enum
	{
		WARN_LEVEL = 0,
		ERROR_LEVEL = 1,
	};

	QThread* thread_ = nullptr;
	Parser parser_;
	__int64 beginLine_ = 0;

	std::atomic_bool isRunning_ = false;
	ParserCallBack pCallback = nullptr;
	QList<QSharedPointer<Interpreter>> subInterpreterList_;
	QFutureSynchronizer<bool> futureSync_;
};