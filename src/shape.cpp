#include "shape.h"

#include "util.h"

#include <fstream>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

std::shared_ptr<Shape> load_shape_unmanaged(std::string filename) {
    std::ifstream file(filename);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> vertex_normals;
    std::vector<glm::vec2> texture_coords;
    std::vector<Face> faces;


    char buffer[500];
    while (file.good()) {
        file.getline(buffer, sizeof(buffer));

        if (buffer[0] == '#') continue;

        std::string line(buffer);

        if (line.length() == 0) continue;

        std::vector<std::string> pieces = {};

        size_t prev_index = 0;
        do {
            size_t index = line.find(' ', prev_index);
            if (prev_index != index) {
                pieces.push_back(line.substr(prev_index, index - prev_index));
            }
            prev_index = index + 1;
        } while (prev_index - 1 != std::string::npos);

        if (pieces.size() == 0) continue;

        if (pieces[0] == "v") {
            vertices.push_back({
                std::stof(pieces[1]),
                std::stof(pieces[2]),
                std::stof(pieces[3]),
            });
        } else if (pieces[0] == "vn") {
            vertex_normals.push_back({
                std::stof(pieces[1]),
                std::stof(pieces[2]),
                std::stof(pieces[3]),
            });

        } else if (pieces[0] == "vt") {
            texture_coords.push_back({
                std::stof(pieces[1]),
                std::stof(pieces[2]),
            });
        } else if (pieces[0] == "f") {
            std::vector<FaceVertex> vertices;
            for (size_t i = 1; i<pieces.size(); i++) {
                auto& piece = pieces[i];

                FaceVertex vertex;
                vertex.vertex = std::stoi(piece) - 1;
                size_t index = piece.find('/');
                if (index != std::string::npos) {
                    index++;
                    if (piece[index] != '/')
                        vertex.texture = std::stoi(piece.substr(index)) - 1;

                    index = piece.find('/', index);
                    if (index != std::string::npos) {
                        vertex.normal = std::stoi(piece.substr(index + 1)) - 1;
                    }
                }
                vertices.push_back(vertex);
            }

            for (size_t i = 0; i<pieces.size() - 3; i++) {
                faces.push_back({
                    vertices[0],
                    vertices[i + 1],
                    vertices[i + 2],
                });
            }
        }
    }

    for (auto& face : faces) {
        bool needs_normal = false;
        for (auto& vertex : face) {
            if (vertex.normal == -1) {
                needs_normal = true;
                break;
            }
        }
        if (!needs_normal) continue;

        glm::vec3 face_vertices[] = {
            vertices[face[0].vertex], 
            vertices[face[1].vertex], 
            vertices[face[2].vertex],
        };
        glm::vec3 normal = glm::normalize(glm::cross(face_vertices[1] - face_vertices[0], face_vertices[2] - face_vertices[0]));
        vertex_normals.push_back(normal);

        for (auto& vertex : face) {
            if (vertex.normal == -1) {
                vertex.normal = vertex_normals.size() - (int32_t)1;
            }
        }
    }

    file.close();
    
    return std::make_shared<Shape>(vertices, texture_coords, vertex_normals, faces);
}

Shape::~Shape() {
    if (solid_displaylist) {
        glDeleteLists(solid_displaylist, 1);
    }
}

void Shape::update_bounds() {
    bounds = AABB(vertices);
}

void Shape::render_solid() const {
    if (solid_displaylist) {
        glCallList(solid_displaylist);
    } else {
        solid_displaylist = glGenLists(1);
        glNewList(solid_displaylist, GL_COMPILE_AND_EXECUTE);
        glBegin(GL_TRIANGLES);
        //float i = 0;
        for (auto& face : faces) {
            //float u = i++ / faces.size();
            //glColor(u, 1-u, 1);
            if (face[0].texture >= 0)
                glTexCoord(texture_coords[face[0].texture]);
            glNormal(vertex_normals[face[0].normal]);
            glVertex(vertices[face[0].vertex]);

            if (face[1].texture >= 0)
                glTexCoord(texture_coords[face[1].texture]);
            glNormal(vertex_normals[face[1].normal]);
            glVertex(vertices[face[1].vertex]);

            if (face[2].texture >= 0)
                glTexCoord(texture_coords[face[2].texture]);
            glNormal(vertex_normals[face[2].normal]);
            glVertex(vertices[face[2].vertex]);
        }
        glEnd();
        glEndList();
    }
}

void Shape::render_wireframe() const {
    glBegin(GL_LINES);
    for (auto& face : faces) {
        glNormal(vertex_normals[face[0].normal]);
        glVertex(vertices[face[0].vertex]);
        glNormal(vertex_normals[face[1].normal]);
        glVertex(vertices[face[1].vertex]);

        glNormal(vertex_normals[face[1].normal]);
        glVertex(vertices[face[1].vertex]);
        glNormal(vertex_normals[face[2].normal]);
        glVertex(vertices[face[2].vertex]);

        glNormal(vertex_normals[face[2].normal]);
        glVertex(vertices[face[2].vertex]);
        glNormal(vertex_normals[face[0].normal]);
        glVertex(vertices[face[0].vertex]);
    }
    glEnd();
}

void Shape::render_shadow(const Light& light) const {
    const float depth = light.shadow_length;
    for (auto& face : faces) {
        glm::vec3 face_vertices[] = {
            vertices[face[0].vertex], 
            vertices[face[1].vertex], 
            vertices[face[2].vertex],
        };


        glm::vec3 normal = glm::cross(face_vertices[1] - face_vertices[0], face_vertices[2] - face_vertices[0]);
        
        glm::vec3 opposing[3];
        if (light.is_point) {
            if (glm::dot(normal, light.pos - face_vertices[0]) <= 0) continue;

            opposing[0] = face_vertices[0] + glm::normalize(face_vertices[0] - light.pos) * depth;
            opposing[1] = face_vertices[1] + glm::normalize(face_vertices[1] - light.pos) * depth;
            opposing[2] = face_vertices[2] + glm::normalize(face_vertices[2] - light.pos) * depth;
        } else {
            if (glm::dot(normal, light.vec) >= 0) continue;

            opposing[0] = face_vertices[0] + light.vec * depth;
            opposing[1] = face_vertices[1] + light.vec * depth;
            opposing[2] = face_vertices[2] + light.vec * depth;
        }

        glBegin(GL_TRIANGLE_STRIP); // Draw triangle prism
        glVertex(face_vertices[0]);
        glVertex(face_vertices[1]);
        glVertex(face_vertices[2]);

        glVertex(opposing[2]);
        glVertex(face_vertices[0]);
        glVertex(opposing[0]);
        glVertex(face_vertices[1]);

        glVertex(opposing[1]);
        glVertex(opposing[2]);
        glVertex(opposing[0]);
        glEnd();
    }
}

void Shape::render_face_normals(float length) const {
    glBegin(GL_LINES);
    for (auto& face : faces) {
        glm::vec3 face_vertices[] = {
            vertices[face[0].vertex], 
            vertices[face[1].vertex], 
            vertices[face[2].vertex],
        };

        glm::vec3 normal = glm::normalize(glm::cross(face_vertices[1] - face_vertices[0], face_vertices[2] - face_vertices[0]));
        glm::vec3 centre = (face_vertices[0] + face_vertices[1] + face_vertices[2]) / 3.0f;
        
        glVertex(centre);

        auto point = centre + normal * length;
        glVertex(point);
    }
    glEnd();
}

void Shape::render_normals(float length) const {
    glBegin(GL_LINES);
    for (auto& face : faces) {
        for (auto& vert : face) {
            auto pos = vertices[vert.vertex];
            auto normal = vertex_normals[vert.normal];
            glVertex(pos);

            auto point = pos + normal * length;
            glVertex(point);
        }
    }
    glEnd();
}

void Shape::dump() const {
    std::cout << "   Faces:\n";
    for (auto face : faces) 
        std::cout << "[" << 
            face[0].vertex << "/" << face[0].texture << "/" << face[0].normal << "," << 
            face[1].vertex << "/" << face[1].texture << "/" << face[1].normal << "," << 
            face[2].vertex << "/" << face[2].texture << "/" << face[2].normal << "]";
    std::cout << std::endl;

    std::cout << "Vertices:\n";
    for (auto vertex : vertices) std::cout << glm::to_string(vertex);
    std::cout << std::endl;

    std::cout << "Texture Coords:\n";
    for (auto coord : texture_coords) std::cout << glm::to_string(coord);
    std::cout << std::endl;

    std::cout << " Normals:\n";
    for (auto normal : vertex_normals) std::cout << glm::to_string(normal);
    std::cout << std::endl;
}

