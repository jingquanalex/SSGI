//  Copyright (c) 2015, Ben Hopkins (kode80)
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification, 
//  are permitted provided that the following conditions are met:
//  
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  
//  2. Redistributions in binary form must reproduce the above copyright notice, 
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
//  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
//  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
//  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#version 450

in vec2 TexCoord;
in vec3 RayDirection;
in mat4 ProjectionTexture;

layout (location = 0) out vec4 outColor;

layout (std140, binding = 9) uniform MatCam
{
    mat4 projection;
	mat4 projectionInverse;
    mat4 view;
	mat4 viewInverse;
	mat4 kinectProjection;
	mat4 kinectProjectionInverse;
};

uniform vec2 bufferSize = vec2(1920, 1080);
uniform vec2 texelSize = 1.0 / vec2(1920, 1080);

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D inColor;
uniform sampler2D dsColor;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;


// Screen Space Raytracing, Morgan McGuire and Michael Mara
// http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
// Adapted in Screen Space Reflections in Unity 5, Ben Hopkins
// http://kode80.com/blog/2015/03/11/screen-space-reflections-in-unity-5/index.html

/**
    \param rayOrigin Camera-space ray origin, which must be 
    within the view volume and must have z < -0.01 and project within the valid screen rectangle

    \param rayDirection Unit length camera-space ray direction

    \param projectToPixelMatrix A projection matrix that maps to pixel coordinates (not [-1, +1] normalized device coordinates)

    \param csZBuffer The depth or camera-space Z buffer, depending on the value of \a csZBufferIsHyperbolic

    \param csZBufferSize Dimensions of csZBuffer

    \param rayZThickness Camera space thickness to ascribe to each pixel in the depth buffer
    
    \param csZBufferIsHyperbolic True if csZBuffer is an OpenGL depth buffer, false (faster) if
     csZBuffer contains (negative) "linear" camera space z values. Const so that the compiler can evaluate the branch based on it at compile time

    \param clipInfo See G3D::Camera documentation

    \param nearPlaneZ Negative number

    \param stride Step in horizontal or vertical pixels between samples. This is a float
     because integer math is slow on GPUs, but should be set to an integer >= 1

    \param jitter  Number between 0 and 1 for how far to bump the ray in stride units
      to conceal banding artifacts

    \param maxSteps Maximum number of iterations. Higher gives better images but may be slow

    \param maxRayTraceDistance Maximum camera-space distance to trace before returning a miss

    \param hitPixel Pixel coordinates of the first intersection with the scene

    \param hitPoint Camera space location of the ray hit

    Single-layer

 */
 
uniform float maxSteps = 100;
uniform float binarySearchSteps = 10;
uniform float maxRayTraceDistance = 0.1;
uniform float nearPlaneZ = -0.01;
uniform float rayZThickness = 0.002;
uniform float stride = 5;
uniform float strideZCutoff = 2.5;

uniform float screenEdgeFadeStart = 0.8;
uniform float cameraFadeStart = 0.01;
uniform float cameraFadeLength = 0.1;

float distanceSquared(vec2 a, vec2 b) { a -= b; return dot(a, a); }
void swap(inout float a, inout float b) { float t = a; a = b; b = t; }

bool intersectDepth(float zA, float zB, vec2 uv)
{
	float cameraZ = texelFetch(gPosition, ivec2(uv), 0).z;
    return zB <= cameraZ && zA >= cameraZ - rayZThickness;
}

bool traceSSRay(vec3 rayOrigin, vec3 rayDirection, float jitter, 
				out vec2 hitPixel, out vec3 hitPoint, out float steps)
{
    // Clip ray to a near plane in 3D (doesn't have to be *the* near plane, although that would be a good idea)
    float rayLength = (rayOrigin.z + rayDirection.z * maxRayTraceDistance) > nearPlaneZ ?
					  (nearPlaneZ - rayOrigin.z) / rayDirection.z : maxRayTraceDistance;

	vec3 rayEnd = rayOrigin + rayDirection * rayLength;

    // Project into screen space
    vec4 H0 = ProjectionTexture * vec4(rayOrigin, 1.0);
    vec4 H1 = ProjectionTexture * vec4(rayEnd, 1.0);
	
	H0.xy *= bufferSize;
	H1.xy *= bufferSize;

    // There are a lot of divisions by w that can be turned into multiplications
    // at some minor precision loss...and we need to interpolate these 1/w values
    // anyway.
    //
    // Because the caller was required to clip to the near plane,
    // this homogeneous division (projecting from 4D to 2D) is guaranteed 
    // to succeed. 
    float k0 = 1.0 / H0.w;
    float k1 = 1.0 / H1.w;
	
	// Switch the original points to values that interpolate linearly in 2D
    vec3 Q0 = rayOrigin * k0; 
    vec3 Q1 = rayEnd * k1;

	// Screen-space endpoints
    vec2 P0 = H0.xy * k0;
    vec2 P1 = H1.xy * k1;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += vec2((distanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0);

    vec2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to reduce
    // large branches later
    bool permute = false;
	if (abs(delta.x) < abs(delta.y))
	{
		// More-vertical line. Create a permutation that swaps x and y in the output
		permute = true;

        // Directly swizzle the inputs
		delta = delta.yx;
		P1 = P1.yx;
		P0 = P0.yx;
	}
    
	// From now on, "x" is the primary iteration direction and "y" is the secondary one
    float stepDirection = sign(delta.x);
    float invdx = stepDirection / delta.x;
	
    // Track the derivatives of Q and k
    vec3 dQ = (Q1 - Q0) * invdx;
    float dk = (k1 - k0) * invdx;
	vec2 dP = vec2(stepDirection, invdx * delta.y);
	
    // Scale derivatives by the desired pixel stride
	float strideScale = 1.0 - min(1.0, rayOrigin.z * strideZCutoff);
    float pixelStride = 1.0 + strideScale * stride;
	
	dP *= pixelStride;
	dQ *= pixelStride;
	dk *= pixelStride;

    // Offset the starting values by the jitter fraction
	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	float iter, zA = 0.0, zB = 0.0;
	
	// Track ray step and derivatives in a float4 to parallelize
	vec4 pqk = vec4(P0, Q0.z, k0);
	vec4 dPQK = vec4(dP, dQ.z, dk);
	bool intersect = false;
	
	for (iter = 0; iter < maxSteps && !intersect; iter++)
	{
		pqk += dPQK;
		
		zA = zB;
		zB = (dPQK.z * 0.5 + pqk.z) / (dPQK.w * 0.5 + pqk.w);
		if (zB > zA) swap(zB, zA);
		
		hitPixel = permute ? pqk.yx : pqk.xy;
		
		intersect = intersectDepth(zA, zB, hitPixel);
	}
	
	// Binary search refinement
	if (binarySearchSteps > 0.0 && pixelStride > 1.0 && intersect)
	{
		pqk -= dPQK;
		dPQK /= pixelStride;
		
		float originalStride = pixelStride * 0.5;
		float stride = originalStride;
		
		zA = pqk.z / pqk.w;
		zB = zA;
		
		for (float j = 0; j < binarySearchSteps; j++)
		{
			pqk += dPQK * stride;
			
			zA = zB;
			zB = (dPQK.z * -0.5 + pqk.z) / (dPQK.w * -0.5 + pqk.w);
			if (zB > zA) swap(zB, zA);
			
			hitPixel = permute ? pqk.yx : pqk.xy;
			
			originalStride *= 0.5;
			stride = intersectDepth(zA, zB, hitPixel) ? -originalStride : originalStride;
		}
	}

    Q0.xy += dQ.xy * iter;
	Q0.z = pqk.z;
	hitPoint = Q0 / pqk.w;
	steps = iter;
	
    return intersect;
}

float SSRayAlpha(float steps, float specularStrength, vec2 hitPixel, vec3 hitPoint, 
	vec3 vsRayOrigin, vec3 vsRayDirection)
{
	float alpha = min(1.0, specularStrength * 1.0);
	
	// Fade ray hits that approach the maximum iterations
	alpha *= 1.0 - pow(steps / maxSteps, 20);
	
	// Fade ray hits that approach the screen edge
	float screenFade = screenEdgeFadeStart;
	vec2 hitPixelNDC = hitPixel * 2.0 - 1.0;
	float maxDimension = min(1.0, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
	alpha *= 1.0 - (max(0.0, maxDimension - screenFade) / (1.0 - screenFade));
	
	// Fade ray hits base on how much they face the camera
	float eyeFadeStart = cameraFadeStart;
	float eyeFadeEnd = cameraFadeStart + cameraFadeLength;
	eyeFadeEnd = min(1.0, eyeFadeEnd);
	
	float eyeDirection = clamp(vsRayDirection.z, eyeFadeStart, eyeFadeEnd);
	alpha *= 1.0 - ((eyeDirection - eyeFadeStart) / (eyeFadeEnd - eyeFadeStart));
	
	// Fade ray hits based on distance from ray origin
	//alpha *= 1.0 - clamp(distance(vsRayOrigin, hitPoint) / maxRayTraceDistance, 0.0, 1.0);
	
	return alpha;
}


// Main

void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
    vec4 color = texture(inColor, TexCoord);
	
	vec3 finalColor = vec3(0);
	
	// Screen space reflections
	vec3 rayOrigin = position;
	vec3 rayDirection = normalize(reflect(normalize(position), normalize(normal)));
	
	vec2 hitPixel;
	vec3 hitPoint;
	float steps;
	
	vec2 uv2 = TexCoord * bufferSize;
	float c = (uv2.x + uv2.y) * 0.25;
	float jitter = mod(c, 1.0);
	
	bool intersect = traceSSRay(rayOrigin, rayDirection, jitter, hitPixel, hitPoint, steps);
	float alpha = 0.0;
	
	if (intersect)
	{
		hitPixel = hitPixel * texelSize;
		alpha = SSRayAlpha(steps, 1.0, hitPixel, hitPoint, rayOrigin, rayDirection);
		
		vec3 irradiance = texture(prefilterMap, mat3(viewInverse) * rayDirection).rgb;
		vec3 reflectedColor = texture(inColor, hitPixel).rgb;
		//if (hitPoint.z > position.z) reflectedColor = texture(dsColor, hitPixel).rgb;
		
		
		finalColor.rgb = reflectedColor;
		//finalColor.rgb = mix(color.rgb, reflectedColor, alpha);
		//finalColor.rgb = vec3(irradiance);
	}
	
	outColor = vec4(finalColor, alpha);
}