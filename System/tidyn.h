#pragma once
 
#include "pttrn.h"
#include <stack> 
#include <bitset>
#include "tisvr.h"

 
VPUREINTERFACE IVDirtyObject
{
    virtual void CleanAndDiffuse() = 0 ;
} ; 

template< class TID > 
class TVElement : public IVUser< IVSpace< IVElement< TID > > > 
                , public IVExtElement< TID >
{
public :
    TVElement()
    {
    } 
    TVElement( IVSpace< IVElement< TID > > & spc )
    {
        m_element.Create( spc ) ;
    } 

    ~TVElement()
    {  
        m_svcDyn.Close() ;
        m_svc.Close() ;
    }

private :
    struct DynSvc : IVDynamicNew< IVDelegate< TID > >
    {
        TVElement< TID > &my ;
        DynSvc( TVElement< TID > &m):my(m){}
        void Trace( VI<  IVTracer  >& spc )
        {
            my._trace( spc ) ;
        } 
        
        virtual void TakeMemory ( VO<  IVDelegate< TID >  >& usr )
        {
            my._takeMemory( usr ) ;
        }
    } ;
    struct Svc : IVDelegate< TID > 
    {
        TVElement< TID > &my ;
        Svc( TVElement< TID > &m):my(m){} 
        virtual void Present()
        {
            my._present() ;
        }
    } ;
   
public :
    void Update()
    {
        m_tracer.TidyTravel( []( IVTracer & t ){
            t.OnChanged() ;
        } ) ;
    } 

    void Apply( VO<  IVDynamicNew< IVDelegate< TID > >  >& usr )
    {
        m_svcDyn.RunNew( usr , *this ) ;
    }

    void Visit( IVSpace< IVElement< TID > > & spc )
    { 
        m_element.Create( spc ) ;
        Update() ;
    } 

    virtual void Present()
    {
        _present() ;
    }
    
private :
    void _trace ( VI<  IVTracer  >& spc ) 
    {
        m_tracer.Add( spc ) ;
    }   
    virtual void _takeMemory ( VO<  IVDelegate< TID >  >& usr ) 
    { 
        m_svc.RunNew( usr , *this ) ;
    }
    virtual void _present()
    {
        m_element.UseServer( []( IVElement< TID > & el ){
            el.Participate() ;
        } ) ;
    }

private :
    TVServiceProxy< IVElement< TID > > m_element ;
    TVServiceProxyArray< IVTracer    > m_tracer  ;
    TVServiceArray< IVDynamicNew< IVDelegate< TID > > , DynSvc > m_svcDyn ;
    TVServiceArray< IVDelegate< TID > , Svc > m_svc ;
} ;
 
template< class TID >
class TVElementArr : public IVUser< IVSpace< IVElement< TID > > > 
                   , public TVServer< IVDynamicNew< IVDelegateArr< TID > > >
                   , public TVServer< IVDelegateArr< TID > > 
                   , public IVExtElementArr< TID >
{
public :
    TVElementArr()
    {
    } 

    ~TVElementArr()
    { 
        TVServer< IVDelegateArr< TID > >::Close() ;
        TVServer< IVDynamicNew< IVDelegateArr< TID > > >::Close() ;
    }

public :
    template< class TSpaceUse > 
    void RunAsDynamic( TSpaceUse usr )
    { 
        TVServer< IVDynamicNew< IVDelegateArr< TID > > >::Run( usr ) ;
    }
    void RunAsDynamic( VO<  IVDynamicNew< IVDelegateArr< TID > >  >& usr )
    { 
        TVServer< IVDynamicNew< IVDelegateArr< TID > > >::Run( usr ) ;
    }
    template< class TSpaceUse > 
    void RunAsStatic( TSpaceUse usr )
    { 
        TVServer< IVDelegateArr< TID > >::Run( usr ) ;
    }
    void RunAsStatic( VO<  IVDynamicNew< IVDelegate< TID > >  >& usr )
    { 
        TVServer< IVDelegateArr< TID > >::Run( usr ) ;
    }
   
public :
    void Apply( VO<  IVDynamicNew< IVDelegateArr< TID > >  >& usr )
    {
        RunAsDynamic( usr ) ;
    }

    void Visit( IVSpace< IVElement< TID > > & spc )
    { 
        m_elementArr.Add( spc ) ;
    } 
    
    virtual void Trace ( VI<  IVTracer  >& spc ) 
    {
        m_tracer.Create( spc ) ;
    }

    virtual void TakeMemory ( VO<  IVDelegateArr< TID >  >& usr ) 
    {
        TVServer< IVDelegateArr< TID > >::Run( usr ) ;
    }

    virtual void Present()
    {
        m_elementArr.TidyTravel( []( IVElement< TID > & el ){
            el.Participate() ;
        } ) ;
    }

private :
    TVServiceProxyArray< IVElement< TID > > m_elementArr ;
    TVServiceProxy< IVTracer         > m_tracer  ;
} ;
//
//#define VD_BEGIN_DECLARE_SYSTEM( name ) struct name {  template< class IID > struct Layer 
//#define VD_SYSTEMLAYER( id , indx , ... )  template<> struct Layer < id > { static const unsigned IINDX = indx ; typedef __VA_ARGS__ element_type ; } 
//#define VD_END_DECLARE_SYSTEM( name , layercount , sysname ) static const unsigned MAXLAYERCOUNT = layercount ; } ; void Elapse(){ sysname.Elapse() ; } ; 
//#define VD_END_DECLARE_SYSTEM_CUSTOMELAPSE( name , layercount , sysname ) static const unsigned MAXLAYERCOUNT = layercount ; } ; 
 
template< class T = void , class TRef = typename TVDataTrait< T >::reference >
struct IVArrOwner : IVOwner< T , TRef >
                  , VSDynVar
{
} ;

namespace VN_ELEMSYS
{ 
    template< class T , bool bHasReset > struct TVSharedInitializer ;

    template< class T >
    struct TVSharedInitializer< T , true > 
    {
        template< class TArg >
        static void Init( T & shared , typename TVDataTrait< TArg >::reference arg )
        {
            shared.Reset( arg ) ;
        }
        template< class TArg1 , class TArg2 >
        static void Init( T & shared , typename TVDataTrait< TArg1 >::reference arg1 , typename TVDataTrait< TArg2 >::reference arg2 )
        {
            shared.Reset( arg1 , arg2 ) ;
        }
        template< class TArg1 , class TArg2 , class TArg3 >
        static void Init( T & shared , typename TVDataTrait< TArg1 >::reference arg1 
                                     , typename TVDataTrait< TArg2 >::reference arg2 
                                     , typename TVDataTrait< TArg3 >::reference arg3 )
        {
            shared.Reset( arg1 , arg2 , arg3 ) ;
        }
        template< class TArg1 , class TArg2 , class TArg3 , class TArg4 >
        static void Init( T & shared , typename TVDataTrait< TArg1 >::reference arg1 
                                     , typename TVDataTrait< TArg2 >::reference arg2 
                                     , typename TVDataTrait< TArg3 >::reference arg3 
                                     , typename TVDataTrait< TArg4 >::reference arg4 )
        {
            shared.Reset( arg1 , arg2 , arg3 , arg4 ) ;
        }
    } ;

    template< class T >
    struct TVSharedInitializer< T , false > 
    {
        template< class TArg >
        static void Init( T & shared , typename TVDataTrait< TArg >::reference arg )
        { 
        }
        template< class TArg1 , class TArg2 >
        static void Init( T & shared , typename TVDataTrait< TArg1 >::reference arg1 , typename TVDataTrait< TArg2 >::reference arg2 )
        { 
        }
        template< class TArg1 , class TArg2 , class TArg3 >
        static void Init( T & shared , typename TVDataTrait< TArg1 >::reference arg1 
                                     , typename TVDataTrait< TArg2 >::reference arg2 
                                     , typename TVDataTrait< TArg3 >::reference arg3 )
        { 
        }
        template< class TArg1 , class TArg2 , class TArg3 , class TArg4 >
        static void Init( T & shared , typename TVDataTrait< TArg1 >::reference arg1 
                                     , typename TVDataTrait< TArg2 >::reference arg2 
                                     , typename TVDataTrait< TArg3 >::reference arg3 
                                     , typename TVDataTrait< TArg4 >::reference arg4 )
        { 
        }
    } ;
    
    template< class T >
    struct  TVDeduceImport
    {
        typedef T           DataType ;
        typedef TVBind< T > ImpType  ;
    } ;

    template< class T >
    struct  TVDeduceImport< T[] >
    {
        typedef T           DataType   ;
        typedef TVBind< T > ImpType[]  ;
    } ;

    template< class T >
    struct  TVDeduceSystem
    {
        typedef typename T::ExpType ExpType ;
    } ;

    template< class T >
    struct  TVDeduceSystem< T[] >
    {
        typedef IVArrOwner< typename T::ExpType > ExpType ;
    } ;

    template< class T >
    struct  TVDeduceExtElement
    {
        typedef IVExtElement< T > type ;
    } ;

    template< class T >
    struct  TVDeduceExtElement< T[] >
    {
        typedef IVExtElementArr< T > type ;
    } ;

    template< class T >
    class TV_IsExportVirtual
    {
    private : 
        template <class C> static char _tesetvirtualexp( typename C::EXPORTDATA * ) ; 
        template <class C> static long _tesetvirtualexp(...) ;     
    public:
        static const bool value = ( sizeof( _tesetvirtualexp< T >( 0 ) ) == sizeof(char) ) ;
    };

    template< class T >
    class TV_IsExportVirtual< T[] >
    { 
    public :
        static const bool value = TV_IsExportVirtual< T >::value ;
    } ;

    template< class T , bool bDef > struct TVDeduceExportBase ;
    template< class T > struct TVDeduceExportBase< T           , true  > { typedef T ElemType           ; typedef  typename T::EXPORTDATA ExpType   ; typedef  typename T::EXPORTDATA ExpUnitType ; } ;
    template< class T > struct TVDeduceExportBase< T           , false > { typedef T ElemType           ; typedef  T                      ExpType   ; typedef  T                      ExpUnitType ; } ; 
    template< class T > struct TVDeduceExportBase< T[]         , true  > { typedef T ElemType           ; typedef  typename T::EXPORTDATA ExpType[] ; typedef  typename T::EXPORTDATA ExpUnitType ; } ;
    template< class T > struct TVDeduceExportBase< T[]         , false > { typedef T ElemType           ; typedef  T                      ExpType[] ; typedef  T                      ExpUnitType ; } ; 
    template< class T > struct TVDeduceExportBase< TVBind< T > , false > { typedef TVBind< T > ElemType ; typedef  T                      ExpType   ; typedef  T                      ExpUnitType ; } ; 
      
    template< class T > struct TVDeduceExport : TVDeduceExportBase< T , TV_IsExportVirtual< T >::value > { } ;

    template< class T >
    class TV_HasShared
    {
    private :
        typedef char one;
        typedef long two;

        template <class C> 
        static one _testshared( typename C::SHARED * ) ;

        template <class C>
        static two _testshared(...) ;    

    public:
        static const bool value = ( sizeof( _testshared< T >( 0 ) ) == sizeof(char) ) ;
    };

    template< class T , class TArg >
    struct TV_HasReset 
    {  
        template<typename U, void ( U::* )( typename TVDataTrait< TArg >::reference ) > struct SFINAE {};
        template<typename U> static char Test( SFINAE< U , &U::Reset >* );
        template<typename U> static int Test(...);
        static const bool value = ( sizeof(Test< T >( 0 )) == sizeof(char) ) ;
    };

    template< class T , class TArg1 , class TArg2 >
    struct TV_HasReset< T , TVBind< TArg1 , TArg2 > >
    {  
        template<typename U, void ( U::* )( typename TVDataTrait< TArg1 >::reference , typename TVDataTrait< TArg2 >::reference ) > struct SFINAE {};
        template<typename U> static char Test( SFINAE< U , &U::Reset >* );
        template<typename U> static int Test(...);
        static const bool value = ( sizeof(Test< T >( 0 )) == sizeof(char) ) ;
    };

    template< class T , class TArg1 , class TArg2 , class TArg3 >
    struct TV_HasReset< T , TVBind< TArg1 , TArg2 , TArg3 > >
    {  
        template<typename U, void ( U::* )( typename TVDataTrait< TArg1 >::reference , typename TVDataTrait< TArg2 >::reference  , typename TVDataTrait< TArg3 >::reference ) > struct SFINAE {};
        template<typename U> static char Test( SFINAE< U , &U::Reset >* );
        template<typename U> static int Test(...);
        static const bool value = ( sizeof(Test< T >( 0 )) == sizeof(char) ) ;
    };

    template< class T , class TArg1 , class TArg2 , class TArg3 , class TArg4 >
    struct TV_HasReset< T , TVBind< TArg1 , TArg2 , TArg3 , TArg4 > >
    {  
        template<typename U, void ( U::* )( typename TVDataTrait< TArg1 >::reference , typename TVDataTrait< TArg2 >::reference  , typename TVDataTrait< TArg3 >::reference  , typename TVDataTrait< TArg4 >::reference ) > struct SFINAE {};
        template<typename U> static char Test( SFINAE< U , &U::Reset >* );
        template<typename U> static int Test(...);
        static const bool value = ( sizeof(Test< T >( 0 )) == sizeof(char) ) ;
    };

    template< class T , bool bHasShared > struct TVDeduceSharedBase ;
    template< class T > struct TVDeduceSharedBase< T , true >
    {
        typedef typename T::SHARED type ;
    } ;
    template< class T > struct TVDeduceSharedBase< T , false >
    {
        typedef void type ;
    } ;
    template< class T > struct TVDeduceShared : TVDeduceSharedBase< T , TV_HasShared< T >::value > {} ;

    template< class T >
    class TV_HasDependent
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

    template< class TImp , class TArg , class TShared = void >
    struct TVSysImpAutoCstrObj
    {
        char m_inst[ sizeof( TImp ) ] ; 
        TImp & GetInst() { return * reinterpret_cast< TImp * >( m_inst ) ; }
        TVSysImpAutoCstrObj( TShared & s , typename TVDataTrait< TArg >::reference arg )
        {
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif
#ifdef WIN32
            static_assert(0,"") ; // 未测试
#endif
            TVSharedInitializer< TShared , TV_HasReset< TShared , TArg >::value >::template Init< TArg >( s , arg ) ;
            new( reinterpret_cast< TImp * >( m_inst ) ) TImp( s , arg ) ;
#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
        }
        ~TVSysImpAutoCstrObj()
        { 
            reinterpret_cast< TImp * >( m_inst )->~TImp() ;
        }
    } ;

    template< class TImp , class TArg , class TShared > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg > , TShared >
    { 
        char m_inst[ sizeof( TImp ) ] ; 
        TImp & GetInst() { return * reinterpret_cast< TImp * >( m_inst ) ; }
        TVSysImpAutoCstrObj( TShared & s , TVBind< TArg > & arg )
        {
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif  
            TVSharedInitializer< TShared , TV_HasReset< TShared , TArg >::value >::template Init< TArg >( s , arg.m_ref1 ) ;
            new( reinterpret_cast< TImp * >( m_inst ) ) TImp( s , arg.m_ref1 ) ;
#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
        }
        ~TVSysImpAutoCstrObj()
        { 
            reinterpret_cast< TImp * >( m_inst )->~TImp() ;
        }
    } ;

    template< class TImp , class TArg1 , class TArg2 , class TShared > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 , TArg2 > , TShared >
    { 
        char m_inst[ sizeof( TImp ) ] ; 
        TImp & GetInst() { return * reinterpret_cast< TImp * >( m_inst ) ; }
        TVSysImpAutoCstrObj( TShared & s , TVBind< TArg1 , TArg2 > & arg )
        {
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif   
            TVSharedInitializer< TShared , TV_HasReset< TShared , TVBind< TArg1 , TArg2 > >::value >::template Init< TArg1 , TArg2 >( s , arg.m_ref1 , arg.m_ref2 ) ;
            new( reinterpret_cast< TImp * >( m_inst ) ) TImp( s , arg.m_ref1 , arg.m_ref2 ) ;
#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
        }
        ~TVSysImpAutoCstrObj()
        { 
            reinterpret_cast< TImp * >( m_inst )->~TImp() ;
        }
    } ; 
    
    template< class TImp , class TArg1 , class TArg2 , class TArg3 , class TShared > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 , TArg2 , TArg3 > , TShared >
    { 
        typedef TVBind< TArg1 , TArg2 , TArg3 > bnd_arg_type ;

        char m_inst[ sizeof( TImp ) ] ; 
        TImp & GetInst() { return * reinterpret_cast< TImp * >( m_inst ) ; }
        TVSysImpAutoCstrObj( TShared & s , bnd_arg_type & arg )
        {
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif   
            TVSharedInitializer< TShared , TV_HasReset< TShared , bnd_arg_type >::value >::template Init< TArg1 , TArg2 , TArg3 >( s , arg.m_ref1 , arg.m_ref2 , arg.m_ref3 ) ;
            new( reinterpret_cast< TImp * >( m_inst ) ) TImp( s , arg.m_ref1 , arg.m_ref2 , arg.m_ref3 ) ;
#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
        }
        ~TVSysImpAutoCstrObj()
        { 
            reinterpret_cast< TImp * >( m_inst )->~TImp() ;
        }
    } ; 

    template< class TImp , class TArg1 , class TArg2 , class TArg3 , class TArg4 , class TShared > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 , TArg2 , TArg3 , TArg4 > , TShared >
    { 
        typedef TVBind< TArg1 , TArg2 , TArg3 , TArg4 > bnd_arg_type ;

        char m_inst[ sizeof( TImp ) ] ; 
        TImp & GetInst() { return * reinterpret_cast< TImp * >( m_inst ) ; }
        TVSysImpAutoCstrObj( TShared & s , bnd_arg_type & arg )
        {
#ifdef _DEBUG
#pragma push_macro("new")
#undef new
#endif   
            TVSharedInitializer< TShared , TV_HasReset< TShared , bnd_arg_type >::value >::template Init< TArg1 , TArg2 , TArg3 , TArg4 >( s , arg.m_ref1 , arg.m_ref2 , arg.m_ref3 , arg.m_ref4 ) ;
            new( reinterpret_cast< TImp * >( m_inst ) ) TImp( s , arg.m_ref1 , arg.m_ref2 , arg.m_ref3 , arg.m_ref4 ) ;
#ifdef _DEBUG
#pragma pop_macro("new") 
#endif
        }
        ~TVSysImpAutoCstrObj()
        { 
            reinterpret_cast< TImp * >( m_inst )->~TImp() ;
        }
    } ; 


    template< class TImp , class TArg > 
    struct TVSysImpAutoCstrObj< TImp , TArg , void >
    {
        TImp m_inst ; TImp & GetInst() { return m_inst ; }
        TVSysImpAutoCstrObj( typename TVDataTrait< TArg >::reference arg ) : m_inst( arg ){}
    } ;

    template< class TImp , class TArg1 > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 > , void >
    {
        TImp m_inst ; TImp & GetInst() { return m_inst ; }
        TVSysImpAutoCstrObj( TVBind< TArg1 > & arg ) : m_inst( arg.m_ref1 ){}
    } ;

    template< class TImp , class TArg1 , class TArg2 > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 , TArg2 > , void >
    {
        TImp m_inst ; TImp & GetInst() { return m_inst ; }
        TVSysImpAutoCstrObj( TVBind< TArg1 , TArg2 > & arg ) : m_inst( arg.m_ref1 , arg.m_ref2 ){}
    } ;

    template< class TImp , class TArg1 , class TArg2 , class TArg3 > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 , TArg2 , TArg3 > , void >
    {
        TImp m_inst ; TImp & GetInst() { return m_inst ; }
        TVSysImpAutoCstrObj( TVBind< TArg1 , TArg2 , TArg3 > & arg ) : m_inst( arg.m_ref1 , arg.m_ref2 , arg.m_ref3 ){}
    } ;

    template< class TImp , class TArg1 , class TArg2 , class TArg3 , class TArg4 > 
    struct TVSysImpAutoCstrObj< TImp , TVBind< TArg1 , TArg2 , TArg3 , TArg4 > , void >
    {
        TImp m_inst ; TImp & GetInst() { return m_inst ; }
        TVSysImpAutoCstrObj( TVBind< TArg1 , TArg2 , TArg3 , TArg4 > & arg ) : m_inst( arg.m_ref1 , arg.m_ref2 , arg.m_ref3 , arg.m_ref4 ){}
    } ;

    template< class TImp , class TArg > 
    struct TVSysImpAutoCstrObj< TImp , IVDynSystem< TArg > , void >
    {
        TImp m_inst ; TImp & GetInst() { return m_inst ; }
        TVSysImpAutoCstrObj( IVDynSystem< TArg > & arg ) : m_inst( arg ){}
    } ;

    template< class TInputArg , class TElement , class TExp , class TShared >
    class TVSysImpDepClnt
    {
    public :
        TVSysImpDepClnt()
        {
        }
        ~TVSysImpDepClnt()
        {
        }

    private :
        typedef  TShared SharedType ;
        typedef TExp ExpType ;
     //   typedef typename TVCondExpTyp< TElement , TV_IsExportVirtual< TElement >::value >::ExpType ExpType ;  

        struct ClntSprt
        {
        public :
            ClntSprt( SharedType & ps ) : pShared( &ps ){}
            SharedType * pShared ;
        } ;

        class ClntImp : public TVServer< IVProvider< TVBind< ExpType > > >
        {
        public :
            template< class F > // void F( VI<  IVProvider< TVBind< ExpType > >  >& )
            ClntImp( ClntSprt & sprt , TInputArg & inval , F svcOuput )
                : m_eleImp( * sprt.pShared , inval )
            {
                TVServer< IVProvider< TVBind< ExpType > > >::Run( svcOuput ) ; 
            } 
            ~ClntImp()
            {
                m_svc.Close() ;
                TVServer< IVProvider< TVBind< ExpType > > >::Close() ;
            }

        private :
            void Apply( VO<  TVBind< ExpType >  >& usr )
            {
                m_svc.RunNew( usr , m_eleImp.GetInst() ) ;
            }
        private :   
            TVSysImpAutoCstrObj< TElement , TInputArg , SharedType > m_eleImp ;
            TVServiceArray< TVBind< ExpType > > m_svc ;
        } ; 

    public :
        void Close()
        {
            m_svcClntSprt.Close() ;
        }

        template< class F > 
        void Create( VI<  TInputArg  >& spcInput , F f )
        {
            m_svcClntSprt.Run( [this,&spcInput,& f]( VI<  ClntSprt  >& spcShared ){
                _create( spcShared , spcInput , f ) ;
            } , m_sharedData ) ;
        } 
    private :
        template< class F > 
        void _create( VI<  ClntSprt  >& spcShared , VI<  TInputArg  >& spcInput , F f )
        { 
            m_clnt.Create( spcShared , spcInput , f ) ; 
        }

    public : 
        SharedType                                         m_sharedData  ;
        TVService< ClntSprt >                              m_svcClntSprt ; 
        TVClient< ClntImp , ClntSprt , TInputArg >        m_clnt        ; 
    } ;

    template< class TInputArg , class TElement , class TExp >
    class TVSysImpDepClnt< TInputArg , TElement , TExp , void >
    {
    public :
        TVSysImpDepClnt()
        {}
        ~TVSysImpDepClnt()
        {}

    private : 
        typedef TExp ExpType ;
        //typedef typename TVCondExpTyp< TElement , TV_IsExportVirtual< TElement >::value >::ExpType ExpType ;  

        class ClntImp : public TVServer< IVProvider< TVBind< ExpType > > >
        {
        public :
            template< class F > // void F( VI<  IVProvider< TVBind< ExpType > >  >& )
            ClntImp( TInputArg & inval  , F svcOuput )
                : m_eleImp( inval )
            {
                TVServer< IVProvider< TVBind< ExpType > > >::Run( svcOuput ) ; 
            } 

            ~ClntImp()
            {
                m_svc.Close() ;
                TVServer< IVProvider< TVBind< ExpType > > >::Close() ;
            }

        private :
            void Apply( VO<  TVBind< ExpType >  >& usr )
            {
                m_svc.RunNew( usr , m_eleImp.GetInst() ) ;
            }

        private :   
            TVSysImpAutoCstrObj< TElement , TInputArg > m_eleImp ;
            TVServiceArray< TVBind< ExpType > > m_svc ;
        } ; 

    public :
        void Close() 
        {
        }
        template< class F > // void F( VI<  IVProvider< TVBind< ExpType > >  >& )
        void Create( VI<  TInputArg  >& spcInput , F f )
        {
            m_clnt.Create( spcInput , f ) ;
        }

    private :
        TVClient< ClntImp , TInputArg > m_clnt ;
    } ;

    class TVSysImpTimeline
    {
    public : 
        void Elapse()
        {
            ClearDirty() ;
        }

        void RegisterDirty( VI<  IVDirtyObject  >& spc )
        {
            m_lstDirty.Add( spc ) ;
        } 
        
        void ClearDirty() 
        {
            m_lstDirty.TidyTravel( []( IVDirtyObject & d ){
                d.CleanAndDiffuse() ;
            } ) ;
            m_lstDirty.Destory() ;
        }   
    private :
        TVServiceProxyArray< IVDirtyObject > m_lstDirty ;
    } ;

    template< class TDerived , class TIDElem , class ExpType >
    class TVExportDynamic : public IVDynamicNew< TVBind< ExpType > > , public TVServer< IVTracer > , public TVServer< IVDirtyObject >
    {
    public :
        TVExportDynamic( TDerived & me , VI<  IVDynamicNew< IVDelegate< TIDElem > >  >& spcDynDelegate )
            : m_owner( me )
        {
            m_proxyDynAgent.Create( spcDynDelegate , [ this ]( IVDynamicNew< IVDelegate< TIDElem > > & dynAgent ){
                _init( dynAgent ) ;
            }) ;
        }
        ~TVExportDynamic()
        {
            TVServer< IVTracer >::Close() ;
            TVServer< IVDirtyObject >::Close() ;
            m_svcTracerOfInput.Close() ;
        }

    private :
        struct VTracer : IVTracer
        {
            TVExportDynamic & m_Exp ;
            VTracer( TVExportDynamic & e ):m_Exp(e){}
            virtual void OnChanged() 
            {
                m_Exp._notifyChng() ; 
            }
        } ;
        struct ClntCpy 
        { 
            ClntCpy( TVBind< ExpType > & val , VO<  TVBind< ExpType >  >& usr )
            {
                m_svc.Run( usr , val ) ;
            }
            ~ClntCpy()
            {
                m_svc.Close() ;
            }
            TVService< TVBind< ExpType > > m_svc ;
        } ;
    private :
        virtual void OnChanged() 
        {
            TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                m_owner.RegisterDirty( spc ) ;
            } ) ;
        } 
        virtual void Trace ( VI<  IVTracer  >& spc ) 
        {
            m_trcObserver.Create( spc ) ;
        } 
        virtual void TakeMemory ( VO<  TVBind< ExpType >  >& usr ) 
        { 
            m_proxyDynData.UseServer( [ this , &usr ]( IVDynamicNew< TVBind< ExpType > > & dyn ){
                _takeMemory( dyn , usr ) ;
            } ) ;
        } 
        virtual void CleanAndDiffuse() 
        {
            m_proxyDynAgent.UseServer( [ this ]( IVDynamicNew< IVDelegate< TIDElem > > & dynAgent ){
                _rebuildBridge( dynAgent ) ;
            }) ; 
            _notifyChng() ;
        }

    private :
        void _notifyChng()
        {
            m_trcObserver.UseServer( []( IVTracer & tr ){ tr.OnChanged() ; } ) ;
        }
        void _init( IVDynamicNew< IVDelegate< TIDElem > > & dynAgent )
        { 
            TVServer< IVTracer >::Run( [ & dynAgent ]( VI<  IVTracer  >& spc ){ dynAgent.Trace( spc ) ; } ) ;
            _rebuildBridge( dynAgent ) ;
        }
        void _rebuildBridge( IVDynamicNew< IVDelegate< TIDElem > > & dynAgent )
        {
            dynAgent.TakeMemory( VD_P2U( m_proxyAgent , [ this ]( IVDelegate< TIDElem > & agent ){ 
                _rebuildBridge( agent ) ;
            } ) ) ;
        }        
        void _rebuildBridge( IVDelegate< TIDElem > & agent )
        {
            m_owner.TranslateAgent( [&agent]{ agent.Present() ; }
            , VD_P2U( m_proxyDynData , [ this ]( IVDynamicNew< TVBind< ExpType > > & dyn ){ _rebuildBridge( dyn ) ; } ) ) ;
        }
        void _rebuildBridge( IVDynamicNew< TVBind< ExpType > > & dyn )
        {
            m_svcTracerOfInput.Run( [ & dyn ]( VI<  IVTracer  >& spc ){
                dyn.Trace( spc ) ;
            } , *this ) ;
        }
        void _takeMemory( IVDynamicNew< TVBind< ExpType > > & dyn , VO<  TVBind< ExpType >  >& usr ) 
        {
            dyn.TakeMemory( VD_L2U( [ this , &usr ]( VI<  TVBind< ExpType >  >& spc ){
                _takeMemory( spc , usr ) ;
            } ) ) ;
        }
        void _takeMemory( VI<  TVBind< ExpType >  >& spc , VO<  TVBind< ExpType >  >& usr )
        {
            m_clntValue.template Create< VO<  TVBind< ExpType >  > >( spc , usr ) ;
        }

    private :
        TDerived & m_owner ; 
        TVServiceProxy< IVDynamicNew< IVDelegate< TIDElem > > > m_proxyDynAgent ;
        TVServiceProxy< IVDelegate< TIDElem > >                 m_proxyAgent    ;  
        TVServiceProxy< IVDynamicNew< TVBind< ExpType > > >     m_proxyDynData  ;
        TVServiceProxy< IVTracer                          >     m_trcObserver   ;
        TVService< IVTracer , VTracer > m_svcTracerOfInput ;
        TVClient< ClntCpy , TVBind< ExpType > > m_clntValue ;
    } ; 
  
    template< class TDerived , class TIDElem , class ExpType >
    class TVExportDynamicArr : public IVDynamicNew< TVBind< IVArrOwner< ExpType > > >
                             , TVServer< IVTracer >
                             , TVServer< IVDirtyObject >
                             , IVUser< IVSpace< IVDynamicNew< TVBind< ExpType > > > >
                             , public IVArrOwner< ExpType >
    {
    public :
        TVExportDynamicArr( TDerived & me , VI<  IVDynamicNew< IVDelegateArr< TIDElem > >  >& spcDynDelegate )
            : m_owner( me )
        { 
            m_proxyDynAgent.Create( spcDynDelegate , [ this ]( IVDynamicNew< IVDelegateArr< TIDElem > > & dynAgent ){
                _init( dynAgent ) ;
            }) ;
        }
        ~TVExportDynamicArr()
        { 
            m_svcAcc.Close() ;
            m_svcSysVector.Close() ;
            TVServer< IVTracer >::Close() ;
            TVServer< IVDirtyObject >::Close() ; 
        }

    private :
        virtual void OnChanged() 
        {
            TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                m_owner.RegisterDirty( spc ) ;
            } ) ;
        } 
        virtual void CleanAndDiffuse() 
        {
            this->Signal() ;
            m_proxyDynAgent.UseServer( [ this ]( IVDynamicNew< IVDelegateArr< TIDElem > > & dynAgent ){
                _rebuildBridge( dynAgent ) ;
            }) ; 
            _notifyChng() ;
        }
        void Trace( VI<  IVTracer  >& spc )
        {
            m_trcObserver.Create( spc ) ;
        }
        void TakeMemory( VO<  TVBind< IVArrOwner< ExpType > >  >& usr )
        {
            m_svcSysVector.Run( usr , *this ) ;
        } 

        // IVUser< IVDynamicNew< TVBind< ExpType > > & dyn >
        void Visit( IVSpace< IVDynamicNew< TVBind< ExpType > > > & dynSpc )
        {
            m_svcAcc.RunNew( [ this , &dynSpc ]( VI<  OwnerAcc  >& spcacc ){
                TVServiceProxyArray< IVOwner< ExpType > >  & pa = m_arrDataProxy ;
                m_clntArrDynData.Add( spcacc , dynSpc , [ & pa ]( VI<  IVOwner< ExpType >  >& spc ){
                    pa.Add( spc ) ;
                } ) ;
            } , *this ) ;
        }

    private :
        virtual void Use( IVUser< ExpType > & usr )
        {
            m_arrDataProxy.TidyTravel( [&usr]( IVOwner< ExpType > & owner ){
                owner.Use( usr ) ;
            } ) ;
        }

    private :   
        struct OwnerAcc
        {
            OwnerAcc( TVExportDynamicArr & owner ):m_owner( owner ){}
            TVExportDynamicArr & m_owner ;
        } ;
        struct VDynClnt : TVServer< IVTracer >
                        , TVServer< IVDirtyObject >
                        , TVServer< IVOwner< ExpType > >
        {
        public :
            template< class F >
            VDynClnt( OwnerAcc & accOwner , IVDynamicNew< TVBind< ExpType > > & dyn , F f ) // VO<  IVOwner< TVBind< ExpType > >  >& usrOwner )
                : m_accOwner( accOwner ) , m_dyn( dyn )
            {
                TVServer< IVTracer >::Run( [ &dyn ]( VI<  IVTracer  >& spc ){
                    dyn.Trace( spc );
                } ) ;
                TVServer< IVOwner< ExpType > >::Run( f ) ;
            }
            ~VDynClnt()
            {
                TVServer< IVOwner< ExpType > >::Close() ;
                TVServer< IVTracer >::Close() ;
                TVServer< IVDirtyObject >::Close() ;
            }
        private :
            virtual void OnChanged() 
            {
                TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                    m_accOwner.m_owner._regiesterItemDirty( spc ) ;
                } ) ;
            }
            virtual void CleanAndDiffuse()
            {
                m_dyn.TakeMemory( VD_P2U( m_proxyData ) ) ;
                m_accOwner.m_owner._notifyChng() ;
            }

            virtual void Use( IVUser< ExpType > & usr ) 
            {
                m_proxyData.UseServer( [&usr]( TVBind< ExpType > & bnd ){
                    usr.Visit( bnd.m_ref1 ) ;
                } ) ;
            }

        private :
            OwnerAcc m_accOwner ;
            IVDynamicNew< TVBind< ExpType > > & m_dyn ;
            TVServiceProxy< TVBind< ExpType > > m_proxyData ;
        } ;
        void _notifyChng()
        {
            m_trcObserver.UseServer( []( IVTracer & tr ){ tr.OnChanged() ; } ) ;
        }
        void _init( IVDynamicNew< IVDelegateArr< TIDElem > > & dynAgent )
        { 
            TVServer< IVTracer >::Run( [ & dynAgent ]( VI<  IVTracer  >& spc ){ dynAgent.Trace( spc ) ; } ) ;
            _rebuildBridge( dynAgent ) ;
        }    
        void _rebuildBridge( IVDynamicNew< IVDelegateArr< TIDElem > > & dynAgent )
        {
            dynAgent.TakeMemory( VD_P2U( m_proxyAgent , [ this ]( IVDelegateArr< TIDElem > & agent ){ 
                _rebuildBridge( agent ) ;
            } ) ) ;
        }         
        void _rebuildBridge( IVDelegateArr< TIDElem > & agent )
        {
            m_clntArrDynData.Destory() ;
            m_owner.TranslateAgent( [&agent]{ agent.Present() ; } , * this ) ;  
        }   
        void _regiesterItemDirty( VI<  IVDirtyObject  >& spc )
        {
            m_owner.RegisterArrItemDirty( spc ) ;
        }

    private : 
        TDerived & m_owner ; 
        TVServiceProxy< IVDynamicNew< IVDelegateArr< TIDElem > > >                     m_proxyDynAgent   ;
        TVServiceProxy< IVDelegateArr< TIDElem > >                                     m_proxyAgent      ;  
        TVServiceArray< OwnerAcc >                                                     m_svcAcc          ;
        TVClientArray< VDynClnt , OwnerAcc , IVDynamicNew< TVBind< ExpType > > >      m_clntArrDynData  ;
        TVServiceProxyArray< IVOwner< ExpType > >                            m_arrDataProxy    ; 
        TVServiceProxy< IVTracer >                                                     m_trcObserver     ; 
        TVService< TVBind< IVArrOwner< ExpType > > >                                   m_svcSysVector    ;
    } ;
     
    template< class TDerived , class TIDElem , class ExpType >
    class TVSysImpExport
    { 
    private : 
        typedef TVBind< ExpType >       BndType    ;
        typedef IVDynamicNew< BndType > DynBndType ; 
        typedef TDerived                DerivedType ; 
        typedef TVExportDynamic< DerivedType , TIDElem , ExpType > EleExp ;

        struct TVSysPort : IVExtData2< ExpType >
        {
            TVSysImpExport & m_sys ;
            IVExtElement< TIDElem > & m_inval ;
            TVSysPort( TVSysImpExport & sys , IVExtElement< TIDElem > & inval ) : m_sys(sys),m_inval(inval){}
            void Apply( VO<  IVDynamicNew< BndType >  >& spc )
            {
                m_sys.Export( m_inval , spc ) ;
            }
        } ;

    public :
        TVSysPort Port( IVExtElement< TIDElem > & inval )
        {
            return TVSysPort( *this , inval ) ;
        }

    public :
        template< class F > 
        void Export( IVExtElement< TIDElem > & inval , F & f )
        { 
            inval.Apply( VD_L2U( [ this , &f ]( VI<  IVDynamicNew< IVDelegate< TIDElem > >  >& spcDyn ){
                _export( spcDyn , f ) ;
            } ) ) ;
        }

        void Close()
        {
            m_svcExport.Close() ;
        }
        void UpdateOutput()
        {
        }

        template< class F >
        void TranslateAgent( F f , VO<  IVDynamicNew< BndType >  >& usr )
        { 
            m_stkTranslate.push( & usr ) ;
            f() ; // agent.Present() ;
            m_stkTranslate.pop() ;
        } 

        template< class F >
        void ParticipateTranslation( F f )
        {
            if( !m_stkTranslate.empty() )
            {
                f( * m_stkTranslate.top() ) ;
            }
        } 
         
    private :
        template< class F >
        void _export( VI<  IVDynamicNew< IVDelegate< TIDElem > >  >& spcDyn , F & f )
        {
            m_svcExport.RunNew( f , * static_cast< TDerived * >( this ) , spcDyn ) ; // []( VI<  IVDynamicNew< ExpType >  >& spc ){} ) ;
        }

    private :
        TVServiceArray< IVDynamicNew< BndType > , EleExp > m_svcExport   ;
        std::stack< IVUser< IVSpace< DynBndType > > * > m_stkTranslate ;
    } ; 
    
    template< class TDerived , class TIDElem , class ExpType >
    class TVSysImpExport< TDerived , TIDElem , ExpType[] > : public TVSysImpExport< TDerived , TIDElem , ExpType >
    {  
    public :
        typedef TVBind< IVArrOwner< ExpType > >                   BndArrType ;
        typedef TVExportDynamicArr< TDerived , TIDElem , ExpType > EleArrExp ;   

        void Close()
        {
            m_svcExport.Close() ;
            TVSysImpExport< TDerived , TIDElem , ExpType >::Close() ;
        }       
        
        void UpdateOutput()
        {
            TVSysImpExport< TDerived , TIDElem , ExpType >::UpdateOutput() ;
            m_lstItemDirty.TidyTravel( []( IVDirtyObject & dty ){
                dty.CleanAndDiffuse() ;
            } ) ;
        }
               
        void RegisterArrItemDirty( VI<  IVDirtyObject  >& spc )
        {
            m_lstItemDirty.Add( spc ) ;
        } 

        template< class F > 
        void Export( IVExtElementArr< TIDElem > & inval , F & f )
        {  
            inval.Apply( VD_L2U( [ this , &f ]( VI<  IVDynamicNew< IVDelegateArr< TIDElem > >  >& spcDyn ){
                m_svcExport.RunNew( f , * static_cast< TDerived * >( this ) , spcDyn ) ;  
            } ) ) ;
        } 

        template< class F > 
        void Export( IVExtElement< TIDElem > & inval , F & f )
        {
            TVSysImpExport< TDerived , TIDElem , ExpType >::Export( inval , f ) ;
        }

    private :
        TVSysImpExport< TDerived , TIDElem , ExpType >           m_expSingle   ;
        TVServiceArray< IVDynamicNew< BndArrType > , EleArrExp > m_svcExport   ;
        TVServiceProxyArray< IVDirtyObject > m_lstItemDirty ;
    } ;  

    template< class TOwner , class TInput , class TIDElem , class TElement , class TExp >
    class TVElementInstImp : public IVElement< TIDElem > , TVServer< IVTracer > , TVServer< IVDirtyObject >
    {
    public :
        typedef TExp ExpType ;
        typedef TVSysImpDepClnt< TVBind< TInput > , TElement , ExpType , typename TVDeduceShared< TElement >::type >   ClntType ;
        typedef TVElementInstImp< TOwner , TInput , TIDElem , TElement , TExp > MyType ;

    public :
        TVElementInstImp( TOwner & me , VI<  IVDynamicNew< TVBind< TInput > >  >& spcDynSrc )
            : m_owner( me ) 
        {   
            m_proxyDynSrc.Create( spcDynSrc , [ this ]( IVDynamicNew< TVBind< TInput > > & dynSrc ){
                _init( dynSrc ) ;
            } ) ;
        } 
        ~TVElementInstImp()
        { 
            m_clntImp.Close() ;
            TVServer< IVTracer >::Close() ;
            TVServer< IVDirtyObject >::Close() ;
            m_svcPartial.Close() ;
        }

    private :
        struct SvcDynBind : IVDynamicNew< TVBind< ExpType > >
        {
            SvcDynBind( MyType & ei ) : m_owner( ei ){}
            void Trace( VI<  IVTracer  >& spc )
            {
                m_owner._traceData( spc ) ;
            }
            void TakeMemory( VO<  TVBind< ExpType >  >& usr )
            {
                m_owner._takeMemory( usr ) ;
            }
            MyType & m_owner ;
        } ;
    private :
        virtual void Participate() 
        {
            m_owner.ParticipateTranslation( [this]( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr ){
                _participateTo( usr ) ;
            } ) ;
        } 
        virtual void OnChanged()
        {
            TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                m_owner.RegisterDirty( spc ) ;
            } ) ;
        }
        virtual void CleanAndDiffuse() 
        {
            m_proxyDynSrc.UseServer(  [ this ]( IVDynamicNew< TVBind< TInput > > & dynSrc ){
                _reset( dynSrc ) ; 
            } ) ;

            m_lstDataTracer.TidyTravel( []( IVTracer & t ){ t.OnChanged() ; } ) ;
        } 

    private :
        void _init( IVDynamicNew< TVBind< TInput > > & dynSrc )
        { 
            TVServer< IVTracer >::Run( [ & dynSrc ]( VI<  IVTracer  >& spc ){
                dynSrc.Trace( spc ) ;
            } ) ;
            _reset( dynSrc ) ;
        }
        void _reset( IVDynamicNew< TVBind< TInput > > & dynSrc )
        {
            dynSrc.TakeMemory( VD_L2U( [this]( VI<  TVBind< TInput >  >& spc ){
                _reset( spc ) ;
            } ) ) ;
        }
        void _reset( VI<  TVBind< TInput >  >& spc )
        {
            m_clntImp.Create( spc , VD_MP2L( m_provMemCreator ) ) ;
        } 
        void _participateTo( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr )
        {
            m_svcPartial.RunNew( usr , *this ) ;
        } 
        void _traceData( VI<  IVTracer  >& spc ) 
        {
            m_lstDataTracer.Add( spc ) ;
        }
        void _takeMemory( VO<  TVBind< ExpType >  >& usr )
        {
            m_provMemCreator.UseServer( [ &usr ]( IVProvider< TVBind< ExpType > > & prov ){
                prov.Apply( usr ) ;
            } ) ;
        }

    private :
        TOwner & m_owner ;
        TVServiceProxy< IVDynamicNew< TVBind< TInput > > >   m_proxyDynSrc    ; 
        ClntType                                             m_clntImp        ;
        TVServiceProxy< IVProvider< TVBind< ExpType > > >    m_provMemCreator ;
        TVServiceArray< IVDynamicNew< TVBind< ExpType > > , SvcDynBind >  m_svcPartial  ;
        TVServiceProxyArray< IVTracer > m_lstDataTracer ;
    } ;

    template< class TDerived , class TIDElem , class TInput , class TElement , class TExp > 
    class TVSysImpElement
    { 
    public :
        typedef TDerived MyType  ;
        typedef TExp     ExpType ;
        
        template< class TChk >
        struct CheckInput
        {
            static const bool value = std::is_same< TInput , typename TChk::ExpType >::value ;
        } ;   

        template< class T1 , class T2 , class T3 , class T4 , class T5 , class T6 >
        struct CheckInput< TVBind< T1 , T2 , T3 , T4 , T5 , T6 > >
        {
            static const bool value = false ;
        } ;

        typedef TVElementInstImp< MyType , TInput , TIDElem , TElement , TExp > EleImp ;

        TVSysImpElement()
        {
        }
        ~TVSysImpElement()
        {
        }
    public :
        void Close()
        {
            m_svcElements.Close() ;
        }
        void Import( IVExtData2< TInput > & inval , VO<  IVElement< TIDElem >  >& outval )
        {
            inval.Apply( VD_L2U( [ this , & outval ]( VI<  IVDynamicNew< TVBind< TInput > >  >& spcInput ){
                _create( spcInput , outval ) ;
            }) ) ;
        }
        template< class TSysImp >
        void Create( TSysImp & sysInput , IVExtElement< typename TSysImp::IDElement > & inval , VO<  IVElement< TIDElem >  >& outval )
        {
            VTRACE( "ss" ) ;
            static_assert( std::is_same< TInput , typename TSysImp::ExpType >::value , "输入系统无法提供对应类型的数据！" ) ; 
            sysInput.Export( inval , [ this , &outval ]( VI<  IVDynamicNew< TVBind< TInput > >  >& spcInput ){
                _create( spcInput , outval ) ;
            } ) ;
        }
        void PrepareInput() 
        {
        }
    private :  
        //class EleImp : public IVElement< TIDElem > , TVServer< IVTracer > , TVServer< IVDirtyObject >
        //{ 
        //} ;
        void _create( VI<  IVDynamicNew< TVBind< TInput > >  >& spcInput , VO<  IVElement< TIDElem >  >& outval )
        {
            m_svcElements.RunNew( outval , * static_cast< TDerived *>( this ) , spcInput ) ;
        } 
        TVServiceArray< IVElement< TIDElem > , EleImp > m_svcElements ;
    } ;

    template< class TDerived , class TIDElem , class TInput1 , class TInput2 , class TElement , class TExp > 
    class TVSysImpElement< TDerived , TIDElem , TVBind< TInput1 , TInput2 > , TElement , TExp >
    { 
    public :
        typedef TDerived MyType  ;
        typedef TExp     ExpType ;   

        template< class TChk >
        struct CheckInput
        {
            static const bool value = false ;
        } ;
        
        template< class TSys1 , class TSys2 >
        struct CheckInput< TVBind< TSys1 , TSys2 > >
        {
            static const bool value = std::is_same< TInput1 , typename TSys1::ExpType >::value && std::is_same< TInput2 , typename TSys2::ExpType >::value ;
        } ;

    private :
        //typedef TVSysImpDepClnt< TVBind< TInput1 , TInput2 > , TElement , TV_HasShared< TElement >::value > ClntType ;
        typedef TVSysImpDepClnt< TVBind< TInput1 , TInput2 > , TElement , TExp , typename TVDeduceShared< TElement >::type > ClntType ;

        class EleClntAdp 
        { 
        public:
            EleClntAdp( TVBind< TInput1 > & bi1 , TVBind< TInput2 > & bi2 , ClntType & clnt , TVServiceProxy< IVProvider< TVBind< ExpType > > > & proxySvc )
            {
                m_svcAdp.Run( [ this , &proxySvc , & clnt ]( VI<  TVBind< TInput1 , TInput2 >  >& spcBind ){
                        _init( spcBind , proxySvc , clnt ) ; 
                } , bi1.m_ref1 , bi2.m_ref1 ) ;
            }
            ~EleClntAdp()
            {
                m_svcAdp.Close() ;
            }
        private :
            void _init( VI<  TVBind< TInput1 , TInput2 >  >& spcBind , TVServiceProxy< IVProvider< TVBind< ExpType > > > & proxySvc , ClntType & clnt )
            { 
                clnt.Create( spcBind , [ & proxySvc ]( VI<  IVProvider< TVBind< ExpType > >  >& spc ){ proxySvc.Create( spc ); } ) ;
            }

        private :
            TVService< TVBind< TInput1 , TInput2 > > m_svcAdp ;
        } ;

        class EleImp : public IVElement< TIDElem > , TVServer< IVDirtyObject >
        { 
        private :
            struct SvcTracer1 : IVTracer 
            {
                EleImp & m_owner ;
                SvcTracer1( EleImp & ei ) : m_owner( ei ){}     
                virtual void OnChanged() { m_owner._onChanged() ; }
            };
            struct SvcTracer2 : IVTracer 
            {
                EleImp & m_owner ;
                SvcTracer2( EleImp & ei ) : m_owner( ei ){}     
                virtual void OnChanged() { m_owner._onChanged() ; }
            };
            struct SvcDynBind : IVDynamicNew< TVBind< ExpType > >
            {
                SvcDynBind( EleImp & ei ) : m_owner( ei ){}
                void Trace( VI<  IVTracer  >& spc )
                {
                    m_owner._traceData( spc ) ;
                }
                void TakeMemory( VO<  TVBind< ExpType >  >& usr )
                {
                    m_owner._takeMemory( usr ) ;
                }
                EleImp & m_owner ;
            } ;

        private :
            MyType & m_owner ;
            TVServiceProxy< IVDynamicNew< TVBind< TInput1 > > >   m_proxyDynSrc1 ; 
            TVServiceProxy< IVDynamicNew< TVBind< TInput2 > > >   m_proxyDynSrc2 ; 
            TVService< IVTracer , SvcTracer1 >                    m_svcTracer1   ; 
            TVService< IVTracer , SvcTracer2 >                    m_svcTracer2   ; 

            TVClient< EleClntAdp , TVBind< TInput1 > , TVBind< TInput2 > >      m_clntAdp        ;
            TVServiceProxy< IVProvider< TVBind< ExpType > > >                    m_provMemCreator ;
            TVServiceArray< IVDynamicNew< TVBind< ExpType > > , SvcDynBind >     m_svcPartial     ;
            TVServiceProxyArray< IVTracer >                                      m_lstDataTracer  ;
            ClntType                                                             m_clntImp        ;

        public :
            EleImp( MyType & me , VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcDynSrc1 , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcDynSrc2 )
                : m_owner( me )
            {   
                m_proxyDynSrc1.Create( spcDynSrc1 , [ this , &spcDynSrc2 ]( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 ){
                    _init( dynSrc1 , spcDynSrc2 ) ;
                } ) ;
            } 
            ~EleImp()
            {  
                m_svcTracer1.Close() ;
                m_svcTracer2.Close() ;
                TVServer< IVDirtyObject >::Close() ;
                m_svcPartial.Close() ;
            }

        private :
            virtual void Participate() 
            {
                m_owner.ParticipateTranslation( [this]( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr ){
                    _participateTo( usr ) ;
                } ) ;
            } 
            virtual void CleanAndDiffuse() 
            {
                m_proxyDynSrc1.UseServer(  [ this ]( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 ){
                    _reset( dynSrc1 ) ; 
                } ) ;

                m_lstDataTracer.TidyTravel( []( IVTracer & t ){ t.OnChanged() ; } ) ;
            } 

        private :
            virtual void _onChanged()
            {
                TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                    m_owner.RegisterDirty( spc ) ;
                } ) ;
            }
        private :
            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcDynSrc2 )
            {
                m_proxyDynSrc2.Create( spcDynSrc2 , [ this , &dynSrc1 ]( IVDynamicNew< TVBind< TInput2 > > & dynSrc2 ){
                    _init( dynSrc1 , dynSrc2 ) ;
                } ) ;
            }
            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 )
            { 
                m_svcTracer1.Run( [ &dynSrc1 ]( VI<  IVTracer  >& spc ) { dynSrc1.Trace( spc ); } , *this );
                m_svcTracer2.Run( [ &dynSrc2 ]( VI<  IVTracer  >& spc ) { dynSrc2.Trace( spc ); } , *this );

                _reset( dynSrc1 , dynSrc2 ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 )
            {
                m_proxyDynSrc2.UseServer(  [ this , & dynSrc1 ]( IVDynamicNew< TVBind< TInput2 > > & dynSrc2 ){
                    _reset( dynSrc1 , dynSrc2 ) ; 
                } ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 )
            {
                dynSrc1.TakeMemory( VD_L2U( [ this , &dynSrc2 ]( VI<  TVBind< TInput1 >  >& spc1 ){
                    _reset( spc1 , dynSrc2 ) ;
                } ) ) ;
            }
            void _reset( VI<  TVBind< TInput1 >  >& spc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 )
            {
                dynSrc2.TakeMemory( VD_L2U( [ this , &spc1 ]( VI<  TVBind< TInput2 >  >& spc2 ){
                    _reset( spc1 , spc2 ) ;
                } ) ) ;
            }
            void _reset( VI<  TVBind< TInput1 >  >& spc1 , VI<  TVBind< TInput2 >  >& spc2 )
            {
                m_clntAdp.template Create< ClntType & , TVServiceProxy< IVProvider< TVBind< ExpType > > > & >( spc1 , spc2 , m_clntImp , m_provMemCreator ) ;
            } 
            void _participateTo( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr )
            {
                m_svcPartial.RunNew( usr , *this ) ;
            } 
            void _traceData( VI<  IVTracer  >& spc ) 
            {
                m_lstDataTracer.Add( spc ) ;
            }
            void _takeMemory( VO<  TVBind< ExpType >  >& usr )
            {
                m_provMemCreator.UseServer( [ &usr ]( IVProvider< TVBind< ExpType > > & prov ){
                    prov.Apply( usr ) ;
                } ) ;
            }
        } ;

    public :
        void Close()
        {  
            m_svcElements.Close() ;
        } 
        void PrepareInput() 
        {
        }

        template< class TSysImp1 , class TSysImp2 , class TExtEle1 , class TExtEle2 >
        void Create( TVBind< TSysImp1 , TSysImp2 > & sysInput
                   , TVBind< TExtEle1 , TExtEle2 > & inval 
                   , VO<  IVElement< TIDElem >  >& outval )
        {
            TSysImp1 & sysInput1 = sysInput.m_ref1 ;
            TSysImp2 & sysInput2 = sysInput.m_ref2 ;
            TExtEle1 & inval1 = inval.m_ref1 ;
            TExtEle2 & inval2 = inval.m_ref2 ;

            static_assert( std::is_same<  TInput1 , typename TSysImp1::ExpType >::value 
                           || std::is_same<  TInput1 , IVArrOwner< typename TSysImp1::ExpType > >::value , "输入系统无法提供对应类型的数据！" ) ;  
            static_assert( std::is_same<  TInput2 , typename TSysImp2::ExpType >::value 
                           || std::is_same< TInput2 , IVArrOwner< typename TSysImp2::ExpType  > >::value , "输入系统无法提供对应类型的数据！" ) ;  

            sysInput1.Export( inval1 , [ this , & sysInput2 , & inval2 , &outval ]( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 ){
                _create( spcInput1 , sysInput2 , inval2 , outval ) ;
            } ) ;
        }

    private :
        template< class TSysImp2 , class TExtEle2 >
        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 , TSysImp2 & sysInput2  , TExtEle2 & inval2 , VO<  IVElement< TIDElem >  >& outval )
        {
            sysInput2.Export( inval2 , [ this , & spcInput1 , &outval ]( VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 ){
                _create( spcInput1 , spcInput2 , outval ) ;
            } ) ;
        } 

        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 , VO<  IVElement< TIDElem >  >& outval )
        {
            m_svcElements.RunNew( outval , * static_cast< MyType * >( this ) , spcInput1 , spcInput2 ) ;
        }

    private :
        TVServiceArray< IVElement< TIDElem > , EleImp > m_svcElements ; 
    } ;
    template< class TDerived , class TIDElem , class TInput1 , class TInput2 , class TInput3  , class TElement , class TExp > 
    class TVSysImpElement< TDerived , TIDElem , TVBind< TInput1 , TInput2 , TInput3  > , TElement , TExp >
    { 
    public :
        typedef TDerived MyType  ;
        typedef TExp     ExpType ;   
        typedef TVBind< TInput1 , TInput2 , TInput3 >  bndInput ;

        template< class TChk >
        struct CheckInput
        {
            static const bool value = false ;
        } ;
        
        template< class TSys1 , class TSys2 , class TSys3 >
        struct CheckInput< TVBind< TSys1 , TSys2 , TSys3 > >
        {
            static const bool value = std::is_same< TInput1 , typename TSys1::ExpType >::value
                                   && std::is_same< TInput2 , typename TSys2::ExpType >::value  
                                   && std::is_same< TInput3 , typename TSys3::ExpType >::value ;
        } ;

    private : 
        typedef TVSysImpDepClnt< bndInput , TElement , TExp , typename TVDeduceShared< TElement >::type > ClntType ;

        class EleClntAdp 
        { 
        public:
            EleClntAdp( TVBind< TInput1 > & bi1 , TVBind< TInput2 > & bi2 , TVBind< TInput3 > & bi3 
                      , ClntType & clnt , TVServiceProxy< IVProvider< TVBind< ExpType > > > & proxySvc )
            {
                m_svcAdp.Run( [ this , &proxySvc , & clnt ]( VI<  bndInput  >& spcBind ){
                        _init( spcBind , proxySvc , clnt ) ; 
                } , bi1.m_ref1 , bi2.m_ref1 , bi3.m_ref1 ) ;

            }
            ~EleClntAdp()
            {
                m_svcAdp.Close() ;
            }
        private :
            void _init( VI<  bndInput  >& spcBind , TVServiceProxy< IVProvider< TVBind< ExpType > > > & proxySvc , ClntType & clnt )
            { 
                clnt.Create( spcBind , [ & proxySvc ]( VI<  IVProvider< TVBind< ExpType > >  >& spc ){ proxySvc.Create( spc ); } ) ;
            }

        private :
            TVService< bndInput > m_svcAdp ;
        } ;

        class EleImp : public IVElement< TIDElem > , TVServer< IVDirtyObject >
        { 
        private :
            struct SvcTracer : IVTracer 
            {
                EleImp & m_owner ;
                SvcTracer( EleImp & ei ) : m_owner( ei ){}     
                virtual void OnChanged() { m_owner._onChanged() ; }
            }; 
            struct SvcDynBind : IVDynamicNew< TVBind< ExpType > >
            {
                SvcDynBind( EleImp & ei ) : m_owner( ei ){}
                void Trace( VI<  IVTracer  >& spc )
                {
                    m_owner._traceData( spc ) ;
                }
                void TakeMemory( VO<  TVBind< ExpType >  >& usr )
                {
                    m_owner._takeMemory( usr ) ;
                }
                EleImp & m_owner ;
            } ;

        private :
            MyType & m_owner ;
            TVServiceProxy< IVDynamicNew< TVBind< TInput1 > > >   m_proxyDynSrc1 ; 
            TVServiceProxy< IVDynamicNew< TVBind< TInput2 > > >   m_proxyDynSrc2 ; 
            TVServiceProxy< IVDynamicNew< TVBind< TInput3 > > >   m_proxyDynSrc3 ; 
            TVService< IVTracer , SvcTracer >                     m_svcTracer[3] ; 

            TVClient< EleClntAdp , TVBind< TInput1 > , TVBind< TInput2 > , TVBind< TInput3 > > m_clntAdp ;
            TVServiceProxy< IVProvider< TVBind< ExpType > > >                    m_provMemCreator ;
            TVServiceArray< IVDynamicNew< TVBind< ExpType > > , SvcDynBind >     m_svcPartial     ;
            TVServiceProxyArray< IVTracer >                                      m_lstDataTracer  ;
            ClntType                                                             m_clntImp        ;

        public :
            EleImp( MyType & me , VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcDynSrc1 
                                , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcDynSrc2 
                                , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcDynSrc3 )
                : m_owner( me )
            {   
                m_proxyDynSrc1.Create( spcDynSrc1 , [ this , &spcDynSrc2  , &spcDynSrc3 ]( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 ){
                    _init( dynSrc1 , spcDynSrc2 , spcDynSrc3 ) ;
                } ) ;
            } 
            ~EleImp()
            {  
                m_svcTracer[1].Close() ;
                m_svcTracer[2].Close() ;
                m_svcTracer[3].Close() ; 
                TVServer< IVDirtyObject >::Close() ;
                m_svcPartial.Close() ;
            }

        private :
            virtual void Participate() 
            {
                m_owner.ParticipateTranslation( [this]( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr ){
                    _participateTo( usr ) ;
                } ) ;
            } 
            virtual void CleanAndDiffuse() 
            {
                m_proxyDynSrc1.UseServer(  [ this ]( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 ){
                    _reset( dynSrc1 ) ; 
                } ) ;

                m_lstDataTracer.TidyTravel( []( IVTracer & t ){ t.OnChanged() ; } ) ;
            } 

        private :
            virtual void _onChanged()
            {
                TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                    m_owner.RegisterDirty( spc ) ;
                } ) ;
            }
        private :
            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcDynSrc2  , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcDynSrc3 )
            {
                m_proxyDynSrc2.Create( spcDynSrc2 , [ this , &dynSrc1  , &spcDynSrc3 ]( IVDynamicNew< TVBind< TInput2 > > & dynSrc2 ){
                    _init( dynSrc1 , dynSrc2 , spcDynSrc3 ) ;
                } ) ;
            }

            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2   , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcDynSrc3 )
            {
                m_proxyDynSrc3.Create( spcDynSrc3 , [ this , &dynSrc1 , &dynSrc2 ]( IVDynamicNew< TVBind< TInput3 > > & dynSrc3 ){
                    _init( dynSrc1 , dynSrc2 , dynSrc3 ) ;
                } ) ;
            } 

            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 )
            { 
                m_svcTracer[ 1 ].template Run< EleImp & >( *this , [ &dynSrc1 ]( VI<  IVTracer  >& spc ) { dynSrc1.Trace( spc ); } );
                m_svcTracer[ 2 ].template Run< EleImp & >( *this , [ &dynSrc2 ]( VI<  IVTracer  >& spc ) { dynSrc2.Trace( spc ); } );
                m_svcTracer[ 3 ].template Run< EleImp & >( *this , [ &dynSrc3 ]( VI<  IVTracer  >& spc ) { dynSrc3.Trace( spc ); } );

                _reset( dynSrc1 , dynSrc2 , dynSrc3 ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 )
            {
                m_proxyDynSrc2.UseServer(  [ this , & dynSrc1 ]( IVDynamicNew< TVBind< TInput2 > > & dynSrc2 ){
                    _reset( dynSrc1 , dynSrc2 ) ; 
                } ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 )
            {
                m_proxyDynSrc3.UseServer(  [ this , & dynSrc1 , & dynSrc2 ]( IVDynamicNew< TVBind< TInput3 > > & dynSrc3 ){
                    _reset( dynSrc1 , dynSrc2 , dynSrc3 ) ; 
                } ) ;
            } 

            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 )
            {
                dynSrc1.TakeMemory( VD_L2U( [ this , &dynSrc2 , &dynSrc3 ]( VI<  TVBind< TInput1 >  >& spc1 ){
                    _reset( spc1 , dynSrc2 , dynSrc3 ) ;
                } ) ) ;
            }

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2  , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 )
            {
                dynSrc2.TakeMemory( VD_L2U( [ this , &spc1 , &dynSrc3 ]( VI<  TVBind< TInput2 >  >& spc2 ){
                    _reset( spc1 , spc2 , dynSrc3 ) ;
                } ) ) ;
            }

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , VI<  TVBind< TInput2 >  >& spc2  , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 )
            {
                dynSrc3.TakeMemory( VD_L2U( [ this , &spc1 , &spc2 ]( VI<  TVBind< TInput3 >  >& spc3 ){
                    _reset( spc1 , spc2 , spc3 ) ;
                } ) ) ;
            } 

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , VI<  TVBind< TInput2 >  >& spc2 , VI<  TVBind< TInput3 >  >& spc3 )
            {
                m_clntAdp.template Create< ClntType & , TVServiceProxy< IVProvider< TVBind< ExpType > > > & >( spc1 , spc2 , spc3 , m_clntImp , m_provMemCreator ) ;
            } 
            void _participateTo( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr )
            {
                m_svcPartial.template RunNew< EleImp & >( *this , usr ) ;
            } 
            void _traceData( VI<  IVTracer  >& spc ) 
            {
                m_lstDataTracer.Add( spc ) ;
            }
            void _takeMemory( VO<  TVBind< ExpType >  >& usr )
            {
                m_provMemCreator.UseServer( [ &usr ]( IVProvider< TVBind< ExpType > > & prov ){
                    prov.Apply( usr ) ;
                } ) ;
            }
        } ;

    public :
        void Close()
        {  
            m_svcElements.Close() ;
        } 
        void PrepareInput() 
        {
        }

        template< class TSysImp1 , class TSysImp2 , class TSysImp3 , class TExtEle1 , class TExtEle2  , class TExtEle3 >
        void Create( TVBind< TSysImp1 , TSysImp2 , TSysImp3 > & sysInput
                   , TVBind< TExtEle1 , TExtEle2 , TExtEle3 > & inval 
                   , VO<  IVElement< TIDElem >  >& outval )
        {
            TSysImp1 & sysInput1 = sysInput.m_ref1 ;
            TSysImp2 & sysInput2 = sysInput.m_ref2 ;
            TSysImp3 & sysInput3 = sysInput.m_ref3 ; 
            TExtEle1 & inval1 = inval.m_ref1 ;
            TExtEle2 & inval2 = inval.m_ref2 ;
            TExtEle3 & inval3 = inval.m_ref3 ; 

            static_assert( std::is_same<  TInput1 , typename TSysImp1::ExpType >::value 
                           || std::is_same<  TInput1 , IVArrOwner< typename TSysImp1::ExpType > >::value , "输入系统无法提供对应类型的数据！" ) ;  
            static_assert( std::is_same<  TInput2 , typename TSysImp2::ExpType >::value 
                           || std::is_same< TInput2 , IVArrOwner< typename TSysImp2::ExpType  > >::value , "输入系统无法提供对应类型的数据！" ) ;  
            static_assert( std::is_same<  TInput3 , typename TSysImp3::ExpType >::value 
                           || std::is_same< TInput3 , IVArrOwner< typename TSysImp3::ExpType  > >::value , "输入系统无法提供对应类型的数据！" ) ;  
 
            sysInput1.Export( inval1 , [ this , & sysInput2 , & inval2 , & sysInput3 , & inval3 , &outval ]( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 ){
                _create( spcInput1 , sysInput2 , inval2 , sysInput3 , inval3 , outval ) ;
            } ) ;
        }

    private :
        template< class TSysImp2 , class TExtEle2 , class TSysImp3 , class TExtEle3 >
        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , TSysImp2 & sysInput2  , TExtEle2 & inval2 
                    , TSysImp3 & sysInput3  , TExtEle3 & inval3  
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            sysInput2.Export( inval2 , [ this , & spcInput1 , & sysInput3 , & inval3 , &outval ]( VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 ){
                _create( spcInput1 , spcInput2 , sysInput3 , inval3 , outval ) ;
            } ) ;
        } 

        template< class TSysImp3 , class TExtEle3 >
        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 
                    , TSysImp3 & sysInput3  , TExtEle3 & inval3  
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            sysInput3.Export( inval3 , [ this , & spcInput1 , & spcInput2 , &outval ]( VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcInput3 ){
                _create( spcInput1 , spcInput2 , spcInput3 , outval ) ;
            } ) ;
        }  

        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2  
                    , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcInput3  
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            m_svcElements.RunNew( outval , * static_cast< MyType * >( this ) , spcInput1 , spcInput2 , spcInput3 ) ;
        }

    private :
        TVServiceArray< IVElement< TIDElem > , EleImp > m_svcElements ; 
    } ;
    template< class TDerived , class TIDElem , class TInput1 , class TInput2 , class TInput3 , class TInput4 , class TElement , class TExp > 
    class TVSysImpElement< TDerived , TIDElem , TVBind< TInput1 , TInput2 , TInput3 , TInput4 > , TElement , TExp >
    { 
    public :
        typedef TDerived MyType  ;
        typedef TExp     ExpType ;   
        typedef TVBind< TInput1 , TInput2 , TInput3 , TInput4 >  bndInput ;

        template< class TChk >
        struct CheckInput
        {
            static const bool value = false ;
        } ;
        
        template< class TSys1 , class TSys2 , class TSys3 , class TSys4 >
        struct CheckInput< TVBind< TSys1 , TSys2 , TSys3 , TSys4 > >
        {
            static const bool value = std::is_same< TInput1 , typename TSys1::ExpType >::value
                                   && std::is_same< TInput2 , typename TSys2::ExpType >::value  
                                   && std::is_same< TInput3 , typename TSys3::ExpType >::value  
                                   && std::is_same< TInput4 , typename TSys4::ExpType >::value ;
        } ;

    private : 
        typedef TVSysImpDepClnt< bndInput , TElement , TExp , typename TVDeduceShared< TElement >::type > ClntType ;

        class EleClntAdp 
        { 
        public:
            EleClntAdp( TVBind< TInput1 > & bi1 , TVBind< TInput2 > & bi2 , TVBind< TInput3 > & bi3 , TVBind< TInput4 > & bi4 
                      , ClntType & clnt , TVServiceProxy< IVProvider< TVBind< ExpType > > > & proxySvc )
            {
                m_svcAdp.Run( [ this , &proxySvc , & clnt ]( VI<  bndInput  >& spcBind ){
                        _init( spcBind , proxySvc , clnt ) ; 
                } , bi1.m_ref1 , bi2.m_ref1 , bi3.m_ref1 , bi4.m_ref1 ) ;

            }
            ~EleClntAdp()
            {
                m_svcAdp.Close() ;
            }
        private :
            void _init( VI<  bndInput  >& spcBind , TVServiceProxy< IVProvider< TVBind< ExpType > > > & proxySvc , ClntType & clnt )
            { 
                clnt.Create( spcBind , [ & proxySvc ]( VI<  IVProvider< TVBind< ExpType > >  >& spc ){ proxySvc.Create( spc ); } ) ;
            }

        private :
            TVService< bndInput > m_svcAdp ;
        } ;

        class EleImp : public IVElement< TIDElem > , TVServer< IVDirtyObject >
        { 
        private :
            struct SvcTracer : IVTracer 
            {
                EleImp & m_owner ;
                SvcTracer( EleImp & ei ) : m_owner( ei ){}     
                virtual void OnChanged() { m_owner._onChanged() ; }
            }; 
            struct SvcDynBind : IVDynamicNew< TVBind< ExpType > >
            {
                SvcDynBind( EleImp & ei ) : m_owner( ei ){}
                void Trace( VI<  IVTracer  >& spc )
                {
                    m_owner._traceData( spc ) ;
                }
                void TakeMemory( VO<  TVBind< ExpType >  >& usr )
                {
                    m_owner._takeMemory( usr ) ;
                }
                EleImp & m_owner ;
            } ;

        private :
            MyType & m_owner ;
            TVServiceProxy< IVDynamicNew< TVBind< TInput1 > > >   m_proxyDynSrc1 ; 
            TVServiceProxy< IVDynamicNew< TVBind< TInput2 > > >   m_proxyDynSrc2 ; 
            TVServiceProxy< IVDynamicNew< TVBind< TInput3 > > >   m_proxyDynSrc3 ; 
            TVServiceProxy< IVDynamicNew< TVBind< TInput4 > > >   m_proxyDynSrc4 ; 
            TVService< IVTracer , SvcTracer >                     m_svcTracer[4] ; 

            TVClient< EleClntAdp , TVBind< TInput1 > , TVBind< TInput2 > , TVBind< TInput3 > , TVBind< TInput4 > > m_clntAdp ;
            TVServiceProxy< IVProvider< TVBind< ExpType > > >                    m_provMemCreator ;
            TVServiceArray< IVDynamicNew< TVBind< ExpType > > , SvcDynBind >     m_svcPartial     ;
            TVServiceProxyArray< IVTracer >                                      m_lstDataTracer  ;
            ClntType                                                             m_clntImp        ;

        public :
            EleImp( MyType & me , VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcDynSrc1 
                                , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcDynSrc2 
                                , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcDynSrc3 
                                , VI<  IVDynamicNew< TVBind< TInput4 > >  >& spcDynSrc4 )
                : m_owner( me )
            {   
                m_proxyDynSrc1.Create( spcDynSrc1 , [ this , &spcDynSrc2  , &spcDynSrc3 , &spcDynSrc4 ]( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 ){
                    _init( dynSrc1 , spcDynSrc2 , spcDynSrc3 , spcDynSrc4 ) ;
                } ) ;
            } 
            ~EleImp()
            {  
                m_svcTracer[1].Close() ;
                m_svcTracer[2].Close() ;
                m_svcTracer[3].Close() ;
                m_svcTracer[4].Close() ;
                TVServer< IVDirtyObject >::Close() ;
                m_svcPartial.Close() ;
            }

        private :
            virtual void Participate() 
            {
                m_owner.ParticipateTranslation( [this]( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr ){
                    _participateTo( usr ) ;
                } ) ;
            } 
            virtual void CleanAndDiffuse() 
            {
                m_proxyDynSrc1.UseServer(  [ this ]( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 ){
                    _reset( dynSrc1 ) ; 
                } ) ;

                m_lstDataTracer.TidyTravel( []( IVTracer & t ){ t.OnChanged() ; } ) ;
            } 

        private :
            virtual void _onChanged()
            {
                TVServer< IVDirtyObject >::Run( [ this ]( VI<  IVDirtyObject  >& spc ){
                    m_owner.RegisterDirty( spc ) ;
                } ) ;
            }
        private :
            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcDynSrc2  , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcDynSrc3 , VI<  IVDynamicNew< TVBind< TInput4 > >  >& spcDynSrc4 )
            {
                m_proxyDynSrc2.Create( spcDynSrc2 , [ this , &dynSrc1  , &spcDynSrc3 , &spcDynSrc4 ]( IVDynamicNew< TVBind< TInput2 > > & dynSrc2 ){
                    _init( dynSrc1 , dynSrc2 , spcDynSrc3 , spcDynSrc4 ) ;
                } ) ;
            }

            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2   , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcDynSrc3 , VI<  IVDynamicNew< TVBind< TInput4 > >  >& spcDynSrc4 )
            {
                m_proxyDynSrc3.Create( spcDynSrc3 , [ this , &dynSrc1 , &dynSrc2 , &spcDynSrc4 ]( IVDynamicNew< TVBind< TInput3 > > & dynSrc3 ){
                    _init( dynSrc1 , dynSrc2 , dynSrc3 , spcDynSrc4 ) ;
                } ) ;
            }

            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2   , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 , VI<  IVDynamicNew< TVBind< TInput4 > >  >& spcDynSrc4 )
            { 
                m_proxyDynSrc4.Create( spcDynSrc4 , [ this , &dynSrc1 , &dynSrc2 , &dynSrc3 ]( IVDynamicNew< TVBind< TInput4 > > & dynSrc4 ){
                    _init( dynSrc1 , dynSrc2 , dynSrc3 , dynSrc4 ) ;
                } ) ;
            }

            void _init( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 , IVDynamicNew< TVBind< TInput4 > > & dynSrc4 )
            { 
                m_svcTracer[ 1 ].Run( [ &dynSrc1 ]( VI<  IVTracer  >& spc ) { dynSrc1.Trace( spc ); } , *this );
                m_svcTracer[ 2 ].Run( [ &dynSrc2 ]( VI<  IVTracer  >& spc ) { dynSrc2.Trace( spc ); } , *this );
                m_svcTracer[ 3 ].Run( [ &dynSrc3 ]( VI<  IVTracer  >& spc ) { dynSrc3.Trace( spc ); } , *this );
                m_svcTracer[ 4 ].Run( [ &dynSrc4 ]( VI<  IVTracer  >& spc ) { dynSrc4.Trace( spc ); } , *this );

                _reset( dynSrc1 , dynSrc2 , dynSrc3 , dynSrc4 ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 )
            {
                m_proxyDynSrc2.UseServer(  [ this , & dynSrc1 ]( IVDynamicNew< TVBind< TInput2 > > & dynSrc2 ){
                    _reset( dynSrc1 , dynSrc2 ) ; 
                } ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 )
            {
                m_proxyDynSrc3.UseServer(  [ this , & dynSrc1 , & dynSrc2 ]( IVDynamicNew< TVBind< TInput3 > > & dynSrc3 ){
                    _reset( dynSrc1 , dynSrc2 , dynSrc3 ) ; 
                } ) ;
            }
            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 )
            {
                m_proxyDynSrc4.UseServer(  [ this , & dynSrc1 , & dynSrc2 , & dynSrc3 ]( IVDynamicNew< TVBind< TInput4 > > & dynSrc4 ){
                    _reset( dynSrc1 , dynSrc2 , dynSrc3 , dynSrc4 ) ; 
                } ) ;
            }

            void _reset( IVDynamicNew< TVBind< TInput1 > > & dynSrc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2 , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 , IVDynamicNew< TVBind< TInput4 > > & dynSrc4 )
            {
                dynSrc1.TakeMemory( VD_L2U( [ this , &dynSrc2 , &dynSrc3 , &dynSrc4 ]( VI<  TVBind< TInput1 >  >& spc1 ){
                    _reset( spc1 , dynSrc2 , dynSrc3 , dynSrc4 ) ;
                } ) ) ;
            }

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , IVDynamicNew< TVBind< TInput2 > > & dynSrc2  , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 , IVDynamicNew< TVBind< TInput4 > > & dynSrc4 )
            {
                dynSrc2.TakeMemory( VD_L2U( [ this , &spc1 , &dynSrc3 , &dynSrc4 ]( VI<  TVBind< TInput2 >  >& spc2 ){
                    _reset( spc1 , spc2 , dynSrc3 , dynSrc4 ) ;
                } ) ) ;
            }

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , VI<  TVBind< TInput2 >  >& spc2  , IVDynamicNew< TVBind< TInput3 > > & dynSrc3 , IVDynamicNew< TVBind< TInput4 > > & dynSrc4 )
            {
                dynSrc3.TakeMemory( VD_L2U( [ this , &spc1 , &spc2 , &dynSrc4 ]( VI<  TVBind< TInput3 >  >& spc3 ){
                    _reset( spc1 , spc2 , spc3 , dynSrc4 ) ;
                } ) ) ;
            }

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , VI<  TVBind< TInput2 >  >& spc2  , VI<  TVBind< TInput3 >  >& spc3 , IVDynamicNew< TVBind< TInput4 > > & dynSrc4 )
            {
                dynSrc4.TakeMemory( VD_L2U( [ this , &spc1 , &spc2 , &spc3 ]( VI<  TVBind< TInput4 >  >& spc4 ){
                    _reset( spc1 , spc2 , spc3 , spc4 ) ;
                } ) ) ;
            }

            void _reset( VI<  TVBind< TInput1 >  >& spc1 , VI<  TVBind< TInput2 >  >& spc2 , VI<  TVBind< TInput3 >  >& spc3 , VI<  TVBind< TInput4 >  >& spc4 )
            {
                m_clntAdp.template Create< ClntType & , TVServiceProxy< IVProvider< TVBind< ExpType > > > & >( spc1 , spc2 , spc3 , spc4 , m_clntImp , m_provMemCreator ) ;
            } 
            void _participateTo( VO<  IVDynamicNew< TVBind< ExpType > >  >& usr )
            {
                m_svcPartial.template RunNew< EleImp & >( *this , usr ) ;
            } 
            void _traceData( VI<  IVTracer  >& spc ) 
            {
                m_lstDataTracer.Add( spc ) ;
            }
            void _takeMemory( VO<  TVBind< ExpType >  >& usr )
            {
                m_provMemCreator.UseServer( [ &usr ]( IVProvider< TVBind< ExpType > > & prov ){
                    prov.Apply( usr ) ;
                } ) ;
            }
        } ;

    public :
        void Close()
        {  
            m_svcElements.Close() ;
        } 
        void PrepareInput() 
        {
        }

        template< class TSysImp1 , class TSysImp2 , class TSysImp3 , class TSysImp4 , class TExtEle1 , class TExtEle2  , class TExtEle3 , class TExtEle4 >
        void Create( TVBind< TSysImp1 , TSysImp2 , TSysImp3 , TSysImp4 > & sysInput
                   , TVBind< TExtEle1 , TExtEle2 , TExtEle3 , TExtEle4 > & inval 
                   , VO<  IVElement< TIDElem >  >& outval )
        {
            TSysImp1 & sysInput1 = sysInput.m_ref1 ;
            TSysImp2 & sysInput2 = sysInput.m_ref2 ;
            TSysImp3 & sysInput3 = sysInput.m_ref3 ;
            TSysImp4 & sysInput4 = sysInput.m_ref4 ;
            TExtEle1 & inval1 = inval.m_ref1 ;
            TExtEle2 & inval2 = inval.m_ref2 ;
            TExtEle3 & inval3 = inval.m_ref3 ;
            TExtEle4 & inval4 = inval.m_ref4 ;

            static_assert( std::is_same<  TInput1 , typename TSysImp1::ExpType >::value 
                           || std::is_same<  TInput1 , IVArrOwner< typename TSysImp1::ExpType > >::value , "输入系统无法提供对应类型的数据！" ) ;  
            static_assert( std::is_same<  TInput2 , typename TSysImp2::ExpType >::value 
                           || std::is_same< TInput2 , IVArrOwner< typename TSysImp2::ExpType  > >::value , "输入系统无法提供对应类型的数据！" ) ;  
            static_assert( std::is_same<  TInput3 , typename TSysImp3::ExpType >::value 
                           || std::is_same< TInput3 , IVArrOwner< typename TSysImp3::ExpType  > >::value , "输入系统无法提供对应类型的数据！" ) ;  
            static_assert( std::is_same<  TInput4 , typename TSysImp4::ExpType >::value 
                           || std::is_same< TInput4 , IVArrOwner< typename TSysImp4::ExpType  > >::value , "输入系统无法提供对应类型的数据！" ) ;  

            sysInput1.Export( inval1 , [ this , & sysInput2 , & inval2 , & sysInput3 , & inval3 , & sysInput4 , & inval4 , &outval ]( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 ){
                _create( spcInput1 , sysInput2 , inval2 , sysInput3 , inval3 , sysInput4 , inval4 , outval ) ;
            } ) ;
        }

    private :
        template< class TSysImp2 , class TExtEle2 , class TSysImp3 , class TExtEle3 , class TSysImp4 , class TExtEle4 >
        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , TSysImp2 & sysInput2  , TExtEle2 & inval2 
                    , TSysImp3 & sysInput3  , TExtEle3 & inval3 
                    , TSysImp4 & sysInput4  , TExtEle4 & inval4 
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            sysInput2.Export( inval2 , [ this , & spcInput1 , & sysInput3 , & inval3 , & sysInput4 , & inval4 , &outval ]( VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 ){
                _create( spcInput1 , spcInput2 , sysInput3 , inval3 , sysInput4 , inval4 , outval ) ;
            } ) ;
        } 

        template< class TSysImp3 , class TExtEle3 , class TSysImp4 , class TExtEle4 >
        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 
                    , TSysImp3 & sysInput3  , TExtEle3 & inval3 
                    , TSysImp4 & sysInput4  , TExtEle4 & inval4 
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            sysInput3.Export( inval3 , [ this , & spcInput1 , & spcInput2 , & sysInput4 , & inval4 , &outval ]( VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcInput3 ){
                _create( spcInput1 , spcInput2 , spcInput3 , sysInput4 , inval4 , outval ) ;
            } ) ;
        } 

        template< class TSysImp4 , class TExtEle4 >
        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2 
                    , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcInput3 
                    , TSysImp4 & sysInput4  , TExtEle4 & inval4 
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            sysInput4.Export( inval4 , [ this , & spcInput1 , & spcInput2 , & spcInput3 , &outval ]( VI<  IVDynamicNew< TVBind< TInput4 > >  >& spcInput4 ){
                _create( spcInput1 , spcInput2 , spcInput3 , spcInput4 , outval ) ;
            } ) ;
        } 

        void _create( VI<  IVDynamicNew< TVBind< TInput1 > >  >& spcInput1 
                    , VI<  IVDynamicNew< TVBind< TInput2 > >  >& spcInput2  
                    , VI<  IVDynamicNew< TVBind< TInput3 > >  >& spcInput3 
                    , VI<  IVDynamicNew< TVBind< TInput4 > >  >& spcInput4
                    , VO<  IVElement< TIDElem >  >& outval )
        {
            m_svcElements.RunNew( outval , * static_cast< MyType * >( this ) , spcInput1 , spcInput2 , spcInput3 , spcInput4 ) ;
        }

    private :
        TVServiceArray< IVElement< TIDElem > , EleImp > m_svcElements ; 
    } ;
   
    template< class TDerived , class TIDElem , class TInput , class TElement , class TExp > 
    class TVSysImpElement< TDerived , TIDElem , IVArrOwner< TInput > , TElement , TExp >
    { 
    public :
        typedef TDerived MyType  ;
        typedef TExp     ExpType ;

        typedef TVElementInstImp< MyType , IVArrOwner< TInput > , TIDElem , TElement , TExp > EleImp ; 
         
        template< class TChk >
        struct CheckInput
        {
            static const bool value = std::is_same< TInput[] , typename TChk::ExpType >::value ;
        } ; 

   public :
        TVSysImpElement()
        { 
        }
        ~TVSysImpElement()
        {
        }
        void Close()
        {  
            m_svcElements.Close() ;
        } 
        void PrepareInput() 
        {
        }

        template< class TSysImp , class TIDInput >
        void Create( TSysImp & sysInput , IVExtElementArr< TIDInput > & inval , VO<  IVElement< TIDElem >  >& outval )
        {       
            static_assert( std::is_same< TInput , typename TSysImp::ExpType >::value , "输入系统无法提供对应类型的数据！" ) ; 
            sysInput.Export( inval , [ this , &outval ]( VI<  IVDynamicNew< TVBind< IVArrOwner< TInput > > >  >& spcInput ){
                _create( spcInput , outval ) ;
            } ) ; 
        } 

    private :
        void _create( VI<  IVDynamicNew< TVBind< IVArrOwner< TInput > > >  >& spcInput , VO<  IVElement< TIDElem >  >& outval )
        {
            m_svcElements.RunNew( outval , * static_cast< TDerived *>( this ) , spcInput ) ;
        } 

    private :
        TVServiceArray< IVElement< TIDElem > , EleImp > m_svcElements ;
    } ; 
} 

template< class T >
class TVExportImp : public T
{
public :
    typedef T EXPORTDATA ;
    using T::T ;
} ;

template< class TIDElem , class TInput , class TElement , class TExp , class TExpUnit >
class TVSYSBase : public VN_ELEMSYS::TVSysImpTimeline
                , public VN_ELEMSYS::TVSysImpExport < TVSYSBase< TIDElem , TInput , TElement , TExp , TExpUnit > , TIDElem  , TExp > 
                , public VN_ELEMSYS::TVSysImpElement< TVSYSBase< TIDElem , TInput , TElement , TExp , TExpUnit > , TIDElem  , TInput , TElement , TExpUnit >
{ 
public :
    typedef TIDElem  IDElement ;
    typedef VN_ELEMSYS::TVSysImpExport < TVSYSBase< TIDElem , TInput , TElement , TExp , TExpUnit > , TIDElem  , TExp > OutputBaseType ;
    typedef VN_ELEMSYS::TVSysImpElement< TVSYSBase< TIDElem , TInput , TElement , TExp , TExpUnit > , TIDElem  , TInput , TElement , TExpUnit > TInputBaseType  ;

private:                                    
    TVSYSBase(const TVSYSBase &);
    TVSYSBase< TIDElem , TInput , TElement , TExp , TExpUnit > & operator=(const TVSYSBase &) ;

public :
    TVSYSBase()
    { 
    }
    ~TVSYSBase()
    {
    } 
    void Close() 
    {
        OutputBaseType::Close() ;
        TInputBaseType::Close() ;
    }   
    void Elapse()
    { 
        TInputBaseType::PrepareInput() ;
        VN_ELEMSYS::TVSysImpTimeline::ClearDirty() ;
        OutputBaseType::UpdateOutput() ;
    }
} ;


template< class TIDElem , class TInput , class TElement = TVBind< TInput > >
class TVSYS : public TVSYSBase< TIDElem , TInput 
                              , typename VN_ELEMSYS::TVDeduceExport< TElement >::ElemType
                              , typename VN_ELEMSYS::TVDeduceExport< TElement >::ExpType
                              , typename VN_ELEMSYS::TVDeduceExport< TElement >::ExpUnitType >
{   
private:                                    
    TVSYS(const TVSYS &);
    TVSYS< TIDElem , TInput , TElement > & operator=(const TVSYS &) ;
public :
    TVSYS()
    { 
    }
    ~TVSYS()
    {
    }  
} ; 
 
//template< class T > class TVSysAdaptor ; 


//template< class TSys , class TIDElem , class TExp >
//struct TSysAdpPort
//{
//    typedef TIDElem IDElement ; 
//    typedef TExp   ExpType    ;
//
//    TVSysAdaptor< IVDynSystem< TSys > > &m_sys ;
//    TSysAdpPort( TVSysAdaptor< IVDynSystem< TSys > > &sa ):m_sys( sa ){}
//
//    template< class F > //  void F( VI<  IVDynamicNew< TVBind< TInput > >  >& spcInput )
//    void Export( IVExtElement< IDElement > & inval , F f )
//    {
//        m_sys.Export( inval , VD_L2U( [ &f ]( VI<  IVDynamicNew< TVBind< ExpType > >  >& spc ){
//            f(spc) ;
//        } )) ; 
//    }
//} ;

//template< class TSys , class TIDElem , class TExp > 
//struct TVDataTrait< TSysAdpPort< TSys , TIDElem , TExp > >
//{
//    typedef TSysAdpPort< TSys , TIDElem , TExp > & reference ;
//} ; 

//template< class TSys >
//class TVSysAdaptor< IVDynSystem< TSys > > : public IVUser< IVSpace< IVDynSystem< TSys > > >
//{
//public :
//    TVSysAdaptor() {}
//    ~TVSysAdaptor(){}
//
//public :  
//    template< class TElem , class TData >
//    TSysAdpPort< TSys , TElem , TData > Port()
//    { 
//        return TSysAdpPort< TSys , TElem , TData >( * this ) ;
//    }
//    
//public :
//    template< class TD , class TE >
//    void Import( IVExtData2< TD > & inval , VO<  IVElement< TE >  >& outval ) 
//    {
//        m_proxySysMeshWork.UseServer( [ &inval , &outval ]( IVDynSystem< TSys > & sys ){
//            sys.Import( inval , outval ) ;
//        } ) ;
//    }
//    template< class TEIn , class TEOut >
//    void Create( IVExtElement< TEIn > & inval , VO<  IVElement< TEOut >  >& outval ) 
//    {
//        m_proxySysMeshWork.UseServer( [ &inval , &outval ]( IVDynSystem< TSys > & sys ){
//            sys.Create( inval , outval ) ;
//        } ) ;
//    }
//    template< class TEIn1 , class TEIn2 , class TEOut >
//    void Create( IVExtElement< TEIn1 > & inval1 , IVExtElement< TEIn2 > & inval2 , VO<  IVElement< TEOut >  >& outval ) 
//    {
//        m_proxySysMeshWork.UseServer( [ &inval1 , &inval2 , &outval ]( IVDynSystem< TSys > & sys ){
//            sys.Create( inval1 , inval2 , outval ) ;
//        } ) ;
//    }
//    template< class TEIn , class TDOut >
//    void Export( IVExtElement< TEIn > & inval , _VExtOut( TDOut ) outval )
//    {
//        m_proxySysMeshWork.UseServer( [ &inval , &outval ]( IVDynSystem< TSys > & sys ){
//            sys.Export( inval , outval ) ;
//        } ) ;
//    }
//
//private :
//    void Visit( IVSpace< IVDynSystem< TSys > > & spc )
//    {
//        m_proxySysMeshWork.Create( spc ) ;
//    }  
//
//private :
//    TVServiceProxy< IVDynSystem< TSys > > m_proxySysMeshWork ;  
//} ;
//
//
//template< class TSys , class TIDElem , class TExp , class F >
//inline void TSysAdpPort< TSys , TIDElem , TExp >::Export( IVExtElement< IDElement > & inval , F f )
//{
//} ;

template< class TIDElem , class TInput , class TElement > 
struct TVDataTrait< TVSYS< TIDElem , TInput , TElement > >
{
    typedef TVSYS< TIDElem , TInput , TElement > & reference ;
} ; 


template< class T > // void F( IVExtData2< T > & ed )
class TVExtScoped : private IVExtData2< T >
{
private :
    struct DynImp : IVDynamicNew< TVBind< T > > 
    {
        typename TVDataTrait< T >::reference data ;
        DynImp( typename TVDataTrait< T >::reference d ):data(d){} 
        ~DynImp(){ m_svc.Close() ; }
        virtual void Trace ( VI<  IVTracer  >& spc ){ }
        virtual void TakeMemory( VO<  TVBind< T >  >& usr ) 
        {
            m_svc.RunNew( usr , data ) ;
        }
        TVServiceArray< TVBind< T > > m_svc ;
    } ;

private :
    typename TVDataTrait< T >::reference m_data ;
    TVService< IVDynamicNew< TVBind< T > > , DynImp > m_svcs ; 
    TVExtScoped( typename TVDataTrait< T >::reference tmpData ) : m_data( tmpData ){}
    ~TVExtScoped(){ m_svcs.Close() ; }

private :
    void Apply( VO<  IVDynamicNew< TVBind< T > >  >& usr )
    {
        m_svcs.Run( usr , m_data ) ;
    }

public : 
    template< class F >
    static void UseData( typename TVDataTrait< T >::reference tmpData , F f )
    {
        f( TVExtScoped( tmpData ) ) ;
    }
} ;

template< class T >
class TVExtData2 : public IVUser< IVSpace< IVDynamicNew< TVBind< T > > > >  
                 , public IVExtData2< T >
                 , public TVServer< IVDynamicNew< TVBind< T > > >  
{
public :
    TVExtData2(){}
    ~TVExtData2()
    {
        TVServer< IVDynamicNew< TVBind< T > > >::Close() ;
    } 

public :
    template< class F >
    void Watch( F f )
    {  
        m_proxyDynBnd.UseServer( [ this , &f ]( IVDynamicNew< TVBind< T > > & dyn ){
            _watch( dyn , f ) ;
        } ) ;
    } 

    void Watch( IVUser< T > & usr )
    {  
        Watch( [ & usr ]( typename TVDataTrait< T >::reference ref ){
            usr.Visit( ref ) ;
        } ) ;
    } 

    virtual void Visit( VI<  IVDynamicNew< TVBind< T > >  >& spc )
    {
        m_proxyDynBnd.Create( spc ) ;
        m_proxyTracer.TidyTravel( []( IVTracer &t ){
            t.OnChanged() ;
        } ) ;
    }

    virtual void Apply( VO<  IVDynamicNew< TVBind< T > >  >& usr )
    {
        TVServer< IVDynamicNew< TVBind< T > > >::Run( usr ) ; 
    } 

    virtual void Trace ( VI<  IVTracer  >& spc )
    {
        m_proxyTracer.Add( spc ) ;  //.Create( spc ) ; 
    }

    virtual void TakeMemory ( VO<  TVBind< T >  >& usr ) 
    {
        m_proxyDynBnd.UseServer( [ &usr]( IVDynamicNew< TVBind< T > > & dyn ){
            dyn.TakeMemory( usr ) ;
        } ) ;
    } 

private :
    template< class F >
    void _watch( IVDynamicNew< TVBind< T > > & dyn , F f )
    {
        TVServiceProxy< TVBind< T > > proxy ;
        dyn.TakeMemory( VD_P2U( proxy , [ &f ]( TVBind< T > & bnd ){
            f( bnd.m_ref1 ) ;
        } ) ) ;
    }

private :
    TVServiceProxy< IVDynamicNew< TVBind< T > > > m_proxyDynBnd ; 
    TVServiceProxyArray< IVTracer > m_proxyTracer; 
} ;

template< class TDerived , class T , unsigned DCOUNT = 1 > 
class TVExtSource2 : public IVExtData2< T > 
{
public :
    TVExtSource2()
    {
        static_assert( std::is_base_of< T , TDerived >::value , "TDerived类需从T继承" ) ;
    }

    ~TVExtSource2()
    { 
        m_svcData.Close() ;
        m_svcsDyn.Close() ;
    } 

public :
    void Update()
    {
        m_proxyListener.TidyTravel( []( IVTracer & t ){ t.OnChanged() ; } ) ;
    }

private :
    typedef TVExtSource2< TDerived , T , DCOUNT > MyType ;

    struct DynImp : IVDynamicNew< TVBind< T > > 
    {
        MyType & m_owner ;
        DynImp( MyType & me ):m_owner(me){} 
        ~DynImp(){}
        virtual void Trace ( VI<  IVTracer  >& spc ){ m_owner._trace( spc ) ; }
        virtual void TakeMemory( VO<  TVBind< T >  >& usr ) { m_owner._takeMemory( usr ) ; } 
    } ;

    void Apply( VO<  IVDynamicNew< TVBind< T > >  >& usr )
    {
        m_svcsDyn.RunNew( usr , *this ) ;
    } 

private :
    virtual void _trace ( VI<  IVTracer  >& spc )
    {
        m_proxyListener.Add( spc ) ;
    } 
    virtual void _takeMemory( VO<  TVBind< T >  >& usr ) 
    { 
        m_svcData.RunNew( usr , * static_cast< TDerived * >( this ) ) ;
    }
     
private :
    TVServiceProxyArray< IVTracer > m_proxyListener ;
    TVServiceArray< TVBind< T > > m_svcData ;
    TVServiceArray< IVDynamicNew< TVBind< T > > , DynImp , DCOUNT > m_svcsDyn ; 
} ;

template< class TSrc , class TExp = TSrc , class TImp = TVBind< TExp > >
class TVExtTranslator2 : private TVServer< IVDynamicNew< TVBind< TExp > > > 
                       , public  IVExtData2< TExp >
                       , public  TVServer< IVTracer >
                       , public  IVUser< IVSpace< IVDynamicNew< TVBind< TSrc > > > >
{
public :
    TVExtTranslator2()
    {}
    ~TVExtTranslator2()
    {
        TVServer< IVTracer >::Close() ;
        TVServer< IVDynamicNew< TVBind< TExp > > >::Close() ;
    }

public :
    void Attach( IVExtData2< TSrc > & ed )
    {
        ed.Apply( *this ) ;
    } 
    virtual void Visit( VI<  IVDynamicNew< TVBind< TSrc > >  >& spc )
    {
        m_proxySrc.Create( spc, [this]( IVDynamicNew< TVBind< TSrc > > & dn ){ 
            this->TVServer< IVTracer >::Run( [&dn]( VI<  IVTracer  >& spc ){
                dn.Trace( spc ) ;
            } ) ;
        } ) ;

        OnChanged() ;
    } 
    virtual void OnChanged()
    { 
        m_proxyTracer.UseServer( []( IVTracer &t ){
            t.OnChanged() ;
        } ) ;
    } ;
    virtual void Trace ( VI<  IVTracer  >& spc )
    {
        m_proxyTracer.Create( spc ) ;  //.Create( spc ) ; 
    }
    virtual void TakeMemory ( VO<  TVBind< TExp >  >& usr ) 
    {
        m_proxySrc.UseServer( [ this , &usr]( IVDynamicNew< TVBind< TSrc > > & dyn ){
            _takeMemory( dyn , usr ) ;
        } ) ;
    } 
    virtual void Apply( VO<  IVDynamicNew< TVBind< TExp > >  >& usr )
    { 
        TVServer< IVDynamicNew< TVBind< TExp > > >::Run( usr ) ;
    }  
    void Watch( IVUser< TExp > & usr )
    {
        TVServiceProxy< TExp > sp ;
        TakeMemory( VD_P2U( sp , [ &usr]( TVBind< TExp > & val ){
            usr.Visit( val.m_ref1 ) ;
        } ) ) ;
    }

private :
    class Clnt 
    {
    public :
        Clnt( TVBind< TSrc > & src , VO<  TVBind< TExp >  >& usr )
            : m_imp( src.m_ref1 )
        {
            m_svc.Run( usr , m_imp ) ;
        }
        ~Clnt()
        {
            m_svc.Close() ;
        }
    private :
        TImp m_imp ;
        TVService< TVBind< TExp > > m_svc ;
    } ;
    void _takeMemory( IVDynamicNew< TVBind< TSrc > > & dyn , VO<  TVBind< TExp >  >& usr )
    {
        dyn.TakeMemory( VD_L2U( [ this, &usr ]( VI<  TVBind< TSrc >  >& spc ){
            _takeMemory( spc , usr ) ;
        } ) ) ;
    }
    void _takeMemory( VI<  TVBind< TSrc >  >& spc , VO<  TVBind< TExp >  >& usr )
    {
        m_clnt.template Create< VO< TVBind< TExp > >& >( spc , usr ) ;
    }

private :
    TVServiceProxy< IVDynamicNew< TVBind< TSrc > > > m_proxySrc ;
    TVClient< Clnt , TVBind< TSrc > > m_clnt ;
    TVServiceProxy< IVTracer > m_proxyTracer; 
} ;

template< class TSrc1 , class TSrc2 , class TExp , class TImp >
class TVExtTranslator2< TVBind< TSrc1 , TSrc2 > , TExp , TImp > : private TVServer< IVDynamicNew< TVBind< TExp > > > 
                                                                , public  IVExtData2< TExp > 
{
public :
    TVExtTranslator2()
    {}
    ~TVExtTranslator2()
    {
        m_tracer[0].Close() ;
        m_tracer[1].Close() ;
        TVServer< IVDynamicNew< TVBind< TExp > > >::Close() ;
    }
private :
    typedef TVExtTranslator2< TVBind< TSrc1 , TSrc2 > , TExp , TImp > MyType ;
    struct Tracer : IVTracer
    {
        MyType & my ;
        Tracer( MyType & t ): my(t){}
        void OnChanged()
        {
            my._onSrcChanged() ;
        }
    } ;  
    class Clnt
    {
    public :
        Clnt( TVBind< TSrc1 > & src1 , TVBind< TSrc2 > & src2  , VO<  TVBind< TExp >  >& usr ) 
            : m_imp( src1.m_ref1 , src2.m_ref1 )
        {
            m_svc.Run( usr , m_imp ) ;
        }
        ~Clnt()
        {
            m_svc.Close() ;
        }
    private :
        TImp m_imp ; 
        TVService< TVBind< TExp > > m_svc ;
    } ;

public :  
    void Attach( IVExtData2< TSrc1 > & ed1 , IVExtData2< TSrc2 > & ed2 )
    { 
        ed1.Apply( VD_P2U( m_proxySrc1 , [this]( IVDynamicNew< TVBind< TSrc1 > > & dn ){
            m_tracer[0].Run( [&dn]( VI<  IVTracer  >& spc ){
                dn.Trace( spc ) ;
            } , *this ) ;
        } ) ) ;  

        ed2.Apply( VD_P2U( m_proxySrc2 , [this]( IVDynamicNew< TVBind< TSrc2 > > & dn ){
            m_tracer[1].Run( [&dn]( VI<  IVTracer  >& spc ){
                dn.Trace( spc ) ;
            } , *this ) ;
        } ) ) ;  

        _onSrcChanged() ;
    } 
    virtual void Trace ( VI<  IVTracer  >& spc )
    {
        m_proxyTracer.Create( spc ) ;  //.Create( spc ) ; 
    }
    virtual void TakeMemory ( VO<  TVBind< TExp >  >& usr ) 
    {
        m_proxySrc1.UseServer( [ this , &usr]( IVDynamicNew< TVBind< TSrc1 > > & dyn1 ){
            _takeMemory( dyn1 , usr ) ;
        } ) ;
    }   
    virtual void Apply( VO<  IVDynamicNew< TVBind< TExp > >  >& usr )
    { 
        TVServer< IVDynamicNew< TVBind< TExp > > >::Run( usr ) ;
    }  

private :
    void _onSrcChanged()
    {
        m_proxyTracer.UseServer( []( IVTracer &t ){
            t.OnChanged() ;
        } ) ;
    }
    
    void _takeMemory( IVDynamicNew< TVBind< TSrc1 > > & dyn1 , VO<  TVBind< TExp >  >& usr )
    {
        m_proxySrc2.UseServer( [ this , &dyn1 , &usr]( IVDynamicNew< TVBind< TSrc2 > > & dyn2 ){
            _takeMemory( dyn1 , dyn2 , usr ) ;
        } ) ;
    }
        
    void _takeMemory( IVDynamicNew< TVBind< TSrc1 > > & dyn1 , IVDynamicNew< TVBind< TSrc2 > > & dyn2 , VO<  TVBind< TExp >  >& usr )
    {
        dyn1.TakeMemory( VD_L2U( [ this,&dyn2, &usr ]( VI<  TVBind< TSrc1 >  >& spc1 ){
            _takeMemory( spc1 , dyn2 , usr ) ;
        } ) ) ;
    }

    void _takeMemory( VI<  TVBind< TSrc1 >  >& spc1 , IVDynamicNew< TVBind< TSrc2 > > & dyn2 , VO<  TVBind< TExp >  >& usr )
    {
        dyn2.TakeMemory( VD_L2U( [ this,&spc1, &usr ]( VI<  TVBind< TSrc2 >  >& spc2 ){
            _takeMemory( spc1 , spc2 , usr ) ;
        } ) ) ;
    }
    void _takeMemory( VI<  TVBind< TSrc1 >  >& spc1 , VI<  TVBind< TSrc2 >  >& spc2 , VO<  TVBind< TExp >  >& usr )
    { 
        m_clnt.template Create< VO<  TVBind< TExp > >& >( spc1 , spc2 , usr ) ;
    }

private :
    TVServiceProxy< IVDynamicNew< TVBind< TSrc1 > > > m_proxySrc1 ;
    TVServiceProxy< IVDynamicNew< TVBind< TSrc2 > > > m_proxySrc2 ;
    TVService< IVTracer , Tracer > m_tracer[2] ;
    TVClient< Clnt , TVBind< TSrc1 > , TVBind< TSrc2 > > m_clnt ;
    TVServiceProxy< IVTracer > m_proxyTracer; 
} ;

template< class TSYS >
class TVExtSystemData  : public IVDynamicNew< TVBind< TSYS > >
{
public :
    TVExtSystemData( TSYS & sys )
        : m_sys( sys )
    {
    }
    ~TVExtSystemData()
    {
    }
    
private :
    virtual void Trace ( VI<  IVTracer  >& spc ){} 
    virtual void TakeMemory ( VO<  TVBind< TSYS >  >& usr ) 
    {
        m_svcData.template RunNew< TSYS & >( m_sys , usr ) ;
    }

private :
    TSYS & m_sys ; 
    TVServiceArray< TVBind< TSYS > > m_svcData      ;
} ;

class VKnownElement : public IVExtElement< VIDUnknown >
{
    void Apply(VO< IVDynamicNew< IVDelegate< VIDUnknown > > >&) {}
};
VD_DECLAREREFCLASS(VKnownElement) ;
 

#define VD_SYSTEM public :\

#define VD_SYSIMPCOMMONFUNC( sysname , c ) template<> void LoopElapse< c >() { LoopElapse< c - 1 >() ; sysname.Elapse() ; } template<> void LoopClose< c >() { LoopClose< c - 1 >() ; sysname.Close() ; } 
//
//#define VD_SYSIMPORT_IMP( c , output_id , system_name , input_type ) public :\
//    void Import( IVExtData2< input_type > & inval , VO<  IVElement< output_id >  >& outval )\
//    {   system_name.Import( inval , outval ) ; }\
//    private :\
//    typedef TVSYS< output_id , VN_ELEMSYS::TVDeduceImport< input_type >::DataType >  system_type_of_##system_name ;\
//    typedef system_type_of_##system_name::IDElement element_id_of_##system_name ;\
//    void getsystem( system_type_of_##system_name ** p ){ *p = &system_name ; } \
//    VD_SYSIMPCOMMONFUNC( system_name , c )\
//    system_type_of_##system_name system_name 

#define VD_SYSIMPORT_IMP( c , output_id , system_name , ... ) public :\
    void Import( IVExtData2< typename VN_ELEMSYS::TVDeduceImport< __VA_ARGS__ >::DataType > & inval , VO<  IVElement< output_id >  >& outval )\
    {   system_name.Import( inval , outval ) ; }\
    private :\
    typedef TVSYS< output_id , typename VN_ELEMSYS::TVDeduceImport< __VA_ARGS__ >::DataType , VN_ELEMSYS::TVDeduceImport< __VA_ARGS__ >::ImpType >  system_type_of_##system_name ;\
    typedef system_type_of_##system_name::IDElement element_id_of_##system_name ;\
    void getsystem( system_type_of_##system_name ** p ){ *p = &system_name ; } \
    VD_SYSIMPCOMMONFUNC( system_name , c )\
    system_type_of_##system_name system_name 

#define VD_SYSCREATE_IMP( c , output_id , system_name , input_sys , ... ) public : \
    void Create( VN_ELEMSYS::TVDeduceExtElement< element_id_of_##input_sys >::type & inval , VO<  IVElement< output_id >  >& outval )\
    { \
    std::remove_extent< system_type_of_##input_sys >::type * psys ;\
    getsystem( &psys ) ;\
    system_name.Create( *psys , inval , outval ) ; \
    } \
    private :\
    typedef TVSYS< output_id , typename VN_ELEMSYS::TVDeduceSystem< system_type_of_##input_sys >::ExpType , __VA_ARGS__ >  system_type_of_##system_name ;\
    typedef system_type_of_##system_name::IDElement element_id_of_##system_name ;\
    void getsystem( system_type_of_##system_name ** p ){ *p = &system_name ; } \
    VD_SYSIMPCOMMONFUNC( system_name , c )\
    system_type_of_##system_name system_name ;

#define VD_SYSCREATE2_IMP( c , output_id , system_name , input_sys1 , input_sys2 , ... ) public : \
    void Create( VN_ELEMSYS::TVDeduceExtElement< element_id_of_##input_sys1 >::type & inval1 \
               , VN_ELEMSYS::TVDeduceExtElement< element_id_of_##input_sys2 >::type & inval2 \
               , VO<  IVElement< output_id >  >& outval )\
    {\
    std::remove_extent< system_type_of_##input_sys1 >::type * psys1(0) ;\
    getsystem( &psys1 ) ;\
    std::remove_extent< system_type_of_##input_sys2 >::type * psys2(0) ;\
    getsystem( &psys2 ) ;\
    system_name.Create( vf_makebind( * psys1 , * psys2 ) , vf_makebind( inval1 , inval2 ) , outval ) ;\
    }\
    private :\
    typedef TVSYS< output_id , TVBind< typename VN_ELEMSYS::TVDeduceSystem< system_type_of_##input_sys1 >::ExpType , typename VN_ELEMSYS::TVDeduceSystem< system_type_of_##input_sys2 >::ExpType > , __VA_ARGS__ >  system_type_of_##system_name ;\
    typedef system_type_of_##system_name::IDElement element_id_of_##system_name ;\
    void getsystem( system_type_of_##system_name ** p ){ *p = &system_name ; }\
    VD_SYSIMPCOMMONFUNC( system_name , c )\
    system_type_of_##system_name system_name ; 

#define VD_SYSIMPORT( output_id , system_name , ... ) VD_SYSIMPORT_IMP( VD_EXPAND( __COUNTER__ ) , output_id , system_name , __VA_ARGS__ )
#define VD_SYSCREATE( output_id , system_name , input_sys , ... ) VD_SYSCREATE_IMP( VD_EXPAND( __COUNTER__ ) , output_id , system_name , input_sys , __VA_ARGS__ )
#define VD_SYSCREATE2( output_id , system_name , input_sys1 , input_sys2 , ... ) VD_SYSCREATE2_IMP( VD_EXPAND( __COUNTER__ ) , output_id , system_name , input_sys1 , input_sys2 , __VA_ARGS__ )
#define VD_SYSEXPORT( input_sys ) public :\
    void Export( VN_ELEMSYS::TVDeduceExtElement< element_id_of_##input_sys >::type & inval , _VExtOut( typename VN_ELEMSYS::TVDeduceSystem< system_type_of_##input_sys >::ExpType ) outval )\
    { input_sys.Export( inval , outval ) ; }

#define VD_SYSHEAD_IMP( sysName , c ) public : ~sysName(){ Close() ; } \
private : \
    template< unsigned N > void LoopElapse() ; \
    template< unsigned N > void LoopClose() ; \
    template<> void LoopElapse< c >() {}  \
    template<> void LoopClose< c >() {} 
#define VD_SYSHEAD( sysName ) VD_SYSHEAD_IMP( sysName , VD_EXPAND( __COUNTER__ ) )

#define VD_SYSTAIL_IMP( c ) public : virtual void Elapse() { LoopElapse< c >() ; } void Close(){ LoopClose< c >() ; }
#define VD_SYSTAIL() VD_SYSTAIL_IMP( VD_EXPAND( __COUNTER__ ) - 1 )
 
#define VD_BEGING_SYSTEM( sysName , sysInterf ) class sysName : public sysInterf {\
public :\
    sysName() {}\
private : \
    VD_SYSHEAD( sysName )
 
#define VD_BEGING_SYSTEM_SPRT( sysName , sysInterf , sysSupport ) class sysName : public sysInterf {\
public :\
    sysName( sysSupport & sprt ) : m_sprt( sprt ){}\
private : \
    sysSupport & m_sprt ;\
    VD_SYSHEAD( sysName )

#define VD_END_SYSTEM()  VD_SYSTAIL() } ;
