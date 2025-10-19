#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <memory>
#include <array>
#include <sstream>
#include <iomanip>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Глобальные переменные для анимации и прозрачности
float global_angle = 0.0f;
float global_alpha = 0.8f;
const float rotation_speed = 1.0f;

// Переменные для эффекта лупы
bool magnifier_active = false;
float magnifier_x = 0.0f;
float magnifier_y = 0.0f;
const float magnifier_radius = 80.0f;
const float magnifier_zoom = 2.0f;

// Переменные для подсчёта FPS
Uint32 frame_start_time = 0;
Uint32 frame_end_time = 0;
float frame_time = 0.0f;
float fps = 0.0f;
const int FPS_SMOOTHING = 10;

struct COLOR {
    float r, g, b, a;
    
    COLOR() : r(0), g(0), b(0), a(1.0f) {}
    COLOR(float red, float green, float blue, float alpha = 1.0f) 
        : r(red), g(green), b(blue), a(alpha) {}
};

// Единый цвет для всех букв
const COLOR LETTER_COLOR = COLOR(70.0f/255.0f, 130.0f/255.0f, 255.0f/255.0f, 1.0f);

class Matrix {
    float M[3][3];
public:
    Matrix() : M{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}} {}
    
    Matrix(float A00, float A01, float A02,
           float A10, float A11, float A12,
           float A20, float A21, float A22) : 
           M{{A00, A01, A02}, {A10, A11, A12}, {A20, A21, A22}} {}
    
    Matrix operator * (const Matrix& A) const {
        Matrix R;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                R.M[i][j] = M[i][0] * A.M[0][j] + 
                           M[i][1] * A.M[1][j] + 
                           M[i][2] * A.M[2][j];
            }
        }
        return R;
    }
    
    static Matrix Rotation(float angle) {
        float rad = angle * M_PI / 180.0f;
        float cosA = cos(rad);
        float sinA = sin(rad);
        return Matrix(cosA, sinA, 0,
                     -sinA, cosA, 0,
                     0, 0, 1);
    }
    
    static Matrix Translation(float tx, float ty) {
        return Matrix(1, 0, 0,
                     0, 1, 0,
                     tx, ty, 1);
    }
    
    static Matrix Scaling(float sx, float sy) {
        return Matrix(sx, 0, 0,
                     0, sy, 0,
                     0, 0, 1);
    }
    
    static Matrix WorldToScreen(float X1, float Y1, float X2, float Y2, 
                               float x1, float y1, float x2, float y2) {
        float px = (X2 - X1) / (x2 - x1);
        float py = (Y2 - Y1) / (y2 - y1);
        return Matrix(px, 0, 0,
                     0, -py, 0,
                     X1 - x1 * px, Y2 + y1 * py, 1);
    }
    
    const float* operator[](int index) const { return M[index]; }
    float* operator[](int index) { return M[index]; }
};

class Vector {
public:
    float x, y;
    
    Vector() : x(0), y(0) {}
    Vector(float _x, float _y) : x(_x), y(_y) {}
    
    Vector operator * (const Matrix &A) const {
        Vector E;
        E.x = x * A[0][0] + y * A[1][0] + A[2][0];
        E.y = x * A[0][1] + y * A[1][1] + A[2][1];
        float h = x * A[0][2] + y * A[1][2] + A[2][2];
        if (h != 0) {
            E.x /= h;
            E.y /= h;
        }
        return E;
    }
};

// Структура для хранения данных треугольника
struct Triangle {
    Vector v0, v1, v2;
    COLOR color;
    float time;
    float alpha;
    int effect_type; // 0: pulse, 1: wave, 2: gradient, 3: flicker, 4: flood
};

// Класс для русской буквы "Р" с отверстием
class LetterR {
private:
    std::vector<Vector> vertices;
    std::vector<std::tuple<int, int, int>> triangles;
    
public:
    LetterR() {
        // Русская буква "Р" - вертикальная палка и полукруг с отверстием
        vertices = {
            // Основной вертикальный стержень
            Vector(0.0f, 0.0f),    // 0 - левый низ
            Vector(0.0f, 2.0f),    // 1 - левый верх
            Vector(0.5f, 2.0f),    // 2 - правый верх
            Vector(0.5f, 0.0f),    // 3 - правый низ
            
            // Внешний полукруг (закругленная часть)
            Vector(0.5f, 1.5f),    // 4 - начало полукруга
            Vector(0.5f, 2.0f),    // 5
            Vector(0.8f, 1.9f),    // 6
            Vector(1.0f, 1.7f),    // 7
            Vector(1.0f, 1.3f),    // 8
            Vector(0.8f, 1.1f),    // 9
            Vector(0.5f, 1.0f),    // 10 - конец полукруга
            
            // Внутренний полукруг (отверстие)
            Vector(0.5f, 1.6f),    // 11 - начало внутреннего полукруга
            Vector(0.5f, 1.9f),    // 12
            Vector(0.7f, 1.85f),   // 13
            Vector(0.85f, 1.75f),  // 14
            Vector(0.85f, 1.45f),  // 15
            Vector(0.7f, 1.35f),   // 16
            Vector(0.5f, 1.3f)     // 17 - конец внутреннего полукруга
        };
        
        // Треугольники для основного стержня
        triangles.push_back(std::make_tuple(0, 1, 2));
        triangles.push_back(std::make_tuple(0, 2, 3));
        
        // Треугольники для внешнего полукруга (с отверстием)
        triangles.push_back(std::make_tuple(4, 5, 6));
        triangles.push_back(std::make_tuple(4, 6, 7));
        triangles.push_back(std::make_tuple(4, 7, 8));
        triangles.push_back(std::make_tuple(4, 8, 9));
        triangles.push_back(std::make_tuple(4, 9, 10));
        
        // Треугольники для внутреннего отверстия (вычитаемая область)
        triangles.push_back(std::make_tuple(11, 12, 13));
        triangles.push_back(std::make_tuple(11, 13, 14));
        triangles.push_back(std::make_tuple(11, 14, 15));
        triangles.push_back(std::make_tuple(11, 15, 16));
        triangles.push_back(std::make_tuple(11, 16, 17));
    }
    
    void GetTriangles(std::vector<Triangle>& output, const Matrix& transform, float time, float alpha) {
        std::vector<Vector> transformedVertices;
        for (const auto& v : vertices) {
            transformedVertices.push_back(v * transform);
        }
        
        // Создаем треугольники с разными эффектами
        for (size_t i = 0; i < triangles.size(); i++) {
            int i0, i1, i2;
            std::tie(i0, i1, i2) = triangles[i];
            
            Triangle tri;
            tri.v0 = transformedVertices[i0];
            tri.v1 = transformedVertices[i1];
            tri.v2 = transformedVertices[i2];
            tri.time = time;
            tri.alpha = alpha;
            tri.color = LETTER_COLOR;
            
            if (i < 2) {
                // Основной стержень - эффект пульсации
                tri.effect_type = 0;
            } else if (i < 7) {
                // Внешний полукруг - эффект волн
                tri.effect_type = 1;
            } else {
                // Внутреннее отверстие - черный цвет
                tri.color = COLOR(0, 0, 0, 1.0f);
                tri.effect_type = 0; // Простой шейдер для отверстия
            }
            
            output.push_back(tri);
        }
    }
};

// Класс для буквы "А"
class LetterA {
private:
    std::vector<Vector> vertices;
    std::vector<std::tuple<int, int, int>> triangles;
    
public:
    LetterA() {
        // Буква "А"
        vertices = {
            // Левая ножка
            Vector(0.0f, 0.0f),    // 0
            Vector(0.3f, 2.0f),    // 1
            Vector(0.5f, 2.0f),    // 2
            Vector(0.2f, 0.0f),    // 3
            
            // Правая ножка
            Vector(0.5f, 2.0f),    // 4
            Vector(0.7f, 2.0f),    // 5
            Vector(1.0f, 0.0f),    // 6
            Vector(0.8f, 0.0f),    // 7
            
            // Перекладина
            Vector(0.3f, 1.0f),    // 8
            Vector(0.7f, 1.0f),    // 9
            Vector(0.7f, 0.8f),    // 10
            Vector(0.3f, 0.8f)     // 11
        };
        
        // Треугольники для левой ножки
        triangles.push_back(std::make_tuple(0, 1, 2));
        triangles.push_back(std::make_tuple(0, 2, 3));
        
        // Треугольники для правой ножки
        triangles.push_back(std::make_tuple(4, 5, 6));
        triangles.push_back(std::make_tuple(4, 6, 7));
        
        // Треугольники для перекладины
        triangles.push_back(std::make_tuple(8, 9, 10));
        triangles.push_back(std::make_tuple(8, 10, 11));
    }
    
    void GetTriangles(std::vector<Triangle>& output, const Matrix& transform, float time, float alpha) {
        std::vector<Vector> transformedVertices;
        for (const auto& v : vertices) {
            transformedVertices.push_back(v * transform);
        }
        
        // Создаем треугольники с эффектами
        for (size_t i = 0; i < triangles.size(); i++) {
            int i0, i1, i2;
            std::tie(i0, i1, i2) = triangles[i];
            
            Triangle tri;
            tri.v0 = transformedVertices[i0];
            tri.v1 = transformedVertices[i1];
            tri.v2 = transformedVertices[i2];
            tri.time = time;
            tri.alpha = alpha;
            tri.color = LETTER_COLOR;
            
            if (i < 2) {
                // Левая ножка - эффект затопления
                tri.effect_type = 4;
            } else if (i < 4) {
                // Правая ножка - эффект градиента
                tri.effect_type = 2;
            } else {
                // Перекладина - эффект мерцания
                tri.effect_type = 3;
            }
            
            output.push_back(tri);
        }
    }
};

// Класс для работы с шейдерами
class ShaderProgram {
private:
    GLuint programID;
    
public:
    ShaderProgram() : programID(0) {}
    
    ~ShaderProgram() {
        if (programID) {
            glDeleteProgram(programID);
        }
    }
    
    bool LoadShaders(const char* vertexShaderSource, const char* fragmentShaderSource) {
        // Создание и компиляция вершинного шейдера
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        // Проверка вершинного шейдера
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            SDL_Log("Ошибка компиляции вершинного шейдера: %s", infoLog);
            return false;
        }
        
        // Создание и компиляция фрагментного шейдера
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        // Проверка фрагментного шейдера
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            SDL_Log("Ошибка компиляции фрагментного шейдера: %s", infoLog);
            glDeleteShader(vertexShader);
            return false;
        }
        
        // Создание шейдерной программы
        programID = glCreateProgram();
        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);
        
        // Проверка шейдерной программы
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            SDL_Log("Ошибка линковки шейдерной программы: %s", infoLog);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return false;
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return true;
    }
    
    void Use() {
        glUseProgram(programID);
    }
    
    GLuint GetID() const { return programID; }
    
    void SetFloat(const std::string& name, float value) {
        glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
    }
    
    void SetVec2(const std::string& name, float x, float y) {
        glUniform2f(glGetUniformLocation(programID, name.c_str()), x, y);
    }
    
    void SetVec3(const std::string& name, float x, float y, float z) {
        glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z);
    }
    
    void SetVec4(const std::string& name, float x, float y, float z, float w) {
        glUniform4f(glGetUniformLocation(programID, name.c_str()), x, y, z, w);
    }
    
    void SetInt(const std::string& name, int value) {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }
};

// Исходный код вершинного шейдера
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 vertexColor;
out vec2 fragPos;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vertexColor = aColor;
    fragPos = aPos;
}
)";

// Исходный код фрагментного шейдера с эффектами
const char* fragmentShaderSource = R"(
#version 330 core
in vec4 vertexColor;
in vec2 fragPos;

out vec4 FragColor;

uniform float uTime;
uniform int uEffectType;
uniform float uAlpha;

// Эффект пульсации
vec4 pulseEffect(vec4 baseColor, float time) {
    float pulse = sin(time * 3.0) * 0.3 + 0.7;
    return vec4(baseColor.rgb * pulse, baseColor.a * uAlpha);
}

// Эффект волн
vec4 waveEffect(vec4 baseColor, vec2 pos, float time) {
    float wave = sin(pos.x * 0.05 + time * 2.0) * 0.3 + 0.7;
    return vec4(baseColor.rgb * wave, baseColor.a * uAlpha);
}

// Эффект градиента
vec4 gradientEffect(vec4 baseColor, vec2 pos) {
    float gradient = (pos.y + 1.0) * 0.5; // Преобразуем из [-1,1] в [0,1]
    return vec4(baseColor.rgb * gradient, baseColor.a * uAlpha);
}

// Эффект мерцания
vec4 flickerEffect(vec4 baseColor, vec2 pos, float time) {
    float flicker = sin(time * 8.0 + pos.x * 0.1) * 0.2 + 0.8;
    return vec4(baseColor.rgb * flicker, baseColor.a * uAlpha);
}

// Эффект затопления
vec4 floodEffect(vec4 baseColor, vec2 pos, float time) {
    float floodLevel = sin(time * 1.5) * 0.3 + 0.5;
    
    if (pos.y < floodLevel - 1.0) { // Преобразуем в локальные координаты
        // "Затопленная" область - более темный оттенок
        return vec4(baseColor.rgb * 0.7, baseColor.a * uAlpha * 0.8);
    } else {
        // Обычная область
        return vec4(baseColor.rgb, baseColor.a * uAlpha);
    }
}

void main() {
    vec4 resultColor = vertexColor;
    
    // Применяем эффекты в зависимости от типа
    if (uEffectType == 0) {
        resultColor = pulseEffect(vertexColor, uTime);
    } else if (uEffectType == 1) {
        resultColor = waveEffect(vertexColor, fragPos, uTime);
    } else if (uEffectType == 2) {
        resultColor = gradientEffect(vertexColor, fragPos);
    } else if (uEffectType == 3) {
        resultColor = flickerEffect(vertexColor, fragPos, uTime);
    } else if (uEffectType == 4) {
        resultColor = floodEffect(vertexColor, fragPos, uTime);
    }
    
    FragColor = resultColor;
}
)";

// Исходный код шейдера для эффекта лупы
const char* magnifierVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* magnifierFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec2 uMousePos;
uniform float uMagnifierRadius;
uniform float uMagnifierZoom;
uniform bool uMagnifierActive;

void main() {
    vec2 texCoord = TexCoord;
    
    if (uMagnifierActive) {
        // Вычисляем расстояние от курсора мыши
        vec2 mouseCoord = vec2(uMousePos.x, uMousePos.y);
        float distance = length(TexCoord - mouseCoord);
        
        // Если пиксель внутри радиуса лупы
        if (distance < uMagnifierRadius) {
            // Вычисляем новые координаты текстуры с увеличением
            texCoord = mouseCoord + (TexCoord - mouseCoord) / uMagnifierZoom;
            
            // Проверяем границы текстуры
            if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
                // За границами - черный цвет
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }
        }
    }
    
    FragColor = texture(uTexture, texCoord);
}
)";

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Не удалось инициализировать SDL: %s", SDL_GetError());
        return 1;
    }

    // Настройка атрибутов OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Лабораторная работа №3 - OpenGL/GLSL версия", 
                                         SDL_WINDOWPOS_CENTERED, 
                                         SDL_WINDOWPOS_CENTERED,
                                         WINDOW_WIDTH, WINDOW_HEIGHT, 
                                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Не удалось создать окно: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("Не удалось создать контекст OpenGL: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("Ошибка инициализации GLEW: %s", glewGetErrorString(glewError));
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Настройка OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Создание шейдерных программ
    ShaderProgram mainShader;
    if (!mainShader.LoadShaders(vertexShaderSource, fragmentShaderSource)) {
        SDL_Log("Не удалось загрузить основные шейдеры");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    ShaderProgram magnifierShader;
    if (!magnifierShader.LoadShaders(magnifierVertexShaderSource, magnifierFragmentShaderSource)) {
        SDL_Log("Не удалось загрузить шейдеры лупы");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Создание FBO (Frame Buffer Object) для рендеринга в текстуру
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_Log("Ошибка настройки FBO");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Создание VBO и VAO для полноэкранного квада (для постобработки)
    GLuint quadVAO, quadVBO;
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // Создание объектов для рендеринга треугольников
    GLuint triangleVAO, triangleVBO;
    glGenVertexArrays(1, &triangleVAO);
    glGenBuffers(1, &triangleVBO);
    
    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    
    // Настройка атрибутов вершин
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    LetterR letterR;
    LetterA letterA;

    // Переменные для подсчёта FPS
    std::vector<float> frame_times;
    float avg_frame_time = 0.0f;
    float avg_fps = 0.0f;

    auto updateWindowTitle = [window, &avg_frame_time, &avg_fps]() {
        std::ostringstream title;
        title << "Лаб. работа №3 - OpenGL/GLSL"
              << " - Прозрачность: " << static_cast<int>(global_alpha * 100) << "%"
              << " - Время кадра: " << std::fixed << std::setprecision(2) << avg_frame_time << " мс"
              << " - FPS: " << std::fixed << std::setprecision(1) << avg_fps;
        SDL_SetWindowTitle(window, title.str().c_str());
    };

    updateWindowTitle();

    bool running = true;
    SDL_Event event;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        frame_start_time = SDL_GetTicks();
        float time = (frame_start_time - startTime) / 1000.0f;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS) {
                    // Увеличение прозрачности
                    global_alpha = std::min(global_alpha + 0.1f, 1.0f);
                    updateWindowTitle();
                } else if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS) {
                    // Уменьшение прозрачности
                    global_alpha = std::max(global_alpha - 0.1f, 0.1f);
                    updateWindowTitle();
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Активируем лупу при клике мыши
                if (event.button.button == SDL_BUTTON_LEFT) {
                    magnifier_active = true;
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    magnifier_x = static_cast<float>(mouseX) / WINDOW_WIDTH;
                    magnifier_y = 1.0f - static_cast<float>(mouseY) / WINDOW_HEIGHT; // Инвертируем Y для OpenGL
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                // Деактивируем лупу при отпускании кнопки мыши
                if (event.button.button == SDL_BUTTON_LEFT) {
                    magnifier_active = false;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                // Обновляем позицию лупы при движении мыши
                if (magnifier_active) {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    magnifier_x = static_cast<float>(mouseX) / WINDOW_WIDTH;
                    magnifier_y = 1.0f - static_cast<float>(mouseY) / WINDOW_HEIGHT; // Инвертируем Y для OpenGL
                }
            }
        }

        global_angle += rotation_speed;
        if (global_angle > 360.0f) {
            global_angle -= 360.0f;
        }

        // === ПЕРВЫЙ ПРОХОД: рендеринг в текстуру ===
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Используем основной шейдер
        mainShader.Use();
        mainShader.SetFloat("uTime", time);
        mainShader.SetFloat("uAlpha", global_alpha);

        // Получаем все треугольники для отрисовки
        std::vector<Triangle> allTriangles;
        
        // Исправленная матрица для преобразования мировых координат в экранные OpenGL
        // В OpenGL координаты от -1 до 1, а наши буквы имеют размер около 2 единиц
        Matrix WS = Matrix::Scaling(0.2f, 0.2f) * Matrix::Translation(0.0f, 0.0f);

        // АНИМАЦИЯ БУКВЫ "Р" - как в оригинальной SDL версии
        Matrix transformR = Matrix::Translation(-2.0f, 0.0f) * 
                          Matrix::Rotation(global_angle) * 
                          Matrix::Scaling(0.6f + 0.1f * sin(time * 2), 
                                        0.6f + 0.1f * cos(time * 2)) * 
                          WS;
        letterR.GetTriangles(allTriangles, transformR, time, global_alpha);

        // АНИМАЦИЯ БУКВЫ "А" - как в оригинальной SDL версии  
        Matrix transformA = Matrix::Translation(2.0f * cos(time * 0.7f), 
                                              1.0f * sin(time * 1.0f)) * 
                          Matrix::Rotation(-global_angle * 0.6f) * 
                          Matrix::Scaling(0.5f + 0.08f * sin(time * 2.5f), 
                                        0.5f + 0.08f * cos(time * 2.0f)) * 
                          WS;
        letterA.GetTriangles(allTriangles, transformA, time, global_alpha);

        // Отрисовка всех треугольников
        glBindVertexArray(triangleVAO);
        for (const auto& tri : allTriangles) {
            // Подготавливаем данные вершин
            float vertices[] = {
                // positions        // colors
                tri.v0.x, tri.v0.y, tri.color.r, tri.color.g, tri.color.b, tri.color.a,
                tri.v1.x, tri.v1.y, tri.color.r, tri.color.g, tri.color.b, tri.color.a,
                tri.v2.x, tri.v2.y, tri.color.r, tri.color.g, tri.color.b, tri.color.a
            };
            
            // Устанавливаем тип эффекта для текущего треугольника
            mainShader.SetInt("uEffectType", tri.effect_type);
            
            // Загружаем данные в VBO
            glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
            
            // Отрисовываем треугольник
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // === ВТОРОЙ ПРОХОД: применение эффекта лупы ===
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Используем шейдер лупы
        magnifierShader.Use();
        magnifierShader.SetInt("uTexture", 0);
        magnifierShader.SetVec2("uMousePos", magnifier_x, magnifier_y);
        magnifierShader.SetFloat("uMagnifierRadius", magnifier_radius / WINDOW_WIDTH);
        magnifierShader.SetFloat("uMagnifierZoom", magnifier_zoom);
        magnifierShader.SetInt("uMagnifierActive", magnifier_active ? 1 : 0);

        // Привязываем текстуру
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Отрисовываем полноэкранный квад
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Обмен буферов
        SDL_GL_SwapWindow(window);

        // Подсчёт времени рендеринга кадра и FPS
        frame_end_time = SDL_GetTicks();
        frame_time = frame_end_time - frame_start_time;
        
        // Сохраняем время кадра для усреднения
        frame_times.push_back(frame_time);
        if (frame_times.size() > FPS_SMOOTHING) {
            frame_times.erase(frame_times.begin());
        }
        
        // Вычисляем среднее время кадра и FPS
        avg_frame_time = 0.0f;
        for (float ft : frame_times) {
            avg_frame_time += ft;
        }
        avg_frame_time /= frame_times.size();
        
        if (avg_frame_time > 0) {
            avg_fps = 1000.0f / avg_frame_time;
        }
        
        // Обновляем заголовок окна с информацией о производительности
        updateWindowTitle();

        // Задержка для стабилизации FPS
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time);
        }
    }

    // Освобождение ресурсов
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}