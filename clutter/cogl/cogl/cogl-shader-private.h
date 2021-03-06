/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2008,2009 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef __COGL_SHADER_H
#define __COGL_SHADER_H

#include "cogl-handle.h"

typedef struct _CoglShader CoglShader;

typedef enum
{
  COGL_SHADER_LANGUAGE_GLSL,
#ifdef HAVE_COGL_GL
  COGL_SHADER_LANGUAGE_ARBFP
#endif
} CoglShaderLanguage;

struct _CoglShader
{
  CoglHandleObject   _parent;
  GLuint             gl_handle;
  int                n_tex_coord_attribs;
  CoglShaderType     type;
  CoglShaderLanguage language;
  char              *source;
};

CoglShader *_cogl_shader_pointer_from_handle (CoglHandle handle);

void
_cogl_shader_compile_real (CoglHandle handle, int n_tex_coord_attribs);

CoglShaderLanguage
_cogl_program_get_language (CoglHandle handle);

void
_cogl_shader_set_source_with_boilerplate (GLuint shader_gl_handle,
                                          GLenum shader_gl_type,
                                          int n_tex_coord_attribs,
                                          GLsizei count_in,
                                          const char **strings_in,
                                          const GLint *lengths_in);

#endif /* __COGL_SHADER_H */
