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
#include "adapter_mapper.hh"
#include "model.hh"
#include "dparse.h"
#include <cstring>
#include "helper.hh"
#include "conf.hh"

#include <sstream>
#include <iostream>
#ifndef DROI
#include <boost/regex.hpp>
#endif

extern "C" {
extern D_ParserTables parser_tables_mrules;
};

extern Rules* amobj;

#define debugprint(...)

Adapter_mapper::Adapter_mapper(Log& log, std::string _params)
  : Adapter(log), Rules(), params(_params)
{
  robin=0;
  
}

bool Adapter_mapper::init()
{
  load(params);
  for(unsigned i=0;i<adapters.size();i++) {
    if (adapters[i]) {
      if (!adapters[i]->init()) {
        return false;
      }
    }
  }
  return true;
}

std::string Adapter_mapper::stringify()
{
  std::ostringstream t(std::ios::out | std::ios::binary);

  /* Write adapters */

  for(unsigned i=1;i<adapters.size();i++) {
    if (adapters[i]) {
      t << i << "=\"" << removehash(adapter_names[i])
	<< capsulate(adapters[i]->stringify()) << std::endl;
    }
  }

  t << std::endl;
  /*
  for(std::map<int,adapter_action>::iterator it=m2.begin();
      it!=m2.end();it++) {
  */
  for(unsigned j=0;j<actions->size();j++) {
    std::map<int,adapter_action>::iterator it=m2.find(j);
    if (it!=m2.end()) {
      adapter_action a=it->second;
      int i=it->first;
      
      t << "\"" << (*actions)[i] << "\" -> ("
	<< a.first << ", \""
	<< adapter_anames[a.first][a.second]
	<< "\")" << std::endl;
    }
  }

  return t.str();
}

int Adapter_mapper::anum_create(int index,std::string& n) {
  
  for(unsigned int i=0;i<adapter_anames[index].size();i++) {
    if (adapter_anames[index][i]==n) {
      return i;
    }
  }

  if (adapter_anames[index].size()==0) {
    std::string tau("");
    adapter_anames[index].push_back(tau);
  }

  adapter_anames[index].push_back(n);
  return adapter_anames[index].size()-1;
}

void Adapter_mapper::add_map(std::vector<int>& index,std::vector<std::string>& n,int action) {

  
  
  int anum = anum_create(index[0],n[0]);

  log.debug("%s(%i,%s,%i) %i\n",__func__,index[0],n[0].c_str(),action,anum);

  adapter_action a(index[0],anum);
  if (!is_used(action) && !is_used(a)) {
    m1[a]=action;
    m2[action]=a;
    for(unsigned u=1;u<index.size();u++) {
      int bnum = anum_create(index[u],n[u]);
      adapter_action b(index[u],bnum);
      if (l_tau[u]) {
	_tm2.insert(std::pair<int,adapter_action>(bnum,b));
      } else {
	_fm2.insert(std::pair<int,adapter_action>(bnum,b));
      }
    }
  } else {
      log.debug("Error in '%s': ", load_name.c_str());
    if (is_used(action)) {
      log.debug("duplicate action on the model side: \"%s\"\n",
                (*actions)[action].c_str());
    } else { //  is_used(a)
      log.debug("duplicate action on the adapter side: \"%s\"\n",
        adapter_anames[a.first][a.second].c_str());
    }
    throw (int)42424;
  }
}


bool Adapter_mapper::load(std::string& name)
{
  D_Parser *p = new_D_Parser(&parser_tables_mrules, 16);
  char *s;

  log.debug("%s called\n",__func__);

  load_name = name;

  Rules* tmp=amobj;

  amobj=this;

  s=readfile(name.c_str());

  bool ret=dparse(p,s,std::strlen(s));

  if (ret) {
    log.debug("loading %s ok\n",name.c_str());
  } else {
    log.debug("loading %s failed\n",name.c_str());
    status=false;
  }

  free(s);

  free_D_Parser(p);

  amobj=tmp;

  /* Time to load adapters */

  for(unsigned int i=0;i<adapter_names.size();i++) {
    if (adapter_names[i]!=std::string("")) {

      log.debug("Loading adapter \"%s\"\n",
             adapter_names[i].c_str());

      std::string adapter_class;
      std::string adapter_params;
      
      Conf::split(adapter_names[i],adapter_class,adapter_params);

      log.debug("class %s, params %i\n",
             adapter_class.c_str(),
             adapter_params.c_str());

      Adapter* a = AdapterFactory::create(log, adapter_class,
                                          adapter_params);
      a->set_actions(&adapter_anames[i]);
      log.debug("Created adapter to %p\n",a);
      a->setparent(this);
      adapters[i] = a;
      if (!a->status) {
	status=false;
      }
    }
  }

  return true;
}

bool Adapter_mapper::is_used(int action)
{
  return (m2.find(action) != m2.end());
}
 
bool Adapter_mapper::is_used(adapter_action& action)
{
  return (m1.find(action)!=m1.end());
}


void Adapter_mapper::add_file(unsigned index, std::string& adaptername)
{
  log.debug("%s(%i,%s)\n",__func__,index,adaptername.c_str());
  if (adapter_names.capacity()<=index) {
    adapter_names.resize(index+2);
    adapter_anames.resize(index+2);
    adapters.resize(index+2);
  }
  adapter_names[index]=std::string(adaptername);
}

int Adapter_mapper::action_number(std::string& name) 
{
  for(size_t i=0;i<actions->size();i++) {
    if ((*actions)[i]==name) {
      return i;
    }
  }
  return -1;
}

/*
 * Adds result action (We'll assume that there are at lest one
 * component)
 *
 */
void Adapter_mapper::add_result_action(std::string* name)
{
  int action=action_number(*name);

  log.debug("%s(%s) called\n",__func__,name->c_str());

  if (action==-1) {
#ifndef DROI
    /* try regexp case */
    log.debug("Regexp case\n");
    const char* format_string = l_name[0].c_str();
    
    log.debug("Format string \"%s\"\n",format_string);

    boost::regex expression(*name);
    boost::cmatch what;

    for(unsigned int i=1;i<actions->size();i++) {
      log.debug("Action %s\n",(*actions)[i].c_str());
      if(boost::regex_match((*actions)[i].c_str(), what, expression)) {
        /* Match */        
        if (!is_used(i)) {
	  std::vector<std::string> s;
	  
	  if (l_index.size()!=l_name.size()) {
	    abort();
	  }

	  for(unsigned j=0;j<l_index.size();j++) {
	    s.push_back(replace(expression,
				l_name[j].c_str(),
				(*actions)[i].begin(),
				(*actions)[i].end()));
	  }
          add_map(l_index,s,i);
        } else {
          log.debug("action %s already used, won't add it again.\n");
        }
      }
    }
#endif
  } else {
    add_map(l_index,l_name,action);
  }
  l_index.resize(0);
  l_name.resize(0);
  l_tau.resize(0);
}

void Adapter_mapper::add_component(unsigned int index,std::string& name, bool tau)
{
  /* Validate index */
  log.debug("%s(%i,%s)\n",__func__,index,name.c_str());

  if (index>=adapter_names.size() || adapter_names[index]==std::string("")) {
    
    log.debug("adapter_names.size() %i,"
           "adapter_names[index] \"%s\"\n",
           adapter_names.size(),adapter_names[index].c_str());

    log.debug("%s\n%s\n",
           adapter_names[1].c_str(),
           adapter_names[2].c_str());

    throw((int)420);
  }
  
  l_name.push_back(name);
  l_index.push_back(index);
  l_tau.push_back(tau);
}

int Adapter_mapper::adapter_action_execute(adapter_action& a)
{
  log.print("<redirect id=\"%i\" name=\"%s\" action=\"%s\"/>\n",
            a.first,adapter_names[a.first].c_str(),adapter_anames[a.first][a.second].c_str());

  std::vector<int> f;
  f.push_back(a.second);

  adapters[a.first]->execute(f);
  return f[0];
}

int Adapter_mapper::map_execute(adapter_action& a)
{
  /* redirect to correct adapter */
  log.debug("execute with adapter %i (%s) action %i (%s)\n",
         a.first,adapter_names[a.first].c_str(),a.second,
         adapter_anames[a.first][a.second].c_str());

  log.print("<redirect id=\"%i\" name=\"%s\" action=\"%s\"/>\n",
            a.first,adapter_names[a.first].c_str(),adapter_anames[a.first][a.second].c_str());

  std::vector<int> f;
  f.push_back(a.second);

  adapters[a.first]->execute(f);
  m1_convert(a.first,f);
  return f[0];
}

bool Adapter_mapper::map_execute(int& action)
{
  adapter_action a=m2[action];
    
  if (a.first==0) { /* No such action map? */
    log.debug("execute without adapter: %i (action not mapped to any adapter)\n",action);
    return false;
  }

  action=map_execute(a);

  return true;
}

void Adapter_mapper::execute(std::vector<int>& action)
{
  log.push("adapter_mapper");
  /* Output Actions for each component */

  bool tau=false;

  if (map_execute(action[0])) {
    std::pair<std::multimap<int,adapter_action>::iterator,
      std::multimap<int,adapter_action>::iterator> r =
      _tm2.equal_range(action[0]);
    adapter_action a;
    
    for(std::multimap<int,adapter_action>::iterator i=r.first;
	i!=r.second;i++) 
      {
	a=i->second;
	log.debug("execute extras with check adapter %i (%s) action %i (%s)\n",
		  a.first,adapter_names[a.first].c_str(),a.second,
		  adapter_anames[a.first][a.second].c_str());

	// Handle true case
	if (a.second!=
	    adapter_action_execute(a)) {
	  /* ERROR. adapter didn't response as expected */
	  log.print("<configuration failure, returning tau>\n");
	  tau=true;
	}
      }

    r = _fm2.equal_range(action[0]);

    for(std::multimap<int,adapter_action>::iterator i=r.first;
	i!=r.second;i++) 
      {
	// Handle false case
	a=i->second;
	log.debug("execute extras without check adapter %i (%s) action %i (%s)\n",
		  a.first,adapter_names[a.first].c_str(),a.second,
		  adapter_anames[a.first][a.second].c_str());

	adapter_action_execute(a);	
      }

  }

  if (tau) {
    action.resize(1);
    action[0]=0;
  }

  log.pop();
}

void Adapter_mapper::m1_convert(int index,
				std::vector<int>&action)
{
  for(unsigned i=0;i<action.size();i++) {
    action[i] = m1[adapter_action(index,action[i])];
  }  
}

bool Adapter_mapper::observeRobin(std::vector<int> &action)
{
  if (adapters.size()==0) {
    abort();
  }

  if (robin==adapters.size()) {
    robin=0;
  }

  if (adapters[robin]==NULL) {
    robin++;
    return observeRobin(action);
  }
  bool ret = adapters[robin]->observe(action,false);
  if (ret)
    m1_convert(robin,action);
  robin++;
  return ret;
}

bool Adapter_mapper::observe(std::vector<int> &action,bool block)
{
  /* Ok. This is a bit hairy */
  do {
    if (observeRobin(action)) {
      return true;
    }
  } while (block);
  return false;
}

FACTORY_DEFAULT_CREATOR(Adapter, Adapter_mapper, "mapper");
