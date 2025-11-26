#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <memory>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int INITIAL_WIDTH = 800;
const int INITIAL_HEIGHT = 600;

struct COLOR {
    Uint8 r, g, b, a;
    
    COLOR() : r(0), g(0), b(0), a(255) {}
    COLOR(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) 
        : r(red), g(green), b(blue), a(alpha) {}
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

class SimpleShader {
    COLOR baseColor;
    
public:
    SimpleShader(COLOR color) : baseColor(color) {}
    
    COLOR getColor(float x, float y, float h0, float h1, float h2) {
        return baseColor;
    }
};

class GradientShader {
    COLOR baseColor;
    
public:
    GradientShader(COLOR color) : baseColor(color) {}
    
    COLOR getColor(float x, float y, float h0, float h1, float h2) {
        float gradient = (h0 + h1 * 0.5f + h2 * 0.5f) / 1.5f;
        return COLOR(
            static_cast<Uint8>(baseColor.r * gradient),
            static_cast<Uint8>(baseColor.g * gradient),
            static_cast<Uint8>(baseColor.b * gradient),
            baseColor.a
        );
    }
};

class Frame {
    int width, height;
    std::vector<COLOR> pixels;

public:
    Frame(int w, int h) : width(w), height(h), pixels(w * h) {}

    void SetPixel(int x, int y, COLOR color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            pixels[y * width + x] = color;
        }
    }

    COLOR GetPixel(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return pixels[y * width + x];
        }
        return COLOR(0, 0, 0, 0);
    }

    void Clear(COLOR color) {
        std::fill(pixels.begin(), pixels.end(), color);
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    class BarycentricInterpolator {
        float x0, y0, x1, y1, x2, y2, S;

    public:
        BarycentricInterpolator(float _x0, float _y0, float _x1, float _y1, 
                               float _x2, float _y2) 
            : x0(_x0), y0(_y0), x1(_x1), y1(_y1), x2(_x2), y2(_y2),
              S((_y1 - _y2) * (_x0 - _x2) + (_x2 - _x1) * (_y0 - _y2)) {}

        void getWeights(float x, float y, float& h0, float& h1, float& h2) {
            h0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / S;
            h1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / S;
            h2 = 1.0f - h0 - h1;
        }
    };

    template <class ShaderClass>
    void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, ShaderClass&& shader) {
        BarycentricInterpolator interpolator(x0, y0, x1, y1, x2, y2);
        
        float minX = std::min({x0, x1, x2});
        float maxX = std::max({x0, x1, x2});
        float minY = std::min({y0, y1, y2});
        float maxY = std::max({y0, y1, y2});
        
        int startX = std::max(0, static_cast<int>(minX));
        int endX = std::min(width - 1, static_cast<int>(maxX));
        int startY = std::max(0, static_cast<int>(minY));
        int endY = std::min(height - 1, static_cast<int>(maxY));
        
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                float h0, h1, h2;
                interpolator.getWeights(x + 0.5f, y + 0.5f, h0, h1, h2);
                
                if (h0 >= -1e-6f && h1 >= -1e-6f && h2 >= -1e-6f) {
                    COLOR color = shader.getColor(x + 0.5f, y + 0.5f, h0, h1, h2);
                    SetPixel(x, y, color);
                }
            }
        }
    }

    // Метод для сохранения Frame в SDL_Surface
    SDL_Surface* CreateSurface() {
        SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 
                                                   0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (!surface) return nullptr;

        Uint32* pixels = static_cast<Uint32*>(surface->pixels);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                COLOR color = GetPixel(x, y);
                pixels[y * surface->pitch / 4 + x] = 
                    SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
            }
        }

        return surface;
    }
};

class LetterR {
private:
    std::vector<Vector> vertices;
    std::vector<std::tuple<int, int, int>> triangles;
    
public:
    LetterR() {
        vertices = {
            Vector(0.0f, 0.0f),    Vector(0.0f, 2.0f),    Vector(0.5f, 2.0f),    
            Vector(0.5f, 0.0f),    Vector(0.5f, 1.5f),    Vector(0.5f, 2.0f),    
            Vector(0.8f, 1.9f),    Vector(1.0f, 1.7f),    Vector(1.0f, 1.3f),    
            Vector(0.8f, 1.1f),    Vector(0.5f, 1.0f),    Vector(0.5f, 1.6f),    
            Vector(0.5f, 1.9f),    Vector(0.7f, 1.85f),   Vector(0.85f, 1.75f),  
            Vector(0.85f, 1.45f),  Vector(0.7f, 1.35f),   Vector(0.5f, 1.3f)     
        };
        
        triangles.push_back(std::make_tuple(0, 1, 2));
        triangles.push_back(std::make_tuple(0, 2, 3));
        triangles.push_back(std::make_tuple(4, 5, 6));
        triangles.push_back(std::make_tuple(4, 6, 7));
        triangles.push_back(std::make_tuple(4, 7, 8));
        triangles.push_back(std::make_tuple(4, 8, 9));
        triangles.push_back(std::make_tuple(4, 9, 10));
        triangles.push_back(std::make_tuple(11, 12, 13));
        triangles.push_back(std::make_tuple(11, 13, 14));
        triangles.push_back(std::make_tuple(11, 14, 15));
        triangles.push_back(std::make_tuple(11, 15, 16));
        triangles.push_back(std::make_tuple(11, 16, 17));
    }
    
    void Draw(Frame& frame, const Matrix& transform) {
        std::vector<Vector> transformedVertices;
        for (const auto& v : vertices) {
            transformedVertices.push_back(v * transform);
        }
        
        for (size_t i = 0; i < triangles.size(); i++) {
            int i0, i1, i2;
            std::tie(i0, i1, i2) = triangles[i];
            
            Vector v0 = transformedVertices[i0];
            Vector v1 = transformedVertices[i1];
            Vector v2 = transformedVertices[i2];
            
            if (i < 7) {
                SimpleShader shader(LETTER_COLOR);
                frame.DrawTriangle(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, shader);
            } else {
                SimpleShader shader(COLOR(0, 0, 0, 255));
                frame.DrawTriangle(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, shader);
            }
        }
    }
};

class LetterA {
private:
    std::vector<Vector> vertices;
    std::vector<std::tuple<int, int, int>> triangles;
    
public:
    LetterA() {
        vertices = {
            Vector(0.0f, 0.0f), Vector(0.3f, 2.0f), Vector(0.5f, 2.0f), Vector(0.2f, 0.0f),
            Vector(0.5f, 2.0f), Vector(0.7f, 2.0f), Vector(1.0f, 0.0f), Vector(0.8f, 0.0f),
            Vector(0.3f, 1.0f), Vector(0.7f, 1.0f), Vector(0.7f, 0.8f), Vector(0.3f, 0.8f)
        };
        
        triangles.push_back(std::make_tuple(0, 1, 2));
        triangles.push_back(std::make_tuple(0, 2, 3));
        triangles.push_back(std::make_tuple(4, 5, 6));
        triangles.push_back(std::make_tuple(4, 6, 7));
        triangles.push_back(std::make_tuple(8, 9, 10));
        triangles.push_back(std::make_tuple(8, 10, 11));
    }
    
    void Draw(Frame& frame, const Matrix& transform) {
        std::vector<Vector> transformedVertices;
        for (const auto& v : vertices) {
            transformedVertices.push_back(v * transform);
        }
        
        for (size_t i = 0; i < triangles.size(); i++) {
            int i0, i1, i2;
            std::tie(i0, i1, i2) = triangles[i];
            
            Vector v0 = transformedVertices[i0];
            Vector v1 = transformedVertices[i1];
            Vector v2 = transformedVertices[i2];
            
            if (i < 2) {
                GradientShader shader(LETTER_COLOR);
                frame.DrawTriangle(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, shader);
            } else if (i < 4) {
                GradientShader shader(LETTER_COLOR);
                frame.DrawTriangle(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, shader);
            } else {
                SimpleShader shader(LETTER_COLOR);
                frame.DrawTriangle(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y, shader);
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Не удалось инициализировать SDL: %s", SDL_GetError());
        return 1;
    }

    int logicalWidth = 800;
    int logicalHeight = 600;
    Frame frame(logicalWidth, logicalHeight);

    LetterR letterR;
    LetterA letterA;

    // Черный фон
    frame.Clear(COLOR(0, 0, 0, 255));

    // Матрица для преобразования мировых координат в экранные
    Matrix WS = Matrix::WorldToScreen(100, 100, logicalWidth - 100, logicalHeight - 100, 
                                     -3, -2, 3, 2);

    // Статичное положение буквы "Р"
    Matrix transformR = Matrix::Translation(-1.5f, 0.0f) * 
                      Matrix::Scaling(0.8f, 0.8f) * 
                      WS;
    letterR.Draw(frame, transformR);

    // Статичное положение буквы "А"
    Matrix transformA = Matrix::Translation(1.0f, 0.0f) * 
                      Matrix::Scaling(0.7f, 0.7f) * 
                      WS;
    letterA.Draw(frame, transformA);

    // Сохраняем в BMP файл напрямую из Frame
    SDL_Surface* surface = frame.CreateSurface();
    if (surface) {
        SDL_SaveBMP(surface, "lab3_cpu_result.bmp");
        SDL_FreeSurface(surface);
        SDL_Log("Изображение сохранено в lab3_cpu_result.bmp");
    } else {
        SDL_Log("Ошибка создания поверхности SDL");
    }

    SDL_Quit();

    return 0;
}