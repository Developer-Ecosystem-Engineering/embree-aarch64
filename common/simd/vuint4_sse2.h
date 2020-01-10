// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
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

#pragma once

#include "../math/math.h"

namespace embree
{
  /* 4-wide SSE integer type */
  template<>
  struct vuint<4>
  {
    ALIGNED_STRUCT_(16);
    
    typedef vboolf4 Bool;
    typedef vuint4   Int;
    typedef vfloat4 Float;

    enum  { size = 4 }; // number of SIMD elements
    union { __m128i v; unsigned int i[4]; }; // data

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////
    
    __forceinline vuint() {}
    __forceinline vuint(const vuint4& a) { v = a.v; }
    __forceinline vuint4& operator =(const vuint4& a) { v = a.v; return *this; }

    __forceinline vuint(const __m128i a) : v(a) {}
    __forceinline operator const __m128i&() const { return v; }
    __forceinline operator       __m128i&()       { return v; }


    __forceinline vuint(unsigned int a) : v(_mm_set1_epi32(a)) {}
    __forceinline vuint(unsigned int a, unsigned int b, unsigned int c, unsigned int d) : v(_mm_set_epi32(d, c, b, a)) {}

#if defined(__AVX512VL__)
    __forceinline explicit vuint(__m128 a) : v(_mm_cvtps_epu32(a)) {}
#endif

#if defined(__AVX512VL__)
    __forceinline explicit vuint(const vboolf4& a) : v(_mm_movm_epi32(a)) {}
#else
    __forceinline explicit vuint(const vboolf4& a) : v(_mm_castps_si128((__m128)a)) {}
#endif

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    __forceinline vuint(ZeroTy)   : v(_mm_setzero_si128()) {}
    __forceinline vuint(OneTy)    : v(_mm_set1_epi32(1)) {}
    __forceinline vuint(PosInfTy) : v(_mm_set1_epi32(unsigned(pos_inf))) {}
    __forceinline vuint(StepTy)   : v(_mm_set_epi32(3, 2, 1, 0)) {}
    __forceinline vuint(TrueTy)   { v = _mm_cmpeq_epi32(v,v); }
    __forceinline vuint(UndefinedTy) : v(_mm_castps_si128(_mm_undefined_ps())) {}

    ////////////////////////////////////////////////////////////////////////////////
    /// Loads and Stores
    ////////////////////////////////////////////////////////////////////////////////

    static __forceinline vuint4 load (const void* a) { return _mm_load_si128((__m128i*)a); }
    static __forceinline vuint4 loadu(const void* a) { return _mm_loadu_si128((__m128i*)a); }

    static __forceinline void store (void* ptr, const vuint4& v) { _mm_store_si128((__m128i*)ptr,v); }
    static __forceinline void storeu(void* ptr, const vuint4& v) { _mm_storeu_si128((__m128i*)ptr,v); }
    
#if defined(__AVX512VL__)
    static __forceinline vuint4 load (const vboolf4& mask, const void* ptr) { return _mm_mask_load_epi32 (_mm_setzero_si128(),mask,ptr); }
    static __forceinline vuint4 loadu(const vboolf4& mask, const void* ptr) { return _mm_mask_loadu_epi32(_mm_setzero_si128(),mask,ptr); }

    static __forceinline void store (const vboolf4& mask, void* ptr, const vuint4& v) { _mm_mask_store_epi32 (ptr,mask,v); }
    static __forceinline void storeu(const vboolf4& mask, void* ptr, const vuint4& v) { _mm_mask_storeu_epi32(ptr,mask,v); }
#elif defined(__AVX__)
    static __forceinline vuint4 load (const vbool4& mask, const void* a) { return _mm_castps_si128(_mm_maskload_ps((float*)a,mask)); }
    static __forceinline vuint4 loadu(const vbool4& mask, const void* a) { return _mm_castps_si128(_mm_maskload_ps((float*)a,mask)); }

    static __forceinline void store (const vboolf4& mask, void* ptr, const vuint4& i) { _mm_maskstore_ps((float*)ptr,(__m128i)mask,_mm_castsi128_ps(i)); }
    static __forceinline void storeu(const vboolf4& mask, void* ptr, const vuint4& i) { _mm_maskstore_ps((float*)ptr,(__m128i)mask,_mm_castsi128_ps(i)); }
#else
    static __forceinline vuint4 load (const vbool4& mask, const void* a) { return _mm_and_si128(_mm_load_si128 ((__m128i*)a),mask); }
    static __forceinline vuint4 loadu(const vbool4& mask, const void* a) { return _mm_and_si128(_mm_loadu_si128((__m128i*)a),mask); }

    static __forceinline void store (const vboolf4& mask, void* ptr, const vuint4& i) { store (ptr,select(mask,i,load (ptr))); }
    static __forceinline void storeu(const vboolf4& mask, void* ptr, const vuint4& i) { storeu(ptr,select(mask,i,loadu(ptr))); }
#endif

#if defined(__aarch64__) && defined(BUILD_IOS)
    static __forceinline vuint4 load(const uint8_t* ptr) {
        return _mm_load4epu8_epi32(((__m128i*)ptr));
    }
    static __forceinline vuint4 loadu(const uint8_t* ptr) {
        return _mm_load4epu8_epi32(((__m128i*)ptr));
    }
#elif defined(__SSE4_1__)
    static __forceinline vuint4 load(const uint8_t* ptr) {
      return _mm_cvtepu8_epi32(_mm_loadl_epi64((__m128i*)ptr));
    }

    static __forceinline vuint4 loadu(const uint8_t* ptr) {
      return  _mm_cvtepu8_epi32(_mm_loadl_epi64((__m128i*)ptr));
    }

#endif

    static __forceinline vuint4 load(const unsigned short* ptr) {
#if defined(__aarch64__) && defined(BUILD_IOS)
      return _mm_load4epu16_epi32(((__m128i*)ptr));
#elif defined (__SSE4_1__)
      return _mm_cvtepu16_epi32(_mm_loadu_si128((__m128i*)ptr));
#else
      return vuint4(ptr[0],ptr[1],ptr[2],ptr[3]);
#endif
    } 

    static __forceinline void store_uint8(uint8_t* ptr, const vuint4& v) {
#if defined(__aarch64__) && defined(BUILD_IOS)
        uint32x4_t x = v;
        uint16x4_t y = vqmovn_u32(x);
        uint8x8_t z = vqmovn_u16(vcombine_u16(y, y));
        vst1_lane_u32(ptr, z, 0);
#elif defined(__SSE4_1__)
      __m128i x = v;
      x = _mm_packus_epi32(x, x);
      x = _mm_packus_epi16(x, x);
      *(unsigned*)ptr = _mm_cvtsi128_si32(x);
#else
      for (size_t i=0;i<4;i++)
        ptr[i] = (uint8_t)v[i];
#endif
    }

    static __forceinline void store_uint8(unsigned short* ptr, const vuint4& v) {
#if defined(__aarch64__) && defined(BUILD_IOS)
        int32x4_t x = v;
        int16x4_t y = vqmovn_u32(x);
        vst1_u16(ptr, y);
#else
      for (size_t i=0;i<4;i++)
        ptr[i] = (unsigned short)v[i];
#endif
    }

    static __forceinline vuint4 load_nt(void* ptr) {
#if (defined(__aarch64__) && defined(BUILD_IOS)) || defined(__SSE4_1__)
      return _mm_stream_load_si128((__m128i*)ptr); 
#else
      return _mm_load_si128((__m128i*)ptr); 
#endif
    }
    
    static __forceinline void store_nt(void* ptr, const vuint4& v) {
#if (defined(__aarch64__) && defined(BUILD_IOS)) || defined(__SSE4_1__)
      _mm_stream_ps((float*)ptr,_mm_castsi128_ps(v)); 
#else
      _mm_store_si128((__m128i*)ptr,v);
#endif
    }

    template<int scale = 4>
    static __forceinline vuint4 gather(const unsigned int* ptr, const vint4& index) {
#if defined(__AVX2__)
      return _mm_i32gather_epi32((const int*)ptr, index, scale);
#else
      return vuint4(
          *(unsigned int*)(((int8_t*)ptr)+scale*index[0]),
          *(unsigned int*)(((int8_t*)ptr)+scale*index[1]),
          *(unsigned int*)(((int8_t*)ptr)+scale*index[2]),
          *(unsigned int*)(((int8_t*)ptr)+scale*index[3]));
#endif
    }

    template<int scale = 4>
    static __forceinline vuint4 gather(const vboolf4& mask, const unsigned int* ptr, const vint4& index) {
      vuint4 r = zero;
#if defined(__AVX512VL__)
      return _mm_mmask_i32gather_epi32(r, mask, index, ptr, scale);
#elif defined(__AVX2__)
      return _mm_mask_i32gather_epi32(r, (const int*)ptr, index, mask, scale);
#else
      if (likely(mask[0])) r[0] = *(unsigned int*)(((int8_t*)ptr)+scale*index[0]);
      if (likely(mask[1])) r[1] = *(unsigned int*)(((int8_t*)ptr)+scale*index[1]);
      if (likely(mask[2])) r[2] = *(unsigned int*)(((int8_t*)ptr)+scale*index[2]);
      if (likely(mask[3])) r[3] = *(unsigned int*)(((int8_t*)ptr)+scale*index[3]);
      return r;
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// Array Access
    ////////////////////////////////////////////////////////////////////////////////

    __forceinline const unsigned int& operator [](size_t index) const { assert(index < 4); return i[index]; }
    __forceinline       unsigned int& operator [](size_t index)       { assert(index < 4); return i[index]; }

    friend __forceinline vuint4 select(const vboolf4& m, const vuint4& t, const vuint4& f) {
#if defined(__AVX512VL__)
      return _mm_mask_blend_epi32(m, (__m128i)f, (__m128i)t);
#elif defined(__SSE4_1__)
      return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(f), _mm_castsi128_ps(t), m)); 
#else
      return _mm_or_si128(_mm_and_si128(m, t), _mm_andnot_si128(m, f)); 
#endif
    }
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

#if defined(__AVX512VL__)
  __forceinline vboolf4 asBool(const vuint4& a) { return _mm_movepi32_mask(a); }
#else
  __forceinline vboolf4 asBool(const vuint4& a) { return _mm_castsi128_ps(a); }
#endif

  __forceinline vuint4 operator +(const vuint4& a) { return a; }
  __forceinline vuint4 operator -(const vuint4& a) { return _mm_sub_epi32(_mm_setzero_si128(), a); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vuint4 operator +(const vuint4& a, const vuint4& b) { return _mm_add_epi32(a, b); }
  __forceinline vuint4 operator +(const vuint4& a, unsigned int  b) { return a + vuint4(b); }
  __forceinline vuint4 operator +(unsigned int  a, const vuint4& b) { return vuint4(a) + b; }

  __forceinline vuint4 operator -(const vuint4& a, const vuint4& b) { return _mm_sub_epi32(a, b); }
  __forceinline vuint4 operator -(const vuint4& a, unsigned int  b) { return a - vuint4(b); }
  __forceinline vuint4 operator -(unsigned int  a, const vuint4& b) { return vuint4(a) - b; }

//#if defined(__SSE4_1__)
//  __forceinline vuint4 operator *(const vuint4& a, const vuint4& b) { return _mm_mullo_epu32(a, b); }
//#else
//  __forceinline vuint4 operator *(const vuint4& a, const vuint4& b) { return vuint4(a[0]*b[0],a[1]*b[1],a[2]*b[2],a[3]*b[3]); }
//#endif
//  __forceinline vuint4 operator *(const vuint4& a, unsigned int  b) { return a * vuint4(b); }
//  __forceinline vuint4 operator *(unsigned int  a, const vuint4& b) { return vuint4(a) * b; }

  __forceinline vuint4 operator &(const vuint4& a, const vuint4& b) { return _mm_and_si128(a, b); }
  __forceinline vuint4 operator &(const vuint4& a, unsigned int  b) { return a & vuint4(b); }
  __forceinline vuint4 operator &(unsigned int  a, const vuint4& b) { return vuint4(a) & b; }

  __forceinline vuint4 operator |(const vuint4& a, const vuint4& b) { return _mm_or_si128(a, b); }
  __forceinline vuint4 operator |(const vuint4& a, unsigned int  b) { return a | vuint4(b); }
  __forceinline vuint4 operator |(unsigned int  a, const vuint4& b) { return vuint4(a) | b; }

  __forceinline vuint4 operator ^(const vuint4& a, const vuint4& b) { return _mm_xor_si128(a, b); }
  __forceinline vuint4 operator ^(const vuint4& a, unsigned int  b) { return a ^ vuint4(b); }
  __forceinline vuint4 operator ^(unsigned int  a, const vuint4& b) { return vuint4(a) ^ b; }

  __forceinline vuint4 operator <<(const vuint4& a, unsigned int n) { return _mm_slli_epi32(a, n); }
  __forceinline vuint4 operator >>(const vuint4& a, unsigned int n) { return _mm_srli_epi32(a, n); }

  __forceinline vuint4 sll (const vuint4& a, unsigned int b) { return _mm_slli_epi32(a, b); }
  __forceinline vuint4 sra (const vuint4& a, unsigned int b) { return _mm_srai_epi32(a, b); }
  __forceinline vuint4 srl (const vuint4& a, unsigned int b) { return _mm_srli_epi32(a, b); }
  
  ////////////////////////////////////////////////////////////////////////////////
  /// Assignment Operators
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vuint4& operator +=(vuint4& a, const vuint4& b) { return a = a + b; }
  __forceinline vuint4& operator +=(vuint4& a, unsigned int  b) { return a = a + b; }
  
  __forceinline vuint4& operator -=(vuint4& a, const vuint4& b) { return a = a - b; }
  __forceinline vuint4& operator -=(vuint4& a, unsigned int  b) { return a = a - b; }

//#if defined(__SSE4_1__)
//  __forceinline vuint4& operator *=(vuint4& a, const vuint4& b) { return a = a * b; }
//  __forceinline vuint4& operator *=(vuint4& a, unsigned int  b) { return a = a * b; }
//#endif
  
  __forceinline vuint4& operator &=(vuint4& a, const vuint4& b) { return a = a & b; }
  __forceinline vuint4& operator &=(vuint4& a, unsigned int  b) { return a = a & b; }
  
  __forceinline vuint4& operator |=(vuint4& a, const vuint4& b) { return a = a | b; }
  __forceinline vuint4& operator |=(vuint4& a, unsigned int  b) { return a = a | b; }
  
  __forceinline vuint4& operator <<=(vuint4& a, unsigned int  b) { return a = a << b; }
  __forceinline vuint4& operator >>=(vuint4& a, unsigned int  b) { return a = a >> b; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators + Select
  ////////////////////////////////////////////////////////////////////////////////

#if defined(__AVX512VL__)
  __forceinline vboolf4 operator ==(const vuint4& a, const vuint4& b) { return _mm_cmp_epu32_mask(a,b,_MM_CMPINT_EQ); }
  __forceinline vboolf4 operator !=(const vuint4& a, const vuint4& b) { return _mm_cmp_epu32_mask(a,b,_MM_CMPINT_NE); }
  //__forceinline vboolf4 operator < (const vuint4& a, const vuint4& b) { return _mm_cmp_epu32_mask(a,b,_MM_CMPINT_LT); }
  //__forceinline vboolf4 operator >=(const vuint4& a, const vuint4& b) { return _mm_cmp_epu32_mask(a,b,_MM_CMPINT_GE); }
  //__forceinline vboolf4 operator > (const vuint4& a, const vuint4& b) { return _mm_cmp_epu32_mask(a,b,_MM_CMPINT_GT); }
  //__forceinline vboolf4 operator <=(const vuint4& a, const vuint4& b) { return _mm_cmp_epu32_mask(a,b,_MM_CMPINT_LE); }
#else
  __forceinline vboolf4 operator ==(const vuint4& a, const vuint4& b) { return _mm_castsi128_ps(_mm_cmpeq_epi32(a, b)); }
  __forceinline vboolf4 operator !=(const vuint4& a, const vuint4& b) { return !(a == b); }
  //__forceinline vboolf4 operator < (const vuint4& a, const vuint4& b) { return _mm_castsi128_ps(_mm_cmplt_epu32(a, b)); }
  //__forceinline vboolf4 operator >=(const vuint4& a, const vuint4& b) { return !(a <  b); }
  //__forceinline vboolf4 operator > (const vuint4& a, const vuint4& b) { return _mm_castsi128_ps(_mm_cmpgt_epu32(a, b)); }
  //__forceinline vboolf4 operator <=(const vuint4& a, const vuint4& b) { return !(a >  b); }
#endif

  __forceinline vboolf4 operator ==(const vuint4& a, unsigned int  b) { return a == vuint4(b); }
  __forceinline vboolf4 operator ==(unsigned int  a, const vuint4& b) { return vuint4(a) == b; }

  __forceinline vboolf4 operator !=(const vuint4& a, unsigned int  b) { return a != vuint4(b); }
  __forceinline vboolf4 operator !=(unsigned int  a, const vuint4& b) { return vuint4(a) != b; }

  //__forceinline vboolf4 operator < (const vuint4& a, unsigned int  b) { return a <  vuint4(b); }
  //__forceinline vboolf4 operator < (unsigned int  a, const vuint4& b) { return vuint4(a) <  b; }

  //__forceinline vboolf4 operator >=(const vuint4& a, unsigned int  b) { return a >= vuint4(b); }
  //__forceinline vboolf4 operator >=(unsigned int  a, const vuint4& b) { return vuint4(a) >= b; }

  //__forceinline vboolf4 operator > (const vuint4& a, unsigned int  b) { return a >  vuint4(b); }
  //__forceinline vboolf4 operator > (unsigned int  a, const vuint4& b) { return vuint4(a) >  b; }

  //__forceinline vboolf4 operator <=(const vuint4& a, unsigned int  b) { return a <= vuint4(b); }
  //__forceinline vboolf4 operator <=(unsigned int  a, const vuint4& b) { return vuint4(a) <= b; }

  __forceinline vboolf4 eq(const vuint4& a, const vuint4& b) { return a == b; }
  __forceinline vboolf4 ne(const vuint4& a, const vuint4& b) { return a != b; }
  //__forceinline vboolf4 lt(const vuint4& a, const vuint4& b) { return a <  b; }
  //__forceinline vboolf4 ge(const vuint4& a, const vuint4& b) { return a >= b; }
  //__forceinline vboolf4 gt(const vuint4& a, const vuint4& b) { return a >  b; }
  //__forceinline vboolf4 le(const vuint4& a, const vuint4& b) { return a <= b; }

#if defined(__AVX512VL__)
  __forceinline vboolf4 eq(const vboolf4& mask, const vuint4& a, const vuint4& b) { return _mm_mask_cmp_epu32_mask(mask, a, b, _MM_CMPINT_EQ); }
  __forceinline vboolf4 ne(const vboolf4& mask, const vuint4& a, const vuint4& b) { return _mm_mask_cmp_epu32_mask(mask, a, b, _MM_CMPINT_NE); }
  //__forceinline vboolf4 lt(const vboolf4& mask, const vuint4& a, const vuint4& b) { return _mm_mask_cmp_epu32_mask(mask, a, b, _MM_CMPINT_LT); }
  //__forceinline vboolf4 ge(const vboolf4& mask, const vuint4& a, const vuint4& b) { return _mm_mask_cmp_epu32_mask(mask, a, b, _MM_CMPINT_GE); }
  //__forceinline vboolf4 gt(const vboolf4& mask, const vuint4& a, const vuint4& b) { return _mm_mask_cmp_epu32_mask(mask, a, b, _MM_CMPINT_GT); }
  //__forceinline vboolf4 le(const vboolf4& mask, const vuint4& a, const vuint4& b) { return _mm_mask_cmp_epu32_mask(mask, a, b, _MM_CMPINT_LE); }
#else
  __forceinline vboolf4 eq(const vboolf4& mask, const vuint4& a, const vuint4& b) { return mask & (a == b); }
  __forceinline vboolf4 ne(const vboolf4& mask, const vuint4& a, const vuint4& b) { return mask & (a != b); }
  //__forceinline vboolf4 lt(const vboolf4& mask, const vuint4& a, const vuint4& b) { return mask & (a <  b); }
  //__forceinline vboolf4 ge(const vboolf4& mask, const vuint4& a, const vuint4& b) { return mask & (a >= b); }
  //__forceinline vboolf4 gt(const vboolf4& mask, const vuint4& a, const vuint4& b) { return mask & (a >  b); }
  //__forceinline vboolf4 le(const vboolf4& mask, const vuint4& a, const vuint4& b) { return mask & (a <= b); }
#endif

  template<int mask>
  __forceinline vuint4 select(const vuint4& t, const vuint4& f) {
#if defined(__SSE4_1__) 
    return _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(f), _mm_castsi128_ps(t), mask));
#else
    return select(vboolf4(mask), t, f);
#endif    
  }
                                                                               
#if defined(__aarch64__) && defined(BUILD_IOS)
  template<> __forceinline vuint4 select<0>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vzero);
  }
  template<> __forceinline vuint4 select<1>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v000F);
  }
  template<> __forceinline vuint4 select<2>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v00F0);
  }
  template<> __forceinline vuint4 select<3>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v00FF);
  }
  template<> __forceinline vuint4 select<4>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v0F00);
  }
  template<> __forceinline vuint4 select<5>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v0F0F);
  }
  template<> __forceinline vuint4 select<6>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v0FF0);
  }
  template<> __forceinline vuint4 select<7>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, v0FFF);
  }
  template<> __forceinline vuint4 select<8>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vF000);
  }
  template<> __forceinline vuint4 select<9>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vF00F);
  }
  template<> __forceinline vuint4 select<10>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vF0F0);
  }
  template<> __forceinline vuint4 select<11>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vF0FF);
  }
  template<> __forceinline vuint4 select<12>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vFF00);
  }
  template<> __forceinline vuint4 select<13>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vFF0F);
  }
  template<> __forceinline vuint4 select<14>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vFFF0);
  }
  template<> __forceinline vuint4 select<15>(const vuint4& t, const vuint4& f) {
    return _mm_blendv_ps(f, t, vFFFF);
  }
#endif

/*#if defined(__SSE4_1__)
  __forceinline vuint4 min(const vuint4& a, const vuint4& b) { return _mm_min_epu32(a, b); }
  __forceinline vuint4 max(const vuint4& a, const vuint4& b) { return _mm_max_epu32(a, b); }

#else
  __forceinline vuint4 min(const vuint4& a, const vuint4& b) { return select(a < b,a,b); }
  __forceinline vuint4 max(const vuint4& a, const vuint4& b) { return select(a < b,b,a); }
#endif

  __forceinline vuint4 min(const vuint4& a, unsigned int  b) { return min(a,vuint4(b)); }
  __forceinline vuint4 min(unsigned int  a, const vuint4& b) { return min(vuint4(a),b); }
  __forceinline vuint4 max(const vuint4& a, unsigned int  b) { return max(a,vuint4(b)); }
  __forceinline vuint4 max(unsigned int  a, const vuint4& b) { return max(vuint4(a),b); }*/

  ////////////////////////////////////////////////////////////////////////////////
  // Movement/Shifting/Shuffling Functions
  ////////////////////////////////////////////////////////////////////////////////

  __forceinline vuint4 unpacklo(const vuint4& a, const vuint4& b) { return _mm_castps_si128(_mm_unpacklo_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b))); }
  __forceinline vuint4 unpackhi(const vuint4& a, const vuint4& b) { return _mm_castps_si128(_mm_unpackhi_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b))); }

#if defined(__aarch64__) && defined(BUILD_IOS)
  template<int i0, int i1, int i2, int i3>
  __forceinline vuint4 shuffle(const vuint4& v) {
    return vqtbl1q_u8( v, _MN_SHUFFLE(i0, i1, i2, i3));
  }
  template<int i0, int i1, int i2, int i3>
  __forceinline vuint4 shuffle(const vuint4& a, const vuint4& b) {
    return vqtbl2q_u8( (uint8x16x2_t){a, b}, _MF_SHUFFLE(i0, i1, i2, i3) );
  }
#else
  template<int i0, int i1, int i2, int i3>
  __forceinline vuint4 shuffle(const vuint4& v) {
    return _mm_shuffle_epi32(v, _MM_SHUFFLE(i3, i2, i1, i0));
  }
  template<int i0, int i1, int i2, int i3>
  __forceinline vuint4 shuffle(const vuint4& a, const vuint4& b) {
    return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(i3, i2, i1, i0)));
  }
#endif
                                                                               
#if defined(__aarch64__) && defined(BUILD_IOS)
  template<> __forceinline vuint4 shuffle<0, 0, 2, 2>(const vuint4& v) { return vqtbl1q_u8( v, (uint8x16_t){0,1,2,3, 0,1,2,3, 8,9,10,11, 8,9,10,11} ); }
  template<> __forceinline vuint4 shuffle<1, 1, 3, 3>(const vuint4& v) { return vqtbl1q_u8( v, (uint8x16_t){4,5,6,7, 4,5,6,7, 12,13,14,15, 12,13,14,15} ); }
  template<> __forceinline vuint4 shuffle<0, 1, 0, 1>(const vuint4& v) { return vqtbl1q_u8( v, (uint8x16_t){0,1,2,3, 4,5,6,7, 0,1,2,3, 4,5,6,7} ); }
#elif defined(__SSE3__)
  template<> __forceinline vuint4 shuffle<0, 0, 2, 2>(const vuint4& v) { return _mm_castps_si128(_mm_moveldup_ps(_mm_castsi128_ps(v))); }
  template<> __forceinline vuint4 shuffle<1, 1, 3, 3>(const vuint4& v) { return _mm_castps_si128(_mm_movehdup_ps(_mm_castsi128_ps(v))); }
  template<> __forceinline vuint4 shuffle<0, 1, 0, 1>(const vuint4& v) { return _mm_castpd_si128(_mm_movedup_pd (_mm_castsi128_pd(v))); }
#endif

  template<int i>
  __forceinline vuint4 shuffle(const vuint4& v) {
    return shuffle<i,i,i,i>(v);
  }

#if defined(__aarch64__) && defined(BUILD_IOS)
  template<int src> __forceinline unsigned int extract(const vuint4& b);
  template<int dst> __forceinline vuint4 insert(const vuint4& a, const unsigned b);
#elif defined(__SSE4_1__)
  template<int src> __forceinline unsigned int extract(const vuint4& b) { return _mm_extract_epi32(b, src); }
  template<int dst> __forceinline vuint4 insert(const vuint4& a, const unsigned b) { return _mm_insert_epi32(a, b, dst); }
#else
  template<int src> __forceinline unsigned int extract(const vuint4& b) { return b[src&3]; }
  template<int dst> __forceinline vuint4 insert(const vuint4& a, const unsigned b) { vuint4 c = a; c[dst&3] = b; return c; }
#endif

#if defined(__aarch64__) && defined(BUILD_IOS)
  template<> __forceinline unsigned int extract<0>(const vuint4& b) {
    return b[0];
  }
  template<> __forceinline unsigned int extract<1>(const vuint4& b) {
    return b[1];
  }
  template<> __forceinline unsigned int extract<2>(const vuint4& b) {
    return b[2];
  }
  template<> __forceinline unsigned int extract<3>(const vuint4& b) {
    return b[3];
  }
                                                                               
  template<> __forceinline vuint4 insert<0>(const vuint4& a, unsigned b){
    vuint4 c = a;
    c[0] = b;
    return c;
  }
  template<> __forceinline vuint4 insert<1>(const vuint4& a, unsigned b){
    vuint4 c = a;
    c[1] = b;
    return c;
  }
  template<> __forceinline vuint4 insert<2>(const vuint4& a, unsigned b){
    vuint4 c = a;
    c[2] = b;
    return c;
  }
  template<> __forceinline vuint4 insert<3>(const vuint4& a, unsigned b){
    vuint4 c = a;
    c[3] = b;
    return c;
  }
                                                                               
  __forceinline unsigned int toScalar(const vuint4& v) {
    return v[0];
  }
#else
  template<> __forceinline unsigned int extract<0>(const vuint4& b) { return _mm_cvtsi128_si32(b); }

  __forceinline unsigned int toScalar(const vuint4& v) { return _mm_cvtsi128_si32(v); }
#endif
                                                                               
  ////////////////////////////////////////////////////////////////////////////////
  /// Reductions
  ////////////////////////////////////////////////////////////////////////////////

#if 0
#if defined(__SSE4_1__)

  __forceinline vuint4 vreduce_min(const vuint4& v) { vuint4 h = min(shuffle<1,0,3,2>(v),v); return min(shuffle<2,3,0,1>(h),h); }
  __forceinline vuint4 vreduce_max(const vuint4& v) { vuint4 h = max(shuffle<1,0,3,2>(v),v); return max(shuffle<2,3,0,1>(h),h); }
  __forceinline vuint4 vreduce_add(const vuint4& v) { vuint4 h = shuffle<1,0,3,2>(v)   + v ; return shuffle<2,3,0,1>(h)   + h ; }

  __forceinline unsigned int reduce_min(const vuint4& v) { return toScalar(vreduce_min(v)); }
  __forceinline unsigned int reduce_max(const vuint4& v) { return toScalar(vreduce_max(v)); }
  __forceinline unsigned int reduce_add(const vuint4& v) { return toScalar(vreduce_add(v)); }

  __forceinline size_t select_min(const vuint4& v) { return bsf(movemask(v == vreduce_min(v))); }
  __forceinline size_t select_max(const vuint4& v) { return bsf(movemask(v == vreduce_max(v))); }

  //__forceinline size_t select_min(const vboolf4& valid, const vuint4& v) { const vuint4 a = select(valid,v,vuint4(pos_inf)); return bsf(movemask(valid & (a == vreduce_min(a)))); }
  //__forceinline size_t select_max(const vboolf4& valid, const vuint4& v) { const vuint4 a = select(valid,v,vuint4(neg_inf)); return bsf(movemask(valid & (a == vreduce_max(a)))); }

#else

  __forceinline unsigned int reduce_min(const vuint4& v) { return min(v[0],v[1],v[2],v[3]); }
  __forceinline unsigned int reduce_max(const vuint4& v) { return max(v[0],v[1],v[2],v[3]); }
  __forceinline unsigned int reduce_add(const vuint4& v) { return v[0]+v[1]+v[2]+v[3]; }

#endif
#endif

  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  inline std::ostream& operator <<(std::ostream& cout, const vuint4& a) {
    return cout << "<" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << ">";
  }
}

