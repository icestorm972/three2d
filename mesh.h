#include "data/struct/chunk_array.h"

#include "types/vector3.h"

typedef enum {
	primitives_none,
	primitives_pixel,
	primitives_line,
	primitives_trig,
	primitives_quad,
} primitives;

char* primitives_to_string(primitives val);

typedef struct mesh { chunk_array_t* vertices;
	chunk_array_t* segments;
	primitives primitive_type;
	
	
	
	
 } mesh;
size_t mesh_num_verts(mesh *instance);size_t mesh_num_segments(mesh *instance);int mesh_get_segment(mesh *instance, size_t index);vector3 mesh_get_vertex(mesh *instance, size_t index);