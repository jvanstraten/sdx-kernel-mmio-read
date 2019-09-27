
#include <iostream>
#include <fstream>
#include <ostream>
#include <assert.h>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include "xrt.h"
#include "experimental/xrt-next.h"
#include "xclperf.h"
#include "core/common/utils.h"
#include "core/common/sensor.h"
#include "core/edge/include/zynq_ioctl.h"
#include "core/edge/user/zynq_dev.h"
#include "xclbin.h"
#include <fcntl.h>
#include <chrono>
#include <sys/mman.h>

template<class Elem, class Traits>
void hex_dump(
    const void* aData,
    std::size_t aLength,
    std::basic_ostream<Elem, Traits>& aStream,
    std::size_t offset = 0,
    std::size_t aWidth = 16)
{
    const char* const start = static_cast<const char*>(aData);
    const char* const end = start + aLength;
    const char* line = start;
    while (line != end)
    {
        aStream.width(4);
        aStream.fill('0');
        aStream << std::hex << (line - start + offset) << " : ";
        std::size_t lineLength = std::min(aWidth, static_cast<std::size_t>(end - line));
        for (std::size_t pass = 1; pass <= 2; ++pass)
        {
            for (const char* next = line; next != end && next != line + aWidth; ++next)
            {
                char ch = *next;
                switch(pass)
                {
                case 1:
                    aStream << (ch < 32 ? '.' : ch);
                    break;
                case 2:
                    if (next != line)
                        aStream << " ";
                    aStream.width(2);
                    aStream.fill('0');
                    aStream << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(ch));
                    break;
                }
            }
            if (pass == 1 && lineLength != aWidth)
                aStream << std::string(aWidth - lineLength, ' ');
            aStream << " ";
        }
        aStream << std::endl;
        line = line + lineLength;
    }
}

int main(int argc, char *argv[]) {

    std::vector<ip_data> computeUnits;

    std::ifstream is("/sys/bus/pci/devices/0000:86:00.1/ip_layout");
    std::istream_iterator<char> start(is), end;
    std::vector<char> buf(start, end);

    if (buf.empty()) {
        printf("buffer empty!\n");
        return -1;
    }

    const ip_layout *map = (ip_layout *)buf.data();
    if (map->m_count < 0) {
        printf("count invalid!\n");
        return -1;
    }

    for (int i = 0; i < map->m_count; i++) {
        computeUnits.emplace_back(map->m_ip_data[i]);
    }

    int mKernelFD = open("/dev/dri/renderD128", O_RDWR);
    if (!mKernelFD) {
        printf("cannot open /dev/dri/renderD128\n");
        return -1;
    }

    void *ptr = mmap(0, 0x2000000, PROT_READ | PROT_WRITE, MAP_SHARED, mKernelFD, 0);//info.apt_idx*getpagesize());
    if (ptr == (void*)-1) {
        printf("failed to map /dev/dri/renderD128\n");
        return -1;
    }

    for (auto cu : computeUnits) {
        if (cu.m_type != IP_KERNEL) {
            continue;
        }

        printf("Kernel at 0x%016llX:\n", cu.m_base_address);
        hex_dump(((char*)ptr) + cu.m_base_address, 0x200, std::cout, cu.m_base_address);
        printf("\n");
    }

    munmap(ptr, 0x10000);
    close(mKernelFD);

}

