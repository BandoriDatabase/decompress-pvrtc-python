#include <stdlib.h>
#include <stdio.h>
#import <stdint.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "Python.h"
#include "pvrtdecompress.h"

const uint16_t bmpfile_magic = 0x4d42;

size_t get_size(FILE *fp);
size_t bmp_init(FILE *fp, int dim);
void write_noise(FILE *fp, int dim);
void hex_dump(void *buffer, size_t size);

static PyObject *get_bmp_from_pvr(PyObject *self, PyObject *args);
static PyMethodDef module_methods[] = {
    {"getBmpFromPvr", get_bmp_from_pvr, METH_VARARGS, "get bmp file from given pvr file"},
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

// int main(int argc, char const *argv[])
static PyObject *get_bmp_from_pvr(PyObject *self, PyObject *args)
{
    // int do_2bpp = (argv[1] && atoi(argv[1]) == 2) ? 1 : 0;
    // char *infile_name = "img/common_rhythmBG.pvr"; // do_2bpp ? "img/firefox-2bpp.pvr" : "img/firefox-4bpp.pvr";
    // char *outfile_name = "img/common_rhythmBG.bmp"; // do_2bpp ? "img/firefox-2bpp.bmp" : "img/firefox-4bpp.bmp";
    
    Py_buffer py_inbuffer;
    int dim;
    
    if(!PyArg_ParseTuple(args, "s*i", &py_inbuffer, &dim))
        return NULL;
    
    // Initialize infile
    // FILE *infile = fopen(infile_name, "rb");
    // size_t insize = get_size(infile);
    void *inbuffer = py_inbuffer.buf; // malloc(insize * sizeof(char));
    
    // Initialize outfile
    // FILE *outfile = fopen(outfile_name, "wb");
    void *outfile = malloc(sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    FILE *foutfile = fmemopen(outfile, sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), "w");
    size_t outsize = bmp_init(foutfile, dim);
    fclose(foutfile);
    void *outbuffer = malloc(outsize * sizeof(char));
                
    // fread(inbuffer, 1, insize, infile); 

    // printf("inbuffer: %x %x\n\n", inbuffer, inbuffer + insize);
    pvrtdecompress(inbuffer, 0, dim, dim, (unsigned char *) outbuffer);

    // hex_dump(outbuffer, outsize);

    // fwrite(outbuffer, 1, outsize, outfile);
        
    // fclose(infile);
    // fclose(outfile);
    
    // free(inbuffer);
    //void *py_outbuffer = malloc(sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + outsize * sizeof(char));
    void *py_outbuffer = malloc(outsize * sizeof(char));
    //memcpy(py_outbuffer, outfile, sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    //memcpy(py_outbuffer + sizeof(bmpfile_magic) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), outbuffer, outsize * sizeof(char));
    memcpy(py_outbuffer, outbuffer, outsize * sizeof(char));
    PyObject *result;
    result = Py_BuildValue("y#", (char *)py_outbuffer,outsize * sizeof(char));
    
    PyBuffer_Release(&py_inbuffer);
    free(py_outbuffer);
    free(outbuffer);
    free(outfile);
    
    return result;
	// return 0;
}

size_t get_size(FILE* fp)
{
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
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

void write_noise(FILE *fp, int dim)
{
    srand(time(NULL));
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            char r = rand() % 255;
            char g = rand() % 255;
            char b = rand() % 255;
            char a = rand() % 255;
            RGBQUAD pixel = {b, g, r, a};
            fwrite(&pixel, sizeof(RGBQUAD), 1, fp);
        }
    }    
}

void hex_dump(void *buffer, size_t size)
{
    for (int i = 0; i < (int) size; i++) {
        printf("%x ", *((char *) buffer + i));
    }
    printf("\n");   
}
