/* Copyright 2006-2024 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_DETAIL_FLYWEIGHT_CORE_HPP
#define BOOST_FLYWEIGHT_DETAIL_FLYWEIGHT_CORE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/core/no_exceptions_support.hpp>
#include <boost/config/workaround.hpp>
#include <boost/flyweight/detail/perfect_fwd.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/type_traits/declval.hpp>
#include <utility>

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1400))
#pragma warning(push)
#pragma warning(disable:4101)  /* unreferenced local vars */
#endif

/* flyweight_core provides the inner implementation of flyweight<> by
 * weaving together a value policy, a flyweight factory, a holder for the
 * factory,a tracking policy and a locking policy.
 */

namespace boost{

namespace flyweights{

namespace detail{

template<
  typename ValuePolicy,typename Tag,typename TrackingPolicy,
  typename FactorySpecifier,typename LockingPolicy,typename HolderSpecifier
>
class flyweight_core;

template<
  typename ValuePolicy,typename Tag,typename TrackingPolicy,
  typename FactorySpecifier,typename LockingPolicy,typename HolderSpecifier
>
struct flyweight_core_tracking_helper
{
private:
  typedef flyweight_core<
    ValuePolicy,Tag,TrackingPolicy,
    FactorySpecifier,LockingPolicy,
    HolderSpecifier
  >                                   core;
  typedef typename core::handle_type  handle_type;
  typedef typename core::entry_type   entry_type;
  
public:
  static const entry_type& entry(const handle_type& h)
  {
    return core::entry(h);
  }

  template<typename Checker>
  static void erase(const handle_type& h,Checker chk)
  {
    typedef typename core::lock_type lock_type;
    core::init();
    lock_type lock(core::mutex());(void)lock;
    if(chk(h))core::factory().erase(h);
  }
};

/* ADL-based customization point for factories providing the undocumented
 * insert_and_visit facility rather than regular insert. Default behavior is
 * to erase the entry if visitation throws.
 */

template<typename Factory,typename Entry,typename F>
typename Factory::handle_type
insert_and_visit(Factory& fac,const Entry& x,F f)
{
  typedef typename Factory::handle_type handle_type;

  handle_type h(fac.insert(x));
  BOOST_TRY{
    f(fac.entry(h));
  }
  BOOST_CATCH(...){
    fac().erase(h);
    BOOST_RETHROW;
  }
  BOOST_CATCH_END
  return h;
}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
template<typename Factory,typename Entry,typename F>
typename Factory::handle_type
insert_and_visit(Factory& fac,Entry&& x,F f)
{
  typedef typename Factory::handle_type handle_type;

  handle_type h(fac.insert(std::forward<Entry>(x)));
  BOOST_TRY{
    f(fac.entry(h));
  }
  BOOST_CATCH(...){
    fac.erase(h);
    BOOST_RETHROW;
  }
  BOOST_CATCH_END
  return h;
}
#endif

template<
  typename ValuePolicy,typename Tag,typename TrackingPolicy,
  typename FactorySpecifier,typename LockingPolicy,typename HolderSpecifier
>
class flyweight_core
{
public:
  typedef typename ValuePolicy::key_type     key_type;
  typedef typename ValuePolicy::value_type   value_type;
  typedef typename ValuePolicy::rep_type     rep_type;
  typedef typename mpl::apply2<
    typename TrackingPolicy::entry_type,
    rep_type,
    key_type
  >::type                                    entry_type;
  typedef typename mpl::apply2<
    FactorySpecifier,
    entry_type,
    key_type
  >::type                                    factory_type;
  typedef typename factory_type::handle_type base_handle_type;
  typedef typename mpl::apply2<
    typename TrackingPolicy::handle_type,
    base_handle_type,
    flyweight_core_tracking_helper<
      ValuePolicy,Tag,TrackingPolicy,
      FactorySpecifier,LockingPolicy,
      HolderSpecifier
    >
  >::type                                    handle_type;
  typedef typename LockingPolicy::mutex_type mutex_type;
  typedef typename LockingPolicy::lock_type  lock_type;

  static bool init()
  {
    if(static_initializer)return true;
    else{
      holder_arg& a=holder_type::get();
      static_factory_ptr=&a.factory;
      static_mutex_ptr=&a.mutex;
      static_initializer=(static_factory_ptr!=0);
      return static_initializer;
    }
  }

  /* insert overloads*/

#define BOOST_FLYWEIGHT_PERFECT_FWD_INSERT_BODY(args)         \
{                                                             \
  return insert_rep(rep_type(BOOST_FLYWEIGHT_FORWARD(args))); \
}

  BOOST_FLYWEIGHT_PERFECT_FWD(
    static handle_type insert,
    BOOST_FLYWEIGHT_PERFECT_FWD_INSERT_BODY)

#undef BOOST_FLYWEIGHT_PERFECT_FWD_INSERT_BODY

  static handle_type insert(const value_type& x){return insert_value(x);}
  static handle_type insert(value_type& x){return insert_value(x);}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  static handle_type insert(const value_type&& x){return insert_value(x);}
  static handle_type insert(value_type&& x){return insert_value(std::move(x));}
#endif

  static const entry_type& entry(const base_handle_type& h)
  {
    return factory().entry(h);
  }

  static const value_type& value(const handle_type& h)
  {
    return static_cast<const rep_type&>(entry(h));
  }

  static const key_type& key(const handle_type& h)
  BOOST_NOEXCEPT_IF(noexcept(
    static_cast<const rep_type&>(boost::declval<const entry_type&>())))
  {
    return static_cast<const rep_type&>(entry(h));
  }

  static factory_type& factory()
  {
    return *static_factory_ptr;
  }

  static mutex_type& mutex()
  {
    return *static_mutex_ptr;
  }

private:
  struct                              holder_arg
  {
    factory_type factory;
    mutex_type   mutex;
  };
  typedef typename mpl::apply1<
    HolderSpecifier,
    holder_arg
  >::type                             holder_type;

  /* [key|copy|move]_construct_value: poor-man's pre-C++11 lambdas */

  struct key_construct_value
  {
    void operator()(const entry_type& e)const
    {
      ValuePolicy::key_construct_value(static_cast<const rep_type&>(e));
    }
  };

  struct copy_construct_value
  {
    copy_construct_value(const value_type& x_):x(x_){}

    void operator()(const entry_type& e)const
    {
      ValuePolicy::copy_construct_value(static_cast<const rep_type&>(e),x);
    }

    const value_type& x;
  };

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  struct move_construct_value
  {
    move_construct_value(value_type&& x_):x(x_){}

    void operator()(const entry_type& e)const
    {
      ValuePolicy::move_construct_value(
        static_cast<const rep_type&>(e),std::move(x));
    }

    value_type& x;
  };
#endif

  static handle_type insert_rep(const rep_type& x)
  {
    init();
    entry_type       e(x);
    lock_type        lock(mutex());(void)lock;

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    return static_cast<handle_type>(
      insert_and_visit(factory(),std::move(e),key_construct_value()));
#else
    return static_cast<handle_type>(
      insert_and_visit(factory(),e,key_construct_value()));
#endif
  }

  static handle_type insert_value(const value_type& x)
  {
    init();
    entry_type       e((rep_type(x)));
    lock_type        lock(mutex());(void)lock;

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    return static_cast<handle_type>(
      insert_and_visit(factory(),std::move(e),copy_construct_value(x)));
#else
    return static_cast<handle_type>(
      insert_and_visit(factory(),e,copy_construct_value(x)));
#endif
  }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
  static handle_type insert_rep(rep_type&& x)
  {
    init();
    entry_type       e(std::move(x));
    lock_type        lock(mutex());(void)lock;

    return static_cast<handle_type>(
      insert_and_visit(factory(),std::move(e),key_construct_value()));
  }

  static handle_type insert_value(value_type&& x)
  {
    init();
    entry_type       e(rep_type(std::move(x)));
    lock_type        lock(mutex());(void)lock;

    return static_cast<handle_type>(
      insert_and_visit(
        factory(),std::move(e),move_construct_value(std::move(x))));
  }
#endif

  static bool          static_initializer;
  static factory_type* static_factory_ptr;
  static mutex_type*   static_mutex_ptr;
};

template<
  typename ValuePolicy,typename Tag,typename TrackingPolicy,
  typename FactorySpecifier,typename LockingPolicy,typename HolderSpecifier
>
bool
flyweight_core<
  ValuePolicy,Tag,TrackingPolicy,
  FactorySpecifier,LockingPolicy,HolderSpecifier>::static_initializer=
  flyweight_core<
      ValuePolicy,Tag,TrackingPolicy,
      FactorySpecifier,LockingPolicy,HolderSpecifier>::init();

template<
  typename ValuePolicy,typename Tag,typename TrackingPolicy,
  typename FactorySpecifier,typename LockingPolicy,typename HolderSpecifier
>
typename flyweight_core<
  ValuePolicy,Tag,TrackingPolicy,
  FactorySpecifier,LockingPolicy,HolderSpecifier>::factory_type*
flyweight_core<
  ValuePolicy,Tag,TrackingPolicy,
  FactorySpecifier,LockingPolicy,HolderSpecifier>::static_factory_ptr=0;

template<
  typename ValuePolicy,typename Tag,typename TrackingPolicy,
  typename FactorySpecifier,typename LockingPolicy,typename HolderSpecifier
>
typename flyweight_core<
  ValuePolicy,Tag,TrackingPolicy,
  FactorySpecifier,LockingPolicy,HolderSpecifier>::mutex_type*
flyweight_core<
  ValuePolicy,Tag,TrackingPolicy,
  FactorySpecifier,LockingPolicy,HolderSpecifier>::static_mutex_ptr=0;

} /* namespace flyweights::detail */

} /* namespace flyweights */

} /* namespace boost */

#if BOOST_WORKAROUND(BOOST_MSVC,BOOST_TESTED_AT(1400))
#pragma warning(pop)
#endif

#endif
