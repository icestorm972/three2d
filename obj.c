#include "obj.h"
#include "data/format/scanner/scanner.h"

extern vector3 v[4096];
extern int s[10000];

extern int v_count, s_count;

void handle_trig(string_slice sl){
    char *s1 = (char*)seek_to(sl.data, '/');
    s[s_count++] = (int)(parse_int64(sl.data,s1-sl.data-1) & 0xFFFFFFFF) - 1;
    //TODO: extra trig data
}

void handle_obj_line(string_slice line){
    Scanner s = scanner_make(line.data, line.length);
    char first = scan_next(&s);
    if (first == 'v'){
        if (scan_next(&s) != ' ') return;//TODO: other options
        string_slice v1 = scan_to(&s, ' ');
        if (!v1.length) return;
        v[v_count].x = parse_float(v1.data, v1.length);
        string_slice v2 = scan_to(&s, ' ');
        if (!v2.length) return;
        v[v_count].y = parse_float(v2.data, v2.length);
        string_slice v3 = scan_to(&s, ' ');
        if (!v3.length) return;
        v[v_count++].z = parse_float(v3.data, v3.length);
    }
    if (first == 'f'){
        if (scan_next(&s) != ' ') return;
        do {
            string_slice sl = scan_to(&s, ' ');
            if (sl.length == 0) break;
            handle_trig(sl);
        } while (!scan_eof(&s));//TODO: mesh triangulation?
    }
}