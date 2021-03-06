#ifndef __QPARSER_FUNCTORS_H__
#define __QPARSER_FUNCTORS_H__
//////////////////////////////////////////////////////////////////////////////
//
//    FUNCTORS.H
//
//    Copyright © 2009, Rehno Lindeque. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////
/*                               DOCUMENTATION                              */
/*
    DESCRIPTION:
      Common functors for functional style map-reduce operations.
*/
namespace QParser
{
  template<class Type>
  struct DeleteFunctor { INLINE void operator()(Type* obj) const { delete obj; } };  
}

#endif
