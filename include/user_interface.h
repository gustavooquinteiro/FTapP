#ifndef _USER_INTERFACE_H
#define _USER_INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

typedef struct applicationWindow ApplicationWindow;

int app_start();
static void file_select_callback(GtkWidget *widget, gpointer data);
static void app_activate(GtkApplication *app);
static ApplicationWindow *create_application_window();

#endif
