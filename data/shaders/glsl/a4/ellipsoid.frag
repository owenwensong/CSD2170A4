/*!*****************************************************************************
 * @file    ellipsoid.frag
 * @author  Owen Huang Wensong, w.huang, 390008220
 * @date    26 NOV 2022
 * @brief   Basic Fragment Shader for assignment 4.
 *          Color information from previous shader stage.
 * 
 * @par Copyright (C) 2022 DigiPen Institute of Technology. All rights reserved.
*******************************************************************************/

#version 450

layout (location = 0) in vec3 te_Col;

layout (location = 0) out vec4 f_FragColor;

void main()
{
  f_FragColor = vec4(te_Col, 1.0);
}
