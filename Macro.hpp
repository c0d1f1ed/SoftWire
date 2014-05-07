#ifndef SoftWire_Macro_hpp
#define SoftWire_Macro_hpp

#include "TokenList.hpp"

namespace SoftWire
{
	class Macro
	{
	public:
		Macro(TokenList &unit);

		~Macro();

		void expandAll(TokenList &unit);

	private:
		char *name;
		int numArguments;
		TokenList arguments;
		TokenList definition;

		void scanName(TokenList &unit);
		void scanArguments(TokenList &unit);
		void scanDefinition(TokenList &unit);

		void expand(TokenList &unit);

		bool hasArguments() const;
		bool hasDefinition() const;
	};
}

#endif   // SoftWire_Macro_hpp
