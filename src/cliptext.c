/* cliptext.c - Adds clickable snippets to the sidebar for text editing */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif
#include "geanyplugin.h"



GeanyPlugin	*geany_plugin;
GeanyData	*geany_data;

static GtkWidget	*sidebar;
static GtkWidget	*clip_vbox;
static GtkWidget	*clip_file;

static GtkListStore *store;

static GPtrArray	*clip_array;

typedef struct {
	gchar *name;
	gchar *value;
} Cliptext;

static gint group_num = 0;
static gint page_num = 0;



void clear_row(gpointer data) {
	Cliptext *clip_text = (Cliptext *)data;
	g_free(clip_text->name);
	g_free(clip_text->value);
}

void clear_array(gpointer data) {
	g_array_unref((GArray *)data);
}

static void cliptext_load(void)
{
	gsize i, j, len = 0, len_keys = 0;

	gchar *sys_file;
	gchar *user_file;
	GKeyFile *config = g_key_file_new();

	gchar **clip_groups, **clip_keys;
	
	/* Base array with all data */
	if (clip_array == NULL) {
		clip_array = g_ptr_array_new_with_free_func(clear_array);
	} else {
		g_ptr_array_set_size(clip_array, 0);
	}
	
	/* Load config file: system (/usr/share/geany-plugins/cliptext/cliptext.conf) or replace by user (/home/user/.config/geany/plugins/cliptext/cliptext.conf) */
	sys_file = g_build_filename(geany_data->app->datadir, "cliptext.conf", NULL);

	user_file = g_strconcat(geany_data->app->configdir, G_DIR_SEPARATOR_S,
		"plugins", G_DIR_SEPARATOR_S,
		"cliptext", G_DIR_SEPARATOR_S, "cliptext.conf", NULL);

	if (g_file_test(user_file, G_FILE_TEST_IS_REGULAR)) {
		g_key_file_load_from_file(config, user_file, G_KEY_FILE_NONE, NULL);
	} else if (g_file_test(sys_file, G_FILE_TEST_IS_REGULAR)) {
		g_key_file_load_from_file(config, sys_file, G_KEY_FILE_NONE, NULL);
	} else {
		g_key_file_load_from_data(config, "[NO CONFIG FOUND]\ngeany=Geany, Geany, Geany\\n\naaja=Aaja, aaja, aaja\\n\n", -1, G_KEY_FILE_NONE, NULL);
	}

	clip_groups = g_key_file_get_groups(config, &len);
	
	for (i = 0; i < len; i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(clip_file), clip_groups[i]);
		
		GArray *clip_row = g_array_new(FALSE, TRUE, sizeof(Cliptext));
		g_array_set_clear_func(clip_row, clear_row);
		
		clip_keys = g_key_file_get_keys(config, clip_groups[i], &len_keys, NULL);
		
		for (j = 0; j < len_keys; j++) {
			Cliptext clip_text = {0};
			
			clip_text.name = g_strdup(clip_keys[j]);
			clip_text.value = utils_get_setting_string(config, clip_groups[i], clip_keys[j], "");
			
			g_array_append_val(clip_row, clip_text);
		}
		
		g_ptr_array_add(clip_array, clip_row);
		
		g_strfreev(clip_keys);
	}
	g_strfreev(clip_groups);

	g_key_file_free(config);
	g_free(user_file);
	g_free(sys_file);
}

/* Dropdown menu select */
static void on_group_changed(GtkComboBoxText *clip_file, gpointer user_data)
{
	group_num = gtk_combo_box_get_active(GTK_COMBO_BOX(clip_file));
	
	gtk_list_store_clear(store);
	
	GArray *clip_row = g_ptr_array_index(clip_array, group_num);
	
	for (guint i = 0; i < clip_row->len; i++) {
		Cliptext *clip_text = &g_array_index(clip_row, Cliptext, i);
		
		GtkTreeIter tree_iter;
		gtk_list_store_append(store, &tree_iter);
		gtk_list_store_set(store, &tree_iter, 0, clip_text->name, -1);
	}
}

static void clip_replace(gchar *clip_value)
{
	GeanyDocument *doc = NULL;
	doc = document_get_current();

	g_return_if_fail(doc != NULL);
	
	gchar *selection = sci_get_selection_contents(doc->editor->sci);
	
	/* gint pos = sci_get_selection_start(doc->editor->sci); */
	gint pos = sci_get_current_position(doc->editor->sci);
	gint cursor_pos = -1;
	
	GString *clip_gstring = g_string_new(clip_value);
	
	/* gint cursor_pos = utils_string_find(clip_gstring, 0, -1, "%sel%"); */
	gchar *sub = strstr(clip_gstring->str, "%sel%");
	if (sub != NULL) {
		cursor_pos = (gint)(sub - clip_gstring->str);
	}
	
	utils_string_replace_first(clip_gstring, "%sel%", selection);
	
	sci_replace_sel(doc->editor->sci, clip_gstring->str);
	
	gtk_widget_grab_focus(GTK_WIDGET(doc->editor->sci));
	
	if (cursor_pos != -1) {
		pos = pos+cursor_pos;
		sci_set_current_position(doc->editor->sci, pos, FALSE);
	}
	
	g_free(selection);
	g_string_free(clip_gstring, TRUE);
}

static void on_clip_clicked(GtkTreeView *tree_view,
							GtkTreePath *path,
							GtkTreeViewColumn *column,
							gpointer user_data) 
{

	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter(model, &iter, path)) {

		/* gtk_tree_model_get(model, &iter, 0, &name, -1); */
		gint row_num = gtk_tree_path_get_indices(path)[0];

		GArray *clip_row = g_ptr_array_index(clip_array, group_num);
		Cliptext *clip_text = &g_array_index(clip_row, Cliptext, row_num);

		clip_replace(clip_text->value);
	}
}

/*
static void on_reload(GtkButton *button, gpointer user_data)
{
	GtkComboBoxText *clip_file = GTK_COMBO_BOX_TEXT(user_data);

	gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(clip_file));

	cliptext_load();

	gtk_combo_box_set_active(GTK_COMBO_BOX(clip_file), group_num);
}
*/

static gboolean cliptext_init(GeanyPlugin *plugin, gpointer pdata)
{
	geany_plugin = plugin;
	geany_data = plugin->geany_data;

	sidebar = geany_data->main_widgets->sidebar_notebook;

	clip_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	clip_file = gtk_combo_box_text_new();

	cliptext_load();

	GtkWidget	*clip_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(clip_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	store = gtk_list_store_new(1, G_TYPE_STRING);
	GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Invis title", renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

	gtk_container_add(GTK_CONTAINER(clip_window), tree_view);
	
	gtk_box_pack_start(GTK_BOX(clip_vbox), clip_file, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(clip_vbox), clip_window, TRUE, TRUE, 0);
	
	
	/*
	GtkWidget *button = gtk_button_new_with_label("Reload");
	gtk_box_pack_start(GTK_BOX(clip_vbox), button, FALSE, FALSE, 0);
	g_signal_connect(button, "clicked", G_CALLBACK(on_reload), clip_file);
	*/
	
	g_signal_connect(clip_file, "changed", G_CALLBACK(on_group_changed), NULL);
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(on_clip_clicked), NULL);
	

	page_num = gtk_notebook_append_page(GTK_NOTEBOOK(sidebar), clip_vbox, gtk_label_new("Cliptext"));
	gtk_widget_show_all(clip_vbox);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(sidebar), page_num);
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(clip_file), 0);
	
	return TRUE;
}

static void cliptext_cleanup(GeanyPlugin *plugin, gpointer pdata)
{
	g_object_unref(store);
	g_ptr_array_unref(clip_array);
	
	/*
	if (page_num >= 0) {
		gtk_notebook_remove_page(GTK_NOTEBOOK(sidebar), page_num);
	}
	*/

	if (clip_vbox) {
		gtk_widget_destroy(clip_vbox);
		clip_vbox = NULL;
	}
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
	/* Step 1: Set metadata */
	plugin->info->name = "Cliptext";
	plugin->info->description = "Adds clickable snippets to the sidebar for text editing";
	plugin->info->version = "0.1";
	plugin->info->author = "Juan Duev <juan.duev@sezample.org>";

	/* Step 2: Set functions */
	plugin->funcs->init = cliptext_init;
	plugin->funcs->cleanup = cliptext_cleanup;

	/* Step 3: Register! */
	GEANY_PLUGIN_REGISTER(plugin, 225);
	/* alternatively:
	GEANY_PLUGIN_REGISTER_FULL(plugin, 225, data, free_func); */
}
