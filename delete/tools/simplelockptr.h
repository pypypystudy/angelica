/*
 * simplelockptr.h
 *
 *  Created on: 2012-5-14
 *      Author: Administrator
 */

#ifndef SIMPLELOCKPTR_H_
#define SIMPLELOCKPTR_H_

#include <cstddef>
#include <atomic>

namespace angelica {
namespace tools {

template<typename T>
class simple_lock_ptr {
public:
	simple_lock_ptr(T *ptr_T) : ptr(ptr_T) {
		 ptr_ref_count = new atomic_long(0);
	};

	simple_lock_ptr(simple_lock_ptr const &r) {
		*(r.ptr_ref_count)++;

		ptr = r.ptr;
		ptr_ref_count = r.ptr_ref_count;
	};

template<typename U>
	simple_lock_ptr(simple_lock_ptr<U> const &r) {
		*(r.ptr_ref_count)++;

		ptr = static_cast<T*>(r.ptr);
		ptr_ref_count = r.ptr_ref_count;
	};

	virtual ~simple_lock_ptr() {
		*(ptr_ref_count)--;
	};

	simple_lock_ptr & operator=(simple_lock_ptr const & r) {
		*(r.ptr_ref_count)++;

		ptr = r.ptr;
		ptr_ref_count = r.ptr_ref_count;
	}

template<typename U>
	simple_lock_ptr & operator=(simple_lock_ptr<U> const &r) {
		*(r.ptr_ref_count)++;

		ptr = static_cast<T*>(r.ptr);
		ptr_ref_count = r.ptr_ref_count;
	}

	T *get() {
		return ptr;
	};

private:
	T *ptr;

	std::atomic_long *ptr_ref_count;

};

} /* namespace tools */
} /* namespace angelica */
#endif /* SIMPLELOCKPTR_H_ */
