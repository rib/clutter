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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cogl.h"
#include "cogl-internal.h"
#include "cogl-util.h"
#include "cogl-texture-private.h"
#include "cogl-fbo.h"
#include "cogl-context.h"
#include "cogl-handle.h"
#include "cogl-gles2-wrapper.h"

#ifdef HAVE_COGL_GLES2

static void _cogl_offscreen_free (CoglFbo *fbo);

COGL_HANDLE_DEFINE (Fbo, offscreen);

CoglHandle
cogl_offscreen_new_to_texture (CoglHandle texhandle)
{
  CoglFbo          *fbo;
  int               width;
  int               height;
  GLuint            tex_gl_handle;
  GLenum            tex_gl_target;
  GLuint            fbo_gl_handle;
  GLuint            gl_stencil_handle;
  GLenum            status;

  _COGL_GET_CONTEXT (ctx, COGL_INVALID_HANDLE);

  if (!cogl_features_available (COGL_FEATURE_OFFSCREEN))
    return COGL_INVALID_HANDLE;

  /* Make texhandle is a valid texture object */
  if (!cogl_is_texture (texhandle))
    return COGL_INVALID_HANDLE;

  /* The texture must not be sliced */
  if (cogl_texture_is_sliced (texhandle))
    return COGL_INVALID_HANDLE;

  /* Pick the single texture slice width, height and GL id */

  width = cogl_texture_get_width (texhandle);
  height = cogl_texture_get_height (texhandle);

  if (!cogl_texture_get_gl_texture (texhandle, &tex_gl_handle, &tex_gl_target))
    return COGL_INVALID_HANDLE;

  if (tex_gl_target != GL_TEXTURE_2D)
    return COGL_INVALID_HANDLE;

  /* Create a renderbuffer for stenciling */
  GE( glGenRenderbuffers (1, &gl_stencil_handle) );
  GE( glBindRenderbuffer (GL_RENDERBUFFER, gl_stencil_handle) );
  GE( glRenderbufferStorage (GL_RENDERBUFFER, GL_STENCIL_INDEX8,
			     cogl_texture_get_width (texhandle),
			     cogl_texture_get_height (texhandle)) );
  GE( glBindRenderbuffer (GL_RENDERBUFFER, 0) );

  /* Generate framebuffer */
  glGenFramebuffers (1, &fbo_gl_handle);
  GE( glBindFramebuffer (GL_FRAMEBUFFER, fbo_gl_handle) );
  GE( glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			      tex_gl_target, tex_gl_handle, 0) );
  GE( glFramebufferRenderbuffer (GL_FRAMEBUFFER,
				 GL_STENCIL_ATTACHMENT,
				 GL_RENDERBUFFER, gl_stencil_handle) );

  /* Make sure it's complete */
  status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE)
    {
      /* Stencil renderbuffers aren't always supported. Try again
	 without the stencil buffer */
      GE( glFramebufferRenderbuffer (GL_FRAMEBUFFER,
				     GL_STENCIL_ATTACHMENT,
				     GL_RENDERBUFFER,
				     0) );
      GE( glDeleteRenderbuffers (1, &gl_stencil_handle) );
      gl_stencil_handle = 0;

      status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

      if (status != GL_FRAMEBUFFER_COMPLETE)
	{
	  /* Still failing, so give up */
	  GE( glDeleteFramebuffers (1, &fbo_gl_handle) );
	  GE( glBindFramebuffer (GL_FRAMEBUFFER, 0) );
	  return COGL_INVALID_HANDLE;
	}
    }

  GE( glBindFramebuffer (GL_FRAMEBUFFER, 0) );

  /* Allocate and init a CoglFbo object (store non-wasted size
     for subsequent blits and viewport setup) */
  fbo = (CoglFbo*) g_malloc (sizeof (CoglFbo));
  fbo->width             = width;
  fbo->height            = height;
  fbo->gl_handle         = fbo_gl_handle;
  fbo->gl_stencil_handle = gl_stencil_handle;

  return _cogl_offscreen_handle_new (fbo);
}

static void
_cogl_offscreen_free (CoglFbo *fbo)
{
  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  /* Frees FBO resources but its handle is not
     released! Do that separately before this! */
  if (fbo->gl_stencil_handle)
    GE( glDeleteRenderbuffers (1, &fbo->gl_stencil_handle) );
  GE( glDeleteFramebuffers (1, &fbo->gl_handle) );
  g_free (fbo);
}

void
cogl_set_draw_buffer (CoglBufferTarget target, CoglHandle offscreen)
{
  CoglFbo *fbo = NULL;
  CoglDrawBufferState *draw_buffer;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  g_assert (ctx->draw_buffer_stack != NULL);
  draw_buffer = ctx->draw_buffer_stack->data;

  if (target == COGL_OFFSCREEN_BUFFER)
    {
      GLboolean scissor_enabled;
      GLint scissor_box[4];

      /* Make sure it is a valid fbo handle */
      if (!cogl_is_offscreen (offscreen))
	return;

      fbo = _cogl_offscreen_pointer_from_handle (offscreen);

      /* Check current draw buffer target */
      if (draw_buffer->target != COGL_OFFSCREEN_BUFFER)
	{
	  /* Push the viewport and matrix setup if redirecting
             from a non-screen buffer */
	  GE( glGetIntegerv (GL_VIEWPORT, ctx->drv.viewport_store) );

          _cogl_matrix_stack_push (ctx->projection_stack);
          _cogl_matrix_stack_load_identity (ctx->projection_stack);

          _cogl_matrix_stack_push (ctx->modelview_stack);
          _cogl_matrix_stack_load_identity (ctx->modelview_stack);
	}
      else
	{
	  /* Override viewport and matrix setup if redirecting
             from another offscreen buffer */
          _cogl_matrix_stack_load_identity (ctx->projection_stack);

          _cogl_matrix_stack_load_identity (ctx->modelview_stack);
	}

      /* Setup new viewport and matrices */
      GE( glViewport (0, 0, fbo->width, fbo->height) );
      _cogl_matrix_stack_translate (ctx->modelview_stack, -1.0f, -1.0f, 0.0f);
      _cogl_matrix_stack_scale (ctx->modelview_stack,
                                2.0f / fbo->width, 2.0f / fbo->height, 1.0f);

      /* Bind offscreen framebuffer object */
      GE( glBindFramebuffer (GL_FRAMEBUFFER, fbo->gl_handle) );
      GE( glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE) );

      /* Some implementation require a clear before drawing
         to an fbo. Luckily it is affected by scissor test. */
      /* FIXME: test where exactly this is needed end whether
         a glClear with 0 argument is enough */

      scissor_enabled = glIsEnabled (GL_SCISSOR_TEST);
      GE( glGetIntegerv (GL_SCISSOR_BOX, scissor_box) );
      GE( glScissor (0, 0, 0, 0) );
      GE( glEnable (GL_SCISSOR_TEST) );
      GE( glClear (GL_COLOR_BUFFER_BIT) );
      if (!scissor_enabled)
	glDisable (GL_SCISSOR_TEST);
      glScissor (scissor_box[0], scissor_box[1],
		 scissor_box[2], scissor_box[3]);

    }
  else if (target & COGL_WINDOW_BUFFER)
    {
      /* Check current draw buffer target */
      if (draw_buffer->target == COGL_OFFSCREEN_BUFFER)
	{
	  /* Pop viewport and matrices if redirecting back
             from an offscreen buffer */
	  GE( glViewport (ctx->drv.viewport_store[0],
                          ctx->drv.viewport_store[1],
			  ctx->drv.viewport_store[2],
                          ctx->drv.viewport_store[3]) );

          _cogl_matrix_stack_pop (ctx->projection_stack);

          _cogl_matrix_stack_pop (ctx->modelview_stack);
	}

      /* Bind window framebuffer object */
      GE( glBindFramebuffer (GL_FRAMEBUFFER, 0) );
    }

  /* Store new target */
  draw_buffer->target = target;
  if (draw_buffer->offscreen != offscreen)
    {
      if (draw_buffer->offscreen != COGL_INVALID_HANDLE)
        cogl_handle_unref (draw_buffer->offscreen);
      if (offscreen != COGL_INVALID_HANDLE)
        cogl_handle_ref (offscreen);
      draw_buffer->offscreen = offscreen;
    }
}

void
cogl_push_draw_buffer(void)
{
  CoglDrawBufferState *old;
  CoglDrawBufferState *draw_buffer;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  g_assert (ctx->draw_buffer_stack != NULL);
  old = ctx->draw_buffer_stack->data;

  draw_buffer = g_slice_new0 (CoglDrawBufferState);
  *draw_buffer = *old;

  ctx->draw_buffer_stack =
    g_slist_prepend (ctx->draw_buffer_stack, draw_buffer);
}

void
cogl_pop_draw_buffer(void)
{
  CoglDrawBufferState *to_pop;
  CoglDrawBufferState *to_restore;

  _COGL_GET_CONTEXT (ctx, NO_RETVAL);

  g_assert (ctx->draw_buffer_stack != NULL);
  if (ctx->draw_buffer_stack->next == NULL)
    {
      g_warning ("1 more cogl_pop_draw_buffer() than cogl_push_draw_buffer()");
      return;
    }

  to_pop = ctx->draw_buffer_stack->data;
  to_restore = ctx->draw_buffer_stack->next->data;

  /* the logic in cogl_set_draw_buffer() only works if
   * to_pop is still on top of the stack, because
   * cogl_set_draw_buffer() needs to know the previous
   * state.
   */
  cogl_set_draw_buffer (to_restore->target, to_restore->offscreen);

  /* cogl_set_draw_buffer() should have set top of stack
   * to to_restore
   */
  g_assert (to_restore->target == to_pop->target);
  g_assert (to_restore->offscreen == to_pop->offscreen);

  g_assert (ctx->draw_buffer_stack->data == to_pop);
  ctx->draw_buffer_stack =
    g_slist_remove_link (ctx->draw_buffer_stack,
                         ctx->draw_buffer_stack);

  g_slice_free (CoglDrawBufferState, to_pop);
}

#else /* HAVE_COGL_GLES2 */

/* No support on regular OpenGL 1.1 */

gboolean
cogl_is_offscreen (CoglHandle handle)
{
  return FALSE;
}

CoglHandle
cogl_offscreen_new_to_texture (CoglHandle texhandle)
{
  return COGL_INVALID_HANDLE;
}

CoglHandle
cogl_offscreen_ref (CoglHandle handle)
{
  return COGL_INVALID_HANDLE;
}

void
cogl_offscreen_unref (CoglHandle handle)
{
}

void
cogl_set_draw_buffer (CoglBufferTarget target, CoglHandle offscreen)
{
}

#endif /* HAVE_COGL_GLES2 */
