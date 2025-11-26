#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <tuple>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================================
// 1. ИСХОДНЫЕ МАТЕМАТИЧЕСКИЕ КЛАССЫ
// (Скопированы из твоего кода для точности вычислений)
// ==========================================

struct COLOR {
    Uint8 r, g, b, a;
    COLOR() : r(0), g(0), b(0), a(255) {}
    COLOR(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) 
        : r(red), g(green), b(blue), a(alpha) {}
    
    // Оператор для сравнения
    bool operator!=(const COLOR& other) const {
        return r != other.r || g != other.g || b != other.b || a != other.a;
    }
};

const COLOR LETTER_COLOR = COLOR(70, 130, 255, 255);

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
        return Matrix(cosA, sinA, 0, -sinA, cosA, 0, 0, 0, 1);
    }
    
    static Matrix Translation(float tx, float ty) {
        return Matrix(1, 0, 0, 0, 1, 0, tx, ty, 1);
    }
    
    static Matrix Scaling(float sx, float sy) {
        return Matrix(sx, 0, 0, 0, sy, 0, 0, 0, 1);
    }
    
    static Matrix WorldToScreen(float X1, float Y1, float X2, float Y2, 
                               float x1, float y1, float x2, float y2) {
        float px = (X2 - X1) / (x2 - x1);
        float py = (Y2 - Y1) / (y2 - y1);
        return Matrix(px, 0, 0, 0, -py, 0, X1 - x1 * px, Y2 + y1 * py, 1);
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
        if (h != 0) { E.x /= h; E.y /= h; }
        return E;
    }
};

// ==========================================
// 2. OPENGL УТИЛИТЫ И ШЕЙДЕРЫ
// ==========================================

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in float aWeight; // Для градиента

out vec4 vertexColor;
out float weight;

uniform vec2 screenSize;

void main() {
    // Преобразование экранных координат (0..800, 0..600) в NDC (-1..1, -1..1)
    // В SDL (0,0) сверху-слева. В OpenGL (-1,1) сверху-слева.
    float ndcX = (aPos.x / screenSize.x) * 2.0 - 1.0;
    float ndcY = 1.0 - (aPos.y / screenSize.y) * 2.0;
    
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);
    vertexColor = aColor;
    weight = aWeight;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec4 vertexColor;
in float weight;

uniform int useGradient; // 0 - обычный цвет, 1 - градиент

void main() {
    if (useGradient == 1) {
        // Логика из GradientShader CPU: (h0*1 + h1*0.5 + h2*0.5) / 1.5
        // Интерполятор сделает работу за нас, если мы передадим веса вершинам
        float gradientFactor = weight / 1.5;
        FragColor = vec4(vertexColor.rgb * gradientFactor, vertexColor.a);
    } else {
        FragColor = vertexColor;
    }
}
)";

GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

// Структура для вершин в буфере OpenGL
struct GLVertex {
    float x, y;
    float r, g, b, a;
    float weight; // Специальный вес для градиента (1.0, 0.5 или 0.5)
};

// ==========================================
// 3. ГЕНЕРАЦИЯ ГЕОМЕТРИИ
// ==========================================

class LetterGeometry {
public:
    std::vector<GLVertex> vertices;
    
    void AddTriangle(Vector v0, Vector v1, Vector v2, COLOR color, bool isGradient) {
        float r = color.r / 255.0f;
        float g = color.g / 255.0f;
        float b = color.b / 255.0f;
        float a = color.a / 255.0f;
        
        // Для GradientShader:
        // Вершина 0 (h0) -> вес 1.0
        // Вершина 1 (h1) -> вес 0.5
        // Вершина 2 (h2) -> вес 0.5
        // Это соответствует формуле: h0*1 + h1*0.5 + h2*0.5
        
        float w0 = isGradient ? 1.0f : 0.0f;
        float w1 = isGradient ? 0.5f : 0.0f;
        float w2 = isGradient ? 0.5f : 0.0f;
        
        vertices.push_back({v0.x, v0.y, r, g, b, a, w0});
        vertices.push_back({v1.x, v1.y, r, g, b, a, w1});
        vertices.push_back({v2.x, v2.y, r, g, b, a, w2});
    }
};

// ==========================================
// 4. СРАВНЕНИЕ ИЗОБРАЖЕНИЙ
// ==========================================

void CompareImages(const char* fileCPU, const char* fileGPU) {
    SDL_Surface* surfCPU = SDL_LoadBMP(fileCPU);
    SDL_Surface* surfGPU = SDL_LoadBMP(fileGPU);
    
    if (!surfCPU || !surfGPU) {
        std::cerr << "Не удалось загрузить изображения для сравнения." << std::endl;
        if(surfCPU) SDL_FreeSurface(surfCPU);
        if(surfGPU) SDL_FreeSurface(surfGPU);
        return;
    }
    
    if (surfCPU->w != surfGPU->w || surfCPU->h != surfGPU->h) {
        std::cerr << "Размеры изображений не совпадают!" << std::endl;
        SDL_FreeSurface(surfCPU);
        SDL_FreeSurface(surfGPU);
        return;
    }
    
    int width = surfCPU->w;
    int height = surfCPU->h;
    long long totalDiff = 0;
    int diffPixels = 0;
    
    SDL_Surface* surfDiff = SDL_CreateRGBSurface(0, width, height, 32, 
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        
    Uint32* pixelsCPU = (Uint32*)surfCPU->pixels;
    Uint32* pixelsGPU = (Uint32*)surfGPU->pixels;
    Uint32* pixelsDiff = (Uint32*)surfDiff->pixels;
    
    for (int i = 0; i < width * height; i++) {
        Uint8 r1, g1, b1, a1;
        Uint8 r2, g2, b2, a2;
        
        SDL_GetRGBA(pixelsCPU[i], surfCPU->format, &r1, &g1, &b1, &a1);
        SDL_GetRGBA(pixelsGPU[i], surfGPU->format, &r2, &g2, &b2, &a2);
        
        int dr = abs((int)r1 - (int)r2);
        int dg = abs((int)g1 - (int)g2);
        int db = abs((int)b1 - (int)b2);
        
        if (dr > 0 || dg > 0 || db > 0) {
            diffPixels++;
            totalDiff += (dr + dg + db);
            // Пишем в карту разницы (усиливаем разницу для наглядности)
            pixelsDiff[i] = SDL_MapRGB(surfDiff->format, 
                std::min(255, dr * 10), std::min(255, dg * 10), std::min(255, db * 10));
        } else {
            pixelsDiff[i] = SDL_MapRGB(surfDiff->format, 0, 0, 0);
        }
    }
    
    float percentDiffPixels = (float)diffPixels / (width * height) * 100.0f;
    // Максимально возможная разница на пиксель = 255 * 3
    double avgColorError = (double)totalDiff / (width * height * 3.0);
    
    std::cout << "=== Результаты сравнения ===" << std::endl;
    std::cout << "Всего пикселей: " << (width * height) << std::endl;
    std::cout << "Отличающихся пикселей: " << diffPixels << std::endl;
    std::cout << "Процент отличия (по пикселям): " << percentDiffPixels << "%" << std::endl;
    std::cout << "Средняя ошибка цвета (0-255): " << avgColorError << std::endl;
    
    SDL_SaveBMP(surfDiff, "diff_result.bmp");
    std::cout << "Карта различий сохранена в 'diff_result.bmp'" << std::endl;
    
    SDL_FreeSurface(surfCPU);
    SDL_FreeSurface(surfGPU);
    SDL_FreeSurface(surfDiff);
}

// ==========================================
// 5. MAIN
// ==========================================

int main(int argc, char* argv[]) {
    // 1. Инициализация SDL и OpenGL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    
    // Настройка атрибутов OpenGL (версия 3.3 Core)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    int width = 800;
    int height = 600;
    
    SDL_Window* window = SDL_CreateWindow("GLSL Lab", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        
    SDL_GLContext context = SDL_GL_CreateContext(window);
    
    // Инициализация GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        return 1;
    }
    
    // 2. Компиляция шейдеров
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    // 3. Подготовка геометрии (аналогично CPU коду)
    LetterGeometry batchSimple;
    LetterGeometry batchGradient;
    
    // --- Подготовка буквы R (Копируем данные из LetterR) ---
    std::vector<Vector> r_verts = {
        Vector(0.0f, 0.0f),    Vector(0.0f, 2.0f),    Vector(0.5f, 2.0f),
        Vector(0.5f, 0.0f),    Vector(0.5f, 1.5f),    Vector(0.5f, 2.0f),
        Vector(0.8f, 1.9f),    Vector(1.0f, 1.7f),    Vector(1.0f, 1.3f),
        Vector(0.8f, 1.1f),    Vector(0.5f, 1.0f),    Vector(0.5f, 1.6f),
        Vector(0.5f, 1.9f),    Vector(0.7f, 1.85f),   Vector(0.85f, 1.75f),
        Vector(0.85f, 1.45f),  Vector(0.7f, 1.35f),   Vector(0.5f, 1.3f)
    };
    std::vector<std::tuple<int, int, int>> r_tris;
    r_tris.push_back(std::make_tuple(0, 1, 2));
    r_tris.push_back(std::make_tuple(0, 2, 3));
    r_tris.push_back(std::make_tuple(4, 5, 6));
    r_tris.push_back(std::make_tuple(4, 6, 7));
    r_tris.push_back(std::make_tuple(4, 7, 8));
    r_tris.push_back(std::make_tuple(4, 8, 9));
    r_tris.push_back(std::make_tuple(4, 9, 10));
    // Черные треугольники (вырез)
    r_tris.push_back(std::make_tuple(11, 12, 13));
    r_tris.push_back(std::make_tuple(11, 13, 14));
    r_tris.push_back(std::make_tuple(11, 14, 15));
    r_tris.push_back(std::make_tuple(11, 15, 16));
    r_tris.push_back(std::make_tuple(11, 16, 17));

    // Матрица трансформации для R
    Matrix WS = Matrix::WorldToScreen(100, 100, width - 100, height - 100, -3, -2, 3, 2);
    Matrix transformR = Matrix::Translation(-1.5f, 0.0f) * Matrix::Scaling(0.8f, 0.8f) * WS;
    
    for(size_t i=0; i<r_tris.size(); ++i) {
        int i0, i1, i2;
        std::tie(i0, i1, i2) = r_tris[i];
        Vector v0 = r_verts[i0] * transformR;
        Vector v1 = r_verts[i1] * transformR;
        Vector v2 = r_verts[i2] * transformR;
        
        if (i < 7) {
            batchSimple.AddTriangle(v0, v1, v2, LETTER_COLOR, false);
        } else {
            // "Дырка" рисуется черным поверх
            batchSimple.AddTriangle(v0, v1, v2, COLOR(0,0,0,255), false);
        }
    }

    // --- Подготовка буквы A (Копируем данные из LetterA) ---
    std::vector<Vector> a_verts = {
        Vector(0.0f, 0.0f), Vector(0.3f, 2.0f), Vector(0.5f, 2.0f), Vector(0.2f, 0.0f),
        Vector(0.5f, 2.0f), Vector(0.7f, 2.0f), Vector(1.0f, 0.0f), Vector(0.8f, 0.0f),
        Vector(0.3f, 1.0f), Vector(0.7f, 1.0f), Vector(0.7f, 0.8f), Vector(0.3f, 0.8f)
    };
    std::vector<std::tuple<int, int, int>> a_tris;
    a_tris.push_back(std::make_tuple(0, 1, 2));
    a_tris.push_back(std::make_tuple(0, 2, 3));
    a_tris.push_back(std::make_tuple(4, 5, 6));
    a_tris.push_back(std::make_tuple(4, 6, 7));
    a_tris.push_back(std::make_tuple(8, 9, 10));
    a_tris.push_back(std::make_tuple(8, 10, 11));

    Matrix transformA = Matrix::Translation(1.0f, 0.0f) * Matrix::Scaling(0.7f, 0.7f) * WS;
    
    for(size_t i=0; i<a_tris.size(); ++i) {
        int i0, i1, i2;
        std::tie(i0, i1, i2) = a_tris[i];
        Vector v0 = a_verts[i0] * transformA;
        Vector v1 = a_verts[i1] * transformA;
        Vector v2 = a_verts[i2] * transformA;
        
        // В LetterA:
        // i < 2 -> Gradient
        // i < 4 -> Gradient (так как else if)
        // else (i >= 4) -> Simple
        if (i < 4) {
            batchGradient.AddTriangle(v0, v1, v2, LETTER_COLOR, true);
        } else {
            batchSimple.AddTriangle(v0, v1, v2, LETTER_COLOR, false);
        }
    }

    // 4. Загрузка данных в GPU
    GLuint VAO[2], VBO[2];
    glGenVertexArrays(2, VAO);
    glGenBuffers(2, VBO);

    // -- Batch Simple --
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, batchSimple.vertices.size() * sizeof(GLVertex), 
                 batchSimple.vertices.data(), GL_STATIC_DRAW);
    
    // Pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    // Weight
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    // -- Batch Gradient --
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, batchGradient.vertices.size() * sizeof(GLVertex), 
                 batchGradient.vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(GLVertex), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    // 5. Рендеринг
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(shaderProgram);
    GLint locScreen = glGetUniformLocation(shaderProgram, "screenSize");
    glUniform2f(locScreen, (float)width, (float)height);
    
    GLint locUseGrad = glGetUniformLocation(shaderProgram, "useGradient");
    
    // Рисуем простые треугольники (R и часть A)
    // Важно: Порядок рисования. В CPU коде сначала R, потом A.
    // Но внутри R есть черные треугольники, которые перекрывают синие.
    // Здесь мы разделили на батчи по шейдерам. 
    // Для идеального совпадения нужно было бы рисовать в том же хронологическом порядке.
    // В данном случае R (простой цвет) + R (черный цвет) + A (простой цвет) попали в batchSimple.
    // A (градиент) попали в batchGradient.
    // Порядок в векторе batchSimple сохранился (добавляли последовательно), 
    // поэтому Z-буфер не обязателен, если рисовать последовательно (Painter's algorithm).
    
    // Однако, в CPU коде сначала рисуется R (весь простой, включая черные дырки), потом A.
    // В A сначала рисуется Градиент, потом Простой.
    // Поэтому порядок отрисовки GPU должен быть:
    // 1. R (simple) -> уже в batchSimple
    // 2. A (gradient) -> batchGradient
    // 3. A (simple) -> уже в batchSimple (добавлен после R)
    
    // Чтобы соблюсти порядок, придется разделить batchSimple. 
    // Но так как R и A не пересекаются в пространстве экрана, порядок между буквами не важен.
    // Важен порядок внутри букв.
    // R: синий -> черный (OK, в batchSimple они лежат по порядку).
    // A: градиент -> синий. (Мы должны нарисовать batchGradient, а потом часть batchSimple, относящуюся к A).
    
    // Упростим: нарисуем batchSimple полностью (R + хвост A), затем batchGradient (тело A).
    // Так как тело A и хвост A не перекрываются (это разные части буквы), порядок не критичен.
    // Но лучше сделаем как было:
    
    // Рисуем простые (включает всю R и ножки A)
    glUniform1i(locUseGrad, 0);
    glBindVertexArray(VAO[0]);
    glDrawArrays(GL_TRIANGLES, 0, batchSimple.vertices.size());
    
    // Рисуем градиентные (верх A)
    glUniform1i(locUseGrad, 1);
    glBindVertexArray(VAO[1]);
    glDrawArrays(GL_TRIANGLES, 0, batchGradient.vertices.size());

    // 6. Сохранение результата (ReadPixels)
    std::vector<unsigned char> pixels(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    
    // Переворот изображения по Y (GL -> SDL/BMP)
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 
                                               0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    
    Uint32* targetPixels = (Uint32*)surface->pixels;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int glY = height - 1 - y; // инверсия Y
            int idx = (glY * width + x) * 4;
            // GL_RGBA
            Uint8 r = pixels[idx];
            Uint8 g = pixels[idx+1];
            Uint8 b = pixels[idx+2];
            // BMP SDL обычно требует Alpha 255
            targetPixels[y * width + x] = SDL_MapRGB(surface->format, r, g, b);
        }
    }
    
    SDL_SaveBMP(surface, "lab3_gpu_result.bmp");
    std::cout << "GPU результат сохранен в lab3_gpu_result.bmp" << std::endl;
    SDL_FreeSurface(surface);
    
    // 7. Сравнение с CPU версией
    CompareImages("lab3_cpu_result.bmp", "lab3_gpu_result.bmp");

    // Очистка
    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(2, VBO);
    glDeleteProgram(shaderProgram);
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}