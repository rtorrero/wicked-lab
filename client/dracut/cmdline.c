/*
 *	wicked client configuration reading for dracut cmdline schema.
 *
 *	Copyright (C) 2018 SUSE LINUX GmbH, Nuernberg, Germany.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *	Authors:
 *		Rubén Torrero Marijnissen <rtorreromarijnissen@suse.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <wicked/util.h>
#include <wicked/logging.h>
#include <wicked/netinfo.h>
#include <wicked/xml.h>

#include "client/dracut/cmdline.h"
#include "client/wicked-client.h"
#include "client/ifconfig.h"
#include "util_priv.h"


void
ni_cmdlineconfig_set(ni_cmdlineconfig_t *sc, const char *name, const char *value)
{
	return;
	//TODO
}

ni_cmdlineconfig_t *
ni_cmdlineconfig_new(const char *pathname)
{
	return NULL;
	//TODO
}

/*
 * Read a dracut file, and return an object containing all variables
 * found. Optionally, @varnames will restrict the list of variables we return.
 */
ni_bool_t
ni_cmdlineconfig_read(const char *filename, ni_cmdlineconfig_t *sc, const char **varnames)
{
	char linebuf[512];
	ni_bool_t is_open_quote = FALSE;
	FILE *fp;

	ni_debug_readwrite("ni_cmdlineconfig_read(%s)", filename);
	if (!(fp = fopen(filename, "r"))) {
		ni_error("unable to open %s: %m", filename);
		return FALSE;
	}

	while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		char *name, *value;
		char *sp = linebuf;

		while (isspace(*sp))
			++sp;

		//Ignore lines starting with '#' (comments)
		if (*sp == '#')
			continue;

		//FIXME: refactor this loop into something more readable
		while (*sp != '\n' && *sp != '\0') {

			//Get the name of the variable
			while (isspace(*sp))
				++sp;
			name = sp;

			//Keep reading next char until we find the ending
			while (isalnum(*sp) || *sp == '_' || *sp == '-' || *sp == '.' || *sp == '\\')
				++sp;

			//If a character that is not part of a valid name is not one of these, just move on
			if (*sp != '=' && *sp != ' ' && *sp != '\n' && *sp != '"')
				continue;

			if (*sp == ' ' || *sp == '\n') {
				// This is the case where we have just a variable with no parameters
				value = NULL;
				*sp++ = '\0';
				ni_cmdlineconfig_set(sc, name, value);
				continue;
			} else {
				*sp++ = '\0';
				value = sp;
			}


			/* If we were given a list of variable names to match
			* against, ignore all variables not in this list. */
			if (varnames) {
				const char **vp = varnames, *match;

				while ((match = *vp++) != NULL) {
					if (!strcmp(match, name))
						break;
				}
				if (!match)
					*sp++ = '\0';
			}

			// FIXME: Remove the || for a function that checks this
			while (isalnum(*sp) || *sp == ':' || *sp == '.' || *sp == '/' || *sp == '[' || *sp == ']' || *sp == '-' || *sp == '_' || *sp == '=' || *sp == '"' || (*sp == ' ' && is_open_quote)) {
				if (*sp == '"' && !is_open_quote)
					is_open_quote = TRUE;
				++sp;
			}

			if (*sp == ' ' && is_open_quote) {
				continue;
			}

			if (*sp != ' ' && *sp != '\n' && *sp != '\0')
				continue;

			if (*sp != '\0')	//FIXME why ?
				*sp++ = '\0';

			is_open_quote == FALSE;
			ni_cmdlineconfig_set(sc, name, value);
		}
	}

	fclose(fp);
	return TRUE;
}

static xml_node_t *
ni_ifconfig_generate_dhcp4_addrconf(xml_node_t *ifnode, ni_cmdlineconfig_t *clf)
{
	xml_node_t *dhcp;

	dhcp = xml_node_new("ipv4:dhcp", ifnode);
	xml_node_new_element("enabled", dhcp, "true");

	return dhcp;

}

static ni_bool_t
ni_ifconfig_read_dracut_interface(xml_document_array_t *array, ni_cmdlineconfig_t *clf)
{
	unsigned int i;
	xml_document_t *config_doc;
	xml_node_t *root, *ifnode, *namenode;
	for (i=0; i < clf->vars.count; i++) {
		if (!strncmp(clf->vars.data[i].name, "ip.", 3 )) {
			config_doc = xml_document_new();
			root = xml_document_root(config_doc);
			ifnode = xml_node_new("interface", xml_document_root(config_doc));

			//FIXME: Hardcoded stuff just as POC
			namenode = xml_node_new_element("name", ifnode, strchr(clf->vars.data[i].name, '.') + 1);
			//xml_node_add_attr(namenode, "namespace", "ethernet");

			//FIXME: just a POC, use a switch.case here for all possible addrconf modes
			if (!strncmp(clf->vars.data[i].value, "dhcp", 4)) {
				ni_ifconfig_generate_dhcp4_addrconf(ifnode, clf);
			}

			xml_document_array_append(array, config_doc);
		}
	}
}

static ni_bool_t
ni_ifconfig_read_dracut_cmdline_file(xml_document_array_t *docs, const char *type,
			const char *root, const char *pathname, ni_bool_t check_prio,
			ni_bool_t raw, ni_cmdlineconfig_t *sc)
{
	//FIXME: Remove this
	unsigned int i;
	char pathbuf[PATH_MAX] = {'\0'};

	if (!ni_string_empty(root)) {
		snprintf(pathbuf, sizeof(pathbuf), "%s/%s", root, pathname);
		pathname = pathbuf;
	}



	return ni_cmdlineconfig_read(pathbuf, sc, NULL);
}

static ni_bool_t
ni_ifconfig_read_dracut_cmdline_dir(xml_document_array_t *docs, const char *type,
			const char *root, const char *pathname, ni_bool_t check_prio,
			ni_bool_t raw, ni_cmdlineconfig_t *sc)
{
	char pathbuf[PATH_MAX] = {'\0'};
	ni_string_array_t files = NI_STRING_ARRAY_INIT;
	unsigned int i;
	ni_bool_t empty = TRUE;

	ni_assert(docs);

	if (!ni_string_empty(root)) {
		snprintf(pathbuf, sizeof(pathbuf), "%s/%s", root, pathname);
		pathname = pathbuf;
	}

	if (ni_scandir(pathname, "*.conf", &files) != 0) {
		for (i = 0; i < files.count; ++i) {
			if (ni_ifconfig_read_dracut_cmdline_file(docs, type, pathname, files.data[i], check_prio, raw, sc))
				empty = FALSE;
		}
	}

	if (empty)
		ni_debug_ifconfig("No valid configuration files found at %s", pathname);

	ni_string_array_destroy(&files);
	return TRUE;
}

ni_bool_t
ni_ifconfig_read_dracut_cmdline(xml_document_array_t *array, const char *type,
			const char *root, const char *path, ni_bool_t check_prio, ni_bool_t raw)
{
	unsigned int i;
	char pathbuf[PATH_MAX];
	ni_bool_t rv = FALSE;
	ni_string_array_t cmdline_files = NI_STRING_ARRAY_INIT;
	ni_cmdlineconfig_t *sc;
	sc = ni_cmdlineconfig_new("/tmp/dracut");

	if (ni_string_empty(path)) {
		ni_string_array_append(&cmdline_files, "/etc/cmdline");
		ni_string_array_append(&cmdline_files, "/etc/cmdline.d/");
		ni_string_array_append(&cmdline_files, "/proc/cmdline");
		rv = TRUE; /* do not fail if default path does not exist */
	} else {
		ni_string_array_append(&cmdline_files, path);
	}
	for (i = 0; i < cmdline_files.count; ++i) {
		if (ni_string_empty(root)) {
			snprintf(pathbuf, sizeof(pathbuf), "%s", cmdline_files.data[i]);
		} else {
			snprintf(pathbuf, sizeof(pathbuf), "%s/%s", root, cmdline_files.data[i]);
		}

		if (ni_isreg(pathbuf))
			rv = ni_ifconfig_read_dracut_cmdline_file(array, type, ni_dirname(pathbuf), ni_basename(pathbuf), check_prio, raw, sc);
		else if (ni_isdir(pathbuf))
			rv = ni_ifconfig_read_dracut_cmdline_dir(array, type, root, pathbuf, check_prio, raw, sc);
	}

	if (rv)
		ni_ifconfig_read_dracut_interface(array, sc);
	return rv;
}