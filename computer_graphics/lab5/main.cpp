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

struct PIXEL {
    Uint8 RED, GREEN, BLUE;
    float Z;
    PIXEL() : RED(0), GREEN(0), BLUE(0), Z(std::numeric_limits<float>::infinity()) {}
    PIXEL(Uint8 r, Uint8 g, Uint8 b) : RED(r), GREEN(g), BLUE(b), Z(std::numeric_limits<float>::infinity()) {}
};

struct Vec3 { 
    float x, y, z; 
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 cross(const Vec3& v) const { return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x }; }
    float dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0) { x/=len; y/=len; z/=len; }
    }
};

struct Vec4 { float x, y, z, w; };

struct Triangle {
    Vec3 p[3];
    PIXEL color;     
    Vec3 normal;     
};

const bool FONT_CHARS[26][15] = {
    {0,1,0,1,0,1,1,1,1,1,0,1,1,0,1}, // A
    {1,1,0,1,0,1,1,1,0,1,0,1,1,1,0}, // B
    {0,1,1,1,0,0,1,0,0,1,0,0,0,1,1}, // C
    {1,1,0,1,0,1,1,0,1,1,0,1,1,1,0}, // D
    {1,1,1,1,0,0,1,1,0,1,0,0,1,1,1}, // E
    {1,1,1,1,0,0,1,1,0,1,0,0,1,0,0}, // F
    {0,1,1,1,0,0,1,0,1,1,0,1,0,1,1}, // G
    {1,0,1,1,0,1,1,1,1,1,0,1,1,0,1}, // H
    {1,1,1,0,1,0,0,1,0,0,1,0,1,1,1}, // I
    {0,0,1,0,0,1,0,0,1,1,0,1,0,1,1}, // J
    {1,0,1,1,1,0,1,0,0,1,1,0,1,0,1}, // K
    {1,0,0,1,0,0,1,0,0,1,0,0,1,1,1}, // L
    {1,0,1,1,1,1,1,0,1,1,0,1,1,0,1}, // M
    {1,1,0,1,0,1,1,0,1,1,0,1,1,0,1}, // N
    {0,1,0,1,0,1,1,0,1,1,0,1,0,1,0}, // O
    {1,1,0,1,0,1,1,1,0,1,0,0,1,0,0}, // P
    {0,1,0,1,0,1,1,0,1,0,1,0,0,1,1}, // Q
    {1,1,0,1,0,1,1,1,0,1,1,0,1,0,1}, // R
    {0,1,1,1,0,0,0,1,0,0,0,1,1,1,0}, // S
    {1,1,1,0,1,0,0,1,0,0,1,0,0,1,0}, // T
    {1,0,1,1,0,1,1,0,1,1,0,1,0,1,1}, // U
    {1,0,1,1,0,1,1,0,1,1,0,1,0,1,0}, // V
    {1,0,1,1,0,1,1,0,1,1,1,1,1,0,1}, // W
    {1,0,1,0,1,0,0,1,0,0,1,0,1,0,1}, // X
    {1,0,1,1,0,1,0,1,0,0,1,0,0,1,0}, // Y
    {1,1,1,0,0,1,0,1,0,1,0,0,1,1,1}  // Z
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
        Matrix4 r={0}; for(int i=0;i<4;i++) for(int j=0;j<4;j++) for(int k=0;k<4;k++) r.m[i][j]+=m[i][k]*o.m[k][j]; return r;
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

PIXEL ApplyLighting(PIXEL base, Vec3 normal) {
    Vec3 light = {0.0f, 0.5f, 1.0f}; light.normalize();
    float i = std::max(0.1f, normal.dot(light));
    i = std::min(1.0f, i + 0.3f);
    return PIXEL((Uint8)(base.RED*i), (Uint8)(base.GREEN*i), (Uint8)(base.BLUE*i));
}

std::vector<Triangle> GenerateCup(int segments) {
    std::vector<Triangle> mesh;
    float r = 1.5f, h = 3.5f;
    float yT = h/2, yB = -h/2;
    PIXEL cOut(100,150,240), cIn(60,80,160), cBot(200,200,220);
    Vec3 nBot = {0,-1,0};

    for(int i=0; i<segments; ++i) {
        float t1 = (float)i/segments*2*M_PI, t2 = (float)(i+1)/segments*2*M_PI;
        float c1=cos(t1), s1=sin(t1), c2=cos(t2), s2=sin(t2);
        Vec3 p1={r*c1,yB,r*s1}, p2={r*c2,yB,r*s2}, p3={r*c2,yT,r*s2}, p4={r*c1,yT,r*s1};
        
        Triangle bot = {{ {0,yB,0}, p2, p1 }, cBot, nBot}; bot.color = ApplyLighting(cBot, nBot);
        mesh.push_back(bot);

        Vec3 nOut = {(c1+c2)/2, 0, (s1+s2)/2}; nOut.normalize();
        Triangle t1_out = {{p1, p2, p3}, cOut, nOut}; t1_out.color = ApplyLighting(cOut, nOut);
        Triangle t2_out = {{p1, p3, p4}, cOut, nOut}; t2_out.color = ApplyLighting(cOut, nOut);
        mesh.push_back(t1_out); mesh.push_back(t2_out);

        Vec3 nIn = {-(c1+c2)/2, 0, -(s1+s2)/2}; nIn.normalize();
        Triangle t1_in = {{p1, p3, p2}, cIn, nIn}; t1_in.color = ApplyLighting(cIn, nIn);
        Triangle t2_in = {{p1, p4, p3}, cIn, nIn}; t2_in.color = ApplyLighting(cIn, nIn);
        mesh.push_back(t1_in); mesh.push_back(t2_in);
    }
    return mesh;
}


const int INSIDE=0, LEFT=1, RIGHT=2, BOTTOM=4, TOP=8;
int ComputeCode(float x, float y, int minX, int minY, int maxX, int maxY) {
    int c = INSIDE;
    if(x<minX) c|=LEFT; else if(x>maxX) c|=RIGHT;
    if(y<minY) c|=TOP; else if(y>maxY) c|=BOTTOM;
    return c;
}

class Application {
    const int WIDTH = 1000;
    const int HEIGHT = 800;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    std::vector<PIXEL> fb;
    std::vector<Uint32> db;

    int segments = 24;
    float angleX = 0.8f, angleY = -0.5f; // Наклон вперед и влево
    float zoom = 1.2f;
    
    enum RenderMode { WIREFRAME=1, SOLID=2, COMBINED=3 } currentMode = COMBINED;
    enum ProjType { CENTRAL, ISO, DIM, TRI } currentProj = CENTRAL;

    bool mouseLeftPressed = false; // Для вращения

public:
    Application() {
        fb.resize(WIDTH*HEIGHT);
        db.resize(WIDTH*HEIGHT);
    }

    bool Init() {
        if(SDL_Init(SDL_INIT_VIDEO)<0) return false;
        window = SDL_CreateWindow("Lab 3D: Final", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
        if(!window) return false;
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
        return true;
    }

    void Clear() {
        for(auto& p : fb) { p.RED=40; p.GREEN=40; p.BLUE=40; p.Z=std::numeric_limits<float>::infinity(); }
    }

    void SetPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
        if(x>=0 && x<WIDTH && y>=0 && y<HEIGHT) {
            int i = y*WIDTH+x;
            fb[i].RED=r; fb[i].GREEN=g; fb[i].BLUE=b;
        }
    }

    void DrawChar(int x, int y, char c, Uint8 r, Uint8 g, Uint8 b, int scale=2) {
        if (c < 'A' || c > 'Z') return;
        int idx = c - 'A';
        for(int row=0; row<5; row++) {
            for(int col=0; col<3; col++) {
                if(FONT_CHARS[idx][row*3 + col]) {
                    for(int dy=0; dy<scale; dy++)
                        for(int dx=0; dx<scale; dx++)
                            SetPixel(x + col*scale + dx, y + row*scale + dy, r, g, b);
                }
            }
        }
    }

    void DrawString(int x, int y, std::string s, Uint8 r, Uint8 g, Uint8 b) {
        int cursor = x;
        for(char c : s) {
            if(c >= 'a' && c <= 'z') c -= 32;
            else if (c < 'A' || c > 'Z') { cursor += 8; continue; } 
            DrawChar(cursor, y, c, r, g, b);
            cursor += 8; 
        }
    }

    void DrawLine(float x0, float y0, float x1, float y1, int minX, int minY, int maxX, int maxY) {
        int code0 = ComputeCode(x0, y0, minX, minY, maxX, maxY);
        int code1 = ComputeCode(x1, y1, minX, minY, maxX, maxY);
        bool accept = false;
        while(true) {
            if(!(code0|code1)) { accept=true; break; }
            else if(code0&code1) break;
            else {
                float x, y;
                int out = code0 ? code0 : code1;
                if(out&BOTTOM) { x=x0+(x1-x0)*(maxY-y0)/(y1-y0); y=maxY; }
                else if(out&TOP) { x=x0+(x1-x0)*(minY-y0)/(y1-y0); y=minY; }
                else if(out&RIGHT) { y=y0+(y1-y0)*(maxX-x0)/(x1-x0); x=maxX; }
                else { y=y0+(y1-y0)*(minX-x0)/(x1-x0); x=minX; }
                if(out==code0) { x0=x; y0=y; code0=ComputeCode(x0,y0,minX,minY,maxX,maxY); }
                else { x1=x; y1=y; code1=ComputeCode(x1,y1,minX,minY,maxX,maxY); }
            }
        }
        if(accept) {
            float dx=x1-x0, dy=y1-y0, steps=std::max(std::abs(dx), std::abs(dy));
            if(steps==0) return;
            float xi=dx/steps, yi=dy/steps, X=x0, Y=y0;
            for(int i=0; i<=steps; i++, X+=xi, Y+=yi) SetPixel((int)X, (int)Y, 255, 255, 255);
        }
    }

    void DrawTriangle(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, PIXEL c, int minX, int minY, int maxX, int maxY) {
        int xMin=std::max(minX, (int)std::min({x0,x1,x2})), xMax=std::min(maxX, (int)std::max({x0,x1,x2}));
        int yMin=std::max(minY, (int)std::min({y0,y1,y2})), yMax=std::min(maxY, (int)std::max({y0,y1,y2}));
        float S = (y1-y2)*(x0-x2) + (x2-x1)*(y0-y2);
        if(std::abs(S)<0.1f) return;

        for(int y=yMin; y<=yMax; y++) {
            for(int x=xMin; x<=xMax; x++) {
                float h0=((y1-y2)*(x-x2)+(x2-x1)*(y-y2))/S;
                float h1=((y2-y0)*(x-x2)+(x0-x2)*(y-y2))/S;
                float h2=((y0-y1)*(x-x1)+(x1-x0)*(y-y1))/S;
                if(h0>=0 && h1>=0 && h2>=0) {
                    float z = h0*z0 + h1*z1 + h2*z2;
                    int idx = y*WIDTH+x;
                    if(z > -500 && z < 500 && z < fb[idx].Z) {
                        fb[idx] = PIXEL(c.RED, c.GREEN, c.BLUE); fb[idx].Z = z;
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
        Matrix4 mUser = Matrix4::RotationX(angleX)*Matrix4::RotationY(angleY)*mUserProj;

        struct View { int x,y,w,h; Matrix4 m; bool p; };
        View views[] = {
            {0,0,hW,hH,mFront,false}, {hW,0,hW,hH,mSide,false},
            {0,hH,hW,hH,mTop,false}, {hW,hH,hW,hH,mUser,persp}
        };

        for(int v=0; v<4; v++) {
            View& vp = views[v];
            float cx = vp.x + vp.w/2.0f, cy = vp.y + vp.h/2.0f;
            float sc = vp.p ? std::min(vp.w,vp.h)/2.0f : 1.0f;

            for(const auto& t : mesh) {
                Vec4 tv[3]; float sx[3], sy[3], sz[3];
                for(int i=0; i<3; i++) {
                    tv[i] = Multiply(vp.m, t.p[i]);
                    if(vp.p && tv[i].w!=0) { tv[i].x/=tv[i].w; tv[i].y/=tv[i].w; tv[i].z/=tv[i].w; }
                    sx[i]=cx+tv[i].x*sc; sy[i]=cy-tv[i].y*sc; sz[i]=tv[i].z;
                }
                if(currentMode!=WIREFRAME) 
                    DrawTriangle(sx[0],sy[0],sz[0], sx[1],sy[1],sz[1], sx[2],sy[2],sz[2], t.color, vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                if(currentMode!=SOLID) {
                    DrawLine(sx[0],sy[0],sx[1],sy[1], vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                    DrawLine(sx[1],sy[1],sx[2],sy[2], vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
                    DrawLine(sx[2],sy[2],sx[0],sy[0], vp.x, vp.y, vp.x+vp.w-1, vp.y+vp.h-1);
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
        DrawString(hW + 20, hH + 20, pName, 0, 255, 0); 
        DrawString(hW + 20, hH + 40, "MOUSE: ROTATE (L) / SEGMENTS (WHEEL)", 200, 200, 200);
        DrawString(hW + 20, hH + 60, "KEYS: +/- FOR ZOOM", 200, 200, 200);

        for(int i=0; i<WIDTH*HEIGHT; i++) 
            db[i] = (255<<24)|(fb[i].RED<<16)|(fb[i].GREEN<<8)|fb[i].BLUE;
        
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
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (e.button.x > WIDTH/2 && e.button.y > HEIGHT/2)
                            mouseLeftPressed = true;
                    }
                    else if (e.button.button == SDL_BUTTON_RIGHT) {
                         if (e.button.x > WIDTH/2 && e.button.y > HEIGHT/2)
                             currentProj = (ProjType)((currentProj + 1) % 4);
                    }
                }
                else if(e.type == SDL_MOUSEBUTTONUP) {
                    if (e.button.button == SDL_BUTTON_LEFT) mouseLeftPressed = false;
                }
                else if(e.type == SDL_MOUSEMOTION) {
                    if(mouseLeftPressed) {
                        angleY += e.motion.xrel * 0.01f;
                        angleX += e.motion.yrel * 0.01f;
                    }
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