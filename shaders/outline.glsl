#version 150
// Copyright 2015, Christopher J. Foster and the other displaz contributors.
// Use of this code is governed by the BSD-style license found in LICENSE.txt

// Mostly has an effect on the outline distance.
const float SampleDistance = 1.0;

// Controls the intensity of the effect based on the difference in depth values, also introduces a 'gradient' effect that bleeds from the outline.
uniform float Exponent = 1.0;       //# uiname=Exponent; min=0.01; max=3.0

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform vec2 widthHeight;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

//------------------------------------------------------------------------------
#if defined(VERTEX_SHADER)

in vec2 position;
in vec2 texCoord;

// Point color which will be picked up by the fragment shader
out vec2 textureCoords;

void main()
{
    textureCoords = texCoord;
    gl_Position = vec4(position, 0.0, 1.0);
}


//------------------------------------------------------------------------------
#elif defined(FRAGMENT_SHADER)

// Input texture coordinates
in vec2 textureCoords;

// Output fragment color
out vec4 fragColor;

// Convert depth buffer value to a linear range of 0.0 to 1.0.
float Linear01Depth(vec4 depthVal)
{
    // These aren't likely to change so we can set them as constants.
    const float near = 0.05;
    const float far = 500000.0;

    return (2.0 * near) / (far + near - depthVal.x * (far - near));
}

void main()
{
    // inspired by borderlands implementation of popular "sobel filter"
    float centerDepth = Linear01Depth(texture(depthTexture, textureCoords));
    vec4 depthsDiag;
    vec4 depthsAxis;

    vec2 uvDist = SampleDistance * vec2(1.0 / widthHeight.x, 1.0 / widthHeight.y);

    depthsDiag.x = Linear01Depth(texture(depthTexture, textureCoords + uvDist)); // TR
    depthsDiag.y = Linear01Depth(texture(depthTexture, textureCoords + uvDist * vec2(-1,1))); // TL
    depthsDiag.z = Linear01Depth(texture(depthTexture, textureCoords - uvDist * vec2(-1,1))); // BR
    depthsDiag.w = Linear01Depth(texture(depthTexture, textureCoords - uvDist)); // BL

    depthsAxis.x = Linear01Depth(texture(depthTexture, textureCoords + uvDist * vec2(0,1))); // T
    depthsAxis.y = Linear01Depth(texture(depthTexture, textureCoords - uvDist * vec2(1,0))); // L
    depthsAxis.z = Linear01Depth(texture(depthTexture, textureCoords + uvDist * vec2(1,0))); // R
    depthsAxis.w = Linear01Depth(texture(depthTexture, textureCoords - uvDist * vec2(0,1))); // B

    depthsDiag -= centerDepth;
    depthsAxis /= centerDepth;

    const vec4 HorizDiagCoeff = vec4(1,1,-1,-1);
    const vec4 VertDiagCoeff = vec4(-1,1,-1,1);
    const vec4 HorizAxisCoeff = vec4(1,0,0,-1);
    const vec4 VertAxisCoeff = vec4(0,1,-1,0);

    vec4 SobelH = depthsDiag * HorizDiagCoeff + depthsAxis * HorizAxisCoeff;
    vec4 SobelV = depthsDiag * VertDiagCoeff + depthsAxis * VertAxisCoeff;

    float SobelX = dot(SobelH, vec4(1,1,1,1));
    float SobelY = dot(SobelV, vec4(1,1,1,1));
    float Sobel = sqrt(SobelX * SobelX + SobelY * SobelY);

    Sobel = 1.0 - pow(clamp(Sobel, 0.0, 1.0), Exponent);
    fragColor = vec4(Sobel * texture(screenTexture, textureCoords).xyz, 1.0);
}

#endif



