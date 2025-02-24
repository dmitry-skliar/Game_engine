// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "math/kmath.h"
#include "resources/resource_types.h"
#include "systems/material_system.h"

typedef struct renderer_system_state {
    renderer_backend backend;
    mat4 projection;
    mat4 view;
    mat4 ui_projection;
    mat4 ui_view;
    f32 fov_radians;
    f32 near_clip;
    f32 far_clip;
    f32 ui_near_clip;
    f32 ui_far_clip;
    // Material шейдер.
    u32 material_shader_id;
    u32 material_shader_projection_location;
    u32 material_shader_view_location;
    u32 material_shader_diffuse_color_location;
    u32 material_shader_diffuse_texture_location;
    u32 material_shader_model_location;
    // UI шейдер.
    u32 ui_shader_id;
    u32 ui_shader_projection_location;
    u32 ui_shader_view_location;
    u32 ui_shader_diffuse_color_location;
    u32 ui_shader_diffuse_texture_location;
    u32 ui_shader_model_location;
} renderer_system_state;

static renderer_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the renderer system to be initialized. Call 'renderer_system_initialize' first.";

#define CRITICAL_INIT(op, msg) \
    if(!op)                    \
    {                          \
        kerror(msg);           \
        return false;          \
    }

bool renderer_system_initialize(u64* memory_requirement, void* memory, window* window_state)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once.", __FUNCTION__);
        return false;
    }

    *memory_requirement = sizeof(struct renderer_system_state);

    if(!memory)
    {
        return true;
    }

    kzero(memory, *memory_requirement);
    state_ptr = memory;

    // Инициализация.
    // TODO: Сделать настраиваемым из приложения!
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
    state_ptr->backend.window_state = window_state;

    CRITICAL_INIT(state_ptr->backend.initialize(&state_ptr->backend), "");
    // if(!state_ptr->backend.initialize(&state_ptr->backend))
    // {
    //     return false;
    // }

    // Шейдеры.
    const char* msg_shader_error = "Error creating built-in materila shader.";
    CRITICAL_INIT(state_ptr->backend.shader_create(
        "Builtin.MaterialShader", BUILTIN_RENDERPASS_WORLD, SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, true, true,
        &state_ptr->material_shader_id
    ), msg_shader_error);

    // Артибуты: Позиция и текстурные координаты.
    CRITICAL_INIT(state_ptr->backend.shader_add_attribute(state_ptr->material_shader_id, "in_position", SHADER_ATTRIB_TYPE_FLOAT32_3), msg_shader_error);
    CRITICAL_INIT(state_ptr->backend.shader_add_attribute(state_ptr->material_shader_id, "in_texcoord", SHADER_ATTRIB_TYPE_FLOAT32_2), msg_shader_error);

    // Uniform: Глобально.
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_mat4(state_ptr->material_shader_id, "projection", SHADER_SCOPE_GLOBAL, &state_ptr->material_shader_projection_location), msg_shader_error);
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_mat4(state_ptr->material_shader_id, "view", SHADER_SCOPE_GLOBAL, &state_ptr->material_shader_view_location), msg_shader_error);

    // Uniform: Экземпляр.
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_vec4(state_ptr->material_shader_id, "diffuse_color", SHADER_SCOPE_INSTANCE, &state_ptr->material_shader_diffuse_color_location), msg_shader_error);
    CRITICAL_INIT(state_ptr->backend.shader_add_sampler(state_ptr->material_shader_id, "diffuse_texture", SHADER_SCOPE_INSTANCE, &state_ptr->material_shader_diffuse_texture_location), msg_shader_error);

    // Uniform: Локально.
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_mat4(state_ptr->material_shader_id, "model", SHADER_SCOPE_LOCAL, &state_ptr->material_shader_model_location), msg_shader_error);

    CRITICAL_INIT(state_ptr->backend.shader_initialize(state_ptr->material_shader_id), msg_shader_error);
    ktrace("Material shaders created.");

    const char* msg_ui_shader_error = "Error creating built-in ui shader.";
    CRITICAL_INIT(state_ptr->backend.shader_create(
        "Builtin.UIShader", BUILTIN_RENDERPASS_UI, SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, true, true,
        &state_ptr->ui_shader_id
    ), msg_ui_shader_error);

    // Артибуты: Позиция и текстурные координаты.
    CRITICAL_INIT(state_ptr->backend.shader_add_attribute(state_ptr->ui_shader_id, "in_position", SHADER_ATTRIB_TYPE_FLOAT32_2), msg_ui_shader_error);
    CRITICAL_INIT(state_ptr->backend.shader_add_attribute(state_ptr->ui_shader_id, "in_texcoord", SHADER_ATTRIB_TYPE_FLOAT32_2), msg_ui_shader_error);

    // Uniform: Глобально.
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_mat4(state_ptr->ui_shader_id, "projection", SHADER_SCOPE_GLOBAL, &state_ptr->ui_shader_projection_location), msg_ui_shader_error);
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_mat4(state_ptr->ui_shader_id, "view", SHADER_SCOPE_GLOBAL, &state_ptr->ui_shader_view_location), msg_ui_shader_error);

    // Uniform: Экземпляр.
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_vec4(state_ptr->ui_shader_id, "diffuse_color", SHADER_SCOPE_INSTANCE, &state_ptr->ui_shader_diffuse_color_location), msg_ui_shader_error);
    CRITICAL_INIT(state_ptr->backend.shader_add_sampler(state_ptr->ui_shader_id, "diffuse_texture", SHADER_SCOPE_INSTANCE, &state_ptr->ui_shader_diffuse_texture_location), msg_ui_shader_error);

    // Uniform: Локально.
    CRITICAL_INIT(state_ptr->backend.shader_add_uniform_mat4(state_ptr->ui_shader_id, "model", SHADER_SCOPE_LOCAL, &state_ptr->ui_shader_model_location), msg_ui_shader_error);

    CRITICAL_INIT(state_ptr->backend.shader_initialize(state_ptr->ui_shader_id), msg_ui_shader_error);
    ktrace("UI shaders created.");

    // Создание матриц проекции и вида (world).
    // TODO: Сделать настраиваемыми!
    state_ptr->fov_radians = 45.0f;
    state_ptr->near_clip = 0.1f;
    state_ptr->far_clip = 1000.0f;
    f32 aspect = window_state->width / (f32)window_state->height;

    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);
    // TODO: Конфигурация стартовой позиции камеры.
    state_ptr->view = mat4_translation((vec3){{0, 0, 30.0f}});
    state_ptr->view = mat4_inverse(state_ptr->view);

    // Создание матриц проекции и вида (ui).
    state_ptr->ui_near_clip = -100.0f;
    state_ptr->ui_far_clip = 100.0f;
    state_ptr->ui_projection = mat4_orthographic(0, window_state->width, window_state->height, 0, state_ptr->ui_near_clip, state_ptr->ui_far_clip);
    state_ptr->ui_view = mat4_inverse(mat4_identity());

    return true;
}

void renderer_system_shutdown()
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Уничтожение шейдеров.
    state_ptr->backend.shader_destroy(state_ptr->material_shader_id);
    state_ptr->material_shader_id = INVALID_ID;
    state_ptr->backend.shader_destroy(state_ptr->ui_shader_id);
    state_ptr->ui_shader_id = INVALID_ID;

    // Завершение работы рендерера.
    state_ptr->backend.shutdown(&state_ptr->backend);
    renderer_backend_destroy(&state_ptr->backend);
    state_ptr = null;
}

bool renderer_draw_frame(render_packet* packet)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(state_ptr->backend.begin_frame(&state_ptr->backend, packet->delta_time))
    {
        // Проход (world).
        if(!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD))
        {
            kerror("Begin renderpass -> BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        state_ptr->backend.shader_use(state_ptr->material_shader_id);
        state_ptr->backend.shader_bind_globals(state_ptr->material_shader_id);
        state_ptr->backend.shader_set_uniform_mat4(state_ptr->material_shader_id, state_ptr->material_shader_projection_location, state_ptr->projection);
        state_ptr->backend.shader_set_uniform_mat4(state_ptr->material_shader_id, state_ptr->material_shader_view_location, state_ptr->view);
        state_ptr->backend.shader_apply_globals(state_ptr->material_shader_id);

        // Рисование World.
        u32 count = packet->geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            material* m = null;
            if(packet->geometries[i].geometry->material)
            {
                m = packet->geometries[i].geometry->material;
            }
            else
            {
                m = material_system_get_default();
            }

            // Применение материала.
            state_ptr->backend.shader_bind_instance(state_ptr->material_shader_id, m->internal_id);
            state_ptr->backend.shader_set_uniform_vec4(state_ptr->material_shader_id, state_ptr->material_shader_diffuse_color_location, m->diffuse_color);
            state_ptr->backend.shader_set_sampler(state_ptr->material_shader_id, state_ptr->material_shader_diffuse_texture_location, m->diffuse_map.texture);
            state_ptr->backend.shader_apply_instance(state_ptr->material_shader_id);
            state_ptr->backend.shader_set_uniform_mat4(state_ptr->material_shader_id, state_ptr->material_shader_model_location, packet->geometries[i].model);
            state_ptr->backend.draw_geometry(packet->geometries[i]);
        }

        if(!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_WORLD))
        {
            kerror("End renderpass -> BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        // Проход (world).
        if(!state_ptr->backend.begin_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI))
        {
            kerror("Begin renderpass -> BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        state_ptr->backend.shader_use(state_ptr->ui_shader_id);
        state_ptr->backend.shader_bind_globals(state_ptr->ui_shader_id);
        state_ptr->backend.shader_set_uniform_mat4(state_ptr->ui_shader_id, state_ptr->ui_shader_projection_location, state_ptr->ui_projection);
        state_ptr->backend.shader_set_uniform_mat4(state_ptr->ui_shader_id, state_ptr->ui_shader_view_location, state_ptr->ui_view);
        state_ptr->backend.shader_apply_globals(state_ptr->ui_shader_id);

        // Рисование UI.
        count = packet->ui_geometry_count;
        for(u32 i = 0; i < count; ++i)
        {
            material* m = null;
            if(packet->ui_geometries[i].geometry->material)
            {
                m = packet->ui_geometries[i].geometry->material;
            }
            else
            {
                m = material_system_get_default();
            }

            // Применение материала.
            state_ptr->backend.shader_bind_instance(state_ptr->ui_shader_id, m->internal_id);
            state_ptr->backend.shader_set_uniform_vec4(state_ptr->ui_shader_id, state_ptr->ui_shader_diffuse_color_location, m->diffuse_color);
            state_ptr->backend.shader_set_sampler(state_ptr->ui_shader_id, state_ptr->ui_shader_diffuse_texture_location, m->diffuse_map.texture);
            state_ptr->backend.shader_apply_instance(state_ptr->ui_shader_id);
            state_ptr->backend.shader_set_uniform_mat4(state_ptr->ui_shader_id, state_ptr->ui_shader_model_location, packet->ui_geometries[i].model);
            state_ptr->backend.draw_geometry(packet->ui_geometries[i]);
        }

        if(!state_ptr->backend.end_renderpass(&state_ptr->backend, BUILTIN_RENDERPASS_UI))
        {
            kerror("End renderpass -> BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        bool result = state_ptr->backend.end_frame(&state_ptr->backend, packet->delta_time);
        state_ptr->backend.frame_number++;
        if(!result)
        {
            kerror("Failed to complete function 'renderer_end_frame'. Shutting down.");
            return false;
        }
    }
    return true;
}

void renderer_on_resize(i32 width, i32 height)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return;
    }

    // Обновление проекции (world).
    f32 aspect = width / (f32)height;
    state_ptr->projection = mat4_perspective(state_ptr->fov_radians, aspect, state_ptr->near_clip, state_ptr->far_clip);

    // Обновление проекции (ui).
    state_ptr->ui_projection = mat4_orthographic(0, (f32)width, (f32)height, 0, state_ptr->ui_near_clip, state_ptr->ui_far_clip);

    state_ptr->backend.resized(&state_ptr->backend, width, height);    
}

void renderer_set_view(mat4 view)
{
    state_ptr->view = view;
}

void renderer_create_texture(texture* texture, const void* pixels)
{
    state_ptr->backend.create_texture(texture, pixels);
}

void renderer_destroy_texture(texture* texture)
{
    state_ptr->backend.destroy_texture(texture);
}

bool renderer_create_material(material* material)
{
    u32 shader_id = material->type == MATERIAL_TYPE_UI ? state_ptr->ui_shader_id : state_ptr->material_shader_id;
    return state_ptr->backend.shader_acquire_instance_resource(shader_id, &material->internal_id);
}

void renderer_destroy_material(material* material)
{
    u32 shader_id = material->type == MATERIAL_TYPE_UI ? state_ptr->ui_shader_id : state_ptr->material_shader_id;
    state_ptr->backend.shader_release_instance_resource(shader_id, material->internal_id);
}

bool renderer_create_geometry(
    geometry* geometry, u32 vertex_size, u32 vertex_count, const void* vertices, u32 index_size, u32 index_count,
    const void* indices
)
{
    return state_ptr->backend.create_geometry(geometry, vertex_size, vertex_count, vertices, index_size, index_count, indices);
}

void renderer_destroy_geometry(geometry* geometry)
{
    state_ptr->backend.destroy_geometry(geometry);
}

bool renderer_shader_create(const char* name, builtin_renderpass renderpass_id, shader_stage stages, bool use_instances, bool use_local, u32* out_shader_id)
{
    return state_ptr->backend.shader_create(name, renderpass_id, stages, use_instances, use_local, out_shader_id);
}

void renderer_shader_destroy(u32 shader_id)
{
    state_ptr->backend.shader_destroy(shader_id);
}

bool renderer_shader_add_attribute(u32 shader_id, const char* name, shader_attribute_type type)
{
    return state_ptr->backend.shader_add_attribute(shader_id, name, type);
}

bool renderer_shader_add_sampler(u32 shader_id, const char* sampler_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_sampler(shader_id, sampler_name, scope, out_location);
}

bool renderer_shader_add_uniform_i8(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_i8(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_i16(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_i16(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_i32(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_i32(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_u8(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_u8(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_u16(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_u16(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_u32(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_u32(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_f32(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_f32(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_vec2(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_vec2(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_vec3(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_vec3(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_vec4(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_vec4(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_mat4(u32 shader_id, const char* uniform_name, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_mat4(shader_id, uniform_name, scope, out_location);
}

bool renderer_shader_add_uniform_custom(u32 shader_id, const char* uniform_name, u32 size, shader_scope scope, u32* out_location)
{
    return state_ptr->backend.shader_add_uniform_custom(shader_id, uniform_name, size, scope, out_location);
}

bool renderer_shader_initialize(u32 shader_id)
{
    return state_ptr->backend.shader_initialize(shader_id);
}

bool renderer_shader_use(u32 shader_id)
{
    return state_ptr->backend.shader_use(shader_id);
}

bool renderer_shader_bind_globals(u32 shader_id)
{
    return state_ptr->backend.shader_bind_globals(shader_id);
}

bool renderer_shader_bind_instance(u32 shader_id, u32 instance_id)
{
    return state_ptr->backend.shader_bind_instance(shader_id, instance_id);
}

bool renderer_shader_apply_globals(u32 shader_id)
{
    return state_ptr->backend.shader_apply_globals(shader_id);
}

bool renderer_shader_apply_instance(u32 shader_id)
{
    return state_ptr->backend.shader_apply_instance(shader_id);
}

bool renderer_shader_acquire_instance_resource(u32 shader_id, u32* out_instance_id)
{
    return state_ptr->backend.shader_acquire_instance_resource(shader_id, out_instance_id);
}

bool renderer_shader_release_instance_resource(u32 shader_id, u32 instance_id)
{
    return state_ptr->backend.shader_release_instance_resource(shader_id, instance_id);
}

u32 renderer_shader_uniform_location(u32 shader_id, const char* uniform_name)
{
    return state_ptr->backend.shader_uniform_location(shader_id, uniform_name);
}

bool renderer_shader_set_sampler(u32 shader_id, u32 location, texture* t)
{
    return state_ptr->backend.shader_set_sampler(shader_id, location, t);
}

bool renderer_shader_set_uniform_i8(u32 shader_id, u32 location, i8 value)
{
    return state_ptr->backend.shader_set_uniform_i8(shader_id, location, value);
}

bool renderer_shader_set_uniform_i16(u32 shader_id, u32 location, i16 value)
{
    return state_ptr->backend.shader_set_uniform_i16(shader_id, location, value);
}

bool renderer_shader_set_uniform_i32(u32 shader_id, u32 location, i32 value)
{
    return state_ptr->backend.shader_set_uniform_i32(shader_id, location, value);
}

bool renderer_shader_set_uniform_u8(u32 shader_id, u32 location, u8 value)
{
    return state_ptr->backend.shader_set_uniform_u8(shader_id, location, value);
}

bool renderer_shader_set_uniform_u16(u32 shader_id, u32 location, u16 value)
{
    return state_ptr->backend.shader_set_uniform_u16(shader_id, location, value);
}

bool renderer_shader_set_uniform_u32(u32 shader_id, u32 location, u32 value)
{
    return state_ptr->backend.shader_set_uniform_u32(shader_id, location, value);
}

bool renderer_shader_set_uniform_f32(u32 shader_id, u32 location, f32 value)
{
    return state_ptr->backend.shader_set_uniform_f32(shader_id, location, value);
}

bool renderer_shader_set_uniform_vec2(u32 shader_id, u32 location, vec2 value)
{
    return state_ptr->backend.shader_set_uniform_vec2(shader_id, location, value);
}

bool renderer_shader_set_uniform_vec2f(u32 shader_id, u32 location, f32 value_0, f32 value_1)
{
    return state_ptr->backend.shader_set_uniform_vec2f(shader_id, location, value_0, value_1);
}

bool renderer_shader_set_uniform_vec3(u32 shader_id, u32 location, vec3 value)
{
    return state_ptr->backend.shader_set_uniform_vec3(shader_id, location, value);
}

bool renderer_shader_set_uniform_vec3f(u32 shader_id, u32 location, f32 value_0, f32 value_1, f32 value_2)
{
    return state_ptr->backend.shader_set_uniform_vec3f(shader_id, location, value_0, value_1, value_2);
}

bool renderer_shader_set_uniform_vec4(u32 shader_id, u32 location, vec4 value)
{
    return state_ptr->backend.shader_set_uniform_vec4(shader_id, location, value);
}

bool renderer_shader_set_uniform_vec4f(u32 shader_id, u32 location, f32 value_0, f32 value_1, f32 value_2, f32 value_3)
{
    return state_ptr->backend.shader_set_uniform_vec4f(shader_id, location, value_0, value_1, value_2, value_3);
}

bool renderer_shader_set_uniform_mat4(u32 shader_id, u32 location, mat4 value)
{
    return state_ptr->backend.shader_set_uniform_mat4(shader_id, location, value);
}

bool renderer_shader_set_uniform_custom(u32 shader_id, u32 location, void* value)
{
    return state_ptr->backend.shader_set_uniform_custom(shader_id, location, value);
}
