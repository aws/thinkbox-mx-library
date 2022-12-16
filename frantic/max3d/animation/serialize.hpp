// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/*A max interface to serialize track view nodes and controllers
Serializing a node/controller will return an xml formatted string representation of that object
That string can then be used to recreate that object

Controllers - The controller behaves as expected.  The given controller will be serialized and
deserialization will return a new instance of that controller

TrackViewNodes - These behave slightly differently.  You pass in node and all the subnodes/controllers
contained beneath that node are serialized. When deserilializing you need to pass in the parent
node you want all the subnodes/controllers to be inserted into.  This is because you cannot just create
free floating track view nodes to be inserted into the track view tree at a later time
*/

#include <frantic/max3d/fpwrapper/static_wrapper.hpp>

class serialization_interface : public frantic::max3d::fpwrapper::FFStaticInterface<serialization_interface, FP_CORE> {

  public:
    serialization_interface();

    std::string serialize_tvnode( Value* tvnode );
    void deserialize_tvnode( const std::string& serializedValueString, Value* parent );

    std::string serialize_controller( Control* controller );
    Control* deserialize_controller( const std::string& serializedValueString );

    std::string test( Value* node );

    void initialize();
};
