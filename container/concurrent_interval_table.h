/*
 * concurrent_interval_table.h
 *  Created on: 2013-1-22
 *      Author: qianqians
 * concurrent_interval_table
 */
#ifndef _CONCURRENT_INTERVAL_TABLE_H
#define _CONCURRENT_INTERVAL_TABLE_H

#include <string>
#include <map>
#include <boost/atomic.hpp>
#include <boost/pool/pool_alloc.hpp>

namespace angelica{
namespace container{

#define mask 1023
#define rehashmask 8191

#define upgradlock 65536 

template <typename K, typename V, typename _Allocator = boost::pool_allocator<V> >
class concurrent_interval_table {
private:
	struct bucket {
		boost::atomic<void * > _hash_bucket;
		boost::atomic_int _rw_flag; // -2 _bucket, -1 write, 0-N read, M upgrad
	};

	struct node {
		V value;
		boost::atomic_int _rw_flag; //  -2 _delete, -1 write, 0-N read, M upgrad
	};

	typedef typename _Allocator::template rebind<std::pair<K, node> >::other _map_node_alloc;
	
	typedef std::pair<K, node*> value_type;
	typedef std::map<K, node*, std::less<K>, _map_node_alloc> _map;
	
	typedef typename _Allocator::template rebind<node>::other _node_alloc_;
	typedef typename _Allocator::template rebind<_map>::other _map_alloc_;
	typedef typename _Allocator::template rebind<bucket>::other _bucket_alloc_;
	
public:
	concurrent_interval_table(){
		for(int i = 0; i < mask; i++){
			_hash_array[i]._hash_bucket.store(0);
		}
	}

	~concurrent_interval_table(){
		for(unsigned int i = 0; i < mask; i++){
			if (_hash_array[i]._rw_flag.load() == -2){
				bucket * _bucket = (bucket *)_hash_array[i]._hash_bucket.load();
				for(unsigned int j = 0; j < rehashmask; j++){
					put_map((_map *)_bucket[j]._hash_bucket.load());
				}
				put_bucket(_bucket, rehashmask);
			}else{
				put_map((_map *)_hash_array[i]._hash_bucket.load());
			}
		}
	}
	
	void insert(K key, V value){
		unsigned int hash_value = hash(key, mask);
		bucket * _bucket = (bucket *)&_hash_array[hash_value];
		while(1){
			if (_bucket->_rw_flag.load() == -2){
				hash_value = hash(key, rehashmask);
				_bucket = (bucket *)_bucket->_hash_bucket.load();
				_bucket = (bucket *)&_bucket[hash_value];
			}

			if (!upgrad_lock(_bucket->_rw_flag)){
				continue;
			}

			_map * _map_ = (_map *)_bucket->_hash_bucket.load();
			if (_map_ == 0){
				_map_ = get_map();
				_bucket->_hash_bucket.store(_map_);
			}
			_map::iterator iter = _map_->find(key);
			if (iter != _map_->end()){
				lock_unique(iter->second->_rw_flag);
				iter->second->value = value;
				unlock_unique(iter->second->_rw_flag);
				unlock_upgrad(_bucket->_rw_flag);
			}else{
				if (!unlock_upgrad_and_lock(_bucket->_rw_flag)){
					continue;
				}

				_map_ = (_map *)_bucket->_hash_bucket.load();
				_map::iterator iter = _map_->find(key);
				if (iter != _map_->end()){
					lock_unique(iter->second->_rw_flag);
					iter->second->value = value;
					unlock_unique(iter->second->_rw_flag);
				}else{
					node * _node = get_node();
					_node->value = value;
					_node->_rw_flag.store(0);
					_map_->insert(std::make_pair(key, _node));
				}

				unlock_unique(_bucket->_rw_flag);
			}

			break;
		}
	}

	bool search(K key, V &value){
		unsigned int hash_value = hash(key, mask);
		bucket * _bucket = (bucket *)&_hash_array[hash_value];
		if (_bucket->_rw_flag.load() == -2){
			hash_value = hash(key, rehashmask);
			_bucket = (bucket *)_bucket->_hash_bucket.load();
			_bucket = (bucket *)&_bucket[hash_value];
		}

		shared_lock(_bucket->_rw_flag);
		
		_map * _map_ = (_map *)_bucket->_hash_bucket.load();
		_map::iterator iter = _map_->find(key);
		if (iter == _map_->end()){
			return false;
		}
		
		if (!shared_lock(iter->second->_rw_flag)){
			return false;
		}
		value = iter->second->value;
		unlock_shared(iter->second->_rw_flag);

		unlock_shared(_bucket->_rw_flag);

		return true;
	}

	bool erase(K key, V value){
		unsigned int hash_value = hash(key, mask);
		bucket * _bucket = (bucket *)&_hash_array[hash_key];
		if (_bucket->_rw_flag.load() == -2){
			hash_value = hash(key, rehashmask);
			_bucket = _bucket->_hash_bucket.load();
			_bucket = (bucket *)&_bucket[hash_value];
		}

		shared_lock(_bucket->_rw_flag);
		
		_map * _map_ = (_map *)_bucket->_hash_bucket.load();
		_map::iterator iter = _map_->find(key);
		if (iter == _map_->end()){
			return false;
		}
		
		if (iter->second._rw_flag){
			if (!lock_unique(iter->second._rw_flag)){
				return false;
			}
			iter->second._rw_flag.store(-2);
		}

		unlock_shared(_bucket->_rw_flag);

		return true;
	}

	unsigned int size(){
		return _size.load();
	}

private:
	bool shared_lock(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();
			
			if (_old_flag == -2){
				return false;
			}

			if (_old_flag < 0 || _old_flag == (upgradlock-1)){
				continue;
			}

			if (_rw_flag.compare_exchange_weak(_old_flag, _old_flag+1)){
				break;
			}
		}

		return true;
	}

	void unlock_shared(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();
			
			if (_old_flag < 0){
				continue;
			}

			if ((_old_flag & 0x0000ffff) == 0){
				break;
			}

			if (_rw_flag.compare_exchange_weak(_old_flag, _old_flag-1)){
				break;
			}
		}
	}

	bool upgrad_lock(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();

			if (_old_flag == -2){
				return false;
			}

			if (_old_flag < 0 || (_old_flag+upgradlock) < 0){
				continue;
			}

			if (_rw_flag.compare_exchange_weak(_old_flag, _old_flag+upgradlock)){
				break;
			}
		}

		return true;
	}

	void unlock_upgrad(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();
			
			if (_old_flag < 0){
				continue;
			}

			if ((_old_flag & 0xffff0000) == 0){
				break;
			}

			if (_rw_flag.compare_exchange_weak(_old_flag, _old_flag-upgradlock)){
				break;
			}
		}
	}

	bool lock_unique(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();
			
			if (_old_flag == -2){
				return false;
			}

			if (_old_flag != 0){
				continue;
			}

			if (_rw_flag.compare_exchange_weak(_old_flag, -1)){
				break;
			}
		}

		return true;
	}

	void unlock_unique(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();
			
			if (_old_flag != -1){
				break;
			}

			if (_rw_flag.compare_exchange_weak(_old_flag, 0)){
				break;
			}
		}
	}

	bool unlock_upgrad_and_lock(boost::atomic_int & _rw_flag){
		while(1){
			int _old_flag = _rw_flag.load();
			
			if (_old_flag == -2){
				return false;
			}

			if ((_old_flag & 0x0000ffff) > 0){
				continue;
			}

			if (_old_flag > 0 && (_old_flag & 0xffff0000) > 0){
				if (_rw_flag.compare_exchange_weak(_old_flag, (0-_old_flag))){
					continue;
				}
			}

			if (_old_flag != 0){
				if (_rw_flag.compare_exchange_weak(_old_flag, _old_flag+upgradlock)){
					break;
				}
			}
		}

		return lock_unique(_rw_flag);
	}

	unsigned int hash(char * skey, unsigned int mod){
		unsigned int hash = 5831;
		unsigned int slen = strlen(skey);
		for(unsigned int i = 0; i < slen; i++){
			hash += hash<<5 + hash + *skey++;
		}

		return hash%mod;
	}

	unsigned int hash(std::string & strkey, unsigned int mod){
		unsigned int hash = 5831;
		for(unsigned int i = 0; i < strkey.size(); i++){
			hash += hash<<5 + hash + (unsigned int)strkey.at(i);
		}

		return hash%mod;
	}

	unsigned int hash(wchar_t * wskey, unsigned int mod){
		unsigned int hash = 5831;
		unsigned int slen = wcslen(wskey);
		for(unsigned int i = 0; i < slen; i++){
			hash += hash<<5 + hash + (unsigned int)*wskey++;
		}

		return hash%mod;
	}

	unsigned int hash(std::wstring & wstrkey, unsigned int mod){
		unsigned int hash = 5831;
		for(unsigned int i = 0; i < wstrkey.size(); i++){
			hash += hash<<5 + hash + (unsigned int)wstrkey.at(i);
		}

		return hash%mod;
	}

	unsigned int hash(int32_t key, int mod){
		key += ~(key<<15);
		key ^= key>>10;
		key += (key<<3);
		key ^= key>>6;
		key += ~(key<<11);
		key ^= key>>16;

		return key%mod;
	}

	unsigned int hash(uint32_t key, int mod){
		key += ~(key<<15);
		key ^= key>>10;
		key += (key<<3);
		key ^= key>>6;
		key += ~(key<<11);
		key ^= key>>16;

		return key%mod;
	}

	unsigned int hash(int64_t key, int mod){
		return key%mod;
	}

	unsigned int hash(uint64_t key, int mod){
		return key%mod;
	}

	template <typename KEY>
	unsigned int hash(KEY key, int mod){
		return key.hash()%mod;
	}
	
	node * get_node(){
		node * _node = _node_alloc.allocate(1);
		::new (_node) node();
		//_node_alloc.construct(_node);

		return _node;
	}

	void put_node(node * _node_){
		_node_alloc.destroy(_node_);
		_node_alloc.deallocate(_node_, 1);
	}

	_map * get_map(){
		_map * _map_ = _map_alloc.allocate(1);
		::new (_map_) _map();
		//_map_alloc.construct(_map_);

		return _map_;
	}

	void put_map(_map * _map_){
		_map_alloc.destroy(_map_);
		_map_alloc.deallocate(_map_, 1);
	}

	bucket * get_bucket(unsigned int count){
		bucket * _bucket = _bucket_alloc.allocate(count);
		bucket * _tmpbucket = _bucket;
		for (int i = 0; i < count; i++)
		{
			::new (_tmpbucket++) bucket();
			//_bucket_alloc.construct(_tmpbucket++);
		}

		return _bucket;
	}

	void put_bucket(bucket * _bucket, unsigned int count){
		bucket * _tmpbucket = _bucket;
		for (unsigned int i = 0; i < count; i++)
		{
			_bucket_alloc.destroy(_tmpbucket++);
		}
		_bucket_alloc.deallocate(_bucket, count);
	}

private:
	bucket _hash_array[mask];
	boost::atomic_uint _size;

	_node_alloc_ _node_alloc;
	_map_alloc_ _map_alloc;
	_bucket_alloc_ _bucket_alloc;

};	
	
} //container
} //angelica

#endif //_CONCURRENT_INTERVAL_TABLE_H