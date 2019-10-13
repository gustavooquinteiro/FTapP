#include "../include/user_interface.h"

static GtkWidget *window;

struct applicationWindow
{
    GtkWidget *grid;
    GtkWidget *send_btn;
    GtkWidget *select_file_btn;
    GtkWidget *ip_addr_textbox;
    GtkWidget *file_select_dialog;
};

int app_start()
{
    GtkApplication *app;
    int status;

    app = gtk_application_new ("ufba.mata59.trabalho", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
    status = g_application_run (G_APPLICATION (app), 0, NULL);
    g_object_unref (app);

    return status;
}

static void app_activate(GtkApplication *app)
{
    ApplicationWindow *app_window = create_application_window();
    window = gtk_application_window_new(app);
    g_signal_connect(GTK_WINDOW(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_title (GTK_WINDOW (window), "Trabalho MATA59");
    gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);
    gtk_window_set_resizable(GTK_WINDOW (window), FALSE);

    app_window->grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(app_window->grid), 10);
    gtk_container_add(GTK_CONTAINER(window), app_window->grid);

    app_window->send_btn = gtk_button_new_with_label("Enviar");
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->send_btn, 1, 0, 1, 1);

    app_window->select_file_btn = gtk_button_new_with_label("Selecionar arquivo");
    g_signal_connect(app_window->select_file_btn, "clicked", G_CALLBACK(file_select_callback), NULL);
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->select_file_btn, 2, 0, 1, 1);

    app_window->ip_addr_textbox = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_window->ip_addr_textbox), "Digite o endereco IP do destinatario");
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->ip_addr_textbox, 0, 0, 1, 1);

    gtk_widget_show_all(window);
}

static void file_select_callback(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Abrir arquivo", GTK_WINDOW(window), action, "Cancelar", GTK_RESPONSE_CANCEL, "Abrir", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run (GTK_DIALOG(dialog));

    if(res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        FILE *file;
        file = fopen(filename, "r");
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static ApplicationWindow *create_application_window()
{
    return (ApplicationWindow*) malloc(sizeof(ApplicationWindow));
}
