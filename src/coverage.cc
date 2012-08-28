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
#include "coverage.hh"

FACTORY_ATEXIT(Coverage)
FACTORY_CREATORS(Coverage)
FACTORY_ADD_FACTORY(Coverage)

Coverage* CoverageFactory::create(Log& log, std::string name,
				  std::string params="")
{
  if (!creators) return NULL;

  creator c = (*creators)[name];

  if (c) {
    return c(log, params);
  } else {
    char* endp;
    long int val=strtol(name.c_str(),&endp,10);
    if (*endp==0) {
      c=(*creators)["const"];
      if (c) {
	return c(log,name);
      }
    }    
  }

  return NULL;
}


Coverage::Coverage(Log& l) :
  model(NULL), log(l)
{
}

Coverage::~Coverage()
{
}

void Coverage::set_model(Model* _model)
{
  model=_model;
}

std::string Coverage::stringify() {
    return std::string("");
}
