#include "mesh.h"

size_t mesh_num_verts(mesh_t *m){
    return chunk_array_count(m->vertices);
}
    
size_t mesh_num_segments(mesh_t *m){
    if (!m->primitive_type) return 0;
    size_t size = chunk_array_count(m->segments);
    return size % m->primitive_type == 0 ? size/m->primitive_type : 0;
}

int mesh_get_segment(mesh_t *m, size_t index){
    return *(int*)chunk_array_get(m->segments, index);
}

vector3 mesh_get_vertices(mesh_t *m, size_t index){
    return *(vector3*)chunk_array_get(m->vertices, index);
}
