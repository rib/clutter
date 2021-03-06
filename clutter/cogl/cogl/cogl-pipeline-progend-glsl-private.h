/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2010 Intel Corporation.
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
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 *
 *
 * Authors:
 *   Neil Roberts <neil@linux.intel.com>
 */

#ifndef __COGL_PIPELINE_PROGEND_GLSL_PRIVATE_H
#define __COGL_PIPELINE_PROGEND_GLSL_PRIVATE_H

#include "cogl-pipeline-private.h"
#include "cogl-attribute-private.h"

extern const CoglPipelineProgend _cogl_pipeline_glsl_progend;

#ifdef HAVE_COGL_GLES2

int
_cogl_pipeline_progend_glsl_get_position_attribute (CoglPipeline *pipeline);

int
_cogl_pipeline_progend_glsl_get_color_attribute (CoglPipeline *pipeline);

int
_cogl_pipeline_progend_glsl_get_normal_attribute (CoglPipeline *pipeline);

int
_cogl_pipeline_progend_glsl_get_tex_coord_attribute (CoglPipeline *pipeline,
                                                     int unit);

#endif /* HAVE_COGL_GLES2 */

#endif /* __COGL_PIPELINE_PROGEND_GLSL_PRIVATE_H */

