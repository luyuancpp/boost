#ifndef BOOST_SMART_PTR_WEAK_PTR_HPP_INCLUDED
#define BOOST_SMART_PTR_WEAK_PTR_HPP_INCLUDED

//
//  weak_ptr.hpp
//
//  Copyright (c) 2001, 2002, 2003 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.
//

#include <boost/smart_ptr/detail/shared_count.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <memory>
#include <cstddef>

namespace boost
{

template<class T> class weak_ptr
{
private:

    // Borland 5.5.1 specific workarounds
    typedef weak_ptr<T> this_type;

public:

    typedef typename boost::detail::sp_element< T >::type element_type;

    constexpr weak_ptr() noexcept : px(0), pn()
    {
    }

//  generated copy constructor, assignment, destructor are fine...
// ... except in C++0x, move disables the implicit copy

    weak_ptr( weak_ptr const & r ) noexcept : px( r.px ), pn( r.pn )
    {
    }

    weak_ptr & operator=( weak_ptr const & r ) noexcept
    {
        px = r.px;
        pn = r.pn;
        return *this;
    }

//
//  The "obvious" converting constructor implementation:
//
//  template<class Y>
//  weak_ptr(weak_ptr<Y> const & r): px(r.px), pn(r.pn)
//  {
//  }
//
//  has a serious problem.
//
//  r.px may already have been invalidated. The px(r.px)
//  conversion may require access to *r.px (virtual inheritance).
//
//  It is not possible to avoid spurious access violations since
//  in multithreaded programs r.px may be invalidated at any point.
//

    template<class Y>
    weak_ptr( weak_ptr<Y> const & r, typename boost::detail::sp_enable_if_convertible<Y,T>::type = boost::detail::sp_empty() )
    noexcept : px(r.lock().get()), pn(r.pn)
    {
        boost::detail::sp_assert_convertible< Y, T >();
    }

    template<class Y>
    weak_ptr( weak_ptr<Y> && r, typename boost::detail::sp_enable_if_convertible<Y,T>::type = boost::detail::sp_empty() )
    noexcept : px( r.lock().get() ), pn( static_cast< boost::detail::weak_count && >( r.pn ) )
    {
        boost::detail::sp_assert_convertible< Y, T >();
        r.px = 0;
    }

    // for better efficiency in the T == Y case
    weak_ptr( weak_ptr && r )
    noexcept : px( r.px ), pn( static_cast< boost::detail::weak_count && >( r.pn ) )
    {
        r.px = 0;
    }

    // for better efficiency in the T == Y case
    weak_ptr & operator=( weak_ptr && r ) noexcept
    {
        this_type( static_cast< weak_ptr && >( r ) ).swap( *this );
        return *this;
    }


    template<class Y>
    weak_ptr( shared_ptr<Y> const & r, typename boost::detail::sp_enable_if_convertible<Y,T>::type = boost::detail::sp_empty() )
    noexcept : px( r.px ), pn( r.pn )
    {
        boost::detail::sp_assert_convertible< Y, T >();
    }

    // aliasing
    template<class Y> weak_ptr(shared_ptr<Y> const & r, element_type * p) noexcept: px( p ), pn( r.pn )
    {
    }

    template<class Y> weak_ptr(weak_ptr<Y> const & r, element_type * p) noexcept: px( p ), pn( r.pn )
    {
    }

    template<class Y> weak_ptr(weak_ptr<Y> && r, element_type * p) noexcept: px( p ), pn( std::move( r.pn ) )
    {
    }

    template<class Y>
    weak_ptr & operator=( weak_ptr<Y> const & r ) noexcept
    {
        boost::detail::sp_assert_convertible< Y, T >();

        px = r.lock().get();
        pn = r.pn;

        return *this;
    }

    template<class Y>
    weak_ptr & operator=( weak_ptr<Y> && r ) noexcept
    {
        this_type( static_cast< weak_ptr<Y> && >( r ) ).swap( *this );
        return *this;
    }

    template<class Y>
    weak_ptr & operator=( shared_ptr<Y> const & r ) noexcept
    {
        boost::detail::sp_assert_convertible< Y, T >();

        px = r.px;
        pn = r.pn;

        return *this;
    }

    shared_ptr<T> lock() const noexcept
    {
        return shared_ptr<T>( *this, boost::detail::sp_nothrow_tag() );
    }

    long use_count() const noexcept
    {
        return pn.use_count();
    }

    bool expired() const noexcept
    {
        return pn.use_count() == 0;
    }

    bool _empty() const noexcept // extension, not in std::weak_ptr
    {
        return pn.empty();
    }

    bool empty() const noexcept // extension, not in std::weak_ptr
    {
        return pn.empty();
    }

    void reset() noexcept
    {
        this_type().swap(*this);
    }

    void swap(this_type & other) noexcept
    {
        std::swap(px, other.px);
        pn.swap(other.pn);
    }

    template<class Y> bool owner_before( weak_ptr<Y> const & rhs ) const noexcept
    {
        return pn < rhs.pn;
    }

    template<class Y> bool owner_before( shared_ptr<Y> const & rhs ) const noexcept
    {
        return pn < rhs.pn;
    }

    template<class Y> bool owner_equals( weak_ptr<Y> const & rhs ) const noexcept
    {
        return pn == rhs.pn;
    }

    template<class Y> bool owner_equals( shared_ptr<Y> const & rhs ) const noexcept
    {
        return pn == rhs.pn;
    }

    std::size_t owner_hash_value() const noexcept
    {
        return pn.hash_value();
    }

private:

    template<class Y> friend class weak_ptr;
    template<class Y> friend class shared_ptr;

    element_type * px;            // contained pointer
    boost::detail::weak_count pn; // reference counter

};  // weak_ptr

template<class T, class U> inline bool operator<(weak_ptr<T> const & a, weak_ptr<U> const & b) noexcept
{
    return a.owner_before( b );
}

template<class T> void swap(weak_ptr<T> & a, weak_ptr<T> & b) noexcept
{
    a.swap(b);
}

#if defined(__cpp_deduction_guides)

template<class T> weak_ptr( shared_ptr<T> ) -> weak_ptr<T>;

#endif

// hash_value

template< class T > std::size_t hash_value( boost::weak_ptr<T> const & p ) noexcept
{
    return p.owner_hash_value();
}

} // namespace boost

// std::hash, std::equal_to

namespace std
{

template<class T> struct hash< ::boost::weak_ptr<T> >
{
    std::size_t operator()( ::boost::weak_ptr<T> const & p ) const noexcept
    {
        return p.owner_hash_value();
    }
};

template<class T> struct equal_to< ::boost::weak_ptr<T> >
{
    bool operator()( ::boost::weak_ptr<T> const & a, ::boost::weak_ptr<T> const & b ) const noexcept
    {
        return a.owner_equals( b );
    }
};

} // namespace std

#endif  // #ifndef BOOST_SMART_PTR_WEAK_PTR_HPP_INCLUDED
