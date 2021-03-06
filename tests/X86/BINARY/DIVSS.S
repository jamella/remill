/* Copyright 2016 Peter Goodman (peter@trailofbits.com), all rights reserved. */

TEST_BEGIN(DIVSSv128v128_40, 1)
TEST_INPUTS(0)
    divss xmm4, xmm0
TEST_END

TEST_BEGIN(DIVSSv128v128_45, 1)
TEST_INPUTS(0)
    divss xmm4, xmm5
TEST_END

#if APPLE_SAFE_TESTS

TEST_BEGIN(DIVSSv128m128_40, 1)
TEST_INPUTS(0)
    movdqu [rsp - 16], xmm0
    divss xmm4, [rsp - 16]
TEST_END

TEST_BEGIN(DIVSSv128m128_45, 1)
TEST_INPUTS(0)
    movdqu [rsp - 16], xmm5
    divss xmm4, [rsp - 16]
TEST_END

#endif

#if HAS_FEATURE_AVX
TEST_BEGIN(VDIVSSv128v128_40, 1)
TEST_INPUTS(0)
    vdivss xmm0, xmm4, xmm0
TEST_END

TEST_BEGIN(VDIVSSv128v128_45, 1)
TEST_INPUTS(0)
    vdivss xmm1, xmm4, xmm5
TEST_END

#if APPLE_SAFE_TESTS
TEST_BEGIN_64(VDIVSSv128m32_45, 3)
TEST_INPUTS(
    0x3f800000,
    0x40490fdb,
    0x47c34f80)

    mov DWORD PTR [rsp - 16], ARG1_32
    vdivss xmm1, xmm4, DWORD PTR [rsp - 16]
TEST_END_64
#endif  // APPLE_SAFE_TESTS
#endif
