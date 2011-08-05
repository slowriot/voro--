/** \file container_2d.cc
 * \brief Function implementations for the container_2d class. */

#include "container_2d.hh"

#include <iostream>
#include <cstring>
#include <math.h>
using namespace std;

/** The class constructor sets up the geometry of container, initializing the
 * minimum and maximum coordinates in each direction, and setting whether each
 * direction is periodic or not. It divides the container into a rectangular
 * grid of blocks, and allocates memory for each of these for storing particle
 * positions and IDs.
 * \param[in] (ax_,bx_) the minimum and maximum x coordinates.
 * \param[in] (ay_,by_) the minimum and maximum y coordinates.
 * \param[in] (nx_,ny_) the number of grid blocks in each of the three
 *                       coordinate directions.
 * \param[in] (xperiodic_,yperiodic_) flags setting whether the container is periodic
 *                        in each coordinate direction.
 * \param[in] init_mem the initial memory allocation for each block. */
container_2d::container_2d(double ax_,double bx_,double ay_,
		double by_,int nx_,int ny_,bool xperiodic_,bool yperiodic_,bool convex_,int init_mem)
	: ax(ax_), bx(bx_), ay(ay_), by(by_), boxx((bx_-ax_)/nx_), boxy((by_-ay_)/ny_),
	xsp(1/boxx), ysp(1/boxy), nx(nx_), ny(ny_), nxy(nx*ny),
	xperiodic(xperiodic_), yperiodic(yperiodic_), convex(convex_),
	co(new int[nxy]), mem(new int[nxy]), id(new int*[nxy]),
	wid(new int*[nxy]), p(new double*[nxy]), nlab(new unsigned int*[nxy]),
	plab(new int**[nxy]), soi(NULL), noofbnds(0), bnds_size(init_bnds_size),
	bnds(new double[2*bnds_size]), edb(new int[bnds_size]), bndpts(new int*[nxy]) {
	int l;
	debug=true;

	for(l=0;l<nxy;l++) co[l]=0;
	for(l=0;l<nxy;l++) mem[l]=init_mem;
	for(l=0;l<nxy;l++) {
		id[l]=new int[init_mem];
		bndpts[l]=new int[init_mem];
	}
	for(l=0;l<nxy;l++) p[l]=new double[2*init_mem];
	for(l=0;l<nxy;l++) {
		wid[l]=new int[6];
		wid[l][0]=1;
	}
	for(l=0;l<nxy;l++) nlab[l]=new unsigned int[init_mem];
	for(l=0;l<nxy;l++) plab[l]=new int*[init_mem];
}

/** The container destructor frees the dynamically allocated memory. */
container_2d::~container_2d() {
	int l;

	// Clear "sphere of influence" array if it has been allocated
	if(soi!=NULL) delete [] soi;

	// Delete the boundary arrays
	delete [] edb;
	delete [] bnds;

	// Deallocate the block-level arrays
	for(l=nxy-1;l>=0;l--) delete [] plab[l];
	for(l=nxy-1;l>=0;l--) delete [] nlab[l];
	for(l=nxy-1;l>=0;l--) delete [] wid[l];
	for(l=nxy-1;l>=0;l--) delete [] p[l];
	for(l=nxy-1;l>=0;l--) delete [] id[l];

	// Delete the two-dimensional 
	delete [] plab;
	delete [] nlab;
	delete [] p;
	delete [] wid;
	delete [] id;
	delete [] mem;
	delete [] co;
}

/** Put a particle into the correct region of the container.
 * \param[in] n the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle. */
void container_2d::put(int n,double x,double y, int boundloc) {
	int ij;
	if(put_locate_block(ij,x,y)) {
		id[ij][co[ij]]=n;
		bndpts[ij][co[ij]]=boundloc;
		double *pp(p[ij]+2*co[ij]++);
		*(pp++)=x;*pp=y;
	}
}

/** This routine takes a particle position vector, tries to remap it into the
 * primary domain. If successful, it computes the region into which it can be
 * stored and checks that there is enough memory within this region to store
 * it.
 * \param[out] ij the region index.
 * \param[in,out] (x,y) the particle position, remapped into the primary
 *                      domain if necessary.
 * \return True if the particle can be successfully placed into the container,
 * false otherwise. */
inline bool container_2d::put_locate_block(int &ij,double &x,double &y) {
	if(put_remap(ij,x,y)) {
		if(co[ij]==mem[ij]) add_particle_memory(ij);
		return true;
	}
#if VOROPP_REPORT_OUT_OF_BOUNDS ==1
	fprintf(stderr,"Out of bounds: (x,y)=(%g,%g)\n",x,y);
#endif
	return false;
}

/** Takes a particle position vector and computes the region index into which
 * it should be stored. If the container is periodic, then the routine also
 * maps the particle position to ensure it is in the primary domain. If the
 * container is not periodic, the routine bails out.
 * \param[out] ij the region index.
 * \param[in,out] (x,y) the particle position, remapped into the primary
 *                      domain if necessary.
 * \return True if the particle can be successfully placed into the container,
 * false otherwise. */
inline bool container_2d::put_remap(int &ij,double &x,double &y) {
	int l;

	ij=step_int((x-ax)*xsp);
	if(xperiodic) {l=step_mod(ij,nx);x+=boxx*(l-ij);ij=l;}
	else if(ij<0||ij>=nx) return false;

	int j(step_int((y-ay)*ysp));
	if(yperiodic) {l=step_mod(j,ny);y+=boxy*(l-j);j=l;}
	else if(j<0||j>=ny) return false;

	ij+=nx*j;
	return true;
}

/** This does the additional set-up for non-convex containers. We assume that
 * **p, **id, *co, *mem, *bnds, and noofbnds have already been setup. We then
 * proceed to setup **wid, *soi, and THE PROBLEM POINTS BOOLEAN ARRAY.
 * This algorithm keeps the importing seperate from the set-up */
void container_2d::setup(){
	probpts=new bool[noofbnds];
	double lx, ly, cx, cy, nx, ny, fx, fy, tmpx, tmpy;//last (x,y), current (x,y), next (x,y), temporary (x,y)
	int wid=1;

	cout << "beginning";
	tmp=tmpp=new int[3*init_temp_label_size];
	tmpe=tmp+3*init_temp_label_size;
	
	lx=bnds[0]; ly=bnds[1]; cx=bnds[2]; cy=bnds[3]; nx=bnds[4]; ny=bnds[5];
	fx=lx; fy=ly;
	while(wid<(noofbnds-1)){
		tag_walls(cx,cy,nx,ny,wid);
		semi_circle_labelling(cx,cy,nx,ny,wid);
		if((((lx-cx)*(ly-cy))+((nx-cx)*(ny-cy)))>0){
			probpts[wid]=true;
		}
		tmpx=cx; tmpy=cy; cx=nx; cy=ny; lx=tmpx; ly=tmpy;
		wid++;
		if(!(wid==(noofbnds-1))){
		nx=bnds[(wid*2)+2]; ny=bnds[(wid*2)+3];
		}
	}
	nx=fx; ny=fy;
	tag_walls(cx,cy,nx,ny,wid);
	semi_circle_labelling(cx,cy,nx,ny,wid);
	if((((lx-cx)*(ly-cy))+((nx-cx)*(ny-cy)))>0){
		probpts[wid]=true;
	}
	lx=cx; ly=cy; wid=0; cx=fx; cy=fy; nx=bnds[2]; ny=bnds[3];
	tag_walls(cx,cy,nx,ny,wid);
	semi_circle_labelling(cx,cy,nx,ny,wid);
	if((((lx-cx)*(ly-cy))+((nx-cx)*(ny-cy)))>0){
		probpts[wid]=true;
	}

	//at this point *tmp is completed RUN CHRIS' ALGORITH TO CREATE *SOI and *SOIP
	create_label_table();

	// Remove temporary array
	delete [] tmp;
}
	
/** Given two points, tags all the computational boxes that the line segment
 * specified by the two points
 * goes through. param[in] (x1,y1) this is one point 
 * \param[in] (x2,y2) this is the other point.
 * \param[in] wid this is the wall id bnds[2*wid] is the x index of the first
 *                vertex in the c-c direction. */
void container_2d::tag_walls(double x1, double y1, double x2, double y2, int wid){
	int cgrid=0, cgridx=0, cgridy=0, fgrid=0;
	double slope, slopec, cx=ax+boxx, cy=ay+boxy, gridx, gridy;

	if(x2<x1){
		double tmpx=x1, tmpy=y1;
		x1=x2; y1=y2; x2=tmpx; y2=tmpy;
	}
	while(cy<y1){
		cgrid+=nx;
		cy+=boxy;
		cgridy++;
	}while(cx<x1){
		cgrid++;
		cx+=boxx;
		cgridx++;
	}
	cy=ay+boxy; cx=ax+boxx;
	while(cy<y2){
		fgrid+=nx;
		cy+=boxy;
	}while(cx<x2){
		fgrid++;
		cx+=boxx;
	}
	if (debug) cout << cgrid << "   ";
	if((x2-x1)==0){
		while(cgrid!=fgrid){
			tag(wid,cgrid);
			cgrid+=nx;
			if (debug) cout<< cgrid << "   ";
		}
	tag(wid,cgrid);
	return;
		}

	slope=((y2-y1)/(x2-x1));


	if(slope>=0){
		gridx=ax+((1+cgridx)*boxx);
		gridy=ay+((1+cgridy)*boxy);

		while(cgrid!=fgrid){
			tag(wid,cgrid);
			slopec=((gridy-y1)/(gridx-x1));
			if(slope>slopec){
				cgrid+=nx;
				x1+=((1/slope)*(gridy-y1));
				y1=gridy;
				cgridy++;
				gridy+=boxy;
			} else if(slope<slopec){
				cgrid++;
				y1+=(slope*(gridx-x1));
				x1=gridx;
				cgridx++;
				gridx+=boxx;
			} else if(slope==slopec){
				cgrid+=(nx+1);
				x1=gridx;
				y1=gridy;
				cgridx++;
			cgridy++;
			tag(wid,cgrid-1);
			tag(wid,cgrid-nx);
			if(debug) cout << cgrid-1 << "   " << cgrid-nx << "   ";
			gridx+=boxx;
			gridy+=boxy;
		}
		if (debug) cout << cgrid << "   ";
		}
	
		tag(wid,cgrid);
		return;
	}

	if(slope<0){
		gridx=ax+((1+cgridx)*boxx);
		gridy=ay+(cgridy*boxy);
		
		while(cgrid!=fgrid){
			tag(wid, cgrid);
			slopec=((gridy-y1)/(gridx-x1));
			if(slope>slopec){
				cgrid++;
				cgridx++;
				y1+=(slope*(gridx-x1));
				x1=gridx;
				gridx+=boxx;
			} else if(slope<slopec){
				cgrid-=nx;
				cgridy--;
				x1+=((1/slope)*(gridy-y1));
				y1=gridy;
				gridy-=boxy;
			} else if(slope==slopec){
				cgrid-=(nx-1);
				x1=gridx;
				y1=gridy;
				gridx+=boxx;
				gridy-=boxy;
				cgridx++;
				cgridy++;
				tag(wid,cgrid-1);
				tag(wid,cgrid+nx);
				if(debug) cout << cgrid-1 << "    " << cgrid+nx << "   ";
			}

		if (debug) cout << cgrid << "   ";
		}
		tag(wid,cgrid);
	}
}

/* A helper function for semi-circle-labelling. If crossproductz(x1,y1,particlex,particley)>0 then the particle is on the appropriate side of the wall to be tagged. */
int container_2d::crossproductz(double x1, double y1, double x2, double y2){
	if(((x1*y2)-(x2*y1))>0) return 1;
	else return -1;
}

/* Tags particles that are within a semicircle (on the appropriate side) of a boundary.
 * \param[in] (x1,y1) the start point of the wall segment, arranged so that it
 *                    is the first point reached in the counter-clockwise
 *                    direction. 
 * \param[in] (x2,y2) the end points of the wall segment. */
void container_2d::semi_circle_labelling(double x1, double y1, double x2, double y2, int wid){
	voropp_loop_2d l(*this);
	double xmin,xmax,ymin,ymax,rs=(dist_squared(x1,y1,x2,y2)/2),radius=pow(rs,1/2),dummy1,dummy2,midx=(x1+x2)/2,
	midy=(y1+y2)/2;
	double cpx,cpy; //these stand for "current particle x" and "current particle y"
	int box; //this holds the current computational box that we're in.
	
	// First we will initialize the voropp_loop_2d object to a rectangle
	// containing all the points we are interested in plus some extraneous
	// points. The larger the slope between (x1,y1) and (x2,y2) is, the
	// more extraneous points we get- might change this later on.
	if(x1!=x2) {
		if(x1>x2) {
			xmin=x2;
			xmax=x1;
			ymin=min(y1,y2);
			ymax=((y1+y2)/2)+radius;
		} else {
			xmin=x1;
			xmax=x2;
			ymax=max(y1,y2);
			ymin=((y1+y2)/2)-radius;
		}
	} else {
		if(y1>y2) {
			ymax=y1;
			ymin=y2;
			xmax=x1;
			xmin=x1-radius;
		} else {
			ymax=y2;
			ymin=y1;
			xmin=x1;
			xmax=x1+radius;
		}
	}
	
	// Now loop through all the particles in the boxes we found. Tagging
	// the ones that are within radius of (midx,midy) and are on the
	// appropriate side of the wall.
	box=l.init(xmin,xmax,ymin,ymax,dummy1,dummy2);
	do {
		for(int j=0;j<co[box];j++){
			cpx=p[box][2*j];
			cpy=p[box][2*j+1];
			if((dist_squared(midx,midy,cpx,cpy)<=rs)&&
			(crossproductz((x1-x2),(y1-y2),(cpx-x2),(cpy-y2))>0)){
				if(tmpp==tmpe) add_temporary_label_memory();
				*(tmpp++)=box;
				*(tmpp++)=j;
				*(tmpp++)=wid;
			}
		}
	} while((box=l.inc(dummy1,dummy2))!=-1);
}
		
void container_2d::create_label_table() {
	int ij,q,*pp,tlab(0);

	// Clear label counters
	for(ij=0;ij<nxy;ij++) for(q=0;q<co[ij];q++) nlab[ij][q]=0;

	// Increment label counters
	for(pp=tmp;pp<tmpp;pp+=3) {nlab[*pp][pp[1]]++;tlab++;}

	// Check for case of no labels at all (which may be common)
	if(tlab==0) {
#if VOROPP_VERBOSE >=2
		fputs("No labels needed",stderr);
#endif
		return;
	}

	// If there was already a table from a previous call, remove it
	if(soi!=NULL) delete [] soi;

	// Allocate the label array, and set up pointers from each particle
	// to the corresponding location
	pp=soi=new int[tlab];
	for(ij=0;ij<nxy;ij++) for(q=0;q<co[ij];pp+=nlab[ij][q++]) plab[ij][q]=pp;

	// Fill in the label entries
	for(pp=tmp;pp<tmpp;pp+=3) *(plab[*pp][pp[1]]++)=pp[2];

	// Reset the label pointers 
	pp=soi;
	for(ij=0;ij<nxy;ij++) for(q=0;q<co[ij];pp+=nlab[ij][q++]) plab[ij][q]=pp;
}

/** Draws the boundaries. (Note: this currently assumes that each boundary loop
 * is a continuous block in the bnds array, which will be true for the import
 * function. However, it may not be true in other cases, in which case this
 * routine would have to be extended.) */
void container_2d::draw_boundary(FILE *fp) {
	int i;
	
	for(i=0;i<noofbnds;i++) {
		fprintf(fp,"%d %g %g\n",i,bnds[2*i],bnds[2*i+1]);

		// If a loop is detected, than complete the loop in the output file
		// and insert a newline
		if(edb[i]<i) fprintf(fp,"%g %g\n\n",bnds[2*edb[i]],bnds[2*edb[i]+1]);
	}
}	

/** Increases the size of the temporary label memory. */
void container_2d::add_temporary_label_memory() {
	int size(tmpe-tmp);
	size<<=1;
	if(size>3*max_temp_label_size)
		voropp_fatal_error("Absolute temporary label memory allocation exceeded",VOROPP_MEMORY_ERROR);
#if VOROPP_VERBOSE >=3
	fprintf(stderr,"Temporary label memory in region scaled up to %d\n",size);
#endif			
	int *ntmp(new int[size]),*tp(tmp);tmpp=ntmp;
	while(tp<tmpe) *(tmpp++)=*(tp++);
	delete [] tmp;
	tmp=ntmp;tmpe=tmp+size;
}

/** Increases the memory allocation for the boundary points. */
void container_2d::add_boundary_memory() {
	int i,size(bnds_size<<1);
	if(size>max_bnds_size)
		voropp_fatal_error("Absolute bounds memory allocation exceeded",VOROPP_MEMORY_ERROR);
#if VOROPP_VERBOSE >=3
	fprintf(stderr,"Bounds memory scaled up to %d\n",size);
#endif
	
	// Reallocate the boundary vertex information
	double *nbnds(new double[2*size]);
	for(i=0;i<2*noofbnds;i++) nbnds[i]=bnds[i];
	delete [] nbnds;bnds=nbnds;

	// Reallocate the edge information
	int *nedb(new int[2*size]);
	for(i=0;i<noofbnds;i++) nedb[i]=edb[i];
	delete [] edb;edb=nedb;
}

/** given two particles, a generator and a potential cutting particle, this returns true iff the cutting
particle and the generator are on the same side of all relevant walls. **/

bool container_2d::OKCuttingParticle(double gx, double gy, int gbox, int gindex, double cx, double cy, int cbox, int cindex){
	int cwid;
	double widx1, widy1, widx2, widy2;
	for(int i=0;i<(signed int)nlab[cbox][cindex];i++){
		cwid=plab[cbox][cindex][i];
		widx1=bnds[2*cwid];
		widy1=bnds[2*cwid+1];
		widx2=bnds[2*cwid+2];
		widy2=bnds[2*cwid+3];
		if(crossproductz(gx-widx1,gy-widy1,widx2-widx1,widy2-widy1)!=crossproductz(cx-widx1,cy-widy1,widx2-widx1,widy2-widy1)) return false;
	}
	return true;
}

/** Increase memory for a particular region.
 * \param[in] i the index of the region to reallocate. */
void container_2d::add_particle_memory(int i) {
	int l;
	mem[i]<<=1;
	if(mem[i]>max_particle_memory)
		voropp_fatal_error("Absolute maximum particle memory allocation exceeded",VOROPP_MEMORY_ERROR);
#if VOROPP_VERBOSE >=3
	fprintf(stderr,"Particle memory in region %d scaled up to %d\n",i,mem[i]);
#endif

	// Create new arrays and copy across the data
	int *idp(new int[mem[i]]);
	for(l=0;l<co[i];l++) idp[l]=id[i][l];
	double *pp(new double[2*mem[i]]);
	for(l=0;l<2*co[i];l++) pp[l]=p[i][l];
	unsigned int *nlabp(new unsigned int[mem[i]]);
	for(l=0;l<co[i];l++) nlabp[l]=nlab[i][l];
	int **plabp(new int*[mem[i]]);
	for(l=0;l<co[i];l++) plabp[l]=plab[i][l];

	// Delete the original arrays and update the pointers
	delete [] id[i];id[i]=idp;
	delete [] p[i];p[i]=pp;
	delete [] nlab[i];nlab[i]=nlabp;
	delete [] plab[i];plab[i]=plabp;
}

/** Imports a list of particles from an input stream.
 * \param[in] fp a file handle to read from. */
void container_2d::import(FILE *fp) {
	int i,sm;
	bool boundary(false);
	double x,y;
	char *buf(new char[512]);

	while(fgets(buf,512,fp)!=NULL) {
		if(strcmp(buf,"#Start\n")==0||strcmp(buf,"# Start\n")==0) {

			// Check that two consecutive start tokens haven't been
			// encountered
			if(boundary) voropp_fatal_error("File import error - two start tokens found",VOROPP_FILE_ERROR);
			
			// Remember this position in the boundary array to
			// connect the end of the loop
			sm=noofbnds;
			boundary=true;

		} else if(strcmp(buf,"#End\n")==0||strcmp(buf,"# End\n")==0) {
			
			// Check that two consecutive end tokens haven't been
			// encountered
			if(!boundary) voropp_fatal_error("File import error - two end tokens found",VOROPP_FILE_ERROR);
			
			// Assuming that at least one point was read, set the edge
			// to connect back to the start point
			if(noofbnds!=sm) edb[noofbnds-1]=sm;
			boundary=false;
		} else {

			// Try and read three entries from the line
			if(sscanf(buf,"%d %lg %lg",&i,&x,&y)!=3) voropp_fatal_error("File import error",VOROPP_FILE_ERROR);
			if(!boundary) put(i,x,y, -1);
			if(boundary) {
				put(i,x,y,noofbnds);
				// Unless this is the start of a boundary loop,
				// connect the previous vertex to this one
				if(noofbnds!=sm) edb[noofbnds-1]=noofbnds;
				bnds[2*noofbnds]=x;
				bnds[2*(noofbnds++)+1]=y;
			}
		}
	}

	if(!feof(fp)) voropp_fatal_error("File import error",VOROPP_FILE_ERROR);
	delete [] buf;	
}

/** Clears a container of particles. */
void container_2d::clear() {
	for(int* cop=co;cop<co+nxy;cop++) *cop=0;
}

/** Dumps all the particle positions and IDs to a file.
 * \param[in] fp the file handle to write to. */
void container_2d::draw_particles(FILE *fp) {
	int ij,q;
	for(ij=0;ij<nxy;ij++) for(q=0;q<co[ij];q++)
		fprintf(fp,"%d %g %g\n",id[ij][q],p[ij][2*q],p[ij][2*q+1]);
}

/** Dumps all the particle positions in POV-Ray format.
 * \param[in] fp the file handle to write to. */
void container_2d::draw_particles_pov(FILE *fp) {
	int ij,q;
	for(ij=0;ij<nxy;ij++) for(q=0;q<co[ij];q++)
		fprintf(fp,"// id %d\nsphere{<%g,%g,0>,s\n",id[ij][q],p[ij][2*q],p[ij][2*q+1]);
}

/** Computes the Voronoi cells for all particles and saves the output in
 * gnuplot format.
 * \param[in] fp a file handle to write to. */
void container_2d::draw_cells_gnuplot(FILE *fp) {
	int i,j,ij=0,q;
	double x,y;
	voronoicell_2d c;
	for(j=0;j<ny;j++) for(i=0;i<nx;i++,ij++) for(q=0;q<co[ij];q++) {
		x=p[ij][2*q];y=p[ij][2*q+1];
		if(compute_cell_sphere(c,i,j,ij,q,x,y)) c.draw_gnuplot(x,y,fp);
	}
}

/** Computes the Voronoi cells for all particles within a rectangular box, and
 * saves the output in POV-Ray format.
 * \param[in] fp a file handle to write to. */
void container_2d::draw_cells_pov(FILE *fp) {
	int i,j,ij=0,q;
	double x,y;
	voronoicell_2d c;
	for(j=0;j<ny;j++) for(i=0;i<nx;i++,ij++) for(q=0;q<co[ij];q++) {
		x=p[ij][2*q];y=p[ij][2*q+1];
		if(compute_cell_sphere(c,i,j,ij,q,x,y)) {
			fprintf(fp,"// cell %d\n",id[ij][q]);
			c.draw_pov(x,y,0,fp);
		}
	}
}

/** Computes the Voronoi cells for all particles in the container, and for each
 * cell, outputs a line containing custom information about the cell structure.
 * The output format is specified using an input string with control sequences
 * similar to the standard C printf() routine.
 * \param[in] format the format of the output lines, using control sequences to
 *                   denote the different cell statistics.
 * \param[in] fp a file handle to write to. */
void container_2d::print_custom(const char *format,FILE *fp) {
	int i,j,ij=0,q;
	double x,y;
	voronoicell_2d c;
	for(j=0;j<ny;j++) for(i=0;i<nx;i++,ij++) for(q=0;q<co[ij];q++) {
		x=p[ij][2*q];y=p[ij][2*q+1];
		if(compute_cell_sphere(c,i,j,ij,q,x,y)) c.output_custom(format,id[ij][q],x,y,default_radius,fp);
	}
}

/** Initializes a voronoicell_2d class to fill the entire container.
 * \param[in] c a reference to a voronoicell_2d class.
 * \param[in] (x,y) the position of the particle that . */
//NEED TO CHANGE
bool container_2d::initialize_voronoicell(voronoicell_2d &c,double x,double y) {
	
		double x1,x2,y1,y2;
		if(xperiodic) x1=-(x2=0.5*(bx-ax));else {x1=ax-x;x2=bx-x;}
		if(yperiodic) y1=-(y2=0.5*(by-ay));else {y1=ay-y;y2=by-y;}
		c.init(x1,x2,y1,y2);
		return true;
	
}

bool container_2d::initialize_voronoicell_nonconvex(voronoicell_2d &c, double x, double y, int bid){
	double* bnds_loc=new double[noofbnds*2];
	bnds_loc[0]=0; bnds_loc[1]=0;
	bid++;
	int cntbndno=1;

	while(cntbndno<noofbnds){
		if(bid==(noofbnds-1)) bid=0;
		bnds_loc[cntbndno*2]=bnds[bid*2]-x;
		bnds_loc[cntbndno*2+1]=bnds[bid*2+1]-y;
		cntbndno++;
	}
	c.init_nonconvex(bnds_loc, noofbnds);
	return true;





}
/** Computes all Voronoi cells and sums their areas.
 * \return The computed area. */
double container_2d::sum_cell_areas() {
	int i,j,ij=0,q;
	double x,y,sum=0;
	voronoicell_2d c;
	for(j=0;j<ny;j++) for(i=0;i<nx;i++,ij++) for(q=0;q<co[ij];q++) {
		x=p[ij][2*q];y=p[ij][2*q+1];
		if(compute_cell_sphere(c,i,j,ij,q,x,y)) sum+=c.area();
	}
	return sum;
}

/** Computes all of the Voronoi cells in the container, but does nothing
 * with the output. It is useful for measuring the pure computation time
 * of the Voronoi algorithm, without any additional calculations such as
 * volume evaluation or cell output. */
void container_2d::compute_all_cells() {
	int i,j,ij=0,q;
	voronoicell_2d c;
	for(j=0;j<ny;j++) for(i=0;i<nx;i++,ij++) for(q=0;q<co[ij];q++)
		compute_cell_sphere(c,i,j,ij,q);
}

void container_2d::debug_output(){
	int id;
	
	cout << "problem points   ";
	for(int i=0; i<nxy; i++){
		for(int j=0; j<co[i]; j++){
			id=bndpts[i][j];
			if(id!=-1 && probpts[id]) cout << id << "   ";
		}
	}
	
	cout << "semi_circle_labelling    ";
	for(int i=0; i<nxy; i++){
		for(int j=0; j<co[i]; j++){
			cout << "computational box=" << i << "   paricle_no=" << j << "wall_ids=";
			for(int q=0;q<(signed int)nlab[i][j];q++){	
				cout << "  " << plab[i][j][q] << "   ";
			}
		}
	}

	cout << "wall_tags    ";
	for(int i=0; i<nxy; i++){
		cout << "computational box # " << i << "  wall tags  ";
		for(int j=1; j<wid[i][0] ; j++){
			cout << wid[i][j] << "   ";
		}

	}
}
/** This routine computes the Voronoi cell for a give particle, by successively
 * testing over particles within larger and larger concentric circles. This
 * routine is simple and fast, although it may not work well for anisotropic
 * arrangements of particles.
 * \param[in,out] c a reference to a voronoicell object.
 * \param[in] (i,j) the coordinates of the block that the test particle is
 *		    in.
 * \param[in] ij the index of the block that the test particle is in, set to
 *		 i+nx*j.
 * \param[in] s the index of the particle within the test block.
 * \param[in] (x,y) the coordinates of the particle.
 * \return False if the Voronoi cell was completely removed during the
 * computation and has zero volume, true otherwise. */
bool container_2d::compute_cell_sphere(voronoicell_2d &c,int i,int j,int ij,int s,double x,double y) {
	
	// This length scale determines how large the spherical shells should
	// be, and it should be set to approximately the particle diameter
	const double length_scale=0.5*sqrt((bx-ax)*(by-ay)/(nx*ny));
	bool problem_point=false;
	double x1,y1,qx,qy,lr=0,lrs=0,ur,urs,rs,wx1,wy1,wx2,wy2;
	int q,t,bid, widc;
	voropp_loop_2d l(*this);
	

	//Check if the current point is a problem point, if it is we will need to use plane-nonconvex, and init_nonconvex
	bid=bndpts[bid][s];
	if(bid!=-1 && probpts[bid]){
			if(!initialize_voronoicell_nonconvex(c,x,y,bid)) return false;
			problem_point=true;
		
	}else{
		if(!initialize_voronoicell(c,x,y)) return false;
	}
	//CUT BY ALL WALLS PASSING THROUGH THE COMPUTATIONAL BOX HERE, USE WID
	for(int b=1;b<wid[ij][0];b++){
	widc=wid[ij][b];
		wx1=bnds[2*widc]-x; wy1=bnds[2*widc-1]-y;
		widc++;
		wx2=bnds[2*widc]-x; wy2=bnds[2*widc-1]-y;
		c.wallcut(wx1,wy1,wx2,wy2);
	}

	// Now the cell is cut by testing neighboring particles in concentric
	// shells. Once the test shell becomes twice as large as the Voronoi
	// cell we can stop testing.
	while(lrs<c.max_radius_squared()) {
		ur=lr+0.5*length_scale;urs=ur*ur;
		t=l.init(x,y,ur,qx,qy);
		do {
			for(q=0;q<co[t];q++) {
				//HERE CHECK IF THE CUTTING PARTICLE IS ON THE WRONG SIDE OF ANY WALL USING SOI				
				if(OKCuttingParticle(x,y,ij,s,x1,y1,t,q)){
					x1=p[t][2*q]+qx-x;y1=p[t][2*q+1]+qy-y;//qx,qy??
					rs=x1*x1+y1*y1;
					if(lrs-tolerance<rs&&rs<urs&&(q!=s||ij!=t)) {
						if(problem_point){
							if(!c.plane_nonconvex(x1,y1,rs)) return false;
						}else{
							if(!c.plane(x1,y1,rs)) return false;
						}
					}
				}
			}
		} while((t=l.inc(qx,qy))!=-1);//loops through boxes in the shell
		lr=ur;lrs=urs;
	}
	return true;
}

/** Creates a voropp_loop_2d object, by setting the necessary constants about the
 * container geometry from a pointer to the current container class.
 * \param[in] con a reference to the associated container class. */
voropp_loop_2d::voropp_loop_2d(container_2d &con) : boxx(con.bx-con.ax), boxy(con.by-con.ay),
	xsp(con.xsp),ysp(con.ysp),
	ax(con.ax),ay(con.ay),nx(con.nx),ny(con.ny),nxy(con.nxy),
	xperiodic(con.xperiodic),yperiodic(con.yperiodic) {}

/** Initializes a voropp_loop_2d object, by finding all blocks which are within a
 * given sphere. It calculates the index of the first block that needs to be
 * tested and sets the periodic displacement vector accordingly.
 * \param[in] (vx,vy) the position vector of the center of the sphere.
 * \param[in] r the radius of the sphere.
 * \param[out] (px,py) the periodic displacement vector for the first block to
 * 		       be tested.??????
 * \return The index of the first block to be tested. */
int voropp_loop_2d::init(double vx,double vy,double r,double &px,double &py) {
	ai=step_int((vx-ax-r)*xsp);
	bi=step_int((vx-ax+r)*xsp);
	if(!xperiodic) {
		if(ai<0) {ai=0;if(bi<0) bi=0;}
		if(bi>=nx) {bi=nx-1;if(ai>=nx) ai=nx-1;}
	}
	aj=step_int((vy-ay-r)*ysp);
	bj=step_int((vy-ay+r)*ysp);
	if(!yperiodic) {
		if(aj<0) {aj=0;if(bj<0) bj=0;}
		if(bj>=ny) {bj=ny-1;if(aj>=ny) aj=ny-1;}
	}
	i=ai;j=aj;
	aip=ip=step_mod(i,nx);apx=px=step_div(i,nx)*boxx;
	ajp=jp=step_mod(j,ny);apy=py=step_div(j,ny)*boxy;
	inc1=aip-step_mod(bi,nx)+nx;
	s=aip+nx*ajp;
	return s;
}

/** Initializes a voropp_loop_2d object, by finding all blocks which overlap a given
 * rectangular box. It calculates the index of the first block that needs to be
 * tested and sets the periodic displacement vector (px,py,pz) accordingly.
 * \param[in] (xmin,xmax) the minimum and maximum x coordinates of the box.
 * \param[in] (ymin,ymax) the minimum and maximum y coordinates of the box.
 * \param[out] (px,py) the periodic displacement vector for the first block
 *		       to be tested.
 * \return The index of the first block to be tested. */
int voropp_loop_2d::init(double xmin,double xmax,double ymin,double ymax,double &px,double &py) {
	ai=step_int((xmin-ax)*xsp);
	bi=step_int((xmax-ax)*xsp);
	if(!xperiodic) {
		if(ai<0) {ai=0;if(bi<0) bi=0;}
		if(bi>=nx) {bi=nx-1;if(ai>=nx) ai=nx-1;}
	}
	aj=step_int((ymin-ay)*ysp);
	bj=step_int((ymax-ay)*ysp);
	if(!yperiodic) {
		if(aj<0) {aj=0;if(bj<0) bj=0;}
		if(bj>=ny) {bj=ny-1;if(aj>=ny) aj=ny-1;}
	}
	i=ai;j=aj;
	aip=ip=step_mod(i,nx);apx=px=step_div(i,nx)*boxx;
	ajp=jp=step_mod(j,ny);apy=py=step_div(j,ny)*boxy;
	inc1=aip-step_mod(bi,nx)+nx;
	s=aip+nx*ajp;
	return s;
}

/** Returns the next block to be tested in a loop, and updates the periodicity
 * vector if necessary.
 * \param[in,out] (px,py) the current block on entering the function, which is
 *                        updated to the next block on exiting the function. */
int voropp_loop_2d::inc(double &px,double &py) {
	if(i<bi) {
		i++;
		if(ip<nx-1) {ip++;s++;} else {ip=0;s+=1-nx;px+=boxx;}
		return s;
	} else if(j<bj) {
		i=ai;ip=aip;px=apx;j++;
		if(jp<ny-1) {jp++;s+=inc1;} else {jp=0;s+=inc1-nxy;py+=boxy;}
		return s;
	} else return -1;
}