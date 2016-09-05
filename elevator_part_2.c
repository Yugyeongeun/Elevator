/* elevator_part_2.c
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
	Dllist people; // dllist of all the people
	pthread_cond_t *block; //condition variable for blocking elevator
	pthread_mutex_t *lock;
}gList;

// initializes void *v in of Elevator_Simulation passed in
void initialize_simulation(Elevator_Simulation *es){
	gList *glist;

	// set up global list, condition variable for blocking evelator and mutex lock
	glist = (gList *) malloc(sizeof(gList));
	glist->people = new_dllist();
	glist->block = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	pthread_cond_init(glist->block, NULL);
	glist->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(glist->lock, NULL);

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

void *elevator(void *arg){
	Elevator *e;
	gList *glist;
	Person *p;
	int nfloors;	// number of floors
	int up; // 1 for up, 0 for down
	Dllist pickupList;
	Dllist tmpP, nextP;

	e = (Elevator *) arg;
	glist = (gList *) e->es->v;
	nfloors = e->es->nfloors;
	pickupList = new_dllist();

	up = 1;
	while(1){
		// if global list is empty, block on condition variable for blocking elevators
		pthread_mutex_lock(e->es->lock);
		while(dll_empty(glist->people)){
			pthread_cond_wait(glist->block, e->es->lock);
		}
		pthread_mutex_unlock(e->es->lock);

		// set elevator to go up when it hits bottom floor and go back down once it hits top floor
		if(e->onfloor == 1) up = 1;
		else if(e->onfloor == nfloors) up = 0;

		// loop through global list's people dllist, and append people to pick up list
		pthread_mutex_lock(glist->lock);
		tmpP = dll_first(glist->people);
		while(tmpP != dll_nil(glist->people) && !dll_empty(glist->people)){
			nextP = dll_next(tmpP);
			p = (Person *)tmpP->val.v;
			// only append people to pick up list when elevator happens to be passing the same floor as people on that floor,
			// and that their destination is on the way of the elevator is going
			if(e->onfloor == p->from){
				if(p->to > e->onfloor && up || p->to < e->onfloor && !up){
					dll_append(pickupList, tmpP -> val);
					// delete the person from the glist->people
					dll_delete_node(tmpP);
				}
			}
			tmpP = nextP;
		}
		pthread_mutex_unlock(glist->lock);

		// loop through pick up list, open door when necessary, pick up people, delete them from pickupList, close door when necessary
		tmpP = dll_first(pickupList);
		while(tmpP != dll_nil(pickupList) && !dll_empty(pickupList)){
			nextP = dll_next(tmpP);

			p = (Person *) tmpP->val.v;

			if(!e -> door_open) open_door(e);
			pthread_mutex_lock(p->lock);

			p->e = e;

			pthread_cond_signal(p->cond);
			pthread_mutex_lock(e->lock);
			pthread_mutex_unlock(p->lock);

			pthread_cond_wait(e->cond, e->lock);
			pthread_mutex_unlock(e->lock);

			dll_delete_node(tmpP);

			tmpP = nextP;
		}
		if(e->door_open){
			close_door(e);
		}

		// move up a floor if elevator is currently going up, and move down a floor if elevator is currently going down
		if(up){
			move_to_floor(e, e->onfloor + 1);
		}else{
			move_to_floor(e, e->onfloor - 1);
		}

		// loop through the people on elevator, drop people off
		tmpP = dll_first(e->people);
		while(tmpP != dll_nil(e->people) && !dll_empty(e->people)){
			nextP = dll_next(tmpP);
			p = (Person *) tmpP->val.v;

			if(p->to == e->onfloor){
				if(!e->door_open){
					open_door(e);
				}

				// signals person and blocks until the person wakes up
				pthread_mutex_lock(p->lock);
				pthread_cond_signal(p->cond);
				pthread_mutex_lock(e->lock);
				pthread_mutex_unlock(p->lock);
				pthread_cond_wait(e->cond, e->lock);
				pthread_mutex_unlock(e->lock);
			}

			tmpP = nextP;
		}

	}

	return NULL;
}
