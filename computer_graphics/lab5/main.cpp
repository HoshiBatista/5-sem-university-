#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <limits>
#include <iostream>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float MAX_DEPTH = 1e30f;

struct PIXEL {
    Uint8 r, g, b;
    float z;
    PIXEL() : r(40), g(40), b(40), z(MAX_DEPTH) {} 
    PIXEL(Uint8 _r, Uint8 _g, Uint8 _b) : r(_r), g(_g), b(_b), z(MAX_DEPTH) {}
};

struct Vec3 { 
    float x, y, z; 
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    float dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0) { x/=len; y/=len; z/=len; }
    }
};

struct Vec4 { float x, y, z, w; };

struct Triangle {
    Vec3 p[3];
    PIXEL baseColor; 
    Vec3 normal;
};


struct Matrix4 {
    float m[4][4];
    static Matrix4 Identity() { Matrix4 r={0}; for(int i=0;i<4;i++) r.m[i][i]=1; return r; }
    static Matrix4 RotationX(float t) { Matrix4 r=Identity(); r.m[1][1]=cos(t); r.m[1][2]=sin(t); r.m[2][1]=-sin(t); r.m[2][2]=cos(t); return r; }
    static Matrix4 RotationY(float t) { Matrix4 r=Identity(); r.m[0][0]=cos(t); r.m[0][2]=-sin(t); r.m[2][0]=sin(t); r.m[2][2]=cos(t); return r; }
    static Matrix4 RotationZ(float t) { Matrix4 r=Identity(); r.m[0][0]=cos(t); r.m[0][1]=sin(t); r.m[1][0]=-sin(t); r.m[1][1]=cos(t); return r; }
    static Matrix4 Scale(float s) { Matrix4 r=Identity(); r.m[0][0]=s; r.m[1][1]=s; r.m[2][2]=s; return r; }
    static Matrix4 Translation(float x, float y, float z) { Matrix4 r=Identity(); r.m[3][0]=x; r.m[3][1]=y; r.m[3][2]=z; return r; }
    static Matrix4 Perspective(float fov, float ar, float n, float f) {
        Matrix4 r={0}; float th=tan(fov/2); r.m[0][0]=1/(ar*th); r.m[1][1]=1/th; 
        r.m[2][2]=-(f+n)/(f-n); r.m[2][3]=-1; r.m[3][2]=-(2*f*n)/(f-n); return r;
    }
    static Matrix4 Isometric() { return RotationX(35.264f*M_PI/180)*RotationY(45.0f*M_PI/180); }
    static Matrix4 Dimetric() { return RotationX(20.0f*M_PI/180)*RotationY(20.0f*M_PI/180); }
    static Matrix4 Trimetric() { return RotationX(15.0f*M_PI/180)*RotationY(30.0f*M_PI/180); }
    
    Matrix4 operator*(const Matrix4& o) const {
        Matrix4 r={0}; 
        for(int i=0;i<4;i++) 
            for(int j=0;j<4;j++) 
                for(int k=0;k<4;k++) 
                    r.m[i][j]+=m[i][k]*o.m[k][j]; 
        return r;
    }
};

Vec4 Multiply(const Matrix4& m, const Vec3& v) {
    return {
        v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + m.m[3][0],
        v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + m.m[3][1],
        v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + m.m[3][2],
        v.x*m.m[0][3] + v.y*m.m[1][3] + v.z*m.m[2][3] + m.m[3][3]
    };
}

Vec3 RotateVector(const Matrix4& m, const Vec3& v) {
    return {
        v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0],
        v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1],
        v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2]
    };
}

std::vector<Triangle> GenerateCup(int segments) {
    std::vector<Triangle> mesh;
    float r = 1.5f;       
    float r_in = r * 0.9f; 
    float h = 3.5f;
    float yT = h/2, yB = -h/2;

    PIXEL cOut(100,150,240); 
    PIXEL cIn(60,80,160);    
    
    PIXEL cBot(255, 0, 0);   

    Vec3 nBotOut = {0,-1,0}; 
    Vec3 nBotIn  = {0, 1,0}; 

    for(int i=0; i<segments; ++i) {
        float t1 = (float)i/segments*2*M_PI;
        float t2 = (float)(i+1)/segments*2*M_PI;
        
        float c1=cos(t1), s1=sin(t1);
        float c2=cos(t2), s2=sin(t2);

        Vec3 p1={r*c1,yB,r*s1}, p2={r*c2,yB,r*s2}, p3={r*c2,yT,r*s2}, p4={r*c1,yT,r*s1};
        Vec3 p1_in={r_in*c1,yB,r_in*s1}, p2_in={r_in*c2,yB,r_in*s2}, p3_in={r_in*c2,yT,r_in*s2}, p4_in={r_in*c1,yT,r_in*s1};

        mesh.push_back({{ {0,yB,0}, p2, p1 }, cBot, nBotOut});

        mesh.push_back({{ {0,yB,0}, p1_in, p2_in }, cBot, nBotIn});

        Vec3 nOut = {(c1+c2)/2, 0, (s1+s2)/2}; nOut.normalize();
        mesh.push_back({{p1, p2, p3}, cOut, nOut});
        mesh.push_back({{p1, p3, p4}, cOut, nOut});

        Vec3 nIn = {-(c1+c2)/2, 0, -(s1+s2)/2}; nIn.normalize();
        mesh.push_back({{p1_in, p3_in, p2_in}, cIn, nIn});
        mesh.push_back({{p1_in, p4_in, p3_in}, cIn, nIn});
    }
    return mesh;
}


inline float EdgeFunction(const Vec3& a, const Vec3& b, float px, float py) {
    return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
}

PIXEL CalculateLight(PIXEL base, Vec3 normal) {
    Vec3 lightDir = {0.2f, 0.5f, 1.0f}; 
    lightDir.normalize();
    
    float dot = normal.dot(lightDir);
    float intensity = std::max(0.2f, dot); 
    
    int r = (int)(base.r * intensity); if (r > 255) r = 255;
    int g = (int)(base.g * intensity); if (g > 255) g = 255;
    int b = (int)(base.b * intensity); if (b > 255) b = 255;
    
    return PIXEL((Uint8)r, (Uint8)g, (Uint8)b);
}

const bool FONT_CHARS[26][15] = {
    {0,1,0,1,0,1,1,1,1,1,0,1,1,0,1}, {1,1,0,1,0,1,1,1,0,1,0,1,1,1,0}, {0,1,1,1,0,0,1,0,0,1,0,0,0,1,1}, 
    {1,1,0,1,0,1,1,0,1,1,0,1,1,1,0}, {1,1,1,1,0,0,1,1,0,1,0,0,1,1,1}, {1,1,1,1,0,0,1,1,0,1,0,0,1,0,0}, 
    {0,1,1,1,0,0,1,0,1,1,0,1,0,1,1}, {1,0,1,1,0,1,1,1,1,1,0,1,1,0,1}, {1,1,1,0,1,0,0,1,0,0,1,0,1,1,1}, 
    {0,0,1,0,0,1,0,0,1,1,0,1,0,1,1}, {1,0,1,1,1,0,1,0,0,1,1,0,1,0,1}, {1,0,0,1,0,0,1,0,0,1,0,0,1,1,1}, 
    {1,0,1,1,1,1,1,0,1,1,0,1,1,0,1}, {1,1,0,1,0,1,1,0,1,1,0,1,1,0,1}, {0,1,0,1,0,1,1,0,1,1,0,1,0,1,0}, 
    {1,1,0,1,0,1,1,1,0,1,0,0,1,0,0}, {0,1,0,1,0,1,1,0,1,0,1,0,0,1,1}, {1,1,0,1,0,1,1,1,0,1,1,0,1,0,1}, 
    {0,1,1,1,0,0,0,1,0,0,0,1,1,1,0}, {1,1,1,0,1,0,0,1,0,0,1,0,0,1,0}, {1,0,1,1,0,1,1,0,1,1,0,1,0,1,1}, 
    {1,0,1,1,0,1,1,0,1,1,0,1,0,1,0}, {1,0,1,1,0,1,1,0,1,1,1,1,1,0,1}, {1,0,1,0,1,0,0,1,0,0,1,0,1,0,1}, 
    {1,0,1,1,0,1,0,1,0,0,1,0,0,1,0}, {1,1,1,0,0,1,0,1,0,1,0,0,1,1,1}
};

class Application {
    const int WIDTH = 1000;
    const int HEIGHT = 800;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    std::vector<PIXEL> fb;
    std::vector<Uint32> db;

    int segments = 24;
    float angleX = 0.8f, angleY = -0.5f;
    float zoom = 1.2f;
    
    enum RenderMode { WIREFRAME=1, SOLID=2, COMBINED=3 } currentMode = COMBINED;
    enum ProjType { CENTRAL, ISO, DIM, TRI } currentProj = CENTRAL;

    bool mouseLeftPressed = false;

public:
    Application() {
        fb.resize(WIDTH*HEIGHT);
        db.resize(WIDTH*HEIGHT);
    }

    bool Init() {
        if(SDL_Init(SDL_INIT_VIDEO)<0) return false;
        window = SDL_CreateWindow("Lab 3D: Red Bottom", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
        if(!window) return false;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
        return true;
    }

    void Clear() {
        std::fill(fb.begin(), fb.end(), PIXEL(40, 40, 40)); 
    }

    void SetPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
        if(x>=0 && x<WIDTH && y>=0 && y<HEIGHT) {
            int i = y*WIDTH+x;
            fb[i].r=r; fb[i].g=g; fb[i].b=b;
        }
    }

    void DrawString(int x, int y, std::string s, Uint8 r, Uint8 g, Uint8 b) {
        int cursor = x;
        for(char c : s) {
            if(c>='a' && c<='z') c-=32;
            if(c<'A' || c>'Z') { cursor+=8; continue; }
            int idx = c-'A';
            for(int row=0; row<5; row++) {
                for(int col=0; col<3; col++) {
                    if(FONT_CHARS[idx][row*3+col]) {
                        for(int dy=0; dy<2; dy++) for(int dx=0; dx<2; dx++)
                            SetPixel(cursor+col*2+dx, y+row*2+dy, r,g,b);
                    }
                }
            }
            cursor+=8;
        }
    }

    void DrawLine(float x0, float y0, float x1, float y1, int minX, int minY, int maxX, int maxY) {
        float dx = x1 - x0;
        float dy = y1 - y0;
        float steps = std::max(std::abs(dx), std::abs(dy));
        float xInc = dx / steps;
        float yInc = dy / steps;
        float x = x0, y = y0;

        for (int i = 0; i <= steps; i++) {
            if (x >= minX && x < maxX && y >= minY && y < maxY)
                SetPixel((int)x, (int)y, 255, 255, 255);
            x += xInc;
            y += yInc;
        }
    }

    void DrawTriangle(Vec3 v0, Vec3 v1, Vec3 v2, PIXEL color, int minX, int minY, int maxX, int maxY) {
        float area = EdgeFunction(v0, v1, v2.x, v2.y);
        
        if (area >= 0) return; 

        int xMin = std::max(minX, (int)std::floor(std::min({v0.x, v1.x, v2.x})));
        int xMax = std::min(maxX, (int)std::ceil(std::max({v0.x, v1.x, v2.x})));
        int yMin = std::max(minY, (int)std::floor(std::min({v0.y, v1.y, v2.y})));
        int yMax = std::min(maxY, (int)std::ceil(std::max({v0.y, v1.y, v2.y})));
        
        float inv_area = 1.0f / area;

        for(int y = yMin; y <= yMax; y++) {
            for(int x = xMin; x <= xMax; x++) {
                float px = x + 0.5f;
                float py = y + 0.5f;

                float w0 = EdgeFunction(v1, v2, px, py);
                float w1 = EdgeFunction(v2, v0, px, py);
                float w2 = EdgeFunction(v0, v1, px, py);

                if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                    w0 *= inv_area; w1 *= inv_area; w2 *= inv_area;
                    float z = w0 * v0.z + w1 * v1.z + w2 * v2.z;
                    int idx = y * WIDTH + x;

                    if (z < fb[idx].z) {
                        fb[idx].r = color.r;
                        fb[idx].g = color.g;
                        fb[idx].b = color.b;
                        fb[idx].z = z;
                    }
                }
            }
        }
    }

    void Render() {
        Clear();
        auto mesh = GenerateCup(segments);
        int hW = WIDTH/2, hH = HEIGHT/2;

        Matrix4 mFront = Matrix4::Scale(40*zoom);
        Matrix4 mSide = Matrix4::RotationZ(-M_PI/2)*Matrix4::Scale(40*zoom);
        Matrix4 mTop = Matrix4::RotationX(M_PI/2)*Matrix4::Scale(40*zoom);
        
        Matrix4 mUserProj;
        bool persp = false;
        
        if(currentProj==CENTRAL) {
            mUserProj = Matrix4::Scale(zoom) * Matrix4::Translation(0,0,-10) * Matrix4::Perspective(45*M_PI/180, (float)hW/hH, 0.1f, 100);
            persp = true;
        } else {
            Matrix4 axon;
            if(currentProj==ISO) axon = Matrix4::Isometric();
            else if(currentProj==DIM) axon = Matrix4::Dimetric();
            else axon = Matrix4::Trimetric();
            mUserProj = axon * Matrix4::Scale(40*zoom);
        }
        
        Matrix4 mRot = Matrix4::RotationX(angleX)*Matrix4::RotationY(angleY);
        Matrix4 mUser = mRot * mUserProj;

        struct View { int x,y,w,h; Matrix4 m; bool p; bool useLight; };
        View views[] = {
            {0,0,hW,hH,mFront,false, false}, 
            {hW,0,hW,hH,mSide,false, false},
            {0,hH,hW,hH,mTop,false, false}, 
            {hW,hH,hW,hH,mUser,persp, true} 
        };

        for(int v=0; v<4; v++) {
            View& vp = views[v];
            float cx = vp.x + vp.w/2.0f;
            float cy = vp.y + vp.h/2.0f;
            float sc = vp.p ? std::min(vp.w,vp.h)/2.0f : 1.0f;

            for(const auto& t : mesh) {
                Vec4 tv[3]; Vec3 sv[3];
                
                PIXEL finalColor = t.baseColor;
                if (vp.useLight) {
                    Vec3 rotatedNormal = RotateVector(mRot, t.normal);
                    rotatedNormal.normalize();
                    finalColor = CalculateLight(t.baseColor, rotatedNormal);
                } else {
                    finalColor = CalculateLight(t.baseColor, t.normal);
                }

                for(int i=0; i<3; i++) {
                    tv[i] = Multiply(vp.m, t.p[i]);
                    if(vp.p && tv[i].w!=0) { tv[i].x/=tv[i].w; tv[i].y/=tv[i].w; tv[i].z/=tv[i].w; }
                    sv[i] = { cx+tv[i].x*sc, cy-tv[i].y*sc, tv[i].z };
                }

                if(currentMode!=WIREFRAME) 
                    DrawTriangle(sv[0], sv[1], sv[2], finalColor, vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                
                if(currentMode!=SOLID) {
                    DrawLine(sv[0].x, sv[0].y, sv[1].x, sv[1].y, vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                    DrawLine(sv[1].x, sv[1].y, sv[2].x, sv[2].y, vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                    DrawLine(sv[2].x, sv[2].y, sv[0].x, sv[0].y, vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                }
            }
        }

        for(int x=0; x<WIDTH; x++) SetPixel(x, hH, 255, 255, 0);
        for(int y=0; y<HEIGHT; y++) SetPixel(hW, y, 255, 255, 0);

        std::string pName;
        switch(currentProj) {
            case CENTRAL: pName = "PERSPECTIVE"; break;
            case ISO: pName = "ISOMETRIC"; break;
            case DIM: pName = "DIMETRIC"; break;
            case TRI: pName = "TRIMETRIC"; break;
        }
        
        DrawString(hW+20, hH+20, pName, 0, 255, 0); 
        DrawString(hW+20, hH+40, "MOUSE: ROTATE (L) / SEGMENTS (WHEEL)", 200, 200, 200);
        DrawString(hW+20, hH+60, "KEYS: +/- FOR ZOOM", 200, 200, 200);

        for(int i=0; i<WIDTH*HEIGHT; i++) 
            db[i] = (255<<24)|(fb[i].r<<16)|(fb[i].g<<8)|fb[i].b;
        
        SDL_UpdateTexture(texture, nullptr, db.data(), WIDTH*4);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    void Run() {
        if(!Init()) return;
        bool run=true;
        SDL_Event e;
        while(run) {
            while(SDL_PollEvent(&e)) {
                if(e.type==SDL_QUIT) run=false;
                else if(e.type == SDL_MOUSEBUTTONDOWN) {
                    if(e.button.button == SDL_BUTTON_LEFT) {
                        if(e.button.x > WIDTH/2 && e.button.y > HEIGHT/2) mouseLeftPressed = true;
                    }
                    else if(e.button.button == SDL_BUTTON_RIGHT) {
                        if(e.button.x > WIDTH/2 && e.button.y > HEIGHT/2) currentProj = (ProjType)((currentProj + 1) % 4);
                    }
                }
                else if(e.type == SDL_MOUSEBUTTONUP) {
                    if(e.button.button == SDL_BUTTON_LEFT) mouseLeftPressed = false;
                }
                else if(e.type == SDL_MOUSEMOTION) {
                    if(mouseLeftPressed) { angleY += e.motion.xrel * 0.01f; angleX += e.motion.yrel * 0.01f; }
                }
                else if(e.type == SDL_MOUSEWHEEL) {
                    if(e.wheel.y > 0) segments++;
                    else if(e.wheel.y < 0 && segments > 3) segments--;
                }
                else if(e.type == SDL_KEYDOWN) {
                    switch(e.key.keysym.scancode) {
                        case SDL_SCANCODE_KP_PLUS:
                        case SDL_SCANCODE_EQUALS: zoom *= 1.1f; break;
                        case SDL_SCANCODE_KP_MINUS:
                        case SDL_SCANCODE_MINUS:  zoom *= 0.9f; break;
                        case SDL_SCANCODE_1: currentMode=WIREFRAME; break;
                        case SDL_SCANCODE_2: currentMode=SOLID; break;
                        case SDL_SCANCODE_3: currentMode=COMBINED; break;
                        case SDL_SCANCODE_Z: currentProj=CENTRAL; break;
                        case SDL_SCANCODE_X: currentProj=ISO; break;
                        case SDL_SCANCODE_C: currentProj=DIM; break;
                        case SDL_SCANCODE_V: currentProj=TRI; break;
                        default: break;
                    }
                }
            }
            Render();
        }
        SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    Application app;
    app.Run();
    return 0;
}