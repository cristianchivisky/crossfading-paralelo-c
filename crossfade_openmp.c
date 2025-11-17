#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*
   Este programa aplica un efecto "cross-fade" (transición progresiva de color a escala de grises)
   sobre una imagen. Genera 96 frames intermedios entre la imagen original y su versión gris.
   - Está paralelizado con OpenMP para aprovechar varios núcleos del procesador.
   - El tiempo medido excluye la escritura de archivos (I/O a disco).
   - Se mide solo el tiempo de procesamiento (conversión + mezcla).
*/

int main() {
    char input_file[] = "imagen_color_800x800.png";
    int width, height, channels;

    // Carga imagen RGB
    unsigned char *img = stbi_load(input_file, &width, &height, &channels, 3);
    if (img == NULL) {
        printf("Error al cargar la imagen %s\n", input_file);
        return 1;
    }

    printf("Imagen cargada: %dx%d\n", width, height);
    int total_pixels = width * height;

    // Reserva memoria para imagen en gris
    unsigned char *gray = malloc(width * height * 3);
    if (!gray) {
        printf("Error al reservar memoria para imagen gris.\n");
        return 1;
    }

    int num_frames = 96; // cantidad de frames del efecto
    // Reserva memoria para punteros de frames
    unsigned char **frames = malloc(num_frames * sizeof(unsigned char*));
    if (!frames) {
        printf("Error al reservar memoria para frames.\n");
        return 1;
    }

    // inicia medicion de tiempo
    double start = omp_get_wtime();

    // Conversión a escala de grises (paralela)
    // Cada iteración procesa un píxel independiente
    // schedule(static) divide el trabajo equitativamente entre hilos.
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < total_pixels; i++) {
        unsigned char r = img[i*3 + 0];
        unsigned char g = img[i*3 + 1];
        unsigned char b = img[i*3 + 2];
        unsigned char gray_value = (unsigned char)(0.3*r + 0.59*g + 0.11*b);
        gray[i*3 + 0] = gray_value;
        gray[i*3 + 1] = gray_value;
        gray[i*3 + 2] = gray_value;
    }

    // Generación de todos los frames en memoria (paralelo)
    // Se usa schedule(dynamic) para distribuir frames entre hilos dinámicamente
    // (si algunos tardan más, otros hilos toman nuevos frames).
    #pragma omp parallel for schedule(dynamic)
    for (int f = 0; f < num_frames; f++) {
        float P = 1.0 - (float)f / (num_frames - 1);
        unsigned char *result = malloc(width * height * 3);
        if (!result) continue;
        frames[f] = result;

        // Paralelismo dentro de cada frame
        // collapse(2) combina bucles y,x para que OpenMP reparta trabajo más fino
        #pragma omp parallel for collapse(2) schedule(static)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                result[idx]     = (unsigned char)(img[idx]     * P + gray[idx]     * (1 - P));
                result[idx + 1] = (unsigned char)(img[idx + 1] * P + gray[idx + 1] * (1 - P));
                result[idx + 2] = (unsigned char)(img[idx + 2] * P + gray[idx + 2] * (1 - P));
            }
        }
    }

    double end = omp_get_wtime();
    // FIN MEDICIÓN DE TIEMPO

    printf("\nTiempo de procesamiento: %.4f segundos\n", end - start);

    // Escritura de imágenes fuera de la medición
    printf("\nGuardando frames (no medido en el tiempo)...\n");
    #pragma omp parallel for schedule(dynamic)
    for (int f = 0; f < num_frames; f++) {
        if (frames[f]) {
            char filename[100];
            sprintf(filename, "omp_frame_%03d.png", f);
            stbi_write_png(filename, width, height, 3, frames[f], width * 3);
            free(frames[f]);
        }
    }

    // Limpieza
    free(frames);
    stbi_image_free(img);
    free(gray);

    printf("\nCross-fading completo. Se generaron %d frames.\n", num_frames);
    return 0;
}
