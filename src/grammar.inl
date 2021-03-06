#ifdef  __QPARSER_GRAMMAR_H__
#ifndef __QPARSER_GRAMMAR_INL__
#define __QPARSER_GRAMMAR_INL__
//////////////////////////////////////////////////////////////////////////////
//
//    GRAMMAR.INL
//
//    Copyright © 2007-2009, Rehno Lindeque. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

namespace QParser
{
  Grammar::Grammar(TokenRegistry& tokenRegistry) :
    tokenRegistry(tokenRegistry),
    activeRule(null),
    rootNonterminal(-1)
  {
  }

  Grammar::~Grammar()
  {
  }

  /*void Grammar::ConstructTokens()
  {
    uint&             nTokens                      = Grammar::nTokens[activeTokenType + activeSubTokenType];
    LexMatch*&        activeTokens                 = tokens[activeTokenType + activeSubTokenType];
    TokenRootIndex(&  activeTokenRootIndices)[256] = tokenRootIndices[activeTokenType + activeSubTokenType];

    //// Consolidate tokens (Copy tokens to a fixed array & construct a root character index)
    // Allocate token array
    nTokens = (uint)constructionTokens.size();
    activeTokens = new LexMatch[nTokens];

    // Clear token root indices
    memset(activeTokenRootIndices, 0, sizeof(TokenRootIndex) * nTokens);

    // Copy tokens to token array (ordered by their parse values)
    {
      uint                           cToken;
      TokenConstructionSet::iterator iToken;

      for(cToken = 0, iToken = constructionTokens.begin(); cToken < nTokens; ++cToken, ++iToken)
      {
        // Add to token "root character" indexing table (optimization index)
        {
          char rootCharacter = tokenCharacters[(*iToken).valueOffset];
          if(rootCharacter == SPECIAL_MULTILINE_BOUNDING_CHAR
              || rootCharacter == SPECIAL_SINGLELINE_BOUNDING_CHAR)
            rootCharacter = tokenCharacters[(*iToken).valueOffset + 1];

          // Copy construction token into active tokens array
          // todo: this is rather ugly and inefficient... we should rewrite this using some indexing library
          TokenRootIndex& tokenRootIndex = activeTokenRootIndices[uint(rootCharacter)];
          if(tokenRootIndex.length == 0)
          {
            tokenRootIndex.offset = cToken;
            activeTokens[tokenRootIndex.offset] = *iToken;
          }
          else
          {
            // (move all tokens up by one)
            memmove(&activeTokens[tokenRootIndex.offset + tokenRootIndex.length + 1], &activeTokens[tokenRootIndex.offset + tokenRootIndex.length], sizeof(LexMatch[cToken - (tokenRootIndex.offset + tokenRootIndex.length)]));
            for(uint c = 0; c <= MAX_UINT8; ++c)
              if(activeTokenRootIndices[c].offset > tokenRootIndex.offset)
                ++activeTokenRootIndices[c].offset;

            // (inert token into this position)
            activeTokens[tokenRootIndex.offset + tokenRootIndex.length] = *iToken;
          }

          ++tokenRootIndex.length;
        }
      }
    }

    // Clear construction tokens
    constructionTokens.clear();
  }*/

  // Rules
  ParseToken Grammar::BeginProduction(const_cstring productionName)
  {
    // Construct a nonterminal token for this production (if none exists)
    ParseToken token = ConstructNonterminal(productionName);

    // Get a production set
    ProductionSet *productionSet = null;
    {
      std::map<ParseToken, ProductionSet*>::iterator i = productionSets.find(token);
      if(i == productionSets.end() || i->first != token)
      {
        productionSet = new ProductionSet();
        productionSets[token] = productionSet;
        productionSet->rulesOffset = rules.size();
        productionSet->rulesLength = 1;
      }
      else
      {
        productionSet = i->second;
        ++productionSet->rulesLength;
      }
      OSI_ASSERT(productionSet != null);
    }

    // Add a rule to a production
    {
      rules.push_back(std::make_pair(ProductionRule(), token));
      activeRule = &rules[rules.size()-1].first;
    }

    return token;
  }

  void Grammar::EndProduction()
  {
    // Add symbols to production rule
    if (activeProductionTokens.size() > 0)
    {
      activeRule->tokens = new ParseToken[activeProductionTokens.size()];
      activeRule->tokensLength = activeProductionTokens.size();
      memcpy(activeRule->tokens, &activeProductionTokens[0], activeProductionTokens.size() * sizeof(ParseToken));
      activeProductionTokens.clear();
    }
    activeRule = null;
  }

  void Grammar::ProductionToken(ParseToken token)
  {
    OSI_ASSERT(tokenRegistry.IsTemporaryToken(token) || tokenRegistry.IsTokenValid(token));
    activeProductionTokens.push_back(token);
  }

  ParseToken Grammar::ProductionToken(const_cstring tokenName)
  {
    ParseToken token = tokenRegistry.GetToken(tokenName);
    if(token == ParseToken(-1))
      token = DeclareProduction(tokenName);
    ProductionToken(token);
    return token;
  }

  ParseToken Grammar::ProductionIdentifierDecl(const_cstring typeName)
  {
    /*hash_set<const char*>::iterator i = identifierTypes.find(typeName);
    ParseToken typeHash = identifierTypes.hash_funct()(typeName);
    activeProductionSymbols.push_back(ProductionRule::Symbol(ID_IDENTIFIER_DECL, typeHash));
    return typeHash;*/
    
    // todo: BUSY HERE... (REWRITE)
    activeProductionTokens.push_back(TOKEN_TERMINAL_IDENTIFIER);
    return TOKEN_TERMINAL_IDENTIFIER;
  }

  /*void Grammar::ProductionIdentifierRef(ParseToken type)
  {
    activeProductionSymbols.push_back(ProductionRule::Symbol(ID_IDENTIFIER_REF, type));
    
    // todo: BUSY HERE... (REWRITE)
    activeProductionSymbols.push_back(TOKEN_TERMINAL_IDENTIFIER);
  }*/

  ParseToken Grammar::ProductionIdentifierRef(const_cstring typeName)
  {
    /*hash_set<const char*>::iterator i = identifierTypes.find(typeName);
    OSid typeHash = identifierTypes.hash_funct()(typeName);
    activeProductionSymbols.push_back(ProductionRule::Symbol(ID_IDENTIFIER_REF, typeHash));
    return typeHash;*/
    
    // todo: BUSY HERE... (REWRITE)
    activeProductionTokens.push_back(TOKEN_TERMINAL_IDENTIFIER);
    return TOKEN_TERMINAL_IDENTIFIER;
  }

  ParseToken Grammar::DeclareProduction(const_cstring productionName)
  {
    return tokenRegistry.FindOrGenerateTemporaryNonterminal(productionName);
  }

  void Grammar::Precedence(const_cstring token1Name, const_cstring token2Name)
  {
    ParseToken token1 = tokenRegistry.GetToken(token1Name);
    ParseToken token2 = tokenRegistry.GetToken(token2Name);
    if(token1 != ParseToken(-1) && token2 != ParseToken(-1))
      Precedence(token1, token2);
    //else error: incorrect ids
  }

  void Grammar::Precedence(ParseToken token1, ParseToken token2)
  {
    precedenceMap.insert(std::make_pair(token1, token2));
  }

  void Grammar::GrammarStartSymbol(ParseToken nonterminal)
  {
    OSI_ASSERT(!TokenRegistry::IsTerminal(nonterminal));
    rootNonterminal = nonterminal;
  }

  bool Grammar::CheckForwardDeclarations() const
  {
    bool success = true;

    // Check for the existence of all forward declarations
    for(ParseToken nonterminal = TOKEN_NONTERMINAL_FIRST; nonterminal < tokenRegistry.GetNextAvailableNonterminal(); ++nonterminal)
    {
      if(!GetProductionSet(nonterminal))
      {
        std::cout << "Error: Token "<< nonterminal << "\'" << tokenRegistry.GetTokenName(nonterminal) << "\' was not declared." << std::endl;
        success = false;
      }
    }

    return success;
  }

  /*ParseToken Grammar::ConstructTerminal(const_cstring tokenName, uint bufferLength, uint valueLength)
  {
    / *ParseToken token = -1;
    
    // Try to find an existing terminal token with this name
    TokenIds::const_iterator i = terminalIds.find(tokenName);
    if(i == terminalIds.end())
    {
      // Create a token id for this terminal
      token = getNextAvailableTerminal();
      terminalNames[id] = tokenName;
      terminalIds[tokenName] = id;

      // Add a temporary token to the temporary construction set    
      constructionTokens.push_back(LexMatch(id, bufferLength, valueLength));
    }
    else
    {
      // Get existing terminal id
      id = i->second;
    }

    return id;* /
    
    // todo: BUSY HERE... (REWRITE)
    
    return token;
  }*/
  
  ParseToken Grammar::ConstructNonterminal(const_cstring tokenName)
  {
    /*ParseToken token = -1;
    // Try to find an existing nonterminal token with this name
    TokenIds::const_iterator i = nonterminalIds.find(tokenName);
    if(i == nonterminalIds.end())
    {
      // Create a new token id for this nonterminal
      token = getNextAvailableNonterminal();
      nonterminalNames[token] = tokenName;
      nonterminalValues[tokenName] = token;
    }
    else
    {
      // Get the existing production id
      token = i->second;
      
      // If the id was forward declared we need to substitute this id (and all uses there of) with the new correct id
      if(token > nextNonterminal)
      {
        ParseToken newToken = getNextAvailableNonterminal();
        nonterminalNames[newToken] = tokenName;
        nonterminalIds[tokenName] = newToken;
        replaceAllTokens(token, newToken);
        return newToken;
      }
    }
    
    return token;*/
    
    // TODO: BUSY HERE (rewriting this)
    
    ParseToken token = tokenRegistry.GetToken(tokenName);
    
    // If the token is a temporary token, we need to substitute it with a new
    // token and replace all instances where the token was used with the proper
    // value
    if(tokenRegistry.IsTemporaryToken(token))
    {
      ParseToken newToken = tokenRegistry.ResolveTemporaryToken(tokenName);
      ReplaceAllTokens(token, newToken);
      return newToken;
    }
    // If the token does not exist yet, we generate one
    else if(token == ParseToken(-1))
    {
      token = tokenRegistry.GenerateNonterminal(tokenName);
    }    
    return token;    
  }
  
  INLINE void Grammar::ReplaceAllTokens(ParseToken oldToken, ParseToken newToken)
  {
    for(std::vector< std::pair<ProductionRule, ParseToken> >::iterator i = rules.begin(); i != rules.end(); ++i)
    {
      ProductionRule& rule = i->first;
      
      for(uint c = 0; c < rule.tokensLength; ++c)
      {
        if(rule.tokens[c] == oldToken)
          rule.tokens[c] = newToken;
      }
    }
  }

  INLINE bool Grammar::IsSilent(const ProductionRule& rule) const
  {
    return rule.tokensLength == 1 && !TokenRegistry::IsTerminal(rule.tokens[0]);
  }

  INLINE bool Grammar::IsSilent(ParseToken token) const
  {
    return TokenRegistry::IsTerminal(token) && silentTerminals.find(token) != silentTerminals.end();
  }

  INLINE std::multimap<ParseToken, ParseToken>::const_iterator Grammar::FindPrecedenceDirective(ParseToken token1, ParseToken token2) const
  {
    std::multimap<ParseToken, ParseToken>::const_iterator i = precedenceMap.lower_bound(token1);
    while(i != precedenceMap.end() && i->first == token1)
    {
      if(i->second == token2)
        return i;
      ++i;
    }
    return precedenceMap.end();
  }
  
  INLINE const Grammar::ProductionSet* Grammar::GetProductionSet(ParseToken nonterminal) const
  {
    std::map<ParseToken, ProductionSet*>::const_iterator i = productionSets.find(nonterminal);
    if(i == productionSets.end() || i->first != nonterminal)
      return null;
    return i->second;
  }

  INLINE Grammar::ProductionSet* Grammar::GetProductionSet(ParseToken nonterminal)
  {
    std::map<ParseToken, ProductionSet*>::iterator i = productionSets.find(nonterminal);
    if(i == productionSets.end() || i->first != nonterminal)
      return null;
    return i->second;
  }
  
  INLINE const Grammar::ProductionRule& Grammar::GetRule(uint index) const
  {
    return rules[index].first;
  }
  
  INLINE Grammar::ProductionRule& Grammar::GetRule(uint index)
  {
    return rules[index].first;
  }
  
  INLINE ParseToken Grammar::GetRuleToken(uint ruleIndex, uint tokenIndex) const
  {
    return rules[ruleIndex].first.tokens[tokenIndex];
  }

/*#ifdef _DEBUG
  void Grammar::OutputStatementMatch(ParseResult& result, uint index) const
  {
    ParseMatch& match = result.parseStream.data[index];
    if(TokenRegistry::IsTerminal(match.token))
      std::cout << getTokenName(match.token);
    else
    {
      std::cout << getTokenName(match.token) << " { ";

      for(uint c = 0; c < (uint)match.length; ++c)
      {
        outputStatementMatch(result, match.offset+c);
        std::cout << ' ';
      }

      std::cout << "} ";
    }
  }
 
  void Grammar::DebugOutputTokens() const
  {
    std::cout << std::endl
         << "Tokens" << std::endl
         << "------" << std::endl;
    for(TokenNames::const_iterator i = terminalNames.begin(); i != terminalNames.end(); ++i)
        std::cout << ' ' << i->second << std::endl;
  }

  void Grammar::DebugOutputProduction(const ProductionRule& rule) const
  {
    for(uint c = 0; c < rule.symbolsLength; ++c)
    {
      debugOutputSymbol(rule.symbols[c].token);
      if(int(c) < rule.symbolsLength-1)
        std::cout << ' ';
    }
  }

  void Grammar::DebugOutputProduction(OSid id, const ProductionRule& rule) const
  {
    debugOutputSymbol(id);
    std::cout << " -> ";
    debugOutputProduction(rule);
  }

  void Grammar::DebugOutputProductions() const
  {
    std::cout << std::endl
              << "Productions" << std::endl
              << "-----------" << std::endl;
    for(map<OSid, ProductionSet*>::const_iterator i = productionSets.begin(); i != productionSets.end(); ++i)
    {
      OSid productionId = i->first;

      for(uint cProduction = 0; cProduction < i->second->rulesLength; ++cProduction)
      {
        const ProductionRule& rule = productions[i->second->rulesOffset + cProduction].first;
        debugOutputProduction(productionId, rule);
        std::cout << std::endl;
      }
    }
  }

  void Grammar::DebugOutputParseResult(OSobject& parseResult) const
  {
    ParseResult& result = *(ParseResult*)parseResult;

    std::cout << "Parse Result:" << std::endl << std::endl;

    // Output lex tokens
    std::cout << "Lex tokens:" << std::endl;
    for(uint c = 0; c < result.lexStream.length; ++c)
      std::cout << getTokenName(result.lexStream.data[c].token) << ' ';
    std::cout << std::endl << std::endl;

    // Output global statements
    std::cout << "Statements:" << std::endl;
    if(result.parseStream.length > 0)
    {
      uint index = 0;
      outputStatementMatch(result, index);
      std::cout << std::endl << std::endl;
    }

    for(uint c = 0; c < result.parseStream.length; ++c)
    {
      debugOutputSymbol(result.parseStream.data[c].token);
      std::cout << ' ';
    }
    std::cout << std::endl;
    std::cout.flush();
  }

  void Grammar::DebugOutputSymbol(OSid symbol) const
  {
      std::cout << getTokenName(symbol);
  }
#endif*/
}

#endif
#endif
