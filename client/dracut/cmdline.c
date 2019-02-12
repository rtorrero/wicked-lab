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
#include <wicked/ipv4.h>
#include <wicked/ipv6.h>
#include <wicked/xml.h>

#include "client/dracut/cmdline.h"
#include "client/wicked-client.h"
#include "client/ifconfig.h"
#include "util_priv.h"


/**
 * FIXME: This static function is already defined in sysconfig.c,
 * move it to util.c and use it from there
 */

static ni_bool_t unquote(char *);
static char * __ni_suse_default_hostname;

/**
 * This function adds an interface to the ni_compat_netdev_array
 * structure. It should also check only for supported configuration values
 * Maybe should be renamed to ni_cmdlineconfig_parse_value or something?
 */
ni_bool_t
ni_cmdlineconfig_add_interface(ni_compat_netdev_array_t *nd, const char *name, const char *value, const char *filename)
{
	char ifname[16];
	char varname_buf[19];
	char filtered_value[512];
	const char delim[2] = ":";
	char *token;
	ni_ipv4_devinfo_t *ipv4;
	ni_ipv6_devinfo_t *ipv6;
	ni_ifworker_control_t *control;
	size_t len;
	ni_compat_netdev_t *compat = NULL;

	int value_pos = 0;

	if (value) {
		strcpy(filtered_value, value);
		unquote(filtered_value);
	}

	//FIXME: This should be parsed using strtok
	if (!strcmp(name, "ip")) {
		if (value && strchr(filtered_value, ':')) {
			len = strchr(filtered_value, ':') - filtered_value;
			strncpy(ifname, filtered_value, len);
			ifname[len] = 0;	// FIXME: not very safe if len is > than array size
			snprintf(varname_buf, sizeof varname_buf, "%s.%s", name, ifname);

			if (!ni_netdev_name_is_valid(ifname)) {
				ni_error("Rejecting suspect interface name: %s", ifname);
				return FALSE;
			}
			compat = ni_compat_netdev_new(ifname);

			ni_compat_netdev_array_append(nd, compat);
			token = strtok(filtered_value, delim);

			control = ni_ifworker_control_new();
			control->link_timeout = 0;
			compat->control = control;
			compat->firewall.enabled = TRUE;

			ni_compat_netdev_set_origin(compat, "dracut:cmdline", filename);

			while (token != NULL) {
				token = strtok(NULL, delim);
				value_pos++;
				if (token == NULL)
					break;
				if (!strcmp(token, "dhcp6")) {
					compat->dhcp6.enabled = TRUE;
					ipv6 = ni_netdev_get_ipv6(compat->dev);
					ipv4 = ni_netdev_get_ipv4(compat->dev);
					ni_tristate_set(&ipv6->conf.enabled, TRUE);
					ni_tristate_set(&ipv4->conf.enabled, TRUE);
					ni_tristate_set(&ipv4->conf.arp_verify, TRUE);
					ni_compat_read_default_hostname("", &__ni_suse_default_hostname);
					ni_string_dup(&compat->dhcp6.hostname, __ni_suse_default_hostname);
				} else if (!strcmp(token, "dhcp")) {
					compat->dhcp4.enabled = TRUE;
					ipv6 = ni_netdev_get_ipv6(compat->dev);
					ipv4 = ni_netdev_get_ipv4(compat->dev);
					ni_tristate_set(&ipv6->conf.enabled, TRUE);
					ni_tristate_set(&ipv4->conf.enabled, TRUE);
					ni_tristate_set(&ipv4->conf.arp_verify, TRUE);
					ni_addrconf_update_set(&compat->dhcp4.update, NI_ADDRCONF_UPDATE_HOSTNAME, TRUE);
					ni_addrconf_update_set(&compat->dhcp4.update, NI_ADDRCONF_UPDATE_SMB, TRUE);
					compat->dhcp4.defer_timeout = 15;	//FIXME: read default as compat-suse.c does
					ni_compat_read_default_hostname("", &__ni_suse_default_hostname);
					ni_string_dup(&compat->dhcp4.hostname, __ni_suse_default_hostname);
				} else if (!strcmp(token, "auto6")) {
					compat->auto6.enabled = TRUE;
					ipv6 = ni_netdev_get_ipv6(compat->dev);
					ipv4 = ni_netdev_get_ipv4(compat->dev);
					ni_tristate_set(&ipv6->conf.enabled, TRUE);
					ni_tristate_set(&ipv4->conf.enabled, TRUE);
					ni_tristate_set(&ipv4->conf.arp_verify, TRUE);
					ni_addrconf_update_set(&compat->auto6.update, NI_ADDRCONF_UPDATE_DNS, TRUE);
					compat->auto6.defer_timeout = 0;

				}
			}
			return TRUE;
		}
		// else ignore ip={dhcp|on|any|dhcp6|auto6} for now
	}

	return FALSE;
}

/*
 * Read a dracut file iterating through lines. When a variable in the form of
 * name=value or name, we call ni_cmdlineconfig_add_interface to process the
 * interface configuration details
 */
ni_bool_t
ni_cmdlineconfig_read(const char *filename, ni_compat_netdev_array_t *nd, const char **varnames)
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
				ni_cmdlineconfig_add_interface(nd, name, value, filename);
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
			ni_cmdlineconfig_add_interface(nd, name, value, filename);
		}
	}

	fclose(fp);
	return TRUE;
}

/**
 * This function takes the path to a file and reads all the configuration variables
 * in it using ni_cmdlineconfig_read
 */
static ni_bool_t
ni_ifconfig_read_dracut_cmdline_file(xml_document_array_t *docs, const char *type,
			const char *root, const char *pathname, ni_bool_t check_prio,
			ni_bool_t raw, ni_compat_ifconfig_t *conf)
{
	char pathbuf[PATH_MAX] = {'\0'};

	if (!ni_string_empty(root)) {
		snprintf(pathbuf, sizeof(pathbuf), "%s/%s", root, pathname);
		pathname = pathbuf;
	}

	return ni_cmdlineconfig_read(pathbuf, &conf->netdevs, NULL);
}

/**
 * This function takes a directory in pathname and scans it for .conf files.
 * Then it uses ni_ifconfig_read_dracut_cmdline_file to read all .conf files
 */
static ni_bool_t
ni_ifconfig_read_dracut_cmdline_dir(xml_document_array_t *docs, const char *type,
			const char *root, const char *pathname, ni_bool_t check_prio,
			ni_bool_t raw, ni_compat_ifconfig_t *conf)
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
			if (ni_ifconfig_read_dracut_cmdline_file(docs, type, pathname, files.data[i], check_prio, raw, conf))
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
	ni_compat_ifconfig_t conf;
	ni_compat_ifconfig_init(&conf, type);

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
			rv = ni_ifconfig_read_dracut_cmdline_file(array, type, ni_dirname(pathbuf), ni_basename(pathbuf), check_prio, raw, &conf);
		else if (ni_isdir(pathbuf))
			rv = ni_ifconfig_read_dracut_cmdline_dir(array, type, root, pathbuf, check_prio, raw, &conf);
	}

	if (rv) {
		ni_compat_generate_interfaces(array, &conf, FALSE, FALSE);
		return rv;
	}
}

/**
 * FIXME: This static function is already defined in sysconfig.c,
 * move it to util.c and use it from there
 */

/* Extract values from a setting. string starts after '='
 * Good: sym=val ; sym="val" ; sym='val'
 * Bad:  sym="val ; sym='val
 *
 * returns true if val is valid.
 */
static ni_bool_t
unquote(char *string)
{
	char quote_sign = 0;
	char *src, *dst, cc, lc = 0;
	ni_bool_t ret = TRUE;

	if (!string)
		return ret;

	ret = TRUE;
	src = dst = string;
	if (*string == '"' || *string == '\'') {
		quote_sign = *string;
		src++;
	}
	do {
		cc = *src;
		if (!cc) {
			ret = quote_sign && lc == quote_sign;
			break;
		}
		if (isspace(cc) && !quote_sign)
			break;
		if (cc == quote_sign)
			break;
		*dst = lc = cc;
		dst++;
		src++;
	} while (1);

	*dst = '\0';
	return ret;
}
