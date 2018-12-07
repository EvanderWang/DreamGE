// 一些常用的模式

#pragma once

#include "obj.h"
#include "mem.h"

#define VD_DEFELEMENT( id )  class id  
#define VD_EXPAND(...) __VA_ARGS__ 

template< class T > 
struct IVRecipient
{
    virtual void Offer( VI< T > & ) = 0 ;
} ;

template< class T >
struct IVProvider
{
    virtual void Apply( VO< T > & ) = 0 ;
} ;


/*
template< class TCookie , class TObj >
struct IVPersistCreator
{
    virtual void Create( IVProvider< TCookie > & prov , VO<  TObj  >& ) = 0 ;
} ;

template< class Initilizer , class CookieType >
struct IVCookieStorage
{
    virtual void Alloc( Initilizer & initializer , VO<  CookieType  >& cookie ) = 0 ; 
} ;

template< class T >
VD_DECLAREINTERFACE( IVDifference )
{ 
    virtual void Review() = 0 ;
} ;

template< class T = void >
struct IVDynTracer
{
    virtual void OnChange( IVDifference<T> & diff ) = 0 ;
} ; 

template<>
struct IVDynTracer< void >
{
    virtual void OnChange() = 0 ;
} ; 

template< class TData , class TBase = void >
struct IVDynamicOld : public TBase
{
    virtual void SnapShot( IVUser< TData > & usr )     = 0 ;
    virtual void Trace( VI<  IVDynTracer< TBase >  >& ) = 0 ;
} ;

template< class TData > 
struct IVDynamicOld< TData , void >
{
    virtual void SnapShot( IVUser< TData > & usr    ) = 0 ;
    virtual void Trace( VI<  IVDynTracer< void >  >& ) = 0 ;
} ;

template< class TData , class TBase = void >
struct IVDynamicValue : public TBase
{
    virtual void SnapShot( IVUser< TData > & usr   ) = 0 ;
    virtual void Trace( VI<  IVDynTracer< TBase >  >&    ) = 0 ;
} ;

template< class TData >
struct IVDynamicValue< TData , void >
{
    virtual void SnapShot( IVUser< TData > & usr ) = 0 ;
    virtual void Trace( VI<  IVDynTracer<>  >&         ) = 0 ;
} ;
 
///////////////////////////  new architecture ///////////////////////////////////////// 
*/
#define IVMemory IVElement 

template< class CID >
struct IVRecallable
{
    typedef IVAgent IVPast ; // External ralaiton to IVMemory< CID >
    virtual void TakeMemory( VO<  IVMemory< CID >  >& ) = 0 ;
} ;

template< class DiffViewer , class CID >
struct IVComparable : IVRecallable< CID >
{ 
    virtual void Compare( typename IVRecallable< CID >::IVPast & eo  , DiffViewer & dr   ) = 0 ;
} ; 

struct IVTracer
{
    virtual void OnChanged() = 0 ;
} ;

template< class T , bool bV = std::is_scalar< T >::value > struct IVDynamicNew ;

template< class T > 
struct IVDynamicNew< T , true >
{
    virtual void Trace     ( VI<  IVTracer  >& ) = 0 ;  
    virtual T    TakeMemory() = 0 ; 
} ;

template< class T > 
struct IVDynamicNew< T , false >
{
    virtual void Trace ( VI<  IVTracer  >& ) = 0 ;  
    virtual void TakeMemory ( VO<  T  >&  ) = 0 ; 
} ; 
 
template< class T >
struct IVExtData : IVProvider< IVDynamicNew< T > > // 以后应放弃
{
} ;

template< class TID >
struct IVExtElement : IVExtData< IVDelegate< TID > > // 以后应放弃
{
} ;

template< class TID >
struct IVExtElementArr : IVExtData< IVDelegateArr< TID > > // 以后应放弃
{
} ; 

template< class T >
using IVExtData2 = IVExtData< TVBind< T > > ;

//template< class TID >
//struct IVExtElement2 : IVExtData2< IVElement< TID > > 
//{
//} ;
//
//template < class TSys >
//using IVExtSystem = IVExtData2< IVSystemBase< TSys > > ;
// 
//template< class TSys >
//struct IVExtDynSystem : IVExtSystem< TSys > // IVExtData2< T >
//{
//    virtual void Elapse() = 0 ;
//} ;

#define _VExtIn(...) IVExtData2< __VA_ARGS__ > & 
#define _VExtOut(...) IVUser< IVSpace< IVDynamicNew< TVBind< __VA_ARGS__ > > > > &

template< class VID >
struct IVSysContainer
{
    virtual void Import( IVExtElement< VID > & ) = 0 ;
};

template< class VID >
struct IVSysVector
{
    VD_DEFELEMENT(VIDContainer);
    virtual void Import( IVExtData2< IVSysContainer< VID > > & , VO< IVElement< VIDContainer > >& ) = 0;
}; 

#define _VElementIn( ... ) IVExtElement< __VA_ARGS__  > &  
#define _VElementOut( ... ) IVUser< IVSpace< IVElement< __VA_ARGS__ > > > & 

/////////////////////////////////////////////////////////////////////
// 以下两个接口未来将不再使用
/////////////////////////////////////////////////////////////////////
template< class TData >
struct IVDynamic
{
    virtual void Watch ( IVUser< TData > & viewer          ) = 0 ;
    virtual void Trace ( VI<  IVUser< TData >  >& spcTracer ) = 0 ;  
} ;

template<>
struct IVDynamic< void >
{
    virtual void Trace ( VI<  IVUser< void >  >& ) = 0 ;
} ;

//
//template< class TBase >
//struct IVDynSystem : TBase 
//{
//    virtual void Elapse() = 0 ;
//} ;
//
template< class TSys >
struct IVDynSystem : TSys
{
    virtual void Elapse() = 0 ;
} ;

template< class TData >
struct IVDynComponent : IVDynamic< TData > , IVComponent
{
} ;

VD_DEFELEMENT( VIDUnknown ) ;
  
struct VSDynVar
{
    VSDynVar() : _flag( true ) {}
    bool          _flag ;
    void Signal() { _flag = !_flag ; } 
} ;

struct VSDynMon
{
    VSDynMon() : _flag( true ) {}
    bool              _flag ; 

    bool Sync( const VSDynVar & e )
    {
        bool bRtn = ( _flag != e._flag ) ;
        _flag = e._flag ;
        return bRtn ;
    }

    bool Cmp( const VSDynVar & e ) // true : 同步， false：不同步
    {
        return ( _flag == e._flag ) ;
    }
} ;

////////////////////////////////////////////////////////////////////////////////////////
// New struct
////////////////////////////////////////////////////////////////////////////////////////
template< typename ... TInputs > struct VI_AND   ; 
template< typename ... TInputs > struct VI_OR    ;
template< typename           T > struct VI_ARR   ;
template< typename           T > struct VI_ARR_01;

#define VD_LATEST_PARA0(               )
#define VD_LATEST_PARA1( elem1         ) elem1
#define VD_LATEST_PARA2( elem1 , elem2 ) elem2
#define VD_CHOOSER_LATEST_PARA_ARGS(_f1, _f2, _f3, ...) _f3
#define VD_RECOMPOSER_LATEST_PARA(argsWithParentheses) VD_CHOOSER_LATEST_PARA_ARGS argsWithParentheses
#define VD_CHOOSER_LATEST_PARA_ARGS_FROM_COUNDEF(...) VD_RECOMPOSER_LATEST_PARA((__VA_ARGS__, VD_LATEST_PARA2, VD_LATEST_PARA1, ))
#define VD_NO_ARG_EXPAND_LATEST_PARA() ,,VD_LATEST_PARA0
#define VD_MACRO_CHOOSER_LATEST_PARA(...) VD_CHOOSER_LATEST_PARA_ARGS_FROM_COUNDEF(VD_NO_ARG_EXPANDER_DEFCOMP __VA_ARGS__ ()) 
#define VD_LATEST_PARA(...) VD_MACRO_CHOOSER_LATEST_PARA(__VA_ARGS__)(__VA_ARGS__)

#define VD_FIRST_PARA0(               )
#define VD_FIRST_PARA1( elem1         ) elem1
#define VD_FIRST_PARA2( elem1 , elem2 ) elem1
#define VD_CHOOSER_FIRST_PARA_ARGS(_f1, _f2, _f3, ...) _f3
#define VD_RECOMPOSER_FIRST_PARA(argsWithParentheses) VD_CHOOSER_FIRST_PARA_ARGS argsWithParentheses
#define VD_CHOOSER_FIRST_PARA_ARGS_FROM_COUNDEF(...) VD_RECOMPOSER_FIRST_PARA((__VA_ARGS__, VD_FIRST_PARA2, VD_FIRST_PARA1, ))
#define VD_NO_ARG_EXPAND_FIRST_PARA() ,,VD_FIRST_PARA0
#define VD_MACRO_CHOOSER_FIRST_PARA(...) VD_CHOOSER_FIRST_PARA_ARGS_FROM_COUNDEF(VD_NO_ARG_EXPANDER_DEFCOMP __VA_ARGS__ ()) 
#define VD_FIRST_PARA(...) VD_MACRO_CHOOSER_FIRST_PARA(__VA_ARGS__)(__VA_ARGS__)
 
template< class T >
using IVDataProv = IVProvider< TVConstRef< T > > ;

// Interface of relation   
template< typename TID > struct IVRLTN ;

enum
{
    VE_BR_CHNGED   ,
    VE_BR_BLOCKED  ,
    VE_BR_UNCHNGED ,
} ;

// Dynamic slot 
template< typename T > struct IVSlot
{
    virtual void Trace   ( VI< IVTracer        > & ) = 0 ;  
    virtual void GetData ( VO< IVDataProv< T > > & ) = 0 ; 
} ;

template< typename TID > struct IVSlot< IVRLTN< TID > >
{ 
    virtual void Present() = 0 ;
} ;  
     
// Port
template< typename ... TINPUTs > struct IVInputPort 
{
    virtual void Input ( IVSlot< TINPUTs > & ... ) = 0 ; 
} ;

template< typename TOUTPUT > struct IVOutputPort
{
    virtual void Output( IVInputPort< TOUTPUT > & ) = 0 ;
} ; 

template< typename ... TOUTPUTs > struct IVOutputPort< VI_AND< TOUTPUTs ... > >
{
    virtual void Output( IVInputPort< TOUTPUTs ... > &  ) = 0 ;
} ; 

// Import interface of hub  
template< typename ... TINPUTs > struct IVRHub ;
template< > struct IVRHub< > {} ; 
template< typename TI , typename ... TOTHERs >
struct IVRHub< TI , TOTHERs ... > : IVRHub< TOTHERs ... >
{
    virtual void Connect( IVSlot< TI > & ) = 0;
} ;  

template< typename TOUTPUT > struct IVRExp
{
    virtual void Output( IVInputPort< TOUTPUT > & ) = 0 ;
} ; 
template< typename TID > struct IVRExp< IVRLTN< TID > >
{
    virtual void Create( VO< typename IVRLTN< TID >::HUB > & sys ) = 0 ;
} ;
template<> struct IVRExp< void >
{
} ; 
template<> struct IVRExp< VI_AND<> >
{
} ; 
template< typename TOU  , typename ... TOUTPUTs > struct IVRExp< VI_AND< TOU , TOUTPUTs ... > > : IVRExp< TOU > , IVRExp< VI_AND< TOUTPUTs ... > >
{ 
} ;  

template< typename TID , typename TOU , typename ... TINPUTs > 
struct IVRUniqHub : IVClassifiable< TID >
                  , IVRHub< TINPUTs ... >  
                  , IVRExp< TOU >
{ 
    typedef IVRExp< TOU > EXP_TYPE ;
} ;   
 

template< typename ... TRs > struct IVSysDynamicBase  ;
template<> struct IVSysDynamicBase<> {} ;
 
template< typename TR , typename ... TROTHERs > struct IVSysDynamicBase< TR , TROTHERs...> : IVSysDynamicBase< TROTHERs ... >
{
    virtual void Create ( VO< typename TR::HUB > & sys ) = 0 ;
} ;

// Declaration of dynamic system
template< typename ... TRs > struct IVSysDynamic ;

template< typename TRFst , typename ... TRs >
struct IVSysDynamic< TRFst , TRs ... > : public IVSysDynamicBase< TRFst , TRs ... >
{
    typedef TRFst FIRST_RELATION ;
    virtual void Elapse() = 0 ; 
} ;
 
template< typename ... TRs >
using IVSYS = IVSysDynamic< TRs ... > ; 

template< typename ... TRs >
struct VSExtSystemSvr
{
    typedef IVSysDynamic< TRs ... > SYSTYPE ;
    IVProvider< IVSysDynamic< TRs ... > > * pSysProv ;
} ;
template< typename ... TRs >
using VSESS = VSExtSystemSvr< TRs ... > ; 

template< typename TSYS > struct TVSystemToExternal ;
template< typename ... TRs > struct TVSystemToExternal< IVSysDynamic< TRs ... > >
{
    typedef VSESS< TRs ... > type ;
};
template< typename TSYS > struct TVExternalToSystem ;
template< typename ... TRs > struct TVExternalToSystem< VSESS< TRs ... > >
{
    typedef IVSysDynamic< TRs ... > type ;
};

template< typename TS > struct VSExtSystemLiberary ;

template< typename ... TRs >
struct VSExtSystemLiberary< IVSysDynamic< TRs ... > >
{
    IVProvider< IVSysDynamic< TRs ... > > * pSysLib ;
}; 
 
template< typename TS > struct VSExtSystem ;

template< typename ... TRs >
struct VSExtSystem< IVSysDynamic< TRs ... > >
{
    IVSysDynamic< TRs ... > * pSysLib ;
}; 

// Relation Description
//template< typename T > class VIID_ArrayItem;
template< typename TID , typename TI , typename TOU = void > struct TVRelationDesc
{ 
    using ID    = TID                  ;
    using NAME  = VI_AND< TI >         ;
    using VALUE = TOU                  ;
    using HUB   = IVRUniqHub < TID , TOU , TI >  ; 
} ;
template< typename TID , typename ... TF , typename TOU > struct TVRelationDesc< TID , VI_AND< TF ... > , TOU >
{  
    using ID    = TID                     ;
    using NAME  = VI_AND< TF ... >        ;
    using VALUE = TOU                     ;
    using HUB   = IVRUniqHub < TID , TOU , TF ... > ;  
} ;
template< typename TID , typename ... TF , typename TOU > struct TVRelationDesc< TID , VI_OR< TF ... > , TOU >
{  
    using ID    = TID                     ;
    using NAME  = VI_OR< TF ... >         ;
    using VALUE = TOU                     ;
    using HUB   = IVRUniqHub < TID , TOU , TF ... > ;  
} ;
template< typename TID , typename T , typename TOU > struct TVRelationDesc< TID , T[] , TOU >
{ 
    using ID    = TID                           ;
    using NAME  = VI_ARR< T >                   ;
    using VALUE = TOU                           ;
    using HUB   = IVRUniqHub < TID , TOU , T >  ;
} ; 
//template< typename TID , typename T , typename TOU , unsigned N = 1 > struct TVRelationDesc< TID , T[ N ] , TOU >
//{
//    using ID = TID;
//    using NAME = 
//};
template< typename TID , typename T , typename TOU > struct TVRelationDesc< TID , T[ 1 ] , TOU >
{
    using ID    = TID                           ;
    using NAME  = VI_ARR_01< T >                ;
    using VALUE = TOU                           ;
    using HUB   = IVRUniqHub < TID , TOU , T >  ;
};

template< typename TID , typename TITEMID , typename TOU > struct TVRelationDesc< TID , IVRLTN< TITEMID >[] , TOU >
{ 
    using ID    = TID                                                    ;
    using NAME  = VI_ARR< IVRLTN< TITEMID > >                            ;
    using VALUE = VI_AND< IVRLTN< TITEMID > , TOU >                      ;
    using HUB   = IVRUniqHub < TID , VI_AND< IVRLTN< TITEMID > , TOU > > ; 
} ;

template< typename TID , typename TITEMID , typename TOU > struct TVRelationDesc< TID , IVRLTN< TITEMID >[ 1 ] , TOU >
{
    using ID    = TID                                                   ;
    using NAME  = VI_ARR_01< IVRLTN< TITEMID > >                        ;
    using VALUE = VI_AND< IVRLTN< TITEMID > , TOU >                     ;
    using HUB   = IVRUniqHub < TID , VI_AND< IVRLTN< TITEMID > , TOU > >;
};


// Relation
//template< typename T >
//struct IVRLTN< VIID_ArrayItem< T > >
//{
//    typedef TVRelationDesc< VIID_ArrayItem< T > , T >   DESC  ; 
//    typedef typename DESC::ID                           ID    ; 
//    typedef typename DESC::NAME                         NAME  ; 
//    typedef typename DESC::VALUE                        VALUE ; 
//    typedef typename DESC::HUB                          HUB   ;  
//};
//
//template< typename T >
//using IVRArrayItem = IVRLTN< VIID_ArrayItem< T > > ;  

// Macro of defining relation
#define VD_DEFRELATION( relation_name , ... )  class VIID_##relation_name ; \
    template<> struct IVRLTN< VIID_##relation_name > \
    {\
        typedef TVRelationDesc< VIID_##relation_name , __VA_ARGS__ >   DESC  ;\
        typedef typename DESC::ID                                      ID    ;\
        typedef typename DESC::NAME                                    NAME  ;\
        typedef typename DESC::VALUE                                   VALUE ;\
        typedef typename DESC::HUB                                     HUB   ;\
    };\
    using relation_name = IVRLTN< VIID_##relation_name >

#define VD_DEFRELATION_NS( ns_name , relation_name , ... )  class VIID_##relation_name ; \
    struct IVRLTN_##relation_name \
    {\
        typedef TVRelationDesc< VIID_##relation_name , __VA_ARGS__ >   DESC  ;\
        typedef typename DESC::ID                                      ID    ;\
        typedef typename DESC::NAME                                    NAME  ;\
        typedef typename DESC::VALUE                                   VALUE ;\
        typedef typename DESC::HUB                                     HUB   ;\
    };};\
    template<> struct IVRLTN< ns_name::VIID_##relation_name > : ns_name::IVRLTN_##relation_name {} ;\
    namespace ns_name { using relation_name = IVRLTN< VIID_##relation_name >

#define VD_TNAME( template_name ) TN_##template_name
#define VD_DEFRELATION_TEMPLATE_NS( ns_name , relation_name , template_name , ... )  template< typename VD_TNAME( template_name ) > class VIID_##relation_name ; \
    template< typename VD_TNAME( template_name ) > \
    struct IVRLTN_##relation_name \
    {\
        typedef TVRelationDesc< VIID_##relation_name< VD_TNAME( template_name ) > , __VA_ARGS__ >   DESC  ;\
        typedef typename DESC::ID                                      ID    ;\
        typedef typename DESC::NAME                                    NAME  ;\
        typedef typename DESC::VALUE                                   VALUE ;\
        typedef typename DESC::HUB                                     HUB   ;\
    };};\
    template< typename VD_TNAME( template_name ) > struct IVRLTN< ns_name::VIID_##relation_name< VD_TNAME( template_name ) > > : ns_name::IVRLTN_##relation_name< VD_TNAME( template_name ) > {} ;\
    namespace ns_name { template< typename VD_TNAME( template_name ) > using relation_name = IVRLTN< VIID_##relation_name< VD_TNAME( template_name ) > >