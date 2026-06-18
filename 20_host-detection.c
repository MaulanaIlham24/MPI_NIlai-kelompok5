#include <mpi.h>
#include <stdio.h>
int main(int argc, char** argv) {
    // 1. Inisialisasi lingkungan MPI
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
 // 2. Mendeteksi nama host/komputer tempat proses ini berjalan secara dinamis
    char host_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(host_name, &name_len);
    // 3. Mencetak identitas proses beserta host tempatnya dieksekusi
    printf("Halo! Saya Rank %d dari total %d proses yang berjalan. Saya dieksekusi di HOST: %s\n", 
           rank, size, host_name);
    // 4. Menutup lingkungan kerja MPI
    MPI_Finalize();
    return 0;
}
