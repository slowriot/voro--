// Voronoi calculation example code
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : July 1st 2008

#include "cell.cc"
#include "container.cc"
#include "wall.cc"

// Set up constants for the container geometry
const double x_min=-1,x_max=1;
const double y_min=-1,y_max=1;
const double z_min=-1,z_max=1;
const double cvol=(x_max-x_min)*(y_max-y_min)*(x_max-x_min);

// Set up the number of blocks that the container is divided into
const int n_x=6,n_y=6,n_z=6;

// Set the number of particles that are going to be randomly introduced
const int particles=20;

// This function returns a random double between 0 and 1
double rnd() {return double(rand())/RAND_MAX;}

int main() {
	int i;
	double *bb;
	bb=new double[particles];
	double v=0,x,y,z;

	// Create a container with the geometry given above, and make it
	// non-periodic in each of the three coordinates. Allocate space for
	// eight particles within each computational block
	container con(x_min,x_max,y_min,y_max,z_min,z_max,n_x,n_y,n_z,
			false,false,false,8);

	// Randomly add particles into the container
	for(i=0;i<particles;i++) {
		x=x_min+rnd()*(x_max-x_min);
		y=y_min+rnd()*(y_max-y_min);
		z=z_min+rnd()*(z_max-z_min);
		con.put(i,x,y,z);
	}
	
	// Output the particle positions in gnuplot format
	con.draw_particles("random_points.gnu");
	
	// Output the Voronoi cells in gnuplot format
	con.draw_cells_gnuplot("random_points2.gnu");

	// Compute the volumes of all the Voronoi cells, and store them in the
	// bb[] array
	con.store_cell_volumes(bb);

	// Sum up the volumes, and check that this matches the container volume
	for(i=0;i<particles;i++) v+=bb[i];
	cout << "Container volume : " << cvol << "\n";
	cout << "Voronoi volume   : " << v << "\n";
	cout << "Difference       : " << v-cvol << endl;
}
