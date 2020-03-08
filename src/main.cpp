#include <iostream>
#include <emscripten.h>
#include <png.h>

struct WriteState {
    png_size_t output_file_size;
    unsigned char* output_file_data;
};

struct WriteState write_state;

// libpng data structures
png_structp png;
png_infop info;

/**
gets called when libpng start writing data to the output
**/
static void png_write_callback(png_structp png, png_bytep data, png_size_t length) {
    struct WriteState *write_state = (struct WriteState *)png_get_io_ptr(png);
    // add a new part of the data to the output buffer
    memcpy(&write_state->output_file_data[write_state->output_file_size], data, length);
    write_state->output_file_size += length;
    //fprintf(stdout, "New output file size: %d\n", write_state->output_file_size);
}

// shim function for the flush handler
static void png_flush_callback(png_structp png) {
   //fprintf(stdout, "Flushing row handler\n");
}

int main() {
    return 0;
}

extern "C" {
    /*
    returns 0 on success

    chunkHeight - the number of rows expected per chunk (can be less for the last chunk)
    output_buffer - a pointer to a memory buffer where the output can be stored

    */
    EMSCRIPTEN_KEEPALIVE int start( int width, int height, int chunkHeight, unsigned char* output_buffer ) {
        int data_size = width * height * 4;

        png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) {
            fprintf(stderr, "Can't create png\n");
            return 2;
        }

        info = png_create_info_struct(png);
        if (!info) {
            fprintf(stderr, "Can't create png info\n");
            return 2;
        }

        if (setjmp(png_jmpbuf(png))) {
            fprintf(stderr, "Can't set png_jmpbuf\n");
            return 2;
        }

        printf("Writing header\n");

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

        // @TODO: initialize the write state data in another way
        write_state = (struct WriteState) {
            .output_file_size = 0,
            .output_file_data = output_buffer,
        };

        png_set_write_fn(png, &write_state, png_write_callback, png_flush_callback);
        png_write_info(png, info);

        // flush to the output buffer every x rows

        //png_set_flush(png, chunkHeight);

        return 0;

        // done! continue with writing chunks
    }

    /**
    add a chunk to the output png
    returns 0 on success
    **/
    EMSCRIPTEN_KEEPALIVE int chunk(int numRows,
                                   int startRow,
                                   void* data) {

        fprintf(stdout, "Processing chunk - startRow: %d, numRows: %d\n", startRow, numRows);

        png_bytep row_pointer;

        int rowbytes = png_get_rowbytes(png, info);
        for (int row = startRow; row < numRows; row++) {
            row_pointer = (unsigned char*)data + row * rowbytes;
            // write a single row
            png_write_rows(png, &row_pointer, 1);
        }

        return 0;

        // when done writing all chunks, call end()
    }

    /**
    finishes off the PNG writing, sets the pointers to the output size and output buffer
    **/
    EMSCRIPTEN_KEEPALIVE int end(// return size of output PNG in bytes to this pointer
                                 int* output_size,
                                 unsigned char** output_data) {
        fprintf(stdout, "Finishing off PNG writing\n");

        // write the ending chunks of the PNG
        png_write_end(png, NULL);

        fprintf(stdout, "Writing finished!\n");

        // copy the data back to the output buffer
        //memcpy(data, write_state.output_file_data, write_state.output_file_size);
        //free(write_state.output_file_data);
        *output_size = write_state.output_file_size;
        *output_data = write_state.output_file_data;

        return 0;
    }

    EMSCRIPTEN_KEEPALIVE int clean() {
        fprintf(stdout, "Freeing data");
        free(write_state.output_file_data);

        // @TODO: also free png struct data

        return 0;
    }
}
