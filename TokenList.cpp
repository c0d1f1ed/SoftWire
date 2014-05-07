#include "TokenList.hpp"

#include "Token.hpp"
#include "Macro.hpp"
#include "Error.hpp"

#include <stdio.h>

namespace SoftWire
{
	TokenList::TokenLink::TokenLink()
	{
		token = 0;
		next = 0;
	}

	TokenList::TokenLink::~TokenLink()
	{
		delete token;
		token = 0;
		
		// Avoid call stack overflow
		while(next)
		{
			TokenLink *nextNext = next->next;
			next->next = 0;
			delete next;
			next = nextNext;
		}
		next = 0;
	}

	TokenList::TokenList()
	{
		head = new TokenLink();
		pointer = head;
		tail = head;
	}

	TokenList::~TokenList()
	{
		delete head;
		head = 0;

		pointer = 0;
	}

	bool TokenList::isEndOfFile() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return true;
		}

		return pointer->token->isEndOfFile();
	}

	bool TokenList::isEndOfLine() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return true;
		}

		return pointer->token->isEndOfLine();
	}

	bool TokenList::isIdentifier(const char *compareString) const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return false;
		}

		return pointer->token->isIdentifier(compareString);
	}

	bool TokenList::isInteger() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return false;
		}

		return pointer->token->isInteger();
	}

	bool TokenList::isReal() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return false;
		}

		return pointer->token->isReal();
	}

	bool TokenList::isPunctuator(char c) const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return false;
		}

		return pointer->token->isPunctuator(c);
	}

	bool TokenList::isLiteral() const
	{
		if(!pointer || !pointer->token)
		{
			return false;
		}

		return pointer->token->isLiteral();
	}

	bool TokenList::isConstant() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return false;
		}

		return pointer->token->isConstant();
	}

	const char *TokenList::getString() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return "";
		}

		return pointer->token->getString();
	}

	char TokenList::getChar() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return '\0';
		}

		return pointer->token->getChar();
	}

	int TokenList::getInteger() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return 0;
		}

		return pointer->token->getInteger();
	}

	float TokenList::getReal() const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		if(!pointer->token)
		{
			return 0;
		}

		return pointer->token->getReal();
	}

	const Token &TokenList::current() const
	{
		if(!pointer || !pointer->token)
		{
			throw INTERNAL_ERROR;
		}

		return *pointer->token;
	}

	const Token &TokenList::lookAhead(int n) const
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		TokenLink *lookAhead = pointer;

		while(lookAhead->next && n--)
		{
			lookAhead = lookAhead->next;
		}

		if(!lookAhead->token)
		{
			throw INTERNAL_ERROR;
		}

		return *lookAhead->token;
	}

	const Token &TokenList::advance(int n)
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		while(pointer->next && n--)
		{
			pointer = pointer->next;
		}

		return *pointer->token;
	}

	void TokenList::append(const Token &token)
	{
		if(!tail || tail->next)
		{
			throw INTERNAL_ERROR;
		}

		tail->token = Token::copy(&token);
		tail->next = new TokenLink();
		tail = tail->next;
	}

	void TokenList::erase(int n)
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		while(n--)
		{
			TokenLink *next = pointer->next;
			

			if(next)
			{
				delete pointer->token;
				
				pointer->token = next->token;
				pointer->next = next->next;

				next->token = 0;
				next->next = 0;
				delete next;
			}
			else
			{
				if(pointer != tail)
				{
					throw INTERNAL_ERROR;
				}
			}
		}
	}

	void TokenList::overwrite(const Token &token)
	{
		if(!pointer || !pointer->next || pointer == tail)
		{
			throw INTERNAL_ERROR;
		}

		delete pointer->token;
		pointer->token = Token::copy(&token);
	}

	void TokenList::insertBefore(const Token &token)
	{
		if(!pointer)
		{
			throw INTERNAL_ERROR;
		}

		TokenLink *oldLink = pointer->next;
		Token *oldToken = pointer->token;

		pointer->token = Token::copy(&token);
		pointer->next = new TokenLink;

		pointer->next->token = oldToken;
		pointer->next->next = oldLink;
	}

	void TokenList::insertAfter(const Token &token)
	{
		if(!pointer || !pointer->next || pointer == tail)
		{
			throw INTERNAL_ERROR;
		}

		TokenLink *oldLink = pointer->next;
		pointer->next = new TokenLink;
		pointer = pointer->next;

		pointer->token = Token::copy(&token);
		pointer->next = oldLink;
	}

	void TokenList::paste(TokenList &tokenList)
	{
		while(!tokenList.isEndOfFile())
		{
			insertAfter(tokenList.current());
			tokenList.advance();
		}

		insertAfter(EndOfLine(getString()));
	}

	void TokenList::expand(Macro &macro)
	{
		TokenLink *originalPosition = pointer;

		macro.expandAll(*this);

		pointer = originalPosition;
	}

	void TokenList::rewind()
	{
		if(!head)
		{
			throw INTERNAL_ERROR;
		}

		pointer = head;
	}

	bool TokenList::isEmpty() const
	{
		if(!head)
		{
			throw INTERNAL_ERROR;
		}

		if(!head->token)
		{
			return true;
		}

		return head->token->isEndOfFile();
	}
}
