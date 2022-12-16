// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <limits>
#include <vector>

#include <frantic/max3d/iostream.hpp>
#include <frantic/max3d/mesh.hpp>

namespace frantic {
namespace max3d {

inline int closestVert( Point3 vertPos, Mesh& mesh, Matrix3 meshTM ) {
    float closestDist = 9999999.9f;
    int closestVertIndex = 0;
    for( int v = 0; v < mesh.numVerts; ++v ) {
        Point3 currV = mesh.verts[v];
        currV = meshTM.PointTransform( currV );

        float currD = FLength( vertPos - currV );

        if( currD < closestDist ) {
            closestVertIndex = v;
            closestDist = currD;
        }
    }
    return closestVertIndex;
}

inline void copy_map_channel_slow( Mesh& sMesh, Matrix3& sTrans, int sMap, Mesh& dMesh, Matrix3& dTrans, int dMap ) {
    using namespace std;

    if( !sMesh.mapSupport( sMap ) || sMesh.getNumMapVerts( sMap ) == 0 ) {
        dMesh.setMapSupport( dMap, FALSE );
        dMesh.freeMapVerts( dMap );
        dMesh.freeMapFaces( dMap );
        return;
    }

    // maps regular vertices to one of the vertex colors
    vector<int> vcv;
    vcv.resize( sMesh.numVerts );

    for( int f = 0; f < sMesh.numFaces; f++ ) {
        vcv[sMesh.faces[f].v[0]] = sMesh.mapFaces( sMap )[f].t[0];
        vcv[sMesh.faces[f].v[1]] = sMesh.mapFaces( sMap )[f].t[1];
        vcv[sMesh.faces[f].v[2]] = sMesh.mapFaces( sMap )[f].t[2];
    }

    // BuildMeshFromShape(t, mc, os, tri->GetMesh());
    dMesh.setMapSupport( dMap, TRUE );
    dMesh.freeMapVerts( dMap );
    dMesh.freeMapFaces( dMap );
    dMesh.setNumMapVerts( dMap, dMesh.numVerts );
    dMesh.setNumMapFaces( dMap, dMesh.numFaces );
    for( int i = 0; i < dMesh.getNumFaces(); i++ ) {
        dMesh.mapFaces( dMap )[i].setTVerts( dMesh.faces[i].getAllVerts() );
    }

    VertColor* dVertCol = dMesh.mapVerts( dMap );
    VertColor* sVertCol = sMesh.mapVerts( sMap );
    for( i = 0; i < dMesh.getNumMapVerts( dMap ); i++ ) {
        Point3 vPos = dMesh.verts[i];
        vPos = dTrans.PointTransform( vPos );
        int closest = closestVert( vPos, sMesh, sTrans );

        dVertCol[i] = sVertCol[vcv[closest]];
    }
}

#ifdef FRANTIC_USING_DOTNET

// TODO: Should switch this to use the C++ kd-tree
inline void copy_map_channel( Mesh& sMesh, Matrix3& sTrans, int sMap, Mesh& dMesh, Matrix3& dTrans, int dMap ) {
    using namespace std;
    using namespace Exocortex::Graphics;
    using namespace Exocortex::Graphics::KDTrees;
    using namespace Exocortex::Graphics::Meshes;

    // If the source does not support the map channel, then make sure the destination mesh does not either
    if( !sMesh.mapSupport( sMap ) || sMesh.getNumMapVerts( sMap ) == 0 ) {
        dMesh.setMapSupport( dMap, FALSE );
        dMesh.freeMapVerts( dMap );
        dMesh.freeMapFaces( dMap );
        return;
    }

    Matrix3 sTransInverse = sTrans;
    sTransInverse.Invert();
    Matrix3 dTos = dTrans * sTransInverse;

    // Initialize the destination map channel
    dMesh.setMapSupport( dMap, TRUE );
    dMesh.freeMapVerts( dMap );
    dMesh.freeMapFaces( dMap );
    dMesh.setNumMapVerts( dMap, dMesh.numVerts );
    dMesh.setNumMapFaces( dMap, dMesh.numFaces );
    for( int i = 0; i < dMesh.getNumFaces(); i++ ) {
        dMesh.mapFaces( dMap )[i].setTVerts( dMesh.faces[i].getAllVerts() );
    }

    // Create the acceleration structure to get the nearest points on the mesh
    KDTree* sKD = new KDTree( max3d::to_TriMesh3( sMesh ) );

    VertColor* dVertCol = dMesh.mapVerts( dMap );
    VertColor* sVertCol = sMesh.mapVerts( sMap );
    for( i = 0; i < dMesh.getNumMapVerts( dMap ); i++ ) {
        Point3 vPos = dMesh.verts[i];
        vPos = dTos.PointTransform( vPos );

        TriMesh3Intersection* iSect =
            sKD->FindNearestPoint( max3d::to_Vector3F( vPos ), ( numeric_limits<float>::max )() );

        DWORD* sVerts = sMesh.mapFaces( sMap )[iSect->FaceIndex].t;
        IPoint3 face = IPoint3( sVerts[0], sVerts[1], sVerts[2] );

        Point3 vColA = sVertCol[face.x], vColB = sVertCol[face.y], vColC = sVertCol[face.z];

        Point3 bary = max3d::to_Point3( iSect->BarycentricCoords );

        dVertCol[i] = bary.x * vColA + bary.y * vColB + bary.z * vColC;
    }
}

#endif

} // namespace max3d
} // namespace frantic
