#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>

#define APP_ID "com.brightness.control"
#define APP_EXECUTABLE "/home/alex/.local/bin/brightness"

static void open_application(
    GtkMenuItem *item,
    gpointer user_data)
{
    (void)item;
    (void)user_data;

    GError *error = NULL;

    if (!g_spawn_command_line_async(APP_EXECUTABLE, &error))
    {
        g_printerr(
            "Could not launch brightness control: %s\n",
            error->message
        );

        g_error_free(error);
    }
}

static void quit_application(
    GtkMenuItem *item,
    gpointer user_data)
{
    (void)item;
    (void)user_data;

    gtk_main_quit();
}

int main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    AppIndicator *indicator =
        app_indicator_new(
            APP_ID,
            APP_ID,
            APP_INDICATOR_CATEGORY_APPLICATION_STATUS
        );

    app_indicator_set_status(
        indicator,
        APP_INDICATOR_STATUS_ACTIVE
    );

    app_indicator_set_title(
        indicator,
        "Brightness Control"
    );

    GtkWidget *menu = gtk_menu_new();

    GtkWidget *open_item =
        gtk_menu_item_new_with_label("Open Brightness Control");

    GtkWidget *separator =
        gtk_separator_menu_item_new();

    GtkWidget *quit_item =
        gtk_menu_item_new_with_label("Quit");

    g_signal_connect(
        open_item,
        "activate",
        G_CALLBACK(open_application),
        NULL
    );

    g_signal_connect(
        quit_item,
        "activate",
        G_CALLBACK(quit_application),
        NULL
    );

    gtk_menu_shell_append(
        GTK_MENU_SHELL(menu),
        open_item
    );

    gtk_menu_shell_append(
        GTK_MENU_SHELL(menu),
        separator
    );

    gtk_menu_shell_append(
        GTK_MENU_SHELL(menu),
        quit_item
    );

    gtk_widget_show_all(menu);

    app_indicator_set_menu(
        indicator,
        GTK_MENU(menu)
    );

    gtk_main();

    g_object_unref(indicator);
    return 0;
}
