/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2010,2011 Intel Corporation.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __COGL_ATLAS_H
#define __COGL_ATLAS_H

#include "cogl-rectangle-map.h"
#include "cogl-callback-list.h"
#include "cogl-object-private.h"

typedef void
(* CoglAtlasUpdatePositionCallback) (void *user_data,
                                     CoglHandle new_texture,
                                     const CoglRectangleMapEntry *rect);

typedef enum
{
  COGL_ATLAS_CLEAR_TEXTURE     = (1 << 0),
  COGL_ATLAS_DISABLE_MIGRATION = (1 << 1)
} CoglAtlasFlags;

typedef struct _CoglAtlas CoglAtlas;

#define COGL_ATLAS(object) ((CoglAtlas *) object)

struct _CoglAtlas
{
  CoglObject _parent;

  CoglRectangleMap *map;

  CoglHandle texture;
  CoglPixelFormat texture_format;
  CoglAtlasFlags flags;

  CoglAtlasUpdatePositionCallback update_position_cb;

  CoglCallbackList pre_reorganize_callbacks;
  CoglCallbackList post_reorganize_callbacks;
};

CoglAtlas *
_cogl_atlas_new (CoglPixelFormat texture_format,
                 CoglAtlasFlags flags,
                 CoglAtlasUpdatePositionCallback update_position_cb);

gboolean
_cogl_atlas_reserve_space (CoglAtlas             *atlas,
                           unsigned int           width,
                           unsigned int           height,
                           void                  *user_data);

void
_cogl_atlas_remove (CoglAtlas *atlas,
                    const CoglRectangleMapEntry *rectangle);

CoglHandle
_cogl_atlas_copy_rectangle (CoglAtlas        *atlas,
                            unsigned int      x,
                            unsigned int      y,
                            unsigned int      width,
                            unsigned int      height,
                            CoglTextureFlags  flags,
                            CoglPixelFormat   format);

void
_cogl_atlas_add_reorganize_callback (CoglAtlas            *atlas,
                                     CoglCallbackListFunc  pre_callback,
                                     CoglCallbackListFunc  post_callback,
                                     void                 *user_data);

void
_cogl_atlas_remove_reorganize_callback (CoglAtlas            *atlas,
                                        CoglCallbackListFunc  pre_callback,
                                        CoglCallbackListFunc  post_callback,
                                        void                 *user_data);

#endif /* __COGL_ATLAS_H */
