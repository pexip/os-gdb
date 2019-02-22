/* MI Console code.

   Copyright (C) 2000-2018 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions (a Red Hat company).

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* An MI console is a kind of ui_file stream that sends output to
   stdout, but encapsulated and prefixed with a distinctive string;
   for instance, error output is normally identified by a leading
   "&".  */

#include "defs.h"
#include "mi-console.h"

/* Create a console that wraps the given output stream RAW with the
   string PREFIX and quoting it with QUOTE.  */

mi_console_file::mi_console_file (ui_file *raw, const char *prefix, char quote)
  : m_raw (raw),
    m_prefix (prefix),
    m_quote (quote)
{}

void
mi_console_file::write (const char *buf, long length_buf)
{
  size_t prev_size = m_buffer.size ();
  /* Append the text to our internal buffer.  */
  m_buffer.write (buf, length_buf);
  /* Flush when an embedded newline is present anywhere in the
     buffer.  */
  if (strchr (m_buffer.c_str () + prev_size, '\n') != NULL)
    this->flush ();
}

/* Write C to STREAM's in an async-safe way.  */

static int
do_fputc_async_safe (int c, ui_file *stream)
{
  char ch = c;
  stream->write_async_safe (&ch, 1);
  return c;
}

void
mi_console_file::write_async_safe (const char *buf, long length_buf)
{
  m_raw->write_async_safe (m_prefix, strlen (m_prefix));
  if (m_quote)
    {
      m_raw->write_async_safe (&m_quote, 1);
      fputstrn_unfiltered (buf, length_buf, m_quote, do_fputc_async_safe,
			   m_raw);
      m_raw->write_async_safe (&m_quote, 1);
    }
  else
    fputstrn_unfiltered (buf, length_buf, 0, do_fputc_async_safe, m_raw);

  char nl = '\n';
  m_raw->write_async_safe (&nl, 1);
}

void
mi_console_file::flush ()
{
  const std::string &str = m_buffer.string ();

  /* Transform a byte sequence into a console output packet.  */
  if (!str.empty ())
    {
      size_t length_buf = str.size ();
      const char *buf = str.data ();

      fputs_unfiltered (m_prefix, m_raw);
      if (m_quote)
	{
	  fputc_unfiltered (m_quote, m_raw);
	  fputstrn_unfiltered (buf, length_buf, m_quote, fputc_unfiltered,
			       m_raw);
	  fputc_unfiltered (m_quote, m_raw);
	  fputc_unfiltered ('\n', m_raw);
	}
      else
	{
	  fputstrn_unfiltered (buf, length_buf, 0, fputc_unfiltered, m_raw);
	  fputc_unfiltered ('\n', m_raw);
	}
      gdb_flush (m_raw);
    }

  m_buffer.clear ();
}

/* Change the underlying stream of the console directly; this is
   useful as a minimum-impact way to reflect external changes like
   logging enable/disable.  */

void
mi_console_file::set_raw (ui_file *raw)
{
  m_raw = raw;
}
