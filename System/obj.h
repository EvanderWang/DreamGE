#pragma once
#include <type_traits>
#define VPUREINTERFACE  struct __declspec(novtable)
//#define VINTERFACE struct

//#ifdef _WIN32
//#define MV_STDCALL     __stdcall
//#define MV_EXPORT_API  __declspec(dllexport)
//#define MV_IMPORT_API  __declspec(dllimport)
//#else
//#define MV_STDCALL
//#define MV_EXPORT_API
//#define MV_IMPORT_API
//#endif

#define VD_DECLARE_NO_COPY_CLASS(classname)         \
        private:                                    \
            classname(const classname&);            \
            classname& operator=(const classname&)

#define VD_DECLARE_NO_COPY_TEMPLATE_CLASS(classname, ...)       \
        private:                                                \
            classname(const classname<__VA_ARGS__>&);           \
            classname& operator=(const classname<__VA_ARGS__>&)

//#define VD_DECLAREOBJECT( className )     VINTERFACE className : public IVObject
//#define VD_DECLAREINTERFACE( interfName ) VPUREINTERFACE interfName

//#define VD_ARG(...) __VA_ARGS__  

// External ralation to IVComponent::Participate
VPUREINTERFACE IVAgent
{
    virtual void Present() = 0 ;
} ;

VPUREINTERFACE IVComponent
{
    virtual void Participate() = 0 ; 
} ;


template< typename ID >
VPUREINTERFACE IVDelegate : IVAgent
{
} ;
  
template< typename ID >
VPUREINTERFACE IVDelegateArr : IVAgent
{
} ;

template< typename ID , unsigned COUNT >
VPUREINTERFACE IVDelegateSet : IVAgent
{
} ;

template< typename ID >
VPUREINTERFACE IVElement : IVComponent
{
} ;


template< typename TID  >  
using IVClassifiable = IVElement< TID > ;
 
template< class T >
struct TVDataTrait
{  
    typedef typename std::conditional< std::is_scalar< T >::value, T, typename std::conditional< std::is_abstract< T >::value, T &, const T & >::type >::type reference; 
} ;    
 
template<>
struct TVDataTrait< void >
{
    typedef void reference ;
} ;

#define VD_CHR( x, y ) x##y 
#define VD_DEC( n ) DEC_##n 
#define VD_REPEAT_1( n, f, e ) VD_CHR( e, n )
#define VD_REPEAT_2( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) )
#define VD_REPEAT_3( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 
#define VD_REPEAT_4( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 
#define VD_REPEAT_5( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 
#define VD_REPEAT_6( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 
#define VD_REPEAT_7( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 
#define VD_REPEAT_8( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 
#define VD_REPEAT_9( n, f, e ) VD_CHR( VD_CHR( REPEAT_, VD_DEC( n ) )( VD_DEC( n ), f, e ), f( n ) ) 

#define VD_TEMPLATE_CLASSTYPE( n ) ,class T##n
#define VD_TEMPLATE_CLASSTYPE_END class T 

#define VD_TEMPLATE_TYPE( n ) ,T##n
#define VD_TEMPLATE_TYPE_END T 

#define VD_DEF_TEMPLATE_CLASSTYPE( n ) VD_REPEAT_##n( n, VD_TEMPLATE_CLASSTYPE, VD_TEMPLATE_CLASSTYPE_END )
#define VD_DEF_TEMPLATE_TYPE( n ) VD_REPEAT_##n( n, VD_TEMPLATE_TYPE, VD_TEMPLATE_TYPE_END )

#define VD_DECLARE_TEMPLATE_CLASS_REF( TemplateClassName , TemplateCount )      \
template<VD_DEF_TEMPLATE_CLASSTYPE(TemplateCount)>                              \
struct TVDataTrait< TemplateClassName<VD_DEF_TEMPLATE_TYPE(TemplateCount)> >    \
{                                                                               \
    typedef TemplateClassName<VD_DEF_TEMPLATE_TYPE(TemplateCount)> & reference ;\
} ;

template< class T = void , class TRef = typename TVDataTrait< T >::reference >
VPUREINTERFACE IVUser
{
    virtual void Visit( TRef t ) = 0 ;
} ;   

template<>
VPUREINTERFACE IVUser< void >
{
    virtual void Visit() = 0 ;
};     
   
template< class T = void , class TRef = typename TVDataTrait< T >::reference >
VPUREINTERFACE IVOwner
{
    virtual void Use( IVUser< T , TRef > & user ) = 0 ;
} ;  

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// 不满足标准使用方式的结构体
// 此结构体必须需要使用提供的模板类库来使用，不应直接访问此结构体
template< class T > VPUREINTERFACE IVSpace
{ 
    typedef void    FreeFuncType ( IVSpace< T > & spc ) ;

    T             * pService     ; 
    IVSpace< T  > * pPreRef      ;
    IVSpace< T  > * pNxtRef      ;
    FreeFuncType  * procFreeClnt ;
    FreeFuncType  * procFreeSrvc ;

    IVSpace(){}
    ~IVSpace(){} 

    VD_DECLARE_NO_COPY_TEMPLATE_CLASS( IVSpace , T ) ;
} ;  
VD_DECLARE_TEMPLATE_CLASS_REF( IVSpace , 1 )

template< class T >
using VI = IVSpace< T > ;

template< class T >
using VO = IVUser< IVSpace< T > > ;