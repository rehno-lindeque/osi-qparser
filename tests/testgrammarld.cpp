//////////////////////////////////////////////////////////////////////////////
//
//    TESTGRAMMARLD.CPP
//
//    Copyright © 2009, Rehno Lindeque. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////
/*                               DOCUMENTATION                              */
/*
    UNIT TEST:
      Test the LD grammar functionality
 */

/*                                 INCLUDES                                 */
// OpenParser
#include <osix/parser/parser.hpp>
#ifdef _DEBUG
  #include <osix/parser/parserdbg.hpp>
#endif

// BaseParser
#include <baseparser/baseparser.h>

// STL
#include <algorithm>

// QParser
#include "../src/token.h"
#include "../src/grammar.h"
#include "../src/grammarld.h"
using namespace QParser;

// QParser unit tests
#include "testcommon.h"

/*                                  ALIASES                                 */
typedef BuilderLD::ActionRow ActionRow;
typedef BuilderLD::PivotSet PivotSet;

/*                                 TEST DATA                                */
// Lexical tokens to use
ParseToken x = TOKEN_FLAG_SHIFT | 0;
ParseToken y = TOKEN_FLAG_SHIFT | 1;
ParseToken z = TOKEN_FLAG_SHIFT | 2; 
ParseToken w = TOKEN_FLAG_SHIFT | 3;

/*                                   TESTS                                  */
// Build a left-recursive grammar with unbounded look-ahead (found at around page 23 of the draft)
void BuildTestGrammar1(BuilderLD& builder)
{
  ParseToken rule1 = 0; // 1.A -> x
  ParseToken rule2 = 1; // 2.B -> x
  ParseToken rule3 = 2; // 3.C -> y
  ParseToken rule4 = 3; // 4.D -> AC
  ParseToken rule5 = 4; // 5.D -> DAC
  ParseToken rule6 = 5; // 6.E -> BC
  ParseToken rule7 = 6; // 7.E -> EBC
  ParseToken rule8 = 7; // 8.S -> Dz
  ParseToken rule9 = 8; // 9.S -> Ew
 
  // s(x), r(i), s(y), r(3), r(i), p{x > 1, z > 2, w > 3}, g{3 > 5}, rp(4), rp(1), r(8), accept
  ActionRow& row0 = builder.AddActionRow();
  row0.AddActionShift(x);
  row0.AddActionReduce(TOKEN_SPECIAL_IGNORE);
  row0.AddActionShift(y);
  row0.AddActionReduce(rule3);
  row0.AddActionReduce(TOKEN_SPECIAL_IGNORE);
  PivotSet& pivot0 = row0.AddActionPivot();
  row0.AddActionGoto(3, 5);
  row0.AddActionReducePrev(rule4);
  row0.AddActionReducePrev(rule1);
  row0.AddActionReduce(rule8);
  row0.AddActionAccept();

  // r(i), s(y), r(3), r(i), p{x > 1, z > 2, w > 3}, g{3 > 4}, rp(5), rp(1), return
  ActionRow& row1 = pivot0.AddPivot(x);
  row1.AddActionReduce(TOKEN_SPECIAL_IGNORE);
  row1.AddActionShift(y);
  row1.AddActionReduce(rule3);
  row1.AddActionReduce(TOKEN_SPECIAL_IGNORE);
  PivotSet& pivot1 = row1.AddActionPivot();
  pivot1.AddPivot(x, 1);
  pivot1.AddPivot(z, 2);
  pivot1.AddPivot(w, 3);
  row1.AddActionGoto(3, 4);
  row1.AddActionReducePrev(rule5);
  row1.AddActionReducePrev(rule1);
  row1.AddActionReturn();

  // return
  ActionRow& row2 = pivot0.AddPivot(z);
  row2.AddActionReturn();
  
  // return
  ActionRow& row3 = pivot0.AddPivot(w);
  row3.AddActionReturn();
  
  // rp(7), rp(2), return
  ActionRow& row4 = builder.AddActionRow();
  row4.AddActionReducePrev(rule7);
  row4.AddActionReducePrev(rule2);
  row4.AddActionReturn();
  
  // rp(6), rp(2), r(9), accept
  ActionRow& row5 = builder.AddActionRow();
  row5.AddActionReducePrev(rule6);
  row5.AddActionReducePrev(rule2);
  row5.AddActionReduce(rule9);
  row5.AddActionAccept();
}

void PackParseResult(Grammar::ParseResult& result, ParseToken* streamBegin, ParseToken* streamEnd)
{
  // Allocate the lex stream
  delete[] result.lexStream.data;
  result.lexStream.length = streamEnd-streamBegin;
  result.lexStream.data = new ParseMatch[result.lexStream.length];
  
  // Assign each match a token and an offset and length of 0.
  for(uint c = 0; c < result.lexStream.length; ++c)
  {
    result.lexStream.data[c].token = streamBegin[c];
    result.lexStream.data[c].offset = 0;
    result.lexStream.data[c].length = 0;
  }
}

void TestGrammar1()
{
  class TestGrammarLD : public GrammarLD
  {
  public:
    BuilderLD& TEST_GetBuilder() { return builder; }
  } grammar;
    
  //// Build the parse table
  BuildTestGrammar1(grammar.TEST_GetBuilder());
  grammar.constructProductions();
  
  //// Construct some test input (lexical) streams along with their expected results (rules)  
  // Stream 1: xyxyxyz (Correct input)
  ParseToken lexStream1[] = { x,y,x,y,x,y,z };
  ParseToken correctOutput1[] = { 0,2,3,0,2,4,0,2,4,7 };
  
  // Stream 2: xyxyxyz (Correct input)
  ParseToken lexStream2[] = { x,y,x,y,x,y,w };
  ParseToken correctOutput2[] = { 1,2,5,1,2,6,1,2,6,8 };
  
  // Stream 3: xyz (Correct input)
  ParseToken lexStream3[] = { x,y,w };
  ParseToken correctOutput3[] = { 0,2,3,7 };
  
  // Stream 4: xyxyxy (Incorrect input)
  ParseToken lexStream4[] = { x,y,x,y,x,y };
  
  // Stream 5: xz (Incorrect input)
  ParseToken lexStream5[] = { x,z };
  
  // Stream 6: xyyz (Incorrect input)
  ParseToken lexStream6[] = { x,y,y,z };
  
  //// Use the grammar class to parse the data
  Grammar::ParseResult parseResult;  
  PackParseResult(parseResult, lexStream1, lexStream1 + sizeof(lexStream1)/sizeof(ParseToken));
  grammar.parse(parseResult);
  
  // todo: busy here...
  
}

/*                                ENTRY POINT                               */
int main()
{
  cout << "-----------------------------------" << endl
       << "Testing GrammarLD: " << endl;
  cout.flush();
  TestGrammar1();
  
  cout << "SUCCESS" << endl;
  cout.flush();
  return 0;
}