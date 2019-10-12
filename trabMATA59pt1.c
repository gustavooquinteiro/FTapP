#include <gtk/gtk.h>
#include <stdio.h>

static GtkWidget *window;

static void
print_hello (GtkWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void procura_arquivo(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new ("Abrir arquivo", GTK_WINDOW(window), action, "Cancelar", GTK_RESPONSE_CANCEL, "Abrir", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));

    if(res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        FILE *arquivo;
        arquivo = fopen(filename, "r");
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *botao_enviar;
  GtkWidget *botao_procurar_arquivo;
  GtkWidget *endereco;
  GtkWidget *grid;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 200);

  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(window), grid);

  botao_enviar = gtk_button_new_with_label("Enviar");
  gtk_grid_attach(GTK_GRID(grid), botao_enviar, 1, 0, 1, 1);

  botao_procurar_arquivo = gtk_button_new_with_label("Procurar arquivo");
  g_signal_connect(botao_procurar_arquivo, "clicked", G_CALLBACK(procura_arquivo), NULL);
  gtk_grid_attach(GTK_GRID(grid), botao_procurar_arquivo, 2, 0, 1, 1);

  endereco = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), endereco, 0, 0, 1, 1);

  gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
