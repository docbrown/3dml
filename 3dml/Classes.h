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

#include "collision\collision.h"

// Type definitions for various integer types.

typedef unsigned char byte;
typedef unsigned char imagebyte;	// Used for memory tracing.
typedef unsigned char cachebyte;	// Used for memory tracing.
typedef unsigned char colmeshbyte;	// Used for memory tracing.
typedef unsigned int dword;
typedef unsigned short word;
typedef unsigned int pixel;
typedef union {						// Used to represent 64-bit integers.
	__int64 complete; 
	struct {
		int low;
		int high;
	} partial;
} int64;

// Type definition and macros for fixed point values.

typedef int fixed;
#define FRAC_BITS	16
#define INT_MASK	0xffff0000

// Minimum delay before checking for an update.

#define MIN_UPDATE_PERIOD			604800		// One week

// Maximum string size.

#define	STRING_SIZE					256

// Default field of view.

#define DEFAULT_FIELD_OF_VIEW		60.0f

// Clipping plane indices.

#define NEAR_CLIPPING_PLANE			0
#define FAR_CLIPPING_PLANE			4

// Frustrum vertex and plane indices.

#define FRUSTUM_VERTICES			8
#define FRUSTUM_NEAR_PLANE			0
#define FRUSTUM_FAR_PLANE			1
#define FRUSTUM_LEFT_PLANE			2
#define FRUSTUM_RIGHT_PLANE			3
#define FRUSTUM_TOP_PLANE			4
#define FRUSTUM_BOTTOM_PLANE		5
#define FRUSTUM_PLANES				6

// Task bar height.

#define TASK_BAR_HEIGHT				20

// Number of texels per unit (block size in texels is 256, and in world
// units is 3.2).

#define UNITS_PER_BLOCK				3.2f
#define UNITS_PER_HALF_BLOCK		1.6f
#define	TEXELS_PER_UNIT				80.0f

// First and last block symbol characters, the number of block symbols available,
// the NULL block symbol, and the ground block symbol.

#define FIRST_BLOCK_SYMBOL		'!'
#define LAST_BLOCK_SYMBOL		'~'
#define BLOCK_SYMBOLS			(LAST_BLOCK_SYMBOL - FIRST_BLOCK_SYMBOL + 1)			
#define NULL_BLOCK_SYMBOL		'.'
#define GROUND_BLOCK_SYMBOL		' '

// Mathematical macros.

#define PI						3.141592654f
#define RAD(x)					((x) / 180.0f * PI)
#define DEG(x)					((x) / PI * 180.0f)
#define ABS(x)					((x) < 0 ? -(x) : (x))
#define INT(x)					(float)((int)(x))
#define CEIL(x)					((x) == INT(x) ? INT(x) : INT(x + 1))
#define MIN(x,y)				((x) < (y) ? (x) : (y))
#define MAX(x,y)				((x) > (y) ? (x) : (y))

// Macro for comparing floating point values with an epsilon value for avoiding
// small errors in precision.

#define EPSILON		0.0001f
#define FEQ(x,y)	(ABS((x) - (y)) < EPSILON)
#define FNE(x,y)	(ABS((x) - (y)) >= EPSILON)
#define FLT(x,y)	((x) < ((y) - EPSILON))
#define FLE(x,y)	((x) <= ((y) + EPSILON))
#define FGT(x,y)	((x) > ((y) + EPSILON))
#define FGE(x,y)	((x) >= ((y) - EPSILON))

// Macros to simplify getting vertices from a particular polygon and block.

#define PREPARE_VERTEX_LIST(block_ptr) \
	vertex *vertex_list = block_ptr->vertex_list

#define PREPARE_VERTEX_DEF_LIST(polygon_ptr) \
	vertex_def *vertex_def_list = polygon_ptr->vertex_def_list

#define VERTEX(n)			vertex_list[vertex_def_list[n].vertex_no]
#define VERTEX_PTR(n)		&VERTEX(n)

// Compass directions.

enum compass { 
	EAST, SOUTH, UP, WEST, NORTH, DOWN, NONE
};

// Arrow cursors.

enum arrow {
	ARROW_N, ARROW_NE, ARROW_E, ARROW_SE, ARROW_S, ARROW_SW, ARROW_W, ARROW_NW
};

// Texture styles.

enum texstyle { TILED_TEXTURE, SCALED_TEXTURE, STRETCHED_TEXTURE };

// Light styles.

enum lightstyle	{ 
	DIRECTIONAL_LIGHT, 
	STATIC_POINT_LIGHT, PULSATING_POINT_LIGHT,
	STATIC_SPOT_LIGHT, REVOLVING_SPOT_LIGHT, SEARCHING_SPOT_LIGHT
};

// Audio styles.

enum audiostyle { PASS_AUDIO, OCCLUDE_AUDIO };

// Map styles.

enum mapstyle { SINGLE_MAP, DOUBLE_MAP };

// Action types.

#define REPLACE_ACTION		0
#define EXIT_ACTION			1
#define SET_STREAM_ACTION	2

// Trigger flags.

#define ROLL_ON			1
#define ROLL_OFF		2
#define ROLLOVER		4
#define CLICK_ON		8
#define STEP_ON			16
#define STEP_IN			32
#define STEP_OUT		64
#define PROXIMITY		128
#define TIMER			256
#define EVERYWHERE		512
#define LOCATION		1024
#define	MOUSE_TRIGGERS	(ROLL_ON | ROLL_OFF | ROLLOVER | CLICK_ON)

// Alignments.

enum alignment { 
	TOP_LEFT, TOP, TOP_RIGHT,
	LEFT, CENTRE, RIGHT, 
	BOTTOM_LEFT, BOTTOM, BOTTOM_RIGHT,
	MOUSE
};

// Block types.

enum blocktype { 
	STRUCTURAL_BLOCK = 1, 
	MULTIFACETED_SPRITE = 2, 
	ANGLED_SPRITE = 4, 
	REVOLVING_SPRITE = 8,
	FACING_SPRITE = 16,
	PLAYER_SPRITE = 32
};
#define SPRITE_BLOCK	(MULTIFACETED_SPRITE | ANGLED_SPRITE | \
						 REVOLVING_SPRITE | FACING_SPRITE | PLAYER_SPRITE)

// Playback mode of a sound.

enum playmode { LOOPED_PLAY, RANDOM_PLAY, SINGLE_PLAY, ONE_PLAY };

// Stream command.

#define STREAM_PLAY_ONCE	0
#define STREAM_PLAY_LOOPED	1
#define STREAM_STOP			2
#define STREAM_PAUSE		3

// Imagemap shapes.

enum shape { RECT_SHAPE, CIRCLE_SHAPE };

// Number of brightness levels.

#define BRIGHTNESS_LEVELS		8
#define	MAX_BRIGHTNESS_INDEX	(BRIGHTNESS_LEVELS - 1)

// Number of image sizes.

#define IMAGE_SIZES			8

// Rate changes.

#define RATE_SLOWER			-1
#define RATE_FASTER			 1

// Number of blocks per audio block.

#define AUDIO_BLOCK_DIMENSIONS	4
#define AUDIO_BLOCK_SHIFT		2

// Video pixel formats.

#define RGB24	0
#define YUV12	1
#define RGB16	2

//==============================================================================
// Timing macros.
//==============================================================================

extern float cycles_per_second;
extern int curr_tabs;

#ifdef PROFILE

#define START_TIMING \
	curr_tabs++; \
	int64 start_cycle, elapsed_cycles; \
	__asm { \
		__asm _emit 0x0f \
		__asm _emit 0x31 \
		__asm mov start_cycle.partial.low, eax \
		__asm mov start_cycle.partial.high, edx \
	}

#define END_TIMING(message) \
	__asm { \
		__asm _emit 0x0f \
		__asm _emit 0x31 \
		__asm sub eax, start_cycle.partial.low \
		__asm sbb edx, start_cycle.partial.high \
		__asm mov elapsed_cycles.partial.low, eax \
		__asm mov elapsed_cycles.partial.high, edx \
	} \
	diagnose(curr_tabs, "Time spent in %s was %I64d cycles (%g seconds)", \
		message, elapsed_cycles.complete, \
		(float)elapsed_cycles.complete / cycles_per_second); \
	curr_tabs--

#define DEFINE_SUM(sum_cycles) static int64 sum_cycles

#define	CLEAR_SUM(sum_cycles) sum_cycles.complete = 0

#define START_SUMMING \
	int64 start_cycle; \
	__asm { \
		__asm _emit 0x0f \
		__asm _emit 0x31 \
		__asm mov start_cycle.partial.low, eax \
		__asm mov start_cycle.partial.high, edx \
	}

#define END_SUMMING(sum_cycles) \
	__asm { \
		__asm _emit 0x0f \
		__asm _emit 0x31 \
		__asm sub eax, start_cycle.partial.low \
		__asm sbb edx, start_cycle.partial.high \
		__asm add sum_cycles.partial.low, eax \
		__asm adc sum_cycles.partial.high, edx \
	}

#define REPORT_SUM(message, sum_cycles) \
	diagnose(curr_tabs, "Sum of time spent in %s was %I64d cycles (%g seconds)", \
		message, sum_cycles.complete, \
		(float)sum_cycles.complete / cycles_per_second);

#else // !PROFILE

#define START_TIMING
#define END_TIMING(message)
#define DEFINE_SUM(sum_cycles)
#define	CLEAR_SUM(sum_cycles)
#define START_SUMMING
#define END_SUMMING(sum_cycles)
#define REPORT_SUM(message, sum_cycles)

#endif // PROFILE

//==============================================================================
// Basic data classes.
//==============================================================================

//------------------------------------------------------------------------------
// String class.  Semantically it behaves as a character array, only one that
// has a variable length.
//------------------------------------------------------------------------------

class string {
private:
	char *text;

public:
	string();
	string(char *new_text);
	string(const char *new_text);
	string(const string &old);
	~string();
	operator char *();
	char& operator[](int index);
	string& operator=(const string &old);
	string& operator +=(const char *new_text);
	string operator +(const char *new_text);
	void truncate(unsigned int new_length);
	void copy(const char *new_text, unsigned int new_length);
};

//------------------------------------------------------------------------------
// RGB colour class.
//------------------------------------------------------------------------------

struct RGBcolour {
	float red, green, blue;			// Colour components.

	RGBcolour();
	~RGBcolour();
	void set(RGBcolour colour);
	void set_RGB(float new_red, float new_green, float new_blue);
	void adjust_brightness(float brightness);
	void normalise(void);
	void blend(RGBcolour colour);
};

//------------------------------------------------------------------------------
// Pixel format class.
//------------------------------------------------------------------------------

struct pixel_format {
	pixel red_mask, green_mask, blue_mask;
	int red_right_shift, green_right_shift, blue_right_shift;
	int red_left_shift, green_left_shift, blue_left_shift;
	pixel alpha_comp_mask;
};

//------------------------------------------------------------------------------
// Texture coordinates class.
//------------------------------------------------------------------------------

struct texcoords {
	float u, v;

	texcoords();
	~texcoords();
};

//------------------------------------------------------------------------------
// Map coordinates class.
//------------------------------------------------------------------------------

struct mapcoords {
	int column, row, level;

	mapcoords();
	~mapcoords();
	void set(int set_column, int set_row, int set_level);
};

//------------------------------------------------------------------------------
// Relative coordinates class.
//------------------------------------------------------------------------------

struct relcoords {
	int column, row, level;
	bool relative_column, relative_row, relative_level;

	relcoords();
	~relcoords();
	void set(int set_column, int set_row, int set_level,
		bool set_relative_column, bool set_relative_row, 
		bool set_relative_level);
};

//------------------------------------------------------------------------------
// Size class.
//------------------------------------------------------------------------------

struct size {
	int width, height;
};

//------------------------------------------------------------------------------
// Orientation class.
//------------------------------------------------------------------------------

struct orientation {
	float angle_x, angle_y, angle_z;
	compass direction;					// NONE if (x,y,z) orientation defined.
	float angle;

	orientation();
	~orientation();
	void set(float set_angle_x, float set_angle_y, float set_angle_z);
};

//------------------------------------------------------------------------------
// Direction class.
//------------------------------------------------------------------------------

struct direction {
	float angle_x, angle_y;

	direction();
	~direction();
	void set(float set_angle_x, float set_angle_y); 
};

//------------------------------------------------------------------------------
// Direction range class.
//------------------------------------------------------------------------------

struct dirrange {
	direction min_direction;
	direction max_direction;
};

//------------------------------------------------------------------------------
// Percentage range class.
//------------------------------------------------------------------------------

struct pcrange {
	float min_percentage;
	float max_percentage;

	pcrange();
	~pcrange();
};

//------------------------------------------------------------------------------
// Delay range class.
//------------------------------------------------------------------------------

struct delayrange {
	int min_delay_ms;
	int delay_range_ms;

	delayrange();
	~delayrange();
};

//------------------------------------------------------------------------------
// Block reference.
//------------------------------------------------------------------------------

struct blockref {
	bool is_symbol;			// TRUE if symbol valid, FALSE if location valid.
	word symbol;			// Block symbol.
	relcoords location;		// Block location.
};

//------------------------------------------------------------------------------
// Vertex entry class.
//------------------------------------------------------------------------------

struct vertex_entry {
	float x, y, z;
	vertex_entry *next_vertex_entry_ptr;
};

//------------------------------------------------------------------------------
// Vertex class.
//------------------------------------------------------------------------------

struct vector;						// Forward declarations.
struct block;
struct polygon;

struct vertex {
	float x, y, z, w;

	vertex();
	vertex(float set_x, float set_y, float set_z);
	~vertex();
	void set(float x, float y, float z);
	void scale(void);
	void set_map_position(int column, int row, int level);
	void set_map_translation(int column, int row, int level);
	void set_scaled_map_position(int column, int row, int level);
	void get_scaled_map_position(int *column_ptr, int *row_ptr, int *level_ptr);
	int get_scaled_map_level(void);
	void get_scaled_audio_map_position(int *audio_column_ptr,
		int *audio_row_ptr, int *audio_level_ptr);
	vertex operator +(vertex A);
	vector operator -(vertex A);
	vertex operator +(vector B);
	vertex operator -(vector B);
	vertex operator +(float A);
	vertex operator -(float A);
	vertex operator *(float A);
	vertex operator /(float A);
	vertex& operator +=(vertex &A);
	vertex& operator -=(vertex &A);
	vertex& operator +=(vector &A);
	vertex& operator -=(vector &A);
	vertex operator -();
	bool operator ==(vertex A);
	bool operator !=(vertex A);
	void rotatex(float angle);
	void rotatey(float angle);
	void rotatez(float angle);
};

//------------------------------------------------------------------------------
// Vector class.
//------------------------------------------------------------------------------

struct vector {
	float dx, dy, dz;

	vector();
	vector(float set_dx, float set_dy, float set_dz);
	~vector();
	void set(float set_dx, float set_dy, float set_dz);

	vector operator *(float A);		// Scaling operator.
	vector operator *(vector A);	// Cross product.
	float operator &(vector A);		// Dot product.
	vector operator +(vector A);
	vector operator -();
	bool operator !();
	bool operator ==(vector A);
	void normalise(void);			// Normalise vector.
	float length(void);				// Normalise vector, returning it's length.
	void rotatex(float look_angle);	// Rotate around X axis.
	void rotatey(float turn_angle);	// Rotate around Y axis.
};	

//------------------------------------------------------------------------------
// Viewpoint class.
//------------------------------------------------------------------------------

struct viewpoint {
	vertex position;
	vertex last_position;
	float turn_angle, look_angle;
	int inv_turn_angle, inv_look_angle;
};

//------------------------------------------------------------------------------
// Rectangle class.
//------------------------------------------------------------------------------

struct rect {
	int x1, y1, x2, y2;
};

//------------------------------------------------------------------------------
// Video rectangle class; each coordinate is expressed in pixel units or as a
// ratio.
//------------------------------------------------------------------------------

struct video_rect {
	float x1, y1, x2, y2;
	bool x1_is_ratio, y1_is_ratio, x2_is_ratio, y2_is_ratio;
};

//------------------------------------------------------------------------------
// Circle class.
//------------------------------------------------------------------------------

struct circle {
	int x, y, r;
};

//==============================================================================
// Polygon rasterisation classes.
//==============================================================================

//------------------------------------------------------------------------------
// Screen point class.
//------------------------------------------------------------------------------

struct spoint {
	float sx, sy;					// Projected screen point.
	float one_on_tz;				// Interpolated depth.
	float u_on_tz, v_on_tz;			// Interpolated texture coordinates.
	RGBcolour colour;				// Lit and normalised colour.
};

//------------------------------------------------------------------------------
// Screen polygon class.
//------------------------------------------------------------------------------

struct pixmap;						// Forward declarations.

struct spolygon {
	pixmap *pixmap_ptr;				// Pointer to pixmap.
	int spoints;					// Number of screen points used.
	int max_spoints;				// Maximum number of screen points.
	spoint *spoint_list;			// List of screen points.
	float alpha;					// Translucency factor.
	spolygon *next_spolygon_ptr;	// Next screen polygon in list.
	spolygon *next_spolygon_ptr2;	// Next screen polygon in secondary list.

	spolygon();
	~spolygon();
	bool create_spoint_list(int set_spoints);
};

//------------------------------------------------------------------------------
// Polygon edge element.
//------------------------------------------------------------------------------

struct edge {
	float sx;						// Screen x coordinate (fractional).
	float one_on_tz;				// 1/tz.
	float u_on_tz;					// u/tz.
	float v_on_tz;					// v/tz.
};

//------------------------------------------------------------------------------
// Span data element.
//------------------------------------------------------------------------------

struct span_data {
	float one_on_tz;				// 1/tz.
	float u_on_tz;					// u/tz.
	float v_on_tz;					// v/tz.
};

//------------------------------------------------------------------------------
// Span buffer element.
//------------------------------------------------------------------------------

struct pixmap;							// Forward declaration.

struct span {
	int sy;								// Screen y coordinate for span.
	int start_sx;						// Start screen x coordinate for span.
	int end_sx;							// End screen x coordinate + 1 for span.
	span_data start_span;				// Start values for span.
	span_data delta_span;				// Delta values for span.
	bool is_popup;						// TRUE if this span belongs to a popup.
	pixmap *pixmap_ptr;					// Pixmap to render on span (or NULL).
	pixel colour_pixel;					// Colour to render on span.
	int brightness_index;				// Brightness index for span.
	span *next_span_ptr;				// Pointer to next span in list.

	span();
	~span();
	void adjust_start(int new_start_sx);
	bool span_in_front(span *span_ptr);
};

//------------------------------------------------------------------------------
// Span buffer row element.
//------------------------------------------------------------------------------

struct span_row {
	span *opaque_span_list;			// Opaque spans (sorted left to right).
	span *transparent_span_list;	// Transparent spans (sorted back to front).

	span_row();
	~span_row();
};

//------------------------------------------------------------------------------
// Span buffer class.
//------------------------------------------------------------------------------

struct span_buffer {
	int rows;					// Number of span buffer rows.
	span_row *buffer_ptr;		// Pointer to span buffer rows.

	span_buffer();
	~span_buffer();
	bool create_buffer(int set_rows);
	span_row *operator[](int row);
};

//==============================================================================
// Texture cache classes.
//==============================================================================

//------------------------------------------------------------------------------
// Texture cache entry.
//------------------------------------------------------------------------------

struct pixmap;							// Forward declaration.

struct cache_entry {
	pixmap *pixmap_ptr;					// Pointer to pixmap using this entry.
	int brightness_index;				// Brightness index used to light image.
	int frame_no;						// Frame number of last reference.
	cachebyte *lit_image_ptr;			// Pointer to lit image in cache.
	int lit_image_size;					// Size of lit image, in bytes.
	int lit_image_mask;					// u/v coordinate mask.
	int lit_image_shift;				// v coordinate shift.
	void *hardware_texture_ptr;			// Pointer to hardware texture.
	cache_entry *next_cache_entry_ptr;	// Pointer to next cache entry in list.

	cache_entry();
	~cache_entry();
	bool create_image_buffer(int image_size_index);
};

//------------------------------------------------------------------------------
// Texture cache.
//------------------------------------------------------------------------------

struct cache {
	int image_size_index;			// Size of images stored in this cache.
	cache_entry *cache_entry_list;	// Linked list of cache entries.

	cache();
	cache(int size_index);
	~cache();
	cache_entry *add_cache_entry(void);
};

//==============================================================================
// Texture map classes.
//==============================================================================

//------------------------------------------------------------------------------
// Pixmap class.
//------------------------------------------------------------------------------

struct pixmap {
	int width, height;				// Image dimensions.
	int size_index;					// Size index.
	bool image_is_16_bit;			// TRUE if image data is 16-bit.
	imagebyte *image_ptr;			// Pointer to 8-bit or 16-bit image data.
	int image_size;					// Size of image in bytes.
	int colours;					// Number of colours in palette.
	pixel *display_palette_list;	// Palette list using display pixel format.
	pixel *texture_palette_list;	// Palette list using texture pixel format.
	byte *palette_index_table;		// Palette index table for remapping texels.
	int transparent_index;			// Index of transparent colour.
	int delay_ms;					// Animation delay time in milliseconds.
	cache_entry *cache_entry_list[BRIGHTNESS_LEVELS];	// Cache entries.
	span *span_lists[BRIGHTNESS_LEVELS];				// Span lists.
	bool image_updated[BRIGHTNESS_LEVELS];				// Image updated list.
	spolygon *spolygon_list;		// Screen polygon list.

	pixmap();
	~pixmap();
};

//------------------------------------------------------------------------------
// Texture class.
//------------------------------------------------------------------------------

struct texture {
	bool custom;					// TRUE if custom texture.
	int load_index;					// Load index for this texture (if custom).
	bool transparent;				// TRUE if at least 1 pixmap is transparent.
	bool loops;						// TRUE if animation loops.
	bool is_16_bit;					// TRUE if texture is 16-bit.
	string URL;						// URL of GIF or JPEG.
	int width, height;				// Dimensions of largest image.
	int pixmaps;					// Number of pixmaps.
	pixmap *pixmap_list;			// Array of pixmaps.
	int total_time_ms;				// Total time required for animation.
	int colours;					// Number of colours in palette.
	int brightness_levels;			// Number of brightness levels used.
	RGBcolour *RGB_palette;			// Original RGB palette.
	pixel *display_palette_list;	// Palette list using display pixel format.
	pixel *texture_palette_list;	// Palette list using texture pixel format.
	byte *palette_index_table;		// Palette index table for remapping texels.
	texture *next_texture_ptr;		// Pointer to next texture.

	texture();
	~texture();
	bool create_RGB_palette(int set_colours, int set_brightness_levels,
		RGBcolour *set_RGB_palette);
	bool create_display_palette_list(void);
	bool create_texture_palette_list(void);
	bool create_palette_index_table(void);
	pixmap *get_curr_pixmap_ptr(int elapsed_time_ms);
};

//------------------------------------------------------------------------------
// Video texture class.
//------------------------------------------------------------------------------

struct video_texture {
	texture *texture_ptr;					// Pointer to texture.
	video_rect source_rect;					// Source rectangle in video frame.
	float delta_u, delta_v;					// Delta (u,v) for scaling.
	video_texture *next_video_texture_ptr;	// Pointer to next video texture.

	video_texture();
	~video_texture();
};

//==============================================================================
// Special object classes.
//==============================================================================

//------------------------------------------------------------------------------
// Light class.
//------------------------------------------------------------------------------

struct square;						// Forward declaration.

struct light {
	bool from_block;				// TRUE if this light came from a block.
	square *square_ptr;				// Square that light belongs to.
	lightstyle style;				// Style.
	vertex pos;						// Position.
	dirrange dir_range;				// Direction range.
	direction delta_dir;			// Change in direction per second.
	direction curr_dir;				// Current direction.
	vector dir;						// Current direction as a vector.
	RGBcolour colour;				// Colour.
	pcrange intensity_range;		// Intensity range.
	float delta_intensity;			// Change in intensity per second.
	float intensity;				// Intensity.
	RGBcolour lit_colour;			// Colour at intensity.
	float radius;					// Radius of light.
	float one_on_radius;			// 1 / radius.
	float cos_cone_angle;			// Cosine of cone angle.
	float cone_angle_M;				// 1 / (1 - cosine of cone angle).
	bool flood;						// TRUE if flood light (no light dropoff).
	light *next_light_ptr;			// Pointer to next light in list.

	light();
	~light();
	void set_direction(direction light_direction);
	void set_dir_range(dirrange light_dir_range);
	void set_dir_speed(float light_speed);
	void set_intensity(float light_intensity);
	void set_intensity_range(pcrange light_intensity_range);
	void set_intensity_speed(float light_speed);
	void set_radius(float light_radius);
	void set_cone_angle(float light_cone_angle);
};

//------------------------------------------------------------------------------
// Wave class.
//------------------------------------------------------------------------------

struct wave {
	bool custom;					// TRUE if custom wave.
	int load_index;					// Load index for this wave (if custom).
	string URL;						// URL of wave.
	void *format_ptr;				// Pointer to wave format.
	char *data_ptr;					// Pointer to wave data.
	int data_size;					// Size of wave data.
	wave *next_wave_ptr;			// Next wave in list.

	wave();
	~wave();
};

//------------------------------------------------------------------------------
// Sound class.
//------------------------------------------------------------------------------

struct sound {
	bool from_block;				// TRUE if this sound came from a block.
	square *square_ptr;				// Square that sound belongs to.
	bool ambient;					// TRUE if an ambient sound.
	bool streaming;					// TRUE if a streaming sound.
	wave *wave_ptr;					// Pointer to wave.
	void *sound_buffer_ptr;			// Pointer to sound buffer.
	vertex position;				// Position of sound.
	float volume;					// Volume of sound.
	float radius;					// Radius of sound field.
	playmode playback_mode;			// Playback mode.
	delayrange delay_range;			// Range of playback delays allowed.
	int start_time_ms;				// Start time for delay.
	int delay_ms;					// Current delay in milliseconds.
	bool flood;						// TRUE if flood mode (no sound rolloff).
	float rolloff;					// Sound rolloff factor.
	bool reflections;				// TRUE if reflections currently enabled.
	bool in_range;					// TRUE if sound is in range.
	bool played_once;				// TRUE if sound has been played once.
	sound *next_sound_ptr;			// Next sound in list.

	sound();
	~sound();
};

//------------------------------------------------------------------------------
// Location class.
//------------------------------------------------------------------------------

struct location {
	bool from_block;				// TRUE if location is from a block.
	int column, row, level;			// Location on map.
	direction initial_direction;	// Initial direction of player.
	location *next_location_ptr;	// Next location in list.
};

//------------------------------------------------------------------------------
// Entrance class.
//------------------------------------------------------------------------------

struct entrance {
	string name;					// Entrance name.
	int locations;					// Number of locations with this entrance.
	location *location_list;		// List of entrance locations.
	entrance *next_entrance_ptr;	// Pointer to next entrance in list.

	entrance();
	~entrance();
	location *find_location(int column, int row, int level);
	bool add_location(int column, int row, int level, 
		direction initial_direction, bool from_block);
	bool del_location(int column, int row, int level, bool from_block);
	location *choose_location(void);
};

//------------------------------------------------------------------------------
// Action class.
//------------------------------------------------------------------------------

struct action {
	int type;					// Action type.
	blockref source;			// Replace source symbol/location.
	relcoords target;			// Replace target location.
	bool target_is_trigger;		// TRUE if replace target is trigger.
	string exit_URL;			// Exit URL.
	string exit_target;			// Exit target.
//	string stream_name;			// Stream name.
//	int stream_command;			// Stream command.
	action *next_action_ptr;	// Next action in list.
};

//------------------------------------------------------------------------------
// Trigger class.
//------------------------------------------------------------------------------

struct square;					// Forward declaration.

struct trigger {
	int trigger_flag;			// Trigger flag.
	bool from_block;			// TRUE if this trigger came from a block.
	square *square_ptr;			// Square that trigger belongs to.
	vertex position;			// Position of trigger (for "step in/out").
	float radius_squared;		// Radius squared of trigger (for "step in/out").
	bool previously_inside;		// TRUE if player was previously inside radius.
	delayrange delay_range;		// Range of timer delays allowed (for "timer").
	int start_time_ms;			// Start time of delay (for "timer").
	int delay_ms;				// Delay time (for "timer").
	mapcoords target;			// Target location (for "location").
	action *action_list;		// List of actions.
	string label;				// Text for label (only used for "click on").
	trigger *next_trigger_ptr;	// Next trigger in list.

	trigger();
	~trigger();
};

//------------------------------------------------------------------------------
// Hyperlink class.
//------------------------------------------------------------------------------

struct hyperlink {
	bool from_block;				// TRUE if hyperlink is from a block.
	int trigger_flags;				// Trigger flags.
	string label;					// Text for label.
	string URL;						// URL.
	string target;					// Window target.
	hyperlink *next_hyperlink_ptr;	// Next hyperlink in list.

	hyperlink();
	~hyperlink();
	void set(const char *set_URL, const char *set_target,
		const char *set_label);
};

//------------------------------------------------------------------------------
// Area class.
//------------------------------------------------------------------------------

struct area {
	shape shape_type;				// Shape type.
	union {
		rect rect_shape;			// Rectangle coordinates.
		circle circle_shape;		// Circle coordinates and radius.
	};
	hyperlink *exit_ptr;			// Exit attached to this area.
	int trigger_flags;				// Trigger flags.
	trigger *trigger_list;			// List of triggers attached to this area.
	area *next_area_ptr;			// Next area in list.

	area();
	~area();
};

//------------------------------------------------------------------------------
// Imagemap class.
//------------------------------------------------------------------------------

struct imagemap {
	string name;					// Imagemap name.
	area *area_list;				// List of areas in this imagemap.
	imagemap *next_imagemap_ptr;	// Next imagemap in list.

	imagemap();
	~imagemap();
	area *find_area(rect *rect_ptr);
	area *find_area(circle *circle_ptr);
	bool add_area(rect *rect_ptr, hyperlink *exit_ptr);
	bool add_area(rect *rect_ptr, trigger *trigger_ptr);
	bool add_area(circle *circle_ptr, hyperlink *exit_ptr);
	bool add_area(circle *circle_ptr, trigger *trigger_ptr);
};

//------------------------------------------------------------------------------
// Popup class.
//------------------------------------------------------------------------------

struct popup {
	bool from_block;				// TRUE if this popup came from a block.
	square *square_ptr;				// Square that popup belongs to.
	int trigger_flags;				// Trigger flags.
	int visible_flags;				// Visible flags.
	bool always_visible;			// TRUE if popup is always visible.
	vertex position;				// Position of popup.
	alignment window_alignment;		// Placement of popup on window.
	float radius_squared;			// Radius of popup squared.
	float brightness;				// Popup brightness.
	int brightness_index;			// Popup brightness index.
	RGBcolour colour;				// Background colour.
	texture *bg_texture_ptr;		// Pointer to background texture (or NULL).
	texture *fg_texture_ptr;		// Pointer to foreground texture (or NULL).
	bool transparent_background;	// TRUE for transparent colour background.
	int width, height;				// Size of popup in pixels.
	string text;					// Text to be drawn on popup.
	RGBcolour text_colour;			// Text colour.
	alignment text_alignment;		// Text alignment mode.
	imagemap *imagemap_ptr;			// Pointer to imagemap (or NULL).
	pixmap *bg_pixmap_ptr;			// Pointer to current background pixmap.
	int sx, sy;						// Current screen position.
	int start_time_ms;				// Time popup was made visible.
	popup *next_popup_ptr;			// Next popup in list.
	popup *next_square_popup_ptr;	// Next popup in square list.
	popup *next_visible_popup_ptr;	// Next popup in visible list.

	popup();
	~popup();
};

//==============================================================================
// Block definition and style classes.
//==============================================================================

//------------------------------------------------------------------------------
// Part class.
//------------------------------------------------------------------------------

struct part {
	string name;					// Name of part.
	texture *texture_ptr;			// Pointer to current texture (or NULL).
	texture *custom_texture_ptr;	// Pointer to custom texture (or NULL).
	bool colour_set;				// TRUE if colour is set.
	RGBcolour colour;				// RGB colour (used if no texture).
	RGBcolour normalised_colour;	// Normalised part colour.
	float alpha;					// Alpha value (0.0f = transparent).
	texstyle texture_style;			// Texture style.
	float texture_angle;			// Texture angle.
	int faces;						// Number of faces per polygon (0, 1 or 2).
	compass projection;				// Texture projection.
	bool solid;						// TRUE if part is solid.
	part *next_part_ptr;			// Next part in list.

	part();
	~part();
};

//------------------------------------------------------------------------------
// Vertex definition entry class.
//------------------------------------------------------------------------------

struct vertex_def_entry {
	int vertex_no;
	float u, v;
	vertex_def_entry *next_vertex_def_entry_ptr;
};

//------------------------------------------------------------------------------
// Vertex definition class.
//------------------------------------------------------------------------------

struct vertex_def {
	int vertex_no;					// Index of vertex in block's vertex list.
	float u, v;						// 2D texture coordinate at vertex.
	float orig_u, orig_v;			// Original texture coordinates.

	vertex_def();
	~vertex_def();
};

//------------------------------------------------------------------------------
// Polygon class.
//------------------------------------------------------------------------------

struct polygon {
	int part_no;					// Part number.
	part *part_ptr;					// Pointer to part.
	int vertices;					// Number of vertex definitions.
	vertex_def *vertex_def_list;	// Clockwise list of vertex definitions.
	vertex centroid;				// Centroid of polygon.
	vector normal_vector;			// Surface normal vector.
	float plane_offset;				// Offset in plane equation.
	compass direction;				// Cardinal direction polygon is facing.
	bool side;						// TRUE if this is a side polygon.
	int front_polygon_ref;			// Front polygon reference in BSP tree.
	int rear_polygon_ref;			// Rear polygon reference in BSP tree.
	polygon *next_polygon_ptr;		// Pointer to next polygon in list.

	polygon();
	~polygon();
	bool create_vertex_def_list(int set_vertices);
	void compute_centroid(vertex *vertex_list);
	void compute_normal_vector(vertex *vertex_list);
	void compute_plane_offset(vertex *vertex_list);
	void project_texture(vertex *vertex_list, compass projection);
	polygon& operator=(const polygon &old_polygon);
};

//------------------------------------------------------------------------------
// BSP node class.
//------------------------------------------------------------------------------

struct BSP_node {
	int polygon_no;
	BSP_node *front_node_ptr;
	BSP_node *rear_node_ptr;

	BSP_node();
	~BSP_node();
};

//------------------------------------------------------------------------------
// Block definition class.
//------------------------------------------------------------------------------

struct block;						// Forward declaration.

struct block_def {
	bool custom;					// TRUE if custom block.
	char single_symbol;				// Single-character symbol.
	word double_symbol;				// Double-character symbol.
	string name;					// Block name.
	blocktype type;					// Block type.
	bool allow_entrance;			// TRUE if block permits an entrance.
	int parts;						// Size of part list.
	part *part_list;				// List of all parts in block.
	int vertices;					// Size of vertex list.
	vertex *vertex_list;			// List of all vertices in block.
	int polygons;					// Size of polygon list. 
	polygon *polygon_list;			// List of all polygons in block.
	light *light_ptr;				// Pointer to light (if any).
	sound *sound_ptr;				// Pointer to sound (if any).
	popup *popup_ptr;				// Pointer to popup (if any).
	string entrance_name;			// Pointer to entrance name (if any).
	direction initial_direction;	// Initial direction of player at entrance.
	hyperlink *exit_ptr;			// Pointer to exit (if any).
	int trigger_flags;				// Trigger flags.
	trigger *trigger_list;			// List of triggers (if any).
	block *free_block_list;			// List of free blocks (if any).
	int root_polygon_ref;			// Root polygon reference in BSP tree.
	BSP_node *BSP_tree;				// Pointer to BSP tree (if any).
	float sprite_angle;				// Angle of sprite (if applicable).
	float degrees_per_ms;			// Rotational speed (if applicable).
	alignment sprite_alignment;		// Sprite alignment (if applicable).
	size sprite_size;				// Sprite size (if applicable).
	bool solid;						// TRUE if block is solid.
	bool movable;					// TRUE is block is movable.
	block_def *next_block_def_ptr;	// Pointer to next block def. in list.

	block_def();
	~block_def();
	void create_part_list(int set_parts);
	void create_vertex_list(int set_vertices);
	void create_polygon_list(int set_polygons);
	void dup_block_def(block_def *block_def_ptr);
	void orient_block_def(orientation block_orientation);
	block *new_block(void);
	block *del_block(block *block_ptr);
};

//------------------------------------------------------------------------------
// Blockset class.
//------------------------------------------------------------------------------

struct blockset {
	string URL;									// URL of blockset.
	string name;								// Name of blockset.
	block_def *block_def_list;					// List of block definitions.
	texture *placeholder_texture_ptr;			// Placeholder texture.
	bool sky_defined;							// TRUE if sky defined.
	texture *sky_texture_ptr;					// Pointer to sky texture.
	bool sky_colour_set;						// TRUE is sky colour set.
	RGBcolour sky_colour;						// Sky colour.
	bool sky_brightness_set;					// TRUE if sky brightness set.
	float sky_brightness;						// Sky brightness.
	bool ground_defined;						// TRUE if ground defined.
	texture *ground_texture_ptr;				// Pointer to ground texture.
	bool ground_colour_set;						// TRUE if ground colour set.
	RGBcolour ground_colour;					// Ground colour.
	bool orb_defined;							// TRUE if orb defined.
	texture *orb_texture_ptr;					// Pointer to orb texture.
	bool orb_direction_set;						// TRUE if orb direction set.
	direction orb_direction;					// Orb direction.
	bool orb_brightness_set;					// TRUE if orb brightness set.
	float orb_brightness;						// Orb brightness.
	bool orb_colour_set;						// TRUE if orb colour set.
	RGBcolour orb_colour;						// Orb colour.
	hyperlink *orb_exit_ptr;					// Pointer to orb exit.
	texture *first_texture_ptr;					// First texture in list.
	texture *last_texture_ptr;					// Last texture in list.
	wave *first_wave_ptr;						// First wave in list.
	wave *last_wave_ptr;						// Last wave in list.
	blockset *next_blockset_ptr;				// Next blockset in list.

	blockset();
	~blockset();
	void add_block_def(block_def *block_def_ptr);
	bool delete_block_def(word symbol);
	block_def *get_block_def(char single_symbol);
	block_def *get_block_def(word double_symbol);
	block_def *get_block_def(const char *name);
	block_def *get_block_def(char single_symbol, const char *name);
	void add_wave(wave *wave_ptr);
	void add_texture(texture *texture_ptr);
};

//------------------------------------------------------------------------------
// Cached blockset class.
//------------------------------------------------------------------------------

struct cached_blockset {
	string href;
	int size;
	int updated;
	string name;
	string synopsis;
	unsigned int version;
	cached_blockset *next_cached_blockset_ptr;
};

//------------------------------------------------------------------------------
// Blockset list class.
//------------------------------------------------------------------------------

struct blockset_list {
	blockset *first_blockset_ptr;				// First blockset in list.
	blockset *last_blockset_ptr;				// Last blockset in list.

	blockset_list();
	~blockset_list();
	void add_blockset(blockset *blockset_ptr);
	bool find_blockset(char *URL);
	blockset *remove_blockset(char *URL);
};

//==============================================================================
// Block and map classes.
//==============================================================================

//------------------------------------------------------------------------------
// Block class.
//------------------------------------------------------------------------------

struct block {
	block_def *block_def_ptr;		// Pointer to block definition.
	vertex translation;				// Translation of block in world space.
	int vertices;					// Size of vertex list.
	vertex *vertex_list;			// List of all vertices in block.
	int polygons;					// Size of polygon list. 
	polygon *polygon_list;			// List of all polygons in block.
	bool *polygon_active_list;		// Flags showing which polygons are active.
	float sprite_angle;				// Angle of sprite (if applicable).
	int start_time_ms;				// Time block was placed on map.
	int last_time_ms;				// Time of last sprite rotational change.
	int pixmap_index;				// Sprite pixmap index (if applicable).
	bool solid;						// TRUE if block is solid.
	COL_MESH *col_mesh_ptr;			// Pointer to the collision mesh.
	int col_mesh_size;				// Size of collision mesh in bytes.
	block *next_block_ptr;			// Pointer to next block in list.

	block();
	~block();
	bool create_vertex_list(int set_vertices);
	bool create_polygon_active_list(int set_polygons);
};
	
//------------------------------------------------------------------------------
// Square class.
//------------------------------------------------------------------------------

struct square {
	word block_symbol;					// Block symbol.
	block *block_ptr;					// Pointer to block (if any).
	hyperlink *exit_ptr;				// Pointer to exit (if any).
	int popup_trigger_flags;			// Popup trigger flags.
	popup *popup_list;					// List of popups (if any).
	popup *last_popup_ptr;				// Last popup in the list.
	int block_trigger_flags;			// Block trigger flags.
	int square_trigger_flags;			// Square trigger flags.
	trigger *block_trigger_list;		// List of block triggers (if any).
	trigger *square_trigger_list;		// List of square triggers (if any).
	trigger *last_square_trigger_ptr;	// Last square trigger in list.
	int sounds;							// Sounds on this square.

	square();
	~square();
};

//------------------------------------------------------------------------------
// World class.
//------------------------------------------------------------------------------

struct world {
	mapstyle map_style;				// Map style.
	bool ground_level_exists;		// TRUE if ground level exists.
	int columns;					// Number of columns in square map.
	int rows;						// Number of rows in square map.
	int levels;						// Number of levels in square map.
	square *square_map;				// Map of squares.
	float block_scale;				// Block scaling factor.
	float block_units;				// Block units.
	float half_block_units;			// Half block units.
	float audio_scale;				// Audio scale (in metres per unit).
#ifdef SUPPORT_A3D
	int audio_columns;				// Number of columns in audio square map.
	int audio_rows;					// Number of rows in audio square map.
	int audio_levels;				// Number of levels in audio square map.
	int audio_squares;				// Total number of audio squares.
	void **audio_square_map;		// Map of audio squares.
	float audio_block_units;		// Audio block units.
#endif

	world();
	~world();
	void set_map_scale(float map_scale);
	bool create_square_map(void);
	square *get_square_ptr(int column, int row, int level);
	block *get_block_ptr(int column, int row, int level);
	void get_square_location(square *square_ptr, int *column_ptr, int *row_ptr,
		int *level_ptr);
#ifdef SUPPORT_A3D
	bool create_audio_square_map(void);
	void init_audio_square_map(void);
	void clear_audio_square_map(void);
	void **get_audio_square_ptr(int audio_column, int audio_row, 
		int audio_level);
#endif
};

//==============================================================================
// Trigonometry classes.
//==============================================================================

//------------------------------------------------------------------------------
// Sine table class.
//------------------------------------------------------------------------------

struct sine_table {
	float table[361];

	sine_table();
	~sine_table();
	float operator[](float angle);
};

//------------------------------------------------------------------------------
// Cosine table class.
//------------------------------------------------------------------------------

struct cosine_table {
	float table[361];

	cosine_table();
	~cosine_table();
	float operator[](float angle);
};

//==============================================================================
// History classes.
//==============================================================================

//------------------------------------------------------------------------------
// History entry class.
//------------------------------------------------------------------------------

struct history_entry {
	hyperlink link;
	viewpoint player_viewpoint;
};

//------------------------------------------------------------------------------
// History class.
//------------------------------------------------------------------------------

struct history {
	int max_entries;
	int entries;
	history_entry **history_list;
	int curr_entry_index;
	int prev_entry_index;
	string base_spot_URL;

	history();
	~history();
	bool create_history_list(int set_max_entries);
	bool add_entry(const char *text, const char *URL, viewpoint *viewpoint_ptr);
	bool insert_entry(const char *spot_title, const char *spot_URL, 
		const char *spot_entrance);
	history_entry *get_entry(int entry_index);
	history_entry *get_current_entry(void);
	history_entry *get_previous_entry(void);
};
