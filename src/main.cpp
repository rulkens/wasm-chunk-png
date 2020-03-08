#include <iostream>
#include <emscripten.h>
#include <png.h>

struct WriteState {
    png_size_t output_file_size;
    unsigned char* output_file_data;
};

static void png_write_callback(png_structp png, png_bytep data, png_size_t length) {
    struct WriteState *write_state = (struct WriteState *)png_get_io_ptr(png);
    memcpy(&write_state->output_file_data[write_state->output_file_size], data, length);
    write_state->output_file_size += length;
}

// shim function for the flush handler
static void png_flush_callback(png_structp png) {
   fprintf(stdout, "Flushing row handler");
}

int main() {
    return 0;
}

extern "C" {
    int compress(
        int width,
        int height,
        void* data,
        int* output_size
    ) {
        *output_size = 0;
        int data_size = width * height * 4;

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) {
            fprintf(stderr, "Can't create png\n");
            return 2;
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            fprintf(stderr, "Can't create png info\n");
            return 2;
        }

        if (setjmp(png_jmpbuf(png))) {
            fprintf(stderr, "Can't set png_jmpbuf\n");
            return 2;
        }

        png_set_IHDR(
            png,
            info,
            width, height,
            8,
            PNG_COLOR_TYPE_RGB_ALPHA,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT
        );
        png_set_filter(png, PNG_FILTER_TYPE_BASE, PNG_FILTER_VALUE_NONE);

        struct WriteState write_state;
        write_state = (struct WriteState) {
            .output_file_data = new unsigned char[data_size],
            .output_file_size = 0
        };

        png_set_write_fn(png, &write_state, png_write_callback, png_flush_callback);
        png_write_info(png, info);

        // flush to the output buffer every 10 rows
        // @TODO: calculate this based on the row size and the maximum memory size
        png_set_flush(png, 10);

//        png_bytep row_pointers[height];
        png_bytep row_pointer;

        int rowbytes = png_get_rowbytes(png, info);
        for (int row = 0; row < height; row++) {
            row_pointer = (unsigned char*)data + row * rowbytes;
            // write a single row
            png_write_rows(png, &row_pointer, 1);
        }

        png_write_end(png, NULL);

        memcpy(data, write_state.output_file_data, write_state.output_file_size);
        free(write_state.output_file_data);
        *output_size = write_state.output_file_size;

        return 0;
    }
}
