#include <d3d11.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <WICTextureLoader.h>
#include <Keyboard.h>
#include <Mouse.h>
#include <Audio.h>
#include <stdexcept>
#include <Effects.h>
#include <VertexTypes.h>
#include <PrimitiveBatch.h>
#include <GeometricPrimitive.h>
#include <Model.h>
#include <SimpleMath.h>
#include <CommonStates.h>
#include <DirectXHelpers.h>
#include "Animation.h"

#pragma comment(lib, "d3d11.lib")

using namespace DirectX;
using namespace DirectX::SimpleMath;


// global declarations
extern const int Width;
extern const int Height;
extern bool gameover;

extern ID3D11Device* dev;                     // the pointer to our Direct3D device interface
extern ID3D11DeviceContext* devcon;           // the pointer to our Direct3D device context
extern IDXGISwapChain* swapchain;             // the pointer to the swap chain interface
extern ID3D11RenderTargetView* renderview;    // the pointer to the render target view
extern std::unique_ptr<SpriteBatch> spriteBatch;  // the pointer to the sprite batch


//Function Prototypes
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

bool Game_Init(HWND hwnd);
void Game_Run();
void Game_End();

bool InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void CleanD3D();        // closes Direct3D and releases memory
void ClearScreen();


struct Model2D
{
    ID3D11ShaderResourceView* texture = NULL;
    int x = 0, y = 0;
    int move_x = 0, move_y = 0;
    int frame = 0;              // the current frame index
    int frame_total = 0;        // how many frames totally in the frame sheet
    int frame_column = 0;       // how many columns in the frame sheet
    int frame_width = 0;        // one frame width
    int frame_height = 0;       // one frame height
};


// load sprite sheet into a model
// frame_total is how many frames in the frame sheet
// frame_column is how many columns in the frame sheet
Model2D CreateModel2D(LPCWSTR filename, int frame_total = 1, int frame_column = 1);
void DrawModel2D(Model2D model, RECT game_window = RECT());
bool CheckModel2DCollided(Model2D model1, Model2D model2);

extern std::unique_ptr<Keyboard> keyboard;
extern std::unique_ptr<Mouse> mouse;
bool InitInput(HWND hwnd);

bool InitSound();
std::unique_ptr<SoundEffect> LoadSound(LPCWSTR filename);
void PlaySound(SoundEffect* soundEffect);



struct Camera
{
    float x = 0, y = 0, z = 0;
    float xrot = 0, yrot = 0, zrot = 0;
    DirectX::SimpleMath::Matrix matrixView;
    DirectX::SimpleMath::Matrix matrixProj;
};

void SetCamera(float x, float y, float z, float lookx, float looky, float lookz);
void SetPerspective(float fieldOfView, float aspectRatio, float nearRange, float farRange);

struct Shape3D
{
    std::unique_ptr<GeometricPrimitive> shape;
    ID3D11ShaderResourceView* texture = NULL;    
    XMFLOAT3 size = { 1.0, 1.0, 1.0 };
    float x = 0, y = 0, z = 0;
    float xrot = 0, yrot = 0, zrot = 0;
    float move_speed = 0;
    Vector3 move_direction = Vector3(0, 0, 0);
    DirectX::SimpleMath::Matrix matrixWorld = Matrix::Identity;
};

// create a box with texture tessellation factors
void CreateBox(Shape3D& box, LPCWSTR filename = NULL, Vector3 size = { 1.0, 1.0, 1.0 }, int tx = 1, int ty = 1, int tz = 1);
void DrawShape3D(Shape3D& shape, BasicEffect* effect = NULL);

struct Model3D
{
    std::unique_ptr<DirectX::Model> model;
    float scale = 1;
    float x = 0, y = 0, z = 0;    
    float xrot = 0, yrot = 0, zrot = 0;
    float move_speed = 0;
    Vector3 move_direction = Vector3(0, 0, 0);    
    DirectX::SimpleMath::Matrix matrixWorld = Matrix::Identity;
    bool animCmo = false;
    DX::AnimationCMO animationCMO;
    bool animMesh = false;
    DX::AnimationSDKMESH animationMesh;
};

bool CreateModel3DFromCmo(Model3D& model3d, LPCWSTR filename);
bool CreateModel3DFromSdkmesh(Model3D& model3d, LPCWSTR meshFileName, LPCWSTR animFileName = nullptr);
void DrawModel3D(Model3D& model3d, float frame_interval=33);

bool CheckModel3DCollided(Model3D& model1, Model3D& model2);
bool CheckModel3DSelected(Model3D& model3d, int x, int y, float& distance);
void GetPickingRay(int x, int y, Vector3& rayOrigin, Vector3& rayDirection);
