#include "mesh.h"





char* primitives_to_string(primitives val){
	switch (val) {
		case primitives_none: return "none";
		case primitives_pixel: return "pixel";
		case primitives_line: return "line";
		case primitives_trig: return "trig";
		case primitives_quad: return "quad";
	} 
} 








size_t mesh_num_verts(mesh *instance){
	return chunk_array_count(instance->vertices);
}
size_t mesh_num_segments(mesh *instance){
	if (!instance->primitive_type){
		return 0;
	}
	
	size_t size = chunk_array_count(instance->segments);
	if (size % instance->primitive_type == 0){
		return size / instance->primitive_type;
	}
	
	return 0;
}
int mesh_get_segment(mesh *instance, size_t index){
	return *(int*)chunk_array_get(instance->segments, index);
}
vector3 mesh_get_vertex(mesh *instance, size_t index){
	return *(vector3*)chunk_array_get(instance->vertices, index);
}
