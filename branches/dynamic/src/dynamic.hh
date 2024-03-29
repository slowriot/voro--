// Voro++, a 3D cell-based Voronoi library
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : July 1st 2008

/** \file dynamic.hh
 * \brief Header file for the dynamic extension classes, which add
 * functionality for a variety of dynamic particle motions. */

#ifndef VOROPP_DYNAMIC_HH
#define VOROPP_DYNAMIC_HH

class cond_all {
	public:
		inline bool test(fpoint cx,fpoint cy,fpoint cz) {return true;}
};

class velocity_internal {
	public:
		velocity_internal(fpoint **&ive) : track_ve(true), ve(ive) {};
		inline void vel(int ijk,int q,fpoint &x,fpoint &y,fpoint &z) {
			x+=ve[ijk][3*q];
			y+=ve[ijk][3*q+1];
			z+=ve[ijk][3*q+2];
		}
		const bool track_ve;
	private:
		fpoint **&ve;
};

class velocity_brownian {
	public:
		velocity_brownian() : track_ve(false), mag(0.05), tmag(2*mag) {}; 
		inline void vel(int ijk,int q,fpoint &x,fpoint &y,fpoint &z) {
			x+=tmag*rnd()-mag;
			y+=tmag*rnd()-mag;
			z+=tmag*rnd()-mag;
		}
		const bool track_ve;
	private:
		const fpoint mag,tmag;
		inline fpoint rnd() {return fpoint(rand())/RAND_MAX;}
};

class velocity_brownian2 {
	public:
		velocity_brownian2() : track_ve(false), mag(0.02), tmag(2*mag) {}; 
		inline void vel(int ijk,int q,fpoint &x,fpoint &y,fpoint &z) {
			fpoint dx,dy,dz;
			do {
				dx=tmag*rnd()-mag;
				dy=tmag*rnd()-mag;
				dz=tmag*rnd()-mag;
			} while (dx*dx+dy*dy+dz*dz>=mag*mag);
			x+=dx/*+0.0001*sin(y*3.1415926535897932384626433832795/20)*/;y+=dy;z+=dz;
		}
		const bool track_ve;
	private:
		const fpoint mag,tmag;
		inline fpoint rnd() {return fpoint(rand())/RAND_MAX;}
};

class velocity_constant {
	public:
		velocity_constant(fpoint idx,fpoint idy,fpoint idz) : track_ve(false),
       			dx(idx), dy(idy), dz(idz) {};
		inline void vel(int ijk,int q,fpoint &x,fpoint &y,fpoint &z) {
			x+=dx;y+=dy;z+=dz;
		}
		const bool track_ve;
	private:
		const fpoint dx,dy,dz;
};

class velocity_gaussian {
	public:
		velocity_gaussian(fpoint icx,fpoint icy,fpoint icz,fpoint idx,fpoint idy,fpoint idz,fpoint idec)
			: track_ve(false), cx(idx), cy(idy), cz(idz), dx(idx), dy(idy), dz(idz), dec(idec) {};
		inline void vel(int ijk,int q,fpoint &x,fpoint &y,fpoint &z) {
			fpoint ex=x-cx,ey=y-cy,ez=z-cz;
			ex=exp(-dec*(ex*ex+ey*ey+ez*ez));
			x+=ex*dx;y+=ex*dy;z+=ex*dz;
		}
		const bool track_ve;
	private:
		const fpoint cx,cy,cz;
		const fpoint dx,dy,dz;
		const fpoint dec;
};

template<class r_option>
class container_dynamic_base : public container_base<r_option> {
	public:
		container_dynamic_base(fpoint xa,fpoint xb,fpoint ya,fpoint yb,fpoint za,
				fpoint zb,int xn,int yn,int zn,const bool xper,const bool yper,const bool zper,int memi);
		~container_dynamic_base();
		using container_base<r_option>::xperiodic;
		using container_base<r_option>::yperiodic;
		using container_base<r_option>::zperiodic;
		using container_base<r_option>::ax;
		using container_base<r_option>::ay;
		using container_base<r_option>::az;
		using container_base<r_option>::bx;
		using container_base<r_option>::by;
		using container_base<r_option>::bz;
		using container_base<r_option>::nx;
		using container_base<r_option>::ny;
		using container_base<r_option>::nz;
		using container_base<r_option>::nxyz;
		using container_base<r_option>::xsp;
		using container_base<r_option>::ysp;
		using container_base<r_option>::zsp;
		using container_base<r_option>::co;
		using container_base<r_option>::p;
		using container_base<r_option>::sz;
		using container_base<r_option>::id;
		using container_base<r_option>::radius;
		using container_base<r_option>::wall_number;
		using container_base<r_option>::walls;
		using container_base<r_option>::mem;
		inline void clear_velocities();
		inline void damp_velocities(fpoint damp);
		void wall_diagnostic();
		int count(fpoint x,fpoint y,fpoint z,fpoint r);
		void spot(fpoint cx,fpoint cy,fpoint cz,fpoint dx,fpoint dy,fpoint dz,fpoint rad);
		void gauss_spot(fpoint cx,fpoint cy,fpoint cz,fpoint dx,fpoint dy,fpoint dz,fpoint dec,fpoint rad);
		void relax(fpoint cx,fpoint cy,fpoint cz,fpoint rad,fpoint alpha);
		template<class cond_class>
		void neighbor_distribution(int *bb,fpoint dr,int max); 
		template<class cond_class>
		fpoint packing_badness(); 
		void full_relax(fpoint alpha);
		template<class v_class>
		void local_move(v_class &vcl,fpoint cx,fpoint cy,fpoint cz,fpoint rad); 
		template<class v_class>
		inline void move() {v_class vcl;move(vcl);}
		template<class v_class>
		void move(v_class &vcl);
		void add_particle_memory(int i);
		inline int full_count();
#ifdef YEAST_ROUTINES
		void stick(fpoint alpha);
		void draw_yeast_pov(const char *filename);
		void draw_yeast_pov(ostream &os);
#endif
	protected:
		int *gh;
		fpoint **ve;
		velocity_internal v_inter;
	private:
		inline int step_mod(int a,int b);
		inline int step_div(int a,int b);
		inline int step_int(fpoint a);
		inline void wall_contribution(int s,int l,fpoint cx,fpoint cy,fpoint cz,fpoint alpha);
		inline void wall_badness(fpoint cx,fpoint cy,fpoint cz,fpoint &badcount);
};

/** The basic dynamic container class. */
typedef container_dynamic_base<radius_mono> container_dynamic;

/** The polydisperse dynamic container class. */
typedef container_dynamic_base<radius_poly> container_dynamic_poly;

#endif
