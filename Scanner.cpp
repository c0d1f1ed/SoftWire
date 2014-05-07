#include "Scanner.hpp"

#include "Token.hpp"
#include "CharType.hpp"
#include "File.hpp"
#include "String.hpp"
#include "Macro.hpp"
#include "Error.hpp"

#include <stdlib.h>

namespace SoftWire
{
	Scanner::SymbolTable *Scanner::symbols;

	Scanner::Scanner()
	{
		source = 0;

		defineSymbol(1, "true");
		defineSymbol(0, "false");
	}

	Scanner::Scanner(const char *fileName, bool doPreprocessing)
	{
		source = 0;

		defineSymbol(1, "true");
		defineSymbol(0, "false");

		scanFile(fileName, doPreprocessing);
	}

	Scanner::~Scanner()
	{
		delete[] source;
		source = 0;

		clearSymbols();
	}

	void Scanner::scanFile(const char *fileName, bool doPreprocessing)
	{
		if(!fileName) throw INTERNAL_ERROR;

		FILE *file = fopen(fileName, "rb");
		if(!file) throw Error("Could not open source file: '%s'\n", fileName);

		const int length = _filelength(fileno(file));
		source = new char[length + 1];
		fread(source, sizeof(char), length, file);
		fclose(file);
		source[length] = '\0';

		scanBuffer(source, doPreprocessing);
	}

	void Scanner::scanString(const char *sourceString, bool doPreprocessing)
	{
		source = strdup(sourceString);
		scanBuffer(source, doPreprocessing);
	}

	void Scanner::scanBuffer(char *source, bool doPreprocessing)
	{
		const char *lineStart = source;

		int i = 0;

		while(true)
		{
			char buffer[tokenMax];
			int c = 0;

			switch(source[i])
			{
			case '\0':
				append(EndOfFile(lineStart));

				if(doPreprocessing)
				{
					preprocess();
				}

		  		return;
			case '\n':
			case '\r':
				do
				{
					source[i++] = '\0';
				}
				while(source[i] == '\n' || source[i] == '\r');
				append(EndOfLine(lineStart));
				lineStart = &source[i];
				break;
			case ';':
				do
				{ 
					i++;
				}
				while(source[i] != '\n' && source[i] != '\r' && source[i] != '\0');

				break;
			case '/':
				i++;
				if(source[i] == '/')
				{
					do
					{
						i++;
					}
					while(source[i] != '\n' && source[i] != '\r' && source[i] != '\0');

					break;
				}
				else if(source[i] == '*')
				{
					do
					{
						i++;

						if(source[i] == '\n' || source[i] == '\r')
						{
							lineStart = &source[i + 1];
						}
					}
					while(source[i] != '\0' && !(source[i] == '*' && source[i + 1] == '/'));

					if(source[i] != '\0')
					{
						i += 2;
					}
					else
					{
						throw Error("Unexpected end of file: missing comment delimiter");
					}
					break;
				}
				else
				{
					append(Punctuator('/'));
				}
				break;
			case ' ':
			case '	':
				i++;
				continue;
			case '\'':
				if(isprint(source[++i]))
				{
					do
					{
						if(c >= tokenMax)
						{
							throw Error("Token too long");
						}

						if(source[i] == '\\')
						{
							switch(source[(++i)++])
							{
							case '\'': buffer[c++] = '\''; break;
							case '\"': buffer[c++] = '\"'; break;
							case '?': buffer[c++] = '\?'; break;
							case '\\': buffer[c++] = '\\'; break;
							case 'a': buffer[c++] = '\a'; break;
							case 'b': buffer[c++] = '\b'; break;
							case 'f': buffer[c++] = '\f'; break;
							case 'n': buffer[c++] = '\n'; break;
							case 'r': buffer[c++] = '\r'; break;
							case 't': buffer[c++] = '\t'; break;
							case 'v': buffer[c++] = '\v'; break;
							default:
								throw Error("Unexpected character following escape sequence");
							}
						}
						else
						{
							buffer[c++] = source[i++];
						}
					}
					while(!(source[i] == '\'') && isprint(source[i]) && c <= 4);

					if(source[i++] != '\'')
					{
						throw Error("Expected closing double quotation mark");
					}

					buffer[c] = 0;
					int value = 0;

					for(int i = 0; i < c; i++)
					{
						value |= (int)buffer[c - i - 1] << (8 * i);
					}

					append(Integer(value));
				}
				break;
			case '\"':
				if(isprint(source[++i]))
				{
					do
					{
						if(c >= tokenMax)
						{
							throw Error("Token too long");
						}

						if(source[i] == '\\')
						{
							switch(source[(++i)++])
							{
							case '\'': buffer[c++] = '\''; break;
							case '\"': buffer[c++] = '\"'; break;
							case '?': buffer[c++] = '\?'; break;
							case '\\': buffer[c++] = '\\'; break;
							case 'a': buffer[c++] = '\a'; break;
							case 'b': buffer[c++] = '\b'; break;
							case 'f': buffer[c++] = '\f'; break;
							case 'n': buffer[c++] = '\n'; break;
							case 'r': buffer[c++] = '\r'; break;
							case 't': buffer[c++] = '\t'; break;
							case 'v': buffer[c++] = '\v'; break;
							case 'x':
								{
									char *endptr;
									unsigned int x = strtol(&source[i], &endptr, 16);
									i = endptr - source;

									if(x > 255)
									{
										throw Error("Expected two hex digits following \\x in string literal");
									}

									buffer[c++] = x;
								}
								break;
							case '0':
							case '1':
							case '2':
							case '3':
							case '4':
							case '5':
							case '6':
							case '7':
								{
									char *endptr;
									unsigned int x = strtol(&source[i - 1], &endptr, 8);
									i = endptr - source;

									if(x > 255)
									{
										throw Error("Expected three octal digits following \\ in string literal");
									}

									buffer[c++] = x;
								}
								break;
							default:
								throw Error("Unexpected character following escape sequence");
							}
						}
						else
						{
							buffer[c++] = source[i++];
						}
					}
					while(!(source[i] == '\"') && isprint(source[i]));

					if(source[i++] != '\"')
					{
						throw Error("Expected closing double quotation mark");
					}
					
					buffer[c] = 0;
					append(Literal(buffer));
				}
				break;
			case '*':
			case '+':
			case '-':
			case '~':
			case ',':
			case '[':
			case ']':
			case ':':
			case '#':
			case '!':
			case '|':
			case '&':
			case '=':
			case '<':
			case '>':
			case '(':
			case ')':
			case '{':
			case '}':
			case '\\':
				append(Punctuator(source[i++]));
				break;
			default:
				if(iscsymf(source[i]))
				{
					do
					{
						if(c >= tokenMax)
						{
							throw Error("Token too long");
						}

						buffer[c++] = source[i++];
					}
					while(iscsym(source[i]));

					buffer[c] = 0;

					append(Identifier(buffer));
				}
				else if(isdigit(source[i]) || source[i] == '.')
				{
					do
					{
						if(c >= tokenMax)
						{
							throw Error("Token too long");
						}

						buffer[c++] = source[i++];
					}
					while(isxdigit(source[i]) || source[i] == 'x' || source[i] == '.');

					buffer[c] = 0;
					char *endptr;

					if(source[i] == 'h')   // Hexadecimal
					{
						buffer[c++] = source[i++];

						append(Integer(strtoul(buffer, &endptr, 16)));
					}
					else if(source[i] == 'q')   // Octal
					{
						buffer[c++] = source[i++];

						append(Integer(strtoul(buffer, &endptr, 8)));
					}
					else if(source[i] == 'b')   // Binary
					{
						buffer[c++] = source[i++];

						append(Integer(strtoul(buffer, &endptr, 2)));
					}
					else if(!isalpha(source[i]) || source[i] == '.')   // Decimal
					{
						int integer = strtoul(buffer, &endptr, 0);

						if(*endptr != '.')
						{
							append(Integer(integer));
						}
						else
						{
							float real = (float)strtod(buffer, &endptr);

							append(Real(real));
						}
					}
					else
					{
						throw Error("Unexpected character in number");
					}
				}
				else
				{
					throw Error("Invalid token '%c'", source[i]);
				}
			}
		}
	}

	void Scanner::preprocess()
	{
		includeFiles();
		substituteSymbols();
		expandMacros();
		conditionalCompilation();
	}

	void Scanner::includeFiles()
	{
		rewind();

		while(!isEndOfFile())
		{
			if(isPunctuator('#') &&
			   lookAhead().isIdentifier("include"))
			{
				erase(2);

				if(isLiteral())
				{
					char fileName[64] = {0};
					strcpy(fileName, getString());
					erase();

					if(!isEndOfLine())
					{
						throw Error("Unexpected tokens following include directive");
					}

					includeFile(fileName);
				}
				else
				{
					throw Error("Syntax error in include directive");
				}
			}

			advance();
		}
	}

	void Scanner::substituteSymbols()
	{
		rewind();

		while(!isEndOfFile())
		{
			if(isIdentifier())
			{
				const SymbolTable *symbol = symbols;

				while(symbol)
				{
					if(symbol->name && strcmp(symbol->name, getString()) == 0)
					{
						overwrite(Integer(symbol->value));
					}

					symbol = symbol->next();
				}
			}

			advance();
		}
	}

	void Scanner::expandMacros()
	{
		for(rewind(); !isEndOfFile(); advance())
		{
			if(isPunctuator('#') &&	
			   lookAhead().isIdentifier("define"))
			{
				erase(2);
			}
			else if(isIdentifier("inline"))
			{
				erase();
			}
			else
			{
				continue;
			}

			if(!isIdentifier())
			{
				throw Error("Expected identifier following define");
			}

			Macro macro(*this);

			expand(macro);
		}
	}

	void Scanner::conditionalCompilation()
	{
		do
		{
			evaluateUnary();
			evaluateMultiplicative();
			evaluateAdditive();
			evaluateRelational();
			evaluateEquality();
			evaluateBitwiseAND();
			evaluateBitwiseOR();
			evaluateLogicalAND();
			evaluateLogicalOR();
		}
		while(cleanupParenthesis());
		
		processDirectives();
	}

	int Scanner::conditionTrue()
	{
		// Just ignore these unary operators, they don't influence the outcome
		if(isPunctuator('+'))
		{
			erase();
		}
		else if(isPunctuator('-'))
		{
			erase();
		}

		if(!isInteger())
		{
			throw INTERNAL_ERROR;
		}

		return getInteger();
	}

	void Scanner::defineSymbol(int value, const char *name)
	{
		if(!symbols)
		{
			symbols = new SymbolTable();
		}

		SymbolTable *symbol = symbols;

		do
		{
			if(symbol->name && strcmp(symbol->name, name) == 0)
			{
				symbol->value = value;
				return;
			}

			symbol = symbol->next();
		}
		while(symbol);

		symbols->append(Symbol(value, name));
	}

	void Scanner::clearSymbols()
	{
		delete symbols;
		symbols = 0;
	}

	void Scanner::evaluateUnary()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(!isConstant() && !isIdentifier())
			{
				if(lookAhead().isPunctuator('+'))
				{
					advance();

					if(lookAhead().isConstant())
					{
						erase();
					}
				}
				else if(lookAhead().isPunctuator('-'))
				{
					advance();

					if(lookAhead().isInteger())
					{
						overwrite(Integer(-lookAhead().getInteger()));
						advance();
						erase();
					}
					else if(lookAhead().isReal())
					{
						overwrite(Real(-lookAhead().getReal()));
						advance();
						erase();
					}
				}
				else
				{
					advance();
				}
			}
			else if(isPunctuator('~'))
			{
				if(lookAhead().isInteger())
				{
					overwrite(Integer(~lookAhead().getInteger()));
					advance();
					erase();
				}
				else
				{
					advance();
				}
			}
			else if(isPunctuator('!'))
			{
				if(lookAhead().isInteger())
				{
					overwrite(Integer(!lookAhead().getInteger()));
					advance();
					erase();
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateMultiplicative()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('*'))
			{
				if(isReal() &&
				   lookAhead(2).isReal())
				{
					float value = getReal() * lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if((isReal() &&
						lookAhead(2).isInteger()))
				{
					float value = getReal() * lookAhead(2).getInteger();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isReal())
				{
					float value = getInteger() * lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isInteger())
				{
					int value = getInteger() * lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else if(lookAhead().isPunctuator('/'))
			{
				if(isReal() &&
				   lookAhead(2).isReal())
				{
					float value = getReal() / lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if((isReal() &&
						lookAhead(2).isInteger()))
				{
					float value = getReal() / lookAhead(2).getInteger();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isReal())
				{
					float value = getInteger() / lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isInteger())
				{
					int value = getInteger() / lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else if(lookAhead().isPunctuator('%'))
			{
				if(isInteger() &&
				   lookAhead(2).isInteger())
				{
					int value = getInteger() % lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateAdditive()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('+'))
			{
				if(isReal() &&
				   lookAhead(2).isReal())
				{
					float value = getReal() + lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if((isReal() &&
						lookAhead(2).isInteger()))
				{
					float value = getReal() + lookAhead(2).getInteger();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isReal())
				{
					float value = getInteger() + lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isInteger())
				{
					int value = getInteger() + lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else if(lookAhead().isPunctuator('-'))
			{
				if(isReal() &&
				   lookAhead(2).isReal())
				{
					float value = getReal() - lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if((isReal() &&
						lookAhead(2).isInteger()))
				{
					float value = getReal() - lookAhead(2).getInteger();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isReal())
				{
					float value = getInteger() - lookAhead(2).getReal();
					erase(2);
					overwrite(Real(value));
				}
				else if(isInteger() &&
						lookAhead(2).isInteger())
				{
					int value = getInteger() - lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateRelational()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('<'))
			{
				if(isInteger() &&
				   lookAhead(2).isInteger())
				{
					int value = getInteger() < lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else if(isInteger() &&
				        lookAhead(2).isPunctuator('=') &&
						lookAhead(3).isInteger())
				{
					int value = getInteger() <= lookAhead(3).getInteger();
					erase(3);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else if(lookAhead().isPunctuator('>'))
			{
				if(isInteger() &&
				   lookAhead(2).isInteger())
				{
					int value = getInteger() > lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else if(isInteger() &&
				        lookAhead(2).isPunctuator('=') &&
						lookAhead(3).isInteger())
				{
					int value = getInteger() >= lookAhead(3).getInteger();
					erase(3);
					overwrite(Integer(value));
				}	
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateEquality()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('='))
			{
				if(isInteger() &&
				   lookAhead(2).isPunctuator('=') &&
				   lookAhead(3).isInteger())
				{
					int value = getInteger() == lookAhead(3).getInteger();
					erase(3);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else if(lookAhead().isPunctuator('!'))
			{
				if(isInteger() &&
				   lookAhead(2).isPunctuator('=') &&
				   lookAhead(3).isInteger())
				{
					int value = getInteger() != lookAhead(3).getInteger();
					erase(3);
					overwrite(Integer(value));
				}	
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateBitwiseAND()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('&'))
			{
				if(isInteger() &&
				   lookAhead(2).isInteger())
				{
					int value = getInteger() & lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateBitwiseOR()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('|'))
			{
				if(isInteger() &&
				   lookAhead(2).isInteger())
				{
					int value = getInteger() | lookAhead(2).getInteger();
					erase(2);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateLogicalAND()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('&'))
			{
				if(isInteger() &&
				   lookAhead(2).isPunctuator('&') &&
				   lookAhead(3).isInteger())
				{
					int value = getInteger() && lookAhead(3).getInteger();
					erase(3);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::evaluateLogicalOR()
	{
		rewind();

		while(!isEndOfFile())
		{
			// Don't do any evaluation on memory references
			if(isPunctuator('['))
			{
				do
				{
					advance();
				}
				while(!isPunctuator(']'));
			}

			if(lookAhead().isPunctuator('|'))
			{
				if(isInteger() &&
				   lookAhead(2).isPunctuator('|') &&
				   lookAhead(3).isInteger())
				{
					int value = getInteger() || lookAhead(3).getInteger();
					erase(3);
					overwrite(Integer(value));
				}
				else
				{
					advance();
				}
			}
			else
			{
				advance();
			}
		}
	}

	void Scanner::processDirectives()
	{
		rewind();

		int indent = 0;
		int active = 0;

		while(!isEndOfFile())
		{
			if(isPunctuator('#'))
			{
				erase();
					
				if(isIdentifier())
				{
					if(isIdentifier("if"))
					{
						erase();

						indent++;

						if(active == indent - 1)
						{
							if(conditionTrue())
							{
								erase();
								active += 2;
							}
							else
							{
								active++;
							}
						}

						indent++;
					}
					else if(isIdentifier("elif"))
					{
						erase();

						if(indent <= 0 || indent & 1)
						{
							throw Error("#elif without matching #if");
						}

						indent--;

						if(active == indent + 1)
						{
							active -= 2;
						}
						else if(active == indent)
						{
							if(conditionTrue())
							{
								erase();
								active++;
							}
						}

						indent++;
					}
					else if(isIdentifier("else") && 
							lookAhead().isIdentifier("if"))
					{
						erase(2);

						if(indent <= 0 || indent & 1)
						{
							throw Error("#else if without matching #if");
						}

						indent--;

						if(active == indent + 1)
						{
							active -= 2;
						}
						else if(active == indent)
						{
							if(conditionTrue())
							{
								erase();
								active++;
							}
						}

						indent++;
					}
					else if(isIdentifier("else"))
					{
						erase();

						if(indent <= 0 || indent & 1)
						{
							throw Error("#else without matching #if");
						}

						indent--;

						if(active == indent)
						{
							active++;
						}
						else if(active == indent + 1)
						{
							active -= 2;
						}

						indent++;
					}
					else if(isIdentifier("endif"))
					{
						erase();

						if(indent <= 0 || indent & 1)
						{
							throw Error("#endif without matching #if");
						}

						indent--;

						if(active == indent)
						{
							active--;
						}
						else if(active == indent + 1)
						{
							active -= 2;
						}

						indent--;
					}
					else
					{
						throw Error("Invalid preprocessor directive '%s'", getString());
					}
				}
				else
				{
					throw Error("Unexpected token following '#'");
				}

				if(indent == active && !isEndOfLine())
				{
					throw INTERNAL_ERROR;
				}

				while(!isEndOfLine())
				{
					erase();
				}
			}
			else if(indent != active)
			{
				while(!isEndOfLine())
				{
					erase();
				}
			}

			while(!isEndOfLine())
			{
				advance();
			}

			if(!isEndOfFile())
			{
				advance();
			}
		}

		if(!(indent == 0 && active == 0))
		{
			throw Error("Unexpected end of file: missing #endif");
		}
	}

	bool Scanner::cleanupParenthesis()
	{
		rewind();

		bool notDone = false;

		while(!isEndOfFile())
		{
			if(isPunctuator('(') && lookAhead().isPunctuator(')'))
			{
				throw Error("Empty parenthesis");
			}

			if(isPunctuator('(') && lookAhead().isConstant() && lookAhead(2).isPunctuator(')'))
			{
				erase();
				advance();
				erase();

				notDone = true;
			}
			else
			{
				advance();
			}
		}

		return notDone;
	}

	void Scanner::includeFile(const char *fileName)
	{
		Scanner file(fileName, false);

		paste(file);
	}
}
