#pragma once

#include "obj.h" 
#include <cstddef>
 
template< class T >
struct TVConstRef
{
    const T & m_ref ; 
    TVConstRef( const T & r) : m_ref( r ){}    
    
    template< class TOther >
    operator TVConstRef< TOther >&() 
    {      
        static_assert( std::is_convertible< T , TOther >::value , " TOther must can be converted from T !" ) ;
        ( const TOther & )( m_ref ) ;
        return * reinterpret_cast< TVConstRef< TOther > * >( this ) ;
    } 
} ;

template< class T >
struct TVRef
{
    T & m_ref ; 
    TVRef( T & r) : m_ref( r ){}
    
    template< class TOther >
    operator TVRef< TOther >&() 
    {      
        static_assert( std::is_convertible< T , TOther >::value , " TOther must can be converted from T !" ) ;
        ( TOther & )( m_ref ) ;
        return * reinterpret_cast< TVRef< TOther > * >( this ) ;
    }  
    template< class TOther >
    operator TVConstRef< TOther >&() 
    {      
        static_assert( std::is_convertible< T , TOther >::value , " TOther must can be converted from T !" ) ;
        ( const TOther & )( m_ref ) ;
        return * reinterpret_cast< TVConstRef< TOther > * >( this ) ;
    } 
} ;

template< class T1 , class T2 = void , class T3 = void , class T4 = void , class T5 = void , class T6 = void >
struct VSConstBuffer
{
    unsigned     len      ;
    const T1 *   pData1   ;
    const T2 *   pData2   ;
    const T3 *   pData3   ;
    const T4 *   pData4   ;
    const T5 *   pData5   ;
    const T6 *   pData6   ;
} ;  

template< class T1 , class T2 , class T3 , class T4 , class T5 >
struct VSConstBuffer< T1 , T2 , T3 , T4 , T5 , void > 
{
    unsigned     len      ;
    const T1 *   pData1   ;
    const T2 *   pData2   ;
    const T3 *   pData3   ;
    const T4 *   pData4   ;
    const T5 *   pData5   ; 
} ;  

template< class T1 , class T2 , class T3 , class T4 >
struct VSConstBuffer< T1 , T2 , T3 , T4 , void , void > 
{
    unsigned     len      ;
    const T1 *   pData1   ;
    const T2 *   pData2   ;
    const T3 *   pData3   ;
    const T4 *   pData4   ; 

    VSConstBuffer(){}
    VSConstBuffer( unsigned n , const T1 * p1 , const T2 * p2 , const T3 * p3 , const T4 * p4 ):len(n),pData1(p1),pData2(p2),pData3(p3),pData4(p4){}

    //typedef VSConstBuffer< T1 , T2 , T3 , T4 , void , void > my_type ;
    //template< unsigned > struct ItemType ;
    //template<> struct ItemType< 0 >{ typedef T1 type ; static const T1 * GetItem( const my_type & my ) { return my.pData1 ; } }  ;
    //template<> struct ItemType< 1 >{ typedef T2 type ; static const T2 * GetItem( const my_type & my ) { return my.pData2 ; } }  ; 
    //template<> struct ItemType< 2 >{ typedef T3 type ; static const T3 * GetItem( const my_type & my ) { return my.pData3 ; } }  ;  
    //template<> struct ItemType< 3 >{ typedef T4 type ; static const T4 * GetItem( const my_type & my ) { return my.pData4 ; } }  ; 

    //template< unsigned idx >
    //VSConstBuffer< typename ItemType< idx >::type > SubBuffer() const 
    //{
    //    return VSConstBuffer< typename ItemType< idx >::type >( len , ItemType< idx >::GetItem( *this ) ) ;
    //}
    //template< unsigned idx1 , unsigned idx2 >
    //VSConstBuffer< typename ItemType< idx1 >::type , typename ItemType< idx2 >::type > SubBuffer2() const 
    //{
    //    return VSConstBuffer< typename ItemType< idx1 >::type , typename ItemType< idx2 >::type >( len , ItemType< idx1 >::GetItem( *this ) , ItemType< idx2 >::GetItem( *this ) ) ;
    //}
    //template< unsigned idx1 , unsigned idx2  , unsigned idx3 >
    //VSConstBuffer< typename ItemType< idx1 >::type , typename ItemType< idx2 >::type  , typename ItemType< idx3 >::type > SubBuffer3() const 
    //{
    //    return VSConstBuffer< typename ItemType< idx1 >::type , typename ItemType< idx2 >::type  , typename ItemType< idx3 >::type >( len , ItemType< idx1 >::GetItem( *this ) , ItemType< idx2 >::GetItem( *this )  , ItemType< idx3 >::GetItem( *this ) ) ;
    //}
} ;  

template< class T1 , class T2 , class T3 >
struct VSConstBuffer< T1 , T2 , T3 , void , void , void > 
{
    unsigned     len      ;
    const T1 *   pData1   ;
    const T2 *   pData2   ;
    const T3 *   pData3   ; 

    VSConstBuffer(){}
    VSConstBuffer( unsigned n , const T1 * p1 , const T2 * p2 , const T3 * p3 ):len(n),pData1(p1),pData2(p2),pData3(p3){}

    //typedef VSConstBuffer< T1 , T2 , T3 , void , void , void > my_type ;
    //template< unsigned > struct ItemType ;
    //template<> struct ItemType< 0 >{ typedef T1 type ; static const T1 * GetItem( const my_type & my ) { return my.pData1 ; } }  ;
    //template<> struct ItemType< 1 >{ typedef T2 type ; static const T2 * GetItem( const my_type & my ) { return my.pData2 ; } }  ; 
    //template<> struct ItemType< 2 >{ typedef T3 type ; static const T3 * GetItem( const my_type & my ) { return my.pData3 ; } }  ; 

    //template< unsigned idx >
    //VSConstBuffer< typename ItemType< idx >::type > SubBuffer() const 
    //{
    //    return VSConstBuffer< typename ItemType< idx >::type >( len , ItemType< idx >::GetItem( *this ) ) ;
    //}
    //template< unsigned idx1 , unsigned idx2 >
    //VSConstBuffer< typename ItemType< idx1 >::type , typename ItemType< idx2 >::type > SubBuffer2() const 
    //{
    //    return VSConstBuffer< typename ItemType< idx1 >::type , typename ItemType< idx2 >::type >( len , ItemType< idx1 >::GetItem( *this ) , ItemType< idx2 >::GetItem( *this ) ) ;
    //}
} ;  

template< class T1 , class T2 >
struct VSConstBuffer< T1 , T2 , void , void , void , void > 
{
    unsigned     len      ;
    const T1 *   pData1   ;
    const T2 *   pData2   ; 

    VSConstBuffer(){}
    VSConstBuffer( unsigned n , const T1 * p1 , const T2 * p2 ):len(n),pData1(p1),pData2(p2){}

    //typedef VSConstBuffer< T1 , T2 , void , void , void , void > my_type ;
    //template< unsigned > struct ItemType ;
    //template<> struct ItemType< 0 >{ typedef T1 type ; static const T1 * GetItem( const my_type & my ) { return my.pData1 ; } }  ;
    //template<> struct ItemType< 1 >{ typedef T2 type ; static const T2 * GetItem( const my_type & my ) { return my.pData2 ; } }  ; 

    //template< unsigned idx >
    //VSConstBuffer< typename ItemType< idx >::type > SubBuffer() const 
    //{
    //    return VSConstBuffer< typename ItemType< idx >::type >( len , ItemType< idx >::GetItem( *this ) ) ;
    //}
} ;   

template< class T >
struct VSConstBuffer< T , void , void , void , void , void >
{
    unsigned    len     ;
    const T *   pData   ;

    VSConstBuffer(){}
    VSConstBuffer( unsigned n , const T * p ):len(n),pData(p){}
} ; 

template< class T >
VPUREINTERFACE IVFixedBuffDiffViewer
{
    virtual void Change( unsigned bgn , unsigned len , const T * pData ) = 0 ;
} ;

template< class T >
VPUREINTERFACE IVBuffDiffViewer : IVFixedBuffDiffViewer< T >
{ 
    virtual void Resize( unsigned len ) = 0 ; 
} ;

template< class T1 , class T2 = void , class T3 = void , class T4 = void , class T5 = void , class T6 = void > struct TVBind
{
    TVBind( typename TVDataTrait< T1 >::reference r1
              , typename TVDataTrait< T2 >::reference r2
              , typename TVDataTrait< T3 >::reference r3
              , typename TVDataTrait< T4 >::reference r4
              , typename TVDataTrait< T5 >::reference r5
              , typename TVDataTrait< T6 >::reference r6
              ) : m_ref1( r1 )
                , m_ref2( r2 )
                , m_ref3( r3 )
                , m_ref4( r4 )
                , m_ref5( r5 )
                , m_ref6( r6 )
    {}

    typename TVDataTrait< T1 >::reference m_ref1 ; 
    typename TVDataTrait< T2 >::reference m_ref2 ; 
    typename TVDataTrait< T3 >::reference m_ref3 ; 
    typename TVDataTrait< T4 >::reference m_ref4 ; 
    typename TVDataTrait< T5 >::reference m_ref5 ; 
    typename TVDataTrait< T6 >::reference m_ref6 ; 
};

template< class T1 , class T2 , class T3 , class T4 , class T5 >
struct TVBind< T1 , T2 , T3 , T4 , T5 , void >
{
    TVBind( typename TVDataTrait< T1 >::reference r1
              , typename TVDataTrait< T2 >::reference r2
              , typename TVDataTrait< T3 >::reference r3
              , typename TVDataTrait< T4 >::reference r4
              , typename TVDataTrait< T5 >::reference r5 
              ) : m_ref1( r1 )
                , m_ref2( r2 )
                , m_ref3( r3 )
                , m_ref4( r4 )
                , m_ref5( r5 ) 
    {}

    typename TVDataTrait< T1 >::reference m_ref1 ; 
    typename TVDataTrait< T2 >::reference m_ref2 ; 
    typename TVDataTrait< T3 >::reference m_ref3 ; 
    typename TVDataTrait< T4 >::reference m_ref4 ; 
    typename TVDataTrait< T5 >::reference m_ref5 ;  
};

template< class T1 , class T2 , class T3 , class T4 >
struct TVBind< T1 , T2 , T3 , T4 , void , void >
{
    TVBind( typename TVDataTrait< T1 >::reference r1
              , typename TVDataTrait< T2 >::reference r2
              , typename TVDataTrait< T3 >::reference r3
              , typename TVDataTrait< T4 >::reference r4 
              ) : m_ref1( r1 )
                , m_ref2( r2 )
                , m_ref3( r3 )
                , m_ref4( r4 ) 
    {}

    typename TVDataTrait< T1 >::reference m_ref1 ; 
    typename TVDataTrait< T2 >::reference m_ref2 ; 
    typename TVDataTrait< T3 >::reference m_ref3 ; 
    typename TVDataTrait< T4 >::reference m_ref4 ;  
};

template< class T1 , class T2 , class T3 >
struct TVBind< T1 , T2 , T3 , void , void , void >
{
    TVBind( typename TVDataTrait< T1 >::reference r1
              , typename TVDataTrait< T2 >::reference r2
              , typename TVDataTrait< T3 >::reference r3 
              ) : m_ref1( r1 )
                , m_ref2( r2 )
                , m_ref3( r3 ) 
    {}

    typename TVDataTrait< T1 >::reference m_ref1 ; 
    typename TVDataTrait< T2 >::reference m_ref2 ; 
    typename TVDataTrait< T3 >::reference m_ref3 ;  
};

template< class T1 , class T2 >
struct TVBind< T1 , T2 , void , void , void , void >
{
    TVBind( typename TVDataTrait< T1 >::reference r1
              , typename TVDataTrait< T2 >::reference r2 
              ) : m_ref1( r1 )
                , m_ref2( r2 ) 
    {}

    typename TVDataTrait< T1 >::reference m_ref1 ; 
    typename TVDataTrait< T2 >::reference m_ref2 ;   
};

template< class T1 >
struct TVBind< T1 , void , void , void , void , void >
{
    TVBind( typename TVDataTrait< T1 >::reference r ) : m_ref1( r ) {}
    typename TVDataTrait< T1 >::reference m_ref1 ;   
};

template<>
struct TVBind< void , void , void , void , void , void > {} ;

template< class T1 , class T2 , class T3 , class T4 , class T5 , class T6 > 
struct TVDataTrait< TVBind< T1 , T2 , T3 , T4 , T5 , T6 > >
{
    typedef TVBind< T1 , T2 , T3 , T4 , T5 , T6 > & reference ;
} ;
 

template< class _Ty1, class _Ty2 > inline TVBind< typename std::remove_const< _Ty1 >::type , typename std::remove_const< _Ty2 >::type >
vf_makebind( _Ty1 & _Val1 , _Ty2 & _Val2 )
{	// return pair composed from arguments
    typedef TVBind< typename std::remove_const< _Ty1 >::type , typename std::remove_const< _Ty2 >::type > _MyBind ;
    return ( _MyBind( _Val1, _Val2 ) ) ;
}

template< class _Ty1, class _Ty2, class _Ty3 > inline TVBind< typename std::remove_const< _Ty1 >::type , typename std::remove_const< _Ty2 >::type , typename std::remove_const< _Ty3 >::type >
vf_makebind( _Ty1 & _Val1 , _Ty2 & _Val2 , _Ty3 & _Val3 )
{	// return pair composed from arguments
    typedef TVBind< typename std::remove_const< _Ty1 >::type , typename std::remove_const< _Ty2 >::type , typename std::remove_const< _Ty3 >::type > _MyBind ;
    return ( _MyBind( _Val1, _Val2 , _Val3 ) ) ;
}
template< class _Ty1, class _Ty2, class _Ty3, class _Ty4 > inline TVBind< typename std::remove_const< _Ty1 >::type , typename std::remove_const< _Ty2 >::type , typename std::remove_const< _Ty3 >::type , typename std::remove_const< _Ty4 >::type >
vf_makebind( _Ty1 & _Val1 , _Ty2 & _Val2 , _Ty3 & _Val3 , _Ty4 & _Val4 )
{	// return pair composed from arguments
    typedef TVBind< typename std::remove_const< _Ty1 >::type , typename std::remove_const< _Ty2 >::type , typename std::remove_const< _Ty3 >::type  , typename std::remove_const< _Ty4 >::type > _MyBind ;
    return ( _MyBind( _Val1, _Val2 , _Val3 , _Val4 ) ) ;
}

#define VD_MKBND vf_makebind

template< typename T >
struct VS01Buffer
{
    bool        m_Vaild;
    const T*    m_pData;

    VS01Buffer():m_Vaild(false),m_pData(nullptr){}
    VS01Buffer( const T & ref ):m_Vaild(true),m_pData(&ref){}
    VS01Buffer( bool v , const T * p ):m_Vaild(v),m_pData(p){}
};

template< typename T >
struct VSBlock1D
{
    unsigned    m_TCount;

    VSBlock1D():m_TCount(0),m_pData(nullptr){}
    VSBlock1D( unsigned count , const T * pDat ):m_TCount(count),m_pData( reinterpret_cast< const void * >( pDat ) ){}

    const T * operator() ()
    {
        return reinterpret_cast< const T * >( m_pData );
    }

    T& operator() ( unsigned idx )
    {
        VASSERT( idx < m_TCount );
        return reinterpret_cast< const T * >( m_pData )[ idx ];
    }

private:
    const void *    m_pData;
};

template<>
struct VSBlock1D< void >
{
    VSBlock1D():m_bufSize(0),m_pData(nullptr){}
    VSBlock1D( unsigned bufsz , const void * pDat ):m_bufSize(bufsz),m_pData(pDat){}

    template< typename T >
    VSBlock1D< T > As()
    {
        if ( m_bufSize == 0 )
        {
            return VSBlock1D< T >();
        }

        unsigned m = m_bufSize % sizeof( T );
        VASSERT( m == 0 );
        return VSBlock1D< T >( m_bufSize / sizeof( T ) , reinterpret_cast< const T * >( m_pData ) );
    }

private:
    unsigned        m_bufSize;
    const void *    m_pData;
};

template< typename T >
struct VSBlock2D
{
    unsigned    m_T1stCount;
    unsigned    m_T2ndCount;

    VSBlock2D():m_T1stCount(0),m_T2ndCount(0),m_pData(nullptr){}
    VSBlock2D( unsigned column , unsigned row , const T * pDat )
        :m_T1stCount(column),m_T2ndCount(row),m_pData( reinterpret_cast< const void * >( pDat ) ){}

    const T * operator() ()
    {
        return reinterpret_cast< const T * >( m_pData );
    }

    T& operator() ( unsigned columnIdx , unsigned rowIdx )
    {
        VASSERT( columnIdx < m_T1stCount && rowIdx < m_T2ndCount );
        return reinterpret_cast< const T * >( m_pData )[ columnIdx + rowIdx * m_T1stCount ];
    }

private:
    const void *    m_pData;
};

template<>
struct VSBlock2D< void >
{
    VSBlock2D(): m_bufSize( 0 ) , m_bufLineSize( 0 ) , m_pData( nullptr ){}
    VSBlock2D( unsigned bufsz , unsigned buflinesz , const void * pDat ) 
        : m_bufSize( bufsz ) , m_bufLineSize( buflinesz ) , m_pData( pDat ){}

    template< typename T >
    VSBlock2D< T > As()
    {
        if ( m_bufSize == 0 )
        {
            return VSBlock2D< T >();
        }

        VASSERT( m_bufSize % m_bufLineSize == 0 );
        VASSERT( m_bufLineSize % sizeof( T ) == 0 );
        return VSBlock2D< T >( m_bufLineSize / sizeof( T ) , m_bufSize / m_bufLineSize , reinterpret_cast< const T * >( m_pData ) );
    }

private:
    unsigned        m_bufSize;
    unsigned        m_bufLineSize;
    const void *    m_pData;
};