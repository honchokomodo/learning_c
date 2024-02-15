// gcc -o infgol3 infgol3.c 2DChunks.c -lraylib -Wall -Wextra -O3 -ggdb
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>
#include "2DChunks.h"

Board_t board;
Camera2D camera;
double simulationInterval = 0.1;
int pause = 1;

void handleUserInput(void)
{
	Vector2 vel = (Vector2){0, 0};
	if (IsKeyDown(KEY_H))
	{
		vel.x += -1;
	}
	if (IsKeyDown(KEY_J))
	{
		vel.y += 1;
	}
	if (IsKeyDown(KEY_K))
	{
		vel.y += -1;
	}
	if (IsKeyDown(KEY_L))
	{
		vel.x += 1;
	}

	if (IsKeyDown(KEY_N))
	{
		camera.zoom *= 1.1;
	}
	if (IsKeyDown(KEY_M))
	{
		camera.zoom /= 1.1;
	}

	if (IsKeyPressed(KEY_W))
	{
		simulationInterval *= 1.5;
	}
	if (IsKeyPressed(KEY_Q))
	{
		simulationInterval /= 1.5;
	}

	if (IsKeyPressed(KEY_Z))
	{
		pause = !pause;
	}

	vel = Vector2Scale(vel, 2 / camera.zoom);
	camera.target = Vector2Add(camera.target, vel);
}

int checkCell(Cell_t * cell)
{
	if (cell == NULL)
	{
		return 0;
	}

	return *cell;
}

void * handleWorldSimulation(void *)
{
	double time_counter = 0;

	clock_t this_time = clock();
	clock_t last_time = this_time;

	while (simulationInterval > 0)
	{
		this_time = clock();
		time_counter += (double)(this_time - last_time);
		last_time = this_time;
		if (time_counter < simulationInterval * CLOCKS_PER_SEC)
		{
			continue;
		}
		time_counter -= simulationInterval * CLOCKS_PER_SEC;

		if (pause)
		{
			continue;
		}

		// handle world simulation
		for (int bucket = 0; bucket < board.numBuckets; bucket++) //TODO: FIX THIS HORRIBLE FOR-STACK!
		{
			for (Chunk_t * chunk = board.chunks[bucket]; chunk != NULL; chunk = chunk->next)
			{
				chunk->keepAlive = 0;

				for (int row = 0; row < board.chunkSize; row++)
				{
					for (int col = 0; col < board.chunkSize; col++)
					{
						Cell_t * cell = getCellChunk(chunk, col, row);

						int sum = 0;
						sum += checkCell(getCellChunk(chunk, col -1, row -1)) % 2;
						sum += checkCell(getCellChunk(chunk, col   , row -1)) % 2;
						sum += checkCell(getCellChunk(chunk, col +1, row -1)) % 2;
						sum += checkCell(getCellChunk(chunk, col -1, row   )) % 2;
						// skip middle cell!                 col     row
						sum += checkCell(getCellChunk(chunk, col +1, row   )) % 2;
						sum += checkCell(getCellChunk(chunk, col -1, row +1)) % 2;
						sum += checkCell(getCellChunk(chunk, col   , row +1)) % 2;
						sum += checkCell(getCellChunk(chunk, col +1, row +1)) % 2;

						if (sum > 0)
						{
							chunk->keepAlive = 1;
						}

						if (sum == 2)
						{
							*cell += 2 * *cell;
						}
						else if (sum == 3)
						{
							*cell += 2;
						}
					}
				} // end loop through cells
			}
		} // end loop through chunks

		for (int bucket = 0; bucket < board.numBuckets; bucket++) //TODO: FIX THIS HORRIBLE FOR-STACK!
		{
			Chunk_t * next;
			for (Chunk_t * chunk = board.chunks[bucket]; chunk != NULL; chunk = next)
			{
				next = chunk->next;
				for (int row = 0; row < board.chunkSize; row++)
				{
					for (int col = 0; col < board.chunkSize; col++)
					{
						Cell_t * cell = getCellChunk(chunk, col, row);
						*cell = *cell > 1;
					}
				} // end loop through cells

				if (chunk->keepAlive == 1)
				{
					for (int neighbor = 0; neighbor < 9; neighbor++)
					{
						int neighborChunkX = chunk->x + (neighbor % 3 - 1);
						int neighborChunkY = chunk->y + (neighbor / 3 - 1);
						createChunk(&board, neighborChunkX, neighborChunkY);
					}
				}

				if (chunk->keepAlive == 0)
				{
					int preserve = 0;

					for (int neighbor = 0; neighbor < 9; neighbor++)
					{
						int neighborChunkX = chunk->x + (neighbor % 3 - 1);
						int neighborChunkY = chunk->y + (neighbor / 3 - 1);
						Chunk_t * neighborChunk = getChunk(&board, neighborChunkX, neighborChunkY);

						if (neighborChunk == NULL)
						{
							continue;
						}

						if (neighborChunk->keepAlive == 1)
						{
							preserve = 1;
							break;
						}
					}

					if (preserve == 0)
					{
						destroyChunk(&board, chunk->x, chunk->y);
					}
				}
			}
		} // end loop through chunks

	} // end while

	return NULL;
}

int main(void)
{
	int screenWidth = 512;
	int screenHeight = 512;
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "");
	SetTargetFPS(60);
	srand(time(NULL));

	board = createBoard(4, 32);
	createChunk(&board, 0, 0);

	Chunk_t * starterchunk = getChunk(&board, 0, 0);
	for (int cell = 0; cell < board.chunkSize * board.chunkSize; cell++)
	{
		starterchunk->cells[cell] = rand() % 2;
	}

	camera.offset = (Vector2){screenWidth / 2, screenHeight / 2};
	camera.target = (Vector2){0, 0};
	camera.rotation = 0;
	camera.zoom = 1;

	pthread_t worldSimulationThread;
	pthread_create(&worldSimulationThread, NULL, handleWorldSimulation, NULL);

	while (!WindowShouldClose())
	{
		handleUserInput();

		camera.offset = (Vector2){GetScreenWidth() / 2, GetScreenHeight() / 2};
		BeginDrawing();
		BeginMode2D(camera);
		ClearBackground(BLACK);

		Vector2 mousepos = GetScreenToWorld2D(GetMousePosition(), camera);
		int mouseChunkX = (int) floor(mousepos.x / board.chunkSize);
		int mouseChunkY = (int) floor(mousepos.y / board.chunkSize);
		Chunk_t * mouseChunk = getChunk(&board, mouseChunkX, mouseChunkY);

		Color color;
		for (int bucket = 0; bucket < board.numBuckets; bucket++) //TODO: FIX THIS HORRIBLE FOR-STACK!
		{
			for (Chunk_t * chunk = board.chunks[bucket]; chunk != NULL; chunk = chunk->next)
			{
				color = GRAY;
				if (mouseChunk != NULL && mouseChunk->x == chunk->x && mouseChunk->y == chunk->y)
				{
					color = RED;
				}
				DrawRectangleLines(chunk->x * board.chunkSize, chunk->y*board.chunkSize, board.chunkSize, board.chunkSize, color);
				for (int row = 0; row < board.chunkSize; row++)
				{
					for (int col = 0; col < board.chunkSize; col++)
					{
						Cell_t * cell = &chunk->cells[board.chunkSize * row + col];
						if (*cell == 1)
						{
							DrawRectangle(chunk->x * board.chunkSize + col, chunk->y * board.chunkSize + row, 1, 1, WHITE);
						}
					}
				}
			}
		}

		EndMode2D();

		char mousedisplay[256];
		if (mouseChunk == NULL)
		{
			snprintf(mousedisplay, 256, "mouseChunkX: %d\nmouseChunkY: %d", mouseChunkX, mouseChunkY);
		}
		else
		{
			snprintf(mousedisplay, 256, "mouseChunkX: %d\nmouseChunkY: %d\nmouseChunk->x: %d\nmouseChunk->y: %d\nmouseChunk->keepAlive: %d", mouseChunkX, mouseChunkY, mouseChunk->x, mouseChunk->y, mouseChunk->keepAlive);
		}
		DrawText(mousedisplay, 10, 10, 20, WHITE);

		EndDrawing();
	}

	simulationInterval = -1;
	pthread_join(worldSimulationThread, NULL);
	CloseWindow();
	return 0;
}
