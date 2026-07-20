#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DISPLAYS 16

typedef struct
{
    int display_number;
    guint timeout_id;
    int pending_value;
} MonitorControl;



static void free_monitor_control(
    gpointer data,
    GClosure *closure)
{
    (void)closure;

    MonitorControl *monitor = data;

    if (monitor->timeout_id != 0)
    {
        g_source_remove(monitor->timeout_id);
    }

    g_free(monitor);
}

static gboolean tray_is_running(void)
{
    int status = system(
        "pgrep -x brightness-tray >/dev/null 2>&1"
    );

    return status == 0;
}

static gboolean window_closed(
    GtkWindow *window,
    gpointer user_data)
{
    (void)window;
    (void)user_data;

    if (!tray_is_running())
    {
        GError *error = NULL;

        if (!g_spawn_command_line_async(
                "/home/alex/.local/bin/brightness-tray",
                &error))
        {
            g_printerr(
                "Could not start tray: %s\n",
                error->message
            );

            g_error_free(error);
        }
    }

    /*
     * FALSE allows GTK to close the window normally.
     */
    return FALSE;
}


static int detect_displays(int displays[], int max_displays)
{
    FILE *pipe = popen("ddcutil detect --brief 2>/dev/null", "r");

    if (pipe == NULL)
    {
        return 0;
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), pipe) != NULL &&
           count < max_displays)
    {
        int display_number;

        if (sscanf(line, "Display %d", &display_number) == 1)
        {
            displays[count++] = display_number;
        }
    }

    pclose(pipe);
    return count;
}

static int get_brightness(int display_number)
{
    char command[128];

    snprintf(
        command,
        sizeof(command),
        "ddcutil getvcp 10 --brief --display %d 2>/dev/null",
        display_number
    );

    FILE *pipe = popen(command, "r");

    if (pipe == NULL)
    {
        return 50;
    }

    char line[256];
    int current_value = 50;
    int maximum_value = 100;

    while (fgets(line, sizeof(line), pipe) != NULL)
    {
        /*
         * Typical brief output contains:
         * VCP 10 C 50 100
         */
        if (sscanf(
                line,
                "VCP 10 C %d %d",
                &current_value,
                &maximum_value
            ) == 2)
        {
            break;
        }
    }

    pclose(pipe);

    if (maximum_value <= 0)
    {
        return 50;
    }

    return current_value * 100 / maximum_value;
}

static gboolean apply_brightness(gpointer data)
{
    MonitorControl *monitor = data;
    char command[160];

    snprintf(
        command,
        sizeof(command),
        "ddcutil setvcp 10 %d --display %d >/dev/null 2>&1",
        monitor->pending_value,
        monitor->display_number
    );

    system(command);

    monitor->timeout_id = 0;
    return G_SOURCE_REMOVE;
}

static void brightness_changed(GtkRange *range, gpointer data)
{
    MonitorControl *monitor = data;

    monitor->pending_value =
        (int)gtk_range_get_value(range);

    /*
     * Avoid running ddcutil for every tiny movement event.
     * Apply the final value after 150 ms.
     */
    if (monitor->timeout_id != 0)
    {
        g_source_remove(monitor->timeout_id);
    }

    monitor->timeout_id = g_timeout_add(
        150,
        apply_brightness,
        monitor
    );
}

static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window =
        gtk_application_window_new(app);


    g_signal_connect(
        window,
        "close-request",
        G_CALLBACK(window_closed),
        NULL
    );

    gtk_window_set_title(
        GTK_WINDOW(window),
        "Monitor Brightness"
    );

    gtk_window_set_default_size(
        GTK_WINDOW(window),
        420,
        100
    );

    GtkWidget *container =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    gtk_widget_set_margin_top(container, 16);
    gtk_widget_set_margin_bottom(container, 16);
    gtk_widget_set_margin_start(container, 16);
    gtk_widget_set_margin_end(container, 16);

    gtk_window_set_child(
        GTK_WINDOW(window),
        container
    );

    int displays[MAX_DISPLAYS];

    int display_count =
        detect_displays(displays, MAX_DISPLAYS);

    if (display_count == 0)
    {
        GtkWidget *label = gtk_label_new(
            "No DDC/CI-capable monitors detected."
        );

        gtk_box_append(GTK_BOX(container), label);
    }

    for (int i = 0; i < display_count; ++i)
    {
        MonitorControl *monitor =
            g_new0(MonitorControl, 1);

        monitor->display_number = displays[i];

        GtkWidget *row =
            gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

        char label_text[64];

        snprintf(
            label_text,
            sizeof(label_text),
            "Display %d",
            monitor->display_number
        );

        GtkWidget *label =
            gtk_label_new(label_text);

        gtk_widget_set_halign(
            label,
            GTK_ALIGN_START
        );

        GtkWidget *slider =
            gtk_scale_new_with_range(
                GTK_ORIENTATION_HORIZONTAL,
                0,
                100,
                1
            );

        gtk_scale_set_draw_value(
            GTK_SCALE(slider),
            TRUE
        );

        gtk_range_set_value(
            GTK_RANGE(slider),
            get_brightness(monitor->display_number)
        );

        g_signal_connect_data(
            slider,
            "value-changed",
            G_CALLBACK(brightness_changed),
            monitor,
            free_monitor_control,
            0
        );

        gtk_box_append(GTK_BOX(row), label);
        gtk_box_append(GTK_BOX(row), slider);
        gtk_box_append(GTK_BOX(container), row);
    }

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app =
        gtk_application_new(
            "com.brightness.control",
            G_APPLICATION_DEFAULT_FLAGS
        );

    g_signal_connect(
        app,
        "activate",
        G_CALLBACK(activate),
        NULL
    );

    int status = g_application_run(
        G_APPLICATION(app),
        argc,
        argv
    );

    g_object_unref(app);
    return status;
}

