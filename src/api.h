#ifndef __QPARSER_API_H__
#define __QPARSER_API_H__
//////////////////////////////////////////////////////////////////////////////
//
//    API.H
//
//    Copyright © 2007-2008, Rehno Lindeque. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////
/*                               DOCUMENTATION                              */
/*
    DESCRIPTION:
      QParser API for OpenParser 0.2.

    TODO:
      + Implement identifier declaration versus reference operators in
        lexer / grammar

      + Implement identifier merging for string identifiers (identifiers
        consisting of more than one word) in lexer.

      + General clean up: memory leaks, code, conventions & depricated code

      OpenParser 0.25 goals:
      + Enable import / export of binary lookup table.

      OpenParser 0.3 goals: (For later implementations...)
      + Separate lex module so that a regular expression tokenizer
        can be plugged in if desired and so that lex libraries can be
        used separately from the parser.

      + (Investigate): Separate grammar state construction (LR / LL / LALR)
        from parse table construction so that different methods can be used.
        I.e. A regular lookup table can be constructed instead of a
        serialized binary tree structure. (First we should see what the
        performance differences are to see whether it makes sense though.
*/
/*                              COMPILER MACROS                             */

/*                                 INCLUDES                                 */
//// Standard Parser (pseudo-OSI)
//#include "../common/types.h"
//#include "../common/definitions.h"

// OpenParser
#include <osix/parser/parser.hpp>
#ifdef _DEBUG
  #include <osix/parser/parserdbg.hpp>
#endif

// BaseParser
#include <baseparser/baseparser.h>

// STL
#include <list>
#include <stack>
//#include <algorithm>

//// SIMD
//#include "../simd/simd.h"

// QParser
#include "token.h"
#include "grammar.h"
//#include "grammarll1.h"
//#include "grammarlr.h"
//#include "grammarlr0.h"
//#include "grammarlr1.h"
//#include "grammarlrk.h"
#include "grammarld.h"

/*                                  CLASSES                                 */
namespace QParser
{
  class Parser : public BaseParser::Parser
  {
  public:
    Grammar* grammar;

    FORCE_INLINE Parser() : grammar(null) {}

#   ifdef _DEBUG
      class ParserDbg : public OSIX::ParserDbg
      {
      public:
        /*OSI_DYNAMIC_METHOD void OSI_API_CALL debugOutputTokens();
        OSI_DYNAMIC_METHOD void OSI_API_CALL debugOutputProductions();
        OSI_DYNAMIC_METHOD void OSI_API_CALL debugOutputGrammar();
        OSI_DYNAMIC_METHOD void OSI_API_CALL debugOutputParseResult(OSobject parseResult);*/

        INLINE Parser& getParser() { return *(Parser*)(((uint8*)this) - (uint8*)&(*(Parser*)null).parserDbg); }
      } parserDbg;
#   endif
  };
}

/*                                   INCLUDES                               */
#ifdef OSI_STATIC_C_BUILD
#include "api.inl"
#endif

#endif
