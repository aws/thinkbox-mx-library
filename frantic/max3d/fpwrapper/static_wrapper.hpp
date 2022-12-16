// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fpwrapper/funcpub_basewrapper.hpp>

namespace frantic {
namespace max3d {
namespace fpwrapper {

// The main class, deriving from FPStaticInterface
template <class MainClass, USHORT FPFlags = 0>
class FFStaticInterface : public FPStaticInterface {
    FFInterfaceWrapper<MainClass> m_FFInterfaceWrapper;

  public:
    // This class accesses the m_FFInterfaceWrapper variable and the function FinalizeFFInterfaceWrapper
    friend class FFCreateDescriptorImpl<MainClass>;

    // The MainClass class uses FFCreateDescriptor to generate the descriptor
    typedef typename FFInterfaceWrapper<MainClass>::FFCreateDescriptor FFCreateDescriptor;

    FFStaticInterface() {
        // Make sure the flags passed are valid
        assert( FPFlags == 0 || FPFlags == FP_CORE );
    }

  private:
    // This is called from the destructor of the FFCreateDescriptor, to finish off creation of the interface
    void FinalizeFFInterfaceWrapper( FFCreateDescriptor& ffcd ) {
        make_varargs va;

        m_FFInterfaceWrapper.make_descriptor_varargs( ffcd, va );

        // va.dump( "c:/varargs.bin" );

        // FPStaticInterface is derived from FPInterfaceDesc, which has this function for creating the descriptor
        load_descriptor( ffcd.GetInterfaceID(), const_cast<TCHAR*>( ffcd.GetInterfaceName().c_str() ), 0,
                         ffcd.GetClassDesc(), FPFlags, va.get() );
    }

    FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
        return m_FFInterfaceWrapper._dispatch_fn( static_cast<MainClass*>( this ), fid, t, result, p );
    }
};

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
