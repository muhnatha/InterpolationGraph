#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    GtkWidget   *window;
    GtkWidget   *spin_n;
    GtkWidget   *btn_set;
    GtkWidget   *grid;
    GtkWidget   *btn_interp;
    GtkWidget   *textview;
    GtkWidget   *drawing_area;
    GPtrArray   *entry_pairs;
    float       *xs_interp;
    float       *ys_interp;
    int          interp_len;
} AppWidgets;

static float linear(float x, float x1, float y1, float x2, float y2) {
    return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    AppWidgets *app = data;
    if (app->interp_len < 2) return FALSE;

    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    double width  = alloc.width;
    double height = alloc.height;
    double margin = 40;

    float xmin = app->xs_interp[0];
    float xmax = app->xs_interp[app->interp_len-1];
    float ymin = app->ys_interp[0];
    float ymax = ymin;
    for (int i = 1; i < app->interp_len; i++) {
        if (app->ys_interp[i] < ymin) ymin = app->ys_interp[i];
        if (app->ys_interp[i] > ymax) ymax = app->ys_interp[i];
    }

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, margin, height - margin);
    cairo_line_to(cr, width - margin, height - margin);
    cairo_move_to(cr, margin, margin);
    cairo_line_to(cr, margin, height - margin);
    cairo_stroke(cr);

    cairo_set_line_width(cr, 1.5);
    cairo_set_source_rgb(cr, 0.2, 0.6, 0.8);

    double x0 = app->xs_interp[0];
    double y0 = app->ys_interp[0];
    double px = margin + (x0 - xmin) / (xmax - xmin) * (width - 2*margin);
    double py = height - margin - (y0 - ymin) / (ymax - ymin) * (height - 2*margin);
    cairo_move_to(cr, px, py);

    for (int i = 1; i < app->interp_len; i++) {
        double x = app->xs_interp[i];
        double y = app->ys_interp[i];
        double mx = margin + (x - xmin) / (xmax - xmin) * (width - 2*margin);
        double my = height - margin - (y - ymin) / (ymax - ymin) * (height - 2*margin);
        cairo_line_to(cr, mx, my);
    }
    cairo_stroke(cr);

    return FALSE;
}

static void on_set_points(GtkButton *btn, AppWidgets *app) {
    int n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->spin_n));
    GList *kids = gtk_container_get_children(GTK_CONTAINER(app->grid));
    for (GList *it = kids; it; it = it->next)
        gtk_widget_destroy(GTK_WIDGET(it->data));
    g_list_free(kids);
    g_ptr_array_set_size(app->entry_pairs, 0);

    for (int i = 0; i < n; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "P%d:", i+1);
        GtkWidget *lbl = gtk_label_new(buf);
        gtk_grid_attach(GTK_GRID(app->grid), lbl, 0, i, 1, 1);

        GtkWidget *ex = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(ex), "x");
        gtk_grid_attach(GTK_GRID(app->grid), ex, 1, i, 1, 1);

        GtkWidget *ey = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(ey), "y");
        gtk_grid_attach(GTK_GRID(app->grid), ey, 2, i, 1, 1);

        g_ptr_array_add(app->entry_pairs, ex);
        g_ptr_array_add(app->entry_pairs, ey);
    }
    gtk_widget_show_all(app->grid);
}

static void on_interpolate(GtkButton *btn, AppWidgets *app) {
    int n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(app->spin_n));
    if ((int)app->entry_pairs->len != 2*n) return;

    float *xs = g_new(float, n);
    float *ys = g_new(float, n);
    for (int i = 0; i < n; i++) {
        xs[i] = atof(gtk_entry_get_text(GTK_ENTRY(g_ptr_array_index(app->entry_pairs,2*i))));
        ys[i] = atof(gtk_entry_get_text(GTK_ENTRY(g_ptr_array_index(app->entry_pairs,2*i+1))));
    }

    float x_min = xs[0], x_max = xs[n-1];
    int steps = (int)floor((x_max - x_min)/0.5) + 1;
    app->interp_len = steps;

    g_free(app->xs_interp);
    g_free(app->ys_interp);
    app->xs_interp = g_new(float, steps);
    app->ys_interp = g_new(float, steps);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->textview));
    gtk_text_buffer_set_text(buf, "", -1);

    for (int i = 0; i < steps; i++) {
        float x = x_min + 0.5f * i;
        float y = 0;
        for (int j = 0; j < n-1; j++) {
            if (x >= xs[j] && x <= xs[j+1]) {
                y = linear(x, xs[j], ys[j], xs[j+1], ys[j+1]);
                break;
            }
        }
        app->xs_interp[i] = x;
        app->ys_interp[i] = y;
        char line[64];
        snprintf(line, sizeof(line), "x=%.2f, y=%.2f\n", x, y);
        gtk_text_buffer_insert_at_cursor(buf, line, -1);
    }

    gtk_widget_queue_draw(app->drawing_area);

    g_free(xs);
    g_free(ys);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    AppWidgets *app = g_slice_new(AppWidgets);
    app->entry_pairs = g_ptr_array_new();
    app->xs_interp = app->ys_interp = NULL;
    app->interp_len = 0;

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Linear Interpolator");
    gtk_container_set_border_width(GTK_CONTAINER(app->window), 10);
    gtk_window_set_default_size(GTK_WINDOW(app->window), 600, 600);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);

    GtkWidget *h1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), h1, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(h1), gtk_label_new("Number of points:"), FALSE, FALSE, 0);
    app->spin_n = gtk_spin_button_new_with_range(2, 100, 1);
    gtk_box_pack_start(GTK_BOX(h1), app->spin_n, FALSE, FALSE, 0);

    app->btn_set = gtk_button_new_with_label("Set Points");
    g_signal_connect(app->btn_set, "clicked", G_CALLBACK(on_set_points), app);
    gtk_box_pack_start(GTK_BOX(h1), app->btn_set, FALSE, FALSE, 0);

    app->btn_interp = gtk_button_new_with_label("Interpolate");
    g_signal_connect(app->btn_interp, "clicked", G_CALLBACK(on_interpolate), app);
    gtk_box_pack_start(GTK_BOX(h1), app->btn_interp, FALSE, FALSE, 0);

    app->grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(app->grid), 4);
    GtkWidget *scroll_in = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_in), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll_in), app->grid);
    gtk_box_pack_start(GTK_BOX(vbox), scroll_in, FALSE, FALSE, 0);

    app->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(app->drawing_area, 500, 300);
    g_signal_connect(app->drawing_area, "draw", G_CALLBACK(on_draw), app);
    gtk_box_pack_start(GTK_BOX(vbox), app->drawing_area, TRUE, TRUE, 0);

    app->textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->textview), FALSE);
    GtkWidget *scroll_out = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_out), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll_out), app->textview);
    gtk_box_pack_start(GTK_BOX(vbox), scroll_out, TRUE, TRUE, 0);

    // initialize with default entries
    on_set_points(NULL, app);

    gtk_widget_show_all(app->window);
    gtk_main();

    g_ptr_array_free(app->entry_pairs, TRUE);
    g_free(app->xs_interp);
    g_free(app->ys_interp);
    g_slice_free(AppWidgets, app);
    return 0;
}
