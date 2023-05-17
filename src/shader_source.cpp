//
// Created by daiyan on 2023/5/10.
//


#if defined(PLATFORM_DESKTOP)// 330

const char *skybox_vs = R"(#version 330
in vec3 vertexPosition;
uniform mat4 matProjection;
uniform mat4 matView;
out vec3 fragPosition;
void main() {
    fragPosition = vertexPosition;
    mat4 rotView = mat4(mat3(matView));
    vec4 clipPos = matProjection*rotView*vec4(vertexPosition, 1.0);
    gl_Position = clipPos;
})";

const char *skybox_fs = R"(#version 330
in vec3 fragPosition;
uniform samplerCube environmentMap;
uniform bool vflipped;
uniform bool doGamma;
out vec4 finalColor;
void main() {
    vec3 color = vec3(0.0);
    if (vflipped) color = texture(environmentMap, vec3(fragPosition.x, -fragPosition.y, fragPosition.z)).rgb;
    else color = texture(environmentMap, fragPosition).rgb;
    if (doGamma) {
        color = color/(color + vec3(1.0));
        color = pow(color, vec3(1.0/2.2));
    }
    finalColor = vec4(color, 1.0);
})";

const char *cubemap_vs = R"(#version 330
in vec3 vertexPosition;
uniform mat4 matProjection;
uniform mat4 matView;
out vec3 fragPosition;
void main() {
    fragPosition = vertexPosition;
    gl_Position = matProjection*matView*vec4(vertexPosition, 1.0);
})";

const char *cubemap_fs = R"(#version 330
in vec3 fragPosition;
uniform sampler2D equirectangularMap;
out vec4 finalColor;
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;
    return uv;
}
void main() {
    vec2 uv = SampleSphericalMap(normalize(fragPosition));
    vec3 color = texture(equirectangularMap, uv).rgb;
    finalColor = vec4(color, 1.0);
})";

#else// 100

const char *skybox_vs = R"(#version 100
attribute vec3 vertexPosition;
uniform mat4 matProjection;
uniform mat4 matView;
varying vec3 fragPosition;
void main() {
    fragPosition = vertexPosition;
    mat4 rotView = mat4(mat3(matView));
    vec4 clipPos = matProjection*rotView*vec4(vertexPosition, 1.0);
    gl_Position = clipPos;
})";

const char *skybox_fs = R"(#version 100
precision mediump float;
varying vec3 fragPosition;
uniform samplerCube environmentMap;
uniform bool vflipped;
uniform bool doGamma;
void main() {
    vec4 texelColor = vec4(0.0);
    if (vflipped) texelColor = textureCube(environmentMap, vec3(fragPosition.x, -fragPosition.y, fragPosition.z));
    else texelColor = textureCube(environmentMap, fragPosition);
    vec3 color = vec3(texelColor.x, texelColor.y, texelColor.z);
    if (doGamma) {
        color = color/(color + vec3(1.0));
        color = pow(color, vec3(1.0/2.2));
    }
    gl_FragColor = vec4(color, 1.0);
})";

const char *cube_vs = R"(#version 100
attribute vec3 vertexPosition;
uniform mat4 matProjection;
uniform mat4 matView;
varying vec3 fragPosition;
void main() {
    fragPosition = vertexPosition;
    gl_Position = matProjection*matView*vec4(vertexPosition, 1.0);
})";

const char *cube_fs = R"(#version 100
precision mediump float;
varying vec3 fragPosition;
uniform sampler2D equirectangularMap;
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;
    return uv;
}
void main() {
    vec2 uv = SampleSphericalMap(normalize(fragPosition));
    vec3 color = texture2D(equirectangularMap, uv).rgb;
    gl_FragColor = vec4(color, 1.0);
})";

#endif
