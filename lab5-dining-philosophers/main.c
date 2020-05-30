#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>

// Made by Jan Radziminski 293052

#define SHM_KEY 0x7000
#define PHILOS_STATES_SEM 0x8000
#define FORKS_SEM 0x9000

#define MAX_PROGRAM_RUNTIME 90

#define PHILOSOPHERS_NUM 5
#define LEFT (PHILOSOPHERS_NUM + id - 1) % PHILOSOPHERS_NUM
#define RIGHT (id + 1) % PHILOSOPHERS_NUM
#define PHILOSOPHERS_MEALS_NUM 3

enum philo_state
{
	Eating,
	Thinking,
	Hungry
};

// Philosophers states arr (shared to all processes)
struct shared_mem_seg
{
	enum philo_state states[PHILOSOPHERS_NUM];
} * shm_seg;

// Time
int random_time(int lowLimit, int highLimit);

// Philospohers functions
void eat_msg(int philosopher);
void think_msg(int philosopher);
void meal_finished_msg(int philosopher);
void init_philosophers();
void kill_all_philosophers(pid_t *philo_processes, int philo_num);
void philosopher(int philo_num);

// Semaphores functions
void grab_forks(int left_fork_id);
void put_away_forks(int left_fork_id);
void toggle_sem(int semid, int bufid, bool lock);
void try_philo_eat(int id);

// MAIN
int main(int argc, char *argv[])
{
	// Creating shared memory whih philosophers states arr
	int shm = shmget(SHM_KEY, sizeof(struct shared_mem_seg), 0644 | IPC_CREAT);
	if (shm == -1)
	{
		perror("There was an error while creating shared memory\n");
		return 1;
	}

	// Attach to the segment and get a pointer to it (stored in global var)
	shm_seg = shmat(shm, NULL, 0);
	if (shm_seg == (void *)-1)
	{
		perror("There was an error while atatching shared memory\n");
		return 1;
	}

	// Fettign semaphore for forks
	int forks_sem = semget(FORKS_SEM, 1, 0666 | IPC_CREAT);
	if (forks_sem < 0)
	{
		perror("There was an error while creating forks semaphore\n");
		return 1;
	}

	union semaphores_union {
		int val;
		unsigned short *array;
	} sem_un;
	sem_un.val = 1;

	// Setting forks semaphore
	if (semctl(forks_sem, 0, SETVAL, sem_un) < 0)
	{
		perror("There was an error while setting forks semaphore\n");
		return 1;
	}

	// Creating semaphores for hungry phliosophers
	int philosophers_sems = semget(PHILOS_STATES_SEM, PHILOSOPHERS_NUM, 0666 | IPC_CREAT);
	if (philosophers_sems < 0)
	{
		perror("There was an error while creating philosophers semaphores\n");
		return 1;
	}

	// Setting those semaphores
	unsigned short zeros[PHILOSOPHERS_NUM];
	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		zeros[i] = 0;
	}
	sem_un.array = zeros;
	if (semctl(philosophers_sems, 0, SETALL, sem_un) < 0)
	{
		perror("There was an error while setting philos semaphores");
		return 1;
	}

	// Setting all philosophers to thinking state
	init_philosophers();

	// Philosphers dinner loop
	pid_t philosophers_process_ids[PHILOSOPHERS_NUM];
	int philosophers_counter = 0;

	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		pid_t pid = fork();

		// Error
		if (pid < 0)
		{
			printf("There was an error while creating %d philosopher (process)\n", i);
			kill_all_philosophers(philosophers_process_ids, (i + 1));
			return 1;
		}

		// Parrent
		if (pid > 0)
		{
			philosophers_process_ids[i] = pid;
			continue;
		}

		// Child
		philosopher(i);
		return 0;
	}

	// Max 90s of program run
	sleep(MAX_PROGRAM_RUNTIME);
	kill_all_philosophers(philosophers_process_ids, PHILOSOPHERS_NUM);

	return 0;
}

// FUNCTIONS

int random_time(int low_limit, int high_limit)
{
	srand(time(0));
	return rand() % (high_limit - low_limit + 1) + low_limit;
}

void eat_msg(int philosopher)
{
	int sleep_time = random_time(2, 10);
	printf("[philosopher %d]: Eating (for %d s)\n", philosopher, sleep_time);
	sleep(sleep_time);
}

void think_msg(int philosopher)
{
	int sleep_time = random_time(2, 10);
	printf("[philosopher %d]: Thinking (for %d s)\n", philosopher, sleep_time);
	sleep(sleep_time);
}

void hungry_msg(int philosopher)
{
	printf("[philosopher %d]: Hungry (and trying to grab forks)\n", philosopher);
}

void meal_finished_msg(int philosopher)
{
	printf("[philosopher %d]: Finished Meal (and will be thinking again)\n", philosopher);
}

void grab_forks(int left_fork_id)
{
	int id = left_fork_id;
	// Getting semaphores
	int sem_philos = semget(PHILOS_STATES_SEM, PHILOSOPHERS_NUM, 0660);
	int sem_forks = semget(FORKS_SEM, 1, 0660);
	if (sem_philos < 0 || sem_forks < 0)
	{
		perror("There was an errir while getting philos semaphore [semget]");
		exit(1);
	}

	// Entering forks-change state
	toggle_sem(sem_forks, 0, true);
	// Philo becomes hungry
	shm_seg->states[id] = Hungry;
	hungry_msg(id);
	// Checking if he may eat
	try_philo_eat(id);
	// Leaving forks-change state
	toggle_sem(sem_forks, 0, false);
	// Toggling philo state- he wants to eat
	toggle_sem(sem_philos, id, true);
}
void put_away_forks(int left_fork_id)
{
	int id = left_fork_id;
	// Getting semaphores
	int sem_philos = semget(PHILOS_STATES_SEM, PHILOSOPHERS_NUM, 0660);
	int sem_forks = semget(FORKS_SEM, 1, 0660);
	if (sem_philos < 0 || sem_forks < 0)
	{
		perror("There was an errir while getting philos semaphore [semget]");
		exit(1);
	}

	// Entering forks-change state
	toggle_sem(sem_forks, 0, true);
	// Philo stopped eating and goes back to thinking
	shm_seg->states[id] = Thinking;
	meal_finished_msg(id);
	// Checking if philo neighoburs arent hungry (of they are they may eat)
	try_philo_eat(LEFT);
	try_philo_eat(RIGHT);
	// Leaving forks-change state
	toggle_sem(sem_forks, 0, false);
}

void try_philo_eat(int id)
{
	int sem_philos = semget(PHILOS_STATES_SEM, 1, 0660);
	if (sem_philos < 0)
	{
		perror("There was an errir while getting philos semaphore [semget]");
		exit(1);
	}

	if (shm_seg->states[id] == Hungry && !(shm_seg->states[LEFT] == Eating) && !(shm_seg->states[RIGHT] == Eating))
	{
		shm_seg->states[id] = Eating;
		toggle_sem(sem_philos, id, false);
	}
}

void init_philosophers()
{

	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		shm_seg->states[i] = Thinking;
	}
}

void toggle_sem(int semid, int bufid, bool lock)
{
	int bufOp = -1;
	if (!lock)
		bufOp = +1;
	struct sembuf semDown = {bufid, bufOp, SEM_UNDO};

	if (semop(semid, &semDown, 1) < 0)
	{
		perror("There wasn an errror during sem lock");
		exit(1);
	}
}

void philosopher(int philo_num)
{
	for (int i = 0; i < PHILOSOPHERS_MEALS_NUM; i++)
	{
		think_msg(philo_num);
		grab_forks(philo_num);
		eat_msg(philo_num);
		put_away_forks(philo_num);
	}
}

void kill_all_philosophers(pid_t *philo_processes, int philo_num)
{
	for (int i = 0; i < philo_num; i++)
	{
		kill(philo_processes[i], SIGTERM);
	}
}
