#include "syscalls/syscalls.h"
#include "input_keycodes.h"
#include "ui/draw/draw.h"
#include "math/vector.h"
#include "data/struct/chunk_array.h"
#include "obj.h"
#include "files/helpers.h"
#include "memory.h"

draw_ctx ctx = {};
float depth = 2;

tern draw_segment(int i0, int i1, mesh_t *m, size_t num_verts, vector2 origin){
    if (i0 < 0 || (uint64_t)i0 >= num_verts){
        print("Wrong index %i. %i vertices",i0,num_verts);
        return -1;
    }
    if (i0 < 0 || (uint64_t)i1 >= num_verts){
        print("Wrong index %i. %i vertices",i1,num_verts);
        return -1;
    }
    vector3 v0 = mesh_get_vertices(m, i0);
    vector3 v1 = mesh_get_vertices(m, i1);
    
    v0.z += depth;
    v1.z += depth;
    
    if (v0.z < 0.1 && v1.z < 0.1) return false;
    
    float x0 = (v0.x*100/maxf(0.1f,v0.z))+origin.x;
    float x1 = (v1.x*100/maxf(0.1f,v1.z))+origin.x;
    float y0 = (-v0.y*100/maxf(0.1f,v0.z))+origin.y;
    float y1 = (-v1.y*100/maxf(0.1f,v1.z))+origin.y;
    
    fb_draw_line(&ctx, x0, y0, x1, y1, 0xFFB4DD13);
    
    return true;
}

int main(int argc, char* argv[]){
    
    size_t file_size = 0;
    char *file = read_full_file("/resources/Windmill.obj",&file_size);
    
    primitives prim_type = prim_trig;
    mesh_t mesh = parse_obj(file, file_size, prim_type);
    
    ctx.width = 1920;
    ctx.height = 1080;
    request_draw_ctx(&ctx);
    vector2 mid = {ctx.width/2.f,ctx.height/2.f};
    
    if (!prim_type){
        print("No primitive type specified");
        return -1;
    }
    
    size_t num_segments = mesh_num_segments(&mesh);
    size_t num_verts = mesh_num_verts(&mesh);
    
    char buf[16];
    
    float last_time = get_time()/1000.f;
    while (!should_close_ctx()){
        begin_drawing(&ctx);
            
        float time = get_time()/1000.f;
        float dt = time-last_time;
        last_time = time;
        fb_clear(&ctx, 0);
        
        for (size_t i = 0; i < num_segments; i++){
            for (int j = 0; j < (int)prim_type; j++){
                int i0 = mesh_get_segment(&mesh, (i * prim_type)+((j)%prim_type));
                int i1 = mesh_get_segment(&mesh, (i * prim_type)+((j+1)%prim_type));
                if (draw_segment(i0, i1, &mesh, num_verts, mid) == -1) return -1;
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
            mid.x += mouse.raw.x * 25 * dt;
            mid.y += mouse.raw.y * 25 * dt;
        }
        depth -= mouse.raw.scroll * 5 * dt;
        memset(buf, 0, 16);
        string_format_buf(buf,16,"%i FPS",(int)(1/dt));
        fb_draw_string(&ctx, buf, 0,0, 2, 0xFFFFFFFF);
    }
    destroy_draw_ctx(&ctx);
    return 0;
}