/*
	===============================================================

	SHADERMAP GEO PLUGIN CORE SOURCE FILE

	ShaderMap Geometry Plugins are used to import 3D Models into
	ShaderMap. ShaderMap may request data in one of two formats:
	RENDER or NODE. RENDER geometry is used in the Material
	Visualizer and NODE geometry is used as source and cage	models 
	in the ShaderMap Project Grid.

	Include this source code file in a Win32 DLL project. 
	#include "geo_plugin_core.cpp". You should ensure that the 
	DLL extension is *.smg. It must be placed in the 
	"plugins/geometry" folder found in the ShaderMap root 
	directory.

	There are examples included with this source code that should
	help you get started.


	SHADERMAP SDK LICENSE

	The ShaderMap SDK is released under The MIT License (MIT)
	http://opensource.org/licenses/MIT

	Copyright (c) 2015 Rendering Systems Inc.
	
	Permission is hereby granted, free of charge, to any person 
	obtaining a copy of this software and associated documentation 
	files (the "Software"), to deal	in the Software without 
	restriction, including without limitation the rights to use, 
	copy, modify, merge, publish, distribute, sublicense, and/or 
	sell copies of the Software, and to permit persons to whom the 
	Software is	furnished to do so, subject to the following 
	conditions:

	The above copyright notice and this permission notice shall be 
	included in	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
	OF MERCHANTABILITY,	FITNESS FOR A PARTICULAR PURPOSE AND 
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
	OTHER DEALINGS IN THE SOFTWARE.
	
	Developed by: Neil Kemp at Rendering Systems Inc.
	
	Online:		http://shadermap.com
	Corporate:	http://renderingsystems.com

	===============================================================
*/


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// General includes

#include <stdio.h>
#include <tchar.h>
#include "windows.h"


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin required defines

// Geometry types - This is the format that ShaderMap will request the geometry data be structured in.
// For more information see the comments for the functions "gp_create_render_geometry" and "gp_create_node_geometry()". 
#define GP_GEOMETRY_TYPE_RENDER					0		
#define GP_GEOMETRY_TYPE_NODE					1

// Error define - a simple way to log errors to the ShaderMap log files located in: "C:\Users\<USERNAME>\AppData\Roaming\SM3\log"
// Example: LOG_ERROR_MSG(plugin_index, _T("Error description");
#define											LOG_ERROR_MSG(plugin_index, error) gp_log_plugin_error(plugin_index, error, _T(__FUNCTION__), _T(__FILE__), __LINE__);


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Plugin required structs

// Geometry RENDER vertex
// This is the format that each vertex must take when importing geometry data of type GP_GEOMETRY_TYPE_RENDER
struct gp_render_vertex_s
{
	float										x, y, z;				// Position
	float										nx, ny, nz;				// Normal
	float										u, v;					// Texture coordinates


	// c()
	gp_render_vertex_s::gp_render_vertex_s(void)
	{	x = y = z = nx = ny = nz = u = v = 0.0f;
	}
	// c(...)
	gp_render_vertex_s::gp_render_vertex_s(float c_x, float c_y, float c_z, float c_nx, float c_ny, float c_nz, float c_u, float c_v)
	{	x = c_x; y = c_y; z = c_z; nx = c_nx; ny = c_ny; nz = c_nz; u = c_u; v = c_v;
	}
};

// Geometry RENDER face
// This is the format that each face (triangle) must take when importing geometry data of type GP_GEOMETRY_TYPE_RENDER
struct gp_render_face_s
{
	unsigned int								a, b, c;				// Indices of triangle vertices
	unsigned int								subset_index;			// The zero based index of this triangle's subset


	// c()
	gp_render_face_s::gp_render_face_s(void)
	{	a = b = c = subset_index = 0;
	}

	// c(...)
	gp_render_face_s::gp_render_face_s(unsigned int c_a, unsigned int c_b, unsigned int c_c, unsigned int subset)
	{	a = c_a; b = c_b; c = c_c; subset_index = subset;
	}
};

// Geometry NODE vertex
// This is the format that each vertex must take when importing geometry data of type GP_GEOMETRY_TYPE_NODE
struct gp_node_vertex_s
{
	float										x, y, z;				// Position
	float										nx, ny, nz;				// Normal


	// c()
	gp_node_vertex_s::gp_node_vertex_s(void)
	{	x = y = z = nx = ny = nz = 0.0f;
	}

	// c(...)
	gp_node_vertex_s::gp_node_vertex_s(float c_x, float c_y, float c_z, float c_nx, float c_ny, float c_nz)
	{	x = c_x; y = c_y; z = c_z; nx = c_nx; ny = c_ny; nz = c_nz;
	}
};

// Geometry NODE vertex
// This is the format that each UV entry must take when importing geometry data of type GP_GEOMETRY_TYPE_NODE
struct gp_node_uv_s
{
	float										u, v;					// Texture coordinates		


	// c()
	gp_node_uv_s::gp_node_uv_s(void)
	{	u = v = 0.0f;
	}

	// c(...)
	gp_node_uv_s::gp_node_uv_s(float c_u, float c_v)
	{	u = c_u; v = c_v;
	}
};

// Geometry node face
// This is the format that each face (triangle) must take when importing geometry data of type GP_GEOMETRY_TYPE_RENDER
struct gp_node_face_s
{
	unsigned int								a, b, c;				// Indices of triangle vertices
	unsigned int								uv_a, uv_b, uv_c;		// Indices of triangle uv coordinates
	unsigned int								subset_index;			// The zero based index of this triangle's subset


	// c()
	gp_node_face_s::gp_node_face_s(void)
	{	a = b = c = uv_a = uv_b = uv_c = subset_index = 0;
	}

	// c(...)
	gp_node_face_s::gp_node_face_s(unsigned int c_a, unsigned int c_b, unsigned int c_c, unsigned int c_uv_a, unsigned int c_uv_b, unsigned int c_uv_c, unsigned int subset)
	{	a = c_a; b = c_b; c = c_c; uv_a = c_uv_a; uv_b = c_uv_b; uv_c = c_uv_c; subset_index = subset;
	}
};


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Definition of function types and pointers to functions

// **
// Setup and info functions

// Functions used in "on_initialize()" to start / stop initialization.
// These two functions should be called at start and end of "on_initialize()"
typedef void									(*gp_begin_initialize_type)(void);
gp_begin_initialize_type						gp_begin_initialize = 0;
typedef void									(*gp_end_initialize_type)(void);
gp_end_initialize_type							gp_end_initialize = 0;
			
// Set the file format info by passing the name of the file format (Example: "Wavefront OBJ"), a list of extensions (Ex. "obj").
// This should be called in "on_initialize()" between "gp_begin_initialize()" and "gp_end_initialize()"
typedef void									(*gp_set_file_info_type)(const wchar_t* /*name*/, const wchar_t** /*extension_array*/, unsigned int /*extension_count*/);
static gp_set_file_info_type					gp_set_file_info = 0;


// **
// Functions used in "on_process()"	to get the expected geometry format and to pass the final geometry to ShaderMap.		

// Get the geometry type expected from ShaderMap. This can be of type RENDER or type NODE. 
// It is important that the correct format is used or the geometry will not work as expected.
typedef unsigned int							(*gp_get_geometry_type_type)(void);
static gp_get_geometry_type_type				gp_get_geometry_type = 0;

// Create the loaded geometry for RENDERING by sending it to ShaderMap - Ensure "gp_get_geo_type()" == GP_GEOMETRY_TYPE_RENDER
// The triangle_array parameter must be sorted from lowest to highest subsets
typedef BOOL									(*gp_create_render_geometry_type)(gp_render_vertex_s* /*vertex_array*/, unsigned int /*vertex_count*/, gp_render_face_s* /*triangle_array*/, unsigned int /*triangle_count*/, unsigned int /*subset_count*/, BOOL /*is_create_normals*/);
static gp_create_render_geometry_type			gp_create_render_geometry = 0;

// Create the loaded geometry for NODES by sending it to ShaderMap - Ensure "gp_get_geo_type()" == GP_GEOMETRY_TYPE_NODE
// The triangle_array parameter must be sorted from lowest to highest subsets - Geometry should be optimized with no duplicate vertices or UV coordinates
typedef BOOL									(*gp_create_node_geometry_type)(gp_node_vertex_s* /*vertex_array*/, unsigned int /*vertex_count*/, gp_node_uv_s* /*uv_array*/, unsigned int /*uv_count*/, gp_node_face_s* /*triangle_array*/, unsigned int /*triangle_count*/, unsigned int /*subset_count*/, BOOL /*is_create_normals*/);
static gp_create_node_geometry_type				gp_create_node_geometry = 0;


// **
// Utility functions using during processing in "on_process()".

// Log a filter error to the ShaderMap log file located: "C:\Users\<USERNAME>\AppData\Roaming\SM3\log".
// Use the "LOG_ERROR_MSG()" macro to simplify calling this function.
typedef void									(*gp_log_plugin_error_type)(unsigned int /*plugin_index*/, const wchar_t* /*error_message*/, const wchar_t* /*function*/, const wchar_t* /*source_filepath*/, int /*source_line_number*/);
static gp_log_plugin_error_type					gp_log_plugin_error = 0;


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Local function prototypes. These must be implemented by the plugin DLL. They are called by 
// the exported functions defined at the bottom of this source code page.

// This function is called when ShaderMap is attaching to the plugin.
// "gp_begin_initialize()" and "gp_end_initialize()" are called inside of "on_initialize()".
// In between those functions, the "gp_set_file_info_type()" should be called.
BOOL											on_initialize(void);

// Called when a plugin is to be processed (the filter is applied to the base image).
// The plugin_index is passed and is required by certain functions such as "gp_log_plugin_error()".
// file_path will contain the full file path to the geometry file the plugin must load and put into the geometry arrays required by ShaderMap.
BOOL											on_process(unsigned int plugin_index, const wchar_t* file_path);

// Called before the plugin is released from ShaderMap at application shutdown. 
// Use this function to release/free any allocated resources.
BOOL											on_shutdown(void);


// ----------------------------------------------------------------
// ----------------------------------------------------------------
// Definition of functions required by the plugin system. 

// These are functions called by ShaderMap. 
// "plugin_initialize()" sets the API function pointers then calls "on_initialize()".
// "plugin_process()" and "plugin_shutdown()" each call the user defined "on_process()" and "on_shutdown()" functions.

#define DLL_EXPORT								__declspec(dllexport)

extern "C" {

	// Exported functions

	// Initialize the plugin with function pointers.
	DLL_EXPORT BOOL plugin_initialize(void** function_pointer_array)
	{	
		// Store function pointers - main app must know order and types.

		gp_begin_initialize						= (gp_begin_initialize_type)function_pointer_array[0];
		gp_end_initialize						= (gp_end_initialize_type)function_pointer_array[1];
		/*Elements 2 - 99 are reserved for future use*/	

		gp_set_file_info						= (gp_set_file_info_type)function_pointer_array[100];
		/*Elements 101 - 199 are reserved for future use*/

		gp_get_geometry_type					= (gp_get_geometry_type_type)function_pointer_array[200];
		gp_create_render_geometry				= (gp_create_render_geometry_type)function_pointer_array[201];	
		gp_create_node_geometry					= (gp_create_node_geometry_type)function_pointer_array[202];
		gp_log_plugin_error						= (gp_log_plugin_error_type)function_pointer_array[203];	
		/*Elements 204 - 299 are reserved for future use*/

		return on_initialize();
	}

	// Process the plugin - param_0 (plugin_index), param_1 (file_path)
	DLL_EXPORT BOOL plugin_process(void* param_0, void* param_1)
	{	return on_process(*(unsigned int*)param_0, (wchar_t*)param_1);
	}

	// Shutdown the plugin
	DLL_EXPORT BOOL plugin_shutdown(void)
	{	return on_shutdown();
	}
}
