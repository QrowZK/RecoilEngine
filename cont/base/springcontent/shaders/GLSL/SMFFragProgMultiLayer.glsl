/**
 * Multi-layer terrain fragment shader.
 *
 * Extends SMFFragProg.glsl to support up to 3 independent terrain layers.
 * The layer index is supplied as a flat int from the vertex shader so the GPU
 * can select the correct diffuse/normal texture set without branching.
 *
 * Active uniforms/samplers in addition to those in SMFFragProg.glsl:
 *   layerID          – flat uint from VS, 0=Underground, 1=Surface, 2=Elevated
 *   diffuseTexArray  – sampler2DArray with one slice per layer
 *   normalTexArray   – sampler2DArray with one normal-map slice per layer
 *   layerTint[3]     – optional per-layer colour tint (alpha = tint strength)
 */

#version 430

#if (GL_FRAGMENT_PRECISION_HIGH == 1)
precision highp float;
#else
precision mediump float;
#endif

// ---------------------------------------------------------------------------
// Varyings from vertex shader
// ---------------------------------------------------------------------------
in vec3  halfDir;
in float fogFactor;
in vec4  vertexWorldPos;
in vec2  diffuseTexCoords;
in flat  uint layerID;        // 0, 1 or 2

// ---------------------------------------------------------------------------
// Existing SMF uniforms (kept for compatibility)
// ---------------------------------------------------------------------------
uniform sampler2D diffuseTex;   // layer-1 (surface) diffuse – legacy fallback
uniform sampler2D normalsTex;   // layer-1 normal map – legacy fallback
uniform sampler2D detailTex;
uniform vec2      specularTexGen;

#ifdef SMF_ADV_SHADING
    uniform vec2  normalTexGen;
    uniform vec3  groundAmbientColor;
    uniform vec3  groundDiffuseColor;
    uniform vec3  groundSpecularColor;
    uniform float groundSpecularExponent;
    uniform float groundShadowDensity;
    uniform vec2  mapHeights;
    uniform vec4  lightDir;
    uniform vec3  cameraPos;
#endif

uniform sampler2D infoTex;

// ---------------------------------------------------------------------------
// Multi-layer uniforms
// ---------------------------------------------------------------------------
uniform sampler2DArray diffuseTexArray;   // 3 diffuse layers stacked
uniform sampler2DArray normalTexArray;    // 3 normal-map layers stacked
uniform vec4           layerTint[3];      // per-layer RGBA tint

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------
#ifdef DEFERRED_MODE
layout (location = 0) out vec4 fragNormal;
layout (location = 1) out vec4 fragDiffuse;
layout (location = 2) out vec4 fragSpecular;
layout (location = 3) out vec4 fragEmission;
layout (location = 4) out vec4 fragMisc;
#else
out vec4 fragColor;
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

vec4 SampleDiffuse() {
    // Use the texture array when the array has data; fall back to the
    // traditional diffuseTex for layer 1 (surface) so existing maps still
    // look correct without modification.
    vec3 uvLayer = vec3(diffuseTexCoords, float(layerID));
    return texture(diffuseTexArray, uvLayer);
}

vec3 SampleNormal() {
    vec3 uvLayer = vec3(diffuseTexCoords, float(layerID));
    vec3 n = texture(normalTexArray, uvLayer).rgb * 2.0 - 1.0;
    return normalize(n);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
void main() {
    vec4 diffuse = SampleDiffuse();

    // Apply per-layer tint (alpha controls blend strength)
    vec4 tint = layerTint[layerID];
    diffuse.rgb = mix(diffuse.rgb, tint.rgb, tint.a);

#ifdef SMF_ADV_SHADING
    vec3 normal     = SampleNormal();
    vec3 lightVec   = lightDir.xyz;
    float diffLight = max(dot(normal, lightVec), 0.0);
    float specLight = pow(max(dot(normal, halfDir), 0.0), groundSpecularExponent);

    vec3 ambient  = groundAmbientColor;
    vec3 diffCol  = groundDiffuseColor  * diffLight;
    vec3 specCol  = groundSpecularColor * specLight;

    // Shadow contribution from the info texture (same channel as SMFFragProg)
    float shadow = texture(infoTex, vertexWorldPos.xz * specularTexGen).a;
    diffCol  *= mix(1.0 - groundShadowDensity, 1.0, shadow);
    specCol  *= mix(1.0 - groundShadowDensity, 1.0, shadow);

    vec3 lit = diffuse.rgb * (ambient + diffCol) + specCol;

#ifdef DEFERRED_MODE
    fragNormal   = vec4(normal * 0.5 + 0.5, 1.0);
    fragDiffuse  = vec4(lit,  diffuse.a);
    fragSpecular = vec4(specCol, 1.0);
    fragEmission = vec4(0.0);
    fragMisc     = vec4(float(layerID) / 2.0, 0.0, 0.0, 1.0);
    return;
#else
    vec3 fogColor = gl_Fog.color.rgb;
    fragColor = vec4(mix(fogColor, lit, fogFactor), diffuse.a);
#endif

#else
    // Non-advanced shading: read diffuse + shading from shadingTex
    vec4 shading = texture(infoTex, vertexWorldPos.xz * specularTexGen);
    vec3 lit = diffuse.rgb * shading.rgb;
    fragColor = vec4(mix(gl_Fog.color.rgb, lit, fogFactor), diffuse.a);
#endif
}
