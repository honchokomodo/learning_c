/* makefile
main: main.c
	gcc -o main main.c -Wall -Wextra -lm -lpthread -lraylib -O3
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>

/* remove this when updating raylib to something past version 5 or soemething */

int CheckCollisionCircleLine(Vector2 center, float radius, Vector2 p1, Vector2 p2)
{
	float dx = p1.x - p2.x;
	float dy = p1.y - p2.y;

	if ((fabsf(dx) + fabsf(dy)) <= 0.00001) // small number
	{
		return CheckCollisionCircles(p1, 0, center, radius);
	}

	float lengthSQ = dx * dx + dy * dy;
	float dotProduct = (center.x - p1.x) * (p2.x - p1.x) + (center.y - p1.y) * (p2.y - p1.y) / lengthSQ;

	dotProduct = Clamp(dotProduct, 0, 1);

	float dx2 = (p1.x - (dotProduct * (dx))) - center.x;
	float dy2 = (p1.y - (dotProduct * (dy))) - center.y;
	float distanceSQ = ((dx2 * dx2) + (dy2 * dy2));

	return (distanceSQ <= radius * radius);
}

/* end thing */

#define FRAMES_PER_SECOND 60
#define TICKS_PER_SECOND 20
#define ATTACK_COOLDOWN_TICKS 4

char isKill = 0;
char isPaused = 0;
double simulationIntervalSeconds = 1.0 / TICKS_PER_SECOND;
int framesPerTick = FRAMES_PER_SECOND / TICKS_PER_SECOND;
float frameDivision; // for frame interpolation/prediction
Camera2D camera = {0};

Vector2 playerPosition = {0};
Vector2 playerVelocity = {0};
int playerhp = 200;
float playersize = 10;
int numKills = 0;
int attackCooldown = 0;

struct enemy {
	Vector2 position;
	Vector2 velocity;
	Color color;
	int hp;
	float size;
	float speed;
	//int damageCooldown;
};

struct enemy * enemies;
int enemiesQuantity;
int enemiesCapacity;

struct damagefield {
	Vector2 position;
	float radius;
	int damage;
	int lifetime;
};

struct damagefield * damagefields;
int damagefieldsQuantity;
int damagefieldsCapacity;

struct projectile {
	Vector2 position;
	Vector2 velocity;
	float radius;
	int lifetime;
	void (*onAttack)(struct projectile);
};

struct projectile * projectiles;
int projectilesQuantity;
int projectilesCapacity;

/*
struct crop {
	Vector2 position;
};
*/

void handleRealloc(void ** list, int * quantity, int * capacity, size_t size)
{
	if (*quantity < *capacity)
	{
		return;
	}

	*capacity *= 2;
	void * newList = realloc(*list, *capacity * size);
	if (newList == NULL)
	{
		printf("REALLOC FAIL!! CRASHING NOW\n");
		exit(1);
	}
	*list = newList;
}

void createEnemy(void)
{
	float angle = 2 * M_PI * rand() / RAND_MAX;
	float spawnDistance = 3000; // make this really big, like 3000--5000

	struct enemy new = {0};
	new.position = (Vector2) {
		.x = spawnDistance * cos(angle),
		.y = spawnDistance * sin(angle)
	};
	new.position = Vector2Add(new.position, playerPosition);
	new.color = RED;
	new.hp = 20;
	new.size = 8 + rand() % 5;
	new.speed = 2 + rand() % 15;

	enemies[enemiesQuantity] = new;
	enemiesQuantity++;
	
	handleRealloc(
			(void **) &enemies,
			&enemiesQuantity,
			&enemiesCapacity,
			sizeof(struct enemy)
			);
}

void createDamagefield(struct damagefield new)
{
	damagefields[damagefieldsQuantity] = new;
	damagefieldsQuantity++;

	handleRealloc(
			(void **) &damagefields,
			&damagefieldsQuantity,
			&damagefieldsCapacity,
			sizeof(struct damagefield)
			);
}

void createProjectile(struct projectile new)
{
	projectiles[projectilesQuantity] = new;
	projectilesQuantity++;

	handleRealloc(
			(void **) &projectiles,
			&projectilesQuantity,
			&projectilesCapacity,
			sizeof(struct projectile)
			);
}

/* ===== PROJECTILES ON-ATTACK SECTION ===== */
// all functions here are void funcname(struct projectile)

void riceAttack(struct projectile parent)
{
	struct damagefield new = {0};
	new.position = parent.position;
	new.radius = 3;
	new.damage = 1;
	new.lifetime = 1;

	createDamagefield(new);
}

void melonAttack(struct projectile parent)
{
	struct damagefield new = {0};
	new.position = parent.position;
	new.radius = 30;
	new.damage = 5;
	new.lifetime = 1;

	createDamagefield(new);

	for (int num = 0; num < 8; num++)
	{
		float speed = 10;
		float angle = 2 * M_PI * rand() / RAND_MAX;
		Vector2 v = (Vector2) {
			.x = speed * cos(angle),
			.y = speed * sin(angle)
		};

		struct projectile new = (struct projectile) {
			.position = parent.position,
			.velocity = v,
			.radius = 1,
			.lifetime = TICKS_PER_SECOND,
			.onAttack = riceAttack
		};
		createProjectile(new);
	}
}

/* ===== END PROJECTILES SECTION ===== */

void handlePlayerInput(void)
{
	float playerWalkSpeed = 4;
	if (IsKeyDown(KEY_W))
	{
		playerVelocity.y -= playerWalkSpeed;
	}
	if (IsKeyDown(KEY_A))
	{
		playerVelocity.x -= playerWalkSpeed;
	}
	if (IsKeyDown(KEY_S))
	{
		playerVelocity.y += playerWalkSpeed;
	}
	if (IsKeyDown(KEY_D))
	{
		playerVelocity.x += playerWalkSpeed;
	}

	if (attackCooldown > 0)
	{
		attackCooldown--;
		return;
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		attackCooldown = ATTACK_COOLDOWN_TICKS;
		Vector2 v = GetMousePosition();
		v = GetScreenToWorld2D(v, camera);
		v = Vector2Subtract(v, playerPosition);
		v = Vector2Scale(v, 0.25);

		struct projectile new = (struct projectile) {
			.position = playerPosition,
			.velocity = v,
			.radius = 1,
			.lifetime = TICKS_PER_SECOND,
			.onAttack = riceAttack
		};
		v = Vector2Scale(v, -0.25);
		playerVelocity = Vector2Add(playerVelocity, v);
		createProjectile(new);
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		attackCooldown = ATTACK_COOLDOWN_TICKS;
		Vector2 v = GetMousePosition();
		v = GetScreenToWorld2D(v, camera);
		v = Vector2Subtract(v, playerPosition);
		v = Vector2Scale(v, 0.25);

		struct projectile new = (struct projectile) {
			.position = playerPosition,
			.velocity = v,
			.radius = 8,
			.lifetime = TICKS_PER_SECOND,
			.onAttack = melonAttack
		};
		v = Vector2Scale(v, -0.25);
		playerVelocity = Vector2Add(playerVelocity, v);
		createProjectile(new);
	}
}

void handleProjectileSimulation(void)
{
	for (int idx = projectilesQuantity - 1; idx >= 0; idx--)
	{
		Vector2 * position = &projectiles[idx].position;
		Vector2 * velocity = &projectiles[idx].velocity;
		float * radius = &projectiles[idx].radius;
		int * lifetime = &projectiles[idx].lifetime;
		void (*onAttack)(struct projectile) = projectiles[idx].onAttack;

		Vector2 oldPosition = *position;

		*position = Vector2Add(*position, *velocity);
		*velocity = Vector2Scale(*velocity, 0.9);

		for (int iidx = 0; iidx < enemiesQuantity; iidx++)
		{
			Vector2 enemyPosition = enemies[iidx].position;
			float enemySize = enemies[iidx].size;

			float threshold = *radius + enemySize;
			if (CheckCollisionCircleLine(enemyPosition, threshold, oldPosition, *position))
			//if (Vector2Distance(enemyPosition, *position) < threshold)
			{
				*lifetime = 0;
				onAttack(projectiles[idx]);
				break;
			}
		}

		*lifetime -= 1;
		if (*lifetime <= 0)
		{
			projectilesQuantity--;
			projectiles[idx] = projectiles[projectilesQuantity];
		}
	}
}

// make the enemies do something
// boids?
void handleEnemySimulation(void)
{
	for (int idx = enemiesQuantity - 1; idx >= 0; idx--)
	{
		Vector2 * position = &enemies[idx].position;
		Vector2 * velocity = &enemies[idx].velocity;
		int * hp = &enemies[idx].hp;
		float * size = &enemies[idx].size;
		float * speed = &enemies[idx].speed;

		*position = Vector2Add(*position, *velocity);
		*velocity = Vector2Scale(*velocity, 0.97);
		if (Vector2Length(*velocity) > *speed)
		{
			*velocity = Vector2Scale(Vector2Normalize(*velocity), *speed);
		}

		// handle attraction towards player
		Vector2 attraction = Vector2Subtract(playerPosition, *position);
		attraction = Vector2Scale(attraction, 0.01);
		*velocity = Vector2Add(*velocity, attraction);

		float threshold = playersize + *size;
		if (Vector2Distance(*position, playerPosition) < threshold)
		{
			*velocity = Vector2Zero();
			playerhp -= 1;
		}

		// handle damaging
		for (int iidx = 0; iidx < damagefieldsQuantity; iidx++)
		{
			Vector2 damageposition = damagefields[iidx].position;
			float damageradius = damagefields[iidx].radius;
			int damage = damagefields[iidx].damage;

			threshold = damageradius + *size;
			if (Vector2Distance(*position, damageposition) < threshold)
			{
				*hp -= damage;
				*velocity = Vector2Zero();
			}
		}

		if (*hp <= 0)
		{
			enemiesQuantity--;
			enemies[idx] = enemies[enemiesQuantity];
			numKills++;

			// gets hectic fast
			// TODO: improve difficulty curve
			createEnemy();
			createEnemy();
		}
	}
}

// the damagefields' job is just to exist for a period of time then disappear
// the other things respond to the damagefields' existence
void handleDamagefieldSimulation(void)
{
	for (int idx = damagefieldsQuantity - 1; idx >= 0; idx--)
	{
		int * lifetime = &damagefields[idx].lifetime;
		*lifetime -= 1;
		if (*lifetime <= 0)
		{
			damagefieldsQuantity--;
			damagefields[idx] = damagefields[damagefieldsQuantity];
		}
	}
}

void * handleWorldSimulation(void *)
{
	clock_t thisTime = clock();
	clock_t lastTime = thisTime;
	double differenceSeconds = 0;

	while (simulationIntervalSeconds > 0)
	{
		thisTime = clock();
		differenceSeconds = (double) (thisTime - lastTime) / CLOCKS_PER_SEC;
		lastTime = thisTime;
		WaitTime(simulationIntervalSeconds - differenceSeconds);

		if (isPaused)
		{
			continue;
		}

		// update position FIRST then update velocity
		// so that the location prediction when rendering doesn't look wonky
		playerPosition = Vector2Add(playerPosition, playerVelocity);
		playerVelocity = Vector2Scale(playerVelocity, 0.5);
		handlePlayerInput();

		handleProjectileSimulation();
		handleEnemySimulation();
		handleDamagefieldSimulation();

		frameDivision = 0;
	}

	return NULL;
}

void handleUserInput(void)
{
	if (IsKeyPressed(KEY_Q))
	{
		isKill = 1;
	}

	if (IsKeyPressed(KEY_P))
	{
		isPaused = !isPaused;
	}

	if (IsKeyPressed(KEY_H))
	{
		simulationIntervalSeconds = -1;
	}
}

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(512, 512, "");
	SetTargetFPS(FRAMES_PER_SECOND);
	SetExitKey(EOF);
	srand(time(NULL));

	camera.zoom = 1;

	enemiesQuantity = 0;
	enemiesCapacity = 1;
	enemies = calloc(sizeof(struct enemy), enemiesCapacity);
	createEnemy();

	damagefieldsQuantity = 0;
	damagefieldsCapacity = 1;
	damagefields = calloc(sizeof(struct damagefield), damagefieldsCapacity);

	projectilesQuantity = 0;
	projectilesCapacity = 1;
	projectiles = calloc(sizeof(struct projectile), projectilesCapacity);

	pthread_t worldSimulationThread;
	pthread_create(&worldSimulationThread, NULL, handleWorldSimulation, NULL);

	while (!WindowShouldClose() && !isKill)
	{
		camera.offset = (Vector2) {
			.x = GetScreenWidth() / 2,
			.y = GetScreenHeight() / 2
		};
		camera.target = Vector2Lerp(camera.target, playerPosition, 0.1);

		handleUserInput();

		BeginDrawing();
		BeginMode2D(camera);
		ClearBackground(BLACK);

		DrawCircleV(Vector2Zero(), 10, BLUE);

		char display[64];
		Vector2 predictedPositionDelta = Vector2Scale(playerVelocity, frameDivision);
		Vector2 renderPosition = Vector2Add(playerPosition, predictedPositionDelta);
		DrawCircleV(renderPosition, playersize, WHITE);
		snprintf(display, sizeof(display), "%d", playerhp);
		DrawText(display, (int) renderPosition.x, (int) renderPosition.y, 18, GRAY);

		for (int idx = 0; idx < enemiesQuantity; idx++)
		{
			Vector2 position = enemies[idx].position;
			Vector2 velocity = enemies[idx].velocity;
			float size = enemies[idx].size;

			predictedPositionDelta = Vector2Scale(velocity, frameDivision);
			renderPosition = Vector2Add(position, predictedPositionDelta);
			DrawCircleV(renderPosition, size, RED);
			//DrawCircleV(position, size, RED);

			int hp = enemies[idx].hp;
			snprintf(display, sizeof(display), "%d", hp);
			DrawText(display, (int) renderPosition.x, (int) renderPosition.y, 18, WHITE);
		}

		for (int idx = 0; idx < damagefieldsQuantity; idx++)
		{
			Vector2 position = damagefields[idx].position;
			float radius = damagefields[idx].radius;
			DrawCircleLinesV(position, radius, BLUE);
		}

		for (int idx = 0; idx < projectilesQuantity; idx++)
		{
			Vector2 position = projectiles[idx].position;
			Vector2 velocity = projectiles[idx].velocity;
			float radius = projectiles[idx].radius;

			predictedPositionDelta = Vector2Scale(velocity, frameDivision);
			renderPosition = Vector2Add(position, predictedPositionDelta);
			DrawCircleV(renderPosition, radius, WHITE);
			//DrawCircleV(position, radius, WHITE);
			
		}

		frameDivision += 1.0 / framesPerTick;
		
		if (playerhp <= 0)
		{
			simulationIntervalSeconds = -1;
		}

		EndMode2D();

		snprintf(display, sizeof(display), "kills: %d", numKills);
		DrawText(display, 10, 10, 18, WHITE);

		EndDrawing();
	}

	simulationIntervalSeconds = -1;
	pthread_join(worldSimulationThread, NULL);
	free(enemies);
	free(damagefields);
	free(projectiles);
	CloseWindow();
	return 0;
}
