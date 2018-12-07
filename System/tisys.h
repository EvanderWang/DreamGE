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


    {
        outVal.Input( *static_cast< IVSlot< TOUTPUT > * >( this ) );
    }

    // IVSlot< VSConstBuffer< const T * > >
    virtual void Trace( VI< IVTracer > & spc )
    {
        m_listeners.Add( spc );
    }
    virtual void GetData( VO< IVDataProv< TOUTPUT > > & usr )
    {
        m_svcProv.RunNew( usr , *this );
    }

    // IVDirtyObject
    virtual void CleanAndDiffuse()
    {
        if ( m_bLengthChanged )   
            _updateList();

        m_listeners.TidyTravel( []( auto&t ) {t.OnChanged(); } );
    }

private:
    typedef TVSwitcher< T > my_type;
    struct VItemTracer : public IVTracer
    {
        my_type & me;
        VItemTracer( my_type & m ) :me( m )
        {}
        ~VItemTracer()
        {
            me._notifyChange( true );
        }
        virtual void OnChanged()
        {
            me._notifyChange( false );
        }
    };
    struct ProvAdp : public IVDataProv< TOUTPUT >
    {
        my_type & me;
        ProvAdp( my_type & m ) :me( m ){}
        ~ProvAdp(){}
        void Apply( VO< TVConstRef< TOUTPUT > > & usr )
        {
            me._applyExpData( usr );
        }
    };

private:
    void _notifyChange( bool bLenChanged )
    {
        m_bLengthChanged |= bLenChanged;

        if ( TVServer< IVDirtyObject >::IsClosed() )
        {
            TVServer< IVDirtyObject >::Run( [ this ]( auto & spcDirty ) {
                m_timeline.RegisterDirty( spcDirty );
            } );
        }
    }
    void _applyExpData( VO< TVConstRef< TOUTPUT > > & usr )
    {
#ifdef VD_NOT_CARE_PERFORMANCE
        _chechData();
#endif
        m_svcExp.RunNew( usr , m_expValue );
    }
    void _updateList()
    {
        m_svcData.Close();
        m_expContainer.clear();

        m_itemProv.UseServer( [ this ]( IVDataProv< T > & dp ) {
            dp.Apply( VD_L2U( [ this ]( VI< TVConstRef< T > > & spcItem ) {
                m_svcData.RunNew( [ this ]( VI< TVSwitcherData< T > >& spc ) {
                    m_svcDataHdlr.Add( spc );
                } , spcItem , m_expContainer );
            } ) );
        } );

        m_expValue.m_Vaild = !m_expContainer.empty();
        if ( !m_expContainer.empty() )
        {
            m_expValue.m_pData = *(&m_expContainer[ 0 ]);
        }

#ifdef VD_NOT_CARE_PERFORMANCE
        _chechData();
#endif
    };

#ifdef VD_NOT_CARE_PERFORMANCE
    void _chechData()
    {
        VASSERT( m_expContainer.size() < 2 );

        std::for_each( m_expContainer.begin() , m_expContainer.end() , []( const T * p ) {
            VASSERT( p != 0 );
        } );
    }
#endif

private:
    VN_ELEMSYS::TVSysImpTimeline                     & m_timeline;
    TVServiceProxyArray< IVTracer >                    m_listeners;
    TVService< IVTracer , VItemTracer >                m_tracers;
    bool                                               m_bLengthChanged;
    TVServiceProxy< IVDataProv< T > >                  m_itemProv;

    TOUTPUT                                            m_expValue;
    std::vector< const T * >                           m_expContainer;
    TVServiceArray< TVSwitcherData< T > >              m_svcData;
    TVServiceProxyArray< TVSwitcherData< T > >         m_svcDataHdlr;
    // 
    TVServiceArray< TVConstRef< TOUTPUT > >            m_svcExp;
    TVServiceArray< IVDataProv< TOUTPUT > , ProvAdp  > m_svcProv;
};

template< typename T >
class TVSvcSwitcher
{
public:
    TVSvcSwitcher(){}
    ~TVSvcSwitcher(){}

public:
    typedef VS01Buffer < T >         TEXP;
    typedef IVConverter< TEXP , T  > interf_type;
    typedef TVSwitcher < T         > imp_type;

public:
    void Close()
    {
        m_svc.Close();
    }
    void Elapse()
    {
        m_timeline.ClearDirty();
    }
    void RunService( VO< interf_type > & pipe )
    {
        m_svc.RunNew( pipe , m_timeline );
    }
    template< typename T >
    void Init( const T & v )
    {
    }

private:
    VN_ELEMSYS::TVSysImpTimeline             m_timeline;
    TVServiceArray< interf_type , imp_type > m_svc;
};

// Collectors 
template< typename T >
class TVCollectorData : public VSConstBuffer< const T * >
{
public :
    TVCollectorData( VI< TVConstRef< T > > & spcValue , std::vector< const T * > & arr )
    {
        m_svcList.Run( [ this , & spcValue ]( VI< TVRef< std::vector< const T * > > > & spcArr ){
            m_clnt.Create( spcValue , spcArr ) ;
        } , arr ) ;
    }  

    ~TVCollectorData()
    {
        m_svcList.Close() ;
    }

private :
    struct VKeeper
    {
    public :
#ifdef VD_NOT_CARE_PERFORMANCE
        VKeeper( const TVConstRef< T > & val , TVRef< std::vector< const T * > > & ctr )
            : m_container( ctr.m_ref ) 
        {
            idxInContainer = m_container.size() ;
            m_container.push_back( & val.m_ref ) ;
        }
        ~VKeeper()
        {
            VASSERT( m_container.size() > idxInContainer ) ;
            m_container[ idxInContainer ] = 0 ;
        }
        unsigned idxInContainer ;
        std::vector< const T * > & m_container ;
#else
        VKeeper( const TVConstRef< T > & val , TVRef< std::vector< const T * > > & ctr ) 
        { 
            ctr.m_ref.push_back( & val.m_ref ) ;
        }
        ~VKeeper()
        {
        } 
#endif
    } ;

private :
    TVClient< VKeeper , TVConstRef< T > , TVRef< std::vector< const T * > > > m_clnt ;
    TVService< TVRef< std::vector< const T * > > > m_svcList ;
} ;

template< typename T >
class TVCollector : public IVConverter< VSConstBuffer< const T * > , T > 
                  , IVSlot< VSConstBuffer< const T * > >
                  , TVServer< IVDirtyObject >  
{
public :
    typedef VSConstBuffer< const T * > TOUTPUT ;

    TVCollector( VN_ELEMSYS::TVSysImpTimeline & tl )
        :m_timeline(tl)
    {
        _notifyChange( true ) ;
    }
    ~TVCollector()
    {
        m_tracers.Close() ;
        m_svcData.Close() ;
        m_svcExp .Close() ;
        m_svcProv.Close() ; 
        TVServer< IVDirtyObject > ::Close() ;  
    }
private :
    virtual void Input ( IVSlot< T > & inVal )
    {
        m_tracers.RunNew( [ &inVal ]( auto & spc ){ inVal.Trace( spc ) ; } , *this ) ;
        inVal.GetData( VD_L2U( [ this ]( VI< IVDataProv< T > > & spc ){
            m_itemProvList.Add( spc ) ;
        } ) ) ;
        _notifyChange( true ) ;
    }
    virtual void Output( IVInputPort< TOUTPUT > & outVal )
    {
        outVal.Input( * static_cast< IVSlot< TOUTPUT > * >( this ) ) ;
    }

    // IVSlot< VSConstBuffer< const T * > >
    virtual void Trace   ( VI< IVTracer > & spc ) 
    {
        m_listeners.Add( spc ) ;
    }
    virtual void GetData ( VO< IVDataProv< TOUTPUT > > & usr ) 
    {
        m_svcProv.RunNew( usr , *this ) ;
    } 
      
    // IVDirtyObject
    virtual void CleanAndDiffuse()
    { 
        if( m_bLengthChanged ) // ���鳤�ȷ������   
            _updateList() ;  

        m_listeners.TidyTravel( [](auto&t){t.OnChanged() ; } ) ;
    }

private :
    typedef TVCollector< T > my_type;
    struct VItemTracer : public IVTracer
    { 
        my_type & me ;
        VItemTracer( my_type & m ) :me( m ){}
        ~VItemTracer(){ me._notifyChange( true )  ; } 
        virtual void OnChanged() 
        {
            me._notifyChange( false ) ;
        }
    };
    struct ProvAdp : public IVDataProv< TOUTPUT >
    {
        my_type & me;
        ProvAdp( my_type & m ) :me( m ){}
        ~ProvAdp(){}
        void Apply( VO< TVConstRef< TOUTPUT > > & usr )
        {
            me._applyExpData( usr );
        }
    };

private :
    void _notifyChange( bool bLenChanged )
    {
        m_bLengthChanged |= bLenChanged ;

        if( TVServer< IVDirtyObject >::IsClosed() )
        {
            TVServer< IVDirtyObject >::Run( [ this ]( auto & spcDirty ){
                m_timeline.RegisterDirty( spcDirty );
            } );
        }
    }  
    void _applyExpData( VO< TVConstRef< TOUTPUT > > & usr )
    { 
#ifdef VD_NOT_CARE_PERFORMANCE
        _chechData() ;
#endif
        m_svcExp.RunNew( usr , m_expValue );
    }
    void _updateList()
    {
        m_svcData.Close() ; 
        m_expContainer.clear() ;

        m_itemProvList.TidyTravel( [ this ]( IVDataProv< T > & dp ){
            dp.Apply( VD_L2U( [ this ]( VI< TVConstRef< T > > & spcItem ){
                m_svcData.RunNew( [ this ]( VI< TVCollectorData< T > >& spc ){
                    m_svcDataHdlr.Add( spc );
                } , spcItem , m_expContainer );
            } ) ) ;
        } ) ;

        m_expValue = VD_V2CB( m_expContainer ) ;
#ifdef VD_NOT_CARE_PERFORMANCE
        _chechData() ;
#endif
    } ;

#ifdef VD_NOT_CARE_PERFORMANCE
    void _chechData()
    {
        std::for_each( m_expContainer.begin() , m_expContainer.end() , []( const T * p ){
            VASSERT( p != 0 ) ;
        } ) ;
    }
#endif

private :
    VN_ELEMSYS::TVSysImpTimeline                     & m_timeline        ;
    TVServiceProxyArray< IVTracer >                    m_listeners       ; 
    TVServiceArray< IVTracer , VItemTracer >           m_tracers         ;
    bool                                               m_bLengthChanged  ;
    TVServiceProxyArray< IVDataProv< T > >             m_itemProvList    ;

    TOUTPUT                                            m_expValue        ;
    std::vector< const T * >                           m_expContainer    ;
    TVServiceArray< TVCollectorData< T > >             m_svcData         ;
    TVServiceProxyArray< TVCollectorData< T > >        m_svcDataHdlr     ;
    // 
    TVServiceArray< TVConstRef< TOUTPUT > >            m_svcExp          ; 
    TVServiceArray< IVDataProv< TOUTPUT > , ProvAdp  > m_svcProv         ;  
} ;

template< typename T >
class TVSvcCollector
{
public :
    TVSvcCollector()
    {}
    ~TVSvcCollector()
    {}

public :
    typedef VSConstBuffer< const T * > TEXP        ;
    typedef IVConverter< TEXP , T    > interf_type ;
    typedef TVCollector< T           > imp_type    ;

public : 
    void Close()
    {
        m_svc.Close() ;
    }
    void Elapse()
    {
        m_timeline.ClearDirty() ;
    } 
    void RunService( VO< interf_type > & pipe )
    {
        m_svc.RunNew( pipe , m_timeline ) ;
    } 
    template< typename T >
    void Init( const T & ) 
    {
    }
private :
    VN_ELEMSYS::TVSysImpTimeline             m_timeline ;
    TVServiceArray< interf_type , imp_type > m_svc      ;
} ;

// Duplicate
template< typename TImp , typename TEXP , typename ... TINPUT >  class TVDuplicateEntry ;

template< typename TImp , typename TEXP >  
class TVDuplicateEntry< TImp , TEXP >
{
public :
    TVDuplicateEntry( ) 
    {
    }
    ~TVDuplicateEntry()
    {
    }
    void Output( typename TVOutputPortParam< TEXP >::type & inPort )
    {
        m_imp.Output( inPort ) ;
    }

    template< unsigned N , typename ... TARGs >
    void Update( const TVInputChngFlag< N > & chngFlag , const TVDynVar< TARGs > & ... args )
    {  
        _call( &TImp::Update , m_imp , args ... ) ; 
    }

    void AttachData()
    {
    } 

private :
    template< typename ... TIAs , typename ... TARGs >
    void _call( void ( TImp::* f )( const TIAs & ... args ) , TImp & cnvtr , const TVDynVar< TARGs > & ... args )
    {
        (cnvtr.*f)( args ... ) ; 
    }
    //template< unsigned N , typename ... TARGs >
    //void _call( void ( TImp::* f )( const TVInputChngFlag< N > & , const TARGs & ... args ) , TImp & cnvtr , const TARGs & ... args )
    //{
    //    (cnvtr.*f)( chngFlag , args ... ) ; 
    //} 

private :
    TImp  m_imp ;
};

template< typename TImp , typename TEXP , typename TI , typename ... TIOthers >
class TVDuplicateEntry< TImp , TEXP ,  TI , TIOthers ... > : public TVDuplicateEntry< TImp , TEXP , TIOthers ... >   
{
private :
    typedef TVDuplicateEntry< TImp , TEXP , TIOthers ... > sub_type ;
    TVServiceProxy< IVDataProv< TI > >                     m_proxyProv ;
    TVServiceProxy< TVConstRef< TI > >                     m_proxyRef  ;   

public :
    TVDuplicateEntry()
    {
    }
    ~TVDuplicateEntry()
    {
    }

public :
    void AttachData( IVSlot< TI > & s , IVSlot< TIOthers > & ... sOthers )
    { 
        s.GetData( VD_P2U( m_proxyProv ) );
        sub_type::AttachData( sOthers ... ) ;
    }

    template< unsigned N , typename ... TARGs >
    void Update( const TVInputChngFlag< N > & chngFlag , const TVDynVar< TARGs > & ... args )
    {

        if( m_proxyRef.IsClosed() )
        {
            m_proxyProv.UseServer( [ this ]( IVDataProv< TI > & mc ){
                mc.Apply( VD_P2U( m_proxyRef ) );
            } );
        }

        m_proxyRef.UseServer( [ this , & chngFlag ,  & args ...]( const TVConstRef< TI > & taRef ){
            const unsigned IDX = N - ( sizeof ... ( TIOthers ) ) - 1 ;
            TVDynVar< TI > var( taRef.m_ref , chngFlag.template Check< IDX >() ) ;
            sub_type::Update( chngFlag , args ... , var ) ;
        } ) ;
    }
} ;

template< typename TImp , typename TEXP , typename ... TINPUT >
class TVDuplicate : public IVConverter< TEXP , TINPUT ... >
                  , public TVDuplicateEntry< TImp , TEXP , TINPUT ... >  
                  , public TVServer< IVDirtyObject >
{
public :
    typedef TVDuplicateEntry< TImp , TEXP , TINPUT ... > entry_type ;
    typedef TVDuplicate< TImp , TEXP , TINPUT ... > my_type ;
    typedef void ( my_type::* call_chng )() ;

    TVDuplicate( VN_ELEMSYS::TVSysImpTimeline & tl )
        :m_timeLine( tl )
    {
    }
    ~TVDuplicate()
    {
        TVServer< IVDirtyObject >::Close() ;
        for( unsigned i = 0 ; i < ARG_COUNT ; i ++ )
        {
            m_tracer[i].Close() ;
        }
    }

public :  
    virtual void Input ( IVSlot< TINPUT > & ... slot )
    {
        _traceSlot( slot ... ) ;
        entry_type::AttachData( slot ... );
    }
    virtual void Output( typename TVOutputPortParam< TEXP >::type & inPort )
    {
        entry_type::Output( inPort ) ;
    }
    virtual void CleanAndDiffuse()
    { 
        entry_type::Update( m_chngFlag ) ;
        m_chngFlag.Reset() ;
    }  
    
    template< unsigned >
    struct VPtr
    {
        my_type & me ;
        VPtr( my_type & m ):me(m){}
    } ;

    struct VTracer : IVTracer 
    {
        my_type & me ;
        call_chng fcall ; 
        template< unsigned IDX >
        VTracer( VPtr< IDX > & ptr ):me(ptr.me),fcall( &my_type::_notifyChange<IDX> )
        {
        } 
        void OnChanged()
        {
            ( me.*fcall )( );
        }
    } ;

private : 
    template< unsigned IDX >
    void _notifyChange()
    {
        m_chngFlag.template Signal<IDX>() ;
        _makeDirty() ;
    }

    void _makeDirty()
    { 
        if( TVServer< IVDirtyObject >::IsClosed() )
            TVServer< IVDirtyObject >::Run( [ this ]( auto &spc ){ m_timeLine.RegisterDirty( spc ) ; } ) ;
    } 

    void _traceSlot( ) 
    { 
    }
    template< typename TI , typename ... TNxts >
    void _traceSlot( IVSlot< TI > & si , IVSlot< TNxts > & ... slotOthers )
    { 
        const unsigned idx = ARG_COUNT - ( sizeof ... ( TNxts ) ) - 1 ;

        m_tracer[idx].Run( [ &si ]( VI< IVTracer > & spc ){
            si.Trace( spc ) ;
        } , VPtr<idx>( *this ) ) ;
    }

private :    
    VN_ELEMSYS::TVSysImpTimeline & m_timeLine ;
    static const unsigned ARG_COUNT = sizeof ... ( TINPUT ) ;
    TVService< IVTracer , VTracer > m_tracer[ ARG_COUNT ] ;
    TVInputChngFlag< ARG_COUNT    > m_chngFlag            ;
} ;

template< typename TImp , typename TEXP , typename ... TINPUT  >
class TVSvcDuplicate
{
public :
    TVSvcDuplicate()
    {}
    ~TVSvcDuplicate()
    {}

public : 
    typedef IVConverter< TEXP , TINPUT ...        > interf_type ;
    typedef TVDuplicate< TImp , TEXP , TINPUT ... > imp_type    ;

public : 
    void Close()
    {
        m_svc.Close() ;
    }
    void Elapse()
    {
        m_timeline.ClearDirty() ;
    } 
    void RunService( VO< interf_type > & pipe )
    {
        m_svc.RunNew( pipe , m_timeline ) ;
    } 
    template< typename T >
    void Init( const T & v )
    {
    }

private :
    VN_ELEMSYS::TVSysImpTimeline             m_timeline ;
    TVServiceArray< interf_type , imp_type > m_svc      ; 
} ;

////////////////////////////////////////////////////////////
template< typename TTARG > struct TVInputTrait
{
    static auto GetFunc() { return &TTARG::Input  ; } 
} ;
template< typename TTARG > struct TVOutputTrait
{
    static decltype( &TTARG::Output ) GetFunc() { return &TTARG::Output ; } 
} ;

template< typename TID > 
struct TVOutputTrait< TVHub< IVRLTN< TID > > >
{
    typedef typename IVRLTN< TID >::VALUE OUVALUE ;

    template< typename T = TID >
    static std::enable_if< std::is_void< typename IVRLTN<T>::VALUE >::value , decltype( TVHub< IVRLTN< TID > >::Output ) > 
        GetFunc()
    {
        return TVHub< IVRLTN< TID > >::Output ;
    } 
} ;


template< typename ... TINPUTs > 
class TVLinkerAdptorFilter 
{
public :
    template< typename F , typename TP , typename ... TPOthers >
    static void TravelArgs( F f , IVSlot< TINPUTs > & ... slots , TP & prov , TPOthers & ... otherProvs )
    {
        _travel( f , slots ... , prov , TVOutputTrait< TP >::GetFunc() , otherProvs ... ) ;
    }   

    template< typename F , typename TD , typename ... TPOthers >
    static void TravelArgs( F f , IVSlot< TINPUTs > & ... slots , IVSlot< TD > & td , TPOthers & ... otherProvs )
    {
        TVLinkerAdptorFilter< TINPUTs ... , TD >::TravelArgs( f , slots ... , td , otherProvs ... ) ;
    }  

    template< typename F >
    static void TravelArgs( F f , IVSlot< TINPUTs > & ... slots )
    { 
        f( slots ... ) ;
    }  

private :
    template< typename F , typename TP , typename TC , typename ... TARGs , typename ... TPOthers >
    static void _travel( F fuse , IVSlot< TINPUTs > & ... slots , TP & prov , void ( TC::*fout )( IVInputPort< TARGs ... > & ) , TPOthers & ... otherProvs )
    {
        static_assert( std::is_base_of< TC , TP >::value , "" ) ;

        auto fnxt =  [ fuse  , & slots ... , & otherProvs ... ]( IVSlot< TARGs > & ... ins ){
            TVLinkerAdptorFilter< TINPUTs ... , TARGs ... >::TravelArgs( fuse , slots ... , ins ... , otherProvs ... ) ;
        } ;  

        ( prov .*fout)( VD_L2IP( fnxt ) ) ; 
    }
} ; 

template< typename TTARG > 
class TVLinkerAdptor
{
public :
    template< typename ... TINPUTs , typename TC , typename ... TIs >
    static void AttachInput( TTARG & cvtr , void ( TC::*f )( IVSlot< TINPUTs > & ... ) , TIs & ... args )
    {
        auto fadp = [ & cvtr , f ]( IVSlot< TINPUTs > & ... ins ){
            (cvtr.*f)( ins ... ) ;
        } ;

        TVLinkerAdptorFilter<>::TravelArgs( fadp , args ... ) ; 
    }  
} ;

template< typename TCONV , typename ... TIs >
void VLNK( TCONV & cvtr ,  TIs & ... args )
{
    TVLinkerAdptor< TCONV >::AttachInput( cvtr , TVInputTrait< TCONV >::GetFunc() , args ... ) ;
} ;

template< typename TOUTPUT , typename ... TINPUTs >
class TVConverterWrapper : public VO< IVConverter< TOUTPUT , TINPUTs ... > > 
                         , public IVConverter< TOUTPUT , TINPUTs ... >
{
public :
    TVConverterWrapper() 
    {}
    ~TVConverterWrapper() 
    {}
public :
    typedef IVConverter< TOUTPUT , TINPUTs ... > interf_type ;  
     
    virtual void Input( IVSlot< TINPUTs > & ... args )
    {
        m_proxy.UseServer(  [ &args ... ]( auto & ds ){
            ds.Input( args ... ) ; 
        } ) ; 
    }

    virtual void Output( typename TVOutputPortParam< TOUTPUT >::type & inp ) 
    {
        m_proxy.UseServer( [ & inp ]( auto & ds ){ ds.Output( inp ) ; } ) ;
    }

private :
    void Visit( VI< interf_type >  & spc )
    {
        m_proxy.Create( spc ) ;
    }

private :
    TVServiceProxy< interf_type > m_proxy ;
} ;

////////////////////////////////////////////////////////////////////////////////////////////////

// System of dynamic relation implements

////////////////////////////////////////////////////////////////////////////////////////////////

template< typename TSysTrait , typename TR > class TVRHubImp ;
 
////////////////////////////////////////////////////////////////////////////////////////////////

// (1) Declaration of TVRHubImp 

////////////////////////////////////////////////////////////////////////////////////////////////

template< typename ... TARGs > class TVRTupleData ;

template<>
class TVRTupleData<>
{
public :
    void Input() {}
    template< typename F , typename ... TARGs >
    void Output( F f , IVSlot< TARGs > & ... slts )
    {
        f( slts ... ) ;
    }

    template< typename F >
    void Output( F f )
    {
    }
} ;

template< typename T , typename ... TOTHERs >
class TVRTupleData< T , TOTHERs ... >
{
public :
    TVRTupleData(){}

public :
    void Input( IVSlot< T > & s , IVSlot< TOTHERs > & ... slts )
    {
        m_data.Input( s ) ;
        m_sub.Input( slts ... ) ;
    }
    
    template< typename F , typename ... TARGs >
    void Output( F f , IVSlot< TARGs > & ... slts )
    {
        m_sub.Output( f , slts ... , static_cast< IVSlot< T > & >( m_data ) ) ;
    } 

private :
    typedef TVRTupleData< TOTHERs ... > sub_type ;
    sub_type                            m_sub    ;
    TVData< T >                         m_data   ;
} ;

template< typename T , T > struct TVR_ComData_Args_From_Func ;
template< typename TIMP , typename ... TOUs , void ( TIMP::*mf )( IVInputPort< TOUs ... > & ) >
struct TVR_ComData_Args_From_Func< void ( TIMP::* )( IVInputPort< TOUs ... > & ) , mf >
{
    typedef TVRTupleData< TOUs ... > arg_type ;

    static void Output( TIMP & inst , arg_type & tpl )
    {
        (inst.*mf)( VD_L2IP( [ &tpl ]( IVSlot< TOUs > & ... slts ){
            tpl.Input( slts ... ) ; // TVR_ComData_Args_Accessor< TOUs ... >::Ouput( tpl , slts ... ) ;
        } ) ) ;
    }

    class ComAdp
    {
    public :
        ComAdp( TVRTupleData< TOUs ... > & td )
            : m_data( td )
        {
        }

    public : 
        void Output( IVInputPort< TOUs ... > & inp )
        { 
            m_data.Output( [ &inp ]( IVSlot< TOUs > & ... sins ){
                inp.Input( sins ... ) ;
            } ) ;
        }

    private :
        TVRTupleData< TOUs ... > & m_data ;
    } ; 
} ;

template< typename TIMP , typename ... TINs , void ( TIMP::*mf )( IVSlot< TINs > & ... ) >
struct TVR_ComData_Args_From_Func< void ( TIMP::* )( IVSlot< TINs > & ... ) , mf >
{
    typedef TVRTupleData< TINs ... > arg_type ;
    static void Input( TIMP & inst , arg_type & tpl )
    {
        tpl.Output( [ & inst ]( IVSlot< TINs > & ... slts ){
            ( inst.*mf )( slts ... ) ;
        } ) ; 
    }

    class ComAdp
    {
    public:
        ComAdp( TVRTupleData< TINs ... > & td )
            : m_data( td )
        {
        }

    public:
        void Input( IVSlot< TINs > & ... slots )
        { 
            m_data.Input( slots ... ) ;
        }

    private:
        TVRTupleData< TINs ... > & m_data;
    }; 
} ;
//
//
//template< typename TIMP , typename ... TOUs , typename ... TINs , void ( TIMP::*mf )( IVInputPort< TOUs ... > & , IVSlot< TINs > & ...  ) >
//struct TVR_ComData_Args_From_Func< void ( TIMP::* )( IVInputPort< TOUs ... > & , IVSlot< TINs > & ... ) , mf >
//{ 
//    typedef std::pair< TVRTupleData< TOUs ... > , TVRTupleData< TINs ... > > arg_type ; 
//
//    static void Output( TIMP & inst , arg_type & tpl )
//    {
//        TVRTupleData< TINs ... > & tdIn = tpl.second ;
//        TVRTupleData< TOUs ... > & tdOu = tpl.first  ;
//
//        tdIn.Output( [ &inst , & tdOu ]( IVSlot< TINs > & ... sins ){
//            ( inst.*mf )( VD_L2IP( [ &tdOu ]( IVSlot< TOUs > & ... souts ){
//                tdOu.Input( souts ... );
//            } ) , sins ... ) ;
//        } ) ;
//    }
//
//    class ComAdp
//    {
//    public:
//        ComAdp( arg_type & td )
//            : m_data( td )
//        {
//        }
//        void Input( IVSlot< TINs > & ... slots )
//        {
//            TVRTupleData< TINs ... > & tdIn = m_data.first  ;
//            tdIn.Input( slots ... )
//        }
//        void Output( IVInputPort< TOUs ... > & inp )
//        {
//            TVRTupleData< TOUs ... > & tdOu = m_data.first  ;
//            tdOu.Output( [ &inp ]( IVSlot< TOUs > & ... sins ){
//                inp.Input( sins ... ) ;
//            } ) ;
//        }
//
//    private:
//        arg_type & m_data;
//    } ;
//};
 
template <typename T>
class TVR_HasOutput
{
    typedef char one;
    typedef long two;

    template <typename C> static one tesetoutput( decltype(&C::Output) ) ;
    template <typename C> static two tesetoutput(...);    

public: 
    static const bool value = ( sizeof( tesetoutput< T >( 0 ) ) == sizeof( char ) );
}; 

template< typename T , bool bHasInput >
class TVOutputableComData
{
public :
    typedef TVR_ComData_Args_From_Func< decltype( &T::Output ) , &T::Output > parser_type ;
    typedef typename parser_type::arg_type                                    arg_type    ;
    arg_type m_args ;

public :
    void Output( T & inst  )
    { 
        parser_type::Output( inst , m_args ) ;
    }
    arg_type & GetArgs( )
    {
        return m_args ;
    } 
} ;

template< typename T >
class TVOutputableComData< T , false >
{
public :
    void Output( T & inst  )
    { 
    }
} ;
  
template <typename T>
class TVR_HasInput
{
    typedef char one;
    typedef long two;

    template <typename C> static one tesetinput( decltype(&C::Input) ) ;
    template <typename C> static two tesetinput(...);    

public: 
    static const bool value = ( sizeof( tesetinput< T >( 0 ) ) == sizeof( char ) );
}; 

template< typename T , bool bHasInput >
class TVInputableComData
{
public :
    typedef TVR_ComData_Args_From_Func< decltype( &T::Input ) , &T::Input > parser_type ;
    typedef typename parser_type::arg_type                                  arg_type    ;
    arg_type m_args ;

public :
    void Input( T & inst  )
    { 
        parser_type::Input( inst , m_args ) ;
    }
    arg_type & GetArgs( )
    {
        return m_args ;
    } 
} ;

template< typename T >
class TVInputableComData< T , false >
{
public :
    void Input( T & inst  )
    { 
    }
} ;
 
template< typename T , bool bHasOutput >
class TVCompBase : public TVR_ComData_Args_From_Func< decltype( &T::Output ) , &T::Output >::ComAdp
{
public :
    TVCompBase( TVOutputableComData< T , true > & co )
        : TVR_ComData_Args_From_Func< decltype( &T::Output ) , &T::Output >::ComAdp( co.GetArgs() )
    {
    }
} ;

template< typename T >
class TVCompBase< T , false >
{
public :
    TVCompBase( TVOutputableComData< T , false > & co ) 
    {
    }
} ;
 
template< typename T , bool bHasInput >
class TVCompBaseRev : public TVR_ComData_Args_From_Func< decltype( &T::Input ) , &T::Input >::ComAdp
{
public :
    TVCompBaseRev( TVInputableComData< T , true > & co )
        : TVR_ComData_Args_From_Func< decltype( &T::Input ) , &T::Input >::ComAdp( co.GetArgs() )
    {
    }
} ;

template< typename T >
class TVCompBaseRev< T , false >
{
public :
    TVCompBaseRev( TVInputableComData< T , false > & co ) 
    {
    }
} ;

template< typename T > 
class TVComp : public TVCompBase< T , TVR_HasOutput< T >::value >
             , public TVCompBaseRev< T , TVR_HasInput< T >::value >
{
public :
    typedef TVOutputableComData    < T , TVR_HasOutput< T >::value > ComDataOuType ; 
    typedef TVInputableComData < T , TVR_HasInput< T >::value  > ComDataInType ; 

    typedef TVCompBase< T , TVR_HasOutput< T >::value   > base_type     ;
    typedef TVCompBaseRev< T , TVR_HasInput< T >::value > base_rev_type ;

    TVComp( ComDataOuType & dt , ComDataInType & rev )
        : base_type( dt ) , base_rev_type( rev )
    {
    }   
} ;

template< typename T > 
using TVAggr = TVCompBase< T , TVR_HasOutput< T >::value > ;

////////////////////////////////////////////////////////////////////////////////////////////////

// (2) Hub linkage implementation

////////////////////////////////////////////////////////////////////////////////////////////////
template< typename TSysTrait , typename TR >
struct TVRHubNodeTrait
{
    typedef typename TSysTrait::template TRTrait< TR >::mngr_type  MNGR;
};

template< typename TSysTrait , typename T > class TVRHubLinkageFull ; 
template< typename TSysTrait , typename TID > 
class TVRHubLinkageFull< TSysTrait , IVRLTN< TID > >   
{
public : 
    typedef TVRHubImp< TSysTrait , IVRLTN< TID > >  hub_inst_type ;
    typedef typename hub_inst_type::MNGR             manager_type  ; 
    typedef TVOutputableComData   < manager_type , TVR_HasOutput< manager_type >::value > data_ou_type ;
    typedef TVInputableComData< manager_type , TVR_HasInput< manager_type >::value  > data_in_type ;

    void AttachProvider( IVSlot< IVRLTN< TID > > & si )
    { 
        hub_inst_type::Slot2Manager( si , [ this ]( manager_type & inst ){ 
            AttachManager( inst ) ;
        } ) ;
    } 

    // Attach Outter value 
    void AttachManager( manager_type & inst )
    {
        m_ou.Output( inst ) ; 
        m_in.Input( inst ) ;
    }

    // Provide Data for using by owner
    template< typename F >
    void UseAsComponent( F f )
    {
        auto Tmt =TVComp< manager_type >( m_ou , m_in );
        f( Tmt) ;
    } 

private :
    data_ou_type  m_ou ;
    data_in_type  m_in ;
 } ;

// ReadOnlyLinkage 
template< typename TSysTrait , typename T > 
class TVRHubLinkageReadOnly
{ 
public :
    TVRHubLinkageReadOnly(){}
    ~TVRHubLinkageReadOnly(){}
    void AttachProvider( IVSlot< T > & si )
    {
        m_data.Input( si ) ;
    }  
    template< typename F >
    void UseAsAggragation( F f )
    { 
        f( static_cast< IVSlot< T > & > ( m_data ) ) ;
    }  

    // Evander added -- 20161109
    void DetachProvider()
    {
        m_data.Close();
    }
    //////////////////////////////////////////////////////////////////////////

    TVData< T > m_data ;
} ;

template< typename TSysTrait , typename TID > 
class TVRHubLinkageReadOnly< TSysTrait , IVRLTN< TID > >   
{
public : 
    typedef TVRHubImp< TSysTrait , IVRLTN< TID > >  hub_inst_type ;
    typedef typename hub_inst_type::MNGR             manager_type  ; 
    typedef TVOutputableComData< manager_type , TVR_HasOutput< manager_type >::value > data_type ;

    void AttachProvider( IVSlot< IVRLTN< TID > > & si )
    { 
        hub_inst_type::Slot2Manager( si , [ this ]( manager_type & inst ){ 
            AttachManager( inst ) ;
        } ) ;
    } 

    void AttachManager( manager_type & inst )
    {
        m_data.Output( inst );
    }
    
    template< typename F >
    void UseAsAggragation( F f )
    {
        auto data=TVAggr< manager_type >( m_data ) ;
        f( data) ;
    }

    // Evander added -- 20161109
    void DetachProvider()
    {
        VASSERT( 0 && "��Ҫʵ��DetachProvider" );
    }
    //////////////////////////////////////////////////////////////////////////

private :
    data_type m_data ;
 } ;
////////////////////////////////////////////////////////////////////////////////////////////////

// (3) Hub implementation

////////////////////////////////////////////////////////////////////////////////////////////////

// root 
template< typename TSysTrait , typename TINTERF >
class TVRHubImpRoot : public TINTERF
{
public :
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    TVRHubImpRoot( TPIPELINE & pl )
        : m_pipeline( pl )
    {
    }
    
    TPIPELINE &  GetPipeLine()
    {
        return m_pipeline ;
    }
    
    void Close()
    {  
    }

private :
    TPIPELINE &   m_pipeline ;
}  ;

template< typename TSysTrait , typename TVALUE , typename TBASE >
class TVRHubImpRootBase : public TBASE
{
public :
    using TBASE::TBASE ;
    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {
        f( args ... , static_cast< IVInputPort< TVALUE > & >( m_expData ) ) ;
    }
    void Output( IVInputPort< TVALUE > & inp )
    {
        inp.Input( m_expData ) ;
    }
    
    void Close()
    {  
    }
private :
    TVData< TVALUE > m_expData ;
} ;

template< typename TSysTrait , typename TEXPID , typename TBASE >
class TVRHubImpRootBase< TSysTrait , IVRLTN< TEXPID > , TBASE  > : public TBASE
{
public :
    using TBASE::TBASE ;

    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {     
        m_linkage.UseAsComponent( [ this , f , & args ... ]( auto & v ){
            f( args ... ,  v );
        } ) ; 
    }
    virtual void Create( VO< typename IVRLTN< TEXPID >::HUB > & sys )
    {
        auto fuse = [ this ]( auto & mngr ){
            m_linkage.AttachManager( mngr ) ;
        } ;

        m_svc.Run( sys , this->GetPipeLine() , fuse ) ;
    }
    void Close()
    {  
        m_svc.Close() ;
        TBASE::Close() ;
    }

private :   
    typedef typename IVRLTN< TEXPID >::HUB                interf_type  ;  
    typedef TVRHubImp< TSysTrait , IVRLTN< TEXPID >  >   svr_type     ; 
    typedef TVRHubLinkageFull< TSysTrait , IVRLTN< TEXPID > > linkage_type ;
    TVService< interf_type , svr_type > m_svc ;
    linkage_type                        m_linkage ;
} ;

template< typename TSysTrait , typename TBASE >
class TVRHubImpRootBase< TSysTrait , void , TBASE > : public TBASE
{
public :
    using TBASE::TBASE ;
    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {
        f( args ... ) ;
    }
    void Close()
    {  
    }
} ;

template< typename TSysTrait , typename TBASE >
class TVRHubImpRootBase< TSysTrait , VI_AND<> , TBASE  > : public TBASE 
{
public :
    using TBASE::TBASE ;
    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {
        f( args ... ) ;
    }
    void Close()
    {  
    }
} ;

template< typename TSysTrait , typename TEXPID , typename ... TOTHEREXPs , typename TBASE >
class TVRHubImpRootBase< TSysTrait , VI_AND< IVRLTN< TEXPID > , TOTHEREXPs ... > , TBASE  >
    : public TVRHubImpRootBase< TSysTrait , VI_AND< TOTHEREXPs ... > , TBASE  >
{
public :
    typedef TVRHubImpRootBase< TSysTrait , VI_AND< TOTHEREXPs ... > , TBASE  > sub_base_type ;
    using sub_base_type::sub_base_type ;

    ~TVRHubImpRootBase()
    { 
    }

public : 
    void Close()
    {  
        m_svc.Close() ;
        sub_base_type::Close() ;
    }

    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {       
        m_linkage.UseAsComponent( [ this , f , & args ... ]( auto & v ){
            sub_base_type::AccessLinkage( f , args ... , v ) ; 
        } ) ; 
    }
    virtual void Create( VO< typename IVRLTN< TEXPID >::HUB > & sys )
    {
        auto fuse = [ this ]( auto & mngr ){
            m_linkage.AttachManager( mngr ) ;
        } ;

        m_svc.Run( sys , this->GetPipeLine() , fuse ) ;
    }

private :   
    typedef typename IVRLTN< TEXPID >::HUB                interf_type  ;  
    typedef TVRHubImp< TSysTrait , IVRLTN< TEXPID >  >   svr_type     ; 
    typedef TVRHubLinkageFull< TSysTrait , IVRLTN< TEXPID > > linkage_type ;
    TVService< interf_type , svr_type > m_svc     ;
    linkage_type                        m_linkage ;
} ;

template< typename TSysTrait , typename TEXP , typename ... TOTHEREXPs , typename TBASE >
class TVRHubImpRootBase< TSysTrait , VI_AND< TEXP , TOTHEREXPs ... > , TBASE  >
    : public TVRHubImpRootBase< TSysTrait , VI_AND< TOTHEREXPs ... > , TBASE  >
{
public :
    typedef TVRHubImpRootBase< TSysTrait , VI_AND< TOTHEREXPs ... > , TBASE  > sub_base_type ;
    using sub_base_type::sub_base_type ;

    ~TVRHubImpRootBase()
    {
    }

public : 
    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    { 
       sub_base_type::AccessLinkage( f ,  args ...  , static_cast< IVInputPort< TEXP > & >( m_expData )  ) ;
    }
    void Output( IVInputPort< TEXP > & inp )
    {
        inp.Input( m_expData ) ;
    }
    
    void Close()
    { 
        sub_base_type::Close() ;
    }
private :
    TVData< TEXP > m_expData ;
} ;

template< typename TSysTrait , typename TID , typename TVALUE , typename ... TINPUTs >
using TVRHubImpExp = TVRHubImpRootBase< TSysTrait , TVALUE , TVRHubImpRoot< TSysTrait , IVRUniqHub< TID , TVALUE , TINPUTs ... > > > ;

// And  
template< typename TSysTrait , typename TBASE , typename TVALUE , typename ... TINPUTs > class TVRHubImpBase_And ;
template< typename TSysTrait , typename TBASE , typename TVALUE >
class TVRHubImpBase_And< TSysTrait , TBASE , TVALUE > : public TBASE
{
public :
    using TBASE::TBASE ;

    ~TVRHubImpBase_And()
    {
    }

    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {  
        TBASE::AccessLinkage( f , args ... ) ;
    }
    
    void Close()
    { 
        TBASE::Close() ;
    }
} ;

template< typename TSysTrait , typename TBASE , typename TVALUE , typename TI , typename ... TOTHERs >
class TVRHubImpBase_And< TSysTrait , TBASE , TVALUE , TI , TOTHERs ... > : public TVRHubImpBase_And< TSysTrait , TBASE , TVALUE , TOTHERs ... >
{  
public :
    typedef TVRHubImpBase_And< TSysTrait , TBASE , TVALUE , TOTHERs ... > BASE_TYPE ;
    using TVRHubImpBase_And< TSysTrait , TBASE , TVALUE , TOTHERs ... >::TVRHubImpBase_And ;

    ~TVRHubImpBase_And()
    {
    }

private :
    typedef TVRHubImpBase_And< TSysTrait , TBASE , TVALUE , TOTHERs ... > sub_type ;  
    typedef TVRHubLinkageReadOnly< TSysTrait , TI > linkage_type ;

    virtual void Connect( IVSlot< TI > & si )
    {
        m_linkage.AttachProvider( si ) ;
    }

public : 
    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {  
        m_linkage.UseAsAggragation( [ this , f , & args ... ]( auto & v ){
            sub_type::AccessLinkage( f , args ... , v ) ; 
        } ) ; 
    }
    
    void Close()
    { 
        BASE_TYPE::Close() ;
    }

private : 
    linkage_type m_linkage ;
} ; 

template< typename TSysTrait , typename TID , typename TVALUE , typename TNAME > class TVRHubImpAdp ; 
template< typename TSysTrait , typename TID , typename TVALUE , typename ... TINPUTs > 
class TVRHubImpAdp< TSysTrait , TID , TVALUE , VI_AND< TINPUTs ... > > 
    : public TVRHubImpBase_And< TSysTrait , TVRHubImpExp< TSysTrait , TID , TVALUE , TINPUTs ... > , TVALUE , TINPUTs ... >
{
public :
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    typedef TVRHubImpBase_And< TSysTrait , TVRHubImpExp< TSysTrait , TID , TVALUE , TINPUTs ... > , TVALUE , TINPUTs ... > BASE_TYPE ;
    typedef typename TVRHubNodeTrait< TSysTrait , IVRLTN< TID > >::MNGR  MNGR ;

public :
    TVRHubImpAdp( TPIPELINE & pl )
        : BASE_TYPE( pl )
    { 
        BASE_TYPE::AccessLinkage( [ this , & pl ]( auto & ... vin ){
            m_node.Renew( pl , vin ... ) ;
        } ) ;
    }
    ~TVRHubImpAdp()
    {
    }
 
    template< typename F >
    void AccessManager( F f )
    {
        m_node.Use( f ) ;
    } ;
    
    void Close()
    {
        m_node.Clear() ; 
        BASE_TYPE::Close() ;
    }

private :
    TVSafeScopedObj< MNGR > m_node ;
} ;

// Or Evander added -- 20161109
template< typename TSysTrait , typename TDERIVE , typename TBASE , typename TVALUE , typename ... TINPUTs > class TVRHubImpBase_Or ;
template< typename TSysTrait , typename TDERIVE , typename TBASE , typename TVALUE >
class TVRHubImpBase_Or< TSysTrait , TDERIVE , TBASE , TVALUE > : public TBASE
{
public:
    using TBASE::TBASE;

    ~TVRHubImpBase_Or(){}

    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {
        VASSERT( sizeof...( ARGs ) == 1 && "Error." );
    }

    template< typename F >
    void AccessLinkage( F f ){}

    template< typename F  , typename ARG >
    void AccessLinkage( F f , ARG & arg )
    {
        TBASE::AccessLinkage( f , arg );
    }

    void Close()
    {
        TBASE::Close();
    }

    void ResetBack(){}
};

template< typename TSysTrait , typename TDERIVE , typename TBASE , typename TVALUE , typename TI , typename ... TOTHERs >
class TVRHubImpBase_Or< TSysTrait , TDERIVE , TBASE , TVALUE , TI , TOTHERs ... > 
    : public TVRHubImpBase_Or< TSysTrait , TVRHubImpBase_Or< TSysTrait , TDERIVE , TBASE , TVALUE , TI , TOTHERs ... > , TBASE , TVALUE , TOTHERs ... >
{
public:
    typedef TVRHubImpBase_Or< TSysTrait , TVRHubImpBase_Or< TSysTrait , TDERIVE , TBASE , TVALUE , TI , TOTHERs ... > , TBASE , TVALUE , TOTHERs ... > BASE_TYPE;
    using BASE_TYPE::BASE_TYPE;

    ~TVRHubImpBase_Or(){}

private:
    typedef BASE_TYPE sub_type;
    typedef TVRHubLinkageReadOnly< TSysTrait , TI > linkage_type;

    virtual void Connect( IVSlot< TI > & si )
    {
        m_bUsing = true;
        m_linkage.AttachProvider( si );
        
        BASE_TYPE::ResetBack();
        static_cast< TDERIVE& >( *this ).ResetFront();
    }

public:
    template< typename F , typename ... ARGs >
    void AccessLinkage( F f , ARGs & ... args )
    {
        if ( m_bUsing )
        {
            m_linkage.UseAsAggragation( [ this , f , &args ... ]( auto & v ){
                sub_type::AccessLinkage( f , args ... , v );
            } );
        }
        else
        {
            sub_type::AccessLinkage( f , args ... );
        }
    }

    void Close()
    {
        BASE_TYPE::Close();
    }

    void ResetFront()
    {
        m_bUsing = false;
        m_linkage.Close();
        static_cast< TDERIVE& >( *this ).ResetFront();
    }

    void ResetBack()
    {
        m_bUsing = false;
        m_linkage.Close();
        BASE_TYPE::ResetBack();
    }

private:
    linkage_type m_linkage;
    bool         m_bUsing = false;
};

template< typename TSysTrait , typename TID , typename TVALUE , typename ... TINPUTs >
class TVRHubImpAdp< TSysTrait , TID , TVALUE , VI_OR< TINPUTs ... > >
    : public TVRHubImpBase_Or< TSysTrait 
                             , TVRHubImpAdp< TSysTrait , TID , TVALUE , VI_OR< TINPUTs ... > > 
                             , TVRHubImpExp< TSysTrait , TID , TVALUE , TINPUTs ... >
                             , TVALUE , TINPUTs ...
                             >
{

public:
    typedef typename TSysTrait::PIPELINE TPIPELINE;
    typedef TVRHubImpBase_Or< TSysTrait , TVRHubImpAdp< TSysTrait , TID , TVALUE , VI_OR< TINPUTs ... > > 
                            , TVRHubImpExp< TSysTrait , TID , TVALUE , TINPUTs ... > , TVALUE , TINPUTs ... > BASE_TYPE;
    typedef typename TVRHubNodeTrait< TSysTrait , IVRLTN< TID > >::MNGR  MNGR;

    TVRHubImpAdp( TPIPELINE & pl )
        : BASE_TYPE( pl )
    {}
    ~TVRHubImpAdp(){}

    template< typename F >
    void AccessManager( F f )
    {
        m_node.Use( f );
    };

    void Close()
    {
        m_node.Clear();
        BASE_TYPE::Close();
    }

    void ResetFront()
    {
        BASE_TYPE::AccessLinkage( [ this ]( auto & ... vin ) {
            m_node.Renew( this->GetPipeLine() , vin ... );
        } );
    }

private:
    TVSafeScopedObj< MNGR > m_node;
};
//////////////////////////////////////////////////////////////////////////

// Arr  
template< typename TSysTrait , typename TID , typename TVALUE , typename TD > 
class TVRHubImpAdp< TSysTrait , TID , TVALUE , VI_ARR< TD > > : public TVRHubImpExp< TSysTrait , TID , TVALUE , TD >
{
public :
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    typedef typename TVRHubNodeTrait< TSysTrait , IVRLTN< TID > >::MNGR MNGR ; 
    typedef  TVRHubImpExp< TSysTrait , TID , TVALUE , TD > BASE_TYPE ;

public :
    TVRHubImpAdp( TPIPELINE & pl ) 
        : BASE_TYPE( pl )
    {
        BASE_TYPE::AccessLinkage( [ this , & pl ]( auto & ... vin ){
            m_node.Renew( pl , vin ... ) ;
        } ) ; 
    }
    ~TVRHubImpAdp()
    {
        m_node.Clear() ;
    }

    //typedef unsigned  MNGR ;
    template< typename F >
    void AccessManager( F f )
    {  
        m_node.Use( f ) ;
    } ; 
    
    void Close()
    {
        m_node.Clear() ; 
        BASE_TYPE::Close() ;
    }

public :  
    void Connect( IVSlot< TD > & si )
    {  
        m_node.Use( [ & si ]( auto & mngr){
            mngr.AddItem( si ) ;
        } ) ;
    }

private :
    TVSafeScopedObj< MNGR > m_node ; 
} ;
 
template< typename TSysTrait , typename TID , typename TVALUE , typename TITEMID > 
class TVRHubImpAdp< TSysTrait , TID , VI_AND< IVRLTN< TITEMID > , TVALUE > , VI_ARR< IVRLTN< TITEMID > > >
    : public TVRHubImpRootBase< TSysTrait , TVALUE , TVRHubImpRoot< TSysTrait , IVRUniqHub< TID , VI_AND< IVRLTN< TITEMID > , TVALUE > > > >
{
public :
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    typedef typename TVRHubNodeTrait< TSysTrait , IVRLTN< TID > >::MNGR MNGR ;
    typedef TVRHubImpRootBase< TSysTrait , TVALUE , TVRHubImpRoot< TSysTrait , IVRUniqHub< TID , VI_AND< IVRLTN< TITEMID > , TVALUE > > > > BASE_TYPE ;

private :
    typedef IVRLTN< TITEMID >                              item_rltn_type     ;
    typedef typename item_rltn_type::HUB                   item_interf_type   ;
    typedef TVRHubImp< TSysTrait , item_rltn_type    >    item_hub_inst_type ;
    typedef typename item_hub_inst_type::MNGR              item_manger_type   ;
    typedef TVRHubLinkageReadOnly< TSysTrait , item_rltn_type >    item_linkage_type  ;

public :
    TVRHubImpAdp( TPIPELINE & pl ) 
        : BASE_TYPE( pl )
    {
        BASE_TYPE::AccessLinkage( [ this , & pl ]( auto & ... vin ){
            m_node.Renew( pl , vin ... ) ;
        } ) ; 
    }
    ~TVRHubImpAdp()
    { 
    }

    void Close()
    {
        m_node.Clear() ;
        m_svc.Close() ;

        BASE_TYPE::Close() ;
    }

    //typedef unsigned  MNGR ;
    template< typename F >
    void AccessManager( F f )
    {  
        m_node.Use( f ) ; 
    } ; 

public : 
    void Create( VO< typename IVRLTN< TITEMID >::HUB > & usr )
    {   
        m_svc.RunNew( usr , this->GetPipeLine() , [ this ]( item_manger_type & mngrItem ){
            m_node.Use( [ & mngrItem ]( MNGR & mngrArr ){
                mngrArr.AddItem( mngrItem ) ;
            } ) ;
        } ) ; 
    }

private :
    TVSafeScopedObj< MNGR > m_node ;  
    TVServiceArray< item_interf_type , item_hub_inst_type > m_svc ; 
} ;

// Arr 01
template< typename TSysTrait , typename TID , typename TVALUE , typename TD > 
class TVRHubImpAdp< TSysTrait , TID , TVALUE , VI_ARR_01< TD > > : public TVRHubImpExp< TSysTrait , TID , TVALUE , TD >
{
public :
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    typedef typename TVRHubNodeTrait< TSysTrait , IVRLTN< TID > >::MNGR MNGR ; 
    typedef  TVRHubImpExp< TSysTrait , TID , TVALUE , TD > BASE_TYPE ;

public :
    TVRHubImpAdp( TPIPELINE & pl ) 
        : BASE_TYPE( pl )
    {
        BASE_TYPE::AccessLinkage( [ this , & pl ]( auto & ... vin ){
            m_node.Renew( pl , vin ... ) ;
        } ) ; 
    }
    ~TVRHubImpAdp()
    {
        m_node.Clear() ;
    }

    //typedef unsigned  MNGR ;
    template< typename F >
    void AccessManager( F f )
    {  
        m_node.Use( f ) ;
    } ; 
    
    void Close()
    {
        m_node.Clear() ; 
        BASE_TYPE::Close() ;
    }

public :  
    void Connect( IVSlot< TD > & si )
    {  
        m_node.Use( [ & si ]( auto & mngr){
            mngr.SetItem( si ) ;
        } ) ;
    }

private :
    TVSafeScopedObj< MNGR > m_node ; 
} ;
 
template< typename TSysTrait , typename TID , typename TVALUE , typename TITEMID > 
class TVRHubImpAdp< TSysTrait , TID , VI_AND< IVRLTN< TITEMID > , TVALUE > , VI_ARR_01< IVRLTN< TITEMID > > >
    : public TVRHubImpRootBase< TSysTrait , TVALUE , TVRHubImpRoot< TSysTrait , IVRUniqHub< TID , VI_AND< IVRLTN< TITEMID > , TVALUE > > > >
{
public :
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    typedef typename TVRHubNodeTrait< TSysTrait , IVRLTN< TID > >::MNGR MNGR ;
    typedef TVRHubImpRootBase< TSysTrait , TVALUE , TVRHubImpRoot< TSysTrait , IVRUniqHub< TID , VI_AND< IVRLTN< TITEMID > , TVALUE > > > > BASE_TYPE ;

private :
    typedef IVRLTN< TITEMID >                              item_rltn_type     ;
    typedef typename item_rltn_type::HUB                   item_interf_type   ;
    typedef TVRHubImp< TSysTrait , item_rltn_type    >    item_hub_inst_type ;
    typedef typename item_hub_inst_type::MNGR              item_manger_type   ;
    typedef TVRHubLinkageReadOnly< TSysTrait , item_rltn_type >    item_linkage_type  ;

public :
    TVRHubImpAdp( TPIPELINE & pl ) 
        : BASE_TYPE( pl )
    {
        BASE_TYPE::AccessLinkage( [ this , & pl ]( auto & ... vin ){
            m_node.Renew( pl , vin ... ) ;
        } ) ; 
    }
    ~TVRHubImpAdp()
    { 
    }

    void Close()
    {
        m_node.Clear() ;
        m_svc.Close() ;

        BASE_TYPE::Close() ;
    }

    //typedef unsigned  MNGR ;
    template< typename F >
    void AccessManager( F f )
    {  
        m_node.Use( f ) ; 
    } ; 

public : 
    void Create( VO< typename IVRLTN< TITEMID >::HUB > & usr )
    {   
        m_svc.RunNew( usr , this->GetPipeLine() , [ this ]( item_manger_type & mngrItem ){
            m_node.Use( [ & mngrItem ]( MNGR & mngrArr ){
                mngrArr.SetItem( mngrItem ) ;
            } ) ;
        } ) ; 
    }

private :
    TVSafeScopedObj< MNGR > m_node ;  
    TVServiceArray< item_interf_type , item_hub_inst_type > m_svc ; 
} ;

// HubImp defination
template< typename TSysTrait , typename TR >
class TVRHubImp : public TVRHubImpAdp< TSysTrait , typename TR::ID , typename TR::VALUE , typename TR::NAME  > 
{ 
public : 
    typedef typename TSysTrait::PIPELINE TPIPELINE ; 
    typedef TVRHubImpAdp< TSysTrait , typename TR::ID , typename TR::VALUE , typename TR::NAME  > base_type ;
    typedef TVRHubImp< TSysTrait , TR > my_type ;

    TVRHubImp( TPIPELINE & pl )
        : base_type( pl )
    {
    }

    template< typename FUSE >
    TVRHubImp( TPIPELINE & pl , FUSE f )
        : base_type( pl )
    {
        this->AccessManager( f ) ; 
    }

    ~TVRHubImp()
    {
        base_type::Close() ;
    }

public : 
    template< typename F >
    static void Slot2Manager( IVSlot< TR > & slot , F f )
    { 
        Slot2Object( slot , [ f ]( my_type & inst ){
            inst.AccessManager( f ) ;
        } ) ;
    } 
     
    template< typename F >
    static void Slot2Object( IVSlot< TR > & slot , F f )
    {
#ifdef VD_NOT_CARE_PERFORMANCE
        bool bHasVal = false ;
        auto FAdp = [ f , & bHasVal ]( my_type & v ){
            f( v ) ;
            bHasVal = true ;
        } ;
        VCMN::TVLambdaUser< decltype( FAdp ) , my_type , my_type & > usr( FAdp ) ;
#else
        VCMN::TVLambdaUser< F , my_type , my_type & > usr( f ) ;
#endif
        IVUser< my_type , my_type & > ** p = _getDock() ;
        VASSERT( * p == 0 ) ;
         *p = & usr ;
         slot.Present() ;
         *p = 0 ;
#ifdef VD_NOT_CARE_PERFORMANCE
         VASSERT_MSG( bHasVal , "No Instantiate object ! " ) ;
#endif
    } 

private :
    virtual void Participate()
    {
        IVUser< my_type , my_type & > ** p = _getDock() ;
        VASSERT( * p != 0 ) ; 
        (*p)->Visit( * this ) ;
    }

private :
    static IVUser< my_type , my_type & > ** _getDock()
    {
        static IVUser< my_type , my_type & > * p(0) ;
        return &p ;
    } 
} ;

////////////////////////////////////////////////////////////////////////////////////////////////

// (4) system implements 

////////////////////////////////////////////////////////////////////////////////////////////////
template< typename TSysTrait , typename TINTF , typename ... TRs > class TVDynamicSystemImpBase2 ;

template< typename TSysTrait , typename TINTF >
class TVDynamicSystemImpBase2< TSysTrait , TINTF > : public TINTF 
{
public : 
    template< typename ... TARGs >
    TVDynamicSystemImpBase2( const TARGs & ... args )
        : m_pipeLine( args ... ) 
    {
    }
    ~TVDynamicSystemImpBase2(){} 
    void Close(){ }
protected :
    typedef typename TSysTrait::PIPELINE    TPIPELINE ; 
    TPIPELINE & GetPipeLine()  { return m_pipeLine ; }

public :
    virtual void Elapse()
    {  
       m_pipeLine.Elapse() ;
    } 

private:
    TPIPELINE m_pipeLine;
} ;  

template< typename TSysTrait , typename TINTF , typename TR ,  typename ... TROTHERs >
class TVDynamicSystemImpBase2< TSysTrait , TINTF , TR , TROTHERs ... >
    : public TVDynamicSystemImpBase2< TSysTrait , TINTF , TROTHERs ... >
{
public :
    typedef TVDynamicSystemImpBase2< TSysTrait , TINTF , TROTHERs ... > base_type    ;
    typedef typename TR::HUB                                            interf_type  ;  
    typedef TVRHubImp< TSysTrait , TR  >                               svr_type     ; 

public : 
    template< typename ... TARGs >
    TVDynamicSystemImpBase2( const TARGs & ... args )
        : base_type( args ... ) 
    {
    }
    ~TVDynamicSystemImpBase2()
    {
        m_svc.Close();
    }  

public :
    virtual void Close ( )
    {
        base_type::Close() ;
    }  
    virtual void Create ( VO< interf_type > & usr )
    {
        m_svc.RunNew( usr , this->GetPipeLine() ) ;
    } 

private :  
    TVServiceArray< interf_type , svr_type > m_svc ; 
} ;

template< typename TSysTrait , typename TSYS > class TVDynamicSystemImp2 ;
template< typename TSysTrait , typename ... TRs >
class TVDynamicSystemImp2< TSysTrait , IVSysDynamic< TRs ... > >
    : public TVDynamicSystemImpBase2< TSysTrait , IVSysDynamic< TRs ... > , TRs ... >
{
public :
    typedef TVDynamicSystemImpBase2< TSysTrait , IVSysDynamic< TRs ... > , TRs ... > IMP_BASE ;

public :
    template< typename ... TARGs >
    TVDynamicSystemImp2( const TARGs & ... args )
        : IMP_BASE( args ... )
    {    
    }
    ~TVDynamicSystemImp2()
    {
        IMP_BASE::Close() ;
    } 
} ;

// Systematic Converter wrapper classes

////////////////////////////////////////////////////////////

template< typename TID , typename DESC , typename TOUTPUT > class TVRSystematicConverter ;

template< typename TID , typename ... TINPUTs , typename TOUTPUT > 
class TVRSystematicConverter< TID , VI_AND< TINPUTs ... > , TOUTPUT > : public IVConverter< TOUTPUT , TINPUTs ... >
{ 
public :
    typedef IVConverter< TOUTPUT , TINPUTs ... > TCONVTR ;
    typedef IVRUniqHub < TID     , TINPUTs ... > TIMPROT ;  

    template< typename TSYS >
    TVRSystematicConverter( TSYS & sys )
        : m_hub( sys )
    { 
    }
    ~TVRSystematicConverter()
    {
    }

public :
    virtual void Input ( IVSlot< TINPUTs > & ... inputs ) 
    {
        m_hub.BatchConnect( inputs ... ) ;
    }

    virtual void Output( IVInputPort< TOUTPUT > & inp ) 
    {
        m_hub.Output( inp ) ;
    }
     
private : 
    TVHub< IVRLTN< TID    > >   m_hub ;   
} ;

template< typename TSYS > class TVRSvcSystematicConverter ; 
template< typename TID >
class TVRSvcSystematicConverter< IVSysDynamic< IVRLTN< TID > > >
{
public :
    typedef IVRLTN< TID >                                                                            ROOT     ; 
    typedef TVRSystematicConverter< typename ROOT::ID , typename ROOT::NAME , typename ROOT::VALUE > ROOTPIPE ;
    typedef typename ROOTPIPE::TCONVTR                                                               INTERF   ;
//
public :
    template< typename TLIB >
    TVRSvcSystematicConverter( TLIB & lib )
    {
        lib.Create( VD_P2U( m_proxySys ) ) ;
    }
    ~TVRSvcSystematicConverter()
    {
        m_svcPipe.Close() ;
    }
//public : 
    void Elapse()
    {
         m_proxySys.UseServer( []( auto & ds ){ ds.Elapse() ; } ) ;
    }
    void Close()
    {
        m_svcPipe.Close() ;
    } 
    void RunService( VO< INTERF > & usr )
    { 
        m_proxySys.UseServer( [ this , &usr ]( auto & sys ){
            m_svcPipe.RunNew( usr , sys ) ;   
        }) ; 
    }

private :
   TVServiceProxy< IVSysDynamic< IVRLTN< TID > > > m_proxySys ;
   TVServiceArray< INTERF , ROOTPIPE >             m_svcPipe  ;
} ;

template< typename TLIB , typename TSYS , void FNAME( IVUser< TLIB > & usr ) > class TVRSvcAsynSystematicConverter ; 
template< typename TLIB , typename TID  , void FNAME( IVUser< TLIB > & usr ) >
class TVRSvcAsynSystematicConverter< TLIB , IVSysDynamic< IVRLTN< TID > > , FNAME >
{
public :
    typedef TVRSvcSystematicConverter< IVSysDynamic< IVRLTN< TID > > > sys_imp_type ; 
    typedef typename sys_imp_type::INTERF                              interf_type  ;

    TVRSvcAsynSystematicConverter() 
        : m_pImp( 0 )
    {
        mutexEnd.lock() ;

        std::thread trd( TVRSvcAsynSystematicConverter::_run , FNAME , this ) ;
        std::unique_lock<std::mutex> ulock( mutexBgn ) ;
        condbgn.wait( ulock, [ this ]{  return m_pImp != 0 ; } ) ;

        m_thread.swap( trd ) ;
    } 
    ~TVRSvcAsynSystematicConverter()
    {
        mutexEnd.unlock() ;   
        std::unique_lock<std::mutex> ulock( mutexBgn ) ;
        condbgn.wait( ulock, [ this ]{  return m_pImp == 0 ; } ) ;  
        m_thread.join() ;
    } ;
    
public :
    void RunService( VO< interf_type > & usr )
    {
        VASSERT( m_pImp != 0 ) ;
        m_pImp->RunService( usr ) ;
    }

    void Elapse()
    {
        VASSERT( m_pImp != 0 ) ;
        m_pImp->Elapse() ;
    }

    void Close() 
    {
        VASSERT( m_pImp != 0 ) ;
        m_pImp->Close() ;
    }
    
    template< typename T >
    void Init( const T & v )
    {
    }

private :
    static void _run( void f( IVUser< TLIB > & usr ) , TVRSvcAsynSystematicConverter * pObj )
    {  
        f( VD_L2U( [ pObj ]( TLIB & lib ){  
            _runWithLib( lib , pObj ) ; 
            _waitforusing( pObj ) ;
            _release( pObj ) ;
        } ) ) ; 
    } 

    static void _runWithLib( TLIB & lib , TVRSvcAsynSystematicConverter * pObj )
    {
        std::lock_guard<std::mutex> guard( pObj->mutexBgn ) ; 
        pObj->m_pImp = new sys_imp_type( lib ) ; 
        pObj->condbgn.notify_all() ;
    }
    
    static void _waitforusing( TVRSvcAsynSystematicConverter * pObj )
    {
        std::lock_guard<std::mutex> guard( pObj->mutexEnd ) ;  
    }

    static void _release( TVRSvcAsynSystematicConverter * pObj ) 
    {         
        std::lock_guard<std::mutex> guard( pObj->mutexBgn ) ; 

        delete pObj->m_pImp ;
        pObj->m_pImp = 0 ; 
        pObj->condbgn.notify_all() ; 
    } 

private :
     
    std::mutex              mutexBgn    ;
    std::mutex              mutexEnd    ;
    std::condition_variable condbgn     ;
    std::condition_variable condend     ;
    sys_imp_type   *        m_pImp      ;
    std::thread             m_thread    ;
} ; 

/////////////////////////////////////////////////////////////

// Macros of system implementation 

////////////////////////////////////////////////////////////

template< typename T , T , typename TCONSTRUCT > struct TVConverterTraitBase ;

template< typename TIMP , typename R , typename TOUTPUT , typename ... TINPUTs , R (TIMP::*mf)( TOUTPUT & , const TINPUTs & ... ) , typename TCONSTRUCT >
struct TVConverterTraitBase< R ( TIMP::* )( TOUTPUT & , const TINPUTs & ... ) , mf , typename TCONSTRUCT > 
{
    typedef TVSvcConverter< TIMP , TOUTPUT , TINPUTs ... > server_type ;
    typedef IVConverter< TOUTPUT , TINPUTs ... >           node_type   ;
} ; 

template< typename TIMP , typename R , typename ... TINPUTs , R (TIMP::*mf)( const TINPUTs & ... ) , typename TCONSTRUCT >
struct TVConverterTraitBase< R ( TIMP::* )( const TINPUTs & ... ) , mf , typename TCONSTRUCT > 
{
    typedef TVSvcConverter< TIMP , void , TINPUTs ... > server_type ;
    typedef IVConverter< void , TINPUTs ... >           node_type   ;
} ; 

template< typename TR , typename TOU , typename TIN > struct TVRelation2ConvertBase ;
template< typename TRID , typename TOU , typename ... TINPUTS > 
struct TVRelation2ConvertBase< IVRLTN< TRID > , TOU , VI_AND< TINPUTS ... > >
{ 
    typedef IVConverter< TOU , TINPUTS ... > type   ; 
} ;

template< typename TRID >
using TVRelation2Convert = TVRelation2ConvertBase< IVRLTN< TRID > , typename IVRLTN< TRID >::VALUE , typename IVRLTN< TRID >::NAME > ;
// 
//template< typename T , typename ... TSUBs > class TVExternalSystemExpandServerBase ;
//
//template< typename TRID , typename TOU , typename ... TINPUTS >  
//class TVExternalSystemExpandServerBase< IVRUniqHub< TRID , TOU , TINPUTS ... > >
//{
//public :
//    typedef IVConverter< TOU , TINPUTS ... > node_type   ;  
//
//private :
//    class VNode : public node_type
//    {
//    public :
//        VNode( IVSysDynamic< IVRLTN< TRID > > & sys )
//        {
//            m_hub.Create( sys ) ; 
//        }
//        ~VNode ()
//        { 
//        }
//    public :
//        virtual void Input ( IVSlot< TINPUTS > & ... inputs ) 
//        {
//            m_hub.BatchConnect( inputs ... ) ;
//        }
//        virtual void Output( IVInputPort< TOU > & op )
//        {
//            m_hub.Output( op ) ;
//        }
//
//    private :
//        TVHub< IVRLTN< TRID > > m_hub ;
//    } ;
//
//public :
//    void Elapse()
//    {
//        m_extSysProxy.UseServer( []( auto & sys ){
//            sys.Elapse() ;
//        }) ;
//    }
//    void Close()
//    {
//        m_svcNode.Close() ;
//    } 
//    //template< void >
//    template< typename ... TAs >
//    void Init( const TVPipelineArg< TAs ... > & v )
//    {   
//        const VSESS< IVRLTN< TRID > > & es = v.Get< VSESS< IVRLTN< TRID > > >( ) ; 
//        es.pSysProv->Apply( VD_P2U( m_extSysProxy ) ) ;
//    }
//    void RunService( VO< node_type > & pipe )
//    {
//        m_extSysProxy.UseServer( [ this , & pipe ]( auto & sys ){
//            m_svcNode.RunNew( pipe , sys ) ;
//        }) ; 
//    }
//private : 
//    TVServiceProxy< IVSysDynamic< IVRLTN< TRID > > > m_extSysProxy ;
//    TVServiceArray< node_type , VNode > m_svcNode ;
//} ;
//
//template< typename TRID , typename TOU , typename ... TINPUTS >  
//class TVExternalSystemExpandServerBase< IVRUniqHub< TRID , VI_AND< TOU > , TINPUTS ... > > : public TVExternalSystemExpandServerBase< IVRUniqHub< TRID , TOU , TINPUTS ... > >
//{
//public :
//    typedef TVExternalSystemExpandServerBase< IVRUniqHub< TRID , TOU , TINPUTS ... > > base_type ;
//    typedef IVConverter< TOU , TINPUTS ... > node_type   ;  
//    using  base_type::base_type ;
//} ;
//
//template< typename TRID , typename TOU , typename ... TINPUTS , typename ... TSUBs >  
//class TVExternalSystemExpandServerBase< IVRUniqHub< TRID , VI_AND< TOU > , TINPUTS ... > , TSUBs ... >
//{
//public :
//    typedef IVConverter< TOU , TINPUTS ... > node_type   ;  
//
//private :
//    class VNode : public node_type
//    {
//    public :
//        VNode( IVSysDynamic< IVRLTN< TRID > > & sys )
//        {
//            m_hub.Create( sys ) ; 
//        }
//        ~VNode ()
//        { 
//        }
//    public :
//        virtual void Input ( IVSlot< TINPUTS > & ... inputs ) 
//        {
//            m_hub.BatchConnect( inputs ... ) ;
//        }
//        virtual void Output( IVInputPort< TOU > & op )
//        {
//            m_hub.Output( op ) ;
//        }
//
//    private :
//        TVHub< IVRLTN< TRID > > m_hub ;
//    } ;
//
//public :
//    void Elapse()
//    {
//        static_assert(0,"") ;
//        m_extSysProxy.UseServer( []( auto & sys ){
//            sys.Elapse() ;
//        }) ;
//    }
//    void Close()
//    {
//        m_svcNode.Close() ;
//    } 
//    //template< void >
//    template< typename ... TAs >
//    void Init( const TVPipelineArg< TAs ... > & v )
//    {   
//        const VSESS< IVRLTN< TRID > > & es = v.Get< VSESS< IVRLTN< TRID > > >( ) ; 
//        es.pSysProv->Apply( VD_P2U( m_extSysProxy ) ) ;
//    }
//    void RunService( VO< node_type > & pipe )
//    {
//        m_extSysProxy.UseServer( [ this , & pipe ]( auto & sys ){
//            m_svcNode.RunNew( pipe , sys ) ;
//        }) ; 
//    }
//private : 
//    TVServiceProxy< IVSysDynamic< IVRLTN< TRID > > > m_extSysProxy ;
//    TVServiceArray< node_type , VNode > m_svcNode ;
//} ;
//
//template< typename TES , typename ... T > class TVExternalSystemNodeServer ;
//
//template< typename TRID > class TVExternalSystemNodeServer< IVRLTN< TRID > >
//    : public TVExternalSystemExpandServerBase< typename IVRLTN< TRID >::HUB >
//{
//public :
//    typedef TVExternalSystemExpandServerBase< typename IVRLTN< TRID >::HUB > base_type ;
//    using base_type::base_type ;
//    typedef typename base_type::node_type node_type ;
//} ;
// 
//template< typename TRID , typename TRSUBID , typename ... TSUBs >
//class TVExternalSystemNodeServer< IVRLTN< TRID > , IVRLTN< TRSUBID > , TSUBs ... > 
//{
//public :
//    typedef TVExternalSystemNodeServer< IVRLTN< TRSUBID > , TSUBs ... > sub_type ;
//    typedef TVExternalSystemExpandServerBase< typename IVRLTN< TRID >::HUB > bridge_type ;
//    typedef typename sub_type::node_type node_type ;
//    typedef typename bridge_type::node_type bridge_node_type ;
//
//public : 
//    TVExternalSystemNodeServer()
//    {
//        //static_assert( 0 , "" ) ;
//    }
//    ~TVExternalSystemNodeServer()
//    {}
//
//public :
//    template< typename ... TAs >
//    void Init( const TVPipelineArg< TAs ... > & v )
//    {
//        m_bridge.Init( v ) ;
//        
//        TVServiceProxy< IVConverter< VSESS< IVRLTN< TRSUBID > > > > extProxy ;
//        m_bridge.RunService( extProxy ) ;
//
//    } 
//    void Elapse()
//    {  
//    } 
//    void RunService( VO< node_type > & pipe ){}
//    template< typename ... TAs >
//    void Close()
//    {}
//private :
//    bridge_type  m_bridge ;
//} ;

template< typename TRID , typename TRSRCID , typename TI , typename TO > class TVRelationNodeServerBase ;

template< typename TRID , typename TRSRCID , typename ... TINPUTs , typename ... TOUTPUTs  >  
class TVRelationNodeServerBase< IVRLTN< TRID > , IVRLTN< TRSRCID > , VI_AND< TINPUTs ... > , VI_AND< TOUTPUTs ... > >
{
public :
    TVRelationNodeServerBase( const VSESS< IVRLTN< TRSRCID > > & src )
        : m_extSystem( src )
    {
        m_extSystem.pSysProv->Apply( VD_P2U( m_sysProxy )) ;
    }

public :
    void Build( TOUTPUTs & ... vOuts , const TINPUTs & ... vIns ) 
    {
    }

private :
    const VSESS< IVRLTN< TRSRCID > > & m_extSystem ;
    TVServiceProxy< IVSysDynamic< IVRLTN< TRSRCID > > > m_sysProxy ;
} ; 

template< typename TRID , typename TRSRCID , typename ... TINPUTs , typename TOUs  >  
class TVRelationNodeServerBase< IVRLTN< TRID > , IVRLTN< TRSRCID > , VI_AND< TINPUTs ... > , TOUs >
    : public TVRelationNodeServerBase< IVRLTN< TRID > , IVRLTN< TRSRCID > , VI_AND< TINPUTs ... > , VI_AND< TOUs > >
{
public :
    typedef TVRelationNodeServerBase< IVRLTN< TRID > , IVRLTN< TRSRCID > , VI_AND< TINPUTs ... > , VI_AND< TOUs > > base_type ;
    using base_type::base_type ;
} ; 

template< typename ... T >  class TVRelationNodeServer ;

template< typename TRID , typename TRSRCID >  
class TVRelationNodeServer< IVRLTN< TRID > , IVRLTN< TRSRCID > >
    : public TVRelationNodeServerBase< IVRLTN< TRID > , IVRLTN< TRSRCID > , typename IVRLTN< TRID >::NAME , typename IVRLTN< TRID >::VALUE >
{
public :
    typedef TVRelationNodeServerBase< IVRLTN< TRID > , IVRLTN< TRSRCID > , typename IVRLTN< TRID >::NAME , typename IVRLTN< TRID >::VALUE > base_type ;
    using base_type::base_type ;
} ;

template< typename ... TDEPs >
class TVConverterDep
{
} ;

template< typename ... T > struct TVConverterTrait ;

template< typename TIMP , typename ... TRDEPIDs >
struct TVConverterTrait< TIMP , TRDEPIDs ... >  
     : TVConverterTraitBase< decltype( &TIMP::Build ) , &TIMP::Build , TVConverterDep< TRDEPIDs ... > > 
{} ;
 

template< typename TRID , typename ... TRDEPIDs >
struct TVConverterTrait< IVRLTN< TRID > , TRDEPIDs ... > : TVConverterTrait< TVRelationNodeServer< IVRLTN< TRID > , TRDEPIDs ... > >
{} ;

// For External system node
//template< typename TRID , typename ... TSUBs >
//struct TVConverterTrait< IVRLTN< TRID > , TSUBs ... >
//{
//    typedef TVExternalSystemNodeServer< IVRLTN< TRID > , TSUBs ... > server_type ;
//    typedef typename server_type::node_type node_type ; 
//} ;

template< typename TITEM >
struct TVConverterArrTrait
{
    typedef TVSvcCollector< TITEM >             server_type ;
    typedef typename server_type::interf_type   node_type   ;
} ;

template< typename TITEM >
struct TVConverterSwitchTrait
{
    typedef TVSvcSwitcher< TITEM >              server_type ;
    typedef typename server_type::interf_type   node_type   ;
};

template< typename TLIB , typename TSYS , void FNAME( IVUser< TLIB > & libUsr ) > 
struct TVConverterSysTrait
{
    typedef TVRSvcAsynSystematicConverter< TLIB , TSYS , FNAME > server_type ; 
    typedef typename server_type::interf_type   node_type   ;
} ;

template< typename TI , typename TO , TI , TO > struct TVConverterCtrlTraitBase ;

template< typename TIMP , typename R , typename ... TINPUTs , typename ... TOUTINs 
        , R (TIMP::* mfin)( const TINPUTs & ... ) 
        , void (TIMP::* mfout)( IVInputPort< TOUTINs ... > & ) >
struct TVConverterCtrlTraitBase< R ( TIMP::* )( const TINPUTs & ... ) 
                              ,  void ( TIMP::* )( IVInputPort< TOUTINs ... > & ) 
                              , mfin 
                              , mfout > 
{
    typedef TVSvcDuplicate< TIMP , VI_AND< TOUTINs ... > , TINPUTs ... > server_type ;
    typedef IVConverter< VI_AND< TOUTINs ... > , TINPUTs ... >           node_type   ;
} ;  

template< typename TIMP >
using TVConverterCtrlTrait = TVConverterCtrlTraitBase< decltype( &TIMP::Update ) 
                                                    , decltype( &TIMP::Output ) 
                                                    , &TIMP::Update , &TIMP::Output > ;


template< unsigned id > struct VPipeNodeID {} ;

template< typename TNID , typename TCNVR > struct TVDeduceConverterWrapper ;

template< unsigned TNID , typename TOUTPUT , typename ... TINPUTs >
struct TVDeduceConverterWrapper< VPipeNodeID< TNID > , IVConverter< TOUTPUT , TINPUTs ... > >
{
    typedef TVConverterWrapper< TOUTPUT , TINPUTs ... > type ;
} ;


template< unsigned > struct TVHelperCounter
{
};

#define VDDP_NODE_IMP_WITHID( nID , name , trait , ... )   typename trait::server_type m_sysSvc##name ;\
    typedef typename TVDeduceConverterWrapper< VPipeNodeID< nID > , typename trait::node_type >::type name ;\
    void Create##name( VO< typename trait::node_type > & usr )\
    { m_sysSvc##name.RunService( usr ); }\
    template< typename TFST , typename ... TINs >\
    void Create##name( name & usr , TFST & fstInput , TINs & ... inputs )\
    {  m_sysSvc##name.RunService( usr );  VLNK( usr , fstInput , inputs ... ) ; }\
    VD_DECLAREEC_IMP( m_sysSvc##name , nID )

#define VDDP_NODE_IMP( name , trait , ... ) VDDP_NODE_IMP_WITHID( VD_EXPAND( __COUNTER__ ) , name , trait , __VA_ARGS__ )


#define VD_DECLAREEC_IMP( sysname , c ) void LoopElapse( const TVHelperCounter< c > * ) \
                                                { LoopElapse( reinterpret_cast< TVHelperCounter< c - 1 > * >( 0 ) ) ; sysname.Elapse() ; } \
                                         void LoopClose( const TVHelperCounter< c > * ) \
                                                { LoopClose( reinterpret_cast< TVHelperCounter< c - 1 > * >( 0 ) ) ; sysname.Close() ; } \
                                         template< typename T > void LoopInit( const TVHelperCounter< c > * , const T & v ) \
                                                { LoopInit<T>( reinterpret_cast< TVHelperCounter< c - 1 > * >( 0 ) , v ) ; sysname.Init( v ) ; } 

#define VD_BEGIN_PIPELINE_IMP( c )      void LoopElapse( const TVHelperCounter< c > * ) {}  \
                                        void LoopClose( const TVHelperCounter< c > * ) {} \
                                        template< typename T > void LoopInit( const TVHelperCounter< c > * , const T & v ){}

#define VD_END_PIPELINE_IMP( c )       public : void Elapse() { LoopElapse( reinterpret_cast< TVHelperCounter< c - 1 > * >( 0 ) ) ; } \
                                                void Close() { LoopClose( reinterpret_cast< TVHelperCounter< c - 1 > * >( 0 ) ) ; } \
                                       private : template< typename T > void _initialize( const T & v ){ LoopInit<T>( reinterpret_cast< TVHelperCounter< c - 1 > * >( 0 ) , v ) ; } 

#define VD_BEGIN_PIPELINE2( plname , envname ) class plname { \
    public : typedef envname DEPENDENCE ; plname( const envname & env ):m_Env(env){ _initialize(env) ; } ~plname(){ Close() ; } \
        const envname & GetEnv() { return m_Env; }\
    private : const envname & m_Env; \
    public : \
        VD_BEGIN_PIPELINE_IMP( VD_EXPAND( __COUNTER__ ) ) 


#define VD_BEGIN_PIPELINE( plname , ... ) class plname { \
    public : typedef TVPipelineArg< __VA_ARGS__ > DEPENDENCE ; \
            template< typename ... TARGs > \
            plname( const TARGs & ... args ):m_Env( args ... ){ _initialize( m_Env ) ; } ~plname(){ Close() ; } \
        const DEPENDENCE & GetEnv() { return m_Env; }\
        private : const DEPENDENCE m_Env; \
        public :\
        VD_BEGIN_PIPELINE_IMP( VD_EXPAND( __COUNTER__ ) )
#define VD_END_PIPELINE()   VD_END_PIPELINE_IMP( VD_EXPAND( __COUNTER__ ) ) } ;


#define VDDP_ARR( name , type )  VDDP_NODE_IMP( name , TVConverterArrTrait< type > )  
#define VDDP_SWT( name , type )  VDDP_NODE_IMP( name , TVConverterSwitchTrait< type > )  
#define VDDP_NOD( name , ...  )  VDDP_NODE_IMP( name , TVConverterTrait< __VA_ARGS__ > , __VA_ARGS__ )
#define VDDP_SYS( name , lib , factory , sys )  static void _get_lib_name_of##name( IVUser< factory > & usr ) \
                                        { VD_USESTDLIBERRARY( lib , [ & usr ]( factory & lib ){ usr.Visit( lib ) ; } ) ;}\
                                        typedef TVConverterSysTrait< factory , sys , _get_lib_name_of##name > sys_svc_type_of##name ;\
                                        VDDP_NODE_IMP( name , sys_svc_type_of##name ) 
#define VDDP_CTR( name , type )  typedef TVConverterCtrlTrait< type > sys_svc_type_of##name ;\
                                VDDP_NODE_IMP( name , sys_svc_type_of##name )  

#define VDDP_USR( name , trait ) VDDP_NODE_IMP( name , trait )



////////////////////////////////////////////////////////////////////////////////////////////////

// (4-2) another system implements 

////////////////////////////////////////////////////////////////////////////////////////////////
template< typename TSysTrait , typename TEnv , typename TINTF , typename ... TRs > class TVDynamicSystemImpBase3;

template< typename TSysTrait , typename TINTF >
class TVDynamicSystemImpBase3< TSysTrait , std::tuple<> , TINTF > : public TINTF
{
public:
    TVDynamicSystemImpBase3( const std::tuple<> & env )
    {}
    ~TVDynamicSystemImpBase3()
    {}
    void Close()
    {}
protected:
    typedef typename TSysTrait::PIPELINE    TPIPELINE;
    TPIPELINE & GetPipeLine()
    {
        return m_pipeLine;
    }

    //TEnv & GetEnv()
    //{
    //    return m_Env;
    //}

public:
    virtual void Elapse()
    {
        m_pipeLine.Elapse();
    }

private:
    TPIPELINE   m_pipeLine ;
};

template< typename TSysTrait , typename TEnv , typename TINTF >
class TVDynamicSystemImpBase3< TSysTrait , TEnv , TINTF > : public TINTF
{
public:
    TVDynamicSystemImpBase3( const TEnv & env ) :m_pipeLine( env )
    {}
    ~TVDynamicSystemImpBase3()
    {}
    void Close()
    {}
protected:
    typedef typename TSysTrait::PIPELINE    TPIPELINE;
    TPIPELINE & GetPipeLine()
    {
        return m_pipeLine;
    }

    //TEnv & GetEnv()
    //{
    //    return m_Env;
    //}

public:
    virtual void Elapse()
    {
        m_pipeLine.Elapse();
    }

private:
    TPIPELINE   m_pipeLine;
    //TEnv &      m_Env;
};

template< typename TSysTrait , typename TEnv , typename TINTF , typename TR , typename ... TROTHERs >
class TVDynamicSystemImpBase3< TSysTrait , TEnv , TINTF , TR , TROTHERs ... >
    : public TVDynamicSystemImpBase3< TSysTrait , TEnv , TINTF , TROTHERs ... >
{
public:
    typedef TVDynamicSystemImpBase3< TSysTrait , TEnv , TINTF , TROTHERs ... > base_type;
    typedef typename TR::HUB                                            interf_type;
    typedef TVRHubImp< TSysTrait , TR  >                               svr_type;

public:
    TVDynamicSystemImpBase3( const TEnv & env ) :base_type( env )
    {}
    ~TVDynamicSystemImpBase3()
    {
        m_svc.Close();
    }

public:
    virtual void Close()
    {
        base_type::Close();
    }
    virtual void Create( VO< interf_type > & usr )
    {
        m_svc.RunNew( usr , this->GetPipeLine() );
    }

private:
    TVServiceArray< interf_type , svr_type > m_svc;
};

template< typename TSysTrait , typename TEnv , typename TSYS > class TVDynamicSystemImp3;

template< typename TSysTrait , typename TEnv , typename ... TRs >
class TVDynamicSystemImp3< TSysTrait , TEnv , IVSysDynamic< TRs ... > >
    : public TVDynamicSystemImpBase3< TSysTrait , TEnv , IVSysDynamic< TRs ... > , TRs ... >
{
public:
    typedef TVDynamicSystemImpBase3< TSysTrait , TEnv , IVSysDynamic< TRs ... > , TRs ... > IMP_BASE;

public:
    TVDynamicSystemImp3( const TEnv & env ) :IMP_BASE( env )
    {}
    ~TVDynamicSystemImp3()
    {
        IMP_BASE::Close();
    }
};

#define VD_BEGIN_PIPELINE2( plname , envname ) class plname { \
    public : typedef envname DEPENDENCE ; plname( const envname & env ):m_Env(env){ _initialize(env) ; } ~plname(){ Close() ; } \
        const envname & GetEnv() { return m_Env; }\
    private : const envname & m_Env; \
    public : \
        VD_BEGIN_PIPELINE_IMP( VD_EXPAND( __COUNTER__ ) ) 

#define VD_END_PIPELINE2()   VD_END_PIPELINE_IMP( VD_EXPAND( __COUNTER__ ) ) } ;

template< typename TTuple , size_t val , typename ... Ts > class TVTupleDataGetterHelper;

template< typename TTuple , size_t val , typename TFst , typename ... TOthers >
class TVTupleDataGetterHelper< TTuple , val , TFst , TOthers ... > : public TVTupleDataGetterHelper< TTuple , val + 1 , TOthers ... >
{
public:
    TVTupleDataGetterHelper(){}
    ~TVTupleDataGetterHelper(){}

    template< typename FUNC , typename ... TDatas >
    void Use( TTuple & tup , FUNC f , TDatas & ... dats )
    {
        TVTupleDataGetterHelper< TTuple , val + 1 , TOthers ... >::Use( tup , f , dats ... , std::get<val>( tup ) );
    }
};

template< typename TTuple , size_t val >
class TVTupleDataGetterHelper< TTuple , val >
{
public:
    TVTupleDataGetterHelper(){}
    ~TVTupleDataGetterHelper(){}

    template< typename FUNC , typename ... TDatas >
    void Use( TTuple & tup , FUNC f , TDatas & ... dats )
    {
        f( dats ... );
    }
};

template< typename ... Ts > class TVTupleDataGetterHelperGen;

template< typename ... TTuples >
class TVTupleDataGetterHelperGen< std::tuple< TTuples ... > >
    : public TVTupleDataGetterHelper< std::tuple< TTuples ... > , 0 , TTuples ... >
{};

template< typename TTuple > 
class TVTupleDataGetter : public TVTupleDataGetterHelperGen< TTuple >
{
public:
    TVTupleDataGetter(){}
    ~TVTupleDataGetter(){}

    template< typename TFunc >
    void Use( TTuple & tup , TFunc func )
    {
        TVTupleDataGetterHelperGen< TTuple >::Use( tup , func );
    }
};

template<>
class TVTupleDataGetter< std::tuple<> >
{
public:
    TVTupleDataGetter(){}
    ~TVTupleDataGetter(){}

    template< typename Func >
    void Use( std::tuple<> & tup , Func func )
    {
        func();
    }
};

template< typename T > class TVLNKObj : public std::false_type{};

template< typename TLNK > class TVLNKObj< IVSlot     < TLNK > > : public std::true_type{};
template< typename TLNK > class TVLNKObj< TVSource   < TLNK > > : public std::true_type{};
template< typename TLNK > class TVLNKObj< TVData     < TLNK > > : public std::true_type{};
template< typename TLNK > class TVLNKObj< IVInputPort< TLNK > > : public std::true_type{};
template< typename TLNK > class TVLNKObj< TVComp     < TLNK > > : public std::true_type{};
template< typename TLNK > class TVLNKObj< TVAggr     < TLNK > > : public std::true_type{};

template< typename TTuple , typename ... TParams > class TVCreateSpliterFinish;

template< typename ... TCons , typename ... TParams >
class TVCreateSpliterFinish< std::tuple< TCons & ... > , TParams ... >
{
public:
    template< typename CreateFunc , typename LinkFunc >
    TVCreateSpliterFinish( CreateFunc cf , LinkFunc lf , std::tuple< TCons & ... > & tup , TParams & ... params )
    {
        TVTupleDataGetter< std::tuple< TCons & ... > > tdg;
        tdg.Use( tup , std::function< void( TCons & ...  ) >( [&cf]( TCons & ... cons ){
            cf.Build( cons... );
        } ) );

        lf.Build( params ... );
    }
    ~TVCreateSpliterFinish(){}
};

template< typename TTuple , typename ... TParams > class TVCreateSpliter;

template< bool lnk , typename TTuple , typename TParam , typename ... TParams > class TVCreateSpliterHelper;

template< typename ... TCons , typename TParam , typename ... TParams >
class TVCreateSpliterHelper< false , std::tuple<TCons & ...> , TParam , TParams ... > 
    : public TVCreateSpliter< std::tuple<TCons & ... , TParam & > , TParams ... >
{
public:
    typedef TVCreateSpliter< std::tuple<TCons & ... , TParam & > , TParams ... > PARENT;
    
    template< typename CreateFunc , typename LinkFunc >
    TVCreateSpliterHelper( CreateFunc cf , LinkFunc lf , std::tuple< TCons & ... > & tup , TParam & param , TParams & ... params )
        :PARENT( cf , lf , std::tuple_cat( tup , std::tuple< TParam & >(param) ) , params ... )
    {}
    ~TVCreateSpliterHelper(){}
};

template< typename ... TCons , typename TParam , typename ... TParams >
class TVCreateSpliterHelper< true , std::tuple<TCons & ...> , TParam , TParams ... >
    : public TVCreateSpliterFinish< std::tuple<TCons & ... > , TParam , TParams ... >
{
public:
    typedef TVCreateSpliterFinish< std::tuple<TCons & ... > , TParam , TParams ... > PARENT;
    using PARENT::PARENT;
};

template< typename ... TCons , typename TParam , typename ... TParams >
class TVCreateSpliter< std::tuple< TCons & ... > , TParam , TParams ... > 
    : public TVCreateSpliterHelper< TVLNKObj<TParam>::value , std::tuple< TCons & ... > , TParam , TParams ... >
{
public:
    typedef TVCreateSpliterHelper< TVLNKObj<TParam>::value , std::tuple< TCons & ... > , TParam , TParams ... > PARENT;

    template< typename CreateFunc , typename LinkFunc >
    TVCreateSpliter( CreateFunc cf , LinkFunc lf , std::tuple< TCons & ... > & tup , TParam & param , TParams & ... params )
        :PARENT( cf , lf , tup , param , params ... )
    {}
    ~TVCreateSpliter(){}
};

template< typename ... TCons >
class TVCreateSpliter< std::tuple< TCons & ... > >
{
public:
    template< typename CreateFunc , typename LinkFunc >
    TVCreateSpliter( CreateFunc cf , LinkFunc lf , std::tuple< TCons & ... > & tup )
    {
        TVTupleDataGetter< std::tuple< TCons & ... > > tdg;
        tdg.Use( tup , std::function< void( TCons & ...  ) >( [ &cf ]( TCons & ... cons ) {
            cf.Build( cons... );
        } ) );
    }
    ~TVCreateSpliter(){}
};

template< typename TServerType , typename TUsr >
class TVCreateFunc
{
public:
    TVCreateFunc( TServerType & sys , TUsr & usr ):m_Sys(sys),m_Usr(usr){}
    ~TVCreateFunc(){}

    template< typename ... TCons >
    void Build( TCons & ... cons )
    {
        m_Sys.RunService( m_Usr , cons ... );
    }

private:
    TServerType & m_Sys;
    TUsr        & m_Usr;
};

template< typename TUsr >
class TVLinkFunc
{
public:
    TVLinkFunc(TUsr & usr):m_Usr(usr){}
    ~TVLinkFunc(){}

    template< typename ... TLnks >
    void Build( TLnks & ... lnks )
    {
        VLNK( m_Usr , lnks... );
    }

private:
    TUsr        & m_Usr;
};

#define VDDP_NODE_IMP2_WITHID( nID , name , trait , ... ) typename trait::server_type m_sysSvc##name ;\
    typedef typename TVDeduceConverterWrapper< VPipeNodeID< nID > , typename trait::node_type >::type name ;\
    template< typename ... TParams >\
    void Create##name( name & usr , TParams & ... params )\
    { TVCreateSpliter<std::tuple<> , TParams ... > cs( TVCreateFunc<typename trait::server_type , name >( m_sysSvc##name , usr ), TVLinkFunc< name >( usr ) , std::tuple<>() , params ... ); }\
    VD_DECLAREEC_IMP( m_sysSvc##name , nID )
 
#define VDDP_NODE_IMP2( name , trait ) VDDP_NODE_IMP2_WITHID( VD_EXPAND( __COUNTER__ ) , name , trait )

/////////////////////////////////////////////////////////////////////////////////
// Converter 2
/////////////////////////////////////////////////////////////////////////////////

template< typename TIMP , typename TEXP >
class TVConverterData2
{
public:
    template< typename ... TEnvs >
    TVConverterData2( TEnvs & ... envs )
        : m_instCnvtr( envs ... )
#ifdef VD_NOT_CARE_PERFORMANCE
        , m_bDataValid( false )
#endif    
    {}
    ~TVConverterData2()
    {
        m_svcExp.Close();
        m_svcProv.Close();
    }

public:
    void Close()
    {
        m_svcExp.Close();
        m_svcProv.Close();
    }

    void RunProvider( VO< IVDataProv< TEXP > > & usr )
    {
        m_svcProv.RunNew( usr , *this );
    }

    template< typename ... TARGs >
    bool Update( const TARGs & ... args )
    {
        return _call( &TIMP::Build , m_instCnvtr , m_expValue , args ... );
    }

    virtual void SignalDirty() = 0;

#ifdef VD_NOT_CARE_PERFORMANCE
    void Validate( bool bv )
    {
        m_bDataValid = bv;
    }
    bool m_bDataValid;
#endif

private:
    typedef TVConverterData2< TIMP , TEXP > my_type;
    struct ProvAdp : public IVDataProv< TEXP >
    {
        my_type & me;
        ProvAdp( my_type & m ) :me( m )
        {}
        ~ProvAdp()
        {}
        void Apply( VO< TVConstRef< TEXP > > & usr )
        {
            me._applyExpData( usr );
        }
    };

    void _applyExpData( VO< TVConstRef< TEXP > > & usr )
    {
#ifdef VD_NOT_CARE_PERFORMANCE
        VASSERT( m_bDataValid );
        if ( m_bDataValid )
        {
            m_svcExp.RunNew( usr , m_expValue );
        }
#else
        m_svcExp.RunNew( usr , m_expValue );
#endif
    }

private:
    template< typename ... TARGs >
    bool _call( bool ( TIMP::* f )( TEXP & expVal , const TARGs & ... ) , TIMP & cnvtr , TEXP & expVal , const TARGs & ... args )
    {
        return ( cnvtr.*f )( expVal , args ... );
    }

    template< typename ... TARGs >
    bool _call( void ( TIMP::* f )( TEXP & expVal , const TARGs & ... ) , TIMP & cnvtr , TEXP & expVal , const TARGs & ... args )
    {
        ( cnvtr.*f )( expVal , args ... );
        return true;
    }

    TIMP m_instCnvtr;
    TEXP m_expValue;
    TVServiceArray< TVConstRef< TEXP > >               m_svcExp;
    TVServiceArray< IVDataProv< TEXP > , ProvAdp     > m_svcProv;
};

template< typename TIMP >
class TVConverterData2< TIMP , void >
{
public:
    template< typename ... TEnvs >
    TVConverterData2( TEnvs & ... envs )
        : m_instCnvtr( envs ... )
#ifdef VD_NOT_CARE_PERFORMANCE
        , m_bDataValid( false )
#endif    
    {}

    ~TVConverterData2()
    { 
    }

public:
    void Close(){}

    void RunProvider( VO< IVDataProv< void > > & usr ){}

    template< typename ... TARGs >
    bool Update( const TARGs & ... args )
    {
        return _call( &TIMP::Build , m_instCnvtr , args ... );
    }

    virtual void SignalDirty() = 0;

#ifdef VD_NOT_CARE_PERFORMANCE
    void Validate( bool bv )
    {
        m_bDataValid = bv;
    }
    bool m_bDataValid;
#endif

private:
    template< typename ... TARGs >
    bool _call( bool ( TIMP::* f )( const TARGs & ... ) , TIMP & cnvtr , const TARGs & ... args )
    {
        return ( cnvtr.*f )( args ... );
    }

    template< typename ... TARGs >
    bool _call( void ( TIMP::* f )( const TARGs & ... ) , TIMP & cnvtr , const TARGs & ... args )
    {
        ( cnvtr.*f )( args ... );
        return true;
    }
    TIMP m_instCnvtr;
};

template< typename TIMP , typename TEXP , typename ... TINPUTs > struct TVConverterClntArg2;

template< typename TIMP , typename TEXP > struct TVConverterClntArg2< TIMP , TEXP >
{
    TVConverterClntArg2()
    {}
    ~TVConverterClntArg2()
    {}

    template< typename ... TARGs >
    bool Update( TVConverterData2< TIMP , TEXP > & expVal , const TARGs & ... args )
    {
        return expVal.Update( args ... );
    }
};

template< typename TIMP , typename TEXP , typename TI , typename ... TOTHERs > struct TVConverterClntArg2< TIMP , TEXP , TI , TOTHERs ... >
{
    typedef TVConverterClntArg2< TIMP , TEXP , TOTHERs ... > sub_type;
    TVConstRef< TI > & m_refArg;
    sub_type           m_subValue;

    TVConverterClntArg2( TVConstRef< TI > & rt , TVConstRef< TOTHERs > & ... rothers )
        : m_refArg( rt )
        , m_subValue( rothers ... )
    {};

    template< typename ... TARGs >
    bool Update( TVConverterData2< TIMP , TEXP > & expVal , const TARGs & ... args )
    {
        return m_subValue.template Update< TARGs ... , TI >( expVal , args ... , m_refArg.m_ref );
    }
};

template< typename TIMP , typename TEXP , typename ... TINPUTs >
class TVConverterClnt2
{
public:
    typedef TVConverterData2< TIMP , TEXP > data_type;
    typedef TVRef< data_type >              data_ref;

    template< typename FU >
    TVConverterClnt2( data_ref & refData , TVConstRef< TINPUTs > & ... spcArgs , FU f )
        : m_refArgs( spcArgs ... )
        , m_refData( refData )
    {
        m_svcSelf.Run( f , *this );
    }
    ~TVConverterClnt2()
    {
#ifdef VD_NOT_CARE_PERFORMANCE
        m_refData.m_ref.Validate( false );
#endif
        m_svcSelf.Close();
    }

public:
    typedef TVConverterClnt2< TIMP , TEXP , TINPUTs ... > my_type;
    typedef TVRef< my_type >                              my_ref;

    bool UpdateData()
    {
#ifdef VD_NOT_CARE_PERFORMANCE
        m_refData.m_ref.Validate( true );
#endif
        return m_refArgs.template Update<>( m_refData.m_ref );
    }

private:
    data_ref                                        & m_refData;
    TVService< my_ref >                               m_svcSelf;
    TVConverterClntArg2< TIMP , TEXP , TINPUTs ... >  m_refArgs;
};

template< typename TIMP , typename TEXP , typename ... TINPUTs > class TVConverterEntry2;

template< typename TIMP , typename TEXP >
class TVConverterEntry2< TIMP , TEXP >
{
public:
    typedef TVConverterData2< TIMP , TEXP > data_type;
    typedef TVRef< data_type >              data_ref;

    TVConverterEntry2( TVConverterData2< TIMP , TEXP > & tr )
        : m_notifier( tr )
    {}
    ~TVConverterEntry2(){}
    void Close()
    {
        m_svcData.Close();
    }
    void OnChanged()
    {
        m_notifier.SignalDirty();
    }
    template< class F , typename ... SPCs >
    void AccessData( F f , SPCs & ... spcs )
    {
        m_svcData.Run( [ f , &spcs ... ]( VI< data_ref > & spcData ) {
            f( spcData , spcs ... );
        } , m_notifier );
    }

private:
    TVConverterData2< TIMP , TEXP > & m_notifier;
    TVService< TVRef< TVConverterData2< TIMP , TEXP > > > m_svcData;
};

template< typename TIMP , typename TEXP , typename TI , typename ... TIOTHERs >
class TVConverterEntry2< TIMP , TEXP , TI , TIOTHERs ...  > : TVServer< IVTracer >
{
public:
    TVConverterEntry2( TVConverterData2< TIMP , TEXP > & tr , IVSlot< TI > & slot , IVSlot< TIOTHERs > & ... otherSlots )
        :m_subInputs( tr , otherSlots ... )
    {
        TVServer< IVTracer >::Run( [ &slot ]( auto & spc ) {
            slot.Trace( spc );
        } );

        slot.GetData( VD_P2U( m_prov ) );
    }

    ~TVConverterEntry2()
    {
        TVServer< IVTracer >::Close();
    }

    void Close()
    {
        m_subInputs.Close();
    }

public:
    virtual void OnChanged()
    {
        m_subInputs.OnChanged();
    }

public:
    template< class F , typename ... SPCs >
    void AccessData( F f , SPCs & ... spcs )
    {
        m_prov.UseServer( [ this , f , &spcs ... ]( auto & prov ){
            prov.Apply( VD_L2U( [ this , f , &spcs ... ]( VI< TVConstRef< TI > > & spcVal ){
                m_subInputs.AccessData( f , spcs ... , spcVal );
            } ) );
        } );
    }

private:
    typedef TVConverterEntry2< TIMP , TEXP , TIOTHERs ... > sub_type;
    sub_type m_subInputs;
    TVServiceProxy< IVDataProv< TI > > m_prov;
};

template< typename TIMP , typename TEXP , typename ... TINPUTs >
class TVConverterTracer2
{
public:
    template< class FU >
    TVConverterTracer2( TVConverterData2< TIMP , TEXP > & tr , FU f , IVSlot< TINPUTs > & ... slots )
        : m_inputs( tr , slots ... )
    {
        m_svcSelf.Run( f , *this );
    }
    ~TVConverterTracer2()
    {
        m_inputs.Close();
        m_svcSelf.Close();
    }

public:
    bool UpdateData()
    {
        _check_and_build_data_prov();

        bool bRtn( false );

        m_proxyClnt.UseServer( [ &bRtn ]( auto & c ) {
            bRtn = c.m_ref.UpdateData();
        } );

        return bRtn;
    }

private:
    typedef TVConverterData2< TIMP , TEXP > data_type;
    typedef TVRef< data_type >              data_ref;
    void _check_and_build_data_prov()
    {
        if ( m_proxyClnt.IsClosed() )
        {
            m_inputs.AccessData( [ this ]( VI< data_ref > & spcVal , VI< TVConstRef< TINPUTs > > & ... spcData ) {
                m_clntData.Create( spcVal , spcData ... , VD_MP2L( m_proxyClnt ) );
            } );
        }
    }

private:
    typedef TVConverterEntry2< TIMP , TEXP , TINPUTs ... >          inputs_type;
    typedef TVConverterClnt2< TIMP , TEXP , TINPUTs ... >           clnt_type;
    typedef TVRef< clnt_type >                                      clnt_ref;
    typedef TVConverterTracer2< TIMP , TEXP , TINPUTs ... >         my_type;
    typedef TVRef< my_type >                                        my_ref;

    inputs_type                                                   m_inputs;
    TVClient< clnt_type , data_ref , TVConstRef< TINPUTs > ... > m_clntData;
    TVServiceProxy< clnt_ref >                                    m_proxyClnt;
    TVService< my_ref >                                           m_svcSelf;
};

template< typename TIMP , typename TEXP , typename ... TINPUTs >
class TVConverter2 : public IVConverter< TEXP , TINPUTs ... >
    , TVServer< IVDirtyObject >
    , TVServer< TVConverterData2< TIMP , TEXP > >

    , IVSlot< TEXP >
{
private:
    typedef TVConverterData2< TIMP , TEXP >                     data_type;
    typedef TVConverterTracer2< TIMP , TEXP , TINPUTs ... >     data_clnt_type;

public:
    template< typename ... TEnvs >
    TVConverter2( VN_ELEMSYS::TVSysImpTimeline & tl , TEnvs & ... envs )
        : m_timeline( tl )
        , TVServer< TVConverterData2< TIMP , TEXP > >( envs ... )
    {
        _registerDirty();
    }
    ~TVConverter2()
    {
        data_type::Close();
        TVServer< IVDirtyObject >::Close();
        TVServer< data_type >::Close();
    }

public:
    virtual void Input( IVSlot< TINPUTs > & ... slots )
    {
        auto runner = [ this , &slots... ]( auto & spcTracer ){
            m_obsvr.Create( spcTracer , VD_MP2L( m_proxyObsvr ) , slots ... );
        };

        TVServer< data_type >::Run( runner );
    }
    virtual void Output( typename TVOutputPortParam< TEXP >::type & ip )
    {
        ip.Input( *this );
    }

private:
    virtual void Trace( VI< IVTracer > & spc )
    {
        m_listeners.Add( spc );
    }
    virtual void GetData( VO< IVDataProv< TEXP > > & usr )
    {
        data_type::RunProvider( usr );
    }
    virtual void SignalDirty()
    {
        if ( TVServer< IVDirtyObject >::IsClosed() )
        {
            _registerDirty();
        }
    }
    virtual void CleanAndDiffuse()
    {
        bool bInfect( false );

        m_proxyObsvr.UseServer( [ this , &bInfect ]( auto&t ) {
            bInfect = t.m_ref.UpdateData();
        } );

        if ( bInfect )
            m_listeners.TidyTravel( []( auto&t ) {t.OnChanged(); } );
    }

private:
    void _registerDirty()
    {
        TVServer< IVDirtyObject >::Run( [ this ]( auto & spcDirty ) {
            m_timeline.RegisterDirty( spcDirty );
        } );
    }

private:
    VN_ELEMSYS::TVSysImpTimeline                     & m_timeline;
    TVServiceProxyArray< IVTracer >                    m_listeners;
    TVClient< data_clnt_type , data_type   >           m_obsvr;
    TVServiceProxy< TVRef< data_clnt_type > >          m_proxyObsvr;
};

template< typename TIMP , typename TEXP , typename ... TINPUTs >
class TVSvcConverter2
{
public:
    TVSvcConverter2(){}
    ~TVSvcConverter2(){}

public:
    typedef TEXP                                     exp_type;
    typedef IVConverter< TEXP , TINPUTs ...        > interf_type;
    typedef TVConverter2< TIMP , TEXP , TINPUTs ... > imp_type;

public:
    void Close()
    {
        m_svc.Close();
    }
    void Elapse()
    {
        m_timeline.ClearDirty();
    }

    template< typename ... TEnvs >
    void RunService( VO< interf_type > & pipe , TEnvs & ... envs )
    {
        m_svc.RunNew( pipe , m_timeline , envs ... );
    }
        
    template< typename T >
    void Init( const T & v )
    {
    }

private:
    VN_ELEMSYS::TVSysImpTimeline m_timeline;
    TVServiceArray< interf_type , imp_type > m_svc;
};

template< typename T , T > struct TVConverterTraitBase2;

template< typename TIMP , typename R , typename TOUTPUT , typename ... TINPUTs , R( TIMP::*mf )( TOUTPUT & , const TINPUTs & ... ) >
struct TVConverterTraitBase2< R( TIMP::* )( TOUTPUT & , const TINPUTs & ... ) , mf >
{
    typedef TVSvcConverter2< TIMP , TOUTPUT , TINPUTs ... >  server_type;
    typedef IVConverter< TOUTPUT , TINPUTs ... >             node_type;
};

template< typename TIMP , typename R , typename ... TINPUTs , R( TIMP::*mf )( const TINPUTs & ... ) >
struct TVConverterTraitBase2< R( TIMP::* )( const TINPUTs & ... ) , mf >
{
    typedef TVSvcConverter2< TIMP , void , TINPUTs ... > server_type;
    typedef IVConverter< void , TINPUTs ... >            node_type;
};

//
//template< typename TIMP >
//using TVConverterTrait2 = TVConverterTraitBase2< decltype( &TIMP::Build ) , &TIMP::Build >;

template< typename ... T > struct TVConverterTrait2 ;

template< typename TIMP , typename ... TRDEPIDs >
struct TVConverterTrait2< TIMP , TRDEPIDs ... >  
     : TVConverterTraitBase2< decltype( &TIMP::Build ) , &TIMP::Build > 
{} ;
 

template< typename TRID , typename ... TRDEPIDs >
struct TVConverterTrait2< IVRLTN< TRID > , TRDEPIDs ... > : TVConverterTrait2< TVRelationNodeServer< IVRLTN< TRID > , TRDEPIDs ... > >
{} ;

#define VDDP_NOD2( name , ... )  VDDP_NODE_IMP2( name , TVConverterTrait2< __VA_ARGS__ > ) 

/////////////////////////////////////////////////////////////

// templates of simple system implementation

////////////////////////////////////////////////////////////
template< typename TR , typename TIMP  , typename ... TDEPs > class TVRSimpPipeLine ;

template< typename TID , typename TIMP  , typename ... TDEPs > 
class TVRSimpPipeLine< IVRLTN< TID > , TIMP , TDEPs ... >
    : public TVRSimpPipeLine< typename IVRLTN< TID >::HUB , TIMP , TDEPs ... > 
{
public :
    using TVRSimpPipeLine< typename IVRLTN< TID >::HUB , TIMP , TDEPs ... >::TVRSimpPipeLine ;
} ;

template< typename TID , typename TOUT , typename ... TINPUTs , typename TIMP , typename ... TDEPs >
class TVRSimpPipeLine< IVRUniqHub< TID , TOUT , TINPUTs ... > , TIMP , TDEPs ... >
{
public :
    typedef TVPipelineArg< TDEPs ... > DEPENDENCE ;
    TVRSimpPipeLine( const TDEPs & ... deps )
        : m_args( deps ... )
    {
    }
    ~TVRSimpPipeLine()
    {
    }
public :
    void Create( VO< IVConverter< TOUT , TINPUTs ... > > & usr )
    { 
        //m_sysPipe.RunService( usr , m_args  ) ;
        m_args.GetSome( [ this , &usr ]( const TDEPs & ... deps ){
            m_sysPipe.RunService( usr , deps ... ) ;
        } ) ;
    }
public :
    void Elapse()
    {
        m_sysPipe.Elapse() ;
    }

private :
    TVSvcConverter2< TIMP , TOUT , TINPUTs ... > m_sysPipe ;
    TVPipelineArg< TDEPs ... >  m_args ;
} ;

template< typename TID , typename TOUT , typename ... TINPUTs , typename TIMP >
class TVRSimpPipeLine< IVRUniqHub< TID , TOUT , TINPUTs ... > , TIMP >
{
public :
    typedef TVPipelineArg<> DEPENDENCE ;

    TVRSimpPipeLine( ) 
    {
    }
    ~TVRSimpPipeLine()
    {
    }
public :
    void Create( VO< IVConverter< TOUT , TINPUTs ... > > & usr )
    { 
        m_sysPipe.RunService( usr ) ;
        //m_sysPipe.RunService( usr , m_args  ) ;
        //m_args.GetSome( [ this , &usr ]( const TDEPs & ... deps ){
        //    m_sysPipe.RunService( usr , deps ... ) ;
        //} ) ;
    }
public :
    void Elapse()
    {
        m_sysPipe.Elapse() ;
    }

private :
    TVSvcConverter2< TIMP , TOUT , TINPUTs ... > m_sysPipe ; 
} ;

template< typename TID , typename TOUT , typename ... TINPUTs , typename TIMP , typename ... TDEPs >
class TVRSimpPipeLine< IVRUniqHub< TID , VI_AND< TOUT > , TINPUTs ... > , TIMP , TDEPs ... >
    : public TVRSimpPipeLine< IVRUniqHub< TID , TOUT , TINPUTs ... > , TIMP , TDEPs ... >
{
public :
    using TVRSimpPipeLine< IVRUniqHub< TID , TOUT , TINPUTs ... > , TIMP , TDEPs ... >::TVRSimpPipeLine ;
} ;

template< typename TIMP , typename TPRT , typename ... TDEPs > class TVRSimpManager ;

template< typename TIMP , typename TID , typename TOUT , typename ... TI  , typename ... TDEPs > 
class TVRSimpManager< TIMP , IVRUniqHub< TID , TOUT , TI ... >  , TDEPs ... > 
{
public :
    TVRSimpManager( TVRSimpPipeLine< IVRLTN< TID > , TIMP , TDEPs ... > & pl , IVSlot< TI > & ... locs  , IVInputPort< TOUT > & inp )
    { 
        pl.Create( m_node ) ;
        VLNK( inp , m_node ) ;
        VLNK( m_node , locs ... ) ;
    }

private :
    TVConverterWrapper< TOUT , TI ... > m_node ; 
} ;

template< typename TIMP , typename TID , typename TOUT , typename ... TI  , typename ... TDEPs > 
class TVRSimpManager< TIMP , IVRUniqHub< TID , VI_AND<TOUT> , TI ... >  , TDEPs ... > 
    : TVRSimpManager< TIMP , IVRUniqHub< TID , TOUT , TI ... >  , TDEPs ... >
{
public :
    using TVRSimpManager< TIMP , IVRUniqHub< TID , TOUT , TI ... >  , TDEPs ... >::TVRSimpManager ;
} ;

template< typename TIMP , typename TID , typename ... TI  , typename ... TDEPs > 
class TVRSimpManager< TIMP , IVRUniqHub< TID , void , TI ... >  , TDEPs ... > 
{
public :
    TVRSimpManager( TVRSimpPipeLine< IVRLTN< TID > , TIMP , TDEPs ... > & pl , IVSlot< TI > & ... locs )
    { 
        pl.Create( m_node ) ;
        VLNK( m_node , locs ... ) ;
    }

private :
    TVConverterWrapper< void , TI ... > m_node ; 
} ;

template< class TRROOT , class TIMP , typename ... TDEP >
struct TVRSimpSysTrait
{
    typedef IVSysDynamic< typename TRROOT > SYSTYPE ;
    typedef TVRSimpPipeLine< TRROOT , TIMP , TDEP ... >  PIPELINE ;

    template< typename TR > struct TRTrait
    {
        typedef TVRSimpManager< TIMP , typename TRROOT::HUB , TDEP ... > mngr_type ;
    };
} ;



template< class TRROOT , class TIMP >
using TVRSimpSystem = TVDynamicSystemImp2< TVRSimpSysTrait< TRROOT , TIMP > , IVSysDynamic< TRROOT > > ;

/////////////////////////////////////////////////////////////
