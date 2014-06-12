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


template <typename RuleType>
RuleType seq(const RuleType& a, const RuleType& b) {
  return RuleType([=](StrIt& first, StrIt& last) -> ParseNode* {
    auto original = first;
    try {
      auto pNode = new typename RuleType::NodeType(); 
      pNode->pLeft.reset (a.parse(first, last));
      pNode->pRight.reset(b.parse(first, last));
      pNode->match = std::string(original, first);
      return pNode;
    }
    catch (std::exception& e) {
      first = original;
      throw;
    }
  });
}

template <typename RuleType>
RuleType alt(const RuleType& a, const RuleType& b) {
  return RuleType([=](StrIt& first, StrIt& last) -> typename RuleType::NodeType* {
    auto original = first;
    try {
      auto pNode = new typename RuleType::NodeType(); 
      pNode->pLeft.reset(a.parse(first, last));
      pNode->match = std::string(original, first);
      return pNode;
    }
    catch (std::exception& e) {
      first = original;
      auto pNode = new typename RuleType::NodeType(); 
      pNode->pRight.reset(b.parse(first, last));
      pNode->match = std::string(original, first);
      return pNode;
    }
  });
}

// Add a layer of indirection via a lambda
template <typename RuleType>
RuleType lazy(const RuleType& a) {
  return RuleType([&](StrIt& begin, StrIt& end) -> typename RuleType::NodeType* {
    return a.parse(begin, end);
  });
}

// maybe
template <typename RuleType>
RuleType maybe(const RuleType& a) {
  return alt(a, RuleType());
}

// TODO this doesn't work yet...
template <typename RuleType>
RuleType some(const RuleType& a) {
  throw std::runtime_error("unimplemented");
}

//------------------------------------------------------------------------------
// operators
template <typename RuleType>
RuleType operator+ (const RuleType& a, const RuleType& b) {
  return seq(a, b);
}

template <typename RuleType>
RuleType operator& (const RuleType& a, const RuleType& b) {
  return seq(a, b);
}

template <typename RuleType>
RuleType operator| (const RuleType& a, const RuleType& b) {
  return alt(a, b);
}

template <typename RuleType>
RuleType operator- (const RuleType& a) {
  return maybe(a);
}

template <typename RuleType>
RuleType operator~ (const RuleType& a) {
  return maybe(lazy(a));
}

template <typename RuleType>
RuleType operator* (const RuleType& a) {
  return many(a);
}

//------------------------------------------------------------------------------
