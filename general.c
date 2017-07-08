/*
Copyright 2017 jun7@hush.mail

This file is part of wyeb.

wyeb is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

wyeb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with wyeb.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#if DEBUG
# define D(f, ...) g_print(#f"\n", __VA_ARGS__);
# define DNN(f, ...) g_print(#f, __VA_ARGS__);
# define DD(a) g_print(#a"\n");
# define DENUM(v, e) if (v == e) D(%s:%3d is %s, #v, v, #e);
#else
# define D(...) ;
# define DNN(...) ;
# define DD(a) ;
# define DENUM(v, e) ;
#endif

#define SIG(o, n, c, u) \
	g_signal_connect(o, n, G_CALLBACK(c), u)
#define SIGA(o, n, c, u) \
	g_signal_connect_after(o, n, G_CALLBACK(c), u)
#define SIGW(o, n, c, u) \
	g_signal_connect_swapped(o, n, G_CALLBACK(c), u)

#define HINTKEYS "fsedagwrvxqcz"
//bt324"

#define SHARED 0

typedef enum {
	Con     = 'o',

	Ckey    = 'k',
	Cclick  = 'c',
	Clink   = 'l',
	Curi    = 'u',
	Ctext   = 't',

	Cfocus  = 's',
	Cblur   = 'b',
	Crm     = 'r',

	Cfree   = 'f',
} Coms;

static void _mkdirif(gchar *path, bool isfile)
{
	gchar *dir;
	if (isfile)
		dir = g_path_get_dirname(path);
	else
		dir = path;

	if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 0700);

	g_assert(g_file_test(dir, G_FILE_TEST_IS_DIR));

	if (isfile)
		g_free(dir);
}
static void mkdirif(gchar *path)
{
	_mkdirif(path, true);
}

//ipc
static gchar *ipcpath(gchar *name)
{
	gchar *path = g_build_filename(g_get_user_runtime_dir(), fullname, name, NULL);
	mkdirif(path);
	return path;
}

static void ipccb(const gchar *line);
static gboolean ipcgencb(GIOChannel *ch, GIOCondition c, gpointer p)
{
	gchar *line;
//	GError *err = NULL;
	g_io_channel_read_line(ch, &line, NULL, NULL, NULL);
//	if (err)
//	{
//		D("ioerr: ", err->message);
//		g_error_free(err);
//	}
	if (!line) return true;
	g_strchomp(line);

	//D(receive %s, line)
	ipccb(line);

	g_free(line);
	return true;
}

static bool ipcsend(gchar *name, gchar *str) {
	gchar *path = ipcpath(name);
	bool ret = false;
	int cpipe = 0;
	if (
		(g_file_test(path, G_FILE_TEST_EXISTS)) &&
		(cpipe = open(path, O_WRONLY | O_NONBLOCK)))
	{
		//D(send start %s %s, name, str)

		ret = true;
		gchar *send = g_strconcat(str, "\n", NULL);
		ret = write(cpipe, send, strlen(send)) != -1;
		g_free(send);
		close(cpipe);

		//D(send -end- %s %s, name, str)
	}
	g_free(path);
	return ret;
}
static void ipcwatch(gchar *name) {
	gchar *path = ipcpath(name);

	if (!g_file_test(path, G_FILE_TEST_EXISTS))
		mkfifo(path, 0600);

	g_io_add_watch(
			g_io_channel_new_file(path, "r+", NULL), G_IO_IN, ipcgencb, NULL);

	g_free(path);
}