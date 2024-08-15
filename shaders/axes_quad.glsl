#version 150

// Copyright 2015, Christopher J. Foster and the other displaz contributors.
// Use of this code is governed by the BSD-style license found in LICENSE.txt

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;

uniform sampler2D texture0;

//------------------------------------------------------------------------------
#if defined(VERTEX_SHADER)

in vec2 position;
in vec2 texCoord;

out vec2 textureCoords;

void main()
{
    textureCoords = texCoord;
    gl_Position = modelViewProjectionMatrix * vec4(position,0.0,1.0);
}


//------------------------------------------------------------------------------
#elif defined(FRAGMENT_SHADER)

// Input texture coordinates
in vec2 textureCoords;

// Output fragment color
out vec4 fragColor;

void main()
{
    // Trivial fragment shader reads from texture
    vec4 tex = texture(texture0, vec2(textureCoords));
    fragColor = tex;
}

#endif

