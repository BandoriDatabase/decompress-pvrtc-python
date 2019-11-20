#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "Python.h"
#include "etcdec.h"
#include "pvrdecompress.h"

const uint16_t bmpfile_magic = 0x4d42;

size_t bmp_init(FILE *fp, int dim);

static PyObject *get_pixel_from_pvr(PyObject *self, PyObject *args);
static PyObject *get_pixel_from_etc1(PyObject *self, PyObject *args);
static PyObject *get_pixel_from_etc2(PyObject *self, PyObject *args);
static PyMethodDef module_methods[] = {
    {"getPixelFromPvr", get_pixel_from_pvr, METH_VARARGS, "get pixel data from given pvr data"},
    {"getPixelFromETC1", get_pixel_from_etc1, METH_VARARGS, "get pixel data from given etc1 data"},
    {"getPixelFromETC2", get_pixel_from_etc2, METH_VARARGS, "get pixel data from given etc2 data"},
    {NULL, NULL, 0, NULL} 
};

static struct PyModuleDef pvrtcmodule = {
    PyModuleDef_HEAD_INIT,
    "pvrtcdecompress",
    "test module",
    -1,
    module_methods
};

PyMODINIT_FUNC PyInit_pvrtcdecompress(void)
{
    PyObject *m = PyModule_Create(&pvrtcmodule);
    if (m == NULL)
        return NULL;
    return m;
}

static PyObject *get_pixel_from_pvr(PyObject *self, PyObject *args)
{
    
    Py_buffer py_inbuffer;
    int dim;
    
    if(!PyArg_ParseTuple(args, "s*i", &py_inbuffer, &dim))
        return NULL;

    void *inbuffer = py_inbuffer.buf;
    
    // Initialize outfile
    void *outfile = malloc(sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    FILE *foutfile = fmemopen(outfile, sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), "w");
    size_t outsize = bmp_init(foutfile, dim);
    fclose(foutfile);
    void *outbuffer = malloc(outsize * sizeof(char));

    pvrtdecompress(inbuffer, 0, dim, dim, (unsigned char *) outbuffer);

    void *py_outbuffer = malloc(outsize * sizeof(char));
    memcpy(py_outbuffer, outbuffer, outsize * sizeof(char));
    PyObject *result;
    result = Py_BuildValue("y#", (char *)py_outbuffer,outsize * sizeof(char));
    
    PyBuffer_Release(&py_inbuffer);
    free(py_outbuffer);
    free(outbuffer);
    free(outfile);
    
    return result;
}

static PyObject *get_pixel_from_etc1(PyObject *self, PyObject *args)
{
    Py_buffer py_inbuf;
    int width, height, channels;

    if(!PyArg_ParseTuple(args, "s*iii", &py_inbuf, &width, &height, &channels))
        return NULL;

    // check buffer size
    if(py_inbuf.len != width*height/2) return NULL;
    FILE *infile = fmemopen(py_inbuf.buf, py_inbuf.len, "r");
    if(infile == NULL) return NULL;

    int bytesRead = 0, startx = 0, starty = 0;
    uint8 img[width*height*channels];

    while(bytesRead != py_inbuf.len) {
        unsigned int block_part1 = read_big_endian_4byte_word(infile);
        unsigned int block_part2 = read_big_endian_4byte_word(infile);
        bytesRead += 8;

        decompressBlockDiffFlipC(block_part1, block_part2, &img[0], width, height, startx, starty, channels);
        startx += 4;
        if(startx > width) {
            startx = startx % width;
            starty += 4;
        }
    }

    PyObject *py_outbuf = Py_BuildValue("y#", img, width*height*channels);

    PyBuffer_Release(&py_inbuf);

    return py_outbuf;
}

static PyObject *get_pixel_from_etc2(PyObject *self, PyObject *args)
{
    Py_buffer py_inbuf;
    int width, height, channels, alpha_block_flag;

    if(!PyArg_ParseTuple(args, "s*iiip", &py_inbuf, &width, &height, &channels, &alpha_block_flag))
        return NULL;

    // check buffer size
    if(alpha_block_flag) {
        if(py_inbuf.len != width*height) return NULL;
        setupAlphaTable();
    } else {
        if(py_inbuf.len != width*height/2) return NULL;
    }
    FILE *infile = fmemopen(py_inbuf.buf, py_inbuf.len, "r");
    if(infile == NULL) return NULL;

    int bytesRead = 0, startx = 0, starty = 0;
    uint8 *img_color = malloc(width*height*channels);
    uint8 *img_alpha = malloc(width*height);

    while(bytesRead != py_inbuf.len) {
        if(alpha_block_flag) {
            uint8 alpha_data[8];
            fread(&alpha_data[0], 1, 8, infile);
            decompressBlockAlpha(alpha_data, &img_alpha[0], width, height, startx, starty);
            bytesRead += 8;
        }
        unsigned int block_part1 = read_big_endian_4byte_word(infile);
        unsigned int block_part2 = read_big_endian_4byte_word(infile);
        bytesRead += 8;

        decompressBlockETC2(block_part1, block_part2, &img_color[0], width, height, startx, starty);
        startx += 4;
        if(startx > width) {
            startx = startx % width;
            starty += 4;
        }
    }

    uint8 *img;
    if(alpha_block_flag) {
        uint8 *img_data = malloc(width*height*(channels+1));
        img = img_data;
        for(int i = 0; i < width*height; i++) {
            for(int j = 0; j < channels; j++) {
                img_data[i*(channels+1)+j] = img_color[i*channels+j];
            }
            img_data[i*(channels+1)+channels] = img_alpha[i];
        }
        free(img_data);
    } else {
        img = img_color;
    }

    PyObject *py_outbuf = Py_BuildValue("y#", img, width*height*(channels+alpha_block_flag));

    PyBuffer_Release(&py_inbuf);
    free(img_color);
    free(img_alpha);

    return py_outbuf;
}

size_t bmp_init(FILE *fp, int dim)
{
    // Magic
    fwrite(&bmpfile_magic, 2, 1, fp);
    
    // BMP File header
    BITMAPFILEHEADER bf;
    bf.creator1 = 0;
    bf.creator2 = 0;
    bf.bmp_offset = 2 + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    
    // BMP Info header
    BITMAPINFOHEADER bi;
    bi.header_sz = 40;
    bi.width = dim; 
    bi.height = dim;
    bi.nplanes = 1;
    bi.bitspp = 32;
    bi.compress_type = 0;
    bi.bmp_bytesz = bi.width * bi.height * sizeof(RGBQUAD);
    bi.hres = 2835;
    bi.vres = 2835;
    bi.ncolors = 0;
    bi.nimpcolors = 0;    
    
    bf.filesz = bi.bmp_bytesz + bf.bmp_offset;
    
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, fp);
    
    return (size_t) bi.bmp_bytesz;
}