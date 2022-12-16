// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fpwrapper/funcpub_basewrapper.hpp>

namespace frantic {
namespace max3d {
namespace fpwrapper {

template <class MainClass>
class FFMixinInterface;

// Gives the FFMixinInterface class access to the load_descriptor function
template <class MainClass>
class FPInterfaceDescWrapper : public FPInterfaceDesc {
    friend class FFMixinInterface<MainClass>;
};

template <class MainClass>
class FPInterfaceDescWrapperHolder {
    FPInterfaceDescWrapper<MainClass>* m_descriptor;

  public:
    FPInterfaceDescWrapperHolder() { m_descriptor = 0; }
    void set( FPInterfaceDescWrapper<MainClass>* descriptor ) {
        assert( m_descriptor == 0 );
        m_descriptor = descriptor;
    }
    FPInterfaceDescWrapper<MainClass>* get() const { return m_descriptor; }
};

// The main class, deriving from FPMixinInterface
template <class MainClass>
class FFMixinInterface : public FPMixinInterface {
    FFInterfaceWrapper<MainClass> m_FFInterfaceWrapper;

    static FPInterfaceDescWrapperHolder<MainClass> m_descriptor;

  public:
    // This class accesses the m_FFInterfaceWrapper variable and the function FinalizeFFInterfaceWrapper
    friend class FFCreateDescriptorImpl<MainClass>;

    // The MainClass class uses FFCreateDescriptor to generate the descriptor
    typedef typename FFInterfaceWrapper<MainClass>::FFCreateDescriptor FFCreateDescriptor;

    FFMixinInterface() {}

  private:
    // This is called from the destructor of the FFCreateDescriptor, to finish off creation of the interface
    void FinalizeFFInterfaceWrapper( FFCreateDescriptor& ffcd ) {
        if( m_descriptor.get() == 0 ) {
            make_varargs va;

            m_FFInterfaceWrapper.make_descriptor_varargs( ffcd, va );

            m_descriptor.set( new FPInterfaceDescWrapper<MainClass>() );

            // FPStaticInterface is derived from FPInterfaceDesc, which has this function for creating the descriptor
            m_descriptor.get()->load_descriptor( ffcd.GetInterfaceID(),
                                                 const_cast<TCHAR*>( ffcd.GetInterfaceName().c_str() ), 0,
                                                 ffcd.GetClassDesc(), FP_MIXIN, va.get() );
        }
    }

    FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
        return m_FFInterfaceWrapper._dispatch_fn( static_cast<MainClass*>( this ), fid, t, result, p );
    }

    FPInterfaceDesc* GetDesc() { return m_descriptor.get(); }
};

template <class MainClass>
FPInterfaceDescWrapperHolder<MainClass> FFMixinInterface<MainClass>::m_descriptor;

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
