#include "obj.h"
#include "data/format/scanner/scanner.h"

void handle_trig(mesh_t *mesh, string_slice sl){
    char *s1 = (char*)seek_to(sl.data, '/');
    int segment = (parse_int64(sl.data,s1-sl.data-1) & 0xFFFFFFFF) - 1;
    chunk_array_push(mesh->segments, &segment);
    //TODO: extra trig data
}

void handle_obj_line(void *ctx, string_slice line){
    mesh_t *mesh = (mesh_t *)ctx;
    Scanner s = scanner_make(line.data, line.length);
    char first = scan_next(&s);
    if (first == 'v'){
        if (scan_next(&s) != ' ') return;//TODO: other options
        vector3 vector = {};
        string_slice v1 = scan_to(&s, ' ');
        if (!v1.length) return;
        vector.x = parse_float(v1.data, v1.length-1);
        string_slice v2 = scan_to(&s, ' ');
        if (!v2.length) return;
        vector.y = parse_float(v2.data, v2.length-1);
        string_slice v3 = scan_to(&s, ' ');
        if (!v3.length) return;
        vector.z = parse_float(v3.data, v3.length);
        chunk_array_push(mesh->vertices, &vector);
    }
    if (first == 'f'){
        if (scan_next(&s) != ' ') return;
        do {
            string_slice sl = scan_to(&s, ' ');
            if (sl.length == 0) break;
            handle_trig(mesh, sl);
        } while (!scan_eof(&s));//TODO: mesh triangulation. Should be enough to do first point + latest point + current point for each > 3
    }
}

void read_lines(char *file, void *ctx, void (*handle_line)(void *ctx, string_slice line)){
    char *point = file;
    do {
        char *new_point = (char*)seek_to(point, '\n');
        if (new_point == point) break;
        int red = 1;
        if (*(new_point-1) == '\r') red++;
        handle_line(ctx, make_string_slice(point, 0, new_point-point-red));
        point = new_point;
    } while(point);
    
}

mesh_t parse_obj(void* obj, size_t size, primitives prim_type){
    mesh_t mesh = {};
    mesh.vertices = chunk_array_create(sizeof(vector3), 1000);
    mesh.segments = chunk_array_create(sizeof(int), 1000);
    mesh.primitive_type = prim_type;
    read_lines(obj, &mesh, handle_obj_line);
    return mesh;
}