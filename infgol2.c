#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>

typedef struct Chunk
{
	char * data; // N x N array
	int size; // N
	int x, y;
	int keepAlive;
	struct Chunk * neighbors[8];
} Chunk_t;

Vector2 viewPosition = {0, 0};
float viewScale = 10;
int worldMouseX, worldMouseY;
int chunkMouseX, chunkMouseY;
int chunkMouseCol, chunkMouseRow;
Chunk_t * chunkMouse;
int numChunks = 0;
Chunk_t ** board = NULL;
double simulationInterval = 1;
int pause = 1;

void forEachCell(Chunk_t * chunk, void (*operation)(Chunk_t *, int, int))
{
	for (int row = 0; row < chunk->size; row++) {
		for (int col = 0; col < chunk->size; col++)
		{
			(*operation)(chunk, row, col);
		}
	}
}

int checkCell(Chunk_t * chunk, int row, int col)
{
	int neighbor = 0;
	neighbor += 3 * (row >= 0);
	neighbor += 3 * (row >= chunk->size);
	neighbor += col >= 0;
	neighbor += col >= chunk->size;

	if (neighbor == 4)
	{
		return chunk->data[row * chunk->size + col];
	}

	row = row % chunk->size + (row < 0) * chunk->size;
	col = col % chunk->size + (col < 0) * chunk->size;

	if (neighbor >= 5)
	{
		neighbor -= 1;
	}

	if (chunk->neighbors[neighbor] == NULL)
	{
		return 0;
	}

	return chunk->neighbors[neighbor]->data[row * chunk->size + col];
}

void initCell(Chunk_t * chunk, int row, int col)
{
	chunk->data[row * chunk->size + col] = rand() % 2;
}

void updateCell(Chunk_t * chunk, int row, int col)
{
	int sum = 0;
	sum += checkCell(chunk, row + -1, col + -1) % 2;
	sum += checkCell(chunk, row + -1, col +  0) % 2;
	sum += checkCell(chunk, row + -1, col +  1) % 2;
	sum += checkCell(chunk, row +  0, col + -1) % 2;
	// skip middle cell!    row +  0, col +  0
	sum += checkCell(chunk, row +  0, col +  1) % 2;
	sum += checkCell(chunk, row +  1, col + -1) % 2;
	sum += checkCell(chunk, row +  1, col +  0) % 2;
	sum += checkCell(chunk, row +  1, col +  1) % 2;

	char * cell = &chunk->data[chunk->size * row + col];

	if (*cell)
	{
		chunk->keepAlive = 1;
	}

	if (sum == 2)
	{
		*cell += 2 * (1 && *cell);
	}
	else if (sum == 3)
	{
		*cell += 2;
	}
}

void fixCell(Chunk_t * chunk, int row, int col)
{
	char * cell = &chunk->data[chunk->size * row + col];
	*cell = *cell > 1;
}

int worldToScreenX(float x, Vector2 viewPosition, float viewScale)
{
	return (int) ((x - viewPosition.x) * viewScale + GetScreenWidth() / 2);
}

int worldToScreenY(float y, Vector2 viewPosition, float viewScale)
{
	return (int) ((y - viewPosition.y) * viewScale + GetScreenHeight() / 2);
}

double screenToWorldX(int x, Vector2 viewPosition, float viewScale)
{
	return (double) (x - GetScreenWidth() / 2) / viewScale + viewPosition.x;
}

double screenToWorldY(int y, Vector2 viewPosition, float viewScale)
{
	return (double) (y - GetScreenHeight() / 2) / viewScale + viewPosition.y;
}

void drawChunk(Chunk_t * chunk, Vector2 viewPosition, float viewScale)
{
	int chunkScreenX = worldToScreenX(chunk->size * chunk->x, viewPosition, viewScale);
	int chunkScreenY = worldToScreenY(chunk->size * chunk->y, viewPosition, viewScale);
	int chunkScreenSize = (int) (chunk->size * viewScale);
	DrawRectangleLines(chunkScreenX, chunkScreenY, chunkScreenSize, chunkScreenSize, GRAY);

	for (int row = 0; row < chunk->size; row++)
	{
		for (int col = 0; col < chunk->size; col++)
		{
			if (chunk->data[chunk->size * row + col] == 0) {
				continue;
			}

			int cellX = worldToScreenX(chunk->size * chunk->x + col, viewPosition, viewScale);
			int cellY = worldToScreenY(chunk->size * chunk->y + row, viewPosition, viewScale);
			int cellSize = (int) viewScale + 1;
			DrawRectangle(cellX, cellY, cellSize, cellSize, WHITE);
		}
	}
}

void createChunk(Chunk_t *** board, int * numChunks, int x, int y)
{
	*numChunks += 1;
	Chunk_t ** newBoard = realloc(*board, *numChunks * sizeof(Chunk_t *));
	if (newBoard == NULL)
	{
		for (int chunk = 0; chunk < *numChunks - 1; chunk++)
		{
			free((*board)[chunk]->data);
			free((*board)[chunk]);
		}
		free(*board);
		exit(1);
	}
	*board = newBoard;

	(*board)[*numChunks - 1] = malloc(sizeof(Chunk_t));
	if ((*board)[*numChunks - 1] == NULL)
	{
		for (int chunk = 0; chunk < *numChunks - 1; chunk++)
		{
			free((*board)[chunk]->data);
			free((*board)[chunk]);
		}
		free(*board);
		exit(1);
	}
	Chunk_t * newChunk = (*board)[*numChunks - 1];
	newChunk->size = 16;
	newChunk->x = x;
	newChunk->y = y;
	newChunk->keepAlive = 0;
	for (int neighbor = 0; neighbor < 8; neighbor++)
	{
		newChunk->neighbors[neighbor] = NULL;
	}
	newChunk->data = calloc(newChunk->size * newChunk->size, sizeof(char));
	if (newChunk->data == NULL)
	{
		for (int chunk = 0; chunk < *numChunks - 1; chunk++)
		{
			free((*board)[chunk]->data);
			free((*board)[chunk]);
		}
		free(*board);
		exit(1);
	}

	for (int chunk = 0; chunk < *numChunks; chunk++)
	{
		if ( (*board)[chunk]->x < x - 1
				|| (*board)[chunk]->y < y - 1
				|| (*board)[chunk]->x > x + 1
				|| (*board)[chunk]->y > y + 1
				)
		{
			continue;
		}

		int neighbor = 0;
		neighbor += 3 * ((*board)[chunk]->y - y + 1);
		neighbor += ((*board)[chunk]->x - x + 1);

		if (neighbor == 4)
		{
			continue;
		}

		if (neighbor >= 5)
		{
			neighbor -= 1;
		}

		(*board)[chunk]->neighbors[7 - neighbor] = newChunk;
		newChunk->neighbors[neighbor] = (*board)[chunk];
	}
}

void handleUserInput(void)
{
	worldMouseX = (int) floor(screenToWorldX(GetMouseX(), viewPosition, viewScale));
	worldMouseY = (int) floor(screenToWorldY(GetMouseY(), viewPosition, viewScale));
	chunkMouseX = (int) floor((double) worldMouseX / board[0]->size);
	chunkMouseY = (int) floor((double) worldMouseY / board[0]->size);
	chunkMouse = NULL;
	for (int chunk = 0; chunk < numChunks; chunk++)
	{
		if (board[chunk]->x == chunkMouseX && board[chunk]->y == chunkMouseY)
		{
			chunkMouse = board[chunk];
			break;
		}
	}
	
	if (chunkMouse != NULL)
	{
		chunkMouseRow = worldMouseY % chunkMouse->size + (worldMouseY < 0) * chunkMouse->size;
		chunkMouseCol = worldMouseX % chunkMouse->size + (worldMouseX < 0) * chunkMouse->size;
		
		if (IsKeyPressed(KEY_I))
		{
			chunkMouse->data[chunkMouseRow * chunkMouse->size + chunkMouseCol] = 1;
		}
	}

	if (IsKeyDown(KEY_H))
	{
		viewPosition = Vector2Add(viewPosition, (Vector2){-0.5, 0});
	}
	if (IsKeyDown(KEY_J))
	{
		viewPosition = Vector2Add(viewPosition, (Vector2){0, 0.5});
	}
	if (IsKeyDown(KEY_K))
	{
		viewPosition = Vector2Add(viewPosition, (Vector2){0, -0.5});
	}
	if (IsKeyDown(KEY_L))
	{
		viewPosition = Vector2Add(viewPosition, (Vector2){0.5, 0});
	}

	if (IsKeyDown(KEY_N))
	{
		viewScale /= 1.1;
	}
	if (IsKeyDown(KEY_M))
	{
		viewScale *= 1.1;
	}

	if (IsKeyDown(KEY_Q))
	{
		simulationInterval /= 1.1;
	}
	if (IsKeyDown(KEY_W))
	{
		simulationInterval *= 1.1;
	}

	if (IsKeyPressed(KEY_Z))
	{
		pause = !pause;
	}
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

		for (int chunk = 0; chunk < numChunks; chunk++)
		{
			board[chunk]->keepAlive = 0;
			forEachCell(board[chunk], updateCell);

			if (board[chunk]->keepAlive == 0)
			{
				continue;
			}

			for (int neighbor = 0; neighbor < 8; neighbor++)
			{
				if (board[chunk]->neighbors[neighbor] != NULL)
				{
					continue;
				}

				int neighborIdx = neighbor;
				if (neighborIdx >= 4)
				{
					neighborIdx += 1;
				}

				int newX = board[chunk]->x + (neighborIdx % 3 - 1);
				int newY = board[chunk]->y + (neighborIdx / 3 - 1);
				createChunk(&board, &numChunks, newX, newY);
			}
		}

		for (int chunk = 0; chunk < numChunks; chunk++)
		{
			forEachCell(board[chunk], fixCell);

			if (board[chunk]->keepAlive == 1)
			{
				continue;
			}

			int preserve = 0;
			for (int neighbor = 0; neighbor < 8; neighbor++)
			{
				if (board[chunk]->neighbors[neighbor] == NULL)
				{
					continue;
				}

				if (board[chunk]->neighbors[neighbor]->keepAlive == 1)
				{
					preserve = 1;
					break;
				}
			}

			if (preserve == 1)
			{
				continue;
			}

			for (int neighbor = 0; neighbor < 8; neighbor++)
			{
				if (board[chunk]->neighbors[neighbor] == NULL)
				{
					continue;
				}

				board[chunk]->neighbors[neighbor]->neighbors[7 - neighbor] = NULL;
			}

			free(board[chunk]->data);
			free(board[chunk]);
			board[chunk] = board[numChunks - 1];
			numChunks -= 1;
		}
	}
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

	createChunk(&board, &numChunks, 0, 0);
	forEachCell(board[0], initCell);

	pthread_t worldSimulationThread;
	pthread_create(&worldSimulationThread, NULL, handleWorldSimulation, NULL);

	while (!WindowShouldClose())
	{
		handleUserInput();

		BeginDrawing();
		ClearBackground(BLACK);

		for (int chunk = 0; chunk < numChunks; chunk++)
		{
			drawChunk(board[chunk], viewPosition, viewScale);
		}

		if (chunkMouse != NULL)
		{
			char mouseDisplay[256];
			snprintf(mouseDisplay, 256, "chunk->x: %d\nchunk->y: %d\nchunkMouseCol: %d\nchunkMouseRow: %d\ncheckCell: %d", chunkMouse->x, chunkMouse->y, chunkMouseCol, chunkMouseRow, checkCell(chunkMouse, chunkMouseRow, chunkMouseCol));
			DrawText(mouseDisplay, 10, 10, 24, GRAY);
		}

		EndDrawing();
	}

	simulationInterval = -1;
	pthread_join(worldSimulationThread, NULL);
	for (int chunk = 0; chunk < numChunks; chunk++)
	{
		free(board[chunk]->data);
		free(board[chunk]);
	}
	free(board);
	CloseWindow();
	return 0;
}
