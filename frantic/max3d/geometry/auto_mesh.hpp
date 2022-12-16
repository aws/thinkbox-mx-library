// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/geopipe/object_dumping_help.hpp>
#include <frantic/strings/tstring.hpp>

namespace frantic {
namespace max3d {
namespace geometry {

namespace {

template <class T>
struct auto_max_obj_traits {
    static void destroy( T* obj ) { obj->DeleteThis(); }
};

template <>
struct auto_max_obj_traits<PolyObject> {
    static void destroy( PolyObject* obj ) { obj->MaybeAutoDelete(); }
};

} // anonymous namespace

// Holds a max object, and deletes it when the last copy is destroyed
// IMPORTANT: these objects should be passed around by value, generally
// This class is not thread-safe
template <class MaxObj>
class AutoMaxObj {
  public:
    AutoMaxObj()
        : _maxObj( 0 )
        , _deleteIt( false ) {
        _refCount = new int( 1 );
    }

    AutoMaxObj( MaxObj* maxObj, bool deleteIt )
        : _maxObj( maxObj )
        , _deleteIt( deleteIt ) {
        _refCount = new int( 1 );
    }

    AutoMaxObj( const AutoMaxObj<MaxObj>& rhs )
        : _maxObj( rhs._maxObj )
        , _deleteIt( rhs._deleteIt )
        , _refCount( rhs._refCount ) {
        ( *_refCount )++;
    }

    ~AutoMaxObj() { destroy(); }

    AutoMaxObj<MaxObj>& operator=( const AutoMaxObj<MaxObj>& rhs ) {
        destroy();
        _maxObj = rhs._maxObj;
        _deleteIt = rhs._deleteIt;
        _refCount = rhs._refCount;
        ( *_refCount )++;

        return *this;
    }

    MaxObj* get() { return _maxObj; }

    const MaxObj* get() const { return _maxObj; }

    MaxObj* operator->() { return _maxObj; }

    const MaxObj* operator->() const { return _maxObj; }

    MaxObj& operator*() { return *_maxObj; }

    const MaxObj& operator*() const { return *_maxObj; }

  private:
    void destroy() {
        ( *_refCount )--;
        if( ( *_refCount ) == 0 ) { // If this is the last reference, delete the object
            if( _deleteIt && _maxObj != 0 )
                auto_max_obj_traits<MaxObj>::destroy( _maxObj );
            delete _refCount;
        }
        _refCount = 0;
    }

    MaxObj* _maxObj;
    bool _deleteIt;
    int* _refCount;
};

// Some default max objects to use
typedef AutoMaxObj<Mesh> AutoMesh;
// typedef AutoMaxObj<MNMesh>      AutoMNMesh;
typedef AutoMaxObj<TriObject> AutoTriObject;
typedef AutoMaxObj<ForceField> AutoForceField;
typedef AutoMaxObj<PolyObject> AutoPolyObject;

class AutoMNMesh {
  public:
    AutoMNMesh()
        : _maxObj( 0 )
        , _deleteIt( false ) {
        _refCount = new int( 1 );
    }

    AutoMNMesh( MNMesh* maxObj, bool deleteIt )
        : _maxObj( maxObj )
        , _deleteIt( deleteIt ) {
        _refCount = new int( 1 );
    }

    AutoMNMesh( const AutoMNMesh& rhs )
        : _maxObj( rhs._maxObj )
        , _deleteIt( rhs._deleteIt )
        , _refCount( rhs._refCount ) {
        ( *_refCount )++;
    }

    ~AutoMNMesh() { destroy(); }

    AutoMNMesh& operator=( const AutoMNMesh& rhs ) {
        destroy();
        _maxObj = rhs._maxObj;
        _deleteIt = rhs._deleteIt;
        _refCount = rhs._refCount;
        ( *_refCount )++;

        return *this;
    }

    MNMesh* get() { return _maxObj; }

    const MNMesh* get() const { return _maxObj; }

    MNMesh* operator->() { return _maxObj; }

    const MNMesh* operator->() const { return _maxObj; }

    MNMesh& operator*() { return *_maxObj; }

    const MNMesh& operator*() const { return *_maxObj; }

  private:
    void destroy() {
        ( *_refCount )--;
        if( ( *_refCount ) == 0 ) { // If this is the last reference, delete the object
            if( _deleteIt )
                delete _maxObj;
            delete _refCount;
        }
        _refCount = 0;
    }

    MNMesh* _maxObj;
    bool _deleteIt;
    int* _refCount;
};

// Utility function which converts an inode into an AutoTriObject
inline AutoTriObject GetMeshFromINode( INode* inode, TimeValue t ) {
    Object* obj = inode->EvalWorldState( t ).obj;

    // Convert the object into a triobject
    TriObject* triObj;
    if( obj->CanConvertToType( Class_ID( TRIOBJ_CLASS_ID, 0 ) ) ) {
        triObj = (TriObject*)obj->ConvertToType( t, Class_ID( TRIOBJ_CLASS_ID, 0 ) );

        return AutoTriObject( triObj,
                              triObj != obj ); // FIXME: this does not seem to be the right condition for deletion
    } else
        return AutoTriObject( NULL, false );
}

inline AutoMesh get_mesh_from_inode( INode* node, TimeValue t, View& view ) {
    if( !node )
        throw std::runtime_error( "get_mesh_from_inode(): INode passed to function was null" );

    ObjectState os = node->EvalWorldState( t );
    Object* obj = os.obj;

    if( obj == 0 )
        throw std::runtime_error( std::string() + "get_mesh_from_inode(): INode \"" +
                                  frantic::strings::to_string( node->GetName() ) + "\" returned null object" );

    SClass_ID scid = obj->SuperClassID();

    // If the object is a derived object, follow its references to the real object
    // This is here because there were some biped objects not being saved when they should have been.
    while( scid == GEN_DERIVOB_CLASS_ID ) {
        obj = ( (IDerivedObject*)obj )->GetObjRef();
        if( obj == 0 )
            throw std::runtime_error( std::string() + "get_mesh_from_inode(): INode \"" +
                                      frantic::strings::to_string( node->GetName() ) +
                                      "\", IDerivedObject returned null object" );
        else
            scid = obj->SuperClassID();
    }

    if( scid != SHAPE_CLASS_ID && scid != GEOMOBJECT_CLASS_ID ) {
        throw std::runtime_error(
            std::string() + "get_mesh_from_inode(): INode \"" + frantic::strings::to_string( node->GetName() ) +
            "\" passed in is not a renderable object (superclassid is " + geopipe::SuperClassIDToString( scid ) + ")" );
    }

    // even Shapes are GeomObjects and they share the getRenderMesh method
    GeomObject* gObj = static_cast<GeomObject*>( obj );

    // get the render mesh:
    AutoMesh mesh;

    try {
        BOOL needs_delete = false;
        Mesh* mesh_p = gObj->GetRenderMesh( t, node, view, needs_delete );
        mesh = AutoMesh( mesh_p, needs_delete != FALSE );
    } catch( std::exception& e ) {
        const frantic::tstring msg = frantic::tstring() + _T("get_mesh_from_inode(): INode \"") + node->GetName() +
                                     _T("\", Caught a std::exception thrown inside geomObj->GetRenderMesh(): ") +
                                     frantic::strings::to_tstring( e.what() );
        throw RuntimeError( const_cast<TCHAR*>( msg.c_str() ) );
    }

    if( mesh.get() == 0 ) {
        const frantic::tstring msg =
            frantic::tstring() + _T("Object ") + node->GetName() + _T(" returned a null mesh.");
        throw RuntimeError( const_cast<TCHAR*>( msg.c_str() ) );
    }

    return mesh;
}

inline AutoPolyObject get_polyobject_from_inode( INode* inode, TimeValue t ) {
    if( !inode )
        throw std::runtime_error( "get_polyobject_from_inode(): INode passed to function was null" );

    ObjectState os = inode->EvalWorldState( t );
    Object* obj = os.obj;

    if( obj == 0 )
        throw std::runtime_error( std::string() + "get_polyobject_from_inode(): INode \"" +
                                  frantic::strings::to_string( inode->GetName() ) + "\" returned null object" );

    bool needsDelete = false;
    PolyObject* pPolyObj = NULL;

    if( os.obj->IsSubClassOf( polyObjectClassID ) ) {
        pPolyObj = static_cast<PolyObject*>( os.obj );
    } else if( os.obj->CanConvertToType( polyObjectClassID ) ) {
        pPolyObj = (PolyObject*)os.obj->ConvertToType( t, polyObjectClassID );
        needsDelete = ( pPolyObj != os.obj );
    } else {
        throw std::runtime_error( "The node: \"" + frantic::strings::to_string( inode->GetName() ) +
                                  "\" can not produce a polygon object" );
    }

    if( pPolyObj == 0 )
        throw std::runtime_error( "The node: \"" + frantic::strings::to_string( inode->GetName() ) +
                                  "\" produced a null polygon object" );

    AutoPolyObject result( pPolyObj, needsDelete );
    return result;
}

inline AutoMNMesh get_mnmesh_from_inode( INode* inode, TimeValue t ) {
    if( !inode )
        throw std::runtime_error( "get_mnmesh_from_inode(): INode passed to function was null" );

    ObjectState os = inode->EvalWorldState( t );
    Object* obj = os.obj;

    if( obj == 0 )
        throw std::runtime_error( std::string() + "get_mnmesh_from_inode(): INode \"" +
                                  frantic::strings::to_string( inode->GetName() ) + "\" returned null object" );

    BOOL needsDelete = FALSE;
    PolyObject* pPolyObj = NULL;

    if( os.obj->IsSubClassOf( polyObjectClassID ) ) {
        pPolyObj = static_cast<PolyObject*>( os.obj );
    } else if( os.obj->CanConvertToType( polyObjectClassID ) ) {
        pPolyObj = (PolyObject*)os.obj->ConvertToType( t, polyObjectClassID );
        needsDelete = ( pPolyObj != os.obj );
    } else {
        throw std::runtime_error( "The node: \"" + frantic::strings::to_string( inode->GetName() ) +
                                  "\" can not produce a polygon mesh" );
    }

    AutoMNMesh mesh = AutoMNMesh( new MNMesh( pPolyObj->GetMesh() ), true );
    return mesh;
}

// inline std::vector<AutoMesh> get_multiple_meshs_from_inode(INode * node, TimeValue t, View & view)
//{
//	if(!node)
//		throw std::runtime_error("get_mesh_from_inode(): INode passed to function was null");
//
//	ObjectState os = node->EvalWorldState(t);
//
//	if( os.obj == 0 )
//		throw std::runtime_error("get_mesh_from_inode(): INode returned null object");
//
//	SClass_ID scid = os.obj->SuperClassID();
//	if( scid != SHAPE_CLASS_ID && scid != GEOMOBJECT_CLASS_ID ){
//		throw std::runtime_error("get_mesh_from_inode(): INode passed in is not a renderable object.");
//	}
//
//	// even Shapes are GeomObjects and they share the getRenderMesh method
//	GeomObject * gObj = static_cast<GeomObject * >(os.obj);
//	std::vector<AutoMesh> meshes;
//
//	for(int i = 0; i < gObj->NumberOfRenderMeshes(); i++)
//	{
//		// get the render mesh:
//		AutoMesh mesh;
//
//		try {
//			BOOL needs_delete = false;
//			Mesh * mesh_p = gObj->GetMultipleRenderMesh(t, node, view, needs_delete,i);
//			mesh = AutoMesh(mesh_p, needs_delete == TRUE);
//		} catch( std::exception& e ) {
//			std::string msg = std::string() + "Caught a std::exception thrown inside geomObj->GetRenderMesh(): "
//+ e.what(); 			throw RuntimeError( const_cast<char*>(msg.c_str()) ); 		} catch( MAXException& ) {
//			// Allow Max exceptions to pass through
//			throw;
//		} catch( MAXScriptException& ) {
//			// Allow Maxscript exceptions to pass through
//			throw;
//		} catch( ... ) {
//			throw RuntimeError( "Caught a crash in geomObj->GetRenderMesh()" );
//		}
//
//		if( mesh.get() == 0 )
//			throw RuntimeError( const_cast<char*>((std::string() + "Object " + node->GetName() + " returned a
//null mesh.").c_str()) );
//
//		meshes.push_back(mesh);
//	}
//	return meshes;
// }

} // namespace geometry
} // namespace max3d
} // namespace frantic
