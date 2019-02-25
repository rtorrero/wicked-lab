/*
 *	wicked client configuration reading along their assosciated sub-types.
 *
 *	Copyright (C) 2010-2018 SUSE LINUX GmbH, Nuernberg, Germany.
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
 *		Olaf Kirch <okir@suse.de>
 *		Marius Tomaschewski <mt@suse.de>
 *		Pawel Wieczorkiewicz <pwieczorkiewicz@suse.de>
 *		Rubén Torrero Marijnissen <rtorreromarijnissen@suse.com>
 */
#ifndef WICKED_CLIENT_DRACUT_CMDLINE_H
#define WICKED_CLIENT_DRACUT_CMDLINE_H

typedef enum ni_cmdlineconfig_ip_syntax {
	NI_CMDLINE_SYNTAX_INVALID,
	NI_CMDLINE_SYNTAX_SIMPLE,
	NI_CMDLINE_SYNTAX_SIMPLE_IFNAME,
	NI_CMDLINE_SYNTAX_EXPLICIT_MACADDR,
	NI_CMDLINE_SYNTAX_EXPLICIT_DNS
} ni_cmdlineconfig_ip_syntax_t;

extern ni_bool_t	ni_ifconfig_read_dracut_cmdline(xml_document_array_t *,
						const char *,
						const char *,
						const char *,
						ni_bool_t,
						ni_bool_t);

#endif /* WICKED_CLIENT_DRACUT_CMDLINE_H */