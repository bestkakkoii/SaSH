#pragma once
#include <QtCore/qglobal.h>

#ifndef DISABLE_COPY
#define DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;
#endif
#ifndef DISABLE_MOVE
#define DISABLE_MOVE(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;
#endif
#ifndef DISABLE_COPY_MOVE
#define DISABLE_COPY_MOVE(Class) \
    DISABLE_COPY(Class) \
    DISABLE_MOVE(Class) \
public:\
	static Class& getInstance() {\
		static Class instance;\
		return instance;\
	}
#endif

#ifndef STATICINS
#define STATICINS(Class) Class& g_##Class = Class::getInstance()
#endif

#ifndef global_assume
#define global_assume
inline static constexpr void sash_assume(bool condition) noexcept
{
	if (!condition)
	{
		qt_assert_x("sash_assume()", "Assumption in sash_assume() was not correct", __FILE__, __LINE__);

	}
	__assume(condition);
}
#endif

#pragma region MACROS
//custom
//#define OCR_ENABLE

//sa original
#define _CHAR_PROFESSION
#define _SHOW_FUSION
#define _CHANNEL_MODIFY
#define _CHANNEL_WORLD
#define _OBJSEND_C
#pragma endregion