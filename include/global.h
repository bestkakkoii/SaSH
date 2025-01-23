#ifndef GLOBAL_H_
#define GLOBAL_H_

namespace global
{
	template <typename Mod, typename Offset, typename Ret>
	void converTo(Mod hBase, Offset offset, Ret* ret)
	{
		if (ret != nullptr)
		{
			*ret = (Ret)(((ULONG_PTR)(hBase)) + (offset));
		}
	}

	extern QTextCodec* gb2312Codec;

}

#endif // GLOBAL_H_