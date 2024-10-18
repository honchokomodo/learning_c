#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(512, 512, "");
	SetTargetFPS(60);
	HideCursor();

	Camera2D camera = {
		.offset = {.x = 0, .y = 0},
		.target = {.x = 0, .y = 0},
		.rotation = 0,
		.zoom = 1
	};
	float sensitivity = 1;

	RenderTexture2D target = LoadRenderTexture(1500, 2000);
	BeginTextureMode(target);
	ClearBackground(WHITE);
	EndTextureMode();

	Color brushcolor = {.a = 0xFF};
	float brushsize = 3;
	Vector2 brushpos = {0};

	while (!WindowShouldClose())
	{
		Vector2 mousepos = GetMousePosition();
		Vector2 mousemove = GetMouseDelta();
		Vector2 screencenter = {
			.x = GetScreenWidth() / 2,
			.y = GetScreenHeight() / 2
		};

		if (IsKeyDown(KEY_A))
		{
			sensitivity = 0.2;
		}
		else
		{
			sensitivity = 1;
		}

		mousemove = Vector2Scale(mousemove, sensitivity);
		camera.offset = screencenter;
		Vector2 move = Vector2Scale(mousemove, 1.0 / camera.zoom);
		SetMousePosition(screencenter.x, screencenter.y);

		Vector2 mouseworld = GetScreenToWorld2D(screencenter, camera);
		float rinner = brushsize;

		float scroll = GetMouseWheelMove();
		int hasKeyPressed = 0;
		if (IsKeyDown(KEY_F))
		{
			brushsize += mousemove.x;
			brushsize = brushsize < 0 ? 0 : brushsize;
			hasKeyPressed = 1;
		}
		if (IsKeyDown(KEY_D))
		{
			rinner = 0;
			int bc = brushcolor.r;
			bc += mousemove.x;
			bc = bc < 0 ? 0 : bc;
			bc = bc > 0xFF ? 0xFF : bc;
			brushcolor.r = bc;
			brushcolor.g = bc;
			brushcolor.b = bc;
			hasKeyPressed = 1;
		}
		if (!hasKeyPressed)
		{
			camera.target = Vector2Add(camera.target, move);
		}
		if (!hasKeyPressed && scroll != 0)
		{
			camera.zoom *= exp(scroll / 4);
		}

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			Vector2 prepos = brushpos;
			brushpos = Vector2Lerp(brushpos, mouseworld, 0.5);
			Vector2 brushmove = Vector2Subtract(brushpos, prepos);

			BeginTextureMode(target);
			float realsize = brushsize / camera.zoom;
			realsize *= Vector2Length(brushmove);
			realsize = realsize < 0.5 ? 0.5 : realsize;
			//DrawCircleV(brushpos, realsize, brushcolor);
			DrawLineEx(prepos, brushpos, realsize, brushcolor);
			EndTextureMode();
		}
		else
		{
			brushpos = mouseworld;
		}
		
		if (IsKeyPressed(KEY_W))
		{
			Image image = LoadImageFromTexture(target.texture);
			ImageFlipVertical(&image);
			ExportImage(image, "myimage.png");
			UnloadImage(image);
		}

		BeginDrawing();
		ClearBackground(BLACK);

		BeginMode2D(camera);

		DrawCircle(0, 0, 10, WHITE);

		Rectangle srcrec = {
			.x = 0, .y = 0,
			.width = target.texture.width,
			.height = -target.texture.height
		};
		DrawTextureRec(target.texture, srcrec, Vector2Zero(), WHITE);

		EndMode2D();

		char display[64];
		snprintf(display, sizeof(display), "mouse at %g, %g", mouseworld.x, mouseworld.y);
		DrawText(display, 10, 10, 20, WHITE);
		snprintf(display, sizeof(display), "color: %02x%02x%02x%02x", brushcolor.r, brushcolor.g, brushcolor.b, brushcolor.a);
		DrawText(display, 10, 30, 20, WHITE);

		DrawRing(screencenter, rinner, brushsize + 5, 0, 360, 0, brushcolor);
		DrawRing(screencenter, brushsize, brushsize + 2, 0, 360, 0, BLACK);
		DrawRing(screencenter, brushsize, brushsize + 1, 0, 360, 0, WHITE);

		EndDrawing();
	}

	UnloadRenderTexture(target);
	CloseWindow();
	return 0;
}

