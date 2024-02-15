/**
 * 2D Chunks -- store and access cells on an infinite grid
 */

typedef int Cell_t; // might change to a struct later on

typedef struct Board Board_t;
typedef struct Chunk Chunk_t;

struct Board {
	int numBuckets;
	int chunkSize;
	struct Chunk ** chunks;
};

struct Chunk {
	Cell_t * cells; // size * size array
	int x, y;
	int keepAlive;
	struct Chunk * next;
	struct Chunk ** prevNext;
	struct Board * parent;
};

// create a board to store cells on
Board_t createBoard(int numBuckets, int chunkSize);

// destroy board and free memory
void destroyBoard(Board_t * board);

// access a chunk
Chunk_t * getChunk(Board_t * board, int x, int y);

// create a chunk
void createChunk(Board_t * board, int x, int y);

// destroy a chunk and free memory
void destroyChunk(Chunk_t * chunk);

// access a cell from a chunk
Cell_t * getCellChunk(Chunk_t * chunk, int col, int row);

// access a cell from a board
Cell_t * getCellBoard(Board_t * board, int x, int y);
