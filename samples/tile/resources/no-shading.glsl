#pragma begin:vertex
#version 330

layout (location = 0) in vec3 position;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
}

#pragma end:vertex

#pragma begin:fragment
#version 330

out vec4 outColour;

void main(void) {
    outColour = vec4(1.0);
}

#pragma end:fragment
