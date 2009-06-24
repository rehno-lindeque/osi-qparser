#ifdef  __QPARSER_GRAMMARLD_H__
#ifndef __QPARSER_GRAMMARLD_INL__
#define __QPARSER_GRAMMARLD_INL__
//////////////////////////////////////////////////////////////////////////////
//
//    GRAMMARLD.INL
//
//    Copyright © 2007-2009, Rehno Lindeque. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

namespace QParser
{
  GrammarLD::~GrammarLD()
  {}
  
  void GrammarLD::ConstructParseTable(ParseTokens& parseTable)
  {
    // Ensure that the root nonterminal a valid nonterminal token
    // We use the last production defined if no root was given by the user    
    if(rootNonterminal == ParseToken(-1))
    {
      if(tokenRegistry.GetNextAvailableNonterminal() == TOKEN_NONTERMINAL_FIRST) 
        return; // error: no nonterminals defined
      else 
        rootNonterminal = tokenRegistry.GetNextAvailableNonterminal() - 1; // Use the last defined nonterminal our root nonterminal
    }
    
    // Construct the the parser
    // Get the start items
    BuilderLD builder;
    states.push_back(new State(builder.AddActionRow()));
    GetStartItems(rootNonterminal, states[0]->items);
    
    // Construct the state graph recursively until we are done
    ConstructStateGraph(builder, *states[0]);
    
    // Use the builder to construct the final parse table
    builder.ConstructParseTable(parseTable);
  }
  
  INLINE void GrammarLD::ConstructStateGraph(BuilderLD& builder, State& state)
  {
    while(true)
    {
      // Expand items in the state
      ExpandItemSet(state);

      // Get the set of terminals (at the input positions) that can be stepped over
      ParseTokenSet terminals;
      StepOverTerminals(terminals, state.items);
      if(terminals.size() == 1)
      {
        // Geneterminalsrate a shift action
        state.row.AddActionShift(*terminals.begin());
        
        // Complete all rules that are left over
        bool allItemsComplete = CompleteItems(builder, state);
        
        // Stop if there are no more items in this state to complete
        if(allItemsComplete)
        {
          //state.row.allItemsCompleteAddActionReturn();
          return; // No items left to complete
        }
      }
      else
      {
        // Pre-condition: There will always be terminals left after the state has been expanded
        //                because then all items must necessarily be complete and rules are not allowed to be empty.
        OSI_ASSERT(terminals.size() > 1);
        /*if(terminals.size() == 0)
          return;*/
        
        // Generate a pivot
        auto& pivots = state.row.AddActionPivot();
        for(auto i = terminals.begin(); i != terminals.end(); ++i)
        {
          // Copy the state and generate a new line in the action table to couple with it
          states.push_back(new State(pivots.AddPivot(*i)));
          auto& targetState = *states.back();
          CopyStateUsingPivot(state, targetState, *i);
          
          // Add the edge to both states
          state.outgoingPivots.insert(std::make_pair(&targetState, *i));
          targetState.incomingPivots.insert(std::make_pair(&state, *i));
          
          // Complete all rules that are now finished (in the target state)
          bool allItemsComplete = CompleteItems(builder, targetState);

          // Stop if there are no more items in this state to complete
          if(allItemsComplete)
            continue; // No items left to complete
          
          /*// Recursively construct the new state and all of its children
          ConstructStateGraph(builder, targetState);*/
        }
        
        // Since we've reached a pivot, this row is now complete (only one decision (pivot) is allowed per row).
        // todo: determine when we can place an "accept" action rather than a "return" statement
        //state.row.AddActionReturn();
        return;
      }
    }
  }
    
  INLINE void GrammarLD::ExpandItemSet(State& state)
  {
    // Expand all items in the set
    ExpandItemSet(state, 0, state.items.size());
  }
  
  INLINE void GrammarLD::ExpandItemSet(State& state, uint cBegin, uint cEnd)
  {
    // Check whether there are any items left to expand
    if (cEnd <= cBegin+1)
      return;
    
    // Expand each item in the given range
    for (uint c = cBegin; c != cEnd; ++c)
    {
      auto& item = state.items[c];
      const ProductionRule& rule = GetRule(item.ruleIndex);
      
      // Check whether this item has already been resolved
      if(item.inputPositionRule != uint(-1))
        continue; 
      
      // Check whether the rule is complete: We cannot expand the rule further if it is complete
      if(IsItemComplete(item))
        continue;
      
      // Check whether the token at the input position is a terminal: We cannot expand a terminal token
      auto inputToken = rule.tokens[item.inputPosition];
      if(TokenRegistry::IsTerminal(inputToken))
        continue;
        
      // Resolve the input position token to rule(s)
      auto& inputProduction = *GetProductionSet(inputToken);
      
      for(auto cRuleIndex = inputProduction.rulesOffset; cRuleIndex < inputProduction.rulesOffset + inputProduction.rulesLength; ++cRuleIndex)
      {
        // Get the item again (because calling push_back may invalidate our previous reference!)
        auto& item = state.items[c];
        
        // Resolve the originating item's input position rule
        // (Note: the first time we'll merely set the input position rule, but there-after
        // the item must be duplicated for every possible input rule corresponding to the nonterminal)
        if(item.inputPositionRule == uint(-1))
        {
          item.inputPositionRule = cRuleIndex;
        }
        else
        {
          state.items.push_back(item);
          state.items.back().inputPositionRule = cRuleIndex;
        }
        
        // Generate a new item for the rule
        // Check whether a similar item already exists...
        Item newItem(inputToken, cRuleIndex);
        auto i = std::find(state.items.begin(), state.items.end(), newItem);
        if(i != state.items.end())
          continue; // The item already exists in the list
        
        // Add the new item to the state
        state.items.push_back(newItem);
      }
    }
    
    // Expand the newly added items
    ExpandItemSet(state, cEnd, state.items.size());
  }
  
  INLINE void GrammarLD::StepOverTerminals(ParseTokenSet& terminals, Items& items) const
  {
    for(auto i = items.begin(); i != items.end(); ++i)
    {
      const auto& rule = GetRule(i->ruleIndex);
      
      // Ignore items that are complete      
      if(IsItemComplete(*i))
        continue;
      
      if(TokenRegistry::IsTerminal(rule.tokens[i->inputPosition]))
      {
        // Add terminal token to the set
        terminals.insert(rule.tokens[i->inputPosition]);
        
        // Step over terminal and reset the input position rule
        ++i->inputPosition;
        i->inputPositionRule = uint(-1);
      }
    }
  }
  
  INLINE bool GrammarLD::CompleteItems(BuilderLD& builder, State& state)
  {
    Items& items = state.items; // Alias for the items
    
    // Find all rules corresponding with complete items
    bool allItemsComplete = true;     // Flag indicating that all items in the state have been completed
    bool waitingShiftActions = false; // Flag indicating that some items in the state have their input position in front of terminal tokens
    
    std::set<uint> completeRules;           // The set of all rules that have been completed
    std::vector<uint> completeItemIndexes;  // A list of indexes to the items that were completed
    for(uint c = 0; c < items.size(); ++c)
    {
      if(IsItemComplete(items[c]))
      {
        completeRules.insert(items[c].ruleIndex);
        completeItemIndexes.push_back(c);
        std::cout  << items[c].ruleIndex << std::endl;
      }
      else
      {
        allItemsComplete = false;
        
        if(TokenRegistry::IsTerminal(GetRuleToken(items[c].ruleIndex, items[c].inputPosition)))
          waitingShiftActions = true;
      }
    }
    
    // If no more complete items could be found: All items are complete or all remaining items are incomplete
    if(completeRules.empty())
      return false;
    
    // TODO: This is probably not the best way of doing things. We can probably improve the algorithm by reducing previous delays
    //       once we have exactly 1 path left, regardless of whether the items are complete or not,
    //       However, this means that we must then continue the parsing of those items in the parent node (and take into account gotos as well)
    //       Actually 
    
    //todo? If all items are complete, store the set of complete rules in the state    
    if (allItemsComplete)
    {
      // If more than one item is complete then the grammar is ambiguous
      if(completeRules.size() > 1)
      {
        // todo: ambiguous grammar
        // print out an error message or produce an ambiguous parse table...
        // e.g. ResolveDelaysAmbiguous(state, completeRules);
        
        //state.ambiguous = true;
      }
      else
      {
        OSI_ASSERT(completeRules.size() == 1);
                
        // Resolve all delayed reductions associated with this leaf state
        ResolveDelays(builder, state, *completeRules.begin());
      }
      return true;
    }

    // Add a reduce / reduce prev action
    if(completeRules.size() > 1 || waitingShiftActions)
    {
      // If there are multiple completed rules, add a delayed reduce action
      state.row.AddActionReduce(TOKEN_SPECIAL_IGNORE);
      
      // Also push the possible reduce tokens onto the state's delayed stack
      state.delayedReductions.push_back(LDState::DelayedRuleMap());
      auto& delayedRuleMap = state.delayedReductions.back();
      for(auto i = completeItemIndexes.begin(); i != completeItemIndexes.end(); ++i)
      {
        const auto& item = items[*i];
        
        // Find all items that have a reference to this rule at their input positions
        for(uint c = 0; c < items.size(); ++c)
        {
          // Check whether the item has a reference to this item's rule 
          if(!IsItemComplete(items[c]) && items[c].inputPositionRule == item.ruleIndex)
            delayedRuleMap.insert(std::make_pair(items[c].ruleIndex, item.ruleIndex));
        }        
      }
      
      // Todo: What happens here if all items in the state is complete?
      //       (i.e. allItemsComplete == true)
      //       Does this mean that the grammar is ambiguous?
    }
    else
    {
      // If there is only one rule, add a reduce action 
      state.row.AddActionReduce(*completeRules.begin());
    }
    
    // Remove complete items from the state
    uint c = 0;
    while(c < items.size())
    {
      if(completeRules.find(items[c].ruleIndex) != completeRules.end())
      {
        items.erase(items.begin()+c);
        continue;
      }

      ++c;
    }

    // Step over complete rules in the remaining incomplete items
    for(uint c = 0; c < items.size(); ++c)
    {
      // Check whether the token at the input position corresponds to a complete rule
      // (All complete items have been removed so we do not need to check for a valid input position here)
      if(completeRules.find(items[c].inputPositionRule) != completeRules.end())
      {
        // Step over the token
        ++items[c].inputPosition;
        items[c].inputPositionRule = uint(-1);
      }
    }
    
    // Repeat the process (until all items are complete or all rules remain incomplete)
    return CompleteItems(builder, state);
  }
  
  INLINE void GrammarLD::CopyStateUsingPivot(const State& state, State& targetState, ParseToken pivotTerminal) const
  {
    std::vector<bool> copyItemsSubset(state.items.size()); // A vector indicating which items should be copied (which items are relevant)
    std::set<uint> copyRuleIndexes;                        // The set of all rule indexes that are produced by the algorithm
    
    // Get the set of rules that have the pivot terminal as their previous token and fill the set of next rules in order 
    // to perform quick updates
    for(uint c = 0; c < state.items.size(); ++c)
    {
      const auto& item = state.items[c];
      if(item.inputPosition > 0 && GetRuleToken(item.ruleIndex, item.inputPosition-1) == pivotTerminal)
      {
        copyItemsSubset[c] = true;
        copyRuleIndexes.insert(item.ruleIndex);
      }
      else
        copyItemsSubset[c] = false;
    }
    OSI_ASSERT(!rules.empty()); // Post-condition: at least one rule must reference the pivot terminal
    
    // Include items in the copy subset if their input position rule refers to any of the rules already in the subset
    bool copySubsetChanged; // A flag indicating that the copy subset changed (grew during this iteration). The loop is terminated once it can no longer grow.
    do
    {
      // Reset the termination flag
      copySubsetChanged = false;
              
      for(uint c = 0; c < state.items.size(); ++c)
      {
        const auto& item = state.items[c];
        
        // Ignore items that are already in the copy subset
        if(copyItemsSubset[c])
          continue; // The rule is already in the set
        
        // Test whether this rule should be included
        if(copyRuleIndexes.find(item.inputPositionRule) == copyRuleIndexes.end())
          continue; // The rule should not be included (yet)
        
        copyRuleIndexes.insert(item.ruleIndex);
        copyItemsSubset[c] = true;
        copySubsetChanged = true;
      }
    } while(copySubsetChanged);
    
    // Copy the resulting subset of items to the new state
    for(uint c = 0; c < state.items.size(); ++c)
      if(copyItemsSubset[c])
        targetState.items.push_back(state.items[c]);
  }
  
  INLINE void GrammarLD::ResolveDelays(BuilderLD& builder, State& leafState, uint leafRule)
  {
    // Determine the index of the leaf state's row in the parse table
    auto leafRowIndex = builder.GetRowIndex(leafState.row);
    OSI_ASSERT(leafRowIndex != ParseToken(-1));
    
    // Start with the leaf rule that was reduced last
    std::stack<uint> ruleResolutionStack; // A stack of the last reduced rules (used to determine the next delayed rule to reduce)
    ruleResolutionStack.push(leafRule);
    
    /*// Get all edges leading to this state
    for(auto iState = leafState.leadingEdges.begin(); iState != leafState.leadingEdges.end(); ++iState)
    {
      auto& edgeState = *iState->first;      // The state leading to the current state
      //auto edgeTerminal = iState->second;   // The terminal token of the edge leading to the current state
      
      // Add a new row for the goto action in the parent state
      auto& gotoRow = builder.AddActionRow();
      auto gotoRowIndex = builder.GetRowIndex(gotoRow);
      OSI_ASSERT(gotoRowIndex != ParseToken(-1));
      edgeState.row.AddActionGoto(leafRowIndex, gotoRowIndex);

      // Resolve each reduction in the state using the leaf state row
      for(auto i = edgeState.delayedReductions.rbegin(); i != edgeState.delayedReductions.rend(); ++i)
      {
        const auto& delayedRuleMap = *i; // The delayed rule map at currently

        // Find the last rule in the rule resolution stack that corresponds to the parent of a delayed reduction
        while(true)
        {
          // If the rule resolution stack has run out of elements a new leaf state will need to be found for any remaining reductions
          if(ruleResolutionStack.empty())
            return;

          // If the rule in the stack doesn't correspond with any delayed reductions, pop it off of the stack and then try it with the next one
          auto iDelayedRule = delayedRuleMap.find(ruleResolutionStack.top());
          if(iDelayedRule == delayedRuleMap.end())
          {
            ruleResolutionStack.pop();
            continue;
          }

          // The delayed rule can be resolved (add a reduce previous action)
          gotoRow.AddActionReducePrev(iDelayedRule->second);
        }
      }
      
      // Add a return action to the goto row to end it (todo: when should we add an accept action?)
      gotoRow.AddActionReturn();
      
      // Resolve the delayed actions of this state's parents
      
      // todo....
    }*/
    
    // Resolve the delayed actions of this state's parents
    for(auto i = leafState.incomingPivots.begin(); i != leafState.incomingPivots.end(); ++i)
      ResolveDelays(builder, leafState, leafRowIndex, *i->first, ruleResolutionStack);
  }
  
  INLINE void GrammarLD::ResolveDelays(BuilderLD& builder, State& leafState, ParseToken leafRowIndex, State& currentState, std::stack<uint>& ruleResolutionStack)
  {
    // Determine the index of the current state's row in the parse table
    /*auto rowIndex = builder.GetRowIndex(currentState.row);
    OSI_ASSERT(rowIndex != ParseToken(-1));*/
    
    // Add a new row for the coming goto action in this state
    auto& gotoRow = builder.AddActionRow();
    auto gotoRowIndex = builder.GetRowIndex(gotoRow);
    OSI_ASSERT(gotoRowIndex != ParseToken(-1));
    currentState.row.AddActionGoto(leafRowIndex, gotoRowIndex);

    // Resolve each reduction in the state using the leaf state row
    for(auto i = currentState.delayedReductions.rbegin(); i != currentState.delayedReductions.rend(); ++i)
    {
      const auto& delayedRuleMap = *i; // The delayed rule map at the current 

      // Find the last rule in the rule resolution stack that corresponds to the parent of a delayed reduction
      while(true)
      {
        // If the rule resolution stack has run out of elements a new leaf state will need to be found for any remaining reductions
        if(ruleResolutionStack.empty())
          return;

        // If the rule in the stack doesn't correspond with any delayed reductions, pop it off of the stack and then try it with the next one
        auto iDelayedRule = delayedRuleMap.find(ruleResolutionStack.top());
        if(iDelayedRule == delayedRuleMap.end())
        {
          ruleResolutionStack.pop();
          continue;
        }

        // The delayed rule can be resolved (add a reduce previous action and continue to the next delayed rule)
        gotoRow.AddActionReducePrev(iDelayedRule->second);
        break;
      }
    }

    // Add a return action to the goto row to end it (todo: when should we add an accept action?)
    gotoRow.AddActionReturn();

    // Resolve the delayed actions of this state's parents
    for(auto i = currentState.incomingPivots.begin(); i != currentState.incomingPivots.end(); ++i)
      ResolveDelays(builder, leafState, leafRowIndex, *i->first, ruleResolutionStack);
  }
  
  /*INLINE void GrammarLD::Closure(Items& items)
  {
    Closure(items, 0, items.size());
  }

  INLINE void GrammarLD::Closure(Items& items, uint cBegin, uint cEnd)
  {
    for(uint c = cBegin; c < cEnd; ++c)
    {
      auto& item = items[c];
      const auto& rule = rules[item.ruleIndex].first;

      // Check whether the last symbol has been passed
      if(item.inputPosition >= rule.tokensLength)
        continue;

      ParseToken token = rule.tokens[item.inputPosition];

      // Check whether the symbol at input position is a terminal symbol
      if(TokenRegistry::IsTerminal(token))
        continue;

      // Add the closure of the nonterminal at input position to the set
      // (but first construct a starting item set for the production)
      GetStartItems(token, items);
    }

    if(cEnd < items.size())
      Closure(items, cEnd, items.size());
  }

  INLINE void GrammarLD::GoTo(States& states)
  {
    GoTo(states, 0, states.size());
  }

  INLINE void GrammarLD::GoTo(States& states, uint cBegin, uint cEnd)
  {
    using std::pair;
    
    for(uint cState = cBegin; cState < cEnd; ++cState)
    {
      auto& state = *states[cState];

      for(uint cItem = 0; cItem < state.items.size(); ++cItem)
      {
        const auto& item = state.items[cItem];
        const auto& rule = rules[item.ruleIndex].first;

        // Check whether the last symbol has already been passed
        if(item.inputPosition >= rule.tokensLength)
          continue;

        ParseToken token = rule.tokens[item.inputPosition];

        // Construct goto item
        Item goToItem = item;
        goToItem.inputPosition = item.inputPosition + 1;

        // Check whether there's already an edge for this symbol in the current state
        // Note: This is not legal for LR(0) grammars! However, SLR grammars can sometimes handle this)
        auto iEdge = state.edges.find(token);
        int targetStateIndex;

        if(iEdge == state.edges.end() || iEdge->first != token)
        {
          // Check whether there's already a state defined for this goto item
          // (This is something that [modcomp] doesn't really mention in any detail - we should look for existing target states for
          // this item or we'll end up in an infinite loop for any recursive grammar. It's very possible that a different state
          // will have the same item leading to some existing state.)
          targetStateIndex = FindItemState(item);

          if (targetStateIndex == -1)
          {
            // Create a target state and a shift/goto edge
            states.push_back(new State);
            targetStateIndex = states.size() - 1;
            state.edges.insert(pair<ParseToken, int>(token, targetStateIndex));
            itemStateMap.insert(pair<Item, uint>(item, targetStateIndex));
          }
          else
          {
            state.edges.insert(pair<ParseToken, int>(token, targetStateIndex));
            continue; // we can continue since the item is already in the target state (i.e. there's no need to add it a second time)
          }
        }
        else
        {
          // (Since there's already an edge for this symbol it follows that the target state also already exists...)
          targetStateIndex = iEdge->second;
        }

        auto& targetState = *states[targetStateIndex];

        // Add goto item to set (J) (the target state)

        // Note: we still have to check whether the state already contains the goto item since it could have
        // been added by the closure of another item in the state
        Items::iterator i;
        for(i = targetState.items.begin(); i != targetState.items.end(); ++i) if(*i == goToItem) break;
        if(i != targetState.items.end()) continue;

        targetState.items.push_back(goToItem);

        // Add closure of goto item to the set (Note that the last item added to the targetState is the goto item we're refering to)
        Closure(targetState.items, targetState.items.size()-1, targetState.items.size());
      }
    }

    if(cEnd < states.size())
      GoTo(states, cEnd, states.size());
  }*/
}

#endif
#endif
