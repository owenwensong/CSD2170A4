/*!*****************************************************************************
 * @file    ellipsoid.tesc
 * @author  Owen Huang Wensong, w.huang, 390008220
 * @date    26 NOV 2022
 * @brief   Tessellation Control Shader for assignment 4.
 *          vertex data generated from a 1 control point patch list.
 * 
 * @par Copyright (C) 2022 DigiPen Institute of Technology. All rights reserved.
*******************************************************************************/

#version 450

layout (binding = 0) uniform UBO 
{
	mat4 m_View;
	mat4 m_Proj;
  vec4 m_Center;
  vec4 m_ScaleAndTeslvl;
} ubo;
 
layout (vertices = 1) out;

// Tessellation Control Shaders built-in patch output variables:
// patch out float gl_TessLevelOuter[4];
// patch out float gl_TessLevelInner[2];

void main()
{
  if (gl_InvocationID == 0)
	{
		gl_TessLevelOuter[0] = ubo.m_ScaleAndTeslvl.w;
		gl_TessLevelOuter[1] = ubo.m_ScaleAndTeslvl.w;
		gl_TessLevelOuter[2] = ubo.m_ScaleAndTeslvl.w;
		gl_TessLevelOuter[3] = ubo.m_ScaleAndTeslvl.w;

    gl_TessLevelInner[0] = ubo.m_ScaleAndTeslvl.w;
		gl_TessLevelInner[1] = ubo.m_ScaleAndTeslvl.w;
	}
} 
