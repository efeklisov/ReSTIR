#include <saveimg.hpp>

namespace hd {
    void saveImg(hd::Image img, hd::Device device, hd::Allocator alloc, std::string method, uint32_t N, uint32_t tolerance, uint32_t frame) {
        size_t delimPos = 0;
        while((delimPos = method.find('/')) != std::string::npos)
            method[delimPos] = '-';

        std::stringstream str;
        str << "screenshot_" << method << '_' << tolerance << '-' << frame << '_' << N << ".ppm" << std::endl;

        vk::ImageSubresource subResource{ vk::ImageAspectFlagBits::eColor, 0, 0 };
        vk::SubresourceLayout subResourceLayout = device->raw().getImageSubresourceLayout(img->raw(), subResource);

        void* data;
        alloc->map(img->memory(), data);
        data = (char*)data + subResourceLayout.offset;

        std::ofstream file(str.str(), std::ios::out | std::ios::binary);

        file << "P6\n" << img->extent().width << "\n" << img->extent().height << "\n" << 255 << "\n";

		for (uint32_t y = 0; y < img->extent().height; y++)
		{
			unsigned int *row = (unsigned int*)data;
			for (uint32_t x = 0; x < img->extent().width; x++)
			{
                file.write((char*)row+2, 1);
                file.write((char*)row+1, 1);
                file.write((char*)row, 1);
				row++;
			}
			data = (char*)data + subResourceLayout.rowPitch;
		}
		file.close();

        alloc->unmap(img->memory());
    }
};
