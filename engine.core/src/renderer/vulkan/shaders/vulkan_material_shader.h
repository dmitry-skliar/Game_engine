#pragma once

#include <defines.h>
#include <renderer/renderer_types.h>
#include <renderer/vulkan/vulkan_types.h>

bool vulkan_material_shader_create(vulkan_context* context, vulkan_shader_type type, vulkan_material_shader* out_shader);

void vulkan_material_shader_destroy(vulkan_context* context, vulkan_material_shader* shader);

void vulkan_material_shader_use(vulkan_context* context, vulkan_material_shader* shader);

void vulkan_material_shader_update_global_state(vulkan_context* context, vulkan_material_shader* shader, f32 delta_time);

void vulkan_material_shader_set_model(vulkan_context* context, vulkan_material_shader* shader, mat4* model);

void vulkan_material_shader_apply_material(vulkan_context* context, vulkan_material_shader* shader, material* material);

bool vulkan_material_shader_acquire_resources(vulkan_context* context, vulkan_material_shader* shader, material* material);

void vulkan_material_shader_release_resources(vulkan_context* context, vulkan_material_shader* shader, material* material);
