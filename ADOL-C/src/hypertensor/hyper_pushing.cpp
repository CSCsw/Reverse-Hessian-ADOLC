#include <iostream>

#include <adolc/adolc.h>
#include <adolc/hypertensor/VectorGraph.h>
#include <adolc/hypertensor/MatrixGraph.h>
#include <adolc/hypertensor/HyperGraph.h>
#include <adolc/hypertensor/hyper_common.h>
#include <adolc/hypertensor/hyper_derivative.h>

#ifdef ENABLE_HYPER_MPI
#include "mpi.h"
#define DEBUG_ID 99

#define IF_MY_DEBUG if(myid==DEBUG_ID){

#define END_MY_DEBUG }

#endif

void hyper_third(DerivativeInfo<locint>& info,
                VectorGraph<locint>* adjoints,
                MatrixGraph<locint>* hessian,
                HyperGraph<locint>* tensor,
                double w,
                VectorGraph<locint>* r,
                MatrixGraph<locint>* e) {
//  std::cout << "In tensor " << std::endl;

  locint p;
  locint q;
  double pw;
  double coeff = 0.0;
  double xcoeff = 0.0;
  double ycoeff = 0.0;

  MatrixGraph<locint>::iterator* e_iter = e->get_iterator();
  VectorGraph<locint>::iterator* r_iter = r->get_iterator();

  bool has_next = e_iter->reset();
  while(has_next) {
    // p >= q
    has_next = e_iter->get_next(p, q, pw);
    if (p != info.r and q!= info.r) {
      if (info.x != NULLLOC && info.dx != 0.0) {
        xcoeff = 1.0;
        if (info.x == p) {xcoeff += 1.0;}
        if (info.x == q) {xcoeff += 1.0;}
        tensor->increase(p, q, info.x, xcoeff * pw * info.dx);
      }
      if (info.y != NULLLOC && info.dy != 0.0) {
        ycoeff = 1.0;
        if (info.y == p) {ycoeff += 1.0;}
        if (info.y == q) {ycoeff += 1.0;}
        tensor->increase(p, q, info.y, ycoeff * pw * info.dy);
      }
    } else { // p == r or q == r;
      if (p != q) {
        if (q == info.r) {T_SWAP(p,q);}  // p == r != q
        if (info.x != NULLLOC && info.y != NULLLOC) {
          coeff = 1.0; xcoeff = 1.0; ycoeff = 1.0;
          if (info.x == q) {coeff += 1.0; xcoeff += 2.0;}
          if (info.y == q) {coeff += 1.0; ycoeff += 2.0;}
          tensor->increase(q, info.x, info.x, xcoeff * pw * info.dx * info.dx);
          tensor->increase(q, info.x, info.y, coeff * pw * info.dx * info.dy);
          tensor->increase(q, info.y, info.y, ycoeff * pw * info.dy * info.dy);
        } else if (info.x != NULLLOC) {
          xcoeff = 1.0;
          if (info.x == q) {xcoeff += 2.0;}
          tensor->increase(q, info.x, info.x, xcoeff * pw * info.dx * info.dx);
        }
      } else {  // p == q == r
        if (info.x != NULLLOC && info.dx != 0.0) {
          tensor->increase(info.x, info.x, info.x,
                           pw * info.dx * info.dx * info.dx);
          if (info.y != NULLLOC && info.dy != 0.0) {
            tensor->increase(info.x, info.x, info.y,
                             pw * info.dx * info.dx * info.dy);
            tensor->increase(info.x, info.y, info.y,
                             pw * info.dx * info.dy * info.dy);
            tensor->increase(info.y, info.y, info.y,
                             pw * info.dy * info.dy * info.dy);
  
          }
        }
      }
    }
  }

  has_next = r_iter->reset();
  while (has_next) {
    has_next = r_iter->get_next(p, pw);
    if (p != info.r) {
      if (info.x != NULLLOC && info.y != NULLLOC) {
        coeff = 1.0; xcoeff = 1.0; ycoeff = 1.0;
        if (info.x == p) {coeff += 1.0; xcoeff += 2.0;}
        if (info.y == p) {coeff += 1.0; ycoeff += 2.0;}
        tensor->increase(p, info.x, info.x, xcoeff * pw * info.pxx);
        tensor->increase(p, info.x, info.y, coeff * pw * info.pxy);
        tensor->increase(p, info.y, info.y, ycoeff * pw * info.pyy);
      } else if (info.x != NULLLOC) {
        xcoeff = 1.0;
        if (info.x == p) {xcoeff += 2.0;}
        tensor->increase(p, info.x, info.x, xcoeff * pw * info.pxx);
      }
    } else { // p == r;
      if (info.x != NULLLOC) {
        if (info.y != NULLLOC) {
          tensor->increase(info.x, info.x, info.x,
                           3.0 * pw * info.dx * info.pxx);
          tensor->increase(info.x, info.x, info.y, 
                           2.0 * pw * info.dx * info.pxy);
          tensor->increase(info.x, info.y, info.y,
                           1.0 * pw * info.dx * info.pyy);
          tensor->increase(info.y, info.x, info.x,
                           1.0 * pw * info.dy * info.pxx);
          tensor->increase(info.y, info.x, info.y,
                           2.0 * pw * info.dy * info.pxy);
          tensor->increase(info.y, info.y, info.y,
                           3.0 * pw * info.dy * info.pyy);
        } else {
          tensor->increase(info.x, info.x, info.x,
                           3.0 * pw * info.dx * info.pxx);
        }
      }
    }
  }
  if (w != 0.0) {
    if (info.x != NULLLOC && info.y != NULLLOC) {
      tensor->increase(info.x, info.x, info.x, w * info.pxxx);
      tensor->increase(info.x, info.x, info.y, w * info.pxxy);
      tensor->increase(info.x, info.y, info.y, w * info.pxyy);
      tensor->increase(info.y, info.y, info.y, w * info.pyyy);
    } else if (info.x != NULLLOC) {
      tensor->increase(info.x, info.x, info.x, w * info.pxxx);
    }
  }
  delete e_iter;
  delete r_iter;
//  tensor->debug();
}

void hyper_hessian(DerivativeInfo<locint>& info,
                  VectorGraph<locint>* adjoints,
                  MatrixGraph<locint>* hessian,
                  double w,
                  VectorGraph<locint>* r) {
//  std::cout << "In hessian" << std::endl;
  typename VectorGraph<locint>::iterator* r_iter = r->get_iterator();
  bool has_next = r_iter->reset();
  locint p;
  double pw;
  while (has_next) {
    has_next = r_iter->get_next(p, pw);
//    std::cout << p << "," << pw << std::endl;
    if (pw != 0.0) {
      if (info.y != NULLLOC){
        if (p != info.r){
          if (p == info.x){
            hessian->increase(p, p, 2 * info.dx * pw);
          } else {
            hessian->increase(info.x, p, info.dx * pw);
          }
          if(p == info.y){
            hessian->increase(p, p, 2 * info.dy * pw);
          } else {
            hessian->increase(info.y, p, info.dy * pw);
          }
        } else {
          hessian->increase(info.x, info.x, info.dx * info.dx * pw);
          hessian->increase(info.x, info.y, info.dx * info.dy * pw);
          hessian->increase(info.y, info.y, info.dy * info.dy * pw);
        }
      } else {
        if (p != info.r){
          if (p == info.x){
            hessian->increase(p, p, 2 * info.dx * pw);
          } else {
            hessian->increase(info.x, p, info.dx * pw);
          }
        } else {
          hessian->increase(info.x,info.x,info.dx * info.dx * pw);
        }
      }
    } // if (pw != 0)
  } // while (has_next)

  if (w != 0.0) {
    if (info.pxx != 0.0) {
      hessian->increase(info.x, info.x, w * info.pxx);
    }
    if (info.pyy != 0.0) {
      hessian->increase(info.y, info.y, w * info.pyy);
    }
    if (info.pxy != 0.0) {
      if (info.x != info.y) {
        hessian->increase(info.x, info.y, w * info.pxy);
      } else {
        hessian->increase(info.x, info.y, 2.0 * w * info.pxy);
      }
    }
  } // if (w != 0)
//  hessian->debug();
  delete r_iter;
}

void hyper_adjoints(DerivativeInfo<locint>& info,
                   VectorGraph<locint>* adjoints,
                   double w) {
//  std::cout << "In adjoints" << std::endl;
  if (info.x != NULLLOC) {
    adjoints->increase(info.x, w * info.dx);
  }
  if (info.y != NULLLOC) {
    adjoints->increase(info.y, w * info.dy);
  }
//  adjoints->debug();
}

void hyper_process_sac(DerivativeInfo<locint>& info,
                       HyperDerivative<locint>& global_gd) {
  double w = global_gd.adjoints->get_and_erase(info.r);
  VectorGraph<locint>* r = global_gd.hessian->get_and_erase(info.r);
  // Hessian
  hyper_hessian(info, global_gd.adjoints, global_gd.hessian, w, r);
  // Adjoints
  hyper_adjoints(info, global_gd.adjoints, w);
  delete r;
}

void hyper_hessian_gd(locint dep,
                      VectorGraph<locint>* l_a,
                      MatrixGraph<locint>* l_h,
                      double w,
                      VectorGraph<locint>* r,
                      MatrixGraph<locint>* g_h) {
  typename VectorGraph<locint>::iterator* r_iter = r->get_iterator();
  bool has_next = r_iter->reset();
  locint p;
  double pw;
  locint v;
  double vw;
  locint v2;
  double vw2;
  while (has_next) {
    has_next = r_iter->get_next(p, pw);
//    std::cout << p << "," << pw << std::endl;
    if (p != dep) {
      typename VectorGraph<locint>::iterator* a_iter = l_a->get_iterator();
      bool a_has_next = a_iter->reset();
      while (a_has_next) {
        a_has_next = a_iter->get_next(v, vw);
        if (p != v) {
          g_h->increase(p, v, pw * vw);
        } else {
          g_h->increase(p, v, 2.0 * pw * vw);
        }
      }
    } else { // p == dep
      typename VectorGraph<locint>::iterator* a_iter = l_a->get_iterator();
      bool a_has_next = a_iter->reset();
      while(a_has_next) {
        VectorGraph<locint>::iterator* a2_iter = a_iter->copy_iter();
        bool a2_has_next = a_has_next;
        a_has_next = a_iter->get_next(v, vw);
        while(a2_has_next) {
          a2_has_next = a2_iter->get_next(v2, vw2);
          g_h->increase(v, v2, pw * vw * vw2);
        }
      }
    }
  } // while
  if (w != 0.0) {
    typename MatrixGraph<locint>::iterator* h_iter = l_h->get_iterator();
    bool h_has_next = h_iter->reset();
    while(h_has_next) {
      h_has_next = h_iter->get_next(v, v2, vw);
      g_h->increase(v, v2, w * vw);
    }
  }
//  hessian->debug();
  delete r_iter;
}

void hyper_adjoints_gd(VectorGraph<locint>* l_a,
                       VectorGraph<locint>* g_a,
                       double w) {
  if (w != 0.0) {
    locint v;
    double vw;
    typename VectorGraph<locint>::iterator* a_iter = l_a->get_iterator();
    bool a_has_next = a_iter->reset();
    while(a_has_next) {
      a_has_next = a_iter->get_next(v, vw);
      g_a->increase(v, w * vw);
    }
  }
}

void hyper_process_gd(locint dep,
                      HyperDerivative<locint>& local_gd,
                      HyperDerivative<locint>& global_gd) {
#ifdef ENABLE_HYPER_MPI
  int myid;
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  if (myid == DEBUG_ID) {
    global_gd.debug();
  }
#endif
  IF_MY_DEBUG
  std::cout << "Processing dep: " << dep << std::endl;
  END_MY_DEBUG
  double w = global_gd.adjoints->get_and_erase(dep);
  IF_MY_DEBUG
  std::cout << " w = " << w << std::endl;
  END_MY_DEBUG
  VectorGraph<locint>* r = global_gd.hessian->get_and_erase(dep);
  IF_MY_DEBUG
  r->debug();
  global_gd.debug();
  END_MY_DEBUG
  // Hessian
  hyper_hessian_gd(dep, local_gd.adjoints, local_gd.hessian,
                   w, r, global_gd.hessian);
  // Adjoints
  hyper_adjoints_gd(local_gd.adjoints, global_gd.adjoints, w);
  delete r;
#ifdef ENABLE_HYPER_MPI
  if (myid == DEBUG_ID) {
    global_gd.debug();
  }
#endif
}

void hyper_process_recv_gd(
    locint dep,
    HyperDerivative<locint>& local_gd,
    std::map<locint, HyperDerivative<locint> >& global_gd) {
  locint loc;
#ifdef ENABLE_HYPER_MPI
  int myid;
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
#endif
  IF_MY_DEBUG
  std::cout << "local_gd :" << std::endl;
  local_gd.debug();
  std::cout << "---------" << std::endl;
  END_MY_DEBUG
  std::map<locint, HyperDerivative<locint> >::iterator t_iter;
  t_iter = global_gd.begin();
  while(t_iter != global_gd.end()) {
    loc = t_iter->first;
    if (global_gd[loc].adjoints->has_live(dep)) {
      hyper_process_gd(dep, local_gd, global_gd[loc]);
    }
    ++t_iter;
  }
}
