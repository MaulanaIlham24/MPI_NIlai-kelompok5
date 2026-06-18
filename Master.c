#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_LINE 256
#define MAX_DATA 1000

// Struktur untuk menyimpan data nilai
typedef struct {
    char nim[20];
    float nilai;
} DataNilai;

// Struktur untuk hasil perhitungan lokal
typedef struct {
    float max;
    float min;
    float sum;
    int count;
} HasilLokal;

int main(int argc, char *argv[]) {
    int rank, size;
    DataNilai *all_data = NULL;
    DataNilai *local_data = NULL;
    int total_data = 0;
    HasilLokal local_result, global_result;

    // Inisialisasi MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Pastikan jumlah proses adalah 5 (1 master + 4 workers)
    if (size != 5) {
        if (rank == 0) {
            printf("Error: Program harus dijalankan dengan 5 proses (1 Master + 4 Workers)\n");
            printf("Gunakan: mpirun -np 5 ./mpi_nilai\n");
        }
        MPI_Finalize();
        return 1;
    }

    // ==================== MASTER NODE (Rank 0) ====================
    if (rank == 0) {
        FILE *file = fopen("data_nilai.csv", "r");
        if (file == NULL) {
            printf("Error: Tidak dapat membuka file data_nilai.csv\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Alokasi memori untuk semua data
        all_data = (DataNilai *)malloc(MAX_DATA * sizeof(DataNilai));

        // Baca file CSV (lewati header)
        char line[MAX_LINE];
        fgets(line, sizeof(line), file); // Skip header

        total_data = 0;
        while (fgets(line, sizeof(line), file) && total_data < MAX_DATA) {
            // Parse CSV: NIM,Nilai
            char *token = strtok(line, ",\n");
            if (token != NULL) {
                strcpy(all_data[total_data].nim, token);
                token = strtok(NULL, ",\n");
                if (token != NULL) {
                    all_data[total_data].nilai = atof(token);
                    total_data++;
                }
            }
        }
        fclose(file);

        printf("Master: Berhasil membaca %d data dari file CSV\n", total_data);

        // Hitung distribusi data ke setiap worker (4 workers)
        int num_workers = size - 1;
        int base_count = total_data / num_workers;
        int remainder = total_data % num_workers;

        int offset = 0;
        for (int i = 1; i < size; i++) {
            int send_count = base_count;
            if (i <= remainder) {
                send_count++;
            }

            // Kirim jumlah data ke worker
            MPI_Send(&send_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            // Kirim data ke worker
            MPI_Send(&all_data[offset], send_count * sizeof(DataNilai),
                     MPI_BYTE, i, 2, MPI_COMM_WORLD);

            offset += send_count;
        }

        // Terima hasil dari semua workers
        global_result.max = -1.0;
        global_result.min = 101.0;
        global_result.sum = 0.0;
        global_result.count = 0;

        for (int i = 1; i < size; i++) {
            MPI_Recv(&local_result, sizeof(HasilLokal), MPI_BYTE, i, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (local_result.max > global_result.max) {
                global_result.max = local_result.max;
            }
            if (local_result.min < global_result.min) {
                global_result.min = local_result.min;
            }
            global_result.sum += local_result.sum;
            global_result.count += local_result.count;
        }

        // Hitung rata-rata global
        float average = global_result.sum / global_result.count;

        // Tampilkan hasil akhir
        printf("\n===========================================\n");
        printf("   HASIL ANALISIS NILAI MAHASISWA (MPI)\n");
        printf("===========================================\n");
        printf("Total Data Diproses : %d baris\n", global_result.count);
        printf("Nilai Tertinggi     : %.2f\n", global_result.max);
        printf("Nilai Terendah      : %.2f\n", global_result.min);
        printf("Rata-rata Kelas     : %.2f\n", average);
        printf("===========================================\n");

        // Bersihkan memori
        free(all_data);
    }
    // ==================== WORKER NODES (Rank 1-4) ====================
    else {
        int local_count;

        // Terima jumlah data dari master
        MPI_Recv(&local_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Alokasi memori untuk data lokal
        local_data = (DataNilai *)malloc(local_count * sizeof(DataNilai));

        // Terima data dari master
        MPI_Recv(local_data, local_count * sizeof(DataNilai), MPI_BYTE, 0, 2,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Hitung hasil lokal
        local_result.max = local_data[0].nilai;
        local_result.min = local_data[0].nilai;
        local_result.sum = 0.0;
        local_result.count = local_count;

        for (int i = 0; i < local_count; i++) {
            if (local_data[i].nilai > local_result.max) {
                local_result.max = local_data[i].nilai;
            }
            if (local_data[i].nilai < local_result.min) {
                local_result.min = local_data[i].nilai;
            }
            local_result.sum += local_data[i].nilai;
        }

        printf("Worker %d: Memproses %d data | Max: %.2f, Min: %.2f, Sum: %.2f\n",
               rank, local_count, local_result.max, local_result.min, local_result.sum);

        // Kirim hasil ke master
        MPI_Send(&local_result, sizeof(HasilLokal), MPI_BYTE, 0, 1, MPI_COMM_WORLD);

        // Bersihkan memori
        free(local_data);
    }

    // Finalisasi MPI
    MPI_Finalize();
    return 0;
}
