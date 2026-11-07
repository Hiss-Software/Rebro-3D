#pragma once

inline const char* kVertexShader = R"(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vNormal;
out vec2 vUV;
out vec3 vWorldPos;

void main() {
    // Normal matrix: correct for non-uniform scaling
    mat3 nrm = mat3(transpose(inverse(uModel)));
    vNormal = nrm * aNormal;
    vUV = aUV;
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

inline const char* kFragmentShader = R"(
#version 460 core
in vec3 vNormal;
in vec2 vUV;
in vec3 vWorldPos;

uniform int uFlat;
uniform int uUseTex;
uniform vec3 uColor;
uniform vec3 uCamPos;
uniform sampler2D uTexture;

out vec4 FragColor;

// Key (warm) and fill (cool) light sources
const vec3 L1_DIR = normalize(vec3( 0.5, 0.8, 0.4));
const vec3 L1_COL = vec3(1.0, 0.96, 0.88);
const vec3 L2_DIR = normalize(vec3(-0.6, 0.2, -0.5));
const vec3 L2_COL = vec3(0.45, 0.55, 0.70);

void main() {
    vec3 base = uColor;
    if (uUseTex == 1)
        base *= texture(uTexture, vUV).rgb;

    if (uFlat == 1) {
        FragColor = vec4(base, 1.0);
        return;
    }

    vec3 n = normalize(vNormal);
    // fallback in case uCamPos is not set
    vec3 camPos = length(uCamPos) < 1e-3 ? vec3(0.0, 0.0, 10.0) : uCamPos;
    vec3 v = normalize(camPos - vWorldPos);

    // two-sided lighting: bottom of the plane does not turn black
    if (dot(n, v) < 0.0) n = -n;

    // hemispheric ambient: sky above, ground below
    float upness = n.y * 0.5 + 0.5;
    vec3 ambient = mix(vec3(0.26, 0.26, 0.30), vec3(0.42, 0.42, 0.46), upness);

    vec3 lit = ambient * base;
    lit += base * max(dot(n, L1_DIR), 0.0) * L1_COL * 0.85;
    lit += base * max(dot(n, L2_DIR), 0.0) * L2_COL * 0.30;

    // Blinn-Phong specular
    vec3 h1 = normalize(L1_DIR + v);
    lit += L1_COL * pow(max(dot(n, h1), 0.0), 48.0) * 0.40;
    vec3 h2 = normalize(L2_DIR + v);
    lit += L2_COL * pow(max(dot(n, h2), 0.0), 24.0) * 0.10;

    // subtle rim light
    float rim = pow(1.0 - clamp(dot(n, v), 0.0, 1.0), 3.0);
    lit += rim * vec3(0.08, 0.09, 0.11);

    // gamma correction
    FragColor = vec4(pow(clamp(lit, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}
)";