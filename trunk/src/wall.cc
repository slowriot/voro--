// Voro++, a 3D cell-based Voronoi library
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : July 1st 2008

#include "wall.hh"

bool wall_sphere::point_inside(fpoint x,fpoint y,fpoint z) {
	return (x-xc)*(x-xc)+(y-yc)*(y-yc)+(z-zc)*(z-zc)<rc*rc;
}

template<class n_option>
inline bool wall_sphere::cut_cell_base(voronoicell_base<n_option> &c,fpoint x,fpoint y,fpoint z) {
	fpoint xd=x-xc,yd=y-yc,zd=z-zc,dq;
	dq=xd*xd+yd*yd+zd*zd;
	if (dq>1e-5) {
		dq=2*(sqrt(dq)*rc-dq);
		return c.nplane(xd,yd,zd,dq,w_id);
	}
	return true;
}

bool wall_plane::point_inside(fpoint x,fpoint y,fpoint z) {
	return x*xc+y*yc+z*zc<ac;
}

template<class n_option>
inline bool wall_plane::cut_cell_base(voronoicell_base<n_option> &c,fpoint x,fpoint y,fpoint z) {
	fpoint dq=2*(ac-x*xc-y*yc-z*zc);
	return c.nplane(xc,yc,zc,dq,w_id);
}

bool wall_cylinder::point_inside(fpoint x,fpoint y,fpoint z) {
	fpoint xd=x-xc,yd=y-yc,zd=z-zc;
	fpoint pa=(xd*xa+yd*ya+zd*za)*asi;
	xd-=xa*pa;yd-=ya*pa;zd-=za*pa;
	return xd*xd+yd*yd+zd*zd<rc*rc;
}

template<class n_option>
inline bool wall_cylinder::cut_cell_base(voronoicell_base<n_option> &c,fpoint x,fpoint y,fpoint z) {
	fpoint xd=x-xc,yd=y-yc,zd=z-zc;
	fpoint pa=(xd*xa+yd*ya+zd*za)*asi;
	xd-=xa*pa;yd-=ya*pa;zd-=za*pa;
	pa=xd*xd+yd*yd+zd*zd;
	if(pa>1e-5) {
		pa=2*(sqrt(pa)*rc-pa);
		return c.nplane(xd,yd,zd,pa,w_id);
	}
	return true;
}