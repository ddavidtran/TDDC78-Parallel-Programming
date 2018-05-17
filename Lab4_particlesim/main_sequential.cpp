#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <vector>
#include <mpi.h>
#include "coordinate.h"
#include "definitions.h"
#include "physics.h"


//Feel free to change this program to facilitate parallelization.

float rand1(){
	return (rand()/(float) RAND_MAX);
}

void init_collisions(std::vector<bool> collisions, unsigned int max){
	for(unsigned int i=0;i<max;++i)
		collisions[i]= false;
}


int main(int argc, char** argv){
	unsigned int time_stamp = 0, time_max;
	float pressure = 0;
	int rank{}, world{}, root{0};
	std::vector<pcord_t> particles;
    std::vector<bool> collisions;
    cord_t wall;

	// parse arguments
	if(argc != 2) {
		fprintf(stderr, "Usage: %s simulation_time\n", argv[0]);
		fprintf(stderr, "For example: %s 10\n", argv[0]);
		exit(1);
	}

	time_max = atoi(argv[1]);


    MPI_Init(nullptr,nullptr);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world);

    std::cout << "Rank " << rank << " out of " << world << "\n";

    //Initializes the wall and particles on the root/master processor.
    if(rank == root){
        /* Initialize */
        // 1. set the walls
        wall.y0 = wall.x0 = 0;
        wall.x1 = BOX_HORIZ_SIZE;
        wall.y1 = BOX_VERT_SIZE;

        // 2. allocate particle buffer and initialize the particles
        particles = std::vector<pcord_t>(INIT_NO_PARTICLES*sizeof(pcord_t));
        collisions = std::vector<bool>(INIT_NO_PARTICLES*sizeof(bool));
    }



	srand( time(nullptr) + 1234 );

	float r, a;
	for(int i=0; i<INIT_NO_PARTICLES; i++){
		// initialize random position
		particles[i].x = static_cast<float>(wall.x0 + rand1()*BOX_HORIZ_SIZE);
		particles[i].y = static_cast<float>(wall.y0 + rand1()*BOX_VERT_SIZE);

		// initialize random velocity
		r = rand1()*MAX_INITIAL_VELOCITY;
		if(r < 50) r = 49;
		a = static_cast<float>(rand1()*2*PI);
		particles[i].vx = r*cos(a);
		particles[i].vy = r*sin(a);
	}


	unsigned int p, pp;

	/* Main loop */
	for (time_stamp=0; time_stamp<time_max; time_stamp++) { // for each time stamp

		init_collisions(collisions, INIT_NO_PARTICLES);

		for(p=0; p<INIT_NO_PARTICLES; p++) { // for all particles
			if(collisions[p]) continue;

			/* check for collisions */
			for(pp=p+1; pp<INIT_NO_PARTICLES; pp++){
				if(collisions[pp]) continue;
				float t=collide(&particles[p], &particles[pp]);
				if(t!=-1){ // collision
					collisions[p]=collisions[pp]=1;
					interact(&particles[p], &particles[pp], t);
					break; // only check collision of two particles
				}
			}

		}

		// move particles that has not collided with another
		for(p=0; p<INIT_NO_PARTICLES; p++)
			if(!collisions[p]){
				feuler(&particles[p], 1);

				/* check for wall interaction and add the momentum */
				pressure += wall_collide(&particles[p], wall);
			}


	}

	printf("Average pressure = %f\n", pressure / (WALL_LENGTH*time_max));
    MPI_Finalize();
	return 0;

}

