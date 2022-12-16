// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef MAX_RELEASE
#error "You must include 3ds Max before including frantic/max3d/max_utility.hpp"
#endif

#pragma warning( push, 3 )
#include <modstack.h>
#pragma warning( pop )

#include <frantic/files/files.hpp>
#include <frantic/geometry/trimesh3.hpp>
#include <frantic/graphics/color3f.hpp>
#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>

namespace frantic {
namespace max3d {

inline void save_max_scene_copy( const frantic::tstring& filename ) {
    using namespace frantic;

    frantic::tstring holdMaxDir = GetCOREInterface()->GetDir( APP_AUTOBACK_DIR );
    frantic::tstring holdFileName = holdMaxDir + _T("/maxhold.mx");
    frantic::tstring holdTempFileName = holdMaxDir = _T("/maxhold.tmp");

    bool holdExists = files::file_exists( holdFileName );

    if( holdExists ) {
        DeleteFile( holdTempFileName.c_str() );
        MoveFile( holdFileName.c_str(), holdTempFileName.c_str() );
    }

    GetCOREInterface()->FileHold();

    if( !files::file_exists( holdFileName ) )
        throw std::runtime_error( "save_max_scene_copy: Saving the 3dsmax file via hold did not work." );

    if( files::file_exists( filename ) )
        DeleteFile( filename.c_str() );

    if( !MoveFile( holdFileName.c_str(), filename.c_str() ) )
        throw std::runtime_error( "save_max_scene_copy: Unable to move the saved 3dsmax file to " +
                                  frantic::strings::to_string( filename ) );

    if( holdExists ) {
        MoveFile( holdTempFileName.c_str(), holdFileName.c_str() );
    }
}

// Builds a vector of INodes from a Value pointer.  Useful for passing an
// array of objects from maxscript to C++
inline void build_inode_list( Value* value, std::vector<INode*>& outINodes ) {
    if( !value->is_kind_of( class_tag( Array ) ) )
        throw std::runtime_error( "build_inode_list: The parameter provided is not an array" );

    mxs::frame<1> f;
    mxs::local<Value> v( f );

    // No need to stick it in a locals spot because the caller must have protected it.
    Array* array = static_cast<Array*>( value );

    for( int i = 0; i < array->size; ++i ) {
        v = array->get( i + 1 );
        if( v->is_kind_of( class_tag( MAXNode ) ) ) {
            outINodes.push_back( fpwrapper::MaxTypeTraits<INode*>::FromValue( v ) );
        }
    }
}

inline INode* get_inode( INode* startNode, RefTargetHandle target ) {

    if( startNode && startNode->GetObjectRef() && startNode->GetObjectRef()->FindBaseObject() == target )
        return startNode;

    if( !startNode || startNode->NumberOfChildren() == 0 )
        return 0;

    INode* theINode = 0;
    for( int i = 0; i < startNode->NumberOfChildren(); ++i ) {
        INode* child = startNode->GetChildNode( i );
        theINode = get_inode( child, target );
        if( theINode )
            return theINode;
    }
    return 0;
}

inline Object* get_base_object( INode* node ) {
    Object* obj = node->GetObjectRef();
    while( obj->SuperClassID() == GEN_DERIVOB_CLASS_ID ) {
        IDerivedObject* derobj = static_cast<IDerivedObject*>( obj );
        obj = derobj->GetObjRef();
    }
    return obj;
}

inline ReferenceTarget* get_delegate_object( INode* node, Class_ID scriptedObjectID, Class_ID delegateObjectID ) {
    if( node ) {
        Object* obj = node->GetObjectRef()->FindBaseObject();
        if( obj && obj->ClassID() == scriptedObjectID && obj->NumRefs() > 0 ) {
            ReferenceTarget* delegateObj = obj->GetReference( 0 );
            if( delegateObj && delegateObj->ClassID() == delegateObjectID )
                return delegateObj;
        }
    }
    return NULL;
}

inline void collect_node_modifiers( INode* pNode, std::vector<std::pair<Modifier*, ModContext*>>& outMods,
                                    SClass_ID filter = 0x00, bool renderMode = false ) {
    Object* obj = pNode->GetObjOrWSMRef();

    while( obj->SuperClassID() == GEN_DERIVOB_CLASS_ID ) {
        IDerivedObject* pDerived = static_cast<IDerivedObject*>( obj );
        for( int i = 0; i < pDerived->NumModifiers(); ++i ) {
            Modifier* pMod = pDerived->GetModifier( i );
            if( ( filter == 0x00 || pMod->SuperClassID() == filter ) &&
                ( renderMode && pMod->IsEnabledInRender() && pMod->IsEnabled() ||
                  !renderMode && pMod->IsEnabledInViews() && pMod->IsEnabled() ) ) {
                outMods.push_back( std::pair<Modifier*, ModContext*>( pMod, pDerived->GetModContext( i ) ) );
            }
        }

        obj = pDerived->GetObjRef();
    }
}

inline void DrawBox( GraphicsWindow* gw, Box3 bbox ) {
    // Draws the specified bounding box into the
    // graphics window

    if( gw ) {
        Point3 points[6];

        // draw the 'top'
        points[0].Set( bbox.pmin.x, bbox.pmin.y, bbox.pmin.z );
        points[1].Set( bbox.pmax.x, bbox.pmin.y, bbox.pmin.z );
        points[2].Set( bbox.pmax.x, bbox.pmax.y, bbox.pmin.z );
        points[3].Set( bbox.pmin.x, bbox.pmax.y, bbox.pmin.z );
        points[4].Set( 0, 0, 0 );
        gw->polyline( 4, points, NULL, NULL, TRUE, NULL );

        // draw the 'bottom' (just change z from m_zFrom to m_zTo)
        for( int b = 0; b < 4; b++ ) {
            points[b].z = bbox.pmax.z;
        }
        gw->polyline( 4, (Point3*)&points, NULL, NULL, TRUE, NULL );

        // draw the 'left'
        points[0].Set( bbox.pmax.x, bbox.pmax.y, bbox.pmin.z );
        points[1].Set( bbox.pmax.x, bbox.pmin.y, bbox.pmin.z );
        points[2].Set( bbox.pmax.x, bbox.pmin.y, bbox.pmax.z );
        points[3].Set( bbox.pmax.x, bbox.pmax.y, bbox.pmax.z );
        points[4].Set( 0, 0, 0 );
        gw->polyline( 4, (Point3*)&points, NULL, NULL, TRUE, NULL );

        // draw the 'right' (just change x from m_xFrom to m_xTo)
        for( int b = 0; b < 4; b++ ) {
            points[b].x = bbox.pmin.x;
        }
        gw->polyline( 4, (Point3*)&points, NULL, NULL, TRUE, NULL );
    }
}

inline void DrawGrid( GraphicsWindow* gw, Box3 box, float cellLength, int everyNth = 1 ) {
    // Draws a grid into the specified box, starting at the minimum point,
    // and proceeding outwards, with cubic cells of sidelength cellLength,
    // and stepping by everyNth cells if requested.

    if( gw ) {
        Point3 points[3]; // need one extra, according to the docs.
        int i, j, k;

        Point3 rawSize = box.Width() / cellLength;

        Point3 base = box.Min();

        int xlimit = (int)rawSize.x;
        int ylimit = (int)rawSize.y;
        int zlimit = (int)rawSize.z;

        // draw lines parallel to the x axis
        for( j = 0; j <= ylimit; j++ ) {
            if( ( j == ylimit ) || ( j % everyNth == 0 ) ) {
                int zInc = ( j == 0 || j == ylimit ) ? everyNth : ( std::max )( zlimit, 1 );
                for( k = 0; k <= zlimit; k += zInc ) {
                    points[0] = base + cellLength * Point3( 0, j, k );
                    points[1] = base + cellLength * Point3( xlimit, j, k );
                    gw->polyline( 2, points, NULL, NULL, false, NULL );
                }
            }
        }

        // draw lines parallel to the y axis
        for( i = 0; i <= xlimit; i++ ) {
            if( ( i == xlimit ) || ( i % everyNth == 0 ) ) {
                int zInc = ( i == 0 || i == xlimit ) ? everyNth : ( std::max )( zlimit, 1 );
                for( k = 0; k <= zlimit; k += zInc ) {
                    points[0] = base + cellLength * Point3( i, 0, k );
                    points[1] = base + cellLength * Point3( i, ylimit, k );
                    gw->polyline( 2, points, NULL, NULL, false, NULL );
                }
            }
        }

        // draw lines parallel to the z axis
        for( i = 0; i <= xlimit; i++ ) {
            if( ( i == xlimit ) || ( i % everyNth == 0 ) ) {
                int yInc = ( i == 0 || i == xlimit ) ? everyNth : ( std::max )( ylimit, 1 );
                for( j = 0; j <= ylimit; j += yInc ) {
                    points[0] = base + cellLength * Point3( i, j, 0 );
                    points[1] = base + cellLength * Point3( i, j, zlimit );
                    gw->polyline( 2, points, NULL, NULL, false, NULL );
                }
            }
        }
    }
}

inline int hit_test_box( GraphicsWindow* gw, const Box3& box, HitRegion& hitRegion, int /*abortOnHit*/ ) {
    if( gw == 0 ) {
        return 0;
    }
    if( box.IsEmpty() ) {
        return 0;
    }

    bool hitAll = true;
    bool hitAny = false;
    DWORD distance = std::numeric_limits<DWORD>::max();

    const int faceCount = 6;

    const Point3 faces[faceCount][4] = { { box[0], box[1], box[3], box[2] }, { box[0], box[1], box[5], box[4] },
                                         { box[0], box[2], box[6], box[4] }, { box[1], box[3], box[7], box[5] },
                                         { box[2], box[3], box[7], box[6] }, { box[4], box[5], box[7], box[6] } };

    gw->clearHitCode();

    // BOOST_FOREACH( const Point3 * face, faces ) {
    for( int i = 0; i < faceCount; ++i ) {
        const Point3* face = faces[i];
        gw->clearHitCode();
        gw->polyline( 4, const_cast<Point3*>( face ), NULL, NULL, 1, NULL );
        if( gw->checkHitCode() ) {
            hitAny = true;
            distance = std::min<DWORD>( distance, gw->getHitDistance() );
        } else {
            hitAll = false;
        }
    }

    const bool requireAllInside = ( hitRegion.type != POINT_RGN ) && ( hitRegion.crossing == 0 );
    if( requireAllInside ) {
        gw->setHitCode( hitAll );
        gw->setHitDistance( distance );
    } else {
        gw->setHitCode( hitAny );
        gw->setHitDistance( distance );
    }

    return gw->checkHitCode();
}

// This draws all the visible edges of a mesh, using segments.  The mesh->render() function
// is kind of funky, so this is a simpler alternative when all you want is a simple wireframe.
inline void draw_mesh_wireframe( GraphicsWindow* gw, Mesh* mesh, const frantic::graphics::color3f& lineColor ) {
    if( gw != 0 && mesh != 0 ) {
        gw->setColor( LINE_COLOR, lineColor.r, lineColor.g, lineColor.b );
        gw->startSegments();

        Point3 positions[2];

        int numFaces = mesh->getNumFaces();
        for( int face = 0; face < numFaces; ++face ) {
            Face& faceData = mesh->faces[face];
            if( faceData.getEdgeVis( 0 ) ) {
                positions[0] = mesh->getVert( faceData.v[0] );
                positions[1] = mesh->getVert( faceData.v[1] );
                gw->segment( positions, 1 );
            }
            if( faceData.getEdgeVis( 1 ) ) {
                positions[0] = mesh->getVert( faceData.v[1] );
                positions[1] = mesh->getVert( faceData.v[2] );
                gw->segment( positions, 1 );
            }
            if( faceData.getEdgeVis( 2 ) ) {
                positions[0] = mesh->getVert( faceData.v[2] );
                positions[1] = mesh->getVert( faceData.v[0] );
                gw->segment( positions, 1 );
            }
        }

        gw->endSegments();
    }
}

/**
 *  Draw all edges in the given mesh as a wireframe.
 *
 * This is copied from draw_mesh_wireframe() in max_utility.hpp.
 *
 * @param gw the graphics window to draw the mesh on.
 * @param mesh the mesh to draw as a wireframe.
 * @param lineColor the line color to draw with.
 */
inline void draw_mesh_wireframe( GraphicsWindow* gw, const frantic::geometry::trimesh3& mesh,
                                 const frantic::graphics::color3f& lineColor ) {
    if( gw != 0 ) {
        gw->setColor( LINE_COLOR, lineColor.r, lineColor.g, lineColor.b );
        gw->startSegments();

        Point3 positions[2];

        std::size_t numFaces = mesh.face_count();
        for( std::size_t faceNumber = 0; faceNumber < numFaces; ++faceNumber ) {
            const frantic::graphics::vector3& face = mesh.get_face( faceNumber );

            positions[0] = to_max_t( mesh.get_vertex( face[0] ) );
            positions[1] = to_max_t( mesh.get_vertex( face[1] ) );
            gw->segment( positions, 1 );

            positions[0] = to_max_t( mesh.get_vertex( face[1] ) );
            positions[1] = to_max_t( mesh.get_vertex( face[2] ) );
            gw->segment( positions, 1 );

            positions[0] = to_max_t( mesh.get_vertex( face[2] ) );
            positions[1] = to_max_t( mesh.get_vertex( face[0] ) );
            gw->segment( positions, 1 );
        }

        gw->endSegments();
    }
}

/**
 *  Draw a given fraction of a mesh's vertices as marker points.
 *
 * @param gw the graphics window to draw the vertices on.
 * @param mesh the mesh whose vertices should be drawn.
 * @param lineColor the color to draw with.
 * @param drawFraction the fraction of the mesh's vertices to draw.  This
 *		should be in the range [0,1].
 */
inline void draw_mesh_vertices( GraphicsWindow* gw, const frantic::geometry::trimesh3& mesh,
                                const frantic::graphics::color3f& lineColor, const float drawFraction ) {
    if( gw != 0 ) {
        gw->setColor( LINE_COLOR, lineColor.r, lineColor.g, lineColor.b );
        gw->startMarkers();

        Point3 position;

        const std::size_t vertexCount = mesh.vertex_count();
        const std::size_t drawCount =
            frantic::math::clamp<std::size_t>( static_cast<std::size_t>( drawFraction * vertexCount ), 0, vertexCount );
        std::size_t error = 0;

        for( std::size_t vertexNumber = 0; vertexNumber < vertexCount; ++vertexNumber ) {
            error += drawCount;
            if( error >= vertexCount ) {
                error -= vertexCount;
                position = to_max_t( mesh.get_vertex( vertexNumber ) );
                gw->marker( &position, POINT_MRKR );
            }
        }

        gw->endMarkers();
    }
}

inline HFONT defaultMaxFont() { return GetCOREInterface()->GetAppHFont(); }

inline void setDefaultMaxFont( HWND hwnd ) {
    SendMessage( hwnd, WM_SETFONT, (WPARAM)frantic::max3d::defaultMaxFont(), TRUE );
}

inline void EnableCustButton( HWND hwndButton, bool enable ) {
    if( hwndButton ) {
        ICustButton* custButton = GetICustButton( hwndButton );
        if( custButton ) {
            custButton->Enable( enable );
            ReleaseICustButton( custButton );
        }
    }
}

inline void EnableCustStatus( HWND hwndStatus, bool enable ) {
    if( hwndStatus ) {
        ICustStatus* custStatus = GetICustStatus( hwndStatus );
        if( custStatus ) {
            custStatus->Enable( enable );
            ReleaseICustStatus( custStatus );
        }
    }
}

inline void SetCustStatusText( HWND hwndStatus, const frantic::tstring& text ) {
    if( hwndStatus ) {
        ICustStatus* custStatus = GetICustStatus( hwndStatus );
        if( custStatus ) {
            MSTR temp( text.c_str() ); // older versions of sdk need non-const
            custStatus->SetText( temp );
            ReleaseICustStatus( custStatus );
        }
    }
}

inline void EnableCustEdit( HWND hwndEdit, bool enable ) {
    if( hwndEdit ) {
        ICustEdit* custEdit = GetICustEdit( hwndEdit );
        if( custEdit ) {
            custEdit->Enable( enable );
            ReleaseICustEdit( custEdit );
        }
    }
}

inline void SetCustEditText( HWND hwndEdit, const frantic::tstring& text ) {
    if( hwndEdit ) {
        ICustEdit* custEdit = GetICustEdit( hwndEdit );
        if( custEdit ) {
#if MAX_VERSION_MAJOR > 11
            custEdit->SetText( text.c_str() );
#else
            MSTR temp( text.c_str() );
            custEdit->SetText( temp );
#endif
            ReleaseICustEdit( custEdit );
        }
    }
}

inline frantic::tstring GetCustEditText( HWND hwndEdit ) {
    if( hwndEdit ) {
        ICustEdit* custEdit = GetICustEdit( hwndEdit );
        if( custEdit ) {
            const TCHAR* result = 0;

#if MAX_VERSION_MAJOR > 10
            MSTR str;
            custEdit->GetText( str );
            result = str.data();
#else
            const std::size_t fixedBufferSize = 1024;
            char fixedBuffer[fixedBufferSize] = { 0 };
            std::vector<char> variableBuffer;
            custEdit->GetText( fixedBuffer, fixedBufferSize - 1 );

            if( strlen( fixedBuffer ) < fixedBufferSize - 2 ) {
                result = fixedBuffer;
            } else {
                // Keep trying to get the string, using an increasing buffer size.
                // Stop when the entire string (including its null terminator) fits
                // in the buffer, or when the buffer size reaches an arbitrary limit.
                std::size_t bufferSize = 32768;
                while( !result && bufferSize < 16777216 && bufferSize ) {
                    variableBuffer.resize( bufferSize );
                    memset( &variableBuffer[0], 0, variableBuffer.size() );
                    custEdit->GetText( &variableBuffer[0], static_cast<int>( variableBuffer.size() ) - 1 );
                    if( strlen( &variableBuffer[0] ) < variableBuffer.size() - 2 ) {
                        result = &variableBuffer[0];
                    }
                    bufferSize *= 2;
                }
            }
#endif
            ReleaseICustEdit( custEdit );
            if( result ) {
                return result;
            }
        }
    }
    return _T("");
}

#if MAX_VERSION_MAJOR > 10
inline void SetCustEditTooltip( HWND hwndEdit, bool enable, const frantic::tstring& text ) {
    if( hwndEdit ) {
        ICustEdit* custEdit = GetICustEdit( hwndEdit );
        if( custEdit ) {
            MSTR temp( text.c_str() );
            custEdit->SetTooltip( enable, temp );
            ReleaseICustEdit( custEdit );
        }
    }
}
#endif

inline void SetSpinnerValue( HWND hwndSpinner, int value, bool notify = false ) {
    if( hwndSpinner ) {
        ISpinnerControl* spinnerControl = GetISpinner( hwndSpinner );
        if( spinnerControl ) {
            spinnerControl->SetValue( value, notify ? TRUE : FALSE );
            ReleaseISpinner( spinnerControl );
        }
    }
}

inline bool is_visible( INode* node ) {
    if( !node || !node->Renderable() || !node->GetPrimaryVisibility() || node->IsNodeHidden( TRUE ) )
        return false;
    else
        return true;
}

inline bool is_network_render_server() {
    return GetCOREInterface()->IsNetworkRenderServer();
}

} // namespace max3d
} // namespace frantic
