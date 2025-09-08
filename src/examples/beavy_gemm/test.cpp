#include <iostream>

int main() {
    // Write C++ code here
    unsigned long dx1, dx0, Dx, dy1, dy0, Dy, ans1, ans2, ans3, ans, x0,x1,y0,y1, ot0,ot1;
    unsigned long  DxDy, Dxdy0, Dxdy1, dx0Dy, dx0dy0,dx0dy1, dx1Dy, dx1dy0, dx1dy1;
    x0 = 6109786543343439470UL; 
    x1 = 12336957530366153106UL;
    y0 = 172445982830849961UL;
    y1 = 18274298090878750807UL;
    std::cout << "X from arith : " << ((x0+x1)>>13) << "\n";
    std::cout << "Y from arith: " << ((y0+y1)>>13) << "\n";

    Dx = 5236063519633194659UL;
    dx0 = 11346891857613738147UL;
    dx1 = 12335915735728967168UL;
    std::cout << "X from ABY : " << ((Dx - dx0 - dx1)>>13) << "\n";

    Dy = 3635283466441938275UL;
    dy0 = 18379453835119198102UL;
    dy1 = 3702573705032242637UL;
    std::cout << "Y from ABY : " << ((Dy - dy0 - dy1)>>13) << "\n";
 
    ot0 = 3486317951023105358UL;
    ot1 = 11359466140460277817UL;

    DxDy = Dx * Dy;
    Dxdy0 = Dx * dy0;
    Dxdy1 = Dx * dy1;
    dx0Dy = dx0 * Dy;
    dx0dy0 = dx0 * dy0;
    dx0dy1 = dx0 * dy1;
    dx1Dy = dx1 * Dy;
    dx1dy0 = dx1 * dy0;
    dx1dy1 = dx1 * dy1;
    
    std::cout << "--------@ Party 0 ------------------\n";
    std::cout << " dx0dy0 " << dx0dy0 << "\n";
    std::cout << " DxDy : " << DxDy << "\n";
    std::cout << " Dxdy0 : " << Dxdy0<< "\n";
    std::cout << " dx0Dy : " << dx0Dy<< "\n";
    auto a3 = dx0dy0 + ot0 + DxDy -Dxdy0 - dx0Dy;
    std::cout << "@P0, dx0dy0 + ot0 + DxDy -Dxdy0 - dx0Dy : " << a3 <<"\n\n\n";

    std::cout << "--------@ Party 1 ------------------\n";
    std::cout << " dx1dy1 " << dx1dy1 << "\n";
    std::cout << " DxDy : " << DxDy << "\n";
    std::cout << " Dxdy1 : " << Dxdy1  << "\n";
    std::cout << " dx1Dy : " << dx1Dy << "\n";
    auto t = 2112389789972505205UL - DxDy;
    auto a6 = dx1dy1 + ot1 - Dxdy1 - dx1Dy;
    std::cout << "dx1dy1 + ot1 - Dxdy1 - dx1Dy1 : " << a6 << "\n";
     
    auto ans7 = a3 + a6 ;

    std::cout << "--------@ Party 2 ------------------\n";
    std::cout << " dx1dy0 : " << dx1dy0<< "\n";
    std::cout << " dx0dy1 : " << dx0dy1 << "\n";
    std::cout << "dx1dy0 + dx0dy1 : " << dx1dy0 + dx0dy1 << "\n";
    unsigned long a = 18446618493884963886UL;
    unsigned long b = 125579824833489UL;

    std ::cout << "Before decoding : " << a+b << "\n";
    std ::cout << "After decoding : " << float (a+b)/8192 << "\n";
    std::cout << "\n\n@ both P0 and P1 " << (ans7>>26) << "\n";

    ans = DxDy - Dxdy0 - Dxdy1 - dx0Dy + dx0dy0- dx1Dy + dx1dy1 + ot0+ ot1;//DxDy - Dxdy0 - Dxdy1 - dx0Dy + dx0dy0- dx1Dy  + dx1dy0 + dx1dy1 + dx0dy1;
    ans1 = DxDy - Dxdy0 - dx0Dy + dx0dy0;
    ans2 = - Dxdy1- dx1Dy+dx1dy1;//dx1dy1 - Dxdy1- dx1Dy;
    ans3 = ot0 + ot1;
    

    std::cout << " P0 + P1 +P2 : "<< ((ans1 + ans2 + ans3)>>26) << "\n";
    
    std::cout << " DxDy - Dxdy0 - Dxdy1 - dx0Dy + dx0dy0 + dx0dy1 - dx1Dy  + dx1dy0 + dx1dy1 : "<< (ans>>26) << "\n";
    std :: cout << " Debugging with DxDy/2";
    auto t1 = dx0dy0 + ot0 + (DxDy>>1) -Dxdy0 - dx0Dy;
    auto t2 = dx1dy1 + ot1 + (DxDy>>1)- Dxdy1 - dx1Dy;

    std:: cout << "t1 + t2 = " << ((t1+t2)>>26) << "\n";
    std:: cout << " 2 X 2 Matrix \n";
    unsigned long a1 = 478714551182825UL;
    unsigned long a2 = 191022722643191UL;
     a3 = 18446113990195520541UL;
    unsigned long a4 = 937599004189835UL;

    unsigned long b1 = 1099870793398642UL;
    unsigned long b2 = 18446087414567164769UL;
    unsigned long b3 = 987580303057575UL;
    unsigned long b4 = 431915405635184UL;

    std :: cout << a1+b1 << "  , " <<  a2+b2 << "  , " <<  a3+b3 << "  , " <<  a4+b4 << "  , ";
    return 0;
}