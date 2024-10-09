#include "DirectX.h"

using VertexType = DirectX::VertexPositionNormalTexture;
const int Width = 800;
const int Height = 600;
const float framerate = 30;
bool gameover;
int game_start_time;
int game_current_time;
int game_play_time_in_seconds;
int game_play_time_in_minutes;
int game_play_time_in_hour;
int  game_play_time_in_hour_and_minutes;
int seconds_count;
int minutes_count;
int hours_count;
Camera camera;
Shape3D ground;
Model3D car;
Model2D aim;
std::unique_ptr<CommonStates> states;
const int extra_number = 2;
int extra_left = 2;
Model3D extra[extra_number];
bool valid_extra[extra_number];
const int bomb_number = 2;
int bomb_left = 2;
Model3D bomb[bomb_number];
bool valid_bomb[bomb_number];
const int other_number = 24;
int other_left = 24;
Model3D other[other_number];
bool valid_other[other_number];
int last_boom = -1;
const int number_of_shots = 100;
int shots_left = 100;
Model3D shot[number_of_shots];
bool valid_shot[number_of_shots];
Model2D boom[number_of_shots];
bool boom_valid[number_of_shots];
int last_shot = -1;
int die = -1;
int score = 0;
int game_state = 1;
Keyboard::State kb;
Mouse::State mb;
Mouse::ButtonStateTracker mousetracker;
std::unique_ptr<SpriteFont> spriteFont;
std::unique_ptr<SoundEffect> gunfire;
std::unique_ptr<SoundEffect> explode;
std::unique_ptr<SoundEffect> sound1;
std::unique_ptr<SoundEffect> sound2;
std::unique_ptr<SoundEffect> bgmusic;

void Updateextra()
{
    for (int i = 0; i < extra_number; i++)
    {
        XMMATRIX rotate = XMMatrixRotationY(extra[i].yrot);
        Vector3 pos;
        pos.x = extra[i].x;
        pos.y = extra[i].y;
        pos.z = extra[i].z;
        extra[i].x = pos.x;
        extra[i].y = pos.y;
        extra[i].z = pos.z;
        extra[i].yrot += 0.1;
    }

    for (int i = 0; i < extra_number; i++)
    {
        if (valid_extra[i] && CheckModel3DCollided(car, extra[i]) && die < 0)
        {
            valid_extra[i] = false;
            PlaySound(sound1.get());
            die -= 1;
            extra_left--;
        }
    }
}

void Updatebomb()
{
    for (int i = 0; i < bomb_number; i++)
    {
        XMMATRIX rotate = XMMatrixRotationY(bomb[i].yrot);
        Vector3 pos;
        pos.x = bomb[i].x;
        pos.y = bomb[i].y;
        pos.z = bomb[i].z;
        bomb[i].x = pos.x;
        bomb[i].y = pos.y;
        bomb[i].z = pos.z;
        bomb[i].yrot += 0.5;
    }

    for (int i = 0; i < bomb_number; i++)
    {
        if (valid_bomb[i] && CheckModel3DCollided(car, bomb[i]) && die < 0)
        {
            valid_bomb[i] = false;
            last_boom++;
            if (last_boom >= other_number)
                last_boom = 0;
            boom[last_boom].x = car.x;
            boom[last_boom].y = car.y;
            boom[last_boom].frame = 0;
            boom_valid[last_boom] = true;
            PlaySound(sound2.get());
            die += 1;
            bomb_left--;
        }
    }
}

void Updatecar()
{
    Vector3 carInitDirection(0.0f, 0.0f, -1.0f);
    float speed = 0, yrot = 0;

    kb = keyboard->GetState();

    if (kb.W)
        speed = 0.3;
    if (kb.S)
        speed = -0.3;
    if (kb.A)
        yrot = -0.05;
    if (kb.D)
        yrot = 0.05;
    car.yrot = car.yrot + yrot;
    XMMATRIX rotate = XMMatrixRotationRollPitchYaw(car.xrot,
        car.yrot, car.zrot);
    Vector3 direction = carInitDirection.Transform(
        carInitDirection, rotate);
    direction.Normalize();
    Vector3 pos;
    pos.x = car.x;
    pos.y = car.y;
    pos.z = car.z;
    pos = pos + direction * speed;
    car.x = pos.x;
    car.y = pos.y;
    car.z = pos.z;
    pos = pos - direction * 5;
    camera.x = pos.x;
    camera.z = pos.z;
    camera.y = pos.y + 2;
    SetCamera(camera.x, camera.y, camera.z, car.x, car.y,
        car.z);
}

Vector3 FindTarget(int x, int y)
{
    float distance[other_number];
    float min_distance = FLT_MAX;
    Vector3 target(0, 0, 0);

    for (int i = 0; i < other_number; i++)
    {
        if (CheckModel3DSelected(other[i], x, y, distance[i])
            && min_distance > distance[i])
        {
            min_distance = distance[i];
            target.x = other[i].x; target.y = other[i].y; target.z = other[i].z;
        }
    }
    if (min_distance == FLT_MAX)
    {
        Vector3 rayOrigin, rayDirection, rayEnd;
        GetPickingRay(x, y, rayOrigin, rayDirection);

        rayEnd = rayOrigin;
        while (rayEnd.y >= 0 && rayEnd.x > -50 && rayEnd.x < 50
            && rayEnd.z > -50 && rayEnd.z < 50)
        {
            rayEnd = rayEnd + rayDirection;
        }
        target = rayEnd;
    }
    return target;
}

void Launchshot(Vector3 target)
{
    PlaySound(gunfire.get());
    last_shot++;
    if (last_shot < number_of_shots)
    {
        valid_shot[last_shot] = true;
        shot[last_shot].x = car.x;
        shot[last_shot].y = car.y;
        shot[last_shot].z = car.z;
        Vector3 pos = Vector3(car.x, car.y, car.z);
        shot[last_shot].move_direction = target - pos;
        shots_left--;
    }
}

void UpdateAim()
{
    const int cooldown = 100;
    static int cooling = 0;
    kb = keyboard->GetState();

    if (kb.Up)
        aim.y -= 5;
    if (kb.Down)
        aim.y += 5;
    if (kb.Left)
        aim.x -= 5;
    if (kb.Right)
        aim.x += 5;

    if (kb.Space && GetTickCount() - cooling >= cooldown)
    {
        cooling = GetTickCount();
        Vector3 target = FindTarget(aim.x + aim.frame_width / 2,
            aim.y + aim.frame_height / 2);
        Launchshot(target);
    }
}

void UpdateShots()
{
    float speed = 0.2;

    for (int i = 0; i < number_of_shots; i++)
    {
        if (valid_shot[i])
        {
            Vector3 pos;
            pos.x = shot[i].x; pos.y = shot[i].y; pos.z = shot[i].z;
            pos = pos + shot[i].move_direction * speed;
            shot[i].x = pos.x; shot[i].y = pos.y; shot[i].z = pos.z;
            for (int j = 0; j < other_number; j++)
            {
                if (valid_other[j] && CheckModel3DCollided(other[j], shot[i]))
                {
                    valid_shot[i] = false;
                    valid_other[j] = false;
                    score++;
                    last_boom++;
                    if (last_boom >= number_of_shots)
                        last_boom = 0;
                    boom[last_boom].x = other[i].x;
                    boom[last_boom].y = other[i].y;
                    boom[last_boom].frame = 0;
                    boom_valid[last_boom] = true;
                    PlaySound(explode.get());
                    other_left--;
                }
            }
        }
    }
}

void Updatebooms()
{
    for (int i = 0; i < number_of_shots; i++)
    {
        if (boom_valid[i])
            boom[i].frame++;

        if (boom[i].frame >= boom[i].frame_total)
            boom_valid[i] = false;
    }

    if (die >= 0 && boom[die].frame >= boom[die].frame_total)
    {
        MessageBox(NULL, L"Don't Get Hit!", L"Game Over", MB_OK);
        gameover = true;
    }

    if (score >= other_number)
    {
        MessageBox(NULL, L"Nice Shooting!", L"Game Over", MB_OK);
        gameover = true;
    }
}

void Updateother()
{
    Vector3 otherInitDirection(0.0f, 0.0f, -1.0f);
    float speed = 0.1;

    for (int i = 0; i < other_number; i++)
    {
        XMMATRIX rotate = XMMatrixRotationY(other[i].yrot);
        Vector3 direction = otherInitDirection.Transform(otherInitDirection, rotate);
        direction.Normalize();
        Vector3 pos;
        pos.x = other[i].x; pos.y = other[i].y; pos.z = other[i].z;
        pos = pos + direction * speed;
        other[i].x = pos.x; other[i].y = pos.y; other[i].z = pos.z;

        if (other[i].x < -ground.size.x / 2 || other[i].x > ground.size.x / 2
            || other[i].z < -ground.size.z / 2 || other[i].z > ground.size.z / 2)
            other[i].yrot += XM_PI;

        for (int j = 0; j < other_number; j++)
        {
            if (CheckModel3DCollided(other[i], other[j]))
            {
                other[i].yrot += XM_PI;
                other[j].yrot += XM_PI;
            }
        }
    }
    bool fireshot = false;

    for (int i = 0; i < other_number; i++)
    {
        if (valid_other[i] && CheckModel3DCollided(car, other[i]) && die < 0)
        {
            valid_other[i] = false;
            last_boom++;
            if (last_boom >= other_number)
                last_boom = 0;
            boom[last_boom].x = car.x;
            boom[last_boom].y = car.y;
            boom[last_boom].frame = 0;
            boom_valid[last_boom] = true;
            PlaySound(explode.get());
            die += 1;
            other_left--;
        }
    }
}

bool Game_Init(HWND hwnd)
{
    gameover = false;
    InitD3D(hwnd);
    InitInput(hwnd);
    InitSound();

    srand(GetTickCount());
    game_start_time = GetTickCount();

    camera.x = 0; 
    camera.y = 2; 
    camera.z = -15;
    SetCamera(0, 2, -15, 0, 0, 0);
    SetPerspective(XM_PI / 4.f, float(Width) / float(Height), 0.1f, 1000.f);

    bgmusic = LoadSound(L"music.wav");
    if (bgmusic == NULL)
        return false;
    if (gameover == false)
    {
        bgmusic->Play();
    }

    aim = CreateModel2D(L"target.png");
    aim.x = Width / 2; -aim.frame_width / 2 - 120;
    aim.y = Height / 2 - aim.frame_height / 2 - 120;

    Vector3 size = { 100, 0.01, 100 };
    CreateBox(ground, L"road.bmp", size, 100, 0.01, 100);

    if (CreateModel3DFromSdkmesh(car, L"car.sdkmesh") == false)
        return false;
    car.scale = 0.2; 
    car.z = 0; 
    car.y = 0;

    for (int i = 0; i < extra_number; i++)
    {
        if (CreateModel3DFromSdkmesh(extra[i], L"car.sdkmesh") == false)
            return false;
        extra[i].scale = 0.3;
        extra[i].x = rand() % 60 - 30;
        extra[i].z = rand() % 60 - 30;
        extra[i].y = 0;
        extra[i].yrot += 0.1;

        valid_extra[i] = true;
    }
    sound1 = LoadSound(L"sound1.wav");
    if (sound1 == NULL)
        return false;

    for (int i = 0; i < bomb_number; i++)
    {
        if (CreateModel3DFromSdkmesh(bomb[i], L"bomb.sdkmesh") == false)
            return false;
        bomb[i].scale = 0.9;
        bomb[i].x = rand() % 60 - 30;
        bomb[i].z = rand() % 60 - 30;
        bomb[i].y = 0;
        bomb[i].yrot += 0.5;

        valid_bomb[i] = true;
    }
    sound2 = LoadSound(L"sound2.wav");
    if (sound2 == NULL)
        return false;

    for (int i = 0; i < other_number; i++)
    {
        if (CreateModel3DFromSdkmesh(other[i], L"car.sdkmesh") == false)
            return false;
        other[i].scale = 0.25;
        other[i].x = rand() % 60 - 30;
        other[i].z = rand() % 60 - 30;
        other[i].y = 0;
        other[i].yrot = float(rand() % 360) / 360 * XM_PI * 2;

        valid_other[i] = true;
    }

    for (int i = 0; i < number_of_shots; i++)
    {
        if (CreateModel3DFromSdkmesh(shot[i], L"ball.sdkmesh") == false)
            return false;
        shot[i].x = car.x;
        shot[i].y = car.y;
        shot[i].z = car.z;
        shot[i].scale = 0.1;
        valid_shot[i] = false;
        gunfire = LoadSound(L"gunfire.wav");
        if (gunfire == NULL)
            return false;

        shot[i].model->UpdateEffects([&](IEffect* effect)
            {
                IEffectLights* lights = dynamic_cast<IEffectLights*>(effect);
                if (lights)
                {
                    lights->SetAmbientLightColor(Colors::White);
                }
            });

    }

    for (int i = 0; i < number_of_shots; i++)
    {
        boom[i] = CreateModel2D(L"boom.png", 8, 8);
        boom_valid[i] = false;
    }
    explode = LoadSound(L"explode.wav");
    if (explode == NULL)
        return false;

    try {
        spriteFont = std::make_unique<SpriteFont>(dev,
            L"times_new_roman.spritefont");
    }
    catch (std::runtime_error e)
    {
        MessageBox(NULL, L"Loading times_new_roman.spritefont error", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

void Game_Run()
{
    long static start = 0;
    float frame_interval = 1000.0 / framerate;

    game_current_time = GetTickCount();

    game_play_time_in_seconds = (game_current_time - game_start_time) / 1000;
    game_play_time_in_minutes = game_play_time_in_seconds / 60;
    game_play_time_in_hour = game_play_time_in_minutes / 60;
    seconds_count = game_play_time_in_seconds%60;
    minutes_count = game_play_time_in_minutes%60;
    hours_count = game_play_time_in_hour;
    if (hours_count >= 60)
    {
        gameover = true;
    }

    if (keyboard->GetState().Escape)
    {
        gameover = true;
    }

    if (GetTickCount() - start >= frame_interval)
    {
        start = GetTickCount();
        ClearScreen();
        mousetracker.Update(mouse->GetState());
        if (mousetracker.leftButton ==
            Mouse::ButtonStateTracker::PRESSED)
        {
            mb = mousetracker.GetLastState();
            Vector3 target = FindTarget(mb.x, mb.y);
            Launchshot(target);
        }

        Updatecar();
        UpdateAim();

        DrawShape3D(ground);
        DrawModel3D(car);

        if (game_state == 1) 
        {
                UpdateShots();
                Updateextra();
                Updatebomb();
                Updateother();

                for (int i = 0; i < extra_number; i++)
                    if (valid_extra[i])
                        DrawModel3D(extra[i]);

                for (int i = 0; i < bomb_number; i++)
                    if (valid_bomb[i])
                        DrawModel3D(bomb[i]);

                for (int i = 0; i < 10; i++)
                    if (valid_other[i])
                        DrawModel3D(other[i]);

                for (int i = 0; i < number_of_shots; i++)
                    if (valid_shot[i])
                        DrawModel3D(shot[i]);
            }
        if (score >= 10 && score < 15)
        {
            game_state = 2;
            UpdateShots();
            Updateextra();
            Updatebomb();
            Updateother();

            for (int i = 0; i < extra_number; i++)
                if (valid_extra[i])
                    DrawModel3D(extra[i]);

            for (int i = 0; i < bomb_number; i++)
                if (valid_bomb[i])
                    DrawModel3D(bomb[i]);

            for (int i = 10; i < 15; i++)
                if (valid_other[i])
                    DrawModel3D(other[i]);

            for (int i = 0; i < number_of_shots; i++)
                if (valid_shot[i])
                    DrawModel3D(shot[i]);
        }
        if (score >= 15 && score < 20)
        {
            game_state = 3;
            UpdateShots();
            Updateextra();
            Updatebomb();
            Updateother();

            for (int i = 0; i < extra_number; i++)
                if (valid_extra[i])
                    DrawModel3D(extra[i]);

            for (int i = 0; i < bomb_number; i++)
                if (valid_bomb[i])
                    DrawModel3D(bomb[i]);

            for (int i = 15; i < 20; i++)
                if (valid_other[i])
                    DrawModel3D(other[i]);

            for (int i = 0; i < number_of_shots; i++)
                if (valid_shot[i])
                    DrawModel3D(shot[i]);
        }
        if (score >= 20)
        {
            game_state = 4;
            UpdateShots();
            Updateextra();
            Updatebomb();
            Updateother();

            for (int i = 0; i < extra_number; i++)
                if (valid_extra[i])
                    DrawModel3D(extra[i]);

            for (int i = 0; i < bomb_number; i++)
                if (valid_bomb[i])
                    DrawModel3D(bomb[i]);

            for (int i = 20; i < 24; i++)
                if (valid_other[i])
                    DrawModel3D(other[i]);

            for (int i = 0; i < number_of_shots; i++)
                if (valid_shot[i])
                    DrawModel3D(shot[i]);
        }

        spriteBatch->Begin();
        DrawModel2D(aim);
        Updatebooms();
        for (int i = 0; i < number_of_shots; i++)
            if (boom_valid[i])
                DrawModel2D(boom[i]);
        wchar_t s[80];
        swprintf(s, 80, L"Shots Left: %i / %i", shots_left, number_of_shots);
        spriteFont->DrawString(spriteBatch.get(), s,
            XMFLOAT2(25, 25), Colors::Yellow);
        swprintf(s, 80, L"Other Cars Left: %i / %i", other_left, other_number);
        spriteFont->DrawString(spriteBatch.get(), s,
            XMFLOAT2(25, 50), Colors::Yellow);
        swprintf(s, 80, L"Extra Life Cars Left: %i / %i", extra_left, extra_number);
        spriteFont->DrawString(spriteBatch.get(), s,
            XMFLOAT2(25, 75), Colors::LightGreen);
        swprintf(s, 80, L"Bomb Cars Left: %i / %i", bomb_left, bomb_number);
        spriteFont->DrawString(spriteBatch.get(), s,
            XMFLOAT2(25, 100), Colors::Red);
        swprintf(s, 80, L"Game Running TIme: %i : %i : %i", hours_count, minutes_count, seconds_count);
        spriteFont->DrawString(spriteBatch.get(), s,
            XMFLOAT2(25, 125), Colors::LightCyan);
        if (game_state == 1)
        {
            swprintf(s, 80, L"Round: %i / 4", game_state);
            spriteFont->DrawString(spriteBatch.get(), s,
                XMFLOAT2(25, 150), Colors::LightCyan);
        }
        else if (game_state == 2)
        {
            swprintf(s, 80, L"Round: %i / 4", game_state);
            spriteFont->DrawString(spriteBatch.get(), s,
                XMFLOAT2(25, 150), Colors::LightPink);
        }
        else if (game_state == 3)
        {
            swprintf(s, 80, L"Round: %i / 4", game_state);
            spriteFont->DrawString(spriteBatch.get(), s,
                XMFLOAT2(25, 150), Colors::LightGreen);
        }
        else if (game_state == 4)
        {
            swprintf(s, 80, L"Round: %i / 4", game_state);
            spriteFont->DrawString(spriteBatch.get(), s,
                XMFLOAT2(25, 150), Colors::LightSeaGreen);
        }
        spriteBatch->End();

        swapchain->Present(0, 0);
    }
}

void Game_End()
{
    bgmusic.release();
    gunfire.release();
    aim.texture->Release();
    ground.shape.release();

    if (ground.texture != NULL)
        ground.texture->Release();
    car.model.release();
    for (int i = 0; i < extra_number; i++)
        extra[i].model.release();
    for (int i = 0; i < bomb_number; i++)
        bomb[i].model.release();
    for (int i = 0; i < other_number; i++)
        other[i].model.release();
    for (int i = 0; i < number_of_shots; i++)
        shot[i].model.release();
    CleanD3D();
}