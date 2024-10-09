#include "DirectX.h"


ID3D11Device* dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext* devcon;           // the pointer to our Direct3D device context
IDXGISwapChain* swapchain;             // the pointer to the swap chain interface
ID3D11RenderTargetView* renderview;    // the pointer to the render target view
std::unique_ptr<SpriteBatch> spriteBatch;

std::unique_ptr<AudioEngine> audEngine;
std::unique_ptr<Keyboard> keyboard;
std::unique_ptr<Mouse> mouse;


ID3D11DepthStencilView* depthStencilView;
ID3D11Texture2D* depthStencilBuffer; 
Camera cam;


bool InitD3D(HWND hWnd)
{
    HRESULT hr;

    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                    // one back buffer in addition with the front buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
    scd.BufferDesc.Width = Width;                           // set the back buffer width
    scd.BufferDesc.Height = Height;                         // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
    scd.OutputWindow = hWnd;                                // the window to be used
    scd.SampleDesc.Count = 4;                               // how many multisamples
    scd.Windowed = TRUE;                                    // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;     // allow full-screen switching

    // create a device, device context and swap chain using the information in the scd struct
    D3D_FEATURE_LEVEL FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_0; // Use d3d11
    UINT              numLevelsRequested = 1; // Number of levels 
    D3D_FEATURE_LEVEL FeatureLevelsSupported;

    hr = D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        &FeatureLevelsRequested,
        numLevelsRequested,
        D3D11_SDK_VERSION,
        &scd,
        &swapchain,
        &dev,
        &FeatureLevelsSupported,
        &devcon);
    if (FAILED(hr))
        return false;

    // create a render target view
    ID3D11Texture2D* backbuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer));
    hr = dev->CreateRenderTargetView(backbuffer, nullptr, &renderview);
    if (FAILED(hr))
        return false;

    D3D11_VIEWPORT vp;
    vp.Width = Width;
    vp.Height = Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    devcon->RSSetViewports(1, &vp);

    spriteBatch.reset(new SpriteBatch(devcon));

    // bind render target view
    //devcon->OMSetRenderTargets(1, &renderview, nullptr);

    //Describe our Depth/Stencil Buffer
    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = Width;
    depthStencilDesc.Height = Height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = scd.SampleDesc.Count;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    dev->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilBuffer);
    dev->CreateDepthStencilView(depthStencilBuffer, NULL, &depthStencilView);
    devcon->OMSetRenderTargets(1, &renderview, depthStencilView);

    return true;
}

void CleanD3D()
{
    // switch with Alt-Enter between full screen and windowed mode
    // switch to windowed mode when closing game
    swapchain->SetFullscreenState(FALSE, NULL);

    // close and release all existing resources
    depthStencilView->Release();
    depthStencilBuffer->Release();

    mouse.release();
    keyboard.release();
    audEngine.release();
    spriteBatch.release();
    renderview->Release();
    swapchain->Release();
    devcon->Release();
    dev->Release();
}

void ClearScreen()
{
    float background_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    devcon->ClearRenderTargetView(renderview, background_color);
    devcon->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}


Model2D CreateModel2D(LPCWSTR filename, int frame_total, int frame_column)
{
    Model2D model;
    model.x = 0;
    model.y = 0;
    model.move_x = 0;
    model.move_y = 0;
    model.frame_total = frame_total;
    model.frame_column = frame_column;

    CreateWICTextureFromFile(dev, filename, NULL, &model.texture);
    if (model.texture != NULL)
    {
        ID3D11Resource* resource;
        model.texture->GetResource(&resource);
        ID3D11Texture2D* tex2D;
        resource->QueryInterface<ID3D11Texture2D>(&tex2D);
        D3D11_TEXTURE2D_DESC desc;
        tex2D->GetDesc(&desc);

        int row_number = ceil(float(frame_total) / float(frame_column));
        model.frame_width = int(desc.Width) / frame_column;
        model.frame_height = int(desc.Height) / row_number;
    }

    return model;
}

void DrawModel2D(Model2D model, RECT game_window)
{
    if (model.texture != NULL)
    {
        model.frame = model.frame % model.frame_total;

        RECT rect;
        rect.left = (model.frame % model.frame_column) * model.frame_width;
        rect.right = rect.left + model.frame_width;
        rect.top = (model.frame / model.frame_column) * model.frame_height;
        rect.bottom = rect.top + model.frame_height;

        XMFLOAT2 pos;
        pos.x = model.x - game_window.left;
        pos.y = model.y - game_window.top;
        spriteBatch->Draw(model.texture, pos, &rect);
    }
}

bool CheckModel2DCollided(Model2D model1, Model2D model2)
{
    RECT rect1;
    rect1.left = model1.x + 1;
    rect1.top = model1.y + 1;
    rect1.right = model1.x + model1.frame_width - 1;
    rect1.bottom = model1.y + model1.frame_height - 1;

    RECT rect2;
    rect2.left = model2.x + 1;
    rect2.top = model2.y + 1;
    rect2.right = model2.x + model2.frame_width - 1;
    rect2.bottom = model2.y + model2.frame_height - 1;

    RECT dest;
    return IntersectRect(&dest, &rect1, &rect2);  // return rect1 if rect2 has intersection 
}


bool InitInput(HWND hwnd)
{
    try {
        keyboard = std::make_unique<Keyboard>();
        mouse = std::make_unique<Mouse>();
        mouse->SetWindow(hwnd);
        //mouse->SetVisible(false);
        return true;
    }
    catch (std::runtime_error e)
    {
        return false;
    }
}


bool InitSound()
{
    try {
        // init sound
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default | AudioEngine_Debug;
        audEngine = std::make_unique<AudioEngine>(eflags);
        return true;
    }
    catch (std::runtime_error e)
    {
        return false;
    }

}


std::unique_ptr<SoundEffect> LoadSound(LPCWSTR filename)
{
    try {
        return std::make_unique<SoundEffect>(audEngine.get(), filename);
    }
    catch (std::runtime_error e)
    {
        return NULL;
    }

}

void PlaySound(SoundEffect* soundEffect)
{
    soundEffect->Play();
}






void SetCamera(float x, float y, float z, float lookx, float looky, float lookz)
{
    Vector3 camPosition(x, y, z), camTarget(lookx, looky, lookz), camUpDirection(0.0f, 1.0f, 0.0f);
    cam.matrixView = XMMatrixLookAtLH(camPosition, camTarget, camUpDirection);
}

void SetPerspective(float fieldOfView, float aspectRatio,
    float nearRange, float farRange)
{
    cam.matrixProj = XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearRange, farRange);
}


// create a box with texture tessellation factors
void CreateBox(Shape3D& box, LPCWSTR filename, Vector3 size, int tx, int ty, int tz)
{

    GeometricPrimitive::VertexCollection vertices;
    GeometricPrimitive::IndexCollection indices;
    GeometricPrimitive::CreateBox(vertices, indices, size, false);

    // Tile the texture in a 10x10 grid
    for (auto& it : vertices)
    {
        if (it.normal.x != 0) {
            it.textureCoordinate.x *= tz;
            it.textureCoordinate.y *= ty;
        }
        if (it.normal.y != 0) {
            it.textureCoordinate.x *= tx;
            it.textureCoordinate.y *= tz;
        }
        if (it.normal.z != 0) {
            it.textureCoordinate.x *= tx;
            it.textureCoordinate.y *= ty;
        }
    }

    box.shape = GeometricPrimitive::CreateCustom(devcon, vertices, indices);

    box.size = size;
    if (filename != NULL)
        CreateWICTextureFromFile(dev, filename, NULL, &box.texture);
}


void DrawShape3D(Shape3D& shape3d, BasicEffect* effect)
{
    XMMATRIX translate = XMMatrixTranslation(shape3d.x, shape3d.y, shape3d.z);
    XMMATRIX rotate = XMMatrixRotationRollPitchYaw(shape3d.xrot, shape3d.yrot, shape3d.zrot);
    shape3d.matrixWorld = rotate * translate;

    if (effect != NULL)
    {
        try {
            effect->SetWorld(shape3d.matrixWorld);
            effect->SetView(cam.matrixView);
            effect->SetProjection(cam.matrixProj);

            effect->SetTexture(shape3d.texture);
            effect->SetTextureEnabled(true);

            Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
            shape3d.shape->CreateInputLayout(effect,
                inputLayout.ReleaseAndGetAddressOf());

            shape3d.shape->Draw(effect, inputLayout.Get());
        }
        catch (std::exception e)
        {
            MessageBox(NULL, L"Error Drawing Shape in 3D", L"Error", MB_OK);
        }
    }
    else
    {
        shape3d.shape->Draw(shape3d.matrixWorld, cam.matrixView, cam.matrixProj, Colors::White, shape3d.texture);
    }
    
}


bool CreateModel3DFromCmo(Model3D& model3d, LPCWSTR filename)
{    
    size_t animsOffset;
    try
    {
        std::unique_ptr<DirectX::IEffectFactory> fxFactory;
        fxFactory = std::make_unique<EffectFactory>(dev);

        // using wrong ModelLoader parameter may make textures on the model appear flipped
        model3d.model = Model::CreateFromCMO(dev, filename, *fxFactory,
            ModelLoader_Clockwise | ModelLoader_IncludeBones,
            &animsOffset);
        HRESULT hr = model3d.animationCMO.Load(filename, animsOffset);
        if (!FAILED(hr))
        {
            model3d.animationCMO.Bind(*model3d.model);
            model3d.animCmo = true;
        }
    }
    catch (std::exception& ex)
    {           
        MessageBox(NULL, filename, L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    model3d.matrixWorld = Matrix::Identity;

    return true;
}

bool CreateModel3DFromSdkmesh(Model3D& model3d, LPCWSTR meshFileName, LPCWSTR animFileName)
{ 
    try
    {
        std::unique_ptr<DirectX::IEffectFactory> fxFactory;
        fxFactory = std::make_unique<EffectFactory>(dev);

        // using wrong ModelLoader parameter may make textures on the model appear flipped
        model3d.model = Model::CreateFromSDKMESH(dev, meshFileName, *fxFactory, 
            ModelLoader_CounterClockwise | ModelLoader_IncludeBones);
        if (animFileName != nullptr)
        {
            HRESULT hr = model3d.animationMesh.Load(animFileName);
            if (!FAILED(hr))
            {
                model3d.animationMesh.Bind(*model3d.model);
                model3d.animMesh = true;
            }           
        }
    }
    catch (std::exception& ex)
    {        
        MessageBox(NULL, meshFileName, L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    return true;
}

void DrawModel3D(Model3D& model3d, float frame_interval)
{
    std::unique_ptr<CommonStates> states;
    states = std::make_unique<CommonStates>(dev);

    XMMATRIX translate = XMMatrixTranslation(model3d.x, model3d.y, model3d.z);
    XMMATRIX scale = XMMatrixScaling(model3d.scale, model3d.scale, model3d.scale);
    XMMATRIX rotate = XMMatrixRotationRollPitchYaw(model3d.xrot, model3d.yrot, model3d.zrot);
    model3d.matrixWorld = scale * rotate * translate;

    // if need animation with mesh    
    if (model3d.animMesh)
    {
        float delta = 1.0 / frame_interval;
        model3d.animationMesh.Update(delta);
        size_t nbones = model3d.model->bones.size();
        DirectX::ModelBone::TransformArray drawBones;
        drawBones = ModelBone::MakeArray(model3d.model->bones.size());
        model3d.animationMesh.Apply(*model3d.model, nbones, drawBones.get());
        model3d.model->DrawSkinned(devcon, *states, nbones, drawBones.get(), model3d.matrixWorld, cam.matrixView, cam.matrixProj);
    }
    // if need animation with CMO
    else if (model3d.animCmo)
    {
        float delta = 1.0 / frame_interval;
        model3d.animationCMO.Update(delta);
        size_t nbones = model3d.model->bones.size();
        DirectX::ModelBone::TransformArray drawBones;
        drawBones = ModelBone::MakeArray(model3d.model->bones.size());
        model3d.animationCMO.Apply(*model3d.model, nbones, drawBones.get());
        model3d.model->DrawSkinned(devcon, *states, nbones, drawBones.get(), model3d.matrixWorld, cam.matrixView, cam.matrixProj);
    }
    else // no animation
    {
        model3d.model->Draw(devcon, *states, model3d.matrixWorld, cam.matrixView, cam.matrixProj);
    }
}

bool CheckModel3DCollided(Model3D& model1, Model3D& model2)
{
    XMMATRIX translate, scale, rotate;

    translate = XMMatrixTranslation(model1.x, model1.y, model1.z);
    scale = XMMatrixScaling(model1.scale, model1.scale, model1.scale);
    rotate = XMMatrixRotationRollPitchYaw(model1.xrot, model1.yrot, model1.zrot);
    model1.matrixWorld = scale * rotate * translate;
    BoundingOrientedBox bounding1;
    bounding1.CreateFromBoundingBox(bounding1, model1.model->meshes.at(0)->boundingBox);
    bounding1.Transform(bounding1, model1.matrixWorld);

    translate = XMMatrixTranslation(model2.x, model2.y, model2.z);
    scale = XMMatrixScaling(model2.scale, model2.scale, model2.scale);
    rotate = XMMatrixRotationRollPitchYaw(model2.xrot, model2.yrot, model2.zrot);
    model2.matrixWorld = scale * rotate * translate;
    BoundingOrientedBox bounding2;
    bounding2.CreateFromBoundingBox(bounding2, model2.model->meshes.at(0)->boundingBox);
    bounding2.Transform(bounding2, model2.matrixWorld);
    
    return bounding1.Intersects(bounding2);
}

void GetPickingRay(int x, int y, Vector3& rayOrigin, Vector3& rayDirection)
{
    float px = (((2.0f * x) / Width) - 1.0f) / cam.matrixProj(0, 0);
    float py = (((-2.0f * y) / Height) + 1.0f) / cam.matrixProj(1, 1);
    rayOrigin = Vector3(0.0f, 0.0f, 0.0f);
    rayDirection = Vector3(px, py, 1.0f);

    // Get Projection matrix
    Matrix viewInverse = XMMatrixInverse(nullptr, cam.matrixView);
    rayOrigin = XMVector3TransformCoord(rayOrigin, viewInverse);
    rayDirection = XMVector3TransformNormal(rayDirection, viewInverse);
    rayDirection = XMVector3Normalize(rayDirection);
}

bool CheckModel3DSelected(Model3D& model3d, int x, int y, float& distance)
{
    Vector3 rayOrigin, rayDirection;
    GetPickingRay(x, y, rayOrigin, rayDirection);

    XMMATRIX translate = XMMatrixTranslation(model3d.x, model3d.y, model3d.z);
    XMMATRIX scale = XMMatrixScaling(model3d.scale, model3d.scale, model3d.scale);
    XMMATRIX rotate = XMMatrixRotationRollPitchYaw(model3d.xrot, model3d.yrot, model3d.zrot);
    model3d.matrixWorld = scale * rotate * translate;

    BoundingOrientedBox bounding;
    bounding.CreateFromBoundingBox(bounding, model3d.model->meshes.at(0)->boundingBox);
    bounding.Transform(bounding, model3d.matrixWorld);

    return bounding.Intersects(rayOrigin, rayDirection, distance);
}

