#include "syscalls/syscalls.h"
#include "input_keycodes.h"
#include "ui/draw/draw.h"
#include "math/vector.h"
#include "obj.h"
#include "files/helpers.h"
#include "memory.h"

draw_ctx ctx = {};
vector3 camera_pos = {0,0,2};

typedef struct {
    float x;
    float y;
    float z;
    float w;
} vector4;

typedef struct {
    float m[4][4];//Row,Column
} matrix4x4;
matrix4x4 proj_matrix;

matrix4x4 matrix_zero(){
    return (matrix4x4){
        .m = {
            { 0.0, 0.0, 0.0, 0.0},
            { 0.0, 0.0, 0.0, 0.0},
            { 0.0, 0.0, 0.0, 0.0},
            { 0.0, 0.0, 0.0, 0.0}, 
        }
    };
}

void build_proj_matrix(float fov, float aspect, float near, float far){
    float tanfov = 0.726542528f;
    
    proj_matrix = matrix_zero();
    
    proj_matrix.m[0][0] = 1.0f/(tanfov*aspect);
    proj_matrix.m[1][1] = 1.0f/tanfov;
    proj_matrix.m[2][2] = -(far+near)/(far-near);
    proj_matrix.m[3][2] = -1;
    proj_matrix.m[2][3] = -(2*far*near)/(far-near);
    
}

vector4 matrix4_mul(vector4 v, matrix4x4 m){
    vector4 out = {};
    
    out.x = v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + v.w*m.m[3][0];
    out.y = v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + v.w*m.m[3][1];
    out.z = v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + v.w*m.m[3][2];
    out.w = v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + v.w*m.m[3][3];
    
    return out;
}

vector4 vert_shader(vector3 pos, vector3 camera){
    vector4 cam_pos = (vector4){pos.x + camera.x, pos.y - camera.y, pos.z - camera.z, 1};
    
    vector4 out_v = matrix4_mul(cam_pos, proj_matrix);
    
    return out_v;
}

static inline bool should_clip(vector4 v){
    return  //v.x <= -v.w || v.x >= v.w ||
            // v.y <= -v.w || v.y >= v.w || 
            // v.z <= -v.w || v.z >= v.w ||
            // 0 <= v.w;
            false;
}

tern draw_segment(int i0, int i1, mesh_t *m, size_t num_verts, vector2 origin){
    if (i0 < 0 || (uint64_t)i0 >= num_verts){
        print("Wrong index %i. %i vertices",i0,num_verts);
        return -1;
    }
    if (i0 < 0 || (uint64_t)i1 >= num_verts){
        print("Wrong index %i. %i vertices",i1,num_verts);
        return -1;
    }
    
    vector4 c0 = vert_shader(mesh_get_vertices(m, i0), camera_pos);
    vector4 c1 = vert_shader(mesh_get_vertices(m, i1), camera_pos);
    
    
    if (should_clip(c0) && should_clip(c1)) return false;
    
    //NDC
    vector3 v0 = {c0.x/c0.w,c0.y/c0.w,c0.z/c0.w};
    vector3 v1 = {c1.x/c1.w,c1.y/c1.w,c1.z/c1.w};
    
    //Basic rasterization
    
    float x0 = (v0.x+1)*0.5f*(1920-1);
    float x1 = (v1.x+1)*0.5f*(1920-1);
    
    float y0 = (1-((v0.y+1)*0.5f))*(1080-1);
    float y1 = (1-((v1.y+1)*0.5f))*(1080-1);
    
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
    
    build_proj_matrix(72, (float)ctx.width/(float)ctx.height, 0.1f, 500);
    
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
            camera_pos.x += mouse.raw.x * dt;
            camera_pos.y += mouse.raw.y * dt;
        }
        camera_pos.z -= mouse.raw.scroll * 25 * dt;
        memset(buf, 0, 16);
        string_format_buf(buf,16,"%i FPS",(int)(1/dt));
        fb_draw_string(&ctx, buf, 0,0, 2, 0xFFFFFFFF);
    }
    destroy_draw_ctx(&ctx);
    return 0;
}