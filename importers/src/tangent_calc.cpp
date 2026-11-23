// TODO implement function to calc tangent when not exists in gltf
#include "tangent_calc.hpp"

#include <assets/mesh.hpp>

#include <glm/glm.hpp>
#include <mikktspace.h>

#include <stdexcept>
#include <iostream>

void TangentCalc::calc_tangents(Mesh &mesh){

  auto get_num_faces = [](const SMikkTSpaceContext* p_context) {
    auto* mesh_data = static_cast<Mesh*>(p_context->m_pUserData);
    return static_cast<int>(mesh_data->indices.size()) / 3;
  };

  auto get_num_vertices_of_face = [](const SMikkTSpaceContext* p_context, const int i_face) {
    return 3;
  };

  auto get_position = [](const SMikkTSpaceContext* p_context, float pos_out[], const int i_face, const int i_vert) {
    auto* mesh_data = static_cast<Mesh*>(p_context->m_pUserData);
    uint32_t index = mesh_data->indices[i_face * 3 + i_vert];
    const glm::vec3& pos = mesh_data->vertex_pos[index];

    pos_out[0] = pos.x;
    pos_out[1] = pos.y;
    pos_out[2] = pos.z;
  };

  auto get_normal = [](const SMikkTSpaceContext* p_context, float norm_out[], const int i_face, const int i_vert) {
    auto* mesh_data = static_cast<Mesh*>(p_context->m_pUserData);
    uint32_t index = mesh_data->indices[i_face * 3 + i_vert];
    const glm::vec3& norm = mesh_data->vertex_normal[index];

    norm_out[0] = norm[0];
    norm_out[1] = norm[1];
    norm_out[2] = norm[2];
  };

  auto get_tex_coord = [](const SMikkTSpaceContext* p_context, float tex_out[], const int i_face, const int i_vert) {
    auto* mesh_data = static_cast<Mesh*>(p_context->m_pUserData);
    uint32_t index = mesh_data->indices[i_face * 3 + i_vert];
    const glm::vec2& tex_coord = mesh_data->vertex_tex_coord[index];

    tex_out[0] = tex_coord.x;
    tex_out[1] = tex_coord.y;
  };

  auto set_t_space_basic = [](const SMikkTSpaceContext* p_context, const float tangent[], const float sign, const int i_face, const int i_vert) {
    auto* mesh_data = static_cast<Mesh*>(p_context->m_pUserData);
    uint32_t index = mesh_data->indices[i_face * 3 + i_vert];

    mesh_data->vertex_tangent[index] = glm::vec4(glm::vec3 {tangent[0], tangent[1], tangent[2]}, sign);
  };

  SMikkTSpaceInterface interface {
    .m_getNumFaces = get_num_faces,
    .m_getNumVerticesOfFace = get_num_vertices_of_face,
    .m_getPosition = get_position,
    .m_getNormal = get_normal,
    .m_getTexCoord = get_tex_coord,
    .m_setTSpaceBasic = set_t_space_basic
  };

  SMikkTSpaceContext context {
    .m_pInterface = &interface,
    .m_pUserData = &mesh
  };

  if (!genTangSpaceDefault(&context)) {
    throw std::runtime_error("Unable to generate tangents");
  }

  //std::cout << "Tangents calculated:\n";
  //for (int i = 0; i < mesh.vertex_tangent.size(); i++) {
  //  if (glm::length(mesh.vertex_tangent[i]) == 0.0) {
  //    continue;
  //  }
  //  std::cout << "x: " << mesh.vertex_tangent[i].x << ", y: " << mesh.vertex_tangent[i].y << ", z: " << mesh.vertex_tangent[i].z << std::endl; 
  //}
  //std::cout << "\n";
}
