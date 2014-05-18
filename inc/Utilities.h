//------------------------------------------------------------------------------
/*
*  
*  The MIT License (MIT)
* 
*  Copyright (C) 2014 Cody Griffin (cody.m.griffin@gmail.com)
* 
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
* 
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
* 
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#ifndef INCLUDED_UTILITIES_H
#define INCLUDED_UTILITIES_H

#ifndef INCLUDED_TUPLE
#include <tuple>
#define INCLUDED_TUPLE
#endif

//------------------------------------------------------------------------------

namespace corvid { 

//------------------------------------------------------------------------------

template <int N>                         
struct TupleApply;

template <int N>                         
struct TupleApplyBases;

template <int N>                         
struct TupleTest;

template <int N>                         
struct TupleWeakTest;

template <int N, typename Tuple, typename Base> 
struct MatchBaseType;

template <int N, typename A>             
struct TupleGetType;

template <int N, typename A>             
struct TupleGetBase;

template <int N, typename B>             
struct TupleProject;

template <int N, typename B>             
struct TupleBaseProject;

template <typename V, typename A>
void apply(V& v, A& a);

template <typename A, typename R, typename...S>
R    apply(R (*f)(S...), A& a);

template <typename P, typename A>
bool test(P& p, A& a);

template <typename T, typename A> 
T& getType (A& tuple);

template <typename Base, typename Tuple> 
typename MatchBaseType<std::tuple_size<Tuple>::value-1, Tuple, Base>::Type& 
getBase (Tuple& tuple);

template <typename A, typename B>
void projectTuple(A& a, B& b);

template <typename A, typename B>
bool projectBases(A& a, B& b);

//------------------------------------------------------------------------------

} // namespace corvid

//------------------------------------------------------------------------------

#include <Utilities.hpp>

#endif
