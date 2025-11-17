#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>            // Solo usamos omp_get_wtime() para medir tiempo
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*
  Programa: Cross-fading secuencial con medición excluyendo la escritura a disco.
  - Carga una imagen RGB.
  - Calcula su versión en escala de grises.
  - Genera en memoria N frames que interpolan color -> gris.
  - Mide solo el tiempo de procesamiento (gris + generación de frames).
  - Luego escribe los PNG (fuera de la medición).
*/

int main() {
    double start, end;
    char input_file[] = "imagen_color_800x800.png";
    int width, height, channels;

    // 1) Cargar imagen RGB (forzamos 3 canales en stbi_load)
    unsigned char *img = stbi_load(input_file, &width, &height, &channels, 3);
    if (img == NULL) {
        printf("Error al cargar la imagen %s\n", input_file);
        return 1;
    }

    printf("Imagen cargada: %dx%d\n", width, height);

    // 2) Reservar buffer para la versión en escala de grises (RGB por pixel: 3 bytes)
    //    Aunque gris tiene el mismo valor en R,G,B, lo guardamos como RGB para simplificar.
    unsigned char *gray = malloc(width * height * 3);

    start = omp_get_wtime();

    // Conversión a gris (por píxel)
    for (int i = 0; i < width * height; i++) {
        unsigned char r = img[i*3 + 0];
        unsigned char g = img[i*3 + 1];
        unsigned char b = img[i*3 + 2];
        unsigned char gray_value = (unsigned char)(0.3*r + 0.59*g + 0.11*b);
        gray[i*3 + 0] = gray_value;
        gray[i*3 + 1] = gray_value;
        gray[i*3 + 2] = gray_value;
    }

    int num_frames = 96;

    // Generar frames en memoria (sin escribir)
    unsigned char **frames = malloc(num_frames * sizeof(unsigned char*));

    // Generar todos los frames en memoria (aquí está la parte medida)
    //    - P varía linealmente de 1.0 a 0.0: P=1 => todo color, P=0 => todo gris
    for (int f = 0; f < num_frames; f++) {
        float P = 1.0 - (float)f / (num_frames - 1);
        frames[f] = malloc(width * height * 3);

        // Mezcla por píxel (guardamos 3 componentes por píxel)
        for (int i = 0; i < width * height; i++) {
            frames[f][i*3 + 0] = (unsigned char)(img[i*3 + 0] * P + gray[i*3 + 0] * (1 - P));
            frames[f][i*3 + 1] = (unsigned char)(img[i*3 + 1] * P + gray[i*3 + 1] * (1 - P));
            frames[f][i*3 + 2] = (unsigned char)(img[i*3 + 2] * P + gray[i*3 + 2] * (1 - P));
        }
    }

    end = omp_get_wtime();
    printf("\nTiempo de procesamiento (sin I/O): %.4f segundos\n", end - start);
    // ------------------------

    // Escritura de frames en disco (fuera de la medición)
    printf("\nGuardando frames (no medido en el tiempo)...\n");
    for (int f = 0; f < num_frames; f++) {
        char filename[100];
        sprintf(filename, "frame_%03d.png", f);
        stbi_write_png(filename, width, height, 3, frames[f], width * 3);
        free(frames[f]);
    }
    free(frames);

    stbi_image_free(img);
    free(gray);

    printf("Cross-fading completo. Se generaron %d frames.\n", num_frames);
    return 0;
}
