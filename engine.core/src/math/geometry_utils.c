// Внутренние подключения.
#include "math/geometry_utils.h"

// Внутренние подключения.
#include "logger.h"
#include "math/kmath.h"
#include "memory/memory.h"

void geometry_generate_normals(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices)
{
    for(u32 i = 0; i < index_count; i += 3)
    {
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        vec3 edge1 = vec3_sub(vertices[i1].position, vertices[i0].position);
        vec3 edge2 = vec3_sub(vertices[i2].position, vertices[i0].position);

        vec3 normal = vec3_normalized(vec3_cross(edge1, edge2));

        // NOTE: Генерирует нормаль поверхности. Сглаживание выполнить отдельно, если это необходимо.
        vertices[i0].normal = normal;
        vertices[i1].normal = normal;
        vertices[i2].normal = normal;
    }
}

// Смотри: https://terathon.com/blog/tangent-space.html
//         https://triplepointfive.github.io/ogltutor/tutorials/tutorial26.html
void geometry_generate_tangent(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices)
{
    for(u32 i = 0; i < index_count; i += 3)
    {
        // Получение индексов вершин треуголиников.
        u32 i0 = indices[i + 0];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        // 
        vec3 edge1 = vec3_sub(vertices[i1].position, vertices[i0].position);
        vec3 edge2 = vec3_sub(vertices[i2].position, vertices[i0].position);

        //
        f32 deltaU1 = vertices[i1].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV1 = vertices[i1].texcoord.y - vertices[i0].texcoord.y;

        f32 deltaU2 = vertices[i2].texcoord.x - vertices[i0].texcoord.x;
        f32 deltaV2 = vertices[i2].texcoord.y - vertices[i0].texcoord.y;

        f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
        f32 fc = 1.0f / dividend;

        vec3 tangent = (vec3){{
            (fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
            (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
            (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z))
        }};

        tangent = vec3_normalized(tangent);

        f32 sx = deltaU1, sy = deltaU2;
        f32 tx = deltaV1, ty = deltaV2;
        f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;
        vec4 t4 = vec4_from_vec3(tangent, handedness);

        vertices[i0].tangent = t4;
        vertices[i1].tangent = t4;
        vertices[i2].tangent = t4;
    }
}

void geometry_reassign_index(u32 index_count, u32* indices, u32 from, u32 to)
{
    for(u32 i = 0; i < index_count; ++i)
    {
        if(indices[i] == from)
        {
            indices[i] = to;
        }
        else if(indices[i] > from)
        {
            indices[i]--;
        }
    }
}

void geometry_deduplicate_vertices(u32 vertex_count, vertex_3d* vertices, u32 index_count, u32* indices, u32* out_vertex_count, vertex_3d** out_vertices)
{
    vertex_3d* unique_vertices = kallocate_tc(vertex_3d, vertex_count, MEMORY_TAG_ARRAY);
    *out_vertex_count = 0;

    u32 found_count = 0;
    for(u32 v = 0; v < vertex_count; ++v)
    {
        bool found = false;

        for(u32 u = 0; u < *out_vertex_count; ++u)
        {
            if(vertex_3d_equal(vertices[v], unique_vertices[u]))
            {
                geometry_reassign_index(index_count, indices, v - found_count, u);
                found = true;
                found_count++;
                break;
            }
        }

        if(!found)
        {
            unique_vertices[*out_vertex_count] = vertices[v];
            (*out_vertex_count)++;
        }
    }

    *out_vertices = kallocate_tc(vertex_3d, (*out_vertex_count), MEMORY_TAG_ARRAY);
    kcopy_tc(*out_vertices, unique_vertices, vertex_3d, (*out_vertex_count));

    kfree_tc(unique_vertices, vertex_3d, vertex_count, MEMORY_TAG_ARRAY);

    kdebug("Function '%s': Removed %u vertices, orign/now %u/%u.", __FUNCTION__, found_count, vertex_count, *out_vertex_count);
}
