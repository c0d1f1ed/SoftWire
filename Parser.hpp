#ifndef SoftWire_Parser_hpp
#define SoftWire_Parser_hpp

#include "InstructionSet.hpp"

namespace SoftWire
{
	class TokenList;
	class Instruction;
	class Token;
	class Synthesizer;
	class InstructionSet;

	class Parser
	{
	public:
		Parser(TokenList &tokenList, Synthesizer &synthesizer, const InstructionSet &instructionSet);

		~Parser();

		const Encoding &parseLine();
		const char *skipLine() const;

	private:
		TokenList &tokenList;
		Synthesizer &synthesizer;
		const InstructionSet &instructionSet;

		Instruction *instruction;

		void parseLabel();
		void parseMnemonic();
		void parseSpecifier();

		void parseFirstOperand();
		void parseSecondOperand();
		void parseThirdOperand();

		OperandIMM parseImmediate();
		OperandSTR parseLiteral();
		OperandREG parseRegister();
		OperandMEM parseMemoryReference();
	};
}

#endif   // SoftWire_Parser_hpp
