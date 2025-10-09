#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int INITIAL_WIDTH = 800;
const int INITIAL_HEIGHT = 600;
const int MIN_RESOLUTION = 10;
const int MAX_RESOLUTION = 2000;

// Глобальные переменные для вращения
float globalRotation = 0.0f;
const float rotationSpeed = 0.01f;

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
        
        auto drawCirclePoints = [&](int x_pos, int y_pos) {
            SetPixel(x0 + x_pos, y0 + y_pos, color);
            SetPixel(x0 - x_pos, y0 + y_pos, color);
            SetPixel(x0 + x_pos, y0 - y_pos, color);
            SetPixel(x0 - x_pos, y0 - y_pos, color);
            SetPixel(x0 + y_pos, y0 + x_pos, color);
            SetPixel(x0 - y_pos, y0 + x_pos, color);
            SetPixel(x0 + y_pos, y0 - x_pos, color);
            SetPixel(x0 - y_pos, y0 - x_pos, color);
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
    
    void FillRing(int x0, int y0, int outerRadius, int innerRadius, COLOR color) {
        for (int y = -outerRadius; y <= outerRadius; y++) {
            for (int x = -outerRadius; x <= outerRadius; x++) {
                float distance = sqrt(x*x + y*y);
                if (distance <= outerRadius && distance >= innerRadius) {
                    SetPixel(x0 + x, y0 + y, color);
                }
            }
        }
    }
    
    // Радиальная градиентная заливка кольца
    void FillRingRadialGradient(int x0, int y0, int outerRadius, int innerRadius, COLOR outerColor, COLOR innerColor) {
        for (int y = -outerRadius; y <= outerRadius; y++) {
            for (int x = -outerRadius; x <= outerRadius; x++) {
                float distance = sqrt(x*x + y*y);
                if (distance <= outerRadius && distance >= innerRadius) {
                    float t = (distance - innerRadius) / (outerRadius - innerRadius);
                    COLOR color = InterpolateColor(innerColor, outerColor, t);
                    SetPixel(x0 + x, y0 + y, color);
                }
            }
        }
    }

    // Секторная заливка кольца
    void FillRingSector(int x0, int y0, int outerRadius, int innerRadius, float startAngle = 0.0f) {
        std::vector<COLOR> sectorColors = {
            COLOR(255, 100, 100),    // Красный
            COLOR(255, 200, 100),    // Оранжевый
            COLOR(255, 255, 100),    // Желтый
            COLOR(100, 255, 100),    // Зеленый
            COLOR(100, 200, 255),    // Голубой
            COLOR(100, 100, 255),    // Синий
            COLOR(200, 100, 255)     // Фиолетовый
        };

        for (int y = -outerRadius; y <= outerRadius; y++) {
            for (int x = -outerRadius; x <= outerRadius; x++) {
                float distance = sqrt(x * x + y * y);
                if (distance <= outerRadius && distance >= innerRadius) {
                    float angle = atan2(y, x);
                    if (angle < 0) angle += 2 * M_PI;
                    angle += startAngle;
                    if (angle >= 2 * M_PI) angle -= 2 * M_PI;
                    
                    int sector = static_cast<int>((angle / (2 * M_PI)) * sectorColors.size()) % sectorColors.size();
                    SetPixel(x0 + x, y0 + y, sectorColors[sector]);
                }
            }
        }
    }

    // Линейная градиентная заливка кольца
    void FillRingLinearGradient(int x0, int y0, int outerRadius, int innerRadius, COLOR startColor, COLOR endColor) {
        for (int y = -outerRadius; y <= outerRadius; y++) {
            for (int x = -outerRadius; x <= outerRadius; x++) {
                float distance = sqrt(x * x + y * y);
                if (distance <= outerRadius && distance >= innerRadius) {
                    // Линейный градиент по горизонтали
                    float t = (x + outerRadius) / (2.0f * outerRadius);
                    COLOR color = InterpolateColor(startColor, endColor, t);
                    SetPixel(x0 + x, y0 + y, color);
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

class LetterO : public Shape {
private:
    int x, y, outerRadius, innerRadius;
    
public:
    LetterO(int x_pos, int y_pos, int radius, int id) 
        : x(x_pos), y(y_pos), outerRadius(radius), innerRadius(radius * 2 / 3) {
        shapeId = id;
    }
    
    void Draw(Frame& frame) override {
        COLOR drawColor = color;
        drawColor.a = static_cast<Uint8>(alpha * 255);
        
        switch (fillType) {
            case 0: // Однородная заливка
                frame.FillRing(x, y, outerRadius, innerRadius, drawColor);
                break;
            case 1: // Радиальная градиентная заливка
                frame.FillRingRadialGradient(x, y, outerRadius, innerRadius, 
                    COLOR(100, 150, 255, static_cast<Uint8>(alpha * 255)),
                    COLOR(50, 100, 200, static_cast<Uint8>(alpha * 255)));
                break;
            case 2: // Секторная заливка
                frame.FillRingSector(x, y, outerRadius, innerRadius, globalRotation);
                break;
            case 3: // Линейная градиентная заливка
                frame.FillRingLinearGradient(x, y, outerRadius, innerRadius,
                    COLOR(255, 100, 100, static_cast<Uint8>(alpha * 255)),
                    COLOR(100, 100, 255, static_cast<Uint8>(alpha * 255)));
                break;
        }
        
        // Выделение
        if (selected) {
            frame.DrawThickCircle(x, y, outerRadius, COLOR(0, 0, 0, 255), 3);
            frame.DrawThickCircle(x, y, innerRadius, COLOR(0, 0, 0, 255), 3);
        }
    }
    
    bool Contains(int testX, int testY) override {
        int dx = testX - x;
        int dy = testY - y;
        float distance = sqrt(dx*dx + dy*dy);
        return distance <= outerRadius && distance >= innerRadius;
    }
    
    float GetArea() override {
        float outerArea = M_PI * outerRadius * outerRadius;
        float innerArea = M_PI * innerRadius * innerRadius;
        return outerArea - innerArea;
    }
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Не удалось инициализировать SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Лабораторная работа №2 - Буква 'О' с разными заливками", 
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

    struct ShapeState {
        bool selected = false;
        int fillType = 0;
        float alpha = 0.8f;
    };
    
    std::vector<ShapeState> shapeStates(1);

    auto updateWindowTitle = [window, &logicalWidth, &logicalHeight, &selectedShapeId, &shapeStates]() {
        std::string title = "Задача Лаб. работа №2 - Буква 'О' - Разрешение: " + 
                           std::to_string(logicalWidth) + "x" + 
                           std::to_string(logicalHeight);
        
        if (selectedShapeId != -1) {
            title += " - Выбрана буква 'О'";
            std::string fillName;
            switch(shapeStates[selectedShapeId].fillType) {
                case 0: fillName = "Однородная"; break;
                case 1: fillName = "Радиальная"; break;
                case 2: fillName = "Секторная"; break;
                case 3: fillName = "Линейная градиентная"; break;
                default: fillName = "Неизвестная"; break;
            }
            title += " - Заливка: " + fillName;
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
                } else if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_4) {
                    int newFillType = event.key.keysym.sym - SDLK_1;
                    if (selectedShapeId != -1) {
                         shapeStates[selectedShapeId].fillType = newFillType;
                    }
                    updateWindowTitle();
                } else if ((event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS)) {
                    if (selectedShapeId != -1) {
                        shapeStates[selectedShapeId].alpha = std::min(shapeStates[selectedShapeId].alpha + 0.1f, 1.0f);
                    }
                    updateWindowTitle();
                } else if ((event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS)) {
                    if (selectedShapeId != -1) {
                        shapeStates[selectedShapeId].alpha = std::max(shapeStates[selectedShapeId].alpha - 0.1f, 0.1f);
                    }
                    updateWindowTitle();
                } else if (event.key.keysym.sym == SDLK_c) {
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

                bool shapeClicked = false;
                for (int i = shapes.size() - 1; i >= 0; --i) {
                    if (shapes[i]->Contains(logicalX, logicalY)) {
                        selectedShapeId = shapes[i]->shapeId;
                        for(auto& state : shapeStates) state.selected = false;
                        shapeStates[selectedShapeId].selected = true;
                        shapeClicked = true;
                        break;
                    }
                }
                
                if (!shapeClicked) {
                     selectedShapeId = -1;
                     for (auto& state : shapeStates) {
                        state.selected = false;
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
        
        int radius = std::min(logicalWidth, logicalHeight) / 3;
    
        auto letterO = std::unique_ptr<LetterO>(new LetterO(centerX, centerY, radius, 0));
        letterO->fillType = shapeStates[0].fillType;
        letterO->alpha = shapeStates[0].alpha;
        letterO->selected = shapeStates[0].selected;
        
        shapes.push_back(std::move(letterO));

        frame.Clear(COLOR(255, 255, 255, 255));

        for (auto& shape : shapes) {
            shape->Draw(frame);
        }

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