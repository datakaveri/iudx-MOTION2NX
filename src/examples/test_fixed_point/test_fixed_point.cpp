// MIT License
//
// Copyright (c) 2020 Lennart Braun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cstdint>
#include "utility/gtest.h"

#include "utility/fixed_point.h"

namespace fp = MOTION::fixed_point;

int main(int argc, char *argv[])
{
  const double g = 0.05;
  std::cout << "\n Input number : ";
  std::cout << g;
  const auto enc_g = fp::encode<std::uint64_t, double>(g, 13);
  std::cout << "\n Encoded in main : ";
  std::cout << enc_g;
  const uint64_t k = 1;
  const auto dec_g = fp::decode<std::uint64_t, double>(enc_g, 13);
  std::cout << "\n Decoded in main : ";
  std::cout << dec_g;
  // std::cout << "\n Encode "<< k << "Decode ";
  // std::cout << fp::decode<std::uint64_t, double>(k, 13);
  return 0;
}