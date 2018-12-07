#pragma once

#include "obj.h"
#include <new>
#include <cassert>
#include "ticmn.h"
#include "pttrn.h"
#include <cstring>  

struct VSpaceUtil
{
    template< class IT > inline static void InitSpace( IVSpace< IT > & spc )
    {
        std::memset( & spc , 0 , sizeof( IVSpace< IT > ) ) ;
    }
} ;

inline bool vf_ispoweroftwo( unsigned x ) 
{
    return (x & (~x + 1)) == x ;
}

template< typename T , typename F , typename FCHNG >
inline void vf_tidypntlst_vchng( std::vector< T * > & arr , unsigned & nCurCount , F f , FCHNG fchng )
{
    VASSERT( nCurCount <= arr.size() ) ; 
    unsigned bgn(0) ; 

    while( bgn < nCurCount && ! f( * arr[bgn] ) ) //(arr[ bgn ]->*MF)() ) // IsClosed() )
        bgn ++ ; 

    unsigned end( bgn + 1 ) ; 

    while( end < nCurCount )
    {
        if( ! f( * arr[ end ] ) )
        {
            VASSERT( f( * arr[bgn] ) && ! f( * arr[end] )  ) ;
            std::swap( arr[ bgn ] , arr[ end ] ) ; 
            fchng( * arr[bgn] , bgn ) ;
            bgn ++ ;
        }

        end ++ ;
        VASSERT( end == nCurCount || f( * arr[bgn] ) ) ;
    } 

    nCurCount = bgn ;
} 
 
template< class T >
inline void vf_tidysvcarr( std::vector< T * > & arr , unsigned & nCurCount )
{
    vf_tidypntlst_vchng( arr , nCurCount , []( const auto & v )->bool{
        return v.IsClosed() ;
    } 
    , []( auto & , unsigned ){} ) ;
}

// 简单结构体实现的Client Service模板
template< class IT >
class TVServiceProxy 
{
public :
    TVServiceProxy() 
    {
        VSpaceUtil::InitSpace( m_space ) ;
    }
    virtual ~TVServiceProxy()
    {
        //VASSERT( m_space.pPreRef == 0 || m_space.pNxtRef == 0 ); // 尚未确定写法，与_useService中有某些关系
        Destory() ;
        //assert( m_space.pPreRef == 0 && m_space.pNxtRef == 0 && m_space.pService == 0 ) ;
    }

public :  
    void Create( IVSpace< IT > & space )
    {   
        assert( ! space.pPreRef   ) ; // 必须是服务提供者
        assert(   space.pService  ) ; // 服务必须存在
        assert( ! space.pNxtRef   ) ; // 尚未有对象使用服务

        Destory() ;

        m_space.pService = space.pService ;
        m_space.pPreRef  = & space        ;
        m_space.pNxtRef  = 0              ;
        m_space.procFreeClnt = ClearClientSpace   ; 
        m_space.procFreeSrvc = space.procFreeSrvc ;

        space.pNxtRef  = & m_space        ;
        space.procFreeClnt = m_space.procFreeClnt ;
    }

    template< class F >
    void Create( IVSpace< IT > & space , F f )
    {
        Create( space ) ;

        if( m_space.pService )
        { 
            _useService( * m_space.pService , f ) ;
        }
    }
   
    template< class F , class UF >
    void Create( IVSpace< IT > & space , F f , UF uf )
    {
        Create( space ) ; 

        if( m_space.pService )
        { 
            _useService( * m_space.pService , f ) ;
        }
        else
        {
            uf() ;
        } 
    }

    template< class F >    
    void UseServer( F f )
    { 
        if( m_space.pService )
        {
            _useService( * m_space.pService , f ) ;
        }
    }

    template< class F , class UF >   
    void UseServer( F f , UF uf )
    { 
        if( m_space.pService )
        { 
            _useService( * m_space.pService , f ) ;
        }
        else
        {
            uf() ;
        }
    }  
        
    void Destory()
    {
        if( m_space.pPreRef )
        {
            if( m_space.pNxtRef ) // 被占用
            { 
                m_space.pNxtRef->pPreRef = 0 ;
                m_space.pNxtRef = m_space.pNxtRef->pNxtRef ;
                if( m_space.pNxtRef )
                    m_space.pNxtRef->pPreRef = &m_space ;
            }
            else
            {
                m_space.procFreeSrvc( * m_space.pPreRef ) ;
                VSpaceUtil::InitSpace( m_space ) ;
            }   
        }
    }

    // wangxinlong added 不确定加的对不对 用于帮助添加辅助类 无序MultiPipe
    bool IsClosed() const
    {
        return ( m_space.pPreRef == 0 ) ;
    } ;

//
//#ifdef _DEBUG
//#pragma push_macro("new")
//#undef new
//private :
//      // Prevent heap allocation
//      void * operator new( size_t , int c , const char * , int nLine )   ;
//      void * operator new[]( size_t , int c , const char * , int nLine ) ;
//      void   operator delete   (void *)                                  ;
//      void   operator delete[] (void*)                                   ;
//#pragma pop_macro("new") 
//#else
//private :
//      // Prevent heap allocation
//      void * operator new(size_t)  ;
//      void * operator new[] (size_t) ;  
//      void   operator delete   (void *);
//      void   operator delete[] (void*);  
//#endif

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVServiceProxy , IT ) ;

    template< class F >
    void _useService( IT & svc , F f )
    {
        // 占用
        IVSpace< IT > spcTmp , * pNxt( m_space.pNxtRef ) ;

        memcpy( & spcTmp , & m_space , sizeof( IVSpace< IT > ) ) ;

        m_space.pNxtRef = & spcTmp  ; 
        spcTmp.pPreRef  = & m_space ;

        if( spcTmp.pNxtRef )
            spcTmp.pNxtRef->pPreRef = & spcTmp ;

        f( svc ) ;

        // 清理占用
        if( spcTmp.pPreRef )
        {
            spcTmp.pPreRef->pNxtRef = spcTmp.pNxtRef ;
            if( spcTmp.pNxtRef )
            {
                spcTmp.pNxtRef->pPreRef = spcTmp.pPreRef ;
            }
        }
    } 

    static void ClearClientSpace( IVSpace< IT > & spc )
    {
        VSpaceUtil::InitSpace( spc  ) ; 
    } 

private :
    IVSpace< IT >  m_space ;
} ; 

template< class IT , class TC = IT >
class TVService
{
public :
    TVService()
    {
        InitSpace() ;
    }
    virtual ~TVService()
    {
#if _ITERATOR_DEBUG_LEVEL == 2 
        VASSERT_MSG( IsClosed() , "析构TVService< IT  >对象前请使用Close释放拥有的life对象，在某些特殊情况下可能会造成错误！\r\n" ) ;
#endif /* _DEBUG */  

        Close() ;
    }

    bool IsClosed() const
    {
        return ( getSpace()->pNxtRef == 0 ) ;
    }
    
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif
    template< class TSpaceUse , class ... ARGs > 
    void Run( TSpaceUse usr , ARGs & ... args )
    {
        _run( usr , [ &args ... ]( void * p )->IT*{ return new(p) TC( args ... ) ; } ) ;
    }
    
    template< class TE , class ... ARGs > 
    void Run( VO< TE > & usr , ARGs & ... args )
    {
        static_assert( std::is_base_of< TE , IT >::value , "IT must derived from Export type TE !" ) ;
        _runWithUser( usr , [ &args... ]( void * p )->IT*{ return new(p) TC( args ... ) ; } ) ;
    }

#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
    void Close()
    {
        CloseServiceFromServer( *getSpace() ) ;  
    }

    template< class TSpaceUse , class TBuilder >
    void _run( TSpaceUse usr , TBuilder tb )
    {
        IVSpace< IT > * pspc = getSpace() ;

        CloseServiceFromServer( *pspc ) ; 

        unsigned A = CLASS_BODY_OFFSET ;
        unsigned B = CLASS_BODY_SIZE ;

        pspc->pService      = tb( getService() ) ; 
        pspc->procFreeSrvc  = ClearServiceSpace ;  

        usr( * pspc ) ;

        if( ! pspc->pNxtRef && pspc->pService )
        {
            assert( ! pspc->pNxtRef ) ; 
            reinterpret_cast< TC * >( pspc->pService )->~TC() ;
            pspc->pService = 0 ;
        }
    }
     
    template< typename TE , typename TBuilder >
    void _runWithUser( VO< TE > & usr , TBuilder tb )
    {
        _run( [&usr]( VI<  IT  >& spc ){ usr.Visit( spc ) ; } , tb ) ;
    } 

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVService , IT , TC ) ;

    static const size_t CLASS_BODY_OFFSET = sizeof( IVSpace< IT > ) ;
    static const size_t CLASS_BODY_SIZE  = sizeof( TC ) ;
    static const size_t CLASS_TOTAL_SIZE = CLASS_BODY_OFFSET + CLASS_BODY_SIZE ;

    void CloseServiceFromServer( IVSpace< IT > & spc )
    {
        IVSpace< IT > * pNxt = spc.pNxtRef ;

        if( pNxt )
        {
            if( pNxt->pNxtRef ) // 被占用
            {
                pNxt->pNxtRef->pPreRef = 0 ;
                VSpaceUtil::InitSpace( *pNxt ) ; 
                VSpaceUtil::InitSpace( spc ) ; 
            } 
            else
            { 
                spc.procFreeClnt( *pNxt ) ;
                ClearServiceSpace( spc ) ;
            }
        }
    } 
      
    void InitSpace()
    { 
        std::memset( & m_space , 0 , CLASS_TOTAL_SIZE ) ;
    }  

    IVSpace< IT > * getSpace()
    {
        return reinterpret_cast< IVSpace< IT > * >( m_space ) ;
    }

    const IVSpace< IT > * getSpace() const
    {
        return reinterpret_cast< const IVSpace< IT > * >( m_space ) ;
    }

    TC * getService()
    {
        return reinterpret_cast< TC * >( m_space + CLASS_BODY_OFFSET ) ;
    }

    static void ClearServiceSpace( IVSpace< IT > & spc )
    {
        TC * p = reinterpret_cast< TC * >( reinterpret_cast< char * >( &spc ) + CLASS_BODY_OFFSET ) ;
        p->~TC() ;
        VSpaceUtil::InitSpace( spc ) ;
    }  

    char  m_space[ CLASS_TOTAL_SIZE ] ;
} ;

template< class IT , unsigned COUNT >
class TVMultiServer : public IT
{
public :
    TVMultiServer()
    {
        std::memset( m_space , 0 , sizeof( IVSpace< IT > ) * COUNT ) ;
    }
    virtual ~TVMultiServer()
    {
        DebugCheckClean() ;
        Close() ;
    }  

    void DebugCheckClean()
    {
        VASSERT_MSG( IsClosed() , "析构TVMultiServer< IT  >对象前请使用Close释放拥有的life对象，在某些特殊情况下可能会造成错误！\r\n");
    }

    bool IsClosed() const
    {
        for( unsigned i = 0 ; i < COUNT ; i ++ )
        {
            if( m_space[i].pNxtRef != 0 )
                return false ;
        }

        return true ;
    }

public : 
    template< unsigned ID , class TSpaceUse > 
    void Run( TSpaceUse usr )
    {
        static_assert( ID < COUNT , "Indx is bigger than count!" ) ;
        IVSpace< IT >  & spc = m_space[ID] ;

        CloseServiceFromServer( spc ) ; 

        spc.pService      = this ; 
        spc.procFreeSrvc  = ClearServiceSpace ;  

        usr( spc ) ;

        if( ! spc.pNxtRef )
        {
            assert( ! spc.pNxtRef ) ; 
            spc.pService = 0 ;
        }
    }

    template< unsigned ID >
    void Run( VO<  IT  >& usr )
    {
        auto RF = [&usr]( VI<  IT  >& spc ){ usr.Visit( spc ) ;  } ;
        Run< ID , decltype(RF) >( RF ) ;
    }

    void Close()
    {
        for( unsigned i = 0 ; i < COUNT ; i ++ )
        {
            CloseServiceFromServer( m_space[i] ) ;  
        }
    }

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVMultiServer , IT , COUNT ) ;
    void CloseServiceFromServer( IVSpace< IT > & spc )
    {
        IVSpace< IT > * pNxt = spc.pNxtRef ;

        if( pNxt )
        {
            VASSERT( pNxt->pNxtRef == 0 ) ;
            //if( pNxt->pNxtRef ) // 被占用
            //{
            //    pNxt->pNxtRef->pPreRef = 0 ;
            //    VSpaceUtil::InitSpace( *pNxt ) ; 
            //    VSpaceUtil::InitSpace( spc ) ; 
            //} 
            //else
            { 
                spc.procFreeClnt( *pNxt ) ;
                VSpaceUtil::InitSpace( spc ) ; 
            }
        }
    }

    static void ClearServiceSpace( IVSpace< IT > & spc )
    { 
        VSpaceUtil::InitSpace( spc ) ;
    } 

private :
    IVSpace< IT >  m_space[ COUNT ] ;
} ;

template< class IT >
class TVServer : public IT
{
public :
    template< class ... TARGs >
    TVServer( TARGs & ... args )
        : IT( args ... )
    {
        VSpaceUtil::InitSpace( m_space ) ;
    }
    virtual ~TVServer()
    {
        DebugCheckClean() ;
        Close() ;
    }  

    void DebugCheckClean()
    {
#if _ITERATOR_DEBUG_LEVEL == 2
        VASSERT_MSG( IsClosed() , "析构TVServer< IT  >对象前请使用Close释放拥有的life对象，在某些特殊情况下可能会造成错误！\r\n");
#endif /* _DEBUG */  
    }

    bool IsClosed() const
    {
        return  m_space.pNxtRef == 0 ;
    }

public :
    template< class TSpaceUse > 
    void Run( TSpaceUse usr )
    {
        CloseServiceFromServer( m_space ) ; 

        m_space.pService      = this ; 
        m_space.procFreeSrvc  = ClearServiceSpace ;  

        usr( m_space ) ;

        if( ! m_space.pNxtRef )
        {
            assert( ! m_space.pNxtRef ) ; 
            m_space.pService = 0 ;
        }
    }

    void Run( VO<  IT  >& usr )
    {
        Run( [&usr]( VI<  IT  >& spc ){ usr.Visit( spc ) ;  } ) ;
    }

    void Close()
    {
        CloseServiceFromServer( m_space ) ;  
    }

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVServer , IT ) ;
    void CloseServiceFromServer( IVSpace< IT > & spc )
    {
        IVSpace< IT > * pNxt = spc.pNxtRef ;

        if( pNxt )
        {
            VASSERT( pNxt->pNxtRef == 0 ) ;
            //if( pNxt->pNxtRef ) // 被占用
            //{
            //    pNxt->pNxtRef->pPreRef = 0 ;
            //    VSpaceUtil::InitSpace( *pNxt ) ; 
            //    VSpaceUtil::InitSpace( spc ) ; 
            //} 
            //else
            { 
                spc.procFreeClnt( *pNxt ) ;
                VSpaceUtil::InitSpace( spc ) ; 
            }
        }
    }

    static void ClearServiceSpace( IVSpace< IT > & spc )
    { 
        VSpaceUtil::InitSpace( spc ) ;
    } 

private :
    IVSpace< IT >  m_space ;
} ;

template< class TLI >
class TVSCArrayTmpl
{
public :
    TVSCArrayTmpl()
        : m_nDrivdCount(0)
    {}

    ~TVSCArrayTmpl()
    {
        VCMN::ClearPtrArray( m_lstClient ) ;
    }
  
public :
    template< class FV >
    void _additem( FV f )
    {
        if( m_nDrivdCount < m_lstClient.size() )
        {
            f( * m_lstClient[ m_nDrivdCount ++ ] ) ;
        }
        else
        {
            if( vf_ispoweroftwo( m_lstClient.size() ) )
            { 
                vf_tidysvcarr( m_lstClient , m_nDrivdCount ) ; 

                if( m_nDrivdCount < m_lstClient.size() )
                {
                    f( * m_lstClient[ m_nDrivdCount ++ ] ) ;
                    return ;
                }
            }

            TLI * p( new TLI ) ;
            f( * p ) ;
            m_lstClient.push_back( p ) ;
            m_nDrivdCount ++ ;
        }
    }

    void _tidy()
    {
        vf_tidysvcarr( m_lstClient , m_nDrivdCount ) ;
    }

    template< class FV >
    void _visititem( FV f )
    { 
        for( unsigned i = 0 ; i < m_nDrivdCount ; i ++ )
        {
            f( * m_lstClient[i] ) ;
        }
    } 

    template< class FV >
    void _clear( FV f )
    {
        for( unsigned i = 0 ; i < m_nDrivdCount ; i ++ )
        {
            f( * m_lstClient[i] ) ;
        }
        m_nDrivdCount = 0 ;
    } ;
     
    void _clear( )
    {
        m_nDrivdCount = 0 ;
    } 

private :
    unsigned             m_nDrivdCount ;
    std::vector< TLI * > m_lstClient   ;
} ;

// MC : 最大长度
template< class IT , class TC = IT , unsigned MC = 0 > 
class TVServiceArray
{
private : 
    class ServiceUnit
    {
    public :
        ServiceUnit()
            :m_pToken(0)
        {
            InitSpace() ;
        }
        virtual ~ServiceUnit()
        {
#if _ITERATOR_DEBUG_LEVEL == 2 
            VASSERT_MSG( IsClosed() , "析构ServiceUnit< IT  >对象前请使用Close释放拥有的life对象，在某些特殊情况下可能会造成错误！\r\n" ) ;
#endif /* _DEBUG */  

            Close() ;
        }
 
        bool IsClosed() const
        {
            const IVSpace< IT > * pspc = getSpace() ;
            return ( getSpace()->pNxtRef == 0 ) ;
        }
         
        void Close()
        {
            CloseServiceFromServer( *getSpace() ) ;  
        }

        template< class TSpaceUse , class TBuilder >
        bool Run( TVServiceArray< IT , TC , MC > * pContainer , TSpaceUse usr , TBuilder tb )
        {
            IVSpace< IT > * pspc = getSpace() ;

            assert( IsClosed() ) ;

            unsigned A = CLASS_BODY_OFFSET ;
            unsigned B = CLASS_BODY_SIZE ;

            pspc->pService      = tb( getService() ) ; 
            pspc->procFreeSrvc  = &TVServiceArray<IT,TC,MC>::ClearServiceSpace ;  

            usr( * pspc ) ;

            if( ! pspc->pNxtRef )
            {
                assert( ! pspc->pNxtRef ) ; 
                ( reinterpret_cast< TC * >( pspc->pService ) )->~TC() ;
                pspc->pService = 0 ;
                return false ;
            }

            m_pToken = pContainer ;

            return true ;
        }

    private :
        static const size_t CLASS_BODY_OFFSET = sizeof( IVSpace< IT > ) ;
        static const size_t CLASS_BODY_SIZE  = sizeof( TC ) ;
        static const size_t CLASS_TOTAL_SIZE = CLASS_BODY_OFFSET + CLASS_BODY_SIZE ;

    public :
        ServiceUnit * GetPreFree()
        {
            assert( IsClosed() ) ;
            return reinterpret_cast< ServiceUnit * >( m_pToken ) ;
        } ;
        
        void PushFree( ServiceUnit * pre )
        {
            assert( IsClosed() ) ;
            m_pToken = pre ;
        }

        TVServiceArray< IT , TC , MC > * GetContainer() 
        {
            assert( !IsClosed() ) ;
            return reinterpret_cast< TVServiceArray< IT , TC , MC > * >( m_pToken ) ;
        }

        void MoveTo( ServiceUnit & other )
        {
            assert( !IsClosed() ) ;
            memcpy( other.m_space , m_space , CLASS_TOTAL_SIZE ) ;
            other.m_pToken = m_pToken ;
            IVSpace< IT > * pSpc = getSpace() ;
            
            pSpc->pNxtRef->pPreRef = other.getSpace() ;  
             
            IT * pNewSvr = other.getService() ;
             
            IVSpace< IT > * pNxt = other.getSpace() ;
            while( pNxt )
            {
                pNxt->pService = pNewSvr ;
                pNxt = pNxt->pNxtRef ;
            }


            InitSpace() ;
        } 

        static ServiceUnit * SpaceToUnit( IVSpace< IT > * spc )
        {
            char * p = reinterpret_cast< char * >( spc ) ;
            p -= offsetof( ServiceUnit , m_space ) ;
            return reinterpret_cast< ServiceUnit * >( p ) ;
        }

        void CloseServiceFromServer( IVSpace< IT > & spc )
        {
            IVSpace< IT > * pNxt = spc.pNxtRef ;

            if( pNxt )
            {
                if( pNxt->pNxtRef ) // 被占用
                {
                    pNxt->pNxtRef->pPreRef = 0 ;
                    VSpaceUtil::InitSpace( *pNxt ) ; 
                    VSpaceUtil::InitSpace( spc ) ; 
                } 
                else
                { 
                    spc.procFreeClnt( *pNxt ) ;
                    getService()->~TC() ;
                    VSpaceUtil::InitSpace( spc ) ;
                }
            }
        } 

        void InitSpace()
        { 
            std::memset( & m_space , 0 , CLASS_TOTAL_SIZE ) ;
        }  

        IVSpace< IT > * getSpace()
        {
            return reinterpret_cast< IVSpace< IT > * >( m_space ) ;
        }

        const IVSpace< IT > * getSpace() const
        {
            return reinterpret_cast< const IVSpace< IT > * >( m_space ) ;
        }

        TC * getService()
        {
            return reinterpret_cast< TC * >( m_space + CLASS_BODY_OFFSET ) ;
        }

        void * m_pToken ;
        char   m_space[ CLASS_TOTAL_SIZE ] ;
    } ;

public :
    TVServiceArray()
        : m_nCurSize( 0 )
        , m_pLastFree( 0 ) 
    { 
    }
    virtual ~TVServiceArray()
    { 
        for( unsigned i = 0 ; i < m_nCurSize ; i ++ )
        {
#if _ITERATOR_DEBUG_LEVEL == 2 
            VASSERT_MSG( m_pServices[i].IsClosed() ,  "析构ServiceUnit< IT  >对象前请使用Close释放拥有的life对象，在某些特殊情况下可能会造成错误！\r\n" ) ;
#endif /* _DEBUG */  
            m_pServices[i].Close() ;
        } 
    }

    bool IsClosed() const
    {
        for( unsigned i = 0 ; i < m_nCurSize ; i ++ )
        { 
            if( ! m_pServices[i].IsClosed() )
                return false ;
        }

        return true ;
    }

#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif
   
    template< class TSpaceUse , class ... ARGs > 
    void RunNew( TSpaceUse usr , ARGs & ... args )
    {
        _runNew( usr , [ &args ... ]( void * p )->IT*{ return new(p) TC( args ... ) ; } ) ;
    }
    
    template< typename TE , class ... ARGs > 
    void RunNew( VO< TE > & usr , ARGs & ... args )
    {
        static_assert( std::is_base_of< TE , IT >::value , "IT must derived from Export type TE !" ) ;
        _runNewWithUser( usr , [ &args... ]( void * p )->IT*{ return new(p) TC( args ... ) ; } ) ;
    }


#ifdef _DEBUG
#pragma pop_macro("new") 
#endif

    void Close()
    {
        for( unsigned i = 0 ; i < m_nCurSize ; i ++ )
        {  
            m_pServices[i].Close() ; 
        }

        m_nCurSize  = 0 ;
        m_pLastFree = 0 ;
    }

private :
    template< class TSpaceUse , class TBuilder >
    void _runNew( TSpaceUse usr , TBuilder tb )
    { 
        if( m_pLastFree != NULL )
        {
            ServiceUnit * pTmpUnit = m_pLastFree ;
            m_pLastFree = pTmpUnit->GetPreFree() ;
            if( ! pTmpUnit->Run( this , usr , tb ) )
            {
                pTmpUnit->PushFree( m_pLastFree ) ;
                m_pLastFree = pTmpUnit ;
            }
        }
        else if( m_nCurSize == MC )
        {
            VASSERT_MSG( 0 , "需要的服务个数超过上限！ \r\n" ) ;

            return ;
        }
        else
        {
            ServiceUnit * pTmpUnit = & m_pServices[ m_nCurSize ] ;
            if( pTmpUnit->Run( this , usr , tb ) )
            {
                m_nCurSize ++ ;
            }
            else
            {
                pTmpUnit->PushFree( m_pLastFree ) ;
                m_pLastFree = pTmpUnit ;
            }
        }
    }
   
    template< class TOut , class TBuilder >
    void _runNewWithUser( VO< TOut > & usr , TBuilder tb )
    {
        _runNew( [&usr]( VI<  IT  >& spc ){ usr.Visit( spc ) ; } , tb ) ;
    } 

    static void ClearServiceSpace( IVSpace< IT > & spc )
    {
        ServiceUnit * pUnit = ServiceUnit::SpaceToUnit( &spc ) ;
        assert( !pUnit->IsClosed() ) ;

        TVServiceArray< IT , TC , MC > * pOwner = pUnit->GetContainer() ;

        pUnit->Close() ;
        pUnit->PushFree( pOwner->m_pLastFree ) ;
        pOwner->m_pLastFree = pUnit ;
    }  

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVServiceArray , IT , TC , MC ) ;
    unsigned      m_nCurSize  ;
    ServiceUnit * m_pLastFree ;
    ServiceUnit   m_pServices[ MC ] ;
} ;

template< class IT , class TC > 
class TVServiceArray< IT , TC , 0 >
{
private : 
    class ServiceUnit
    {
    public :
        ServiceUnit()
            :m_pToken(0)
        {
            InitSpace() ;
        }
        virtual ~ServiceUnit()
        {
#if _ITERATOR_DEBUG_LEVEL == 2 
            VASSERT_MSG( IsClosed() , "析构ServiceUnit< IT  >对象前请使用Close释放拥有的life对象，在某些特殊情况下可能会造成错误！\r\n" ) ;
#endif /* _DEBUG */  

            Close() ;
        }
 
        bool IsClosed() const
        {
            const IVSpace< IT > * pspc = getSpace() ;
            return ( getSpace()->pNxtRef == 0 ) ;
        }
         
        void Close()
        {
            CloseServiceFromServer( *getSpace() ) ;  
        }

        template< class TSpaceUse , class TBuilder >
        bool Run( TVServiceArray< IT , TC , 0 > * pContainer , TSpaceUse usr , TBuilder tb )
        {
            IVSpace< IT > * pspc = getSpace() ;

            assert( IsClosed() ) ;

            unsigned A = CLASS_BODY_OFFSET ;
            unsigned B = CLASS_BODY_SIZE ;

            pspc->pService      = tb( getService() ) ; 
            pspc->procFreeSrvc  = &TVServiceArray<IT,TC,0>::ClearServiceSpace ;  

            usr( * pspc ) ;

            if( ! pspc->pNxtRef )
            {
                assert( ! pspc->pNxtRef ) ; 
                ( reinterpret_cast< TC * >( pspc->pService ) )->~TC() ;
                pspc->pService = 0 ;
                return false ;
            }

            m_pToken = pContainer ;

            return true ;
        }

    private :
        static const size_t CLASS_BODY_OFFSET = sizeof( IVSpace< IT > ) ;
        static const size_t CLASS_BODY_SIZE  = sizeof( TC ) ;
        static const size_t CLASS_TOTAL_SIZE = CLASS_BODY_OFFSET + CLASS_BODY_SIZE ;

    public :
        ServiceUnit * GetPreFree()
        {
            assert( IsClosed() ) ;
            return reinterpret_cast< ServiceUnit * >( m_pToken ) ;
        } ;
        
        void PushFree( ServiceUnit * pre )
        {
            assert( IsClosed() ) ;
            m_pToken = pre ;
        }

        TVServiceArray< IT , TC , 0 > * GetContainer() 
        {
            assert( !IsClosed() ) ;
            return reinterpret_cast< TVServiceArray< IT , TC , 0 > * >( m_pToken ) ;
        }

        void MoveTo( ServiceUnit & other )
        {
            assert( !IsClosed() ) ;
            memcpy( other.m_space , m_space , CLASS_TOTAL_SIZE ) ;
            other.m_pToken = m_pToken ;
            IVSpace< IT > * pSpc = getSpace() ;
            
            pSpc->pNxtRef->pPreRef = other.getSpace() ;  
             
            IT * pNewSvr = other.getService() ;
             
            IVSpace< IT > * pNxt = other.getSpace() ;
            while( pNxt )
            {
                pNxt->pService = pNewSvr ;
                pNxt = pNxt->pNxtRef ;
            }


            InitSpace() ;
        } 

        static ServiceUnit * SpaceToUnit( IVSpace< IT > * spc )
        {
            char * p = reinterpret_cast< char * >( spc ) ;
            p -= offsetof( ServiceUnit , m_space ) ;
            return reinterpret_cast< ServiceUnit * >( p ) ;
        }

        void CloseServiceFromServer( IVSpace< IT > & spc )
        {
            IVSpace< IT > * pNxt = spc.pNxtRef ;

            if( pNxt )
            {
                if( pNxt->pNxtRef ) // 被占用
                {
                    pNxt->pNxtRef->pPreRef = 0 ;
                    VSpaceUtil::InitSpace( *pNxt ) ; 
                    VSpaceUtil::InitSpace( spc ) ; 
                } 
                else
                { 
                    spc.procFreeClnt( *pNxt ) ;
                    getService()->~TC() ;
                    VSpaceUtil::InitSpace( spc ) ;
                }
            }
        } 

        void InitSpace()
        { 
            std::memset( & m_space , 0 , CLASS_TOTAL_SIZE ) ;
        }  

        IVSpace< IT > * getSpace()
        {
            return reinterpret_cast< IVSpace< IT > * >( m_space ) ;
        }

        const IVSpace< IT > * getSpace() const
        {
            return reinterpret_cast< const IVSpace< IT > * >( m_space ) ;
        }

        TC * getService()
        {
            return reinterpret_cast< TC * >( m_space + CLASS_BODY_OFFSET ) ;
        }

        void * m_pToken ;
        char   m_space[ CLASS_TOTAL_SIZE ] ;
    } ;

public :
    typedef IT                   interf_type ;
    typedef TC                   imp_type ;
    typedef TVService< interf_type , imp_type > service_type ;
    typedef std::vector< service_type * >  svc_arr ;
    typedef TVServiceArray< IT , TC , 0 >  my_type ;

    TVServiceArray()
        : m_nDrivdCount(0)
    { 
    }
    virtual ~TVServiceArray()
    { 
#ifdef _DEBUG 
        std::for_each( m_pServices.begin() , m_pServices.end() , []( service_type * psvr ){
            VASSERT( psvr->IsClosed() ) ;
        }) ;
#endif       

        for( unsigned i = 0 ; i < m_pServices.size() ; i ++ ) 
            delete m_pServices[i] ;
    }
    
    bool IsClosed() const
    {
        for( unsigned i = 0 ; i < m_pServices.size() ; i ++ )
        { 
            if( ! m_pServices[i]->IsClosed() )
                return false ;
        }

        return true ;
    }
       
    template< class TSpaceUse , class ... ARGs > 
    void RunNew( TSpaceUse usr , ARGs & ... args )
    {
        _runNew( usr , [ &args ... ]( void * p )->IT*{ 
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif
            return new(p) TC( args ... ) ; 

#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
        } ) ;
    }
    
    template< typename TE , class ... ARGs > 
    void RunNew( VO< TE > & usr , ARGs & ... args )
    {
        static_assert( std::is_base_of< TE , IT >::value , "IT must derived from Export type TE !" ) ;
        _runNewWithUser( usr , [ &args... ]( void * p )->IT*{ 
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif
            return new(p) TC( args ... ) ; 
 #ifdef _DEBUG
#pragma pop_macro("new") 
#endif
       } ) ;
    }

    void Close()
    {
        std::for_each( m_pServices.begin() , m_pServices.end() , []( service_type * psvr ){
            psvr->Close() ; 
        }) ;
    }

public : 
    template< class TSpaceUse , class TBuilder >
    void _runNew( TSpaceUse usr , TBuilder tb )
    {   
        if( m_nDrivdCount < m_pServices.size() )
            m_pServices[ m_nDrivdCount ++ ]->_run( usr , tb ) ;
        else
        {
            if( vf_ispoweroftwo( m_pServices.size() ) )
            {
                vf_tidysvcarr( m_pServices , m_nDrivdCount ) ;

                if( m_nDrivdCount < m_pServices.size() )
                {
                    unsigned cur = m_nDrivdCount;
                    auto p = m_pServices[ cur ];
                    m_nDrivdCount++;
                    p->_run( usr , tb ) ;
                    return ;
                }
            }

            m_nDrivdCount++;
            m_pServices.push_back( new service_type ) ;
            auto p = m_pServices.back();
            p->_run( usr , tb ) ;
        } 
    }
    
    template< typename TE , class TBuilder >
    void _runNewWithUser( VO< TE > & usr , TBuilder tb )
    {
        _runNew( [&usr]( VI<  IT  >& spc ){ usr.Visit( spc ) ; } , tb ) ;
    } 
     
    static void ClearServiceSpace( IVSpace< IT > & spc )
    {
        ServiceUnit * pUnit = ServiceUnit::SpaceToUnit( &spc ) ;
        assert( !pUnit->IsClosed() ) ;

        my_type * pOwner = pUnit->GetContainer() ;

        pUnit->Close() ;
        pUnit->PushFree( pOwner->m_pLastFree ) ;
        pOwner->m_pLastFree = pUnit ;
    }  

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVServiceArray , IT , TC ) ;
    
    unsigned  m_nDrivdCount ;
    svc_arr   m_pServices   ;
} ;
 
template< class IT , class TC > 
class TVServiceArray< IT , TC , 1 >
{
public :
    TVServiceArray()
    { 
    }
    virtual ~TVServiceArray()
    {  
        VASSERT( m_svc.IsClosed() ) ;   
    }
    
    bool IsClosed() const
    {
        return m_svc.IsClosed() ;
    }

#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif
   
    template< class TSpaceUse , class ... ARGs > 
    void RunNew( TSpaceUse usr , ARGs & ... args )
    {
        _runNew( usr , [ &args ... ]( void * p )->IT*{ return new(p) TC( args ... ) ; } ) ;
    }
    
    template< typename TE , class ... ARGs > 
    void RunNew( VO< TE > & usr , ARGs & ... args )
    {
        static_assert( std::is_base_of< TE , IT >::value , "IT must derived from Export type TE !" ) ;
        _runNewWithUser( usr , [ &args... ]( void * p )->IT*{ return new(p) TC( args ... ) ; } ) ;
    }

#ifdef _DEBUG
#pragma pop_macro("new") 
#endif

    void Close()
    {
        m_svc.Close() ; 
    }

private : 
    template< class TSpaceUse , class TBuilder >
    void _runNew( TSpaceUse usr , TBuilder tb )
    {   
        m_svc._run( usr , tb ) ;
    }
    
    template< typename TE , class TBuilder >
    void _runNewWithUser( VO< TE > & usr , TBuilder tb )
    {
        _runNew( [&usr]( VI<  IT  >& spc ){ usr.Visit( spc ) ; } , tb ) ;
    } 
 

private :
    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVServiceArray , IT , TC ) ;

    TVService< IT , TC > m_svc ;
} ;
//
//template< class Trait >
//struct TVCombSvcOffsetCalculator
//{ 
//    template< unsigned ID > 
//    static unsigned offsetOfSpace()
//    {
//        return offsetOfSpace< ID + 1 >() + sizeof( IVSpace< Trait::DepSpcType<ID>::svc_type > ) ;
//    }
//
//    template<> 
//    static unsigned offsetOfSpace< Trait::DepCount >()
//    {
//        return 0 ;
//    } 
//
//    template< unsigned ID > 
//    static unsigned revOffsetOfSpace()
//    {
//        return revOffsetOfSpace< ID - 1 >() + sizeof( IVSpace< Trait::DepSpcType< ID - 1 >::svc_type > ) ;
//    }
//
//    template<> 
//    static unsigned revOffsetOfSpace<0>()
//    {
//        return 0 ;
//    }
//} ;
//
//template< class TDerived , class Trait , template< unsigned > class TraitDepSpcType = Trait::DepSpcType >
//class VMultiClient
//{
//public :
//    typedef Trait ServiceTrait ; 
//
//protected :
//    template< unsigned ID >
//    typename TraitDepSpcType<ID>::svc_type & GetService()
//    {
//        char * p = reinterpret_cast< char * >( static_cast< TDerived * >( this ) ) ; 
//        IVSpace< TraitDepSpcType<ID>::svc_type > * pscp =
//            reinterpret_cast< IVSpace< TraitDepSpcType<ID>::svc_type > * >( p - TVCombSvcOffsetCalculator< ServiceTrait >::offsetOfSpace< ID >() ) ;
//        return * pscp->pService ;
//    }
//} ;
//
//template< class TData , template< unsigned > class TraitDepSpcType = TData::ServiceTrait::DepSpcType >
//class TVCombination
//{
//public :
//    TVCombination()
//    {  
//        m_data.flag = 0 ;
//    }
//
//    ~TVCombination()
//    { 
//        Destory() ;
//    }
//    
//    typedef TVCombination<TData , TraitDepSpcType>  SelfType ;
//    typedef typename TData::ServiceTrait            ServiceTrait    ;
//    static const unsigned FLAG_OBJEXIST = ( ( 1 << ServiceTrait::DepCount ) - 1 ) ; 
//
//private : 
//    template< unsigned ID > void _releaseService()
//    { 
//        if( 0 != ( m_data.flag & ( 1 << ( ID ) ) ) )
//        {
//            IVSpace< TraitDepSpcType< ID >::svc_type > * pspc = m_data.spcList.UseSpace< ID , IVSpace< TraitDepSpcType< ID >::svc_type > >() ;
//            m_data.flag &= ( ~ ( 1 << ( ID ) ) ) ;
//            pspc->pPreRef->procFreeSrvc( * pspc->pPreRef ) ;  
//        }
//    }
//    
//    template< unsigned ID >
//    void _releaseServiceIterate()
//    { 
//        _releaseService< ID >() ;
//        _releaseServiceIterate< ID + 1 >() ;
//    }
//
//    template<>
//    void _releaseServiceIterate< ServiceTrait::DepCount >()
//    {
//    }
//
//
//    bool _isDataActive() const
//    {
//        return m_data.flag == FLAG_OBJEXIST ; 
//    }
//
//    void _releaseData()
//    {
//        if( _isDataActive() )
//            ( reinterpret_cast< TData * >( m_data.pObj ) )->~TData() ; 
//    }
//
//public :
//    void Destory()
//    {
//        _releaseData() ;
//        _releaseServiceIterate< 0 >() ;
//    } 
//
//    template< unsigned ID >
//    void Attach( IVSpace< typename TraitDepSpcType< ID >::svc_type > & spc )
//    {
//        _releaseData() ;
//        _releaseService< ID >() ;
//
//        IVSpace< TraitDepSpcType< ID >::svc_type > * pspc = m_data.spcList.UseSpace< ID , IVSpace< TraitDepSpcType< ID >::svc_type > >() ;
//
//        VASSERT( spc.pService != NULL ) ;
//        pspc->pService      = spc.pService ;
//        pspc->pPreRef       = & spc        ;
//        pspc->pNxtRef       = 0              ;
//        pspc->procFreeClnt  = TVCombination< TData >::ClearClientSpace< ID > ; 
//        pspc->procFreeSrvc  = spc.procFreeSrvc ;
//
//        spc.pNxtRef  = pspc                  ;
//        spc.procFreeClnt = pspc->procFreeClnt ;
//
//        m_data.flag |= ( 1 << ( ID ) ) ;
//
//        if( _isDataActive() )
//        { 
//#ifdef _DEBUG
//#pragma push_macro("new")
//#undef new
//#endif 
//            new( m_data.pObj ) TData() ;
//#ifdef _DEBUG
//#pragma pop_macro("new") 
//#endif
//        }
//    } ;
//     
//    struct Data
//    {
//        unsigned flag ;
//        typename ServiceTrait::SpaceList spcList ;
//        char     pObj[ sizeof( TData ) ] ;
//    } ; 
//    
//private :
//    template< unsigned ID >
//    static void ClearClientSpace( IVSpace< typename TraitDepSpcType< ID >::svc_type > & spc )
//    {     
//        SelfType * pObj = reinterpret_cast< SelfType * >(
//                                         reinterpret_cast< char * >( &spc ) 
//                                       - TVCombSvcOffsetCalculator< ServiceTrait >::revOffsetOfSpace< ID >() // ::DepSpcType< ID >::szOffset
//                                       - offsetof( Data , spcList )
//                                       - offsetof( SelfType , m_data ) ) ; // offsetof( SelfType , m_data ) ) ;
//        
//        pObj->_releaseData() ;
//
//        VASSERT( 0 != ( pObj->m_data.flag & ( 1 << ( ID ) ) ) ) ;
//        pObj->m_data.flag &= ( ~ ( 1 << ( ID ) ) ) ; 
//    } 
//
//private :
//    Data m_data ;
//} ;

//
//template< class Derived , class T1 , class T2 >
//class TVCombo
//{
//public :
//    TVCombo(){}
//    ~TVCombo(){}
//
//public :
//    void SetFirst( IVSpace< T1 > &spc )
//    {
//        m_proxy1.Create( spc , [ this ]( T1 & fst ){
//            _linkOnFst( fst ) ;
//        } ) ;
//    } ;
//
//    void SetSecond( IVSpace< T2 > & spc )
//    {
//        m_proxy2.Create( spc , [ this ]( T2 & scd ){
//            _linkOnScd( scd ) ;
//        }) ;
//    } 
//
//private :
//    void _linkOnFst( T1 & fst )
//    {
//        m_proxy2.UseServer( [ this , &fst]( T2 & scd ){
//            static_cast< Derived * >( this )->OnCombo( fst , scd ) ;
//        } ) ;
//    }
//
//    void _linkOnScd( T2 & scd )
//    {
//        m_proxy1.UseServer( [ this , &scd ]( T1 & fst ){
//            static_cast< Derived * >( this )->OnCombo( fst , scd ) ;
//        }) ;
//    } 
//
//private :
//    TVServiceProxy< T1 > m_proxy1 ;
//    TVServiceProxy< T2 > m_proxy2 ;
//} ;

//template< class T >
//class TVJunction 
//{
//public :
//    void SetProvider( IVSpace< IVProvider< T > > &spc )
//    {
//        _comb.Attach<0>( spc ) ;
//    } 
//
//    void SetRecipient( IVSpace< IVRecipient< T > > & spc )
//    {
//        _comb.Attach<1>( spc ) ;
//    }   
//
//private :
//    class Linker : public VMultiClient< Linker , TVCombSvcTrait< IVProvider< T > , IVRecipient< T > > >
//    {
//    public :
//        Linker()
//        {
//            IVProvider < T > & svcProv = GetService<0>() ; 
//            IVRecipient< T > & svcRecp = GetService<1>() ;
//            
//            svcProv.Apply( VCMN::TVUserAdaptor< IVSpace< T > >( [ &svcRecp ]( IVSpace< T > & spcSvc ){
//                svcRecp.Offer( spcSvc ) ;
//            })) ; 
//        }
//        ~Linker()
//        {
//        }
//    } ;
//
//private : 
//    TVCombination< Linker > _comb ;
//} ;
 
template< class T >
class TVServiceProxyArray : TVSCArrayTmpl< TVServiceProxy< T > >
{
public :
    TVServiceProxyArray()
    {}

    ~TVServiceProxyArray()
    {
    }
 
public :
    void Add( VI<  T  >& spc )
    {
        this->_additem( [ &spc ]( TVServiceProxy< T > & sp ){
            sp.Create( spc ) ;
        } ) ;
    }
    
    template< class F >
    void Add( IVSpace< T > & spc , F f )
    {
        _additem( [ &spc , &f ]( TVServiceProxy< T > & sp ){
            sp.Create( spc , f ) ;
        } ) ;
    } 

    template< class F >
    void TidyTravel( F f )
    {
        this-> _tidy() ;
        this->_visititem( [ f ]( TVServiceProxy< T > & sp ){
            sp.UseServer( f ) ;
        } ) ; 
    }

    void Destory()
    {
        this->_clear( []( TVServiceProxy< T > & sp ){
            sp.Destory() ; 
        } ) ; 
    } 
} ;

template< class IT , class TC = IT >
class TVSafeScopedObj
{
public :
    TVSafeScopedObj(){}
    ~TVSafeScopedObj()
    {
        DebugCheckClean() ;
        Clear();
    }
    
    void DebugCheckClean()
    {
#if _ITERATOR_DEBUG_LEVEL == 2 
        VASSERT_MSG( IsNull() , "析构TVService< IT  >对象前请使用Clear释放拥有的对象，在某些特殊情况下可能会造成错误！\r\n" ) ;
#endif /* _DEBUG */  
    }

public :   
    void Clear()
    {
        m_svc.Close() ;
    } 
    
    template< class ... ARGs > 
    void Renew( ARGs & ... args )
    {
        m_svc.Run( [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; }  , args ... ) ;
    } 

    //void Renew( )
    //{
    //    m_svc.Run( [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //}  
    //template< class ARG1 > 
    //void Renew( ARG1 arg1 )
    //{
    //    m_svc.Run< ARG1 >( arg1 ,  [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //}  
    //template< class ARG1 , class ARG2 > 
    //void Renew( ARG1 arg1 , ARG2 arg2 )
    //{
    //    m_svc.Run< ARG1 , ARG2 >( arg1 , arg2 , [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //}  
    //template< class ARG1 , class ARG2 , class ARG3 > 
    //void Renew( ARG1 arg1 , ARG2 arg2 , ARG3 arg3 )
    //{
    //    m_svc.Run< ARG1 , ARG2 , ARG3 >( arg1 , arg2 , arg3 , [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //}  
    //template< class ARG1 , class ARG2 , class ARG3 , class ARG4 > 
    //void Renew( ARG1 arg1 , ARG2 arg2 , ARG3 arg3 , ARG4 arg4 )
    //{
    //    m_svc.Run< ARG1 , ARG2 , ARG3 , ARG4 >( arg1 , arg2 , arg3 , arg4 , [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //}  
    //template< class ARG1 , class ARG2 , class ARG3 , class ARG4 , class ARG5 > 
    //void Renew( ARG1 arg1 , ARG2 arg2 , ARG3 arg3 , ARG4 arg4 , ARG5 arg5 )
    //{
    //    m_svc.Run< ARG1 , ARG2 , ARG3 , ARG4 , ARG5 >( arg1 , arg2 , arg3 , arg4 , arg5 , [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //}  
    //template< class ARG1 , class ARG2 , class ARG3 , class ARG4 , class ARG5 , class ARG6 > 
    //void Renew( ARG1 arg1 , ARG2 arg2 , ARG3 arg3 , ARG4 arg4 , ARG5 arg5 , ARG6 arg6 )
    //{
    //    m_svc.Run< ARG1 , ARG2 , ARG3 , ARG4 , ARG5 , ARG6 >( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , [this]( VI<  IT  >& spc ){ m_proxy.Create( spc ) ; } ) ;
    //} 
     
    template< class F            > void Use( F f         ) { m_proxy.UseServer( f       ) ; }
    template< class F , class UF > void Use( F f , UF uf ) { m_proxy.UseServer( f , uf  ) ; }
    bool IsNull() const
    {
        return m_proxy.IsClosed() ;
    } ;

    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( TVSafeScopedObj , IT , TC ) ;

private :
    TVService< IT , TC >  m_svc   ;
    TVServiceProxy< IT >  m_proxy ;
} ;

namespace VCMN
{
    template< class T >
    class TVProxyUser : public IVUser< IVSpace< T > >
    {
    public :
        TVProxyUser( TVServiceProxy< T > & proxy ):m_proxy(proxy){} 

    protected :
        virtual void Visit( IVSpace< T > & t )
        {
            m_proxy.Create( t );
        }

    protected :
        TVServiceProxy< T > & m_proxy ;
    } ;

    template< class T , class F >
    class TVProxyUser2 : public IVUser< IVSpace< T > >
    {
    public :
        TVProxyUser2( TVServiceProxy< T > & proxy , F f ):m_proxy(proxy),m_func(f){} 

    protected :
        virtual void Visit( IVSpace< T > & t )
        {
            m_proxy.template Create< F >( t , m_func );
        }

    protected :
        TVServiceProxy< T > & m_proxy ;
        F m_func ;
    } ;

    template< class T , class F , class UF >
    class TVProxyUser3 : public IVUser< IVSpace< T > >
    {
    public :
        TVProxyUser3( TVServiceProxy< T > & proxy , F f , UF uf ):m_proxy(proxy),m_func(f),m_ferr(uf){} 

    protected :
        virtual void Visit( IVSpace< T > & t )
        {
            m_proxy.template Create< F , UF >( t , m_func , m_ferr ) ;
        }

    protected :
        TVServiceProxy< T > & m_proxy ;
        F  m_func ;
        UF m_ferr ;
    } ;

    template< class T >
    TVProxyUser< T > ProxyToUser( TVServiceProxy< T > & proxy )
    {
        return TVProxyUser< T >( proxy ) ;
    }  
    template< class T , class F >
    TVProxyUser2< T , F > ProxyToUser( TVServiceProxy< T > & proxy , F f )
    {
        return TVProxyUser2< T , F >( proxy , f ) ;
    }  
    template< class T , class F , class UF >
    TVProxyUser3< T , F , UF > ProxyToUser( TVServiceProxy< T > & proxy , F f , UF uf )
    {
        return TVProxyUser3< T , F , UF >( proxy , f , uf ) ;
    }  

    template< class T > struct VTSvcProxyTrait ;
    template< class T > struct VTSvcProxyTrait< TVServiceProxy< T > >
    {
        typedef T  type ;
        typedef typename TVDataTrait< T >::reference ref_type ;
    } ; 
};

template< class TImp , class ... TDeps > struct TVClientUtil ;

template< class TImp >
struct TVClientUtil< TImp >
{
    static const unsigned SPC_SELF_SIZE  = 0 ;
    static const unsigned SPC_TOTAL_SIZE = 0 ;

    static bool IsClosed( const char * p )
    {
        return true ;
    }  
    
    template< unsigned LEN >
    static void InitSpace( char (&p)[LEN] )
    {  
    }   

    static void Destory( const char * p )
    {
    }

    template< bool bval >
    static void AssertPreValidation( const char * p )
    { 
    }

    template< bool bval >
    static void AssertNxtValidation( const char * p )
    { 
    }
    
    static void AssertCreation()
    {
    }

    template< class F >
    static void BuildInstace( char * p , F f )
    {  
        f( reinterpret_cast< TImp * >( p ) ) ;
    } 
    
    template< typename ... TPres >
    static void FillSpaces( const char * pspc , IVSpace< TPres > &  ... pres ) 
    {
    }
    
    static void FreeService( char * p )
    {
    }

    static void RemoveOccupy( char * p )
    {
    }
} ;

template< class TImp , class T , class ... TDeps >
struct TVClientUtil< TImp , T , TDeps ... > 
{
    typedef TVClientUtil< TImp , TDeps ... > sub_data_type ;

    static const unsigned SPC_SELF_SIZE   = sizeof( IVSpace< T > ) ; 
    static const unsigned SPC_TOTAL_SIZE = SPC_SELF_SIZE + sub_data_type::SPC_TOTAL_SIZE ;
    static const unsigned CLASS_TOTAL_SIZE = SPC_TOTAL_SIZE + sizeof( TImp ) ;
     
    static const TImp * getClient( const char * p ) 
    {
        return reinterpret_cast< const TImp * >( p + SPC_TOTAL_SIZE ) ;
    }

    static  TImp * getClient( char * p ) 
    {
        return reinterpret_cast< TImp * >( p + SPC_TOTAL_SIZE ) ;
    }

    static bool IsClosed( const char * p )
    { 
        const IVSpace< T > * pspc = c2s( p ) ; 

        if (pspc->pPreRef)
        { 
            sub_data_type::AssertPreValidation< false >(p + SPC_SELF_SIZE); 
            return false;
        }
         
        sub_data_type::AssertPreValidation< true >(p + SPC_SELF_SIZE); 

        return true; 
    } 
    
    static void RemoveOccupy( char * p )
    {
        IVSpace< T > * pspc = c2s( p ) ;

        assert( pspc->pPreRef && pspc->pNxtRef ) ;
        pspc->pNxtRef->pPreRef = 0 ; 
        VSpaceUtil::InitSpace( * pspc->pPreRef ) ;

        sub_data_type::RemoveOccupy( p + SPC_SELF_SIZE ) ;
    }
        
    template< unsigned LEN >
    static void InitSpace( char (&p)[LEN] )
    { 
        static_assert( LEN == CLASS_TOTAL_SIZE , "" ) ;
        std::memset( p , 0 , CLASS_TOTAL_SIZE ) ;
    }   
     
    template< unsigned LEN >
    static void Destory( char (&p)[LEN] )
    {
        static_assert( LEN == CLASS_TOTAL_SIZE , "" ) ;

        const IVSpace< T > * pspc = c2s( p ) ;  

        if (pspc->pPreRef)
        {
            sub_data_type::AssertPreValidation< false >(p + SPC_SELF_SIZE);
            
            if( pspc->pNxtRef ) // 被占用
            {
                sub_data_type::AssertNxtValidation< false >(p + SPC_SELF_SIZE);

                RemoveOccupy( p ) ;  
                InitSpace( p ) ;
            }
            else
            {
                sub_data_type::AssertNxtValidation< true >(p + SPC_SELF_SIZE);
                
                getClient( p )->~TImp() ;  
                FreeService( p  ) ;
                std::memset( p , 0 , CLASS_TOTAL_SIZE ) ;
            }
        }
        else
        {
            sub_data_type::AssertPreValidation< true >(p + SPC_SELF_SIZE); 
        }
    }

    template< bool bval >
    static void AssertPreValidation( const char * p )
    {
#ifdef _DEBUG
        const IVSpace< T > * pspc = c2s( p ) ; 
        assert( bval == ( pspc->pPreRef == 0 ) ) ;
#endif   
    }

    template< bool bval >
    static void AssertNxtValidation( const char * p )
    {
#ifdef _DEBUG
        const IVSpace< T > * pspc = c2s( p ) ; 
        assert( bval == ( pspc->pNxtRef == 0 ) ) ;
#endif   
    }

    static IVSpace< T > * c2s( char * pspc )
    {
        return reinterpret_cast< IVSpace< T > * >( pspc ) ;
    }

    static const IVSpace< T > * c2s( const char * pspc ) 
    {
        return reinterpret_cast< const IVSpace< T > * >( pspc ) ;
    }

    template< class F >
    static void BuildInstace( char * p , IVSpace< T > &  spc , IVSpace< TDeps > &  ... TOthers , F f )
    {
        sub_data_type::BuildInstace( p + SPC_SELF_SIZE , TOthers ... , [ &spc , f ]( TImp * pImp , TDeps & ... deps ){
            f( pImp , * spc.pService , deps ... ) ;
        } ) ;
    }
     
    static void AssertCreation( IVSpace< T > &  spc , IVSpace< TDeps > &  ... TOthers )
    {
#ifdef _DEBUG
        assert( ! spc.pPreRef   ) ; // 必须是服务提供者
        assert(   spc.pService  ) ; // 服务必须存在
        sub_data_type::AssertCreation( TOthers ... ) ;
#endif
    }
     
    template< typename ... TPres >
    static void FillSpaces( char * pspc , IVSpace< T > &  spc , IVSpace< TDeps > &  ... TOthers , IVSpace< TPres > & ... pres ) 
    {
        IVSpace< T > * pdest = c2s( pspc ) ;

        pdest->pService = spc.pService;
        pdest->pPreRef = &spc;
        pdest->pNxtRef = 0;
        pdest->procFreeClnt = ClearClientSpace< TPres ... > ;
        pdest->procFreeSrvc = spc.procFreeSrvc ; 

        spc.pNxtRef  = pdest                    ;
        spc.procFreeClnt = pdest->procFreeClnt  ;

        sub_data_type::FillSpaces( pspc + SPC_SELF_SIZE , TOthers ... , pres ... , spc ) ;
    }

    template< typename ... TPres >
    static void ClearClientSpace( IVSpace< T > & spc )
    {
        const unsigned presize = TVClientUtil< TImp , TPres ... >::SPC_TOTAL_SIZE ; 

        unsigned myoff = presize ;
        char * pthis = reinterpret_cast< char * >( &spc ) ;
        char * ppre = pthis - myoff ;
        char * pnxt = pthis + SPC_SELF_SIZE ;
         
        IVSpace< T > * pSpc = & spc ; 

        if( pSpc->pPreRef )
        {
            sub_data_type::AssertPreValidation< false >( pnxt ) ;

            TImp * p = reinterpret_cast< TImp * >( pthis + SPC_TOTAL_SIZE ) ; 
            p->~TImp();
             
            TVClientUtil< TImp , TPres ... >::FreeService( ppre ) ;
            TVClientUtil< TImp , TDeps ... >::FreeService( pnxt ) ;

            std::memset( ppre , 0 , presize + CLASS_TOTAL_SIZE ) ;
        } 
        else
        {
            sub_data_type::AssertPreValidation< true >( pnxt ) ;
        }
    }
     
    static void FreeService( char * p )
    {
        const IVSpace< T > * pspc = c2s( p ) ; 
        VASSERT( pspc->pPreRef != 0 ) ;
        pspc->procFreeSrvc( *pspc->pPreRef ) ; 
        sub_data_type::FreeService( p + SPC_SELF_SIZE ) ;
   }
} ;

template< class TImp , class ... TDeps >
class TVClient
{
private :
    typedef TVClientUtil< TImp , TDeps ... > spc_data_type ;
    static const unsigned CLASS_TOTAL_SIZE = spc_data_type::CLASS_TOTAL_SIZE ;

    char m_space[ CLASS_TOTAL_SIZE ] ; 

    template< class FCreate >
    void _create( IVSpace< TDeps > &  ... spcs , FCreate fNew )
    { 
        spc_data_type::AssertCreation( spcs ... ) ; 
        Destory() ; 
        spc_data_type::FillSpaces( m_space , spcs ... ) ;
        spc_data_type::BuildInstace( m_space , spcs ... , fNew ) ;  
    }

public :
    TVClient()
    {
        spc_data_type::InitSpace( m_space ) ;
    }
    ~TVClient()
    { 
        Destory() ;
        spc_data_type::AssertPreValidation< true >( m_space ) ;
        //static_assert(0 ,"") ;
    } 
       
    bool IsClosed() const
    {
        return spc_data_type::IsClosed( m_space ) ; 
    }
    
    template< typename ... TARGS >
    void Create( IVSpace< TDeps > &  ... spcs , TARGS & ... args )
    { 
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif 
        auto f = [ &args ...]( TImp * p , TDeps & ... psvrs )
        {
            new(p) TImp( psvrs ... , args ... ) ;
        } ;

        _create( spcs ..., f ) ;

#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
    }

    void Destory()
    {  
        spc_data_type::Destory( m_space ) ; 
    } 
} ;

template< class TImp >
class TVClient< TImp >
{
private : 
    static const unsigned CLASS_TOTAL_SIZE = sizeof( TImp ) ;
    TImp * m_pinst ;
    char m_value[ CLASS_TOTAL_SIZE ] ; 
     
public :
    TVClient()
    {
        m_pinst = 0 ;
    }
    ~TVClient()
    { 
        Destory() ;
    } 
       
    bool IsClosed() const
    {
        return m_pinst == 0 ;
    }
    
    template< typename ... TARGS >
    void Create( TARGS & ... args )
    { 
        Destory() ;

#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif  
        m_pinst = getClient( m_value ) ;  
        new( m_pinst ) TImp( args ... ) ; 

#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
    }

    void Destory()
    {  
        if( m_pinst )
            m_pinst->~TImp() ; 
        m_pinst = 0 ;
    } 

private :      
    static const TImp * getClient( const char * p ) 
    {
        return reinterpret_cast< const TImp * >( p ) ;
    }

    static  TImp * getClient( char * p ) 
    {
        return reinterpret_cast< TImp * >( p ) ;
    }
} ;

template< class TImp , class ... TDeps >
class TVClientArray : TVSCArrayTmpl< TVClient< TImp , TDeps... > >
{
public:
    TVClientArray(){}
    ~TVClientArray(){}

public:
    template< class... Args >
    void Add( VI< TDeps >&... spcs , Args&... args )
    {
        _additem( [ &spcs... , &args... ]( TVClient< TImp , TDeps... > & clnt ) {
            clnt.Create( spcs... , args... );
        } );
    }

    void Destory()
    {
        _clear( []( TVClient< TImp , TDeps... > & clnt ) {
            clnt.Destory();
        } );
    }
};

#define VD_P2U  VCMN::ProxyToUser 

#define VD_P2L( proxy ) [ & proxy ]( VI<  typename VCMN::VTSvcProxyTrait< decltype( proxy ) >::type >& spc ){ proxy.Create( spc ); }
#define VD_MP2L( proxy ) [ this ]( VI<  typename VCMN::VTSvcProxyTrait< decltype( proxy ) >::type >& spc ){ proxy.Create( spc ); }


