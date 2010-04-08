/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Copyright (C) 2010  Intel Corporation.
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
 * Author:
 *   Emmanuele Bassi <ebassi@linux.intel.com>
 */

/**
 * SECTION:clutter-shader-effect
 * @short_description: Base class for fragment shader effects
 * @See_Also: #ClutterEffect, #ClutterOffscreenEffect
 *
 * #ClutterShaderEffect is an abstract class that implements all the
 * plumbing for creating #ClutterEffect<!-- -->s using GLSL fragment
 * shaders.
 *
 * #ClutterShaderEffect creates an offscreen buffer and then applies the GLSL
 * fragment shader (after checking whether the compilation and linking were
 * successfull) to the buffer before painting it on screen.
 *
 * <refsect2 id="ClutterShaderEffect-implementing">
 *   <title>Implementing a ClutterShaderEffect</title>
 *   <para>Creating a sub-class of #ClutterShaderEffect requires the
 *   overriding of the <function>set_actor()</function> virtual function
 *   from the #ClutterActorMeta class, and the <function>pre_paint()</function>
 *   virtual functions from the #ClutterEffect class.</para>
 *   <para>The <function>set_actor()</function> must chain up to the
 *   #ClutterShaderEffect implementation; if the effect has not been disabled
 *   by the super class implementation, then the sub-class should get the
 *   #CoglHandle for the fragment shader using
 *   clutter_shader_effect_get_shader() and call cogl_shader_source() to set
 *   the source of the program.</para>
 *   <example id="ClutterShaderEffect-example-preparing">
 *     <title>Preparing a ClutterShaderEffect</title>
 *     <para>The example below shows the typical implementation of the
 *     <function>set_actor()</function> phase of a #ClutterShaderEffect
 *     sub-class.</para>
 *     <programlisting>
 *  static void
 *  my_effect_set_actor (ClutterActorMeta *meta,
 *                       ClutterActor     *actor)
 *  {
 *    MyEffect *self = MY_EFFECT (meta);
 *    ClutterActorMetaClass *parent_class;
 *    CoglHandle handle;
 *
 *    /&ast; chain up to the parent's implementation &ast;/
 *    parent_class = CLUTTER_EFFECT_CLASS (my_effect_parent_class);
 *    parent_class->set_actor (meta, actor));
 *
 *    /&ast; if the parent class disabled the effect then we return &ast;
 *    if (!clutter_actor_meta_get_enabled (meta))
 *      return;
 *
 *    /&ast; we should have a handle to the shader at this point &ast;/
 *    handle = clutter_shader_effect_get_shader (CLUTTER_SHADER_EFFECT (self));
 *    if (handle == COGL_INVALID_HANDLE)
 *      return;
 *
 *    /&ast; set the source of the shader &ast;/
 *    cogl_shader_source (handle, my_effect_glsl_source);
 *  }
 *     </programlisting>
 *   </example>
 *   <para>The <function>pre_paint()</function> is optional and it depends on
 *   whether the fragment shader has uniforms to set. The sub-class should call
 *   clutter_shader_effect_set_uniform_value() or
 *   clutter_shader_effect_set_uniform() and then chain up to the
 *   #ClutterShaderEffect implementation.</para>
 *   <example id="ClutterShaderEffect-example-uniforms">
 *     <title>Setting uniforms on a ClutterShaderEffect</title>
 *     <para>The example below shows a typical implementation of the
 *     <function>pre_paint()</function> phase of a #ClutterShaderEffect
 *     sub-class.</para>
 *     <programlisting>
 *  static gboolean
 *  my_effect_pre_paint (ClutterEffect *effect)
 *  {
 *    MyEffect *self = MY_EFFECT (effect);
 *    ClutterShaderEffect *shader = CLUTTER_SHADER_EFFECT (effect);
 *    ClutterEffectClass *parent_class;
 *    gfloat component_r, component_g, component_b;
 *
 *    /&ast; chain up to the parent's implementation &ast;/
 *    parent_class = CLUTTER_EFFECT_CLASS (my_effect_parent_class);
 *    if (!parent_class->pre_paint (effect))
 *      return FALSE;
 *
 *    /&ast; the "tex" uniform is declared in the shader as:
 *     &ast;
 *     &ast;   uniform int tex;
 *     &ast;
 *     &ast; and it is passed a constant value of 0
 *     &ast;/
 *    clutter_shader_effect_set_uniform (shader, "tex", G_TYPE_INT, 1, 0);
 *
 *    /&ast; the "component" uniform is declared in the shader as:
 *     &ast;
 *     &ast;   uniform vec3 component;
 *     &ast;
 *     &ast; and it's defined to contain the normalized components
 *     &ast; of a #ClutterColor
 *     &ast;/
 *    component_r = self->color.red   / 255.0f;
 *    component_g = self->color.green / 255.0f;
 *    component_b = self->color.blue  / 255.0f;
 *    clutter_shader_effect_set_uniform (shader, "component",
 *                                       G_TYPE_FLOAT, 3,
 *                                       component_r,
 *                                       component_g,
 *                                       component_b);
 *
 *    return TRUE;
 *  }
 *     </programlisting>
 *   </example>
 * </refsect2>
 *
 * #ClutterShaderEffect is available since Clutter 1.4
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-shader-effect.h"

#include "cogl/cogl.h"

#include "clutter-debug.h"
#include "clutter-feature.h"
#include "clutter-private.h"
#include "clutter-shader-types.h"

typedef struct _ShaderUniform
{
  gchar *name;
  GType type;
  GValue value;
  GLint location;
} ShaderUniform;

struct _ClutterShaderEffectPrivate
{
  ClutterActor *actor;

  CoglHandle program;
  CoglHandle shader;

  GHashTable *uniforms;

  guint is_compiled : 1;
};

G_DEFINE_ABSTRACT_TYPE (ClutterShaderEffect,
                        clutter_shader_effect,
                        CLUTTER_TYPE_OFFSCREEN_EFFECT);

static inline void
clutter_shader_effect_clear (ClutterShaderEffect *effect,
                             gboolean             reset_uniforms)
{
  ClutterShaderEffectPrivate *priv = effect->priv;

  if (priv->shader != COGL_INVALID_HANDLE)
    {
      cogl_handle_unref (priv->shader);
      priv->shader = COGL_INVALID_HANDLE;
    }

  if (priv->program != COGL_INVALID_HANDLE)
    {
      cogl_handle_unref (priv->program);
      priv->program = COGL_INVALID_HANDLE;
    }

  if (reset_uniforms && priv->uniforms != NULL)
    {
      g_hash_table_destroy (priv->uniforms);
      priv->uniforms = NULL;
    }

  priv->actor = NULL;
  priv->is_compiled = FALSE;
}

static void
clutter_shader_effect_reset_uniforms (ClutterShaderEffect *effect)
{
  ClutterShaderEffectPrivate *priv = effect->priv;
  GHashTableIter iter;
  gpointer key, value;

  if (priv->uniforms == NULL)
    return;

  key = value = NULL;
  g_hash_table_iter_init (&iter, priv->uniforms);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      ShaderUniform *uniform = value;

      uniform->location = -1;
    }
}

static void
clutter_shader_effect_update_uniforms (ClutterShaderEffect *effect)
{
  ClutterShaderEffectPrivate *priv = effect->priv;
  GHashTableIter iter;
  gpointer key, value;
  gsize size;

  if (priv->uniforms == NULL)
    return;

  key = value = NULL;
  g_hash_table_iter_init (&iter, priv->uniforms);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      ShaderUniform *uniform = value;

      if (uniform->location == -1)
        uniform->location = cogl_program_get_uniform_location (priv->program,
                                                               uniform->name);

      if (CLUTTER_VALUE_HOLDS_SHADER_FLOAT (&uniform->value))
        {
          const GLfloat *floats;

          floats = clutter_value_get_shader_float (&uniform->value, &size);
          cogl_program_uniform_float (uniform->location, size, 1, floats);
        }
      else if (CLUTTER_VALUE_HOLDS_SHADER_INT (&uniform->value))
        {
          const GLint *ints;

          ints = clutter_value_get_shader_int (&uniform->value, &size);
          cogl_program_uniform_int (uniform->location, size, 1, ints);
        }
      else if (CLUTTER_VALUE_HOLDS_SHADER_MATRIX (&uniform->value))
        {
          const GLfloat *matrix;

          matrix = clutter_value_get_shader_matrix (&uniform->value, &size);
          cogl_program_uniform_matrix (uniform->location, size, 1, FALSE, matrix);
        }
      else if (G_VALUE_HOLDS_FLOAT (&uniform->value))
        {
          const GLfloat float_val = g_value_get_float (&uniform->value);

          cogl_program_uniform_float (uniform->location, 1, 1, &float_val);
        }
      else if (G_VALUE_HOLDS_INT (&uniform->value))
        {
          const GLint int_val = g_value_get_int (&uniform->value);

          cogl_program_uniform_int (uniform->location, 1, 1, &int_val);
        }
      else
        g_warning ("Invalid uniform of type '%s' for name '%s'",
                   g_type_name (G_VALUE_TYPE (&uniform->value)),
                   uniform->name);
    }
}

static void
clutter_shader_effect_set_actor (ClutterActorMeta *meta,
                                 ClutterActor     *actor)
{
  ClutterShaderEffect *self = CLUTTER_SHADER_EFFECT (meta);
  ClutterShaderEffectPrivate *priv = self->priv;
  ClutterActorMetaClass *parent;
  ClutterActorBox allocation;
  gfloat width, height;

  if (!clutter_feature_available (CLUTTER_FEATURE_SHADERS_GLSL))
    {
      /* if we don't have support for GLSL shaders then we
       * forcibly disable the ActorMeta
       */
      g_warning ("Unable to use the ShaderEffect: the graphics hardware "
                 "or the current GL driver does not implement support "
                 "for the GLSL shading language.");
      clutter_actor_meta_set_enabled (meta, FALSE);
      return;
    }

  clutter_shader_effect_clear (self, FALSE);
  clutter_shader_effect_reset_uniforms (self);

  parent = CLUTTER_ACTOR_META_CLASS (clutter_shader_effect_parent_class);
  parent->set_actor (meta, actor);

  /* we keep a back pointer here */
  priv->actor = clutter_actor_meta_get_actor (meta);
  if (priv->actor == NULL)
    return;

  CLUTTER_NOTE (SHADER, "Preparing shader effect of type '%s'",
                G_OBJECT_TYPE_NAME (meta));

  clutter_actor_get_allocation_box (priv->actor, &allocation);
  clutter_actor_box_get_size (&allocation, &width, &height);

  priv->program = cogl_create_program ();

  priv->shader = cogl_create_shader (COGL_SHADER_TYPE_FRAGMENT);
  g_assert (priv->shader != COGL_INVALID_HANDLE);
}

static void
clutter_shader_effect_paint_target (ClutterOffscreenEffect *effect)
{
  ClutterShaderEffectPrivate *priv = CLUTTER_SHADER_EFFECT (effect)->priv;
  ClutterOffscreenEffectClass *parent;

  /* we haven't been prepared or we don't have support for
   * GLSL shaders in Clutter
   */
  if (priv->program == COGL_INVALID_HANDLE ||
      priv->shader == COGL_INVALID_HANDLE)
    return;

  if (!priv->is_compiled)
    {
      CLUTTER_NOTE (SHADER, "Compiling shader effect");

      cogl_shader_compile (priv->shader);
      if (!cogl_shader_is_compiled (priv->shader))
        {
          gchar *log_buf = cogl_shader_get_info_log (priv->shader);

          g_warning ("Unable to compile the GLSL shader: %s", log_buf);
          g_free (log_buf);

          cogl_handle_unref (priv->shader);
          priv->shader = COGL_INVALID_HANDLE;

          cogl_handle_unref (priv->program);
          priv->shader = COGL_INVALID_HANDLE;

          return;
        }

      cogl_program_attach_shader (priv->program, priv->shader);
      cogl_program_link (priv->program);

      priv->is_compiled = TRUE;
    }

  CLUTTER_NOTE (SHADER, "Applying the shader effect of type '%s'",
                G_OBJECT_TYPE_NAME (effect));

  /* set the shader */
  cogl_program_use (priv->program);

  clutter_shader_effect_update_uniforms (CLUTTER_SHADER_EFFECT (effect));

  /* paint the offscreen buffer */
  parent = CLUTTER_OFFSCREEN_EFFECT_CLASS (clutter_shader_effect_parent_class);
  parent->paint_target (effect);

  /* unset the shader */
  cogl_program_use (COGL_INVALID_HANDLE);
}

static void
clutter_shader_effect_finalize (GObject *gobject)
{
  ClutterShaderEffect *effect = CLUTTER_SHADER_EFFECT (gobject);

  clutter_shader_effect_clear (effect, TRUE);

  G_OBJECT_CLASS (clutter_shader_effect_parent_class)->finalize (gobject);
}

static void
clutter_shader_effect_class_init (ClutterShaderEffectClass *klass)
{
  ClutterActorMetaClass *meta_class = CLUTTER_ACTOR_META_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterOffscreenEffectClass *offscreen_class;

  g_type_class_add_private (klass, sizeof (ClutterShaderEffectPrivate));

  offscreen_class = CLUTTER_OFFSCREEN_EFFECT_CLASS (klass);

  gobject_class->finalize = clutter_shader_effect_finalize;

  meta_class->set_actor = clutter_shader_effect_set_actor;

  offscreen_class->paint_target = clutter_shader_effect_paint_target;
}

static void
clutter_shader_effect_init (ClutterShaderEffect *effect)
{
  effect->priv = G_TYPE_INSTANCE_GET_PRIVATE (effect,
                                              CLUTTER_TYPE_SHADER_EFFECT,
                                              ClutterShaderEffectPrivate);
}

/**
 * clutter_shader_effect_get_shader:
 * @effect: a #ClutterShaderEffect
 *
 * Retrieves a pointer to the shader's handle
 *
 * Return value: (transfer none): a pointer to the shader's handle,
 *   or %COGL_INVALID_HANDLE
 *
 * Since: 1.4
 */
CoglHandle
clutter_shader_effect_get_shader (ClutterShaderEffect *effect)
{
  g_return_val_if_fail (CLUTTER_IS_SHADER_EFFECT (effect),
                        COGL_INVALID_HANDLE);

  return effect->priv->shader;
}

/**
 * clutter_shader_effect_get_program:
 * @effect: a #ClutterShaderEffect
 *
 * Retrieves a pointer to the program's handle
 *
 * Return value: (transfer none): a pointer to the program's handle,
 *   or %COGL_INVALID_HANDLE
 *
 * Since: 1.4
 */
CoglHandle
clutter_shader_effect_get_program (ClutterShaderEffect *effect)
{
  g_return_val_if_fail (CLUTTER_IS_SHADER_EFFECT (effect),
                        COGL_INVALID_HANDLE);

  return effect->priv->program;
}

static void
shader_uniform_free (gpointer data)
{
  if (data != NULL)
    {
      ShaderUniform *uniform = data;

      g_value_unset (&uniform->value);
      g_free (uniform->name);

      g_slice_free (ShaderUniform, uniform);
    }
}

static ShaderUniform *
shader_uniform_new (const gchar  *name,
                    const GValue *value)
{
  ShaderUniform *retval;

  retval = g_slice_new (ShaderUniform);
  retval->name = g_strdup (name);
  retval->type = G_VALUE_TYPE (value);
  retval->location = -1;

  g_value_init (&retval->value, retval->type);
  g_value_copy (value, &retval->value);

  return retval;
}

static void
shader_uniform_update (ShaderUniform *uniform,
                       const GValue  *value)
{
  g_value_unset (&uniform->value);

  g_value_init (&uniform->value, G_VALUE_TYPE (value));
  g_value_copy (value, &uniform->value);
}

static inline void
clutter_shader_effect_add_uniform (ClutterShaderEffect *effect,
                                   const gchar         *name,
                                   const GValue        *value)
{
  ClutterShaderEffectPrivate *priv = effect->priv;
  ShaderUniform *uniform;

  if (priv->uniforms == NULL)
    {
      priv->uniforms = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              NULL,
                                              shader_uniform_free);
    }

  uniform = g_hash_table_lookup (priv->uniforms, name);
  if (uniform == NULL)
    {
      uniform = shader_uniform_new (name, value);
      g_hash_table_insert (priv->uniforms, uniform->name, uniform);
    }
  else
    shader_uniform_update (uniform, value);

  if (priv->is_compiled)
    {
      uniform->location =
        cogl_program_get_uniform_location (priv->program, uniform->name);
    }
}

/**
 * clutter_shader_effect_set_uniform_value:
 * @effect: a #ClutterShaderEffect
 * @name: the name of the uniform to set
 * @value: a #GValue with the value of the uniform to set
 *
 * Sets @value as the payload for the uniform @name inside the shader
 * effect
 *
 * The #GType of the @value must be one of: %G_TYPE_INT, for a single
 * integer value; %G_TYPE_FLOAT, for a single floating point value;
 * %CLUTTER_TYPE_SHADER_INT, for an array of integer values;
 * %CLUTTER_TYPE_SHADER_FLOAT, for an array of floating point values;
 * and %CLUTTER_TYPE_SHADER_MATRIX, for a matrix of floating point
 * values
 *
 * Since: 1.4
 */
void
clutter_shader_effect_set_uniform_value (ClutterShaderEffect *effect,
                                         const gchar         *name,
                                         const GValue        *value)
{
  ClutterShaderEffectPrivate *priv;

  g_return_if_fail (CLUTTER_IS_SHADER_EFFECT (effect));
  g_return_if_fail (name != NULL);
  g_return_if_fail (value != NULL);

  priv = effect->priv;

  clutter_shader_effect_add_uniform (effect, name, value);
}

static void
clutter_shader_effect_set_uniform_valist (ClutterShaderEffect *effect,
                                          const gchar         *name,
                                          GType                value_type,
                                          gsize                n_values,
                                          va_list             *args)
{
  GValue value = { 0, };

  if (value_type == CLUTTER_TYPE_SHADER_INT)
    {
      gint *int_values = va_arg (*args, gint*);

      g_value_init (&value, CLUTTER_TYPE_SHADER_INT);
      clutter_value_set_shader_int (&value, n_values, int_values);

      goto add_uniform;
    }

  if (value_type == CLUTTER_TYPE_SHADER_FLOAT)
    {
      gfloat *float_values = va_arg (*args, gfloat*);

      g_value_init (&value, CLUTTER_TYPE_SHADER_FLOAT);
      clutter_value_set_shader_float (&value, n_values, float_values);

      goto add_uniform;
    }

  if (value_type == CLUTTER_TYPE_SHADER_MATRIX)
    {
      gfloat *float_values = va_arg (*args, gfloat*);

      g_value_init (&value, CLUTTER_TYPE_SHADER_MATRIX);
      clutter_value_set_shader_matrix (&value, n_values, float_values);

      goto add_uniform;
    }

  if (value_type == G_TYPE_INT)
    {
      /* if we only have one value we can go through the fast path
       * of using G_TYPE_INT, otherwise we create a vector of integers
       * from the passed values
       */
      if (n_values == 1)
        {
          gint int_val = va_arg (*args, gint);

          g_value_init (&value, G_TYPE_INT);
          g_value_set_int (&value, int_val);
        }
      else
        {
          gint *int_values = g_new (gint, n_values);
          gint i;

          for (i = 0; i < n_values; i++)
            int_values[i] = va_arg (*args, gint);

          g_value_init (&value, CLUTTER_TYPE_SHADER_INT);
          clutter_value_set_shader_int (&value, n_values, int_values);

          g_free (int_values);
        }

      goto add_uniform;
    }

  if (value_type == G_TYPE_FLOAT)
    {
      /* if we only have one value we can go through the fast path
       * of using G_TYPE_FLOAT, otherwise we create a vector of floats
       * from the passed values
       */
      if (n_values == 1)
        {
          gfloat float_val = (gfloat) va_arg (*args, gdouble);

          g_value_init (&value, G_TYPE_FLOAT);
          g_value_set_float (&value, float_val);
        }
      else
        {
          gfloat *float_values = g_new (gfloat, n_values);
          gint i;

          for (i = 0; i < n_values; i++)
            float_values[i] = (gfloat) va_arg (*args, double);

          g_value_init (&value, CLUTTER_TYPE_SHADER_FLOAT);
          clutter_value_set_shader_float (&value, n_values, float_values);

          g_free (float_values);
        }

      goto add_uniform;
    }

  g_warning ("Unrecognized type '%s' (values: %d) for uniform name '%s'",
             g_type_name (value_type),
             (int) n_values,
             name);
  return;

add_uniform:
  clutter_shader_effect_add_uniform (effect, name, &value);
  g_value_unset (&value);
}

/**
 * clutter_shader_effect_set_uniform:
 * @effect: a #ClutterShaderEffect
 * @name: the name of the uniform to set
 * @gtype: the type of the uniform to set
 * @n_value: the number of values
 * @VarArgs: a list of values
 *
 * Sets a list of values as the payload for the uniform @name inside
 * the shader effect
 *
 * The @gtype must be one of: %G_TYPE_INT, for 1 or more integer values;
 * %G_TYPE_FLOAT, for 1 or more floating point values;
 * %CLUTTER_TYPE_SHADER_INT, for a pointer to an array of integer values;
 * %CLUTTER_TYPE_SHADER_FLOAT, for a pointer to an array of floating point
 * values; and %CLUTTER_TYPE_SHADER_MATRIX, for a pointer to an array of
 * floating point values mapping a matrix
 *
 * The number of values interepreted is defined by the @n_value
 * argument, and by the @gtype argument. For instance, a uniform named
 * "sampler0" and containing a single integer value is set using:
 *
 * |[
 *   clutter_shader_effect_set_uniform (effect, "sampler0",
 *                                      G_TYPE_INT, 1,
 *                                      0);
 * ]|
 *
 * While a uniform named "components" and containing a 3-elements vector
 * of floating point values (a "vec3") can be set using:
 *
 * |[
 *   gfloat component_r, component_g, component_b;
 *
 *   clutter_shader_effect_set_uniform (effect, "components",
 *                                      G_TYPE_FLOAT, 3,
 *                                      component_r,
 *                                      component_g,
 *                                      component_b);
 * ]|
 *
 * or can be set using:
 *
 * |[
 *   gfloat component_vec[3];
 *
 *   clutter_shader_effect_set_uniform (effect, "components",
 *                                      CLUTTER_TYPE_SHADER_FLOAT, 3,
 *                                      component_vec);
 * ]|
 *
 * Finally, a uniform named "map" and containing a matrix can be set using:
 *
 * |[
 *   clutter_shader_effect_set_uniform (effect, "map",
 *                                      CLUTTER_TYPE_SHADER_MATRIX, 1,
 *                                      cogl_matrix_get_array (&matrix));
 * ]|
 *
 * Since: 1.4
 */
void
clutter_shader_effect_set_uniform (ClutterShaderEffect *effect,
                                   const gchar         *name,
                                   GType                gtype,
                                   gsize                n_values,
                                   ...)
{
  va_list args;

  g_return_if_fail (CLUTTER_IS_SHADER_EFFECT (effect));
  g_return_if_fail (name != NULL);
  g_return_if_fail (gtype != G_TYPE_INVALID);
  g_return_if_fail (n_values > 0);

  va_start (args, n_values);
  clutter_shader_effect_set_uniform_valist (effect, name,
                                            gtype,
                                            n_values,
                                            &args);
  va_end (args);
}