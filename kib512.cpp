#include <iostream>
#include <stdint.h>
#include <cmath>
#include <bitset>
#include <string>
#include <sstream>
#include <iomanip>
#include <bit>
#include <assert.h>

// use little-endian since it is the most common and will lead to less
// conversions making it more efficient
// try padding with ones instead of zeros for the manip_m matrix or with 0x3030303030303030ULL

// algorithm supports proper diffusion

#if !defined(UINT8_MAX)
    using uint8_t = unsigned char
#elif !defined(UINT32_MAX)
    using uint32_t = unsigned int
#elif !defined(UINT64_MAX)
    using uint64_t = unsigned long long
#endif

inline uint64_t rr(uint64_t x, unsigned int n) {
    return (x >> n)|(x << ((sizeof(x)<<3)-n));
}

inline uint64_t lr(uint64_t x, unsigned int n) {
    return (x << n)|(x >> ((sizeof(x)<<3)-n));
}

inline std::string bin(uint64_t x) {
    return std::bitset<64>(x).to_string();
}

// pre-processing of kib512
class Kib512 {
    
    public:
    uint64_t m_ch;
    static constexpr uint64_t const const_m[8][8] = {
        {0x159a300c24f5dc1eULL, 0x178f1324ac498bfcULL, 0x10e55f6926814653ULL,
         0x1963cf80d6276cccULL,0x10980aa9695d807aULL, 0x1703f56bbe1f7f5eULL,
         0x16eb052c4abc81feULL, 0x1f190bb16583b88fULL}, {0x1d6d15eb483d1a05ULL,
         0x1e2c7d2c722534b7ULL, 0x14dcbefdb2afe52dULL, 0xf5fbc705a80df745ULL,
         0xe1803d026daf382fULL, 0x4817a3bcb90fa1deULL, 0x3b8d9167a61165bdULL,
         0x400b71315483aecfULL}, {0x7e253caa1049536cULL, 0x7143a810657de4e0ULL,
         0xba3d16917c91aae3ULL, 0x249b4da86d970a00ULL, 0x290ffdedae0a1c66ULL,
         0x8d490ccfccd45ebbULL, 0x1852f3c77fae1cb7ULL, 0x8f078d043c3be329ULL},
        {0x568d492cc5140ae4ULL, 0x1cd5bbf83190c484ULL, 0x99d177432a40d119ULL,
         0x1f21998dc8c9145eULL, 0x560d95c1d691bf3fULL, 0x57131d9cde1633e5ULL,
         0xcccd035627a53b51ULL, 0x13d653d8c3ae0192ULL}, {0x5d1e80c0123206a1ULL,
         0x91b1b3f312ac1be8ULL, 0x19bc3eed09a866a2ULL, 0x29ed711bc0303fb1ULL,
         0x35fa6009ed810bd6ULL, 0x69a90fb1b95d63aeULL, 0x38bedd43ad759c34ULL,
         0xa787e2edc8133e84ULL}, {0x552c119614a1f4e5ULL, 0x68d39af3ec9b831fULL,
         0x3996ac49f898e965ULL, 0x8d79f80825eb64f1ULL, 0x5babe19fdfe4ec70ULL, 
         0x7aa6a18653c9a0ccULL, 0x510e3b69d44d759aULL, 0x3486650c8e18c529ULL},
        {0x2b2b4051acc6e9a5ULL, 0x49a9817e3d56ff74ULL, 0xc6fd82e30671aeb5ULL,
         0x20be895e589409fdULL, 0xd8adec17910a1d4fULL, 0x18882870301f5a0bULL,
         0x225cb56188423cf3ULL, 0x4b61cc25258ea5c8ULL}, {0x63f68eef219e6c01ULL,
         0x7c29a354966d3f55ULL, 0x1cf4c5f3b98ce4deULL, 0x1476b731259de5f8ULL, 
         0x73642b2ec6c2a44bULL, 0x407adff58f6e054cULL, 0x811f7cbc6a7e08c7ULL,
         0x277c0323eb636b90ULL}
    };
    
    mutable uint64_t hash[8] {
        0x5287173768046659ULL, 0x1388c1a81885db29ULL, 0xc582055c7b0f1a24ULL,
        0x9be22d058c2ae082ULL, 0xa36b2344c3b2e0d0ULL, 0x8b74c2c08074d4b1ULL,
        0x2a1c87eb1011bd80ULL, 0x64edcba54a4d0a5aULL
    };
    
    uint8_t** kib512_prep(std::string input)
    {
        uint8_t** matrix = nullptr;
        std::stringstream ss;
        
        // matrix column height
        uint64_t len = input.length();
        uint32_t pad = (((512-((len*8)+1)-128) % 512)-7)/8;
        m_ch = (pad + len + 17)/8;
        
        // declare 8 by multiple of 8 matrix
        matrix = new uint8_t*[8];
        for(int c=0;c<8;c++) matrix[c] = new uint8_t[m_ch];
        
        // add delimeter to denote end of data
        input+=0x90;
        
        // padding
        for(int c=0;c<pad;c++) input+='0';
        
        // add length of input in hex to end of input input
        ss << std::setfill('0') << std::setw(16) << std::hex << len;
        input+=ss.str();
        
        // 1-d array to 2-d matrix
        for(int r=0;r<8;r++) {
            for(uint64_t c=0;c<m_ch;c++) {
                matrix[r][c] = (uint8_t)input[r*m_ch+c]; // input ordered as row
            }
        }
        
        // for(int r=0;r<8;r++) {
        //     for(uint64_t c=0;c<m_ch;c++) {
        //         std::cout << std::hex << matrix[r][c]+0;
        //     }
        // }std::cout << std::endl;
        
        return matrix;
    }
    
    uint64_t ***prec_kib512(uint8_t **matrix)
    {
        // for compression have 
        // initialize manipulation matrix with input matrix
        uint64_t ***manip_m = nullptr;
        manip_m = new uint64_t **[m_ch/8];
        uint64_t loop_count = 0;
        
        // initialize 3-d matrix to zero to indexes that won't be initialized to
        // values of 2-d matrix
        for(int i=0;i<m_ch/8;i++) {
            manip_m[i] = new uint64_t *[8];
            manip_m[i][0] = new uint64_t[8];
            for(int j=1;j<8;j++) { // to avoid unnecesary padding, j=1
                manip_m[i][j] = new uint64_t[8];
                for(int k=0;k<8;k++) manip_m[i][j][k] = 0x0000000000000000ULL;
            }
        }
        
        uint64_t mmi=0; // manipulation matrix i
        int mmj=0;  // manipulation matrix j
        for(int j=0;j<8;j++) {
            for(uint64_t i=0;i<m_ch/8;i++) {
                uint64_t temp=0;
                // add matrix values while avoiding repetition of values
                for(int x=0;x<8;x++) {
                    temp or_eq (uint64_t)matrix[j][x+i*8] << 56-x*8;
                }
                
                // data in matrix has to be little-endian
                if constexpr(std::endian::native == std::endian::big) {
                    manip_m[mmi][0][mmj] = __builtin_bswap64(temp &
                                                             0xffffffffffffffffULL);
                } else {
                    manip_m[mmi][0][mmj] = temp & 0xffffffffffffffffULL;
                }
                mmj = (mmj+1)%8;
            }
            if(mmj%8==0) mmi++;
        }
        
        // pre-compression. Get rid of extra padding
        for(uint64_t i=0;i<m_ch/8;i++) {
            for(int j=1;j<8;j++) {
                for(int k=0;k<8;k++) {
                    // primes used for rotation and shifting
                    // chosen so that one big, one small one rotation is made each time
                    unsigned int p[4] = {37, 3, 59, 5};
                    
                    // matrix indexes are chosen within reason, +7,+6 is for
                    // length of matrix and +1 and +0 is for start of the message.
                    
                    uint64_t tn1,tn2,tn3,tn4 = manip_m[(i+m_ch/8)%m_ch/8][j-1][(k+7)%8];
                    tn3 = manip_m[(i+m_ch/8)%m_ch/8][j-1][(k+6)%8];
                    tn2 = manip_m[i][j-1][(k+1)%8];
                    tn1 = manip_m[i][j-1][k];
                    
                    if(j < 2)
                        std::cout << (k+7)%8 << " " << (k+6)%8 << " " << (k+1)%8
                                  << " " << k%8 << "\n";
                    // TODO: try only using sigma0 and sigma1 for half rotation
                    // and more efficient version if it still seems secure
                    
                    uint64_t sigma0 = rr(tn1, p[0]) xor lr(tn2, p[1]) xor (tn3 << p[2]) |
                                      (tn4 << p[3]) & 0xffffffffffffffffULL;
                    uint64_t sigma1 = rr(tn4, p[3]) xor lr(tn1, p[0]) xor (tn2 << p[1]) |
                                      (tn3 << p[2]) & 0xffffffffffffffffULL;
                    uint64_t sigma2 = rr(tn3, p[2]) xor lr(tn4, p[3]) xor (tn1 << p[0]) |
                                      (tn2 << p[1]) & 0xffffffffffffffffULL;
                    uint64_t sigma3 = rr(tn2, p[1]) xor lr(tn3, p[2]) xor (tn4 << p[3]) |
                                      (tn1 << p[0]) & 0xffffffffffffffffULL;
                    
                    // use arithmetic addition for non-linearity
                    manip_m[i][j][k] = (sigma0 + sigma1 + sigma2 + sigma3)*p[k%4] &
                                       0xffffffffffffffffULL;
                    
                    // the value in after sigma3 is added for better diffusion
                    // and to get rid of repeating values
                }
            }
        }
        return manip_m;
    }
    
    void hash_kib512(uint64_t*** manip_m); // compression function
    
    template<typename T>
    T hashstr(std::string input) {
        // check if type of T is valid, then return hash
        assert((typeid(T) != typeid(std::string) || typeid(T) !=
                typeid(uint64_t*)) && "Kib512 hashstr:\t wrong type detected");
        
        // calculate hash
        uint8_t **m = kib512_prep(input);
        uint64_t ***manipm = prec_kib512(m);
        hash_kib512(manipm);
        
        // return hash as requested data type
        if(typeid(T) == typeid(std::string)) {
            std::stringstream ss;
            for(int i=0;i<8;i++) {
                ss << std::setfill('0') << std::setw(16) << std::hex << hash[i];
            }
            return ss.str();
        }
        else {
            return hash;
        }
    }
};

int main() {
    Kib512 kib512 = Kib512();
    
    // try: 11th prime number(37),
    // try: 1st prime number(3),
    // try: 16th prime number(59)
    // try: try 2nd prime number(5)
    
    std::string in = "abcdefghqwertyuioplkjhgfdsazxcvbnm1234567890!@#$%^&*()\\/";
    // std::string in = "abc";
    uint8_t **m = kib512.kib512_prep(in);
    uint64_t ***manipm = kib512.prec_kib512(m);
    
    for(int i=0;i<kib512.m_ch/8;i++) {
        for(int j=0;j<8;j++) {
            for(int k=0;k<8;k++) {
                std::cout << std::setfill('0') << std::setw(16) << std::hex
                          << manipm[i][j][k] << "\t";
            }
        }
    }
    std::cout << "\n\n" << std::hex << in.length();
    return 0;
}


// CHANGE ENDIANNES TO LITTLE IN PY
