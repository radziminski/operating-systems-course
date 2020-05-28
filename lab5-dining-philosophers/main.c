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

#define SHM_KEY 0x1000
#define PHILOS_STATES_SEM 0x2000
#define FORKS_SEM 0x3000

#define PHILOSOPHERS_NUM 5
#define LEFT (PHILOSOPHERS_NUM + id - 1) % PHILOSOPHERS_NUM
#define RIGHT (id + 1) % PHILOSOPHERS_NUM

enum philo_state
{
	Eating,
	Thinking,
	Hungry
};
// Philosophers arr
struct shared_mem_seg
{
	enum philo_state states[PHILOSOPHERS_NUM];
} * shm_seg;

// TIME
int random_time(int lowLimit, int highLimit);

// PHILOSOPHERS
void eat_msg(int philosopher);
void think_msg(int philosopher);
void meal_finished_msg(int philosopher);
void init_philosophers();
void kill_all_philosophers(pid_t *philo_processes, int philo_num);
void philosopher(int philo_num);

// SEMS
void grab_forks(int left_fork_id);
void put_away_forks(int left_fork_id);
void toggle_sem(int semid, int bufid, bool lock);
void try_philo_eat(int id);

int main(int argc, char *argv[])
{
	// Creating shared memory whih philosophers arr
	int shm = shmget(SHM_KEY, sizeof(struct shared_mem_seg), 0644 | IPC_CREAT);
	if (shm == -1)
	{
		perror("There was an error while creating shared memory\n");
		return 1;
	}

	// Attach to the segment to get a pointer to it.
	shm_seg = shmat(shm, NULL, 0);
	if (shm_seg == (void *)-1)
	{
		perror("There was an error while atatching shared memory\n");
		return 1;
	}

	init_philosophers();

	///////////////////////////////////////////////////// do ogarniecia wziete od wiktos

	//mutex for state changes
	int state_mutex = semget(FORKS_SEM, 1, 0666 | IPC_CREAT);
	if (state_mutex < 0)
	{
		perror("semget: state mutex not created");
		return 1;
	}

	union semaphore_un {
		int val;
		unsigned int *array;
	} sem_un;

	//init mutex counter
	sem_un.val = 1;
	if (semctl(state_mutex, 0, SETVAL, sem_un) < 0)
	{
		perror("semctl: state mutex value failed to set");
		return 1;
	}

	//mutexes for philosopher grab\put away forks
	int philo_sems = semget(PHILOS_STATES_SEM, PHILOSOPHERS_NUM, 0666 | IPC_CREAT);
	if (philo_sems < 0)
	{
		perror("semget: philosophers semaphores not created");
		return 1;
	}

	//init semaphores
	unsigned int zeros[PHILOSOPHERS_NUM];
	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		zeros[i] = 0;
	}
	sem_un.array = zeros;
	if (semctl(philo_sems, 0, SETALL, sem_un) < 0)
	{
		perror("semctl: philo semaphores values failed to set");
		return 1;
	}

	///////////////////////////////////////////////////// koniec do ogarniecia

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

	sleep(210);
	kill_all_philosophers(philosophers_process_ids, 5);

	return 0;
}

// FUNCTIONS

int random_time(int lowLimit, int highLimit)
{
	srand(time(0));
	return rand() % (highLimit - lowLimit + 1) + lowLimit;
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

	//printf("Trying: philo: %d, philostate: %d, leftstate: %d, rightstate: %d\n", id, shm_seg->states[id], shm_seg->states[LEFT], shm_seg->states[RIGHT]);

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
	for (int i = 0; i < 3; i++)
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
