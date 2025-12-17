#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <tuple>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// 1. КЛАСС КАМЕРЫ
// ==========================================
class Camera {
public:
    enum Direction { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 10.0f, 25.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = -90.0f, float pitch = -20.0f) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(8.0f), 
          MouseSensitivity(0.1f), 
          Zoom(45.0f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Direction direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD) Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT) Position -= Right * velocity;
        if (direction == RIGHT) Position += Right * velocity;
        if (direction == UP) Position += WorldUp * velocity;
        if (direction == DOWN) Position -= WorldUp * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 90.0f) Zoom = 90.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

// ==========================================
// 2. КЛАСС ШЕЙДЕРА
// ==========================================
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath, bool isFile = false) {
        std::string vertexSource, fragmentSource;
        
        if (isFile) {
            vertexSource = readFile(vertexPath);
            fragmentSource = readFile(fragmentPath);
        } else {
            vertexSource = vertexPath;
            fragmentSource = fragmentPath;
        }
        
        const char* vShaderCode = vertexSource.c_str();
        const char* fShaderCode = fragmentSource.c_str();

        unsigned int vertex = compileShader(GL_VERTEX_SHADER, vShaderCode);
        unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }

    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setVec2(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string &name, float x, float y) const {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    void setVec4(const std::string &name, const glm::vec4 &value) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    std::string readFile(const char* filepath) {
        std::string content;
        std::ifstream file(filepath, std::ios::in);
        if (file.is_open()) {
            std::stringstream ss;
            ss << file.rdbuf();
            content = ss.str();
            file.close();
        } else {
            std::cerr << "Could not open file: " << filepath << std::endl;
        }
        return content;
    }

    unsigned int compileShader(GLenum type, const char* source) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        checkCompileErrors(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
        return shader;
    }

    void checkCompileErrors(unsigned int shader, std::string type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "SHADER_COMPILATION_ERROR: " << type << "\n" << infoLog << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "PROGRAM_LINKING_ERROR: " << type << "\n" << infoLog << std::endl;
            }
        }
    }
};

// ==========================================
// 3. СТРУКТУРЫ ДАННЫХ
// ==========================================
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Color;
    float Weight;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

// ==========================================
// 4. КЛАСС МЕША
// ==========================================
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO, VBO, EBO;

    // Конструктор по умолчанию
    Mesh() : VAO(0), VBO(0), EBO(0) {}

    Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds, std::vector<Texture> texs) {
        vertices = verts;
        indices = inds;
        textures = texs;
        setupMesh();
    }

    void Draw(Shader &shader) {
        if (VAO == 0) return; // Проверка инициализации
        
        // Используем текстуры если они есть
        if (!textures.empty() && textures[0].id != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[0].id);
            shader.setInt("material.diffuse", 0);

            if (textures.size() >= 2 && textures[1].id != 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, textures[1].id);
                shader.setInt("material.specular", 1);
            } else {
                shader.setInt("material.specular", 0);
            }
        } else {
            // Если текстур нет, используем цвет вершин
            shader.setInt("material.diffuse", 0);
            shader.setInt("material.specular", 0);
        }

        glBindVertexArray(VAO);
        if(indices.size() > 0) {
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        } else if(vertices.size() > 0) {
            glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        }
        glBindVertexArray(0);
    }

private:
    void setupMesh() {
        if (vertices.empty()) {
            std::cerr << "Warning: Mesh has no vertices!" << std::endl;
            return;
        }
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        if(indices.size() > 0) {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        }

        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // Color
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
        // Weight
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weight));

        glBindVertexArray(0);
    }
};

// ==========================================
// 5. УТИЛИТЫ ДЛЯ СОЗДАНИЯ МЕШЕЙ
// ==========================================
unsigned int loadTexture(const char *path) {
    unsigned int textureID = 0;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int createSolidColorTexture(glm::vec3 color) {
    unsigned int textureID = 0;
    
    // Проверяем, готов ли OpenGL
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error before texture creation: " << error << std::endl;
    }
    
    glGenTextures(1, &textureID);
    if (textureID == 0) {
        std::cerr << "Warning: Failed to generate texture! Using fallback." << std::endl;
        return 0;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    unsigned char data[3] = {
        (unsigned char)(color.r * 255),
        (unsigned char)(color.g * 255),
        (unsigned char)(color.b * 255)
    };
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return textureID;
}

Mesh createCube(glm::vec3 color = glm::vec3(1.0f), float size = 1.0f) {
    float s = size / 2.0f;
    std::vector<Vertex> vertices = {
        // Front face
        {glm::vec3(-s, -s, s), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, -s, s), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, s, s), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(-s, s, s), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), color, 1.0f},
        // Back face
        {glm::vec3(-s, -s, -s), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, -s, -s), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, s, -s), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(-s, s, -s), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), color, 1.0f},
        // Top face
        {glm::vec3(-s, s, -s), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, s, -s), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, s, s), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(-s, s, s), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), color, 1.0f},
        // Bottom face
        {glm::vec3(-s, -s, -s), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, -s, -s), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, -s, s), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(-s, -s, s), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f), color, 1.0f},
        // Right face
        {glm::vec3(s, -s, -s), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, -s, s), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(s, s, s), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(s, s, -s), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), color, 1.0f},
        // Left face
        {glm::vec3(-s, -s, -s), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(-s, -s, s), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(-s, s, s), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(-s, s, -s), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), color, 1.0f}
    };

    std::vector<unsigned int> indices = {
        0,1,2, 0,2,3,       // Front
        4,5,6, 4,6,7,       // Back
        8,9,10, 8,10,11,    // Top
        12,13,14, 12,14,15, // Bottom
        16,17,18, 16,18,19, // Right
        20,21,22, 20,22,23  // Left
    };

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f, 0.5f, 0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createSphere(float radius = 1.0f, int sectors = 36, int stacks = 18, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;

    for(int i = 0; i <= stacks; ++i) {
        float stackAngle = M_PI / 2 - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * sectorStep;

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            glm::vec3 position(x, y, z);
            glm::vec3 normal = glm::normalize(position);
            glm::vec2 texCoord((float)j / sectors, (float)i / stacks);

            vertices.push_back({position, normal, texCoord, color, 1.0f});
        }
    }

    for(int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if(i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if(i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createCylinder(float radius = 0.5f, float height = 2.0f, int segments = 36, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Side vertices
    for(int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
        
        // Bottom vertex
        vertices.push_back({
            glm::vec3(x, -height/2, z),
            normal,
            glm::vec2((float)i/segments, 0.0f),
            color,
            1.0f
        });
        
        // Top vertex
        vertices.push_back({
            glm::vec3(x, height/2, z),
            normal,
            glm::vec2((float)i/segments, 1.0f),
            color,
            1.0f
        });
    }

    // Side indices
    for(int i = 0; i < segments; ++i) {
        int idx = i * 2;
        indices.push_back(idx);
        indices.push_back(idx + 1);
        indices.push_back(idx + 2);
        
        indices.push_back(idx + 1);
        indices.push_back(idx + 3);
        indices.push_back(idx + 2);
    }

    // Top and bottom caps
    int centerBottom = vertices.size();
    vertices.push_back({glm::vec3(0.0f, -height/2, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f), color, 1.0f});
    
    int centerTop = vertices.size();
    vertices.push_back({glm::vec3(0.0f, height/2, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f), color, 1.0f});

    for(int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        
        // Bottom cap vertex
        vertices.push_back({
            glm::vec3(x, -height/2, z),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2(x/radius/2 + 0.5f, z/radius/2 + 0.5f),
            color,
            1.0f
        });
        
        // Top cap vertex
        vertices.push_back({
            glm::vec3(x, height/2, z),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec2(x/radius/2 + 0.5f, z/radius/2 + 0.5f),
            color,
            1.0f
        });
        
        if(i < segments) {
            // Bottom cap indices
            int bottomIdx = centerBottom + 1 + i * 2;
            indices.push_back(centerBottom);
            indices.push_back(bottomIdx);
            indices.push_back(bottomIdx + 2);
            
            // Top cap indices
            int topIdx = centerTop + 1 + i * 2;
            indices.push_back(centerTop);
            indices.push_back(topIdx + 2);
            indices.push_back(topIdx);
        }
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createPlane(float size = 40.0f, glm::vec3 color = glm::vec3(0.2f, 0.6f, 0.3f)) {
    float half = size / 2.0f;
    std::vector<Vertex> vertices = {
        {glm::vec3(-half, 0.0f, -half), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(half, 0.0f, -half), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(size, 0.0f), color, 1.0f},
        {glm::vec3(half, 0.0f, half), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(size, size), color, 1.0f},
        {glm::vec3(-half, 0.0f, half), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, size), color, 1.0f}
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 2, 3
    };

    unsigned int tex = createSolidColorTexture(glm::vec3(0.2f, 0.8f, 0.3f));
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.1f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createPyramid(float base = 1.0f, float height = 1.5f, glm::vec3 color = glm::vec3(1.0f, 0.5f, 0.0f)) {
    float half = base / 2.0f;
    std::vector<Vertex> vertices = {
        // Base vertices (counter-clockwise)
        {glm::vec3(-half, 0.0f, -half), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f), color, 1.0f},
        {glm::vec3(half, 0.0f, -half), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f), color, 1.0f},
        {glm::vec3(half, 0.0f, half), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f), color, 1.0f},
        {glm::vec3(-half, 0.0f, half), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f), color, 1.0f},
        // Apex
        {glm::vec3(0.0f, height, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f), color, 1.0f}
    };
    
    std::vector<unsigned int> indices = {
        // Base (two triangles)
        0, 2, 1,
        0, 3, 2,
        
        // Side faces
        // Front face (0-1-4)
        0, 1, 4,
        // Right face (1-2-4)
        1, 2, 4,
        // Back face (2-3-4)
        2, 3, 4,
        // Left face (3-0-4)
        3, 0, 4
    };
    
    // Calculate normals for side faces
    for (int i = 2; i < 6; ++i) {  // 4 side faces starting from index 2
        int idx0 = indices[i*3];
        int idx1 = indices[i*3+1];
        int idx2 = indices[i*3+2];
        
        glm::vec3 edge1 = vertices[idx1].Position - vertices[idx0].Position;
        glm::vec3 edge2 = vertices[idx2].Position - vertices[idx0].Position;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices[idx0].Normal = normal;
        vertices[idx1].Normal = normal;
        vertices[idx2].Normal = normal;
    }
    
    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };
    
    return Mesh(vertices, indices, textures);
}

Mesh createTorus(float majorRadius = 1.0f, float minorRadius = 0.3f, int majorSegments = 36, int minorSegments = 18, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for(int i = 0; i <= majorSegments; ++i) {
        float majorAngle = 2.0f * M_PI * i / majorSegments;
        glm::vec3 center(majorRadius * cosf(majorAngle), 0.0f, majorRadius * sinf(majorAngle));
        
        for(int j = 0; j <= minorSegments; ++j) {
            float minorAngle = 2.0f * M_PI * j / minorSegments;
            
            glm::vec3 position = center + glm::vec3(
                minorRadius * cosf(minorAngle) * cosf(majorAngle),
                minorRadius * sinf(minorAngle),
                minorRadius * cosf(minorAngle) * sinf(majorAngle)
            );
            
            glm::vec3 normal = glm::normalize(position - center);
            glm::vec2 texCoord((float)i/majorSegments, (float)j/minorSegments);
            
            vertices.push_back({position, normal, texCoord, color, 1.0f});
        }
    }

    for(int i = 0; i < majorSegments; ++i) {
        for(int j = 0; j < minorSegments; ++j) {
            int first = i * (minorSegments + 1) + j;
            int second = first + minorSegments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createCone(float radius = 0.5f, float height = 2.0f, int segments = 36, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Apex
    vertices.push_back({
        glm::vec3(0.0f, height, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.5f, 0.5f),
        color,
        1.0f
    });

    // Base vertices
    for(int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        
        glm::vec3 pos(x, 0.0f, z);
        glm::vec3 sideNormal = glm::normalize(glm::vec3(x, radius/height, z));
        glm::vec3 baseNormal(0.0f, -1.0f, 0.0f);
        
        // Side vertex
        vertices.push_back({
            pos,
            sideNormal,
            glm::vec2((float)i/segments, 0.0f),
            color,
            1.0f
        });
    }

    // Side indices
    for(int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }

    // Base center
    int centerIdx = vertices.size();
    vertices.push_back({
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.5f, 0.5f),
        color,
        1.0f
    });

    // Base triangles
    for(int i = 0; i < segments; ++i) {
        indices.push_back(centerIdx);
        indices.push_back(i + 2);
        indices.push_back(i + 1);
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createPrism(int sides = 6, float radius = 0.8f, float height = 2.0f, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Top and bottom vertices
    for(int i = 0; i <= sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        
        // Bottom vertex
        vertices.push_back({
            glm::vec3(x, -height/2, z),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec2((float)i/sides, 0.0f),
            color,
            1.0f
        });
        
        // Top vertex
        vertices.push_back({
            glm::vec3(x, height/2, z),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec2((float)i/sides, 1.0f),
            color,
            1.0f
        });
    }

    // Side faces
    for(int i = 0; i < sides; ++i) {
        int bottom1 = i * 2;
        int bottom2 = (i + 1) * 2;
        int top1 = bottom1 + 1;
        int top2 = bottom2 + 1;
        
        indices.push_back(bottom1);
        indices.push_back(top1);
        indices.push_back(bottom2);
        
        indices.push_back(top1);
        indices.push_back(top2);
        indices.push_back(bottom2);
    }

    // Top and bottom caps
    int centerBottom = vertices.size();
    vertices.push_back({
        glm::vec3(0.0f, -height/2, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.5f, 0.5f),
        color,
        1.0f
    });
    
    int centerTop = vertices.size();
    vertices.push_back({
        glm::vec3(0.0f, height/2, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.5f, 0.5f),
        color,
        1.0f
    });

    for(int i = 0; i < sides; ++i) {
        int idx1 = i * 2;
        int idx2 = (i + 1) * 2;
        
        // Bottom cap
        indices.push_back(centerBottom);
        indices.push_back(idx1);
        indices.push_back(idx2);
        
        // Top cap
        indices.push_back(centerTop);
        indices.push_back(idx2 + 1);
        indices.push_back(idx1 + 1);
    }

    // Recalculate normals for sides
    for(int i = 0; i < sides * 2; ++i) {
        int idx = i * 3;
        glm::vec3 v0 = vertices[indices[idx]].Position;
        glm::vec3 v1 = vertices[indices[idx+1]].Position;
        glm::vec3 v2 = vertices[indices[idx+2]].Position;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices[indices[idx]].Normal = normal;
        vertices[indices[idx+1]].Normal = normal;
        vertices[indices[idx+2]].Normal = normal;
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

// ==========================================
// 5.1 НОВЫЕ ФИГУРЫ
// ==========================================

Mesh createOctahedron(float size = 1.0f, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    float s = size;
    
    // Вершины октаэдра
    std::vector<glm::vec3> positions = {
        glm::vec3(0.0f, s, 0.0f),    // Верх
        glm::vec3(0.0f, -s, 0.0f),   // Низ
        glm::vec3(s, 0.0f, 0.0f),    // Право
        glm::vec3(-s, 0.0f, 0.0f),   // Лево
        glm::vec3(0.0f, 0.0f, s),    // Перед
        glm::vec3(0.0f, 0.0f, -s)    // Зад
    };
    
    // Грань 1: верх-право-перед
    glm::vec3 v0 = positions[0], v1 = positions[2], v2 = positions[4];
    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    
    // Грань 2: верх-перед-лево
    v0 = positions[0]; v1 = positions[4]; v2 = positions[3];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(3); indices.push_back(4); indices.push_back(5);
    
    // Грань 3: верх-лево-зад
    v0 = positions[0]; v1 = positions[3]; v2 = positions[5];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(6); indices.push_back(7); indices.push_back(8);
    
    // Грань 4: верх-зад-право
    v0 = positions[0]; v1 = positions[5]; v2 = positions[2];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(9); indices.push_back(10); indices.push_back(11);
    
    // Грань 5: низ-перед-право
    v0 = positions[1]; v1 = positions[4]; v2 = positions[2];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(12); indices.push_back(13); indices.push_back(14);
    
    // Грань 6: низ-право-зад
    v0 = positions[1]; v1 = positions[2]; v2 = positions[5];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(15); indices.push_back(16); indices.push_back(17);
    
    // Грань 7: низ-зад-лево
    v0 = positions[1]; v1 = positions[5]; v2 = positions[3];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(18); indices.push_back(19); indices.push_back(20);
    
    // Грань 8: низ-лево-перед
    v0 = positions[1]; v1 = positions[3]; v2 = positions[4];
    normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    vertices.push_back({v0, normal, glm::vec2(0.5f, 1.0f), color, 1.0f});
    vertices.push_back({v1, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
    vertices.push_back({v2, normal, glm::vec2(1.0f, 0.0f), color, 1.0f});
    indices.push_back(21); indices.push_back(22); indices.push_back(23);

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createIcosahedron(float radius = 1.0f, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // Константы для создания икосаэдра
    const float t = (1.0f + sqrt(5.0f)) / 2.0f;
    
    // 12 вершин икосаэдра
    std::vector<glm::vec3> positions = {
        glm::vec3(-1.0f, t, 0.0f), glm::vec3(1.0f, t, 0.0f),
        glm::vec3(-1.0f, -t, 0.0f), glm::vec3(1.0f, -t, 0.0f),
        glm::vec3(0.0f, -1.0f, t), glm::vec3(0.0f, 1.0f, t),
        glm::vec3(0.0f, -1.0f, -t), glm::vec3(0.0f, 1.0f, -t),
        glm::vec3(t, 0.0f, -1.0f), glm::vec3(t, 0.0f, 1.0f),
        glm::vec3(-t, 0.0f, -1.0f), glm::vec3(-t, 0.0f, 1.0f)
    };
    
    // Нормализуем вершины до заданного радиуса
    for (auto& pos : positions) {
        pos = glm::normalize(pos) * radius;
    }
    
    // 20 граней икосаэдра
    std::vector<std::tuple<int, int, int>> faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };
    
    for (size_t i = 0; i < faces.size(); ++i) {
        int i0 = std::get<0>(faces[i]);
        int i1 = std::get<1>(faces[i]);
        int i2 = std::get<2>(faces[i]);
        
        glm::vec3 v0 = positions[i0];
        glm::vec3 v1 = positions[i1];
        glm::vec3 v2 = positions[i2];
        
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        
        vertices.push_back({v0, normal, glm::vec2(0.0f, 0.0f), color, 1.0f});
        vertices.push_back({v1, normal, glm::vec2(0.5f, 0.0f), color, 1.0f});
        vertices.push_back({v2, normal, glm::vec2(0.0f, 0.5f), color, 1.0f});
        
        indices.push_back(i * 3);
        indices.push_back(i * 3 + 1);
        indices.push_back(i * 3 + 2);
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

Mesh createHelix(float radius = 1.0f, float height = 3.0f, float turns = 3.0f, int segments = 100, glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        float angle = t * 2.0f * M_PI * turns;
        float y = t * height - height/2.0f;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        
        // Тангенциальный вектор для вычисления нормали
        glm::vec3 tangent(-radius * sinf(angle), height/(segments*turns*2.0f*M_PI), radius * cosf(angle));
        glm::vec3 normal = glm::normalize(glm::vec3(cosf(angle), 0.0f, sinf(angle)));
        
        vertices.push_back({
            glm::vec3(x, y, z),
            normal,
            glm::vec2(t, 0.0f),
            color,
            1.0f
        });
        
        // Вторая точка для создания ленты
        float innerRadius = radius * 0.3f;
        float innerX = innerRadius * cosf(angle);
        float innerZ = innerRadius * sinf(angle);
        
        vertices.push_back({
            glm::vec3(innerX, y, innerZ),
            normal,
            glm::vec2(t, 1.0f),
            color,
            1.0f
        });
    }
    
    for (int i = 0; i < segments; ++i) {
        int idx = i * 2;
        indices.push_back(idx);
        indices.push_back(idx + 1);
        indices.push_back(idx + 2);
        
        indices.push_back(idx + 1);
        indices.push_back(idx + 3);
        indices.push_back(idx + 2);
    }

    unsigned int tex = createSolidColorTexture(color);
    unsigned int specTex = createSolidColorTexture(glm::vec3(0.5f));
    std::vector<Texture> textures = {
        {tex, "diffuse", ""},
        {specTex, "specular", ""}
    };

    return Mesh(vertices, indices, textures);
}

// ==========================================
// 6. ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ И ШЕЙДЕРЫ
// ==========================================
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Освещение
bool directionalLightEnabled = true;
bool pointLightEnabled = true;
bool spotLightEnabled = true;

// Позиция точечного источника
glm::vec3 pointLightPos = glm::vec3(5.0f, 5.0f, 5.0f);

// Камера
Camera camera(glm::vec3(0.0f, 15.0f, 25.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -25.0f);

// Структура для объекта сцены
struct SceneObject {
    Mesh mesh;
    glm::vec3 position;
    glm::vec3 scale;
    float rotationSpeed;
    glm::vec3 rotationAxis;
    bool useVertexColor;
    bool useGradient;
    glm::vec3 color;
    std::string name;
    
    // Конструктор по умолчанию
    SceneObject() : 
        mesh(), 
        position(0.0f), 
        scale(1.0f), 
        rotationSpeed(0.0f), 
        rotationAxis(0.0f, 1.0f, 0.0f),
        useVertexColor(true),
        useGradient(false),
        color(1.0f),
        name("Object")
    {}
    
    // Конструктор с параметрами
    SceneObject(const Mesh& m, const glm::vec3& pos, const glm::vec3& scl, 
                float rotSpeed, const glm::vec3& rotAxis, bool useVertCol,
                bool useGrad, const glm::vec3& col, const std::string& n) :
        mesh(m),
        position(pos),
        scale(scl),
        rotationSpeed(rotSpeed),
        rotationAxis(rotAxis),
        useVertexColor(useVertCol),
        useGradient(useGrad),
        color(col),
        name(n)
    {}
};

// Шейдеры
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aColor;
layout (location = 4) in float aWeight;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 VertexColor;
out float Weight;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    VertexColor = aColor;
    Weight = aWeight;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 VertexColor;
in float Weight;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;
uniform Material material;
uniform bool useVertexColor;
uniform bool useGradient;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 color);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color);

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 baseColor = useVertexColor ? VertexColor : texture(material.diffuse, TexCoords).rgb;
    if (useGradient) {
        baseColor *= Weight;
    }
    
    vec3 result = vec3(0.0);
    if (dirLight.enabled) result += CalcDirLight(dirLight, norm, viewDir, baseColor);
    if (pointLight.enabled) result += CalcPointLight(pointLight, norm, FragPos, viewDir, baseColor);
    if (spotLight.enabled) result += CalcSpotLight(spotLight, norm, FragPos, viewDir, baseColor);
    
    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 color) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;
    
    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}
)";

const char* lightCubeShaderVS = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* lightCubeShaderFS = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;

void main() {
    FragColor = vec4(lightColor, 1.0);
}
)";

// ==========================================
// 7. ФУНКЦИЯ ОБРАБОТКИ ВВОДА
// ==========================================
void processInput(SDL_Window* window, float deltaTime, bool& running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_1:
                    directionalLightEnabled = !directionalLightEnabled;
                    std::cout << "Directional Light: " << (directionalLightEnabled ? "ON" : "OFF") << std::endl;
                    break;
                case SDLK_2:
                    pointLightEnabled = !pointLightEnabled;
                    std::cout << "Point Light: " << (pointLightEnabled ? "ON" : "OFF") << std::endl;
                    break;
                case SDLK_3:
                    spotLightEnabled = !spotLightEnabled;
                    std::cout << "Spot Light: " << (spotLightEnabled ? "ON" : "OFF") << std::endl;
                    break;
                case SDLK_f:
                    // Переключение полного экрана
                    static bool fullscreen = false;
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    break;
                case SDLK_h:
                    std::cout << "\n=== УПРАВЛЕНИЕ ===" << std::endl;
                    std::cout << "WASD + Space/Shift: Движение камеры" << std::endl;
                    std::cout << "Мышь: Вращение камеры" << std::endl;
                    std::cout << "Колесо мыши: Приближение/отдаление" << std::endl;
                    std::cout << "1, 2, 3: Включение/выключение источников света" << std::endl;
                    std::cout << "Стрелки + PageUp/Down: Движение точечного источника" << std::endl;
                    std::cout << "F: Полный экран" << std::endl;
                    std::cout << "H: Помощь" << std::endl;
                    std::cout << "ESC: Выход" << std::endl;
                    break;
            }
        }
        
        if (event.type == SDL_MOUSEMOTION) {
            static bool firstMouse = true;
            static float lastX = SCR_WIDTH / 2.0f;
            static float lastY = SCR_HEIGHT / 2.0f;
            
            if (firstMouse) {
                lastX = event.motion.x;
                lastY = event.motion.y;
                firstMouse = false;
            }
            
            float xoffset = event.motion.x - lastX;
            float yoffset = lastY - event.motion.y;
            
            lastX = event.motion.x;
            lastY = event.motion.y;
            
            camera.ProcessMouseMovement(xoffset, yoffset);
        }
        
        if (event.type == SDL_MOUSEWHEEL) {
            camera.ProcessMouseScroll(event.wheel.y);
        }
    }
    
    const Uint8* keyState = SDL_GetKeyboardState(NULL);
    
    if (keyState[SDL_SCANCODE_W]) camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
    if (keyState[SDL_SCANCODE_S]) camera.ProcessKeyboard(Camera::BACKWARD, deltaTime);
    if (keyState[SDL_SCANCODE_A]) camera.ProcessKeyboard(Camera::LEFT, deltaTime);
    if (keyState[SDL_SCANCODE_D]) camera.ProcessKeyboard(Camera::RIGHT, deltaTime);
    if (keyState[SDL_SCANCODE_SPACE]) camera.ProcessKeyboard(Camera::UP, deltaTime);
    if (keyState[SDL_SCANCODE_LSHIFT]) camera.ProcessKeyboard(Camera::DOWN, deltaTime);
    
    // Движение точечного источника
    float lightSpeed = 8.0f * deltaTime;
    if (keyState[SDL_SCANCODE_UP]) pointLightPos.z -= lightSpeed;
    if (keyState[SDL_SCANCODE_DOWN]) pointLightPos.z += lightSpeed;
    if (keyState[SDL_SCANCODE_LEFT]) pointLightPos.x -= lightSpeed;
    if (keyState[SDL_SCANCODE_RIGHT]) pointLightPos.x += lightSpeed;
    if (keyState[SDL_SCANCODE_PAGEUP]) pointLightPos.y += lightSpeed;
    if (keyState[SDL_SCANCODE_PAGEDOWN]) pointLightPos.y -= lightSpeed;
}

// ==========================================
// 8. ОСНОВНАЯ ФУНКЦИЯ
// ==========================================
int main() {
    std::cout << "Starting program..." << std::endl;
    
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    std::cout << "SDL initialized successfully" << std::endl;
    
    // Настройка OpenGL атрибутов
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    // Создание окна
    SDL_Window* window = SDL_CreateWindow("3D Сцена с разными фигурами",
                                         SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED,
                                         SCR_WIDTH, SCR_HEIGHT,
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    std::cout << "Window created successfully" << std::endl;
    
    // Создание OpenGL контекста
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // Установка VSync
    SDL_GL_SetSwapInterval(1);
    
    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    std::cout << "GLEW initialized successfully" << std::endl;
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    // Проверка поддержки OpenGL 3.3
    if (!GLEW_VERSION_3_3) {
        std::cerr << "OpenGL 3.3 is not supported!" << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // Проверка ошибок после инициализации
    GLenum glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after initialization: " << glError << std::endl;
    }
    
    // Настройка OpenGL состояний
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    
    // Проверка ошибок после настройки состояний
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error after state setup: " << glError << std::endl;
    }
    
    // Компиляция шейдеров (теперь контекст полностью инициализирован)
    std::cout << "Compiling shaders..." << std::endl;
    Shader lightingShader(vertexShaderSource, fragmentShaderSource, false);
    Shader lightCubeShader(lightCubeShaderVS, lightCubeShaderFS, false);
    
    // Проверка компиляции шейдеров
    if (lightingShader.ID == 0 || lightCubeShader.ID == 0) {
        std::cerr << "Failed to compile shaders!" << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    std::cout << "Shaders compiled successfully" << std::endl;
    
    // Теперь, когда OpenGL контекст полностью инициализирован,
    // можно безопасно создавать текстуры и мешы
    std::cout << "Creating scene objects..." << std::endl;
    
    // Создание объектов сцены
    std::vector<SceneObject> objects;
    
    // 1. Плоскость (земля)
    std::cout << "Creating plane..." << std::endl;
    Mesh planeMesh = createPlane(60.0f, glm::vec3(0.2f, 0.6f, 0.3f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during plane creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        planeMesh,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        false,
        false,
        glm::vec3(1.0f),
        "Плоскость"
    ));
    
    // 2. Куб
    std::cout << "Creating cube..." << std::endl;
    Mesh cubeMesh = createCube(glm::vec3(0.9f, 0.9f, 0.0f), 4.0f);
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during cube creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        cubeMesh,
        glm::vec3(0.0f, 2.0f, 0.0f),
        glm::vec3(1.0f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.9f, 0.9f, 0.0f),
        "Куб"
    ));
    
    // 3. Октаэдр
    std::cout << "Creating octahedron..." << std::endl;
    Mesh octahedronMesh = createOctahedron(0.8f, glm::vec3(1.0f, 0.2f, 0.2f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during octahedron creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        octahedronMesh,
        glm::vec3(6.0f, 2.0f, 0.0f),
        glm::vec3(0.8f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(1.0f, 0.2f, 0.2f),
        "Октаэдр"
    ));
    
    // 4. Сфера
    std::cout << "Creating sphere..." << std::endl;
    Mesh sphereMesh = createSphere(1.0f, 32, 16, glm::vec3(0.2f, 0.4f, 1.0f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during sphere creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        sphereMesh,
        glm::vec3(-6.0f, 2.0f, 0.0f),
        glm::vec3(0.7f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.2f, 0.4f, 1.0f),
        "Сфера"
    ));
    
    // 5. Икосаэдр
    std::cout << "Creating icosahedron..." << std::endl;
    Mesh icosahedronMesh = createIcosahedron(0.7f, glm::vec3(0.2f, 0.8f, 0.3f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during icosahedron creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        icosahedronMesh,
        glm::vec3(0.0f, 2.0f, 6.0f),
        glm::vec3(0.6f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.2f, 0.8f, 0.3f),
        "Икосаэдр"
    ));
    
    // 6. Тор
    std::cout << "Creating torus..." << std::endl;
    Mesh torusMesh = createTorus(0.8f, 0.3f, 32, 16, glm::vec3(0.8f, 0.2f, 0.8f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during torus creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        torusMesh,
        glm::vec3(0.0f, 2.0f, -6.0f),
        glm::vec3(0.5f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.8f, 0.2f, 0.8f),
        "Тор"
    ));
    
    // 7. Конус
    std::cout << "Creating cone..." << std::endl;
    Mesh coneMesh = createCone(0.6f, 1.8f, 24, glm::vec3(1.0f, 0.5f, 0.1f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during cone creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        coneMesh,
        glm::vec3(6.0f, 2.0f, 6.0f),
        glm::vec3(0.6f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(1.0f, 0.5f, 0.1f),
        "Конус"
    ));
    
    // 8. Призма (шестиугольная)
    std::cout << "Creating prism..." << std::endl;
    Mesh prismMesh = createPrism(6, 0.8f, 1.6f, glm::vec3(0.3f, 0.7f, 1.0f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during prism creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        prismMesh,
        glm::vec3(-6.0f, 2.0f, 6.0f),
        glm::vec3(0.5f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        true,
        glm::vec3(0.3f, 0.7f, 1.0f),
        "Призма"
    ));
    
    // 9. Спираль
    std::cout << "Creating helix..." << std::endl;
    Mesh helixMesh = createHelix(0.5f, 2.0f, 5.0f, 60, glm::vec3(0.7f, 0.7f, 0.9f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during helix creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        helixMesh,
        glm::vec3(6.0f, 2.0f, -6.0f),
        glm::vec3(0.4f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.7f, 0.7f, 0.9f),
        "Спираль"
    ));
    
    // 10. Пирамида
    std::cout << "Creating pyramid..." << std::endl;
    Mesh pyramidMesh = createPyramid(1.5f, 2.0f, glm::vec3(0.9f, 0.8f, 0.1f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during pyramid creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        pyramidMesh,
        glm::vec3(-6.0f, 2.0f, -6.0f),
        glm::vec3(0.5f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.9f, 0.8f, 0.1f),
        "Пирамида"
    ));
    
    // 11. Цилиндр
    std::cout << "Creating cylinder..." << std::endl;
    Mesh cylinderMesh = createCylinder(0.5f, 2.0f, 24, glm::vec3(0.8f, 0.2f, 0.2f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during cylinder creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        cylinderMesh,
        glm::vec3(12.0f, 2.0f, 0.0f),
        glm::vec3(0.6f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.8f, 0.2f, 0.2f),
        "Цилиндр"
    ));
    
    // 12. Маленький куб
    std::cout << "Creating small cube..." << std::endl;
    Mesh smallCubeMesh = createCube(glm::vec3(0.5f, 0.5f, 0.5f), 0.6f);
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during small cube creation: " << glError << std::endl;
    }
    objects.push_back(SceneObject(
        smallCubeMesh,
        glm::vec3(-12.0f, 2.0f, 0.0f),
        glm::vec3(0.5f),
        0.0f,
        glm::vec3(0.0f, 1.0f, 0.0f),
        true,
        false,
        glm::vec3(0.5f, 0.5f, 0.5f),
        "Маленький куб"
    ));
    
    // Создание меша для точечного источника (сфера)
    std::cout << "Creating light sphere..." << std::endl;
    Mesh pointLightSphere = createSphere(0.3f, 16, 8, glm::vec3(1.0f, 1.0f, 0.8f));
    glError = glGetError();
    if (glError != GL_NO_ERROR) {
        std::cerr << "OpenGL error during light sphere creation: " << glError << std::endl;
    }
    
    std::cout << "=== 3D СЦЕНА С РАЗНЫМИ ФИГУРАМИ ===" << std::endl;
    std::cout << "Управление:" << std::endl;
    std::cout << "WASD + Space/Shift: Движение камеры" << std::endl;
    std::cout << "Мышь: Вращение камеры" << std::endl;
    std::cout << "Колесо мыши: Приближение/отдаление" << std::endl;
    std::cout << "1, 2, 3: Включение/выключение источников света" << std::endl;
    std::cout << "Стрелки + PageUp/Down: Движение точечного источника" << std::endl;
    std::cout << "F: Полный экран" << std::endl;
    std::cout << "H: Помощь" << std::endl;
    std::cout << "ESC: Выход" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "All objects created successfully!" << std::endl;
    std::cout << "Entering main loop..." << std::endl;
    
    // Главный цикл
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float totalTime = 0.0f;
    bool running = true;
    
    while (running) {
        // Обработка времени
        float currentFrame = SDL_GetTicks() / 1000.0f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        totalTime += deltaTime;
        
        // Обработка ввода
        processInput(window, deltaTime, running);
        
        // Проверка изменения размера окна
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
        
        // Очистка буферов
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Матрицы вида и проекции
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)width / (float)height,
            0.1f, 200.0f
        );
        glm::mat4 view = camera.GetViewMatrix();
        
        // Рендеринг объектов с освещением
        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setVec3("viewPos", camera.Position);
        
        // Материал
        lightingShader.setFloat("material.shininess", 64.0f);
        
        // Направленный свет (солнечный свет)
        lightingShader.setVec3("dirLight.direction", glm::vec3(-0.5f, -1.0f, -0.3f));
        lightingShader.setVec3("dirLight.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
        lightingShader.setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.8f, 0.6f));
        lightingShader.setVec3("dirLight.specular", glm::vec3(1.0f, 1.0f, 0.9f));
        lightingShader.setBool("dirLight.enabled", directionalLightEnabled);
        
        // Точечный свет (дополнительный источник)
        lightingShader.setVec3("pointLight.position", pointLightPos);
        lightingShader.setFloat("pointLight.constant", 1.0f);
        lightingShader.setFloat("pointLight.linear", 0.09f);
        lightingShader.setFloat("pointLight.quadratic", 0.032f);
        lightingShader.setVec3("pointLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        lightingShader.setVec3("pointLight.diffuse", glm::vec3(0.8f, 0.8f, 0.7f));
        lightingShader.setVec3("pointLight.specular", glm::vec3(1.0f, 1.0f, 0.9f));
        lightingShader.setBool("pointLight.enabled", pointLightEnabled);
        
        // Прожектор (следит за камерой)
        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setVec3("spotLight.ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        lightingShader.setVec3("spotLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setVec3("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setBool("spotLight.enabled", spotLightEnabled);
        
        // Рендеринг всех объектов
        for (size_t i = 0; i < objects.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, objects[i].position);
            model = glm::scale(model, objects[i].scale);
            lightingShader.setMat4("model", model);
            lightingShader.setBool("useVertexColor", objects[i].useVertexColor);
            lightingShader.setBool("useGradient", objects[i].useGradient);
            objects[i].mesh.Draw(lightingShader);
        }
        
        // Рендеринг точечного источника (сфера)
        if (pointLightEnabled) {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPos);
            model = glm::scale(model, glm::vec3(0.3f));
            lightCubeShader.setMat4("model", model);
            lightCubeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.8f));
            pointLightSphere.Draw(lightCubeShader);
        }
        
        // Проверка ошибок OpenGL в конце кадра
        glError = glGetError();
        if (glError != GL_NO_ERROR && glError != GL_INVALID_OPERATION) {
            std::cerr << "OpenGL error during rendering: " << glError << std::endl;
        }
        
        // Обмен буферов
        SDL_GL_SwapWindow(window);
    }
    
    std::cout << "Exiting..." << std::endl;
    
    // Очистка ресурсов
    // (VAO, VBO, текстуры будут автоматически удалены при завершении программы)
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}