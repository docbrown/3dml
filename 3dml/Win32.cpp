//******************************************************************************
// $Header$
//
// The contents of this file are subject to the Flatland Public License 
// Version 1.1 (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at 
// http://www.3dml.org/FPL/ 
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
// the specific language governing rights and limitations under the License. 
//
// The Original Code is Rover. 
//
// The Initial Developer of the Original Code is Flatland Online, Inc. 
// Portions created by Flatland are Copyright (C) 1998-2000 Flatland
// Online Inc. All Rights Reserved. 
//
// Contributor(s): Philip Stephens.
//******************************************************************************

#define STRICT
#define INITGUID
#define D3D_OVERLOADS
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <process.h>
#include <pntypes.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <objbase.h>
#include <cguid.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>
#include <ddraw.h>
#include <d3d.h>
#include "Classes.h"
#include "Image.h"
#include "Main.h"
#include "Memory.h"
#include "Parser.h"
#include "Platform.h"
#include "Plugin.h"
#include "Real.h"
#include "Render.h"
#include "Spans.h"
#include "Utils.h"
#include "resource.h"
#include "amdlib.h"

#ifdef SUPPORT_A3D
#include "ia3dapi.h"
#include "ia3dutil.h"
#else
#include <dsound.h>
#endif

//==============================================================================
// Global definitions.
//==============================================================================

//------------------------------------------------------------------------------
// Event class.
//------------------------------------------------------------------------------

// Default constructor initialises event handle and value.

event::event()
{
	event_handle = NULL;
	event_value = false;
}

// Default destructor closes event handle.

event::~event()
{
	if (event_handle)
		CloseHandle(event_handle);
}

// Method to create the event handle.

void
event::create_event(void)
{
	event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
}

// Method to destroy the event handle.

void
event::destroy_event(void)
{
	if (event_handle) {
		CloseHandle(event_handle);
		event_handle = NULL;
	}
}

// Method to send an event.

void
event::send_event(bool value)
{
	event_value = value;
	SetEvent(event_handle);
}

// Method to reset an event.

void
event::reset_event(void)
{
	ResetEvent(event_handle);
	event_value = false;
}

// Method to check if an event has been sent.

bool
event::event_sent(void)
{
	return(WaitForSingleObject(event_handle, 0) != WAIT_TIMEOUT);
}

// Method to wait for an event.

bool
event::wait_for_event(void)
{
	WaitForSingleObject(event_handle, INFINITE);
	return(event_value);
}

//------------------------------------------------------------------------------
// Global variables.
//------------------------------------------------------------------------------

// Operating system name and version.

string os_version;

// Application directory.

string app_dir;

// Display, texture and video pixel formats.

pixel_format display_pixel_format;
pixel_format texture_pixel_format;

// Display properties.

int display_width, display_height, display_depth;
int window_width, window_height;
int render_mode;

// Flag indicating whether the main window is ready.

bool main_window_ready;

// Flag indicating whether sound is enabled, the sound system being used,
// and whether reflections are available (for A3D only).

bool sound_on;
int sound_system;
bool reflections_available;

//==============================================================================
// Local definitions.
//==============================================================================

//------------------------------------------------------------------------------
// Bitmap class.
//------------------------------------------------------------------------------

// Bitmap class definition.

struct bitmap {
	void *handle;						// Bitmap handle.
	byte *pixels;						// Pointer to bitmap pixel data.
	int width, height;					// Size of bitmap.
	int bytes_per_row;					// Number of bytes per row.
	int colours;						// Number of colours in palette.
	RGBcolour *RGB_palette;				// Palette in RGB format.
	pixel *palette;						// Palette in pixel format.
	int transparent_index;				// Transparent pixel index.

	bitmap();
	~bitmap();
};

// Default constructor initialises the bitmap handle.

bitmap::bitmap()
{
	handle = NULL;
}

// Default destructor deletes the bitmap, if it exists.

bitmap::~bitmap()
{
	if (handle)
		DeleteBitmap(handle);
}

//------------------------------------------------------------------------------
// Cursor class.
//------------------------------------------------------------------------------

// Cursor class definition.

struct cursor {
	HCURSOR handle;
	bitmap *mask_bitmap_ptr;
	bitmap *image_bitmap_ptr;
	int hotspot_x, hotspot_y;

	cursor();
	~cursor();
};

// Default constructor initialises the bitmap pointers.

cursor::cursor()
{
	mask_bitmap_ptr = NULL;
	image_bitmap_ptr = NULL;
}

// Default destructor deletes the bitmaps, if they exist.

cursor::~cursor()
{
	if (mask_bitmap_ptr)
		DEL(mask_bitmap_ptr, bitmap);
	if (image_bitmap_ptr)
		DEL(image_bitmap_ptr, bitmap);
}

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
// Miscellaneous classes.
//------------------------------------------------------------------------------

// Hardware texture class.

struct hardware_texture {
	int image_size_index;
	LPDIRECTDRAWSURFACE ddraw_texture_ptr;
	LPDIRECT3DTEXTURE2 d3d_texture_ptr;
	D3DTEXTUREHANDLE d3d_texture_handle;
};

// Structure to hold the colour palette.

struct MYLOGPALETTE {
	WORD palVersion;
	WORD palNumEntries; 
    PALETTEENTRY palPalEntry[256];
};

// Structure to hold bitmap info for a DIB section.

struct MYBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
};

//------------------------------------------------------------------------------
// Local variables.
//------------------------------------------------------------------------------

// Width and height factors, used to scale mouse coordinates in full-screen
// mode.

static float width_factor;
static float height_factor;

// Pixel mask table for component sizes of 1 to 8 bits.

static int pixel_mask_table[8] = {
	0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF
};

// Dither tables.

static byte *dither_table[4];
static byte *dither00, *dither01, *dither10, *dither11;

// Lighting tables.

static pixel *light_table[BRIGHTNESS_LEVELS];

// The standard colour palette in both RGB and pixel format, the table used
// to get the index of a colour in the 6x6x6 colour cube, and the index of
// the standard transparent pixel.

static HPALETTE standard_palette_handle;
static RGBcolour standard_RGB_palette[256];
static pixel standard_palette[256];
static byte colour_index[216];
static byte standard_transparent_index;

// Flag indicating whether a label is currently visible, and tbe label text.

static bool label_visible;
static const char *label_text;

// Title text.

static string title_text;

// Movement cursor IDs.

#define MOVEMENT_CURSORS	8
static int movement_cursor_ID_list[MOVEMENT_CURSORS] = { 
	IDC_ARROW_N,	IDC_ARROW_NE,	IDC_ARROW_E,	IDC_ARROW_SE,
	IDC_ARROW_S,	IDC_ARROW_SW,	IDC_ARROW_W,	IDC_ARROW_NW
};

// Handles to available cursors.

static HCURSOR movement_cursor_handle_list[MOVEMENT_CURSORS];
static HCURSOR hand_cursor_handle;
static HCURSOR arrow_cursor_handle;
static HCURSOR crosshair_cursor_handle;

// Pointers to available cursors.

static cursor *movement_cursor_ptr_list[MOVEMENT_CURSORS];
static cursor *hand_cursor_ptr;
static cursor *arrow_cursor_ptr;
static cursor *crosshair_cursor_ptr;

// Pointer to current cursor.

static cursor *curr_cursor_ptr;

// Private display data.

static LPDIRECTDRAW ddraw_object_ptr;
static LPGUID ddraw_guid_ptr;
static GUID ddraw_guid;
static LPDIRECTDRAWSURFACE primary_surface_ptr;
static LPDIRECTDRAWCLIPPER clipper_ptr;
static LPDIRECTDRAWSURFACE framebuf_surface_ptr;
static byte *framebuf_ptr;
static int framebuf_width;
static LPDIRECT3D2 d3d_object_ptr;
static LPDIRECT3DDEVICE2 d3d_device_ptr;
static LPDIRECT3DVIEWPORT2 d3d_viewport_ptr;
static LPDIRECTDRAWSURFACE zbuffer_surface_ptr;
static D3DTLVERTEX *d3d_vertex_list;
static hardware_texture *curr_hardware_texture_ptr;
static DDPIXELFORMAT ddraw_texture_pixel_format;
static LPDIRECTDRAWSURFACE ddraw_texture_list[IMAGE_SIZES];
static LPDIRECT3DTEXTURE2 d3d_texture_list[IMAGE_SIZES];
static int AGP_flag;

// Private sound data.

#ifdef SUPPORT_A3D
static LPA3D4 a3d_object_ptr;
static LPA3DGEOM a3d_geometry_ptr;
static LPA3DLISTENER a3d_listener_ptr;
static LPA3DMATERIAL a3d_material_ptr;
static int curr_audio_polygon_ID;
static int prev_audio_column;
static int prev_audio_row;
static int prev_audio_level;
static bool bind_sound_sources;
static bool reflections_on;
#else
static LPDIRECTSOUND dsound_object_ptr;
#endif

// Private streaming media data common to RealPlayer and WMP.

#define PLAYER_UNAVAILABLE		1
#define	STREAM_UNAVAILABLE		2
#define	STREAM_STARTED			3

static int media_player;
static int unscaled_video_width, unscaled_video_height;
static int video_pixel_format;
//static int stream_command, new_stream_command;
static event stream_opened;
//static event stream_command_sent;
static event terminate_streaming_thread;
static event rp_download_requested;
static event wmp_download_requested;

// Private streaming media data specific to RealPlayer.

static HINSTANCE hDll;
static ExampleClientContext *exContext;
static FPRMCREATEENGINE	m_fpCreateEngine;
static FPRMCLOSEENGINE m_fpCloseEngine;
static IRMAClientEngine *pEngine;
static IRMAPlayer *pPlayer;
static IRMAPlayer2 *pPlayer2;
static IRMAAudioPlayer *pAudioPlayer;
#ifdef SUPPORT_A3D
static ExampleAudioDevice *pAudDev;
#endif
static IRMAErrorSink *pErrorSink;
static IRMAErrorSinkControl *pErrorSinkControl;
static byte *video_buffer_ptr;

// Private streaming media data specific to WMP.

#define	AUDIO_PACKET_SIZE			5000
#define MAX_AUDIO_PACKETS			10
#define HALF_AUDIO_PACKETS			5
#define SOUND_BUFFER_SIZE			AUDIO_PACKET_SIZE * MAX_AUDIO_PACKETS

static IMultiMediaStream *global_stream_ptr;
static HANDLE end_of_stream_handle;

static bool streaming_video_available;
static IMediaStream *primary_video_stream_ptr;
static IDirectDrawMediaStream *ddraw_stream_ptr;
static IDirectDrawStreamSample *video_sample_ptr;
static LPDIRECTDRAWSURFACE video_surface_ptr;
static DDPIXELFORMAT ddraw_video_pixel_format;
static event video_frame_available;

//static bool streaming_audio_available;
//static IMediaStream *primary_audio_stream_ptr;
//static IAudioMediaStream *audio_stream_ptr;
//static IAudioData *audio_data_ptr;
//static byte *audio_buffer_ptr;
//static IAudioStreamSample *audio_sample_ptr;
//static event refill_sound_buffer;
//static event audio_packet_available;
//static int sound_buffer_offset;
//static int audio_packets_needed;
//static bool audio_packet_requested;
//static bool streaming_audio_started;

// Instance handle, player thread handle and streaming thread handle.

static HINSTANCE instance_handle;
static HANDLE player_thread_handle;
static unsigned long streaming_thread_handle;

// Splash graphic as a texture and as a bitmap.

static texture *splash_texture_ptr;
static bitmap *splash_bitmap_ptr;

#ifdef SUPPORT_A3D

// A3D splash graphic as a texture.

static texture *A3D_splash_texture_ptr;

#endif

// Main window data.

static HWND main_window_handle;
static HWND browser_window_handle;
static void (*key_callback_ptr)(int key_code, bool key_down);
static void (*mouse_callback_ptr)(int x, int y, int button_code,
								  int task_bar_button_code);
static void (*timer_callback_ptr)(void);
static void (*resize_callback_ptr)(void *window_handle, int width, int height);

// Progress window data.

static HWND progress_window_handle;
static HWND progress_bar_handle;
static void (*progress_callback_ptr)(void);

// Light window data.

static HWND light_window_handle;
static HWND light_slider_handle;
static void (*light_callback_ptr)(float brightness);

// Options window data.

static HFONT bold_font_handle;
static HFONT symbol_font_handle;
static HWND options_window_handle;
static HWND sound_control_handle;
static HWND reflection_control_handle;
static HWND accel_control_handle;
static HWND edit_control_handle;
static HWND updown_control_handle;
static void (*options_callback_ptr)(int option_ID, int option_value);

// History menu data.

static HMENU history_menu_handle;

// Password window data.

static HWND password_window_handle;
static HWND username_edit_handle;
static HWND password_edit_handle;
static void (*password_callback_ptr)(const char *username, const char *password);

// Message log window data.

static HWND message_log_window_handle;
static HWND log_edit_control_handle;

// Critical section object used to synchronise thread access to global 
// variables.

static CRITICAL_SECTION critical_section;

// Inactive window callback procedure prototype.

typedef void (*inactive_callback)(void *window_data_ptr);

// Macro for converting a point size into pixel units

#define	POINTS_TO_PIXELS(point_size) \
	MulDiv(point_size, GetDeviceCaps(dc_handle, LOGPIXELSY), 72)

// Macro to pass a mouse message to a handler.

#define HANDLE_MOUSE_MSG(window_handle, message, fn) \
	return(((fn)((window_handle), (message), LOWORD(lParam), HIWORD(lParam), \
		   (UINT)(wParam)), 0L))

// Number of task bar buttons, the logo label, and the inactive label.

#define TASK_BAR_BUTTONS	4
#define LOGO_LABEL			"Go to Flatland!"
#define INACTIVE_LABEL		"Press the 'T' key to turn off 3D acceleration"

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
	IDR_HISTORY0, IDR_OPTIONS0, IDR_LIGHT0, IDR_BUILDER0
};

static int button1_ID_list[TASK_BAR_BUTTONS] = {
	IDR_HISTORY1, IDR_OPTIONS1, IDR_LIGHT1, IDR_BUILDER1
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

// Splash graphic flag.

static bool splash_graphic_enabled;

// Width of a linearly interpolated texture span.

#define SPAN_WIDTH			32
#define SPAN_SHIFT			5

// Two constants used to load immediate floating point values into floating
// point registers.

const float const_1 = 1.0;
const float fixed_shift = 65536.0;

// Assembly macro for fast float to fixed point conversion for texture (u,v)
// coordinates.

#define COMPUTE_UV(u,v,u_on_tz,v_on_tz,end_tz) __asm \
{ \
	__asm fld	fixed_shift \
	__asm fmul	end_tz \
	__asm fld	st(0) \
	__asm fmul	u_on_tz \
	__asm fistp	DWORD PTR u \
	__asm fmul  v_on_tz \
	__asm fistp DWORD PTR v \
}

// Assembly macro for drawing a 16-bit pixel.

#define DRAW_PIXEL16 __asm \
{ \
	__asm mov edi, ebx \
	__asm and edi, ecx \
	__asm shr edi, FRAC_BITS \
	__asm mov eax, edx \
	__asm and eax, ecx \
	__asm and eax, INT_MASK \
	__asm shr eax, cl \
	__asm or  edi, eax \
	__asm mov eax, image_ptr \
	__asm mov ax, [eax + edi * 2] \
	__asm add ebx, ebp \
	__asm mov [esi], ax \
	__asm add edx, esp \
	__asm add esi, 2 \
}

// Assembly macro for drawing a 24-bit pixel.

#define DRAW_PIXEL24 __asm \
{ \
	__asm mov edi, ebx \
	__asm and edi, ecx \
	__asm shr edi, FRAC_BITS \
	__asm mov eax, edx \
	__asm and eax, ecx \
	__asm and eax, INT_MASK \
	__asm shr eax, cl \
	__asm or  edi, eax \
	__asm mov eax, image_ptr \
	__asm mov eax, [eax + edi * 4] \
	__asm add ebx, ebp \
	__asm mov edi, [esi] \
	__asm and edi, 0xff000000 \
	__asm or edi, eax \
	__asm mov [esi], edi \
	__asm add edx, esp \
	__asm add esi, 3 \
}
// Assembly macro for drawing a 32-bit pixel.

#define DRAW_PIXEL32 __asm \
{ \
	__asm mov edi, ebx \
	__asm and edi, ecx \
	__asm shr edi, FRAC_BITS \
	__asm mov eax, edx \
	__asm and eax, ecx \
	__asm and eax, INT_MASK \
	__asm shr eax, cl \
	__asm or  edi, eax \
	__asm mov eax, image_ptr \
	__asm mov eax, [eax + edi * 4] \
	__asm add ebx, ebp \
	__asm mov [esi], eax \
	__asm add edx, esp \
	__asm add esi, 4 \
}

// Assembly macro for drawing a possibly transparent 16-bit pixel.

#define DRAW_TRANSPARENT_PIXEL16(label) __asm \
{ \
	__asm mov edi, ebx \
	__asm and edi, ecx \
	__asm shr edi, FRAC_BITS \
	__asm mov eax, edx \
	__asm and eax, ecx \
	__asm and eax, INT_MASK \
	__asm shr eax, cl \
	__asm or  edi, eax \
	__asm mov eax, image_ptr \
	__asm mov ax, [eax + edi * 2] \
	__asm test ax, transparency_mask16 \
	__asm jnz label \
	__asm mov [esi], ax \
} __asm { \
label: \
	__asm add ebx, ebp \
	__asm add edx, esp \
	__asm add esi, 2 \
}

// Assembly macro for drawing a possibly transparent 24-bit pixel.

#define DRAW_TRANSPARENT_PIXEL24(label) __asm \
{ \
	__asm mov edi, ebx \
	__asm and edi, ecx \
	__asm shr edi, FRAC_BITS \
	__asm mov eax, edx \
	__asm and eax, ecx \
	__asm and eax, INT_MASK \
	__asm shr eax, cl \
	__asm or  edi, eax \
	__asm mov eax, image_ptr \
	__asm mov eax, [eax + edi * 4] \
	__asm test eax, transparency_mask24 \
	__asm jnz label \
	__asm mov edi, [esi] \
	__asm and edi, 0xff000000 \
	__asm or edi, eax \
	__asm mov [esi], edi \
} __asm { \
label: \
	__asm add ebx, ebp \
	__asm add edx, esp \
	__asm add esi, 3 \
}

// Assembly macro for drawing a possibly transparent 32-bit pixel.

#define DRAW_TRANSPARENT_PIXEL32(label) __asm \
{ \
	__asm mov edi, ebx \
	__asm and edi, ecx \
	__asm shr edi, FRAC_BITS \
	__asm mov eax, edx \
	__asm and eax, ecx \
	__asm and eax, INT_MASK \
	__asm shr eax, cl \
	__asm or  edi, eax \
	__asm mov eax, image_ptr \
	__asm mov eax, [eax + edi * 4] \
	__asm test eax, transparency_mask32 \
	__asm jnz label \
	__asm mov [esi], eax \
} __asm { \
label: \
	__asm add ebx, ebp \
	__asm add edx, esp \
	__asm add esi, 4 \
}

// These variables are globals so that our assembler code does not have to use
// the base pointer to get to them (that way we can temporarily use the base
// pointer for something else).

static delta_sx;
static byte *image_ptr, *end_image_ptr, *new_image_ptr;
static byte *fb_ptr;
static int span_start_sx, span_end_sx;
static fixed u, end_u, delta_u;
static fixed v, end_v, delta_v;
static float end_one_on_tz, end_tz;
static int ebp_save, esp_save;
static int span_width, image_width, image_height;
static int transparent_index;
static word transparency_mask16;
static pixel transparency_mask24;
static pixel transparency_mask32;
static int mask, shift;
static word colour_pixel16;
static pixel colour_pixel24;
static pixel colour_pixel32;
static pixel *palette_ptr;
static int lit_image_dimensions;
static int row_gap;

//==============================================================================
// Private functions.
//==============================================================================

//------------------------------------------------------------------------------
// Functions to display error messages that begin with common phrases.
//------------------------------------------------------------------------------

static void
failed_to(const char *message)
{
	diagnose("Failed to %s", message);
}

static void
failed_to_create(const char *message)
{
	diagnose("Failed to create %s", message);
}

static void
failed_to_get(const char *message)
{
	diagnose("Failed to get %s", message);
}

static void
failed_to_set(const char *message)
{
	diagnose("Failed to set %s", message);
}

//------------------------------------------------------------------------------
// Blit the frame buffer onto the primary surface.
//------------------------------------------------------------------------------

static void
blit_frame_buffer(void)
{
	POINT pos;
	RECT framebuf_surface_rect;
	RECT primary_surface_rect;
	HRESULT bltresult;

	START_TIMING;

	// Set the frame buffer surface rectangle, and copy it to the primary
	// surface rectangle.

	framebuf_surface_rect.left = 0;
	framebuf_surface_rect.top = 0;
	framebuf_surface_rect.right = display_width;
	framebuf_surface_rect.bottom = display_height;
	primary_surface_rect = framebuf_surface_rect;

	// Offset the primary surface rectangle by the position of the main 
	// window's client area.

	pos.x = 0;
	pos.y = 0;
	ClientToScreen(main_window_handle, &pos);
	primary_surface_rect.left += pos.x;
	primary_surface_rect.top += pos.y;
	primary_surface_rect.right += pos.x;
	primary_surface_rect.bottom += pos.y;

	// Blit the frame buffer surface rectangle to the the primary surface.

	while (true) {
		bltresult = primary_surface_ptr->Blt(&primary_surface_rect,
			framebuf_surface_ptr, &framebuf_surface_rect, 0, NULL);
		if (bltresult == DD_OK || bltresult != DDERR_WASSTILLDRAWING)
			break;
	}

	END_TIMING("blit_frame_buffer");
}

//------------------------------------------------------------------------------
// Load a GIF resource.
//------------------------------------------------------------------------------

static texture *
load_GIF_resource(int resource_ID)
{
	HRSRC resource_handle;
	HGLOBAL GIF_handle;
	LPVOID GIF_ptr;
	texture *texture_ptr;

	// Find the resource with the given ID.

	if ((resource_handle = FindResource(instance_handle, 
		MAKEINTRESOURCE(resource_ID), "GIF")) == NULL)
		return(NULL);

	// Load the resource.

	if ((GIF_handle = LoadResource(instance_handle, resource_handle)) == NULL)
		return(NULL);

	// Lock the resource, obtaining a pointer to the raw data.

	if ((GIF_ptr = LockResource(GIF_handle)) == NULL)
		return(NULL);

	// Open the raw data as if it were a file.

	if (!push_buffer((const char *)GIF_ptr, 
		SizeofResource(instance_handle, resource_handle)))
		return(NULL);

	// Load the raw data as a GIF.

	if ((texture_ptr = load_GIF_image()) == NULL)
		return(NULL);

	// Close the raw data "file" and return a pointer to the texture.

	pop_file();
	return(texture_ptr);
}

//------------------------------------------------------------------------------
// Load an icon.
//------------------------------------------------------------------------------

static icon *
load_icon(int resource0_ID, int resource1_ID)
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
// Create an 8-bit bitmap of the given dimensions, using the given palette.
//------------------------------------------------------------------------------

static bitmap *
create_bitmap(int width, int height, int colours, RGBcolour *RGB_palette,
			  pixel *palette, int transparent_index)
{
	bitmap *bitmap_ptr;
	HDC hdc;
	MYBITMAPINFO bitmap_info;
	DIBSECTION DIB_info;
	int index;

	// If the bitmap row pitch will be greater than the display width, decrease
	// the width of the bitmap by one.  This is to workaround what appears to be
	// a bug in Windows NT.

	if ((width & 1) && width == display_width)
		width--;

	// Create the bitmap object.

	NEW(bitmap_ptr, bitmap);
	if (bitmap_ptr == NULL)
		return(NULL);

	// Initialise the bitmap info structure.

	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biWidth = width;
	bitmap_info.bmiHeader.biHeight = -height;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 8;
	bitmap_info.bmiHeader.biSizeImage = 0;
	bitmap_info.bmiHeader.biXPelsPerMeter = 0;
	bitmap_info.bmiHeader.biYPelsPerMeter = 0;
	bitmap_info.bmiHeader.biClrUsed = 0;
	bitmap_info.bmiHeader.biClrImportant = 0;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	for (index = 0; index < colours; index++) {
		bitmap_info.bmiColors[index].rgbRed = (byte)RGB_palette[index].red;
		bitmap_info.bmiColors[index].rgbGreen = (byte)RGB_palette[index].green;
		bitmap_info.bmiColors[index].rgbBlue = (byte)RGB_palette[index].blue;
		bitmap_info.bmiColors[index].rgbReserved = 0;
	}

	// Create the bitmap image as a DIB section.

	hdc = GetDC(main_window_handle);
	bitmap_ptr->handle = CreateDIBSection(hdc, (BITMAPINFO *)&bitmap_info,
		DIB_RGB_COLORS, (void **)&bitmap_ptr->pixels, NULL, 0);
	ReleaseDC(main_window_handle, hdc);
	if (bitmap_ptr->handle == NULL) {
		DEL(bitmap_ptr, bitmap);
		return(NULL);
	}

	// If the bitmap image was created, fill in the remainder of the bitmap
	// structure.

	GetObject(bitmap_ptr->handle, sizeof(DIBSECTION), &DIB_info);
	bitmap_ptr->width = width;
	bitmap_ptr->height = height;
	bitmap_ptr->bytes_per_row = DIB_info.dsBm.bmWidthBytes;
	bitmap_ptr->colours = colours;
	bitmap_ptr->RGB_palette = RGB_palette;
	bitmap_ptr->palette = palette;
	bitmap_ptr->transparent_index = transparent_index;
	return(bitmap_ptr);
}

//------------------------------------------------------------------------------
// Create a bitmap from a texture.
//------------------------------------------------------------------------------

static bitmap *
texture_to_bitmap(texture *texture_ptr)
{
	pixmap *pixmap_ptr;
	byte *old_image_ptr, *new_image_ptr;
	int row, column, row_gap;
	bitmap *bitmap_ptr;

	// Create a bitmap large enough to store the GIF.

	if ((bitmap_ptr = create_bitmap(texture_ptr->width, texture_ptr->height,
		texture_ptr->colours, texture_ptr->RGB_palette, NULL, -1)) == NULL)
		return(NULL);

	// Copy the first pixmap of the texture to the bitmap.

	pixmap_ptr = &texture_ptr->pixmap_list[0];
	old_image_ptr = pixmap_ptr->image_ptr;
	new_image_ptr = bitmap_ptr->pixels;
	row_gap = bitmap_ptr->bytes_per_row - texture_ptr->width;
	for (row = 0; row < texture_ptr->height; row++) {
		for (column = 0; column < texture_ptr->width; column++)
			*new_image_ptr++ = *old_image_ptr++;
		new_image_ptr += row_gap;
	}

	// Return a pointer to the bitmap.

	return(bitmap_ptr);
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
	y = display_height - icon_ptr->height;
	draw_pixmap(pixmap_ptr, 0, x, y);
}

//------------------------------------------------------------------------------
// Draw the logo.
//------------------------------------------------------------------------------

static void
draw_logo(bool logo_active, bool clear_background)
{
	if (clear_background)
		clear_frame_buffer(logo_x, display_height - logo_icon_ptr->height,
			logo_icon_ptr->width, logo_icon_ptr->height);
	else
		clear_frame_buffer(logo_x, display_height - TASK_BAR_HEIGHT,
			logo_icon_ptr->width, TASK_BAR_HEIGHT);
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

	// Draw the right edge of the button bar.

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
		active_button_index != prev_active_button_index) {
		if (full_screen)
			show_label(INACTIVE_LABEL);
		else
			show_label(button_label_list[active_button_index]);
	}
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
	int x;
	HDC dc_handle;
	RECT title_rect;

	// Draw the background for the title.

	for (x = title_start_x; x < title_end_x; x += title_bg_icon_ptr->width)
		draw_icon(title_bg_icon_ptr, false, x);
	draw_icon(title_end_icon_ptr, false, title_end_x);

	// Get the device context for the frame buffer surface.

	if (framebuf_surface_ptr->GetDC(&dc_handle) != DD_OK)
		return;

	// Set the rectangle in which to draw the title.

	title_rect.left = title_start_x;
	title_rect.top = window_height;
	title_rect.right = title_end_x;
	title_rect.bottom = display_height;

	// Draw the text centred in the title rectangle.

	SetBkMode(dc_handle, TRANSPARENT);
	SetTextColor(dc_handle, RGB(0xff, 0xcc, 0x66));
	DrawText(dc_handle, title_text, strlen(title_text), &title_rect, 
		DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

	// Release the device context.

	framebuf_surface_ptr->ReleaseDC(dc_handle);
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

#ifdef SUPPORT_A3D

		// Draw the A3D splash graphic if it exists.

		if (A3D_splash_texture_ptr) {

			// Draw the first pixmap.

			pixmap_ptr = &A3D_splash_texture_ptr->pixmap_list[0];

			// Position the pixmap in the bottom-right corner of the window,
			// offset by 20 pixels to give some space around it.

			x = window_width - A3D_splash_texture_ptr->width - 20;
			y = window_height - A3D_splash_texture_ptr->height - 20;

			// Draw the pixmap at full brightness.

			draw_pixmap(pixmap_ptr, 0, x, y);
		}

#endif

	}
}

//------------------------------------------------------------------------------
// Draw the task bar.
//------------------------------------------------------------------------------

static void
draw_task_bar(bool loading_spot)
{
	START_TIMING;

	// If the spot is being loaded, draw the logo as active.

	if (loading_spot)
		draw_logo(true, true);
	
	// If the spot is not been loaded...
	
	else {

		// If full-screen mode is active, the logo is inactive.

		if (full_screen)
			draw_logo(false, false);

		// Otherwise draw it as inactive or active depending on whether the
		// mouse is pointing at it.
		
		else {
			start_atomic_operation();
			draw_logo(logo_active, false);
			end_atomic_operation();
		}

		// Display a label above the logo while it's active.  If full screen
		// mode is in effect, this is a generic inactive message.

		start_atomic_operation();
		if (logo_active && !prev_logo_active) {
			if (full_screen)
				show_label(INACTIVE_LABEL);
			else
				show_label(LOGO_LABEL);
		}
		if (!logo_active && prev_logo_active)
			hide_label();
		prev_logo_active = logo_active;
		end_atomic_operation();
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
	HDC dc_handle;
	int x, y;
	RECT label_rect;
	
	// Get a handle to the frame buffer surface's device context.

	if (framebuf_surface_ptr->GetDC(&dc_handle) != DD_OK)
		return;

	// Determine the size of the rectangle required to draw the label text.

	label_rect.left = 0;
	label_rect.top = 0;
	label_rect.right = 0;
	label_rect.bottom = TASK_BAR_HEIGHT;
	DrawText(dc_handle, label_text, strlen(label_text), &label_rect, 
		DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT | DT_NOPREFIX);
	if (label_rect.right > display_width)
		label_rect.right = display_width;

	// Place the label above or below the mouse cursor, making sure it doesn't
	// get clipped by the right edge of the screen.

	x = mouse_x - curr_cursor_ptr->hotspot_x;
	if (x + label_rect.right >= window_width)
		x = window_width - label_rect.right;
	if (mouse_y >= window_height)
		y = window_height - TASK_BAR_HEIGHT;
	else {
		y = mouse_y - curr_cursor_ptr->hotspot_y + GetSystemMetrics(SM_CYCURSOR);
		if (y + label_rect.bottom >= window_height)
			y = mouse_y - curr_cursor_ptr->hotspot_y - TASK_BAR_HEIGHT;
	}
	label_rect.left += x;
	label_rect.top += y;
	label_rect.right += x;
	label_rect.bottom += y;

	// Draw the label text.

	SetBkMode(dc_handle, OPAQUE);
	SetBkColor(dc_handle, RGB(0x33, 0x33, 0x33));
	SetTextColor(dc_handle, RGB(0xff, 0xcc, 0x66));
	DrawText(dc_handle, label_text, strlen(label_text), &label_rect, 
		DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);

	// Release the device context.

	framebuf_surface_ptr->ReleaseDC(dc_handle);
}

//------------------------------------------------------------------------------
// Create a cursor.
//------------------------------------------------------------------------------

static cursor *
create_cursor(HCURSOR cursor_handle) 
{
	cursor *cursor_ptr;
	ICONINFO cursor_info;

	// Create the cursor object, and initialise it's handle.

	if ((cursor_ptr = new cursor) == NULL)
		return(NULL);
	cursor_ptr->handle = cursor_handle;

	// Get the cursor mask and image bitmaps, and the cursor hotspot.

	GetIconInfo(cursor_handle, &cursor_info);
	cursor_ptr->hotspot_x = cursor_info.xHotspot;
	cursor_ptr->hotspot_y = cursor_info.yHotspot;

	// If we are running full-screen, make copies of the mask and image bitmaps.

	if (full_screen) {
		int cursor_width, cursor_height;
		HDC fb_hdc;
		HDC source_hdc, target_hdc;
		HBITMAP old_source_bitmap_handle, old_target_bitmap_handle;

		// Determine the size of the cursor, then create the bitmaps for the
		// cursor mask and image.

		cursor_width = GetSystemMetrics(SM_CXCURSOR);
		cursor_height = GetSystemMetrics(SM_CYCURSOR);
		cursor_ptr->mask_bitmap_ptr = create_bitmap(cursor_width, cursor_height,
			256, standard_RGB_palette, standard_palette, colour_index[215]);
		cursor_ptr->image_bitmap_ptr = create_bitmap(cursor_width, cursor_height, 
			256, standard_RGB_palette, standard_palette, colour_index[0]);
		if (cursor_ptr->mask_bitmap_ptr == NULL || 
			cursor_ptr->image_bitmap_ptr == NULL)
			return(NULL);

		// Create a source and target device context that is compatible with
		// the frame buffer device context.

		framebuf_surface_ptr->GetDC(&fb_hdc);
		source_hdc = CreateCompatibleDC(fb_hdc);
		target_hdc = CreateCompatibleDC(fb_hdc);
		framebuf_surface_ptr->ReleaseDC(fb_hdc);

		// Make a copy of the actual cursor mask bitmap.

		old_target_bitmap_handle = SelectBitmap(target_hdc, 
			cursor_ptr->mask_bitmap_ptr->handle);
		old_source_bitmap_handle = SelectBitmap(source_hdc, 
			cursor_info.hbmMask);
		BitBlt(target_hdc, 0, 0, cursor_width, cursor_height, source_hdc, 0, 0,
			SRCCOPY);
		SelectBitmap(source_hdc, old_source_bitmap_handle);
		SelectBitmap(target_hdc, old_target_bitmap_handle);

		// Make a copy of the actual cursor image bitmap.

		old_target_bitmap_handle = SelectBitmap(target_hdc, 
			cursor_ptr->image_bitmap_ptr->handle);
		if (cursor_info.hbmColor) {
			old_source_bitmap_handle = SelectBitmap(source_hdc, 
				cursor_info.hbmColor);
			BitBlt(target_hdc, 0, 0, cursor_width, cursor_height, source_hdc,
				0, 0, SRCCOPY);
		} else {
			old_source_bitmap_handle = SelectBitmap(source_hdc, 
				cursor_info.hbmMask);
			BitBlt(target_hdc, 0, 0, cursor_width, cursor_height, source_hdc,
				0, cursor_height, SRCCOPY);
		}
		SelectBitmap(source_hdc, old_source_bitmap_handle);
		SelectBitmap(target_hdc, old_target_bitmap_handle);

		// Delete the source and target device contexts.

		DeleteDC(source_hdc);
		DeleteDC(target_hdc);
	}

	// Delete the actual cursor bitmaps.

	DeleteBitmap(cursor_info.hbmMask);
	if (cursor_info.hbmColor)
		DeleteBitmap(cursor_info.hbmColor);

	// Return a pointer to the cursor object.

	return(cursor_ptr);
}

//------------------------------------------------------------------------------
// Draw the current cursor.
//------------------------------------------------------------------------------

static void
draw_bitmap(bitmap *bitmap_ptr, int x, int y);

static void
draw_cursor(void)
{
	POINT cursor_pos;

	// Get the cursor position, scale it to the display size, and adjust it to
	// take into account the cursor hotspot.

	GetCursorPos(&cursor_pos);
	cursor_pos.x = (int)((float)cursor_pos.x * width_factor) -
		 curr_cursor_ptr->hotspot_x;
	cursor_pos.y = (int)((float)cursor_pos.y * height_factor) -
		curr_cursor_ptr->hotspot_y;

	// Draw the cursor mask bitmap followed by the cursor image bitmap.

	draw_bitmap(curr_cursor_ptr->mask_bitmap_ptr, cursor_pos.x, cursor_pos.y);
	draw_bitmap(curr_cursor_ptr->image_bitmap_ptr, cursor_pos.x, cursor_pos.y);
}

//------------------------------------------------------------------------------
// Function to compute constants for converting a colour component to a value
// that forms part of a pixel value.
//------------------------------------------------------------------------------

static void
set_component(pixel component_mask, pixel &pixel_mask, int &right_shift,
			  int &left_shift)
{
	int component_size;

	// Count the number of zero bits in the component mask, starting from the
	// rightmost bit.  This is the left shift.

	left_shift = 0;
	while ((component_mask & 1) == 0 && left_shift < 32) {
		component_mask >>= 1;
		left_shift++;
	}

	// Count the number of one bits in the component mask, starting from the
	// rightmost bit.  This is the component size.

	component_size = 0;
	while ((component_mask & 1) == 1 && left_shift + component_size < 32) {
		component_mask >>= 1;
		component_size++;
	}
	if (component_size > 8)
		component_size = 8;

	// Compute the right shift as 8 - component size.  Use the component size to
	// look up the pixel mask in a table.

	right_shift = 8 - component_size;
	pixel_mask = pixel_mask_table[component_size - 1];
}

//------------------------------------------------------------------------------
// Set up a pixel format based upon the component masks.
//------------------------------------------------------------------------------

static void
set_pixel_format(pixel_format *pixel_format_ptr, pixel red_comp_mask,
				 pixel green_comp_mask, pixel blue_comp_mask,
				 pixel alpha_comp_mask)
{
	set_component(red_comp_mask, pixel_format_ptr->red_mask,
		pixel_format_ptr->red_right_shift, pixel_format_ptr->red_left_shift);
	set_component(green_comp_mask, pixel_format_ptr->green_mask,
		pixel_format_ptr->green_right_shift, pixel_format_ptr->green_left_shift);
	set_component(blue_comp_mask, pixel_format_ptr->blue_mask,
		pixel_format_ptr->blue_right_shift, pixel_format_ptr->blue_left_shift);
	pixel_format_ptr->alpha_comp_mask = alpha_comp_mask;
}

//------------------------------------------------------------------------------
// Allocate and create the dither tables.
//------------------------------------------------------------------------------

static bool
create_dither_tables(void)
{
	int index, table;
	float source_factor;
	float target_factor;
	float RGB_threshold[4];
	int red, green, blue;

	// Allocate the four dither tables, each containing 32768 palette indices.

	if ((dither_table[0] = new byte[32768]) == NULL ||
		(dither_table[1] = new byte[32768]) == NULL ||
		(dither_table[2] = new byte[32768]) == NULL ||
		(dither_table[3] = new byte[32768]) == NULL)
		return(false);

	// Set up some convienance pointers to each dither table.

	dither00 = dither_table[0];
	dither01 = dither_table[1];
	dither10 = dither_table[2];
	dither11 = dither_table[3];

	// Compute the source and destination RGB factors.

	source_factor = 256.0f / 32.0f;
	target_factor = 256.0f / 6.0f;

	// Compute the RGB threshold values.

	for (table = 0; table < 4; table++)
		RGB_threshold[table] = table * target_factor / 4.0f;

	// Now generate the dither tables.

	index = 0;
	for (red = 0; red < 32; red++)
		for (green = 0; green < 32; green++)
			for (blue = 0; blue < 32; blue++) {
				for (table = 0; table < 4; table++) {
					int r, g, b;

					r = (int)(MIN(red * source_factor / target_factor, 4.0));
					if (red * source_factor - r * target_factor >
						RGB_threshold[table])
						r++;
					g = (int)(MIN(green * source_factor / target_factor, 4.0));
					if (green * source_factor - g * target_factor >
						RGB_threshold[table])
						g++;
					b = (int)(MIN(blue * source_factor / target_factor, 4.0));
					if (blue * source_factor - b * target_factor >
						RGB_threshold[table])
						b++;
					dither_table[table][index] = 
						colour_index[(r * 6 + g) * 6 + b];
				}
				index++;
			}
	return(true);
}

//------------------------------------------------------------------------------
// Allocate and create the light tables.
//------------------------------------------------------------------------------

static bool
create_light_tables(void)
{
	int table, index;
	float red, green, blue;
	float brightness;
	RGBcolour colour;

	// Create a light table for each brightness level.

	for (table = 0; table < BRIGHTNESS_LEVELS; table++) {

		// Create a table of 32768 pixels.

		if ((light_table[table] = new pixel[65536]) == NULL)
			return(false);

		// Choose a brightness factor for this table.

		brightness = (float)(MAX_BRIGHTNESS_INDEX - table) / 
			(float)MAX_BRIGHTNESS_INDEX;

		// Step through the 32768 RGB combinations, and convert each one to a
		// display pixel at the chosen brightness.

		index = 0;
		for (red = 0.0f; red < 256.0f; red += 8.0f)
			for (green = 0.0f; green < 256.0f; green += 8.0f)
				for (blue = 0.0f; blue < 256.0f; blue += 8.0f) {
					colour.set_RGB(red, green, blue); 
					colour.adjust_brightness(brightness);
					light_table[table][index] = RGB_to_display_pixel(colour);
					index++;
				}
	}
	return(true);
}

//------------------------------------------------------------------------------
// Create the standard palette (a 6x6x6 colour cube).
//------------------------------------------------------------------------------

static bool
create_standard_palette()
{
	int index, red, green, blue;

	// For an 8-bit colour depth, create the standard palette from colour
	// entries in the system palette.  This really only works well if the
	// browser has already created a 6x6x6 colour cube in the system palette,
	// otherwise the colours chosen won't match the desired colours very well
	// at all.

	if (display_depth == 8) {
		HDC hdc;
		MYLOGPALETTE palette;
		PALETTEENTRY *palette_entries;
		bool used_palette_entry[256];

		// Get the system palette entries.

		hdc = GetDC(NULL);
		palette_entries = (PALETTEENTRY *)&palette.palPalEntry;
		GetSystemPaletteEntries(hdc, 0, 256, palette_entries);
		ReleaseDC(NULL, hdc);

		// Create a palette matching the system palette.

		palette.palVersion = 0x300;
		palette.palNumEntries = 256;
		if ((standard_palette_handle = CreatePalette((LOGPALETTE *)&palette))
			== NULL)
			return(false);

		// Copy the system palette entries into our standard RGB palette, and
		// initialise the used palette entry flags.

		for (index = 0; index < 256; index++) {
			standard_RGB_palette[index].red = palette_entries[index].peRed;
			standard_RGB_palette[index].green = palette_entries[index].peGreen;
			standard_RGB_palette[index].blue = palette_entries[index].peBlue;
			used_palette_entry[index] = false;
		}

		// Locate all colours in the 6x6x6 colour cube, and set up the colour
		// index table to match.  Used palette entries are flagged.

		index = 0;
		for (red = 0; red < 6; red++)
			for (green = 0; green < 6; green++)
				for (blue = 0; blue < 6; blue++) {
					int palette_index = 
						GetNearestPaletteIndex(standard_palette_handle,
						RGB(0x33 * red, 0x33 * green, 0x33 * blue));
					colour_index[index] = palette_index;
					used_palette_entry[palette_index] = true;
					index++;
				}

		// Choose an unused palette entry as the transparent pixel.

		for (index = 0; index < 256; index++)
			if (!used_palette_entry[index])
				break;
		standard_transparent_index = index;
	}

	// For all other colour depths, simply create our own standard palette.

	else {

		// Create the 6x6x6 colour cube and set up the colour index table to
		// match.

		index = 0;
		for (red = 0; red < 6; red++)
			for (green = 0; green < 6; green++)
				for (blue = 0; blue < 6; blue++) {
					standard_RGB_palette[index].red = (byte)(0x33 * red);
					standard_RGB_palette[index].green = (byte)(0x33 * green);
					standard_RGB_palette[index].blue = (byte)(0x33 * blue);
					colour_index[index] = index;
					index++;
				}

		// Assign the COLOR_MENU colour to the next available palette entry.
		
		standard_RGB_palette[index].red = GetRValue(GetSysColor(COLOR_MENU));
		standard_RGB_palette[index].green = GetGValue(GetSysColor(COLOR_MENU));
		standard_RGB_palette[index].blue = GetBValue(GetSysColor(COLOR_MENU));
		index++;

		// Assign the standard transparent pixel to the next available palette
		// entry.

		standard_transparent_index = index;

		// Fill the remaining palette entries with black.

		while (index < 256) {
			standard_RGB_palette[index].red = 0;
			standard_RGB_palette[index].green = 0;
			standard_RGB_palette[index].blue = 0;
			index++;
		}
	}

	// Indicate success.

	return(true);
}

//------------------------------------------------------------------------------
// Initialise an icon.
//------------------------------------------------------------------------------

static bool
init_icon(icon *icon_ptr)
{
	// If the display depth is 8, create the palette index tables for the icon
	// textures.

	if (display_depth == 8) {
		if (icon_ptr->texture0_ptr &&
			!icon_ptr->texture0_ptr->create_palette_index_table())
			return(false);
		if (icon_ptr->texture1_ptr &&
			!icon_ptr->texture1_ptr->create_palette_index_table())
			return(false);
	}

	// If the display depth is 16, 24 or 32, create the display palette list for
	// the icon textures.

	else {
		if (icon_ptr->texture0_ptr)
			icon_ptr->texture0_ptr->create_display_palette_list();
		if (icon_ptr->texture1_ptr)
			icon_ptr->texture1_ptr->create_display_palette_list();
	}

	// Return success status.

	return(true);
}

//------------------------------------------------------------------------------
// Set the active cursor.
//------------------------------------------------------------------------------

static void
set_active_cursor(cursor *cursor_ptr)
{
	// If the requested cursor is already the current one, do nothing.
	// Otherwise make it the current cursor.

	if (cursor_ptr == curr_cursor_ptr)
		return;
	curr_cursor_ptr = cursor_ptr;

	// If not running full_screen, set the cursor in the class and make it the
	// current cursor, then force a cursor change by explicitly setting the
	// cursor position.

	if (!full_screen) {
		POINT cursor_pos;

		SetClassLong(main_window_handle, GCL_HCURSOR, 
			(LONG)curr_cursor_ptr->handle);
		SetCursor(curr_cursor_ptr->handle);
		GetCursorPos(&cursor_pos);
		SetCursorPos(cursor_pos.x, cursor_pos.y);
	}
}	

//------------------------------------------------------------------------------
// Convienance function for setting the render state.
//------------------------------------------------------------------------------

static bool
set_render_state(D3DRENDERSTATETYPE render_state, DWORD value)
{
	return(d3d_device_ptr->SetRenderState(render_state, value) == D3D_OK);
}

//------------------------------------------------------------------------------
// Callback function for enumerating DirectDraw 3D-enabled devices.
//------------------------------------------------------------------------------

static BOOL WINAPI
ddraw_callback(LPGUID guid_ptr, LPSTR device_desc, LPSTR device_name,
			   LPVOID user_arg_ptr)
{
	DDCAPS hw_caps;

	// If we can't create the DirectDraw device, just skip it.

	if (DirectDrawCreate(guid_ptr, &ddraw_object_ptr, NULL) != DD_OK)
		return(DDENUMRET_OK);

	// If we can't get the device's hardware capabilities or the device does
	// not support 3D in hardware, release the device and skip it.
	
	memset(&hw_caps, 0, sizeof(DDCAPS));
	hw_caps.dwSize = sizeof(DDCAPS);
	if (ddraw_object_ptr->GetCaps(&hw_caps, NULL) != DD_OK ||
		!(hw_caps.dwCaps & DDCAPS_3D)) {
		ddraw_object_ptr->Release();
		ddraw_object_ptr = NULL;
		return(DDENUMRET_OK);
	}

	// If the GUID for this device is NULL, the 3D hardware can render into a
	// window, otherwise it can only render in full-screen mode.

	if (user_arg_ptr != NULL) {
		if (guid_ptr == NULL)
			*(int *)user_arg_ptr = HARDWARE_WINDOW;
		else
			*(int *)user_arg_ptr = HARDWARE_FULLSCREEN;
	}

	// Stop the enumeration.

	return(DDENUMRET_CANCEL);
}

//------------------------------------------------------------------------------
// Callback function for enumerating 3D devices.
//------------------------------------------------------------------------------

static HRESULT WINAPI
device_callback(LPGUID guid_ptr, LPSTR device_desc, LPSTR device_name,
				LPD3DDEVICEDESC hw_device_desc_ptr, 
				LPD3DDEVICEDESC sw_device_desc_ptr, LPVOID user_arg_ptr)
{
	// If this is not a HAL device that supports RGB colour mode in hardware,
	// ignore it.

	if (*guid_ptr != IID_IDirect3DHALDevice ||
		hw_device_desc_ptr->dcmColorModel != D3DCOLOR_RGB)
		return(D3DENUMRET_OK);

	// Set a global flag indicating whether this device is capable of using
	// AGP memory for textures.

	if (hw_device_desc_ptr->dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM)
		AGP_flag = DDSCAPS_NONLOCALVIDMEM;
	else
		AGP_flag = 0;

	// Accept this device and return the Z buffer depth if it's a supported
	// depth.

	switch (hw_device_desc_ptr->dwDeviceZBufferBitDepth) {
	case DDBD_8:
		*(int *)user_arg_ptr = 8;
	    return(D3DENUMRET_CANCEL); 
	case DDBD_16:
		*(int *)user_arg_ptr = 16;
	    return(D3DENUMRET_CANCEL); 
	case DDBD_24:
		*(int *)user_arg_ptr = 24;
	    return(D3DENUMRET_CANCEL); 
	case DDBD_32:
		*(int *)user_arg_ptr = 32;
		return(D3DENUMRET_CANCEL);
	default:
		*(int *)user_arg_ptr = 24;
		return(D3DENUMRET_CANCEL);
	}	
}

//------------------------------------------------------------------------------
// Callback function for enumerating texture formats; a 16-bit ARGB texture
// format of 1555 is required.
//------------------------------------------------------------------------------
 
static HRESULT WINAPI
texture_callback(LPDDSURFACEDESC ddraw_surface_desc_ptr, LPVOID user_arg_ptr)
{
	LPDDPIXELFORMAT pixel_format_ptr;

	pixel_format_ptr = &ddraw_surface_desc_ptr->ddpfPixelFormat;
	if ((pixel_format_ptr->dwFlags & DDPF_RGB) &&
		(pixel_format_ptr->dwFlags & DDPF_ALPHAPIXELS) &&
		pixel_format_ptr->dwRGBBitCount == 16 &&
		pixel_format_ptr->dwRGBAlphaBitMask == 0x8000 &&
		pixel_format_ptr->dwRBitMask == 0x7c00 &&
		pixel_format_ptr->dwGBitMask == 0x03e0 &&
		pixel_format_ptr->dwBBitMask == 0x001f) {
		*(bool *)user_arg_ptr = true;
		ddraw_texture_pixel_format = *pixel_format_ptr;
		return(D3DENUMRET_CANCEL);
	}
	return(D3DENUMRET_OK);
}

//------------------------------------------------------------------------------
// Start up the 3D interface.
//------------------------------------------------------------------------------

static bool
start_up_3D_interface(void)
{
	DDSURFACEDESC ddraw_surface_desc;
	D3DVIEWPORT2 viewport_data;
	int zbuffer_depth;
	bool found_format;
	int size_index;
	int image_dimensions;

	// Query DirectDraw for the IDirect3D2 interface.

	if (ddraw_object_ptr->QueryInterface(IID_IDirect3D2,
		(void **)&d3d_object_ptr) != DD_OK) {
		failed_to_get("Direct3D interface");
		return(false);
	}

	// Enumerate the supported 3D devices, looking for a HAL device that
	// supports hardware RGB mode.

	zbuffer_depth = 0;
	if (d3d_object_ptr->EnumDevices(device_callback, &zbuffer_depth) != D3D_OK ||
		!zbuffer_depth) {
		failed_to("find compatible Z buffer");
		return(false);
	}

	// If running in a window, create seperate primary and frame buffer
	// surfaces.

	if (!full_screen) {

		// Create the primary surface.

		memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
		ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
		ddraw_surface_desc.dwFlags = DDSD_CAPS;
		ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc,
			&primary_surface_ptr, NULL) != DD_OK) {
			failed_to_create("primary surface");
			return(false);
		}

		// Create the frame buffer surface.

		ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
			DDSCAPS_3DDEVICE;
		ddraw_surface_desc.dwWidth = display_width;
		ddraw_surface_desc.dwHeight = display_height;
		if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc,
			&framebuf_surface_ptr, NULL) != DD_OK) {
			failed_to_create("frame buffer surface");
			return(false);
		}
	}

	// If running full-screen, create a flippable primary surface with a back
	// buffer.

	else {
		DDSCAPS surface_caps;

		// Create the flippable primary surface and back buffer.

		memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
		ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
		ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | 
			DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
		ddraw_surface_desc.dwBackBufferCount = 1;
		if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc,
			&primary_surface_ptr, NULL) != DD_OK) {
			failed_to_create("flippable surfaces");
			return(false);
		}

		// Get a pointer to the back buffer, which becomes the frame buffer.
	
		surface_caps.dwCaps = DDSCAPS_BACKBUFFER;
		if (primary_surface_ptr->GetAttachedSurface(&surface_caps,
			&framebuf_surface_ptr) != DD_OK) {
			failed_to_get("back buffer");
			return(false);
		}
	}

	// Create the Z buffer surface.

	memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
	ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
	ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT |
		DDSD_ZBUFFERBITDEPTH;
	ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
	ddraw_surface_desc.dwWidth = display_width;
	ddraw_surface_desc.dwHeight = display_height;
	ddraw_surface_desc.dwZBufferBitDepth = zbuffer_depth;
	if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc,
		&zbuffer_surface_ptr, NULL) != DD_OK) {
		failed_to_create("Z buffer");
		return(false);
	}

	// Attach the Z buffer surface to the frame buffer surface.
		
	if (framebuf_surface_ptr->AddAttachedSurface(zbuffer_surface_ptr) != DD_OK) {
		failed_to("attach Z buffer");
		return(false);
	}

	// Create the 3D device for the frame buffer surface.

	if (d3d_object_ptr->CreateDevice(IID_IDirect3DHALDevice,
		framebuf_surface_ptr, &d3d_device_ptr) != D3D_OK) {
		failed_to_create("3D device");
		return(false);
	}

	// Enumerate the texture formats that the 3D device supports, looking for
	// the 1555 ARGB format.

	found_format = false;
	if (d3d_device_ptr->EnumTextureFormats(texture_callback, &found_format)
		!= D3D_OK || !found_format) {
		failed_to("find compatible texture format");
		return(false);
	}

	// Create the viewport.

	if (d3d_object_ptr->CreateViewport(&d3d_viewport_ptr, NULL) != D3D_OK) {
		failed_to_create("viewport");
		return(false);
	}

	// Set up the viewport data.

	memset(&viewport_data, 0, sizeof(D3DVIEWPORT2));
	viewport_data.dwSize = sizeof(D3DVIEWPORT2);
	viewport_data.dwX = 0;
	viewport_data.dwY = 0;
	viewport_data.dwWidth = display_width;
	viewport_data.dwHeight = display_height;
	viewport_data.dvClipX = 0.0f;
	viewport_data.dvClipY = 0.0f;
	viewport_data.dvClipWidth = (float)display_width;
	viewport_data.dvClipHeight = (float)display_height;
	viewport_data.dvMinZ = 0.0f;
	viewport_data.dvMaxZ = 1.0f;

	// Add the viewport to the 3D device, make it the current viewport, then
	// set the viewport position and size.

	if (d3d_device_ptr->AddViewport(d3d_viewport_ptr) != D3D_OK ||
		d3d_device_ptr->SetCurrentViewport(d3d_viewport_ptr) != D3D_OK ||
		d3d_viewport_ptr->SetViewport2(&viewport_data) != D3D_OK) {
		failed_to_set("viewport");
		return(false);
	}
	
	// Set the render state.

	if (!set_render_state(D3DRENDERSTATE_ZENABLE, TRUE) ||
		!set_render_state(D3DRENDERSTATE_SUBPIXEL, TRUE) ||
		!set_render_state(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL) ||
		!set_render_state(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE) ||
		!set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE) ||
		!set_render_state(D3DRENDERSTATE_TEXTUREMAPBLEND, 
						  D3DTBLEND_MODULATEALPHA) ||
		!set_render_state(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA) ||
		!set_render_state(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA) ||
		!set_render_state(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR) ||
		!set_render_state(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_LINEAR) ||
		!set_render_state(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE)) {
		failed_to_set("render state");
		return(false);
	}

	// Create a DirectDraw texture surface in system memory for each supported
	// image size, and obtain the Direct3D texture interfaces for each.

	for (size_index = 0; size_index < IMAGE_SIZES; size_index++) {
		image_dimensions = image_dimensions_list[size_index];
		memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
		ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
		ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT |
			DDSD_PIXELFORMAT;
		ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | 
			DDSCAPS_SYSTEMMEMORY;
		ddraw_surface_desc.dwWidth = image_dimensions;
		ddraw_surface_desc.dwHeight = image_dimensions;
		ddraw_surface_desc.ddpfPixelFormat = ddraw_texture_pixel_format;
		if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc, 
			&ddraw_texture_list[size_index], NULL) != DD_OK) {
			failed_to_create("texture surface");
			return(false);
		}
		if (ddraw_texture_list[size_index]->QueryInterface(IID_IDirect3DTexture2, 
			(void **)&d3d_texture_list[size_index]) != DD_OK) {
			failed_to_get("texture interface");
			return(false);
		}
	}

	// Return a success status.

	return(true);
}

//------------------------------------------------------------------------------
// Shut down the 3D interface.
//------------------------------------------------------------------------------

static void
shut_down_3D_interface(void)
{
	int size_index;

	for (size_index = 0; size_index < IMAGE_SIZES; size_index++) {
		if (d3d_texture_list[size_index])
			d3d_texture_list[size_index]->Release();
		if (ddraw_texture_list[size_index])
			ddraw_texture_list[size_index]->Release();
	}
	if (d3d_viewport_ptr)
		d3d_viewport_ptr->Release();
	if (d3d_device_ptr)
		d3d_device_ptr->Release();
	if (zbuffer_surface_ptr)
		zbuffer_surface_ptr->Release();
	if (framebuf_surface_ptr) {
		framebuf_surface_ptr->Release();
		framebuf_surface_ptr = NULL;
	}
	if (primary_surface_ptr) {
		if (clipper_ptr) {
			primary_surface_ptr->SetClipper(NULL);
			clipper_ptr->Release();
			clipper_ptr = NULL;
		}
		primary_surface_ptr->Release();
		primary_surface_ptr = NULL;
	}
	if (d3d_object_ptr)
		d3d_object_ptr->Release();
}

#ifdef SUPPORT_A3D

//------------------------------------------------------------------------------
// Start up the A3D sound system.
//------------------------------------------------------------------------------

bool
start_up_A3D(void)
{
	// Create the A3D4 object.

	if (FAILED(A3dCreate(NULL, (void **)&a3d_object_ptr, NULL, A3D_OCCLUSIONS | 
		A3D_1ST_REFLECTIONS | A3D_DISABLE_SPLASHSCREEN)))
		return(false);

	// Set the cooperative level and maximum reflection delay time.

	if (FAILED(a3d_object_ptr->SetCooperativeLevel(main_window_handle, 
		A3D_CL_NORMAL)))
		return(false);
	a3d_object_ptr->SetMaxReflectionDelayTime(1.5f);

	// Create and initialise the A3D geometry object.

	if (FAILED(a3d_object_ptr->QueryInterface(IID_IA3dGeom, 
		(void **)&a3d_geometry_ptr)))
		return(false);

	// Enable occlusions.

	a3d_geometry_ptr->Enable(A3D_OCCLUSIONS);

	// Enable reflections if necessary, and set the reflection delay scale.

	start_atomic_operation();
	reflections_on = reflections_enabled;
	end_atomic_operation();
	if (reflections_on)
		a3d_geometry_ptr->Enable(A3D_1ST_REFLECTIONS);
	a3d_geometry_ptr->SetReflectionDelayScale(1.5f);

	// Create the A3D material object.

	if (FAILED(a3d_geometry_ptr->NewMaterial(&a3d_material_ptr)))
		return(false);
	a3d_material_ptr->SetTransmittance(0.5f, 0.5f);
	a3d_material_ptr->SetReflectance(0.75f, 0.75f);

	// Create the A3D listener object.

	return(SUCCEEDED(a3d_object_ptr->QueryInterface(IID_IA3dListener,
		(void **)&a3d_listener_ptr)));
}

//------------------------------------------------------------------------------
// Shut down the A3D sound system.
//------------------------------------------------------------------------------

void
shut_down_A3D(void)
{
	if (a3d_listener_ptr) {
		a3d_listener_ptr->Release();
		a3d_listener_ptr = NULL;
	}
	if (a3d_material_ptr) {
		a3d_material_ptr->Release();
		a3d_material_ptr = NULL;
	}
	if (a3d_geometry_ptr) {
		a3d_geometry_ptr->Release();
		a3d_geometry_ptr = NULL;
	}
	if (a3d_object_ptr) {
		a3d_object_ptr->Release();
		a3d_object_ptr = NULL;
	}
}

#else // !SUPPORT_A3D

//------------------------------------------------------------------------------
// Start up the DirectSound system.
//------------------------------------------------------------------------------

bool
start_up_DirectSound(void)
{
	// Create the DirectSound object.

	if (DirectSoundCreate(NULL, &dsound_object_ptr, NULL) != DD_OK)
		return(false);

	// Set the cooperative level for this application.

	return(dsound_object_ptr->SetCooperativeLevel(main_window_handle,
		DSSCL_NORMAL) == DD_OK);
}

//------------------------------------------------------------------------------
// Shut down the DirectSound system.
//------------------------------------------------------------------------------

void
shut_down_DirectSound(void)
{
	if (dsound_object_ptr) {
		dsound_object_ptr->Release();
		dsound_object_ptr = NULL;
	}
}

#endif

//------------------------------------------------------------------------------
// Determine if the required sound system is available.
//------------------------------------------------------------------------------

static void
determine_sound_system(void)
{
#ifdef SUPPORT_A3D

	// By default, reflection aren't available.

	reflections_available = false;

	// Attempt to start up A3D; if this succeeds, determine if reflections are
	// available, then shut A3D down and indicate A3D sound is available.

	if (SUCCEEDED(A3dCreate(NULL, (void **)&a3d_object_ptr, NULL,
		A3D_OCCLUSIONS | A3D_1ST_REFLECTIONS | A3D_DISABLE_SPLASHSCREEN))) {
		if (a3d_object_ptr->IsFeatureAvailable(A3D_1ST_REFLECTIONS))
			reflections_available = true;
		a3d_object_ptr->Release();
		a3d_object_ptr = NULL;
		sound_system = A3D_SOUND;
		return;
	}

#else

	// Attempt to start up DirectSound; if this succeeds, shut it down and
	// indicate DirectSound is available.

	if (SUCCEEDED(DirectSoundCreate(NULL, &dsound_object_ptr, NULL))) {
		dsound_object_ptr->Release();
		dsound_object_ptr = NULL;
		sound_system = DIRECT_SOUND;
		return;
	}

#endif

	// Indicate no sound system is available.

	sound_system = NO_SOUND;
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
	EnterCriticalSection(&critical_section);
}

//------------------------------------------------------------------------------
// End an atomic operation.
//------------------------------------------------------------------------------

void
end_atomic_operation(void)
{
	LeaveCriticalSection(&critical_section);
}

//==============================================================================
// Plugin window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Paint splash graphic and window text on the given window.
//------------------------------------------------------------------------------

static void
paint_splash_graphic(HWND window_handle, HDC hdc, int window_width,
					 int window_height, const char *window_text)
{
	// Draw the splash graphic if it exists and is smaller than the window
	// and there is enough room for the window text.

	if (splash_bitmap_ptr && 
		splash_bitmap_ptr->width <= window_width &&
		splash_bitmap_ptr->height <= window_height - TASK_BAR_HEIGHT) {
		HDC splash_hdc;
		HBITMAP old_bitmap_handle;
		int x, y;
		RECT rect;

		// Determine the position to draw the splash graphic so that it's
		// centered in the window.

		x = (window_width - splash_bitmap_ptr->width) / 2;
		y = (window_height - TASK_BAR_HEIGHT - splash_bitmap_ptr->height) / 2;

		// Create a device context for the splash graphic.

		splash_hdc = CreateCompatibleDC(hdc);
		old_bitmap_handle = SelectBitmap(splash_hdc, splash_bitmap_ptr->handle);

		// Copy the splash bitmap onto the window.

		BitBlt(hdc, x, y, splash_bitmap_ptr->width, 
			splash_bitmap_ptr->height, splash_hdc, 0, 0, SRCCOPY);

		// Select the default bitmap into the device context before
		// deleting it.

		SelectBitmap(splash_hdc, old_bitmap_handle);
		DeleteDC(splash_hdc);

		// Draw the window text centered below the splash graphic.

		rect.left = 0;
		rect.top = y + splash_bitmap_ptr->height;
		rect.right = window_width;
		rect.bottom = rect.top + TASK_BAR_HEIGHT;
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0xff, 0xcc, 0x66));
		DrawText(hdc, window_text, strlen(window_text), &rect, 
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
}

//------------------------------------------------------------------------------
// Inactive plugin window procedure.
//------------------------------------------------------------------------------

static LRESULT CALLBACK
handle_inactive_window_event(HWND window_handle, UINT message, WPARAM wParam, 
							 LPARAM lParam)
{
	WNDPROC prev_wndproc_ptr;
	void *window_data_ptr;
	const char *window_text;
	inactive_callback callback_ptr;
	PAINTSTRUCT paintStruct;
	HDC hdc;
	RECT window_rect;

	// Get pointers to the previous window procedure, the window data, the
	// window text, and the callback procedure.

	prev_wndproc_ptr = (WNDPROC)GetProp(window_handle, "prev_wndproc_ptr");
	window_data_ptr = (void *)GetProp(window_handle, "window_data_ptr");
	window_text = (const char *)GetProp(window_handle, "window_text");
	callback_ptr = (inactive_callback)GetProp(window_handle, "callback_ptr");

	// Handle the message...

	switch (message) {

	// Handle the paint message by drawing the splash graphic and the window
	// text on a black background.  If the window is too small for the splash 
	// graphic, it isn't drawn.

	case WM_PAINT:
		hdc = BeginPaint(window_handle, &paintStruct);
		GetClientRect(window_handle, &window_rect);
		FillRect(hdc, &window_rect, GetStockBrush(BLACK_BRUSH));
		paint_splash_graphic(window_handle, hdc, window_rect.right,
			window_rect.bottom, window_text);
		EndPaint(window_handle, &paintStruct);
		break;

	// Handle the mouse activate message by calling the callback procedure.

	case WM_MOUSEACTIVATE:
		(*callback_ptr)(window_data_ptr);
		break;
		
	// Handle all other messages by sending to the previous window procedure.

	default:
		return(CallWindowProc(prev_wndproc_ptr, window_handle, message, wParam,
			lParam));
	}
	return(0);
}

//------------------------------------------------------------------------------
// Make a plugin window inactive.
//------------------------------------------------------------------------------

void
set_plugin_window(void *window_handle, void *window_data_ptr,
				  const char *window_text, inactive_callback window_callback)
{
	// Save pointers to the current window procedure, the window data, the
	// window text, and the callback procedure as window properties.

	SetProp((HWND)window_handle, "prev_wndproc_ptr",
		(HANDLE)GetWindowLong((HWND)window_handle, GWL_WNDPROC));
	SetProp((HWND)window_handle, "window_data_ptr", (HANDLE)window_data_ptr);
	SetProp((HWND)window_handle, "callback_ptr", (HANDLE)window_callback);
	SetProp((HWND)window_handle, "window_text", (HANDLE)window_text);

	// Set the window procedure.

	SetWindowLong((HWND)window_handle, GWL_WNDPROC,
		(LONG)handle_inactive_window_event);
	
	// Update the window.

	InvalidateRect((HWND)window_handle, NULL, TRUE);
	UpdateWindow((HWND)window_handle);
}

//------------------------------------------------------------------------------
// Restore a plugin window.
//------------------------------------------------------------------------------

void
restore_plugin_window(void *window_handle)
{
	// Recover the previous window procedure for this plugin window.

	SetWindowLong((HWND)window_handle, GWL_WNDPROC,
		(LONG)GetProp((HWND)window_handle, "prev_wndproc_ptr"));

	// Remove the properties on the window.

	RemoveProp((HWND)window_handle, "prev_wndproc_ptr");
	RemoveProp((HWND)window_handle, "window_data_ptr");
	RemoveProp((HWND)window_handle, "callback_ptr");
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
	OSVERSIONINFO os_version_info;
	static char app_path[_MAX_PATH];
	char *app_name;
	static char version_number[16];
	WNDCLASS window_class;
	int icon_x, index;

	// Initialise global variables.

	instance_handle = NULL;
	logo_icon_ptr = NULL;
	left_edge_icon_ptr = NULL;
	right_edge_icon_ptr = NULL;
	title_bg_icon_ptr = NULL;
	title_end_icon_ptr = NULL;
	for (index = 0; index < TASK_BAR_BUTTONS; index++)
		button_icon_list[index] = NULL;
	splash_texture_ptr = NULL;
	splash_bitmap_ptr = NULL;
#ifdef SUPPORT_A3D
	A3D_splash_texture_ptr = NULL;
#endif

	// Determine which OS we are running under.

	os_version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os_version_info);
	switch (os_version_info.dwPlatformId) {
	case VER_PLATFORM_WIN32s:
		os_version = "Windows 3.1 (Win32s)";
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		if (os_version_info.dwMinorVersion == 0)
			os_version = "Windows 95";
		else if (os_version_info.dwMinorVersion == 10)
			os_version = "Windows 98";
		else
			os_version = "Windows XX";
		sprintf(version_number, " (%d.%d.%d)", os_version_info.dwMajorVersion,
			os_version_info.dwMinorVersion, 
			os_version_info.dwBuildNumber & 0xffff);
		os_version += version_number;
		break;
	case VER_PLATFORM_WIN32_NT:
		os_version = "Windows NT";
		sprintf(version_number, " (%d.%d.%d)", os_version_info.dwMajorVersion,
			os_version_info.dwMinorVersion, os_version_info.dwBuildNumber);
		os_version += version_number;
	}

	// Get the instance handle.
	
	if ((instance_handle = GetModuleHandle("NPRover.dll")) == NULL)
		return(false);

	// Determine the application path.

	GetModuleFileName(instance_handle, app_path, _MAX_PATH);
	app_name = strrchr(app_path, '\\') + 1;
	*app_name = '\0';
	app_dir = app_path;

	// Initialise the critical section object.

	InitializeCriticalSection(&critical_section);

	// Get handles to all of the required cursors.

	for (index = 0; index < MOVEMENT_CURSORS; index++)
		movement_cursor_handle_list[index] = LoadCursor(instance_handle, 
			MAKEINTRESOURCE(movement_cursor_ID_list[index]));
	arrow_cursor_handle = LoadCursor(NULL, IDC_ARROW);
	hand_cursor_handle = LoadCursor(instance_handle, MAKEINTRESOURCE(IDC_HAND));
	crosshair_cursor_handle = LoadCursor(NULL, IDC_CROSS);	

	// Register the main window class.

	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = instance_handle;
	window_class.lpfnWndProc = DefWindowProc;
	window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	window_class.hCursor = arrow_cursor_handle;
	window_class.hbrBackground = GetStockBrush(BLACK_BRUSH);
	window_class.lpszMenuName = NULL;
	window_class.lpszClassName = "MainWindow";
	if (!RegisterClass(&window_class)) {
		failed_to("register main window class");
		return(false);
	}

	// Reset flag indicating whether the main window is ready.

	main_window_ready = false;

	// Initialise the main window, browser window and options window handles.

	main_window_handle = NULL;
	browser_window_handle = NULL;
	options_window_handle = NULL;

	// Initialise the common control DLL.

	InitCommonControls();

	// Initialise the COM library.

	CoInitialize(NULL);

	// Determine the render mode that is available.  

	render_mode = SOFTWARE;
	if (DirectDrawEnumerate(ddraw_callback, (VOID *)&render_mode) == DD_OK &&
		render_mode != SOFTWARE) {
		ddraw_object_ptr->Release();
		ddraw_object_ptr = NULL;
	}

	// Determine which sound system (if any) is available, and whether
	// reflections are also available.

	determine_sound_system();
	
	// If the display depth is less than 8 bits, we cannot support it.

	HDC screen = CreateIC("Display", NULL, NULL, NULL);
	display_depth = GetDeviceCaps(screen, BITSPIXEL);
	DeleteDC(screen);
	if (display_depth < 8) {
		fatal_error("Unsupported colour mode", "The Flatland Rover does not "
			"support 16-color displays.\n\nTo view this 3DML document, change "
			"your display setting in the Display control panel,\nthen click on "
			"the RELOAD or REFRESH button of your browser.\nSome versions of "
			"Windows may require you to reboot your PC first.");
		return(false);
	}

	// Load the splash graphic GIF resource as a texture and bitmap, and the
	// A3D splash graphic GIF resource as a texture.

	if ((splash_texture_ptr = load_GIF_resource(IDR_SPLASH)) == NULL ||
		(splash_bitmap_ptr = texture_to_bitmap(splash_texture_ptr)) == NULL)
		return(false);
#ifdef SUPPORT_A3D
	if ((A3D_splash_texture_ptr = load_GIF_resource(IDR_A3D_SPLASH)) == NULL)
		return(false);
#endif

	// Load the task bar icon textures.

	if ((logo_icon_ptr = load_icon(IDR_LOGO0, IDR_LOGO1)) == NULL ||
		(left_edge_icon_ptr = load_icon(IDR_LEFT, 0)) == NULL ||
		(right_edge_icon_ptr = load_icon(IDR_RIGHT, 0)) == NULL || 
		(title_bg_icon_ptr = load_icon(IDR_TITLE_BG, 0)) == NULL ||
		(title_end_icon_ptr = load_icon(IDR_TITLE_END, 0)) == NULL)
		return(false);
	for (index = 0; index < TASK_BAR_BUTTONS; index++)
		if ((button_icon_list[index] = load_icon(button0_ID_list[index],
			 button1_ID_list[index])) == NULL)
			 return(false);

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

	// Return sucess status.

	return(true);
}

//------------------------------------------------------------------------------
// Shut down the platform API.
//------------------------------------------------------------------------------

void
shut_down_platform_API(void)
{
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

	// Delete the splash graphic texture and bitmap, and the A3D splash
	// graphic texture.

	if (splash_texture_ptr)
		DEL(splash_texture_ptr, texture);
	if (splash_bitmap_ptr)
		DEL(splash_bitmap_ptr, bitmap);
#ifdef SUPPORT_A3D
	if (A3D_splash_texture_ptr)
		DEL(A3D_splash_texture_ptr, texture);
#endif

	// Delete the critical section object.

	DeleteCriticalSection(&critical_section);

	// Unregister the main window class.

	if (instance_handle)
		UnregisterClass("MainWindow", instance_handle);

	// Uninitialize the COM library.

	CoUninitialize();
}

//------------------------------------------------------------------------------
// Get the process ID of this application.
//------------------------------------------------------------------------------
		
int
get_process_ID(void)
{
	return(GetCurrentProcessId());
}

//------------------------------------------------------------------------------
// Start the player thread.
//------------------------------------------------------------------------------

bool
start_player_thread(void)
{
	player_thread_handle = (HANDLE)_beginthread(player_thread, 0, NULL);
	streaming_thread_handle = -1;
	return(player_thread_handle >= 0);
}

//------------------------------------------------------------------------------
// Wait for player thread termination.
//------------------------------------------------------------------------------

void
wait_for_player_thread_termination(void)
{
	WaitForSingleObject(player_thread_handle, INFINITE);
}

//==============================================================================
// Main window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to handle a main window mouse activation message.
//------------------------------------------------------------------------------

static int
handle_activate_message(HWND window_handle, HWND toplevel_window_handle,
					    UINT code_hit_test, UINT msg)
{
	// Set the keyboard focus to this window.

	SetFocus(window_handle);
	return(MA_ACTIVATE);
}

//------------------------------------------------------------------------------
// Function to handle key events in the main window.
//------------------------------------------------------------------------------

static void
handle_key_event(HWND window_handle, UINT virtual_key, BOOL key_down, 
				 int repeat, UINT flags)
{
	int key_code;

	// If there is no key callback function, do nothing.

	if (key_callback_ptr == NULL)
		return;

	// If the virtual key is a letter, pass it to the callback function.

	if (virtual_key >= 'A' && virtual_key <= 'Z')
		(*key_callback_ptr)(virtual_key, key_down ? true : false);

	// Otherwise convert the virtual key code to a platform independent key
	// code, and pass it to the callback function.

	else {
		switch (virtual_key) {
		case VK_SHIFT:
			key_code = SHIFT_KEY;
			break;
		case VK_CONTROL:
			key_code = CONTROL_KEY;
			break;
		case VK_UP:
			key_code = UP_KEY;
			break;
		case VK_DOWN:
			key_code = DOWN_KEY;
			break;
		case VK_LEFT:
			key_code = LEFT_KEY;
			break;
		case VK_RIGHT:
			key_code = RIGHT_KEY;
			break;
		case VK_ADD:
			key_code = ADD_KEY;
			break;
		case VK_SUBTRACT:
			key_code = SUBTRACT_KEY;
			break;
		case VK_INSERT:
			key_code = INSERT_KEY;
			break;
		case VK_DELETE:
			key_code = DELETE_KEY;
			break;
		case VK_PRIOR:
			key_code = PAGEUP_KEY;
			break;
		case VK_NEXT:
			key_code = PAGEDOWN_KEY;
			break;
		default:
			key_code = 0;
		}
		if (key_code != 0)
			(*key_callback_ptr)(key_code, key_down ? true : false);
	}
}

//------------------------------------------------------------------------------
// Function to handle mouse events in the main window.
//------------------------------------------------------------------------------

static void
handle_mouse_event(HWND window_handle, UINT message, short x, short y,
				   UINT flags)
{
	bool logo_active_flag;
	int selected_button_index;
	int task_bar_button_code;

	// If running full-screen, adjust the mouse coordinates by the width and
	// height factors.

	if (full_screen) {
		x = (int)((float)x * width_factor);
		y = (int)((float)y * width_factor);
	}

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

	// Send the mouse data to the callback function, if there is one.

	if (mouse_callback_ptr == NULL)
		return;
	switch (message) {
	case WM_MOUSEMOVE:
		(*mouse_callback_ptr)(x, y, MOUSE_MOVE_ONLY, task_bar_button_code);
		break;
	case WM_LBUTTONDOWN:
		(*mouse_callback_ptr)(x, y, LEFT_BUTTON_DOWN, task_bar_button_code);
		break;
	case WM_LBUTTONUP:
		(*mouse_callback_ptr)(x, y, LEFT_BUTTON_UP, task_bar_button_code);
		break;
	case WM_RBUTTONDOWN:
		(*mouse_callback_ptr)(x, y, RIGHT_BUTTON_DOWN, task_bar_button_code);
		break;
	case WM_RBUTTONUP:
		(*mouse_callback_ptr)(x, y, RIGHT_BUTTON_UP, task_bar_button_code);
	}
}

//------------------------------------------------------------------------------
// Function to handle events in the main window.
//------------------------------------------------------------------------------

static LRESULT CALLBACK
handle_main_window_event(HWND window_handle, UINT message, WPARAM wParam,
						 LPARAM lParam)
{
	WNDPROC prev_wndproc_ptr;

	// Get the pointer to the previous window procedure.

	prev_wndproc_ptr = (WNDPROC)GetProp(window_handle, "prev_wndproc_ptr");

	// Handle the message.
 
	switch(message) {
	HANDLE_MSG(window_handle, WM_MOUSEACTIVATE, handle_activate_message);
	HANDLE_MSG(window_handle, WM_KEYDOWN, handle_key_event);
	HANDLE_MSG(window_handle, WM_KEYUP, handle_key_event);
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		HANDLE_MOUSE_MSG(window_handle, message, handle_mouse_event);
		break;
	case WM_TIMER:
		if (timer_callback_ptr)
			(*timer_callback_ptr)();
		break;
	case WM_SIZE:
		if (resize_callback_ptr)
			(*resize_callback_ptr)(window_handle, LOWORD(lParam),
				HIWORD(lParam));
		break;
	default:
		return(CallWindowProc(prev_wndproc_ptr, window_handle, message, wParam,
			lParam));
	}
	return(0);
}

//------------------------------------------------------------------------------
// Resize the main window.
//------------------------------------------------------------------------------

void
set_main_window_size(int width, int height)
{
	// Remember the new window size.

	display_width = width;
	display_height = height;
	prev_window_width = width;
	prev_window_height = height;

#ifdef TASK_BAR

	// If the width of the main window is less than 320 or the height is less
	// than 240, disable the task bar and splash graphic.

	window_width = width;
	if (width < 320 || height < 240) {
		window_height = height;
		task_bar_enabled = false;
		splash_graphic_enabled = false;
	} else {
		window_height = height - TASK_BAR_HEIGHT;
		task_bar_enabled = true;
		splash_graphic_enabled = true;
	}
#else
	window_width = width;
	window_height = height;
	task_bar_enabled = false;
	if (width < 320 || height < 240)
		splash_graphic_enabled = false;
	else
		splash_graphic_enabled = true;
#endif

	// Reset the x coordinate of the end of the task bar title.

	title_end_x = window_width - title_end_icon_ptr->width;
}

//------------------------------------------------------------------------------
// Create the main window.
//------------------------------------------------------------------------------

bool
create_main_window(void *window_handle, int width, int height,
				   void (*key_callback)(int key_code, bool key_down),
				   void (*mouse_callback)(int x, int y, int button_code,
									      int task_bar_button_code),
				   void (*timer_callback)(void),
				   void (*resize_callback)(void *window_handle, int width,
										   int height))
{
	HWND desktop_window_handle;
	HWND parent_window_handle;
	int index;
	pixel red_comp_mask, green_comp_mask, blue_comp_mask, alpha_comp_mask;

	// Do nothing if the main window already exists.

	if (main_window_handle)
		return(false);

	// Initialise the global variables.

	ddraw_object_ptr = NULL;
	primary_surface_ptr = NULL;
	framebuf_surface_ptr = NULL;
	framebuf_ptr = NULL;
	clipper_ptr = NULL;
	label_visible = false;
	d3d_object_ptr = NULL;
	d3d_device_ptr = NULL;
	d3d_viewport_ptr = NULL;
	zbuffer_surface_ptr = NULL;
	curr_hardware_texture_ptr = NULL;
	for (index = 0; index < IMAGE_SIZES; index++) {
		ddraw_texture_list[index] = NULL;
		d3d_texture_list[index] = NULL;
	}
#ifdef SUPPORT_A3D
	a3d_object_ptr = NULL;
	a3d_geometry_ptr = NULL;
	a3d_material_ptr = NULL;
	a3d_listener_ptr = NULL;
#else
	dsound_object_ptr = NULL;
#endif
	progress_window_handle = NULL;	
	light_window_handle = NULL;
	history_menu_handle = NULL;
	message_log_window_handle = NULL;
	active_button_index = -1;
	prev_active_button_index = -1;
	logo_active = false;
	prev_logo_active = false;
	key_callback_ptr = NULL;
	mouse_callback_ptr = NULL;
	timer_callback_ptr = NULL;
	resize_callback_ptr = NULL;

	// If the render mode is software, disable hardware acceleration.

	if (render_mode == SOFTWARE)
		hardware_acceleration = false;

	// If hardware acceleration is enabled, and render mode is hardware
	// full-screen, set the full screen flag.

	if (hardware_acceleration && render_mode == HARDWARE_FULLSCREEN)
		full_screen = true;
	else
		full_screen = false;

	// If running full-screen, choose a display resolution of 640x480x16, and
	// create a top-most popup window that covers the entire desktop.

	if (full_screen) {
		width = 640;
		height = 480;
		display_width = GetSystemMetrics(SM_CXSCREEN);
		display_height = GetSystemMetrics(SM_CYSCREEN);
		display_depth = 16;
		if ((main_window_handle = CreateWindowEx(WS_EX_TOPMOST, "MainWindow",
			"Main Window", WS_POPUP, 0, 0, display_width, display_height,
			NULL, NULL, instance_handle, NULL))
			== NULL)
			return(false);
		width_factor = (float)width / (float)display_width;
		height_factor = (float)height / (float)display_height;
	}

	// If running in a window, use the plugin window as the main window.

	else {
		HDC screen = CreateIC("Display", NULL, NULL, NULL);
		display_depth = GetDeviceCaps(screen, BITSPIXEL);
		DeleteDC(screen);
		main_window_handle = (HWND)window_handle;

		// If the main window has zero width and height (due to a nasty bug in
		// Internet Explorer when a local spot is refreshed), resize the window 
		// to what it was in the previous Rover session.

		if (width == 0 || height == 0) {
			POINT pos;

			pos.x = 0;
			pos.y = 0;
			ClientToScreen(main_window_handle, &pos);
			width = prev_window_width;
			height = prev_window_height;
			MoveWindow(main_window_handle, pos.x, pos.y, width, height, false);
		}
	}

	// Search for the browser window, which is a parent of the main window
	// without a parent itself, or with the desktop window as a parent.

	desktop_window_handle = GetDesktopWindow();
	browser_window_handle = main_window_handle;
	while ((parent_window_handle = GetParent(browser_window_handle)) != NULL &&
		parent_window_handle != desktop_window_handle)
		browser_window_handle = parent_window_handle;

	// Save the current window procedure as a window property, then subclass
	// the window to use the main window event handler.

	SetProp(main_window_handle, "prev_wndproc_ptr",
		(HANDLE)GetWindowLong(main_window_handle, GWL_WNDPROC));
	SetWindowLong(main_window_handle, GWL_WNDPROC,
		(LONG)handle_main_window_event);

	// Set the main window size.

	set_main_window_size(width, height);

	// Make sure the main window is displayed if running full screen.

	if (full_screen) {
		ShowWindow(main_window_handle, SW_NORMAL);
		InvalidateRect(main_window_handle, NULL, TRUE);
		UpdateWindow(main_window_handle);
	}

	// Set the input focus to the main window, and set a timer to go off 33
	// times a second.

	SetFocus(main_window_handle);
	SetTimer(main_window_handle, 1, 30, NULL);

	// Save the pointers to the callback functions.

	key_callback_ptr = key_callback;
	mouse_callback_ptr = mouse_callback;
	timer_callback_ptr = timer_callback;
	resize_callback_ptr = resize_callback;

	// Initialise the dither table pointers.

	for (index = 0; index < 4; index++)
		dither_table[index] = NULL;

	// Initialise the light table pointers.

	for (index = 0; index < BRIGHTNESS_LEVELS; index++)
		light_table[index] = NULL;

	// Create the standard RGB colour palette.

	if (!create_standard_palette())
		return(false);

	// Create the DirectDraw object.  If hardware acceleration is not enabled,
	// use the primary DirectDraw device, otherwise use the hardware device.

	if (hardware_acceleration) {
		int dummy_render_mode = SOFTWARE;
		if (DirectDrawEnumerate(ddraw_callback, (VOID *)&dummy_render_mode) 
			!= DD_OK || dummy_render_mode == SOFTWARE)
			return(false);
	} else { 
		if (DirectDrawCreate(NULL, &ddraw_object_ptr, NULL) != DD_OK)
			return(false);
	}

	// If running full-screen, set the exclusive full-screen cooperative level
	// and set the display mode.  Otherwise, set the normal cooperative level.

	if (full_screen) {
		if (ddraw_object_ptr->SetCooperativeLevel(main_window_handle,
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) != DD_OK ||
			ddraw_object_ptr->SetDisplayMode(display_width, display_height,
				display_depth) != DD_OK)
			return(false);
	} else {
		if (ddraw_object_ptr->SetCooperativeLevel(main_window_handle,
			DDSCL_NORMAL) != DD_OK)
			return(false);
	}

	// Create the frame buffer.

	if (!create_frame_buffer())
		return(false);

	// The texture pixel format is 1555 ARGB.

	set_pixel_format(&texture_pixel_format, 0x7c00, 0x03e0, 0x001f, 0x8000);

	// If the display has a depth of 8 bits...

	if (display_depth == 8) {

		// Create the dither tables needed to convert from 16-bit pixels to
		// 8-bit pixels using the palette.

		if (!create_dither_tables())
			return(false);

		// Use a display pixel format of 1555 ARGB for the frame buffer, which
		// will be dithered to the 8-bit display.

		set_pixel_format(&display_pixel_format, 0x7c00, 0x03e0, 0x001f, 0x8000);
	}
	
	// If the display has a depth of 16, 24 or 32 bits...

	else {
		DDSURFACEDESC ddraw_surface_desc;

		// Get the colour component masks from the primary surface.

		ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
		ddraw_surface_desc.dwFlags = DDSD_PIXELFORMAT;
		if (primary_surface_ptr->GetSurfaceDesc(&ddraw_surface_desc) != DD_OK)
			return(false);
		red_comp_mask = (pixel)ddraw_surface_desc.ddpfPixelFormat.dwRBitMask;
		green_comp_mask = (pixel)ddraw_surface_desc.ddpfPixelFormat.dwGBitMask;
		blue_comp_mask = (pixel)ddraw_surface_desc.ddpfPixelFormat.dwBBitMask;

		// Determine the alpha component mask...

		switch (display_depth) {

		// If the display depth is 16 bits, and one of the components uses 6
		// bits, then reduce it to 5 bits and use the least significant bit of
		// the component as the alpha component mask.  If the pixel format is 
		// 555, then use the most significant bit of the pixel as the alpha
		// component mask.

		case 16:
			if (red_comp_mask == 0xfc00) {
				red_comp_mask = 0xf800;
				alpha_comp_mask = 0x0400;
			} else if (green_comp_mask == 0x07e0) {
				green_comp_mask = 0x07c0;
				alpha_comp_mask = 0x0020;
			} else if (blue_comp_mask == 0x003f) {
				blue_comp_mask = 0x003e;
				alpha_comp_mask = 0x0001;
			} else
				alpha_comp_mask = 0x8000;
			break;

		// If the display depth is 24 bits, reduce the green component to 7 bits
		// and use the least significant green bit as the alpha component mask.

		case 24:
			green_comp_mask = 0x0000fe00;
			alpha_comp_mask = 0x00000100;
			break;
		
		// If the display depth is 32 bits, use the upper 8 bits as the alpha
		// component mask.
		
		case 32:
			alpha_comp_mask = 0xff000000;
		}

		// Set the display pixel format.

		set_pixel_format(&display_pixel_format, red_comp_mask, green_comp_mask,
			blue_comp_mask, alpha_comp_mask);
	}

	// Set the video pixel format to be 555 RGB i.e. the same as the texture
	// format, but with no alpha channel.

	ddraw_video_pixel_format.dwSize = sizeof(DDPIXELFORMAT);
	ddraw_video_pixel_format.dwFlags = DDPF_RGB;
	ddraw_video_pixel_format.dwRGBBitCount = 16;
	ddraw_video_pixel_format.dwRBitMask = 0x7c00;
	ddraw_video_pixel_format.dwGBitMask = 0x03e0;
	ddraw_video_pixel_format.dwBBitMask = 0x001f;

	// Create a mapping table from 

	// Convert the standard palette from RGB values to display pixels.

	for (index = 0; index < 256; index++)
		standard_palette[index] = 
			RGB_to_display_pixel(standard_RGB_palette[index]);

	// Create the light tables.

	if (!create_light_tables())
		return(false);

	// Create the palette index table or initialise the display palette list for
	// the splash graphic textures.

	if (display_depth == 8) {
		if (splash_texture_ptr)
			splash_texture_ptr->create_palette_index_table();
#ifdef SUPPORT_A3D
		if (A3D_splash_texture_ptr)
			A3D_splash_texture_ptr->create_palette_index_table();
#endif
	} else {
		if (splash_texture_ptr)
			splash_texture_ptr->create_display_palette_list();
#ifdef SUPPORT_A3D
		if (A3D_splash_texture_ptr)
			A3D_splash_texture_ptr->create_display_palette_list();
#endif
	}

	// Initialise the task bar icons.

	if (!init_icon(logo_icon_ptr) || !init_icon(left_edge_icon_ptr) ||
		!init_icon(right_edge_icon_ptr) || !init_icon(title_bg_icon_ptr) ||
		!init_icon(title_end_icon_ptr))
		return(false);
	for (index = 0; index < TASK_BAR_BUTTONS; index++)
		if (!init_icon(button_icon_list[index]))
			return(false);

	// Create all of the required cursors.

	for (index = 0; index < MOVEMENT_CURSORS; index++)
		movement_cursor_ptr_list[index] = 
			create_cursor(movement_cursor_handle_list[index]);
	arrow_cursor_ptr = create_cursor(arrow_cursor_handle);
	hand_cursor_ptr = create_cursor(hand_cursor_handle);
	crosshair_cursor_ptr = create_cursor(crosshair_cursor_handle);

	// Make the current cursor the arrow cursor.

	curr_cursor_ptr = arrow_cursor_ptr;

	// Start up sound system, if one is available.

#ifdef SUPPORT_A3D
	if (sound_system == A3D_SOUND && !start_up_A3D()) {
		shut_down_A3D();
		sound_system = NO_SOUND;
	}
#else
	if (sound_system == DIRECT_SOUND && !start_up_DirectSound()) {
		shut_down_DirectSound();
		sound_system = NO_SOUND;
	}
#endif

	// Indicate the main window is ready.

	main_window_ready = true;
	return(true);
}

//------------------------------------------------------------------------------
// Destroy the main window.
//------------------------------------------------------------------------------

void
destroy_main_window(void)
{
	int index;

	// Do nothing if the main window doesn't exist.

	if (main_window_handle == NULL)
		return;

	// Shut down sound system, if one was started up.

#ifdef SUPPORT_A3D
	if (sound_system == A3D_SOUND)
		shut_down_A3D();
#else
	if (sound_system == DIRECT_SOUND)
		shut_down_DirectSound();
#endif

	// Destroy the cursors.

	for (index = 0; index < MOVEMENT_CURSORS; index++)
		if (movement_cursor_ptr_list[index])
			delete movement_cursor_ptr_list[index];
	if (arrow_cursor_ptr)
		delete arrow_cursor_ptr;
	if (hand_cursor_ptr)
		delete hand_cursor_ptr;
	if (crosshair_cursor_ptr)
		delete crosshair_cursor_ptr;

	// Destroy the frame buffer (and related data).

	destroy_frame_buffer();

	// Delete the dither tables and the standard palette if the display used an
	// 8-bit colour depth.

	if (display_depth == 8) {
		for (index = 0; index < 4; index++) {
			if (dither_table[index])
				delete []dither_table[index];
		}
		DeletePalette(standard_palette_handle);
	}

	// Delete the light tables.

	for (index = 0; index < BRIGHTNESS_LEVELS; index++) {
		if (light_table[index])
			delete []light_table[index];
	}

	// Restore the display mode if running full-screen, then delete the
	// DirectDraw object.

	if (ddraw_object_ptr) {
		if (full_screen)
			ddraw_object_ptr->RestoreDisplayMode();
		ddraw_object_ptr->Release();
	}

	// Stop the timer.

	KillTimer(main_window_handle, 1);

	// Recover the previous window procedure, and remove the window property
	// that contained it.

	SetWindowLong(main_window_handle, GWL_WNDPROC,
		(LONG)GetProp(main_window_handle, "prev_wndproc_ptr"));
	RemoveProp(main_window_handle, "prev_wndproc_ptr");
	
	// Destroy the main window if running full screen, then reset the main
	// window handle and reset the main window ready flag.

	if (full_screen)
		DestroyWindow(main_window_handle);
	main_window_handle = NULL;
	main_window_ready = false;
}

//------------------------------------------------------------------------------
// Determine if the browser window is currently minimised.
//------------------------------------------------------------------------------

bool
browser_window_is_minimised(void)
{
	return(browser_window_handle != NULL && IsIconic(browser_window_handle));
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

	// Create a message string by parsing the variable argument list according
	// to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Display this message in a message box, using the exclamation icon.

	MessageBoxEx(NULL, message, title, 
		MB_TASKMODAL | MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, 
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
}

//------------------------------------------------------------------------------
// Function to display an informational message in a message box.
//------------------------------------------------------------------------------

void
information(char *title, char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];

	// Create a message string by parsing the variable argument list according
	// to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Display this message in a message box, using the information icon.

	MessageBoxEx(NULL, message, title, 
		MB_TASKMODAL | MB_OK | MB_ICONINFORMATION | MB_TOPMOST,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
}

//------------------------------------------------------------------------------
// Function to display a query in a message box, and to return the response.
//------------------------------------------------------------------------------

bool
query(char *title, bool yes_no_format, char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	int options, result;

	// Create a message string by parsing the variable argument list according
	// to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Display this message in a message box, using the question mark icon,
	// and return TRUE if the YES button was selected, FALSE otherwise.

	if (yes_no_format)
		options = MB_YESNO | MB_ICONQUESTION;
	else
		options = MB_OKCANCEL | MB_ICONINFORMATION;
	result = MessageBoxEx(NULL, message, title, options | MB_TOPMOST | 
		MB_TASKMODAL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	return(result == IDYES || result == IDOK);
}

//==============================================================================
// Progress window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Handle a progress window event.
//------------------------------------------------------------------------------

static BOOL CALLBACK
handle_progress_event(HWND window_handle, UINT message, WPARAM wParam,
					  LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		return(TRUE);
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDB_CANCEL)
			(*progress_callback_ptr)();
		return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Create the progress window.
//------------------------------------------------------------------------------

void
open_progress_window(int file_size, void (*progress_callback)(void),
					 char *format, ...)
{
	va_list arg_ptr;
	char message[BUFSIZ];
	HWND control_handle;

	// If the progress window is already open, do nothing.

	if (progress_window_handle != NULL)
		return;

	// Save the pointer to the progress callback function.

	progress_callback_ptr = progress_callback;

	// Create the progress message string by parsing the variable argument list
	// according to the contents of the format string.

	va_start(arg_ptr, format);
	vsprintf(message, format, arg_ptr);
	va_end(arg_ptr);

	// Create the progress window.

	progress_window_handle = CreateDialog(instance_handle,
		MAKEINTRESOURCE(IDD_PROGRESS), NULL, handle_progress_event);

	// Get the handle to the progress text static control, and set it's text.

	control_handle = GetDlgItem(progress_window_handle, IDC_PROGRESS_TEXT);
	SetWindowText(control_handle, message);

	// Get the handle to the file size static control, and set it's text.

	control_handle = GetDlgItem(progress_window_handle, IDC_FILE_SIZE);
	if (file_size > 0) {
		sprintf(message, "File Size:\t%d KB", file_size / 1024);
		SetWindowText(control_handle, message);
	} else
		SetWindowText(control_handle, "File Size:\tNot determined");

	// Get the handle to the progress bar control, and initialise it's range
	// to match the file size.

	progress_bar_handle = GetDlgItem(progress_window_handle, IDC_PROGRESS_BAR);
	SendMessage(progress_bar_handle, PBM_SETRANGE, 0, 
		MAKELPARAM(0, file_size / 1024));
	SendMessage(progress_bar_handle, PBM_SETPOS, 0, 0);

	// Show the progress window.

	ShowWindow(progress_window_handle, SW_NORMAL);
}

//------------------------------------------------------------------------------
// Update the progress window.
//------------------------------------------------------------------------------

void
update_progress_window(int file_pos)
{	
	char message[BUFSIZ];
	HWND control_handle;

	// Set the position of the progress bar.

	SendMessage(progress_bar_handle, PBM_SETPOS, file_pos / 1024, 0);

	// Update the file status.

	control_handle = GetDlgItem(progress_window_handle, IDC_FILE_STATUS);
	sprintf(message, "File Status:\t%d KB downloaded", file_pos / 1024);
	SetWindowText(control_handle, message);
}

//------------------------------------------------------------------------------
// Close the progress window.
//------------------------------------------------------------------------------

void
close_progress_window(void)
{
	if (progress_window_handle) {
		DestroyWindow(progress_window_handle);
		progress_window_handle = NULL;
	}
}

//==============================================================================
// Light window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to handle light control events.
//------------------------------------------------------------------------------

static BOOL CALLBACK
handle_light_event(HWND window_handle, UINT message, WPARAM wParam,
						   LPARAM lParam)
{
	int brightness;

	switch (message) {
	case WM_INITDIALOG:
		return(TRUE);
	case WM_VSCROLL:
		brightness = SendMessage(light_slider_handle, TBM_GETPOS, 0, 0);
		(*light_callback_ptr)((float)brightness);
		return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Open the light control.
//------------------------------------------------------------------------------

void
open_light_window(float brightness, void (*light_callback)(float brightness))
{
	// If the light window is already open, do nothing.

	if (light_window_handle != NULL)
		return;

	// Save the light callback function pointer.

	light_callback_ptr = light_callback;

	// Create the light window.

	light_window_handle = CreateDialog(instance_handle,
		MAKEINTRESOURCE(IDD_LIGHT), main_window_handle, 
		handle_light_event);

	// Get the handle to the slider control, and initialise it with the
	// current brightness setting.

	light_slider_handle = GetDlgItem(light_window_handle, IDC_SLIDER1);
	SendMessage(light_slider_handle, TBM_SETTICFREQ, 10, 0);
	SendMessage(light_slider_handle, TBM_SETRANGE, TRUE, MAKELONG(-100, 100));
	SendMessage(light_slider_handle, TBM_SETPOS, TRUE,
		(int)(-brightness * 100.f));

	// Show the light window.

	ShowWindow(light_window_handle, SW_NORMAL);
}

//------------------------------------------------------------------------------
// Close the light window.
//------------------------------------------------------------------------------

void
close_light_window(void)
{
	if (light_window_handle) {
		DestroyWindow(light_window_handle);
		light_window_handle = NULL;
	}
}

//==============================================================================
// Options window functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to handle option window events.
//------------------------------------------------------------------------------

static BOOL CALLBACK
handle_options_event(HWND window_handle, UINT message, WPARAM wParam,
					 LPARAM lParam)
{
	HDC dc_handle;
	int text_height;
	HWND control_handle;

	switch (message) {

	// Load the required fonts, and set the font for each control that needs
	// it.

	case WM_INITDIALOG:
		dc_handle = GetDC(window_handle);
		text_height = POINTS_TO_PIXELS(8);
		ReleaseDC(window_handle, dc_handle);
		bold_font_handle = CreateFont(-text_height, 0, 0, 0, FW_BOLD,
			FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			"MS Sans Serif");
		symbol_font_handle = CreateFont(-text_height, 0, 0, 0, FW_BOLD,
			FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			"Symbol");
		control_handle = GetDlgItem(window_handle, IDC_STATIC_BOLD1);
		SendMessage(control_handle, WM_SETFONT, (WPARAM)bold_font_handle, 0);
		control_handle = GetDlgItem(window_handle, IDC_STATIC_BOLD2);
		SendMessage(control_handle, WM_SETFONT, (WPARAM)bold_font_handle, 0);
		control_handle = GetDlgItem(window_handle, IDC_STATIC_BOLD3);
		SendMessage(control_handle, WM_SETFONT, (WPARAM)bold_font_handle, 0);
		control_handle = GetDlgItem(window_handle, IDC_STATIC_SYMBOL);
		SendMessage(control_handle, WM_SETFONT, (WPARAM)symbol_font_handle, 0);
		return(TRUE);
	case WM_DESTROY:
		DeleteObject(bold_font_handle);
		DeleteObject(symbol_font_handle);
		return(TRUE);
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDB_OK:
				(*options_callback_ptr)(OK_BUTTON, 1);
				break;
			case IDB_CANCEL:
				(*options_callback_ptr)(CANCEL_BUTTON, 1);
				break;
			case IDB_DOWNLOAD_SOUNDS:
				if (SendMessage(sound_control_handle, BM_GETCHECK, 0, 0) ==
					BST_CHECKED)
					(*options_callback_ptr)(DOWNLOAD_SOUNDS_CHECKBOX, 1);
				else
					(*options_callback_ptr)(DOWNLOAD_SOUNDS_CHECKBOX, 0);
				break;
#ifdef SUPPORT_A3D
			case IDB_REFLECTIONS:
				if (SendMessage(reflection_control_handle, BM_GETCHECK, 0, 0)
					== BST_CHECKED)
					(*options_callback_ptr)(ENABLE_REFLECTIONS_CHECKBOX, 1);
				else
					(*options_callback_ptr)(ENABLE_REFLECTIONS_CHECKBOX, 0);
				break;
#endif
			case IDB_3D_ACCELERATION:
				if (SendMessage(accel_control_handle, BM_GETCHECK, 0, 0) ==
					BST_CHECKED)
					(*options_callback_ptr)(ENABLE_3D_ACCELERATION_CHECKBOX, 1);
				else
					(*options_callback_ptr)(ENABLE_3D_ACCELERATION_CHECKBOX, 0);
				break;
			case IDB_CHECK_FOR_UPDATE:
				(*options_callback_ptr)(CHECK_FOR_UPDATE_BUTTON, 1);
				break;
#ifdef SUPPORT_A3D
			case IDB_A3D_WEB_SITE:
				(*options_callback_ptr)(A3D_WEB_SITE_BUTTON, 1);
				break;
#endif
			case IDB_VIEW_SOURCE:
				(*options_callback_ptr)(VIEW_SOURCE_BUTTON, 1);
			}
			break;
		case EN_CHANGE:
			(*options_callback_ptr)(VISIBLE_RADIUS_EDITBOX,
				SendMessage(updown_control_handle, UDM_GETPOS, 0, 0));
		}
		return(TRUE);
	case WM_VSCROLL:
		(*options_callback_ptr)(VISIBLE_RADIUS_EDITBOX,
			SendMessage(updown_control_handle, UDM_GETPOS, 0, 0));
		return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Open the options window.
//------------------------------------------------------------------------------

void
open_options_window(bool download_sounds_value, bool reflections_enabled_value,
					int visible_radius_value,
					void (*options_callback)(int option_ID, int option_value))
{
	// If the options window is already open, do nothing.

	if (options_window_handle != NULL)
		return;

	// Save the options callback pointer.

	options_callback_ptr = options_callback;

	// Create the options dialog box.

#ifdef SUPPORT_A3D
	options_window_handle = CreateDialog(instance_handle,
		MAKEINTRESOURCE(IDD_OPTIONS), main_window_handle,
		handle_options_event);
#else
	options_window_handle = CreateDialog(instance_handle,
		MAKEINTRESOURCE(IDD_OPTIONS_NOA3D), main_window_handle,
		handle_options_event);
#endif

	// Initialise the "download sounds" check box.

	sound_control_handle = GetDlgItem(options_window_handle, 
		IDB_DOWNLOAD_SOUNDS);
	SendMessage(sound_control_handle, BM_SETCHECK,
		download_sounds_value ? BST_CHECKED : BST_UNCHECKED, 0);

#ifdef SUPPORT_A3D

	// Initialise the "enable reflections" check box.

	reflection_control_handle = GetDlgItem(options_window_handle, 
		IDB_REFLECTIONS);
	if (sound_system != A3D_SOUND || !reflections_available)
		EnableWindow(reflection_control_handle, false);
	else
		SendMessage(reflection_control_handle, BM_SETCHECK,
			reflections_enabled_value ? BST_CHECKED : BST_UNCHECKED, 0);

#endif

	// Initialise the "enable 3D acceleration" check box.

	accel_control_handle = GetDlgItem(options_window_handle,
		IDB_3D_ACCELERATION);
	if (render_mode == SOFTWARE)
		EnableWindow(accel_control_handle, false);
	else
		SendMessage(accel_control_handle, BM_SETCHECK,
			hardware_acceleration ? BST_CHECKED : BST_UNCHECKED, 0);
	
	// Initialise the "viewing distance" edit box.

	edit_control_handle = GetDlgItem(options_window_handle, IDC_EDIT1);
	SendMessage(edit_control_handle, EM_SETLIMITTEXT, 2, 0);
	updown_control_handle = GetDlgItem(options_window_handle, IDC_UPDOWN1);
	SendMessage(updown_control_handle, UDM_SETBUDDY, 
		(WPARAM)edit_control_handle, 0);
	SendMessage(updown_control_handle, UDM_SETRANGE, 0, MAKELONG(99, 0));
	SendMessage(updown_control_handle, UDM_SETPOS, 0, 
		MAKELONG(visible_radius_value, 0));

	// Show the options window.

	ShowWindow(options_window_handle, SW_NORMAL);
}

//------------------------------------------------------------------------------
// Close the options window.
//------------------------------------------------------------------------------

void
close_options_window(void)
{
	if (options_window_handle) {
		DestroyWindow(options_window_handle);
		options_window_handle = NULL;
	}
}

//==============================================================================
// History menu functions.
//==============================================================================

//------------------------------------------------------------------------------
// Open the history menu.
//------------------------------------------------------------------------------

void
open_history_menu(history *history_list)
{
	int index;

	// If the history menu already exists, do nothing.

	if (history_menu_handle != NULL)
		return;
	
	// Create the popup menu, and add a "Return to entrance" menu item with
	// a separator below it.

	history_menu_handle = CreatePopupMenu();
	AppendMenu(history_menu_handle, MF_STRING | MF_ENABLED, 1,
		"Return to entrance");
	AppendMenu(history_menu_handle, MF_SEPARATOR, 0, NULL);
	
	// Add the entries of the history list to the popup menu.
	
	for (index = history_list->entries - 1; index >= 0; index--) {
		history_entry *history_entry_ptr = 
			history_list->get_entry(index);
		AppendMenu(history_menu_handle, MF_STRING | MF_ENABLED, index + 2, 
			history_entry_ptr->link.label);
	}

	// Place a check mark against the current history entry.

	CheckMenuItem(history_menu_handle, history_list->curr_entry_index + 2,
		MF_BYCOMMAND | MF_CHECKED);
}

//------------------------------------------------------------------------------
// Track the user's movement across the history menu, and return the entry
// that was selected.
//------------------------------------------------------------------------------

int
track_history_menu(void)
{
	int x, y;

	// Display the popup menu at the current mouse cursor position, wait
	// for the user to select an entry or dismiss the menu, then destroy
	// the popup menu.

	get_mouse_position(&x, &y, false);
	return(TrackPopupMenu(history_menu_handle, TPM_CENTERALIGN |
		TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON,
		x, y, 0, main_window_handle, NULL));
}

//------------------------------------------------------------------------------
// Close the history menu.
//------------------------------------------------------------------------------

void
close_history_menu(void)
{
	if (history_menu_handle) {
		DestroyMenu(history_menu_handle);
		history_menu_handle = NULL;
	}
}

//==============================================================================
// Password window functions.
//==============================================================================

static char username[16], password[16];

//------------------------------------------------------------------------------
// Function to handle password window events.
//------------------------------------------------------------------------------

static BOOL CALLBACK
handle_password_event(HWND window_handle, UINT message, WPARAM wParam,
					  LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDOK:
				GetDlgItemText(window_handle, IDC_USERNAME, username, 16);
				GetDlgItemText(window_handle, IDC_PASSWORD, password, 16);
				EndDialog(window_handle, TRUE);
				break;
			case IDCANCEL:
				EndDialog(window_handle, FALSE);
				break;
			}
		}
		return(TRUE);
	default:
		return(FALSE);
	}
}

//------------------------------------------------------------------------------
// Get a username and password.
//------------------------------------------------------------------------------

bool
get_password(string *username_ptr, string *password_ptr)
{
	// Bring up a dialog box that requests a username and password.

	if (DialogBox(instance_handle, MAKEINTRESOURCE(IDD_PASSWORD), 
		main_window_handle, handle_password_event)) {
		*username_ptr = username;
		*password_ptr = password;
		return(true);
	}
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
	DDSURFACEDESC ddraw_surface_desc;
	HRESULT result;

	// If hardware acceleration has been requested, attempt to start up the 3D
	// interface; if this fails, turn off hardware acceleration.

	if (hardware_acceleration && !start_up_3D_interface()) {
		shut_down_3D_interface();
		hardware_acceleration = false;
	}

	// If hardware acceleration is not enabled...

	if (!hardware_acceleration) {

		// Create the primary surface.

		memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
		ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
		ddraw_surface_desc.dwFlags = DDSD_CAPS;
		ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc,
			&primary_surface_ptr, NULL) != DD_OK)
			return(false);

		// Create a seperate frame buffer surface in system memory.

		ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | 
			DDSCAPS_SYSTEMMEMORY;
		ddraw_surface_desc.dwWidth = display_width;
		ddraw_surface_desc.dwHeight = display_height;
		if ((result = ddraw_object_ptr->CreateSurface(&ddraw_surface_desc,
			&framebuf_surface_ptr, NULL)) != DD_OK)
			return(false);

		// If the display depth is 8 bits, allocate a 16-bit frame buffer.

		if (display_depth == 8 && 
			(framebuf_ptr = new byte[window_width * window_height * 2]) == NULL)
			return(false);
	}

	// If not running full-screen, create a clipper object for the main
	// window, and attach it to the primary surface.

	if (!full_screen) {
		if (ddraw_object_ptr->CreateClipper(0, &clipper_ptr, NULL) != DD_OK ||
			clipper_ptr->SetHWnd(0, main_window_handle) != DD_OK ||
			primary_surface_ptr->SetClipper(clipper_ptr) != DD_OK)
			return(false);
	}

	// Return success status.

	return(true);
}

//------------------------------------------------------------------------------
// Destroy the frame buffer.
//------------------------------------------------------------------------------

void
destroy_frame_buffer(void)
{
	// If hardware acceleration was enabled, shut down the 3D interface.

	if (hardware_acceleration)
		shut_down_3D_interface();

	// If hardware acceleration was not enabled, and the display depth is 8
	// bits, delete the 16-bit frame buffer.

	else if (display_depth == 8 && framebuf_ptr)
		delete []framebuf_ptr;

	// Release the clipper object and the frame buffer and primary surfaces.

	if (framebuf_surface_ptr) {
		framebuf_surface_ptr->Release();
		framebuf_surface_ptr = NULL;
	}
	if (clipper_ptr) {
		primary_surface_ptr->SetClipper(NULL);
		clipper_ptr->Release();
	}
	if (primary_surface_ptr)
		primary_surface_ptr->Release();
}

//------------------------------------------------------------------------------
// Lock the frame buffer and return it's address and the width of the frame
// buffer in bytes.
//------------------------------------------------------------------------------

bool
lock_frame_buffer(byte *&frame_buffer_ptr, int &frame_buffer_width)
{
	// If hardware acceleration is enabled, begin the 3D scene.  There is
	// no frame buffer pointer or width to return.

	if (hardware_acceleration)
		d3d_device_ptr->BeginScene();

	// Else if the display depth is 8, return the 16-bit frame buffer pointer.

	else if (display_depth == 8) {
		frame_buffer_ptr = framebuf_ptr;
		frame_buffer_width = display_width << 1;
	}

	// Else if the display depth is 16, 24 or 32...

	else {
		DDSURFACEDESC ddraw_surface_desc;

		// If the frame buffer is already locked, simply return the current
		// frame buffer address and width.

		if (framebuf_ptr) {
			frame_buffer_ptr = framebuf_ptr;
			frame_buffer_width = framebuf_width;
		}

		// Otherwise attempt to lock the frame buffer and return it's address
		// and width.

		else {
			ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
			if (framebuf_surface_ptr->Lock(NULL, &ddraw_surface_desc,
				DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
				return(false);
			framebuf_ptr = (byte *)ddraw_surface_desc.lpSurface;
			framebuf_width = ddraw_surface_desc.lPitch;
			frame_buffer_ptr = framebuf_ptr;
			frame_buffer_width = framebuf_width;
		}
	}

	// Return success status.

	return(true);
}

//------------------------------------------------------------------------------
// Unlock the frame buffer.
//------------------------------------------------------------------------------

void
unlock_frame_buffer(void)
{
	// If hardware acceleration is active, end the 3D scene.

	if (hardware_acceleration)
		d3d_device_ptr->EndScene();

	// Otherwise if the display depth is 16, 24 or 32 and the frame buffer is 
	// locked, unlock it.

	else if (display_depth >= 16 && framebuf_ptr) {
		framebuf_surface_ptr->Unlock(framebuf_ptr);
		framebuf_ptr = NULL;
	}
}

//------------------------------------------------------------------------------
// Display the frame buffer.
//------------------------------------------------------------------------------

bool
display_frame_buffer(bool loading_spot, bool show_splash_graphic)
{
	START_TIMING;

	// If not loading a spot, hardware acceleration is not enabled, and the
	// display depth is 8, dither the contents of the 16-bit frame buffer into
	// the frame buffer surface.

	if (!loading_spot && !hardware_acceleration && display_depth == 8) {
		DDSURFACEDESC ddraw_surface_desc;
		byte *fb_ptr;
		int row_pitch;
		word *old_pixel_ptr;
		byte *new_pixel_ptr;
		int row_gap;
		int row, col;

		// Lock the frame buffer surface.

		ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
		if (framebuf_surface_ptr->Lock(NULL, &ddraw_surface_desc,
			DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
			return(false);
		fb_ptr = (byte *)ddraw_surface_desc.lpSurface;
		row_pitch = ddraw_surface_desc.lPitch;

		// Get pointers to the first pixel in the frame buffer and primary
		// surface, and compute the row gap.

		old_pixel_ptr = (word *)framebuf_ptr;
		new_pixel_ptr = fb_ptr;
		row_gap = row_pitch - display_width;

		// Perform the dither.

		for (row = 0; row < window_height - 1; row += 2) {
			for (col = 0; col < display_width - 1; col += 2) {
				*new_pixel_ptr++ = dither00[*old_pixel_ptr++];
				*new_pixel_ptr++ = dither01[*old_pixel_ptr++];
			}
			if (col < window_width)
				*new_pixel_ptr++ = dither00[*old_pixel_ptr++];
			new_pixel_ptr += row_gap;
			for (col = 0; col < window_width - 1; col += 2) {
				*new_pixel_ptr++ = dither10[*old_pixel_ptr++];
				*new_pixel_ptr++ = dither11[*old_pixel_ptr++];
			}
			if (col < window_width)
				*new_pixel_ptr++ = dither10[*old_pixel_ptr++];
			new_pixel_ptr += row_gap;
		}
		if (row < window_height) {
			for (col = 0; col < window_width - 1; col += 2) {
				*new_pixel_ptr++ = dither00[*old_pixel_ptr++];
				*new_pixel_ptr++ = dither01[*old_pixel_ptr++];
			}
			if (col < window_width)
				*new_pixel_ptr++ = dither00[*old_pixel_ptr++];
		}

		// Unlock the frame buffer surface.

		if (framebuf_surface_ptr->Unlock(fb_ptr) != DD_OK)
			return(false);
	}

	// Draw the label if it's visible and we're not loading a spot.

	if (label_visible && !loading_spot)
		draw_label();

	// Draw the splash graphic if it's requested and it's enabled.

	if (show_splash_graphic && splash_graphic_enabled)
		draw_splash_graphic();

	// Draw the task bar if it's enabled.

	if (task_bar_enabled)
		draw_task_bar(loading_spot);

	// Draw the cursor if full-screen hardware acceleration is active.

	if (full_screen)
		draw_cursor();

	// If hardware acceleration is enabled and the render mode is full-screen,
	// flip the primary and frame buffer surfaces.  Otherwise blit the frame
	// buffer onto the primary surface.

	if (full_screen)
		primary_surface_ptr->Flip(NULL, DDFLIP_WAIT);
	else
		blit_frame_buffer();

	END_TIMING("display_frame_buffer");
	return(true);
}

//------------------------------------------------------------------------------
// Method to clear a rectangle in the frame buffer.
//------------------------------------------------------------------------------

void
clear_frame_buffer(int x, int y, int width, int height)
{
	DDSURFACEDESC ddraw_surface_desc;
	byte *fb_ptr;
	int row_pitch;
	int bytes_per_pixel;

	// Lock the frame buffer surface.

	ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
	if (framebuf_surface_ptr->Lock(NULL, &ddraw_surface_desc,
		DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
		return;
	fb_ptr = (byte *)ddraw_surface_desc.lpSurface;
	row_pitch = ddraw_surface_desc.lPitch;
	bytes_per_pixel = display_depth / 8;

	// Clear the frame buffer surface.

	for (int row = y; row < y + height; row++) {
		byte *row_ptr = fb_ptr + row * row_pitch + x * bytes_per_pixel;
		memset(row_ptr, 0, width * bytes_per_pixel);
	}

	// Unlock the frame buffer surface.

	framebuf_surface_ptr->Unlock(fb_ptr);
}

//==============================================================================
// Software rendering functions.
//==============================================================================

//------------------------------------------------------------------------------
// Create a lit image for the given cache entry.
//------------------------------------------------------------------------------

void
create_lit_image(cache_entry *cache_entry_ptr, int image_dimensions)
{
	pixmap *pixmap_ptr;

	// Get the unlit image pointer and it's dimensions, and set a pointer to
	// the end of the image data.

	pixmap_ptr = cache_entry_ptr->pixmap_ptr;
	image_ptr = pixmap_ptr->image_ptr;
	if (pixmap_ptr->image_is_16_bit)
		image_width = pixmap_ptr->width * 2;
	else
		image_width = pixmap_ptr->width;
	image_height = pixmap_ptr->height;
	end_image_ptr = image_ptr + image_width * image_height;

	// Put the transparent index in a static variable so that the assembly code
	// can get to it, and get a pointer to the palette for the desired brightness
	// index.

	if (!pixmap_ptr->image_is_16_bit) {
		transparent_index = pixmap_ptr->transparent_index;
		palette_ptr = pixmap_ptr->display_palette_list +
			cache_entry_ptr->brightness_index * pixmap_ptr->colours;
	} else
		palette_ptr = light_table[cache_entry_ptr->brightness_index];

	// Get the start address of the lit image and it's dimensions.

	new_image_ptr = cache_entry_ptr->lit_image_ptr;
	lit_image_dimensions = image_dimensions;

	// If the pixmap is a 16-bit image...

	if (pixmap_ptr->image_is_16_bit) {
		switch (display_depth) {

		// If the display depth is 8 or 16, convert the unlit image to a 16-bit
		// lit image...

		case 8:
		case 16:

			// Get the transparency mask.

			transparency_mask16 = (word)display_pixel_format.alpha_comp_mask;
		
			// Perform the conversion.

			__asm {
			
				// Save EBP and ESP so that we can use them.

				mov ebp_save, ebp
				mov esp_save, esp

				// EBX: holds pointer to current row of old image.
				// EDX: holds pointer to current pixel of new image.
				// EDI: holds pointer to palette.
				// EBP: holds number of pixels in a row or column.
				// ESP: holds the number of rows left to copy.

				mov ebx, image_ptr
				mov edx, new_image_ptr
				mov edi, palette_ptr
				mov ebp, lit_image_dimensions
				mov esp, ebp

			next_row1:

				// ECX: holds offset in current row of old image.
				// ESI: holds number of columns left to copy.

				mov ecx, 0
				mov esi, ebp

			next_column1:

				// Get the unlit 16-bit pixel from the old image, use it to 
				// obtain the lit 16-bit pixel, and store it in the new image.
				// The transparency mask must be transfered from the unlit to
				// lit pixel.

				mov eax, 0
				mov ax, [ebx + ecx]
				test ax, 0x8000
				js transparent_pixel1
				mov ax, [edi + eax * 4]
				jmp store_pixel1
			transparent_pixel1:
				and ax, 0x7fff
				mov ax, [edi + eax * 4]
				or ax, transparency_mask16
			store_pixel1:
				mov [edx], ax

				// Increment the old image offset, wrapping back to zero if the 
				// end of the row is reached.

				add ecx, 2
				cmp ecx, image_width 
				jl next_pixel1
				mov ecx, 0

			next_pixel1:

				// Increment the new image pointer.

				add edx, 2

				// Decrement the column counter, and copy next pixel in row if
				// there are any left.

				dec esi
				jnz next_column1

				// Increment the old image row pointer, and wrap back to the
				// first row if the end of the image has been reached.

				add ebx, image_width
				cmp ebx, end_image_ptr
				jl skip_wrap1
				mov ebx, image_ptr

			skip_wrap1:

				// Decrement the row counter, and copy next row if there are any
				// left.

				dec esp
				jnz next_row1

				// Restore EBP and ESP.

				mov ebp, ebp_save
				mov esp, esp_save
			}
			break;

		// If the display depth is 24 or 32, convert the unlit image to a 32-bit
		// lit image...

		case 24:
		case 32:

			// Get the transparency mask.

			transparency_mask32 = display_pixel_format.alpha_comp_mask;

			// Perform the conversion.

			__asm {
			
				// Save EBP and ESP so that we can use them.

				mov ebp_save, ebp
				mov esp_save, esp

				// EBX: holds pointer to current row of old image.
				// EDX: holds pointer to current pixel of new image.
				// EDI: holds pointer to palette.
				// EBP: holds number of pixels in a row or column.
				// ESP: holds the number of rows left to copy.

				mov ebx, image_ptr
				mov edx, new_image_ptr
				mov edi, palette_ptr
				mov ebp, lit_image_dimensions
				mov esp, ebp

			next_row2:

				// ECX: holds offset in current row of old image.
				// ESI: holds number of columns left to copy.

				mov ecx, 0
				mov esi, ebp

			next_column2:

				// Get the unlit 16-bit index from the old image, use it to
				// obtain the 32-bit pixel, and store it in the new image.
				// The transparency mask must be transfered from the unlit to
				// lit pixel.

				mov eax, 0
				mov ax, [ebx + ecx]
				test ax, 0x8000
				jz transparent_pixel2
				mov eax, [edi + eax * 4]	
				jmp store_pixel2
			transparent_pixel2:
				and ax, 0x7fff
				mov eax, [edi + eax * 4]
				or eax, transparency_mask32
			store_pixel2:
				mov [edx], eax

				// Increment the old image offset, wrapping back to zero if the 
				// end of the row is reached.

				add ecx, 2
				cmp ecx, image_width 
				jl next_pixel2
				mov ecx, 0

			next_pixel2:

				// Increment the new image pointer.

				add edx, 4

				// Decrement the column counter, and copy next pixel in row if
				// there are any left.

				dec esi
				jnz next_column2

				// Increment the old image row pointer, and wrap back to the
				// first row if the end of the image has been reached.

				add ebx, image_width
				cmp ebx, end_image_ptr
				jl skip_wrap2
				mov ebx, image_ptr

			skip_wrap2:

				// Decrement the row counter, and copy next row if there are any
				// left.

				dec esp
				jnz next_row2

				// Restore EBP and ESP.

				mov ebp, ebp_save
				mov esp, esp_save
			}
		}
	} 
	
	// If the pixmap is an 8-bit image...

	else {
		switch (display_depth) {

		// If the display depth is 8 or 16, convert the unlit image to a 16-bit
		// lit image...

		case 8:
		case 16:

			// Get the transparency mask.

			transparency_mask16 = (word)display_pixel_format.alpha_comp_mask;

			// Perform the conversion.

			__asm {
			
				// Save EBP and ESP so that we can use them.

				mov ebp_save, ebp
				mov esp_save, esp

				// EBX: holds pointer to current row of old image.
				// EDX: holds pointer to current pixel of new image.
				// EDI: holds pointer to palette.
				// EBP: holds number of pixels in a row or column.
				// ESP: holds the number of rows left to copy.

				mov ebx, image_ptr
				mov edx, new_image_ptr
				mov edi, palette_ptr
				mov ebp, lit_image_dimensions
				mov esp, ebp

			next_row3:

				// ECX: holds offset in current row of old image.
				// ESI: holds number of columns left to copy.

				mov ecx, 0
				mov esi, ebp

			next_column3:

				// Get the current 8-bit index from the old image, use it to
				// obtain the 16-bit pixel, and store it in the new image.  If 
				// the 8-bit index is the transparent index, mark the pixel as
				// transparent by setting the tranparency bit.

				mov eax, 0
				mov al, [ebx + ecx]
				cmp eax, transparent_index
				je transparent_pixel3
				mov ax, [edi + eax * 4]	
				jmp store_pixel3
			transparent_pixel3:
				mov ax, [edi + eax * 4]
				or ax, transparency_mask16
			store_pixel3:
				mov [edx], ax

				// Increment the old image offset, wrapping back to zero if the 
				// end of the row is reached.

				inc ecx
				cmp ecx, image_width 
				jl next_pixel3
				mov ecx, 0

			next_pixel3:

				// Increment the new image pointer.

				add edx, 2

				// Decrement the column counter, and copy next pixel in row if
				// there are any left.

				dec esi
				jnz next_column3

				// Increment the old image row pointer, and wrap back to the
				// first row if the end of the image has been reached.

				add ebx, image_width
				cmp ebx, end_image_ptr
				jl skip_wrap3
				mov ebx, image_ptr

			skip_wrap3:

				// Decrement the row counter, and copy next row if there are any
				// left.

				dec esp
				jnz next_row3

				// Restore EBP and ESP.

				mov ebp, ebp_save
				mov esp, esp_save
			}
			break;

		// If the display depth is 24 or 32, convert the unlit image to a 32-bit
		// lit image...

		case 24:
		case 32:

			// Get the transparency mask.

			transparency_mask32 = display_pixel_format.alpha_comp_mask;

			// Perform the conversion.

			__asm {
			
				// Save EBP and ESP so that we can use them.

				mov ebp_save, ebp
				mov esp_save, esp

				// EBX: holds pointer to current row of old image.
				// EDX: holds pointer to current pixel of new image.
				// EDI: holds pointer to palette.
				// EBP: holds number of pixels in a row or column.
				// ESP: holds the number of rows left to copy.

				mov ebx, image_ptr
				mov edx, new_image_ptr
				mov edi, palette_ptr
				mov ebp, lit_image_dimensions
				mov esp, ebp

			next_row4:

				// ECX: holds offset in current row of old image.
				// ESI: holds number of columns left to copy.

				mov ecx, 0
				mov esi, ebp

			next_column4:

				// Get the current 8-bit index from the old image, use it to
				// obtain the 24-bit pixel, and store it in the new image.  If 
				// the 8-bit index is the transparent index, mark the pixel as
				// transparent by setting the tranparency bit.

				mov eax, 0
				mov al, [ebx + ecx]
				cmp eax, transparent_index
				je transparent_pixel4
				mov eax, [edi + eax * 4]	
				jmp store_pixel4
			transparent_pixel4:
				mov eax, [edi + eax * 4]
				or eax, transparency_mask32
			store_pixel4:
				mov [edx], eax

				// Increment the old image offset, wrapping back to zero if the 
				// end of the row is reached.

				inc ecx
				cmp ecx, image_width 
				jl next_pixel4
				mov ecx, 0

			next_pixel4:

				// Increment the new image pointer.

				add edx, 4

				// Decrement the column counter, and copy next pixel in row if
				// there are any left.

				dec esi
				jnz next_column4

				// Increment the old image row pointer, and wrap back to the
				// first row if the end of the image has been reached.

				add ebx, image_width
				cmp ebx, end_image_ptr
				jl skip_wrap4
				mov ebx, image_ptr

			skip_wrap4:

				// Decrement the row counter, and copy next row if there are any
				// left.

				dec esp
				jnz next_row4

				// Restore EBP and ESP.

				mov ebp, ebp_save
				mov esp, esp_save
			}
		}
	}
}

//------------------------------------------------------------------------------
// Render a colour span to a 16-bit frame buffer.
//------------------------------------------------------------------------------

void
render_colour_span16(span *span_ptr)
{
	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Calculate the frame buffer pointer and span width.

	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 1);
	span_width = span_ptr->end_sx - span_ptr->start_sx;

	// Get the 16-bit colour pixel.

	colour_pixel16 = (word)span_ptr->colour_pixel;

	// Render the span into the 16-bit frame buffer.

	__asm {

		// Put the texture colour into AX, the image pointer into EBX, and
		// the span width into ECX.

		mov ax, colour_pixel16
		mov ebx, fb_ptr
		mov ecx, span_width

	next_pixel16:

		// Store the texture colour in the frame buffer, then advance the
		// frame buffer pointer.

		mov [ebx], ax
		add ebx, 2

		// Decrement the loop counter, and continue if we haven't filled the
		// whole span yet.

		dec ecx
		jnz next_pixel16
	}
}

//------------------------------------------------------------------------------
// Render a colour span to a 24-bit frame buffer.
//------------------------------------------------------------------------------

void
render_colour_span24(span *span_ptr)
{
	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Calculate the frame buffer pointer and span width.

	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		span_ptr->start_sx * 3;
	span_width = span_ptr->end_sx - span_ptr->start_sx;

	// Get the 24-bit colour pixel.

	colour_pixel24 = span_ptr->colour_pixel;

	// Render the span into the 24-bit frame buffer.

	__asm {

		// Put the texture colour into EAX, the image pointer into EBX, and
		// the span width into ECX.

		mov eax, colour_pixel24
		mov ebx, fb_ptr
		mov ecx, span_width

	next_pixel24:

		// Store the texture colour in the frame buffer, then advance the
		// frame buffer pointer.

		mov edx, [ebx]
		and edx, 0xff000000
		or edx, eax
		mov [ebx], edx
		add ebx, 3

		// Decrement the loop counter, and continue if we haven't filled the
		// whole span yet.

		dec ecx
		jnz next_pixel24
	}
}

//------------------------------------------------------------------------------
// Render a colour span to a 32-bit frame buffer.
//------------------------------------------------------------------------------

void
render_colour_span32(span *span_ptr)
{
	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Calculate the frame buffer pointer and span width.

	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 2);
	span_width = span_ptr->end_sx - span_ptr->start_sx;

	// Get the 32-bit colour pixel.

	colour_pixel32 = span_ptr->colour_pixel;

	// Render the span into the 32-bit frame buffer.

	__asm {

		// Put the texture colour into EAX, the image pointer into EBX, and
		// the span width into ECX.

		mov eax, colour_pixel32
		mov ebx, fb_ptr
		mov ecx, span_width

	next_pixel32:

		// Store the texture colour in the frame buffer, then advance the
		// frame buffer pointer.

		mov [ebx], eax
		add ebx, 4

		// Decrement the loop counter, and continue if we haven't filled the
		// whole span yet.

		dec ecx
		jnz next_pixel32
	}
}

//------------------------------------------------------------------------------
// Render an opaque span to a 16-bit frame buffer.
//------------------------------------------------------------------------------

void
render_opaque_span16(span *span_ptr)
{
	cache_entry *cache_entry_ptr;
	int end_sx;
	float one_on_tz, u_on_tz, v_on_tz;
	span_data scaled_delta_span;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the image data.

	cache_entry_ptr = get_cache_entry(span_ptr->pixmap_ptr, 
		span_ptr->brightness_index);
	image_ptr = cache_entry_ptr->lit_image_ptr;
	mask = cache_entry_ptr->lit_image_mask;
	shift = cache_entry_ptr->lit_image_shift;

	// Pre-scale the deltas for faster calculations when rendering spans that
	// are SPAN_WIDTH in width.

	scaled_delta_span.one_on_tz = span_ptr->delta_span.one_on_tz * SPAN_WIDTH;
	scaled_delta_span.u_on_tz = span_ptr->delta_span.u_on_tz * SPAN_WIDTH;
	scaled_delta_span.v_on_tz = span_ptr->delta_span.v_on_tz * SPAN_WIDTH;

	// Get the starting 1/tz value; if it is zero, make it one (this is used
	// by sky spans to ensure they are furthest from the viewer, rather than
	// using a tiny 1/tz value that introduces errors into the texture
	// coordinates).

	one_on_tz = span_ptr->start_span.one_on_tz;
	if (one_on_tz == 0.0)
		one_on_tz = 1.0;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 1);

	// Compute (u,v) for that pixel.  We are now representing (u,v) as true fixed
	// point values for speed.

	u_on_tz = span_ptr->start_span.u_on_tz;
	v_on_tz = span_ptr->start_span.v_on_tz;
	end_tz = 1.0f / one_on_tz;
	COMPUTE_UV(u, v, u_on_tz, v_on_tz, end_tz);

	// Compute the start and end values for the first span.

	span_start_sx = span_ptr->start_sx;
	span_end_sx = span_ptr->start_sx + SPAN_WIDTH;
	end_sx = span_ptr->end_sx;
	end_one_on_tz = one_on_tz + scaled_delta_span.one_on_tz;
	end_tz = 1.0f / end_one_on_tz;

	// Now render the row one span at a time, until we have less than a span's
	// width of pixels left.

	while (span_end_sx < end_sx) {
			
		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.  We also compute 
		// the ending 1/tz value for the *next* span.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;
		end_one_on_tz += scaled_delta_span.one_on_tz;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Start computing 1/one_on_tz for the next span (the floating
			// point divide will overlap the span render loop on a Pentium).

			fld const_1
			fdiv end_one_on_tz

			// Put u in EDX and v in EBX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask and shift in ECX; the mask occupies the top
			// word, and the shift occupies CL.

			mov ecx, mask
			or  ecx, shift

			// Move the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render 32 texture mapped pixels.

			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			DRAW_PIXEL16
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Store the result of 1/one_on_tz in end_tz (this computation
			// should be well and truly completed by now).

			fstp end_tz

			// Recover value of base and stack pointer (otherwise we lose access
			// to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
		
		// Get ready for the next span.
	
		u = end_u;
		v = end_v;
		span_start_sx = span_end_sx;
		span_end_sx += SPAN_WIDTH;
	}

	// If there are pixels left, render one more shorter span.

	if (span_start_sx < end_sx) {

		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;

		// Compute the size of this last span.

		span_width = (end_sx - span_start_sx) << 8;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask, shift and span width in ECX; the mask
			// occupies the top word, the shift occupies CL, and the span 
			// width occupies CH.

			mov ecx, mask
			or  ecx, shift
			or	ecx, span_width

			// Move the the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render span_width texture mapped pixels.

		next_pixel16:
			DRAW_PIXEL16
			dec ch
			jnz next_pixel16
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Recover value of base and stack pointer (otherwise we lose
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render an opaque span to a 24-bit frame buffer.
//------------------------------------------------------------------------------

void
render_opaque_span24(span *span_ptr)
{
	cache_entry *cache_entry_ptr;
	int end_sx;
	float one_on_tz, u_on_tz, v_on_tz;
	span_data scaled_delta_span;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the image data.

	cache_entry_ptr = get_cache_entry(span_ptr->pixmap_ptr, 
		span_ptr->brightness_index);
	image_ptr = cache_entry_ptr->lit_image_ptr;
	mask = cache_entry_ptr->lit_image_mask;
	shift = cache_entry_ptr->lit_image_shift;

	// Pre-scale the deltas for faster calculations when rendering spans that
	// are SPAN_WIDTH in width.

	scaled_delta_span.one_on_tz = span_ptr->delta_span.one_on_tz * SPAN_WIDTH;
	scaled_delta_span.u_on_tz = span_ptr->delta_span.u_on_tz * SPAN_WIDTH;
	scaled_delta_span.v_on_tz = span_ptr->delta_span.v_on_tz * SPAN_WIDTH;

	// Get the starting 1/tz value; if it is zero, make it one (this is used
	// by sky spans to ensure they are furthest from the viewer, rather than
	// using a tiny 1/tz value that introduces errors into the texture
	// coordinates).

	one_on_tz = span_ptr->start_span.one_on_tz;
	if (one_on_tz == 0.0)
		one_on_tz = 1.0;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		span_ptr->start_sx * 3;

	// Compute (u,v) for that pixel.  We are now representing (u,v) as true fixed
	// point values for speed.

	u_on_tz = span_ptr->start_span.u_on_tz;
	v_on_tz = span_ptr->start_span.v_on_tz;
	end_tz = 1.0f / one_on_tz;
	COMPUTE_UV(u, v, u_on_tz, v_on_tz, end_tz);

	// Compute the start and end values for the first span.

	span_start_sx = span_ptr->start_sx;
	span_end_sx = span_ptr->start_sx + SPAN_WIDTH;
	end_sx = span_ptr->end_sx;
	end_one_on_tz = one_on_tz + scaled_delta_span.one_on_tz;
	end_tz = 1.0f / end_one_on_tz;

	// Now render the row one span at a time, until we have less than a span's
	// width of pixels left.

	while (span_end_sx < end_sx) {
		
		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.  We also compute 
		// the ending 1/tz value for the *next* span.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;
		end_one_on_tz += scaled_delta_span.one_on_tz;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Start computing 1/one_on_tz for the next span (the floating
			// point divide will overlap the span render loop on a Pentium).

			fld const_1
			fdiv end_one_on_tz

			// Put u in EDX and v in EBX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask and shift in ECX; the mask occupies the top
			// word, and the shift occupies CL.

			mov ecx, mask
			or  ecx, shift

			// Move the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render 32 texture mapped pixels.

			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			DRAW_PIXEL24
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Store the result of 1/one_on_tz in end_tz (this computation
			// should be well and truly completed by now).

			fstp end_tz

			// Recover value of base and stack pointer (otherwise we lose access
			// to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
		
		// Get ready for the next span.
	
		u = end_u;
		v = end_v;
		span_start_sx = span_end_sx;
		span_end_sx += SPAN_WIDTH;
	}

	// If there are pixels left, render one more shorter span.

	if (span_start_sx < end_sx) {

		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;

		// Compute the size of this last span.

		span_width = (end_sx - span_start_sx) << 8;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask, shift and span width in ECX; the mask
			// occupies the top word, the shift occupies CL, and the span 
			// width occupies CH.

			mov ecx, mask
			or  ecx, shift
			or	ecx, span_width

			// Move the the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render span_width texture mapped pixels.

		next_pixel24:
			DRAW_PIXEL24
			dec ch
			jnz next_pixel24
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Recover value of base and stack pointer (otherwise we lose
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render an opaque span to a 32-bit frame buffer.
//------------------------------------------------------------------------------

void
render_opaque_span32(span *span_ptr)
{
	cache_entry *cache_entry_ptr;
	int end_sx;
	float one_on_tz, u_on_tz, v_on_tz;
	span_data scaled_delta_span;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the image data.

	cache_entry_ptr = get_cache_entry(span_ptr->pixmap_ptr, 
		span_ptr->brightness_index);
	image_ptr = cache_entry_ptr->lit_image_ptr;
	mask = cache_entry_ptr->lit_image_mask;
	shift = cache_entry_ptr->lit_image_shift;

	// Pre-scale the deltas for faster calculations when rendering spans that
	// are SPAN_WIDTH in width.

	scaled_delta_span.one_on_tz = span_ptr->delta_span.one_on_tz * SPAN_WIDTH;
	scaled_delta_span.u_on_tz = span_ptr->delta_span.u_on_tz * SPAN_WIDTH;
	scaled_delta_span.v_on_tz = span_ptr->delta_span.v_on_tz * SPAN_WIDTH;

	// Get the starting 1/tz value; if it is zero, make it one (this is used
	// by sky spans to ensure they are furthest from the viewer, rather than
	// using a tiny 1/tz value that introduces errors into the texture
	// coordinates).

	one_on_tz = span_ptr->start_span.one_on_tz;
	if (one_on_tz == 0.0)
		one_on_tz = 1.0;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 2);

	// Compute (u,v) for that pixel.  We are now representing (u,v) as true fixed
	// point values for speed.

	u_on_tz = span_ptr->start_span.u_on_tz;
	v_on_tz = span_ptr->start_span.v_on_tz;
	end_tz = 1.0f / one_on_tz;
	COMPUTE_UV(u, v, u_on_tz, v_on_tz, end_tz);

	// Compute the start and end values for the first span.

	span_start_sx = span_ptr->start_sx;
	span_end_sx = span_ptr->start_sx + SPAN_WIDTH;
	end_sx = span_ptr->end_sx;
	end_one_on_tz = one_on_tz + scaled_delta_span.one_on_tz;
	end_tz = 1.0f / end_one_on_tz;

	// Now render the row one span at a time, until we have less than a span's
	// width of pixels left.

	while (span_end_sx < end_sx) {
		
		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.  We also compute 
		// the ending 1/tz value for the *next* span.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;
		end_one_on_tz += scaled_delta_span.one_on_tz;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Start computing 1/one_on_tz for the next span (the floating
			// point divide will overlap the span render loop on a Pentium).

			fld const_1
			fdiv end_one_on_tz

			// Put u in EDX and v in EBX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask and shift in ECX; the mask occupies the top
			// word, and the shift occupies CL.

			mov ecx, mask
			or  ecx, shift

			// Move the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render 32 texture mapped pixels.

			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			DRAW_PIXEL32
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Store the result of 1/one_on_tz in end_tz (this computation
			// should be well and truly completed by now).

			fstp end_tz

			// Recover value of base and stack pointer (otherwise we lose access
			// to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
		
		// Get ready for the next span.
	
		u = end_u;
		v = end_v;
		span_start_sx = span_end_sx;
		span_end_sx += SPAN_WIDTH;
	}

	// If there are pixels left, render one more shorter span.

	if (span_start_sx < end_sx) {

		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;

		// Compute the size of this last span.

		span_width = (end_sx - span_start_sx) << 8;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask, shift and span width in ECX; the mask
			// occupies the top word, the shift occupies CL, and the span 
			// width occupies CH.

			mov ecx, mask
			or  ecx, shift
			or	ecx, span_width

			// Move the the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render span_width texture mapped pixels.

		next_pixel32:
			DRAW_PIXEL32
			dec ch
			jnz next_pixel32
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Recover value of base and stack pointer (otherwise we lose
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a transparent span to a 16-bit frame buffer.
//------------------------------------------------------------------------------

void
render_transparent_span16(span *span_ptr)
{
	cache_entry *cache_entry_ptr;
	float start_one_on_tz;
	int span_start_sx, span_end_sx, end_sx;
	float u_on_tz, v_on_tz;
	fixed end_u, end_v;
	span_data scaled_delta_span;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the lit image data.

	cache_entry_ptr = get_cache_entry(span_ptr->pixmap_ptr, 
		span_ptr->brightness_index);
	image_ptr = cache_entry_ptr->lit_image_ptr;
	mask = cache_entry_ptr->lit_image_mask;
	shift = cache_entry_ptr->lit_image_shift;

	// Pre-scale the deltas for faster calculations when rendering spans that
	// are SPAN_WIDTH in width.

	scaled_delta_span.one_on_tz = span_ptr->delta_span.one_on_tz * SPAN_WIDTH;
	scaled_delta_span.u_on_tz = span_ptr->delta_span.u_on_tz * SPAN_WIDTH;
	scaled_delta_span.v_on_tz = span_ptr->delta_span.v_on_tz * SPAN_WIDTH;

	// Get the starting 1/tz value; if it is zero, make it one (this is used
	// by sky spans to ensure they are furthest from the viewer, rather than
	// using a tiny 1/tz value that introduces errors into the texture
	// coordinates).

	start_one_on_tz = span_ptr->start_span.one_on_tz;
	if (start_one_on_tz == 0.0)
		start_one_on_tz = 1.0;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 1);

	// Get the transparency mask.

	transparency_mask16 = (word)display_pixel_format.alpha_comp_mask;

	// Compute (u,v) for that pixel.  We are now representing (u,v) as true fixed
	// point values for speed.

	u_on_tz = span_ptr->start_span.u_on_tz;
	v_on_tz = span_ptr->start_span.v_on_tz;
	end_tz = 1.0f / start_one_on_tz;
	COMPUTE_UV(u, v, u_on_tz, v_on_tz, end_tz);

	// Compute the start and end data for the first span.

	span_start_sx = span_ptr->start_sx;
	span_end_sx = span_ptr->start_sx + SPAN_WIDTH;
	end_sx = span_ptr->end_sx;
	end_one_on_tz = start_one_on_tz + scaled_delta_span.one_on_tz;
	end_tz = 1.0f / end_one_on_tz;

	// Now render the row one span at a time, until we have less than a span's
	// width of pixels left.

	while (span_end_sx < end_sx) {
			
		// Compute (end_u, end_v) and (delta_u, delta_v) for this span, 
		// storing them as fixed point numbers for speed.  We also compute
		// the ending 1/tz value for the *next* span.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;
		end_one_on_tz += scaled_delta_span.one_on_tz;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Start computing 1/one_on_tz for the next span (the floating 
			// point divide will overlap the span render loop on a Pentium).

			fld const_1
			fdiv end_one_on_tz

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask and shift in ECX; the mask occupies the top 
			// word, and the shift occupies CL.

			mov ecx, mask
			or  ecx, shift

			// Move the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render the pixels in the span.

			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_1)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_2)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_3)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_4)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_5)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_6)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_7)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_8)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_9)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_10)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_11)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_12)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_13)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_14)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_15)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_16)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_17)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_18)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_19)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_20)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_21)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_22)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_23)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_24)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_25)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_26)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_27)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_28)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_29)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_30)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_31)
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16_32)

			// Save new value of frame buffer

			mov fb_ptr, esi

			// Store the result of 1/one_on_tz in end_tz (this computation
			// should be well and truly completed by now).

			fstp end_tz

			// Recover value of base and stack pointer (otherwise we lose 
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}

		// Get ready for the next span.

		u = end_u;
		v = end_v;
		span_start_sx = span_end_sx;
		span_end_sx += SPAN_WIDTH;
	}

	// If there are pixels left, render one more shorter span.

	if (span_start_sx < end_sx) {

		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;

		// Compute the size of this last span.

		span_width = (end_sx - span_start_sx) << 8;

		// Render the span...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask, shift and span width in ECX; the mask
			// occupies the top word, the shift occupies CL, and the span
			// width occupies CH.

			mov ecx, mask
			or  ecx, shift
			or	ecx, span_width

			// Move the the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render span_width texture mapped pixels.

		next_pixel16:
			DRAW_TRANSPARENT_PIXEL16(skip_pixel16)
			dec ch
			jnz next_pixel16
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Recover value of base and stack pointer (otherwise we lose
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a transparent span to a 24-bit frame buffer.
//------------------------------------------------------------------------------

void
render_transparent_span24(span *span_ptr)
{
	cache_entry *cache_entry_ptr;
	float start_one_on_tz;
	int span_start_sx, span_end_sx, end_sx;
	float u_on_tz, v_on_tz;
	fixed end_u, end_v;
	span_data scaled_delta_span;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the lit image data.

	cache_entry_ptr = get_cache_entry(span_ptr->pixmap_ptr, 
		span_ptr->brightness_index);
	image_ptr = cache_entry_ptr->lit_image_ptr;
	mask = cache_entry_ptr->lit_image_mask;
	shift = cache_entry_ptr->lit_image_shift;

	// Pre-scale the deltas for faster calculations when rendering spans that
	// are SPAN_WIDTH in width.

	scaled_delta_span.one_on_tz = span_ptr->delta_span.one_on_tz * SPAN_WIDTH;
	scaled_delta_span.u_on_tz = span_ptr->delta_span.u_on_tz * SPAN_WIDTH;
	scaled_delta_span.v_on_tz = span_ptr->delta_span.v_on_tz * SPAN_WIDTH;

	// Get the starting 1/tz value; if it is zero, make it one (this is used
	// by sky spans to ensure they are furthest from the viewer, rather than
	// using a tiny 1/tz value that introduces errors into the texture
	// coordinates).

	start_one_on_tz = span_ptr->start_span.one_on_tz;
	if (start_one_on_tz == 0.0)
		start_one_on_tz = 1.0;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx * 3);

	// Get the transparency mask.

	transparency_mask24 = display_pixel_format.alpha_comp_mask;

	// Compute (u,v) for that pixel.  We are now representing (u,v) as true fixed
	// point values for speed.

	u_on_tz = span_ptr->start_span.u_on_tz;
	v_on_tz = span_ptr->start_span.v_on_tz;
	end_tz = 1.0f / start_one_on_tz;
	COMPUTE_UV(u, v, u_on_tz, v_on_tz, end_tz);

	// Compute the start and end data for the first span.

	span_start_sx = span_ptr->start_sx;
	span_end_sx = span_ptr->start_sx + SPAN_WIDTH;
	end_sx = span_ptr->end_sx;
	end_one_on_tz = start_one_on_tz + scaled_delta_span.one_on_tz;
	end_tz = 1.0f / end_one_on_tz;

	// Now render the row one span at a time, until we have less than a span's
	// width of pixels left.

	while (span_end_sx < end_sx) {
			
		// Compute (end_u, end_v) and (delta_u, delta_v) for this span, 
		// storing them as fixed point numbers for speed.  We also compute
		// the ending 1/tz value for the *next* span.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;
		end_one_on_tz += scaled_delta_span.one_on_tz;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Start computing 1/one_on_tz for the next span (the floating 
			// point divide will overlap the span render loop on a Pentium).

			fld const_1
			fdiv end_one_on_tz

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask and shift in ECX; the mask occupies the top 
			// word, and the shift occupies CL.

			mov ecx, mask
			or  ecx, shift

			// Move the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render the pixels in the span.

			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_1)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_2)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_3)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_4)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_5)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_6)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_7)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_8)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_9)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_10)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_11)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_12)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_13)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_14)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_15)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_16)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_17)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_18)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_19)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_20)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_21)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_22)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_23)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_24)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_25)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_26)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_27)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_28)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_29)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_30)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_31)
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24_32)

			// Save new value of frame buffer

			mov fb_ptr, esi

			// Store the result of 1/one_on_tz in end_tz (this computation
			// should be well and truly completed by now).

			fstp end_tz

			// Recover value of base and stack pointer (otherwise we lose 
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}

		// Get ready for the next span.

		u = end_u;
		v = end_v;
		span_start_sx = span_end_sx;
		span_end_sx += SPAN_WIDTH;
	}

	// If there are pixels left, render one more shorter span.

	if (span_start_sx < end_sx) {

		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;

		// Compute the size of this last span.

		span_width = (end_sx - span_start_sx) << 8;

		// Render the span...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask, shift and span width in ECX; the mask
			// occupies the top word, the shift occupies CL, and the span
			// width occupies CH.

			mov ecx, mask
			or  ecx, shift
			or	ecx, span_width

			// Move the the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render span_width texture mapped pixels.

		next_pixel24:
			DRAW_TRANSPARENT_PIXEL24(skip_pixel24)
			dec ch
			jnz next_pixel24
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Recover value of base and stack pointer (otherwise we lose
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a transparent span to a 32-bit frame buffer.
//------------------------------------------------------------------------------

void
render_transparent_span32(span *span_ptr)
{
	cache_entry *cache_entry_ptr;
	float start_one_on_tz;
	int span_start_sx, span_end_sx, end_sx;
	float u_on_tz, v_on_tz;
	fixed end_u, end_v;
	span_data scaled_delta_span;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the lit image data.

	cache_entry_ptr = get_cache_entry(span_ptr->pixmap_ptr, 
		span_ptr->brightness_index);
	image_ptr = cache_entry_ptr->lit_image_ptr;
	mask = cache_entry_ptr->lit_image_mask;
	shift = cache_entry_ptr->lit_image_shift;

	// Pre-scale the deltas for faster calculations when rendering spans that
	// are SPAN_WIDTH in width.

	scaled_delta_span.one_on_tz = span_ptr->delta_span.one_on_tz * SPAN_WIDTH;
	scaled_delta_span.u_on_tz = span_ptr->delta_span.u_on_tz * SPAN_WIDTH;
	scaled_delta_span.v_on_tz = span_ptr->delta_span.v_on_tz * SPAN_WIDTH;

	// Get the starting 1/tz value; if it is zero, make it one (this is used
	// by sky spans to ensure they are furthest from the viewer, rather than
	// using a tiny 1/tz value that introduces errors into the texture
	// coordinates).

	start_one_on_tz = span_ptr->start_span.one_on_tz;
	if (start_one_on_tz == 0.0)
		start_one_on_tz = 1.0;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 2);

	// Get the transparency mask.

	transparency_mask32 = display_pixel_format.alpha_comp_mask;

	// Compute (u,v) for that pixel.  We are now representing (u,v) as true fixed
	// point values for speed.

	u_on_tz = span_ptr->start_span.u_on_tz;
	v_on_tz = span_ptr->start_span.v_on_tz;
	end_tz = 1.0f / start_one_on_tz;
	COMPUTE_UV(u, v, u_on_tz, v_on_tz, end_tz);

	// Compute the start and end data for the first span.

	span_start_sx = span_ptr->start_sx;
	span_end_sx = span_ptr->start_sx + SPAN_WIDTH;
	end_sx = span_ptr->end_sx;
	end_one_on_tz = start_one_on_tz + scaled_delta_span.one_on_tz;
	end_tz = 1.0f / end_one_on_tz;

	// Now render the row one span at a time, until we have less than a span's
	// width of pixels left.

	while (span_end_sx < end_sx) {
			
		// Compute (end_u, end_v) and (delta_u, delta_v) for this span, 
		// storing them as fixed point numbers for speed.  We also compute
		// the ending 1/tz value for the *next* span.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;
		end_one_on_tz += scaled_delta_span.one_on_tz;

		// The rest of the render span code is done in assembler for speed...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Start computing 1/one_on_tz for the next span (the floating 
			// point divide will overlap the span render loop on a Pentium).

			fld const_1
			fdiv end_one_on_tz

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask and shift in ECX; the mask occupies the top 
			// word, and the shift occupies CL.

			mov ecx, mask
			or  ecx, shift

			// Move the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render the pixels in the span.

			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_1)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_2)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_3)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_4)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_5)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_6)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_7)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_8)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_9)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_10)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_11)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_12)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_13)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_14)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_15)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_16)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_17)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_18)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_19)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_20)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_21)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_22)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_23)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_24)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_25)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_26)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_27)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_28)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_29)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_30)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_31)
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32_32)

			// Save new value of frame buffer

			mov fb_ptr, esi

			// Store the result of 1/one_on_tz in end_tz (this computation
			// should be well and truly completed by now).

			fstp end_tz

			// Recover value of base and stack pointer (otherwise we lose 
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}

		// Get ready for the next span.

		u = end_u;
		v = end_v;
		span_start_sx = span_end_sx;
		span_end_sx += SPAN_WIDTH;
	}

	// If there are pixels left, render one more shorter span.

	if (span_start_sx < end_sx) {

		// Compute (end_u, end_v) and (delta_u, delta_v) for this span,
		// storing them as fixed point numbers for speed.
		
		u_on_tz += scaled_delta_span.u_on_tz;
		v_on_tz += scaled_delta_span.v_on_tz;
		COMPUTE_UV(end_u, end_v, u_on_tz, v_on_tz, end_tz);
		delta_u = (end_u - u) >> SPAN_SHIFT;
		delta_v = (end_v - v) >> SPAN_SHIFT;

		// Compute the size of this last span.

		span_width = (end_sx - span_start_sx) << 8;

		// Render the span...

		__asm {

			// Save base and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX and v in EDX.

			mov ebx, u
			mov edx, v

			// Put delta u in EBP and delta v in ESP.

			mov ebp, delta_u
			mov esp, delta_v

			// Combine the mask, shift and span width in ECX; the mask
			// occupies the top word, the shift occupies CL, and the span
			// width occupies CH.

			mov ecx, mask
			or  ecx, shift
			or	ecx, span_width

			// Move the the frame buffer pointer into ESI.

			mov esi, fb_ptr

			// Render span_width texture mapped pixels.

		next_pixel32:
			DRAW_TRANSPARENT_PIXEL32(skip_pixel32)
			dec ch
			jnz next_pixel32
			
			// Save new value of the frame buffer pointer.

			mov fb_ptr, esi

			// Recover value of base and stack pointer (otherwise we lose
			// access to all our local variables and call stack :-)

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a linear span into a 16-bit frame buffer.  It is assumed the necessary
// data to render the span is already set up in static variables.
//------------------------------------------------------------------------------

static void
render_linear_span16(bool image_is_16_bit)
{
	// If the image is 16 bit...

	if (image_is_16_bit) {
		__asm {

			// Save the base pointer and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX, image_width in ECX and span_width in EDX.

			mov ebx, u
			mov ecx, image_width
			mov edx, span_width

			// Put the image pointer in EDI and the frame buffer pointer in ESI.

			mov edi, image_ptr
			mov esi, fb_ptr

			// Put the palette pointer in EBP, and the transparent colour index
			// in ESP.

			mov ebp, palette_ptr
			mov esp, transparency_mask32

		next_pixel16a:

			// Load the 16-bit image pixel into EAX.  If it has the transparency
			// mask set, skip it.
			
			mov eax, 0
			mov ax, [edi + ebx]
			test eax, esp
			jnz skip_pixel16a

			// Use the unlit 16-bit pixel as an index into the palette to obtain
			// the lit 16-bit pixel, which we then store in the frame buffer.

			mov ax, [ebp + eax * 4]
			mov [esi], ax

		skip_pixel16a:

			// Advance the frame buffer pointer.

			add esi, 2

			// Advance u, wrapping to zero if it equals image_width.

			add ebx, 2
			cmp ebx, ecx
			jl done_pixel16a
			mov ebx, 0

		done_pixel16a:

			// If there are still pixels to render, repeat loop.

			dec edx
			jnz next_pixel16a

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}

	// If the image is 8 bit...

	else {
		__asm {

			// Save the base pointer and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX, image_width in ECX and span_width in EDX.

			mov ebx, u
			mov ecx, image_width
			mov edx, span_width

			// Put the image pointer in EDI and the frame buffer pointer in ESI.

			mov edi, image_ptr
			mov esi, fb_ptr

			// Put the palette pointer in EBP, and the transparent colour index
			// in ESP.

			mov ebp, palette_ptr
			mov esp, transparent_index

		next_pixel16b:

			// Load the 8-bit image pixel into EAX.  If it's the transparent
			// colour index, skip this pixel.
			
			mov eax, 0
			mov al, [edi + ebx]
			cmp eax, esp
			jz skip_pixel16b

			// Use the 8-bit pixel as an index into the palette to obtain the 
			// 16-bit pixel, which we then store in the frame buffer.

			mov ax, [ebp + eax * 4]
			mov [esi], ax

		skip_pixel16b:

			// Advance the frame buffer pointer.

			add esi, 2

			// Advance u, wrapping to zero if it equals image_width.

			inc ebx
			cmp ebx, ecx
			jl done_pixel16b
			mov ebx, 0

		done_pixel16b:

			// If there are still pixels to render, repeat loop.

			dec edx
			jnz next_pixel16b

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a linear span into a 24-bit frame buffer.  It is assumed the necessary
// data to render the span is already set up in static variables.
//------------------------------------------------------------------------------

static void
render_linear_span24(bool image_is_16_bit)
{
	// If the image is 16 bit...

	if (image_is_16_bit) {
		__asm {

			// Save the base pointer and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX, image_width in ECX and span_width in EDX.

			mov ebx, u
			mov ecx, image_width
			mov edx, span_width

			// Put the image pointer in EDI and the frame buffer pointer in ESI.

			mov edi, image_ptr
			mov esi, fb_ptr

			// Put the palette pointer in EBP.

			mov ebp, palette_ptr

		next_pixel24a:

			// Load the 16-bit image pixel into EAX.  If it's transparent,
			// skip this pixel.
			
			mov eax, 0
			mov ax, [edi + ebx]
			test eax, transparency_mask32
			jnz skip_pixel24a

			// Use the 16-bit pixel as an index into the palette to obtain the 
			// 24-bit pixel, which we then store in the frame buffer.

			mov esp, [esi]
			and esp, 0xff000000
			or esp, [ebp + eax * 4]
			mov [esi], esp

		skip_pixel24a:

			// Advance the frame buffer pointer.

			add esi, 3

			// Advance u, wrapping to zero if it equals image_width.

			add ebx, 2
			cmp ebx, ecx
			jl done_pixel24a
			mov ebx, 0

		done_pixel24a:

			// If there are still pixels to render, repeat loop.

			dec edx
			jnz next_pixel24a

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}

	// If the image is 8 bit...
	
	else {
		__asm {

			// Save the base pointer and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX, image_width in ECX and span_width in EDX.

			mov ebx, u
			mov ecx, image_width
			mov edx, span_width

			// Put the image pointer in EDI and the frame buffer pointer in ESI.

			mov edi, image_ptr
			mov esi, fb_ptr

			// Put the palette pointer in EBP.

			mov ebp, palette_ptr

		next_pixel24b:

			// Load the 8-bit image pixel into EAX.  If it's the transparent
			// colour index, skip this pixel.
			
			mov eax, 0
			mov al, [edi + ebx]
			cmp eax, transparent_index
			jz skip_pixel24b

			// Use the 8-bit pixel as an index into the palette to obtain the 
			// 24-bit pixel, which we then store in the frame buffer.

			mov esp, [esi]
			and esp, 0xff000000
			or esp, [ebp + eax * 4]
			mov [esi], esp

		skip_pixel24b:

			// Advance the frame buffer pointer.

			add esi, 3

			// Advance u, wrapping to zero if it equals image_width.

			inc ebx
			cmp ebx, ecx
			jl done_pixel24b
			mov ebx, 0

		done_pixel24b:

			// If there are still pixels to render, repeat loop.

			dec edx
			jnz next_pixel24b

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a linear span into a 32-bit frame buffer.  It is assumed the necessary
// data to render the span is already set up in static variables.
//------------------------------------------------------------------------------

static void
render_linear_span32(bool image_is_16_bit)
{
	// If the image is 16 bit...

	if (image_is_16_bit) {
		__asm {

			// Save the base pointer and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX, image_width in ECX and span_width in EDX.

			mov ebx, u
			mov ecx, image_width
			mov edx, span_width

			// Put the image pointer in EDI and the frame buffer pointer in ESI.

			mov edi, image_ptr
			mov esi, fb_ptr

			// Put the palette pointer in EBP, and the transparent colour index
			// in ESP.

			mov ebp, palette_ptr
			mov esp, transparency_mask32

		next_pixel32a:

			// Load the 16-bit image pixel into EAX.  If it's transparent,
			// skip this pixel.

			mov eax, 0
			mov ax, [edi + ebx]
			test eax, esp
			jnz skip_pixel32a

			// Use the 16-bit pixel as an index into the palette to obtain the 
			// 32-bit pixel, which we then store in the frame buffer.

			mov eax, [ebp + eax * 4]
			mov [esi], eax

		skip_pixel32a:

			// Advance the frame buffer pointer.

			add esi, 4

			// Advance u, wrapping to zero if it equals image_width.

			add ebx, 2
			cmp ebx, ecx
			jl done_pixel32a
			mov ebx, 0

		done_pixel32a:

			// If there are still pixels to render, repeat loop.

			dec edx
			jnz next_pixel32a

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}

	// If the pixmap is 8 bit...
	
	else {
		__asm {

			// Save the base pointer and stack pointer; we want to use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// Put u in EBX, image_width in ECX and span_width in EDX.

			mov ebx, u
			mov ecx, image_width
			mov edx, span_width

			// Put the image pointer in EDI and the frame buffer pointer in ESI.

			mov edi, image_ptr
			mov esi, fb_ptr

			// Put the palette pointer in EBP, and the transparent colour index
			// in ESP.

			mov ebp, palette_ptr
			mov esp, transparent_index

		next_pixel32b:

			// Advance the frame buffer pointer.

			mov eax, 0
			mov al, [edi + ebx]
			cmp eax, esp
			jz skip_pixel32b

			// Use the 8-bit pixel as an index into the palette to obtain the 
			// 24-bit pixel, which we then store in the frame buffer.

			mov eax, [ebp + eax * 4]
			mov [esi], eax

		skip_pixel32b:

			// Advance the frame buffer pointer.

			add esi, 4

			// Advance u, wrapping to zero if it equals image_width.

			inc ebx
			cmp ebx, ecx
			jl done_pixel32b
			mov ebx, 0

		done_pixel32b:

			// If there are still pixels to render, repeat loop.

			dec edx
			jnz next_pixel32b

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}
}

//------------------------------------------------------------------------------
// Render a popup span into a 16-bit frame buffer.
//------------------------------------------------------------------------------

void
render_popup_span16(span *span_ptr)
{
	pixmap *pixmap_ptr;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the pointer to the pixmap to render, the display palette for
	// the desired brightness level, the transparent colour index or mask,
	// and the image width (in bytes).
	
	pixmap_ptr = span_ptr->pixmap_ptr;
	if (pixmap_ptr->image_is_16_bit) {
		palette_ptr = light_table[span_ptr->brightness_index];
		transparency_mask32 = texture_pixel_format.alpha_comp_mask;
		image_width = pixmap_ptr->width * 2;
		u = ((int)span_ptr->start_span.u_on_tz % pixmap_ptr->width) * 2;
	} else {
		palette_ptr = pixmap_ptr->display_palette_list + 
			span_ptr->brightness_index * pixmap_ptr->colours;
		transparent_index = pixmap_ptr->transparent_index;
		image_width = pixmap_ptr->width;
		u = (int)span_ptr->start_span.u_on_tz % pixmap_ptr->width;
	}
	v = (int)span_ptr->start_span.v_on_tz % pixmap_ptr->height;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 1);

	// Compute the image pointer.

	image_ptr = pixmap_ptr->image_ptr + v * image_width;

	// Compute the span width.

	span_width = span_ptr->end_sx - span_ptr->start_sx;

	// Render the span

	render_linear_span16(pixmap_ptr->image_is_16_bit);
}

//------------------------------------------------------------------------------
// Render a popup span into a 24-bit frame buffer.
//------------------------------------------------------------------------------

void
render_popup_span24(span *span_ptr)
{
	pixmap *pixmap_ptr;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the pointer to the pixmap to render, the display palette for
	// the desired brightness level, the transparent colour index or mask,
	// and the image width (in bytes).
	
	pixmap_ptr = span_ptr->pixmap_ptr;
	if (pixmap_ptr->image_is_16_bit) {
		palette_ptr = light_table[span_ptr->brightness_index];
		transparency_mask32 = texture_pixel_format.alpha_comp_mask;
		image_width = pixmap_ptr->width * 2;
		u = ((int)span_ptr->start_span.u_on_tz % pixmap_ptr->width) * 2;
	} else {
		palette_ptr = pixmap_ptr->display_palette_list + 
			span_ptr->brightness_index * pixmap_ptr->colours;
		transparent_index = pixmap_ptr->transparent_index;
		image_width = pixmap_ptr->width;
		u = (int)span_ptr->start_span.u_on_tz % pixmap_ptr->width;
	}
	v = (int)span_ptr->start_span.v_on_tz % pixmap_ptr->height;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		span_ptr->start_sx * 3;

	// Compute the image pointer.

	image_ptr = pixmap_ptr->image_ptr + v * image_width;

	// Compute the span width.

	span_width = span_ptr->end_sx - span_ptr->start_sx;

	// Render the span.

	render_linear_span24(pixmap_ptr->image_is_16_bit);
}


//------------------------------------------------------------------------------
// Render a popup span into a 32-bit frame buffer.
//------------------------------------------------------------------------------

void
render_popup_span32(span *span_ptr)
{
	pixmap *pixmap_ptr;

	// Ignore span if it has zero width.

	if (span_ptr->start_sx == span_ptr->end_sx)
		return;

	// Get the pointer to the pixmap to render, the display palette for
	// the desired brightness level, the transparent colour index or mask,
	// and the image width (in bytes).
	
	pixmap_ptr = span_ptr->pixmap_ptr;
	if (pixmap_ptr->image_is_16_bit) {
		palette_ptr = light_table[span_ptr->brightness_index];
		transparency_mask32 = texture_pixel_format.alpha_comp_mask;
		image_width = pixmap_ptr->width * 2;
		u = ((int)span_ptr->start_span.u_on_tz % pixmap_ptr->width) * 2;
	} else {
		palette_ptr = pixmap_ptr->display_palette_list + 
			span_ptr->brightness_index * pixmap_ptr->colours;
		transparent_index = pixmap_ptr->transparent_index;
		image_width = pixmap_ptr->width;
		u = (int)span_ptr->start_span.u_on_tz % pixmap_ptr->width;
	}
	v = (int)span_ptr->start_span.v_on_tz % pixmap_ptr->height;

	// Get the pointer to the starting pixel in the frame buffer.
	
	fb_ptr = frame_buffer_ptr + frame_buffer_width * span_ptr->sy + 
		(span_ptr->start_sx << 2);

	// Compute the image pointer.

	image_ptr = pixmap_ptr->image_ptr + v * image_width;

	// Compute the span width.

	span_width = span_ptr->end_sx - span_ptr->start_sx;

	// Render the span.

	render_linear_span32(pixmap_ptr->image_is_16_bit);
}

//==============================================================================
// Hardware rendering functions.
//==============================================================================

//------------------------------------------------------------------------------
// Initialise the Direct3D vertex list.
//------------------------------------------------------------------------------

void
hardware_init_vertex_list(void)
{
	d3d_vertex_list = NULL;
}

//------------------------------------------------------------------------------
// Create the Direct3D vertex list.
//------------------------------------------------------------------------------

bool
hardware_create_vertex_list(int max_vertices)
{
	NEWARRAY(d3d_vertex_list, D3DTLVERTEX, max_vertices);
	return(d3d_vertex_list != NULL);
}

//------------------------------------------------------------------------------
// Destroy the Direct3D vertex list.
//------------------------------------------------------------------------------

void
hardware_destroy_vertex_list(int max_vertices)
{
	if (d3d_vertex_list) {
		DELARRAY(d3d_vertex_list, D3DTLVERTEX, max_vertices);
		d3d_vertex_list = NULL;
	}
}

//------------------------------------------------------------------------------
// Create a Direct3D texture.
//------------------------------------------------------------------------------

void *
hardware_create_texture(int image_size_index)
{
	int image_dimensions;
	hardware_texture *hardware_texture_ptr;
	DDSURFACEDESC ddraw_surface_desc;
	LPDIRECTDRAWSURFACE ddraw_texture_ptr;
	LPDIRECT3DTEXTURE2 d3d_texture_ptr, dummy_d3d_texture_ptr;
	D3DTEXTUREHANDLE d3d_texture_handle;

	// Create the DirectDraw texture surface, which won't be allocated until
	// the texture is loaded.  The AGP flag determines whether the surface is
	// allocated in AGP or video memory.

	image_dimensions = image_dimensions_list[image_size_index];
	memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
	ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
	ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT |
		DDSD_PIXELFORMAT;
	ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD |
		DDSCAPS_VIDEOMEMORY | AGP_flag;
	ddraw_surface_desc.dwWidth = image_dimensions;
	ddraw_surface_desc.dwHeight = image_dimensions;
	ddraw_surface_desc.ddpfPixelFormat = ddraw_texture_pixel_format;
	if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc, 
		&ddraw_texture_ptr, NULL) != DD_OK)
		return(NULL);

	// Get a pointer to the Direct3D texture interface.

	if (ddraw_texture_ptr->QueryInterface(IID_IDirect3DTexture2, 
		(void **)&d3d_texture_ptr) != DD_OK) {
		ddraw_texture_ptr->Release();
		return(NULL);
	}

	// Get a handle to the Direct3D texture.

	if (d3d_texture_ptr->GetHandle(d3d_device_ptr, &d3d_texture_handle)
		!= DD_OK) {
		d3d_texture_ptr->Release();
		ddraw_texture_ptr->Release();
		return(NULL);
	}

	// Attempt to load an uninitialised texture surface into the Direct3D
	// texture, which forces it to allocate the texture in video memory.
	// If this fails, texture memory is full.

	dummy_d3d_texture_ptr = d3d_texture_list[image_size_index];
	if (FAILED(d3d_texture_ptr->Load(dummy_d3d_texture_ptr))) {
		d3d_texture_ptr->Release();
		ddraw_texture_ptr->Release();
		return(NULL);
	}

	// Create the hardware texture object, initialise it, and return a pointer
	// to it.

	if ((hardware_texture_ptr = new hardware_texture) == NULL) {
		d3d_texture_ptr->Release();
		ddraw_texture_ptr->Release();
		return(NULL);
	}
	hardware_texture_ptr->image_size_index = image_size_index;
	hardware_texture_ptr->ddraw_texture_ptr = ddraw_texture_ptr;
	hardware_texture_ptr->d3d_texture_ptr = d3d_texture_ptr;
	hardware_texture_ptr->d3d_texture_handle = d3d_texture_handle;
	return(hardware_texture_ptr);
}

//------------------------------------------------------------------------------
// Destroy an existing Direct3D texture.
//------------------------------------------------------------------------------

void
hardware_destroy_texture(void *hardware_texture_ptr)
{
	if (hardware_texture_ptr) {
		hardware_texture *cast_hardware_texture_ptr = 
			(hardware_texture *)hardware_texture_ptr;
		cast_hardware_texture_ptr->d3d_texture_ptr->Release();
		cast_hardware_texture_ptr->ddraw_texture_ptr->Release();
		delete cast_hardware_texture_ptr;
	}
}

//------------------------------------------------------------------------------
// Set the image of a Direct3D texture.
//------------------------------------------------------------------------------

void
hardware_set_texture(cache_entry *cache_entry_ptr)
{
	hardware_texture *hardware_texture_ptr;
	DDSURFACEDESC ddraw_surface_desc;
	LPDIRECTDRAWSURFACE ddraw_texture_ptr;
	LPDIRECT3DTEXTURE2 d3d_texture_ptr;
	byte *surface_ptr;
	int row_pitch;
	int image_size_index, image_dimensions;
	pixmap *pixmap_ptr;

	// Get a pointer to the hardware texture object.

	hardware_texture_ptr = 
		(hardware_texture *)cache_entry_ptr->hardware_texture_ptr;

	// Get the image size index and dimensions.

	image_size_index = hardware_texture_ptr->image_size_index;
	image_dimensions = image_dimensions_list[image_size_index];

	// Get pointers to the texture surface in system memory that is the
	// correct size for this image.

	ddraw_texture_ptr = ddraw_texture_list[image_size_index];
	d3d_texture_ptr = d3d_texture_list[image_size_index];

	// Lock the texture surface in system memory.

	ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
	if (ddraw_texture_ptr->Lock(NULL, &ddraw_surface_desc, 
		DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK) {
		ddraw_texture_ptr->Release();
		return;
	}
	surface_ptr = (byte *)ddraw_surface_desc.lpSurface;
	row_pitch = ddraw_surface_desc.lPitch;
	row_gap = row_pitch - image_dimensions * 2;

	// Get the unlit image pointer and it's dimensions, and set a pointer to
	// the end of the image data.

	pixmap_ptr = cache_entry_ptr->pixmap_ptr;
	image_ptr = pixmap_ptr->image_ptr;
	if (pixmap_ptr->image_is_16_bit)
		image_width = pixmap_ptr->width * 2;
	else
		image_width = pixmap_ptr->width;
	image_height = pixmap_ptr->height;
	end_image_ptr = image_ptr + image_width * image_height;

	// If the pixmap is an 8-bit image, put the transparent index and palette 
	// pointer in static variables so that the assembly code can get to them.
	// Also put the transparency mask into a static variable.

	if (!pixmap_ptr->image_is_16_bit) {
		transparent_index = pixmap_ptr->transparent_index;
		palette_ptr = pixmap_ptr->texture_palette_list;
	}
	transparency_mask16 = (word)texture_pixel_format.alpha_comp_mask;

	// Get the start address of the lit image and it's dimensions.

	new_image_ptr = surface_ptr;
	lit_image_dimensions = image_dimensions;

	// If the pixmap is a 16-bit image, simply copy it to the new image buffer.

	if (pixmap_ptr->image_is_16_bit) {
		__asm {
		
			// Save EBP and ESP so that we can use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// EBX: holds pointer to current row of old image.
			// EDX: holds pointer to current pixel of new image.
			// EBP: holds number of pixels in a row or column.
			// ESP: holds the number of rows left to copy.

			mov ebx, image_ptr
			mov edx, new_image_ptr
			mov ebp, lit_image_dimensions
			mov esp, ebp

		next_row:

			// ECX: holds offset in current row of old image.
			// ESI: holds number of columns left to copy.

			mov ecx, 0
			mov esi, ebp

		next_column:

			// Get the current 16-bit index from the old image, and store it in
			// the new image.

			mov ax, [ebx + ecx]
			or ax, transparency_mask16
			mov [edx], ax

			// Increment the old image offset, wrapping back to zero if the 
			// end of the row is reached.

			add ecx, 2
			cmp ecx, image_width 
			jl next_pixel
			mov ecx, 0

		next_pixel:

			// Increment the new image pointer.

			add edx, 2

			// Decrement the column counter, and copy next pixel in row if
			// there are any left.

			dec esi
			jnz next_column

			// Increment the old image row pointer, and wrap back to the
			// first row if the end of the image has been reached.

			add ebx, image_width
			cmp ebx, end_image_ptr
			jl skip_wrap
			mov ebx, image_ptr

		skip_wrap:

			// Skip over the gap in the new image row.

			add edx, row_gap

			// Decrement the row counter, and copy next row if there are any
			// left.

			dec esp
			jnz next_row

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}

	// If the pixmap is an 8-bit image, convert it to a 16-bit lit image.

	else {
		__asm {
		
			// Save EBP and ESP so that we can use them.

			mov ebp_save, ebp
			mov esp_save, esp

			// EBX: holds pointer to current row of old image.
			// EDX: holds pointer to current pixel of new image.
			// EDI: holds pointer to palette.
			// EBP: holds number of pixels in a row or column.
			// ESP: holds the number of rows left to copy.

			mov ebx, image_ptr
			mov edx, new_image_ptr
			mov edi, palette_ptr
			mov ebp, lit_image_dimensions
			mov esp, ebp

		next_row2:

			// ECX: holds offset in current row of old image.
			// ESI: holds number of columns left to copy.

			mov ecx, 0
			mov esi, ebp

		next_column2:

			// Get the current 8-bit index from the old image, use it to
			// obtain the 16-bit pixel, and store it in the new image.  If 
			// the 8-bit index is not the transparent index, mark the pixel as
			// opaque by setting the transparency bit.

			mov eax, 0
			mov al, [ebx + ecx]
			cmp eax, transparent_index
			jne opaque_pixel2
			mov ax, [edi + eax * 4]	
			jmp store_pixel2
		opaque_pixel2:
			mov ax, [edi + eax * 4]
			or ax, transparency_mask16
		store_pixel2:
			mov [edx], ax

			// Increment the old image offset, wrapping back to zero if the 
			// end of the row is reached.

			inc ecx
			cmp ecx, image_width 
			jl next_pixel2
			mov ecx, 0

		next_pixel2:

			// Increment the new image pointer.

			add edx, 2

			// Decrement the column counter, and copy next pixel in row if
			// there are any left.

			dec esi
			jnz next_column2

			// Increment the old image row pointer, and wrap back to the
			// first row if the end of the image has been reached.

			add ebx, image_width
			cmp ebx, end_image_ptr
			jl skip_wrap2
			mov ebx, image_ptr

		skip_wrap2:

			// Skip over the gap in the new image row.

			add edx, row_gap

			// Decrement the row counter, and copy next row if there are any
			// left.

			dec esp
			jnz next_row2

			// Restore EBP and ESP.

			mov ebp, ebp_save
			mov esp, esp_save
		}
	}

	// Unlock the texture surface.

	ddraw_texture_ptr->Unlock(surface_ptr);

	// Load the texture from system memory into the texture in video memory.

	if (FAILED(hardware_texture_ptr->d3d_texture_ptr->Load(d3d_texture_ptr)))
		diagnose("Failed to load texture into texture memory");
}

//------------------------------------------------------------------------------
// Enable a texture for rendering.
//------------------------------------------------------------------------------

static void
hardware_enable_texture(void *hardware_texture_ptr)
{
	if (hardware_texture_ptr) {
		hardware_texture *cast_hardware_texture_ptr =
			(hardware_texture *)hardware_texture_ptr;

		// Only enable the DirectDraw texture if it's different to the
		// currently enabled DirectDraw texture.

		if (cast_hardware_texture_ptr != curr_hardware_texture_ptr) {
			curr_hardware_texture_ptr = cast_hardware_texture_ptr;
			set_render_state(D3DRENDERSTATE_TEXTUREHANDLE, 
				curr_hardware_texture_ptr->d3d_texture_handle);
		}
	}
}

//------------------------------------------------------------------------------
// Disable texture rendering.
//------------------------------------------------------------------------------

static void
hardware_disable_texture(void)
{
	if (curr_hardware_texture_ptr != NULL) {
		set_render_state(D3DRENDERSTATE_TEXTUREHANDLE, NULL);
		curr_hardware_texture_ptr = NULL;
	}
}

//------------------------------------------------------------------------------
// Render a 2D polygon onto the Direct3D viewport.
//------------------------------------------------------------------------------

#define FAR_PLANE	0.0025f

void
hardware_render_2D_polygon(pixmap *pixmap_ptr, RGBcolour colour,
						   float brightness, float x, float y, float width,
						   float height, float start_u, float start_v,
						   float end_u, float end_v, bool disable_transparency)
{
	float red, green, blue;

	// If transparency is disabled, turn off alpha blending.

	if (disable_transparency)
		set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	
	// Turn off the Z buffer test.

	set_render_state(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);

	// If the polygon has a pixmap, get the cache entry and enable the texture,
	// and use a grayscale colour for lighting.
	
	if (pixmap_ptr) {
		cache_entry *cache_entry_ptr = get_cache_entry(pixmap_ptr, 0);
		hardware_enable_texture(cache_entry_ptr->hardware_texture_ptr);
		red = brightness;
		green = brightness;
		blue = brightness;
	} 
	
	// If the polygon has a colour, disable the texture and use the colour for
	// lighting.
	
	else {
		hardware_disable_texture();
		red = colour.red / 255.0f;
		green = colour.green / 255.0f;
		blue = colour.blue / 255.0f;
	}

	// Construct the Direct3D vertex list for the sky polygon.  The polygon is
	// placed in the far distance so it will appear behind everything else.

	d3d_vertex_list[0] = D3DTLVERTEX(D3DVECTOR(x, y, 0.99999f), 
		FAR_PLANE, D3DRGBA(red, green, blue, 1.0f), D3DRGB(0.0f, 0.0f, 0.0f), 
		start_u, start_v);
	d3d_vertex_list[1] = D3DTLVERTEX(D3DVECTOR(x + width, y, 0.99999f),
		FAR_PLANE, D3DRGBA(red, green, blue, 1.0f), D3DRGB(0.0f, 0.0f, 0.0f),
		end_u, start_v);
	d3d_vertex_list[2] = D3DTLVERTEX(D3DVECTOR(x + width, y + height, 0.99999f),
		FAR_PLANE, D3DRGBA(red, green, blue, 1.0f), D3DRGB(0.0f, 0.0f, 0.0f),
		end_u, end_v);
	d3d_vertex_list[3] = D3DTLVERTEX(D3DVECTOR(x, y + height, 0.99999f), 
		FAR_PLANE, D3DRGBA(red, green, blue, 1.0f), D3DRGB(0.0f, 0.0f, 0.0f),
		start_u, end_v);

	// Render the polygon as a triangle fan.

	d3d_device_ptr->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX,
		d3d_vertex_list, 4, D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);

	// Turn the Z buffer test back on.

	set_render_state(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);

	// If transparency was disabled, re-enable alpha blending.

	if (disable_transparency)
		set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
}

//------------------------------------------------------------------------------
// Render a polygon onto the Direct3D viewport.
//------------------------------------------------------------------------------

void
hardware_render_polygon(spolygon *spolygon_ptr)
{
	pixmap *pixmap_ptr;
	int index;

	// If the polygon has a pixmap, get the cache entry and enable the texture,
	// otherwise disable the texture.

	pixmap_ptr = spolygon_ptr->pixmap_ptr;
	if (pixmap_ptr) {
		cache_entry *cache_entry_ptr = get_cache_entry(pixmap_ptr, 0);
		hardware_enable_texture(cache_entry_ptr->hardware_texture_ptr);
	} else
		hardware_disable_texture();

	// Construct the Direct3D vertex list for this polygon.

	for (index = 0; index < spolygon_ptr->spoints; index++) {
		spoint *spoint_ptr = &spolygon_ptr->spoint_list[index];
		float tz = 1.0f / spoint_ptr->one_on_tz;
		d3d_vertex_list[index] = D3DTLVERTEX(D3DVECTOR(spoint_ptr->sx,
			spoint_ptr->sy, 1.0f - spoint_ptr->one_on_tz), 
			spoint_ptr->one_on_tz, D3DRGBA(spoint_ptr->colour.red, 
			spoint_ptr->colour.green, spoint_ptr->colour.blue, 
			spolygon_ptr->alpha), D3DRGB(0.0f, 0.0f, 0.0f), 
			spoint_ptr->u_on_tz * tz, 
			spoint_ptr->v_on_tz * tz);
	}

	// Render the polygon as a triangle fan.

	d3d_device_ptr->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX,
		d3d_vertex_list, spolygon_ptr->spoints, D3DDP_DONOTCLIP |
		D3DDP_DONOTUPDATEEXTENTS);
}

//==============================================================================
// 2D drawing functions.
//==============================================================================

//------------------------------------------------------------------------------
// Function to draw a bitmap onto the frame buffer surface at the given (x,y)
// coordinates.  Drawing onto an 8-bit frame buffer is not supported by this
// function.
//------------------------------------------------------------------------------

static void
draw_bitmap(bitmap *bitmap_ptr, int x, int y)
{
	DDSURFACEDESC ddraw_surface_desc;
	int clipped_x, clipped_y;
	int clipped_width, clipped_height;
	byte *surface_ptr;
	int fb_bytes_per_row, bytes_per_pixel;
	int row;

	// If the image is completely off screen then return without having drawn
	// anything.

	if (x >= display_width || y >= display_height ||
		x + bitmap_ptr->width <= 0 || y + bitmap_ptr->height <= 0) 
		return;

	// If the frame buffer x or y coordinates are negative, then we clamp them
	// at zero and adjust the image coordinates and size to match.

	if (x < 0) {
		clipped_x = -x;
		clipped_width = bitmap_ptr->width - clipped_x;
		x = 0;
	} else {
		clipped_x = 0;
		clipped_width = bitmap_ptr->width;
	}	
	if (y < 0) {
		clipped_y = -y;
		clipped_height = bitmap_ptr->height - clipped_y;
		y = 0;
	} else {
		clipped_y = 0;
		clipped_height = bitmap_ptr->height;
	}

	// If the image crosses the right or bottom edge of the display, we must
	// adjust the size of the area we are going to draw even further.

	if (x + clipped_width > display_width)
		clipped_width = display_width - x;
	if (y + clipped_height > display_height)
		clipped_height = display_height - y;

	// Lock the frame buffer surface.

	ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
	if (framebuf_surface_ptr->Lock(NULL, &ddraw_surface_desc,
		DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
		return;
	surface_ptr = (byte *)ddraw_surface_desc.lpSurface;
	fb_bytes_per_row = ddraw_surface_desc.lPitch;
	bytes_per_pixel = display_depth / 8;

	// Determine the transparent index, and the palette pointer.

	palette_ptr = bitmap_ptr->palette;
	transparent_index = bitmap_ptr->transparent_index;
	image_width = bitmap_ptr->width;
	u = clipped_x % bitmap_ptr->width;
	v = clipped_y % bitmap_ptr->height;

	// Compute the starting frame buffer and image pointers.

	fb_ptr = surface_ptr + (y * fb_bytes_per_row) + (x * bytes_per_pixel);
	image_ptr = bitmap_ptr->pixels + v * image_width;
	
	// The span width is the same as the clipped width.

	span_width = clipped_width;

	// Now render the bitmap.  Note that rendering to an 8-bit frame buffer is
	// not supported.

	switch (bytes_per_pixel) {
	case 2:
		for (row = 0; row < clipped_height; row++) {
			render_linear_span16(false);
			fb_ptr += fb_bytes_per_row;
			image_ptr += image_width;
		}
		break;
	case 3:
		for (row = 0; row < clipped_height; row++) {
			render_linear_span24(false);
			fb_ptr += fb_bytes_per_row;
			image_ptr += image_width;
		}
		break;
	case 4:
		for (row = 0; row < clipped_height; row++) {
			render_linear_span32(false);
			fb_ptr += fb_bytes_per_row;
			image_ptr += image_width;
		}
		break;
	}

	// Unlock the frame buffer surface.

	framebuf_surface_ptr->Unlock(surface_ptr);
}

//------------------------------------------------------------------------------
// Function to draw a pixmap at the given brightness index onto the frame buffer
// surface at the given (x,y) coordinates.
//------------------------------------------------------------------------------

void
draw_pixmap(pixmap *pixmap_ptr, int brightness_index, int x, int y)
{
	DDSURFACEDESC ddraw_surface_desc;
	int clipped_x, clipped_y;
	int clipped_width, clipped_height;
	byte *surface_ptr;
	int fb_bytes_per_row, bytes_per_pixel;
	int fb_row_gap, image_row_gap;
	int row, col;
	byte *palette_index_table;

	// If the pixmap is completely off screen then return without having drawn
	// anything.

	if (x >= display_width || y >= display_height ||
		x + pixmap_ptr->width <= 0 || y + pixmap_ptr->height <= 0) 
		return;

	// If the frame buffer x or y coordinates are negative, then we clamp them
	// at zero and adjust the image coordinates and size to match.

	if (x < 0) {
		clipped_x = -x;
		clipped_width = pixmap_ptr->width - clipped_x;
		x = 0;
	} else {
		clipped_x = 0;
		clipped_width = pixmap_ptr->width;
	}	
	if (y < 0) {
		clipped_y = -y;
		clipped_height = pixmap_ptr->height - clipped_y;
		y = 0;
	} else {
		clipped_y = 0;
		clipped_height = pixmap_ptr->height;
	}

	// If the pixmap crosses the right or bottom edge of the display, we must
	// adjust the size of the area we are going to draw even further.

	if (x + clipped_width > display_width)
		clipped_width = display_width - x;
	if (y + clipped_height > display_height)
		clipped_height = display_height - y;

	// Lock the frame buffer surface.

	ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
	if (framebuf_surface_ptr->Lock(NULL, &ddraw_surface_desc,
		DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
		return;
	surface_ptr = (byte *)ddraw_surface_desc.lpSurface;
	fb_bytes_per_row = ddraw_surface_desc.lPitch;
	bytes_per_pixel = display_depth / 8;

	// Determine the transprency mask or index, and the palette pointer.  For
	// 16-bit pixmaps, the image width must be doubled to give the width in
	// bytes.

	if (pixmap_ptr->image_is_16_bit) {
		transparency_mask32 = texture_pixel_format.alpha_comp_mask;
		palette_ptr = light_table[brightness_index];
		image_width = pixmap_ptr->width * 2;
		u = (clipped_x % pixmap_ptr->width) * 2;
	} else {
		palette_ptr = pixmap_ptr->display_palette_list + 
			brightness_index * pixmap_ptr->colours;
		transparent_index = pixmap_ptr->transparent_index;
		image_width = pixmap_ptr->width;
		u = clipped_x % pixmap_ptr->width;
	}
	v = clipped_y % pixmap_ptr->height;

	// Compute the starting frame buffer and image pointers.

	fb_ptr = surface_ptr + (y * fb_bytes_per_row) + (x * bytes_per_pixel);
	image_ptr = pixmap_ptr->image_ptr + v * image_width;
	
	// The span width is the same as the clipped width.

	span_width = clipped_width;

	// Now render the pixmap.  Note that rendering to an 8-bit frame buffer is
	// a special case that is only defined for 8-bit pixmaps.

	switch (bytes_per_pixel) {
	case 1:
		if (!pixmap_ptr->image_is_16_bit) {
			fb_row_gap = fb_bytes_per_row - clipped_width * bytes_per_pixel;	
			image_row_gap = image_width - clipped_width;
			palette_index_table = pixmap_ptr->palette_index_table;
			image_ptr += u;
			for (row = 0; row < clipped_height; row++) {
				for (col = 0; col < clipped_width; col++) {
					if (*image_ptr != transparent_index)
						*fb_ptr = palette_index_table[*image_ptr];
					fb_ptr++;
					image_ptr++;
				}
				fb_ptr += fb_row_gap;
				image_ptr += image_row_gap;
			}
		}
		break;
	case 2:
		for (row = 0; row < clipped_height; row++) {
			render_linear_span16(pixmap_ptr->image_is_16_bit);
			fb_ptr += fb_bytes_per_row;
			image_ptr += image_width;
		}
		break;
	case 3:
		for (row = 0; row < clipped_height; row++) {
			render_linear_span24(pixmap_ptr->image_is_16_bit);
			fb_ptr += fb_bytes_per_row;
			image_ptr += image_width;
		}
		break;
	case 4:
		for (row = 0; row < clipped_height; row++) {
			render_linear_span32(pixmap_ptr->image_is_16_bit);
			fb_ptr += fb_bytes_per_row;
			image_ptr += image_width;
		}
		break;
	}

	// Unlock the frame buffer surface.

	framebuf_surface_ptr->Unlock(surface_ptr);
}

//------------------------------------------------------------------------------
// Convert an RGB colour to a display pixel.
//------------------------------------------------------------------------------

pixel
RGB_to_display_pixel(RGBcolour colour)
{
	pixel red, green, blue;

	// Compute the pixel for this RGB colour.

	red = (pixel)colour.red & display_pixel_format.red_mask;
	red >>= display_pixel_format.red_right_shift;
	red <<= display_pixel_format.red_left_shift;
	green = (pixel)colour.green & display_pixel_format.green_mask;
	green >>= display_pixel_format.green_right_shift;
	green <<= display_pixel_format.green_left_shift;
	blue = (pixel)colour.blue & display_pixel_format.blue_mask;
	blue >>= display_pixel_format.blue_right_shift;
	blue <<= display_pixel_format.blue_left_shift;
	return(red | green | blue);
}

//------------------------------------------------------------------------------
// Convert an RGB colour to a texture pixel.
//------------------------------------------------------------------------------

pixel
RGB_to_texture_pixel(RGBcolour colour)
{
	pixel red, green, blue;

	// Compute the pixel for this RGB colour.

	red = (pixel)colour.red & texture_pixel_format.red_mask;
	red >>= texture_pixel_format.red_right_shift;
	red <<= texture_pixel_format.red_left_shift;
	green = (pixel)colour.green & texture_pixel_format.green_mask;
	green >>= texture_pixel_format.green_right_shift;
	green <<= texture_pixel_format.green_left_shift;
	blue = (pixel)colour.blue & texture_pixel_format.blue_mask;
	blue >>= texture_pixel_format.blue_right_shift;
	blue <<= texture_pixel_format.blue_left_shift;
	return(red | green | blue);
}

//------------------------------------------------------------------------------
// Return a pointer to the standard RGB palette.
//------------------------------------------------------------------------------

RGBcolour *
get_standard_RGB_palette(void)
{
	return((RGBcolour *)standard_RGB_palette);
}

//------------------------------------------------------------------------------
// Return an index to the nearest colour in the standard palette.
//------------------------------------------------------------------------------

byte
get_standard_palette_index(RGBcolour colour)
{
	return(GetNearestPaletteIndex(standard_palette_handle,
		RGB((byte)colour.red, (byte)colour.green, (byte)colour.blue)));
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
	if (task_bar_enabled)
		draw_title();
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
	texture *bg_texture_ptr;
	texture *fg_texture_ptr;
	pixmap *fg_pixmap_ptr;
	bitmap *image_bitmap_ptr;
	int row, col;
	HDC hdc, bitmap_hdc;
	HBITMAP old_bitmap_handle;
	RECT bitmap_rect;
	byte *bitmap_row_ptr;
	byte *pixmap_row_ptr;
	byte colour_index;
	int x_offset, y_offset;
	unsigned int alignment;
	int popup_width, popup_height;
	RGBcolour fg_RGB_palette[2];

	// If this popup has a background texture, create it's 16-bit display
	// palette list, and set the size of the popup to be the size of the
	// background texture.  Otherwise use the popup's default size.

	if ((bg_texture_ptr = popup_ptr->bg_texture_ptr) != NULL) {
		if (!bg_texture_ptr->is_16_bit)
			bg_texture_ptr->create_display_palette_list();
		popup_width = bg_texture_ptr->width;
		popup_height = bg_texture_ptr->height;
	} else {
		popup_width = popup_ptr->width;
		popup_height = popup_ptr->height;
	}

	// If this popup has no foreground texture, then we're done.

	if ((fg_texture_ptr = popup_ptr->fg_texture_ptr) == NULL)
		return;
		
	// Create the foreground pixmap object and initialise it.

	NEWARRAY(fg_pixmap_ptr, pixmap, 1);
	if (fg_pixmap_ptr == NULL)
		memory_error("popup pixmap");
	fg_pixmap_ptr->image_is_16_bit = false;
	fg_pixmap_ptr->image_size = popup_width * popup_height;
	NEWARRAY(fg_pixmap_ptr->image_ptr, imagebyte, fg_pixmap_ptr->image_size);
	if (fg_pixmap_ptr->image_ptr == NULL)
		memory_error("popup pixmap image");
	fg_pixmap_ptr->width = popup_width;
	fg_pixmap_ptr->height = popup_height;
	fg_pixmap_ptr->transparent_index = 2;

	// Initialise the foreground texture.

	fg_texture_ptr->is_16_bit = false;
	fg_texture_ptr->transparent = popup_ptr->transparent_background;
	fg_texture_ptr->width = popup_width;
	fg_texture_ptr->height = popup_height;
	fg_texture_ptr->pixmaps = 1;
	fg_texture_ptr->pixmap_list = fg_pixmap_ptr;

	// Use a two-colour palette containing the popup and text colours as the
	// only entries.

	fg_RGB_palette[0].set(popup_ptr->colour);
	fg_RGB_palette[1].set(popup_ptr->text_colour);
	if (!fg_texture_ptr->create_RGB_palette(2, BRIGHTNESS_LEVELS, 
		fg_RGB_palette))
		memory_error("popup RGB palette");
			
	// Create the display palette list for the foreground texture.

	if (!fg_texture_ptr->create_display_palette_list())
		memory_error("popup display palette");

	// Create a bitmap for the foreground pixmap image.

	if ((image_bitmap_ptr = create_bitmap(popup_width, popup_height,
		 fg_texture_ptr->colours, fg_texture_ptr->RGB_palette,
		 fg_texture_ptr->display_palette_list, -1)) == NULL)
		 memory_error("popup image bitmap");

	// Set the rectangle representing the bitmap area.

	bitmap_rect.left = 0;
	bitmap_rect.top = 0;
	bitmap_rect.right = popup_width;
	bitmap_rect.bottom = popup_height;

	// Create a device context and select the bitmap into it.

	hdc = GetDC(main_window_handle);
	bitmap_hdc = CreateCompatibleDC(hdc);
	ReleaseDC(main_window_handle, hdc);
	old_bitmap_handle = SelectBitmap(bitmap_hdc, image_bitmap_ptr->handle);

	// Initialise the bitmap with either the popup colour or the standard
	// transparent index.

	if (popup_ptr->transparent_background)
		colour_index = 2;
	else
		colour_index = 0;
	for (row = 0; row < popup_height; row++) {
		bitmap_row_ptr = image_bitmap_ptr->pixels + 
			row * image_bitmap_ptr->bytes_per_row;
		for (col = 0; col < popup_width; col++)
			bitmap_row_ptr[col] = colour_index;
	}

	
	// Select the horizontal text alignment mode.

	switch (popup_ptr->text_alignment) {
	case TOP_LEFT:
	case LEFT:
	case BOTTOM_LEFT:
		alignment = DT_LEFT;
		break;
	case TOP:
	case CENTRE:
	case BOTTOM:
		alignment = DT_CENTER;
		break;
	case TOP_RIGHT:
	case RIGHT:
	case BOTTOM_RIGHT:
		alignment = DT_RIGHT;
	}

	// Adjust the bitmap rectangle so that it snaps to the vertical size
	// of the text.

	DrawText(bitmap_hdc, popup_ptr->text, strlen(popup_ptr->text),
		&bitmap_rect, alignment | DT_WORDBREAK | DT_EXPANDTABS |
		DT_NOPREFIX | DT_CALCRECT);

	// Now compute the offset needed to align the bitmap horizontally and
	// vertically, and adjust the bitmap rectangle.

	switch (popup_ptr->text_alignment) {
	case TOP_LEFT:
	case LEFT:
	case BOTTOM_LEFT:
		x_offset = 0;
		break;
	case TOP:
	case CENTRE:
	case BOTTOM:
		x_offset = (popup_width - (bitmap_rect.right - bitmap_rect.left)) / 2;
		break;
	case TOP_RIGHT:
	case RIGHT:
	case BOTTOM_RIGHT:
		x_offset = popup_width - (bitmap_rect.right - bitmap_rect.left);
	}
	switch (popup_ptr->text_alignment) {
	case TOP_LEFT:
	case TOP:
	case TOP_RIGHT:
		y_offset = 0;
		break;
	case LEFT:
	case CENTRE:
	case RIGHT:
		y_offset = (popup_height - (bitmap_rect.bottom - bitmap_rect.top)) / 2;
		break;
	case BOTTOM_LEFT:
	case BOTTOM:
	case BOTTOM_RIGHT:
		y_offset = popup_height - (bitmap_rect.bottom - bitmap_rect.top);
	}
	bitmap_rect.left += x_offset;
	bitmap_rect.right += x_offset;
	bitmap_rect.top += y_offset;
	bitmap_rect.bottom += y_offset;

	// Now draw the text for real.  It is drawn in the popup colour
	// first, then the text colour offset by one pixel.  This creates
	// a nice effect that is easy to read over a texture if the popup
	// and text colours are chosen approapiately.

	SetBkMode(bitmap_hdc, TRANSPARENT);
	SetTextColor(bitmap_hdc, RGB(popup_ptr->colour.red,
		popup_ptr->colour.green, popup_ptr->colour.blue));
	DrawText(bitmap_hdc, popup_ptr->text, strlen(popup_ptr->text),
		&bitmap_rect, alignment | DT_WORDBREAK | DT_EXPANDTABS |
		DT_NOPREFIX);
	bitmap_rect.left++;
	bitmap_rect.right++;
	SetTextColor(bitmap_hdc, RGB(popup_ptr->text_colour.red,
		popup_ptr->text_colour.green, popup_ptr->text_colour.blue));
	DrawText(bitmap_hdc, popup_ptr->text, strlen(popup_ptr->text),
		&bitmap_rect, alignment | DT_WORDBREAK | DT_EXPANDTABS |
		DT_NOPREFIX);

	// Copy the bitmap back into the pixmap image.

	for (row = 0; row < popup_height; row++) {
		pixmap_row_ptr = fg_pixmap_ptr->image_ptr + row * popup_width;
		bitmap_row_ptr = image_bitmap_ptr->pixels + 
			row * image_bitmap_ptr->bytes_per_row;
		for (col = 0; col < popup_width; col++)
			pixmap_row_ptr[col] = bitmap_row_ptr[col];
	}

	// Select the default bitmap into the device context before deleting
	// it and the bitmap.

	SelectBitmap(bitmap_hdc, old_bitmap_handle);
	DeleteDC(bitmap_hdc);
	DEL(image_bitmap_ptr, bitmap);
}

//------------------------------------------------------------------------------
// Get the relative or absolute position of the mouse.
//------------------------------------------------------------------------------

void
get_mouse_position(int *x, int *y, bool relative)
{
	POINT cursor_pos;

	// Get the absolute cursor position.

	GetCursorPos(&cursor_pos);

	// If a relative position is requested...

	if (relative) {

		// If running full-screen, adjust the cursor position by the width and
		// height factors.

		if (full_screen) {
			cursor_pos.x = (int)((float)cursor_pos.x * width_factor);
			cursor_pos.y = (int)((float)cursor_pos.y * width_factor);
		}

		// If not running full-screen, adjust the cursor position so that it's
		// relative to the main window position.

		else {
			POINT main_window_pos;

			main_window_pos.x = 0;
			main_window_pos.y = 0;
			ClientToScreen(main_window_handle, &main_window_pos);
			cursor_pos.x -= main_window_pos.x;
			cursor_pos.y -= main_window_pos.y;
		}
	}

	// Pass the cursor position back via the parameters.

	*x = cursor_pos.x;
	*y = cursor_pos.y;
}

//------------------------------------------------------------------------------
// Set the arrow cursor.
//------------------------------------------------------------------------------

void
set_arrow_cursor(void)
{	
	set_active_cursor(arrow_cursor_ptr);
}

//------------------------------------------------------------------------------
// Set a movement cursor.
//------------------------------------------------------------------------------

void
set_movement_cursor(arrow movement_arrow)
{	
	set_active_cursor(movement_cursor_ptr_list[movement_arrow]);
}

//------------------------------------------------------------------------------
// Set the hand cursor.
//------------------------------------------------------------------------------

void
set_hand_cursor(void)
{	
	set_active_cursor(hand_cursor_ptr);
}

//------------------------------------------------------------------------------
// Set the crosshair cursor.
//------------------------------------------------------------------------------

void
set_crosshair_cursor(void)
{	
	set_active_cursor(crosshair_cursor_ptr);
}

//------------------------------------------------------------------------------
// Capture the mouse.
//------------------------------------------------------------------------------

void
capture_mouse(void)
{
	SetCapture(main_window_handle);
}

//------------------------------------------------------------------------------
// Release the mouse.
//------------------------------------------------------------------------------

void
release_mouse(void)
{
	ReleaseCapture();
}

//------------------------------------------------------------------------------
// Get the time since Windows last started, in milliseconds.
//------------------------------------------------------------------------------

int
get_time_ms(void)
{
	return(GetTickCount());
}

//------------------------------------------------------------------------------
// Load wave data into a wave object.
//------------------------------------------------------------------------------

bool
load_wave_data(wave *wave_ptr, char *wave_file_buffer, int wave_file_size)
{
	MMIOINFO info;
	MMCKINFO parent, child;
	HMMIO handle;
	WAVEFORMATEX *wave_format_ptr;
	char *wave_data_ptr;
	int wave_data_size;

	// Initialise the parent and child MMCKINFO structures.

	memset(&parent, 0, sizeof(MMCKINFO));
	memset(&child, 0, sizeof(MMCKINFO));

	// Open the specified wave file; the file has already been loaded into a
	// memory buffer.

	memset(&info, 0, sizeof(MMIOINFO));
	info.fccIOProc = FOURCC_MEM;
	info.pchBuffer = wave_file_buffer;
	info.cchBuffer = wave_file_size;
	if ((handle = mmioOpen(NULL, &info, MMIO_READ | MMIO_ALLOCBUF)) == NULL)
		return(false);

	// Verify we've open a wave file by descending into the WAVE chunk.

	parent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	if (mmioDescend(handle, &parent, NULL, MMIO_FINDRIFF)) {
		mmioClose(handle, 0);
		return(false);
	}

	// Descend into the fmt chunk.

	child.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if (mmioDescend(handle, &child, &parent, 0)) {
		mmioClose(handle, 0);
		return(false);
	}

	// Allocate the wave format structure.

	if ((wave_format_ptr = new WAVEFORMATEX) == NULL) {
		mmioClose(handle, 0);
		return(false);
	}

	// Read the wave format.

	if (mmioRead(handle, (char *)wave_format_ptr, sizeof(WAVEFORMATEX)) !=
		sizeof(WAVEFORMATEX)) {
		delete wave_format_ptr;
		mmioClose(handle, 0);
		return(false);
	}

	// Verify that the wave is in PCM format.

	if (wave_format_ptr->wFormatTag != WAVE_FORMAT_PCM) {
		delete wave_format_ptr;
		mmioClose(handle, 0);
		return(false);
	}

	// Ascend out of the fmt chunk.

	if (mmioAscend(handle, &child, 0)) {
		delete wave_format_ptr;
		mmioClose(handle, 0);
		return(false);
	}

	// Descend into the data chunk.

	child.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if (mmioDescend(handle, &child, &parent, MMIO_FINDCHUNK)) {
		delete wave_format_ptr;
		mmioClose(handle, 0);
		return(false);
	}

	// Allocate the wave data buffer.

	wave_data_size = child.cksize;
	if ((wave_data_ptr = new char[wave_data_size]) == NULL) {
		delete wave_format_ptr;
		mmioClose(handle, 0);
		return(false);
	}

	// Read the wave data into the buffer.

	if (mmioRead(handle, wave_data_ptr, wave_data_size) != wave_data_size) {
		mmioClose(handle, 0);
		delete []wave_data_ptr;
		return(false);
	}

	// Set up the wave structure, close the file and return with success.

	wave_ptr->format_ptr = wave_format_ptr;
	wave_ptr->data_ptr = wave_data_ptr;
	wave_ptr->data_size = wave_data_size;
	mmioClose(handle, 0);
	return(true);
}

//------------------------------------------------------------------------------
// Load a wave file into a wave object.
//------------------------------------------------------------------------------

bool
load_wave_file(wave *wave_ptr, const char *wave_URL, const char *wave_file_path)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr;
	WAVEFORMATEX *wave_format_ptr;
	char *wave_data_ptr;
	int wave_data_size;
	LPVOID buffer1_ptr, buffer2_ptr;
	DWORD buflen1, buflen2;

	// Create an A3D source to recieve the wave data.

	if (FAILED(a3d_object_ptr->NewSource(A3DSOURCE_TYPE3D, 
		&a3d_source_ptr))) {
		failed_to_create("A3D source");
		return(false);
	}

	// Load the wave file into the A3D source.

	if (FAILED(a3d_source_ptr->LoadWaveFile((char *)wave_file_path))) {
		delete a3d_source_ptr;
		failed_to("load wave file");
		return(false);
	}

	// Determine the wave data size, and allocate the wave data buffer.

	wave_data_size = a3d_source_ptr->GetWaveSize();
	if ((wave_data_ptr = new char[wave_data_size]) == NULL) {
		delete a3d_source_ptr;
		failed_to_create("wave data buffer");
		return(false);
	}

	// Lock the A3D source buffer.

	if (FAILED(a3d_source_ptr->Lock(0, wave_data_size, &buffer1_ptr, 
		&buflen1, &buffer2_ptr, &buflen2, A3D_ENTIREBUFFER))) {
		a3d_source_ptr->Release();
		failed_to("lock A3D source");
		return(false);
	}

	// Copy the A3D source buffer into the wave data buffer.

	CopyMemory(wave_data_ptr, buffer1_ptr, buflen1);
	if (buffer2_ptr != NULL)
		CopyMemory(wave_data_ptr + buflen1, buffer2_ptr, buflen2);

	// Unlock the A3D source buffer.

	if (FAILED(a3d_source_ptr->Unlock(buffer1_ptr, buflen1, buffer2_ptr,
		buflen2))) {
		a3d_source_ptr->Release();
		failed_to("unlock A3D source");
		return(false);
	}		
	
	// Allocate the wave format structure, and fill it in.

	if ((wave_format_ptr = new WAVEFORMATEX) == NULL) {
		delete a3d_source_ptr;
		failed_to_create("wave format");
		return(false);
	}
	if (FAILED(a3d_source_ptr->GetWaveFormat(wave_format_ptr))) {
		delete wave_format_ptr;
		delete a3d_source_ptr;
		failed_to_get("wave format");
		return(false);
	}

	// Release the A3D source.

	a3d_source_ptr->Release();
	
	// Set up the wave structure and return with success.

	wave_ptr->format_ptr = wave_format_ptr;
	wave_ptr->data_ptr = wave_data_ptr;
	wave_ptr->data_size = wave_data_size;
	return(true);

#else // !SUPPORT_A3D
	bool result;

	// load the wave file into a memory buffer.

	if (!push_file(wave_file_path, false))
		return(false);

	// Now load the wave data into the wave object.

	result = load_wave_data(wave_ptr, top_file_ptr->file_buffer,
		top_file_ptr->file_size);

	// Close the wave file and return success or failure.

	pop_file();
	return(result);
#endif
}

#ifdef SUPPORT_A3D

//------------------------------------------------------------------------------
// Set the render mode for an A3D sound source.
//------------------------------------------------------------------------------

static void
set_sound_mode(LPA3DSOURCE a3d_source_ptr, bool occlusions, bool reflections)
{
	int mode;

	mode = A3DSOURCE_RENDERMODE_A3D;
	if (occlusions)
		mode |= A3DSOURCE_RENDERMODE_OCCLUSIONS;
	if (reflections)
		mode |= A3DSOURCE_RENDERMODE_1ST_REFLECTIONS;
	a3d_source_ptr->SetRenderMode(mode);
}

#endif

//------------------------------------------------------------------------------
// Write the specified wave data to the given sound buffer, starting from the
// given write position.
//------------------------------------------------------------------------------

void
update_sound_buffer(void *sound_buffer_ptr, char *data_ptr, int data_size,
				   int data_start)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr;
#else
	LPDIRECTSOUNDBUFFER dsound_buffer_ptr;
	HRESULT result;
#endif
	LPVOID buffer1_ptr, buffer2_ptr;
	DWORD buflen1, buflen2;

#ifdef SUPPORT_A3D

	// Lock the buffer.

	a3d_source_ptr = (LPA3DSOURCE)sound_buffer_ptr;
	if (FAILED(a3d_source_ptr->Lock(data_start, data_size, 
		&buffer1_ptr, &buflen1, &buffer2_ptr, &buflen2, 0))) {
		failed_to("lock A3D source");
		return;
	}

	// Copy the wave data into the sound buffer.

	memcpy(buffer1_ptr, data_ptr, buflen1);
	if (buffer2_ptr != NULL)
		memcpy(buffer2_ptr, data_ptr + buflen1, buflen2);

	// Unlock the buffer.

	if (FAILED(a3d_source_ptr->Unlock(buffer1_ptr, buflen1, buffer2_ptr,
		buflen2)))
		failed_to("unlock A3D source");

#else

	// Lock the buffer.

	dsound_buffer_ptr = (LPDIRECTSOUNDBUFFER)sound_buffer_ptr;
	result = dsound_buffer_ptr->Lock(data_start, data_size, 
		&buffer1_ptr, &buflen1, &buffer2_ptr, &buflen2, 0);
	if (result == DSERR_BUFFERLOST) {
		dsound_buffer_ptr->Restore();
		result = dsound_buffer_ptr->Lock(data_start, data_size, 
			&buffer1_ptr, &buflen1, &buffer2_ptr, &buflen2, 0);
	}
	if (result != DS_OK) {
		failed_to("lock DirectSound buffer");
		return;
	}

	// Copy the wave data into the DirectSoundBuffer object.

	CopyMemory(buffer1_ptr, data_ptr, buflen1);
	if (buffer2_ptr != NULL)
		CopyMemory(buffer2_ptr, data_ptr + buflen1, buflen2);

	// Unlock the buffer.

	if (dsound_buffer_ptr->Unlock(buffer1_ptr, buflen1, buffer2_ptr,
		buflen2) != DS_OK)
		failed_to("unlock DirectSound buffer");

#endif
}

//------------------------------------------------------------------------------
// Create a sound buffer for the given sound.
//------------------------------------------------------------------------------

bool
create_sound_buffer(sound *sound_ptr)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr;
	int flags;
#else
	DSBUFFERDESC dsbdesc;
	LPDIRECTSOUNDBUFFER dsound_buffer_ptr;
#endif
	wave *wave_ptr;
	WAVEFORMATEX *wave_format_ptr;

	// If there is no wave defined, the sound buffer cannot be created.

	wave_ptr = sound_ptr->wave_ptr;
	if (wave_ptr == NULL || wave_ptr->data_size == 0)
		return(false);

	// Get a pointer to the wave format.

	wave_format_ptr = (WAVEFORMATEX *)wave_ptr->format_ptr;

#ifdef SUPPORT_A3D

	// Create a new A3D sound source.

	if (sound_ptr->streaming)
		flags = A3DSOURCE_TYPESTREAMED | A3DSOURCE_TYPEUNMANAGED; 
	else
		flags = A3DSOURCE_TYPE3D;
	if (FAILED(a3d_object_ptr->NewSource(flags, &a3d_source_ptr))) {
		failed_to_create("A3D source");
		return(false);
	}
	
	// Initialise the sound source's wave format.

	if (FAILED(a3d_source_ptr->SetWaveFormat(wave_format_ptr))) {
		failed_to_set("wave format");
		a3d_source_ptr->Release();
		return(false);
	}

	// Allocate the wave data buffer.

	if (FAILED(a3d_source_ptr->AllocateWaveData(wave_ptr->data_size))) {
		failed_to_create("wave data buffer");
		a3d_source_ptr->Release();
		return(false);
	}

	// If there is wave data present, initialise the wave data buffer with
	// it.

	if (wave_ptr->data_ptr)
		update_sound_buffer(a3d_source_ptr, wave_ptr->data_ptr, 
			wave_ptr->data_size, 0);

	// Set the render mode for this sound source; if it's an ambient sound,
	// don't turn on occlusions.

	if (sound_ptr->ambient)
		set_sound_mode(a3d_source_ptr, false, true);
	else
		set_sound_mode(a3d_source_ptr, true, true);
	
	// Set the volume of the sound source.

	a3d_source_ptr->SetGain(sound_ptr->volume);

	// Set the distance scale model to provide the desired sound rolloff.

	a3d_source_ptr->SetDistanceModelScale(sound_ptr->rolloff);

	// If this is a flood sound, the minimum distance is set to the
	// radius to ensure no sound drop-off occurs; if the radius is
	// zero, then use a very large radius instead.
	
	if (sound_ptr->flood) {
		float scaled_radius = sound_ptr->radius * world_ptr->audio_scale;
		if (scaled_radius > 0.0f)
			a3d_source_ptr->SetMinMaxDistance(scaled_radius, 5000.0f,
				A3D_AUDIBLE);
		else
			a3d_source_ptr->SetMinMaxDistance(5000.0f, 5000.0f,
				A3D_AUDIBLE);
	}

	// Bind the sound source to the geometry object for the time time,
	// so it can immediately be positioned in 3D space.

	a3d_geometry_ptr->LoadIdentity();
	a3d_geometry_ptr->BindSource(a3d_source_ptr);

	// Store a void pointer to the A3D source object in the sound.

	sound_ptr->sound_buffer_ptr = (void *)a3d_source_ptr;
	return(true);

#else // !SUPPORT_A3D

	// Initialise the DirectSound buffer description structure.

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_STATIC;
	dsbdesc.dwBufferBytes = wave_ptr->data_size;
	dsbdesc.lpwfxFormat = wave_format_ptr;

	// Create the DirectSoundBuffer object.

	if (dsound_object_ptr->CreateSoundBuffer(&dsbdesc, &dsound_buffer_ptr,
		NULL) != DS_OK)
		return(false);

	// If there is wave data present, initialise the wave data buffer with
	// it.

	if (wave_ptr->data_ptr)
		update_sound_buffer(dsound_buffer_ptr, wave_ptr->data_ptr, 
			wave_ptr->data_size, 0);

	// Store a void pointer to the DirectSoundBuffer object in the sound
	// object.

	sound_ptr->sound_buffer_ptr = (void *)dsound_buffer_ptr;
	return(true);

#endif
}

//------------------------------------------------------------------------------
// Destroy a sound buffer.
//------------------------------------------------------------------------------

void
destroy_sound_buffer(sound *sound_ptr)
{
	void *sound_buffer_ptr = sound_ptr->sound_buffer_ptr;
	if (sound_buffer_ptr) {
#ifdef SUPPORT_A3D
		((LPA3DLISTENER)sound_buffer_ptr)->Release();
#else
		((LPDIRECTSOUNDBUFFER)sound_buffer_ptr)->Release();
#endif
		sound_ptr->sound_buffer_ptr = NULL;
	}
}

#ifdef SUPPORT_A3D

//------------------------------------------------------------------------------
// Reset the audio system.
//------------------------------------------------------------------------------

void
reset_audio(void)
{
	curr_audio_polygon_ID = 0;
	prev_audio_column = -1;
	prev_audio_row = -1;
	prev_audio_level = -1;
	a3d_object_ptr->Clear();
}

//------------------------------------------------------------------------------
// Create an audio block.
//------------------------------------------------------------------------------

void *
create_audio_block(int min_column, int min_row, int min_level, 
				   int max_column, int max_row, int max_level)
{
	LPA3DLIST a3d_list_ptr;
	int column, row, level;
	square *square_ptr;
	block *block_ptr;
	int polygon_no, vertex_no;
	bool *polygon_active_ptr;
	polygon *polygon_ptr;
	vertex *vertex0_ptr, *vertex1_ptr, *vertex2_ptr, *vertex3_ptr;
	float scale;

	// If the creation of the A3D list object fails, just return a NULL pointer.
 
	if (FAILED(a3d_geometry_ptr->NewList(&a3d_list_ptr)))
		return(NULL);

	// Begin the creation of the list.

	a3d_list_ptr->Begin();
	a3d_geometry_ptr->LoadIdentity();
	a3d_geometry_ptr->BindMaterial(a3d_material_ptr);
	scale = world_ptr->audio_scale;

	// Step through the given range of blocks on the map, ignoring squares
	// with one or more sounds attached to them.

	for (level = min_level; level <= max_level; level++)
		for (row = min_row; row <= max_row; row++)
			for (column = min_column; column <= max_column; column++) {
				if ((square_ptr = world_ptr->get_square_ptr(column, row, level))
					== NULL || square_ptr->sounds > 0 || (block_ptr = 
					square_ptr->block_ptr) == NULL)
					continue;

				// Step through the polygon definitions in this block that are
				// active.

				for (polygon_no = 0; polygon_no < block_ptr->polygons;
					polygon_no++) {
					polygon_active_ptr = 
						&block_ptr->polygon_active_list[polygon_no];
					if (!*polygon_active_ptr)
						continue;
					polygon_ptr = &block_ptr->polygon_list[polygon_no];

					// If this polygon has four sides, add it as a quad.

					PREPARE_VERTEX_LIST(block_ptr);
					PREPARE_VERTEX_DEF_LIST(polygon_ptr);
					vertex0_ptr = VERTEX_PTR(0);
					if (polygon_ptr->vertices == 4) {
						a3d_geometry_ptr->Begin(A3D_QUADS);
						a3d_geometry_ptr->Tag(curr_audio_polygon_ID);
						vertex1_ptr = VERTEX_PTR(1);
						vertex2_ptr = VERTEX_PTR(2);
						vertex3_ptr = VERTEX_PTR(3);
						a3d_geometry_ptr->Vertex3f(vertex0_ptr->x * scale, 
							vertex0_ptr->y * scale, -vertex0_ptr->z * scale);
						a3d_geometry_ptr->Vertex3f(vertex1_ptr->x * scale, 
							vertex1_ptr->y * scale, -vertex1_ptr->z * scale);
						a3d_geometry_ptr->Vertex3f(vertex2_ptr->x * scale, 
							vertex2_ptr->y * scale, -vertex2_ptr->z * scale);
						a3d_geometry_ptr->Vertex3f(vertex3_ptr->x * scale, 
							vertex3_ptr->y * scale, -vertex3_ptr->z * scale);
					}

					// Otherwise convert the polygon into a triangle fan, adding
					// each triangle to the geometry object.  All triangles are
					// given the same tag since they are coplanar.

					else {
						a3d_geometry_ptr->Begin(A3D_TRIANGLES);
						a3d_geometry_ptr->Tag(curr_audio_polygon_ID);
						for (vertex_no = 2; vertex_no < polygon_ptr->vertices;
							vertex_no++) {
							vertex1_ptr = VERTEX_PTR(vertex_no - 1);
							vertex2_ptr = VERTEX_PTR(vertex_no);
							a3d_geometry_ptr->Vertex3f(vertex0_ptr->x * scale, 
								vertex0_ptr->y * scale, -vertex0_ptr->z * scale);
							a3d_geometry_ptr->Vertex3f(vertex1_ptr->x * scale, 
								vertex1_ptr->y * scale, -vertex1_ptr->z * scale);
							a3d_geometry_ptr->Vertex3f(vertex2_ptr->x * scale, 
								vertex2_ptr->y * scale, -vertex2_ptr->z * scale);
						}
					}
					a3d_geometry_ptr->End();

					// Increment the current audio polygon ID.

					curr_audio_polygon_ID++;
				}
			}
			
	// End the creation of the list, and return a void pointer to it.

	a3d_list_ptr->End();
	return((void *)a3d_list_ptr);
}

//------------------------------------------------------------------------------
// Destroy an audio block.
//------------------------------------------------------------------------------

void
destroy_audio_block(void *audio_block_ptr)
{
	((LPA3DLIST)audio_block_ptr)->Release();
}

#endif

//------------------------------------------------------------------------------
// Set the delay for a sound buffer.
//------------------------------------------------------------------------------

static void
set_sound_delay(sound *sound_ptr) 
{
	sound_ptr->start_time_ms = curr_time_ms;
	sound_ptr->delay_ms = sound_ptr->delay_range.min_delay_ms;
	if (sound_ptr->delay_range.delay_range_ms > 0)
		sound_ptr->delay_ms += (int)((float)rand() / RAND_MAX * 
			sound_ptr->delay_range.delay_range_ms);
}

//------------------------------------------------------------------------------
// Set the volume of a sound buffer.
//------------------------------------------------------------------------------

void
set_sound_volume(sound *sound_ptr, float volume)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr = (LPA3DSOURCE)sound_ptr->sound_buffer_ptr;
	if (a3d_source_ptr)
		a3d_source_ptr->SetGain(volume);
#else
	int volume_level;
	LPDIRECTSOUNDBUFFER dsound_buffer_ptr = 
		(LPDIRECTSOUNDBUFFER)sound_ptr->sound_buffer_ptr;
	if (dsound_buffer_ptr) {

		// Convert the volume level from a fractional value between 0.0 and
		// 1.0, to an integer value between -10,000 (-100 dB) and 0 (0 dB).

		if (FLT(volume, 0.005f))
			volume_level = -10000;
		else if (FLT(volume, 0.05f))
			volume_level = (int)(47835.76f * sqrt(0.4f * volume) - 
				12756.19f);
		else if (FLT(volume, 0.5f))
			volume_level = (int)(-500.0f / volume);
		else
			volume_level = (int)(2000.0f * (volume - 1.0f));	
		dsound_buffer_ptr->SetVolume(volume_level);
	}
#endif
}

//------------------------------------------------------------------------------
// Set the playback position for a sound.
//------------------------------------------------------------------------------

static void
set_sound_position(sound *sound_ptr, int position)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr = (LPA3DSOURCE)sound_ptr->sound_buffer_ptr;
	if (a3d_source_ptr)
		a3d_source_ptr->SetWavePosition(position);
#else
	LPDIRECTSOUNDBUFFER dsound_buffer_ptr = 
		(LPDIRECTSOUNDBUFFER)sound_ptr->sound_buffer_ptr;
	if (dsound_buffer_ptr)
		dsound_buffer_ptr->SetCurrentPosition(position);
#endif
}

//------------------------------------------------------------------------------
// Play a sound.
//------------------------------------------------------------------------------

void
play_sound(sound *sound_ptr, bool looped)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr = (LPA3DSOURCE)sound_ptr->sound_buffer_ptr;
	if (a3d_source_ptr)
		a3d_source_ptr->Play(looped ? A3D_LOOPED : A3D_SINGLE);
#else
	LPDIRECTSOUNDBUFFER dsound_buffer_ptr = 
		(LPDIRECTSOUNDBUFFER)sound_ptr->sound_buffer_ptr;
	if (dsound_buffer_ptr)
		dsound_buffer_ptr->Play(0, 0, looped ? DSBPLAY_LOOPING : 0);
#endif
}

//------------------------------------------------------------------------------
// Stop a sound from playing.
//------------------------------------------------------------------------------

void
stop_sound(sound *sound_ptr)
{
#ifdef SUPPORT_A3D
	LPA3DSOURCE a3d_source_ptr = (LPA3DSOURCE)sound_ptr->sound_buffer_ptr;
	if (a3d_source_ptr)
		a3d_source_ptr->Stop();
#else
	LPDIRECTSOUNDBUFFER dsound_buffer_ptr = 
		(LPDIRECTSOUNDBUFFER)sound_ptr->sound_buffer_ptr;
	if (dsound_buffer_ptr)
		dsound_buffer_ptr->Stop();
#endif
}

//------------------------------------------------------------------------------
// Begin a sound update.
//------------------------------------------------------------------------------

void
begin_sound_update(void)
{
#ifdef SUPPORT_A3D
	int audio_column, audio_row, audio_level;
	vector forward_vector, upward_vector;
	vector camera_offset_vector;
	vertex listener_position;
	float scale;
	bool reflections_enabled_flag;

	// If the reflections enabled flag has changed, enable or disable
	// reflections.

	start_atomic_operation();
	reflections_enabled_flag = reflections_enabled;
	end_atomic_operation();
	if (reflections_enabled_flag != reflections_on) {
		reflections_on = reflections_enabled_flag;
		if (reflections_on)
			a3d_geometry_ptr->Enable(A3D_1ST_REFLECTIONS);
		else
			a3d_geometry_ptr->Disable(A3D_1ST_REFLECTIONS);
	}

	// Determine which audio block the player is in.  If this differs from
	// the previous audio block, then we need to clear the audio frame and
	// insert the geometry found in the new range of audio blocks.

	player_viewpoint.position.get_scaled_audio_map_position(&audio_column,
		&audio_row, &audio_level);
	if (audio_column != prev_audio_column || audio_row != prev_audio_row ||
		audio_level != prev_audio_level) {
		vertex min_position, max_position;
		int min_audio_column, min_audio_row, min_audio_level;
		int max_audio_column, max_audio_row, max_audio_level;
		void **audio_square_ptr;
		LPA3DLIST a3d_list_ptr;

		// Remember the audio block the player is in.

		prev_audio_column = audio_column;
		prev_audio_row = audio_row;
		prev_audio_level = audio_level;

		// Determine which audio blocks are within the audio radius.

		min_position = player_viewpoint.position - audio_radius;
		max_position = player_viewpoint.position + audio_radius;
		min_position.get_scaled_audio_map_position(&min_audio_column, 
			&max_audio_row, &min_audio_level);
		max_position.get_scaled_audio_map_position(&max_audio_column,
			&min_audio_row, &max_audio_level);

		// Clamp this range to the boundaries of the audio map.

		if (min_audio_column < 0)
			min_audio_column = 0;
		if (min_audio_row < 0)
			min_audio_row = 0;
		if (min_audio_level < 0)
			min_audio_level = 0;
		if (max_audio_column >= world_ptr->audio_columns)
			max_audio_column = world_ptr->audio_columns - 1;
		if (max_audio_row >= world_ptr->audio_rows)
			max_audio_row = world_ptr->audio_rows - 1;
		if (max_audio_level >= world_ptr->audio_levels)
			max_audio_level = world_ptr->audio_levels - 1;

		// Clear all existing geometry data, sound source bindings and 
		// the listener binding.

		a3d_object_ptr->Clear();

		// Step through the audio squares in this range, adding their
		// geometry lists to the geometry object.

		for (audio_level = min_audio_level; audio_level <= max_audio_level;
			audio_level++)
			for (audio_row = min_audio_row; audio_row <= max_audio_row;
				audio_row++)
				for (audio_column = min_audio_column; audio_column <= 
					max_audio_column; audio_column++)
					if ((audio_square_ptr = world_ptr->get_audio_square_ptr(
						audio_column, audio_row, audio_level)) != NULL &&
						(a3d_list_ptr = (LPA3DLIST)*audio_square_ptr)
						!= NULL)
						a3d_list_ptr->Call();

		// Bind the listener to the geometry object.

		a3d_geometry_ptr->LoadIdentity();
		a3d_geometry_ptr->BindListener();

		// Set a flag to indicate all sound sources need to be bound to the
		// geometry object.

		bind_sound_sources = true;
	} else
		bind_sound_sources = false;

	// Compute the forward and upward orientation vectors for the listener.

	forward_vector.set(0.0f, 0.0f, 1.0f);
	forward_vector.rotatex(player_viewpoint.look_angle);
	forward_vector.rotatey(player_viewpoint.turn_angle);
	upward_vector.set(0.0f, 1.0f, 0.0f);
	upward_vector.rotatex(player_viewpoint.look_angle);
	upward_vector.rotatey(player_viewpoint.turn_angle);

	// Compute the camera offset vector for the current player orientation,
	// and use it to determine the listener position.

	camera_offset_vector = player_camera_offset;
	camera_offset_vector.rotatex(player_viewpoint.look_angle);
	camera_offset_vector.rotatey(player_viewpoint.turn_angle);
	listener_position = player_viewpoint.position + camera_offset_vector;

	// Set the orientation and position of the listener.

	a3d_listener_ptr->SetOrientation6f(forward_vector.dx, forward_vector.dy,
		-forward_vector.dz, upward_vector.dx, upward_vector.dy,
		-upward_vector.dz);
	scale = world_ptr->audio_scale;
	a3d_listener_ptr->SetPosition3f(listener_position.x * scale,
		listener_position.y * scale, -listener_position.z * scale);
#endif
}

//------------------------------------------------------------------------------
// Update a sound.
//------------------------------------------------------------------------------

void
update_sound(sound *sound_ptr)
{
	if (sound_ptr->sound_buffer_ptr) {
#ifdef SUPPORT_A3D
		LPA3DSOURCE a3d_source_ptr;
		float scale;
#else
		LPDIRECTSOUNDBUFFER dsound_buffer_ptr;
		float volume;
		int pan_position;
#endif
		vertex sound_position;
		vertex relative_sound_position;
		vertex transformed_sound_position;
		vector transformed_sound_vector;
		float distance;
		bool prev_in_range, in_range;

		// Get the position of the sound source; ambient sound sources are
		// located one unit above the player's position.

		if (sound_ptr->ambient) {
			sound_position = player_viewpoint.position;
			sound_position.y += 1.0f;
		} else
			sound_position = sound_ptr->position;

		// Determine the position of the sound source relative to the player 
		// position, then compute the distance between the player position and
		// the sound source.

		translate_vertex(&sound_position, &relative_sound_position);
		distance = (float)sqrt(relative_sound_position.x * 
			relative_sound_position.x + relative_sound_position.y * 
			relative_sound_position.y + relative_sound_position.z * 
			relative_sound_position.z);

		// Determine if the player is in or out of range of the sound,
		// remembering the previous state.  The sound radius cannot exceed the
		// audio radius; sounds that don't specify a radius (which includes
		// non-flood sounds using "looped" or "random" playback mode) use the
		// audio radius.

		prev_in_range = sound_ptr->in_range;
		if (sound_ptr->radius == 0.0f)
			in_range = FLE(distance, audio_radius);
		else
			in_range = FLE(distance, MIN(sound_ptr->radius, audio_radius));
		sound_ptr->in_range = in_range;

		// If this sound is using "looped" playback mode...

		switch (sound_ptr->playback_mode) {
		case LOOPED_PLAY:

			// If this is not a streaming source, play sound looped if player
			// has come into range.

			if (!sound_ptr->streaming && !prev_in_range && in_range)
				play_sound(sound_ptr, true);

			// Stop sound if player has gone out of range and this is a
			// non-streaming flood sound.

			if (!sound_ptr->streaming && sound_ptr->flood &&
				prev_in_range && !in_range)
				stop_sound(sound_ptr);
			break;

		// If this sound is using "random" playback mode...

		case RANDOM_PLAY:

			// Reset the delay time if the player has come into range.

			if (!prev_in_range && in_range)
				set_sound_delay(sound_ptr);

			// If the delay time has elapsed while the player is in range,
			// play the sound once through and calculate the next delay time.

			if (in_range && (curr_time_ms - sound_ptr->start_time_ms >= 
				sound_ptr->delay_ms)) {
				play_sound(sound_ptr, false);
				set_sound_delay(sound_ptr);
			}
			break;

		// If this sound is using "single" playback mode, play it once through
		// if the player has come into range.

		case SINGLE_PLAY:
			if (!prev_in_range && in_range)
				play_sound(sound_ptr, false);
			break;

		// If this sound is using "once" playback mode, play it once through if
		// the player has come into range and the sound has not being played
		// before.

		case ONE_PLAY:
			if (!prev_in_range && in_range && !sound_ptr->played_once) {
				play_sound(sound_ptr, false);
				sound_ptr->played_once = true;
			}
		}

#ifdef SUPPORT_A3D

		// If A3D is active and the sound source needs to be bound the geometry
		// object, do so.  Also set the gain of the source to maximum.

		a3d_source_ptr = (LPA3DSOURCE)sound_ptr->sound_buffer_ptr;
		if (bind_sound_sources) {
			a3d_geometry_ptr->LoadIdentity();
			a3d_geometry_ptr->BindSource(a3d_source_ptr);
		}
		a3d_source_ptr->SetGain(1.0f);

#endif

		// If the sound is currently in range, update it's presence in the
		// sound field...

		if (in_range) {

#ifdef SUPPORT_A3D

			// Set the position of the sound source.

			scale = world_ptr->audio_scale;
			a3d_source_ptr->SetPosition3f(sound_position.x * scale, 
				sound_position.y * scale, -sound_position.z * scale);

			// If the distance is greater than or equal to the reflection
			// radius, then turn off reflections for this source if they
			// are currently on.  Otherwise turn on reflections if they're
			// currently off.

			if (FGE(distance, reflection_radius)) { 
				if (sound_ptr->reflections) {
					if (sound_ptr->ambient)
						set_sound_mode(a3d_source_ptr, false, false);
					else
						set_sound_mode(a3d_source_ptr, true, false);
					sound_ptr->reflections = false;
				}
			} else {
				if (sound_ptr->ambient)
					set_sound_mode(a3d_source_ptr, false, true);
				else
					set_sound_mode(a3d_source_ptr, true, true);
				sound_ptr->reflections = true;
			}

#else

			// Get a pointer to the DirectSound buffer.

			dsound_buffer_ptr = 
				(LPDIRECTSOUNDBUFFER)sound_ptr->sound_buffer_ptr;

			// If the flood flag is set for this sound, play it at full
			// volume regardless of the distance from the source.

			if (sound_ptr->flood)
				set_sound_volume(sound_ptr, sound_ptr->volume);

			// Otherwise compute the volume of the sound as a function of
			// the scaled distance from the source.

			else {
				volume = 1.0f / (distance * sound_ptr->rolloff * 
					world_ptr->audio_scale + 1.0f);
				set_sound_volume(sound_ptr, volume);
			}

			// Compute the fully transformed position of the sound, and
			// store it as a vector.

			rotate_vertex(&relative_sound_position,
				&transformed_sound_position);

			// Treat the transformed position as a vector, and normalise
			// it.

			transformed_sound_vector.dx = transformed_sound_position.x;
			transformed_sound_vector.dy = transformed_sound_position.y;
			transformed_sound_vector.dz = transformed_sound_position.z;
			transformed_sound_vector.normalise();
			
			// Compute the pan position of the sound based upon the x
			// component of the normalised sound vector.

			pan_position = (int)(transformed_sound_vector.dx * 10000.0f);
			dsound_buffer_ptr->SetPan(pan_position);
#endif

		}
	}
}

//------------------------------------------------------------------------------
// End a sound update.
//------------------------------------------------------------------------------

void
end_sound_update(void)
{
#ifdef SUPPORT_A3D
#ifdef RENDERSTATS
	int start_time_ms = get_time_ms();
#endif
	a3d_object_ptr->Flush();
#ifdef RENDERSTATS
	int end_time_ms = get_time_ms();
	diagnose("Time required to flush sounds = %d ms", 
		end_time_ms - start_time_ms);
#endif
#endif
}

//------------------------------------------------------------------------------
// Initialise the unscaled video texture. 
//------------------------------------------------------------------------------

static void
init_unscaled_video_texture(void)
{
	pixmap *pixmap_ptr;

	// If the unscaled video texture already has a pixmap list, assume it has
	// been initialised already.

	if (unscaled_video_texture_ptr->pixmap_list)
		return;

	// Create the video pixmap object and initialise it.

	NEWARRAY(pixmap_ptr, pixmap, 1);
	if (pixmap_ptr == NULL) {
		diagnose("Unable to create video pixmap");
		return;
	}
	pixmap_ptr->image_is_16_bit = true;
	pixmap_ptr->width = unscaled_video_width;
	pixmap_ptr->height = unscaled_video_height;
	pixmap_ptr->image_size = unscaled_video_width * unscaled_video_height * 2;
	NEWARRAY(pixmap_ptr->image_ptr, imagebyte, pixmap_ptr->image_size);
	if (pixmap_ptr->image_ptr == NULL) {
		diagnose("Unable to create video pixmap image");
		DELARRAY(pixmap_ptr, pixmap, 1);
		return;
	}
	memset(pixmap_ptr->image_ptr, 0, pixmap_ptr->image_size);
	pixmap_ptr->colours = 0;
	pixmap_ptr->transparent_index = -1;
	pixmap_ptr->delay_ms = 0;

	// Initialise the texture object.

	unscaled_video_texture_ptr->transparent = false;
	unscaled_video_texture_ptr->loops = false;
	unscaled_video_texture_ptr->is_16_bit = true;
	unscaled_video_texture_ptr->width = unscaled_video_width;
	unscaled_video_texture_ptr->height = unscaled_video_height;
	unscaled_video_texture_ptr->pixmaps = 1;
	unscaled_video_texture_ptr->pixmap_list = pixmap_ptr;
	set_size_indices(unscaled_video_texture_ptr);
}

//------------------------------------------------------------------------------
// Initialise a scaled video texture. 
//------------------------------------------------------------------------------

static void
init_scaled_video_texture(video_texture *scaled_video_texture_ptr)
{
	video_rect *rect_ptr;
	float unscaled_width, unscaled_height;
	float scaled_width, scaled_height;
	float aspect_ratio;
	pixmap *pixmap_ptr;
	texture *texture_ptr;

	// If this scaled video texture has a pixmap list, assume it has already
	// been initialised.

	texture_ptr = scaled_video_texture_ptr->texture_ptr;
	if (texture_ptr->pixmap_list)
		return;

	// Get a pointer to the source rectangle.  If ratios are being used for
	// any of the coordinates, convert them to pixel units.

	rect_ptr = &scaled_video_texture_ptr->source_rect;
	if (rect_ptr->x1_is_ratio)
		rect_ptr->x1 *= unscaled_video_width;
	if (rect_ptr->y1_is_ratio)
		rect_ptr->y1 *= unscaled_video_height;
	if (rect_ptr->x2_is_ratio)
		rect_ptr->x2 *= unscaled_video_width;
	if (rect_ptr->y2_is_ratio)
		rect_ptr->y2 *= unscaled_video_height;

	// Swap x1 and x2 if x2 < x1.

	if (rect_ptr->x2 < rect_ptr->x1) {
		float temp_x = rect_ptr->x1;
		rect_ptr->x1 = rect_ptr->x2;
		rect_ptr->x2 = temp_x;
	}

	// Swap y1 and y2 if y2 < y1.

	if (rect_ptr->y2 < rect_ptr->y1) {
		float temp_y = rect_ptr->y1;
		rect_ptr->y1 = rect_ptr->y2;
		rect_ptr->y2 = temp_y;
	}

	// Clamp the source rectangle to the boundaries of the unscaled video
	// frame.

	rect_ptr->x1 = MAX(rect_ptr->x1, 0);
	rect_ptr->y1 = MAX(rect_ptr->y1, 0);
	rect_ptr->x2 = MIN(rect_ptr->x2, unscaled_video_width);
	rect_ptr->y2 = MIN(rect_ptr->y2, unscaled_video_height);

	// If the clamped rectangle doesn't intersect the unscaled video frame
	// or has zero width or height, then set the source rectangle to be the
	// whole frame.

	if (rect_ptr->x1 >= unscaled_video_width || rect_ptr->x2 <= 0 || 
		rect_ptr->y1 >= unscaled_video_height || rect_ptr->y2 <= 0 ||
		rect_ptr->x1 == rect_ptr->x2 || rect_ptr->y1 == rect_ptr->y2) {
		rect_ptr->x1 = 0.0f;
		rect_ptr->y1 = 0.0f;
		rect_ptr->x2 = (float)unscaled_video_width;
		rect_ptr->y2 = (float)unscaled_video_height;
	}

	// Get the unscaled width and height of the texture from the source
	// rectangle, then calculate the scaled width and height such that the
	// dimensions are no greater than 256 pixels.

	unscaled_width = rect_ptr->x2 - rect_ptr->x1;
	unscaled_height = rect_ptr->y2 - rect_ptr->y1;
	if (unscaled_width <= 256.0f && unscaled_height <= 256.0f) {
		scaled_width = unscaled_width;
		scaled_height = unscaled_height;
	} else {
		aspect_ratio = unscaled_width / unscaled_height;
		if (aspect_ratio > 1.0f) {
			scaled_width = 256.0f;
			scaled_height = unscaled_height / aspect_ratio; 
		} else {
			scaled_width = unscaled_width * aspect_ratio;
			scaled_height = 256.0f;
		}
	}

	// Calculate delta (u,v) for the video texture.

	scaled_video_texture_ptr->delta_u = unscaled_width / scaled_width;
	scaled_video_texture_ptr->delta_v = unscaled_height / scaled_height;

	// Create the video pixmap object and initialise it.

	NEWARRAY(pixmap_ptr, pixmap, 1);
	if (pixmap_ptr == NULL) {
		diagnose("Unable to create video pixmap");
		return;
	}
	pixmap_ptr->image_is_16_bit = true;
	pixmap_ptr->width = (int)scaled_width;
	pixmap_ptr->height = (int)scaled_height;
	pixmap_ptr->image_size = pixmap_ptr->width * pixmap_ptr->height * 2;
	NEWARRAY(pixmap_ptr->image_ptr, imagebyte, pixmap_ptr->image_size);
	if (pixmap_ptr->image_ptr == NULL) {
		diagnose("Unable to create video pixmap image");
		DELARRAY(pixmap_ptr, pixmap, 1);
		return;
	}
	memset(pixmap_ptr->image_ptr, 0, pixmap_ptr->image_size);
	pixmap_ptr->colours = 0;
	pixmap_ptr->transparent_index = -1;
	pixmap_ptr->delay_ms = 0;

	// Initialise the texture object.

	texture_ptr->transparent = false;
	texture_ptr->loops = false;
	texture_ptr->is_16_bit = true;
	texture_ptr->width = pixmap_ptr->width;
	texture_ptr->height = pixmap_ptr->height;
	texture_ptr->pixmaps = 1;
	texture_ptr->pixmap_list = pixmap_ptr;
	set_size_indices(texture_ptr);
}

//------------------------------------------------------------------------------
// Initialise all video textures for the given unscaled video dimensions.
//------------------------------------------------------------------------------

void
init_video_textures(int video_width, int video_height, int pixel_format)
{
	// Set the unscaled video width and height, and the pixel format.

	unscaled_video_width = video_width;
	unscaled_video_height = video_height;
	video_pixel_format = pixel_format;

	// Initialise the unscaled video texture, if it exists.

	if (unscaled_video_texture_ptr)
		init_unscaled_video_texture();

	// If the unscaled video texture doesn't exist, and we're using RealPlayer,
	// allocate a seperate 16-bit image buffer for receiving the downconverted
	// video frame.

	else if (media_player == REAL_PLAYER)
		NEWARRAY(video_buffer_ptr, byte,
			unscaled_video_width * unscaled_video_height * 2);

	// Step through the list of scaled video textures, and initialise each one.

	video_texture *video_texture_ptr = scaled_video_texture_list;
	while (video_texture_ptr) {
		init_scaled_video_texture(video_texture_ptr);
		video_texture_ptr = video_texture_ptr->next_video_texture_ptr;
	}

	// Send an event to the player thread indicating that the stream has
	// started.

	stream_opened.send_event(true);
}

//------------------------------------------------------------------------------
// Initialise the video stream (Windows Media Player only).
//------------------------------------------------------------------------------

static bool
init_video_stream(void)
{
	DDSURFACEDESC ddraw_surface_desc;
	RECT rect;
	int video_width, video_height;

	// Get the primary video stream, if there is one.

	if (global_stream_ptr->GetMediaStream(MSPID_PrimaryVideo,
		&primary_video_stream_ptr) != S_OK)
		return(false);

	// Obtain the DirectDraw stream object from the primary video stream.

	if (primary_video_stream_ptr->QueryInterface(IID_IDirectDrawMediaStream,
		(void **)&ddraw_stream_ptr) != S_OK)
		return(false);

	// Determine the unscaled size of the video frame.

	ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
	if (ddraw_stream_ptr->GetFormat(&ddraw_surface_desc, NULL, NULL, NULL)
		!= S_OK)
		return(false);
	video_width = ddraw_surface_desc.dwWidth;
	video_height = ddraw_surface_desc.dwHeight;

	// Create a DirectDraw video surface using the texture pixel format, but 
	// without an alpha channel (otherwise CreateSample will spit the dummy).

	memset(&ddraw_surface_desc, 0, sizeof(DDSURFACEDESC));
	ddraw_surface_desc.dwSize = sizeof(DDSURFACEDESC);
	ddraw_surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT |
		DDSD_PIXELFORMAT;
	ddraw_surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
	ddraw_surface_desc.dwWidth = video_width;
	ddraw_surface_desc.dwHeight = video_height;
	ddraw_surface_desc.ddpfPixelFormat = ddraw_video_pixel_format;
	if (ddraw_object_ptr->CreateSurface(&ddraw_surface_desc, &video_surface_ptr,
		NULL) != DD_OK)
		return(false);

	// Set the rectangle that is to be rendered to on the video surface.

	rect.left = 0;
 	rect.right = video_width;
 	rect.top = 0;
 	rect.bottom = video_height;

	// Create the video sample for the video surface.

	if (ddraw_stream_ptr->CreateSample(video_surface_ptr, &rect, 0, 
		&video_sample_ptr) != S_OK)
		return(false);

	// Create the event that will be used to signal that a video frame is
	// available.

	video_frame_available.create_event();

	// Initialise the video textures now, since we already know the
	// dimensions of the video frame.

	init_video_textures(video_width, video_height, RGB16);
	return(true);
}

//------------------------------------------------------------------------------
// Create a stream for the given streaming media file.
//------------------------------------------------------------------------------

static int
create_stream(const char *file_path, int player)
{
	// If RealPlayer was requested...

	media_player = player;
	if (media_player == REAL_PLAYER) {
		char szDllName[_MAX_PATH];
		DWORD bufSize;
		HKEY hKey;
		
		// Initialize the global variables.

		exContext = NULL;
		hDll = NULL;
		m_fpCreateEngine = NULL;
		m_fpCloseEngine	= NULL;
		pEngine = NULL;
		pPlayer = NULL;
#ifdef SUPPORT_A3D
		pAudDev = NULL;
#endif
		pErrorSinkControl = NULL;
		video_buffer_ptr = NULL;

	   // Get location of rmacore DLL from windows registry

		szDllName[0] = '\0'; 
		bufSize = sizeof(szDllName) - 1;
		if(RegOpenKey(HKEY_CLASSES_ROOT, 
			"Software\\RealNetworks\\Preferences\\DT_Common", &hKey) 
			== ERROR_SUCCESS) { 
			RegQueryValue(hKey, "", szDllName, (long *)&bufSize); 
			RegCloseKey(hKey); 
		}
		strcat(szDllName, "pnen3260.dll");
    
		// Load the rmacore library.

		if ((hDll = LoadLibrary(szDllName)) == NULL)
			return(PLAYER_UNAVAILABLE);

		// Retrieve the function addresses from the module that we need to call.

		m_fpCreateEngine = (FPRMCREATEENGINE)GetProcAddress(hDll, 
			"CreateEngine");
		m_fpCloseEngine = (FPRMCLOSEENGINE)GetProcAddress(hDll, "CloseEngine");
		if (m_fpCreateEngine == NULL || m_fpCloseEngine == NULL)
			return(PLAYER_UNAVAILABLE);

		// Create the client context.

		if ((exContext = new ExampleClientContext()) == NULL)
			return(PLAYER_UNAVAILABLE);
		exContext->AddRef();

		// Create client engine.
		
		if (m_fpCreateEngine((IRMAClientEngine **)&pEngine) != PNR_OK)
			return(PLAYER_UNAVAILABLE);

		// Create player.

		if (pEngine->CreatePlayer(pPlayer) != PNR_OK)
			return(PLAYER_UNAVAILABLE);

#ifdef SUPPORT_A3D

		// Replace the audio device before we initialize the player.

		if ((pAudDev = new ExampleAudioDevice(pPlayer)) != NULL) {
			pAudDev->AddRef();
			IRMAAudioDeviceManager *pAudDevMgr = NULL;
			if (pEngine->QueryInterface(IID_IRMAAudioDeviceManager,
				(void **)&pAudDevMgr) == PNR_OK) {
				if (pAudDevMgr->Replace(pAudDev) != PNR_OK) {
					pAudDev->Release();
					pAudDev = NULL;
				}
				pAudDevMgr->Release();
			}
		}

#endif

		// Initialize the context and error sink control.

		exContext->Init(pPlayer);
		pPlayer->SetClientContext(exContext);
		pPlayer->QueryInterface(IID_IRMAErrorSinkControl,
			(void **)&pErrorSinkControl);
		if (pErrorSinkControl) {	
			exContext->QueryInterface(IID_IRMAErrorSink, (void**)&pErrorSink);
			if (pErrorSink)
				pErrorSinkControl->AddErrorSink(pErrorSink, PNLOG_EMERG, 
					PNLOG_INFO);
		}
		
		// Open the streaming media URL.

		if (pPlayer->OpenURL(file_path) != PNR_OK) {
			diagnose("RealPlayer was unable to open stream URL '%s'", 
				file_path);
			return(STREAM_UNAVAILABLE);
		}
	}

	// If Windows Media Player was requested...

	else {
		IAMMultiMediaStream *local_stream_ptr;
		WCHAR wPath[MAX_PATH];

		// Initialise the COM library.

		CoInitialize(NULL);

		// Initialise the global variables.

		global_stream_ptr = NULL;
		primary_video_stream_ptr = NULL;
		ddraw_stream_ptr = NULL;
		video_sample_ptr = NULL;
		video_surface_ptr = NULL;

		// Create the local multi-media stream object.

		if (CoCreateInstance(CLSID_AMMultiMediaStream, NULL, 
			CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, 
			(void **)&local_stream_ptr) != S_OK)
			return(PLAYER_UNAVAILABLE);

		// Initialise the local stream object.

		if (local_stream_ptr->Initialize(STREAMTYPE_READ, AMMSF_NOGRAPHTHREAD,
			NULL) != S_OK) {
			local_stream_ptr->Release();
			return(PLAYER_UNAVAILABLE);
		}

		// Add a primary video stream to the local stream object.

		if (local_stream_ptr->AddMediaStream(ddraw_object_ptr, 
			&MSPID_PrimaryVideo, 0, NULL) != S_OK) {
			local_stream_ptr->Release();
			return(PLAYER_UNAVAILABLE);
		}

		// Add a primary audio stream to the local stream object, using the 
		// default audio renderer for playback.

		if (local_stream_ptr->AddMediaStream(NULL, &MSPID_PrimaryAudio, 
			AMMSF_ADDDEFAULTRENDERER, NULL) != S_OK) {
			local_stream_ptr->Release();
			return(PLAYER_UNAVAILABLE);
		}

		// Open the streaming media file.

		MultiByteToWideChar(CP_ACP, 0, file_path, -1, wPath, MAX_PATH);    
		if (local_stream_ptr->OpenFile(wPath, 0) != S_OK) {
			local_stream_ptr->Release();
			diagnose("Windows Media Player was unable to open stream URL '%s'", 
				file_path);
			return(STREAM_UNAVAILABLE);
		}

		// Convert the local stream object into a global stream object.

		global_stream_ptr = local_stream_ptr;

		// Initialise the primary video stream, if it exists.

		streaming_video_available = init_video_stream();

		// Get the end of stream event handle.

		global_stream_ptr->GetEndOfStreamEventHandle(&end_of_stream_handle);
	}

	// Return a success status.

	return(STREAM_STARTED);
}

//------------------------------------------------------------------------------
// Convert a 24-bit YUV pixel to a 555 RGB pixel.
//------------------------------------------------------------------------------

float yuvmatrix[9] = {
	1.164f, 1.164f, 1.164f,
	0.000f,-0.391f, 2.018f,
	1.596f,-0.813f, 0.000f
};

word
optimised_YUV_to_pixel(float *yuv)
{
	float rgb[3];
	word pix;

	// Convert YUV to RGB.

	_trans_v1x3(rgb, yuvmatrix, yuv);

	// Set red component in 16-bit pixel.

	if (rgb[0] <= 0.0f)
		pix = 0x0000;
	else if (rgb[0] > 255.0f)
		pix = 0x7c00;
	else
		pix = ((word)rgb[0] & 0x00f8) << 7;
		
	// Add green component to 16-bit pixel.

	if (rgb[1] > 255.0f)
		pix |= 0x003e0;
	else if (rgb[1] > 0.0f)
		pix |= ((word)rgb[1] & 0x00f8) << 2;

	// Add blue component to 16-bit pixel.

	if (rgb[2] > 255.0f)
		pix |= 0x001f;
	else if (rgb[2] > 0.0f)
		pix |= (word)rgb[2] >> 3;
	
	// Return this pixel value.

	return(pix);
}

word
YUV_to_pixel(float *yuv)
{
	float rgb[3];
	word pix;

	// Convert YUV to RGB.

	rgb[0] = 1.164f * yuv[0] + 1.596f * yuv[2];
	rgb[1] = 1.164f * yuv[0] - 0.391f * yuv[1] - 0.813f * yuv[2];
	rgb[2] = 1.164f * yuv[0] + 2.018f * yuv[1];

	// Set red component in 16-bit pixel.

	if (rgb[0] <= 0.0f)
		pix = 0x0000;
	else if (rgb[0] > 255.0f)
		pix = 0x7c00;
	else
		pix = ((word)rgb[0] & 0x00f8) << 7;
		
	// Add green component to 16-bit pixel.

	if (rgb[1] > 255.0f)
		pix |= 0x003e0;
	else if (rgb[1] > 0.0f)
		pix |= ((word)rgb[1] & 0x00f8) << 2;

	// Add blue component to 16-bit pixel.

	if (rgb[2] > 255.0f)
		pix |= 0x001f;
	else if (rgb[2] > 0.0f)
		pix |= (word)rgb[2] >> 3;
	
	// Return this pixel value.

	return(pix);
}

//------------------------------------------------------------------------------
// Draw the current video frame (RealPlayer only).
//------------------------------------------------------------------------------

void
draw_frame(byte *fb_ptr)
{
	byte *vb_ptr;
	byte *y_buffer_ptr, *u_buffer_ptr, *v_buffer_ptr;
	byte *u_row_ptr, *v_row_ptr;
	float yuv[3];
	int vb_row_pitch;
	byte *fb_row_ptr, *pixmap_row_ptr;
	RGBcolour colour;
	int row, column;
	float u, v, start_u, start_v, delta_u, delta_v;
	video_texture *scaled_video_texture_ptr;
	texture *texture_ptr;
	pixmap *pixmap_ptr;
	int index;

	// If there is an unscaled texture, use it's pixmap image as the target for
	// the video frame downconversion.  Otherwise use the seperately allocated 
	// video buffer as the target.

	if (unscaled_video_texture_ptr) {
		pixmap_ptr = unscaled_video_texture_ptr->pixmap_list;
		vb_ptr = pixmap_ptr->image_ptr;
	} else
		vb_ptr = video_buffer_ptr;

	// Convert the video frame into texture pixel format.

	switch (video_pixel_format) {

	// If the video format is 24-bit RGB...

	case RGB24:
		pixmap_row_ptr = vb_ptr;
		for (row = 0; row < unscaled_video_height; row++) {
			fb_row_ptr = fb_ptr + (unscaled_video_height - row) *
				unscaled_video_width * 3;
			for (column = 0; column < unscaled_video_width; column++) {
				colour.blue = *fb_row_ptr++;
				colour.green = *fb_row_ptr++;
				colour.red = *fb_row_ptr++;
				*(word *)pixmap_row_ptr = (word)RGB_to_texture_pixel(colour);
				pixmap_row_ptr += 2;
			}
		}
		break;

	// If the video format is "12-bit" YUV...

	case YUV12:

		// Set pointers to the start of the Y, U and V data.

		y_buffer_ptr = fb_ptr;
		u_buffer_ptr = fb_ptr + unscaled_video_width * unscaled_video_height;
		v_buffer_ptr = u_buffer_ptr + 
			(unscaled_video_width * unscaled_video_height / 4);

		// Convert each 888 YUV pixel to an 555 RGB pixel.

		pixmap_row_ptr = vb_ptr;
		if (AMD_3DNow_supported) {
			for (row = 0; row < unscaled_video_height; row++) {
				u_row_ptr = u_buffer_ptr;
				v_row_ptr = v_buffer_ptr;
				for (column = 0; column < unscaled_video_width; column += 2) {
					yuv[0] = (float)*y_buffer_ptr++ - 16.0f;
					yuv[1] = (float)*u_row_ptr++ - 128.0f;
					yuv[2] = (float)*v_row_ptr++ - 128.0f;
					*(word *)pixmap_row_ptr = optimised_YUV_to_pixel(yuv);
					pixmap_row_ptr += 2;
					yuv[0] = (float)*y_buffer_ptr++ - 16.0f;
					*(word *)pixmap_row_ptr = optimised_YUV_to_pixel(yuv);
					pixmap_row_ptr += 2;
				}
				if (row & 1) {
					u_buffer_ptr += unscaled_video_width >> 1;
					v_buffer_ptr += unscaled_video_width >> 1;
				}
			}
		} else {
			for (row = 0; row < unscaled_video_height; row++) {
				u_row_ptr = u_buffer_ptr;
				v_row_ptr = v_buffer_ptr;
				for (column = 0; column < unscaled_video_width; column += 2) {
					yuv[0] = (float)*y_buffer_ptr++ - 16.0f;
					yuv[1] = (float)*u_row_ptr++ - 128.0f;
					yuv[2] = (float)*v_row_ptr++ - 128.0f;
					*(word *)pixmap_row_ptr = YUV_to_pixel(yuv);
					pixmap_row_ptr += 2;
					yuv[0] = (float)*y_buffer_ptr++ - 16.0f;
					*(word *)pixmap_row_ptr = YUV_to_pixel(yuv);
					pixmap_row_ptr += 2;
				}
				if (row & 1) {
					u_buffer_ptr += unscaled_video_width >> 1;
					v_buffer_ptr += unscaled_video_width >> 1;
				}
			}
		}
	}

	// If there is an unscaled video texture, indicate it's pixmap has been 
	// updated.

	if (unscaled_video_texture_ptr) {
		start_atomic_operation();
		for (index = 0; index < BRIGHTNESS_LEVELS; index++)
			pixmap_ptr->image_updated[index] = true;
		end_atomic_operation();
	}

	// If there are scaled video textures, copy the video surface to each
	// one.

	scaled_video_texture_ptr = scaled_video_texture_list;
	while (scaled_video_texture_ptr) {

		// Scale the video texture and convert from video to texture pixels.

		texture_ptr = scaled_video_texture_ptr->texture_ptr;
		pixmap_ptr = texture_ptr->pixmap_list;
		start_u = scaled_video_texture_ptr->source_rect.x1;
		start_v = scaled_video_texture_ptr->source_rect.y1;
		delta_u = scaled_video_texture_ptr->delta_u;
		delta_v = scaled_video_texture_ptr->delta_v;
		v = start_v;
		pixmap_row_ptr = pixmap_ptr->image_ptr;
		vb_row_pitch = unscaled_video_width * 2;
		for (row = 0; row < pixmap_ptr->height; row++) {
			fb_row_ptr = vb_ptr + (int)v * vb_row_pitch;
			u = start_u;
			for (column = 0; column < pixmap_ptr->width; column++) {
				*(word *)pixmap_row_ptr = *((word *)fb_row_ptr + (int)u);
				pixmap_row_ptr += 2;
				u += delta_u;
			}
			v += delta_v;
		}

		// Indicate the pixmap has been updated.

		start_atomic_operation();
		for (index = 0; index < BRIGHTNESS_LEVELS; index++)
			pixmap_ptr->image_updated[index] = true;
		end_atomic_operation();

		// Move onto the next video texture.

		scaled_video_texture_ptr = 
			scaled_video_texture_ptr->next_video_texture_ptr;
	}
}

//------------------------------------------------------------------------------
// Play the stream.
//------------------------------------------------------------------------------

static void
play_stream(void)
{
	if (media_player == REAL_PLAYER)
		pPlayer->Begin();
	else {
		global_stream_ptr->SetState(STREAMSTATE_RUN);
		if (streaming_video_available)
			video_sample_ptr->Update(0, video_frame_available.event_handle, 
				NULL, NULL);
	}
}

//------------------------------------------------------------------------------
// Stop the stream.
//------------------------------------------------------------------------------

static void
stop_stream(void)
{
	if (media_player == REAL_PLAYER)
		pPlayer->Stop();
	else {
		global_stream_ptr->SetState(STREAMSTATE_STOP);
		global_stream_ptr->Seek(0);
	}
}

//------------------------------------------------------------------------------
// Pause the stream.
//------------------------------------------------------------------------------

static void
pause_stream(void)
{
	if (media_player == REAL_PLAYER)
		pPlayer->Pause();
	else
		global_stream_ptr->SetState(STREAMSTATE_STOP);
}

//------------------------------------------------------------------------------
// Update stream (Windows Media Player only).
//------------------------------------------------------------------------------

static void
update_stream(void)
{
	// If there is a video stream, and the next video frame is available...

	if (streaming_video_available && video_frame_available.event_sent()) {
		DDSURFACEDESC ddraw_surface_desc;
		byte *fb_ptr, *fb_row_ptr, *pixmap_row_ptr;
		int row_pitch;
		int row, column;
		float u, v, start_u, start_v, delta_u, delta_v;
		video_texture *scaled_video_texture_ptr;
		texture *texture_ptr;
		pixmap *pixmap_ptr;
		int index;

		// Lock the video surface.

		ddraw_surface_desc.dwSize = sizeof(ddraw_surface_desc);
		if (video_surface_ptr->Lock(NULL, &ddraw_surface_desc,
			DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL) != DD_OK)
			return;
		fb_ptr = (byte *)ddraw_surface_desc.lpSurface;
		row_pitch = ddraw_surface_desc.lPitch;

		// If there is an unscaled texture, copy the video surface to it.

		if (unscaled_video_texture_ptr) {
			pixmap_ptr = &unscaled_video_texture_ptr->pixmap_list[0];
			memcpy(pixmap_ptr->image_ptr, fb_ptr, pixmap_ptr->image_size);

			// Indicate the pixmap has been updated.

			start_atomic_operation();
			for (index = 0; index < BRIGHTNESS_LEVELS; index++)
				pixmap_ptr->image_updated[index] = true;
			end_atomic_operation();
		}

		// If there are scaled video textures, copy the video surface to each
		// one.

		scaled_video_texture_ptr = scaled_video_texture_list;
		while (scaled_video_texture_ptr) {
			texture_ptr = scaled_video_texture_ptr->texture_ptr;
			pixmap_ptr = texture_ptr->pixmap_list;
			start_u = scaled_video_texture_ptr->source_rect.x1;
			start_v = scaled_video_texture_ptr->source_rect.y1;
			delta_u = scaled_video_texture_ptr->delta_u;
			delta_v = scaled_video_texture_ptr->delta_v;
			v = start_v;
			for (row = 0; row < pixmap_ptr->height; row++) {
				fb_row_ptr = fb_ptr + (int)v * row_pitch;
				pixmap_row_ptr = pixmap_ptr->image_ptr + 
					row * pixmap_ptr->width * 2;
				u = start_u;
				for (column = 0; column < pixmap_ptr->width; column++) {
					*(word *)pixmap_row_ptr = *((word *)fb_row_ptr + (int)u);
					pixmap_row_ptr += 2;
					u += delta_u;
				}
				v += delta_v;
			}

			// Indicate the pixmap has been updated.

			start_atomic_operation();
			for (index = 0; index < BRIGHTNESS_LEVELS; index++)
				pixmap_ptr->image_updated[index] = true;
			end_atomic_operation();

			// Move onto the next video texture.

			scaled_video_texture_ptr = 
				scaled_video_texture_ptr->next_video_texture_ptr;
		}

		// Unlock the video surface.

		video_surface_ptr->Unlock(fb_ptr);

		// Request the next video frame.
		
		video_sample_ptr->Update(0, video_frame_available.event_handle,
			NULL, NULL);
	}

	// If the end of the stream has been reached, restart the stream.

	if (/* stream_command == STREAM_PLAY_LOOPED && */ end_of_stream_handle && 
		WaitForSingleObject(end_of_stream_handle, 0) != WAIT_TIMEOUT) {
		stop_stream();
		play_stream();
	}
}

//------------------------------------------------------------------------------
// Destroy the stream.
//------------------------------------------------------------------------------

static void
destroy_stream(void)
{
	// If RealPlayer was chosen...

	if (media_player == REAL_PLAYER) {

		// Release the error sink control.

		if (pErrorSinkControl) {
			pErrorSinkControl->RemoveErrorSink(pErrorSink);
			pErrorSinkControl->Release();
		}

		// Release the context.

		if (exContext)
			exContext->Release();

		// Close and release the player.

		if (pPlayer) {
			pEngine->ClosePlayer(pPlayer);
			pPlayer->Release();
		}

#ifdef SUPPORT_A3D

		// Remove and release the audio device.

		if (pAudDev) {
			IRMAAudioDeviceManager *pAudDevMgr = NULL;
			if (pEngine->QueryInterface(IID_IRMAAudioDeviceManager,
				(void **)&pAudDevMgr) == PNR_OK) {
				pAudDevMgr->Remove(pAudDev);
				pAudDevMgr->Release();
			}
			pAudDev->Release();
		}

#endif

		// Close the engine.

		if (pEngine)
			m_fpCloseEngine(pEngine);

		// Free the RealPlayer library.

		if (hDll)
			FreeLibrary(hDll);

		// Delete the video buffer.

		if (video_buffer_ptr)
			DELARRAY(video_buffer_ptr, byte, 
				unscaled_video_width * unscaled_video_height * 2);
	} 
	
	// If Windows Media Player was chosen...

	else {

		// Destroy the "video frame available" event.

		if (video_frame_available.event_handle)
			video_frame_available.destroy_event();

		// Release the video sample object.

		if (video_sample_ptr)
			video_sample_ptr->Release();

		// Release the video surface object.

		if (video_surface_ptr)
			video_surface_ptr->Release();

		// Release the DirectDraw stream object.

		if (ddraw_stream_ptr)
			ddraw_stream_ptr->Release();

		// Release the primary video stream object.

		if (primary_video_stream_ptr)
			primary_video_stream_ptr->Release();

		// Release the global stream object.

		if (global_stream_ptr)
			global_stream_ptr->Release();

		// Shut down the COM library.

		CoUninitialize();
	}
}

//------------------------------------------------------------------------------
// Ask user whether they want to download Windows Media Player.
//------------------------------------------------------------------------------

static void
download_wmp(void)
{
	if (query("Windows Media Player required", true,
		"Flatland Rover requires the latest version of Windows Media Player\n"
		"to play back the streaming media in this spot.\n\n"
		"Do you wish to download Windows Media Player now?"))
		wmp_download_requested.send_event(true);
}

//------------------------------------------------------------------------------
// Display a message telling the user that Windows Media Player does not
// support the video format.
//------------------------------------------------------------------------------

static void
unsupported_wmp_format(void)
{
	information("Unable to play streaming media",
		"Windows Media Player is unable to play the streaming media in this\n"
		"spot; either the streaming media URL is invalid, or the streaming\n"
		"media format is not supported by Windows Media Player.");
}

//------------------------------------------------------------------------------
// Ask user whether they want to download RealPlayer.
//------------------------------------------------------------------------------

static void
download_rp(void)
{				
	if (query("RealPlayer required", true,
		"Flatland Rover requires the latest version of RealPlayer\n"
		"to play back the streaming media in this spot.\n\n"
		"Do you wish to download RealPlayer now?"))
		rp_download_requested.send_event(true);
}

//------------------------------------------------------------------------------
// Display a message telling the user that RealPlayer does not support the
// streaming media format.
//------------------------------------------------------------------------------

static void
unsupported_rp_format(void)
{
	information("Unable to play streaming media",
		"RealPlayer is unable to play the streaming media in this spot;\n"
		"either the streaming media URL is invalid, or the streaming media\n"
		"format is not currently supported by RealPlayer.");
}

//------------------------------------------------------------------------------
// Streaming thread.
//------------------------------------------------------------------------------

static void
streaming_thread(void *arg_list)
{
	int rp_result, wmp_result;
	MSG msg;

	// Decrease the priority level on this thread, to ensure that the browser
	// and the rest of the system remains responsive.

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	// If the stream URL for Windows Media Player was specified, create that
	// stream.

	if (strlen(wmp_stream_URL) > 0) {
		wmp_result = create_stream(wmp_stream_URL, WINDOWS_MEDIA_PLAYER);
		if (wmp_result != STREAM_STARTED)
			destroy_stream();
	}

	// If the stream URL for RealPlayer was specified, create that stream.

	if (strlen(wmp_stream_URL) == 0 || wmp_result != STREAM_STARTED) {
		rp_result = create_stream(rp_stream_URL, REAL_PLAYER);
		if (rp_result != STREAM_STARTED)
			destroy_stream();
	}

	// If there was a stream URL for Windows Media Player, and it failed
	// to start, then either ask the user if they want to download Windows
	// Media Player, or display an error message.

	if (strlen(wmp_stream_URL) > 0 && wmp_result != STREAM_STARTED &&
		(strlen(rp_stream_URL) == 0 || rp_result != STREAM_STARTED)) { 
		if (wmp_result == PLAYER_UNAVAILABLE)
			download_wmp();
		else
			unsupported_wmp_format();
		return;
	}

	// If there was no stream URL for Windows Media Player, meaning there
	// was one for RealPlayer, and it failed to start, then either ask the user
	// if they want to download RealPlayer, or display an error message.

	if (strlen(wmp_stream_URL) == 0 && rp_result != STREAM_STARTED) {
		if (rp_result == PLAYER_UNAVAILABLE)
			download_rp();
		else
			unsupported_rp_format();
		return;
	}

	// Start playing the stream.

	play_stream();

	// Loop until a request to terminate the thread has been recieved.

	while (!terminate_streaming_thread.event_sent()) {

		// If we are using RealPlayer, dispatch any pending Windows message
		// and if the stream is playing looped check whether the stream has
		// finished, meaning it needs to be restarted from the beginning.

		if (media_player == REAL_PLAYER) {
			if (GetMessage(&msg, NULL, 0, 0))
				DispatchMessage(&msg);
			if (/* stream_command == STREAM_PLAY_LOOPED && */
				pPlayer->IsDone()) {
				stop_stream();
				play_stream();
			}
		} 
		
		// If we are using Windows Media Player, call the update function for
		// the stream.

		else
			update_stream();

		// If a new stream command has been sent, act upon it.

/*
		if (stream_command_sent.event_sent()) {
			switch (new_stream_command) {
			case STREAM_PLAY_ONCE:
			case STREAM_PLAY_LOOPED:
				if (stream_command == STREAM_STOP || 
					stream_command == STREAM_PAUSE)
					play_stream();
				stream_command = new_stream_command;
				break;
			case STREAM_STOP:
				if (stream_command == STREAM_PLAY_ONCE ||
					stream_command == STREAM_PLAY_LOOPED) {
					stop_stream();
					stream_command = STREAM_STOP;
				}
				break;
			case STREAM_PAUSE:
				if (stream_command == STREAM_PLAY_ONCE ||
					stream_command == STREAM_PLAY_LOOPED) {
					pause_stream();
					stream_command = STREAM_PAUSE;
				}
			}
		}
*/
	}

	// Stop then destroy the stream.

	stop_stream();
	destroy_stream();
}

//------------------------------------------------------------------------------
// Determine if the stream is ready.
//------------------------------------------------------------------------------

bool
stream_ready(void)
{
	return(stream_opened.event_sent());
}

//------------------------------------------------------------------------------
// Determine if download of RealPlayer was requested.
//------------------------------------------------------------------------------

bool
download_of_rp_requested(void)
{
	return(rp_download_requested.event_sent());
}

//------------------------------------------------------------------------------
// Determine if download of Windows Media Player was requested.
//------------------------------------------------------------------------------

bool
download_of_wmp_requested(void)
{
	return(wmp_download_requested.event_sent());
}

//------------------------------------------------------------------------------
// Send a stream command.
//------------------------------------------------------------------------------

/*
void
send_stream_command(int command)
{
	start_atomic_operation();
	new_stream_command = command;
	end_atomic_operation();
	stream_command_sent.send_event(true);
}
*/

//------------------------------------------------------------------------------
// Start the streaming thread.
//------------------------------------------------------------------------------

void
start_streaming_thread(void)
{
	// Create the events needed to communicate with the streaming thread.

	stream_opened.create_event();
//	stream_command_sent.create_event();
	terminate_streaming_thread.create_event();
	rp_download_requested.create_event();
	wmp_download_requested.create_event();

	// Set the initial stream command.

//	stream_command = STREAM_STOP;

	// Start the stream thread.

	streaming_thread_handle = _beginthread(streaming_thread, 0, NULL);
}

//------------------------------------------------------------------------------
// Stop the streaming thread.
//------------------------------------------------------------------------------

void
stop_streaming_thread(void)
{
	// Stop the stream, if it hasn't already stopped.

//	send_stream_command(STREAM_STOP);

	// Send the streaming thread a request to terminate, then wait for it to
	// do so.

	if (streaming_thread_handle >= 0) {
		terminate_streaming_thread.send_event(false);
		WaitForSingleObject((HANDLE)streaming_thread_handle, INFINITE);
	}

	// Destroy the events used to communicate with the stream thread.

	stream_opened.destroy_event();
//	stream_command_sent.destroy_event();
	terminate_streaming_thread.destroy_event();
	rp_download_requested.destroy_event();
	wmp_download_requested.destroy_event();
}
