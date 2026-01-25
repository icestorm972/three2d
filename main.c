#include "syscalls/syscalls.h"
#include "input_keycodes.h"
#include "ui/draw/draw.h"
#include "math/vector.h"
#include "std/string_slice.h"
#include "data/struct/chunked_list.h"
#include "data/format/scanner/scanner.h"
#include "obj.h"
#include "files/helpers.h"
#include "memory.h"

draw_ctx ctx = {};

tern draw_segment(int i0, int i1, vector3* v, int num_verts, vector2 origin){
    if (i0 >= num_verts){
        print("Wrong index %i. %i vertices",i0,num_verts);
        return -1;
    }
    if (i1 >= num_verts){
        print("Wrong index %i. %i vertices",i1,num_verts);
        return -1;
    }
    vector3 v0 = v[i0];
    vector3 v1 = v[i1];
    
    if (v0.z < 0.1 && v1.z < 0.1) return false;
    
    float x0 = (v0.x*100/maxf(0.1f,v0.z+5))+origin.x;
    float x1 = (v1.x*100/maxf(0.1f,v1.z+5))+origin.x;
    float y0 = (-v0.y*100/maxf(0.1f,v0.z+5))+origin.y;
    float y1 = (-v1.y*100/maxf(0.1f,v1.z+5))+origin.y;
    
    fb_draw_line(&ctx, x0, y0, x1, y1, 0xFFB4DD13);
    
    return true;
}

void read_lines(char *file, void (*handle_line)(string_slice line)){
    char *point = file;
    do {
        char *new_point = (char*)seek_to(point, '\n');
        if (new_point == point) break;
        handle_line(make_string_slice(point, 0, new_point-point-2));
        point = new_point;
    } while(point);
    
}

vector3 v[4096];
int s[10000];

int v_count, s_count;

int main(int argc, char* argv[]){
    
    char *file = read_full_file("/resources/Windmill.obj",0);
    
    read_lines(file, handle_obj_line);
    
    ctx.width = 1920;
    ctx.height = 1080;
    request_draw_ctx(&ctx);
    vector2 mid = {ctx.width/2.f,ctx.height/2.f};
    primitives prim_type = prim_trig;
    
    if (!prim_type){
        print("No primitive type specified");
        return -1;
    }
    
    if (s_count % prim_type != 0){
        print("Wrong number of segments, found %i, must be a multiple of %i",s_count, prim_type);
        return -1;
    }
    int num_segments = s_count/prim_type;
    int num_verts = v_count;
    
    char buf[16];
    
    float last_time = get_time()/1000.f;
    while (!should_close_ctx()){
        begin_drawing(&ctx);
            
        float time = get_time()/1000.f;
        float dt = time-last_time;
        last_time = time;
        fb_clear(&ctx, 0);
        
        for (int i = 0; i < num_segments; i++){
            for (int j = 0; j < (int)prim_type; j++){
                int i0 = s[(i * prim_type)+((j)%prim_type)];
                int i1 = s[(i * prim_type)+((j+1)%prim_type)];
                if (draw_segment(i0, i1, v, num_verts, mid) == -1) return -1;
            }
        }
        commit_draw_ctx(&ctx);

        kbd_event ev = {};
        read_event(&ev);
        if (ev.key == KEY_ESC){
            destroy_draw_ctx(&ctx);
            return 0;
        } 
        mouse_data mouse = {};
        get_mouse_status(&mouse);
        if (mouse_button_down(&mouse, 1)){
            mid.x += mouse.raw.x*dt;
            mid.y += mouse.raw.y*dt;
        }
        memset(buf, 0, 16);
        string_format_buf(buf,16,"%i FPS",(int)(1/dt));
        fb_draw_string(&ctx, buf, 0,0, 2, 0xFFFFFFFF);
    }
    destroy_draw_ctx(&ctx);
    return 0;
}