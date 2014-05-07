#ifndef SoftWire_CharType_hpp
#define SoftWire_CharType_hpp

#include <ctype.h>

namespace SoftWire
{
#ifdef __unix__
	static int iscsymf(int c)
	{
		return c == '_' || isalpha(c);
	}

	static int iscsym(int c)
	{
		return c == '_' || isalnum(c);
	}
#endif
}

#endif   // SoftWire_CharType_hpp
