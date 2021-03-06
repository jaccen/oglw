#pragma begin:vertex
#version 330

in vec2 position;
in vec2 uv;

uniform mat4 mvp;
uniform sampler2D tex;
uniform vec4 clipPlane;

out float fOffset;
out vec3 fPos;

void main() {
    float offset = texture(tex, uv * 2.0).r;
    vec3 pos = vec3(position, 4.0 * offset);
    gl_Position = mvp * vec4(pos, 1.0);
    fPos = pos;
    fOffset = offset;
    gl_ClipDistance[0] = dot(vec4(pos, 1.0), clipPlane);
}

#pragma end:vertex

#pragma begin:fragment
#version 330

out vec4 outColour;

in float fOffset;
in vec3 fPos;

uniform mat3 normalMatrix;
uniform mat4 modelView;
uniform vec3 lightPosition;

#define FOG_DENSITY 0.1
#define FOG_COLOR vec3(1.0)

float expFog(const float dist, const float density) {
    float d = density * dist;
    return exp2(d * d * -1.44);
}

void main(void) {
    vec3 p0 = dFdx(fPos);
    vec3 p1 = dFdy(fPos);
    vec3 n = normalMatrix * normalize(cross(p0, p1));

    vec3 surfaceColor = vec3(0.89,0.64, 0.26);
    float ambientIntensity = 0.2;

    vec3 surfaceToLight = normalize(modelView * vec4(lightPosition, 0.0)).xyz;
    vec3 surfaceToCamera = normalize(-modelView * vec4(fPos, 0.0)).xyz;

    vec3 ambient = ambientIntensity * surfaceColor;
    float diffuseCoefficient = max(0.0, dot(n, surfaceToLight));
    vec3 diffuse = diffuseCoefficient * surfaceColor;

    vec3 lightFactor = ambient + diffuse;

    outColour = vec4(lightFactor, 1.0);
    outColour.rgb = mix(outColour.rgb, FOG_COLOR, 1.0 - expFog(gl_FragCoord.z / gl_FragCoord.w, FOG_DENSITY));
    //outColour.rgb = pow(outColour.rgb, vec3(0.4545));
}

#pragma end:fragment
