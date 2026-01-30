#include "syscalls/syscalls.h"
#include "input_keycodes.h"
#include "ui/draw/draw.h"
#include "obj.h"
#include "files/helpers.h"
#include "memory.h"
#include "types/matrix4.h"
#include "types/vector4.h"
#include "math/vector.h"
#include "test.h"
#include "math/aabb2.h"

draw_ctx ctx = {};
vector3 camera_pos = {0,0,15};

matrix4x4 proj_matrix;

void build_proj_matrix(float fov, float aspect, float near, float far){
    float tanfov = 0.726542528f;
    
    proj_matrix = matrix_zero();
    
    proj_matrix.m[0][0] = 1.0f/(tanfov*aspect);
    proj_matrix.m[1][1] = 1.0f/tanfov;
    proj_matrix.m[2][2] = -(far+near)/(far-near);
    proj_matrix.m[3][2] = -1;
    proj_matrix.m[2][3] = -(2*far*near)/(far-near);
    
}

vector4 vert_shader(vector3 pos, vector3 camera){
    vector4 cam_pos = (vector4){pos.x + camera.x, pos.y - camera.y, pos.z - camera.z, 1};
    
    vector4 out_v = matrix4_mul(cam_pos, proj_matrix);
    
    return out_v;
}

argbcolor frag_shader(vector4 frag_coord, int trig_id){
    return (argbcolor){
        .alpha = 0xFF,
        .red = (trig_id) % 255,
        .green = (trig_id + 20) % 255,
        .blue = (trig_id + 50) % 255,
    };
}

static inline bool should_clip(vector4 v){//FIXME
    return  //v.x <= -v.w || v.x >= v.w ||
            // v.y <= -v.w || v.y >= v.w || 
            // v.z <= -v.w || v.z >= v.w ||
            // 0 <= v.w;
            false;
}

static inline float triangle_area(int_point a, int_point b, int_point c){
    return 0.5f*((b.y-a.y)*(b.x+a.x) + (c.y-b.y)*(c.x+b.x) + (a.y-c.y)*(a.x+c.x));
}

void rasterize_triangle(int_point v0, int_point v1, int_point v2, int trig_id, int downscale){
    aabb2 bb = {};
    int min_x = min(v0.x,min(v1.x,v2.x));
    int min_y = min(v0.y,min(v1.y,v2.y));
    int max_x = max(v0.x,max(v1.x,v2.x));
    int max_y = max(v0.y,max(v1.y,v2.y));
    
    min_x = clamp(min_x, 0, ctx.width-1);
    min_y = clamp(min_y, 0, ctx.height-1);
    max_x = clamp(max_x, 0, ctx.width-1);
    max_y = clamp(max_y, 0, ctx.height-1);
    
    float total_area = triangle_area(v0, v1, v2);
    if (total_area < 1) return;
    
    for (float y = min_y; y <= max_y; y += downscale){
        for (float x = min_x; x <= max_x; x += downscale){
            int_point p = (int_point){x,y};
            if (triangle_area(v0, v1, p) < 0) continue;
            if (triangle_area(v1, v2, p) < 0) continue;
            if (triangle_area(v2, v0, p) < 0) continue;
            uint32_t color = frag_shader((vector4){x,y,0,0},trig_id).color;
            for (int yy = 0; yy < downscale && y + yy < ctx.height; yy++)
                for (int xx = 0; xx < downscale  && x + xx < ctx.width; xx++)
                    ctx.fb[((int)(y + yy) * ctx.width) + (int)(x + xx)] = color;
        }
    }
    
    // fb_draw_line(&ctx, bb.min.x, bb.min.y, bb.max.x, bb.min.y, 0xFFFF0000);
    // fb_draw_line(&ctx, bb.min.x, bb.min.y, bb.min.x, bb.max.y, 0xFFFF0000);
    // fb_draw_line(&ctx, bb.min.x, bb.max.y, bb.max.x, bb.max.y, 0xFFFF0000);
    // fb_draw_line(&ctx, bb.max.x, bb.min.y, bb.max.x, bb.max.y, 0xFFFF0000);
    
    // fb_draw_line(&ctx, v0.x, v0.y, v1.x, v1.y, 0xFFB4DD13);
    // fb_draw_line(&ctx, v1.x, v1.y, v2.x, v2.y, 0xFFB4DD13);
    // fb_draw_line(&ctx, v2.x, v2.y, v0.x, v0.y, 0xFFB4DD13);
    
}

void rasterize_quad(vector2 v0, vector2 v1, vector2 v2, vector2 v3){
    //Stub
}

tern draw(int segment_index, size_t num_segs, primitives prim_type, mesh *m, size_t num_verts, vector2 origin, gpu_size screen){
    int i0 = mesh_get_segment(m, segment_index+0);
    int i1 = mesh_get_segment(m, segment_index+1);
    int i2 = prim_type > primitives_line ? mesh_get_segment(m, segment_index+2) : -1;
    int i3 = prim_type > primitives_trig ? mesh_get_segment(m, segment_index+3) : -1;
    assert_true(i0 >= 0 && (uint64_t)i0 < num_verts, "Wrong index %i. Num verts %i",i0,num_verts);
    assert_true(i1 >= 0 && (uint64_t)i1 < num_verts, "Wrong index %i. Num verts %i",i1,num_verts);
    if (i2 != -1) assert_true(i2 >= 0 && (uint64_t)i2 < num_verts, "Wrong index %i. Num verts %i",i2,num_verts);
    if (i3 != -1) assert_true(i3 >= 0 && (uint64_t)i3 < num_verts, "Wrong index %i. Num verts %i",i3,num_verts);
    
    vector4 c0 = vert_shader(mesh_get_vertex(m, i0), camera_pos);
    vector4 c1 = vert_shader(mesh_get_vertex(m, i1), camera_pos);
    vector4 c2 = i2 > -1 ? vert_shader(mesh_get_vertex(m, i2), camera_pos) : (vector4){};
    vector4 c3 = i3 > -1 ? vert_shader(mesh_get_vertex(m, i3), camera_pos) : (vector4){};
    
    if (should_clip(c0) && should_clip(c1)
        && (i2 > -1 ? should_clip(c2) : true)
        && (i3 > -1 ? should_clip(c3) : true)
    ) return -1;
    
    //NDC
    vector3 v0 = {c0.x/c0.w,c0.y/c0.w,c0.z/c0.w};
    vector3 v1 = {c1.x/c1.w,c1.y/c1.w,c1.z/c1.w};
    vector3 v2 = i2 < 0 ? (vector3){} : (vector3){c2.x/c2.w,c2.y/c2.w,c2.z/c2.w};
    vector3 v3 = i3 < 0 ? (vector3){} : (vector3){c3.x/c3.w,c3.y/c3.w,c3.z/c3.w};
    
    vector2 s0 = {(v0.x+1)*0.5f*(screen.width-1),(1-((v0.y+1)*0.5f))*(screen.height-1)};
    vector2 s1 = {(v1.x+1)*0.5f*(screen.width-1),(1-((v1.y+1)*0.5f))*(screen.height-1)};
    
    if (i2 >= 0){
        vector2 s2 = {(v2.x+1)*0.5f*(screen.width-1),(1-((v2.y+1)*0.5f))*(screen.height-1)};
        if (i3 >= 0){
            vector2 s3 = {(v3.x+1)*0.5f*(screen.width-1),(1-((v3.y+1)*0.5f))*(screen.height-1)};
            rasterize_quad(s0,s1,s2,s3);
        } else {
            rasterize_triangle((int_point){(int)s0.x,(int)s0.y},(int_point){(int)s1.x,(int)s1.y},(int_point){(int)s2.x,(int)s2.y},segment_index, 2);
        }
    }
    
    return true;
}

int main(int argc, char* argv[]){
    
    size_t file_size = 0;
    char *file = read_full_file("/resources/Windmill.obj",&file_size);
    
    primitives prim_type = primitives_trig;
    mesh m = parse_obj(file, file_size, prim_type);
    
    ctx.width = 1920;
    ctx.height = 1080;
    request_draw_ctx(&ctx);
    vector2 mid = {ctx.width/2.f,ctx.height/2.f};
    
    if (!prim_type){
        print("No primitive type specified");
        return -1;
    }
    
    size_t num_segments = mesh_num_segments(&m);
    size_t num_verts = mesh_num_verts(&m);
    
    assert_true(num_segments && num_verts, "Empty mesh");
    
    char buf[16];
    
    build_proj_matrix(72, (float)ctx.width/(float)ctx.height, 0.1f, 100);
    
    float last_time = get_time()/1000.f;
    while (!should_close_ctx()){
        begin_drawing(&ctx);
            
        float time = get_time()/1000.f;
        float dt = time-last_time;
        last_time = time;
        fb_clear(&ctx, 0);
        
        for (size_t i = 0; i < num_segments; i++){
            if (draw((i * prim_type), num_segments * prim_type, prim_type, &m, num_verts, mid, (gpu_size){ctx.width,ctx.height}) == -1) return -1;
        }
        
        string_format_buf(buf,16,"%i FPS",(int)(1/dt));
        fb_draw_string(&ctx, buf, 0,0, 2, 0xFFFFFFFF);
        commit_draw_ctx(&ctx);

        kbd_event ev = {};
        read_event(&ev);
        if (ev.key == KEY_ESC){
            destroy_draw_ctx(&ctx);
            return 0;
        } 
        mouse_data mouse = {};
        get_mouse_status(&mouse);
        float adj_dt = minf(0.03f,dt);
        if (mouse_button_down(&mouse, 1)){
            camera_pos.x += mouse.raw.x * adj_dt;
            camera_pos.y += mouse.raw.y * adj_dt;
        }
        camera_pos.z -= mouse.raw.scroll * 2500 * adj_dt;
        memset(buf, 0, 16);
        
    }
    destroy_draw_ctx(&ctx);
    return 0;
}