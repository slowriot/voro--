// Voronoi calculation example code
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : October 19th 2007

#include "container.cc"

// Set up constants for the container geometry
const double x_min=-1,x_max=1;
const double y_min=-1,y_max=1;
const double z_min=-1,z_max=1;

// Set up the number of blocks that the container is divided
// into.
const int n_x=3,n_y=3,n_z=3;

// Set the number of particles that are going to be randomly
// introduced
const int particles=50;

// This function returns a random double between 0 and 1;
double rnd() {return double(rand())/RAND_MAX;}

int main() {
	int i;
	double x,y,z;

	// Create a container with the geometry given above, and make it
	// periodic in each of the three coordinates. Allocate space for
	// 100 particles within each computational block
	container con(x_min,x_max,y_min,y_max,z_min,z_max,n_x,n_y,n_z,
			true,true,true,100);

	//Randomly add particles into the container
	for(i=0;i<particles;i++) {
		x=x_min+rnd()*(x_max-x_min);
		y=y_min+rnd()*(y_max-y_min);
		z=z_min+rnd()*(z_max-z_min);
		con.put(i,x,y,z);
	}

	// Print out a list of the particles, and their Voronoi volumes
	con.vprintall();

	// Save the Voronoi network of all the particles to a text file
	// in a format ready for plotting by gnuplot
	con.vdraw("voronoi_cells",gnuplot);
}
