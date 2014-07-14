//******************************************************************************
// $Header$
// Copyright (c) 1998,1999 by Flatland Online Inc.
// All rights reserved.  Patent Pending.
//******************************************************************************

// Header files required to implement class library.

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <unix.h>
#include <unistd.h>
#include <utsname.h>
#include <ctype.h>
#include <Dialogs.h>
#include <Resources.h>
#include <TextUtils.h>
#include <Fonts.h>
#include "Classes.h"
#include "Image.h"
#include "Main.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "Render.h"
#include "Spans.h"
#include "Utils.h"
#include "npapi.h"
#include "agl.h"
#include "gl.h"
#include "glu.h"

// Resource IDs for alerts.

#define FATAL_ERROR_ALERT	128
#define QUERY_ALERT			129
#define INFORMATION_ALERT	130
#define ABOUT_ALERT			131

// Resource IDs for query alert buttons.

#define YES_BUTTON			2
#define NO_BUTTON			3

// Resource IDs for task bar GIFs.

#define BUILDER0_GIF		128
#define BUILDER1_GIF		129
#define HISTORY0_GIF		130
#define HISTORY1_GIF		131
#define LEFT_GIF			132
#define LIGHT0_GIF			133
#define LIGHT1_GIF			134
#define LOGO0_GIF			135
#define LOGO1_GIF			136
#define OPTIONS0_GIF		137
#define OPTIONS1_GIF		138
#define RIGHT_GIF			139
#define SPLASH_GIF			140
#define TITLE_BG_GIF		141
#define TITLE_END_GIF		142

// Number of task bar buttons, and the logo label.

#define TASK_BAR_BUTTONS	4
#define LOGO_LABEL			"Go to Flatland!"

// Resource and menu IDs for menus.

#define OPTIONS_MENU		128
#define OPTIONS_MENU_ID		32766
#define LIGHT_MENU			129
#define LIGHT_MENU_ID		32767
#define DISTANCE_MENU		130
#define DISTANCE_MENU_ID	130

//------------------------------------------------------------------------------
// Icon class.
//------------------------------------------------------------------------------

// Icon class definition.

struct icon {
	texture *texture0_ptr;
	texture *texture1_ptr;
	int width, height;

	icon();
	~icon();
};

// Default constructor initialises the texture pointers.

icon::icon()
{
	texture0_ptr = NULL;
	texture1_ptr = NULL;
}

// Default destructor deletes the textures, if they exist.

icon::~icon()
{
	if (texture0_ptr)
		DEL(texture0_ptr, texture);
	if (texture1_ptr)
		DEL(texture1_ptr, texture);
}

//------------------------------------------------------------------------------
// Global variables.
//------------------------------------------------------------------------------

// Operating system name and version.

string os_version;

// Application directory.

string app_dir;

// Display properties.

int display_width, display_height, display_depth;
int window_width, window_height;
int render_mode;

// Flag indicating if player is running.

bool player_running;

// Flag indicating whether sound is enabled, the sound system being used,
// and whether reflections are available (for A3D only).

bool sound_on;
int sound_system;
bool reflections_available;

// Size of the heap zone allocated.

long heap_zone_size;

//------------------------------------------------------------------------------
// Local variables.
//------------------------------------------------------------------------------

// Reference number for the plugin's resource file.

static short plugin_res_file;

// Heap zone handles, and size of new heap zone.

Handle heap_zone_handle;
THz old_heap_zone_handle, new_heap_zone_handle;
#define MAX_HEAP_ZONE_SIZE	20 * 1024 * 1024
#define MIN_HEAP_ZONE_SIZE	4 * 1024 * 1024
#define MIN_HEAP_SIZE		512 * 1024

// AGL variables.

static AGLPixelFormat pixel_format;
static AGLContext agl_context;

// Size of parent window.

static int parent_window_width, parent_window_height;

// Position of plugin window.

static int window_x, window_y;

// GrafPort, clip rectangle and origin of active window.

static CGrafPort *active_graf_port_ptr;
static Rect active_clip_rect;
static int active_origin_x, active_origin_y;

// GrafPort, clip rectangle and origin of active window.

static CGrafPort *inactive_graf_port_ptr;
static Rect inactive_clip_rect;
static int inactive_origin_x, inactive_origin_y;

// Flag indicating whether the active window is ready.

static bool active_window_ready;

// Size of inactive window.

static inactive_window_width, inactive_window_height;

// Window data, window text and callback for inactive window.

static void *inactive_window_data_ptr;
static const char *inactive_window_text;
static void (*inactive_callback_ptr)(void *window_data_ptr);

// Saved graf port, port rectangle and clip region.

static CGrafPort *saved_graf_port_ptr;
static Rect saved_port_rect;
static RgnHandle saved_clip_rgn;

// Callback function pointers.

static void (*key_callback_ptr)(int key_code, bool key_down, int modifiers);
static void (*mouse_callback_ptr)(int x, int y, int button_code,
								  int task_bar_button_code, int modifiers);
static void (*timer_callback_ptr)(void);
static bool (*render_function_ptr)(void);

// Hardware texture class.

struct hardware_texture {
	int image_size_index;
	GLuint texture_ref_no;
	bool set;
};

// Pointer to current hardware texture.

static hardware_texture *curr_hardware_texture_ptr;

// Texture image array.

static GLubyte texture_image[256 * 256 * 4];

// Splash graphic texture.

static texture *splash_texture_ptr;

// Cursors.

static Cursor **arrow_cursor_handle_list[8];
static Cursor **hand_cursor_handle;
static Cursor **crosshair_cursor_handle;
static Cursor **curr_cursor_handle;

// Task bar icons.

static icon *logo_icon_ptr;
static icon *left_edge_icon_ptr;
static icon *button_icon_list[TASK_BAR_BUTTONS];
static icon *right_edge_icon_ptr;
static icon *title_bg_icon_ptr;
static icon *title_end_icon_ptr;

// X coordinates of all the task bar elements.

static int logo_x;
static int left_edge_x;
static int button_x_list[TASK_BAR_BUTTONS + 1];
static int right_edge_x;
static int title_start_x;
static int title_end_x;

// Task bar button GIF resource IDs and labels.

static int button0_ID_list[TASK_BAR_BUTTONS] = {
	HISTORY0_GIF, OPTIONS0_GIF, LIGHT0_GIF, BUILDER0_GIF
};

static int button1_ID_list[TASK_BAR_BUTTONS] = {
	HISTORY1_GIF, OPTIONS1_GIF, LIGHT1_GIF, BUILDER1_GIF
};

static char *button_label_list[TASK_BAR_BUTTONS] = { 
	"Recent Spots", "Options & Help", "Brightness", "Builders Guide"
};

// Task bar variables.

static bool task_bar_enabled;
static int active_button_index;
static int prev_active_button_index;
static bool logo_active;
static bool prev_logo_active;
static int last_logo_time_ms;

// Splash graphic flag.

static bool splash_graphic_enabled;

// Title and label text.

static string title_text;
static string label_text;
static bool label_visible;

// Current mouse position in global coordinates.

static Point global_mouse_point;

// Options and light menu handles.

static MenuHandle options_menu_handle;
static MenuHandle light_menu_handle;
static MenuHandle distance_menu_handle;

// Last key down event.

static int last_key_down_code;
static int last_key_down_time;
static int last_key_down_modifiers;

//==============================================================================
// Private functions.
//==============================================================================

//------------------------------------------------------------------------------
// Initialise the active viewport.
//------------------------------------------------------------------------------

static void
init_active_viewport(NPWindow *np_window_ptr)
{
	NPRect *np_clip_rect_ptr;
	
	// Determine the origin and clipping rectangle for the active window.
	
	np_clip_rect_ptr = &np_window_ptr->clipRect;
	active_origin_x = -np_window_ptr->x;
	active_origin_y = -np_window_ptr->y;
	active_clip_rect.left = np_clip_rect_ptr->left + active_origin_x;
	active_clip_rect.top = np_clip_rect_ptr->top + active_origin_y;
	active_clip_rect.right = np_clip_rect_ptr->right + active_origin_x;
	active_clip_rect.bottom = np_clip_rect_ptr->bottom + active_origin_y;
}

//------------------------------------------------------------------------------
// Initialise the inactive viewport.
//------------------------------------------------------------------------------

static void
init_inactive_viewport(NPWindow *np_window_ptr)
{
	NPRect *np_clip_rect_ptr;
	
	// Determine the origin and clipping rectangle for the active window.
	
	np_clip_rect_ptr = &np_window_ptr->clipRect;
	inactive_origin_x = -np_window_ptr->x;
	inactive_origin_y = -np_window_ptr->y;
	inactive_clip_rect.left = np_clip_rect_ptr->left + inactive_origin_x;
	inactive_clip_rect.top = np_clip_rect_ptr->top + inactive_origin_y;
	inactive_clip_rect.right = np_clip_rect_ptr->right + inactive_origin_x;
	inactive_clip_rect.bottom = np_clip_rect_ptr->bottom + inactive_origin_y;
}

//------------------------------------------------------------------------------
// Set the viewport.
//------------------------------------------------------------------------------

static void
set_viewport(void)
{
	GLint window_rect[4];
	
	// Initialise the window rectangle for OpenGL.
	
	window_rect[0] = -active_origin_x;
	window_rect[1] = parent_window_height - 
		(-active_origin_y + display_height);
	window_rect[2] = display_width;
	window_rect[3] = display_height;
	
	// Set the buffer rectangle and viewport to match the plugin window's
	// rectangle.
	
	aglSetInteger(agl_context, AGL_SWAP_RECT, window_rect);
	glViewport(window_rect[0], window_rect[1], window_rect[2], window_rect[3]);		
}

//------------------------------------------------------------------------------
// Set up the port for the active window.
//------------------------------------------------------------------------------

static void
set_active_port(void)
{
	// Set the OpenGL viewport.
	
	set_viewport();
	
	// Preserve the current GrafPort, and set the GrafPort for the active 
	// window.
					
	GetPort((GrafPort **)&saved_graf_port_ptr);
	SetPort((GrafPort *)active_graf_port_ptr);
	
	// Preserve the old port rectangle and clipping region.
			
	saved_port_rect = active_graf_port_ptr->portRect;
	GetClip(saved_clip_rgn);
			
	// Set the new clipping region and port origin.
	
	ClipRect(&active_clip_rect);
	SetOrigin(active_origin_x, active_origin_y);
}

//------------------------------------------------------------------------------
// Set up the port for the inactive window.
//------------------------------------------------------------------------------

static void
set_inactive_port(void)
{
	// Set the OpenGL viewport.
	
	set_viewport();
	
	// Preserve the current GrafPort, and set the GrafPort for the inactive 
	// window.
					
	GetPort((GrafPort **)&saved_graf_port_ptr);
	SetPort((GrafPort *)inactive_graf_port_ptr);
	
	// Preserve the old port rectangle and clipping region.
			
	saved_port_rect = inactive_graf_port_ptr->portRect;
	GetClip(saved_clip_rgn);
			
	// Set the new clipping region and port origin.
	
	ClipRect(&inactive_clip_rect);
	SetOrigin(inactive_origin_x, inactive_origin_y);
}

//------------------------------------------------------------------------------
// Reset the port that was used for the active window.
//------------------------------------------------------------------------------

static void
reset_port(void)
{
	// Restore the original port origin, clipping region, and GrafPort.
	
	SetOrigin(saved_port_rect.left, saved_port_rect.top);
	SetClip(saved_clip_rgn);
	SetPort((GrafPort *)saved_graf_port_ptr);
}

//------------------------------------------------------------------------------
// Convert a C string into a Pascal string.  Note that this function uses a
// static buffer to hold the converted string.
//------------------------------------------------------------------------------

static unsigned char *
pstring(char *cstring)
{
	static unsigned char pbuffer[256];
	int length;
	
	length = strlen(cstring);
	if (length > 255)
		length = 255;
	pbuffer[0] = length;
	strncpy((char *)&pbuffer[1], cstring, length);
	return(pbuffer);
}

//------------------------------------------------------------------------------
// Load a GIF resource.
//------------------------------------------------------------------------------

static texture *
load_GIF_resource(short resource_ID)
{
	Handle resource_handle;
	long resource_size;
	char *resource_ptr;
	texture *texture_ptr;

	// Load the GIF resource with the given ID, determine it's size, then lock
	// the resource handle to obtain a pointer to the raw data.

	if ((resource_handle = GetResource('GIF ', resource_ID)) == NULL)
		return(NULL);
	resource_size = GetResourceSizeOnDisk(resource_handle);
	HLock(resource_handle);
	resource_ptr = (char *)*resource_handle;

	// Open the raw data as if it were a file.

	if (!push_buffer(resource_ptr, resource_size)) {
		HUnlock(resource_handle);
		ReleaseResource(resource_handle);
		return(NULL);
	}

	// Load the raw data as a GIF.

	if ((texture_ptr = load_GIF_image()) == NULL) {
		pop_file();
		HUnlock(resource_handle);
		ReleaseResource(resource_handle);
		return(NULL);
	}

	// Close the raw data "file", release the resource, and return a pointer to
	// the texture.

	pop_file();
	HUnlock(resource_handle);
	ReleaseResource(resource_handle);
	return(texture_ptr);
}

//------------------------------------------------------------------------------
// Load an icon.
//------------------------------------------------------------------------------

static icon *
load_icon(short resource0_ID, short resource1_ID)
{
	icon *icon_ptr;

	// Create the icon.

	NEW(icon_ptr, icon);
	if (icon_ptr == NULL)
		return(NULL);

	// Load the GIF resources as textures.  If the second resource ID is zero,
	// there is no second texture for this icon.

	if ((icon_ptr->texture0_ptr = load_GIF_resource(resource0_ID)) == NULL ||
		(resource1_ID > 0 &&
		 (icon_ptr->texture1_ptr = load_GIF_resource(resource1_ID)) == NULL)) {
		DEL(icon_ptr, icon);
		return(NULL);
	}

	// Initialise the size of the icon.

	icon_ptr->width = icon_ptr->texture0_ptr->width;
	icon_ptr->height = icon_ptr->texture0_ptr->height;
	return(icon_ptr);
}

//------------------------------------------------------------------------------
// Draw an icon onto the frame buffer surface at the given x coordinate, and
// aligned to the bottom of the display.
//------------------------------------------------------------------------------

static void
draw_icon(icon *icon_ptr, bool active_icon, int x)
{
	int y;
	texture *texture_ptr;
	pixmap *pixmap_ptr;

	// Select the correct icon texture.

	if (active_icon)
		texture_ptr = icon_ptr->texture1_ptr;
	else
		texture_ptr = icon_ptr->texture0_ptr;

	// Draw the current pixmap.

	pixmap_ptr = texture_ptr->get_curr_pixmap_ptr(get_time_ms());
	y = window_height - icon_ptr->height;
	draw_pixmap(pixmap_ptr, 0, x, y);
}

//------------------------------------------------------------------------------
// Draw the logo.
//------------------------------------------------------------------------------

static void
draw_logo(bool logo_active, bool clear_background)
{
	if (clear_background) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	draw_icon(logo_icon_ptr, logo_active, logo_x);
}

//------------------------------------------------------------------------------
// Draw the buttons on the task bar.
//------------------------------------------------------------------------------

static void
draw_buttons(bool loading_spot)
{
	int button_index;
	bool button_active;

	// Draw the left edge of the button bar.

	draw_icon(left_edge_icon_ptr, false, left_edge_x);

	// Draw each button.

	for (button_index = 0; button_index < TASK_BAR_BUTTONS; button_index++) {

		// Determine if this button is active.  If we are loading a spot or
		// running hardware acceleration full-screen, no buttons should be
		// active.

		if (loading_spot || full_screen)
			button_active = false;
		else {
			start_atomic_operation();
			button_active = button_index == active_button_index;
			end_atomic_operation();
		}

		// Draw the active or inactive texture for this button.

		draw_icon(button_icon_list[button_index], button_active,
			button_x_list[button_index]);
	}

	// Draw the right edge of the button bar.

	draw_icon(right_edge_icon_ptr, false, right_edge_x);

	// If not running full-screen and a new task bar button has become active,
	// show it's label, otherwise display a generic inactive message.  If all
	// task bar buttons have become inactive, hide the label.

	start_atomic_operation();
	if (active_button_index >= 0 && 
		active_button_index != prev_active_button_index)
		show_label(button_label_list[active_button_index]);
	if (active_button_index < 0 && prev_active_button_index >= 0)
		hide_label();
	prev_active_button_index = active_button_index;
	end_atomic_operation();
}

//------------------------------------------------------------------------------
// Draw the title on the task bar.
//------------------------------------------------------------------------------

static void
draw_title(void)
{
	int x, width, max_width;
	GLfloat raster_pos[4];

	// Draw the background for the title (the text is drawn in display_buffer
	// after the OpenGL buffers have been swapped).

	for (x = title_start_x; x < title_end_x; x += title_bg_icon_ptr->width)
		draw_icon(title_bg_icon_ptr, false, x);
	draw_icon(title_end_icon_ptr, false, title_end_x);
		
	// Temporarily switch to an orthographic projection whose clipping
	// planes match the size of the display window.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, window_width, 0.0, window_height);

	// Disable texture mapping and alpha blending.

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	
	// Draw the string with a depth function of GL_NEVER, and calculate
	// the width of the string in pixels.
	
	glDepthFunc(GL_NEVER);
	glRasterPos2i(title_start_x, 0);
	glGetFloatv(GL_CURRENT_RASTER_POSITION, (GLfloat *)&raster_pos);
	x = (int)raster_pos[0];
	glListBase(0);
	glCallLists(strlen(title_text), GL_BYTE, title_text);
	glGetFloatv(GL_CURRENT_RASTER_POSITION, (GLfloat *)&raster_pos);
	width = (int)raster_pos[0] - x;
	
	// Calculate a new raster position for the string that will center
	// it, then draw the string with a depth function of GL_ALWAYS.
	
	max_width = title_end_x - title_start_x;
	if (width > max_width)
		x = title_start_x;
	else
		x = title_start_x + (max_width - width) / 2;
	glDepthFunc(GL_ALWAYS);
	glColor3ub(255, 204, 102);
	glRasterPos2i(x, 5);
	glListBase(0);
	glCallLists(strlen(title_text), GL_BYTE, title_text);

	// Switch back to the perspective projection, re-enable texture
	// mapping and alpha blending, then restore the depth function
	// to GL_LESS.

	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LESS);
}

//------------------------------------------------------------------------------
// Draw the splash graphic.
//------------------------------------------------------------------------------

static void
draw_splash_graphic(void)
{
	// Draw the splash graphic and A3D splash graphic if both exist and the
	// splash graphic is smaller than the 3D window.

	if (splash_texture_ptr && 
		splash_texture_ptr->width <= window_width &&
		splash_texture_ptr->height <= window_height) {
		pixmap *pixmap_ptr;
		int x, y;

		// Draw the first pixmap of the splash graphic.

		pixmap_ptr = &splash_texture_ptr->pixmap_list[0];

		// Center the pixmap in the window.

		x = (window_width - splash_texture_ptr->width) / 2;
		y = (window_height - splash_texture_ptr->height) / 2;

		// Draw the pixmap at full brightness.

		draw_pixmap(pixmap_ptr, 0, x, y);
	}
}
	
//------------------------------------------------------------------------------
// Draw the task bar.
//------------------------------------------------------------------------------

static void
draw_task_bar(bool loading_spot)
{
	bool logo_active_flag;

	START_TIMING;

	// If the spot is being loaded, draw the logo as active.

	if (loading_spot)
		draw_logo(true, true);
	
	// If the spot is not been loaded...
	
	else {

		// Draw the logo as inactive or active depending on whether the
		// mouse is pointing at it.
		
		start_atomic_operation();
		logo_active_flag = logo_active;
		end_atomic_operation();
		draw_logo(logo_active_flag, false);

		// Display a label above the logo while it's active.  If full screen
		// mode is in effect, this is a generic inactive message.

		if (logo_active_flag && !prev_logo_active)
			show_label(LOGO_LABEL);
		if (!logo_active_flag && prev_logo_active)
			hide_label();
		prev_logo_active = logo_active_flag;
	}

	// Draw the buttons and title.

	draw_buttons(loading_spot);
	draw_title();

	END_TIMING("draw_task_bar");
}

//------------------------------------------------------------------------------
// Draw a label.
//------------------------------------------------------------------------------

static void
draw_label(void)
{
	int x, y, width;
	GLfloat raster_pos[4];
		
	// Temporarily switch to an orthographic projection whose clipping
	// planes match the size of the display window.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, window_width, 0.0, window_height);

	// Disable texture mapping and alpha blending.

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	
	// Draw the label with a depth function of GL_NEVER, and calculate
	// the width of the label in pixels.
	
	glDepthFunc(GL_NEVER);
	glRasterPos2i(0, 0);
	glGetFloatv(GL_CURRENT_RASTER_POSITION, (GLfloat *)&raster_pos);
	x = (int)raster_pos[0];
	glListBase(0);
	glCallLists(strlen(label_text), GL_BYTE, label_text);
	glGetFloatv(GL_CURRENT_RASTER_POSITION, (GLfloat *)&raster_pos);
	width = (int)raster_pos[0] - x;	

	// Place the label above or below the mouse cursor, making sure it doesn't
	// get clipped by the right edge of the screen.

	if (curr_cursor_handle) {
		x = mouse_x - (*curr_cursor_handle)->hotSpot.h;
		y = mouse_y - (*curr_cursor_handle)->hotSpot.v + 16;
		if (y + 10 >= window_height)
			y = mouse_y - (*curr_cursor_handle)->hotSpot.v - TASK_BAR_HEIGHT;
	} else {
		x = mouse_x;
		y = window_height - TASK_BAR_HEIGHT;
	}
	if (width > window_width)
		x = 0;
	else if (x + width >= window_width)
		x = window_width - width;
	y = window_height - y;
		
	// Draw the label text.

	glDepthFunc(GL_ALWAYS);
	glColor3ub(255, 204, 102);
	glRasterPos2i(x, y);
	glListBase(0);
	glCallLists(strlen(label_text), GL_BYTE, label_text);

	// Switch back to the perspective projection, re-enable texture
	// mapping and alpha blending, then restore the depth function
	// to GL_LESS.

	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LESS);
}

//==============================================================================
// Heap zone functions.
//==============================================================================

//------------------------------------------------------------------------------
// Enable the new heap zone.  If the minimum heap size has been reached,
// return FALSE.
//------------------------------------------------------------------------------

bool
new_heap_zone(void)
{	
	SetZone(new_heap_zone_handle);
	return(MaxBlock() >= MIN_HEAP_SIZE);
}

//------------------------------------------------------------------------------
// Enable the old heap zone.
//------------------------------------------------------------------------------

void
old_heap_zone(void)
{
	SetZone(old_heap_zone_handle);
}

//==============================================================================
// Thread synchronisation functions.
//==============================================================================

//------------------------------------------------------------------------------
// Start an atomic operation.
//------------------------------------------------------------------------------

void
start_atomic_operation(void)
{
}

//------------------------------------------------------------------------------
// End an atomic operation.
//------------------------------------------------------------------------------

void
end_atomic_operation(void)
{
}

//==============================================================================
// Start up/Shut down functions.
//==============================================================================

//------------------------------------------------------------------------------
// Start up the platform API.
//------------------------------------------------------------------------------

bool
start_up_platform_API(void)
{
	Handle resource_handle;
	long grow;
	short curr_res_file;
	OSErr result;
	char cwd[256];
	int index, icon_x;
	struct utsname name_info;
	
	// Get the special 'FLAT' resource with ID of 128.  Then use this
	// resource to get the reference number for the plugin's resource file.

	if ((resource_handle = GetResource('FLAT', 128)) == NULL)
		return(false);
	plugin_res_file = HomeResFile(resource_handle);
	ReleaseResource(resource_handle);

	// Allocate a new heap zone in temporary memory, and initialise it.  We
	// aim to allocate no less than MIN_HEAP_ZONE_SIZE bytes, and no more than 
	// MAX_HEAP_ZONE_SIZE bytes, while leaving at least MIN_HEAP_ZONE_SIZE bytes
	// of temporary memory for other applications.
	
	old_heap_zone_handle = GetZone();
	heap_zone_size = TempMaxMem(&grow) - MIN_HEAP_ZONE_SIZE;
	if (heap_zone_size < MIN_HEAP_ZONE_SIZE)
		return(false);
	if (heap_zone_size > MAX_HEAP_ZONE_SIZE)
		heap_zone_size = MAX_HEAP_ZONE_SIZE;
	if ((heap_zone_handle = TempNewHandle(heap_zone_size, &result)) == NULL)
		return(false);
	HLock(heap_zone_handle);
	InitZone(NULL, 16, *heap_zone_handle + heap_zone_size, *heap_zone_handle);
	new_heap_zone_handle = GetZone();
	SetZone(old_heap_zone_handle);

	// Initialise global and local variables.
	
	player_running = false;
	logo_icon_ptr = NULL;
	left_edge_icon_ptr = NULL;
	right_edge_icon_ptr = NULL;
	title_bg_icon_ptr = NULL;
	title_end_icon_ptr = NULL;
	for (index = 0; index < TASK_BAR_BUTTONS; index++)
		button_icon_list[index] = NULL;
	splash_texture_ptr = NULL;
		
	// Determine the OS version.
		
	uname(&name_info);
	os_version = name_info.sysname;
	os_version += " ";
	os_version += name_info.release;
	os_version += ".";
	os_version += name_info.version;
	
	// Get the path of the browser application, and add the path to the
	// plug-ins folder.
	
	getcwd(cwd, 256);
	app_dir = cwd;
	app_dir += "Plug-ins:";
	
	// Initialise some global variables.
	
	active_window_ready = false;
	render_mode = HARDWARE_WINDOW;
	sound_system = NO_SOUND;
	
	// Set the creator and type for files created by Rover.
	
	_fcreator = 'ttxt';
	_ftype = 'TEXT';
	
	// Initialise the dialog manager.
	
	InitDialogs(NULL);
	
	// Switch to the plugin's resource file.
	
	curr_res_file = CurResFile();
	UseResFile(plugin_res_file);
	
	// Load the splash graphic texture resource.

	if ((splash_texture_ptr = load_GIF_resource(SPLASH_GIF)) == NULL)
		return(false);

	// Load the cursor resources.
	
	for (index = 0; index < 8; index++)
		arrow_cursor_handle_list[index] = GetCursor(index + 128);
	hand_cursor_handle = GetCursor(136);
	crosshair_cursor_handle = GetCursor(137);
	
	// Load the task bar icon texture resources.

	if ((logo_icon_ptr = load_icon(LOGO0_GIF, LOGO1_GIF)) == NULL ||
		(left_edge_icon_ptr = load_icon(LEFT_GIF, 0)) == NULL ||
		(right_edge_icon_ptr = load_icon(RIGHT_GIF, 0)) == NULL || 
		(title_bg_icon_ptr = load_icon(TITLE_BG_GIF, 0)) == NULL ||
		(title_end_icon_ptr = load_icon(TITLE_END_GIF, 0)) == NULL)
		return(false);
	for (index = 0; index < TASK_BAR_BUTTONS; index++)
		if ((button_icon_list[index] = load_icon(button0_ID_list[index],
			 button1_ID_list[index])) == NULL)
			 return(false);
			 
	// Load the options, light and distance menus.
		
	options_menu_handle = GetMenu(OPTIONS_MENU);
	InsertMenu(options_menu_handle, -1);
	light_menu_handle = GetMenu(LIGHT_MENU);
	InsertMenu(light_menu_handle, -1);
	distance_menu_handle = GetMenu(DISTANCE_MENU);
	InsertMenu(distance_menu_handle, -1);
	
	// Switch back to the current resource file.
	
	UseResFile(curr_res_file);

	// Initialise the x coordinates of the task bar elements, except for
	// title_end_x which cannot be determined until the 3D window is created.

	logo_x = 0;
	left_edge_x = logo_icon_ptr->width;
	icon_x = left_edge_x + left_edge_icon_ptr->width;
	for (index = 0; index < TASK_BAR_BUTTONS; index++) {
		button_x_list[index] = icon_x;
		icon_x += button_icon_list[index]->width;
	}
	right_edge_x = icon_x;
	button_x_list[TASK_BAR_BUTTONS] = right_edge_x;
	title_start_x = right_edge_x + right_edge_icon_ptr->width;
	
	// Create the saved clip region.
	
	saved_clip_rgn = NewRgn();
	return(true);
}

//------------------------------------------------------------------------------
// Shut down the platform API.
//------------------------------------------------------------------------------

void
shut_down_platform_API(void)
{
	// Delete the splash graphic texture.
	
	if (splash_texture_ptr)
		DEL(splash_texture_ptr, texture);
		
	// Delete the icons.

	if (logo_icon_ptr)
		DEL(logo_icon_ptr, icon);
	if (left_edge_icon_ptr)
		DEL(left_edge_icon_ptr, icon);
	if	(right_edge_icon_ptr)
		DEL(right_edge_icon_ptr, icon);
	if	(title_bg_icon_ptr)
		DEL(title_bg_icon_ptr, icon);
	if	(title_end_icon_ptr)
		DEL(title_end_icon_ptr, icon);
	for (int index = 0; index < TASK_BAR_BUTTONS; index++)
		if (button_icon_list[index])
			DEL(button_icon_list[index], icon);

	// Destroy the options, light and distance menus.
		
	DeleteMenu(OPTIONS_MENU_ID);
	ReleaseResource((Handle)options_menu_handle);
	DeleteMenu(LIGHT_MENU_ID);
	ReleaseResource((Handle)light_menu_handle);
	DeleteMenu(DISTANCE_MENU_ID);
	ReleaseResource((Handle)distance_menu_handle);
	
	// Dispose of the saved clip region.
				
	DisposeRgn(saved_clip_rgn);
	
	// Dispose of the alternate heap zone.
	
	HUnlock(heap_zone_handle);
	DisposeHandle(heap_zone_handle);
}

//==============================================================================
// Player functions.
//==============================================================================

// Save the pointer to the render function, and set the flag that indicates
// the player is running.

void
start_player(bool (*render_function)(void))
{
	render_function_ptr = render_function;
	player_running = true;
}

// Call the render function, returning it's value.

bool
render_spot(void)
{
	return((*render_function_ptr)());
}

// Reset the flag that indicates the player is running.

void
stop_player(void)
{
	player_running = false;
}

//==============================================================================
// Inactive window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Draw an inactive window.
//------------------------------------------------------------------------------

static void
draw_inactive_window(void)
{
	RGBColor colour;
	Rect rect;
	int text_width, x, y;
	
	// Fill the inactive window with a black background.
	
	colour.red = 0;
	colour.green = 0;
	colour.blue = 0;
	RGBForeColor(&colour);
	rect.left = 0;
	rect.top = 0;
	rect.right = inactive_window_width;
	rect.bottom = inactive_window_height;
	PaintRect(&rect);
	
	// Set the characteristics of the text font.
	
	TextFont(applFont);
	TextFace(bold);
	TextSize(10);
	
	// Determine the size of the inactive window text, and center it
	// horizontally and vertically.
	 
	text_width = TextWidth(inactive_window_text, 0, 
		strlen(inactive_window_text));
	x = (inactive_window_width - text_width) / 2;
	y = (inactive_window_height - 10) / 2 + 10;
	
	// Draw the text.
	
	colour.red = 0xffff;
	colour.green = 0xcccc;
	colour.blue = 0x6666;
	RGBForeColor(&colour);
	MoveTo(x, y);
	DrawText(inactive_window_text, 0, strlen(inactive_window_text));
}

//------------------------------------------------------------------------------
// Create an inactive window.
//------------------------------------------------------------------------------

void
create_inactive_window(void *window_ptr, void *window_data_ptr,
				       const char *window_text, 
				       void (*inactive_callback)(void *window_data_ptr))
{
	NPWindow *np_window_ptr;
	NP_Port *np_port_ptr;
	
	// Save the size of the inactive window and initialise the inactive
	// viewport.
	
	np_window_ptr = (NPWindow *)window_ptr;
	inactive_window_width = np_window_ptr->width;
	inactive_window_height = np_window_ptr->height;
	np_port_ptr = (NP_Port *)np_window_ptr->window;
	active_graf_port_ptr = np_port_ptr->port;
	init_inactive_viewport(np_window_ptr);
	
	// Save the window data pointer, window text, and inactive callback.
	
	inactive_window_data_ptr = window_data_ptr;
	inactive_window_text = window_text;
	inactive_callback_ptr = inactive_callback;
	
	// Draw the inactive window now.
	
	set_inactive_port();
	draw_inactive_window();
	reset_port();
}

//------------------------------------------------------------------------------
// Handle an event in an inactive window.
//------------------------------------------------------------------------------

int
handle_inactive_event(void *window_ptr, void *event_ptr)
{
	EventRecord *event_record_ptr;
	int event_handled;
	
	// Handle the event.
	
	event_handled = FALSE;
	event_record_ptr = (EventRecord *)event_ptr;
	switch (event_record_ptr->what) {
	case updateEvt:
		draw_inactive_window();
		event_handled = TRUE;
		break;
	case mouseDown:
		(*inactive_callback_ptr)(inactive_window_data_ptr);
		event_handled = TRUE;
	}	
	return(event_handled);
}

//------------------------------------------------------------------------------
// Destroy an inactive window.
//------------------------------------------------------------------------------

void
destroy_inactive_window(void *window_ptr)
{
}

//==============================================================================
// Active window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Set the active window size.
//------------------------------------------------------------------------------

void
set_active_window_size(int width, int height)
{	
	// Remember this width and height as the previous set dimensions (for
	// writing to the config file) as well as the current dimensions.
	
	prev_window_width = width;
	prev_window_height = height;
	display_width = width;
	display_height = height;
	
	// If the width of the main window is less than 320 or the height is less
	// than 240, disable the task bar and splash graphic.

	window_width = width;
	if (width < 320 || height < 240) {
		window_height = height;
		task_bar_enabled = false;
		splash_graphic_enabled = false;
	} else {
		window_height = display_height - TASK_BAR_HEIGHT;
		task_bar_enabled = true;
		splash_graphic_enabled = true;
	}
	
	// Reset the x coordinate of the end of the task bar title.

	title_end_x = display_width - title_end_icon_ptr->width;
}

//------------------------------------------------------------------------------
// Create the active window.
//------------------------------------------------------------------------------

bool
create_active_window(void *window_ptr,
				     void (*key_callback)(int key_code, bool key_down,
				   						  int modifiers),
				     void (*mouse_callback)(int x, int y, int button_code,
									        int task_bar_button_code,
									        int modifiers),
				     void (*timer_callback)(void))
{
	NPWindow *np_window_ptr;
	NP_Port *np_port_ptr;
	GLint attrib[] = {
		AGL_ALL_RENDERERS, 
		AGL_RGBA,
		AGL_DOUBLEBUFFER,
		AGL_DEPTH_SIZE, 32,
		AGL_NONE 
	};
	
	// Save callback function pointers.
	
	key_callback_ptr = key_callback;
	mouse_callback_ptr = mouse_callback;
	timer_callback_ptr = timer_callback;
	
	// Get a pointer to the active window's GrafPort.
	
	np_window_ptr = (NPWindow *)window_ptr;
	np_port_ptr = (NP_Port *)np_window_ptr->window;
	active_graf_port_ptr = np_port_ptr->port;
		
	// Determine the size of the parent window.
	
	parent_window_width = active_graf_port_ptr->portRect.right - 
		active_graf_port_ptr->portRect.left;
	parent_window_height = active_graf_port_ptr->portRect.bottom - 
		active_graf_port_ptr->portRect.top;
			
	// Choose an RGB pixel format.
	
	if ((pixel_format = aglChoosePixelFormat(NULL, 0, attrib)) == NULL) {
		log("Failed to choose pixel format");
		return(false);
	}

	// Create an AGL context.
	
	if ((agl_context = aglCreateContext(pixel_format, NULL)) == NULL) {
		log("Failed to create context");
		aglDestroyPixelFormat(pixel_format);
		return(false);
	}
	
	// Attach the active window to the context.
	
	if (!aglSetDrawable(agl_context, active_graf_port_ptr)) {
		log("Failed to set drawable");
		aglDestroyContext(agl_context);
		aglDestroyPixelFormat(pixel_format);
		return(false);
	}
	
	// Make the context the current one.
	
	if (!aglSetCurrentContext(agl_context)) {
		log("Failed to set current context");
		aglSetDrawable(agl_context, NULL);
		aglDestroyContext(agl_context);
		aglDestroyPixelFormat(pixel_format);
		return(false);
	}
	
	// Create display lists for the courier font.
	
	aglUseFont(agl_context, applFont, bold, 10, 0, 128, 0);

	// Enable the swap rectangle, then initialise and set the viewport.
	
	aglEnable(agl_context, AGL_SWAP_RECT);
	init_active_viewport(np_window_ptr);
	set_viewport();
		
	// Select smooth shading model.

	glShadeModel(GL_SMOOTH);

	// Enable depth testing.

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	// Enable alpha blending.

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable modulated perspective-correct texture mapping.

	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	
	// Set the pixel zoom mode to flip 2D pixmaps vertically.
	
	glPixelZoom(1.0, -1.0);
	
	// Initialise some variables.
	
	hardware_acceleration = true;
	full_screen = false;
	curr_hardware_texture_ptr = NULL;
	active_button_index = -1;
	prev_active_button_index = -1;
	logo_active = false;
	prev_logo_active = false;
	label_visible = false;
	curr_cursor_handle = NULL;
	
	// Set the active window size.
	
	set_active_window_size(np_window_ptr->width, np_window_ptr->height);
	
	// Indicate active window is now ready.
	
	active_window_ready = true;
	return(true);
}

//------------------------------------------------------------------------------
// Set the perspective projection.
//------------------------------------------------------------------------------

void
set_perspective_projection(float vert_field_of_view, float aspect_ratio)
{
	// Set up the projection matrix to be a orthographic projection.

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(vert_field_of_view, aspect_ratio, 1.0, 256.0);
}

//------------------------------------------------------------------------------
// Send a key event to the key callback function.
//------------------------------------------------------------------------------

static void
send_key_event(int key_code, bool key_down, int modifiers)
{
	int modifier_flags;
	
	// Set the modifier flags.
	
	modifier_flags = 0;
	if (modifiers & shiftKey)
		modifier_flags |= SHIFT_MODIFIER;
	if (modifiers & controlKey)
		modifier_flags |= CONTROL_MODIFIER;
		
	// Set the key code, and call the key callback function.
	
	if (key_code >= 'A' && key_code <= 'Z')
		(*key_callback_ptr)(key_code, key_down, modifier_flags);
	else if (key_code >= 'a' && key_code <= 'z')
		(*key_callback_ptr)(toupper(key_code), key_down, modifier_flags);
	else {
		switch (key_code) {
		case kUpArrowCharCode:	
			key_code = UP_KEY;
			break;
		case kDownArrowCharCode:
			key_code = DOWN_KEY;
			break;
		case kLeftArrowCharCode:
			key_code = LEFT_KEY;
			break;
		case kRightArrowCharCode:
			key_code = RIGHT_KEY;
			break;
		case '+':
			key_code = ADD_KEY;
			break;
		case '-':
			key_code = SUBTRACT_KEY;
			break;
		default:
			key_code = 0;
		}
		if (key_code != 0)
			(*key_callback_ptr)(key_code, key_down, modifier_flags);
	}
}

//------------------------------------------------------------------------------
// Function to handle mouse events in the main window.
//------------------------------------------------------------------------------

static void
handle_mouse_event(EventKind what, int x, int y, int modifiers)
{
	bool logo_active_flag;
	int selected_button_index;
	int task_bar_button_code;
	int modifier_flags;

	// If we are inside the task bar, check whether we are pointing at one
	// of the task bar buttons or the logo, and if so make it active.

	selected_button_index = -1;
	logo_active_flag = false;
	if (task_bar_enabled && y >= window_height && y < display_height &&
		x >= 0 && x < window_width) {
		for (int index = 0; index < TASK_BAR_BUTTONS; index++)
			if (x >= button_x_list[index] && 
				x < button_x_list[index + 1]) {
				selected_button_index = index;
				break;
			}
		if (x >= logo_x && x < left_edge_x)
			logo_active_flag = true;
	}

	// Remember the logo active flag and the selected button index.  Access to
	// logo_active and active_button_index must be synchronised because both
	// the plugin and player threads use them.

	start_atomic_operation();
	logo_active = logo_active_flag;
	active_button_index = selected_button_index;
	end_atomic_operation();

	// Set the task bar button code.

	if (!logo_active_flag && selected_button_index == -1)
		task_bar_button_code = NO_TASK_BAR_BUTTON;
	else if (logo_active_flag)
		task_bar_button_code = LOGO_BUTTON;
	else
		task_bar_button_code = selected_button_index + 2;
	
	// Set the modifier flags.
	
	modifier_flags = 0;
	if (modifiers & shiftKey)
		modifier_flags |= SHIFT_MODIFIER;
	if (modifiers & controlKey)
		modifier_flags |= CONTROL_MODIFIER;
		
	// Send the mouse data to the callback function, if there is one.

	switch (what) {
	case adjustCursorEvent:
		(*mouse_callback_ptr)(x, y, MOUSE_MOVE_ONLY, task_bar_button_code,
			modifier_flags);
		break;
	case mouseDown:
		if (modifiers & optionKey)
			(*mouse_callback_ptr)(x, y, RIGHT_BUTTON_DOWN, task_bar_button_code,
				modifier_flags);
		else
			(*mouse_callback_ptr)(x, y, LEFT_BUTTON_DOWN, task_bar_button_code,
				modifier_flags);
		break;
	case mouseUp:
		if (modifiers & optionKey)
			(*mouse_callback_ptr)(x, y, RIGHT_BUTTON_UP, task_bar_button_code,
				modifier_flags);
		else
			(*mouse_callback_ptr)(x, y, LEFT_BUTTON_UP, task_bar_button_code,
				modifier_flags);
	}
}


//------------------------------------------------------------------------------
// Handle an event in the active window.
//------------------------------------------------------------------------------

int
handle_active_event(void *window_ptr, void *event_ptr)
{
	NPWindow *np_window_ptr;
	EventRecord *event_record_ptr;
	int event_handled;
	Point mouse_point;
	int key_code;

	// Initialise the viewport.
	
	np_window_ptr = (NPWindow *)window_ptr;
	init_active_viewport(np_window_ptr);
	
	// Handle the event.
	
	event_handled = FALSE;
	event_record_ptr = (EventRecord *)event_ptr;
	switch (event_record_ptr->what) {
	case nullEvent:
		if (web_browser_ID == INTERNET_EXPLORER && 
			event_record_ptr->when - last_key_down_time > 25)
			send_key_event(last_key_down_code, false, last_key_down_modifiers);		
		(*timer_callback_ptr)();
		event_handled = TRUE;
		break;
	
	case getFocusEvent:
		event_handled = TRUE;
		break;
		
	case keyDown:
		key_code = event_record_ptr->message & charCodeMask;
		last_key_down_code = key_code;
		last_key_down_time = event_record_ptr->when;
		last_key_down_modifiers = event_record_ptr->modifiers;
		send_key_event(key_code, true, event_record_ptr->modifiers);
		event_handled = TRUE;
		break;
		
	case autoKey:
		last_key_down_code = event_record_ptr->message & charCodeMask;
		last_key_down_time = event_record_ptr->when;
		last_key_down_modifiers = event_record_ptr->modifiers;
		break;
		
	case keyUp:
		key_code = event_record_ptr->message & charCodeMask;
		send_key_event(key_code, false, event_record_ptr->modifiers);
		event_handled = TRUE;
		break;
		
	case adjustCursorEvent:
	case mouseDown:
	case mouseUp:
		set_active_port();
		global_mouse_point = event_record_ptr->where;
		mouse_point = event_record_ptr->where;
		GlobalToLocal(&mouse_point);
		reset_port();
		handle_mouse_event(event_record_ptr->what, mouse_point.h, 
			mouse_point.v, event_record_ptr->modifiers);
		event_handled = TRUE;
	}	
	return(event_handled);
}

//------------------------------------------------------------------------------
// Destroy the active window.
//------------------------------------------------------------------------------

void
destroy_active_window(void)
{
	aglSetCurrentContext(NULL);
	aglSetDrawable(agl_context, NULL);
	aglDestroyContext(agl_context);
	aglDestroyPixelFormat(pixel_format);
	active_window_ready = false;
}

//------------------------------------------------------------------------------
// Determine if the browser window is currently minimised.
//------------------------------------------------------------------------------

bool
browser_window_is_minimised(void)
{
	return(false);
}

//==============================================================================
// Message functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to display a fatal error message in a message box.
//------------------------------------------------------------------------------

void
fatal_error(char *title, char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	short curr_res_file;

	// Create a message string by parsing the variable argument list according
	// to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	
	// Display a stop alert with the formatted message.
	
	curr_res_file = CurResFile();
	UseResFile(plugin_res_file);
	set_arrow_cursor();
	ParamText(pstring(message), "\p", "\p", "\p");
	StopAlert(FATAL_ERROR_ALERT, NULL);
	UseResFile(curr_res_file);
}

//------------------------------------------------------------------------------
// Function to display an informational message in a message box.
//------------------------------------------------------------------------------

void
information(char *title, char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	short curr_res_file;

	// Create a message string by parsing the variable argument list according
	// to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Display a note alert with the formatted message.
	
	curr_res_file = CurResFile();
	UseResFile(plugin_res_file);
	set_arrow_cursor();
	ParamText(pstring(message), "\p", "\p", "\p");
	NoteAlert(INFORMATION_ALERT, NULL);
	UseResFile(curr_res_file);
}

//------------------------------------------------------------------------------
// Function to display a query in a message box, and to return the response.
//------------------------------------------------------------------------------

bool
query(char *title, char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	short curr_res_file;
	short item_hit;

	// Create a message string by parsing the variable argument list according
	// to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);
	
	// Display a note alert with the formatted message.
	
	curr_res_file = CurResFile();
	UseResFile(plugin_res_file);
	set_arrow_cursor();
	ParamText(pstring(message), "\p", "\p", "\p");
	item_hit = NoteAlert(QUERY_ALERT, NULL);
	UseResFile(curr_res_file);
	
	// Return TRUE if the YES button was selected, FALSE otherwise.
	
	return(item_hit == YES_BUTTON);
}

//==============================================================================
// About window function.
//==============================================================================

void
about_window(void)
{
	short curr_res_file;
	
	curr_res_file = CurResFile();
	UseResFile(plugin_res_file);
	set_arrow_cursor();
	NoteAlert(ABOUT_ALERT, NULL);
	UseResFile(curr_res_file);
}

//==============================================================================
// Progress window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Create the progress window.
//------------------------------------------------------------------------------

void
open_progress_window(int file_size, void (*progress_callback)(void),
					 char *format, ...)
{
}

//------------------------------------------------------------------------------
// Update the progress window.
//------------------------------------------------------------------------------

void
update_progress_window(int file_pos)
{	
}

//------------------------------------------------------------------------------
// Close the progress window.
//------------------------------------------------------------------------------

void
close_progress_window(void)
{
}

//==============================================================================
// Light menu function.
//==============================================================================

float
light_menu(float brightness)
{
	int curr_light_item;
	long result;
	
	// Check the initial menu item.
	
	curr_light_item = 12 - ((int)(brightness * 5.0f) + 6);
	SetItemMark(light_menu_handle, curr_light_item, checkMark);

	// Get the menu index of the user's selection.
	
	result = PopUpMenuSelect(light_menu_handle, global_mouse_point.v, 
		global_mouse_point.h, 11) & 0x7fff;
		
	// Uncheck the initial menu item.
	
	SetItemMark(light_menu_handle, curr_light_item, noMark);

	// Return the selection as a new brightness value, otherwise return
	// the original brightness value.
	
	if (result == 0)
		return(brightness);
	else
		return((float)((12 - result) - 6) / 5.0f);
}

//==============================================================================
// Options menu.
//==============================================================================

int
options_menu(int visible_distance)
{	
	int curr_distance_item;
	long result;
	int menu_ID, item_ID;

	// Check the current distance item.
	
	curr_distance_item = visible_distance / 10;
	SetItemMark(distance_menu_handle, curr_distance_item, checkMark);
	
	// Get the menu and item ID of the user's selection.
	
	result = PopUpMenuSelect(options_menu_handle, global_mouse_point.v, 
		global_mouse_point.h, 7);
	menu_ID = result >> 16;
	item_ID = result & 0x7fff;
			
	// Uncheck the distance item.
	
	SetItemMark(distance_menu_handle, curr_distance_item, noMark);

	// Return the user's selection.
	
	switch (menu_ID) {
	case OPTIONS_MENU_ID:
		switch (item_ID) {
		case 1:
			result = ABOUT_ITEM;
			break;
		case 2:
			result = REGISTER_ITEM;
			break;
		case 3:
			result = UPGRADE_ITEM;
			break;
		case 5:
			result = VIEW_SOURCE_ITEM;
		}
		break;
	case DISTANCE_MENU_ID:
		switch (item_ID) {
		case 1:
			result = DISTANCE_10_ITEM;
			break;
		case 2:
			result = DISTANCE_20_ITEM;
			break;
		case 3:
			result = DISTANCE_30_ITEM;
			break;
		case 4:
			result = DISTANCE_40_ITEM;
			break;
		case 5:
			result = DISTANCE_50_ITEM;
			break;
		}		
	}
	return(result);
}

//==============================================================================
// Password window functions.
//==============================================================================

static char username[16], password[16];

//------------------------------------------------------------------------------
// Get a username and password.
//------------------------------------------------------------------------------

bool
get_password(string *username_ptr, string *password_ptr)
{
	return(false);
}

//==============================================================================
// Frame buffer functions.
//==============================================================================

//------------------------------------------------------------------------------
// Create the frame buffer.
//------------------------------------------------------------------------------

bool
create_frame_buffer(void)
{
	return(true);
}

//------------------------------------------------------------------------------
// Destroy the frame buffer.
//------------------------------------------------------------------------------

void
destroy_frame_buffer(void)
{
}

//------------------------------------------------------------------------------
// Lock the frame buffer.
//------------------------------------------------------------------------------

bool
lock_frame_buffer(byte *&frame_buffer_ptr, int &frame_buffer_width)
{
	set_active_port();
	return(true);
}

//------------------------------------------------------------------------------
// Unlock the frame buffer.
//------------------------------------------------------------------------------

void
unlock_frame_buffer(void)
{
}

//------------------------------------------------------------------------------
// Display the frame buffer.
//------------------------------------------------------------------------------

bool
display_frame_buffer(bool loading_spot, bool show_splash_graphic)
{	
	// Draw the task bar.
	
	draw_task_bar(loading_spot);

	// Draw the label if it's visible and we're not loading a spot.

	if (label_visible && !loading_spot)
		draw_label();

	// Draw the splash graphic if it's requested and it's enabled.

	if (show_splash_graphic && splash_graphic_enabled)
		draw_splash_graphic();

	// Reveal the OpenGL buffer just rendered to.
	
	aglSwapBuffers(agl_context);
	
	// Reset the port.
	
	reset_port();	
	return(true);
}

//------------------------------------------------------------------------------
// Method to clear a rectangle in the frame buffer.
//------------------------------------------------------------------------------

void
clear_frame_buffer(int x, int y, int width, int height)
{
}

//------------------------------------------------------------------------------
// Update logo animation every 1/10th of a second, but only if the active
// window is ready.
//------------------------------------------------------------------------------

void
update_logo(void)
{
	byte *fb_ptr;
	int row_pitch;
	
	if (active_window_ready) {
		int logo_time_ms = get_time_ms();
		if (logo_time_ms - last_logo_time_ms > 100) {
			last_logo_time_ms = logo_time_ms;
			lock_frame_buffer(fb_ptr, row_pitch);
			unlock_frame_buffer();
			display_frame_buffer(true, !seen_first_spot_URL);
		}
	}
}

//==============================================================================
// Hardware rendering functions.
//==============================================================================

//------------------------------------------------------------------------------
// Initialise the hardware vertex list.
//------------------------------------------------------------------------------

void
hardware_init_vertex_list(void)
{
}

//------------------------------------------------------------------------------
// Create the hardware vertex list.
//------------------------------------------------------------------------------

bool
hardware_create_vertex_list(int max_vertices)
{
	return(true);
}

//------------------------------------------------------------------------------
// Destroy the hardware vertex list.
//------------------------------------------------------------------------------

void
hardware_destroy_vertex_list(int max_vertices)
{
}

//------------------------------------------------------------------------------
// Create a hardware texture.
//------------------------------------------------------------------------------

void *
hardware_create_texture(int image_size_index)
{
	hardware_texture *hardware_texture_ptr;
		
	// Create the hardware texture object, and initialise it.
	
	NEW(hardware_texture_ptr, hardware_texture);
	if (hardware_texture_ptr == NULL)
		return(NULL);
	hardware_texture_ptr->image_size_index = image_size_index;
	hardware_texture_ptr->set = false;

	// Create the OpenGL texture.

	glGenTextures(1, &hardware_texture_ptr->texture_ref_no);
	glBindTexture(GL_TEXTURE_2D, hardware_texture_ptr->texture_ref_no);

	// Set the texture wrap and filter modes.

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Return a pointer to the hardware texture.

	return(hardware_texture_ptr);
}

//------------------------------------------------------------------------------
// Destroy an existing hardware texture.
//------------------------------------------------------------------------------

void
hardware_destroy_texture(void *hardware_texture_ptr)
{
	glDeleteTextures(1, &((hardware_texture *)hardware_texture_ptr)->texture_ref_no);
	DEL((hardware_texture *)hardware_texture_ptr, hardware_texture);
}

//------------------------------------------------------------------------------
// Set the image of a Direct3D texture.
//------------------------------------------------------------------------------

void
hardware_set_texture(cache_entry *cache_entry_ptr)
{
	hardware_texture *hardware_texture_ptr;
	int image_size_index, image_dimensions;
	pixmap *pixmap_ptr;
	RGBcolour *RGB_palette, *colour_ptr;
	int transparent_index;
	byte *texture_image_ptr, *image_row_ptr;
	int pixmap_row, pixmap_column;
	int row, column;
	
	// Get a pointer to the hardware texture object.

	hardware_texture_ptr = 
		(hardware_texture *)cache_entry_ptr->hardware_texture_ptr;

	// Get the image size index and dimensions.

	image_size_index = hardware_texture_ptr->image_size_index;
	image_dimensions = image_dimensions_list[image_size_index];
	
	// Create an RGBA texture image for this pixmap.  Note that the texture
	// image dimensions must be a power of two, where as the pixmap may have
	// arbitrary dimensions; in this case, the pixmap is tiled across and down
	// the texture image.

	pixmap_ptr = cache_entry_ptr->pixmap_ptr;
	texture_image_ptr = (byte *)texture_image;
	pixmap_row = 0;
	if (pixmap_ptr->image32_ptr) {
		for (row = 0; row < image_dimensions; row++) {
			pixmap_column = 0;
			image_row_ptr = pixmap_ptr->image32_ptr + 
				pixmap_row * pixmap_ptr->width * 4;
			for (column = 0; column < image_dimensions; column++) {
				*(pixel *)texture_image_ptr = *(pixel *)image_row_ptr;
				texture_image_ptr += 4;
				image_row_ptr += 4;
				pixmap_column++;
				if (pixmap_column >= pixmap_ptr->width) {
					pixmap_column = 0;
					image_row_ptr = pixmap_ptr->image32_ptr + 
						pixmap_row * pixmap_ptr->width * 4;
				}
			}
			pixmap_row++;
			if (pixmap_row >= pixmap_ptr->height)
				pixmap_row = 0;
		}
	} else {
		RGB_palette = pixmap_ptr->RGB_palette;
		transparent_index = pixmap_ptr->transparent_index;
		for (row = 0; row < image_dimensions; row++) {
			image_row_ptr = pixmap_ptr->image8_ptr + pixmap_row * pixmap_ptr->width;
			pixmap_column = 0;
			for (column = 0; column < image_dimensions; column++) {
				byte pixel = image_row_ptr[pixmap_column];
				colour_ptr = &RGB_palette[pixel];
				*texture_image_ptr++ = colour_ptr->red;
				*texture_image_ptr++ = colour_ptr->green;
				*texture_image_ptr++ = colour_ptr->blue;
				*texture_image_ptr++ = ((int)pixel == transparent_index) ? 0 : 255;
				pixmap_column++;
				if (pixmap_column >= pixmap_ptr->width)
					pixmap_column = 0;
			}
			pixmap_row++;
			if (pixmap_row >= pixmap_ptr->height)
				pixmap_row = 0;
		}
	}

	// Set the pixel store options so that the image will be clipped to the 
	// viewport as per the above calculations.

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pixmap_ptr->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	// Bind and initialise the OpenGL texture with this texture image, storing
	// it as a 5551 RGBA internal texture format, if possible.

	glBindTexture(GL_TEXTURE_2D, hardware_texture_ptr->texture_ref_no);
	if (hardware_texture_ptr->set)
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_dimensions, 
			image_dimensions, GL_RGBA, GL_UNSIGNED_BYTE, texture_image);
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, image_dimensions, 
			image_dimensions, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_image);
		hardware_texture_ptr->set = true;
	}
}

//------------------------------------------------------------------------------
// Enable a texture for rendering.
//------------------------------------------------------------------------------

static void
hardware_enable_texture(void *hardware_texture_ptr)
{	
	hardware_texture *this_hardware_texture_ptr =
		(hardware_texture *)hardware_texture_ptr;
	if (this_hardware_texture_ptr != curr_hardware_texture_ptr) {
		curr_hardware_texture_ptr = this_hardware_texture_ptr;
		glBindTexture(GL_TEXTURE_2D, curr_hardware_texture_ptr->texture_ref_no);
		glEnable(GL_TEXTURE_2D);
	}
}

//------------------------------------------------------------------------------
// Disable texture rendering.
//------------------------------------------------------------------------------

static void
hardware_disable_texture(void)
{
	if (curr_hardware_texture_ptr) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		curr_hardware_texture_ptr = NULL;
	}
}

//------------------------------------------------------------------------------
// Render a 2D polygon onto the viewport.
//------------------------------------------------------------------------------

void
hardware_render_2D_polygon(pixmap *pixmap_ptr, RGBcolour colour,
						   float brightness, float x, float y, float width,
						   float height, float start_u, float start_v,
						   float end_u, float end_v, bool disable_transparency)
{
	// Switch to an orthographic projection.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();	
	glLoadIdentity();
	glOrtho(0.0, window_width, window_height, 0.0, 1.0, 128.0);
	
	// Turn off alpha blending if disable_transparency is TRUE, and set the depth
	// function to pass all pixels through regardless of depth; this will
	// initialise the depth buffer.

	if (disable_transparency)
		glDisable(GL_BLEND);
	glDepthFunc(GL_ALWAYS);
	
	// If the polygon has a pixmap...
	
	if (pixmap_ptr) {
	
		// Get the cache entry and enable the OpenGL texture.
		
		cache_entry *cache_entry_ptr = get_cache_entry(pixmap_ptr, 0);
		hardware_enable_texture(cache_entry_ptr->hardware_texture_ptr);
		
		// Draw a 2D polygon that fills the viewport and has a depth of z = 127,
		// just before the far clipping plane.

		glBegin(GL_POLYGON);
		glColor4f(brightness, brightness, brightness, 1.0);
		glTexCoord2f(start_u, start_v); 
		glVertex3f(x, y, -127.0f);
		glTexCoord2f(end_u, start_v);
		glVertex3f(x + width, y, -127.0f);
		glTexCoord2f(end_u, end_v);
		glVertex3f(x + width, y + height, -127.0f);
		glTexCoord2f(start_u, end_v);
		glVertex3f(x, y + height, -127.0f);
		glEnd();
	}
	
	// If the polygon has a colour.
	
	else {
	
		// Disable the OpenGL texture.
		
		hardware_disable_texture();
		
		// Render the rectangle as a solid opaque colour.

		glBegin(GL_POLYGON);
		glColor4f(colour.red / 255.0f, colour.green / 255.0f, colour.blue / 255.0f, 1.0f);
		glVertex3f(x, y, -127.0f);
		glVertex3f(x + width, y, -127.0f);
		glVertex3f(x + width, y + height, -127.0f);
		glVertex3f(x, y + height, -127.0f);
		glEnd();

	}

	// Re-enable alpha blending if it was disabled, and reset the depth buffer
	// to only pass through pixels that are closer.

	if (disable_transparency)
		glEnable(GL_BLEND);
	glDepthFunc(GL_LESS);

	// Return to the perspective projection.
		
	glPopMatrix();
}

//------------------------------------------------------------------------------
// Render a polygon onto the viewport.
//------------------------------------------------------------------------------

void
hardware_render_polygon(spolygon *spolygon_ptr)
{
	pixmap *pixmap_ptr;
	int index;
	
	// If the polygon has a pixmap...
	
	pixmap_ptr = spolygon_ptr->pixmap_ptr;
	if (pixmap_ptr) {
	
		// Get the cache entry and enable the OpenGL texture.
		
		cache_entry *cache_entry_ptr = get_cache_entry(pixmap_ptr, 0);
		hardware_enable_texture(cache_entry_ptr->hardware_texture_ptr);
		
		// Render the polygon as a triangle fan.
		
		glBegin(GL_TRIANGLE_FAN);
		for (index = 0; index < spolygon_ptr->spoints; index++) {
			spoint *spoint_ptr = &spolygon_ptr->spoint_list[index];
			glColor4f(spoint_ptr->colour.red, spoint_ptr->colour.green,
				spoint_ptr->colour.blue, spolygon_ptr->alpha);
			glTexCoord2f(spoint_ptr->u, spoint_ptr->v);
			glVertex3f(spoint_ptr->tx, spoint_ptr->ty, -spoint_ptr->tz);
		}
		glEnd();
	} 
	
	// If the polygon has a colour...
	
	else {
	
		// Disable the OpenGL texture.
		
		hardware_disable_texture();
		
		// Render the polygon as a triangle fan.
		
		glBegin(GL_TRIANGLE_FAN);
		for (index = 0; index < spolygon_ptr->spoints; index++) {
			spoint *spoint_ptr = &spolygon_ptr->spoint_list[index];
			glColor4f(spoint_ptr->colour.red, spoint_ptr->colour.green,
				spoint_ptr->colour.blue, spolygon_ptr->alpha);
			glVertex3f(spoint_ptr->tx, spoint_ptr->ty, -spoint_ptr->tz);
		}
		glEnd();
	}
}

//==============================================================================
// 2D drawing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to draw a pixmap at the given brightness index onto the frame buffer
// surface at the given (x,y) coordinates.
//------------------------------------------------------------------------------

void
draw_pixmap(pixmap *pixmap_ptr, int brightness_index, int x, int y)
{
	int clipped_width, clipped_height, left_offset, bottom_offset;

	// If the image x or y coordinates are negative, then we clamp them
	// at zero and adjust the image offsets and size to match.

	if (x < 0) {
		left_offset = -x;
		clipped_width = pixmap_ptr->width - left_offset;
		x = 0;
	} else {
		left_offset = 0;
		clipped_width = pixmap_ptr->width;
	}	
	if (y < 0) {
		bottom_offset = -y;
		clipped_height = pixmap_ptr->height - bottom_offset;
		y = 0;
	} else {
		bottom_offset = 0;
		clipped_height = pixmap_ptr->height;
	}

	// If the bitmap crosses the right or bottom edge of the viewport, we must
	// adjust the size of the area we are going to draw even further.

	if (x + clipped_width > window_width)
		clipped_width = window_width - x;
	if (y + clipped_height > window_height)
		clipped_height = window_height - y;

	// Adjust the image y coordinate to reflect a y axis whose origin is at
	// the bottom of the screen, and whose positive direction is up.

	y = window_height - y - 1;

	// Set the pixel store options so that the image will be clipped to the 
	// viewport as per the above calculations.

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pixmap_ptr->width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, left_offset);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, bottom_offset);

	// Temporarily switch to an orthographic projection whose clipping
	// planes match the size of the display window.

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, window_width, 0.0, window_height);

	// Disable texture mapping and depth buffering.  If the image is not
	// transparent, also disable alpha blending.

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	if (pixmap_ptr->transparent_index == -1)
		glDisable(GL_BLEND);
		
	// Set the raster position and draw the 32-bit image.

	glRasterPos2i(x, y);
	glDrawPixels(clipped_width, clipped_height, GL_RGBA, 
		GL_UNSIGNED_BYTE, pixmap_ptr->image32_ptr);		

	// Switch back to the perspective projection, and re-enable texture
	// mapping, depth buffering and alpha blending, if necessary.

	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	if (pixmap_ptr->transparent_index == -1)
		glEnable(GL_BLEND);
}

//------------------------------------------------------------------------------
// Convert an RGB colour to a display pixel.
//------------------------------------------------------------------------------

pixel
RGB_to_display_pixel(RGBcolour colour)
{
	pixel red, green, blue;

	red = ((pixel)colour.red & 0xff) << 24;
	green = ((pixel)colour.green & 0xff) << 16;
	blue = ((pixel)colour.blue & 0xff) << 8;
	return(red | green | blue);
}

//------------------------------------------------------------------------------
// Convert an RGB colour to a texture pixel.
//------------------------------------------------------------------------------

pixel
RGB_to_texture_pixel(RGBcolour colour)
{
	pixel red, green, blue;

	red = ((pixel)colour.red & 0xff) << 24;
	green = ((pixel)colour.green & 0xff) << 16;
	blue = ((pixel)colour.blue & 0xff) << 8;
	return(red | green | blue);
}

//------------------------------------------------------------------------------
// Return a pointer to the standard RGB palette.
//------------------------------------------------------------------------------

RGBcolour *
get_standard_RGB_palette(void)
{
	return(NULL);
}

//------------------------------------------------------------------------------
// Return an index to the nearest colour in the standard palette.
//------------------------------------------------------------------------------

byte
get_standard_palette_index(RGBcolour colour)
{
	return(0);
}

//------------------------------------------------------------------------------
// Set the title.
//------------------------------------------------------------------------------

void
set_title(char *format, ...)
{
	va_list arg_ptr;
	char title[BUFSIZ];

	va_start(arg_ptr, format);
	vsprintf(title, format, arg_ptr);
	va_end(arg_ptr);
	title_text = title;
}

//------------------------------------------------------------------------------
// Display a label near the current cursor position.
//------------------------------------------------------------------------------

void
show_label(const char *label)
{
	label_visible = true;
	label_text = label;
}

//------------------------------------------------------------------------------
// Hide the label.
//------------------------------------------------------------------------------

void
hide_label(void)
{
	label_visible = false;
}

//------------------------------------------------------------------------------
// Initialise a popup.
//------------------------------------------------------------------------------

void
init_popup(popup *popup_ptr)
{
	pixmap *fg_pixmap_ptr;
	texture *fg_texture_ptr;
	texture *bg_texture_ptr;
	int popup_width, popup_height;
	Rect fg_rect;
	GWorldPtr gworld_ptr;
	CGrafPtr old_graf_port_ptr;
	GDHandle old_device_handle;
	RGBColor colour;
	pixel argb_pixel;
	PixMapHandle pixmap_handle;
	imagebyte *image_ptr, *new_image_ptr;
	int skip_bytes;
	int row, col, x_offset, y_offset;
	StyledLineBreakCode code;
	char *text_ptr, *end_text_ptr;
	int text_length, text_width;
	Fixed row_width;
	long text_offset;
	
	// If this popup has a background texture...
	
	if ((bg_texture_ptr = popup_ptr->bg_texture_ptr) != NULL) {
		int pixmap_no;
		pixmap *pixmap_ptr;
		RGBcolour *RGB_palette;
	
		// The size of the foreground texture will be the same as the
		// background texture.
		
		popup_width = bg_texture_ptr->width;
		popup_height = bg_texture_ptr->height;
		
		// Step through the pixmap list of the background texture...
		
		for (pixmap_no = 0; pixmap_no < bg_texture_ptr->pixmaps;
			pixmap_no++) {
			pixmap_ptr = &bg_texture_ptr->pixmap_list[pixmap_no];
			
			// If this pixmap does not have a 32-bit image...
			
			if (pixmap_ptr->image32_ptr == NULL) {
			
				// Create a new 32-bit image buffer.
				
				NEWARRAY(pixmap_ptr->image32_ptr, imagebyte,
					popup_width * popup_height * 4);
					
				// Copy the 8-bit image to the 32-bit image buffer, using
				// the approapiate pixel conversion.
				
				RGB_palette = pixmap_ptr->RGB_palette;
				image_ptr = pixmap_ptr->image8_ptr;
				new_image_ptr = pixmap_ptr->image32_ptr;
				for (row = 0; row < pixmap_ptr->height; row++)
					for (col = 0; col < pixmap_ptr->width; col++) {
						if (*image_ptr == pixmap_ptr->transparent_index)
							*(pixel *)new_image_ptr = 0;
						else
							*(pixel *)new_image_ptr = 0xff |
								RGB_to_display_pixel(RGB_palette[*image_ptr]);
						image_ptr++;
						new_image_ptr += 4;
					}
			}
		}
	} 
	
	// If this popup does not have a background texture, use the popup's
	// default size for the foreground texture.
	
	else {
		popup_width = popup_ptr->width;
		popup_height = popup_ptr->height;
	}
				
	// If this popup has no foreground texture, then there isn't anything
	// else to do.
	
	if ((fg_texture_ptr = popup_ptr->fg_texture_ptr) == NULL)
		return;	
	
	// Create the foreground pixmap and initialise it with a 32-bit
	// image buffer.

	NEWARRAY(fg_pixmap_ptr, pixmap, 1);
	if (fg_pixmap_ptr == NULL)
		memory_error("popup foreground pixmap");
	NEWARRAY(fg_pixmap_ptr->image32_ptr, imagebyte, 
		popup_width * popup_height * 4);
	if (fg_pixmap_ptr->image32_ptr == NULL)
		memory_error("popup foreground image");
	fg_pixmap_ptr->width = popup_width;
	fg_pixmap_ptr->height = popup_height;
	fg_pixmap_ptr->transparent_index = 0;

	// Initialise the foreground texture.

	fg_texture_ptr->needs_palette = false;
	fg_texture_ptr->transparent = popup_ptr->transparent_background;
	fg_texture_ptr->width = popup_width;
	fg_texture_ptr->height = popup_height;
	fg_texture_ptr->pixmaps = 1;
	fg_texture_ptr->pixmap_list = fg_pixmap_ptr;
	
	// Create a off-screen graphics world for the foreground texture.
	
	fg_rect.left = 0;
	fg_rect.top = 0;
	fg_rect.right = popup_width;
	fg_rect.bottom = popup_height;
	NewGWorld(&gworld_ptr, 32, &fg_rect, NULL, NULL, useTempMem);
	
	// Save the current GrafPort and device handle, then make the
	// off-screen graphics world the current GrafPort.
	
	GetGWorld(&old_graf_port_ptr, &old_device_handle);
	SetGWorld(gworld_ptr, NULL);
	
	// Get a pointer to the off-screen graphics world's pixmap, and calculate
	// the number of bytes to skip at the end of each row.
	
	pixmap_handle = GetGWorldPixMap(gworld_ptr);
	image_ptr = (imagebyte *)GetPixBaseAddr(pixmap_handle);
	skip_bytes = ((*pixmap_handle)->rowBytes & 0x7fff) - popup_width * 4;
	
	// Initialise the off-screen graphics world with either the popup colour
	// or the transparent colour.  For now the pixel format is ARGB and the
	// alpha channel is inverted ($ffrrbbgg = transparent, 
	// $00rrggbb = opaque).

	if (bg_texture_ptr || popup_ptr->transparent_background)
		argb_pixel = 0xff000000;
	else
		argb_pixel = RGB_to_display_pixel(popup_ptr->colour) >> 8;
	for (row = 0; row < popup_height; row++) {
		for (col = 0; col < popup_width; col++) {
			*(pixel *)image_ptr = argb_pixel;
			image_ptr += 4;
		}
		image_ptr += skip_bytes;
	}
	
	// Set the characteristics of the text font.
	
	TextFont(applFont);
	TextFace(bold);
	TextSize(10);
	
	// Determine the number of lines of text to be drawn.
	
	row = 0;
	text_ptr = popup_ptr->text;
	end_text_ptr = text_ptr + strlen(popup_ptr->text);
	do {
		row += 10;
		text_length = strlen(text_ptr);
		text_offset = text_length;
		row_width = popup_width << 16;
		code = StyledLineBreak(text_ptr, text_length, 0, text_length, 0,
			&row_width, &text_offset);
		text_ptr += text_offset;
	} while (code != smBreakOverflow && row < popup_height &&
		text_ptr < end_text_ptr);
		
	// Calculate the y offset according to the text alignment requested.	
	
 	switch (popup_ptr->text_alignment) {
	case TOP_LEFT:
	case TOP:
	case TOP_RIGHT:
		y_offset = 0;
		break;
	case LEFT:
	case CENTRE:
	case RIGHT:
		y_offset = (popup_height - row) / 2;
		break;
	case BOTTOM_LEFT:
	case BOTTOM:
	case BOTTOM_RIGHT:
		y_offset = popup_height - row - 1;
	}
	
	// Draw each line of text twice; once in the background colour, once in
	// the foreground colour offset to the right by one pixel.
	
	row = y_offset;
	text_ptr = popup_ptr->text;
	end_text_ptr = text_ptr + strlen(popup_ptr->text);
	do {
	
		// Calculate the line break.
		
		row += 10;
		text_length = strlen(text_ptr);
		text_offset = text_length;
		row_width = popup_width << 16;
		code = StyledLineBreak(text_ptr, text_length, 0, text_length, 0,
			&row_width, &text_offset);
			
		// Calculate the text width, and from this the x offset
		// according to the text alignment requested.
		
		text_width = TextWidth(text_ptr, 0, text_offset);
		switch (popup_ptr->text_alignment) {
		case TOP_LEFT:
		case LEFT:
		case BOTTOM_LEFT:
			x_offset = 0;
			break;
		case TOP:
		case CENTRE:
		case BOTTOM:
			x_offset = (popup_width - text_width) / 2;
			break;
		case TOP_RIGHT:
		case RIGHT:
		case BOTTOM_RIGHT:
			x_offset = popup_width - text_width - 1;
		}
		
		// Draw the line in the background colour first, then the
		// text colour shifted one pixel to the right.
		
		colour.red = (short)popup_ptr->colour.red << 8;
		colour.green = (short)popup_ptr->colour.green << 8;
		colour.blue = (short)popup_ptr->colour.blue << 8;
		RGBForeColor(&colour);
		MoveTo(x_offset, row);
		DrawText(text_ptr, 0, text_offset);
		colour.red = (short)popup_ptr->text_colour.red << 8;
		colour.green = (short)popup_ptr->text_colour.green << 8;
		colour.blue = (short)popup_ptr->text_colour.blue << 8;
		RGBForeColor(&colour);
		MoveTo(x_offset + 1, row);
		DrawText(text_ptr, 0, text_offset);
		
		// Move to the start of the next line of text.
		
		text_ptr += text_offset;
	} while (code != smBreakOverflow && row < popup_height &&
		text_ptr < end_text_ptr);
	
	// Copy the off-screen graphics world to the foreground texture,
	// converting the ARGB pixels to RGBA pixels and inverting the alpha
	// channel again.
	
	image_ptr = (imagebyte *)GetPixBaseAddr(pixmap_handle);
	new_image_ptr = fg_pixmap_ptr->image32_ptr;
	for (row = 0; row < popup_height; row++) {
		for (col = 0; col < popup_width; col++) {
			argb_pixel = *(pixel *)image_ptr;
			if (argb_pixel & 0xff000000)
				*(pixel *)new_image_ptr = argb_pixel << 8;
			else
				*(pixel *)new_image_ptr = (argb_pixel << 8) | 0xff;
			image_ptr += 4;
			new_image_ptr += 4;
		}
		image_ptr += skip_bytes;
	}
		
	// Recover the old GrafPort and device handle, then dispose of the
	// off-screen graphics world.
	
	SetGWorld(old_graf_port_ptr, old_device_handle);
	DisposeGWorld(gworld_ptr);
}

//------------------------------------------------------------------------------
// Get the relative or absolute position of the mouse.
//------------------------------------------------------------------------------

void
get_mouse_position(int *x, int *y, bool relative)
{
}

//------------------------------------------------------------------------------
// Set the arrow cursor.
//------------------------------------------------------------------------------

void
set_arrow_cursor(void)
{
	curr_cursor_handle = NULL;
	InitCursor();
}

//------------------------------------------------------------------------------
// Set a movement cursor.
//------------------------------------------------------------------------------

void
set_movement_cursor(arrow movement_arrow)
{
	curr_cursor_handle = arrow_cursor_handle_list[movement_arrow];
	SetCursor(*curr_cursor_handle);
}

//------------------------------------------------------------------------------
// Set the hand cursor.
//------------------------------------------------------------------------------

void
set_hand_cursor(void)
{
	curr_cursor_handle = hand_cursor_handle;
	SetCursor(*curr_cursor_handle);
}

//------------------------------------------------------------------------------
// Set the crosshair cursor.
//------------------------------------------------------------------------------

void
set_crosshair_cursor(void)
{
	curr_cursor_handle = crosshair_cursor_handle;
	SetCursor(*curr_cursor_handle);	
}

//------------------------------------------------------------------------------
// Capture the mouse.
//------------------------------------------------------------------------------

void
capture_mouse(void)
{
}

//------------------------------------------------------------------------------
// Release the mouse.
//------------------------------------------------------------------------------

void
release_mouse(void)
{
}

//------------------------------------------------------------------------------
// Get the time since Windows last started, in milliseconds.
//------------------------------------------------------------------------------

int
get_time_ms(void)
{
	return((int)(((float)clock() / CLOCKS_PER_SEC) * 1000.0f));
}

//------------------------------------------------------------------------------
// Load wave data into a wave object.
//------------------------------------------------------------------------------

bool
load_wave_data(wave *wave_ptr, char *wave_file_buffer, int wave_file_size)
{
	return(true);
}

//------------------------------------------------------------------------------
// Load a wave file into a wave object.
//------------------------------------------------------------------------------

bool
load_wave_file(wave *wave_ptr, const char *wave_URL, const char *wave_file_path)
{
	return(false);
}

//------------------------------------------------------------------------------
// Write the specified wave data to the given sound buffer, starting from the
// given write position.
//------------------------------------------------------------------------------

void
update_sound_buffer(void *sound_buffer_ptr, char *data_ptr, int data_size,
				   int data_start)
{
}

//------------------------------------------------------------------------------
// Create a sound buffer for the given sound.
//------------------------------------------------------------------------------

bool
create_sound_buffer(sound *sound_ptr)
{
	return(true);
}

//------------------------------------------------------------------------------
// Destroy a sound buffer.
//------------------------------------------------------------------------------

void
destroy_sound_buffer(sound *sound_ptr)
{
}

//------------------------------------------------------------------------------
// Reset the audio system.
//------------------------------------------------------------------------------

void
reset_audio(void)
{
}

//------------------------------------------------------------------------------
// Create an audio block.
//------------------------------------------------------------------------------

void *
create_audio_block(int min_column, int min_row, int min_level, 
				   int max_column, int max_row, int max_level)
{
	return(NULL);
}

//------------------------------------------------------------------------------
// Destroy an audio block.
//------------------------------------------------------------------------------

void
destroy_audio_block(void *audio_block_ptr)
{
}

//------------------------------------------------------------------------------
// Set the volume of a sound buffer.
//------------------------------------------------------------------------------

void
set_sound_volume(sound *sound_ptr, float volume)
{

}

//------------------------------------------------------------------------------
// Play a sound.
//------------------------------------------------------------------------------

void
play_sound(sound *sound_ptr, bool looped)
{
}

//------------------------------------------------------------------------------
// Stop a sound from playing.
//------------------------------------------------------------------------------

void
stop_sound(sound *sound_ptr)
{
}

//------------------------------------------------------------------------------
// Begin a sound update.
//------------------------------------------------------------------------------

void
begin_sound_update(void)
{
}

//------------------------------------------------------------------------------
// Update a sound.
//------------------------------------------------------------------------------

void
update_sound(sound *sound_ptr)
{
}

//------------------------------------------------------------------------------
// End a sound update.
//------------------------------------------------------------------------------

void
end_sound_update(void)
{
}

//------------------------------------------------------------------------------
// Initialise all video textures for the given unscaled video dimensions.
//------------------------------------------------------------------------------

void
init_video_textures(int video_width, int video_height, int pixel_format)
{
}

//------------------------------------------------------------------------------
// Determine if the stream is ready.
//------------------------------------------------------------------------------

bool
stream_ready(void)
{
	return(false);
}

//------------------------------------------------------------------------------
// Determine if download of RealPlayer was requested.
//------------------------------------------------------------------------------

bool
download_of_rp_requested(void)
{
	return(false);
}

//------------------------------------------------------------------------------
// Determine if download of Windows Media Player was requested.
//------------------------------------------------------------------------------

bool
download_of_wmp_requested(void)
{
	return(false);
}

//------------------------------------------------------------------------------
// Start the streaming thread.
//------------------------------------------------------------------------------

void
start_streaming_thread(void)
{
}

//------------------------------------------------------------------------------
// Stop the streaming thread.
//------------------------------------------------------------------------------

void
stop_streaming_thread(void)
{
}
