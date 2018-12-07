#pragma once

#include "obj.h"
#include "pttrn.h"
#include "mem.h"
#include <ostream>
#include <istream>
#include <vector> 
#include <fstream>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// persist 
////////////////////////////////////////////////////////////////////////////////////////////////////////////

template< typename T , bool BEDISPERSE > struct VBMarshalling ;
template< typename T > struct VBMarshalling< T , false >
{
    template< typename TP > static void SaveExtData( TP &  , const T & ){}
    template< typename CP > static void CorrectPtr( CP & , unsigned & posEnd , const VSConstBuffer< char > & , unsigned , unsigned curPos ) { posEnd = curPos ; }
};
template< typename T > struct VBMarshalling< T , true >
{
    template< typename TP > static void SaveExtData( TP & mapper , const T & v )
    {
        T::MapTo( mapper , v ) ;
    }
    template< typename CP > static void CorrectPtr( CP & r , unsigned & posEnd , const VSConstBuffer< char > & pbuff , unsigned objPos , unsigned curPos )
    {
        T::CorrectPtr( r , posEnd , pbuff , objPos , curPos ) ;
    }
};
template< typename T > struct VBMarshalling< const T * , true >
{ 
    template< typename TP >
    static void SaveExtData( TP & r , const T * const & v ) 
    {
        r.Append( VSConstBuffer< char >( sizeof( T ) , reinterpret_cast< const char * >( v ) ) ) ; 
        VBMarshalling< T , TV_HasExternalMember< T , TP >::value >::SaveExtData( r , *v ) ;
    }

    template< typename CP > static void CorrectPtr( CP & r , unsigned & posEnd , const VSConstBuffer< char > & pbuff , unsigned objPos , unsigned curPos )
    {
        const T * const * pptr  = reinterpret_cast< const T * const * >( pbuff.pData + objPos ) ;
        const T * pval          = reinterpret_cast< const T * >( pbuff.pData + curPos ) ;
       
        r.Modify( objPos , VSConstBuffer< char >( sizeof( const T * ) , reinterpret_cast< const char * >( &pval ) ) ) ;
    
        posEnd = curPos + sizeof( T ) ;

        if( TV_HasPtrMember< T , CP >::value )
        { 
            VBMarshalling< T , TV_HasPtrMember< T , CP >::value >::CorrectPtr( r , posEnd , pbuff , curPos , posEnd ) ;
        } 
    }
}; 
template< typename T > struct VBMarshalling< VSConstBuffer< T > , true >
{ 
    template< typename TP >
    static void SaveExtData( TP & mapper , const VSConstBuffer< T > & v ) 
    { 
        if( v.len > 0 )
        {
            mapper.Append( VSConstBuffer< char >( v.len * sizeof( T ) , reinterpret_cast< const char * >( v.pData ) ) );
            if( TV_HasExternalMember< T , TP >::value )
            {
                for( unsigned i = 0; i < v.len; i++ )
                {
                    VBMarshalling< T , TV_HasExternalMember< T , TP >::value >::SaveExtData( mapper , v.pData[ i ] );
                }
            }
        }
    }
    template< typename CP > static void CorrectPtr( CP & r , unsigned & posEnd , const VSConstBuffer< char > & pbuff , unsigned objPos , unsigned curPos )
    { 
        unsigned off = offsetof( VSConstBuffer< T > , pData ) ;
        const VSConstBuffer< T > * pcb = reinterpret_cast< const VSConstBuffer< T > * >( pbuff.pData + objPos ) ;
        if( pcb->len )
        {
            const T * pval = reinterpret_cast< const T * >( pbuff.pData + curPos );
            r.Modify( objPos + off , VSConstBuffer< char >( sizeof( const T * ) , reinterpret_cast< const char * >( &pval ) ) );
            posEnd = curPos + pcb->len * sizeof( T );

            if( TV_HasPtrMember< T , CP >::value )
            {
                for( unsigned i = 0; i < pcb->len; i++ )
                {
                    VBMarshalling< T , TV_HasPtrMember< T , CP >::value >::CorrectPtr( r , posEnd , pbuff , curPos + i * sizeof( T ) , posEnd );
                }
            }
        }
    } 
};  
template< typename T > struct VBMarshalling< VS01Buffer< T > , true >
{ 
    template< typename TP >
    static void SaveExtData( TP & mapper , const VS01Buffer< T > & v ) 
    {
        if( v.m_Vaild )
        {
            mapper.Append( VSConstBuffer< char >( sizeof( T ) , reinterpret_cast< const char * >( v.m_pData ) ) );
            if( TV_HasExternalMember< T , TP >::value )
            {
                VBMarshalling< T , TV_HasExternalMember< T , TP >::value >::SaveExtData( mapper , *v.m_pData );
            }
        }
    }

    template< typename CP > static void CorrectPtr( CP & r , unsigned & posEnd , const VSConstBuffer< char > & pbuff , unsigned objPos , unsigned curPos )
    { 
        unsigned off = offsetof( VS01Buffer<T> , m_pData ) ;
        const VS01Buffer< T > * pcb = reinterpret_cast< const VS01Buffer<T> * >( pbuff.pData + objPos ) ;

        if( pcb->m_Vaild )
        {
            const T * pval = reinterpret_cast< const T * >( pbuff.pData + curPos ) ;
            
            r.Modify( objPos + off , VSConstBuffer< char >( sizeof( const T * ) , reinterpret_cast< const char * >( &pval ) ) ) ;
            posEnd = curPos + sizeof( T ) ;

            if( TV_HasPtrMember< T , CP >::value )
            {
                VBMarshalling< T , TV_HasPtrMember< T , CP >::value >::CorrectPtr( r , posEnd , pbuff , curPos , posEnd ) ;
            }
        }
    } 
};  


template< typename T >
class TVFilePersist
{
public:
    TVFilePersist(){}

private:
    struct InputPort
    {
        std::vector< char > & m_buff;
        InputPort( std::vector< char > & buff ) :m_buff( buff )
        {}
        void Append( const VSConstBuffer< char > & cb )
        {
            unsigned curPos = m_buff.size();
            m_buff.resize( curPos + cb.len );
            memcpy( &m_buff[ curPos ] , cb.pData , cb.len );
        }
    };
    struct CoorectPort
    {
        char     * m_pBuff;
        unsigned   m_len;
        CoorectPort( unsigned len , char * buff ) : m_len( len ) , m_pBuff( buff ){}
        void Modify( unsigned pos , const VSConstBuffer< char > & cb )
        {
            assert( cb.len == 4 );
            unsigned offset = *(unsigned*)cb.pData - (unsigned)m_pBuff;
            memcpy( m_pBuff + pos , &offset , 4 );
        }
    };

    void _refreshPtr()
    {
        unsigned nEnd = sizeof( T );
        VBMarshalling< T , TV_HasPtrMember< T , CoorectPort >::value >
            ::CorrectPtr( CoorectPort( m_buff.size() , &m_buff.front() ) , nEnd , VD_V2CB( m_buff ) , 0 , sizeof( T ) );
    }
public:
    void Build( const T & val )
    {
        m_buff.clear();

        InputPort   inp( m_buff );

        inp.Append( VSConstBuffer< char >( sizeof( T ) , reinterpret_cast< const char * >( &val ) ) );
        VBMarshalling< T , TV_HasExternalMember< T , InputPort >::value >::SaveExtData( inp , val );

        _refreshPtr();
    }

    VSConstBuffer< char > GetBuffer() const
    {
        return VD_V2CB( m_buff );
    }

    template< typename TB >
    friend std::ostream & operator <<( std::ostream & istm , TVFilePersist< TB > & stg );

private:
    std::vector< char > m_buff;
};

template< typename T >
inline std::ostream & operator << ( std::ostream & ostm , TVFilePersist< T > & stg )
{
    auto cbTemp = VD_V2CB( stg.m_buff );

    if ( cbTemp.len )
        ostm.write( cbTemp.pData , cbTemp.len );

    return ostm;
}

template< typename T >
class TVPersist
{
public :
    TVPersist(){}

private :

    struct InputPort
    {
        std::vector< char > & m_buff ;
        InputPort( std::vector< char > & buff ):m_buff(buff){}
        void Append( const VSConstBuffer< char > & cb )
        {
            unsigned curPos = m_buff.size() ;
            m_buff.resize( curPos + cb.len ) ;
            memcpy( & m_buff[curPos] , cb.pData , cb.len ) ;
        }
    } ;
    struct CoorectPort 
    { 
        char     * m_pBuff ;
        unsigned   m_len   ;
        CoorectPort( unsigned len , char * buff ) : m_len( len ) , m_pBuff( buff ) {}
        void Modify( unsigned pos , const VSConstBuffer< char > & cb )
        { 
            memcpy( m_pBuff + pos , cb.pData , cb.len );
       } 
    } ;  

    void _refreshPtr()
    { 
        unsigned nEnd = sizeof(T);

        VBMarshalling< T , TV_HasPtrMember< T , CoorectPort >::value >
            ::CorrectPtr( CoorectPort( m_buff.size() , &m_buff.front() ) , nEnd , VD_V2CB( m_buff ) , 0 , sizeof( T ) ); 
    }

public :
    const T & Get() const
    { 
        return * reinterpret_cast< const T * >( VD_V2CB( m_buff ).pData ) ;
    }

    void Build( const T & val )
    {
        m_buff.clear();

        InputPort   inp( m_buff );

        inp.Append( VSConstBuffer< char >( sizeof( T ) , reinterpret_cast< const char * >( &val ) ) ); 
        VBMarshalling< T , TV_HasExternalMember< T , InputPort >::value >::SaveExtData( inp , val );

        _refreshPtr();
    }

    template< typename TB > 
    friend std::istream & operator >>( std::istream & istm , TVPersist< TB > & stg ) ;

    template< typename TB > 
    friend std::ostream & operator <<( std::ostream & istm , TVPersist< TB > & stg ) ;

    VSConstBuffer< char > GetBuffer() const
    {
        return VD_V2CB( m_buff ) ;
    }
private :
    std::vector< char > m_buff ;
} ;

template< typename T >
inline std::istream & operator >> ( std::istream & istm , TVPersist< T > & stg )
{ 
    unsigned len    ;

    istm.read( reinterpret_cast< char * >( &len ) , sizeof(unsigned ) ) ;
    stg.m_buff.resize( len ) ;
    if( len )
        istm.read( & stg.m_buff.front() , len ) ;

    stg._refreshPtr() ;

    return istm ;
}

template< typename T >
inline std::ostream & operator << ( std::ostream & ostm , TVPersist< T > & stg )
{
    auto cbTemp = VD_V2CB( stg.m_buff ); 

    ostm.write( reinterpret_cast< const char * >( &cbTemp.len ) , sizeof( unsigned ) ) ;

    if( cbTemp.len  )
        ostm.write( cbTemp.pData , cbTemp.len ) ;

    return ostm ;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Macro of persistable class
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define VD_PERSISTCLASS_BEGIN_HELP( structname , c ) struct structname { \
static const unsigned LOOPBGN = c ;\
typedef structname MYTYPE ;

#define VD_DEFMEMBER_HELP( type , name , c ) type name ;\
    template< typename TP >  static void LoopMap( const TVHelperCounter< c - LOOPBGN - 1 > * , TP & mapper , const MYTYPE & v )  {\
        VBMarshalling< type , TV_HasExternalMember< type , TP >::value >::SaveExtData( mapper , v.name ) ;\
        LoopMap( reinterpret_cast< TVHelperCounter< c - LOOPBGN > * >(0) , mapper , v ) ; \
    }\
    template< typename CP > static void LoopCorrect( const TVHelperCounter< c - LOOPBGN - 1 > * , CP & r , unsigned & posEnd , const VSConstBuffer< char > & pbuff , unsigned objPos , unsigned curPos ){\
        VBMarshalling< type , TV_HasPtrMember< type , CP >::value >::CorrectPtr( r , posEnd , pbuff , objPos + offsetof( MYTYPE , name ) , curPos ) ;\
        LoopCorrect( reinterpret_cast< TVHelperCounter< c - LOOPBGN > * >(0) , r , posEnd , pbuff , objPos , posEnd ) ; \
    } 

#define VD_PERSISTCLASS_END_HELP( c )  template< typename TP > static void LoopMap( const TVHelperCounter< c - LOOPBGN - 1 > * , TP & , const MYTYPE & )  {}\
    template< typename CP > static void LoopCorrect( const TVHelperCounter< c - LOOPBGN - 1 > * , CP & , unsigned & , const VSConstBuffer< char > & , unsigned , unsigned )  {}\
    template< typename TP > static void MapTo( TP & mapper , const MYTYPE & ti ){  \
    LoopMap( reinterpret_cast< TVHelperCounter< 0 > * >(0) , mapper , ti ) ; \
    }\
    template< typename CP >\
    static void CorrectPtr( CP & r , unsigned & posEnd , const VSConstBuffer< char > & pbuff , unsigned objPos , unsigned curPos ){\
        LoopCorrect( reinterpret_cast< TVHelperCounter< 0 > * >(0) , r , posEnd , pbuff , objPos , curPos ) ; \
    } ;\
} ; 

#define VD_PERSISTCLASS_BEGIN( structname ) VD_PERSISTCLASS_BEGIN_HELP( structname , VD_EXPAND( __COUNTER__ ) )
#define VD_DEFMEMBER( type , name )         VD_DEFMEMBER_HELP( type , name , VD_EXPAND( __COUNTER__ ) ) 
#define VD_PERSISTCLASS_END( )   VD_PERSISTCLASS_END_HELP( VD_EXPAND( __COUNTER__ ) )

template< class T , typename TP >
struct TV_HasExternalMember
{
    template<typename U , void ( * )( TP & , const U & ) > struct SFINAE {};
    template<typename U> static char Test( SFINAE< U , &U::MapTo >* );
    template<typename U> static int  Test( ... );
    static const bool value = std::is_pointer< T >::value || ( sizeof( Test< T >( 0 ) ) == sizeof( char ) );
};

template< class T , typename TP > struct TV_HasExternalMember< T * , TP > { static const bool value = true ; } ;
template< class T , typename TP > struct TV_HasExternalMember< VSConstBuffer< T > , TP > { static const bool value = true ; } ;
template< class T , typename TP > struct TV_HasExternalMember< VS01Buffer< T > , TP > { static const bool value = true ; } ;

template< class T , typename CP >
struct TV_HasPtrMember
{
    template<typename U , void ( * )( CP & , unsigned & , const VSConstBuffer< char > & , unsigned , unsigned ) > struct SFINAE {};
    template<typename U> static char Testrev( SFINAE< U , &U::CorrectPtr >* );
    template<typename U> static int  Testrev( ... );
    static const bool value = std::is_pointer< T >::value || ( sizeof( Testrev< T >( 0 ) ) == sizeof( char ) );
}; 

template< class T , typename CP > struct TV_HasPtrMember< T * , CP > { static const bool value = true ; } ;
template< class T , typename CP > struct TV_HasPtrMember< VSConstBuffer< T > , CP > { static const bool value = true ; } ;
template< class T , typename CP > struct TV_HasPtrMember< VS01Buffer< T > , CP >    { static const bool value = true ; } ;


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Old classes
////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum EVStrmSeekMode
{
    VSM_FROMSTART ,
    VSM_FROMEND   ,
    VSM_FROMCURRNET ,
} ;

VPUREINTERFACE IVInputStream 
{ 
    virtual size_t Read( void * buffer , size_t sz )       = 0 ;  
} ;

VPUREINTERFACE IVOutputStream
{
    virtual size_t Write( const void * buffer , size_t sz ) = 0 ; 
} ; 
 
VPUREINTERFACE IVOutputable
{
    virtual void SaveToStream ( IVOutputStream & stm ) = 0 ;
} ;

VPUREINTERFACE IVInputable
{
    virtual void LoadFromStream( IVInputStream & stm ) = 0 ;
} ;

VPUREINTERFACE IVPersist : public IVOutputable, public IVInputable
{
} ;


class VMemStream : public IVInputStream
				 , public IVOutputStream
{
public:
	VMemStream( unsigned buffLen = 1024 )
		: m_nReadPos( 0 )
		, m_nWritePos( 0 )
		, m_nLastWrite( 0 )
		, m_nLastRead( 0 )
		, m_nBuffLen( buffLen )
	{
		m_pBuffer = new unsigned char[ buffLen ] ;
	}

	VMemStream( IVInputStream & is )
		: m_nReadPos( 0 )
		, m_nWritePos( 0 )
		, m_nLastWrite( 0 )
		, m_nLastRead( 0 )
        //, m_nBuffLen( is.Tell() )
	{
        const unsigned ILEN = 1024 ;
        std::vector< unsigned char > vBuff ;
        unsigned char vTmp[ILEN] ;

        while(1)
        {
            unsigned rl = is.Read( vTmp , ILEN ) ;
            if( rl > 0 )
            {
                vBuff.insert( vBuff.end() , vTmp , vTmp + rl ) ;
            }
            else
                break ;
        }

        m_nBuffLen = vBuff.size() ;
        m_nWritePos = m_nBuffLen  ;
		m_pBuffer = new unsigned char[ m_nBuffLen ] ;
        memcpy( m_pBuffer , &vBuff[0] , m_nBuffLen * sizeof( unsigned char ) ) ;
	}

	~VMemStream(void)
	{
		delete[] m_pBuffer ;
	}

public :
    void Clean()
    { 
        m_nReadPos = 0 ;
        m_nWritePos = 0 ; 
        m_nLastWrite = 0 ;
        m_nLastRead = 0 ;
    } 

public :
	// IVOutputStream
    virtual size_t Write( const void * buffer , size_t sz )
	{ 
		if( m_nWritePos + sz >= m_nBuffLen )
			m_nLastWrite = m_nBuffLen - m_nWritePos ;
		else
			m_nLastWrite = sz ;

		if( m_nLastWrite > 0 )
		{
			memcpy( m_pBuffer + m_nWritePos , buffer , m_nLastWrite ) ;
			m_nWritePos += m_nLastWrite ;
		}

		return m_nLastWrite ;
	} 

public :
    virtual bool             Eof()                                   
	{
		return m_nReadPos >= m_nBuffLen ;
	}

    virtual size_t Read( void * buffer , size_t sz )       
	{
		if( m_nReadPos + sz >= m_nWritePos )
			m_nLastRead = m_nWritePos - m_nReadPos ;
		else
			m_nLastRead = sz ;

		if( m_nLastRead > 0 )
		{
			memcpy( buffer , m_pBuffer + m_nReadPos , m_nLastRead ) ;
			m_nReadPos += m_nLastRead ;
		}

		return m_nLastRead ;
	}

    virtual void  Seek( int pos , EVStrmSeekMode sm )         
	{
		int toPos ;

		switch( sm )
		{
		case VSM_FROMSTART :
			toPos = pos ;
			break ;
		case VSM_FROMEND :
			toPos = (int)m_nWritePos + pos ;
			break ;
		case VSM_FROMCURRNET :
			toPos = (int)m_nReadPos + pos ;
			break ;
		} 

		if( toPos < 0 )
			m_nReadPos = 0 ;
		else if( toPos > static_cast<int>( m_nWritePos ) )
			m_nReadPos = m_nWritePos ;
		else
			m_nReadPos = static_cast<unsigned>( toPos ) ;
	}

    virtual size_t        Tell()                                  
	{
		return static_cast< size_t >( m_nReadPos ) ;
	}

protected :
	size_t          m_nLastWrite ;
	size_t			m_nLastRead  ;
	unsigned        m_nReadPos   ;
	unsigned        m_nWritePos  ;
	unsigned        m_nBuffLen   ;
	unsigned char * m_pBuffer    ;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

template< typename... TARGs > class TVBuilderArgTuple ;
template<> 
class TVBuilderArgTuple<>
{
public :
    TVBuilderArgTuple(){}
    ~TVBuilderArgTuple(){}

public :
    void Resume( std::ifstream & ss ){}
    template< typename F >
    void Travel( F f ){ f() ; }
};
template< typename TA , typename... TARGs >
class TVBuilderArgTuple< TA , TARGs ... >
{
public :
    TVBuilderArgTuple(){}
    ~TVBuilderArgTuple(){}

public :
    void Resume( std::ifstream & ss )
    {
        ss >> m_val ;
        m_sub.Resume( ss ) ;
    }

    template< typename F >
    void Travel( F f )
    {
        m_sub.Travel( [this,f]( const TARGs & ... args ){
            f( m_val.Get() , args ... ) ;
        }) ;
    }

private :
    TVPersist< TA > m_val ;
    TVBuilderArgTuple< TARGs ... > m_sub ;
} ;

inline void vf_serialze_args( std::ofstream & ss )
{
}
template< typename TI , typename ... TINPUTs >
inline void vf_serialze_args( std::ofstream & ss , const TI & v , const TINPUTs & ... inputs )
{
    {
        TVPersist< TI > stg;
        stg.Build( v );
        ss << stg ;
    }

    vf_serialze_args( ss , inputs ... ) ;
}

template<  typename ... TINPUTs >
inline void vf_serialze_args_to_file( const char * lpFileName , const TINPUTs & ... inputs )
{ 
    std::ofstream foutput( lpFileName , std::ios::binary | std::ios::trunc ) ;
    vf_serialze_args( foutput , inputs... ) ;
}
template< typename F , typename ... TINPUTs >
inline void vf_serialze_args_to_file_c( F fBuildFileName , const TINPUTs & ... inputs )
{ 
    static unsigned counter(0) ;
    
    std::stringstream ss ;
    fBuildFileName( ss , counter ) ;
    std::ofstream foutput( ss.str() , std::ios::binary | std::ios::trunc ) ;
    vf_serialze_args( foutput , inputs... ) ;

    counter ++ ;
}

template< typename T > 
class TVTestBuilder
{
public :
    TVTestBuilder(){}
    ~TVTestBuilder(){}
    
public:
    template< typename T , typename TOutput , typename... TARGs , typename F >
    static void BuildPersist( T & obj , std::ifstream & ss , void ( T::*func )( TOutput & outval , const TARGs & ... ) , F f )
    {
        TOutput outval ;
        TVBuilderArgTuple< TARGs ... > tplArgs;
        tplArgs.Resume( ss );
        tplArgs.Travel( [ &obj , &outval , &func ]( const TARGs & ... args ){
            (obj.*func)( outval , args ... ) ;
        } ) ;

        f( outval ) ;
    }

public : 
    inline void TestFromStream( std::ifstream & ss )
    {
        BuildPersist( m_obj , ss , &T::Build , []( const auto & val ){} ) ;
    }
    inline void TestFromFile( const char * lpFileName )
    {
        std::ifstream fstm( lpFileName , std::ios::binary ) ;
        TestFromStream( fstm ) ;
    }

    template< typename F >
    inline void TestFromStream( std::ifstream & ss , F f )
    {
        BuildPersist( m_obj , ss , &T::Build , f ) ;
    }
    template< typename F >
    inline void TestFromFile( const char * lpFileName  , F f )
    {
        std::ifstream fstm( lpFileName , std::ios::binary ) ;
        TestFromStream( fstm , f ) ;
    }

private :
    T m_obj ;
} ;
