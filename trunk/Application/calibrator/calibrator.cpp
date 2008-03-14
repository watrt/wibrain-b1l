/***************************************************************************
 *   Copyright (C) 2007 by Kevron Rees                                     *
 *   tripzero@nextabyte.com                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 * simple and generic touchscreen calibration program using the linux 2.6
 * input event interface
 * compile with command: 
 *	"g++ -L/usr/X11R6/lib/ -lX11 -o calibrator calibrator.cpp"
 * V 1.0RC4
 * Maintained by Kevron Rees tripzero@nextabyte.com
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>

///added
using namespace std;

/*****************************************************************************/
/* stuff for event interface */
struct input_event {
	struct timeval time;
	unsigned short type;
	unsigned short code;
	int value;
};

/* event types */
#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03

/* codes */
#define ABS_X			0x00
#define ABS_Y			0x01
#define ALT_ABS_X		0x02  //For samsung Q1 type touchscreens
#define ALT_ABS_Y		0x03  //For samsung Q1 type touchscreens
#define SYN_REPORT		0
#define BTN_LEFT		0x110
#define BTN_RIGHT		0x111
#define BTN_TOUCH		0x14a

/*****************************************************************************/
/* some constants */
#define FONT_NAME		"9x15"
#define IDLETIMEOUT		15
#define BLINKPERD		0.16        

#define ROUND_SYMBOL
#define NumRect			5

#if 1 
#define Background		cWHITE
#define	TouchedCross		cYELLOW
#define BlinkCrossBG		cRED
#define BlinkCrossFG		cBLACK
#define nTouchedCross		cCYAN
#define Cross			cWHITE
#define DrawGrid		cWHITE
#define DrawLine		cBLACK
#define DrawUp			cRED
#define DrawDown		cBLUE
#define TimerLine		cRED
#define PromptText		cBLACK
#else
#define Background		cBLACK
#define	TouchedCross		cYELLOW
#define BlinkCrossBG		cRED
#define BlinkCrossFG		cWHITE
#define nTouchedCross		cBLUE
#define Cross			cYELLOW
#define DrawGrid		cGREEN
#define DrawLine		cYELLOW
#define DrawUp			cRED
#define DrawDown		cGREEN
#define TimerLine		cRED
#define PromptText		cWHITE
#endif

#define N_Colors		10

static char colors[N_Colors][10] =
{ "BLACK", "WHITE", "RED", "YELLOW", "GREEN", "BLUE", "#40C0C0" };
				
static unsigned long pixels[N_Colors];

#define cBLACK			(pixels[0])
#define cWHITE			(pixels[1])
#define cRED			(pixels[2])
#define cYELLOW			(pixels[3])
#define cGREEN			(pixels[4])
#define cBLUE			(pixels[5])
#define cCYAN			(pixels[6])

/* some stupid loops */
#define SYS_1( zzz... ) do {	\
	while ( (zzz) != 1 );	\
} while (0)

#define SYS_0( zzz... ) do {	\
	while ( (zzz) != 0 );	\
} while (0)

/* where the calibration points are placed */
#define SCREEN_DIVIDE	16
#define SCREEN_MAX	0x800
#define M_POINT		(SCREEN_MAX/SCREEN_DIVIDE)
int MARK_POINT[] = { M_POINT, SCREEN_MAX - 1 - M_POINT };

/*****************************************************************************/
/* globals */
int job_done = 0;
int points_touched = 0;
//- string deviceName;
int points_x[4], points_y[4];
//- int do800x600=0;
//- int doedit=0;
//- string editfile="/etc/X11/xorg.conf";
Display *display;
int screen;
GC gc;
Window root;
Window win;
XFontStruct *font_info;
unsigned int width, height;	/* window size */
char *progname;

void editxorgconf(int, int, int, int, int, int);
/*****************************************************************************/

int get_events(int *px, int *py)
{
	int ret;
	int x = -1, y = -1;
	int touch = 0, sync = 0;
	//- struct input_event ev;
	char ev[255] = {0, };
	int evfd;

	/* read till sync event */
	while (!sync) {
	
		evfd = open("/proc/touchscreen/raw_data", O_RDONLY | O_NONBLOCK);

		if (evfd == -1) {
		   	fprintf(stderr, "Cannot open device file!\n");
		   	return 1;
	   	}

		ret = read(evfd, &ev, sizeof(ev));

		if (ret == -1) {
			close(evfd);
			return -1;
		}
#if 1
		if(ev[7] == 0x31) {
		   	if(x == -1) {
			   	x = (ev[13] - 0x30) * 1000;
			   	x += (ev[14] - 0x30) * 100;
			   	x += (ev[15] - 0x30) * 10;
			   	x += (ev[16] - 0x30);
		   	}
		   	if(y == -1) {
			   	y = (ev[22] - 0x30) * 1000;
			   	y += (ev[23] - 0x30) * 100;
			   	y += (ev[24] - 0x30) * 10;
			   	y += (ev[25] - 0x30);
		   	}
		}
			
		if((x != -1) && (y != -1) && (ev[7] == 0x30))
		   	sync = 1;
#else
		switch (ev.type) {
		case EV_ABS:
			switch (ev.code) {
			case ABS_X:
				if (x == -1) x = ev.value;
				break;

			case ABS_Y:
				if (y == -1) y = ev.value;
				break;
			case ALT_ABS_X:
				if(x==-1) x=ev.value;
				break;
			case ALT_ABS_Y:
				if(y==-1) y=ev.value;
				break;
			default:
				break;
			}

			break;

		case EV_KEY:
			switch (ev.code) {
			case BTN_LEFT:
			case BTN_TOUCH:
				touch = 1;

			default:
			       break;
			}

			break;

		case EV_SYN:
			if (ev.code == SYN_REPORT)
				sync = 1;

			break;

		default:
			break;
		}
#endif
			   	
		close(evfd);	

		//- if (!touch || x == -1 || y == -1)
		
		if (x == -1 || y == -1)
		   	return -1;
	}

	*px = x;
	*py = y;

	return 0;
}


/*****************************************************************************/

void cleanup_exit()
{
	SYS_1(XUnloadFont(display, font_info->fid));
	XUngrabServer(display);
	XUngrabKeyboard(display, CurrentTime);
	SYS_1(XFreeGC(display, gc));
	SYS_0(XCloseDisplay(display));
	//- close(evfd);
	exit(0);
}


void load_font(XFontStruct **font_info)
{
	char *fontname = FONT_NAME;

	if ((*font_info = XLoadQueryFont(display, fontname)) == NULL) {
		printf("Cannot open %s font\n", FONT_NAME);
		exit(1);
	}
}


void draw_point(int x, int y, int width, int size, unsigned long color)
{
	XSetForeground(display, gc, color);
	XSetLineAttributes(display, gc, width, LineSolid,
			   CapRound, JoinRound);
	XDrawLine(display, win, gc, x - size, y, x + size, y);
	XDrawLine(display, win, gc, x, y - size, x, y + size);
}


void point_blink(unsigned long color)
{
	int i, j;
	int cx, cy;
	static int shift = 0;
			
	if (points_touched != 4) {
		int RectDist = width / 200;
		i = points_touched / 2;
		j = points_touched % 2;
		cx = (MARK_POINT[j] * width) / SCREEN_MAX;
		cy = (MARK_POINT[i] * height) / SCREEN_MAX;

		XSetLineAttributes(display, gc, 1, LineSolid, CapRound, JoinRound);
	
		for (i = 0; i < NumRect; i++) {
			if ((i + shift) % NumRect == 0)
				XSetForeground(display, gc, BlinkCrossBG);
			else
				XSetForeground(display, gc, BlinkCrossFG);

#ifdef ROUND_SYMBOL
			XDrawArc(display, win, gc,
			         cx - i * RectDist, cy - i * RectDist,
			         i * (2 * RectDist), i * (2 * RectDist),
			         0, 359 * 64);
#else
			XDrawRectangle(display, win, gc,
				       cx - i * RectDist, cy - i * RectDist,
				       i * (2 * RectDist), i * (2 * RectDist));
#endif
		}
		shift++;
	}
}


void draw_message(char *msg)
{
	char buf[300];
	char *prompt[] = { buf };
#define num	(sizeof(prompt) / sizeof(prompt[0]))
	static int init = 0;
	static int p_len[num];
	static int p_width[num];
	static int p_height;
	static int p_maxwidth = 0;
	int i, x, y;
	int line_height;

	strncpy(buf, msg, sizeof buf);

	for (i = 0; i < num; i++) {
		p_len[i] = strlen(prompt[i]);
		p_width[i] = XTextWidth(font_info, prompt[i], p_len[i]);

		if (p_width[i] > p_maxwidth)
			p_maxwidth = p_width[i];
	}

	p_height = font_info->ascent + font_info->descent;
	init = 1;

	line_height = p_height + 5;
	x = (width - p_maxwidth) / 2;
	y = height / 2 - line_height;

	XSetForeground(display, gc, PromptText);
	XSetLineAttributes(display, gc, 3, LineSolid, CapRound, JoinRound);
	XClearArea(display, win, x - 8, y - 8 - p_height, p_maxwidth + 8 * 2,
	           num * line_height + 8 * 2, False);
	XDrawRectangle(display, win, gc, x - 8, y - 8 - p_height,
	               p_maxwidth + 8 * 2, num * line_height + 8 * 2);

	for (i = 0; i < num; i++) {
		XDrawString(display, win, gc, x, y + i * line_height, prompt[i],
			    p_len[i]);
	}
#undef num
}


void draw_text()
{
	static char *prompt[] = {
		"                    4-Pt Calibration",
		"Please touch the blinking symbol until beep or stop blinking",
		"                     (ESC to Abort)",
	};
#define num	(sizeof(prompt) / sizeof(prompt[0]))
	static int init = 0;
	static int p_len[num];
	static int p_width[num];
	static int p_height;
	static int p_maxwidth = 0;
	int i, x, y;
	int line_height;

	if (!init) {
		for (i = 0; i < num; i++) {
			p_len[i] = strlen(prompt[i]);
			p_width[i] = XTextWidth(font_info, prompt[i], p_len[i]);
			if (p_width[i] > p_maxwidth)
				p_maxwidth = p_width[i];
		}
		p_height = font_info->ascent + font_info->descent;
		init = 1;
	}
	line_height = p_height + 5;
	x = (width - p_maxwidth) / 2;
	y = height / 2 - 6 * line_height;

	XSetForeground(display, gc, PromptText);
	XClearArea(display, win, x - 11, y - 8 - p_height,
		   p_maxwidth + 11 * 2, num * line_height + 8 * 2, False);
	XSetLineAttributes(display, gc, 3, FillSolid,
			   CapRound, JoinRound);
	XDrawRectangle(display, win, gc, x - 11, y - 8 - p_height,
		       p_maxwidth + 11 * 2, num * line_height + 8 * 2);

	for (i = 0; i < num; i++) {
		XDrawString(display, win, gc, x, y + i * line_height, prompt[i],
			    p_len[i]);
	}
#undef num
}


void draw_graphics()
{
	int i, j;
	unsigned cx, cy;
	unsigned long color;

	draw_text();

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			int num = 2 * i + j;

			if (num == points_touched)
				continue;
			
			if (num > points_touched)
				color = nTouchedCross;
			else
				color = TouchedCross;

			cx = (MARK_POINT[j] * width) / SCREEN_MAX;
			cy = (MARK_POINT[i] * height) / SCREEN_MAX;
			draw_point(cx, cy, width / 200, width / 64, color);
		}
	}
}


void get_gc(Window win, GC *gc, XFontStruct *font_info)
{
	unsigned long valuemask = 0;	/* ignore XGCvalues and use defaults */
	XGCValues values;
	unsigned int line_width = 5;
	int line_style = LineSolid;
	int cap_style = CapRound;
	int join_style = JoinRound;

	*gc = XCreateGC(display, win, valuemask, &values);

	XSetFont(display, *gc, font_info->fid);

	XSetLineAttributes(display, *gc, line_width, line_style,
			   cap_style, join_style);
}


int get_color()
{
	int default_depth;
	Colormap default_cmap;
	XColor my_color;
	int i;

	default_depth = DefaultDepth(display, screen);
	default_cmap = DefaultColormap(display, screen);

	for (i = 0; i < N_Colors; i++) {
		XParseColor(display, default_cmap, colors[i], &my_color);
		XAllocColor(display, default_cmap, &my_color);
		pixels[i] = my_color.pixel;
	}

	return 0;
}


Cursor create_empty_cursor()
{
	char nothing[] = { 0 };
	XColor nullcolor;
	Pixmap src = XCreateBitmapFromData(display, root, nothing, 1, 1);
	Pixmap msk = XCreateBitmapFromData(display, root, nothing, 1, 1);
	Cursor mycursor = XCreatePixmapCursor(display, src, msk,
					      &nullcolor, &nullcolor, 0, 0);
	XFreePixmap(display, src);
	XFreePixmap(display, msk);

	return mycursor;
}


void process_event()
{
	XEvent event;

	while (XCheckWindowEvent(display, win, -1, &event) == True) {
		switch (event.type) {
		case KeyPress:
			{
				KeySym keysym = XKeycodeToKeysym(display,
							     event.xkey.keycode, 0);

				if (keysym == XK_Escape) {
					puts("Aborted");
					cleanup_exit();
				}
			}
			break;

		case Expose:
			draw_graphics(/*win, gc, width, height*/);
			break;

		default:
			break;
		}
	}
}

double idle_time = 0;
double tick = 0;

void set_timer(double interval /* in second */ )
{
	struct itimerval timer;
	long sec = (long)interval;
	long usec = (long)((interval - sec) * 1.0e6);

	timer.it_value.tv_sec = sec;
	timer.it_value.tv_usec = usec;
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_REAL, &timer, NULL);
	tick = interval;
}


void update_timer(void)
{
	int current = (int)(width * idle_time / IDLETIMEOUT);

	XSetLineAttributes(display, gc, 2, LineSolid, CapRound, JoinRound);
	XSetForeground(display, gc, Background);
	XDrawLine(display, win, gc, 0, height - 1, current, height - 1);
	XSetForeground(display, gc, TimerLine);
	XDrawLine(display, win, gc, current, height - 1, width, height - 1);
}


int register_fasync(int fd, void (*handle) (int))
{
	signal(SIGIO, handle);
	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC);
	return 0;
}


void sig_handler(int num)
{
	char buf[255] = {0, };
	int i, rval, x, y;
	static int is_busy = 0;

	if (is_busy)
		return;
	is_busy = 1;
		
	switch (num) {
	case SIGALRM:
		if (!job_done) {
			point_blink(BlinkCrossFG);
		}
			
		update_timer();
		
		if (idle_time >= IDLETIMEOUT)
			cleanup_exit();

		idle_time += tick;
			
		XFlush(display);
		process_event();

		rval = get_events(&x, &y);

		if(rval == -1)
			break;
		
		idle_time = 0;

		points_x[points_touched] = x;
		points_y[points_touched] = y;
		
		points_touched++;
		draw_graphics();

		break;

	case SIGIO:
		rval = get_events(&x, &y);

		if (rval == -1)
			break;

		idle_time = 0;

		points_x[points_touched] = x;
		points_y[points_touched] = y;
		
		points_touched++;
		draw_graphics();

		break;

	default:
		break;
	}

	/* do the math */
	if (points_touched == 4 && !job_done) {
		int x_low, y_low, x_hi, y_hi;
		int x_min, y_min, x_max, y_max;
		int x_seg, y_seg;
		int x_inv = 0, y_inv = 0;
		int rotate=0;

		if (abs(points_x[0] - points_x[1])<10)
    		{
		  x_low = (points_x[0] + points_x[1]) / 2;
    		  y_low = (points_y[0] + points_y[2]) / 2;
    		  x_hi = (points_x[2] + points_x[3]) / 2;
    		  y_hi = (points_y[1] + points_y[3]) / 2;
    		  rotate=1;
		}
		else
    		{
		 x_low = (points_x[0] + points_x[2]) / 2;
    		 y_low = (points_y[0] + points_y[1]) / 2;
   		 x_hi = (points_x[1] + points_x[3]) / 2;
    		 y_hi = (points_y[2] + points_y[3]) / 2;
		}

		/* see if one of the axes is inverted */
		if (x_low > x_hi) {
			int tmp = x_hi;
			x_hi = x_low;
			x_low = tmp;
			x_inv = 1;
		}
		if (y_low > y_hi) {
			int tmp = y_hi;
			y_hi = y_low;
			y_low = tmp;
			y_inv = 1;
		}


		/* calc the min and max values */
		x_seg = (x_hi - x_low) / (SCREEN_DIVIDE - 2);
		x_min = x_low - x_seg;
		x_max = x_hi + x_seg;

		y_seg = (y_hi - y_low) / (SCREEN_DIVIDE - 2);
		y_min = y_low - y_seg;
		y_max = y_hi + y_seg;
		
		
		/* print it, hint: evtouch has Y inverted */
		//- printf("Copy-Paste friendly, for evtouch XFree86 driver\n");
		//- printf("	Option \"MinX\" \"%d\"\n", x_inv ? x_max : x_min);
		//- printf("	Option \"MinY\" \"%d\"\n", y_inv ? y_max : y_min);
		//- printf("	Option \"MaxX\" \"%d\"\n", x_inv ? x_min : x_max);
		//- printf("	Option \"MaxY\" \"%d\"\n", y_inv ? y_min : y_max);
		
		draw_message("   Done...   ");
		XFlush(display);

		sprintf(buf, "/usr/local/bin/xorgApd.sh %d %d %d %d", x_min, y_min, x_max, y_max);
		system(buf);
	
		//- if(doedit) editxorgconf(x_inv ? x_max : x_min, x_inv ? x_min : x_max,
		//- 			y_inv ? y_min : y_max, y_inv ? y_max : y_min, y_inv, rotate);

		job_done = 1;
		idle_time = IDLETIMEOUT * 3 / 4;
		update_timer();
	}

	is_busy = 0;

	return;
}

//////////////////////////////////////////////////////////////////////////////
//           MY EDITS
//////////////////////////////////////////////////////////////////////////////
#if 0
void editxorgconf(int x_min, int x_max, int y_min, int y_max,int y_inv,int rotate)
{
	cout<<x_min<<" "<<x_max<<" "<<y_min<<" "<<y_max<<" inverted y: "<<y_inv<<endl;
	fstream xorg;
	ofstream newxorg;
	int pos=0;
	string line="";
	xorg.open(editfile.c_str());
	if(!xorg)
	{
		cout<<"xorg.conf failed to open"<<endl;
		return;
	}
	newxorg.open("newxorg.conf");
	if(!newxorg)
	{
		cout<<"newxorg.conf failed to open!"<<endl;
		return;
	}
	do
	{
		getline(xorg,line);
		pos=line.find("evtouch",pos);
		if(pos!=string::npos)
		{
			cout<<"already edited xorg.conf before"<<endl;
			cout<<"scuttling mission...(please edit manually)"<<endl;
			return;
		}
		pos=line.find("/dev/input/mice",pos);
		if(pos!=string::npos)
		{
			line.replace(pos,15,"/dev/input/mouse0");
			newxorg<<line<<endl;
			getline(xorg,line);
		}
		newxorg<<line<<endl;
		pos=line.find("ServerLayout",pos);
		if(pos!=string::npos)
		{
			newxorg<<"\tInputDevice\t\"touchscreen\"\t\"SendCoreEvents\""<<endl;		
			newxorg<<"\tInputDevice\t\"dummy\""<<endl;
		}
		if(do800x600)
		{
			pos=line.find("Modes",pos);
			if(pos!=string::npos)
			{
				newxorg<<"\tModes \"800x600\"\t\"640x480\""<<endl;		
			}
		}
		
	}while (!xorg.eof());
	
	xorg.close();
	//xorg.open("/etc/X11/xorg.conf",iostream::app);
	newxorg<<"Section \"InputDevice\"\n"
		"Identifier \"dummy\"\n"
		"Driver \"void\"\n"
		"Option \"Device\" \"/dev/input/mice\"\n"
		"EndSection"<<endl<<endl;

	newxorg<<"Section\t\"InputDevice\"\n"
		"\tIdentifier\t\"touchscreen\"\n"
		"\tDriver\t\"evtouch\"\n"
		"\tOption\t\"Device\"\t\"/dev/input/touchscreen\"\n"
		"\tOption\t\"DeviceName\"\t\"touchscreen\"\n"
		"\tOption\t\"MinX\"\t\t\""<<x_min<<"\"\n"
		"\tOption\t\"MinY\"\t\t\""<<y_min<<"\"\n"
		"\tOption\t\"MaxX\"\t\t\""<<x_max<<"\"\n"
		"\tOption\t\"MaxY\"\t\t\""<<y_max<<"\"\n"
		"\tOption\t\"SwapY\"\t\t\""<<y_inv<<"\"\n"
		"\tOption\t\"maybetapped_action\"\t\"click\"\n"
		"\tOption\t\"maybetapped_button\"\t\"1\"\n"
		"\tOption\t\"longtouch_action\"\t\"down\"\n"
		"\tOption\t\"longtouch_button\"\t\"1\"\n";
		if(rotate)newxorg<<"\tOption\t\"Rotate\"\t\"ccw\"\n";
		
		newxorg<<"\tOption\t\"ReportingMode\"\t\"Raw\"\n"
		"\tOption\t\"Emulate3Buttons\"\n"
		"\tOption\t\"Emulate3Timeout\"\t\"50\"\n"
		"\tOption\t\"SendCoreEvents\"\n"
		"EndSection"<<endl;
		
	newxorg.close();
	system("cp /etc/X11/xorg.conf /etc/X11/xorg.backup.conf");///make a back up of xorg.conf
	system("mv newxorg.conf /etc/X11/xorg.conf");
}
#endif

int main(int argc, char *argv[], char *env[])
{
	char *display_name = NULL;
	XSetWindowAttributes xswa;
	///added
	char answer;

	/* one arg: device name */
	//if (argc != 1) {
	//	fprintf(stderr, "Usage %s <device>!\n", argv[0]);
	//	return 1;
	//}

#if 0
	deviceName="/dev/input/touchscreen";
	string temp;

	for (int i=1;i<argc;i++)
	{
		temp=argv[i];
		if (temp=="-o")
		{
			doedit=1;
			if(argc>i)
			{ 
				editfile=argv[i+1];
				i++;
			}
		}
		else if (temp=="ice") do800x600=1;
		else deviceName=argv[i];
		evfd = open(deviceName.c_str(), O_RDONLY | O_NONBLOCK);
	}
#endif

	/* connect to X server */
	if ((display = XOpenDisplay(display_name)) == NULL) {
		fprintf(stderr, "%s: cannot connect to X server %s\n",
			progname, XDisplayName(display_name));
		//- close(evfd);
		exit(1);
	}

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);

	/* setup window attributes */
	xswa.override_redirect = True;
	xswa.background_pixel = BlackPixel(display, screen);
	xswa.event_mask = ExposureMask | KeyPressMask;
	xswa.cursor = create_empty_cursor();

	/* get screen size from display structure macro */
	
	width = DisplayWidth(display, screen);
	height = DisplayHeight(display, screen);
	
	//- cout<<"Trying to calibrate screen to: "<<width<<" x "<<height<<" resolution"<<endl;
	
	/*cout<<"Is this setting correct? y/n"<<endl;
	cin>>answer;
	if(answer=='n')
	{
		cout<<"Enter correct resolution width: ";
		cin>>width;
		cout<<"\nEnter correct resolution height: ";
		cin>>height;
	}*/

	win = XCreateWindow(display, RootWindow(display, screen),
	                    0, 0, width, height, 0,
	                    CopyFromParent, InputOutput, CopyFromParent,
	                    CWOverrideRedirect | CWBackPixel | CWEventMask |
	                    CWCursor, &xswa);
	XMapWindow(display, win);
	XGrabKeyboard(display, win, False, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	XGrabServer(display);
	load_font(&font_info);
	get_gc(win, &gc, font_info);
	get_color();

	XSetWindowBackground(display, win, Background);
	XClearWindow(display, win);

	signal(SIGALRM, sig_handler);
	set_timer(BLINKPERD);
	//- register_fasync(evfd, sig_handler);

	/* wait for signals */
	while (1)
	   	pause();

	return 0;
}
