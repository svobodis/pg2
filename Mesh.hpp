#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp> 
#include <glm/ext.hpp>

#include "assets.hpp"
#include "non_copyable.hpp"

class Mesh : private NonCopyable
{
public:
    static constexpr GLuint attribute_location_position{ 0 };
    static constexpr GLuint attribute_location_normal{ 1 };
    static constexpr GLuint attribute_location_texture_coords{ 2 };

    Mesh() = delete;

    Mesh(std::vector<Vertex> const& vertices, GLenum primitive_type)
        : primitive_type_{ primitive_type }, vertices{ vertices } 
    {
        glCreateVertexArrays(1, &vao_);
        glCreateBuffers(1, &vbo_);

        glNamedBufferData(vbo_, this->vertices.size() * sizeof(Vertex), this->vertices.data(), GL_STATIC_DRAW);

        glEnableVertexArrayAttrib(vao_, attribute_location_position);
        glVertexArrayAttribFormat(vao_, attribute_location_position, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexArrayAttribBinding(vao_, attribute_location_position, 0);

        glEnableVertexArrayAttrib(vao_, attribute_location_normal);
        glVertexArrayAttribFormat(vao_, attribute_location_normal, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexArrayAttribBinding(vao_, attribute_location_normal, 0);

        glEnableVertexArrayAttrib(vao_, attribute_location_texture_coords);
        glVertexArrayAttribFormat(vao_, attribute_location_texture_coords, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoords));
        glVertexArrayAttribBinding(vao_, attribute_location_texture_coords, 0);

        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(Vertex));
    }

    Mesh(std::vector<Vertex> const& vertices, std::vector<GLuint> const& indices, GLenum primitive_type) :
        Mesh{ vertices, primitive_type }
    {
        this->indices = indices;

        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_, this->indices.size() * sizeof(GLuint), this->indices.data(), GL_STATIC_DRAW);

        glVertexArrayElementBuffer(vao_, ebo_);
    }

    void draw() {
        glBindVertexArray(vao_);

        if (ebo_ == 0) {
            glDrawArrays(primitive_type_, 0, vertices.size());
        }
        else {
            glDrawElements(primitive_type_, indices.size(), GL_UNSIGNED_INT, nullptr);
        }
    }

    ~Mesh() {
        glDeleteBuffers(1, &ebo_);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
    };

private:
    GLenum primitive_type_{ GL_TRIANGLES };

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    GLuint vao_{ 0 };
    GLuint vbo_{ 0 };
    GLuint ebo_{ 0 };
};