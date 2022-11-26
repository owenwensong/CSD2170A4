/*!*****************************************************************************
 * @file    ellipsoid.vert
 * @author  Owen Huang Wensong, w.huang, 390008220
 * @date    26 NOV 2022
 * @brief   Empty Vertex Shader for assignment 4.
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

void main()
{
  
}
