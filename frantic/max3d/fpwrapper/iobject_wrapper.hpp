// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fpwrapper/funcpub_basewrapper.hpp>

namespace frantic {
namespace max3d {
namespace fpwrapper {

template <class MainClass>
class FFIObject : public IObject, public FPStaticInterface {
    FFInterfaceWrapper<MainClass> m_FFInterfaceWrapper;

    int m_ffRefCount;
    std::string m_ffObjectName;
    Interface_ID m_ffInterfaceID;

  public:
    // This class accesses the m_FFInterfaceWrapper variable and the function FinalizeFFInterfaceWrapper
    friend class FFCreateDescriptorImpl<MainClass>;

    FFIObject() { m_ffRefCount = 0; }

    // Important: subclass should also use a virtual destructor to ensure proper destruction
    virtual ~FFIObject() {}

    TCHAR* GetIObjectName() { return const_cast<char*>( m_ffObjectName.c_str() ); }
    int NumInterfaces() const { return 1; }
    BaseInterface* GetInterfaceAt( int i ) const {
        if( i == 0 )
            return const_cast<MainClass*>( static_cast<const MainClass*>( this ) );
        else
            return 0;
    }
    BaseInterface* GetInterface( Interface_ID id ) {
        if( id == m_ffInterfaceID )
            return GetInterfaceAt( 0 );
        else
            return 0;
    }
    void AcquireIObject() { ++m_ffRefCount; }
    void ReleaseIObject() {
        --m_ffRefCount;
        if( m_ffRefCount == 0 )
            DeleteIObject();
    }
    void DeleteIObject() {
        assert( m_ffRefCount == 0 );
        delete this;
    }

    // The MainClass class uses FFCreateDescriptor to generate the descriptor
    typedef typename FFInterfaceWrapper<MainClass>::FFCreateDescriptor FFCreateDescriptor;

  private:
    // This is called from the destructor of the FFCreateDescriptor, to finish off creation of the interface
    void FinalizeFFInterfaceWrapper( FFCreateDescriptor& ffcd ) {
        // Save these values so they can be used by GetIObjectName and GetInterface
        m_ffObjectName = ffcd.GetInterfaceName();
        m_ffInterfaceID = ffcd.GetInterfaceID();

        make_varargs va;

        m_FFInterfaceWrapper.make_descriptor_varargs( ffcd, va );

        // va.dump( "c:/varargs.bin" );

        // FPStaticInterface is derived from FPInterfaceDesc, which has this function for creating the descriptor
        load_descriptor( ffcd.GetInterfaceID(), _T( const_cast<char*>( ffcd.GetInterfaceName().c_str() ) ), 0,
                         ffcd.GetClassDesc(), 0, va.get() );
    }

    FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
        return m_FFInterfaceWrapper._dispatch_fn( static_cast<MainClass*>( this ), fid, t, result, p );
    }
};

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
