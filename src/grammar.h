  #ifndef __QPARSER_GRAMMAR_H__
#define __QPARSER_GRAMMAR_H__
//////////////////////////////////////////////////////////////////////////////
//
//    GRAMMAR.H
//
//    Copyright © 2007-2009, Rehno Lindeque. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////
/*                               DOCUMENTATION                              */
/*
    DESCRIPTION:
      QParser grammar base.

    USAGE:
      + Derive specific grammar implementations from this class. (e.g. for
        LL / LR / LARL class grammars etc.)

    IMPLEMENTATION:
      + Root tables (e.g. lexRootIndices, nilTokensRootIndices, etc) list
        indexes into their respective token parse table. An index of 255
        indicates no token.
      + Token ids are sequenced rather than hashed. Tokens and statements
        share the same id space, hence a token can't have the same id as a
        statement.
 
    OLD: This is no longer true! Have a look at the tokens.h header
        we'll have to re-factor this system...
        + Even tokens are nonterminals (production ids)
        + Odd tokens are terminal tokens (lexical ids)
          + -1 is a special token, sometimes used to represent no token or
            end-of-stream token in subclasses (specific to the grammar
            implementation)
          + 1 is an identifier token used for both declerations and
            references (during lexical analysis and parsing stage)
          + 3 is an identifier declaration (psuedo-)token used in production symbols
            during the grammar construction phase
          + 5 is an identifier reference (psuedo-)token used in production symbols
            during the grammar construction phase
          + 7 is a numeric constant token. representing integer or
            real values

    TODO:
      + Add "multi-identifier" merging extension
      + The lexer implementation should be separated from the parser!
*/

/*                              COMPILER MACROS                             */
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4311)
# pragma warning(disable:4312)
#endif

/*                                  CLASSES                                 */
namespace QParser
{
  class Grammar : public Base::Object
  {
  public:
    // Constructor
    Grammar(TokenRegistry& tokenRegistry);
    Grammar(const Grammar&) = delete;
    Grammar() = delete;

    //Destructor
    virtual ~Grammar();

    // Productions
    ParseToken BeginProduction(const_cstring productionName);
    void EndProduction();

    void ProductionToken(ParseToken token);
    ParseToken ProductionToken(const_cstring tokenName);
    ParseToken ProductionIdentifierDecl(const_cstring typeName);
    //void ProductionIdentifierRef(OSid type);
    ParseToken ProductionIdentifierRef(const_cstring typeName);

    ParseToken DeclareProduction(const_cstring productionName);

    void Precedence(const_cstring token1Name, const_cstring token2Name);
    void Precedence(ParseToken token1, ParseToken token2);

    void GrammarStartSymbol(ParseToken nonterminal);

    bool CheckForwardDeclarations() const;
    
    // Tokens
    INLINE TokenRegistry& GetTokenRegistry() { return tokenRegistry; }
    INLINE const TokenRegistry& GetTokenRegistry() const { return tokenRegistry; }
      
/*#ifdef _DEBUG
    void DebugOutputTokens() const;
    void DebugOutputProductions() const;
    //virtual void DebugOutputStates() const = 0;
    virtual void DebugOutputStates() const {};
    virtual void DebugOutputTable() const {};
    void DebugOutputParseResult(OSobject& parseResult) const;
#endif*/

  protected:
    // Production rules (which produce nonterminals)
    struct ProductionRule
    {
      ParseToken* tokens;
      uint8 tokensLength;
      FORCE_INLINE ProductionRule() : tokens(null), tokensLength(0) {}
    };

    // A production set is the set of productions producing the same nonterminal token
    struct ProductionSet
    {
      uint rulesOffset;   // Offset of the rules contained matching this production
      uint8 rulesLength;  // Number of rules matching this production
      bool nullable;      // Flag indicating whether any of the rules matching this production may be produced from an empty string
      uint8 visitedCount; // A counter used for when determining whether this set is nullable
      FORCE_INLINE ProductionSet() : 
        rulesOffset(0), 
        rulesLength(0), 
        nullable(false), 
        visitedCount(0) {}
    };
    
    // Container types
    typedef std::vector<ParseToken> ParseTokens;                                  // A list of parse tokens
    typedef std::map<ParseToken, ProductionSet*> ProductionSets;                  // A grouping of productions mapped to the nonterminals that they produce
    typedef std::vector< std::pair<ProductionRule, ParseToken> > ProductionRules; // A list of production rules together with the nonterminal that each produces
    typedef std::multimap<ParseToken, ParseToken> PrecedenceMap;                  // A map which indicates how shift-reduce errors should be resolved (by giving one of the two tokens precedence)
    typedef std::set<ParseToken> ParseTokenSet;                                   // A unique set of tokens
    
    // Members
    TokenRegistry& tokenRegistry;       // A registry of the tokens used by both the parser and the lexer
    ProductionSets productionSets;      // A mapping of all production sets from the nonterminal they produce
    ProductionRules rules;              // All production rules in the grammar
    ProductionRule* activeRule;         // A reference to the production rule that is currently being used (or constructed)
    ParseTokens activeProductionTokens; // A list of the tokens inside a production which is currently being used
    PrecedenceMap precedenceMap;        // A map which indicates how shift-reduce errors should be resolved (by giving one of the two tokens precedence)
    ParseTokenSet silentTerminals;      // Terminals which should not be output by the parser (or Lexer?? todo: resolve)
    ParseToken rootNonterminal;         // The nonterminal which should be used to identify the root of the grammar used to build the parser (this nonterminal will also be the root of the produced tree)

    // Grammar construction operations  
    // Construct a non-terminal token
    ParseToken ConstructNonterminal(const_cstring tokenName);
    
    // Replace all tokens 
    void ReplaceAllTokens(ParseToken oldToken, ParseToken newToken);

    // Test whether a production is silent
    bool IsSilent(const ProductionRule& rule) const;

    // Test whether a lexical token is silent
    bool IsSilent(ParseToken token) const;
    
    // Find a precedence directive for token1 < token2
    PrecedenceMap::const_iterator FindPrecedenceDirective(ParseToken token1, ParseToken token2) const;

    //// Accessors
    // Get the production corresponding to the given nonterminal token
    const ProductionSet* GetProductionSet(ParseToken nonterminal) const;
          ProductionSet* GetProductionSet(ParseToken nonterminal);
    
    // Get the production rule identified by its index
    const ProductionRule& GetRule(uint index) const;
          ProductionRule& GetRule(uint index);
          
    // Get a token in a production rule identified by the rule index and the token index
    ParseToken GetRuleToken(uint ruleIndex, uint tokenIndex) const;
    
    //// Miscelaneous    
/*#ifdef _DEBUG
    //void OutputStatementMatch(ParseResult& result, uint& index) const;
    void OutputStatementMatch(ParseResult& result, uint index) const;
    
    INLINE void DebugOutputProduction(ParseToken token, const Production& production) const;
    INLINE void DebugOutputProduction(const Production& production) const;
    INLINE void DebugOutputSymbol(ParseToken token) const;
#endif*/
  };
}

/*                                   INCLUDES                               */
#include "grammar.inl"

#endif
