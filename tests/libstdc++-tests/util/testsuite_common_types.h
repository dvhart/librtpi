// -*- C++ -*-
// typelist for the C++ library testsuite.
//
// Copyright (C) 2005-2022 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.
//

#ifndef _TESTSUITE_COMMON_TYPES_H
#define _TESTSUITE_COMMON_TYPES_H 1

#include <ext/typelist.h>

#include <ext/new_allocator.h>
#include <ext/malloc_allocator.h>
#include <ext/mt_allocator.h>
#include <ext/bitmap_allocator.h>
#include <ext/pool_allocator.h>

#include <algorithm>

#include <vector>
#include <list>
#include <deque>
#include <string>
#include <limits>

#include <map>
#include <set>

#if __cplusplus >= 201103L
#include <atomic>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
namespace unord = std;
#else
#include <tr1/unordered_map>
#include <tr1/unordered_set>
namespace unord = std::tr1;
#endif

namespace __gnu_test
{
  using __gnu_cxx::typelist::null_type;
  using __gnu_cxx::typelist::node;
  using __gnu_cxx::typelist::transform;
  using __gnu_cxx::typelist::append;

#if __cplusplus >= 201103L

  struct constexpr_comparison_eq_ne
  {
    template<typename _Tp1, typename _Tp2 = _Tp1>
      void
      operator()()
      {
	static_assert(_Tp1() == _Tp2(), "eq");
	static_assert(!(_Tp1() != _Tp2()), "ne");
      }
  };

  struct constexpr_comparison_operators
  {
    template<typename _Tp>
      void
      operator()()
      {
	static_assert(!(_Tp() < _Tp()), "less");
	static_assert(_Tp() <= _Tp(), "leq");
	static_assert(!(_Tp() > _Tp()), "more");
	static_assert(_Tp() >= _Tp(), "meq");
	static_assert(_Tp() == _Tp(), "eq");
	static_assert(!(_Tp() != _Tp()), "ne");
      }
  };

  struct has_trivial_cons_dtor
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    typedef std::is_trivially_default_constructible<_Tp> ctor_p;
	    static_assert(ctor_p::value, "default constructor not trivial");

	    typedef std::is_trivially_destructible<_Tp> dtor_p;
	    static_assert(dtor_p::value, "destructor not trivial");
	  }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  struct has_trivial_dtor
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    typedef std::is_trivially_destructible<_Tp> dtor_p;
	    static_assert(dtor_p::value, "destructor not trivial");
	  }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test standard layout
  struct standard_layout
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    typedef std::is_standard_layout<_Tp> standard_layout_p;
	    static_assert(standard_layout_p::value, "not standard_layout");
	  }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };
#endif

  // Generator to test base class
  struct has_required_base_class
  {
    template<typename _TBase, typename _TDerived>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    const _TDerived& obj = __a;
	    const _TBase* base __attribute__((unused)) = &obj;
	  }

	  _TDerived __a;
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test assignment operator.
  struct assignable
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  { __v1 = __v2; }

	  _Tp __v1;
	  _Tp __v2;
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test default constructor.
  struct default_constructible
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  { _Tp __v __attribute__((unused)); }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test copy constructor.
  struct copy_constructible
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  { _Tp __v2(__v1); }

	  _Tp __v1;
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test direct initialization, single value constructor.
  struct single_value_constructible
  {
    template<typename _Ttype, typename _Tvalue>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  { _Ttype __v(__a); }

	  _Tvalue __a;
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

#if __cplusplus >= 201103L
  // Generator to test non-explicit default constructor.
  struct implicitly_default_constructible
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  struct Aggregate { _Tp v; };

	  void __constraint()
	  { Aggregate __v __attribute__((unused)) = { }; }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test default constructor.
  struct constexpr_default_constructible
  {
    template<typename _Tp, bool _IsLitp = __is_literal_type(_Tp)>
      struct _Concept;

    // NB: _Tp must be a literal type.
    // Have to have user-defined default ctor for this to work,
    // or implicit default ctor must initialize all members.
    template<typename _Tp>
      struct _Concept<_Tp, true>
      {
	void __constraint()
	{ constexpr _Tp __obj; }
      };

    // Non-literal type, declare local static and verify no
    // constructors generated for _Tp within the translation unit.
    template<typename _Tp>
      struct _Concept<_Tp, false>
      {
	void __constraint()
	{ static _Tp __obj; }
      };

    template<typename _Tp>
      void
      operator()()
      {
	_Concept<_Tp> c;
	c.__constraint();
      }
  };

  // Generator to test defaulted default constructor.
  struct constexpr_defaulted_default_constructible
  {
    template<typename _Tp>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  { constexpr _Tp __v __attribute__((unused)) { }; }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  struct constexpr_single_value_constructible
  {
    template<typename _Ttesttype, typename _Tvaluetype,
	     bool _IsLitp = __is_literal_type(_Ttesttype)>
      struct _Concept;

    // NB: _Tvaluetype and _Ttesttype must be literal types.
    // Additional constraint on _Tvaluetype needed.  Either assume
    // user-defined default ctor as per
    // constexpr_default_constructible and provide no initializer,
    // provide an initializer, or assume empty-list init-able. Choose
    // the latter.
    template<typename _Ttesttype, typename _Tvaluetype>
      struct _Concept<_Ttesttype, _Tvaluetype, true>
      {
	void __constraint()
	{
	  constexpr _Tvaluetype __v { };
	  constexpr _Ttesttype __obj(__v);
	}
      };

    template<typename _Ttesttype, typename _Tvaluetype>
      struct _Concept<_Ttesttype, _Tvaluetype, false>
      {
	void __constraint()
	{
	  const _Tvaluetype __v { };
	  static _Ttesttype __obj(__v);
	}
      };

    template<typename _Ttesttype, typename _Tvaluetype>
      void
      operator()()
      {
	_Concept<_Ttesttype, _Tvaluetype> c;
	c.__constraint();
      }
  };
#endif

  // Generator to test direct list initialization
#if __cplusplus >= 201103L
  struct direct_list_initializable
  {
    template<typename _Ttype, typename _Tvalue>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    _Ttype __v1 { }; // default ctor
	    _Ttype __v2 { __a };  // single-argument ctor
	  }

	  _Tvalue __a;
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };
#endif

  // Generator to test copy list initialization, aggregate initialization
  struct copy_list_initializable
  {
    template<typename _Ttype, typename _Tvalue>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  { _Ttype __v __attribute__((unused)) = {__a}; }

	  _Tvalue __a;
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test integral conversion operator
  struct integral_convertable
  {
    template<typename _Ttype, typename _Tvalue>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    _Tvalue __v0(0);
	    _Tvalue __v1(1);
	    _Ttype __a(__v1);
	    __v0 = __a;

	    VERIFY( __v1 == __v0 );
	  }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

  // Generator to test integral assignment operator
  struct integral_assignable
  {
    template<typename _Ttype, typename _Tvalue>
      void
      operator()()
      {
	struct _Concept
	{
	  void __constraint()
	  {
	    _Tvalue __v0(0);
	    _Tvalue __v1(1);
	    _Ttype __a(__v0);
	    __a = __v1;
	    _Tvalue __vr = __a;

	    VERIFY( __v1 == __vr );
	  }
	};

	void (_Concept::*__x)() __attribute__((unused))
	  = &_Concept::__constraint;
      }
  };

#if __cplusplus >= 201103L
  // Generator to test lowering requirements for compare-and-exchange.
  template<std::memory_order _Torder>
  struct compare_exchange_order_lowering
  {
    template<typename _Tp>
      void
      operator()()
      {
        std::atomic<_Tp> __x;
        _Tp __expected = 0;
        __x.compare_exchange_strong(__expected, 1, _Torder);
        __x.compare_exchange_weak(__expected, 1, _Torder);
      }
  };
#endif

#if __cplusplus >= 201402L
  // Check that bitmask type T supports all the required operators,
  // with the required semantics. Check that each bitmask element
  // has a distinct, nonzero value, and that each bitmask constant
  // has no bits set which do not correspond to a bitmask element.
  template<typename T>
    constexpr bool
    test_bitmask_values(std::initializer_list<T> elements,
			std::initializer_list<T> constants = {})
    {
      const T t0{};

      if (!(t0 == t0))
	return false;
      if (t0 != t0)
	return false;

      if (t0 & t0)
	return false;
      if (t0 | t0)
	return false;
      if (t0 ^ t0)
	return false;

      T all = t0;

      for (auto t : elements)
	{
	  // Each bitmask element has a distinct value.
	  if (t & all)
	    return false;

	  // Each bitmask element has a nonzero value.
	  if (!bool(t))
	    return false;

	  // Check operators

	  if (!(t == t))
	    return false;
	  if (t != t)
	    return false;
	  if (t == t0)
	    return false;
	  if (t == all)
	    return false;

	  if (t & t0)
	    return false;
	  if ((t | t0) != t)
	    return false;
	  if ((t ^ t0) != t)
	    return false;

	  if ((t & t) != t)
	    return false;
	  if ((t | t) != t)
	    return false;
	  if (t ^ t)
	    return false;

	  T t1 = t;
	  if ((t1 &= t) != t)
	    return false;
	  if ((t1 |= t) != t)
	    return false;
	  if (t1 ^= t)
	    return false;

	  t1 = all;
	  if ((t1 &= t) != (all & t))
	    return false;
	  t1 = all;
	  if ((t1 |= t) != (all | t))
	    return false;
	  t1 = all;
	  if ((t1 ^= t) != (all ^ t))
	    return false;

	  all |= t;
	  if (!(all & t))
	    return false;
	}

      for (auto t : constants)
	{
	  // Check that bitmask constants are composed of the bitmask elements.
	  if ((t & all) != t)
	    return false;
	  if ((t | all) != all)
	    return false;
	}

      return true;
    }
#endif // C++14


} // namespace __gnu_test
#endif
