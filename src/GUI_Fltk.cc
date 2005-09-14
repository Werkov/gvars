#include "gvars3/GUI_Fltk.h"
#include "gvars3/GStringUtil.h"
#include <vector>
#include <string.h>
#include <sstream>
#include <unistd.h>

#include <error.h>

#include <FL/Fl.h>
#include <FL/Fl_Window.h>
#include <FL/Fl_Widget.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Check_Button.h>
#include <Fl/Fl_Value_Slider.h>
#include <Fl/Fl_Box.h>


#define POLL_UPDATE 1

using namespace std;
namespace GVars3
{


GUI_Fltk::GUI_Fltk(GUI *pGUI, GVars2* pGV2)
{
	gui=pGUI;
	gv2=pGV2;
	init = 0;
	gui->RegisterCommand("GUI_Fltk.InitXInterface", InitXInterfaceCB, this);
}


void poll_callback(void* v)
{
	GUI_Fltk* t = (GUI_Fltk*) v;

	t->poll_windows();
	Fl::check();

	//Repeat the polling timeout
	Fl::repeat_timeout(0.02,  poll_callback, v);
}

void* GUI_Fltk::do_stuff_CB(void* v)
{
	GUI_Fltk* t = (GUI_Fltk*)v;

	//Add a one shot timeout which makes widgets poll for changes
	Fl::add_timeout(0.02,  poll_callback,  v);
	
	for(;;)
	{
		//Fl::lock();	
		Fl::run();
		Fl::check();
		//Fl::unlock();
		
		//If no windows are present, sleep and start again
		usleep(100000);
		
	}
}


#define UI (*(GUI_Fltk*)(ptr))

void GUI_Fltk::InitXInterfaceCB(void* ptr, string sCommand, string sParams)
{
	UI.InitXInterface(sParams);
}

void GUI_Fltk::InitXInterface(string args)
{
	if(init)
	{
		cerr << "??GUI_Fltk::InitXInterface: already initialised." << endl;
		return;
	}
		
	vector<string> vs = ChopAndUnquoteString(args);

	if(vs.size() > 0)
		name = vs[0];
	else
		name = "GUI_Fltk";

	gui->RegisterCommand(name + ".AddWindow", AddWindowCB, this);

	init = 1;
}

void GUI_Fltk::start_thread()
{
	pthread_create(&gui_thread, 0, do_stuff_CB, this);
}

void GUI_Fltk::process_in_crnt_thread()
{
	//Fl::lock();
	poll_windows();
	Fl::check();
	//Fl::unlock();
}


////////////////////////////////////////////////////////////////////////////////
//
// Helper functions
//

string GUI_Fltk::remove_suffix(string cmd, string suffix)
{
	cmd.resize(cmd.length() - suffix.length());
	return cmd;
}

bool GUI_Fltk::check_window(string win_name, string err)
{
	if(windows.find(win_name) != windows.end())
		return 1;

	cerr << "!!!! GUI_Fltk::" << err << "<<: window " << win_name << " does not exist!" << endl;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Window
//

void GUI_Fltk::AddWindowCB(void* ptr, string sCommand, string sParams)
{
	//Fl::lock();
	UI.AddWindow(sParams);
	//Fl::unlock();
}

class GUI_Fltk_win:public Fl_Window
{
	public:
		GUI_Fltk_win(int w, string name, string caption, GUI* pgui)
		:Fl_Window(w, 0),win_name(name),labl(caption),gui(pgui)
		{
			label(caption.c_str());
			callback(my_callback);
		}

		void add(Fl_Widget* widg)
		{
			//Position the widget
			widg->resize(GUI_Fltk::widget_padding_x, h() + GUI_Fltk::widget_padding_y, 
						 w() - 2 * GUI_Fltk::widget_padding_x , GUI_Fltk::widget_height);

			//Resize the window
			size(w(), h() + GUI_Fltk::widget_height + 2 * GUI_Fltk::widget_padding_y);

			Fl_Window::add(widg);
		}

		void poll_update()
		{
			//Go through all the widgets, updating if necessary
			for(int c=0; c < children(); c++)
				child(c)->do_callback(child(c),POLL_UPDATE);
		}

	private:
		string  win_name, labl;
		GUI* 	gui;
		
		static void my_callback(Fl_Widget* w)
		{
			//Called on close event
			GUI_Fltk_win* win = (GUI_Fltk_win*) w;
			win->gui->ParseLine(win->win_name+".Destroy");
		}
};

void GUI_Fltk::poll_windows()
{
	for(map<string, window>::iterator i=windows.begin(); i != windows.end(); i++)
	{
		if(i->second.showme)
		{
			i->second.win->show();
			i->second.showme = false;
		}
		i->second.win->poll_update();
	}
}



void GUI_Fltk::AddWindow(string sParams)
{
	if(!init)
	{
		cerr << "! GUI_Fltk::AddWindow: Not initialised. Call InitXInterface first." << endl;
		return;
	}

	vector<string> vs = ChopAndUnquoteString(sParams);
	if(vs.size()<1 || vs.size()>3)
	{
		cerr << "! GUI_Motif::AddWindow: need 1 - 3 params: Name, Caption=Name, Width=200 " << endl;
		return;
	}

	if(windows.count(vs[0])>0)
	{
		cerr << "? GUI_Motif::AddWindow: A window with id \"" << vs[0] << "\" already exists." << endl;
		return;
	}

	string sCaption;
	int width = 200;

	if(vs.size()>=2)
		sCaption=vs[1];
	else
		sCaption=vs[0];

	if(vs.size() > 2)
		width = atoi(vs[2].c_str());

	window w;
	w.win = new GUI_Fltk_win(width, vs[0], sCaption, gui);
	w.showme = true;
	w.win->end();
	//w.win->show();

	windows[vs[0]] = w;

	gui->RegisterCommand(vs[0] + ".Destroy", DestroyWindowCB, this);
	gui->RegisterCommand(vs[0] + ".AddPushButton", AddPushButtonCB, this);
	gui->RegisterCommand(vs[0] + ".AddToggleButton", AddToggleButtonCB, this);
	gui->RegisterCommand(vs[0] + ".AddSlider", AddSliderCB, this);
	gui->RegisterCommand(vs[0] + ".AddMonitor", AddMonitorCB, this);
}

void GUI_Fltk::DestroyWindowCB(void* ptr, string cmd, string args)
{
	//Fl::lock();
	UI.DestroyWindow(cmd);
	//Fl::unlock();
}

void GUI_Fltk::DestroyWindow(string cmd)
{
	string win_name = remove_suffix(cmd, ".Destroy");

	if(!check_window(win_name, "Destroy"))
		return;
	
	gui->UnRegisterCommand(win_name + ".Destroy");
	gui->UnRegisterCommand(win_name + ".AddPushButton");
	gui->UnRegisterCommand(win_name + ".AddToggleButton");
	gui->UnRegisterCommand(win_name + ".AddSlider");
	gui->UnRegisterCommand(win_name + ".AddMonitor");

	delete windows[win_name].win;
	//windows[win_name].win->hide();
	windows.erase(win_name);
}




////////////////////////////////////////////////////////////////////////////////
//
// Pushbutton stuff
//

void GUI_Fltk::AddPushButtonCB(void* ptr, string cmd, string args)
{
	//Fl::lock();
	UI.AddPushButton(cmd, args);
	//Fl::unlock();
}

class cmd_button:public Fl_Button
{
	public:
		cmd_button(string name, string command, GUI* pgui)
		:Fl_Button(0, 0, 1, 1),labl(name), cmd(command), gui(pgui)
		{
			label(labl.c_str());
			callback(my_callback);
		}

	private:
		//The button label just stores the pointer, so we need to store the string here	
		string cmd, labl;
		GUI*   gui;

		static void my_callback(Fl_Widget* w, long what_shall_I_do)
		{
			if(what_shall_I_do == POLL_UPDATE)
				return;
	
			cmd_button* b = (cmd_button*) w;
			b->gui->ParseLine(b->cmd);
		}
};



void GUI_Fltk::AddPushButton(string cmd, string args)
{
	string window_name = remove_suffix(cmd, ".AddPushButton");

	vector<string> vs = ChopAndUnquoteString(args);
	if(vs.size()!=2) 
	{
		cerr << "! GUI_Fltk::AddPushButton: Need 2 params (name, command)." << endl;
		return;
	};

	if(!check_window(window_name, "AddPushButton"))
		return;

	window& w=windows[window_name];

	//Create button

	Fl_Button* b = new cmd_button(vs[0], vs[1], gui);
	w.win->add(b);
}


////////////////////////////////////////////////////////////////////////////////
//
// Toggle button stuff
//

void GUI_Fltk::AddToggleButtonCB(void* ptr, string cmd, string args)
{
	//Fl::lock();
	UI.AddToggleButton(cmd, args);
	//Fl::unlock();
}

class toggle_button: public Fl_Check_Button
{
	public:
		toggle_button(string name, string gvar, GVars2* gv2, string def)
		:Fl_Check_Button(0, 0, 1, 1),labl(name)
		{
			callback(my_callback);

			gv2->Register(my_int, gvar, def, true);
			value(*my_int);
			label(labl.c_str());

			when(FL_WHEN_CHANGED);
		}

		void poll_update()
		{
			if(*my_int != value())
				value((bool)*my_int);
		}

	private:
		gvar2_int my_int;
		string labl;

		static void my_callback(Fl_Widget* w, long what_shall_I_do)
		{
			toggle_button* b = (toggle_button*)w;

			if(what_shall_I_do == POLL_UPDATE)
				b->poll_update();
			else
				*(b->my_int) = b->value();
		}
};


void GUI_Fltk::AddToggleButton(string cmd, string args)
{
	string window_name = remove_suffix(cmd, ".AddToggleButton");

	vector<string> vs = ChopAndUnquoteString(args);
	if(vs.size() != 2 && vs.size() != 3) 
	{
		cerr << "! GUI_Fltk::AddToggleButton: Need 2-3 params (name, gvar2_int name, {default})." << endl;
		return;
	}

	if(!check_window(window_name, "AddToggleButton"))
		return;

	window& w = windows[window_name];

	if(vs.size() == 2)
		vs.push_back("true");

	Fl_Widget* b = new toggle_button(vs[0], vs[1], gv2, vs[2]);
	w.win->add(b);
}

////////////////////////////////////////////////////////////////////////////////
//
// Slider stuff
//

void GUI_Fltk::AddSliderCB(void* ptr, string cmd, string args)
{
	//Fl::lock();
	UI.AddSlider(cmd, args);
	//Fl::unlock();
}

typedef Fl_Slider slider_type;

class slider_bar: public slider_type
{
	public:
		slider_bar(string gvar_name, GVars2 *pgv2, double min, double max)
		:slider_type(0, 0, 1, 1),gv2(pgv2),varname(gvar_name)
		{
			type(FL_HOR_SLIDER);
			bounds(min, max);
			callback(my_callback);
			when(FL_WHEN_CHANGED);
			step(0);
			poll_update();
		}

		void poll_update()
		{
			string crnt=gv2->StringValue(varname, true);

			
			if(crnt != cached_value)
			{
				cached_value = crnt;
				double newval=0;
				serialize::from_string(crnt, newval);

				//Update range if necessary
				if(newval > maximum()) maximum(newval);
				if(newval < minimum()) minimum(newval);

				value(newval);
			}
		}

		void set_gvar_to_value()
		{	
			ostringstream ost;
			ost << value();
			gv2->SetVar(varname, ost.str(), 1);
			cached_value = ost.str();
		}

	private:
		GVars2 *gv2;
		string varname, cached_value;

		static void my_callback(Fl_Widget* w, long what_shall_I_do)
		{
			slider_bar* s = (slider_bar*) w;

			if(what_shall_I_do == POLL_UPDATE)
				s->poll_update();
			else
				s->set_gvar_to_value();
		}
};

void GUI_Fltk::AddSlider(string cmd, string args)
{
	string win_name = remove_suffix(cmd, ".AddSlider");

	vector<string> vs = ChopAndUnquoteString(args);
	if(vs.size() != 3)
	{
		cerr << "! GUI_Fltk::AddSlider: Need 3 params (gvar_name min max)." << endl;
		return;
	}

	if(!check_window(win_name, "AddSlider"))
		return;
	
	window& w = windows[win_name];
	
	double min, max;
	
	serialize::from_string(vs[1], min);
	serialize::from_string(vs[2], max);

	Fl_Widget* b = new slider_bar(vs[0], gv2, min, max);

	w.win->add(b);
}


////////////////////////////////////////////////////////////////////////////////
//
// Monitor stuff
// 

void GUI_Fltk::AddMonitorCB(void* ptr, string cmd, string args)
{
	//Fl::lock();
	UI.AddMonitor(cmd, args);
	//Fl::unlock();
}

class monitor: public Fl_Box
{
	public:
		monitor(string t, string gvar_name, GVars2* pgv)
		:Fl_Box(0, 0, 1, 1),title(t), gv_name(gvar_name),gv(pgv) 
		{
			callback(my_callback);
			align(FL_ALIGN_TOP|FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
			poll_update();
		}

		void poll_update()
		{
			string gvar_text = gv->StringValue(gv_name);

			if(gvar_text != cached_gv_text)
			{
				cached_gv_text = gvar_text;
				full_label = title + ": " + cached_gv_text;
				label(full_label.c_str());
			}
		}

	private:
		string title, gv_name, full_label, cached_gv_text;
		GVars2* gv;

		static void my_callback(Fl_Widget* w, long what_shall_I_do)
		{
			if(what_shall_I_do == POLL_UPDATE)
				((monitor*)w)->poll_update();
		}

};


void GUI_Fltk::AddMonitor(string cmd, string args)
{
	string win_name = remove_suffix(cmd, ".AddMonitor");

	vector<string> vs = ChopAndUnquoteString(args);
	if(vs.size() != 2 && vs.size() != 2)
	{
		cerr << "! GUI_Fltk::AddMonitor: Need 2 or 3 params (label, gvar_name, {poll update !!ignored!! )." << endl;
		return;
	}

	if(!check_window(win_name, "AddSlider"))
		return;

	window& w = windows[win_name];
	
	Fl_Widget* m = new monitor(vs[0], vs[1], gv2);
	w.win->add(m);
}

//GUI_Fltk  Gui_Fltk(&GUI);
}
