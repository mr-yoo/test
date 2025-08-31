#include <png.h>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>

bool read_png(const char* filename, std::vector<unsigned char>& image,
              unsigned& width, unsigned& height)
{
    FILE* fp = fopen(filename, "rb");
    if(!fp) {
        std::cerr << "Failed to open " << filename << "\n";
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if(!png) return false;
    png_infop info = png_create_info_struct(png);
    if(!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return false;
    }
    if(setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if(bit_depth == 16) png_set_strip_16(png);
    if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if(png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(png, info);
    png_size_t rowbytes = png_get_rowbytes(png, info);
    image.resize(rowbytes * height);
    std::vector<png_bytep> rows(height);
    for(size_t y = 0; y < height; ++y) {
        rows[y] = image.data() + y * rowbytes;
    }
    png_read_image(png, rows.data());

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
    return true;
}

bool write_png(const char* filename, const std::vector<unsigned char>& image,
               unsigned width, unsigned height)
{
    FILE* fp = fopen(filename, "wb");
    if(!fp) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return false;
    }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if(!png) return false;
    png_infop info = png_create_info_struct(png);
    if(!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return false;
    }
    if(setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }
    png_init_io(png, fp);

    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_size_t rowbytes = width * 4;
    std::vector<png_bytep> rows(height);
    for(size_t y = 0; y < height; ++y) {
        rows[y] = const_cast<unsigned char*>(image.data() + y * rowbytes);
    }
    png_write_image(png, rows.data());
    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return true;
}

std::vector<unsigned char> horizontal_lowpass(const std::vector<unsigned char>& src,
                                              unsigned width, unsigned height)
{
    const int channels = 4;
    std::vector<unsigned char> dst(src.size());
    for(unsigned y = 0; y < height; ++y) {
        for(unsigned x = 0; x < width; ++x) {
            for(int c = 0; c < channels; ++c) {
                auto sample = [&](int xx) {
                    int nx = std::clamp(xx, 0, (int)width - 1);
                    return src[(y * width + nx) * channels + c];
                };
                int sum = sample(x - 1) + 2 * sample(x) + sample(x + 1);
                dst[(y * width + x) * channels + c] = static_cast<unsigned char>(sum / 4);
            }
        }
    }
    return dst;
}

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.png> <output.png>\n";
        return 1;
    }
    std::vector<unsigned char> image;
    unsigned width, height;
    if(!read_png(argv[1], image, width, height)) {
        std::cerr << "Could not read input image\n";
        return 1;
    }
    auto filtered = horizontal_lowpass(image, width, height);
    if(!write_png(argv[2], filtered, width, height)) {
        std::cerr << "Could not write output image\n";
        return 1;
    }
    return 0;
}
