#ifndef SoftWire_TokenList_hpp
#define SoftWire_TokenList_hpp

namespace SoftWire
{
	class Macro;
	class Token;

	class TokenList
	{
	public:
		TokenList();

		~TokenList();
		
		// Properties of current token (pointer)
		bool isEndOfFile() const;
		bool isEndOfLine() const;
		bool isIdentifier(const char *compareString = 0) const;
		bool isInteger() const;
		bool isReal() const;
		bool isPunctuator(char c = 0) const;
		bool isLiteral() const;
		bool isConstant() const;

		const char *getString() const;
		char getChar() const;
		int getInteger() const;
		float getReal() const;

		const Token &current() const;
		const Token &lookAhead(int n = 1) const;
		const Token &advance(int n = 1);
		
		void append(const Token &token);
		void erase(int n = 1);
		void overwrite(const Token &token);
		void insertBefore(const Token &token);
		void insertAfter(const Token &token);

		void paste(TokenList &tokenList);
		void expand(Macro &macro);

		void rewind();
		bool isEmpty() const;

	private:
		class TokenLink
		{
		public:
			TokenLink();

			~TokenLink();

			Token *token;
			TokenLink *next;
		};

		TokenLink *head;
		TokenLink *tail;
		TokenLink *pointer;
	};
}

#endif   // SoftWire_TokenList_hpp
