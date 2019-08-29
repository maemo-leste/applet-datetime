#include <hildon/hildon.h>
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <clockd/libtime.h>

#include <libintl.h>
#include <string.h>

#include "config.h"

#include "../clock/clockcore.h"
#include "../hildon-time-zone-chooser/hildon-time-zone-chooser.h"

typedef struct
{
  gboolean autosync_enabled;
  struct tm local_time;
  gboolean state;
  GtkWidget *date_button;
  GtkWidget *date_label;
  GtkWidget *hbox1;
  GtkWidget *dialog;
  GtkWidget *clock_24h_button;
  osso_context_t *osso;
  HildonWindow *parent;
  GtkWidget *autosync_button;
  GtkWidget *adj_dt_label;
  GtkWidget *time_button;
  GtkWidget *time_label;
  GtkWidget *align;
  GtkWidget *tz_button;
  GtkWidget *net_time_label;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  Citytime *ct;
} cpa_dialog;

static gboolean *global_state = NULL;
static gboolean time_changed = FALSE;

static void
cpa_hide_ui(cpa_dialog *dialog)
{
  gtk_widget_hide(dialog->clock_24h_button);
  gtk_widget_hide(dialog->autosync_button);
  gtk_widget_hide(dialog->adj_dt_label);
  gtk_widget_hide(dialog->tz_button);
  gtk_widget_hide(dialog->date_button);
  gtk_widget_hide(dialog->time_button);
  gtk_widget_hide(dialog->hbox2);
  gtk_widget_hide(dialog->hbox1);
  gtk_widget_hide(dialog->align);
  gtk_widget_hide(dialog->vbox);
  gtk_widget_hide(dialog->dialog);
  gtk_widget_queue_resize(dialog->dialog);
}

static void
cpa_show_autosync_widgets(cpa_dialog *dialog, gboolean show)
{
#if 0
  gint width;

  gtk_window_get_size(GTK_WINDOW(dialog->dialog), &width, NULL);
#endif
  if (show)
  {
    gtk_widget_show_all(dialog->hbox2);
    gtk_widget_show_all(dialog->hbox1);
    gtk_widget_show_all(dialog->align);
    gtk_widget_hide(dialog->tz_button);
    gtk_widget_hide(dialog->date_button);
    gtk_widget_hide(dialog->time_button);
  }
  else
  {
    gtk_widget_hide(dialog->hbox2);
    gtk_widget_hide(dialog->hbox1);
    gtk_widget_hide(dialog->align);
    gtk_widget_show(dialog->tz_button);
    gtk_widget_show(dialog->date_button);
    gtk_widget_show(dialog->time_button);
  }

  gtk_widget_show(dialog->vbox);
  gtk_window_resize(GTK_WINDOW(dialog->dialog), 1, 1);
  gtk_widget_queue_resize(dialog->dialog);
}

static void
cpa_unhide_hide_ui(cpa_dialog *dialog)
{
  gboolean autosync_available = clock_is_autosync_available();

  gtk_widget_show(dialog->clock_24h_button);

  if (autosync_available)
  {
    gtk_widget_hide(dialog->adj_dt_label);
    gtk_widget_show(dialog->autosync_button);
  }
  else
  {
    gtk_widget_show(dialog->adj_dt_label);
    gtk_widget_hide(dialog->autosync_button);
  }

  cpa_show_autosync_widgets(dialog,
                            dialog->autosync_enabled && autosync_available);
  gtk_widget_show(dialog->dialog);
  gtk_widget_queue_resize(dialog->dialog);
}

static void
cpa_write_city_zone(HildonButton *button, GtkLabel *label, Citytime *ct)
{
  gint gmt_offset;
  gchar *city;
  gint hours;
  gchar *country;
  gchar *city_zone;

  gmt_offset = clock_citytime_get_gmt_offset(ct);
  hours = gmt_offset / 3600;
  city = clock_citytime_get_city(ct);
  country = clock_citytime_get_country(ct);

  if (gmt_offset == 3600 * hours)
  {
    const char *fmt = dgettext("osso-clock", "cloc_fi_timezonefull");

    city_zone = g_strdup_printf(fmt, hours, city, country);
  }
  else
  {
    gint mins = (gmt_offset / 60) % 60;
    const char *fmt = dgettext("osso-clock", "cloc_fi_timezonefull_minutes");

    if (mins < 0)
      mins = -mins;

    city_zone = g_strdup_printf(fmt, hours, mins, city, country);
  }

  hildon_button_set_value(button, city_zone);

  if (label)
    gtk_label_set_text(label, city_zone);

  g_free(city_zone);
}

static void
cpa_get_time_zone(GtkWidget *button, cpa_dialog *dialog)
{
  HildonTimeZoneChooser *tz_chooser;
  FeedbackDialogResponse res;
  time_t tick;
  Cityinfo *city;
  Citytime *ct;
  int zones_differ;
  gchar *tz;

  dialog->state = 1;
  tz_chooser = hildon_time_zone_chooser_new();
  hildon_time_zone_chooser_set_city(tz_chooser, dialog->ct->city);
  cpa_hide_ui(dialog);
  res = hildon_time_zone_chooser_run(tz_chooser);
  gtk_window_present(GTK_WINDOW(dialog->parent));

  if (res == FEEDBACK_DIALOG_RESPONSE_CITY_CHOSEN)
  {
    struct tm tm;

    time_get_local(&tm);
    tick = time_mktime(&tm, clock_citytime_get_zone(dialog->ct));
    city = hildon_time_zone_chooser_get_city(tz_chooser);
    ct = clock_get_time_in_city(city);
    zones_differ = g_strcmp0(clock_citytime_get_zone(dialog->ct),
                             cityinfo_get_zone(city));
    clock_citytime_free(dialog->ct);
    dialog->ct = ct;

    cpa_write_city_zone(HILDON_BUTTON(dialog->tz_button),
                        GTK_LABEL(dialog->net_time_label), ct);
    tz = cityinfo_get_zone(city);
    time_get_remote(tick, tz, &tm);
    dialog->local_time.tm_mday = tm.tm_mday;
    dialog->local_time.tm_mon = tm.tm_mon;
    dialog->local_time.tm_year = tm.tm_year;
    dialog->local_time.tm_hour = tm.tm_hour;
    dialog->local_time.tm_min = tm.tm_min;

    hildon_date_button_set_date(HILDON_DATE_BUTTON(dialog->date_button),
                                dialog->local_time.tm_year + 1900,
                                dialog->local_time.tm_mon,
                                dialog->local_time.tm_mday);

    if (zones_differ)
    {
      hildon_time_button_set_time(HILDON_TIME_BUTTON(dialog->time_button),
                                  dialog->local_time.tm_hour,
                                  dialog->local_time.tm_min);
      gtk_label_set_text(
            GTK_LABEL(dialog->time_label),
            hildon_button_get_value(HILDON_BUTTON(dialog->time_button)));
    }

    gtk_label_set_text(
          GTK_LABEL(dialog->date_label),
          hildon_button_get_value(HILDON_BUTTON(dialog->date_button)));
  }

  hildon_time_zone_chooser_free(tz_chooser);
  cpa_unhide_hide_ui(dialog);
  dialog->state = 0;
}

static void
cpa_update_remote_time(cpa_dialog *dialog, const char *fmt)
{
  gchar *net_time;
  struct tm *tm;
  const char *time_fmt;
  char buf[256];
  time_t tick;

  if (time_get_net_time(&tick, buf, sizeof(buf)) != -1)
  {
    if (g_str_has_prefix(buf, ":Etc/"))
    {
      if (g_str_has_prefix(buf, ":Etc/GMT+") && g_strcmp0(buf, ":Etc/GMT+0"))
        *strchr(buf, '+') = '-';
      else if (g_str_has_prefix(buf, ":Etc/GMT-") &&
               g_strcmp0(buf, ":Etc/GMT-0"))
      {
        *strchr(buf, '-') = '+';
      }

      net_time = g_strdup(&buf[5]);
    }
    else
      net_time = g_strdup(buf);

    gtk_label_set_text(GTK_LABEL(dialog->net_time_label), net_time);
    g_free(net_time);

    tm = localtime(&tick);
    time_format_time(
          tm, dgettext("hildon-libs", "wdgt_va_date_long"), buf, sizeof(buf));
    gtk_label_set_text(GTK_LABEL(dialog->date_label), buf);

    if (g_strcmp0(fmt, "%r"))
      time_fmt = dgettext("hildon-libs", "wdgt_va_24h_time");
    else if (tm->tm_hour < 12)
      time_fmt = dgettext("hildon-libs", "wdgt_va_12h_time_am");
    else
      time_fmt = dgettext("hildon-libs", "wdgt_va_12h_time_pm");

    time_format_time(tm, time_fmt, buf, sizeof(buf));
    gtk_label_set_text(GTK_LABEL(dialog->time_label), buf);
  }
}

static void
_time_button_value_changed(HildonPickerButton *widget, gpointer user_data)
{
  time_changed = TRUE;
}

static gboolean
cpa_update_ui(cpa_dialog *dialog)
{
  time_get_synced();
  dialog->autosync_enabled = time_get_autosync();
#if 0
  hildon_check_button_get_active(HILDON_CHECK_BUTTON(dialog->autosync_button));
#endif
  if (dialog->autosync_enabled)
  {
    char fmt[16];

    time_get_time_format(fmt, sizeof(fmt));
    cpa_update_remote_time(dialog, fmt);
  }

  dialog->ct = clock_get_home_location();

  if (time_get_local(&dialog->local_time) == -1)
    g_debug("time not available");

  hildon_check_button_set_active(HILDON_CHECK_BUTTON(dialog->autosync_button),
                                 dialog->autosync_enabled);

  hildon_check_button_set_active(HILDON_CHECK_BUTTON(dialog->clock_24h_button),
                                 clock_get_is_24h_format());

  cpa_write_city_zone(HILDON_BUTTON(dialog->tz_button), NULL, dialog->ct);

  hildon_date_button_set_date(HILDON_DATE_BUTTON(dialog->date_button),
                              dialog->local_time.tm_year + 1900,
                              dialog->local_time.tm_mon,
                              dialog->local_time.tm_mday);
  hildon_time_button_set_time(HILDON_TIME_BUTTON(dialog->time_button),
                              dialog->local_time.tm_hour,
                              dialog->local_time.tm_min);
  time_changed = FALSE;
  g_signal_connect(G_OBJECT(dialog->time_button), "value-changed",
                   G_CALLBACK(_time_button_value_changed), dialog);

  return TRUE;
}

static gboolean
cpa_state_load(gboolean *state_data, osso_context_t *osso)
{
  osso_state_t state;

  state.state_size = sizeof(*state_data);
  state.state_data = state_data;

  return osso_state_read(osso, &state) == OSSO_OK;
}

static gint
cpa_dialog_run(cpa_dialog *dialog)
{
  cpa_update_ui(dialog);

  if (!cpa_state_load(&dialog->state, dialog->osso))
    dialog->state = 0;

  if (dialog->state == 0)
  {
    cpa_unhide_hide_ui(dialog);
    return gtk_dialog_run(GTK_DIALOG(dialog->dialog));
  }
  else
  {
    if (dialog->state == 1)
      cpa_get_time_zone(dialog->tz_button, dialog);

    return 1;
  }
}

static void
_clock_24h_button_toggled_cb(HildonCheckButton *button, gpointer user_data)
{
  cpa_dialog *dialog = user_data;
  gboolean active;
  HildonTouchSelector *selector;

  active = hildon_check_button_get_active(HILDON_CHECK_BUTTON(button));
  selector = hildon_picker_button_get_selector(
        HILDON_PICKER_BUTTON(dialog->time_button));

  if (active)
    active = TRUE;

  g_object_set(G_OBJECT(selector), "time-format-policy", active, NULL);

  if (hildon_check_button_get_active(HILDON_CHECK_BUTTON(dialog->autosync_button)))
  {
    if (active)
      cpa_update_remote_time(dialog, "%R");
    else
      cpa_update_remote_time(dialog, "%r");
  }
}

static gboolean
_idle_present_parent_window_cb(gpointer user_data)
{
  cpa_dialog *dialog = user_data;

  if (dialog)
  {
    cpa_dialog_run(dialog);
    gtk_window_present(GTK_WINDOW(dialog->parent));
  }

  return FALSE;
}

static gboolean
cpa_save_settings(cpa_dialog *dialog)
{
  clock_set_24h_format(hildon_check_button_get_active(
                         HILDON_CHECK_BUTTON(dialog->clock_24h_button)));

  if (clock_is_autosync_available())
  {
    clock_enable_autosync(dialog->autosync_enabled);

    if (!dialog->autosync_enabled)
    {
      guint year = 0;
      guint minutes = 0;
      guint hours = 0;
      guint day = 0;
      guint month = 0;
      Citytime *ct;

      hildon_date_button_get_date(HILDON_DATE_BUTTON(dialog->date_button),
                                  &year, &month, &day);
      hildon_time_button_get_time(HILDON_TIME_BUTTON(dialog->time_button),
                                  &hours, &minutes);
      ct = clock_change_current_location(dialog->ct->city);

      if (time_changed)
        clock_change_time(year, month + 1, day, hours, minutes);
      else
        clock_change_time(year, month + 1, day, -1, -1);

      if (ct)
        clock_citytime_free(ct);
      else
        g_debug("There was an error while setting new home city!");
    }
  }
  else if (!dialog->autosync_enabled)
    clock_enable_autosync(FALSE);

  return TRUE;
}

static void
_sync_button_toggled_cb(HildonCheckButton *button, gpointer user_data)
{
  cpa_dialog *dialog = user_data;

  dialog->autosync_enabled =
      hildon_check_button_get_active(HILDON_CHECK_BUTTON(button));

  cpa_show_autosync_widgets(dialog, dialog->autosync_enabled);

  if (dialog->autosync_enabled)
  {
    if (hildon_check_button_get_active(
          HILDON_CHECK_BUTTON(dialog->clock_24h_button)))
    {
      cpa_update_remote_time(dialog, "%R");
    }
    else
      cpa_update_remote_time(dialog, "%r");
  }
}

static void
_dialog_response_cb(GtkDialog *_dialog, gint response_id, gpointer user_data)
{
  cpa_dialog *dialog = user_data;
  GtkWindow *transient;

  if (response_id == GTK_RESPONSE_APPLY)
    cpa_save_settings(dialog);
  else if (response_id != GTK_RESPONSE_DELETE_EVENT)
  {
    g_debug("Wrong response ID! Some Error had occured.");
    return;
  }

  cpa_hide_ui(dialog);
  transient = gtk_window_get_transient_for(GTK_WINDOW(dialog->dialog));

  if (transient)
    gtk_widget_set_sensitive(GTK_WIDGET(transient), TRUE);

  gtk_widget_destroy(GTK_WIDGET(dialog->dialog));
  g_object_unref(dialog->vbox);
  clock_citytime_free(dialog->ct);
  gtk_main_quit();
}

static cpa_dialog *
cpa_dialog_new(GtkWindow *parent)
{
  cpa_dialog *dialog;
  GtkSizeGroup *title_size_group;
  GtkWidget *tz_label;
  GtkWidget *date_label;
  GtkWidget *date_align;
  GtkWidget *time_label;
  GtkWidget *time_align;
  GtkWidget *hbox;
  HildonTouchSelector *selector;
  GtkSizeGroup *value_size_group;
  GtkSizeGroup *size_group;
  GtkWidget *tz_align;

  dialog = g_try_new0(cpa_dialog, 1);

  if (!dialog)
    return NULL;

  title_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  if (!title_size_group)
    return NULL;

  value_size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  if (!value_size_group)
    return NULL;

  size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  if (!size_group)
    return NULL;

  dialog->clock_24h_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);

  if (!dialog->clock_24h_button)
    return NULL;

  gtk_button_set_label(GTK_BUTTON(dialog->clock_24h_button),
                       dgettext("osso-clock", "dati_fi_24_clock"));

  dialog->autosync_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);

  if (!dialog->autosync_button)
    return NULL;

  gtk_button_set_label(GTK_BUTTON(dialog->autosync_button),
                       dgettext("osso-clock", "dati_fi_time_synchronization"));

  dialog->adj_dt_label = gtk_label_new(
        dgettext("osso-clock", "dati_ia_adjust_date_and_time"));

  if (!dialog->adj_dt_label)
    return NULL;

  tz_label = gtk_label_new(dgettext("osso-clock", "dati_ia_pr_timezone"));
  gtk_misc_set_alignment(GTK_MISC(tz_label), 0.0, 0.5);
  tz_align = gtk_alignment_new(0.0, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(tz_align), 0, 0, 16, 0);
  gtk_container_add(GTK_CONTAINER(tz_align), tz_label);
  hildon_helper_set_logical_color(tz_label, GTK_RC_FG, GTK_STATE_NORMAL,
                                  "SecondaryTextColor");

  dialog->net_time_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(dialog->net_time_label), 0.0, 0.5);
  dialog->hbox2 = gtk_hbox_new(FALSE, 16);
  gtk_box_pack_start(GTK_BOX(dialog->hbox2), tz_align, FALSE, FALSE, 0);
  gtk_box_pack_start(
        GTK_BOX(dialog->hbox2), dialog->net_time_label, TRUE, TRUE, 0);

  dialog->tz_button =
      hildon_button_new_with_text(HILDON_SIZE_FINGER_HEIGHT,
                                  HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
                                  dgettext("osso-clock", "dati_ia_pr_timezone"),
                                  NULL);
  if (!dialog->hbox2)
    return NULL;

  if (!dialog->net_time_label)
    return NULL;

  if (!dialog->tz_button)
    return NULL;

  hildon_button_set_style(HILDON_BUTTON(dialog->tz_button),
                          HILDON_BUTTON_STYLE_PICKER);

  date_label = gtk_label_new(dgettext("osso-clock", "dati_fi_pr_date"));
  gtk_misc_set_alignment(GTK_MISC(date_label), 0.0, 0.5);
  date_align = gtk_alignment_new(0.0, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(date_align), 0, 0, 16, 0);
  gtk_container_add(GTK_CONTAINER(date_align), date_label);
  hildon_helper_set_logical_color(date_label, GTK_RC_FG, GTK_STATE_NORMAL,
                                  "SecondaryTextColor");

  dialog->date_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(dialog->date_label), 0.0, 0.5);

  dialog->hbox1 = gtk_hbox_new(FALSE, 16);
  gtk_box_pack_start(GTK_BOX(dialog->hbox1), date_align, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dialog->hbox1), dialog->date_label, TRUE, TRUE, 0);

  dialog->date_button = hildon_date_button_new_with_year_range(
        HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
        2009, 2036);

  if (!dialog->hbox1)
    return NULL;

  if (!dialog->date_label)
    return NULL;

  if (!dialog->date_button)
    return NULL;

  time_label = gtk_label_new(dgettext("osso-clock", "dati_fi_pr_time"));
  gtk_misc_set_alignment(GTK_MISC(time_label), 0.0, 0.5);
  time_align = gtk_alignment_new(0.0, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(time_align), 0, 0, 16, 0);
  gtk_container_add(GTK_CONTAINER(time_align), time_label);
  hildon_helper_set_logical_color(time_label, GTK_RC_FG, GTK_STATE_NORMAL,
                                  "SecondaryTextColor");

  dialog->time_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(dialog->time_label), 0.0, 0.5);
  dialog->align = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(dialog->align), 0, 0, 0, 0);

  hbox = gtk_hbox_new(FALSE, 16);
  gtk_container_add(GTK_CONTAINER(dialog->align), hbox);
  gtk_box_pack_start(GTK_BOX(hbox), time_align, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), dialog->time_label, TRUE, TRUE, 0);

  dialog->time_button =
      hildon_time_button_new(HILDON_SIZE_FINGER_HEIGHT,
                             HILDON_BUTTON_ARRANGEMENT_HORIZONTAL);

  if (!dialog->align)
    return NULL;

  if (!dialog->time_label)
    return NULL;

  if (!dialog->time_button)
    return NULL;

  selector = hildon_picker_button_get_selector(
        HILDON_PICKER_BUTTON(dialog->time_button));
  g_object_set(G_OBJECT(selector),
               "time-format-policy", clock_get_is_24h_format(),
               NULL);

  gtk_size_group_add_widget(GTK_SIZE_GROUP(size_group), tz_align);
  gtk_size_group_add_widget(GTK_SIZE_GROUP(size_group), date_align);
  gtk_size_group_add_widget(GTK_SIZE_GROUP(size_group), time_align);

  hildon_button_add_size_groups(HILDON_BUTTON(dialog->tz_button),
                                title_size_group, value_size_group, NULL);
  hildon_button_add_size_groups(HILDON_BUTTON(dialog->date_button),
                                title_size_group, value_size_group, NULL);
  hildon_button_add_size_groups(HILDON_BUTTON(dialog->time_button),
                                title_size_group, value_size_group, NULL);

  dialog->vbox = gtk_vbox_new(FALSE, 0);

  if (!dialog->vbox)
    return NULL;

  g_object_ref_sink(G_OBJECT(dialog->vbox));
  gtk_box_pack_start(
        GTK_BOX(dialog->vbox), dialog->clock_24h_button, FALSE, FALSE, 0);
  gtk_box_pack_start(
        GTK_BOX(dialog->vbox), dialog->autosync_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->tz_button, FALSE, FALSE, 0);
  gtk_box_pack_start(
        GTK_BOX(dialog->vbox), dialog->adj_dt_label, FALSE, FALSE, 0);
  gtk_box_pack_start(
        GTK_BOX(dialog->vbox), dialog->date_button, FALSE, FALSE, 0);
  gtk_box_pack_start(
        GTK_BOX(dialog->vbox), dialog->time_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->hbox1, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->align, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(dialog->autosync_button), "toggled",
                   G_CALLBACK(_sync_button_toggled_cb), dialog);
  g_signal_connect(G_OBJECT(dialog->clock_24h_button), "toggled",
                   G_CALLBACK(_clock_24h_button_toggled_cb), dialog);
  g_signal_connect(dialog->tz_button, "clicked",
                   G_CALLBACK(cpa_get_time_zone), dialog);

  dialog->dialog = gtk_dialog_new_with_buttons(
        dgettext("osso-clock", "dati_ap_application_title"),
        GTK_WINDOW(parent),
        GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
        NULL);

  if (!dialog->dialog)
    return NULL;

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), dialog->vbox,
                     FALSE, FALSE, 0);
  gtk_dialog_add_buttons(GTK_DIALOG(dialog->dialog),
                         dgettext("hildon-libs", "wdgt_bd_save"),
                         GTK_RESPONSE_APPLY, NULL);
  g_signal_connect(dialog->dialog, "response",
                   G_CALLBACK(_dialog_response_cb), dialog);

  g_object_unref(title_size_group);
  g_object_unref(value_size_group);
  g_object_unref(size_group);

  return dialog;
}

osso_return_t
execute(osso_context_t *osso, gpointer data, gboolean user_activated)
{
  GtkWindow *parent = (GtkWindow *)data;
  osso_return_t rv = OSSO_ERROR;

  bindtextdomain("osso-clock", LOCALE_DIR);
  bindtextdomain("hildon-libs", LOCALE_DIR);

  if ( parent )
  {
    cpa_dialog *dialog = cpa_dialog_new(parent);

    if (dialog)
    {
      global_state = &dialog->state;
      dialog->parent = HILDON_WINDOW(parent);
      g_idle_add(_idle_present_parent_window_cb, dialog);
      gtk_main();
      rv =  OSSO_OK;
    }
    else
      g_critical("Unable to create applet dialog, exiting..");
  }
  else
    g_critical("Missing data");

  return rv;
}

static gboolean
cpa_state_save(gboolean state_data, osso_context_t *osso)
{
  osso_state_t state;

  state.state_size = sizeof(state_data);
  state.state_data = &state_data;

  return osso_state_write(osso, &state) == OSSO_OK ;
}

osso_return_t
save_state(osso_context_t *osso, gpointer user_data)
{
  cpa_state_save(*global_state, osso);
  gtk_widget_destroy(GTK_WIDGET(user_data));

  return OSSO_OK;
}

GtkWidget *
cpa_get_dialog_widget(cpa_dialog *dialog)
{
  return GTK_WIDGET(dialog->dialog);
}
