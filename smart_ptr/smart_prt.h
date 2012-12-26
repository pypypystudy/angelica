/*
 * smart_prt.hpp
 * Created on: 2012-8-30
 *	   Author: qianqians
 *  smart_prt: a smart_ptr based on Mark-Sweep
 */

#ifndef _SMART_PRT_H
#define _SMART_PRT_H

#include <angelica/smart_ptr/detail/_mark_sweep_sys.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace angelica{
namespace smart_ptr{

template <class T>
class smart_prt {
private:
	typedef smart_prt<T> this_type;
	// deallocate function
	typedef boost::function<void(typename T *)> fn_dealloc;
	// _mark_sweep_sys
	typedef angelica::smart_ptr::detail::_mark_sweep_sys<typename T> _mark_sweep_sys_;
	// _mark_list_node
	typedef angelica::smart_ptr::detail::_list_node_<typename T> _list_node;

public:
	template<class Y> 
	explicit smart_prt(Y * p,  fn_dealloc fn = boost::bind(_put_data, _1)) 
		: data(static_cast<T *>(p)){
		_root->mount(data, fn);
		_node = _root->mark(data);
	}

	template<class Y>
	explicit smart_prt(smart_prt<Y> const & r) : data(static_cast<T *>(r.data)) {
		_node = _root->mark(data);
	}

	~smart_prt() {
		_root->umark(_node);
	}

public:
    template<class Y> smart_prt & operator=(Y * p) {
        this_type(p).swap( *this );
        return *this;
    }

	template <class Y>
	smart_prt & operator=(smart_prt<Y> const & r) {
		this_type(r).swap(*this);
		return *this;
	}

	T & operator* () const {
        BOOST_ASSERT(data != 0);
        return *data;
    }

    T * operator-> () const {
        BOOST_ASSERT(data != 0);
        return data;
    }

	T * get() const {
		return data;
	}

	void swap(smart_prt<T> & other) {
		std::swap(data, other.data);
	}

	 template<class Y> bool owner_before( smart_prt<Y> const & rhs ) const
    {
        return data < rhs.data;
    }

private:
	static void _put_data(T * ptr) {
		delete ptr;
	}

private:
	T * data;
	static _mark_sweep_sys_ * _root;

	_list_node _node;

};

template<typename T> angelica::smart_ptr::detail::_mark_sweep_sys<typename T> * smart_prt<typename T>::_root = 
	new angelica::smart_ptr::detail::_mark_sweep_sys<typename T>();

template<class T, class U> inline bool operator==(smart_prt<T> const & a, smart_prt<U> const & b){
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(smart_prt<T> const & a, smart_prt<U> const & b){
    return a.get() != b.get();
}

template<class T, class U> inline bool operator<(smart_prt<T> const & a, smart_prt<U> const & b){
    return a.owner_before( b );
}

}//angelica
}//smart_ptr
#endif //_SMART_PRT_H