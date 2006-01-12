/*
	This file is part of the GVars3 Library.

	Copyright (C) 2005 The Authors

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "gvars3/gvars3.h"
#include <vector>
#include <algorithm>

using namespace std;
using namespace TooN;

namespace GVars3
{

	 std::map<std::string, std::string>		GV3::unmatched_tags;
	 std::map<std::string, BaseMap*>		GV3::typeof_tags;
	 std::list<BaseMap*>					GV3::maps;


	void GV3::add_typemap(BaseMap* m)
	{
		maps.push_back(m);
	}


	string GV3::get_var(string name)
	{
		if(typeof_tags.count(name))
			return typeof_tags[name]->get_as_string(name);
		else if(unmatched_tags.count(name))
			return unmatched_tags[name];
		else
			return "(Not present in GVar list.)";
	}

	bool GV3::set_var(string name, string val, bool silent)
	{
		if(typeof_tags.count(name))
		{
			int e = typeof_tags[name]->set_from_string(name, val);
			if(!silent)
				parse_warning(e, typeof_tags[name]->name(), name, val);
			return e==0;
		}
		else
		{
			unmatched_tags[name]=val;
			return true;
		}
	}

	void GV3::print_var_list(ostream& o)
	{
		o << "//Registered GVars:" << endl;

		for(map<string, BaseMap*>::iterator i=typeof_tags.begin(); i != typeof_tags.end(); i++)
			cout << i->first << "=" << get_var(i->first) << endl;

		o << "//Unmatched tags:" << endl;

		for(map<string,string>::iterator i=unmatched_tags.begin(); i != unmatched_tags.end(); i++)
			o << i->first << "=" << i->second << endl;
		o << "// End of GVar list." << endl;

	}

	vector<string> GV3::tag_list()
	{
		vector<string> v;
		for(map<string, BaseMap*>::iterator i=typeof_tags.begin(); i != typeof_tags.end(); i++)
			v.push_back(i->first);

		return v;
	}

	void parse_warning(int e, string type, string name, string from)
	{
	if(e > 0)
		std::cerr << "GV3:Parse error setting " << type << " " << name << " from " << from << std::endl;
	else if (e < 0)
		std::cerr << "GV3:Parse warning setting " << type << " " << name << " from " << from << ": "
				  << "junk is -->" << from.c_str()-e  << "<--" << std::endl;
	}
};
