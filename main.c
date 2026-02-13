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
#include "memory/memory.h"

draw_ctx ctx = {};
vector3 camera_pos = {0,0,15};

matrix4x4 proj_matrix;

u8 *z_buffer;
size_t z_buf_size = 0;

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
        .red = ((trig_id) % 225) + 30,
        .green = ((trig_id + 30) % 225) + 30,
        .blue = ((trig_id + 50) % 225) + 30,
    };
}

static inline bool should_clip(vector4 v){//FIXME
    return  v.x <= -v.w || v.x >= v.w ||
             v.y <= -v.w || v.y >= v.w || 
             v.z <= -v.w || v.z >= v.w ||
             0 <= v.w;
            false;
}

static inline float triangle_area(vector3 a, vector3 b, vector3 c){
    return 0.5f*((b.y-a.y)*(b.x+a.x) + (c.y-b.y)*(c.x+b.x) + (a.y-c.y)*(a.x+c.x));
}

void rasterize_triangle(vector3 v0, vector3 v1, vector3 v2, int trig_id, int downscale){
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
            vector3 p = (vector3){x,y,0};
            float alpha = triangle_area(p, v1, v2);
            if (alpha < 0) continue;
            float beta = triangle_area(p, v2, v0);
            if (beta < 0) continue;
            float gamma = triangle_area(p, v0, v1);
            if (gamma < 0) continue;
            
            float depth = (alpha * v0.z + beta * v1.z + gamma * v2.z)/total_area;
            u8 depth_color = (u8)255-(128+((depth-4.5f)*100));
            
            u8 current_z = z_buffer[((int)y * ctx.width) + (int)x];
            if (depth_color > current_z){
                z_buffer[((int)y * ctx.width) + (int)x] = depth_color;
            } else continue;
            
            uint32_t color = (0xFF << 24) | (depth_color << 16) | (depth_color << 8) | depth_color;
            for (int yy = 0; yy < downscale && y + yy < ctx.height; yy++)
                for (int xx = 0; xx < downscale  && x + xx < ctx.width; xx++)
                    ctx.fb[((int)(y + yy) * ctx.width) + (int)(x + xx)] = color;
        }
    }
    
    // fb_draw_line(&ctx, min_x, min_y, max_x, min_y, 0xFFFF0000);
    // fb_draw_line(&ctx, min_x, min_y, min_x, max_y, 0xFFFF0000);
    // fb_draw_line(&ctx, min_x, max_y, max_x, max_y, 0xFFFF0000);
    // fb_draw_line(&ctx, max_x, min_y, max_x, max_y, 0xFFFF0000);
    
    // fb_draw_line(&ctx, v0.x, v0.y, v1.x, v1.y, 0xFFB4DD13);
    // fb_draw_line(&ctx, v1.x, v1.y, v2.x, v2.y, 0xFFB4DD13);
    // fb_draw_line(&ctx, v2.x, v2.y, v0.x, v0.y, 0xFFB4DD13);
    
}

void rasterize_quad(vector3 v0, vector3 v1, vector3 v2, vector3 v3){
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
    
    // if (should_clip(c0) && should_clip(c1)
        // && (i2 > -1 ? should_clip(c2) : true)
        // && (i3 > -1 ? should_clip(c3) : true)
    // ) return false;
    
    //NDC
    vector3 v0 = {c0.x/c0.w,c0.y/c0.w,c0.z/c0.w};
    vector3 v1 = {c1.x/c1.w,c1.y/c1.w,c1.z/c1.w};
    vector3 v2 = i2 < 0 ? (vector3){} : (vector3){c2.x/c2.w,c2.y/c2.w,c2.z/c2.w};
    vector3 v3 = i3 < 0 ? (vector3){} : (vector3){c3.x/c3.w,c3.y/c3.w,c3.z/c3.w};
    
    vector3 s0 = {(v0.x+1)*0.5f*(screen.width-1),(1-((v0.y+1)*0.5f))*(screen.height-1),v0.z};
    vector3 s1 = {(v1.x+1)*0.5f*(screen.width-1),(1-((v1.y+1)*0.5f))*(screen.height-1),v1.z};
    
    float min_depth = min(c0.w,c1.w);
    
    if (i2 >= 0){
        vector3 s2 = {(v2.x+1)*0.5f*(screen.width-1),(1-((v2.y+1)*0.5f))*(screen.height-1),v2.z};
        min_depth = min(min_depth,c2.w);
        if (i3 >= 0){
            min_depth = min(min_depth,c3.w);
            vector3 s3 = {(v3.x+1)*0.5f*(screen.width-1),(1-((v3.y+1)*0.5f))*(screen.height-1),v3.z};
            rasterize_quad(s0,s1,s2,s3);
        } else {
            rasterize_triangle(s0,s1,s2,segment_index, 1 + floor(min_depth/50));
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
    
    z_buf_size = ctx.width * ctx.height;
    z_buffer = zalloc(z_buf_size);
    
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
    
    u64 last_time = get_time();
    u64 sample = 0;
    while (!should_close_ctx()){
        begin_drawing(&ctx);
            
        u64 time = get_time();
        double dt = ((double)time-last_time)/1000;
        last_time = time;
        fb_clear(&ctx, 0);
        
        memset(z_buffer, 0, z_buf_size);
        
        for (size_t i = 0; i < num_segments; i++){
            if (draw((i * prim_type), num_segments * prim_type, prim_type, &m, num_verts, mid, (gpu_size){ctx.width,ctx.height}) == -1) return -1;
        }
        
        if (sample % 10 == 0){
            memset(buf, 0, 16);
            string_format_buf(buf,16,"%i FPS",(int)(1/dt));
        }
        fb_draw_string(&ctx, buf, 0,0, 2, 0xFFFFFFFF);
        sample++;
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
        camera_pos.z -= mouse.raw.scroll * 250 * adj_dt;
        camera_pos.z = maxf(0,camera_pos.z);
        
    }
    destroy_draw_ctx(&ctx);
    return 0;
}
