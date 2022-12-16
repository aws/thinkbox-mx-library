// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vector>

// The max headers only work at warning level 3
#pragma warning( push, 3 )

// The 3ds max ref.h header
#if !defined(NO_INIUTIL_USING)
#define NO_INIUTIL_USING
#endif
#include <max.h>
//#include <max_mem.h>
#include <modstack.h>
#include <ref.h>

#pragma warning( pop )

// This class provides some functions which allow one to enumerate all the INodes of an object,
// and enumerate different objects which reference a given object.

namespace frantic {
namespace max3d {

// This gets every ReferenceMaker object referring to refTarget
void get_refmakers( std::vector<ReferenceMaker*>& refMakers, ReferenceTarget* refTarget );

// This retrieves the list of all the inodes of refTarget.
void get_object_inodes( std::vector<INode*>& inodes, ReferenceTarget* refTarget );

// This gets every INode which references the refTarget
void get_referring_inodes( std::vector<INode*>& inodes, ReferenceTarget* refTarget );

// This gets every INode which references the refTarget. recursive, so it checks each reference's referers too.
void get_referring_inodes_recursive( std::vector<INode*>& inodes, ReferenceTarget* refTarget );

// This gets every object of the given class_id which refers to the target.
void get_referring_objects( std::vector<ReferenceMaker*>& objects, ReferenceTarget* refTarget,
                            Class_ID cid = Class_ID( 0, 0 ) );

// This gets every inode of every object of the given class_id which refers to the target.
void get_referring_objects_inodes( std::vector<INode*>& inodes, ReferenceTarget* refTarget, Class_ID cid );

// This gets every object with the given modifier class id applied to it, referencing the
// given refTarget. NOTE: this is a fairly specialized function designed to enumerate all the emitters of a
// fluid object.  It only accepts object space modifiers.
// WARNING: It might be possible for something incorrect to slip through, as I haven't figured out how to verify that
// the modifier
//          is in the modifier stack of the object.  It hasn't happened in practice, though.
void get_referring_osmodifier_inodes( std::vector<std::pair<INode*, Modifier*>>& mods, ReferenceTarget* refTarget,
                                      Class_ID cid = Class_ID( 0, 0 ) );

// Gets all the modifiers in the stack of the given inode.
// Given in the order that EnumGeomPipeline gives
void get_modifier_stack( std::vector<Modifier*>& mods, INode* inode );

bool is_in_modifier_stack( Modifier* mod, INode* inode );

// Given a modifier and a mod context, this will find which INode owns it
INode* find_modifier_inode( Modifier* mod, ModContext& mc );

} // namespace max3d
} // namespace frantic
