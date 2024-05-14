/*
 * Copyright (c) 2009-2024 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
 * Project:      Arm-2D Library
 * Title:        __arm_2d_tile_copy_with_mask_and_opacity_helium.c
 * Description:  APIs tile copy with mask and opacity
 *
 * $Date:        14. May 2024
 * $Revision:    V.0.1.0
 *
 * Target Processor:  Cortex-M cores
 *
 * -------------------------------------------------------------------- */


/*============================ INCLUDES ======================================*/

#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wempty-translation-unit"
#endif

#ifdef __ARM_2D_COMPILATION_UNIT
#undef __ARM_2D_COMPILATION_UNIT

#define __ARM_2D_IMPL__

#include "arm_2d.h"
#include "__arm_2d_impl.h"

#ifdef   __cplusplus
extern "C" {
#endif

#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wunknown-warning-option"
#   pragma clang diagnostic ignored "-Wreserved-identifier"
#   pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#   pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#   pragma clang diagnostic ignored "-Wcast-qual"
#   pragma clang diagnostic ignored "-Wcast-align"
#   pragma clang diagnostic ignored "-Wextra-semi-stmt"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wunused-function"
#   pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#   pragma clang diagnostic ignored "-Wdouble-promotion"
#   pragma clang diagnostic ignored "-Wunused-parameter"
#   pragma clang diagnostic ignored "-Wimplicit-float-conversion"
#   pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#   pragma clang diagnostic ignored "-Wtautological-pointer-compare"
#   pragma clang diagnostic ignored "-Wsign-compare"
#   pragma clang diagnostic ignored "-Wmissing-prototypes"
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#   pragma clang diagnostic ignored "-Wswitch-enum"
#   pragma clang diagnostic ignored "-Wswitch"
#   pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#elif defined(__IS_COMPILER_ARM_COMPILER_5__)
#   pragma diag_suppress 174,177,188,68,513,144
#endif

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/


__STATIC_FORCEINLINE
void __arm_2d_ccca8888_unpack_u16(const uint8_t * pSource, uint16x8_t * opa,
                                            uint16x8_t * R, uint16x8_t * G, uint16x8_t * B)
{
    uint8x16x2_t    vdeintr2 = vld2q_u8(pSource);

    *opa = vmovltq(vdeintr2.val[1]);
    *G = vmovlbq(vdeintr2.val[1]);
    *R = vmovltq(vdeintr2.val[0]);
    *B = vmovlbq(vdeintr2.val[0]);
}

__STATIC_FORCEINLINE
void __arm_2d_ccca8888_get_and_dup_opa(const uint8_t * pSource, uint16x8_t * opa,
                                            uint16x8_t * src)
{
    /* offset to replicate the opacity accross the 4 channels */
    uint16x8_t offset = {3, 3, 3, 3, 7, 7, 7, 7};

    *src = vldrbq_u16(pSource);

    /*
      replicate alpha, but alpha location = 0 (zeroing) so that transparency = 0x100
      and leaves target 0 unchanged
      vSrcOpa = | opa0 | opa0 | opa0 |  0  | opa1 | opa1 | opa1 |  0  |
      */
    *opa = vldrbq_gather_offset_z_u16(pSource,  offset, 0x3f3f);
}

__STATIC_FORCEINLINE
uint16x8_t __arm_2d_unpack_and_blend_gray8(const uint8_t * pchTarget, uint16x8_t opa,
    uint16x8_t R, uint16x8_t G, uint16x8_t B)
{
    uint16x8_t      vtrgt = vldrbq_u16(pchTarget);
    uint16x8_t      vTrans = 256 - opa;

    uint16x8_t vAvg = R + G + B;
    vAvg = vAvg / 3;

    return (vmulq(vAvg, opa) + vmulq(vtrgt, vTrans)) >> 8;
}


__STATIC_FORCEINLINE
uint16x8_t __arm_2d_unpack_and_blend_rg565(const uint16_t * phwTarget, uint16x8_t opa,
    uint16x8_t vRsrc, uint16x8_t vGsrc, uint16x8_t vBsrc)
{
    uint16x8_t      vtrgt = vld1q(phwTarget);
    uint16x8_t      vTrans = 256 - opa;
    uint16x8_t      vRtgt, vGtgt, vBtgt;

    __arm_2d_rgb565_unpack_single_vec(vtrgt, &vRtgt, &vGtgt, &vBtgt);


    vRtgt = (vmulq(vRsrc, opa) + vmulq(vRtgt, vTrans)) >> 8;
    vGtgt = (vmulq(vGsrc, opa) + vmulq(vGtgt, vTrans)) >> 8;
    vBtgt = (vmulq(vBsrc, opa) + vmulq(vBtgt, vTrans)) >> 8;

    return __arm_2d_rgb565_pack_single_vec(vRtgt, vGtgt, vBtgt);
}


__STATIC_FORCEINLINE
uint16x8_t __arm_2d_unpack_and_blend_cccn888(const uint8_t * pwTarget, uint16x8_t opa,
    uint16x8_t vSrc)
{
    uint16x8_t      vTrg = vldrbq_u16(pwTarget);
    uint16x8_t      vTrans = 256 - opa;

    return (vTrg *vTrans + vSrc*opa) >> 8;
}


__STATIC_FORCEINLINE
uint16x8_t __arm_2d_scale_alpha_mask_opa(uint16x8_t opa, uint16x8_t vSrcMask,
    uint_fast16_t hwOpacity)
{
    uint16x8_t vHhwTransparency = 256 - (vmulq(vSrcMask, hwOpacity) >> 8);

    opa = vpselq(vdupq_n_u16(256),opa, vcmpeqq_n_u16(opa, 255));
    opa=  vmulq(opa, (256-vHhwTransparency))  >> 8;
    return opa;
}

uint16x8_t __arm_2d_scale_alpha_mask(uint16x8_t opa, uint16x8_t vSrcMask)
{
    uint16x8_t vHhwTransparency = 256 - vSrcMask;

    opa = vpselq(vdupq_n_u16(256),opa, vcmpeqq_n_u16(opa, 255));
    opa=  vmulq(opa, (256-vHhwTransparency))  >> 8;
    return opa;
}


__OVERRIDE_WEAK
void __MVE_WRAPPER(__arm_2d_impl_ccca8888_tile_copy_to_gray8_with_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint8_t *__RESTRICT pchTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwRatio)
{
    hwRatio += (hwRatio == 255);


    for (int_fast16_t y = 0; y < ptCopySize->iHeight; y++) {

        const uint8_t   *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint8_t         *__RESTRICT pchTarget = pchTargetBase;
        int32_t         blkCnt = ptCopySize->iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);

            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);


            vSrcOpa = vpselq(vdupq_n_u16(256),vSrcOpa, vcmpeqq_n_u16(vSrcOpa, 255));
            vSrcOpa=  vmulq_n_u16(vSrcOpa, hwRatio)  >> 8;


            vstrbq_p_u16(pchTarget,
                __arm_2d_unpack_and_blend_gray8(pchTarget, vSrcOpa, vSrcR, vSrcG, vSrcB),
                tailPred);

            pSource += 32;
            pchTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        pchTargetBase += iTargetStride;
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER(__arm_2d_impl_ccca8888_tile_copy_to_gray8_with_src_mask_and_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint8_t * __RESTRICT pchSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint8_t *__RESTRICT pchTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwOpacity)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint8_t *pchSourceMask = pchSourceMaskBase;


    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* preprocess the opacity */
    hwOpacity += (hwOpacity == 255);

    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint8_t        *__RESTRICT pchTarget = pchTargetBase;
        uint8_t        *__RESTRICT pchSourceMaskLine = pchSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);

            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_u16(pchSourceMaskLine);

            vSrcOpa = __arm_2d_scale_alpha_mask_opa(vSrcOpa, vSrcMask, hwOpacity);


            vstrbq_p_u16(pchTarget,
                __arm_2d_unpack_and_blend_gray8(pchTarget, vSrcOpa, vSrcR, vSrcG, vSrcB),
                tailPred);

            pchSourceMaskLine += 8;

            pSource += 32;
            pchTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);

        pwSourceBase += iSourceStride;
        pchTargetBase += iTargetStride;

        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pchSourceMask = pchSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pchSourceMask += iSourceMaskStride;
        }
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_gray8_with_src_chn_mask_and_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint32_t * __RESTRICT pwSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint8_t *__RESTRICT pchTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwOpacity)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint32_t *pwSourceMask = pwSourceMaskBase;
    uint16x8_t      vStride4Offs = vidupq_n_u16(0, 4);

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* preprocess the opacity */
    hwOpacity += (hwOpacity == 255);

    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint8_t        *__RESTRICT pchTarget = pchTargetBase;
        uint8_t        *__RESTRICT pwSourceMaskLine = ( uint8_t *__RESTRICT)pwSourceMask;
        int32_t         blkCnt = iWidth;


        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);

            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pwSourceMaskLine, vStride4Offs);

            vSrcOpa = __arm_2d_scale_alpha_mask_opa(vSrcOpa, vSrcMask, hwOpacity);

            vstrbq_p_u16(pchTarget,
                __arm_2d_unpack_and_blend_gray8(pchTarget, vSrcOpa, vSrcR, vSrcG, vSrcB),
                tailPred);

            pwSourceMaskLine += 8*4;

            pSource += 32;
            pchTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);

        pwSourceBase += iSourceStride;
        pchTargetBase += iTargetStride;

        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pwSourceMask = pwSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pwSourceMask += iSourceMaskStride;
        }
    }
}

__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_gray8_with_src_mask)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint8_t * __RESTRICT pchSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint8_t *__RESTRICT pchTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint8_t *pchSourceMask = pchSourceMaskBase;


    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;


    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint8_t *__RESTRICT pchTarget = pchTargetBase;
        uint8_t *__RESTRICT pchSourceMaskLine = pchSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);

            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_u16(pchSourceMaskLine);

            vSrcOpa = __arm_2d_scale_alpha_mask(vSrcOpa, vSrcMask);


            vstrbq_p_u16(pchTarget,
                __arm_2d_unpack_and_blend_gray8(pchTarget, vSrcOpa, vSrcR, vSrcG, vSrcB),
                tailPred);

            pchSourceMaskLine += 8;

            pSource += 32;
            pchTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);

        pwSourceBase += iSourceStride;
        pchTargetBase += iTargetStride;

        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pchSourceMask = pchSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pchSourceMask += iSourceMaskStride;
        }
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER(__arm_2d_impl_ccca8888_tile_copy_to_gray8_with_src_chn_mask)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint32_t * __RESTRICT pwSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint8_t *__RESTRICT pchTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint32_t *pwSourceMask = pwSourceMaskBase;
    uint16x8_t      vStride4Offs = vidupq_n_u16(0, 4);

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;


    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint8_t *__RESTRICT pchTarget = pchTargetBase;
        uint8_t *__RESTRICT pwSourceMaskLine = ( uint8_t *__RESTRICT)pwSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);

            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pwSourceMaskLine, vStride4Offs);

            vSrcOpa = __arm_2d_scale_alpha_mask(vSrcOpa, vSrcMask);

            vstrbq_p_u16(pchTarget,
                __arm_2d_unpack_and_blend_gray8(pchTarget, vSrcOpa, vSrcR, vSrcG, vSrcB),
                tailPred);

            pwSourceMaskLine += 8*4;

            pSource += 32;
            pchTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);

        pwSourceBase += iSourceStride;
        pchTargetBase += iTargetStride;

        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pwSourceMask = pwSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pwSourceMask += iSourceMaskStride;
        }
    }
}

__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_rgb565_with_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint16_t *__RESTRICT phwTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwRatio)
{

    hwRatio += (hwRatio == 255);


    for (int_fast16_t y = 0; y < ptCopySize->iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint16_t       *__RESTRICT phwTarget = phwTargetBase;
        int32_t         blkCnt = ptCopySize->iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);
            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);


            vSrcOpa = vpselq(vdupq_n_u16(256),vSrcOpa, vcmpeqq_n_u16(vSrcOpa, 255));
            vSrcOpa=  vmulq_n_u16(vSrcOpa, hwRatio)  >> 8;


            vst1q_p(phwTarget,
                __arm_2d_unpack_and_blend_rg565(phwTarget, vSrcOpa,vSrcR, vSrcG, vSrcB),
                tailPred);

            pSource += 32;
            phwTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        phwTargetBase += iTargetStride;
    }

}



__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_rgb565_with_src_mask_and_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,

                                    uint8_t * __RESTRICT pchSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,

                                    uint16_t *__RESTRICT phwTargetBase,
                                    int16_t iTargetStride,

                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwOpacity)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint8_t *pchSourceMask = pchSourceMaskBase;

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* preprocess the opacity */
    hwOpacity += (hwOpacity == 255);

    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint16_t       *__RESTRICT phwTarget = phwTargetBase;
        uint8_t        *__RESTRICT pchSourceMaskLine = pchSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);
            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_u16(pchSourceMaskLine);

            vSrcOpa = __arm_2d_scale_alpha_mask_opa(vSrcOpa, vSrcMask, hwOpacity);

            vst1q_p(phwTarget,
                __arm_2d_unpack_and_blend_rg565(phwTarget, vSrcOpa,vSrcR, vSrcG, vSrcB),
                tailPred);

            pchSourceMaskLine += 8;
            pSource += 32;
            phwTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        phwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pchSourceMask = pchSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pchSourceMask += iSourceMaskStride;
        }
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_rgb565_with_src_chn_mask_and_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,

                                    uint32_t * __RESTRICT pwSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,

                                    uint16_t *__RESTRICT phwTargetBase,
                                    int16_t iTargetStride,

                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwOpacity)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint32_t *pwSourceMask = pwSourceMaskBase;
    uint16x8_t      vStride4Offs = vidupq_n_u16(0, 4);

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* preprocess the opacity */
    hwOpacity += (hwOpacity == 255);

    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint16_t       *__RESTRICT phwTarget = phwTargetBase;
        uint8_t        *__RESTRICT pwSourceMaskLine = ( uint8_t *__RESTRICT)pwSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);
            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pwSourceMaskLine, vStride4Offs);

            vSrcOpa = __arm_2d_scale_alpha_mask_opa(vSrcOpa, vSrcMask, hwOpacity);

            vst1q_p(phwTarget,
                __arm_2d_unpack_and_blend_rg565(phwTarget, vSrcOpa,vSrcR, vSrcG, vSrcB),
                tailPred);;

            pwSourceMaskLine += 8*4;
            pSource += 32;
            phwTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        phwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pwSourceMask = pwSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pwSourceMask += iSourceMaskStride;
        }
    }
}

__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_rgb565_with_src_mask)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,

                                    uint8_t * __RESTRICT pchSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,

                                    uint16_t *__RESTRICT phwTargetBase,
                                    int16_t iTargetStride,

                                    arm_2d_size_t *__RESTRICT ptCopySize)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint8_t *pchSourceMask = pchSourceMaskBase;

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;


    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint16_t       *__RESTRICT phwTarget = phwTargetBase;
        uint8_t        *__RESTRICT pchSourceMaskLine = pchSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);
            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_u16(pchSourceMaskLine);

            vSrcOpa = __arm_2d_scale_alpha_mask(vSrcOpa, vSrcMask);

            vst1q_p(phwTarget,
                __arm_2d_unpack_and_blend_rg565(phwTarget, vSrcOpa,vSrcR, vSrcG, vSrcB),
                tailPred);

            pchSourceMaskLine += 8;
            pSource += 32;
            phwTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        phwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pchSourceMask = pchSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pchSourceMask += iSourceMaskStride;
        }
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_rgb565_with_src_chn_mask)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,

                                    uint32_t * __RESTRICT pwSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,

                                    uint16_t *__RESTRICT phwTargetBase,
                                    int16_t iTargetStride,

                                    arm_2d_size_t *__RESTRICT ptCopySize)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint32_t *pwSourceMask = pwSourceMaskBase;
    uint16x8_t      vStride4Offs = vidupq_n_u16(0, 4);

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    for (int_fast16_t y = 0; y < iHeight; y++) {

        const uint8_t  *__RESTRICT pSource = (const uint8_t *) pwSourceBase;
        uint16_t       *__RESTRICT phwTarget = phwTargetBase;
        uint8_t        *__RESTRICT pwSourceMaskLine = ( uint8_t *__RESTRICT)pwSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp16q(blkCnt);
            uint16x8_t      vSrcOpa, vSrcG, vSrcR, vSrcB;

            __arm_2d_ccca8888_unpack_u16(pSource, &vSrcOpa, &vSrcR, &vSrcG, &vSrcB);

            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pwSourceMaskLine, vStride4Offs);

            vSrcOpa = __arm_2d_scale_alpha_mask(vSrcOpa, vSrcMask);

            vst1q_p(phwTarget,
                __arm_2d_unpack_and_blend_rg565(phwTarget, vSrcOpa,vSrcR, vSrcG, vSrcB),
                tailPred);

            pwSourceMaskLine += 8*4;
            pSource += 32;
            phwTarget += 8;
            blkCnt -= 8;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        phwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pwSourceMask = pwSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pwSourceMask += iSourceMaskStride;
        }
    }
}

__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_cccn888_with_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint32_t *__RESTRICT pwTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwRatio)
{
    hwRatio += (hwRatio == 255);


    for (int_fast16_t y = 0; y < ptCopySize->iHeight; y++) {

        const uint8_t *__RESTRICT pwSource = (const uint8_t *)pwSourceBase;
        uint8_t       *__RESTRICT pwTarget = (uint8_t *)pwTargetBase;
        int32_t         blkCnt = ptCopySize->iWidth;

        do {
            mve_pred16_t    tailPred = vctp64q(blkCnt);

            uint16x8_t vSrc, vSrcOpa;

            __arm_2d_ccca8888_get_and_dup_opa(pwSource, &vSrcOpa, &vSrc);

            vSrcOpa = vpselq(vdupq_n_u16(256),vSrcOpa, vcmpeqq_n_u16(vSrcOpa, 255));
            vSrcOpa=  vmulq_n_u16(vSrcOpa, hwRatio)  >> 8;


            vstrbq_p_u16(pwTarget,
                __arm_2d_unpack_and_blend_cccn888(pwTarget, vSrcOpa, vSrc),
                tailPred);

            pwSource += 8;
            pwTarget += 8;
            blkCnt -= 2;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;
    }

}



__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_cccn888_with_src_mask_and_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint8_t * __RESTRICT pchSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint32_t *__RESTRICT pwTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwOpacity)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint8_t *pchSourceMask = pchSourceMaskBase;  /// useless

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* preprocess the opacity */
    hwOpacity += (hwOpacity == 255);

    /* offset to replicate 2 masks accross the 4 channels */
    uint16x8_t offsetMsk = {0, 0, 0, 0, 1, 1, 1, 1};

    for (int_fast16_t y = 0; y < iHeight; y++) {


        const uint8_t *__RESTRICT pwSource = (const uint8_t *)pwSourceBase;
        uint8_t       *__RESTRICT pwTarget = (uint8_t *)pwTargetBase;
        uint8_t        *__RESTRICT pchSourceMaskLine = pchSourceMask;
        int32_t         blkCnt = iWidth;

        do {

            mve_pred16_t    tailPred = vctp64q(blkCnt);

            uint16x8_t vSrc, vSrcOpa;

            __arm_2d_ccca8888_get_and_dup_opa(pwSource, &vSrcOpa, &vSrc);


            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pchSourceMaskLine,  offsetMsk);
            vSrcOpa = __arm_2d_scale_alpha_mask_opa(vSrcOpa, vSrcMask, hwOpacity);


            vstrbq_p_u16(pwTarget,
                __arm_2d_unpack_and_blend_cccn888(pwTarget, vSrcOpa, vSrc),
                tailPred);

            pchSourceMaskLine += 2;
            pwSource += 8;
            pwTarget += 8;
            blkCnt -= 2;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pchSourceMask = pchSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pchSourceMask += iSourceMaskStride;
        }
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_cccn888_with_src_chn_mask_and_opacity)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint32_t * __RESTRICT pwSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint32_t *__RESTRICT pwTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize,
                                    uint_fast16_t hwOpacity)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint32_t *pwSourceMask = pwSourceMaskBase;

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* preprocess the opacity */
    hwOpacity += (hwOpacity == 255);


    /* offset to replicate 2 masks accros the 4 channels */
    uint16x8_t offsetMsk = {0, 0, 0, 0, 4, 4, 4, 4};

    for (int_fast16_t y = 0; y < iHeight; y++) {


        const uint8_t *__RESTRICT pwSource = (const uint8_t *)pwSourceBase;
        uint8_t       *__RESTRICT pwTarget = (uint8_t *)pwTargetBase;
        uint8_t       *__RESTRICT pwSourceMaskLine = ( uint8_t *__RESTRICT)pwSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp64q(blkCnt);

            uint16x8_t vSrc, vSrcOpa;

            __arm_2d_ccca8888_get_and_dup_opa(pwSource, &vSrcOpa, &vSrc);


            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pwSourceMaskLine,  offsetMsk);

            vSrcOpa = __arm_2d_scale_alpha_mask_opa(vSrcOpa, vSrcMask, hwOpacity);

            vstrbq_p_u16(pwTarget,
                __arm_2d_unpack_and_blend_cccn888(pwTarget, vSrcOpa, vSrc),
                tailPred);

            pwSourceMaskLine += 2*4;
            pwSource += 8;
            pwTarget += 8;
            blkCnt -= 2;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pwSourceMask = pwSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pwSourceMask += iSourceMaskStride;
        }
    }
}

__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_cccn888_with_src_mask)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,

                                    uint8_t * __RESTRICT pchSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,

                                    uint32_t *__RESTRICT pwTargetBase,
                                    int16_t iTargetStride,

                                    arm_2d_size_t *__RESTRICT ptCopySize)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint8_t *pchSourceMask = pchSourceMaskBase;  /// useless

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;

    /* offset to replicate 2 masks accross the 4 channels */
    uint16x8_t offsetMsk = {0, 0, 0, 0, 1, 1, 1, 1};

    for (int_fast16_t y = 0; y < iHeight; y++) {


        const uint8_t *__RESTRICT pwSource = (const uint8_t *)pwSourceBase;
        uint8_t       *__RESTRICT pwTarget = (uint8_t *)pwTargetBase;
        uint8_t        *__RESTRICT pchSourceMaskLine = pchSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp64q(blkCnt);

            uint16x8_t vSrc, vSrcOpa;

            __arm_2d_ccca8888_get_and_dup_opa(pwSource, &vSrcOpa, &vSrc);


            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pchSourceMaskLine,  offsetMsk);

            vSrcOpa = __arm_2d_scale_alpha_mask(vSrcOpa, vSrcMask);

            vstrbq_p_u16(pwTarget,
                __arm_2d_unpack_and_blend_cccn888(pwTarget, vSrcOpa, vSrc),
                tailPred);

            pchSourceMaskLine += 2;
            pwSource += 8;
            pwTarget += 8;
            blkCnt -= 2;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pchSourceMask = pchSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pchSourceMask += iSourceMaskStride;
        }
    }
}


__OVERRIDE_WEAK
void __MVE_WRAPPER( __arm_2d_impl_ccca8888_tile_copy_to_cccn888_with_src_chn_mask)(
                                    uint32_t *__RESTRICT pwSourceBase,
                                    int16_t iSourceStride,
                                    uint32_t * __RESTRICT pwSourceMaskBase,
                                    int16_t iSourceMaskStride,
                                    arm_2d_size_t * __RESTRICT ptSourceMaskSize,
                                    uint32_t *__RESTRICT pwTargetBase,
                                    int16_t iTargetStride,
                                    arm_2d_size_t *__RESTRICT ptCopySize)
{
    /* handle source mask */
    assert(ptSourceMaskSize->iWidth >= ptCopySize->iWidth);
    int_fast16_t iSourceMaskY = 0;
    int_fast16_t iSourceMaskHeight = ptSourceMaskSize->iHeight;
    uint32_t *pwSourceMask = pwSourceMaskBase;

    int_fast16_t iHeight = ptCopySize->iHeight;
    int_fast16_t iWidth  = ptCopySize->iWidth;


    /* offset to replicate 2 masks accros the 4 channels */
    uint16x8_t offsetMsk = {0, 0, 0, 0, 4, 4, 4, 4};

    for (int_fast16_t y = 0; y < iHeight; y++) {


        const uint8_t *__RESTRICT pwSource = (const uint8_t *)pwSourceBase;
        uint8_t       *__RESTRICT pwTarget = (uint8_t *)pwTargetBase;
        uint8_t       *__RESTRICT pwSourceMaskLine = ( uint8_t *__RESTRICT)pwSourceMask;
        int32_t         blkCnt = iWidth;

        do {
            mve_pred16_t    tailPred = vctp64q(blkCnt);

            uint16x8_t vSrc, vSrcOpa;

            __arm_2d_ccca8888_get_and_dup_opa(pwSource, &vSrcOpa, &vSrc);


            uint16x8_t vSrcMask = vldrbq_gather_offset_u16(pwSourceMaskLine,  offsetMsk);

            vSrcOpa = __arm_2d_scale_alpha_mask(vSrcOpa, vSrcMask);

            vstrbq_p_u16(pwTarget,
                __arm_2d_unpack_and_blend_cccn888(pwTarget, vSrcOpa, vSrc),
                tailPred);

            pwSourceMaskLine += 2*4;
            pwSource += 8;
            pwTarget += 8;
            blkCnt -= 2;
        }
        while (blkCnt > 0);
        pwSourceBase += iSourceStride;
        pwTargetBase += iTargetStride;


        /* source mask rolling */
        if (++iSourceMaskY >= iSourceMaskHeight) {
            pwSourceMask = pwSourceMaskBase;
            iSourceMaskY = 0;
        } else {
            pwSourceMask += iSourceMaskStride;
        }
    }
}



#ifdef   __cplusplus
}
#endif

#endif /* __ARM_2D_COMPILATION_UNIT */