#pragma begin:vertex
#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat4 depthMVP;
uniform mat3 normalMat;

out vec4 shadowCoord;
out vec3 n;
out vec4 pos;

void main() {
    pos = mvp * vec4(position, 1.0);
    gl_Position = pos;
    shadowCoord = depthMVP * vec4(position, 1);

    // FIXME : add normals to plane
    if (normal == vec3(0.0)) {
        n = vec3(0.0);
    } else {
        n = normalMat * normal;
    }
}

#pragma end:vertex

#pragma begin:fragment
#version 330

uniform sampler2D depthMap;
uniform vec3 lightPos;

out vec4 outColour;
in vec4 shadowCoord;
in vec3 n;
in vec4 pos;

float visibility(vec4 lightSpace) {
    vec3 shadowPosition = lightSpace.xyz / lightSpace.w;
    vec2 uvs;
    uvs.x = 0.5 * shadowPosition.x + 0.5;
    uvs.y = 0.5 * shadowPosition.y + 0.5;
    float z = 0.5 * shadowPosition.z + 0.5;
    float depth = texture(depthMap, uvs).x;

    float bias = 0.0025;
    if (depth < (z - bias)) {
        return 0.0;
    } else {
        return 1.0;
    }
}

void main(void) {
    vec3 surfaceToLight = normalize(lightPos - pos.xyz);

    float D = max(dot(n, surfaceToLight), 0.0);
    if (n == vec3(0.0)) {
        D = 1.0;
    }
    float shadow = visibility(shadowCoord);
    if (shadow == 0.0) {
        outColour = vec4(vec3(0.5) * vec3(D), 1.0);
    } else {
        outColour = vec4(vec3(D) * visibility(shadowCoord), 1.0);
    }
    //outColour = vec4(shadowCoord.xy, 0.0, 1.0);
    //outColour = vec4(vec3(shadowCoord.z), 1.0);
}

#pragma end:fragment
