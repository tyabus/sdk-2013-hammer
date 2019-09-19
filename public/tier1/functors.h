//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a generic infrastucture for functors combining
//			a number of techniques to provide transparent parameter type
//			deduction and packaging. Supports both member and non-member functions.
//
//			See also: http://en.wikipedia.org/wiki/Function_object
//
//			Note that getting into the guts of this file is not for the
//			feint of heart. The core concept here is that the compiler can
//			figure out all the parameter types.
//
//			E.g.:
//
//			struct CMyClass
//			{
//				void foo( int i) {}
//			};
//
//			int bar(int i) { return i; }
//
//			CMyClass myInstance;
//
//			CFunctor *pFunctor = CreateFunctor( &myInstance, CMyClass::foo, 8675 );
//			CFunctor *pFunctor2 = CreateFunctor( &bar, 309 );
//
//			void CallEm()
//			{
//				(*pFunctor)();
//				(*pFunctor2)();
//			}
//
//=============================================================================

#ifndef FUNCTORS_H
#define FUNCTORS_H

#include "tier0/platform.h"
#include "tier1/refcount.h"
#include "tier1/utlenvelope.h"
#include "tier1/utlcommon.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
//
// Purpose: Base class of all function objects
//
//-----------------------------------------------------------------------------

abstract_class CFunctor : public IRefCounted
{
public:
	CFunctor()
	{
#ifdef DEBUG
		m_nUserID = 0;
#endif
	}
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
	virtual ~CFunctor() {}

	virtual void operator()() = 0;

	unsigned m_nUserID; // For debugging
};


//-----------------------------------------------------------------------------
// When calling through a functor, care needs to be taken to not pass objects that might go away. 
// Since this code determines the type to store in the functor based on the actual arguments,
// this is achieved by changing the point of call. 
//
// See also CUtlEnvelope
//-----------------------------------------------------------------------------

// convert a reference to a passable value
template <typename T>
inline T RefToVal( const T& item )
{
   return item;
}

//-----------------------------------------------------------------------------
// This class can be used to pass into a functor a proxy object for a pointer
// to be resolved later. For example, you can execute a "call" on a resource
// whose actual value is not known until a later time
//-----------------------------------------------------------------------------

template <typename T>
class CLateBoundPtr
{
public:
	CLateBoundPtr( T** ppObject ) : m_ppObject( ppObject ) {}

	T* operator->() { return *m_ppObject; }
	T& operator *() { return **m_ppObject; }
	operator T*() const { return (T*)( *m_ppObject ); }
	operator void* () { return *m_ppObject; }

private:
	T **m_ppObject;
};

//-----------------------------------------------------------------------------
//
// Purpose: Classes to define memory management policies when operating
//			on pointers to members
//
//-----------------------------------------------------------------------------

class CFuncMemPolicyNone
{
public:
	static void OnAcquire( void* pObject ) {}
	static void OnRelease( void* pObject ) {}
};

template <class OBJECT_TYPE_PTR = IRefCounted*>
class CFuncMemPolicyRefCount
{
public:
	static void OnAcquire( OBJECT_TYPE_PTR pObject ) { pObject->AddRef(); }
	static void OnRelease( OBJECT_TYPE_PTR pObject ) { pObject->Release(); }
};

//-----------------------------------------------------------------------------
//
// The actual functor implementation
//
//-----------------------------------------------------------------------------

#include "tier0/memdbgon.h"
#include "tier0/valve_minmax_off.h"
#include <tuple>
#include "tier0/valve_minmax_on.h"

typedef CRefCounted1<CFunctor, CRefCountServiceMT> CFunctorBase;

template <typename FUNC_TYPE, class FUNCTOR_BASE = CFunctorBase, typename... Args>
class CFunctorI : public FUNCTOR_BASE
{
public:
	CFunctorI( FUNC_TYPE pfnProxied, Args&&... args ) : m_pfnProxied( pfnProxied ), m_params( std::forward<Args>( args )... ) {}
	~CFunctorI() {}
	void operator()() override { apply(); }

private:
	template<size_t... S>
	FORCEINLINE void apply( std::index_sequence<S...> )
	{
		m_pfnProxied( std::get<S>( m_params )... );
	}

	FORCEINLINE void apply()
	{
		apply( std::make_index_sequence<sizeof...( Args )>() );
	}
	
	FUNC_TYPE m_pfnProxied;
	std::tuple<std::decay_t<Args>...> m_params;
};

template <class OBJECT_TYPE_PTR, typename FUNCTION_TYPE, class FUNCTOR_BASE = CFunctorBase, class MEM_POLICY = CFuncMemPolicyNone, typename... Args>
class CMemberFunctorI : public FUNCTOR_BASE
{
public:
	CMemberFunctorI( OBJECT_TYPE_PTR pObject, FUNCTION_TYPE pfnProxied, Args&&... args ) : m_Proxy( pObject, pfnProxied ), m_params( std::forward<Args>( args )... ) {}
	~CMemberFunctorI() {}
	void operator()() override { apply(); }

private:
	template<size_t... S>
	FORCEINLINE void apply( std::index_sequence<S...> )
	{
		m_Proxy( std::get<S>( m_params )... );
	}

	FORCEINLINE void apply()
	{
		apply( std::make_index_sequence<sizeof...( Args )>() );
	}

	class CMemberFuncProxy
	{
	public:
		CMemberFuncProxy( OBJECT_TYPE_PTR pObject, FUNCTION_TYPE pfnProxied ) : m_pfnProxied( pfnProxied ), m_pObject( pObject )
		{
			MEM_POLICY::OnAcquire( m_pObject );
		}

		~CMemberFuncProxy()
		{
			MEM_POLICY::OnRelease( m_pObject );
		}

		void operator()( const Args&... args )
		{
			Assert( m_pObject != nullptr );
			( ( *m_pObject ).*m_pfnProxied )( args... );
	}

		FUNCTION_TYPE m_pfnProxied;
		OBJECT_TYPE_PTR m_pObject;
	};

	CMemberFuncProxy m_Proxy;
	std::tuple<std::decay_t<Args>...> m_params;
};

//-----------------------------------------------------------------------------
//
// The real magic, letting the compiler figure out all the right template parameters
//
//-----------------------------------------------------------------------------

template <typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto CreateFunctor( FUNCTION_RETTYPE( *pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
{
	typedef FUNCTION_RETTYPE( *Func_t )( FuncArgs... );
	return new CFunctorI<Func_t, CFunctorBase, Args...>( pfnProxied, std::forward<Args>( args )... );
}

template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto CreateFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
{
	using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... );
	return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyNone, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
}

template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto CreateFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
{
	using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... ) const;
	return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyNone, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
}

template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto CreateRefCountingFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
{
	using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... );
	return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyRefCount<OBJECT_TYPE_PTR>, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
}

template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto CreateRefCountingFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
{
	using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... ) const;
	return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyRefCount<OBJECT_TYPE_PTR>, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
}

//-----------------------------------------------------------------------------
//
// Templates to assist early-out direct call code
//
//-----------------------------------------------------------------------------

template <typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto FunctorDirectCall( FUNCTION_RETTYPE( *pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value>
{
	( *pfnProxied )( std::forward<Args>( args )... );
}

template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto FunctorDirectCall( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value>
{
	( ( *pObject ).*pfnProxied )( std::forward<Args>( args )... );
}

template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
inline auto FunctorDirectCall( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value>
{
	( ( *pObject ).*pfnProxied )( std::forward<Args>( args )... );
}

#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// Factory class useable as templated traits
//-----------------------------------------------------------------------------

class CDefaultFunctorFactory
{
public:
	template <typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateFunctor( FUNCTION_RETTYPE( *pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		typedef FUNCTION_RETTYPE( *Func_t )( FuncArgs... );
		return new CFunctorI<Func_t, CFunctorBase, Args...>( pfnProxied, std::forward<Args>( args )... );
	}

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE(FUNCTION_CLASS::*)( FuncArgs... );
		return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyNone, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
	}

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE(FUNCTION_CLASS::*)( FuncArgs... ) const;
		return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyNone, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
	}

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateRefCountingFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE(FUNCTION_CLASS::*)( FuncArgs... );
		return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyRefCount<OBJECT_TYPE_PTR>, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
	}

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateRefCountingFunctor( OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE( FUNCTION_CLASS::* pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE(FUNCTION_CLASS::*)( FuncArgs... ) const;
		return new CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CFunctorBase, CFuncMemPolicyRefCount<OBJECT_TYPE_PTR>, Args...>( pObject, pfnProxied, std::forward<Args>( args )... );
	}
};

template <class CAllocator, class CCustomFunctorBase = CFunctorBase>
class CCustomizedFunctorFactory
{
public:
	void SetAllocator( CAllocator *pAllocator )
	{
		m_pAllocator = pAllocator;
	}
	
	template <typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateFunctor( FUNCTION_RETTYPE (*pfnProxied)( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE( * )( FuncArgs... );
		using Functor_t = CFunctorI<Func_t, CCustomFunctorBase, Args...>;
		return new ( m_pAllocator->Alloc( sizeof( Functor_t ) ) ) Functor_t( pfnProxied, std::forward<Args>( args )... );
		}

	//-------------------------------------

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateFunctor(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... );
		using Functor_t = CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CCustomFunctorBase, CFuncMemPolicyNone, Args...>;
		return new ( m_pAllocator->Alloc( sizeof( Functor_t ) ) ) Functor_t( pObject, pfnProxied, std::forward<Args>( args )... );
		}

	//-------------------------------------

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateFunctor(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... ) const;
		using Functor_t = CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CCustomFunctorBase, CFuncMemPolicyNone, Args...>;
		return new ( m_pAllocator->Alloc( sizeof( Functor_t ) ) ) Functor_t( pObject, pfnProxied, std::forward<Args>( args )... );
		}

	//-------------------------------------

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateRefCountingFunctor(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FuncArgs... ), Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... );
		using Functor_t = CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CCustomFunctorBase, CFuncMemPolicyRefCount<OBJECT_TYPE_PTR>, Args...>;
		return new ( m_pAllocator->Alloc( sizeof( Functor_t ) ) ) Functor_t( pObject, pfnProxied, std::forward<Args>( args )... );
		}

	//-------------------------------------

	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE, typename... FuncArgs, typename... Args>
	inline auto CreateRefCountingFunctor(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FuncArgs... ) const, Args&&... args ) -> std::enable_if_t<std::is_base_of<FUNCTION_CLASS, std::remove_pointer_t<OBJECT_TYPE_PTR>>::value && std::is_same<detail::param_pack<std::decay_t<Args>...>, detail::param_pack<std::decay_t<FuncArgs>...>>::value, CFunctor*>
	{
		using Func_t = FUNCTION_RETTYPE( FUNCTION_CLASS::* )( FuncArgs... ) const;
		using Functor_t = CMemberFunctorI<OBJECT_TYPE_PTR, Func_t, CCustomFunctorBase, CFuncMemPolicyRefCount<OBJECT_TYPE_PTR>, Args...>;
		return new ( m_pAllocator->Alloc( sizeof( Functor_t ) ) ) Functor_t( pObject, pfnProxied, std::forward<Args>( args )... );
		}

	//-------------------------------------

private:
	CAllocator* m_pAllocator;
	
};

//-----------------------------------------------------------------------------

#endif // FUNCTORS_H