#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <memory>

const int INITIAL_WIDTH = 800;
const int INITIAL_HEIGHT = 600;
const int MIN_RESOLUTION = 10;
const int MAX_RESOLUTION = 2000;

struct COLOR {
    Uint8 r, g, b, a;
    
    COLOR() : r(0), g(0), b(0), a(255) {}
    COLOR(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) 
        : r(red), g(green), b(blue), a(alpha) {}
};

class Frame {
    int width, height;
    std::vector<COLOR> pixels;

public:
    Frame(int w, int h) : width(w), height(h), pixels(w * h) {}

    void Resize(int w, int h) {
        width = w;
        height = h;
        pixels.resize(w * h);
    }

    void SetPixel(int x, int y, COLOR color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (color.a == 255) {
                pixels[y * width + x] = color;
            } else {
                COLOR dest = pixels[y * width + x];
                float alpha = color.a / 255.0f;
                float inv_alpha = 1.0f - alpha;
                pixels[y * width + x] = {
                    static_cast<Uint8>(color.r * alpha + dest.r * inv_alpha),
                    static_cast<Uint8>(color.g * alpha + dest.g * inv_alpha),
                    static_cast<Uint8>(color.b * alpha + dest.b * inv_alpha),
                    255
                };
            }
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

    void DrawLine(int x1, int y1, int x2, int y2, COLOR color) {
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            SetPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

    void DrawThickLine(int x1, int y1, int x2, int y2, COLOR color, int thickness) {
        if (thickness <= 1) {
            DrawLine(x1, y1, x2, y2, color);
            return;
        }
        
        float dx = x2 - x1;
        float dy = y2 - y1;
        float length = sqrt(dx*dx + dy*dy);
        if (length < 0.001f) return;
        
        dx /= length;
        dy /= length;
        
        float px = -dy;
        float py = dx;
        
        int half = thickness / 2;
        for (int i = -half; i <= half; i++) {
            int offsetX = static_cast<int>(px * i);
            int offsetY = static_cast<int>(py * i);
            DrawLine(x1 + offsetX, y1 + offsetY, x2 + offsetX, y2 + offsetY, color);
        }
    }

    void DrawCircle(int x0, int y0, int radius, COLOR color) {
        if (radius <= 0) return;
        
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius;
        
        auto drawCirclePoints = [&](int x, int y) {
            SetPixel(x0 + x, y0 + y, color);
            SetPixel(x0 - x, y0 + y, color);
            SetPixel(x0 + x, y0 - y, color);
            SetPixel(x0 - x, y0 - y, color);
            SetPixel(x0 + y, y0 + x, color);
            SetPixel(x0 - y, y0 + x, color);
            SetPixel(x0 + y, y0 - x, color);
            SetPixel(x0 - y, y0 - x, color);
        };
        
        while (y >= x) {
            drawCirclePoints(x, y);
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
        }
    }

    void DrawThickCircle(int x0, int y0, int radius, COLOR color, int thickness) {
        for (int i = 0; i < thickness; i++) {
            DrawCircle(x0, y0, radius + i, color);
        }
    }

    class BarycentricInterpolator {
        float x0, y0, x1, y1, x2, y2, S;
        COLOR C0, C1, C2;

    public:
        BarycentricInterpolator(float _x0, float _y0, float _x1, float _y1, 
                               float _x2, float _y2, COLOR A0, COLOR A1, COLOR A2) 
            : x0(_x0), y0(_y0), x1(_x1), y1(_y1), x2(_x2), y2(_y2),
              S((_y1 - _y2) * (_x0 - _x2) + (_x2 - _x1) * (_y0 - _y2)),
              C0(A0), C1(A1), C2(A2) {}

        COLOR getColor(float x, float y) {
            float h0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / S;
            float h1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / S;
            float h2 = 1.0f - h0 - h1;
            
            return COLOR(
                static_cast<Uint8>(h0 * C0.r + h1 * C1.r + h2 * C2.r),
                static_cast<Uint8>(h0 * C0.g + h1 * C1.g + h2 * C2.g),
                static_cast<Uint8>(h0 * C0.b + h1 * C1.b + h2 * C2.b),
                static_cast<Uint8>(h0 * C0.a + h1 * C1.a + h2 * C2.a)
            );
        }
    };

    template <class ShaderClass>
    void DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, ShaderClass& shader) {
        if (y1 < y0) { std::swap(y1, y0); std::swap(x1, x0); }
        if (y2 < y1) { std::swap(y2, y1); std::swap(x2, x1); }
        if (y1 < y0) { std::swap(y1, y0); std::swap(x1, x0); }

        int Y0 = static_cast<int>(y0 + 0.5f);
        int Y1 = static_cast<int>(y1 + 0.5f);
        int Y2 = static_cast<int>(y2 + 0.5f);

        Y0 = std::max(0, std::min(Y0, height - 1));
        Y1 = std::max(0, std::min(Y1, height - 1));
        Y2 = std::max(0, std::min(Y2, height - 1));

        for (int y = Y0; y < Y1; y++) {
            float fy = y + 0.5f;
            int X0 = static_cast<int>((fy - y0) / (y1 - y0) * (x1 - x0) + x0 + 0.5f);
            int X1 = static_cast<int>((fy - y0) / (y2 - y0) * (x2 - x0) + x0 + 0.5f);
            
            if (X0 > X1) std::swap(X0, X1);
            X0 = std::max(0, std::min(X0, width - 1));
            X1 = std::max(0, std::min(X1, width - 1));

            for (int x = X0; x < X1; x++) {
                SetPixel(x, y, shader.getColor(x + 0.5f, fy));
            }
        }

        for (int y = Y1; y < Y2; y++) {
            float fy = y + 0.5f;
            int X0 = static_cast<int>((fy - y1) / (y2 - y1) * (x2 - x1) + x1 + 0.5f);
            int X1 = static_cast<int>((fy - y0) / (y2 - y0) * (x2 - x0) + x0 + 0.5f);
            
            if (X0 > X1) std::swap(X0, X1);
            X0 = std::max(0, std::min(X0, width - 1));
            X1 = std::max(0, std::min(X1, width - 1));

            for (int x = X0; x < X1; x++) {
                SetPixel(x, y, shader.getColor(x + 0.5f, fy));
            }
        }
    }

    void FillCircle(int x0, int y0, int radius, COLOR color) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    SetPixel(x0 + x, y0 + y, color);
                }
            }
        }
    }

    void FillCircleRadial(int x0, int y0, int radius, COLOR centerColor, COLOR edgeColor) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    float distance = sqrt(x*x + y*y) / radius;
                    COLOR interpolated = InterpolateColor(centerColor, edgeColor, distance);
                    SetPixel(x0 + x, y0 + y, interpolated);
                }
            }
        }
    }

    void FillCircleSector(int x0, int y0, int radius) {
        std::vector<COLOR> sectorColors = {
            COLOR(255, 100, 100),    // Красный
            COLOR(255, 200, 100),    // Оранжевый
            COLOR(255, 255, 100),    // Желтый
            COLOR(100, 255, 100),    // Зеленый
            COLOR(100, 200, 255),    // Голубой
            COLOR(100, 100, 255),    // Синий
            COLOR(200, 100, 255)     // Фиолетовый
        };

        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    float angle = atan2(y, x);
                    if (angle < 0) angle += 2 * M_PI;
                    int sector = static_cast<int>((angle / (2 * M_PI)) * sectorColors.size()) % sectorColors.size();
                    SetPixel(x0 + x, y0 + y, sectorColors[sector]);
                }
            }
        }
    }

private:
    COLOR InterpolateColor(COLOR c1, COLOR c2, float t) {
        return COLOR(
            static_cast<Uint8>(c1.r * (1-t) + c2.r * t),
            static_cast<Uint8>(c1.g * (1-t) + c2.g * t),
            static_cast<Uint8>(c1.b * (1-t) + c2.b * t),
            static_cast<Uint8>(c1.a * (1-t) + c2.a * t)
        );
    }
};

class Shape {
public:
    virtual ~Shape() = default;
    virtual void Draw(Frame& frame) = 0;
    virtual bool Contains(int x, int y) = 0;
    virtual float GetArea() = 0;
    
    COLOR color = COLOR(70, 130, 180, 200);
    bool selected = false;
    int fillType = 0;
    float alpha = 0.8f;
    int shapeId = -1; 
};

class Circle : public Shape {
public:
    int x, y, radius;
    
    Circle(int x, int y, int radius, int id) : x(x), y(y), radius(radius) {
        shapeId = id;
    }
    
    void Draw(Frame& frame) override {
        COLOR drawColor = color;
        drawColor.a = static_cast<Uint8>(alpha * 255);
        
        switch (fillType) {
            case 0:
                frame.FillCircle(x, y, radius, drawColor);
                break;
            case 1:
                frame.FillCircleRadial(x, y, radius, 
                    COLOR(173, 216, 230, static_cast<Uint8>(alpha * 255)), drawColor);
                break;
            case 2:
                frame.FillCircleSector(x, y, radius);
                break;
        }
        
        if (selected) {
            frame.DrawThickCircle(x, y, radius, COLOR(0, 0, 0, 255), 5);
        }
    }
    
    bool Contains(int testX, int testY) override {
        int dx = x - testX;
        int dy = y - testY;
        return dx * dx + dy * dy <= radius * radius;
    }
    
    float GetArea() override {
        return M_PI * radius * radius;
    }
};

class Triangle : public Shape {
public:
    float x1, y1, x2, y2, x3, y3;
    
    Triangle(float x1, float y1, float x2, float y2, float x3, float y3, int id) 
        : x1(x1), y1(y1), x2(x2), y2(y2), x3(x3), y3(y3) {
        shapeId = id;
    }
    
    void Draw(Frame& frame) override {
        COLOR drawColor = color;
        drawColor.a = static_cast<Uint8>(alpha * 255);
        
        COLOR vertexColors[3];
        
        switch (fillType) {
            case 0:
                // Однородная заливка
                vertexColors[0] = vertexColors[1] = vertexColors[2] = drawColor;
                break;
            case 1:
                // Градиентная заливка
                vertexColors[0] = COLOR(255, 100, 100, static_cast<Uint8>(alpha * 255));
                vertexColors[1] = COLOR(100, 255, 100, static_cast<Uint8>(alpha * 255));
                vertexColors[2] = COLOR(100, 100, 255, static_cast<Uint8>(alpha * 255));
                break;
            case 2:
                // Секторная заливка
                vertexColors[0] = COLOR(255, 255, 100, static_cast<Uint8>(alpha * 255));
                vertexColors[1] = COLOR(100, 255, 255, static_cast<Uint8>(alpha * 255));
                vertexColors[2] = COLOR(255, 100, 255, static_cast<Uint8>(alpha * 255));
                break;
        }
        
        Frame::BarycentricInterpolator interpolator(
            x1, y1, x2, y2, x3, y3,
            vertexColors[0], vertexColors[1], vertexColors[2]
        );
        
        frame.DrawTriangle(x1, y1, x2, y2, x3, y3, interpolator);
        
        if (selected) {
            frame.DrawThickLine(static_cast<int>(x1), static_cast<int>(y1), 
                              static_cast<int>(x2), static_cast<int>(y2), 
                              COLOR(0, 0, 0, 255), 5);
            frame.DrawThickLine(static_cast<int>(x2), static_cast<int>(y2), 
                              static_cast<int>(x3), static_cast<int>(y3), 
                              COLOR(0, 0, 0, 255), 5);
            frame.DrawThickLine(static_cast<int>(x3), static_cast<int>(y3), 
                              static_cast<int>(x1), static_cast<int>(y1), 
                              COLOR(0, 0, 0, 255), 5);
        }
    }
    
    bool Contains(int testX, int testY) override {
        float denominator = ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
        if (fabs(denominator) < 0.0001f) return false;
        
        float a = ((y2 - y3) * (testX - x3) + (x3 - x2) * (testY - y3)) / denominator;
        float b = ((y3 - y1) * (testX - x3) + (x1 - x3) * (testY - y3)) / denominator;
        float c = 1.0f - a - b;
        
        return a >= 0 && a <= 1 && b >= 0 && b <= 1 && c >= 0 && c <= 1;
    }
    
    float GetArea() override {
        return fabs((x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0f);
    }
};

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Не удалось инициализировать SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Лабораторная работа №2 - Растровая заливка", 
                                         SDL_WINDOWPOS_CENTERED, 
                                         SDL_WINDOWPOS_CENTERED,
                                         INITIAL_WIDTH, INITIAL_HEIGHT, 
                                         SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Не удалось создать окно: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
                                               SDL_RENDERER_ACCELERATED | 
                                               SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("Не удалось создать рендерер: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
    if (!format) {
        SDL_Log("Не удалось получить формат пикселей: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int logicalWidth = 500;
    int logicalHeight = 500;
    Frame frame(logicalWidth, logicalHeight);

    SDL_Texture* texture = SDL_CreateTexture(renderer, 
                                            SDL_PIXELFORMAT_RGBA32, 
                                            SDL_TEXTUREACCESS_STREAMING,
                                            logicalWidth, logicalHeight);
    if (!texture) {
        SDL_Log("Не удалось создать текстуру: %s", SDL_GetError());
        SDL_FreeFormat(format);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<std::unique_ptr<Shape>> shapes;
    int selectedShapeId = -1;
    float globalRotation = 0.0f;
    const float rotationSpeed = 0.02f;

    // Хранилище состояний фигур (чтобы не терять при пересоздании)
    struct ShapeState {
        bool selected = false;
        int fillType = 0;
        float alpha = 0.8f;
    };
    
    std::vector<ShapeState> shapeStates(9); 

    auto updateWindowTitle = [window, &logicalWidth, &logicalHeight, &selectedShapeId, &shapeStates]() {
        std::string title = "Лаб. работа №2 - Разрешение: " + 
                           std::to_string(logicalWidth) + "x" + 
                           std::to_string(logicalHeight);
        
        if (selectedShapeId != -1) {
            title += " - Фигура: " + std::to_string(selectedShapeId);
            title += " - Заливка: " + std::to_string(shapeStates[selectedShapeId].fillType);
            title += " - Прозрачность: " + std::to_string(static_cast<int>(shapeStates[selectedShapeId].alpha * 100)) + "%";
        }
        SDL_SetWindowTitle(window, title.c_str());
    };

    updateWindowTitle();

    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_F2) {
                    logicalWidth = std::max(MIN_RESOLUTION, logicalWidth - 10);
                    logicalHeight = logicalWidth;
                    frame.Resize(logicalWidth, logicalHeight);
                    SDL_DestroyTexture(texture);
                    texture = SDL_CreateTexture(renderer, 
                                               SDL_PIXELFORMAT_RGBA32, 
                                               SDL_TEXTUREACCESS_STREAMING,
                                               logicalWidth, logicalHeight);
                    updateWindowTitle();
                } else if (event.key.keysym.sym == SDLK_F3) {
                    logicalWidth = std::min(MAX_RESOLUTION, logicalWidth + 10);
                    logicalHeight = logicalWidth;
                    frame.Resize(logicalWidth, logicalHeight);
                    SDL_DestroyTexture(texture);
                    texture = SDL_CreateTexture(renderer, 
                                               SDL_PIXELFORMAT_RGBA32, 
                                               SDL_TEXTUREACCESS_STREAMING,
                                               logicalWidth, logicalHeight);
                    updateWindowTitle();
                } else if (event.key.keysym.sym == SDLK_1 || event.key.keysym.sym == SDLK_2 || event.key.keysym.sym == SDLK_3) {
                    // Изменение типа заливки для выделенных фигур
                    int newFillType = event.key.keysym.sym - SDLK_1;
                    for (auto& state : shapeStates) {
                        if (state.selected) {
                            state.fillType = newFillType;
                        }
                    }
                    updateWindowTitle();
                } else if ((event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS)) {
                    // Увеличение прозрачности для выделенных фигур
                    for (auto& state : shapeStates) {
                        if (state.selected) {
                            state.alpha = std::min(state.alpha + 0.1f, 1.0f);
                        }
                    }
                    updateWindowTitle();
                } else if ((event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)) {
                    // Уменьшение прозрачности для выделенных фигур
                    for (auto& state : shapeStates) {
                        if (state.selected) {
                            state.alpha = std::max(state.alpha - 0.1f, 0.1f);
                        }
                    }
                    updateWindowTitle();
                } else if (event.key.keysym.sym == SDLK_c) {
                    // Сброс выделения
                    selectedShapeId = -1;
                    for (auto& state : shapeStates) {
                        state.selected = false;
                    }
                    updateWindowTitle();
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                
                int windowWidth, windowHeight;
                SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                int viewportSize = (7 * std::min(windowWidth, windowHeight)) / 8;
                int viewportX = (windowWidth - viewportSize) / 2;
                int viewportY = (windowHeight - viewportSize) / 2;
                
                if (mouseX < viewportX || mouseX >= viewportX + viewportSize ||
                    mouseY < viewportY || mouseY >= viewportY + viewportSize) {
                    continue;
                }
                
                float scaleX = static_cast<float>(logicalWidth) / viewportSize;
                float scaleY = static_cast<float>(logicalHeight) / viewportSize;
                
                int logicalX = static_cast<int>((mouseX - viewportX) * scaleX);
                int logicalY = static_cast<int>((mouseY - viewportY) * scaleY);

                // Сброс предыдущего выделения
                selectedShapeId = -1;
                for (auto& state : shapeStates) {
                    state.selected = false;
                }

                // Проверка попадания в фигуры (от задних к передним)
                for (int i = shapes.size() - 1; i >= 0; --i) {
                    if (shapes[i]->Contains(logicalX, logicalY)) {
                        selectedShapeId = shapes[i]->shapeId;
                        shapeStates[selectedShapeId].selected = true;
                        break;
                    }
                }
                updateWindowTitle();
            }
        }

        globalRotation += rotationSpeed;
        if (globalRotation > 2 * M_PI) {
            globalRotation -= 2 * M_PI;
        }

        shapes.clear();

        int centerX = logicalWidth / 2;
        int centerY = logicalHeight / 2;
        int circleRadius = (std::min(logicalWidth, logicalHeight) / 2) * 7 / 8;

        int triangleCount = 8;
        int triangleBaseDistance = circleRadius * 0.5;

        // Создаем треугольники
        for (int i = 0; i < triangleCount; i++) {
            float angle = i * (2 * M_PI / triangleCount) + globalRotation;

            float x1 = centerX + circleRadius * cos(angle);
            float y1 = centerY + circleRadius * sin(angle);
            float x2 = centerX + triangleBaseDistance * cos(angle - 0.2f);
            float y2 = centerY + triangleBaseDistance * sin(angle - 0.2f);
            float x3 = centerX + triangleBaseDistance * cos(angle + 0.2f);
            float y3 = centerY + triangleBaseDistance * sin(angle + 0.2f);

            auto triangle = std::unique_ptr<Shape>(new Triangle(x1, y1, x2, y2, x3, y3, i));
            triangle->color = COLOR(150, 200, 255, 200);
            triangle->fillType = shapeStates[i].fillType;
            triangle->alpha = shapeStates[i].alpha;
            triangle->selected = shapeStates[i].selected;
            
            shapes.push_back(std::move(triangle));
        }

        // Создаем центральный круг
        auto circle = std::unique_ptr<Shape>(new Circle(centerX, centerY, circleRadius / 3, triangleCount));
        circle->color = COLOR(70, 130, 180, 200);
        circle->fillType = shapeStates[triangleCount].fillType;
        circle->alpha = shapeStates[triangleCount].alpha;
        circle->selected = shapeStates[triangleCount].selected;
        
        shapes.push_back(std::move(circle));

        // Сортируем фигуры по площади (от большей к меньшей)
        std::sort(shapes.begin(), shapes.end(), 
            [](const std::unique_ptr<Shape>& a, const std::unique_ptr<Shape>& b) {
                return a->GetArea() > b->GetArea();
            });

        // Очищаем буфер белым цветом
        frame.Clear(COLOR(255, 255, 255, 255));

        // Рисуем внешний круг
        frame.DrawCircle(centerX, centerY, circleRadius, COLOR(0, 0, 0, 255));

        // Рисуем все фигуры
        for (auto& shape : shapes) {
            shape->Draw(frame);
        }

        // Обновляем текстуру
        void* texturePixels;
        int pitch;
        SDL_LockTexture(texture, NULL, &texturePixels, &pitch);
        Uint32* texPixels = static_cast<Uint32*>(texturePixels);

        for (int y = 0; y < logicalHeight; y++) {
            for (int x = 0; x < logicalWidth; x++) {
                COLOR color = frame.GetPixel(x, y);
                texPixels[y * (pitch / sizeof(Uint32)) + x] = 
                    SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
            }
        }
        SDL_UnlockTexture(texture);

        SDL_RenderClear(renderer);

        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        int viewportSize = (7 * std::min(windowWidth, windowHeight)) / 8;
        int viewportX = (windowWidth - viewportSize) / 2;
        int viewportY = (windowHeight - viewportSize) / 2;
        SDL_Rect viewport = {viewportX, viewportY, viewportSize, viewportSize};
        SDL_RenderSetViewport(renderer, &viewport);

        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < 33) {
            SDL_Delay(33 - frameTime);
        }
    }

    SDL_FreeFormat(format);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}