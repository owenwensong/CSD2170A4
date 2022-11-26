/*!*****************************************************************************
 * @file    ellipsoid.tese
 * @author  Owen Huang Wensong, w.huang, 390008220
 * @date    26 NOV 2022
 * @brief   Tessellation Evaluation Shader for assignment 4.
 *          Color information produced from sphere coordinates.
 * 
 * @par Copyright (C) 2022 DigiPen Institute of Technology. All rights reserved.
*******************************************************************************/

#version 450

layout(quads, equal_spacing, ccw) in;

layout (binding = 0) uniform UBO 
{
  mat4 m_View;
  mat4 m_Proj;
  vec4 m_Center;
  vec4 m_ScaleAndTeslvl;
} ubo;

layout (location = 0) out vec3 te_Col;

const float PI = 3.1415926535897932384626433832795;
const float TWOPI = 2 * PI;

void main()
{
  vec3 center = ubo.m_Center.xyz;         // translation
  vec3 scale = ubo.m_ScaleAndTeslvl.xyz;  // convert sphere to ellipsoid

  // gl_TessCoord space is [0, 1] for quads
  // https://stackoverflow.com/questions/28946396/how-the-gl-tesscoord-is-computed-during-the-tessellation
  float phi = PI * (gl_TessCoord.x - 0.5);     // [0, 1] to [-0.5, 0.5] to [-PI/2, PI/2] 
  float theta = TWOPI * (gl_TessCoord.y - 0.5);// [0, 1] to [-0.5, 0.5] to [-PI, PI]
  float cosPhi = cos(phi);  // common between x and z coordinates

  vec3 spherePos = vec3(cosPhi * cos(theta), sin(phi), cosPhi * sin(theta));
  te_Col = clamp(spherePos, 0.0, 1.0);
  
  gl_Position = ubo.m_Proj * ubo.m_View * vec4(center + scale * spherePos, 1.0);
}