#version 440 core // Identifies the version of the shader, this line must be on a separate line from the rest of the shader code

out vec4 color; // Our vec4 color variable containing r, g, b, a

uniform mat4 MVP; // Our uniform Model-View-Projection matrix to modify our position values

// this has a "samplerBuffer" type sampler; this limits us to texelFetch calls on it.
layout(binding = 1) uniform samplerBuffer positionBufferTexture;

// another samplerBuffer type; however, this contains signed ints, hence isamplerBuffer
layout(binding = 2) uniform isamplerBuffer triangleBufferTexture;

void main(void)
{
    int i = gl_VertexID;

    // Get the index of the position we will be using for this vertex.
    // Essentially means: Index = triangleBuffer[i]
    int Index = texelFetch( triangleBufferTexture, i ).r;

    // Then use that index to get a position.
    // Essentially means: vertexPosition = positionBuffer[Index]
    vec3 vertexPosition = texelFetch( positionBufferTexture, (Index) ).rgb;


    vec4 ClipSpace = MVP * vec4( vertexPosition.xyz, 1.0 );

    // Use the indicies to give it a bit of color to show off the general shape of things.
    color = vec4(float(Index) * .0075, 1.0, 1.0, 1.0);

    gl_Position = ClipSpace;
}													 

