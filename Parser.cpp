#include "Parser.hpp"

#include "TokenList.hpp"
#include "Token.hpp"
#include "Error.hpp"
#include "String.hpp"
#include "Synthesizer.hpp"

namespace SoftWire
{
	#error Important: In newer versions of SoftWire the built-in file assembler might be removed.
	#error Classes like the Parser, Scanner, Token would be deprecated. The reason for this is
	#error that it is in the way of new development, and has no added value for me any more.
	#error Run-time intrinsics are far more powerful, while the file assembler hasn't changed a
	#error single bit for a whole year. The file assembler would still be available in older 
	#error versions of course. If you are using the file assembler and/or would like to keep it in
	#error newer versions, for any reason, please let me know! E-mail: nicolas@capens.net
	#error Remove these lines to continue compilation.

	Parser::Parser(TokenList &tokenList, Synthesizer &synthesizer, const InstructionSet &instructionSet) : tokenList(tokenList), synthesizer(synthesizer), instructionSet(instructionSet)
	{
	}

	Parser::~Parser()
	{
	}

	const Encoding &Parser::parseLine()
	{
		instruction = 0;
		synthesizer.reset();

		if(!tokenList.isEndOfLine())
		{
			parseLabel();
		}
		if(!tokenList.isEndOfLine())
		{
			parseMnemonic();
			parseFirstOperand();
			parseSecondOperand();
			parseThirdOperand();
		}

		int shortestSize = 16;
		Instruction *bestMatch = 0;

		if(instruction)
		{
			do
			{
				if(instruction->matchSyntax())
				{
					const int size = instruction->approximateSize();

					if(size < shortestSize)
					{
						bestMatch = instruction;
						shortestSize = size;
					}
				}

				instruction = instruction->getNext();
			}
			while(instruction);

			if(!bestMatch)
			{
				throw Error("Operands mismatch");
			}
		}

		return synthesizer.encodeInstruction(bestMatch);
	}

	const char *Parser::skipLine() const
	{
		while(!tokenList.isEndOfLine())
		{
			tokenList.advance();
		}

		const char *currentLine = tokenList.getString();

		if(!tokenList.isEndOfFile())
		{
			tokenList.advance();
		}

		return currentLine;
	}

	void Parser::parseLabel()
	{
		if(tokenList.isIdentifier() && tokenList.lookAhead().isPunctuator(':'))
		{
			synthesizer.defineLabel(tokenList.getString());

			tokenList.advance(2);
		}
	}

	void Parser::parseMnemonic()
	{
		char mnemonic[256] = {0};

		strcpy(mnemonic, tokenList.getString());
		tokenList.advance();

		if(stricmp(mnemonic, "LOCK") == 0)
		{
			strcat(mnemonic, " ");
			strcat(mnemonic, tokenList.getString());
			tokenList.advance();
		}

		instruction = instructionSet.query(mnemonic);

		if(!instruction)
		{
			throw Error("Unrecognized mnemonic '%s'", mnemonic);
		}
	}

	void Parser::parseSpecifier()
	{
		Specifier::Type type = Specifier::TYPE_UNKNOWN;

		if(tokenList.isIdentifier())
		{
			 type = Specifier::scan(tokenList.getString());
		}
		
		instruction->matchSpecifier(type);

		if(type != Specifier::TYPE_UNKNOWN)
		{
			tokenList.advance();

			type = Specifier::scan(tokenList.getString());

			if(type == Specifier::TYPE_PTR)
			{
				tokenList.advance();
			}
		}
	}

	void Parser::parseFirstOperand()
	{
		if(!instruction)
		{
			throw INTERNAL_ERROR;
		}

		parseSpecifier();

		Operand firstOperand;

		if(tokenList.isEndOfLine())
		{
		}
		else if(tokenList.isPunctuator())
		{
			switch(tokenList.getChar())
			{
			case '[':
				firstOperand = parseMemoryReference();
				break;
			case '+':
			case '-':
			case '~':
				firstOperand = parseImmediate();
				break;
			default:
				throw Error("Unexpected punctuator '%c' after mnemonic", tokenList.getChar());
			}
		}
		else if(tokenList.isInteger() || tokenList.isReal())
		{
			firstOperand = parseImmediate();
		}
		else if(tokenList.isIdentifier())
		{
			firstOperand = parseRegister();
		}
		else if(tokenList.isLiteral())
		{
			firstOperand = parseLiteral();
		}
		else
		{
			throw Error("Invalid destination operand");
		}

		instruction->matchFirstOperand(firstOperand);
		synthesizer.encodeFirstOperand(firstOperand);
	}

	void Parser::parseSecondOperand()
	{
		if(!instruction)
		{
			throw INTERNAL_ERROR;
		}

		if(tokenList.isPunctuator(','))
		{
			tokenList.advance();
		}
		else if(!tokenList.isEndOfLine())
		{
			throw Error("Operands must be separated by comma, found '%c'", tokenList.getChar());
		}
		else
		{
			instruction->matchSecondOperand(Operand::OPERAND_VOID);
			return;
		}

		parseSpecifier();

		Operand secondOperand;

		if(tokenList.isEndOfLine())
		{
		}
		else if(tokenList.isPunctuator())
		{
			switch(tokenList.getChar())
			{
			case '[':
				secondOperand = parseMemoryReference();
				break;
			case '+':
			case '-':
			case '~':
				secondOperand = parseImmediate();
				break;
			default:
				throw Error("Unexpected punctuator after mnemonic '%c'", tokenList.getChar());
			}
		}
		else if(tokenList.isInteger())
		{
			secondOperand = parseImmediate();
		}
		else if(tokenList.isIdentifier())
		{
			secondOperand = parseRegister();
		}
		else
		{
			throw Error("Invalid source operand");
		}

		instruction->matchSecondOperand(secondOperand);
		synthesizer.encodeSecondOperand(secondOperand);
	}

	void Parser::parseThirdOperand()
	{
		if(!instruction)
		{
			throw INTERNAL_ERROR;
		}

		if(tokenList.isPunctuator(','))
		{
			tokenList.advance();
		}
		else if(!tokenList.isEndOfLine())
		{
			throw Error("Operands must be separated by comma, found '%c'", tokenList.getChar());
		}
		else
		{
			instruction->matchThirdOperand(Operand::OPERAND_VOID);
			return;
		}

		Operand thirdOperand;

		if(tokenList.isEndOfLine())
		{
		}
		else if(tokenList.isPunctuator())
		{
			switch(tokenList.getChar())
			{
			case '+':
			case '-':
			case '~':
				thirdOperand = parseImmediate();
				break;
			default:
				throw Error("Unexpected punctuator after mnemonic '%c'", tokenList.getChar());
			}
		}
		else if(tokenList.isInteger())
		{
			thirdOperand = parseImmediate();
		}
		else
		{
			throw Error("Too many operands");
		}

		instruction->matchThirdOperand(thirdOperand);
		synthesizer.encodeThirdOperand(thirdOperand);
	}

	OperandIMM Parser::parseImmediate()
	{
		OperandIMM imm;

		if(tokenList.isPunctuator())
		{
			if(tokenList.isPunctuator('+'))
			{
				tokenList.advance();

				if(tokenList.isInteger())
				{
					imm.value = +tokenList.getInteger();
				}
				else if(tokenList.isReal())
				{
					float real = +tokenList.getReal();

					imm.value = *(int*)&real;
				}
				else
				{
					throw Error("Unexpected token following '+'");
				}
			}
			else if(tokenList.isPunctuator('-'))
			{
				tokenList.advance();

				if(tokenList.isInteger())
				{
					imm.value = -tokenList.getInteger();
				}
				else if(tokenList.isReal())
				{
					float real = -tokenList.getReal();

					imm.value = *(int*)&real;
				}
				else
				{
					throw Error("Unexpected token following '-'");
				}
			}
			else if(tokenList.isPunctuator('~'))
			{
				tokenList.advance();

				if(tokenList.isInteger())
				{
					imm.value = ~tokenList.getInteger();
				}
				else
				{
					throw Error("Unexpected token following '~'");
				}
			}
			else
			{
				throw INTERNAL_ERROR;   // Method shouldn't have been called
			}
		}
		else if(tokenList.isInteger())
		{
			imm.value = tokenList.getInteger();
		}
		else if(tokenList.isReal())
		{
			float real = tokenList.getReal();

			imm.value = *(int*)&real;
		}
		else
		{
			throw INTERNAL_ERROR;   // Method shouldn't have been called
		}

		if((signed char)imm.value == imm.value)
		{
			imm.type = Operand::OPERAND_EXT8;
		}
		else if((unsigned char)imm.value == imm.value)
		{
			imm.type = Operand::OPERAND_IMM8;
		}
		else if((unsigned short)imm.value == imm.value)
		{
			imm.type = Operand::OPERAND_IMM16;
		}
		else
		{
			imm.type = Operand::OPERAND_IMM32;
		}

		tokenList.advance();

		return imm;
	}

	OperandSTR Parser::parseLiteral()
	{
		OperandSTR str(tokenList.getString());

		tokenList.advance();

		return str;
	}

	OperandREG Parser::parseRegister()
	{
		Operand reg = Operand::scanReg(tokenList.getString());

		// It's not a register, so it must be a label
		if(reg.type == Operand::OPERAND_UNKNOWN)
		{
			// Operand type should be immediate
			reg.type = Operand::OPERAND_IMM;
			reg.notation = tokenList.getString();
		}

		tokenList.advance();

		return reg;
	}

	OperandMEM Parser::parseMemoryReference()
	{
		OperandMEM mem;

		while(!tokenList.lookAhead().isEndOfLine())
		{
			const Token &prev = tokenList.current();
			tokenList.advance();
			const Token &next = tokenList.lookAhead();

			if(tokenList.isIdentifier())
			{
				// Try if it's a register
				Operand reg = Operand::scanReg(tokenList.getString());

				if(reg.type != Operand::OPERAND_UNKNOWN)
				{
					// Check if this is a scaled index register
					if(prev.isPunctuator('*') || next.isPunctuator('*'))
					{
						mem.indexReg = reg.reg;
					}
					else
					{
						mem.baseReg = reg.reg;
					}
				}
				else
				{
					// Reference to a variable
					mem.reference = tokenList.getString();
				}
			}
			else if(tokenList.isPunctuator())
			{
				switch(tokenList.getChar())
				{
				case ']':
					mem.type = Operand::OPERAND_MEM;
					tokenList.advance();
					return mem;
				case '+':
					// Check if previous and next tokens are identifier or number
					if((!prev.isInteger() && !prev.isIdentifier()) ||
						(!next.isIdentifier() && !next.isInteger() && !next.isPunctuator('-')))
					{
						throw Error("Syntax error '+' in memory reference");
					}
					break;
				case '-':
					// Check if previous and next tokens are correct type
					if((!prev.isInteger() && !prev.isIdentifier() && !prev.isPunctuator('[') && !prev.isPunctuator('+')) ||
					   !next.isInteger())
					{
						throw Error("Syntax error '-' in memory reference");
					}
					break;
				case '*':
					// Check if previous and next tokens are index and scale
					if(!(prev.isInteger() && next.isIdentifier()) &&
					   !(next.isInteger() && prev.isIdentifier()))
					{
						throw Error("Syntax error '*' in memory reference");
					}
					break;
				default:
					throw Error("Unexpected punctuator in memory reference '%c'", tokenList.getChar());
				}
			}
			else if(tokenList.isInteger())
			{
				if(prev.isPunctuator('*') || next.isPunctuator('*'))
				{
					if(tokenList.getInteger() == 1 ||
					   tokenList.getInteger() == 2 ||
					   tokenList.getInteger() == 4 ||
					   tokenList.getInteger() == 8)
					{
						mem.scale = tokenList.getInteger();
					}
					else
					{
						throw Error("Invalid scale in memory reference");
					}
				}
				else if(prev.isPunctuator('-'))
				{
					mem.displacement -= tokenList.getInteger();
				}
				else if(prev.isPunctuator('+') || next.isPunctuator('+'))
				{
					mem.displacement += tokenList.getInteger();
				}
				else   // Static address
				{
					mem.displacement += tokenList.getInteger();
				}
			}
			else
			{
				throw Error("Unexpected token in memory reference '%s'", tokenList.getString());
			}
		}

		throw Error("Unexpected end of line in memory reference");
	}
}
