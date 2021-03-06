/*
 * Cogl
 *
 * An object oriented GL/GLES Abstraction/Utility Layer
 *
 * Copyright (C) 2007,2008,2009,2010 Intel Corporation.
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
 * Authors:
 *   Robert Bragg <robert@linux.intel.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <string.h>

#include "cogl-types.h"
#include "cogl-object-private.h"

void *
cogl_object_ref (void *object)
{
  CoglObject *obj = object;

  g_return_val_if_fail (object != NULL, NULL);

  obj->ref_count++;
  return object;
}

CoglHandle
cogl_handle_ref (CoglHandle handle)
{
  return cogl_object_ref (handle);
}

void
cogl_object_unref (void *object)
{
  CoglObject *obj = object;

  g_return_if_fail (object != NULL);
  g_return_if_fail (obj->ref_count > 0);

  if (--obj->ref_count < 1)
    {
      void (*free_func)(void *obj);

      if (obj->n_user_data_entries)
        {
          int i;
          int count = MIN (obj->n_user_data_entries,
                           COGL_OBJECT_N_PRE_ALLOCATED_USER_DATA_ENTRIES);

          for (i = 0; i < count; i++)
            {
              CoglUserDataEntry *entry = &obj->user_data_entry[i];
              if (entry->destroy)
                entry->destroy (entry->user_data, obj);
            }

          if (obj->user_data_array != NULL)
            {
              for (i = 0; i < obj->user_data_array->len; i++)
                {
                  CoglUserDataEntry *entry =
                    &g_array_index (obj->user_data_array,
                                    CoglUserDataEntry, i);

                  if (entry->destroy)
                    entry->destroy (entry->user_data, obj);
                }
              g_array_free (obj->user_data_array, TRUE);
            }
        }

      COGL_OBJECT_DEBUG_FREE (obj);
      free_func = obj->klass->virt_free;
      free_func (obj);
    }
}

void
cogl_handle_unref (CoglHandle handle)
{
  cogl_object_unref (handle);
}

GType
cogl_handle_get_type (void)
{
  static GType our_type = 0;

  /* XXX: We are keeping the "CoglHandle" name for now incase it would
   * break bindings to change to "CoglObject" */
  if (G_UNLIKELY (our_type == 0))
    our_type = g_boxed_type_register_static (g_intern_static_string ("CoglHandle"),
                                             (GBoxedCopyFunc) cogl_object_ref,
                                             (GBoxedFreeFunc) cogl_object_unref);

  return our_type;
}

/* XXX: Unlike for cogl_object_get_user_data this code will return
 * an empty entry if available and no entry for the given key can be
 * found. */
static CoglUserDataEntry *
_cogl_object_find_entry (CoglObject *object, CoglUserDataKey *key)
{
  CoglUserDataEntry *entry = NULL;
  int count;
  int i;

  count = MIN (object->n_user_data_entries,
               COGL_OBJECT_N_PRE_ALLOCATED_USER_DATA_ENTRIES);

  for (i = 0; i < count; i++)
    {
      CoglUserDataEntry *current = &object->user_data_entry[i];
      if (current->key == key)
        return current;
      if (current->user_data == NULL)
        entry = current;
    }

  if (G_UNLIKELY (object->user_data_array != NULL))
    {
      for (i = 0; i < object->user_data_array->len; i++)
        {
          CoglUserDataEntry *current =
            &g_array_index (object->user_data_array, CoglUserDataEntry, i);

          if (current->key == key)
            return current;
          if (current->user_data == NULL)
            entry = current;
        }
    }

  return entry;
}

void
_cogl_object_set_user_data (CoglObject *object,
                            CoglUserDataKey *key,
                            void *user_data,
                            CoglUserDataDestroyInternalCallback destroy)
{
  CoglUserDataEntry new_entry;
  CoglUserDataEntry *entry;

  if (user_data)
    {
      new_entry.key = key;
      new_entry.user_data = user_data;
      new_entry.destroy = destroy;
    }
  else
    memset (&new_entry, 0, sizeof (new_entry));

  entry = _cogl_object_find_entry (object, key);
  if (entry)
    {
      if (G_LIKELY (entry->destroy))
        entry->destroy (entry->user_data, object);
    }
  else
    {
      if (G_LIKELY (object->n_user_data_entries <
                    COGL_OBJECT_N_PRE_ALLOCATED_USER_DATA_ENTRIES))
        entry = &object->user_data_entry[object->n_user_data_entries++];
      else
        {
          if (G_UNLIKELY (object->user_data_array == NULL))
            {
              object->user_data_array =
                g_array_new (FALSE, FALSE, sizeof (CoglUserDataEntry));
            }

          g_array_set_size (object->user_data_array,
                            object->user_data_array->len + 1);
          entry =
            &g_array_index (object->user_data_array, CoglUserDataEntry,
                            object->user_data_array->len - 1);

          object->n_user_data_entries++;
        }
    }

  *entry = new_entry;
}

void
cogl_object_set_user_data (CoglObject *object,
                           CoglUserDataKey *key,
                           void *user_data,
                           CoglUserDataDestroyCallback destroy)
{
  _cogl_object_set_user_data (object, key, user_data,
                              (CoglUserDataDestroyInternalCallback)destroy);
}

void *
cogl_object_get_user_data (CoglObject *object, CoglUserDataKey *key)
{
  int count;
  int i;

  count = MIN (object->n_user_data_entries,
               COGL_OBJECT_N_PRE_ALLOCATED_USER_DATA_ENTRIES);

  for (i = 0; i < count; i++)
    {
      CoglUserDataEntry *entry = &object->user_data_entry[i];
      if (entry->key == key)
        return entry->user_data;
    }

  if (object->user_data_array != NULL)
    {
      for (i = 0; i < object->user_data_array->len; i++)
        {
          CoglUserDataEntry *entry =
            &g_array_index (object->user_data_array, CoglUserDataEntry, i);

          if (entry->key == key)
            return entry->user_data;
        }
    }

  return NULL;
}

