#include <stdlib.h>
#include "2DChunks.h"
#include "mathhelper.h"

// generate a hash based off of x and y position
int genHash(int x, int y)
{
	// these two could be anything really
	int bigNum = 2654435761;
	int primeNum = 7331;
	int hash = ((primeNum + x * bigNum) * primeNum + y * bigNum);
	return hash;
}

// create a board to store cells on
Board_t createBoard(int numBuckets, int chunkSize)
{
	Board_t newBoard;
	newBoard.numBuckets = numBuckets;
	newBoard.chunkSize = chunkSize;
	newBoard.chunks = calloc(numBuckets, sizeof(Chunk_t *));

	if (newBoard.chunks == NULL)
	{
		destroyBoard(&newBoard);
		exit(1);
	}

	return newBoard;
}

// destroy board and free memory
void destroyBoard(Board_t * board)
{
	for (int bucket = 0; bucket < board->numBuckets; bucket++)
	{
		Chunk_t * chunk = board->chunks[bucket];

		// go down the linked list and free everything
		while (chunk != NULL)
		{
			Chunk_t * nextChunk = chunk->next;
			if (chunk->cells != NULL)
			{
				free(chunk->cells);
			}
			free(chunk);
			chunk = nextChunk;
		}
	}

	free(board->chunks);
}

// access a chunk
Chunk_t * getChunk(Board_t * board, int x, int y)
{
	int hash = cmodulus(genHash(x, y), board->numBuckets);
	Chunk_t * chunk = board->chunks[hash];

	// go down the linked list until the chunk is found
	for (; chunk != NULL; chunk = chunk->next)
	{
		if (chunk->x == x && chunk->y == y)
		{
			return chunk;
		}
	}

	return NULL;
}

// create a chunk
void createChunk(Board_t * board, int x, int y)
{
	if (getChunk(board, x, y) != NULL)
	{
		return; // do nothing if a chunk already exists
	}

	int hash = cmodulus(genHash(x, y), board->numBuckets);
	Chunk_t * prevHead = board->chunks[hash];
	Chunk_t * newChunk = malloc(sizeof(Chunk_t));

	if (newChunk == NULL)
	{
		destroyBoard(board);
		exit(1);
	}
	
	int size = board->chunkSize;
	
	newChunk->x = x;
	newChunk->y = y;
	newChunk->keepAlive = 0;
	newChunk->next = prevHead;
	newChunk->prevNext = &board->chunks[hash];
	newChunk->parent = board;
	newChunk->cells = calloc(size * size, sizeof(Cell_t));

	if (newChunk->cells == NULL)
	{
		destroyBoard(board);
		exit(1);
	}

	if (prevHead != NULL)
	{
		prevHead->prevNext = &newChunk->next;
	}

	board->chunks[hash] = newChunk;
}

// destroy a chunk and free memory
void destroyChunk(Chunk_t * chunk)
{
	if (chunk->next != NULL)
	{
		chunk->next->prevNext = chunk->prevNext;
	}
	*(chunk->prevNext) = chunk->next;

	free(chunk->cells);
	free(chunk);
}

// access a cell from a chunk
Cell_t * getCellChunk(Chunk_t * chunk, int col, int row)
{
	if (chunk == NULL)
	{
		return NULL;
	}

	Cell_t * cell;
	int size = chunk->parent->chunkSize;
	if (row < 0 || col < 0 || row >= size || col >= size)
	{
		int chunkX = chunk->x * size + col;
		int chunkY = chunk->y * size + row;

		cell = getCellBoard(chunk->parent, chunkX, chunkY);
	}
	else
	{
		int idx = size * row + col;
		cell = &chunk->cells[idx];
	}

	return cell;
}

// access a cell from a board
Cell_t * getCellBoard(Board_t * board, int x, int y)
{
	int chunkX = fdiv(x, board->chunkSize);
	int chunkY = fdiv(y, board->chunkSize);
	int col = cmodulus(x, board->chunkSize);
	int row = cmodulus(y, board->chunkSize);

	Chunk_t * chunk = getChunk(board, chunkX, chunkY);
	Cell_t * cell = getCellChunk(chunk, col, row);
	return cell;
}
