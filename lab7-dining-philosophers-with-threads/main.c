#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

// EOPSY Lab 7 Laboratories project
// Dining Philosophers - made with threads

// Remark - I decided to remove message displayed when whilosopher is hungry since its making solution less clean and readable.
// However it may be "turned on" by uncommeting the next line
// #define SHOW_HUNGRY_MSGS

/* Lab Questions:
	1. Would it be sufficient just to add to the old algorithm from task5 additional mutex variable to organize critical sections in functions
	grab_forks() and put_away_forks() for making changes to values of two mutexes indivisably?  If not, why?
	

	2. Why m mutex is initialized with 1 and mutexes from the array s are initialized with 0's?
	Mutex m is used to control the critical sections where the given philosopher tries to grab or put down fork. If it would be locked at the start, 
	no philosopher would be able to enter the critical section and deadlock would occur. Since it is unlocked att start, first thread to reach it
	locks it, so that only he has "access" to critical section.
	The mutexes s[] are used to block the execution of a thread, that cannot eat at the moment, since one (or more) of its neibouhr is eating.
	Since it is locked at start, when first philosopher tries to eat, it unlocks it if he can eat and locks it again - since he is not locking already 
	locked mutex its execution does not stop. However, if philosopher cannot eat, he doesnt unlock it during try philo eat, so he tries to lock already 
	locked mutex. Therefore his execution stops and he must wait for the other philosopher to finish eating and unlock this mutex.
	If all s mutexes were initialized with 1 instead of 0, all philosophers would be able to eat at start (since none would lock locked mutex) and
	it would be wrong (not following the task).
*/

#define MAX_PROGRAM_RUNTIME 90

#define PHILOSOPHERS_NUM 5
#define PHILOSOPHERS_MEALS_NUM 3
#define LEFT (PHILOSOPHERS_NUM + id - 1) % PHILOSOPHERS_NUM
#define RIGHT (id + 1) % PHILOSOPHERS_NUM

enum philo_state
{
	Eating,
	Thinking,
	Hungry
};

// Mutexes
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s[PHILOSOPHERS_NUM] = PTHREAD_MUTEX_INITIALIZER;
enum philo_state state[PHILOSOPHERS_NUM];

// Time
int random_time(int lowLimit, int highLimit);

// Philospohers functions
void eat_msg(int philosopher);
void think_msg(int philosopher);
void meal_finished_msg(int philosopher);
int init_philosophers(pthread_t *philos, int *ids);
int kill_all_philosophers(pthread_t *philo_threads);
void *philosopher(void *arg);

// Mutexes functions
int init_mutexes();
int destroy_mutexes();
void grab_forks(int left_fork_id);
void put_away_forks(int left_fork_id);
void try_philo_eat(int id);

// MAIN
int main(int argc, char *argv[])
{
	pthread_t philosophers[PHILOSOPHERS_NUM];
	int ids[PHILOSOPHERS_NUM];

	if (init_mutexes() != 0)
		return 1;

	if (init_philosophers(philosophers, ids) != 0)
		return 1;

	sleep(MAX_PROGRAM_RUNTIME);

	if (kill_all_philosophers(philosophers) != 0)
		return 1;

	if (destroy_mutexes() != 0)
		return 1;

	return 0;
}

// FUNCTIONS

int init_mutexes()
{
	// Unlocking forks mutex
	pthread_mutex_unlock(&m);

	// Philosophers mutexes
	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		// Initializing mutex
		int status = pthread_mutex_init(&s[i], NULL);
		if (status != 0)
		{
			perror("There was an error while creating philosophers mutexes");
			return 1;
		}

		// Locking it at start
		pthread_mutex_lock(&s[i]);
	}

	return 0;
}

int destroy_mutexes()
{
	// Philosophers mutexes
	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		// Destroying mutex
		pthread_mutex_destroy(&s[i]);
	}

	return 0;
}

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

	// Fork critical state
	// down( &m );
	pthread_mutex_lock(&m);
	// State change - philosopher wants to eat
	state[id] = Hungry;
#ifdef SHOW_HUNGRY_MSGS
	hungry_msg(id);
#endif
	// Trying to eat
	try_philo_eat(id);
	// up( &m );
	pthread_mutex_unlock(&m);

	//down( &s[i] );
	pthread_mutex_lock(&s[id]);
}
void put_away_forks(int left_fork_id)
{
	int id = left_fork_id;

	// Fork critical state
	// down( &m );
	pthread_mutex_lock(&m);
	// Philosophear has eaten
	state[id] = Thinking;
	// Testing if neighboughr philosophers want to eat
	try_philo_eat(LEFT);
	try_philo_eat(RIGHT);
	// up( &m );
	pthread_mutex_unlock(&m);
}

void try_philo_eat(int id)
{
	if (state[id] == Hungry && state[LEFT] != Eating && state[RIGHT] != Eating)
	{
		state[id] = Eating;
		pthread_mutex_unlock(&s[id]);
	}
}

int init_philosophers(pthread_t *philos, int *ids)
{

	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		// Initializing state array
		state[i] = Thinking;

		// Philo number
		ids[i] = i;

		// Creating thread
		int status = pthread_create(philos + i, NULL, philosopher, (void *)(ids + i));
		if (status != 0)
		{
			perror("There was an error while creating the threads");
			return 1;
		}
	}

	return 0;
}

void *philosopher(void *arg)
{
	int *id = (int *)arg;
	for (int i = 0; i < PHILOSOPHERS_MEALS_NUM; i++)
	{
		think_msg(*id);
		grab_forks(*id);
		eat_msg(*id);
#ifdef SHOW_HUNGRY_MSGS
		meal_finished_msg(*id);
#endif
		put_away_forks(*id);
	}
	// After finishing meal philosofer is only thinking
	printf("[philosopher %d]: Finished all his meals, is thinking...\n", *id);
	return NULL;
}

int kill_all_philosophers(pthread_t *philo_threads)
{
	for (int i = 0; i < PHILOSOPHERS_NUM; i++)
	{
		// Cancleling execution of a process
		pthread_cancel(philo_threads[i]);

		// Waiting for it to cancel
		int status = pthread_join(philo_threads[i], NULL);
		if (status != 0)
		{
			perror("There was an error while killing the threads");
			return 1;
		}
	}

	return 0;
}
