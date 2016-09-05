/* elevator_part_1.c
 *
 * This program is part of an elevator simulation program defined in the 
 * following link: http://web.eecs.utk.edu/~plank/plank/classes/cs360/360/labs/labb/index.html
 * The driver program and header file are elevator_skeleton.c and elevator.h.
 * These drive a simulation for a building with multiple floors and elevators.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "elevator.h"

typedef struct{
	Dllist people; // global dllist of all the people
	pthread_cond_t *block; //global condition variable for blocking elevator
}gList;

// initializes void *v in of Elevator_Simulation passed in
void initialize_simulation(Elevator_Simulation *es){
	gList *glist;
	
	// set up global list and condition variable for blocking evelator
	glist = (gList *) malloc(sizeof(gList));
	glist->people = new_dllist();
	glist->block = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	pthread_cond_init(glist->block, NULL);
	
	// initialize *v
	es->v = glist;
}

void initialize_elevator(Elevator *e){
		
}

void initialize_person(Person *p){

}

void wait_for_elevator(Person *p){
	gList *glist;

	glist =(gList *) p->es->v;
	
	pthread_mutex_lock(p->es->lock);	
	// append the person to the global list
	dll_append(glist->people, new_jval_v(p));
	// signal the condition variable for blocking elevators
	pthread_cond_signal(glist->block);
	pthread_mutex_unlock(p->es->lock);

	// Block on the person's condition variable
	pthread_mutex_lock(p->lock);
	pthread_cond_wait(p->cond, p->lock);
	pthread_mutex_unlock(p->lock);
}

//Unblock the elevator's condition variable and block on the person's condition variable.
void wait_to_get_off_elevator(Person *p){
	// unblock the elevator's condition variable
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);

	// block on the person's condition variable
	pthread_mutex_lock(p->lock);
	pthread_cond_wait(p->cond, p->lock);
	pthread_mutex_unlock(p->lock);
}

void person_done(Person *p){
	// Unblock the elevator's condition variable
	pthread_mutex_lock(p->e->lock);
	pthread_cond_signal(p->e->cond);
	pthread_mutex_unlock(p->e->lock);
}

// Each elevator is a while loop. Check the global list and if it's empty, block on the condition variable for blocking elevators. When the elevator gets a person to service, it moves to the appropriate floor and opens its door. It puts itself into the person's e field, then signals the person and blocks until the person wakes it up. When it wakes up, it goes to the person's destination floor, opens its door, signals the person and blocks. When the person wakes it up, it closes its door and re-executes its while loop.
void *elevator(void *arg){
	Elevator *e;
	gList *glist;
	Person *p;
	
	e = (Elevator *) arg;
	glist = (gList *) e->es->v;
	while(1){
		// if global list is empty, block on condition variable for blocking elevators
		pthread_mutex_lock(e->es->lock);
		while(dll_empty(glist->people)){
			pthread_cond_wait(glist->block, e->es->lock);
		}
		pthread_mutex_unlock(e->es->lock);

		// get first person on the people dllist, remove the person from the dllist
		pthread_mutex_lock(e->es->lock);
		p = (Person *) jval_v(dll_val(dll_first(glist->people)));
		dll_delete_node(glist->people->flink);
		pthread_mutex_unlock(e->es->lock);
		
		// move to the floor the first person is on, open door, and put elevator in person's e field
		move_to_floor(e, p->from);
		open_door(e);
		p->e = e;
		
		// signals person and blocks until the person wakes up
		pthread_mutex_lock(e->lock);
		pthread_cond_signal(p->cond);
		pthread_cond_wait(e->cond, e->lock);
		pthread_mutex_unlock(e->lock);
		
		// close door when person wakes up
		close_door(e);
		
		// move to person's requested floor, open door, signals person, blocks until person wakes up
		move_to_floor(e, p->to);
		open_door(e);
		pthread_mutex_lock(e->lock);
		pthread_cond_signal(p->cond);
		pthread_cond_wait(e->cond, e->lock);
		pthread_mutex_unlock(e->lock);

		// close door when person wakes up
		close_door(e);
	}
	
	return NULL;
}
