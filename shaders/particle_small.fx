////////////////////////////////
//	 	Per-Object Data	      //
////////////////////////////////

//Data in video memory that exists only once and is used by all particles
cbuffer cbPerObject {
	//The world transform matrix
	float4x4 World 		   : WORLD;

	//World * View * Proj
	float4x4 WorldViewProj : WORLDVIEWPROJ;

	//The projection transform matrix
	float4x4 Projection    : PROJECTION;

	//World * View
	float4x4 WorldView 	   : WORLDVIEW;

	//The location of the camera in world space
	float3 Camera		   : WORLD_CAMERA_POSITION;
}

//Global
float VIEWPORT_FOV;

////////////////////////////////
//  Data Transfer Structures  //
////////////////////////////////

//Used as input for a vertex shader
//Represents per-particle data from the particle buffers
struct VS_IN {
	float3 pos 		: POSITION;
	float4 color 	: COLOR;
};

//Used as output for the vertex shader and input to pixel and geometry sahders
struct VS_OUT {
	float4 pos 		: SV_POSITION;
	float4 color 	: COLOR;
	float4 size     : PSIZE;
};

///////////////////////////////////////////
//   Large Particle Perspective Shaders  //
///////////////////////////////////////////

//Vertex Shader
//Transforms the particle coordinates by world and view.
//We can't transform by projection until the geometry shader as
//the particles must be expanded to quads in view space.
//Calculates the distance between the particle and the camera.
VS_OUT vs_large_perspec_main( VS_IN IN ) {
	VS_OUT Out;
	Out.pos  = mul( float4( IN.pos, 1 ), WorldView );
	Out.size = distance( float4( Camera, 1), mul( float4( IN.pos, 1), World ) );
	Out.color = IN.color;
	return Out;
}

//Geometry Shader
//Expands a particle into a quad centered at the particle's location
//The size of the quads is based on the distance to the camera calculated
//in the vertex shader to keep the quads a consistent size regardless of
//how close the camera is
//We do not have to worry about the direction the quads are facing as
//the input particle coordinates were already multiplied by the view matrix
//in the vertex shader, and they are expanded only along the x and y axies
//so they will always face the camera
//The quad's vertices are then multiplied by the projection matrix
[maxvertexcount(4)]
void gs_large_persp( point VS_OUT input[1], inout TriangleStream<VS_OUT> triStream ) {
	VS_OUT Out;
	Out.color = input[0].color;
	Out.size = 0.0f;
	float4 center = input[0].pos;
	//The following line ensures that the quad remains constant size despite changes to the viewport FOV (See Dolly Zoom)
	float change = (2 * tan(0.5 * VIEWPORT_FOV) * input[0].size) / 1024.0f;
	float xChange = change;
	float yChange = change;

	Out.pos = mul(float4( center.x - xChange, center.y + yChange, center.z, center.w ), Projection );
	triStream.Append( Out );

	Out.pos = mul(float4( center.x + xChange, center.y + yChange, center.z, center.w ), Projection );
	triStream.Append( Out );

	Out.pos = mul(float4( center.x - xChange, center.y - yChange, center.z, center.w ), Projection );
	triStream.Append( Out );

	Out.pos = mul(float4( center.x + xChange, center.y - yChange, center.z, center.w ), Projection );
	triStream.Append( Out );

	triStream.RestartStrip();
}

///////////////////////////////////////////
//  Large Particle Orthographic Shaders  //
///////////////////////////////////////////

//Vertex Shader
VS_OUT vs_large_ortho_main( VS_IN IN ) {
	VS_OUT Out;
	Out.pos  = mul( float4( IN.pos, 1 ), WorldViewProj );
	Out.size = 0;
	Out.color = IN.color;
	return Out;
}


//Geometry Shader
[maxvertexcount(4)]
void gs_large_ortho( point VS_OUT input[1], inout TriangleStream<VS_OUT> triStream ) {
	VS_OUT Out;
	Out.color = input[0].color;
	Out.size = 0.0f;
	float4 center = input[0].pos;
	float change = 0.006f;
	float xChange = change * 0.5f;
	float yChange = change;

	Out.pos = float4( center.x - xChange, center.y + yChange, center.z, center.w );
	triStream.Append( Out );

	Out.pos = float4( center.x + xChange, center.y + yChange, center.z, center.w );
	triStream.Append( Out );

	Out.pos = float4( center.x - xChange, center.y - yChange, center.z, center.w );
	triStream.Append( Out );

	Out.pos = float4( center.x + xChange, center.y - yChange, center.z, center.w );
	triStream.Append( Out );

	triStream.RestartStrip();
}

////////////////////////////////
//	 Small Particle Shaders	  //
////////////////////////////////

//Vertex Shader
//Unlike large particles we don't need to do any further per-vertex processing
//after the vertex shader, so we can multiply by the combined transformation matrix
//to speed things up
VS_OUT vs_small_main( VS_IN IN ) {
	VS_OUT Out;
	Out.pos  = mul( float4( IN.pos, 1 ), WorldViewProj );
	Out.size = 4.0f;
	Out.color = IN.color;
	return Out;
}

////////////////////////////////
//		 Shared Shaders		  //
////////////////////////////////

//Pixel Shader
//A basic pixel shader that simply passes through the particle colour
float4 ps_main( VS_OUT IN ) : SV_TARGET {
	return IN.color;
}

////////////////////////////////
//		  Techniques		  //
////////////////////////////////

//These define the actual DirectX 11 techniques we can specify particles be rendered with.
//The directx rendering pipeline is processed in the order Vertex Shader -> Geometry Shader -> Pixel Shader.
//Shaders are small programs (written in HLSL for DirectX) that are executed on the GPU.
//Vertex shaders are for per-vertex processing, geometry shaders (optional) are for modifying geometry
//and pixel shaders are for per-pixel processing. These shaders are all compiled for DirectX 11 (Windows 7+, shader model 5).

technique11 ParticleLargePerspectiveShader {
	pass P0 {
		SetVertexShader( CompileShader( vs_5_0, vs_large_perspec_main() ) );
		SetGeometryShader( CompileShader( gs_5_0, gs_large_persp() ) );
		SetPixelShader( CompileShader( ps_5_0, ps_main() ) );
	}
}

technique11 ParticleLargeOrthographicShader {
	pass P0 {
		SetVertexShader( CompileShader( vs_5_0, vs_large_ortho_main() ) );
		SetGeometryShader( CompileShader( gs_5_0, gs_large_ortho() ) );
		SetPixelShader( CompileShader( ps_5_0, ps_main() ) );
	}
}

technique11 ParticleSmallShader {
	pass P0 {
		SetVertexShader( CompileShader( vs_5_0, vs_small_main() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, ps_main() ) );
	}
}