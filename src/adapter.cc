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
#include "adapter.hh"
#include <cstdio>
#include "log.hh"

FACTORY_IMPLEMENTATION(Adapter)

Adapter::Adapter(Log& l, std::string params) :
log(l), actions(NULL), parent(NULL)
{
}

void Adapter::set_actions(std::vector<std::string>* _actions)
{
  actions = _actions;
  unames.resize(actions->size()+1);
}

bool Adapter::init()
{
  return true;
}

Adapter* Adapter::up()
{
  return parent;
}

Adapter* Adapter::down(unsigned int a)
{
  printf("adapter_base %i\n",a);
  return NULL;
}

std::vector<std::string>& Adapter::getAdapterNames()
{
  return adapter_names;
}

std::vector<std::string>& Adapter::getAllActions()
{ 
  return *actions;
}

std::string& Adapter::getActionName(int action) {
  return (*actions)[action];
}

int Adapter::getActionNumber(std::string& name) 
{
  for(size_t i=0;i<actions->size();i++) {
    if ((*actions)[i]==name) {
      return i;
    }
  }
  return -1;
}

int Adapter::getActionNumber(const char *name)
{
  std::string _name(name);
  return getActionNumber(_name);
}


void Adapter::setparent(Adapter* a)
{
  parent = a;
}

const char* Adapter::getUActionName(int action)
{
  if (!unames[action]) {
    unames[action]=escape_string((*actions)[action].c_str());
  }
  return unames[action];
}

