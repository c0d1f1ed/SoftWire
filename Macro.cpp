#include "Macro.hpp"

#include "String.hpp"
#include "Token.hpp"
#include "Error.hpp"

namespace SoftWire
{
	Macro::Macro(TokenList &unit)
	{
		scanName(unit);
		scanArguments(unit);
		scanDefinition(unit);
	}

	Macro::~Macro()
	{
		delete[] name;
	}

	void Macro::scanName(TokenList &unit)
	{
		this->name = strdup(unit.getString());
		unit.erase();
	}

	void Macro::scanArguments(TokenList &unit)
	{
		int parenthesis = 0;
		numArguments = 0;

		if(unit.isPunctuator('('))
		{
			unit.erase();

			parenthesis++;
			numArguments++;

			while(true)
			{
				if(unit.isEndOfLine())
				{
					throw Error("Missing ')' before newline");
				}			
				else
				{
					if(unit.isPunctuator('('))
					{
						parenthesis++;
					}
					else if(unit.isPunctuator(')'))
					{
						parenthesis--;

						if(parenthesis == 0)
						{
							unit.erase();
							break;
						}
					}
					else if(unit.isPunctuator(','))
					{
						if(parenthesis == 1)
						{
							numArguments++;
						}
					}
					else if(!unit.isIdentifier())
					{
						throw Error("Unexpected token in macro argument definition");
					}

					arguments.append(unit.current());

					unit.erase();
				}
			}
		}	
	}

	void Macro::scanDefinition(TokenList &unit)
	{
		int braces = 0;

		while(true)
		{
			if(unit.isPunctuator('{'))
			{
				unit.erase();
				braces++;
			}
			else if(unit.isPunctuator('}'))
			{
				unit.erase();
				braces--;
			}
			else if(unit.isPunctuator('\\'))
			{
				unit.erase();

				if(!unit.isEndOfLine())
				{
					throw Error("Expected end-of-line in multi-line macro");
				}

				definition.append(unit.current());
				unit.erase();
			}
			else if(unit.isEndOfFile())
			{
				break;
			}
			else if(unit.isEndOfLine() && braces <= 0)
			{
				if(unit.lookAhead().isEndOfLine() || unit.lookAhead().isPunctuator('{'))
				{
					unit.advance();
				}
				else
				{
					break;
				}
			}
			else
			{
				definition.append(unit.current());

				unit.erase();
			}
		}

		if(braces != 0)
		{
			throw Error("Braces mismatch in inline function");
		}
	}

	void Macro::expandAll(TokenList &unit)
	{
		while(!unit.isEndOfFile())
		{
			if(unit.isIdentifier(name))
			{
				expand(unit);
			}
			
			unit.advance();
		}
	}

	void Macro::expand(TokenList &unit)
	{
		if(hasDefinition())
		{
			unit.erase();
		}
		else
		{
			unit.overwrite(Integer(1));
		}

		TokenList expand;   // Arguments to be expanded

		if(hasArguments())
		{
			int numExpand = 1;
			bool implicit = false;   // Argument list without parenthesis
			int parenthesis = 0;

			if(unit.isPunctuator('('))
			{
				unit.erase();

				implicit = false;
				parenthesis++;
			}
			else if(!unit.isEndOfLine())
			{
				implicit = true;
			}
			else
			{
				throw Error("Expected macro argument list");
			}

			while(!unit.isEndOfLine())
			{
				if(unit.isPunctuator('('))
				{
					parenthesis++;
				}
				else if(unit.isPunctuator(')'))
				{
					parenthesis--;

					if(!implicit && parenthesis == 0)
					{
						unit.erase();
						break;
					}
				}
				else if(unit.isPunctuator(','))
				{
					if(!implicit)
					{
						if(parenthesis == 1)
						{
							numExpand++;
						}
					}
					else
					{
						if(parenthesis == 0)
						{
							numExpand++;
						}	
					}
				}

				expand.append(unit.current());

				unit.erase();
			}

			if(numExpand > numArguments)
			{
				throw Error("Too many arguments in macro '%s'", name);
			}
			
			if(numExpand < numArguments)
			{
				throw Error("Too little arguments in macro '%s'", name);
			}
			
			if(parenthesis != 0)
			{
				throw Error("Parenthesis mismatch");
			}
		}

		definition.rewind();

		while(!definition.isEndOfFile())
		{
			if(hasArguments())
			{
				expand.rewind();
							
				for(arguments.rewind(); !arguments.isEndOfLine(); arguments.advance(2))
				{
					if(definition.isIdentifier(arguments.getString()))
					{
						while(!expand.isEndOfLine() && !expand.isPunctuator(','))
						{
							unit.insertBefore(expand.current());
							unit.advance();

							expand.advance();
						}

						break;
					}

					if(!arguments.isPunctuator(','))
					{
						while(!expand.isEndOfLine() && !expand.isPunctuator(','))
						{
							expand.advance();
						}
									
						if(!expand.isEndOfLine() && expand.isPunctuator(','))
						{
							expand.advance();
						}
					}
				}
			}

			if(arguments.isEndOfFile())
			{
				unit.insertBefore(definition.current());
				unit.advance();
			}

			definition.advance();
		}
	}

	bool Macro::hasArguments() const
	{
		return !arguments.isEmpty();
	}

	bool Macro::hasDefinition() const
	{
		return !definition.isEmpty();
	}
}
