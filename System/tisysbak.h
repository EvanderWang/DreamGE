#pragma once

#include "vstd/tisys.h"
#include "vstd/strm.h"

struct VClock
{
    unsigned nTime ;
} ;

template< typename TSYSTRAIT , typename TSYS , typename TDEP > class TVSystemProviderImpBase ;

template< typename TSYSTRAIT , typename ... TRs , typename ... TDEP > 
class TVSystemProviderImpBase< TSYSTRAIT , IVSysDynamic< TRs ... > , TVPipelineArg< TDEP ... > > 
                : public IVProvider< IVSysDynamic< TRs ... > >
{
public :
    TVSystemProviderImpBase( const TDEP & ... esss ):m_arg( esss...){}
    ~TVSystemProviderImpBase(){ Close() ; }   

public :
    typedef TVPipelineArg< TDEP ... > TplArg    ; 
    typedef IVSysDynamic< TRs ... > SysInterf                            ;
    typedef TVDynamicSystemImp3< TSYSTRAIT , TplArg , SysInterf > SysImp ;

public :
    void Close()
    {
        m_svr.Close() ;
    } 

public:
    void Apply( VO< IVSysDynamic< TRs ... > > & usr )
    {
        m_svr.RunNew( usr , m_arg ) ;
    }

private : 
    TVPipelineArg< TDEP ... > m_arg;
    TVServiceArray< SysInterf , SysImp  > m_svr; 
} ; 

template< typename TSYSTRAIT , typename ... TRs > 
class TVSystemProviderImpBase< TSYSTRAIT , IVSysDynamic< TRs ... > , TVPipelineArg<> > 
    : public IVProvider< IVSysDynamic< TRs ... > >
{
public :
    TVSystemProviderImpBase(){}
    ~TVSystemProviderImpBase(){ Close() ; }   

public : 
    typedef IVSysDynamic< TRs ... > SysInterf                   ;
    typedef TVDynamicSystemImp2< TSYSTRAIT , SysInterf > SysImp ;

public :
    void Close()
    {
        m_svr.Close() ;
    } 

public :
    void Build( VSESS< TRs ... > & sysOut )
    { 
        sysOut.pSysProv = this ;
    } 

public :
    void Apply( VO< IVSysDynamic< TRs ... > > & usr )
    {
        m_svr.RunNew( usr ) ;
    }
private : 
    TVServiceArray< SysInterf , SysImp  > m_svr ;
} ; 

template< typename TSYSTRAIT , typename TSYS > 
using TVSystemProviderImp = TVSystemProviderImpBase< TSYSTRAIT , TSYS , typename TSYSTRAIT::PIPELINE::DEPENDENCE > ;


// relation_name: relation name
// ... : system_trait_name
// 
#ifdef _LIB
#ifndef VLIBEXPFUNCNAME
    static_assert( 0 , "Need define export function name like this: VLIBEXPFUNCNAME=vf_$(ProjectName)_get_system; " ) ;
#endif
#endif

template< typename TSysTrait , typename TSYS , typename TDEP > class TVDynamicSystemBuilderBase ;
template< typename TSysTrait , typename ... TRs , typename ... TARGs >
class TVDynamicSystemBuilderBase< TSysTrait , IVSysDynamic< TRs ... > , TVPipelineArg< TARGs ... > >
    : private IVProvider< IVSysDynamic< TRs ... > >
{
public :
    TVDynamicSystemBuilderBase()
    {
    }
    ~TVDynamicSystemBuilderBase()
    {
        Close() ;
    }
public :
    void Close() 
    {
        m_args.Clear() ;
        m_svr.Close() ;
    }
public : 
    void Build( VSESS< TRs ... > & sysOut , const TARGs & ... args ) 
    {
        m_svr.Close() ;
        m_args.Renew( args ... ) ;
        sysOut.pSysProv = this ;
    } 
private :
    typedef TVDynamicSystemImp2< TSysTrait , IVSysDynamic< TRs ... > > SysImpType ;

    void Apply( VO< IVSysDynamic< TRs ... > > & usr )
    {
        VASSERT( ! m_args.IsNull() ) ;
        m_args.Use( [ this , & usr ]( TVPipelineArg< TARGs ... > & pa )
        {
            pa.GetSome( [ this , & usr ]( const TARGs & ... args ){
                m_svr.RunNew( usr , args ... ) ;
            } ) ;
        }) ;
    }

    TVSafeScopedObj< TVPipelineArg< TARGs ... > > m_args ;
    TVServiceArray< IVSysDynamic< TRs ... > , SysImpType > m_svr ; 
} ;   

template< typename TSysTrait , typename TSYS >
using TVDynamicSystemBuilder = TVDynamicSystemBuilderBase< TSysTrait , TSYS , typename TSysTrait::PIPELINE::DEPENDENCE >;

// 
#define VD_EXPORT_SYSTEM( system_trait_name , ... )  void VLIBEXPFUNCNAME( VSESS< __VA_ARGS__ > & extsys ) \
{ static TVSystemProviderImp< system_trait_name , IVSysDynamic< __VA_ARGS__ > > svr ; extsys.pSysProv = &svr ; }

// imp_builder 是relation相关的builder
#define VD_EXPORT_SYSTEM_SIMPLE( imp_builder , ralation ) void VLIBEXPFUNCNAME( typename TVSystemToExternal< TVRSimpSysTrait< ralation , imp_builder >::SYSTYPE >::type & extsys ) \
{ static TVSystemProviderImp< TVRSimpSysTrait< ralation , imp_builder > , TVRSimpSysTrait< ralation , imp_builder >::SYSTYPE > svr ; extsys.pSysProv = &svr ; }

// 当ralation的输出类型是VSESS< TR > 
// systrait所描述的系统是TR对应的系统实现描述
#define VD_EXPORT_SYSTEM_SYSTEM_SIMPLE( systrait , ralation ) using __SysBuilderOf##systrait = \
                TVDynamicSystemBuilder< systrait , typename TVExternalToSystem< ralation::VALUE >::type > ;\
                VD_EXPORT_SYSTEM_SIMPLE( __SysBuilderOf##systrait , ralation ) ;

#define VD_EXPORT_SYSTEM_SYSTEM_SIMPLE_NAME( name , systrait , ralation ) using __SysBuilderOf##name = \
                TVDynamicSystemBuilder< systrait , typename TVExternalToSystem< ralation::VALUE >::type > ;\
                VD_EXPORT_SYSTEM_SIMPLE( __SysBuilderOf##name , ralation ) ;

//template< unsigned LIBID , class TEXP , typename TFUNC , typename IMPFUNC > ;
//
//template< unsigned LIBID , class TEXP >
//class VPackageLiberary< LIBID , TEXP , int (*)( VSESS< TEXP > & ) >
//{
//} ;

//using libname = VPackageLiberary< VD_EXPAND( __COUNTER__ ) , __VA_ARGS__ , vf_##libname##_get_system > ; 
template< typename TLIBID , typename ... TR > struct TVExtSystemLiberary ;

#define VD_IMPORT_SYSTEM_WITH_ID( libname , nID , ... ) extern void vf_##libname##_get_system( VSESS< __VA_ARGS__ > & ) ;
#define VD_IMPORT_DLL_SYSTEM_WITH_ID( libname , nID , ... ) void vf_##libname##_get_system( VSESS< __VA_ARGS__ > & ) ;
#define VD_DEF_EXT_SYSTEM( libname , varname , ... )  VSESS< __VA_ARGS__ > varname ; vf_##libname##_get_system( varname ) ;
#define VD_SYSFUNC_OF_LIB( libname ) vf_##libname##_get_system
#define VD_GET_EXT_SYSTEM( libname , ... )  ; vf_##libname##_get_system( __VA_ARGS__ ) ;


//struct VLID_##libname{ static const unsigned ID = nID ; } ; using libname = TVExtSystemLiberary< VLID_##libname , __VA_ARGS__ > ;

template< typename ... TRs > class TVPrimExtSys ;

template< typename ... TRs > class TVExtSource ;

#define VD_IMPORT_STLIB_SYSTEM_WITH_ID( libname , ... ) VD_IMPORT_SYSTEM_WITH_ID( libname , VD_EXPAND( __COUNTER__ ) , __VA_ARGS__ ) \
                                                        __declspec(dllexport) void VLIBEXPFUNCNAME( VSESS< __VA_ARGS__ > & e ) { vf_##libname##_get_system( e ) ; }

#define VD_IMPORT_SYSTEM( libname , ... ) VD_IMPORT_SYSTEM_WITH_ID( libname , VD_EXPAND( __COUNTER__ ) , __VA_ARGS__ ) \
template<> class TVPrimExtSys< __VA_ARGS__ > : public VSESS< __VA_ARGS__ >\
{ public : \
TVPrimExtSys(){vf_##libname##_get_system( *this ) ;}\
~TVPrimExtSys(){}\
} ;\
template<> class TVExtSource< __VA_ARGS__ > : public IVSlot< VSESS< __VA_ARGS__ > >\
{\
public:\
    TVExtSource()\
    {\
        vf_##libname##_get_system( m_imp ) ;\
    }\
    ~TVExtSource()\
    { m_svcDyn.Close() ;m_svcData.Close();}\
private:\
    typedef VSESS< __VA_ARGS__ > DATATYPE ;\
    typedef TVExtSource< __VA_ARGS__ > my_type ;\
    struct Prov : IVDataProv< DATATYPE >\
    {\
        my_type & me;\
        Prov( my_type & m ) : me( m ) {}\
        void Apply( VO< TVConstRef< DATATYPE > > & usr ){ me._getData( usr ); }\
    };\
public:\
    virtual void Trace( VI< IVTracer > & spc )\
    {\
        m_listener.Add( spc ) ;\
    }\
    void GetData( VO< IVDataProv< DATATYPE > > & usr )\
    {\
        m_svcDyn.RunNew( usr , *this );\
    }\
    const DATATYPE & operator*() const \
    {\
        return _getValue() ;\
    }\
    const DATATYPE * operator->() const \
    { \
        return &(_getValue());\
    } \
    void Output( IVInputPort< DATATYPE > &  inp )\
    {\
        inp.Input( *this ) ;\
    }\
private:\
    void _getData( VO< TVConstRef< DATATYPE > > & usr )\
    {\
        m_svcData.RunNew( usr , m_imp );\
    }\
    const DATATYPE & _getValue() const\
    {\
        return ( m_imp );\
    }\
private:\
    TVServiceArray< IVDataProv< DATATYPE > , Prov > m_svcDyn;\
    TVServiceArray< TVConstRef< DATATYPE > > m_svcData;\
    TVServiceProxyArray< IVTracer >          m_listener ;\
    DATATYPE m_imp ;\
};

#define VD_IMPORT_DLL_SYSTEM( libname , ... ) VD_IMPORT_DLL_SYSTEM_WITH_ID( libname , VD_EXPAND( __COUNTER__ ) , __VA_ARGS__ ) \
template<> class TVPrimExtSys< __VA_ARGS__ > : public VSESS< __VA_ARGS__ >\
{ public : \
TVPrimExtSys(){vf_##libname##_get_system( *this ) ;}\
~TVPrimExtSys(){}\
} ;\
template<> class TVExtSource< __VA_ARGS__ > : public IVSlot< VSESS< __VA_ARGS__ > >\
{\
public:\
    TVExtSource()\
    {\
        vf_##libname##_get_system( m_imp ) ;\
    }\
    ~TVExtSource()\
    { m_svcDyn.Close() ;m_svcData.Close();}\
private:\
    typedef VSESS< __VA_ARGS__ > DATATYPE ;\
    typedef TVExtSource< __VA_ARGS__ > my_type ;\
    struct Prov : IVDataProv< DATATYPE >\
    {\
        my_type & me;\
        Prov( my_type & m ) : me( m ) {}\
        void Apply( VO< TVConstRef< DATATYPE > > & usr ){ me._getData( usr ); }\
    };\
public:\
    virtual void Trace( VI< IVTracer > & spc )\
    {\
        m_listener.Add( spc ) ;\
    }\
    void GetData( VO< IVDataProv< DATATYPE > > & usr )\
    {\
        m_svcDyn.RunNew( usr , *this );\
    }\
    const DATATYPE & operator*() const \
    {\
        return _getValue() ;\
    }\
    const DATATYPE * operator->() const \
    { \
        return &(_getValue());\
    } \
    void Output( IVInputPort< DATATYPE > &  inp )\
    {\
        inp.Input( *this ) ;\
    }\
private:\
    void _getData( VO< TVConstRef< DATATYPE > > & usr )\
    {\
        m_svcData.RunNew( usr , m_imp );\
    }\
    const DATATYPE & _getValue() const\
    {\
        return ( m_imp );\
    }\
private:\
    TVServiceArray< IVDataProv< DATATYPE > , Prov > m_svcDyn;\
    TVServiceArray< TVConstRef< DATATYPE > > m_svcData;\
    TVServiceProxyArray< IVTracer >          m_listener ;\
    DATATYPE m_imp ;\
};

template< typename TSYSTRAIT , typename TRSYS , typename ... TINPUTs > 
void vf_run( const TINPUTs & ... para )
{
    TVPipelineArg< TINPUTs ... > arg( para... ) ;
    TVDynamicSystemImp3< TSYSTRAIT , TVPipelineArg< TINPUTs ... > , IVSysDynamic< TRSYS > > sys( arg ) ;
    TVHub< TRSYS > hub( sys ) ;
    sys.Elapse() ;
} ;


template< typename TSYSTRAIT , typename TRSYS , typename TF , typename ... TINPUTs > 
void vf_run_while( TF f , const TINPUTs & ... para )
{
    TVPipelineArg< TINPUTs ... > arg( para... ) ;
    TVDynamicSystemImp3< TSYSTRAIT , TVPipelineArg< TINPUTs ... > , IVSysDynamic< TRSYS > > sys( arg ) ;
    TVHub< TRSYS > hub( sys ) ;
    TVData< TRSYS::VALUE > m_rtn ;

    hub.Output( m_rtn ) ;

    auto localCheckReturn = [ f ]( TVData< TRSYS::VALUE > & m_rtn )->bool{
        bool brtn(false) ;
        m_rtn.Peer( [&brtn , f]( const TRSYS::VALUE & v ){
            brtn = f( v ) ;
        }) ;
        return brtn ;
    } ;

    do
    {
        sys.Elapse() ; 
    } while(  localCheckReturn( m_rtn ) );
} ;
//
//template< typename T > class  TVRelationNodeServerBase ; 
//template< typename TRID , typename ... TOUs , typename ... TINPUTs >
//class TVRelationNodeServerBase< IVRUniqHub< TRID , VI_AND< TOUs ... > , TINPUTs ... > >
//{
//public :
//    TVRelationNodeServerBase()
//    {}
//    ~TVRelationNodeServerBase()
//    {}
//    void Build( VSESS< IVRLTN< TRID > > & sys )
//    {
//    }
//} ;
//
//template< typename TRID , typename OU , typename ... TINPUTs >
//class TVRelationNodeServerBase< IVRUniqHub< TRID , OU , TINPUTs ... > >
//{
//public :
//    TVRelationNodeServerBase()
//    {}
//    ~TVRelationNodeServerBase()
//    {}
//    void Build( VSESS< IVRLTN< TRID > > & sys )
//    {
//    }
//} ;
//
//template< typename TRID , typename ... TRDEPIDs > 
//class TVRelationNodeServer
//    : public TVRelationNodeServerBase< typename IVRLTN< TRID >::HUB >
//{ 
//public :
//    using TVRelationNodeServerBase< typename IVRLTN< TRID >::HUB >::TVRelationNodeServerBase ;
//} ; 
//
//template< typename TLIBID , typename ... TR > struct TVExtSystemLiberary ;
//
//template< typename TRID , typename TLIBID , typename ... TROTHERs >
//struct TVConverterTrait< IVRLTN< TRID > , TVExtSystemLiberary< TLIBID , TROTHERs ... > > 
//    : TVConverterTrait< TVRelationNodeServer< TRID > >
//{
//} ;


template< typename TR , typename TNAME , typename TVALUE > class TVVirtualMachineBase ; 
template< typename TR , typename TNAME , typename TVALUE > class TVR2BBase ;

template< typename F  >
void vf_ChangeValueToSource( F f )
{
    f() ;
}
template< typename F , typename T , typename ... TOTHERs >
void vf_ChangeValueToSource( F f , const T & t , const TOTHERs & ... others )
{
    TVSource< T > src( t ) ;

    auto fsub = [ f , &src ]( TVSource< TOTHERs > & ... otherSrcs ){
        f( src , otherSrcs ... );
    };

    vf_ChangeValueToSource( fsub , others ... ) ;
} 
//
//
//
//template< typename TRID >
//void vf_AttachOutputsToHub( TVHub< IVRLTN< TRID > > &hub )
//{ 
//}
//
//template< typename TRID , typename TFST , typename ... TOUOTHERs >
//void vf_AttachOutputsToHub( TVHub< IVRLTN< TRID > > &hub , TVData< TFST > & tfst , TVData< TOUOTHERs > & ... others )
//{
//    hub.Output( tfst ) ;
//    vf_AttachOutputsToHub( hub , others ... ) ;
//} 
//
//template< typename F >
//void vf_AccessOutputsWithPipelineArg( F f )
//{ 
//    f() ;
//}
//
//template< typename F , typename TFST , typename ... TOUOTHERs >
//void vf_AccessOutputsWithPipelineArg( F f , TVData< TFST > & tfst , TVData< TOUOTHERs > & ... others )
//{
//    auto FSub = [ f , & tfst ]( const TOUOTHERs & ... others ){
//        tfst.Peer( [ f , & others ... ]( const TFST & fst ){
//            f( fst , others ... ) ;
//        } ) ;
//    } ;
//
//    vf_AccessOutputsWithPipelineArg( FSub , others ... ) ;
//} 
//
//template< typename ... TOUTPUTs > class TVVirtualMachineOutput ;
//
//template<>
//class TVVirtualMachineOutput<>
//{
//public :
//    template< typename F >
//    static void UseData( F f )
//    {
//        f() ;
//    } ; 
//} ;
//
//template< typename TFST , typename ... TOTHERs >
//class TVVirtualMachineOutput< TFST , TOTHERs ... >
//{
//public : 
//    template< typename F >
//    static void UseData( F f )
//    {
//        TVVirtualMachineOutput< TOTHERs ... >::UseData( [ f ]( TVData< TOTHERs > & ... dts ){
//            TVData< TFST > data;
//            f( data , dts ... );
//        } );
//    };
//} ;

//namespace VCMN
//{
//    template <typename F, typename Tuple, bool Done, int Total, int... N>
//    struct call_impl
//    {
//        static void call(F f, Tuple && t)
//        {
//            call_impl<F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
//        }
//    };
//
//    template <typename F, typename Tuple, int Total, int... N>
//    struct call_impl<F, Tuple, true, Total, N...>
//    {
//        static void call(F f, Tuple && t)
//        {
//            f(std::get<N>(std::forward<Tuple>(t))...);
//        }
//    };
//
//    template <typename F, typename Tuple>
//    void TravelTuple( Tuple && t , F f )
//    {
//        typedef typename std::decay<Tuple>::type ttype;
//        VCMN::call_impl<F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
//    }
//} ;

template< typename ... TINPUTs > class TVVMInputPort ;
template<> class TVVMInputPort<>
{
public :
    TVVMInputPort()
    {}
    ~TVVMInputPort()
    {}
public :
    template< typename F >
    void Travel( F f )
    {
        f() ;
    }
};
template< typename TFst , typename ... TOthers > 
class TVVMInputPort< TFst , TOthers ... >
{
public :
    TVVMInputPort( const TFst & v , const TOthers & ... vOthers )
        : m_val( v ) , m_others( vOthers ... )
    {}
    ~TVVMInputPort()
    {
    }
public :
    template< typename F >
    void Travel( F f )
    {
        m_others.Travel( [ f , this ]( TVSource< TOthers > &  ... srcs  ){
            f( m_val , srcs ... ) ;
        } ) ;
    }

private :
    TVSource< TFst > m_val ;
    TVVMInputPort< TOthers ... > m_others ;
} ;

template< typename ... TOUTPUTs > class TVVMOutputPort ;
template< > class TVVMOutputPort< >
{
public :
    TVVMOutputPort()
    {}
    ~TVVMOutputPort()
    {}
public :
    template< typename TRID >
    void AttachTo( TVHub< IVRLTN< TRID > > & hub )
    { 
    }
    template< typename F >
    void TravelValue( F f )
    {
        f() ;
    }
    template< typename F >
    void TravelData( F f )
    {
        f() ;
    }
    void CopyTo( )
    {
    }
    void InputPort()
    { 
    }
    
    void OutputAllPorts(IVInputPort<> & ip)
    {
        ip.Input() ;
    }
};
template< typename TFst , typename ... TOthers > 
class TVVMOutputPort< TFst , TOthers ... >
{
public :
    TVVMOutputPort()
    {}
    ~TVVMOutputPort()
    {}
private :
    class InputPortAdp : public IVInputPort< TOthers ... > 
    {
    public : 
        InputPortAdp( IVInputPort< TFst , TOthers ... > & ip , TVData< TFst > & d ) :m_ip(ip), m_data(d){}; 
        TVData< TFst > & m_data ;
        IVInputPort< TFst , TOthers ... > & m_ip ;
        virtual void Input ( IVSlot< TOthers > & ... sltOthers ) 
        {
            m_ip.Input( m_data , sltOthers... ) ;
        }
    } ;
public :
    template< typename TRID >
    void AttachTo( TVHub< IVRLTN< TRID > > & hub )
    {
        hub.Output( m_data ) ;
        m_others.AttachTo( hub ) ;
    }
    template< typename F >
    void TravelValue( F f )
    {
        m_data.Peer( [ f , this ]( const TFst & v ){
            m_others.TravelValue( [ f , &v ]( const TOthers &  ... srcs ){
                f( v , srcs ... );
            } );
        } );
    }
    template< typename F >
    void TravelData( F f )
    {
        m_others.TravelData( [ f , this ]( TVData< TOthers > &  ... srcs ){
            f( m_data , srcs ... );
        } );
    }
    void CopyTo( TFst & fst , TOthers & ... others )
    {
        m_data.Peer( [ & fst ]( const TFst & v ){
            fst = v ;
        }) ;
        m_others.CopyTo( others ... ) ;
    }
    template< typename T >
    void OutputPort( IVInputPort< T > & ip )
    {
        m_others.OutputPort( ip ) ;
    }  
    void OutputPort( IVInputPort< TFst > & ip )
    {
        VLNK( ip , m_data ) ;
    }

    void OutputAllPorts( IVInputPort< TFst , TOthers ... > & ip ) //, IVInputPort< TOthers > & ... ipOthers )
    { 
        InputPortAdp ia( ip , m_data ) ;
        //VLNK( ip , m_data ) ;
        m_others.OutputAllPorts( ia ) ; //ipOthers ... ) ;
    }

    void InputPort( IVSlot< TFst > & slotFst , IVSlot< TOthers > & ... slotOthers )
    {
        VLNK( m_data , slotFst ) ;
        m_others.InputPort( slotOthers ... ) ;
    }

private :
    TVData< TFst > m_data ;
    TVVMOutputPort< TOthers ... > m_others ;
} ;

template< typename ... TOUTPUTs > class TVVMOutputPersist ;
template< > class TVVMOutputPersist<>
{
public :
    TVVMOutputPersist()
    {}
    ~TVVMOutputPersist()
    {}
public :
    template< typename F >
    void Travel( F f )
    {
        f() ;
    }
    void CopyFrom()
    {
    }

    void CopyTo()
    {
    }
    
    template< typename ... TPREs >
    TVPipelineArg< TPREs ... > GetValuePackage( const TPREs & ... pres ) const
    {
        return TVPipelineArg< TPREs ... >( pres ... ) ;
    }
};

template< typename TFst , typename ... TOthers > 
class TVVMOutputPersist< TFst , TOthers ... >
{
public :
    TVVMOutputPersist()
    {}
    ~TVVMOutputPersist()
    {}
public :
    template< typename F >
    void Travel( F f )
    {
        m_others.Travel( [ f , this ]( const TOthers &  ... srcs ){
            f( m_data.Get() , srcs ... );
        } );
    }

    void CopyFrom( const TFst & fst , const TOthers & ... others )
    {
        m_data.Build( fst ) ;
        m_others.CopyFrom( others ... ) ;
    }

    void CopyTo( TFst & fst , TOthers & ... others )
    {
        fst = m_data.Get() ;
        m_others.CopyTo( others ... ) ;
    }

    template< typename ... TPREs >
    TVPipelineArg< TPREs ... , TFst , TOthers ... > GetValuePackage( const TPREs & ... pres ) const
    {
        return m_others.GetValuePackage( pres ... , m_data.Get() ) ;
    }

private :
    TVPersist< TFst > m_data ;
    TVVMOutputPersist< TOthers ... > m_others ;
} ;

template< typename TRID , typename ... TINPUTs , typename ... TOUTPUTs >
class TVVirtualMachineBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , VI_AND< TOUTPUTs ... > > 
{
public :
    TVVirtualMachineBase(  void (*sf)( VSESS< IVRLTN< TRID > > & ) , const TINPUTs & ... inps )
        :m_arg( inps ... )
    {
        VSESS< IVRLTN< TRID > > es ;
        sf( es ) ;
        es.pSysProv->Apply( VD_P2U( m_sysProxy ) ) ; 
        m_sysProxy.UseServer( [ this ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
            _buildHub( sys ) ;
        } ) ; 
    } 

    TVVirtualMachineBase( const VSESS< IVRLTN< TRID > > & es , const TINPUTs & ... inps )
        :m_arg( inps ... )
    {
        es.pSysProv->Apply( VD_P2U( m_sysProxy ) ) ;

        m_sysProxy.UseServer( [ this ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
            _buildHub( sys ) ;
        } ) ; 
    } 
    ~TVVirtualMachineBase()
    {
    }

public :
    template< typename F >
    void Run( F fCheck )
    { 
        m_sysProxy.UseServer( [ this , fCheck ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
            _run( fCheck , sys ) ;
        } ) ; 
    }  

    void Close()
    {
    }

private :   
    void _buildHub( IVSysDynamic< IVRLTN< TRID > > & sys )
    { 
        m_arg.Travel( [ this , &sys ]( TVSource< TINPUTs > & ... srcs ){
            m_hub.Create( sys , srcs ... ) ;
        } ) ; 

        m_rtn.AttachTo( m_hub ) ;
    } 

    template< typename F >
    void _run( F f , IVSysDynamic< IVRLTN< TRID > > & sys )
    { 
        sys.Elapse() ;
        m_rtn.TravelValue( f ) ;
    }  

private : 
    TVVMInputPort< TINPUTs ...  > m_arg ;
    TVVMOutputPort< TOUTPUTs ... > m_rtn ; 
    TVHub< IVRLTN< TRID > > m_hub ;  
    TVServiceProxy< IVSysDynamic< IVRLTN< TRID > > > m_sysProxy ;  
} ;

template< typename TRID , typename ... TINPUTs , typename TOUTPUT >
class TVVirtualMachineBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , TOUTPUT >
    : public TVVirtualMachineBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , VI_AND< TOUTPUT > >
{
public :
    typedef TVVirtualMachineBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , VI_AND< TOUTPUT > > base_type ;
    using base_type::base_type ;
    ~TVVirtualMachineBase()
    {
        base_type::Close() ;
    }
} ;

template< typename TR > 
using TVVM = TVVirtualMachineBase< TR , typename TR::NAME , typename TR::VALUE > ;


template< typename TRID , typename ... TINPUTs , typename ... TOUTPUTs >
class TVR2BBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , VI_AND< TOUTPUTs ... > > 
{
public :
    TVR2BBase( void (*sf)( VSESS< IVRLTN< TRID > > & ) ) 
    {
        sf( m_extsys ) ; 
        m_extsys.pSysProv->Apply( VD_P2U( m_sysProxy ) ) ; 
        m_sysProxy.UseServer( [ this ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
            m_hub.Create( sys ) ;
        } ) ; 
    }

    TVR2BBase( const VSESS< IVRLTN< TRID > > & es )
        : m_extsys( es ) 
    {
        m_extsys.pSysProv->Apply( VD_P2U( m_sysProxy ) ) ;

        m_sysProxy.UseServer( [ this ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
            m_hub.Create( sys ) ;
        } ) ; 
    } 
    ~TVR2BBase()
    {
    }

public :
    void Build( TOUTPUTs & ... vouts , const TINPUTs & ... inps )
    { 
        _run( inps ... ) ;
        m_PerssitRtnValue.CopyTo( vouts ... ) ;
    } 

    TVPipelineArg< TOUTPUTs ... > Run( const TINPUTs & ... inps )
    {
        _run( inps ... ) ;
        return m_PerssitRtnValue.GetValuePackage() ;
    }

private : 
    template< typename F >
    void _usesystem( F f )
    {
        bool bUsed(true); 
        m_sysProxy.UseServer( [ f , this ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
            f( sys , m_hub );
        } , [ &bUsed ](){
            bUsed = false ;
        } );

        if( !bUsed )
        {
            m_extsys.pSysProv->Apply( VD_P2U( m_sysProxy ) );
            m_sysProxy.UseServer( [ f , this  ]( IVSysDynamic< IVRLTN< TRID > > & sys ){
                m_hub.Create( sys ) ; //( sys );
                f( sys , m_hub ) ;
            } ); 
        }
    }

    void _run( const TINPUTs & ... inps )
    {
        TVVMInputPort< TINPUTs ...  > m_arg( inps ... ) ;
        TVVMOutputPort< TOUTPUTs ... > m_rtn ;

        _usesystem(  [ this , & m_arg , &m_rtn ]( IVSysDynamic< IVRLTN< TRID > > & sys , TVHub< IVRLTN< TRID > > & hub ){
            _run( sys , hub , m_arg , m_rtn ) ;
        } ) ;

        m_rtn.TravelValue( [ this ]( const TOUTPUTs & ... ous ){
            m_PerssitRtnValue.CopyFrom( ous ... );
        } );
    }
    void _run( IVSysDynamic< IVRLTN< TRID > > & sys , TVHub< IVRLTN< TRID > > & hub , TVVMInputPort< TINPUTs ...  > & arg , TVVMOutputPort< TOUTPUTs ... > & rtn )
    { 
        arg.Travel( [ &hub ]( TVSource< TINPUTs > & ... srcs ){
            hub.BatchConnect( srcs ... ) ;
        }) ;

        rtn.AttachTo( hub ) ;
        sys.Elapse() ;
    } 
     
    void _run( IVSysDynamic< IVRLTN< TRID > > & sys , TOUTPUTs & ... vouts )
    { 
        //sys.Elapse() ;

        //m_rtn.CopyTo( vouts ... ) ;
    } 

private : 
    VSESS< IVRLTN< TRID > >  m_extsys ;
    TVHub< IVRLTN< TRID > > m_hub ;  
    TVServiceProxy< IVSysDynamic< IVRLTN< TRID > > > m_sysProxy ;   
    TVVMOutputPersist< TOUTPUTs ... > m_PerssitRtnValue ;
} ;

template< typename TRID , typename ... TINPUTs , typename TOUTPUT >
class TVR2BBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , TOUTPUT >
    : public TVR2BBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , VI_AND< TOUTPUT > >
{
public :
    typedef TVR2BBase< IVRLTN< TRID > ,  VI_AND< TINPUTs ... > , VI_AND< TOUTPUT > > base_type ;
    using base_type::base_type ;
    ~TVR2BBase()
    { 
    }
} ;

template< typename TR > 
using TVR2B = TVR2BBase< TR , typename TR::NAME , typename TR::VALUE > ;

 
template< typename TSYS , typename TIN , typename TOU > class TVExtSystemNodeBase ;

template< typename TSYS , typename ... TINs , typename ... TOUs >
class TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< TOUs ...> >
                                   : TVServer< IVTracer >
{
public:
    TVExtSystemNodeBase()
    {
    }
    ~TVExtSystemNodeBase()
    {
        TVServer< IVTracer >::Close() ;
    }
public :
    void AttachSystem( IVSlot< TVRef< IVSysDynamic< TSYS > > > & slotSys )
    {
        slotSys.GetData( VD_P2U( m_proxySysProv ) ) ;

        OnChanged() ;

        TVServer< IVTracer >::Run( [ & slotSys ]( VI< IVTracer > & spcTracer ){
            slotSys.Trace( spcTracer ) ; 
        } );
    }
    void OnChanged()
    { 
        m_proxySysProv.UseServer( [this]( IVDataProv< TVRef< IVSysDynamic< TSYS > > > & cr ){
            cr.Apply( VD_P2U( m_proxySysRef ) ) ;
        }) ;
        m_proxySysRef.UseServer( [this]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){
            _rebuildHub( cr.m_ref.m_ref ) ;
            //cr.m_ref.m_ref.Elapse() ;
        }) ;
    } 

    void Input( IVSlot< TINs > & ... args )
    {
        m_inPort.InputPort( args ... ) ;
    }
 
    void Output( IVInputPort< TOUs > & ... ips )
    {
        m_ouPort.OutputAllPorts( ips ... ) ;
        //TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< TOU > >::TVExtSystemNodeBase::PipeOut< TOU >( ip ) ;
    }
    
    template< typename T >
    void OutputInstant( IVInputPort< T > & ip , IVSlot< TINs > & ... args )
    {
        m_inPort.InputPort( args ... ) ;
#ifdef DEBUG
        bool bTest(false);
        m_proxySysRef.UseServer( [this, &bTest]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){ 
            cr.m_ref.m_ref.Elapse() ;
            bTest = true ;
        }) ;
        VASSERT( bTest ) ;
#else
        m_proxySysRef.UseServer( [this]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){ 
            cr.m_ref.m_ref.Elapse() ;
        }) ;
#endif // DEBUG  

        m_ouPort.OutputPort( ip ) ;
    }

private :
    void _rebuildHub( IVSysDynamic< TSYS > & sys  )
    {
        m_inPort.TravelData( [ this , &sys ]( IVSlot< TINs > & ... slInputs ){
            m_hub.Create( sys , slInputs ... ); 
        } );

        m_ouPort.AttachTo( m_hub );
    }

private :
    TVHub< TSYS > m_hub ;
    TVServiceProxy< IVDataProv< TVRef< IVSysDynamic< TSYS > > > > m_proxySysProv ;
    TVServiceProxy< TVConstRef< TVRef< IVSysDynamic< TSYS > > > > m_proxySysRef ;
    TVVMOutputPort< TINs ... > m_inPort ;
    TVVMOutputPort< TOUs ... > m_ouPort ;
} ;

template< typename TSYS , typename ... TINs >
class TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< void > >
                                   : TVServer< IVTracer >
{
public:
    TVExtSystemNodeBase()
    {
    }
    ~TVExtSystemNodeBase()
    {
        TVServer< IVTracer >::Close() ;
    }
public :
    void AttachSystem( IVSlot< TVRef< IVSysDynamic< TSYS > > > & slotSys )
    {
        slotSys.GetData( VD_P2U( m_proxySysProv ) ) ;

        OnChanged() ;

        TVServer< IVTracer >::Run( [ & slotSys ]( VI< IVTracer > & spcTracer ){
            slotSys.Trace( spcTracer ) ; 
        } );
    }
    void OnChanged()
    { 
        m_proxySysProv.UseServer( [this]( IVDataProv< TVRef< IVSysDynamic< TSYS > > > & cr ){
            cr.Apply( VD_P2U( m_proxySysRef ) ) ;
        }) ;
        m_proxySysRef.UseServer( [this]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){
            _rebuildHub( cr.m_ref.m_ref ) ;
            //cr.m_ref.m_ref.Elapse() ;
        }) ;
    } 
    void Input( IVSlot< TINs > & ... args )
    {
        m_inPort.InputPort( args ... ) ;
    }
    void Output()
    {
        //m_ouPort.OutputAllPorts( ips ... ) ;
        //TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< TOU > >::TVExtSystemNodeBase::PipeOut< TOU >( ip ) ;
    }

private :
    void _rebuildHub( IVSysDynamic< TSYS > & sys  )
    {
        m_inPort.TravelData( [ this , &sys ]( IVSlot< TINs > & ... slInputs ){
            m_hub.Create( sys , slInputs ... ); 
        } ); 
    }

private :
    TVHub< TSYS > m_hub ;
    TVServiceProxy< IVDataProv< TVRef< IVSysDynamic< TSYS > > > > m_proxySysProv ;
    TVServiceProxy< TVConstRef< TVRef< IVSysDynamic< TSYS > > > > m_proxySysRef ;
    TVVMOutputPort< TINs ... > m_inPort ;
} ;

template< typename TSYS , typename ... TOUs >
class TVExtSystemNodeBase< TSYS , VI_AND<> , VI_AND< TOUs ...> >
                                   : TVServer< IVTracer >
{
public:
    TVExtSystemNodeBase()
    {
    }
    ~TVExtSystemNodeBase()
    {
        TVServer< IVTracer >::Close() ;
    }
public :
    void AttachSystem( IVSlot< TVRef< IVSysDynamic< TSYS > > > & slotSys )
    {
        slotSys.GetData( VD_P2U( m_proxySysProv ) ) ;

        OnChanged() ;

        TVServer< IVTracer >::Run( [ & slotSys ]( VI< IVTracer > & spcTracer ){
            slotSys.Trace( spcTracer ) ; 
        } );
    }

    void OnChanged()
    { 
        m_proxySysProv.UseServer( [this]( IVDataProv< TVRef< IVSysDynamic< TSYS > > > & cr ){
            cr.Apply( VD_P2U( m_proxySysRef ) ) ;
        }) ;
        m_proxySysRef.UseServer( [this]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){
            _rebuildHub( cr.m_ref.m_ref ) ; 
        }) ;
    } 

    template< typename T >
    void OutputInstant( IVInputPort< T > & ip )
    {
#ifdef DEBUG
        bool bTest(false);
        m_proxySysRef.UseServer( [this, &bTest]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){ 
            cr.m_ref.m_ref.Elapse() ;
            bTest = true ;
        }) ;
        VASSERT( bTest ) ;
#else
        m_proxySysRef.UseServer( [this]( TVConstRef< TVRef< IVSysDynamic< TSYS > > > & cr ){ 
            cr.m_ref.m_ref.Elapse() ;
        }) ;
#endif // DEBUG  

        m_ouPort.OutputPort( ip ) ;
    }

    void Input()
    {
        m_hub.BatchConnect() ;
    }
 
    void Output( IVInputPort< TOUs ... > & ips )
    {
        m_ouPort.OutputAllPorts( ips ) ;
        //TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< TOU > >::TVExtSystemNodeBase::PipeOut< TOU >( ip ) ;
    }

private :
    void _rebuildHub( IVSysDynamic< TSYS > & sys  )
    {
        m_hub.Create( sys ) ;
        m_ouPort.AttachTo( m_hub );
    }

private :
    TVHub< TSYS > m_hub ;
    TVServiceProxy< IVDataProv< TVRef< IVSysDynamic< TSYS > > > > m_proxySysProv ;
    TVServiceProxy< TVConstRef< TVRef< IVSysDynamic< TSYS > > > > m_proxySysRef ;
    TVVMOutputPort< TOUs ... > m_ouPort ;
} ;

template< typename TSYS , typename ... TINs , typename TOU > 
class TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , TOU >
    : public TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< TOU > >
{
public :
    using TVExtSystemNodeBase< TSYS , VI_AND< TINs ... > , VI_AND< TOU > >::TVExtSystemNodeBase ; 
} ;

template< typename TSYS >
using TVExtSystemNode = TVExtSystemNodeBase< TSYS , typename TSYS::NAME , typename TSYS::VALUE > ;

template< typename TSYS >
class TVExtSystemNodeService
{
public:
    TVExtSystemNodeService( const VSESS< TSYS > & v )
        : m_extsys( v )
    {
        m_extsys.pSysProv->Apply( VD_L2U( [ this ]( VI< IVSysDynamic< TSYS > > &spc ){
            m_clnt.Create( spc , m_sysData );
        } ) );
    }

    void RunService( TVExtSystemNode< TSYS > & dt )
    {
        dt.AttachSystem( m_sysData );
    }
    void Elapse()
    {
        VASSERT( ! m_clnt.IsClosed() ) ; // 如果出现这种现象，考虑添加以下注释内的内容
        //if( m_clnt.IsClosed() )
        //{
        //    m_extsys.pSysProv->Apply( VD_L2U( [ this ]( VI< IVSysDynamic< TSYS > > &spc ){
        //        m_clnt.Create( spc , m_sysData );
        //    } ) );
        //}
        m_sysData.Peer( []( const TVRef< IVSysDynamic< TSYS > >  & rs ){
            rs.m_ref.Elapse();
        } );
    }

    void ApplySystem( VO< IVSysDynamic< TSYS > > & usr )
    {
        m_extsys.pSysProv->Apply( usr ) ;
    }

private:
    class EsClnt
    {
    public:
        EsClnt( IVSysDynamic< TSYS > & sys , TVData< TVRef< IVSysDynamic< TSYS > > > & data )
            : m_srcRef( sys )
        {
            VLNK( data , m_srcRef );
        }
    private:
        TVSource< TVRef< IVSysDynamic< TSYS > > > m_srcRef;
    };

private:
    VSESS< TSYS > m_extsys;
    TVClient< EsClnt , IVSysDynamic< TSYS > > m_clnt;
    TVData< TVRef< IVSysDynamic< TSYS > > > m_sysData;
};

// 将pipelien的输入参数转化为IVConverter模式
template< typename TSYS > 
class TVSvcExtSystemFromArg : public IVProvider< IVSysDynamic< TSYS > >
{
public : 
    TVSvcExtSystemFromArg()
    {}
    ~TVSvcExtSystemFromArg()
    { 
        Close() ;
    }

    void RunService( TVExtSystemNode< TSYS > & dt )
    {
        m_svc.Use( [&dt]( TVExtSystemNodeService< TSYS > & svc ){
            svc.RunService( dt ) ;
        } ) ;
    }
    void Elapse()
    {
        m_svc.Use([]( TVExtSystemNodeService< TSYS > & cs ){
            cs.Elapse();
        } ) ;
    }
    void Close()
    {
        m_svc.Clear();
    } 
    template< typename ... TARGs >
    void Init( const TVPipelineArg< TARGs ... > & args )
    {
        m_svc.Renew( args.Get< VSESS< TSYS > >() ) ; 
    } 

    void Apply( VO< IVSysDynamic< TSYS > > & usr )
    {
        m_svc.Use( [&usr]( TVExtSystemNodeService< TSYS > & res ){
            res.ApplySystem( usr ) ;
        } ) ;
    } 

private :
    TVSafeScopedObj< TVExtSystemNodeService< TSYS > > m_svc ;
} ;

#define VDDP_ARG_IMP( nID , name , sys )  TVSvcExtSystemFromArg< sys > m_sysSvc##name ;\
    VSESS< sys > GetExt##name(){ VSESS< sys > ess ; ess.pSysProv = & m_sysSvc##name ; return ess ; }\
    typedef TVExtSystemNode< sys > name ;\
    void Create##name( TVExtSystemNode< sys > & data ) \
    { m_sysSvc##name.RunService( data ) ; }\
    VD_DECLAREEC_IMP( m_sysSvc##name , nID ) 

#define VDDP_ARG( name , sys ) VDDP_ARG_IMP( VD_EXPAND( __COUNTER__ ) , name , sys ) 

template< typename TSYS , typename TSYSPARENT , typename ... TDEPs > 
class TVSvcExtSystemNode
{
public :
    void Input( )
    {
    }
} ;

template< typename TSYS , typename TSYSPARENT , typename ... TDEPs > class TVSvcExtSystemAdaptor ;

template< typename TSYS , typename TSYSPARENT , typename ... TDEPs > 
class TVSvcExtSystemAdaptor< TSYS , TVExtSystemNode< TSYSPARENT > , TDEPs ... >
                                   : TVServer< IVTracer >
                                   , public IVProvider< IVSysDynamic< TSYS > >
{ 
public :
    TVSvcExtSystemAdaptor()
    {
        TVServer< IVTracer >::Run( [ this ]( VI< IVTracer > & spc ){
            m_dataSys.Trace( spc ) ;
        } ) ;
    }
    ~TVSvcExtSystemAdaptor()
    {
        Close() ;
    }
    
private :
    class EsClnt //: TVServer< TVRef< TVExtSystemNodeService< TSYS > > >
    {
    public:
        EsClnt( TVConstRef< VSESS<TSYS > > & sys , VO< TVRef< TVExtSystemNodeService< TSYS > > > & usr )
            : m_svc( sys.m_ref )
        {
            m_svcRef.Run( usr , m_svc ) ;
            //TVServer< TVRef< TVExtSystemNodeService< TSYS > > >::Run( usr , m_svc ) ;
            //VLNK( data , m_srcRef );
        }
        ~EsClnt()
        {
            m_svcRef.Close() ;
        }
    private:
        TVExtSystemNodeService< TSYS > m_svc ;
        TVService< TVRef< TVExtSystemNodeService< TSYS > > > m_svcRef ;
    };
public : 
    void RunService( TVExtSystemNode< TSYS > & dt )
    {
        m_service.UseServer( [ & dt ]( const TVRef< TVExtSystemNodeService< TSYS > > & cs ){
            cs.m_ref.RunService( dt ) ; 
        } ) ; 
    }
    void Elapse()
    {
        m_service.UseServer( []( TVRef< TVExtSystemNodeService< TSYS > > & cs ){
            cs.m_ref.Elapse() ;
        } ) ; 
    }
    void Close()
    {
        //m_service.Clear() ;
    } 
    void Init( TVExtSystemNode< TSYSPARENT > & cw , IVSlot< TDEPs > & ... para )
    {
        cw.OutputInstant< VSESS< TSYS > >( m_dataSys , para ... ) ; 
    }
    void OnChanged()
    { 
        m_dataSys.GetData( VD_P2U( m_spSysProv ) ) ;
        m_spSysProv.UseServer( [ this ]( IVDataProv< VSESS< TSYS > > & sysProv ){
            _rebuild( sysProv ) ; 
        } ) ; 
    }

    void Apply( VO< IVSysDynamic< TSYS > > & usr )
    {
        m_service.UseServer( [&usr]( TVRef< TVExtSystemNodeService< TSYS > > & res ){
            res.m_ref.ApplySystem( usr ) ;
        } ) ;
    } 
private :
    void _rebuild( IVDataProv< VSESS< TSYS > > & sysProv )
    {
        sysProv.Apply( VD_L2U( [ this ]( VI< TVConstRef< VSESS<TSYS > > > & spc ){
            _rebuild( spc ) ;
        } ) ) ;
    }
    void _rebuild( VI< TVConstRef< VSESS<TSYS > > > & spc )
    {
        m_clnt.Create( spc , VD_P2U( m_service )) ;
    }

private : 
    TVData< VSESS< TSYS > > m_dataSys ;
    TVServiceProxy< IVDataProv< VSESS< TSYS > > > m_spSysProv ;
    TVClient< EsClnt , TVConstRef< VSESS<TSYS > > > m_clnt ;
    TVServiceProxy< TVRef< TVExtSystemNodeService< TSYS > > > m_service ;
} ;

#define VDDP_NODE_EXT_IMP( nID , name , sys , owner ) TVSvcExtSystemAdaptor< sys , owner > m_sysSvc##name ;\
    typedef TVExtSystemNode< sys > name ;\
    owner m_linkage_##owner##_##name ;\
    VSESS< sys > GetExt##name(){ VSESS< sys > ess ; ess.pSysProv = & m_sysSvc##name ; return ess ; }\
    template< typename ... TINs >\
    void Create##name( TVExtSystemNode< sys > & data , TINs & ... inputs )\
    {  m_sysSvc##name.RunService( data ); VLNK( data , inputs ... ) ; }\
    void LoopElapse( const TVHelperCounter< nID > * ) \
    { LoopElapse( reinterpret_cast< TVHelperCounter< nID - 1 > * >( 0 ) ); m_sysSvc##name.Elapse(); } \
    void LoopClose( const TVHelperCounter< nID > * ) \
    { LoopClose( reinterpret_cast< TVHelperCounter< nID - 1 > * >( 0 ) ); m_sysSvc##name.Close(); } \
    template< typename T > void LoopInit( const TVHelperCounter< nID > * , const T & v ) \
    { LoopInit<T>( reinterpret_cast< TVHelperCounter< nID - 1 > * >( 0 ) , v );\
      Create##owner( m_linkage_##owner##_##name ) ;\
       m_sysSvc##name.Init( m_linkage_##owner##_##name ); }

#define VDDP_EXT( name , sys , owner )  VDDP_NODE_EXT_IMP( VD_EXPAND( __COUNTER__ ) , name , sys , owner ) 

template< typename T > struct TVNodeType2DatatType ;
template< typename T > struct TVNodeType2DatatType< TVExtSystemNode< T > >
{
    typedef VSESS< T > type ;
} ;

#define VDDP_NODE_EXT_P_IMP( nID , name , sys , owner , para ) TVSvcExtSystemAdaptor< sys , owner , typename TVNodeType2DatatType< para >::type > m_sysSvc##name ;\
    typedef TVExtSystemNode< sys > name ;\
    owner m_linkage_##owner##_##name ;\
    TVSafeScopedObj< TVSource< typename TVNodeType2DatatType< para >::type > > m_paralink_##name##para ;\
    VSESS< sys > GetExt##name(){ VSESS< sys > ess ; ess.pSysProv = & m_sysSvc##name ; return ess ; }\
    template< typename ... TINs >\
    void Create##name( TVExtSystemNode< sys > & data , TINs & ... inputs )\
    {  m_sysSvc##name.RunService( data ); VLNK( data , inputs ... ) ; }\
    void LoopElapse( const TVHelperCounter< nID > * ) \
    { LoopElapse( reinterpret_cast< TVHelperCounter< nID - 1 > * >( 0 ) ); m_sysSvc##name.Elapse(); } \
    void LoopClose( const TVHelperCounter< nID > * ) \
    { LoopClose( reinterpret_cast< TVHelperCounter< nID - 1 > * >( 0 ) ); m_sysSvc##name.Close(); } \
    template< typename T > void LoopInit( const TVHelperCounter< nID > * , const T & v ) \
    { LoopInit<T>( reinterpret_cast< TVHelperCounter< nID - 1 > * >( 0 ) , v );\
      Create##owner( m_linkage_##owner##_##name ) ;\
       m_paralink_##name##para.Renew( GetExt##para() );\
       m_paralink_##name##para.Use([this]( TVSource< typename TVNodeType2DatatType< para >::type > & src ){\
        m_sysSvc##name.Init( m_linkage_##owner##_##name , src ); } ) ; \
    }

#define VDDP_EXT_P( name , sys , owner , para )  VDDP_NODE_EXT_P_IMP( VD_EXPAND( __COUNTER__ ) , name , sys , owner , para ) 

template< typename TB , typename TIN , typename TOU > struct VBuilderTraitFromBulderL2Base ;

template< typename TB , typename ... TINs , typename VROUTPUT > 
struct VBuilderTraitFromBulderL2Base< TB , VI_AND< TINs ... > , VSESS< VROUTPUT > > 
{
    typedef TVDynamicSystemBuilder< TVRSimpSysTrait< VROUTPUT , TB , TINs ... >
                           , typename TVRSimpSysTrait< VROUTPUT , TB , TINs ... >::SYSTYPE > BuilderType ;
};

template< typename TR >
struct VBuilderTraitFromBulderL2
{
    template< typename TB >
    struct TVPass
    {
        typedef typename VBuilderTraitFromBulderL2Base< TB , typename TR::NAME , typename TR::VALUE >::BuilderType BuilderType ;
    } ;
} ;

#define VD_EXPORT_SYSTEM_SIMPLE_L2( builder , builderCreatorRelation ) VD_EXPORT_SYSTEM_SIMPLE( \
             VBuilderTraitFromBulderL2< builderCreatorRelation >::template TVPass< builder >::BuilderType \
            , builderCreatorRelation ) ;


template< typename ... Ts > class VIID_ENTRY{} ;

template< typename ... Ts > 
struct IVRLTN< VIID_ENTRY< Ts ... > >  
{
    typedef TVRelationDesc< VIID_ENTRY< Ts ... > , VI_AND<> , VI_AND< Ts ... > > DESC;
    typedef typename DESC::ID                                      ID;
    typedef typename DESC::NAME                                    NAME;
    typedef typename DESC::VALUE                                   VALUE;
    typedef typename DESC::HUB                                     HUB;
} ; 

template< typename ... Ts >
using VREntry = IVRLTN< VIID_ENTRY< Ts ... > > ;



template< typename TSYS , typename TOU > 
class TVUnpackedSysNodeImp : public TVUnpackedSysNodeImp< TSYS , VI_AND< TOU > >
{
public :
    using TVUnpackedSysNodeImp< TSYS , VI_AND< TOU > >::TVUnpackedSysNodeImp ; 
} ;

template< typename TSYS , typename TD >
class TVUnpackedSysNodeImp< TSYS , VI_AND< TD > > : IVSlot< TD >
                                                  , TVServer< IVTracer >
{
public: 
    TVUnpackedSysNodeImp( )
    { 
    }
    ~TVUnpackedSysNodeImp()
    {
        m_svcProv.Close();
        TVServer< IVTracer >::Close();
        m_svrElapse.Close() ;
    }
public:
    template< typename F >
    void ListernElapse( F f )
    {
        m_svrElapse.Run( f , *this ) ;
    }
    void Output( IVInputPort< TD > & ip )
    {
        ip.Input( *this );
    }
    void OnChanged()
    {
        m_listerner.TidyTravel( []( IVTracer & t ){
            t.OnChanged();
        } );
    }

private:
    class DataProv : public IVDataProv< TD >
    {
    public:
        DataProv( TVUnpackedSysNodeImp & sys )
            : m_sys( sys ) {}
        ~DataProv()
        {
        }
    public:
        void Apply( VO< TVConstRef< TD > > & usr )
        {
            m_sys.queryData( usr );
        }

    private:
        TVUnpackedSysNodeImp & m_sys;
    };
    class VTraceElapse : public IVTracer
    {
    public :
        VTraceElapse( TVUnpackedSysNodeImp & sys)
            : m_sys( sys ) {}
        ~VTraceElapse()
        {
        }
    public :
        void OnChanged()
        {
            m_sys.onElapse() ;
        }
    private:
        TVUnpackedSysNodeImp & m_sys;
    } ;
public:
    void Input( IVSlot< VSESS< VREntry< TD > > > & slt )
    {
        TVServer< IVTracer >::Run( [ &slt ]( VI<IVTracer>& spc ){
            slt.Trace( spc );
        } );
        slt.GetData( VD_P2U( m_proxyProv ) );
    }
    virtual void Trace( VI< IVTracer > & spc )
    {
        m_listerner.Add( spc );
    }
    virtual void GetData( VO< IVDataProv< TD > > &usr )
    {
        m_svcProv.RunNew( usr , *this );
    }

public:
    void queryData( VO< TVConstRef< TD > > & usr )
    {
        if( m_proxyRef.IsClosed() )
        {
            m_proxyProv.UseServer( [ this ]( IVDataProv< VSESS< VREntry< TD > > > & dp ){
                dp.Apply( VD_P2U( m_proxyRef ) );
            } );
        }

        if( m_proxyToothProv.IsClosed() )
        {
            m_proxyRef.UseServer( [ this ]( TVConstRef< VSESS< VREntry< TD > > > & dp ){
                dp.m_ref.pSysProv->Apply( VD_P2U( m_proxyToothProv ) );
            } );
        }

        if( m_proxyDataProv.IsClosed() )
        {
            m_proxyToothProv.UseServer( [ this ]( IVSysDynamic< VREntry< TD >  > & sys ){
                m_hub.Create( sys );
                m_hub.Output( VD_L2IP( [ this ]( IVSlot< TD > & slot ){
                    slot.GetData( VD_P2U( m_proxyDataProv ) );
                } ) );
                sys.Elapse();
            } );
        }

        VASSERT( !m_proxyDataProv.IsClosed() );
        m_proxyDataProv.UseServer( [ &usr ]( IVDataProv< TD > & dp ){
            dp.Apply( usr );
        } );
    }
    void onElapse()
    {
        if( m_proxyRef.IsClosed() )
        {
            m_proxyProv.UseServer( [ this ]( IVDataProv< VSESS< VREntry< TD > > > & dp ){
                dp.Apply( VD_P2U( m_proxyRef ) );
            } ); 

            m_proxyRef.UseServer( [ this ]( TVConstRef< VSESS< VREntry< TD > > > & dp ){
                dp.m_ref.pSysProv->Apply( VD_P2U( m_proxyToothProv ) );
            } );

            m_proxyToothProv.UseServer( [ this ]( IVSysDynamic< VREntry< TD >  > & sys ){
                m_hub.Create( sys );
                m_hub.Output( VD_L2IP( [ this ]( IVSlot< TD > & slot ){
                    slot.GetData( VD_P2U( m_proxyDataProv ) );
                } ) );
            } );
        }

        m_proxyToothProv.UseServer( [ this ]( IVSysDynamic< VREntry< TD >  > & sys ){     
                sys.Elapse();
        } ) ;
    }

private:
    TVServiceProxy< IVDataProv< VSESS< VREntry< TD > > > > m_proxyProv;
    TVServiceProxy< TVConstRef< VSESS< VREntry< TD > > > > m_proxyRef;
    TVServiceProxy< IVSysDynamic< VREntry< TD >  > > m_proxyToothProv;
    TVHub< VREntry< TD > > m_hub;
    TVServiceProxy< IVDataProv< TD > > m_proxyDataProv;
    TVServiceArray< IVDataProv< TD > , DataProv > m_svcProv;
    TVServiceProxyArray< IVTracer > m_listerner;
    TVService< IVTracer , VTraceElapse > m_svrElapse ;
} ;

template< typename TExp >
using TVUnpackedSysNode = TVUnpackedSysNodeImp< TExp , typename TExp::VALUE > ;

template< typename TSYS > 
class TVSvcUnpackedSystem
{
public : 
    TVSvcUnpackedSystem()
    {}
    ~TVSvcUnpackedSystem()
    { 
        Close() ;
    }

    void RunService( TVUnpackedSysNode< TSYS > & dt )
    {
        dt.ListernElapse( [ this ]( VI< IVTracer >& spc ){
            m_elapseNodifier.Add( spc ) ;
        } ) ; 
    }
    void Elapse()
    {
        m_elapseNodifier.TidyTravel([]( IVTracer & t ){
            t.OnChanged() ;
        }) ; 
    }
    void Close()
    { 
    } 
    template< typename ... TARGs >
    void Init( const TVPipelineArg< TARGs ... > & args )
    {
    }  
private:
    TVServiceProxyArray< IVTracer > m_elapseNodifier ;
} ;

#define VDDP_UNPCK_IMP( nID , name , ... )  TVSvcUnpackedSystem< VREntry< __VA_ARGS__ > > m_sysSvc##name ;\
    typedef TVUnpackedSysNode< VREntry< __VA_ARGS__ > > name ;\
    void Create##name( TVUnpackedSysNode< VREntry< __VA_ARGS__ > > & data ) \
    { m_sysSvc##name.RunService( data ) ; }\
    VD_DECLAREEC_IMP( m_sysSvc##name , nID ) 

#define VDDP_UNPCK( name , ... ) VDDP_UNPCK_IMP( VD_EXPAND( __COUNTER__ ) , name , __VA_ARGS__ ) 
