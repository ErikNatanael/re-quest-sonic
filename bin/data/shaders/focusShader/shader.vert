#version 330

// these come from the programmable pipeline
uniform mat4 modelViewProjectionMatrix;
uniform mat4 textureMatrix;

uniform float displacement;

in vec4 position;


void main()
{
    vec4 modifiedPosition = modelViewProjectionMatrix * position;
	gl_Position = modifiedPosition;
}
