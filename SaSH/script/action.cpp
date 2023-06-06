#include "stdafx.h"
#include "interpreter.h"
#include "util.h"
#include "injector.h"

//action
int Interpreter::useitem(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}

int Interpreter::dropitem(const TokenMap& TK)
{
	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
		return Parser::kError;

	Injector& injector = Injector::getInstance();
}