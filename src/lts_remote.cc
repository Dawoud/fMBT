/*
 * fMBT, free Model Based Testing tool
 * Copyright (c) 2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include "lts_remote.hh"
#include "factory.hh"
#include <glib.h>

bool Lts_remote::load(std::string& name)
{
  std::string model("remote.lts#");
  gchar* stdout=NULL;
  gchar* stderr=NULL;
  gint   exit_status=0;
  GError *ger=NULL;
  bool ret;
  g_spawn_command_line_sync(name.c_str()+11,&stdout,&stderr,
			    &exit_status,&ger);
  if (!stdout) {
    errormsg = std::string("Lts_remote cannot execute \"") + (name.c_str()+11) + "\"";
    status = false;
    ret = false;
  } else {
    model+=stdout;
    if (exit_status) {
      ret=false;
    } else {
      ret=Lts::load(model);
    }
    g_free(stdout);
    g_free(stderr);
    g_free(ger);
  }
  return ret;
}

FACTORY_DEFAULT_CREATOR(Model, Lts_remote, "lts_remote")
