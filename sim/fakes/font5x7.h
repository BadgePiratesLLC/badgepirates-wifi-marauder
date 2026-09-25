#pragma once
// BSidesKC Badge simulator — tiny 5x7 dot-matrix font.
//
// Hand-authored (not a vendored table) so screenshots have legible text
// without pulling in a real font renderer. Covers space, 0-9, A-Z and the
// punctuation the badge UI actually uses. Lowercase input is upper-cased
// before lookup — the sim does not distinguish case, which is exactly the
// kind of "does not cover real rendering" limit sim/README.md calls out.

#include <cstdint>
#include <cctype>

#define B5(a,b,c,d,e) (uint8_t)(((a)<<4)|((b)<<3)|((c)<<2)|((d)<<1)|(e))

inline const uint8_t* font5x7_glyph(char c) {
  static const uint8_t SPACE[7] = {0,0,0,0,0,0,0};
  static const uint8_t BANG[7]  = {B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),0,B5(0,0,1,0,0)};
  static const uint8_t PCT[7]   = {B5(1,0,0,0,1),B5(1,0,0,1,0),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,1,0,0,0),B5(1,0,0,1,0),B5(1,0,0,0,1)};
  static const uint8_t PLUS[7]  = {0,B5(0,0,1,0,0),B5(0,0,1,0,0),B5(1,1,1,1,1),B5(0,0,1,0,0),B5(0,0,1,0,0),0};
  static const uint8_t DASH[7]  = {0,0,0,B5(1,1,1,1,1),0,0,0};
  static const uint8_t DOT[7]   = {0,0,0,0,0,0,B5(0,1,1,0,0)};
  static const uint8_t SLASH[7] = {B5(0,0,0,0,1),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,1,0,0,0),B5(1,0,0,0,0),0,0};
  static const uint8_t D0[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,1,1),B5(1,0,1,0,1),B5(1,1,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t D1[7] = {B5(0,0,1,0,0),B5(0,1,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,1,1,1,0)};
  static const uint8_t D2[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(0,0,0,0,1),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,1,0,0,0),B5(1,1,1,1,1)};
  static const uint8_t D3[7] = {B5(1,1,1,1,1),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,0,0,1,0),B5(0,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t D4[7] = {B5(0,0,0,1,0),B5(0,0,1,1,0),B5(0,1,0,1,0),B5(1,0,0,1,0),B5(1,1,1,1,1),B5(0,0,0,1,0),B5(0,0,0,1,0)};
  static const uint8_t D5[7] = {B5(1,1,1,1,1),B5(1,0,0,0,0),B5(1,1,1,1,0),B5(0,0,0,0,1),B5(0,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t D6[7] = {B5(0,0,1,1,0),B5(0,1,0,0,0),B5(1,0,0,0,0),B5(1,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t D7[7] = {B5(1,1,1,1,1),B5(0,0,0,0,1),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0)};
  static const uint8_t D8[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t D9[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,1),B5(0,0,0,0,1),B5(0,0,0,1,0),B5(0,1,1,0,0)};
  static const uint8_t COLON[7] = {0,B5(0,1,1,0,0),B5(0,1,1,0,0),0,B5(0,1,1,0,0),B5(0,1,1,0,0),0};
  static const uint8_t LT[7] = {B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,1,0,0,0),B5(1,0,0,0,0),B5(0,1,0,0,0),B5(0,0,1,0,0),B5(0,0,0,1,0)};
  static const uint8_t EQ[7] = {0,0,B5(1,1,1,1,1),0,B5(1,1,1,1,1),0,0};
  static const uint8_t GT[7] = {B5(1,0,0,0,0),B5(0,1,0,0,0),B5(0,0,1,0,0),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,1,0,0,0),B5(1,0,0,0,0)};
  static const uint8_t A[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1)};
  static const uint8_t Bc[7] = {B5(1,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,0)};
  static const uint8_t C[7] = {B5(0,1,1,1,1),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(0,1,1,1,1)};
  static const uint8_t D[7] = {B5(1,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,0)};
  static const uint8_t E[7] = {B5(1,1,1,1,1),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,1,1,1,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,1,1,1,1)};
  static const uint8_t F[7] = {B5(1,1,1,1,1),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,1,1,1,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0)};
  static const uint8_t G[7] = {B5(0,1,1,1,1),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,1,1,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,1)};
  static const uint8_t H[7] = {B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1)};
  static const uint8_t I[7] = {B5(0,1,1,1,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,1,1,1,0)};
  static const uint8_t J[7] = {B5(0,0,1,1,1),B5(0,0,0,1,0),B5(0,0,0,1,0),B5(0,0,0,1,0),B5(0,0,0,1,0),B5(1,0,0,1,0),B5(0,1,1,0,0)};
  static const uint8_t K[7] = {B5(1,0,0,0,1),B5(1,0,0,1,0),B5(1,0,1,0,0),B5(1,1,0,0,0),B5(1,0,1,0,0),B5(1,0,0,1,0),B5(1,0,0,0,1)};
  static const uint8_t L[7] = {B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,1,1,1,1)};
  static const uint8_t M[7] = {B5(1,0,0,0,1),B5(1,1,0,1,1),B5(1,0,1,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1)};
  static const uint8_t N[7] = {B5(1,0,0,0,1),B5(1,1,0,0,1),B5(1,0,1,0,1),B5(1,0,0,1,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1)};
  static const uint8_t O[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t P[7] = {B5(1,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,0),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(1,0,0,0,0)};
  static const uint8_t Q[7] = {B5(0,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,1,0,1),B5(1,0,0,1,0),B5(0,1,1,0,1)};
  static const uint8_t R[7] = {B5(1,1,1,1,0),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,1,1,1,0),B5(1,0,1,0,0),B5(1,0,0,1,0),B5(1,0,0,0,1)};
  static const uint8_t S[7] = {B5(0,1,1,1,1),B5(1,0,0,0,0),B5(1,0,0,0,0),B5(0,1,1,1,0),B5(0,0,0,0,1),B5(0,0,0,0,1),B5(1,1,1,1,0)};
  static const uint8_t T[7] = {B5(1,1,1,1,1),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0)};
  static const uint8_t U[7] = {B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,1,1,0)};
  static const uint8_t V[7] = {B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(0,1,0,1,0),B5(0,0,1,0,0)};
  static const uint8_t W[7] = {B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,0,0,1),B5(1,0,1,0,1),B5(1,0,1,0,1),B5(1,1,0,1,1),B5(1,0,0,0,1)};
  static const uint8_t X[7] = {B5(1,0,0,0,1),B5(0,1,0,1,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,1,0,1,0),B5(1,0,0,0,1)};
  static const uint8_t Y[7] = {B5(1,0,0,0,1),B5(0,1,0,1,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0),B5(0,0,1,0,0)};
  static const uint8_t Z[7] = {B5(1,1,1,1,1),B5(0,0,0,0,1),B5(0,0,0,1,0),B5(0,0,1,0,0),B5(0,1,0,0,0),B5(1,0,0,0,0),B5(1,1,1,1,1)};

  char u = (char)std::toupper((unsigned char)c);
  switch (u) {
    case '!': return BANG;
    case '%': return PCT;
    case '+': return PLUS;
    case '-': return DASH;
    case '.': return DOT;
    case '/': return SLASH;
    case '0': return D0; case '1': return D1; case '2': return D2;
    case '3': return D3; case '4': return D4; case '5': return D5;
    case '6': return D6; case '7': return D7; case '8': return D8;
    case '9': return D9;
    case ':': return COLON;
    case '<': return LT;
    case '=': return EQ;
    case '>': return GT;
    case 'A': return A; case 'B': return Bc; case 'C': return C;
    case 'D': return D; case 'E': return E; case 'F': return F;
    case 'G': return G; case 'H': return H; case 'I': return I;
    case 'J': return J; case 'K': return K; case 'L': return L;
    case 'M': return M; case 'N': return N; case 'O': return O;
    case 'P': return P; case 'Q': return Q; case 'R': return R;
    case 'S': return S; case 'T': return T; case 'U': return U;
    case 'V': return V; case 'W': return W; case 'X': return X;
    case 'Y': return Y; case 'Z': return Z;
    default: return SPACE;
  }
}

#undef B5
