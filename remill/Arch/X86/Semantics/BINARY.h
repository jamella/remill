/* Copyright 2015 Peter Goodman (peter@trailofbits.com), all rights reserved. */

#ifndef REMILL_ARCH_X86_SEMANTICS_BINARY_H_
#define REMILL_ARCH_X86_SEMANTICS_BINARY_H_

namespace {

template <typename Tag, typename T>
ALWAYS_INLINE void SetFlagsIncDec(State &state, T lhs, T rhs, T res) {
  state.aflag.pf = ParityFlag(res);
  state.aflag.af = AuxCarryFlag(lhs, rhs, res);
  state.aflag.zf = ZeroFlag(res);
  state.aflag.sf = SignFlag(res);
  state.aflag.of = Overflow<Tag>::Flag(lhs, rhs, res);
}

template <typename Tag, typename T>
ALWAYS_INLINE void SetFlagsAddSub(State &state, T lhs, T rhs, T res) {
  state.aflag.cf = Carry<Tag>::Flag(lhs, rhs, res);
  SetFlagsIncDec<Tag>(state, lhs, rhs, res);
}

template <typename D, typename S1, typename S2>
DEF_SEM(ADD, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  const T src1 = R(src1_);
  const T src2 = R(src2_);
  const T res = src1 + src2;
  W(dst) = res;
  __remill_barrier_compiler();
  SetFlagsAddSub<tag_add>(state, src1, src2, res);
}

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(ADDPx, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  const T src1 = R(src1_);
  const T src2 = R(src2_);
  W(dst) = FloatAcc::Read(src1) + FloatAcc::Read(src2);
}

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(ADDSx, D dst, const S1 src1_, const S2 src2_) {
  auto src1 = R(src1_);
  auto src2 = R(src2_);
  FloatAcc::Write0(src1, FloatAcc::Read0(src1) + FloatAcc::Read0(src2));
  W(dst) = src1;
}

// Atomic fetch-add.
template <typename MW, typename M, typename RW, typename RT>
DEF_SEM(XADD, MW mdst, const M msrc_, const RW rdst, const RT rsrc_) {
  typedef BASE_TYPE_OF(RT) T;

  // Our lifter only injects atomic begin/end around memory access instructions
  // but this instruction is a full memory barrier, even when registers are
  // accessed.
  if (IsRegister<RW>::kValue) {
    __remill_memory_order = __remill_barrier_store_load(__remill_memory_order);
  }

  const T src1 = R(msrc_);
  const T src2 = R(rsrc_);
  const T res = src1 + src2;
  W(mdst) = res;
  __remill_barrier_compiler();
  W(rdst) = src1;
  __remill_barrier_compiler();
  SetFlagsAddSub<tag_add>(state, src1, src2, res);
}

}  // namespace

DEF_ISEL(ADD_MEMb_IMMb_80r0) = ADD<M8W, M8, I8>;
DEF_ISEL(ADD_GPR8_IMMb_80r0) = ADD<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(ADD_MEMv_IMMz, ADD);
DEF_ISEL_RnW_Rn_In(ADD_GPRv_IMMz, ADD);
DEF_ISEL(ADD_MEMb_IMMb_82r0) = ADD<M8W, M8, I8>;
DEF_ISEL(ADD_GPR8_IMMb_82r0) = ADD<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(ADD_MEMv_IMMb, ADD);
DEF_ISEL_RnW_Rn_In(ADD_GPRv_IMMb, ADD);
DEF_ISEL(ADD_MEMb_GPR8) = ADD<M8W, M8, R8>;
DEF_ISEL(ADD_GPR8_GPR8_00) = ADD<R8W, R8, R8>;
DEF_ISEL_MnW_Mn_Rn(ADD_MEMv_GPRv, ADD);
DEF_ISEL_RnW_Rn_Rn(ADD_GPRv_GPRv_01, ADD);
DEF_ISEL(ADD_GPR8_MEMb) = ADD<R8W, R8, M8>;
DEF_ISEL(ADD_GPR8_GPR8_02) = ADD<R8W, R8, R8>;
DEF_ISEL_RnW_Rn_Mn(ADD_GPRv_MEMv, ADD);
DEF_ISEL_RnW_Rn_Rn(ADD_GPRv_GPRv_03, ADD);
DEF_ISEL(ADD_AL_IMMb) = ADD<R8W, R8, I8>;
DEF_ISEL_RnW_Rn_In(ADD_OrAX_IMMz, ADD);

DEF_ISEL(XADD_MEMb_GPR8) = XADD<M8W, M8, R8W, R8>;
DEF_ISEL(XADD_GPR8_GPR8) = XADD<R8W, R8, R8W, R8>;
DEF_ISEL_MnW_Mn_RnW_Rn(XADD_MEMv_GPRv, XADD);
DEF_ISEL_RnW_Rn_RnW_Rn(XADD_GPRv_GPRv, XADD);

DEF_ISEL(ADDPS_XMMps_MEMps) = ADDPx<V128W, V128, MV128, Float32VecOps>;
DEF_ISEL(ADDPS_XMMps_XMMps) = ADDPx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VADDPS_XMMdq_XMMdq_MEMdq) = ADDPx<VV128W, VV128, MV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VADDPS_XMMdq_XMMdq_XMMdq) = ADDPx<VV128W, VV128, VV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VADDPS_YMMqq_YMMqq_MEMqq) = ADDPx<VV256W, VV256, MV256, Float32VecOps>;)
IF_AVX(DEF_ISEL(VADDPS_YMMqq_YMMqq_YMMqq) = ADDPx<VV256W, VV256, VV256, Float32VecOps>;)

DEF_ISEL(ADDPD_XMMpd_MEMpd) = ADDPx<V128W, V128, MV128, Float64VecOps>;
DEF_ISEL(ADDPD_XMMpd_XMMpd) = ADDPx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VADDPD_XMMdq_XMMdq_MEMdq) = ADDPx<VV128W, VV128, MV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VADDPD_XMMdq_XMMdq_XMMdq) = ADDPx<VV128W, VV128, VV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VADDPD_YMMqq_YMMqq_MEMqq) = ADDPx<VV256W, VV256, MV256, Float64VecOps>;)
IF_AVX(DEF_ISEL(VADDPD_YMMqq_YMMqq_YMMqq) = ADDPx<VV256W, VV256, VV256, Float64VecOps>;)

DEF_ISEL(ADDSS_XMMss_MEMss) = ADDSx<V128W, V128, MV32, Float32VecOps>;
DEF_ISEL(ADDSS_XMMss_XMMss) = ADDSx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VADDSS_XMMdq_XMMdq_MEMd) = ADDSx<VV128W, VV128, MV32, Float32VecOps>;)
IF_AVX(DEF_ISEL(VADDSS_XMMdq_XMMdq_XMMd) = ADDSx<VV128W, VV128, VV128, Float32VecOps>;)

DEF_ISEL(ADDSD_XMMsd_MEMsd) = ADDSx<V128W, V128, MV64, Float64VecOps>;
DEF_ISEL(ADDSD_XMMsd_XMMsd) = ADDSx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VADDSD_XMMdq_XMMdq_MEMq) = ADDSx<VV128W, VV128, MV64, Float64VecOps>;)
IF_AVX(DEF_ISEL(VADDSD_XMMdq_XMMdq_XMMq) = ADDSx<VV128W, VV128, VV128, Float64VecOps>;)

namespace {

template <typename D, typename S1, typename S2>
DEF_SEM(SUB, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  const T src1 = R(src1_);
  const T src2 = R(src2_);
  const T res = src1 - src2;
  W(dst) = res;
  __remill_barrier_compiler();
  SetFlagsAddSub<tag_sub>(state, src1, src2, res);
}

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(SUBPx, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  T src1 = R(src1_);
  T src2 = R(src2_);
  W(dst) = FloatAcc::Read(src1) - FloatAcc::Read(src2);
}

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(SUBSx, D dst, const S1 src1_, const S2 src2_) {
  auto src1 = R(src1_);
  auto src2 = R(src2_);
  FloatAcc::Write0(src1, FloatAcc::Read0(src1) - FloatAcc::Read0(src2));
  W(dst) = src1;
}

}  // namespace

DEF_ISEL(SUB_MEMb_IMMb_80r5) = SUB<M8W, M8, I8>;
DEF_ISEL(SUB_GPR8_IMMb_80r5) = SUB<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(SUB_MEMv_IMMz, SUB);
DEF_ISEL_RnW_Rn_In(SUB_GPRv_IMMz, SUB);
DEF_ISEL(SUB_MEMb_IMMb_82r5) = SUB<M8W, M8, I8>;
DEF_ISEL(SUB_GPR8_IMMb_82r5) = SUB<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(SUB_MEMv_IMMb, SUB);
DEF_ISEL_RnW_Rn_In(SUB_GPRv_IMMb, SUB);
DEF_ISEL(SUB_MEMb_GPR8) = SUB<M8W, M8, I8>;
DEF_ISEL(SUB_GPR8_GPR8_28) = SUB<R8W, R8, R8>;
DEF_ISEL_MnW_Mn_Rn(SUB_MEMv_GPRv, SUB);
DEF_ISEL_RnW_Rn_Rn(SUB_GPRv_GPRv_29, SUB);
DEF_ISEL(SUB_GPR8_GPR8_2A) = SUB<R8W, R8, R8>;
DEF_ISEL(SUB_GPR8_MEMb) = SUB<R8W, R8, M8>;
DEF_ISEL_RnW_Rn_Rn(SUB_GPRv_GPRv_2B, SUB);
DEF_ISEL_RnW_Rn_Mn(SUB_GPRv_MEMv, SUB);
DEF_ISEL(SUB_AL_IMMb) = SUB<R8W, R8, I8>;
DEF_ISEL_RnW_Rn_In(SUB_OrAX_IMMz, SUB);

DEF_ISEL(SUBPS_XMMps_MEMps) = SUBPx<V128W, V128, MV128, Float32VecOps>;
DEF_ISEL(SUBPS_XMMps_XMMps) = SUBPx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VSUBPS_XMMdq_XMMdq_MEMdq) = SUBPx<VV128W, VV128, MV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VSUBPS_XMMdq_XMMdq_XMMdq) = SUBPx<VV128W, VV128, VV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VSUBPS_YMMqq_YMMqq_MEMqq) = SUBPx<VV256W, VV256, MV256, Float32VecOps>;)
IF_AVX(DEF_ISEL(VSUBPS_YMMqq_YMMqq_YMMqq) = SUBPx<VV256W, VV256, VV256, Float32VecOps>;)

DEF_ISEL(SUBPD_XMMpd_MEMpd) = SUBPx<V128W, V128, MV128, Float64VecOps>;
DEF_ISEL(SUBPD_XMMpd_XMMpd) = SUBPx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VSUBPD_XMMdq_XMMdq_MEMdq) = SUBPx<VV128W, VV128, MV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VSUBPD_XMMdq_XMMdq_XMMdq) = SUBPx<VV128W, VV128, VV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VSUBPD_YMMqq_YMMqq_MEMqq) = SUBPx<VV256W, VV256, MV256, Float64VecOps>;)
IF_AVX(DEF_ISEL(VSUBPD_YMMqq_YMMqq_YMMqq) = SUBPx<VV256W, VV256, VV256, Float64VecOps>;)

DEF_ISEL(SUBSS_XMMss_MEMss) = SUBSx<V128W, V128, MV32, Float32VecOps>;
DEF_ISEL(SUBSS_XMMss_XMMss) = SUBSx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VSUBSS_XMMdq_XMMdq_MEMd) = SUBSx<VV128W, VV128, MV32, Float32VecOps>;)
IF_AVX(DEF_ISEL(VSUBSS_XMMdq_XMMdq_XMMd) = SUBSx<VV128W, VV128, VV128, Float32VecOps>;)

DEF_ISEL(SUBSD_XMMsd_MEMsd) = SUBSx<V128W, V128, MV64, Float64VecOps>;
DEF_ISEL(SUBSD_XMMsd_XMMsd) = SUBSx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VSUBSD_XMMdq_XMMdq_MEMq) = SUBSx<VV128W, VV128, MV64, Float64VecOps>;)
IF_AVX(DEF_ISEL(VSUBSD_XMMdq_XMMdq_XMMq) = SUBSx<VV128W, VV128, VV128, Float64VecOps>;)

namespace {

template <typename S1, typename S2>
DEF_SEM(CMP, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;
  const T src1 = R(src1_);
  const T src2 = R(src2_);
  const T res = src1 - src2;
  __remill_barrier_compiler();
  SetFlagsAddSub<tag_sub>(state, src1, src2, res);
}

}  // namespace

DEF_ISEL(CMP_MEMb_IMMb_80r7) = CMP<M8, I8>;
DEF_ISEL(CMP_GPR8_IMMb_80r7) = CMP<R8, I8>;
DEF_ISEL_Mn_In(CMP_MEMv_IMMz, CMP);
DEF_ISEL_Rn_In(CMP_GPRv_IMMz, CMP);
DEF_ISEL(CMP_MEMb_IMMb_82r7) = CMP<M8, I8>;
DEF_ISEL(CMP_GPR8_IMMb_82r7) = CMP<R8, I8>;
DEF_ISEL_Mn_In(CMP_MEMv_IMMb, CMP);
DEF_ISEL_Rn_In(CMP_GPRv_IMMb, CMP);
DEF_ISEL(CMP_MEMb_GPR8) = CMP<M8, I8>;
DEF_ISEL(CMP_GPR8_GPR8_38) = CMP<R8, R8>;
DEF_ISEL_Mn_In(CMP_MEMv_GPRv, CMP);
DEF_ISEL_Rn_Rn(CMP_GPRv_GPRv_39, CMP);
DEF_ISEL(CMP_GPR8_GPR8_3A) = CMP<R8, R8>;
DEF_ISEL(CMP_GPR8_MEMb) = CMP<R8, M8>;
DEF_ISEL_Rn_Rn(CMP_GPRv_GPRv_3B, CMP);
DEF_ISEL_Rn_Mn(CMP_GPRv_MEMv, CMP);
DEF_ISEL(CMP_AL_IMMb) = CMP<R8, I8>;
DEF_ISEL_Rn_In(CMP_OrAX_IMMz, CMP);

namespace {

template <typename T, typename U, typename V>
ALWAYS_INLINE void SetFlagsMul(State &state, T lhs, T rhs, U res, V res_trunc) {
  const auto new_of = Overflow<tag_mul>::Flag(lhs, rhs, res);

  state.aflag.cf = new_of;
  state.aflag.pf = __remill_undefined_bool();
  state.aflag.af = __remill_undefined_bool();
  state.aflag.zf = __remill_undefined_bool();
  state.aflag.sf = std::is_signed<T>::value ?
      SignFlag(res_trunc) :
      __remill_undefined_bool();
  state.aflag.of = new_of;
}

// Creates signed or unsigned multipliers. The `Converter` template template
// parameter is used to take on integer type and convert it to its signed
// counterpart.
template <template <typename> class Converter, size_t size=0>
struct DivMul {

  // 2-operand and 3-operand multipliers truncate their results down to their
  // base types.
  template <typename D, typename S1, typename S2>
  DEF_SEM(MUL, D dst, const S1 src1_, const S2 src2_) {
    typedef BASE_TYPE_OF(S1) T;
    typedef WIDEN_INTEGER_TYPE(T) WT;

    typedef typename Converter<T>::Type CT;
    typedef typename Converter<WT>::Type CWT;

    const auto src1 = static_cast<CT>(R(src1_));
    const auto src2 = static_cast<CT>(R(src2_));

    const auto src1_wide = static_cast<CWT>(src1);
    const auto src2_wide = static_cast<CWT>(src2);
    const auto res = static_cast<CWT>(src1_wide * src2_wide);
    const auto res_trunc = static_cast<CT>(res);

    W(dst) = static_cast<T>(res_trunc);
    __remill_barrier_compiler();
    SetFlagsMul(state, src1, src2, res, res_trunc);
  }

  // Unsigned multiply without affecting flags.
  template <typename D, typename S2>
  DEF_SEM(MULX, D dst1, D dst2, const S2 src2_) {
    typedef BASE_TYPE_OF(S2) T;
    typedef WIDEN_INTEGER_TYPE(T) WT;
    enum {
      kShiftSize = sizeof(T) * 8
    };

    const auto src2 = static_cast<WT>(R(src2_));
    const auto src1 = static_cast<WT>(R(state.gpr.rdx));
    const auto res = src1 * src2;
    W(dst1) = static_cast<T>(res >> kShiftSize);
    W(dst2) = static_cast<T>(res);
  }

  // `MUL8` and `IMUL8` of `AL` doesn't update `RDX`.
  template <typename S2>
  DEF_SEM(MULA_8, const S2 val) {
    typedef BASE_TYPE_OF(S2) T;  // 8 bit.
    typedef WIDEN_INTEGER_TYPE(T) WT;  // 16-bit.
    typedef typename Converter<T>::Type CT;
    typedef typename Converter<WT>::Type CWT;

    const auto src1 = static_cast<CT>(R(state.gpr.rax.byte.low));
    const auto src2 = static_cast<CT>(R(val));

    const auto src1_wide = static_cast<CWT>(src1);
    const auto src2_wide = static_cast<CWT>(src2);
    const auto res = static_cast<CWT>(src1_wide * src2_wide);
    const auto res_trunc = static_cast<CT>(res);

    W(state.gpr.rax.word) = static_cast<WT>(res);
    __remill_barrier_compiler();
    SetFlagsMul(state, src1, src2, res, res_trunc);
  }


#define MAKE_MULTIPLIER(size, read_sel, write_sel) \
  template <typename S2> \
  DEF_SEM(MULAD_ ## size, const S2 src2_) { \
    typedef BASE_TYPE_OF(S2) T; \
    typedef WIDEN_INTEGER_TYPE(T) WT; \
    typedef typename Converter<T>::Type CT; \
    typedef typename Converter<WT>::Type CWT; \
    \
    const auto src1 = static_cast<CT>(R(state.gpr.rax.read_sel)); \
    const auto src2 = static_cast<CT>(R(src2_)); \
    const auto src1_wide = static_cast<CWT>(src1); \
    const auto src2_wide = static_cast<CWT>(src2); \
    const auto res = static_cast<CWT>(src1_wide * src2_wide); \
    const auto res_trunc = static_cast<CT>(res); \
    \
    W(state.gpr.rax.write_sel) = static_cast<T>(res_trunc); \
    W(state.gpr.rdx.write_sel) = static_cast<T>(static_cast<WT>(res) >> size); \
    __remill_barrier_compiler(); \
    SetFlagsMul(state, src1, src2, res, res_trunc); \
  }

MAKE_MULTIPLIER(16, word, word)
MAKE_MULTIPLIER(32, dword, IF_64BIT_ELSE(qword, dword))
IF_64BIT(MAKE_MULTIPLIER(64, qword, qword))

#undef MAKE_MULTIPLIER

  // `DIV8` and `IDIV8` of `AL` doesn't update `RDX`.
  template <typename S2>
  DEF_SEM(DIVA_8, const S2 src2_) {

    typedef BASE_TYPE_OF(S2) T;
    typedef WIDEN_INTEGER_TYPE(T) WT;

    typedef typename Converter<T>::Type CT;
    typedef typename Converter<WT>::Type CWT;

    const auto src1 = static_cast<CWT>(R(state.gpr.rax.word));
    const CWT src2 = static_cast<CT>(R(src2_));

    const CWT quot = src1 / src2;

    if (quot != static_cast<CT>(quot)) {
      __remill_error(state, R(state.gpr.rip));
      __builtin_unreachable();
    }

    W(state.gpr.rax.byte.low) = static_cast<T>(quot);

    const CWT rem = src1 % src2;
    W(state.gpr.rax.byte.high) = static_cast<T>(rem);
    CLEAR_AFLAGS();
  }

#define MAKE_DIVIDER(size, read_sel, write_sel) \
    template <typename S2> \
    DEF_SEM(DIVA_ ## size, const S2 src2_) { \
      typedef BASE_TYPE_OF(S2) T; \
      typedef WIDEN_INTEGER_TYPE(T) WT; \
      \
      typedef typename Converter<T>::Type CT; \
      typedef typename Converter<WT>::Type CWT; \
      \
      const auto src1_low = static_cast<WT>(R(state.gpr.rax.read_sel)); \
      const auto src1_high = static_cast<WT>(R(state.gpr.rdx.read_sel)); \
      \
      const CWT src1 = static_cast<CWT>((src1_high << size) | src1_low);\
      const CWT src2 = static_cast<CT>(R(src2_)); \
      \
      const CWT quot = src1 / src2; \
      \
      if (quot != static_cast<CT>(quot)) { \
        __remill_error(state, R(state.gpr.rip)); \
        __builtin_unreachable(); \
      } \
      \
      W(state.gpr.rax.write_sel) = static_cast<T>(quot); \
      \
      const CWT rem = src1 % src2; \
      W(state.gpr.rdx.write_sel) = static_cast<T>(rem); \
      CLEAR_AFLAGS(); \
    }

MAKE_DIVIDER(16, word, word)
MAKE_DIVIDER(32, dword, IF_64BIT_ELSE(qword, dword))
IF_64BIT( MAKE_DIVIDER(64, qword, qword) )

#undef MAKE_DIVIDER
};

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(MULPx, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  T src1 = R(src1_);
  T src2 = R(src2_);
  W(dst) = FloatAcc::Read(src1) * FloatAcc::Read(src2);
}

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(MULSx, D dst, const S1 src1_, const S2 src2_) {
  auto src1 = R(src1_);
  auto src2 = R(src2_);
  FloatAcc::Write0(src1, FloatAcc::Read0(src1) * FloatAcc::Read0(src2));
  W(dst) = src1;
}
template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(DIVPx, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  T src1 = R(src1_);
  T src2 = R(src2_);
  W(dst) = FloatAcc::Read(src1) / FloatAcc::Read(src2);
}

template <typename D, typename S1, typename S2, typename FloatAcc>
DEF_SEM(DIVSx, D dst, const S1 src1_, const S2 src2_) {
  auto src1 = R(src1_);
  auto src2 = R(src2_);
  FloatAcc::Write0(src1, FloatAcc::Read0(src1) / FloatAcc::Read0(src2));
  W(dst) = src1;
}

}  // namespace

DEF_ISEL(IMUL_MEMb) = DivMul<SignedIntegerType>::MULA_8<M8>;
DEF_ISEL(IMUL_GPR8) = DivMul<SignedIntegerType>::MULA_8<R8>;
DEF_ISEL(IMUL_MEMv_8) = DivMul<SignedIntegerType>::MULA_8<M8>;
DEF_ISEL(IMUL_MEMv_16) = DivMul<SignedIntegerType>::MULAD_16<M16>;
DEF_ISEL(IMUL_MEMv_32) = DivMul<SignedIntegerType>::MULAD_32<M32>;
IF_64BIT(DEF_ISEL(IMUL_MEMv_64) = DivMul<SignedIntegerType>::MULAD_64<M64>;)
DEF_ISEL(IMUL_GPRv_8) = DivMul<SignedIntegerType>::MULA_8<R8>;
DEF_ISEL(IMUL_GPRv_16) = DivMul<SignedIntegerType>::MULAD_16<R16>;
DEF_ISEL(IMUL_GPRv_32) = DivMul<SignedIntegerType>::MULAD_32<R32>;
IF_64BIT(DEF_ISEL(IMUL_GPRv_64) = DivMul<SignedIntegerType>::MULAD_64<R64>;)

// All dests are registers, albeit different ones from the sources.
DEF_ISEL_RnW_Mn_In(IMUL_GPRv_MEMv_IMMz, DivMul<SignedIntegerType>::MUL);
DEF_ISEL_RnW_Rn_In(IMUL_GPRv_GPRv_IMMz, DivMul<SignedIntegerType>::MUL);
DEF_ISEL_RnW_Mn_In(IMUL_GPRv_MEMv_IMMb, DivMul<SignedIntegerType>::MUL);
DEF_ISEL_RnW_Rn_In(IMUL_GPRv_GPRv_IMMb, DivMul<SignedIntegerType>::MUL);

// Two-operand, but dest is a register so turns into a three-operand.
DEF_ISEL_RnW_Rn_Mn(IMUL_GPRv_MEMv, DivMul<SignedIntegerType>::MUL);
DEF_ISEL_RnW_Rn_Rn(IMUL_GPRv_GPRv, DivMul<SignedIntegerType>::MUL);

DEF_ISEL(MUL_GPR8) = DivMul<UnsignedIntegerType>::MULA_8<R8>;
DEF_ISEL(MUL_MEMb) = DivMul<UnsignedIntegerType>::MULA_8<M8>;
DEF_ISEL(MUL_MEMv_8) = DivMul<UnsignedIntegerType>::MULA_8<M8>;
DEF_ISEL(MUL_MEMv_16) = DivMul<UnsignedIntegerType>::MULAD_16<M16>;
DEF_ISEL(MUL_MEMv_32) = DivMul<UnsignedIntegerType>::MULAD_32<M32>;
IF_64BIT(DEF_ISEL(MUL_MEMv_64) = DivMul<UnsignedIntegerType>::MULAD_64<M64>;)
DEF_ISEL(MUL_GPRv_8) = DivMul<UnsignedIntegerType>::MULA_8<R8>;
DEF_ISEL(MUL_GPRv_16) = DivMul<UnsignedIntegerType>::MULAD_16<R16>;
DEF_ISEL(MUL_GPRv_32) = DivMul<UnsignedIntegerType>::MULAD_32<R32>;
IF_64BIT(DEF_ISEL(MUL_GPRv_64) = DivMul<UnsignedIntegerType>::MULAD_64<R64>;)

DEF_ISEL(MULX_VGPR32d_VGPR32d_VGPR32d) =
    DivMul<UnsignedIntegerType>::MULX<R32W, R32>;
DEF_ISEL(MULX_VGPR32d_VGPR32d_MEMd) =
    DivMul<UnsignedIntegerType>::MULX<R32W, M32>;
IF_64BIT(DEF_ISEL(MULX_VGPR64q_VGPR64q_VGPR64q) =
    DivMul<UnsignedIntegerType>::MULX<R64W, R64>;)
IF_64BIT(DEF_ISEL(MULX_VGPR64q_VGPR64q_MEMq) =
    DivMul<UnsignedIntegerType>::MULX<R64W, M64>;)

DEF_ISEL(MULPS_XMMps_MEMps) = MULPx<V128W, V128, MV128, Float32VecOps>;
DEF_ISEL(MULPS_XMMps_XMMps) = MULPx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VMULPS_XMMdq_XMMdq_MEMdq) = MULPx<VV128W, VV128, MV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VMULPS_XMMdq_XMMdq_XMMdq) = MULPx<VV128W, VV128, VV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VMULPS_YMMqq_YMMqq_MEMqq) = MULPx<VV256W, VV256, MV256, Float32VecOps>;)
IF_AVX(DEF_ISEL(VMULPS_YMMqq_YMMqq_YMMqq) = MULPx<VV256W, VV256, VV256, Float32VecOps>;)

DEF_ISEL(MULPD_XMMpd_MEMpd) = MULPx<V128W, V128, MV128, Float64VecOps>;
DEF_ISEL(MULPD_XMMpd_XMMpd) = MULPx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VMULPD_XMMdq_XMMdq_MEMdq) = MULPx<VV128W, VV128, MV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VMULPD_XMMdq_XMMdq_XMMdq) = MULPx<VV128W, VV128, VV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VMULPD_YMMqq_YMMqq_MEMqq) = MULPx<VV256W, VV256, MV256, Float64VecOps>;)
IF_AVX(DEF_ISEL(VMULPD_YMMqq_YMMqq_YMMqq) = MULPx<VV256W, VV256, VV256, Float64VecOps>;)

DEF_ISEL(MULSS_XMMss_MEMss) = MULSx<V128W, V128, MV128, Float32VecOps>;
DEF_ISEL(MULSS_XMMss_XMMss) = MULSx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VMULSS_XMMdq_XMMdq_MEMd) = MULSx<VV128W, VV128, MV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VMULSS_XMMdq_XMMdq_XMMd) = MULSx<VV128W, VV128, VV128, Float32VecOps>;)

DEF_ISEL(MULSD_XMMsd_MEMsd) = MULSx<V128W, V128, MV128, Float64VecOps>;
DEF_ISEL(MULSD_XMMsd_XMMsd) = MULSx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VMULSD_XMMdq_XMMdq_MEMq) = MULSx<VV128W, VV128, MV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VMULSD_XMMdq_XMMdq_XMMq) = MULSx<VV128W, VV128, VV128, Float64VecOps>;)

DEF_ISEL(IDIV_MEMb) = DivMul<SignedIntegerType>::DIVA_8<M8>;
DEF_ISEL(IDIV_GPR8) = DivMul<SignedIntegerType>::DIVA_8<R8>;
DEF_ISEL(IDIV_MEMv_8) = DivMul<SignedIntegerType>::DIVA_8<M8>;
DEF_ISEL(IDIV_MEMv_16) = DivMul<SignedIntegerType>::DIVA_16<M16>;
DEF_ISEL(IDIV_MEMv_32) = DivMul<SignedIntegerType>::DIVA_32<M32>;
IF_64BIT(DEF_ISEL(IDIV_MEMv_64) = DivMul<SignedIntegerType>::DIVA_64<M64>;)
DEF_ISEL(IDIV_GPRv_8) = DivMul<SignedIntegerType>::DIVA_8<R8>;
DEF_ISEL(IDIV_GPRv_16) = DivMul<SignedIntegerType>::DIVA_16<R16>;
DEF_ISEL(IDIV_GPRv_32) = DivMul<SignedIntegerType>::DIVA_32<R32>;
IF_64BIT(DEF_ISEL(IDIV_GPRv_64) = DivMul<SignedIntegerType>::DIVA_64<R64>;)

DEF_ISEL(DIV_MEMb) = DivMul<UnsignedIntegerType>::DIVA_8<M8>;
DEF_ISEL(DIV_GPR8) = DivMul<UnsignedIntegerType>::DIVA_8<R8>;
DEF_ISEL(DIV_MEMv_8) = DivMul<UnsignedIntegerType>::DIVA_8<M8>;
DEF_ISEL(DIV_MEMv_16) = DivMul<UnsignedIntegerType>::DIVA_16<M16>;
DEF_ISEL(DIV_MEMv_32) = DivMul<UnsignedIntegerType>::DIVA_32<M32>;
IF_64BIT(DEF_ISEL(DIV_MEMv_64) = DivMul<UnsignedIntegerType>::DIVA_64<M64>;)
DEF_ISEL(DIV_GPRv_8) = DivMul<UnsignedIntegerType>::DIVA_8<R8>;
DEF_ISEL(DIV_GPRv_16) = DivMul<UnsignedIntegerType>::DIVA_16<R16>;
DEF_ISEL(DIV_GPRv_32) = DivMul<UnsignedIntegerType>::DIVA_32<R32>;
IF_64BIT(DEF_ISEL(DIV_GPRv_64) = DivMul<UnsignedIntegerType>::DIVA_64<R64>;)

DEF_ISEL(DIVPS_XMMps_MEMps) = DIVPx<V128W, V128, MV128, Float32VecOps>;
DEF_ISEL(DIVPS_XMMps_XMMps) = DIVPx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VDIVPS_XMMdq_XMMdq_MEMdq) = DIVPx<VV128W, VV128, MV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VDIVPS_XMMdq_XMMdq_XMMdq) = DIVPx<VV128W, VV128, VV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VDIVPS_YMMqq_YMMqq_MEMqq) = DIVPx<VV256W, VV256, MV256, Float32VecOps>;)
IF_AVX(DEF_ISEL(VDIVPS_YMMqq_YMMqq_YMMqq) = DIVPx<VV256W, VV256, VV256, Float32VecOps>;)

DEF_ISEL(DIVPD_XMMpd_MEMpd) = DIVPx<V128W, V128, MV128, Float64VecOps>;
DEF_ISEL(DIVPD_XMMpd_XMMpd) = DIVPx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VDIVPD_XMMdq_XMMdq_MEMdq) = DIVPx<VV128W, VV128, MV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VDIVPD_XMMdq_XMMdq_XMMdq) = DIVPx<VV128W, VV128, VV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VDIVPD_YMMqq_YMMqq_MEMqq) = DIVPx<VV256W, VV256, MV256, Float64VecOps>;)
IF_AVX(DEF_ISEL(VDIVPD_YMMqq_YMMqq_YMMqq) = DIVPx<VV256W, VV256, VV256, Float64VecOps>;)

DEF_ISEL(DIVSS_XMMss_MEMss) = DIVSx<V128W, V128, MV128, Float32VecOps>;
DEF_ISEL(DIVSS_XMMss_XMMss) = DIVSx<V128W, V128, V128, Float32VecOps>;
IF_AVX(DEF_ISEL(VDIVSS_XMMdq_XMMdq_MEMd) = DIVSx<VV128W, VV128, MV128, Float32VecOps>;)
IF_AVX(DEF_ISEL(VDIVSS_XMMdq_XMMdq_XMMd) = DIVSx<VV128W, VV128, VV128, Float32VecOps>;)

DEF_ISEL(DIVSD_XMMsd_MEMsd) = DIVSx<V128W, V128, MV128, Float64VecOps>;
DEF_ISEL(DIVSD_XMMsd_XMMsd) = DIVSx<V128W, V128, V128, Float64VecOps>;
IF_AVX(DEF_ISEL(VDIVSD_XMMdq_XMMdq_MEMq) = DIVSx<VV128W, VV128, MV128, Float64VecOps>;)
IF_AVX(DEF_ISEL(VDIVSD_XMMdq_XMMdq_XMMq) = DIVSx<VV128W, VV128, VV128, Float64VecOps>;)

namespace {

template <typename D, typename S1>
DEF_SEM(INC, D dst, const S1 src) {
  typedef BASE_TYPE_OF(S1) T;
  const T val1 = R(src);
  const T val2 = 1;
  const T res = val1 + val2;
  W(dst) = res;
  __remill_barrier_compiler();
  SetFlagsIncDec<tag_add>(state, val1, val2, res);
}

template <typename D, typename S1>
DEF_SEM(DEC, D dst, const S1 src) {
  typedef BASE_TYPE_OF(S1) T;
  const T val1 = R(src);
  const T val2 = 1;
  const T res = val1 - val2;
  W(dst) = res;
  __remill_barrier_compiler();
  SetFlagsIncDec<tag_sub>(state, val1, val2, res);
}

template <typename D, typename S1>
DEF_SEM(NEG, D dst, const S1 src) {
  typedef BASE_TYPE_OF(S1) T;
  typedef TO_SIGNED_INTEGER_TYPE(T) ST;
  const auto val = R(src);
  const auto res = static_cast<T>(-static_cast<ST>(val));
  W(dst) = res;
  __remill_barrier_compiler();
  state.aflag.cf = NotZeroFlag(val);
  SetFlagsIncDec<tag_sub, T>(state, 0, val, res);
}

}  // namespace

DEF_ISEL(INC_MEMb) = INC<M8W, M8>;
DEF_ISEL(INC_GPR8) = INC<R8W, R8>;
DEF_ISEL_MnW_Mn(INC_MEMv, INC);
DEF_ISEL_RnW_Rn(INC_GPRv_FFr0, INC);
DEF_ISEL_RnW_Rn(INC_GPRv_40, INC);

DEF_ISEL(DEC_MEMb) = DEC<M8W, M8>;
DEF_ISEL(DEC_GPR8) = DEC<R8W, R8>;
DEF_ISEL_MnW_Mn(DEC_MEMv, DEC);
DEF_ISEL_RnW_Rn(DEC_GPRv_FFr1, DEC);
DEF_ISEL_RnW_Rn(DEC_GPRv_48, DEC);

DEF_ISEL(NEG_MEMb) = NEG<M8W, M8>;
DEF_ISEL(NEG_GPR8) = NEG<R8W, R8>;
DEF_ISEL_MnW_Mn(NEG_MEMv, NEG);
DEF_ISEL_RnW_Rn(NEG_GPRv, NEG);

namespace {

template <typename TagT, typename T>
NEVER_INLINE static bool CarryFlag(T a, T b, T ab, T c, T abc) {
  static_assert(std::is_unsigned<T>::value,
                "Invalid specialization of `CarryFlag` for addition.");
  __remill_defer_inlining();
  return Carry<TagT>::Flag(a, b, ab) || Carry<TagT>::Flag(ab, c, abc);
}

template <typename D, typename S1, typename S2>
DEF_SEM(ADC, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  const auto src1 = R(src1_);
  const auto src2 = R(src2_);
  const auto carry = static_cast<T>(state.aflag.cf);
  const T res_add = src1 + src2;
  const T res = res_add + carry;
  W(dst) = res;
  __remill_barrier_compiler();
  state.aflag.cf = CarryFlag<tag_add>(src1, src2, res_add, carry, res);
  SetFlagsIncDec<tag_add>(state, src1, src2, res);
}

template <typename D, typename S1, typename S2>
DEF_SEM(SBB, D dst, const S1 src1_, const S2 src2_) {
  typedef BASE_TYPE_OF(S1) T;  // `D` might be wider than `S1`.
  const T src1 = R(src1_);
  const T src2 = R(src2_);
  const T borrow = static_cast<T>(state.aflag.cf);
  const T res_sub = src1 - src2;
  const T res = res_sub - borrow;
  W(dst) = res;
  __remill_barrier_compiler();
  state.aflag.cf = CarryFlag<tag_sub>(src1, src2, res_sub, borrow, res);
  SetFlagsIncDec<tag_sub>(state, src1, src2, res);
}

}  // namespace

DEF_ISEL(SBB_MEMb_IMMb_80r3) = SBB<M8W, M8, I8>;
DEF_ISEL(SBB_GPR8_IMMb_80r3) = SBB<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(SBB_MEMv_IMMz, SBB);
DEF_ISEL_RnW_Rn_In(SBB_GPRv_IMMz, SBB);
DEF_ISEL(SBB_MEMb_IMMb_82r3) = SBB<M8W, M8, I8>;
DEF_ISEL(SBB_GPR8_IMMb_82r3) = SBB<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(SBB_MEMv_IMMb, SBB);
DEF_ISEL_RnW_Rn_In(SBB_GPRv_IMMb, SBB);
DEF_ISEL(SBB_MEMb_GPR8) = SBB<M8W, M8, I8>;
DEF_ISEL(SBB_GPR8_GPR8_18) = SBB<R8W, R8, R8>;
DEF_ISEL_MnW_Mn_Rn(SBB_MEMv_GPRv, SBB);
DEF_ISEL_RnW_Rn_Rn(SBB_GPRv_GPRv_19, SBB);
DEF_ISEL(SBB_GPR8_GPR8_1A) = SBB<R8W, R8, R8>;
DEF_ISEL(SBB_GPR8_MEMb) = SBB<R8W, R8, M8>;
DEF_ISEL_RnW_Rn_Rn(SBB_GPRv_GPRv_1B, SBB);
DEF_ISEL_RnW_Rn_Mn(SBB_GPRv_MEMv, SBB);
DEF_ISEL(SBB_AL_IMMb) = SBB<R8W, R8, I8>;
DEF_ISEL_RnW_Rn_In(SBB_OrAX_IMMz, SBB);

DEF_ISEL(ADC_MEMb_IMMb_80r2) = ADC<M8W, M8, I8>;
DEF_ISEL(ADC_GPR8_IMMb_80r2) = ADC<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(ADC_MEMv_IMMz, ADC);
DEF_ISEL_RnW_Rn_In(ADC_GPRv_IMMz, ADC);
DEF_ISEL(ADC_MEMb_IMMb_82r2) = ADC<M8W, M8, I8>;
DEF_ISEL(ADC_GPR8_IMMb_82r2) = ADC<R8W, R8, I8>;
DEF_ISEL_MnW_Mn_In(ADC_MEMv_IMMb, ADC);
DEF_ISEL_RnW_Rn_In(ADC_GPRv_IMMb, ADC);
DEF_ISEL(ADC_MEMb_GPR8) = ADC<M8W, M8, R8>;
DEF_ISEL(ADC_GPR8_GPR8_10) = ADC<R8W, R8, R8>;
DEF_ISEL_MnW_Mn_Rn(ADC_MEMv_GPRv, ADC);
DEF_ISEL_RnW_Rn_Rn(ADC_GPRv_GPRv_11, ADC);
DEF_ISEL(ADC_GPR8_MEMb) = ADC<R8W, R8, M8>;
DEF_ISEL(ADC_GPR8_GPR8_12) = ADC<R8W, R8, R8>;
DEF_ISEL_RnW_Rn_Mn(ADC_GPRv_MEMv, ADC);
DEF_ISEL_RnW_Rn_Rn(ADC_GPRv_GPRv_13, ADC);
DEF_ISEL(ADC_AL_IMMb) = ADC<R8W, R8, I8>;
DEF_ISEL_RnW_Rn_In(ADC_OrAX_IMMz, ADC);

#endif  // REMILL_ARCH_X86_SEMANTICS_BINARY_H_
