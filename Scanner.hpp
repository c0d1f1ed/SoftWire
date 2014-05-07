#ifndef SoftWire_Scanner_hpp
#define SoftWire_Scanner_hpp

#include "TokenList.hpp"

#include "Link.hpp"

namespace SoftWire
{
	class Scanner : public TokenList
	{
	public:
		Scanner();
		Scanner(const char *fileName, bool doPreprocessing = true);

		~Scanner();

		void scanFile(const char *fileName, bool doPreprocessing = true);
		void scanString(const char *sourceString, bool doPreprocessing = true);

		static void defineSymbol(int value, const char *name);
		static void clearSymbols();

	private:
		struct Symbol
		{
			Symbol(int value = 0, const char *name = 0) : value(value), name(name) {};

			int value;
			const char *name;
		};

		char *source;

		enum {tokenMax = 256};   // Maximum token length

		typedef Link<Symbol> SymbolTable;
		static SymbolTable *symbols;

		void scanBuffer(char *source, bool doPreprocessing);
		void preprocess();
		void includeFiles();
		void substituteSymbols();
		void expandMacros();
		void conditionalCompilation();

		int conditionTrue();
		void evaluateUnary();
		void evaluateMultiplicative();
		void evaluateAdditive();
		void evaluateRelational();
		void evaluateEquality();
		void evaluateBitwiseAND();
		void evaluateBitwiseOR();
		void evaluateLogicalAND();
		void evaluateLogicalOR();
		void processDirectives();

		bool cleanupParenthesis();

		void includeFile(const char *fileName);
	};
}

#endif   // SoftWire_Scanner_hpp
