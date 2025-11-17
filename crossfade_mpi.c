#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/*
   Este programa aplica un efecto "cross-fade" (transición gradual de color a gris)
   sobre una imagen, pero usando **procesamiento distribuido con MPI**.
   - Cada proceso (nodo) recibe una parte de la imagen (filas).
   - Cada uno convierte su parte a escala de grises y genera sus propios frames.
   - El proceso 0 (maestro) reúne los fragmentos de cada proceso
     y escribe los archivos PNG en disco.
   - Solo se mide el tiempo de procesamiento (sin escritura a disco).
*/


int main(int argc, char *argv[]) {
    int rank, size;
    double start, end;

    // Inicializa el entorno MPI
    MPI_Init(&argc, &argv);
    // rank = ID del proceso (0 es el maestro)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // size = número total de procesos ejecutándose
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width, height, channels;
    unsigned char *img = NULL;
    int num_frames = 96;

    // Carga imagen solo en el proceso 0 (maestro)
    if (rank == 0) {
        img = stbi_load("imagen_color_800x800.png", &width, &height, &channels, 3);
        if (!img) {
            printf("Error al cargar la imagen.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Imagen cargada: %dx%d\n", width, height);
    }

    // Difundir dimensiones de la imagen
    // Enviamos width, height y channels a todos los procesos
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular qué filas maneja cada proceso
    int rows_per_proc = height / size;     // división base
    int extra = height % size;             // filas restantes (si no divide exacto)
    // Fila inicial que le toca a este proceso
    int start_row = rank * rows_per_proc + (rank < extra ? rank : extra);
    // Número de filas que maneja este proceso
    int my_rows = rows_per_proc + (rank < extra ? 1 : 0);
    // Total de bytes de la porción de imagen local (RGB)
    int my_pixels = my_rows * width * 3;
    
    // Buffer local para la parte de imagen que recibirá
    unsigned char *local_img = malloc(my_pixels);

    // Preparar estructuras para repartir la imagen
    int *sendcounts = malloc(size * sizeof(int));  // bytes por proceso
    int *displs = malloc(size * sizeof(int));      // desplazamiento en la imagen global

    // Solo el proceso 0 calcula los tamaños reales
    if (rank == 0) {
        int offset = 0;
        for (int p = 0; p < size; p++) {
            int n_rows = rows_per_proc + (p < extra ? 1 : 0);
            sendcounts[p] = n_rows * width * 3;
            displs[p] = offset;
            offset += sendcounts[p];
        }
    }

    // Broadcast de sendcounts y displs a todos (evita NULL o valores inválidos)
    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

    // Distribuir partes de la imagen (Scatterv)
    // Cada proceso recibe su parte en local_img
    MPI_Scatterv(img, sendcounts, displs, MPI_UNSIGNED_CHAR,
                 local_img, my_pixels, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Crear buffers locales para gris y frames
    unsigned char *local_gray = malloc(my_pixels);
    unsigned char **local_frames = malloc(num_frames * sizeof(unsigned char*));

    // Sincronización global y inicio del cronómetro
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    // Conversión local a escala de grises
    for (int i = 0; i < my_rows * width; i++) {
        unsigned char r = local_img[i*3 + 0];
        unsigned char g = local_img[i*3 + 1];
        unsigned char b = local_img[i*3 + 2];
        unsigned char gray_value = (unsigned char)(0.3*r + 0.59*g + 0.11*b);
        local_gray[i*3 + 0] = gray_value;
        local_gray[i*3 + 1] = gray_value;
        local_gray[i*3 + 2] = gray_value;
    }

    // Generar frames localmente  (color → gris)
    for (int f = 0; f < num_frames; f++) {
        float P = 1.0f - (float)f / (num_frames - 1);
        local_frames[f] = malloc(my_pixels);
        unsigned char *frame = local_frames[f];
        for (int i = 0; i < my_rows * width * 3; i++) {
            frame[i] = (unsigned char)(local_img[i] * P + local_gray[i] * (1 - P));
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    if (rank == 0)
        printf("\nTiempo de procesamiento: %.4f segundos\n", end - start);

    // Escritura fuera de la medición
    if (rank == 0)
        printf("\nGuardando frames (no medido en el tiempo)...\n");

    for (int f = 0; f < num_frames; f++) {
        unsigned char *final_frame = NULL;
        if (rank == 0)
            final_frame = malloc(width * height * 3);

        // Recolectar todas las partes
        MPI_Gatherv(local_frames[f], my_pixels, MPI_UNSIGNED_CHAR,
                    final_frame, sendcounts, displs, MPI_UNSIGNED_CHAR,
                    0, MPI_COMM_WORLD);
        
        // Solo el maestro escribe la imagen en disco
        if (rank == 0) {
            char filename[100];
            sprintf(filename, "mpi_frame_%03d.png", f);
            stbi_write_png(filename, width, height, 3, final_frame, width * 3);
            free(final_frame);
        }
        free(local_frames[f]);
    }
    if (rank == 0)
        printf("\nCross-fading completo. Se generaron %d frames.\n", num_frames);


    // Cleanup
    free(local_img);
    free(local_gray);
    free(local_frames);
    free(sendcounts);
    free(displs);
    if (rank == 0)
        stbi_image_free(img);

    MPI_Finalize();
    return 0;
}
