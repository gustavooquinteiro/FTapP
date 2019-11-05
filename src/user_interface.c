#include "../include/user_interface.h"
#include "../include/client.h"

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

typedef struct applicationWindow ApplicationWindow;

static void file_select_callback(GtkWidget *widget, gpointer data);
static void app_activate(GtkApplication *app);
static ApplicationWindow *create_application_window();
static void update_selected_filename(ApplicationWindow *app_window, char *name);
static void display_send_result_dialog(int send_result);

static GtkWidget *app_container;

char** filename;

struct applicationWindow
{
    GtkWidget *grid;
    GtkWidget *send_btn;
    GtkWidget *select_file_btn;
    GtkWidget *ip_addr_textbox;
    GtkWidget *file_select_dialog;
    GtkWidget *selected_file_label;
    GtkWidget *ip_addr_label;
};

static ApplicationWindow *app_window;


static void send(GtkWidget *widget, gpointer data)
{
    char** name = data;
    GtkEntry * entry = GTK_ENTRY(app_window->ip_addr_textbox);
    char* ip = gtk_entry_get_text(entry);
    printf("\nIP FODA = %s\n", ip);
    display_send_result_dialog(send_file(name[0], ip));
}

static void display_send_result_dialog(int send_result)
{
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;

    GtkWidget* dialog;
    GtkWidget* content_area;
    GtkWidget* label;

    switch(send_result)
    {
        case SUCCESS:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Arquivo enviado com sucesso.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case REQUEST_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro no requerimento.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case RESPONSE_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro na resposta.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case SEND_INFO_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro ao enviar info.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case SERVER_CONFIRM_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro na confirmação do penis.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case CONN_SOCKET_CREATION_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro na criação do penis.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case FILE_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro no arquivo.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;

        case SEND_FILE_ERROR:
            dialog = gtk_message_dialog_new (GTK_WINDOW(app_container), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Erro no envio do penis");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            break;
    }
}

int app_start()
{
    GtkApplication *app;
    int status;
    filename = (char**)malloc(sizeof(char*));

    app = gtk_application_new("ufba.mata59.trabalho", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    status = g_application_run(G_APPLICATION (app), 0, NULL);
    g_object_unref(app);

    return status;
}

static void app_activate(GtkApplication *app)
{
    app_window = create_application_window();
    app_container = gtk_application_window_new(app);
    g_signal_connect(GTK_WINDOW(app_container), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_title (GTK_WINDOW (app_container), "Trabalho MATA59");
    gtk_window_set_default_size (GTK_WINDOW (app_container), 200, 200);
    gtk_window_set_resizable(GTK_WINDOW (app_container), FALSE);

    app_window->grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(app_window->grid), 5);
    gtk_widget_set_vexpand(app_window->grid, TRUE);
    gtk_container_add(GTK_CONTAINER(app_container), app_window->grid);

    app_window->ip_addr_label = gtk_label_new("Digite o endereço IP:");
    gtk_label_set_markup(GTK_LABEL(app_window->ip_addr_label), "<span font_desc=\"16.0\">Digite o endereço IP:</span>");
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->ip_addr_label, 0, 0, 1, 1);

    app_window->ip_addr_textbox = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app_window->ip_addr_textbox), "Digite o endereco IP do destinatario");
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->ip_addr_textbox, 1, 0, 1, 1);

    app_window->send_btn = gtk_button_new_with_label("Enviar");
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->send_btn, 2, 0, 1, 1);
    g_signal_connect(app_window->send_btn, "clicked", G_CALLBACK(send), filename);

    app_window->select_file_btn = gtk_button_new_with_label("Selecionar arquivo");
    g_signal_connect(app_window->select_file_btn, "clicked", G_CALLBACK(file_select_callback), NULL);
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->select_file_btn, 3, 0, 1, 1);


    app_window->selected_file_label = gtk_label_new("Arquivo escolhido: ");
    gtk_label_set_markup(GTK_LABEL(app_window->selected_file_label), "<span font_desc=\"16.0\">Arquivo escolhido</span>");
    gtk_grid_attach(GTK_GRID(app_window->grid), app_window->selected_file_label, 0, 1, 1, 1);

    gtk_widget_show_all(app_container);
}

static void file_select_callback(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Abrir arquivo", GTK_WINDOW(app_container), action, "Cancelar", GTK_RESPONSE_CANCEL, "Abrir", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run (GTK_DIALOG(dialog));

    if(res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename[0] = gtk_file_chooser_get_filename(chooser);

        FILE *file;
        file = fopen(filename[0], "r");

        if(file != NULL)
            update_selected_filename(app_window, filename[0]);
        fclose(file);
    }

    gtk_widget_destroy(dialog);
}

static void update_selected_filename(ApplicationWindow *app_window, char *name)
{
    char buffer[120];
    strcat(buffer, "Arquivo escolhido: ");
    strcat(buffer, name);
    char *new_filename = buffer;
    printf("Novo nome: %s", new_filename);
    char *format = "<span font_desc=\"16.0\">%s</span>";
    char *markup;
    markup = g_markup_printf_escaped(format, new_filename);
    gtk_label_set_markup(GTK_LABEL(app_window->selected_file_label), markup);
    g_free(markup);
}

static ApplicationWindow *create_application_window()
{
    return (ApplicationWindow*) malloc(sizeof(ApplicationWindow));
}
