#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <iostream>
#include <mutex>
using namespace std;

namespace cs540
{	
	static std::mutex mtx; 
	
	template<class T>
	void del(void* ptr) 
	{
		delete static_cast<T*>(ptr);
	}
	
	class RC
	{
		private:
			int m_iRefCount = 0;
			void* ptr;
			void (*deleter)(void*);	//function pointer which will point to release Helper::release function
		public:
			template<class U>
			void aquaire(U* ptr) 
			{
				this->ptr = static_cast<void*>(ptr);
				deleter = &del<U>; 
			}
			void addRef(){ ++m_iRefCount; }
			int decRef() 
			{
				return --this->m_iRefCount ? m_iRefCount : 0;
			}
			void release()
		    	{
		    		(*deleter)(this->ptr);
		    		this->ptr = nullptr;
			}
	};

	template<typename T>
	class SharedPtr
	{
		private:
			T* m_tObj;
			RC* m_rc;
		public:
			
			//points to null object
			SharedPtr() : m_tObj(nullptr), m_rc(nullptr) {}
			
			template<typename U> explicit SharedPtr(U* pObj) 
			{
				mtx.lock();
				m_tObj = pObj;
				m_rc = new RC();
				m_rc->aquaire(pObj);
				m_rc->addRef();
				mtx.unlock();
			}
			
			template<typename U> SharedPtr(U* pObj, RC* rc) : m_tObj(pObj), m_rc(rc)
			{
				
			}
			
			SharedPtr(const SharedPtr& p)
			{
				m_tObj = p.get();
				m_rc = p.getRef();
				mtx.lock();
				if(m_tObj != nullptr)
					m_rc->addRef();
				mtx.unlock();
			}
			
			template <typename U> SharedPtr(const SharedPtr<U> &p)
			{
				m_tObj = p.get();
				m_rc = p.getRef();
				mtx.lock();
				
				if(m_tObj != nullptr)
					m_rc->addRef();
				mtx.unlock();
			}
			
			SharedPtr(SharedPtr&& p)
			{
				mtx.lock();
				//cout << "4" << endl;
				if(p.get() != nullptr)
				{
				 	m_tObj = std::move(p.get());
				 	m_rc = std::move(p.getRef());
					p.clear();
				}
				else p.clear();
				mtx.unlock();
			}
			
			template <typename U> SharedPtr(SharedPtr<U> &&p)
			{
				mtx.lock();
				//cout << "5" << endl;
				if(p.get() != nullptr)
				{
					m_tObj = std::move(p.get());
				 	m_rc = std::move(p.getRef());
					p.clear();
				}
				else p.clear();
				mtx.unlock();
			}
			
			SharedPtr& operator=(const SharedPtr &p)
			{
				mtx.lock();
				if(m_tObj != p.get())
				{
					if(m_tObj != nullptr)
					{
						if(m_rc != nullptr && m_rc->decRef() == 0)
						{
							m_rc->release();
							delete m_rc;
							
						}
						m_rc = nullptr;
						m_tObj = nullptr;
					}
					if(p.get() != nullptr)
					{
						m_tObj = p.get();
						m_rc = p.getRef();
						m_rc->addRef();
					}
				}
				mtx.unlock();
				return *this;
			}
			
			template <typename U> SharedPtr<T>& operator=(const SharedPtr<U> &p)
			{
				mtx.lock();
				if(m_tObj != p.get())
				{
					if(m_tObj != nullptr)
					{
						if(m_rc->decRef() == 0)
						{
							m_rc->release();
							delete m_rc;
							
						}
						m_rc = nullptr;
						m_tObj = nullptr;
					}
					if(p.get() != nullptr)
					{
						m_tObj = p.get();
						m_rc = p.getRef();
						m_rc->addRef();
					}
				}
				mtx.unlock();
				return *this;
			}
			
			SharedPtr &operator=(SharedPtr &&p)
			{
				mtx.lock();
				if(p.get() != nullptr && m_tObj != p.get())
				{
					m_tObj = std::move(p.get());
					m_rc = std::move(p.getRef());
				}
				mtx.unlock();
			} 
			
			template <typename U> SharedPtr &operator=(SharedPtr<U> &&p)
			{
				mtx.lock();
				if(m_tObj != p.get())
				{
					if(p.get() != nullptr)
					{
						m_tObj = std::move(p.get());
						m_rc = std::move(p.getRef());
						p.clear();
					}
				}
				mtx.unlock();
			}
			
			void release(void)
			{
				mtx.lock();
				//cout << "10" << endl;
				if(m_tObj != nullptr)
				{
					if(m_rc != nullptr && m_rc->decRef() == 0)
					{
						m_rc->release();
						delete m_rc;
						m_rc = nullptr;
					}
				}
				mtx.unlock();
			}
			
			//Decrements reference count
			//if reference count is 0 then delete object pointed to by m_tObj
			~SharedPtr(void)
			{
				release();
			}
			
			void reset(void)
			{
				mtx.lock();
				//cout << "11" << endl;
				if(m_rc != nullptr && m_rc->decRef() == 0)
				{
					m_rc->release();
					delete m_rc;
					m_rc = nullptr;
					m_tObj = nullptr;
				}
				m_tObj = nullptr;
				m_rc = nullptr;
				mtx.unlock();
			}
			
			template <typename U> void reset(U* p)
			{
				mtx.lock();
				//cout << "12" << endl;
				if(m_rc != nullptr && m_rc->decRef() == 0)
				{
					m_rc->release();
					delete m_rc;
					m_rc = nullptr;
					m_tObj = nullptr;
				}
				m_tObj = p;
				m_rc = new RC();
				m_rc->aquaire(p);
				m_rc->addRef();
				mtx.unlock();
			}
			
			T* get(void) const { return this->m_tObj; }
			
			void clear(void) { this->m_tObj = nullptr; this->m_rc = nullptr; }
			
			RC* getRef(void) const { return this->m_rc; }
			
			T& operator*(void)const { return *(this->m_tObj); }
			
			T* operator->(void) const { return this->m_tObj; }
			
			explicit operator bool(void) const { return ((m_tObj != nullptr)?true : false); }
			
			///////////////////////////////////// Friends //////////////////////////////////////////////////////////////
			template <typename T1, typename T2> friend bool operator==(const SharedPtr<T1>&, const SharedPtr<T2>&);
			
			template <typename T1> friend bool operator==(const SharedPtr<T1> &, std::nullptr_t);
			
			template <typename T1> friend bool operator==(std::nullptr_t, const SharedPtr<T1> &);
			
			template <typename T1, typename T2> friend bool operator!=(const SharedPtr<T1>&, const SharedPtr<T2> &);
			
			template <typename T1> friend bool operator!=(const SharedPtr<T1> &, std::nullptr_t);
			
			template <typename T1> friend bool operator!=(std::nullptr_t, const SharedPtr<T1> &);
			
			template <typename T1, typename U> friend SharedPtr<T1> static_pointer_cast(const SharedPtr<U> &sp);
			
			template <typename T1, typename U> friend SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<U> &sp);
	};
	
	template <typename T1, typename T2> bool operator==(const SharedPtr<T1>& sp1, const SharedPtr<T2>& sp2)
	{
		return (sp1.get() == sp2.get());
	}
	
	template <typename T1> bool operator==(const SharedPtr<T1> &sp, std::nullptr_t)
	{
		return (sp.get() == nullptr);
	}
	
	template <typename T1> bool operator==(std::nullptr_t, const SharedPtr<T1>& sp)
	{
		return nullptr == sp.get();
	}
			
	template <typename T1, typename T2> bool operator!=(const SharedPtr<T1>& sp1, const SharedPtr<T2>& sp2)
	{
		return (sp1.get() != sp2.get());
	}
	
	template <typename T1> bool operator!=(const SharedPtr<T1>& sp, std::nullptr_t)
	{
		return (sp.get() != nullptr);
	}
	
	template <typename T1> bool operator!=(std::nullptr_t, const SharedPtr<T1>& sp)
	{
		return (nullptr != sp.get());
	}
	
	template <typename T, typename U> SharedPtr<T> static_pointer_cast(const SharedPtr<U>& sp)
	{
		if(static_cast<T*>(sp.get()) == nullptr)
			return SharedPtr<T>();
		return SharedPtr<T>(static_cast<T*>(sp.get()), sp.getRef());
		//SharedPtr<T>::p_cast = false;
	}
	
	template <typename T, typename U> SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp)
	{
		 if(dynamic_cast<T*>(sp.get()) == nullptr)
			return SharedPtr<T>();
		return SharedPtr<T>(dynamic_cast<T*>(sp.get()), sp.getRef());
	}
}

#endif
