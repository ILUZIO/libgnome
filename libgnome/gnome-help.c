/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-help.c
 * Copyright (C) 2001 Sid Vicious
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>

#include <string.h>
#include <glib.h>

#include "gnome-i18nP.h"

#include "gnome-program.h"
#include "gnome-url.h"

#include "gnome-help.h"

static char *
locate_help_file (const char *path, const char *doc_name)
{
	int i;
	char *exts[] = { ".xml", ".sgml", ".html", NULL };
	const GList *lang_list = gnome_i18n_get_language_list ("LC_MESSAGES");

	for (;lang_list != NULL; lang_list = lang_list->next) {
		const char *lang = lang_list->data;

		/* This has to be a valid language AND a language with
		 * no encoding postfix.  The language will come up without
		 * encoding next */
		if (lang == NULL ||
		    strchr (lang, '.') != NULL)
			continue;

		for (i = 0; exts[i] != NULL; i++) {
			char * full = g_strconcat (path, "/", lang, "/",
						   doc_name, exts[i], NULL);
			if (g_file_test (full, G_FILE_TEST_EXISTS)) {
				return full;
			}
			g_free (full);
		}
	}

	return NULL;
}

static gboolean
display_help_file (GnomeProgram *program,
		   const char *file,
		   const char *link_id,
		   GError **error)
{
	char *url;
	gboolean ret;

	if (link_id != NULL)
		url = g_strdup_printf ("ghelp://%s?%s", file, link_id);
	else
		url = g_strdup_printf ("ghelp://%s", file);

	ret = gnome_help_display_uri (url, error);

	g_free (url);

	return ret;
}

gboolean
gnome_help_display (GnomeProgram  *program,
		    const char    *doc_name,
		    const char    *link_id,
		    GError       **error)
{
	char *path;
	char *file;
	gboolean ret;

	if (program == NULL)
		program = gnome_program_get ();

	path = gnome_program_locate_file (program,
					  GNOME_FILE_DOMAIN_APP_HELP,
					  "",
					  FALSE /* only_if_exists */,
					  NULL /* ret_locations */);

	if (path == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_INTERNAL,
			     _("Internal error"));
		return FALSE;
	}

	file = locate_help_file (path, doc_name);

	g_free (path);

	if (file == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_NOT_FOUND,
			     _("Help document %s for %s not found"),
			     doc_name, 
			     gnome_program_get_app_id (program));
		return FALSE;
	}

	ret = display_help_file (program, file, link_id, error);

	g_free (file);

	return ret;
}

gboolean
gnome_help_display_desktop (GnomeProgram  *program,
			    const char    *doc_name,
			    const char    *link_id,
			    GError       **error)
{
	GSList *ret_locations, *li;
	char *file;
	gboolean ret;

	if (program == NULL)
		program = gnome_program_get ();

	ret_locations = NULL;
	gnome_program_locate_file (program,
				   GNOME_FILE_DOMAIN_HELP,
				   "",
				   FALSE /* only_if_exists */,
				   &ret_locations);

	if (ret_locations == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_INTERNAL,
			     _("Internal error"));
		return FALSE;
	}

	file = NULL;
	for (li = ret_locations; li != NULL; li = li->next) {
		char *path = li->data;

		file = locate_help_file (path, doc_name);
		if (file != NULL)
			break;
	}

	g_slist_foreach (ret_locations, (GFunc)g_free, NULL);
	g_slist_free (ret_locations);

	if (file == NULL) {
		g_set_error (error,
			     GNOME_HELP_ERROR,
			     GNOME_HELP_ERROR_NOT_FOUND,
			     _("Help document %s for %s not found"),
			     doc_name, 
			     gnome_program_get_app_id (program));
		return FALSE;
	}

	ret = display_help_file (program, file, link_id, error);

	g_free (file);

	return ret;
}

gboolean
gnome_help_display_uri (const char    *help_uri,
			GError       **error)
{
	GError *err;
	gboolean ret;

	err = NULL;
	ret = gnome_url_show (help_uri, &err);
	g_propagate_error (error, err);
	return ret;
}

GQuark
gnome_help_error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0)
		error_quark =
			g_quark_from_static_string ("gnome-help-error-quark");

	return error_quark;
}