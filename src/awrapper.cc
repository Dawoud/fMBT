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

#include "awrapper.hh"
#include <algorithm>
#include "helper.hh"

Awrapper::Awrapper(Log&l, std::string params, aal* _ada):
  Adapter(l, params), ada(_ada) {
  if (ada==NULL) {
    status=false;
  }  
}

void Awrapper::set_actions(std::vector<std::string>* _actions)
{
  Adapter::set_actions(_actions);

  if (!status) {
    return;
  }

  std::vector<std::string>& wn=ada->getActionNames();

  std::vector<std::string> splitted_actions;

  splitted_actions.push_back(std::string(""));

  for(unsigned i=1;i<actions->size();i++) {
    std::string name,paramname;
    split(wn[i],name,paramname,"(");
    if (paramname!="") {
      paramname=paramname.substr(0,paramname.length()-1);
      parameters[i]=paramname;
    }
    splitted_actions.push_back(name);      
  }

  for(unsigned i=1;i<wn.size();i++) {
    unsigned result=find(*actions,wn[i]);

    if ((*actions)[result]!=wn[i]) {
      /* With parameters? */
      std::string adaname,adaparamname;
      split(wn[i],adaname,adaparamname,"(");
      adaparamname=adaparamname.substr(0,adaparamname.length()-1);
      result=find(splitted_actions,adaname);
      if (splitted_actions[result]==adaname) {
	parameters[result]=adaparamname;
	log.debug("action \"%s\" mapped to \"%s\" with parameter \"%s\"\n",
		  (*actions)[result].c_str(),wn[i].c_str(),adaparamname.c_str());
      }
    }

    ada2aal[i]=result;
    aal2ada[result]=i;
  }

}

void Awrapper::execute(std::vector<int>& action)
{
  /* We need to map.. */

  int tmp=ada->adapter_execute(aal2ada[action[0]],parameters[action[0]].c_str());
  action[0]=ada2aal[tmp];
  action.resize(1);
}

int  Awrapper::observe(std::vector<int> &action,
		       bool block) {
  int tmp=ada->observe(action,block);
  for(int i=0;i<tmp;i++) {
    action[i]=ada2aal[action[i]];
  }
  return tmp;
}
