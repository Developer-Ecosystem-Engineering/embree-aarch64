// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "bvh4i_intersector16_subdiv.h"
#include "bvh4i_leaf_intersector.h"
#include "geometry/subdivpatch1.h"

#define TIMER(x) x

namespace embree
{

  static AtomicCounter numLazyBuildPatches = 0;

  static AtomicMutex mtx;



  namespace isa
  {

    static __aligned(64) RegularGridLookUpTables gridLookUpTables;

    __forceinline void createSubPatchBVH4iLeaf(BVH4i::NodeRef &ref,
					       const unsigned int patchIndex) 
    {
      *(volatile unsigned int*)&ref = (patchIndex << BVH4i::encodingBits) | BVH4i::leaf_mask;
    }

    BBox3fa createSubTree(BVH4i::NodeRef &curNode,
			  BVH4i *bvh,
			  const SubdivPatch1 &patch,
			  const float *const grid_u_array,
			  const float *const grid_v_array,
			  const unsigned int u_start,
			  const unsigned int u_end,
			  const unsigned int v_start,
			  const unsigned int v_end,
			  size_t &localCounter)
    {
      const unsigned int u_size = u_end-u_start+1;
      const unsigned int v_size = v_end-v_start+1;
      
      assert(u_size >= 1);
      assert(v_size >= 1);

      if (u_size <= 4 && v_size <= 4)
	{

	  assert(u_size*v_size <= 16);

	  const unsigned int currentIndex = localCounter;
	  localCounter += 2;

	  mic_f &leaf_u_array = bvh->lazymem[currentIndex+0];
	  mic_f &leaf_v_array = bvh->lazymem[currentIndex+1];

	  leaf_u_array = mic_f::inf();
	  leaf_v_array = mic_f::inf();

	  for (unsigned int v=v_start;v<=v_end;v++)
	    for (unsigned int u=u_start;u<=u_end;u++)
	      {
		const unsigned int local_v = v - v_start;
		const unsigned int local_u = u - u_start;
		leaf_u_array[4 * local_v + local_u] = grid_u_array[ v * patch.grid_u_res + u ];
		leaf_v_array[4 * local_v + local_u] = grid_v_array[ v * patch.grid_u_res + u ];
	      }

	  /* set invalid grid u,v value to border elements */

	  for (unsigned int y=0;y<4;y++)
	    for (unsigned int x=u_size-1;x<4;x++)
	      {
		leaf_u_array[4 * y + x] = leaf_u_array[4 * y + u_size-1];
		leaf_v_array[4 * y + x] = leaf_v_array[4 * y + u_size-1];
	      }

	  for (unsigned int x=0;x<4;x++)
	    for (unsigned int y=v_size-1;y<4;y++)
	      {
		leaf_u_array[4 * y + x] = leaf_u_array[4 * (v_size-1) + x];
		leaf_v_array[4 * y + x] = leaf_v_array[4 * (v_size-1) + x];
	      }
	  
	  const mic3f leafGridVtx = patch.eval16(leaf_u_array,leaf_v_array);

	  //const Vec3fa b_min( reduce_min(leafGridVtx.x), reduce_min(leafGridVtx.y), reduce_min(leafGridVtx.z) );
	  //const Vec3fa b_max( reduce_max(leafGridVtx.x), reduce_max(leafGridVtx.y), reduce_max(leafGridVtx.z) );

	  //const BBox3fa leafGridBounds( b_min, b_max );

	  const BBox3fa leafGridBounds = getBBox3fa(leafGridVtx, 0xffff);

	  createSubPatchBVH4iLeaf( curNode, currentIndex);
	  
	  return leafGridBounds;
	}

      /* allocate new bvh4i node */
      const size_t num64BytesBlocksPerNode = 2;
      const size_t currentIndex = localCounter;
      localCounter += num64BytesBlocksPerNode;

      createBVH4iNode<2>(curNode,currentIndex);

      BVH4i::Node &node = *(BVH4i::Node*)curNode.node((BVH4i::Node*)bvh->lazymem);

      node.setInvalid();

      const unsigned int u_mid = (u_start+u_end)/2;
      const unsigned int v_mid = (v_start+v_end)/2;


      const unsigned int subtree_u_start[4] = { u_start ,u_mid ,u_mid ,u_start };
      const unsigned int subtree_u_end  [4] = { u_mid   ,u_end ,u_end ,u_mid };

      const unsigned int subtree_v_start[4] = { v_start ,v_start ,v_mid ,v_mid};
      const unsigned int subtree_v_end  [4] = { v_mid   ,v_mid   ,v_end ,v_end };


      /* create four subtrees */
      BBox3fa bounds( empty );

      for (unsigned int i=0;i<4;i++)
	{
	  BBox3fa bounds_subtree = createSubTree( node.child(i), 
						  bvh, 
						  patch, 
						  grid_u_array,
						  grid_v_array,
						  subtree_u_start[i], 
						  subtree_u_end[i],
						  subtree_v_start[i],
						  subtree_v_end[i],
						  localCounter);
	  node.setBounds(i, bounds_subtree);
	  bounds.extend( bounds_subtree );
	}

      return bounds;
    }

    void initLazySubdivTree(SubdivPatch1 &patch,
			    BVH4i *bvh)
    {
      unsigned int build_state = atomic_add((volatile atomic_t*)&patch.under_construction,+1);
      if (build_state != 0)
	{
	  while(patch.bvh4i_subtree_root == BVH4i::invalidNode)
	    __pause(512);	  

	  atomic_add((volatile atomic_t*)&patch.under_construction,-1);
	}


      if (unlikely(patch.bvh4i_subtree_root != BVH4i::invalidNode)) return;

      TIMER(double msec = 0.0);
      TIMER(msec = getSeconds());

      assert( patch.grid_size_64b_blocks > 1 );
      __aligned(64) float u_array[(patch.grid_size_64b_blocks+1)*16]; // for unaligned access
      __aligned(64) float v_array[(patch.grid_size_64b_blocks+1)*16];

      gridUVTessellator(patch.level,
			patch.grid_u_res,
			patch.grid_v_res,
			u_array,
			v_array);

      BVH4i::NodeRef subtree_root = 0;
      size_t localCounter = bvh->lazyMemUsed64BytesBlocks.add( patch.grid_size_64b_blocks );
      const size_t oldCounter = localCounter;
      if (localCounter + patch.grid_size_64b_blocks > bvh->lazyMemAllocated64BytesBlocks) 
	{
	  DBG_PRINT(localCounter);
	  DBG_PRINT(bvh->lazyMemUsed64BytesBlocks);
	  FATAL("alloc");
	}

      BBox3fa bounds = createSubTree( subtree_root,
				      bvh,
				      patch,
				      u_array,
				      v_array,
				      0,
				      patch.grid_u_res-1,
				      0,
				      patch.grid_v_res-1,
				      localCounter);

      assert( localCounter - oldCounter == patch.grid_size_64b_blocks );
    TIMER(msec = getSeconds()-msec);    

#if 0 // DEBUG
      //numLazyBuildPatches++;
      //DBG_PRINT( numLazyBuildPatches );
      mtx.lock();
      DBG_PRINT( bvh->lazyMemUsed64BytesBlocks);
      DBG_PRINT( bvh->lazyMemAllocated64BytesBlocks );
      const size_t size_allocated_lazymem = bvh->lazyMemAllocated64BytesBlocks * sizeof(mic_f);
      const size_t size_used_lazymem      = bvh->lazyMemUsed64BytesBlocks * sizeof(mic_f);

      std::cout << "lazymem: " << 100.0f * size_used_lazymem / size_allocated_lazymem << "% used of " <<  size_allocated_lazymem << " bytes allocated" << std::endl;

      TIMER(std::cout << "build patch subtree in  " << 1000. * msec << " ms" << std::endl);

      mtx.unlock();
#endif
      /* build done */
      patch.bvh4i_subtree_root = subtree_root;

      /* release lock */
      atomic_add((volatile atomic_t*)&patch.under_construction,-1);
    }


    __aligned(64) float u_start[16] = { 0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0 };
    __aligned(64) float v_start[16] = { 0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1 };

    __forceinline bool intersect1Eval(const SubdivPatch1 &subdiv_patch,
				      const unsigned int subdiv_patch_index,
				      const float &u0,
				      const float &v0,
				      const float &u1,
				      const float &v1,
				      const float &u2,
				      const float &v2,
				      const float &u3,
				      const float &v3,
				      const size_t rayIndex, 
				      const mic_f &dir_xyz,
				      const mic_f &org_xyz,
				      Ray16& ray16)
    {
      __aligned(64) Vec3fa vtx[4];

      const mic_f uu = gather16f_4f_align(mic_f(u0),mic_f(u1),mic_f(u2),mic_f(u3));
      const mic_f vv = gather16f_4f_align(mic_f(v0),mic_f(v1),mic_f(v2),mic_f(v3));

      const mic_f _vtx = subdiv_patch.eval4(uu,vv);
      store16f(vtx,_vtx);

      return intersect1_quad(rayIndex, 
			     dir_xyz,
			     org_xyz,
			     ray16,
			     vtx[0],
			     vtx[1],
			     vtx[2],
			     vtx[3],
			     subdiv_patch_index);
    }


    __forceinline bool intersect1Eval16(const SubdivPatch1 &subdiv_patch,
					const unsigned int subdiv_patch_index,					
					const unsigned int grid_u_res,
					const unsigned int grid_v_res,
					const mic3f &vtx,
					const mic_f &u,
					const mic_f &v,
					const mic_m m_active,
					const size_t rayIndex, 
					const mic_f &dir_xyz,
					const mic_f &org_xyz,
					Ray16& ray16)
    {
      

      intersect1_quad16(rayIndex, 
			dir_xyz,
			org_xyz,
			ray16,
			vtx,
			u,
			v,
			grid_u_res,
			m_active,
			subdiv_patch_index);
      return true;
    }

    __noinline bool intersectEvalGrid1(const size_t rayIndex, 
				       const mic_f &dir_xyz,
				       const mic_f &org_xyz,
				       Ray16& ray16,
				       const SubdivPatch1& subdiv_patch,
				       const unsigned int subdiv_patch_index)
    {
      const float * const edge_levels = subdiv_patch.level;
      const unsigned int grid_u_res   = subdiv_patch.grid_u_res;
      const unsigned int grid_v_res   = subdiv_patch.grid_v_res;

      __aligned(64) float u_array[(subdiv_patch.grid_size_64b_blocks+1)]; // for unaligned access
      __aligned(64) float v_array[(subdiv_patch.grid_size_64b_blocks+1)];

      gridUVTessellator(edge_levels,grid_u_res,grid_v_res,u_array,v_array);

      bool hit = false;

      if (likely(subdiv_patch.grid_size_64b_blocks==1))
	{
	  const mic_m m_active = subdiv_patch.grid_mask;

	  const mic_f uu = load16f(u_array);
	  const mic_f vv = load16f(v_array);

	  const mic3f vtx = subdiv_patch.eval16(uu,vv);
	  	  
	  hit |= intersect1Eval16(subdiv_patch,
				  subdiv_patch_index,
				  grid_u_res,
				  grid_v_res,
				  vtx,
				  uu,
				  vv,
				  m_active,
				  rayIndex,
				  dir_xyz,
				  org_xyz,
				  ray16);	    	  
	}
      else
	{
	  FATAL("HERE");
	  size_t offset_line0 = 0;
	  size_t offset_line1 = grid_u_res;

#if 1
	  for (unsigned int y=0;y<grid_v_res-1;y++,offset_line0+=grid_u_res,offset_line1+=grid_u_res)
	    {
	      for (unsigned int x=0;x<grid_u_res-1;x+=7)
		{
		  const mic_f u8_line0 = uload16f(&u_array[offset_line0+x]);
		  const mic_f u8_line1 = uload16f(&u_array[offset_line1+x]);
		  const mic_f v8_line0 = uload16f(&v_array[offset_line0+x]);
		  const mic_f v8_line1 = uload16f(&v_array[offset_line1+x]);

		  const mic_f uu = select(0xff,u8_line0,align_shift_right<8>(u8_line1,u8_line1));
		  const mic_f vv = select(0xff,v8_line0,align_shift_right<8>(v8_line1,v8_line1));

		  __aligned(64) const mic3f vtx = subdiv_patch.eval16(uu,vv);
	  	  unsigned int m_active = 0x7f;
		  if (unlikely(x + 7 >= grid_u_res-1)) 
		    {
		      const unsigned int shift = x + 7 - (grid_u_res-1);
		      m_active >>= shift;
		    }

		  hit |= intersect1Eval16(subdiv_patch,
					  subdiv_patch_index,
					  8,
					  2,
					  vtx,
					  uu,
					  vv,
					  m_active,
					  rayIndex,
					  dir_xyz,
					  org_xyz,
					  ray16);	    	  

		}
	      
	    }	  
#else

	  /* 8x2 grid scan-line intersector */
	  for (unsigned int y=0;y<grid_v_res-1;y++,offset_line0++,offset_line1++)
	    for (unsigned int x=0;x<grid_u_res-1;x++,offset_line0++,offset_line1++)
	      {
		const float &u0 = u_array[offset_line0+0];
		const float &v0 = v_array[offset_line0+0];
		
		const float &u1 = u_array[offset_line0+1];
		const float &v1 = v_array[offset_line0+1];
		
		const float &u2 = u_array[offset_line1+1];
		const float &v2 = v_array[offset_line1+1];

		const float &u3 = u_array[offset_line1+0];
		const float &v3 = v_array[offset_line1+0];
		
		hit |= intersect1Eval(subdiv_patch,subdiv_patch_index,u0,v0,u1,v1,u2,v2,u3,v3,rayIndex,dir_xyz,org_xyz,ray16);	    
	      }
#endif
	}

      return hit;
    }

    

  template<bool ENABLE_INTERSECTION_FILTER>
    struct SubdivLeafIntersector
    {
      // ==================
      // === single ray === 
      // ==================
      static __forceinline bool intersect(BVH4i::NodeRef curNode,
					  const mic_f &dir_xyz,
					  const mic_f &org_xyz,
					  const mic_f &min_dist_xyz,
					  mic_f &max_dist_xyz,
					  Ray& ray, 
					  const void *__restrict__ const accel,
					  const Scene*__restrict__ const geometry)
      {
	unsigned int items = curNode.items();
	unsigned int index = curNode.offsetIndex();
	const SubdivPatch1 *__restrict__ const patch_ptr = (SubdivPatch1*)accel + index;
	FATAL("NOT IMPLEMENTED");

	// return SubdivPatchIntersector1<ENABLE_INTERSECTION_FILTER>::intersect1(dir_xyz,
	// 								       org_xyz,
	// 								       ray,
	// 								       *patch_ptr);	
      }

      static __forceinline bool occluded(BVH4i::NodeRef curNode,
					 const mic_f &dir_xyz,
					 const mic_f &org_xyz,
					 const mic_f &min_dist_xyz,
					 const mic_f &max_dist_xyz,
					 Ray& ray,
					 const void *__restrict__ const accel,
					 const Scene*__restrict__ const geometry)
      {
	unsigned int items = curNode.items();
	unsigned int index = curNode.offsetIndex();
	const SubdivPatch1 *__restrict__ const patch_ptr = (SubdivPatch1*)accel + index;
	FATAL("NOT IMPLEMENTED");
	// return SubdivPatchIntersector1<ENABLE_INTERSECTION_FILTER>::occluded1(dir_xyz,
	// 								      org_xyz,
	// 								      ray,
	// 								      *patch_ptr);	
      }


    };

    static unsigned int BVH4I_LEAF_MASK = BVH4i::leaf_mask; // needed due to compiler efficiency bug
    static unsigned int M_LANE_7777 = 0x7777;               // needed due to compiler efficiency bug

    // ============================================================================================
    // ============================================================================================
    // ============================================================================================

    template<typename LeafIntersector, bool ENABLE_COMPRESSED_BVH4I_NODES>
    void BVH4iIntersector16Subdiv<LeafIntersector,ENABLE_COMPRESSED_BVH4I_NODES>::intersect(mic_i* valid_i, BVH4i* bvh, Ray16& ray16)
    {
      /* near and node stack */
      __aligned(64) float   stack_dist[3*BVH4i::maxDepth+1];
      __aligned(64) NodeRef stack_node[3*BVH4i::maxDepth+1];


      /* setup */
      const mic_m m_valid    = *(mic_i*)valid_i != mic_i(0);
      const mic3f rdir16     = rcp_safe(ray16.dir);
      const mic_f inf        = mic_f(pos_inf);
      const mic_f zero       = mic_f::zero();

      store16f(stack_dist,inf);
      ray16.primID = select(m_valid,mic_i(-1),ray16.primID);
      ray16.geomID = select(m_valid,mic_i(-1),ray16.geomID);

      mic_f     * const __restrict__ lazymem     = bvh->lazymem;
      const Node      * __restrict__ const nodes = (Node     *)bvh->nodePtr();
      Triangle1 * __restrict__ const accel       = (Triangle1*)bvh->triPtr();

      stack_node[0] = BVH4i::invalidNode;
      long rayIndex = -1;
      while((rayIndex = bitscan64(rayIndex,toInt(m_valid))) != BITSCAN_NO_BIT_SET_64)	    
        {
	  stack_node[1] = bvh->root;
	  size_t sindex = 2;

	  const mic_f org_xyz      = loadAOS4to16f(rayIndex,ray16.org.x,ray16.org.y,ray16.org.z);
	  const mic_f dir_xyz      = loadAOS4to16f(rayIndex,ray16.dir.x,ray16.dir.y,ray16.dir.z);
	  const mic_f rdir_xyz     = loadAOS4to16f(rayIndex,rdir16.x,rdir16.y,rdir16.z);
	  const mic_f org_rdir_xyz = org_xyz * rdir_xyz;
	  const mic_f min_dist_xyz = broadcast1to16f(&ray16.tnear[rayIndex]);
	  mic_f       max_dist_xyz = broadcast1to16f(&ray16.tfar[rayIndex]);

	  const unsigned int leaf_mask = BVH4I_LEAF_MASK;

	  while (1)
	    {

	      NodeRef curNode = stack_node[sindex-1];
	      sindex--;

	      traverse_single_intersect<ENABLE_COMPRESSED_BVH4I_NODES>(curNode,
								      sindex,
								      rdir_xyz,
								      org_rdir_xyz,
								      min_dist_xyz,
								      max_dist_xyz,
								      stack_node,
								      stack_dist,
								      nodes,
								      leaf_mask);
		   

	      /* return if stack is empty */
	      if (unlikely(curNode == BVH4i::invalidNode)) break;

	      STAT3(normal.trav_leaves,1,1,1);
	      STAT3(normal.trav_prims,4,4,4);

	      //////////////////////////////////////////////////////////////////////////////////////////////////

	      const unsigned int subdiv_level = 2; 
	      const unsigned int patchIndex = curNode.offsetIndex();
	      SubdivPatch1& subdiv_patch = ((SubdivPatch1*)accel)[patchIndex];

	      /* fast patch for grid with <= 16 points */
	      if (likely(subdiv_patch.grid_size_64b_blocks == 1))
		intersectEvalGrid1(rayIndex,
				   dir_xyz,
				   org_xyz,
				   ray16,
				   subdiv_patch,
				   patchIndex);
	      else
		{
		  /* traverse sub-patch bvh4i for grids with > 16 points */

		  if (unlikely(subdiv_patch.bvh4i_subtree_root == BVH4i::invalidNode))
		    {
		      /* build sub-patch bvh4i */
		      initLazySubdivTree(subdiv_patch,bvh);
		    }

		  assert(subdiv_patch.bvh4i_subtree_root != BVH4i::invalidNode);

		  // -------------------------------------
		  // -------------------------------------
		  // -------------------------------------

		  __aligned(64) float   sub_stack_dist[64];
		  __aligned(64) NodeRef sub_stack_node[64];
		  sub_stack_node[0] = BVH4i::invalidNode;
		  sub_stack_node[1] = subdiv_patch.bvh4i_subtree_root;
		  store16f(sub_stack_dist,inf);
		  size_t sub_sindex = 2;

		  while (1)
		    {
		      curNode = sub_stack_node[sub_sindex-1];
		      sub_sindex--;

		      traverse_single_intersect<ENABLE_COMPRESSED_BVH4I_NODES>(curNode,
									       sub_sindex,
									       rdir_xyz,
									       org_rdir_xyz,
									       min_dist_xyz,
									       max_dist_xyz,
									       sub_stack_node,
									       sub_stack_dist,
									       (BVH4i::Node*)lazymem,
									       leaf_mask);
		 		   

		      /* return if stack is empty */
		      if (unlikely(curNode == BVH4i::invalidNode)) break;

		      const unsigned int uvIndex = curNode.offsetIndex();
		      prefetch<PFHINT_NT>(&lazymem[uvIndex + 0]);
		      prefetch<PFHINT_NT>(&lazymem[uvIndex + 1]);

		  
		      const mic_m m_active = 0x777;
		      const mic_f &uu = lazymem[uvIndex + 0];
		      const mic_f &vv = lazymem[uvIndex + 1];		  
		      const mic3f vtx = subdiv_patch.eval16(uu,vv);
		  
		      intersect1_quad16(rayIndex, 
					dir_xyz,
					org_xyz,
					ray16,
					vtx,
					uu,
					vv,
					4,
					m_active,
					patchIndex);
		    }
		}
	      // -------------------------------------
	      // -------------------------------------
	      // -------------------------------------

	      compactStack(stack_node,stack_dist,sindex,mic_f(ray16.tfar[rayIndex]));

	      // ------------------------
	    }
	}

#if 0
      DBG_PRINT( ray16.primID );
      DBG_PRINT( ray16.geomID );
      DBG_PRINT( ray16.u );
      DBG_PRINT( ray16.v );
      DBG_PRINT( ray16.Ng.x );
      DBG_PRINT( ray16.Ng.y );
      DBG_PRINT( ray16.Ng.z );
      exit(0);
#endif

      /* update primID/geomID and compute normals */
#if 1
      // FIXME: test all rays with the same primID
      mic_m m_hit = (ray16.primID != -1) & m_valid;
      rayIndex = -1;
      while((rayIndex = bitscan64(rayIndex,toInt(m_hit))) != BITSCAN_NO_BIT_SET_64)	    
        {
	  const SubdivPatch1& subdiv_patch = ((SubdivPatch1*)accel)[ray16.primID[rayIndex]];
	  ray16.primID[rayIndex] = subdiv_patch.primID;
	  ray16.geomID[rayIndex] = subdiv_patch.geomID;
	  ray16.u[rayIndex]      = (1.0f-ray16.u[rayIndex]) * subdiv_patch.u_range.x + ray16.u[rayIndex] * subdiv_patch.u_range.y;
	  ray16.v[rayIndex]      = (1.0f-ray16.v[rayIndex]) * subdiv_patch.v_range.x + ray16.v[rayIndex] * subdiv_patch.v_range.y;
	  const Vec3fa normal    = subdiv_patch.normal(ray16.u[rayIndex],ray16.v[rayIndex]);
	  ray16.Ng.x[rayIndex]   = normal.x;
	  ray16.Ng.y[rayIndex]   = normal.y;
	  ray16.Ng.z[rayIndex]   = normal.z;

	}
#endif

    }

    template<typename LeafIntersector,bool ENABLE_COMPRESSED_BVH4I_NODES>    
    void BVH4iIntersector16Subdiv<LeafIntersector,ENABLE_COMPRESSED_BVH4I_NODES>::occluded(mic_i* valid_i, BVH4i* bvh, Ray16& ray16)
    {
      /* near and node stack */
      __aligned(64) NodeRef stack_node[3*BVH4i::maxDepth+1];

      /* setup */
      const mic_m m_valid = *(mic_i*)valid_i != mic_i(0);
      const mic3f rdir16  = rcp_safe(ray16.dir);
      mic_m terminated    = !m_valid;
      const mic_f inf     = mic_f(pos_inf);
      const mic_f zero    = mic_f::zero();

      const Node      * __restrict__ nodes = (Node     *)bvh->nodePtr();
      const Triangle1 * __restrict__ accel = (Triangle1*)bvh->triPtr();

      stack_node[0] = BVH4i::invalidNode;

      long rayIndex = -1;
      while((rayIndex = bitscan64(rayIndex,toInt(m_valid))) != BITSCAN_NO_BIT_SET_64)	    
        {
	  stack_node[1] = bvh->root;
	  size_t sindex = 2;

	  const mic_f org_xyz      = loadAOS4to16f(rayIndex,ray16.org.x,ray16.org.y,ray16.org.z);
	  const mic_f dir_xyz      = loadAOS4to16f(rayIndex,ray16.dir.x,ray16.dir.y,ray16.dir.z);
	  const mic_f rdir_xyz     = loadAOS4to16f(rayIndex,rdir16.x,rdir16.y,rdir16.z);
	  const mic_f org_rdir_xyz = org_xyz * rdir_xyz;
	  const mic_f min_dist_xyz = broadcast1to16f(&ray16.tnear[rayIndex]);
	  const mic_f max_dist_xyz = broadcast1to16f(&ray16.tfar[rayIndex]);
	  const mic_i v_invalidNode(BVH4i::invalidNode);
	  const unsigned int leaf_mask = BVH4I_LEAF_MASK;

	  while (1)
	    {
	      NodeRef curNode = stack_node[sindex-1];
	      sindex--;

	      traverse_single_occluded< ENABLE_COMPRESSED_BVH4I_NODES >(curNode,
								       sindex,
								       rdir_xyz,
								       org_rdir_xyz,
								       min_dist_xyz,
								       max_dist_xyz,
								       stack_node,
								       nodes,
								       leaf_mask);

	      /* return if stack is empty */
	      if (unlikely(curNode == BVH4i::invalidNode)) break;

	      STAT3(shadow.trav_leaves,1,1,1);
	      STAT3(shadow.trav_prims,4,4,4);

	      /* intersect one ray against four triangles */

	      //////////////////////////////////////////////////////////////////////////////////////////////////

	      const bool hit = false;

	      FATAL("NOT YET IMPLEMENTED");

	      if (unlikely(hit)) break;
	      //////////////////////////////////////////////////////////////////////////////////////////////////

	    }


	  if (unlikely(all(toMask(terminated)))) break;
	}


      store16i(m_valid & toMask(terminated),&ray16.geomID,0);

    }

    template<typename LeafIntersector, bool ENABLE_COMPRESSED_BVH4I_NODES>
    void BVH4iIntersector1Subdiv<LeafIntersector,ENABLE_COMPRESSED_BVH4I_NODES>::intersect(BVH4i* bvh, Ray& ray)
    {
      /* near and node stack */
      __aligned(64) float   stack_dist[3*BVH4i::maxDepth+1];
      __aligned(64) NodeRef stack_node[3*BVH4i::maxDepth+1];

      /* setup */
      //const mic_m m_valid    = *(mic_i*)valid_i != mic_i(0);
      const mic3f rdir16     = rcp_safe(mic3f(mic_f(ray.dir.x),mic_f(ray.dir.y),mic_f(ray.dir.z)));
      const mic_f inf        = mic_f(pos_inf);
      const mic_f zero       = mic_f::zero();

      store16f(stack_dist,inf);

      const Node      * __restrict__ nodes = (Node    *)bvh->nodePtr();
      const Triangle1 * __restrict__ accel = (Triangle1*)bvh->triPtr();

      stack_node[0] = BVH4i::invalidNode;      
      stack_node[1] = bvh->root;

      size_t sindex = 2;

      const mic_f org_xyz      = loadAOS4to16f(ray.org.x,ray.org.y,ray.org.z);
      const mic_f dir_xyz      = loadAOS4to16f(ray.dir.x,ray.dir.y,ray.dir.z);
      const mic_f rdir_xyz     = loadAOS4to16f(rdir16.x[0],rdir16.y[0],rdir16.z[0]);
      const mic_f org_rdir_xyz = org_xyz * rdir_xyz;
      const mic_f min_dist_xyz = broadcast1to16f(&ray.tnear);
      mic_f       max_dist_xyz = broadcast1to16f(&ray.tfar);
	  
      const unsigned int leaf_mask = BVH4I_LEAF_MASK;
	  
      while (1)
	{
	  NodeRef curNode = stack_node[sindex-1];
	  sindex--;

	  traverse_single_intersect<ENABLE_COMPRESSED_BVH4I_NODES>(curNode,
								   sindex,
								   rdir_xyz,
								   org_rdir_xyz,
								   min_dist_xyz,
								   max_dist_xyz,
								   stack_node,
								   stack_dist,
								   nodes,
								   leaf_mask);            		    

	  /* return if stack is empty */
	  if (unlikely(curNode == BVH4i::invalidNode)) break;


	  /* intersect one ray against four triangles */

	  //////////////////////////////////////////////////////////////////////////////////////////////////

	  bool hit = LeafIntersector::intersect(curNode,
						dir_xyz,
						org_xyz,
						min_dist_xyz,
						max_dist_xyz,
						ray,
						accel,
						(Scene*)bvh->geometry);
	  if (hit)
	    compactStack(stack_node,stack_dist,sindex,max_dist_xyz);
	}
    }

    template<typename LeafIntersector, bool ENABLE_COMPRESSED_BVH4I_NODES>
    void BVH4iIntersector1Subdiv<LeafIntersector,ENABLE_COMPRESSED_BVH4I_NODES>::occluded(BVH4i* bvh, Ray& ray)
    {
      /* near and node stack */
      __aligned(64) NodeRef stack_node[3*BVH4i::maxDepth+1];

      /* setup */
      const mic3f rdir16      = rcp_safe(mic3f(ray.dir.x,ray.dir.y,ray.dir.z));
      const mic_f inf         = mic_f(pos_inf);
      const mic_f zero        = mic_f::zero();

      const Node      * __restrict__ nodes = (Node     *)bvh->nodePtr();
      const Triangle1 * __restrict__ accel = (Triangle1*)bvh->triPtr();

      stack_node[0] = BVH4i::invalidNode;
      stack_node[1] = bvh->root;
      size_t sindex = 2;

      const mic_f org_xyz      = loadAOS4to16f(ray.org.x,ray.org.y,ray.org.z);
      const mic_f dir_xyz      = loadAOS4to16f(ray.dir.x,ray.dir.y,ray.dir.z);
      const mic_f rdir_xyz     = loadAOS4to16f(rdir16.x[0],rdir16.y[0],rdir16.z[0]);
      const mic_f org_rdir_xyz = org_xyz * rdir_xyz;
      const mic_f min_dist_xyz = broadcast1to16f(&ray.tnear);
      const mic_f max_dist_xyz = broadcast1to16f(&ray.tfar);

      const unsigned int leaf_mask = BVH4I_LEAF_MASK;
	  
      while (1)
	{
	  NodeRef curNode = stack_node[sindex-1];
	  sindex--;
            
	  
	  traverse_single_occluded< ENABLE_COMPRESSED_BVH4I_NODES>(curNode,
								   sindex,
								   rdir_xyz,
								   org_rdir_xyz,
								   min_dist_xyz,
								   max_dist_xyz,
								   stack_node,
								   nodes,
								   leaf_mask);	    

	  /* return if stack is empty */
	  if (unlikely(curNode == BVH4i::invalidNode)) break;


	  /* intersect one ray against four triangles */

	  //////////////////////////////////////////////////////////////////////////////////////////////////

	  bool hit = LeafIntersector::occluded(curNode,
					       dir_xyz,
					       org_xyz,
					       min_dist_xyz,
					       max_dist_xyz,
					       ray,
					       accel,
					       (Scene*)bvh->geometry);

	  if (unlikely(hit))
	    {
	      ray.geomID = 0;
	      return;
	    }
	  //////////////////////////////////////////////////////////////////////////////////////////////////

	}
    }


    // ----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------------------------------------



    typedef BVH4iIntersector16Subdiv< SubdivLeafIntersector    < true  >, false > SubdivIntersector16SingleMoellerFilter;
    typedef BVH4iIntersector16Subdiv< SubdivLeafIntersector    < false >, false > SubdivIntersector16SingleMoellerNoFilter;

    DEFINE_INTERSECTOR16   (BVH4iSubdivMeshIntersector16        , SubdivIntersector16SingleMoellerFilter);
    DEFINE_INTERSECTOR16   (BVH4iSubdivMeshIntersector16NoFilter, SubdivIntersector16SingleMoellerNoFilter);

    typedef BVH4iIntersector1Subdiv< SubdivLeafIntersector    < true  >, false > SubdivMeshIntersector1MoellerFilter;
    typedef BVH4iIntersector1Subdiv< SubdivLeafIntersector    < false >, false > SubdivMeshIntersector1MoellerNoFilter;

    DEFINE_INTERSECTOR1    (BVH4iSubdivMeshIntersector1        , SubdivMeshIntersector1MoellerFilter);
    DEFINE_INTERSECTOR1    (BVH4iSubdivMeshIntersector1NoFilter, SubdivMeshIntersector1MoellerNoFilter);

  }
}
