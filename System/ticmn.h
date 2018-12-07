#pragma once

#include <vector>
#include <algorithm> 
#include <assert.h>
#include <functional> 
#include "mem.h"
#include <cstdarg>

#ifdef _DEBUG 
#include <windows.h>
inline bool vf_trace( char * format , ...)
{
   char buffer[1000];
#ifdef WIN32
   va_list argptr;
   va_start(argptr, format);
   vsprintf_s(buffer, format, argptr);
   va_end(argptr);

   OutputDebugStringA(buffer);
#endif

   return true;
}
inline void vf_assert( bool bexp )
{
#ifdef WIN32
    if( ! bexp )
        std::_DEBUG_ERROR( "Assert Error!" ) ;
#endif
} ;
#endif  

#ifdef _DEBUG
inline void vf_assert_msg( const wchar_t * format , const wchar_t * fn , unsigned ln )
{
#ifdef WIN32
    OutputDebugStringW( fn ) ;
    OutputDebugStringW( format ) ; 
    std::_Debug_message( format , fn , ln ) ;
#endif
}
#endif 

#ifdef _DEBUG
#else
#endif 

#ifdef _DEBUG 
#define VTRACE     vf_trace
#define VASSERT    vf_assert
#define VVERIFY(f) VASSERT( f )  
#define VASSERT_MSG( b , str ) if( !(b) ) vf_assert_msg( L ## str ,__FILEW__, __LINE__) 
#else
#ifdef WIN32
#define VTRACE  __noop
#define VASSERT __noop
#else
#define VTRACE
#define VASSERT
#endif

#define VVERIFY(f) ((void)(f))
#ifdef WIN32
#define VASSERT_MSG( b , str ) __noop
#else
#define VASSERT_MSG( b , str )
#endif
#endif

  
template <class T>
class VScopedArray
{
public:
    typedef T element_type;

    explicit VScopedArray(T * array = NULL) : m_array(array) { }

    ~VScopedArray() { delete [] m_array; }

    // test for pointer validity: defining conversion to unspecified_bool_type
    // and not more obvious bool to avoid implicit conversions to integer types
    typedef T *(VScopedArray<T>::*unspecified_bool_type)() const;
    operator unspecified_bool_type() const
    {
        return m_array ? &VScopedArray<T>::get : NULL;
    }

    void reset(T *array = NULL)
    {
        if ( array != m_array )
        {
            delete [] m_array;
            m_array = array;
        }
    }

    T* release()
    { 
        T* p = m_array ; 
        m_array = NULL ;
        return p ;
    }

    T& operator[](size_t n) const { return m_array[n]; }

    T *get() const { return m_array; }

    void swap(VScopedArray &other)
    {
        T * const tmp = other.m_array;
        other.m_array = m_array;
        m_array = tmp;
    }

private:
    T *m_array;

private : // 防止copy操作
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( VScopedArray , T ) ;
};

template< unsigned V , unsigned N , unsigned NMAX >
struct TVNextPower2It
{
    static const unsigned moved = ( 1 << N ) ;
    static const unsigned uval  = TVNextPower2It< V , N + 1 , NMAX >::value ;
    static const unsigned value = uval | uval >> moved ;
} ;

template< unsigned V , unsigned N >
struct TVNextPower2It< V , N , N >
{ 
    static const unsigned value = V - 1 ;
} ;

template< unsigned V >
struct TVNextPower2
{
    static const unsigned value = TVNextPower2It< V , 0 , 5 >::value + 1 ; 
} ;

namespace VCMN
{
    // 参考：http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda

    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())>{} ; 

    template < typename ClassType, typename ReturnType , typename Args >
    struct function_traits< ReturnType ( ClassType::* )( Args ) const > 
    {
        typedef ReturnType result_type ;
        typedef Args       arg_type    ;
        //typedef std::remove_reference< Args >::type clean_arg_type ;
    }; 

    template< typename ClassType, typename ReturnType >
    struct function_traits< ReturnType( ClassType::* )() const >
    {
        typedef ReturnType result_type;
        typedef void       arg_type;
        //typedef std::remove_reference< Args >::type clean_arg_type ;
    };


    template< class T  , class TRef = typename TVDataTrait< T >::reference >
    class TVUserAdaptor : public IVUser< T , TRef >
    {   
    public :
        TVUserAdaptor(){ m_func = [](TRef){} ; }
        TVUserAdaptor( std::function< void ( TRef ) > f ) : m_func( f ){}

    protected :
        virtual void Visit( TRef t ){ m_func( t ) ; }

    protected :
        std::function<void ( TRef )> m_func ;
    } ;

    template<>
    class TVUserAdaptor<void ,void > : public IVUser< void , void >
    {
    public:
        TVUserAdaptor< void ,void >():m_func([](){}){}
        TVUserAdaptor< void ,void >( std::function< void () > f ) : m_func( f ){}

    protected:
        virtual void Visit(){ m_func(); }

    protected:
        std::function<void ()> m_func ;
    };
     
    template< class F 
            , class T = typename std::remove_const< typename std::remove_reference< typename VCMN::function_traits< F >::arg_type >::type >::type
            , class TRef = typename TVDataTrait< T >::reference >
    class TVLambdaUser : public IVUser< T , TRef >
    {
    public :
        TVLambdaUser( F f ) : m_func( f ) {} 

    protected :
        virtual void Visit( TRef t )
        {
            m_func( t ) ; 
        }

    protected :
        F m_func ;
    } ;

    template< class F >
    class TVLambdaUser< F , void , void > : public IVUser< void , void >
    {
    public:
        TVLambdaUser( F f ) : m_func( f ) {}

    protected:
        virtual void Visit()
        {
            m_func();
        }

    protected:
        F m_func;
    };

    template< class F >
    TVLambdaUser< F > LambdaToUser( F func )
    {
        return VCMN::TVLambdaUser< F >( func ) ;
    }  

    template< class T >
    inline 
        void ClearPtrArray( std::vector< T * > & vec )
    {
        for( typename std::vector< T * >::iterator it = vec.begin() ; it != vec.end() ; it ++ )
        {
            delete *it ;
        }

        vec.clear() ;
    } ;

    template< class T , class F >
    inline 
        void ClearPtrArray( std::vector< T * > & vec , F f )
    {
        for( typename std::vector< T * >::iterator it = vec.begin() ; it != vec.end() ; it ++ )
        {
            f( *it ) ;
            delete *it ;
        }

        vec.clear() ;
    } ;
    
    template< class T >
    void Copy( unsigned count , const T * pBuff , std::vector< T > & v )
    {
        v.resize( count ) ;
        if( count )
        { 
            memcpy( &v.front() , pBuff , count * sizeof( T ) ) ;
        }
    }

    template< class T >
    void Copy( const VSConstBuffer< T > & cb , std::vector< T > & v )
    {
        VCMN::Copy( cb.len , cb.pData , v ) ; 
    }

    template< class T1 , class T2 >
    void Copy( const VSConstBuffer< T1 , T2 > & cb , std::vector< T1 > & v1 , std::vector< T2 > & v2 )
    {
        v1.resize( cb.len ) ;
        v2.resize( cb.len ) ;
        if( cb.len )
        { 
            memcpy( &v1.front() , cb.pData1 , cb.len * sizeof( T1 ) ) ;
            memcpy( &v2.front() , cb.pData2 , cb.len * sizeof( T2 ) ) ;
        }
    } 

    template< class T >
    class Is_Dependent
    {
    private :
        typedef char one;
        typedef long two;

        template <class C> 
        static one _testsupport( typename C::ISupport * ) ;

        template <class C>
        static two _testsupport(...) ;    

    public:
        static const bool value = ( sizeof( _testsupport< T >( 0 ) ) == sizeof(char) ) ;
    };
}

#define VD_L2U       VCMN::LambdaToUser 

template< class T >
VSConstBuffer< T > VD_V2CB( const std::vector< T > & v )
{
    VSConstBuffer< T > cb ; 
    cb.len = v.size() ;

    if( cb.len )
    {
        cb.pData = & v.front() ; 
    }
    else
    {
        cb.pData = 0 ;
    }

    return cb ;
}

template< class T1 , class T2 >
VSConstBuffer< T1 , T2 > VD_V2CB( const std::vector< T1 > & v1 , const std::vector< T2 > & v2 )
{
    VSConstBuffer< T1 , T2 > cb ; 
    VASSERT( v1.size() == v2.size() ) ;
    cb.len = v1.size() ;

    if( cb.len )
    {
        cb.pData1 = & v1.front() ; 
        cb.pData2 = & v2.front() ; 
    }
    else
    {
        cb.pData1 = 0 ; 
        cb.pData2 = 0 ; 
    }

    return cb ;
}