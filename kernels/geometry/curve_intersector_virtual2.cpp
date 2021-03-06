// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
 
#include "curve_intersector_virtual.h"
//#include "intersector_epilog.h"
//
//#include "../subdiv/bezier_curve.h"
//#include "../subdiv/bspline_curve.h"
//#include "../subdiv/hermite_curve.h"
//#include "../subdiv/catmullrom_curve.h"
//
//#include "spherei_intersector.h"
//#include "disci_intersector.h"
//
//#include "linei_intersector.h"
//
//#include "curveNi_intersector.h"
//#include "curveNv_intersector.h"
//#include "curveNi_mb_intersector.h"
//
//#include "curve_intersector_distance.h"
//#include "curve_intersector_ribbon.h"
//#include "curve_intersector_oriented.h"
//#include "curve_intersector_sweep.h"

#include "curve_intersector_virtual_point.h"
#include "curve_intersector_virtual_linear_curve.h"
#include "curve_intersector_virtual_bezier_curve.h"
#include "curve_intersector_virtual_bspline_curve.h"
#include "curve_intersector_virtual_hermite_curve.h"
#include "curve_intersector_virtual_catmullrom_curve.h"

namespace embree
{
  namespace isa
  {
#if defined (__AVX__)
    
    VirtualCurveIntersector* VirtualCurveIntersector8i()
    {
      static VirtualCurveIntersector function_local_static_prim;
      AddVirtualCurvePointInterector8i(function_local_static_prim);
      AddVirtualCurveLinearCurveInterector8i(function_local_static_prim);
      AddVirtualCurveBezierCurveInterector8i(function_local_static_prim);
      AddVirtualCurveBSplineCurveInterector8i(function_local_static_prim);
      AddVirtualCurveHermiteCurveInterector8i(function_local_static_prim);
      AddVirtualCurveCatmullRomCurveInterector8i(function_local_static_prim);
      //function_local_static_prim.vtbl[Geometry::GTY_SPHERE_POINT] = SphereNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_DISC_POINT] = DiscNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_DISC_POINT] = OrientedDiscNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_CONE_LINEAR_CURVE ] = LinearConeNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_LINEAR_CURVE ] = LinearRoundConeNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_LINEAR_CURVE ] = LinearRibbonNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_BEZIER_CURVE] = CurveNiIntersectors <BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_BEZIER_CURVE ] = RibbonNiIntersectors<BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_BEZIER_CURVE] = OrientedCurveNiIntersectors<BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_BSPLINE_CURVE] = CurveNiIntersectors <BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_BSPLINE_CURVE ] = RibbonNiIntersectors<BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_BSPLINE_CURVE] = OrientedCurveNiIntersectors<BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_HERMITE_CURVE] = HermiteCurveNiIntersectors <HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_HERMITE_CURVE ] = HermiteRibbonNiIntersectors<HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_HERMITE_CURVE] = HermiteOrientedCurveNiIntersectors<HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_CATMULL_ROM_CURVE] = CurveNiIntersectors <CatmullRomCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_CATMULL_ROM_CURVE ] = RibbonNiIntersectors<CatmullRomCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_CATMULL_ROM_CURVE] = OrientedCurveNiIntersectors<CatmullRomCurveT,8>();
      return &function_local_static_prim;
    }

    VirtualCurveIntersector* VirtualCurveIntersector8v()
    {
      static VirtualCurveIntersector function_local_static_prim;
      AddVirtualCurvePointInterector8v(function_local_static_prim);
      AddVirtualCurveLinearCurveInterector8v(function_local_static_prim);
      AddVirtualCurveBezierCurveInterector8v(function_local_static_prim);
      AddVirtualCurveBSplineCurveInterector8v(function_local_static_prim);
      AddVirtualCurveHermiteCurveInterector8v(function_local_static_prim);
      AddVirtualCurveCatmullRomCurveInterector8v(function_local_static_prim);
      //function_local_static_prim.vtbl[Geometry::GTY_SPHERE_POINT] = SphereNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_DISC_POINT] = DiscNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_DISC_POINT] = OrientedDiscNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_CONE_LINEAR_CURVE ] = LinearConeNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_LINEAR_CURVE ] = LinearRoundConeNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_LINEAR_CURVE ] = LinearRibbonNiIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_BEZIER_CURVE] = CurveNvIntersectors <BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_BEZIER_CURVE ] = RibbonNvIntersectors<BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_BEZIER_CURVE] = OrientedCurveNiIntersectors<BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_BSPLINE_CURVE] = CurveNvIntersectors <BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_BSPLINE_CURVE ] = RibbonNvIntersectors<BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_BSPLINE_CURVE] = OrientedCurveNiIntersectors<BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_HERMITE_CURVE] = HermiteCurveNiIntersectors <HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_HERMITE_CURVE ] = HermiteRibbonNiIntersectors<HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_HERMITE_CURVE] = HermiteOrientedCurveNiIntersectors<HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_CATMULL_ROM_CURVE] = CurveNiIntersectors <CatmullRomCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_CATMULL_ROM_CURVE ] = RibbonNiIntersectors<CatmullRomCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_CATMULL_ROM_CURVE] = OrientedCurveNiIntersectors<CatmullRomCurveT,8>();
      return &function_local_static_prim;
    }
    
    VirtualCurveIntersector* VirtualCurveIntersector8iMB()
    {
      static VirtualCurveIntersector function_local_static_prim;
      AddVirtualCurvePointInterector8iMB(function_local_static_prim);
      AddVirtualCurveLinearCurveInterector8iMB(function_local_static_prim);
      AddVirtualCurveBezierCurveInterector8iMB(function_local_static_prim);
      AddVirtualCurveBSplineCurveInterector8iMB(function_local_static_prim);
      AddVirtualCurveHermiteCurveInterector8iMB(function_local_static_prim);
      AddVirtualCurveCatmullRomCurveInterector8iMB(function_local_static_prim);
      //function_local_static_prim.vtbl[Geometry::GTY_SPHERE_POINT] = SphereNiMBIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_DISC_POINT] = DiscNiMBIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_DISC_POINT] = OrientedDiscNiMBIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_CONE_LINEAR_CURVE ] = LinearConeNiMBIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_LINEAR_CURVE ] = LinearRoundConeNiMBIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_LINEAR_CURVE ] = LinearRibbonNiMBIntersectors<8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_BEZIER_CURVE] = CurveNiMBIntersectors <BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_BEZIER_CURVE ] = RibbonNiMBIntersectors<BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_BEZIER_CURVE] = OrientedCurveNiMBIntersectors<BezierCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_BSPLINE_CURVE] = CurveNiMBIntersectors <BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_BSPLINE_CURVE ] = RibbonNiMBIntersectors<BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_BSPLINE_CURVE] = OrientedCurveNiMBIntersectors<BSplineCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_HERMITE_CURVE] = HermiteCurveNiMBIntersectors <HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_HERMITE_CURVE ] = HermiteRibbonNiMBIntersectors<HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_HERMITE_CURVE] = HermiteOrientedCurveNiMBIntersectors<HermiteCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ROUND_CATMULL_ROM_CURVE] = CurveNiMBIntersectors <CatmullRomCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_FLAT_CATMULL_ROM_CURVE ] = RibbonNiMBIntersectors<CatmullRomCurveT,8>();
      //function_local_static_prim.vtbl[Geometry::GTY_ORIENTED_CATMULL_ROM_CURVE] = OrientedCurveNiMBIntersectors<CatmullRomCurveT,8>();
      return &function_local_static_prim;
    }
  
#endif
  }
}
