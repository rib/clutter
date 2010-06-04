#include <stdlib.h>
#include <math.h>
#include <gmodule.h>
#include <clutter/clutter.h>

#define STAGE_WIDTH    1024
#define STAGE_HEIGHT   768

#define ACTOR_WIDTH    128
#define ACTOR_HEIGHT   128

#define COLS  (STAGE_WIDTH/ACTOR_WIDTH)
#define ROWS  (STAGE_HEIGHT/ACTOR_HEIGHT)
#define TOTAL (ROWS*COLS)


static gboolean press_event (ClutterActor *actor,
                             ClutterEvent *event,
                             gpointer      user_data)
{
  ClutterState *state = CLUTTER_STATE (user_data);
  clutter_grab_pointer (actor);
  clutter_state_change (state, "right", TRUE);
  return TRUE;
}

static gboolean release_event (ClutterActor *actor,
                               ClutterEvent *event,
                               gpointer      user_data)
{
  ClutterState *state = CLUTTER_STATE (user_data);
  clutter_state_change (state, "active", TRUE);
  clutter_ungrab_pointer ();
  return TRUE;
}

static gboolean enter_event (ClutterActor *actor,
                             ClutterEvent *event,
                             gpointer      user_data)
{
  ClutterState *state = CLUTTER_STATE (user_data);
  clutter_state_change (state, "hover", TRUE);
  return TRUE;
}

static gboolean leave_event (ClutterActor *actor,
                             ClutterEvent *event,
                             gpointer      user_data)
{
  ClutterState *state = CLUTTER_STATE (user_data);
  clutter_state_change (state, "normal", TRUE);
  return TRUE;
}

static void completed (ClutterState *state,
                       gpointer      data)
{
  g_print ("Completed transitioning to state: %s\n",
           clutter_state_get_target_state (state));

  if (g_str_equal (clutter_state_get_target_state (state), "right"))
    {
      /* skip straight to left state when reaching right */
      clutter_state_change (state, "left", FALSE);
    }
}

static ClutterActor *new_rect (gint r,
                               gint g,
                               gint b,
                               gint a)
{
  GError *error = NULL;
  ClutterColor *color = clutter_color_new (r, g, b, a);
  ClutterActor *group = clutter_group_new ();
  ClutterActor *rectangle = clutter_rectangle_new_with_color (color);
  ClutterActor *hand = NULL;

  gchar *file = g_build_filename (TESTS_DATADIR, "redhand.png", NULL);

  hand = clutter_texture_new_from_file (file, &error);
  if (rectangle == NULL)
    g_error ("image load failed: %s", error->message);
  g_free (file);
  clutter_actor_set_size (hand, ACTOR_WIDTH,ACTOR_HEIGHT);

  clutter_actor_set_size (rectangle, ACTOR_WIDTH,ACTOR_HEIGHT);
  clutter_color_free (color);
  clutter_container_add (CLUTTER_CONTAINER (group), rectangle, hand, NULL);
  return group;
}

G_MODULE_EXPORT gint
test_state_main (gint    argc,
                  gchar **argv)
{
  ClutterColor  black={0,0,0,0xff};
  ClutterActor *stage;
  ClutterState *layout_state;
  gint i;
  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  layout_state = clutter_state_new ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &black);
  clutter_actor_set_size (stage, STAGE_WIDTH, STAGE_HEIGHT);


  for (i=0; i<TOTAL; i++)
    {
      ClutterActor *actor;
      ClutterState *a_state;

      int row = i/COLS;
      int col = i%COLS;

      actor = new_rect (255 * ( 1.0*col/COLS), 50,
                        255 * ( 1.0*row/ROWS), 255);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), actor);
      clutter_actor_set_position (actor, 320.0, 240.0);
      clutter_actor_set_reactive (actor, TRUE);

      g_signal_connect (actor, "button-press-event",
                        G_CALLBACK (press_event), layout_state);
      g_signal_connect (actor, "button-release-event",
                        G_CALLBACK (release_event), layout_state);

      clutter_state_set (layout_state, NULL, "active",
            actor, "delayed::x", CLUTTER_LINEAR,
                                         ACTOR_WIDTH * 1.0 * ((TOTAL-1-i) % COLS), 
                                        ((row*1.0/ROWS))/2, (1.0-(row*1.0/ROWS))/2,
            actor, "delayed::y", CLUTTER_LINEAR,
                                         ACTOR_HEIGHT * 1.0 * ((TOTAL-1-i) / COLS),
                                        ((row*1.0/ROWS))/2, 0.0,
            actor, "rotation-angle-x", CLUTTER_LINEAR, 0.0,
            actor, "rotation-angle-y", CLUTTER_LINEAR, 0.0,
            NULL);

      clutter_state_set (layout_state, NULL, "right",
            actor, "delayed::x", CLUTTER_LINEAR, STAGE_WIDTH * 1.0,
                                    ((row*1.0/ROWS))/2,
                                    (1.0-(row*1.0/ROWS))/2,
            actor, "delayed::y", CLUTTER_LINEAR, STAGE_HEIGHT * 1.0,
                                    ((row*1.0/ROWS))/2,
                                    0.0,
            NULL);

      clutter_state_set (layout_state, NULL, "left",
            actor, "rotation-angle-x", CLUTTER_LINEAR, 45.0,
            actor, "rotation-angle-y", CLUTTER_LINEAR, 5.0,
            actor, "x", CLUTTER_LINEAR, 0-64.0,
            actor, "y", CLUTTER_LINEAR, 0-64.0,
                         NULL);

      a_state = clutter_state_new ();
      g_object_set_data_full (G_OBJECT (actor), "hover-state-machine",
                              a_state, g_object_unref);
      g_signal_connect (actor, "enter-event",
                        G_CALLBACK (enter_event), a_state);
      g_signal_connect (actor, "leave-event",
                        G_CALLBACK (leave_event), a_state);

      clutter_state_set (a_state, NULL, "normal",
                         actor, "opacity", CLUTTER_LINEAR, 0x77,
                         actor, "rotation-angle-z", CLUTTER_LINEAR, 0.0,
                         NULL);
      clutter_state_set (a_state, NULL, "hover",
                         actor, "opacity", CLUTTER_LINEAR, 0xff,
                         actor, "rotation-angle-z", CLUTTER_LINEAR, 10.0,
                         NULL);
      clutter_actor_set_opacity (actor, 0x77);

      clutter_state_set_duration (a_state, NULL, NULL, 500);
    }

  clutter_state_set_duration (layout_state, NULL, NULL, 1000);
  clutter_state_set_duration (layout_state, "active", "left", 1400);

  g_signal_connect (layout_state, "completed", G_CALLBACK (completed), NULL);

  clutter_actor_show (stage);
  clutter_state_change (layout_state, "left", FALSE);
  clutter_state_change (layout_state, "active", TRUE);

  clutter_main ();
  g_object_unref (layout_state);

  return EXIT_SUCCESS;
}