// By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause
//
// Copyright (c) 2014, Morgan McGuire and Michael Mara
// All rights reserved.
//
// From McGuire and Mara, Efficient GPU Screen-Space Ray Tracing,
// Journal of Computer Graphics Techniques, 2014
//
// This software is open source under the "BSD 2-clause license":
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the following
// conditions are met:
//
// 1. Redistributions of source code must retain the above
// copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials provided
// with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.

#version 450

in vec2 TexCoord;
in vec3 RayDirection;
in mat4 ProjectionTexture;

layout (location = 0) out vec4 outReflection;
layout (location = 1) out vec4 outReflectionRay;
layout (location = 2) out vec4 outAmbientOcclusion;

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


// Screen Space Raytracing, Morgan McGuire and Michael Mara
// http://casual-effects.blogspot.com/2014/08/screen-space-ray-tracing.html
// Modified with improvements from Ben Hopkins
// http://kode80.com/blog/2015/03/11/screen-space-reflections-in-unity-5/index.html
// and Will Pearce
// http://roar11.com/2015/07/screen-space-glossy-reflections/

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
uniform float maxRayTraceDistance = 0.2;
uniform float nearPlaneZ = -0.01;
uniform float rayZThickness = 0.002;
uniform float stride = 3;
uniform float strideZCutoff = 2.5;
uniform float jitterFactor = 0.5;

uniform float screenEdgeFadeStart = 0.8;
uniform float cameraFadeStart = 0.01;
uniform float cameraFadeLength = 0.1;

float distanceSquared(vec2 a, vec2 b) { a -= b; return dot(a, a); }
void swap(inout float a, inout float b) { float t = a; a = b; b = t; }

bool intersectZ(float rayZMin, float rayZMax, float sceneZMax)
{
	return (rayZMax >= sceneZMax - rayZThickness) && (rayZMin <= sceneZMax) && (sceneZMax != 0.0);
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
	
	vec2 dP = vec2(stepDirection, invdx * delta.y);
    vec3 dQ = (Q1 - Q0) * invdx; // Track the derivatives of Q and k
    float dk = (k1 - k0) * invdx;
	
    // Scale derivatives by the desired pixel stride
	float strideScale = 1.0 - min(1.0, rayOrigin.z * strideZCutoff);
    float pixelStride = 1.0 + strideScale * stride;
	if (strideZCutoff == 0.0) pixelStride = stride;
	
	dP *= pixelStride;
	dQ *= pixelStride;
	dk *= pixelStride;

    // Offset the starting values by the jitter fraction
	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;
	
	// Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, and k from k0 to k1
    vec3 Q = Q0;
    vec4 PQk = vec4(P0, Q0.z, k0);
    vec4 dPQk = vec4(dP, dQ.z, dk);
	/*PQk -= dPQk;
	dPQk /= 2;
	PQk += dPQk;*/
	
	// We track the ray depth at +/- 1/2 pixel to treat pixels as clip-space solid 
	// voxels. Because the depth at -1/2 for a given pixel will be the same as at 
	// +1/2 for the previous iteration, we actually only have to compute one value 
	// per iteration.
	float prevZMaxEstimate = rayOrigin.z;
    float stepCount = 0.0;
    float rayZMax = prevZMaxEstimate;
	float rayZMin = prevZMaxEstimate;
    float sceneZMax = rayZMax + 1e4;

    // P1.x is never modified after this point, so pre-scale it by 
    // the step direction for a signed comparison
    float end = P1.x * stepDirection;

	// Ray-depth intersection test
	bool intersect = false;
	
    // We only advance the z field of Q in the inner loop, since
    // Q.xy is never used until after the loop terminates.
	for (;
		(PQk.x * stepDirection) <= end && 
		(stepCount < maxSteps) && !intersect;
        PQk += dPQk, stepCount++)
	{
		hitPixel = permute ? PQk.yx : PQk.xy;

        // The depth range that the ray covers within this loop
        // iteration.  Assume that the ray is moving in increasing z
        // and swap if backwards.  Because one end of the interval is
        // shared between adjacent iterations, we track the previous
        // value and then swap as needed to ensure correct ordering
        rayZMin = prevZMaxEstimate;

        // Compute the value at 1/2 pixel into the future
        rayZMax = (dPQk.z * 0.5 + PQk.z) / (dPQk.w * 0.5 + PQk.w);
		prevZMaxEstimate = rayZMax;
        if (rayZMin > rayZMax) { swap(rayZMin, rayZMax); }

        // Camera-space z of the background
        sceneZMax = texelFetch(gPosition, ivec2(hitPixel), 0).z;

		intersect = intersectZ(rayZMin, rayZMax, sceneZMax);
    } // pixel on ray
	
	// Binary search refinement
	if (binarySearchSteps > 0.0 && pixelStride > 1.0 && intersect)
	{
		// Step back one step and half the delta
		PQk -= dPQk;
		dPQk /= 2;
		
		float intersectCount = 0;
		prevZMaxEstimate = PQk.z / PQk.w;
		
		// Step forward until hit, then step back and half the delta
		for (float i = 0; i < binarySearchSteps; PQk += dPQk, i++)
		{
			hitPixel = permute ? PQk.yx : PQk.xy;
			
			rayZMin = prevZMaxEstimate;
			rayZMax = (dPQk.z * 0.5 + PQk.z) / (dPQk.w * 0.5 + PQk.w);
			prevZMaxEstimate = rayZMax;
			if (rayZMin > rayZMax) { swap(rayZMin, rayZMax); }
		
			sceneZMax = texelFetch(gPosition, ivec2(hitPixel), 0).z;
			
			bool newIntersect = intersectZ(rayZMin, rayZMax, sceneZMax);
			if (newIntersect)
			{
				PQk -= dPQk;
				dPQk /= 2;
			}
		}
	}

    Q.xy += dQ.xy * stepCount;
	hitPoint = Q / PQk.w;
	hitPixel = hitPixel * texelSize;

    // Matches the new loop condition:
    return intersect;
}

float SSRayAlpha(float steps, float specularStrength, vec2 hitCoord, vec3 hitPoint, 
	vec3 vsRayOrigin, vec3 vsRayDirection)
{
	float alpha = min(1.0, specularStrength * 1.0);
	
	// Fade ray hits that approach the maximum iterations
	alpha *= 1.0 - pow(steps / maxSteps, 20);
	
	// Fade ray hits that approach the screen edge
	float screenFade = screenEdgeFadeStart;
	vec2 hitCoordNDC = hitCoord * 2.0 - 1.0;
	float maxDimension = min(1.0, max(abs(hitCoordNDC.x), abs(hitCoordNDC.y)));
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

uniform float roughness = 0.99;
uniform sampler2D inColor;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to normal's space
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------

// Main

void main()
{
	vec3 position = texture(gPosition, TexCoord).xyz;
    vec3 normal = texture(gNormal, TexCoord).xyz;
	
	// Screen space reflections
	vec3 rayOrigin = position;
	vec3 rayDirection = normalize(reflect(normalize(position), normalize(normal)));
	
	vec2 hitCoord = vec2(0);
	vec3 hitPoint = vec3(0);
	float steps;
	
	float rand = fract(sin(dot(TexCoord, vec2(12.9898, 78.233))) * 43758.5453);
	float jitter = 1.0 + (rand - 0.5) * jitterFactor;
	//if (stride == 1) jitter = 1;
	
	bool intersect = traceSSRay(rayOrigin, rayDirection, jitter, hitCoord, hitPoint, steps);
	float alpha = SSRayAlpha(steps, 1.0, hitCoord, hitPoint, rayOrigin, rayDirection);
	alpha = alpha * float(intersect);
	
	vec3 color = texture(inColor, TexCoord).rgb;
	vec3 reflectedColor = texture(inColor, hitCoord).rgb;
	
	/*const float MAX_REFLECTION_LOD = 5.0;
	vec3 reflectedNormal = texture(gNormal, hitCoord).rgb;
	vec3 rN = mat3(viewInverse) * reflectedNormal;
    vec3 prefilteredColor = textureLod(prefilterMap, rN, 0.24 * MAX_REFLECTION_LOD).rgb;
	
	outReflection = vec4(mix((color+prefilteredColor)/2, prefilteredColor, alpha), alpha);*/
	outReflection = vec4(reflectedColor, alpha);
	outReflectionRay = vec4(hitCoord, distance(position, hitPoint), float(intersect));
	
	// Ambient Occlusion
	// Cast cosine distributed samples on a hemisphere for each fragment
	const uint SAMPLE_COUNT = 0;
	vec3 totalColor = vec3(0);
    float totalWeight = 0.0;
	vec3 N = normal;
    
    for(uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        
		intersect = traceSSRay(rayOrigin, H, jitter, hitCoord, hitPoint, steps);
		
		// No occlusion contribution from rays facing camera
		vec3 rayDir = normalize(hitPoint - position);
		if (dot(normalize(position), rayDir) < -0.99) intersect = false;
		
		if (intersect)
		{
			totalColor += texture(inColor, hitCoord).rgb;
			totalWeight += 1;
		}
		else
		{
			vec3 worldH = (viewInverse * vec4(H, 1)).xyz;
			vec3 irradiance = texture(irradianceMap, worldH).rgb;
			totalColor += irradiance;
		}
    }

	totalColor = totalColor / SAMPLE_COUNT;
    float ao = 1.0 - totalWeight / SAMPLE_COUNT;
	vec3 aoColor = totalColor * ao;
	//ao = ao * alpha;

	outAmbientOcclusion = vec4(vec3(aoColor), float(intersect));
}